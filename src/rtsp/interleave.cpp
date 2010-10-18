/*************************************************************************
 *
 * Kinoglaz Streaming Server Daemon
 * Copyright (C) 2010 Emiliano Leporati ( emiliano.leporati@gmail.com )
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 *************************************************************************
 *
 * File name: ./rtsp/interleave.cpp
 * First submitted: 2010-02-07
 * First submitter: Emiliano Leporati <emiliano.leporati@gmail.com>
 * Contributor(s) so far - 2010-07-30 :
 *     Emiliano Leporati <emiliano.leporati@gmail.com>
 *
 * Last changes :
 *     interleaved channels shutdown fixed
 *     Kinoglaz birth: pervasive bug fixing, improvements and client support
 *     fixed some bug in Log and in insertMedia; comments
 *     log messages refactor; shared descriptor are optional now; spot insertion support
 *     ns refactor over messages; seek exception blobbed up
 *
 **/


#include "rtsp/interleave.h"
#include "lib/array.hpp"
#include "daemon.h"

namespace KGD
{
	namespace RTSP
	{
		Interleave::Interleave( TPort local,
								TPort remote,
								TSessionID ssid,
								TcpTunnel & s,
								Socket & sock )
		: _channel( local )
		, _remote( remote )
		, _ssid( ssid )
		, _sock( s )
		, _rtspSocket( sock )
		, _running( true )
		, _logName( sock.getLogName() + string( " CHANNEL $" ) + toString( local ) )
		{
			Log::debug("%s: created", getLogName() );
		}

		Interleave::~Interleave( )
		{
			Log::debug("%s: shutting down", getLogName() );
			this->stop();
			Log::debug("%s: stopped", getLogName() );
			_rtspSocket.release( _channel );
			Log::debug("%s: released", getLogName() );
			{
				Lock lk( _recvMux );
				_recv.clear();
			}
			Log::debug("%s: cleared", getLogName() );
		}

		TSessionID Interleave::getSessionID() const throw()
		{
			return _ssid;
		}

		const char * Interleave::getLogName() const throw()
		{
			return _logName.c_str();
		}

		void Interleave::pushToRead( const ByteArray & data ) throw( )
		{
			this->pushToRead( data.get(), data.size() );
		}

		void Interleave::pushToRead( void const * data, size_t sz ) throw( )
		{
			_recvMux.lock();
			_recv.push_back( new ByteArray( data, sz ) );
			_recvMux.unlock();

			_condNotEmpty.notify_all();
		}
		void Interleave::stop() throw()
		{
			Log::debug("%s: stopping", getLogName() );

			_recvMux.lock();
			if ( _running )
			{
				Log::debug( "%s: notify readers", getLogName() );
				_running = false;
				_recvMux.unlock();
				_condNotEmpty.notify_all();
			}
			else
			{
				Log::debug( "%s: reader queue not running", getLogName() );
				_recvMux.unlock();
			}
		}

		size_t Interleave::writeSome( void const * data, size_t sz ) throw( KGD::Socket::Exception )
		{
			ByteArray envelope( 4 + sz );
			envelope[0] = '$';
			envelope[1] = _remote;
			envelope
				.set( htons( sz ), 2 )
				.set( data, sz, 4 );

			{
				TcpTunnel::LockerType lk( _sock );
				// not counting header bytes
				return (*_sock)->writeAll( envelope ) - 4;
			}
		}

		size_t Interleave::writeLast( void const * data, size_t sz ) throw( KGD::Socket::Exception )
		{
			TcpTunnel::LockerType lk( _sock );
			(*_sock)->setLastPacket( true );
			return this->writeSome( data, sz );
		}

		size_t Interleave::readSome( void * data, size_t sz ) throw( KGD::Socket::Exception )
		{
			Lock lk( _recvMux );
			for(;;)
			{
				if ( !_running )
				{
					Log::debug( "%s: socket has stopped", getLogName() );
					throw KGD::Socket::Exception( "readSome", "connection shut down" );
				}
				else if ( ! _recv.empty() )
				{
					Log::debug( "%s: socket has data", getLogName() );
					ByteArray & buf = _recv.front();
					size_t rt = buf.copyTo( data, sz );
					if ( rt >= buf.size() )
						_recv.pop_front();
					else
						buf.chopFront( rt );
					return rt;
				}
				else
				{
					Log::debug( "%s: waiting interleaved data", getLogName() );
					_condNotEmpty.wait( lk );
				}
			}
		}

		Channel::Description Interleave::getDescription() const
		{
			Channel::Description rt;
			rt.type = Channel::Shared;
			rt.ports = make_pair( _channel, _channel );

			return rt;
		}

		// ******************************************************************************************************************

		Socket::Socket( KGD::Socket::Tcp * sk, const string & parentLogName )
		: _cseq( 0 )
		, _logName( parentLogName + " SOCKET" )
		{
			(*_sock).reset( sk );
		}

		Socket::~Socket()
		{
			Log::debug("%s: socket shutting down", getLogName());
			this->stopInterleaving();
			{
				// wait termination
				Lock lk( _muxPorts );
				if ( ! _iChans.empty() )
					_condNoInterleave.wait( lk );
			}
			Log::debug("%s: released and closed", getLogName());
		}

		void Socket::stopInterleaving() throw()
		{
			Lock lk( _muxPorts );
			// stop all interleaved channels
			BOOST_FOREACH( ChannelMap::iterator::reference ch, _iChans )
				ch.second->stop();
		}

		void Socket::stopInterleaving( const TSessionID & ssid ) throw()
		{
			Lock lk( _muxPorts );
			BOOST_FOREACH( ChannelMap::iterator::reference ch, _iChans )
				// more interleaves can be on a single session
				if ( ch.second->getSessionID() == ssid )
					ch.second->stop();					
		}

		const char * Socket::getLogName() const throw()
		{
			return _logName.c_str();
		}

		Channel::Description Socket::getDescription() const
		{
			return (*_sock)->getDescription();
		}

		void Socket::release( TPort p )
		{
			bool empty = false;
			{
				Lock lk( _muxPorts );

				Log::debug( "%s: releasing interleaved channel %", getLogName(), p );
				if ( _iChans.find( p ) != _iChans.end() )
				{
					_iChans.erase( p );
					_iPorts.release( p );
				}
				else
					Log::debug( "%s: %d interleaved not found", getLogName(), p );

				Log::debug( "%s: %d interleaved channels remaining", getLogName(), _iChans.size() );

				empty = _iChans.empty();
			}

			if ( empty )
				_condNoInterleave.notify_all();
		}

		TPort Socket::addInterleave( TPort remote, const TSessionID & ssid ) throw( KGD::Exception::NotFound )
		{
			Lock lk(_muxPorts);

			TPort local = _iPorts.getOne();
			Interleave * intlv = new Interleave( local, remote, ssid, _sock, *this );
			ref< Interleave > ref( *intlv );

			_iChans.insert( make_pair( local, ref ) );

			Log::debug( "%s: added interleave %d", getLogName(), local );

			return local;
		}

		TPortPair Socket::addInterleavePair( const TPortPair & remote, const TSessionID & ssid ) throw( KGD::Exception::NotFound )
		{
			Lock lk(_muxPorts);

			TPortPair local = _iPorts.getPair();
			Interleave * a = new Interleave( local.first, remote.first, ssid, _sock, *this );
			Interleave * b = new Interleave( local.second, remote.second, ssid, _sock, *this );
			ref< Interleave > aRef( *a ), bRef( *b );

			_iChans.insert( make_pair( local.first, aRef ) );
			_iChans.insert( make_pair( local.second, bRef ) );

			Log::debug( "%s: added interleave %d - %d", getLogName(), local.first, local.second );

			return local;
		}

		boost::shared_ptr< Interleave > Socket::getInterleave( TPort p ) throw( KGD::Exception::NotFound )
		{
			Lock lk(_muxPorts);
			ChannelMap::iterator it = _iChans.find( p );
			if ( it == _iChans.end( ) )
				throw KGD::Exception::NotFound("interleaved channel " + toString( p ));
			else
				return boost::shared_ptr< Interleave >( it->second.getPtr() );
		}

		Interleave & Socket::getInterleaveRef( TPort p ) throw( KGD::Exception::NotFound )
		{
			Lock lk(_muxPorts);
			ChannelMap::iterator it = _iChans.find( p );
			if ( it == _iChans.end( ) )
				throw KGD::Exception::NotFound("interleaved channel " + toString( p ));
			else
				return *it->second;
		}

		void Socket::listen() throw( Message::Request *, Message::Response *, KGD::Exception::Generic )
		{
			ByteArray receiveBuffer( 1024 );
			for(;;)
			{
				// read bytes from underlying socket
				size_t receivedBytes = (*_sock)->Channel::In::readSome( receiveBuffer );

				// put in parser buffer
				_inBuf.enqueue( receiveBuffer.get(), receivedBytes );
				// get next packet length
				size_t msgSz = _inBuf.getNextPacketLength();
				if ( msgSz )
				{
					// try if a request
					try
					{
						auto_ptr< Message::Request > rq( _inBuf.getNextRequest( msgSz, (*_sock)->getRemoteHost() ) );
						_cseq = rq->getCseq();

						Log::message( "%s: request: %s", getLogName(), Method::name[ rq->getMethodID() ].c_str() );
						Log::request( rq->get() );

						throw rq.release();
					}
					catch( KGD::Exception::NotFound )
					{
						// try if a response
						try
						{
							auto_ptr< Message::Response > resp = _inBuf.getNextResponse( msgSz );
							if ( resp->getCseq() != _cseq )
								throw RTSP::Exception::CSeq();

							throw resp.release();
						}
						catch( KGD::Exception::NotFound )
						{
							// try interleaved
							try
							{
								pair< TPort, boost::shared_ptr< RTP::Packet > > intlv( _inBuf.getNextInterleave( msgSz ) );
								this->getInterleaveRef( intlv.first ).pushToRead( intlv.second->data );

							}
							catch( KGD::Exception::NotFound )
							{
								// deque
								Log::error( "%s: invalid interleaved channel, invalid inbound packet, deque %lu", getLogName(), msgSz );
								_inBuf.dequeue( msgSz );
							}
						}
					}
					catch ( const RTSP::Exception::ManagedError &e)
					{
						Log::error( "%s: %s", getLogName(), e.what() );
						this->reply( e.getError() );
					}
					catch ( const KGD::Socket::Exception & )
					{
						throw;
					}
					catch ( const KGD::Exception::Generic &e)
					{
						Log::error( "%s: %s", getLogName(), e.what() );
					}

				}
			}
		}


		string Socket::getLocalHost() const
		{
			return (*_sock)->getLocalHost();
		}

		string Socket::getRemoteHost() const
		{
			return (*_sock)->getRemoteHost();
		}

		size_t Socket::writeSome( void const * data, size_t sz ) throw( KGD::Socket::Exception )
		{
			TcpTunnel::LockerType lk( _sock );
			return (*_sock)->writeAll( data, sz );
		}

		size_t Socket::writeLast( void const * data, size_t sz ) throw( KGD::Socket::Exception )
		{
			TcpTunnel::LockerType lk( _sock );
			(*_sock)->setLastPacket( true );
			return this->writeSome( data, sz );
		}

		void Socket::reply( const uint16_t code, const string & msg ) throw( KGD::Socket::Exception )
		{
			ostringstream s;
			s << VER << " " << code << " " << msg << EOL
				<< "CSeq: " << _cseq << EOL << EOL;

			string str = s.str();
			Log::reply( str );
			this->writeSome( str.c_str(), str.size() );
		}

		void Socket::reply(const Error::Definition & error) throw( KGD::Socket::Exception )
		{
			this->reply( error.getCode(), error.getDescription() );
		}

		void Socket::reply( Method::Base & m ) throw( KGD::Socket::Exception )
		{
			try
			{
				// prapare data
				m.prepare();

				// prepend common header do reply
				ostringstream s;
				s << VER << " " << Error::Ok.getCode() << " " << Error::Ok.getDescription() << EOL
					<< "CSeq: " << _cseq << EOL
					<< "Server: " << KGD::Daemon::getName() << EOL
					<< m.getReply();

				string str = s.str();
				Log::reply( str );
				this->writeSome( str.c_str(), str.size() );

				// do required actions
				m.execute();
			}
			catch( const Exception::ManagedError & e )
			{
				this->reply( e.getError() );
			}
		}

	}

}
