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
 * File name: src/rtsp/interleave.cpp
 * First submitted: 2010-07-30
 * First submitter: Emiliano Leporati <emiliano.leporati@gmail.com>
 * Contributor(s) so far - 2010-11-04 :
 *     Emiliano Leporati <emiliano.leporati@gmail.com>
 *
 * Last changes :
 *     fixed RTSP buffer enqueue
 *     "would block" cleanup
 *     added param for tcp socket send buffer
 *     english comments; removed leak with connection serving threads
 *     removed magic numbers in favor of constants / ini parameters
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
		, _rdTimeout( -1 )
		, _running( true )
		, _logName( sock.getLogName() + string( " CHANNEL $" ) + toString( local ) )
		{
			Log::verbose("%s: created", getLogName() );
		}

		Interleave::~Interleave( )
		{
			Log::debug("%s: shutting down", getLogName() );
			this->close();
			Log::debug("%s: stopped", getLogName() );
			_rtspSocket.release( _channel );
			Log::debug("%s: released", getLogName() );
			{
				InputBuffer::Lock lk( _recv );
				(*_recv).clear();
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
			Log::verbose( "%s: pushing %u bytes to read", getLogName(), sz );
			{
				InputBuffer::Lock lk( _recv );
				(*_recv).push_back( new ByteArray( data, sz ) );
			}

			_condNotEmpty.notify_all();
		}
		void Interleave::close() throw()
		{
			Log::debug("%s: stopping", getLogName() );

			_recv.lock();
			if ( _running )
			{
				Log::verbose( "%s: notify readers", getLogName() );
				_running = false;
				_recv.unlock();
				_condNotEmpty.notify_all();
			}
			else
			{
				Log::verbose( "%s: channel already stopped", getLogName() );
				_recv.unlock();
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
				TcpTunnel::Lock lk( _sock );
				if ( _running )
				{
					// not counting header bytes
					return (*_sock)->writeSome( envelope.get(), envelope.size() ) - 4;
				}
				else
					throw KGD::Socket::Exception( "writeSome", "connection shut down" );
			}
		}

		size_t Interleave::writeLast( void const * data, size_t sz ) throw( KGD::Socket::Exception )
		{
			TcpTunnel::Lock lk( _sock );
			if ( _running )
			{
				(*_sock)->setLastPacket( true );
				return this->writeSome( data, sz );
			}
			else
				throw KGD::Socket::Exception( "writeLast", "connection shut down" );
		}

		size_t Interleave::readSome( void * data, size_t sz ) throw( KGD::Socket::Exception )
		{
			InputBuffer::Lock lk( _recv );
			while( _running )
			{
				// have data
				if ( ! (*_recv).empty() )
				{
					Log::verbose( "%s: socket has data", getLogName() );
					ByteArray & buf = (*_recv).front();
					size_t rt = buf.copyTo( data, sz );
					if ( rt >= buf.size() )
						(*_recv).pop_front();
					else
						buf.chopFront( rt );
					return rt;
				}
				// read is not blocking
				else if ( _rdTimeout == 0 )
				{
					Log::verbose( "%s: socket non blocking, no data", getLogName() );
					return 0;
				}
				// read is blocking or timed blocking
				else
				{
					try
					{
						Log::verbose( "%s: waiting interleaved data", getLogName() );
						// blocking, wait until data arrives
						if ( _rdTimeout < 0 )
							_condNotEmpty.wait( lk );
						// timed, wait at most timeout
						else
						{
							if ( _condNotEmpty.timed_wait( lk, Clock::boostDeltaSec( _rdTimeout ) ) )
								Log::verbose( "%s: data arrived", getLogName(), _rdTimeout );
							else
							{
								Log::debug( "%s: no data in %.2lf sec", getLogName(), _rdTimeout );
								throw KGD::Socket::Exception( "readSome", EAGAIN );
							}
						}
					}
					catch( boost::thread_interrupted )
					{
						Log::debug( "%s: thread was interrupted waiting data", getLogName() );
						return 0;
					}
				}
			}

			Log::warning( "%s: socket has stopped", getLogName() );
			throw KGD::Socket::Exception( "readSome", "connection shut down" );
		}

		bool Interleave::isWriteBlock( ) const throw()
		{
			return (*_sock)->isWriteBlock();
		}
		void Interleave::setWriteBufferSize( size_t ) throw()
		{
			BOOST_ASSERT( false );
		}
		void Interleave::setWriteBlock( bool ) throw()
		{
			BOOST_ASSERT( false );
		}
		void Interleave::setWriteTimeout( double ) throw()
		{
			BOOST_ASSERT( false );
		}
		void Interleave::setReadBlock( bool b ) throw()
		{
			if ( (b && _rdTimeout >= 0) || (!b && _rdTimeout < 0) )
				_rdTimeout = ( b ? -1 : 0 );
		}
		void Interleave::setReadTimeout( double t ) throw()
		{
			_rdTimeout = t;
		}
		bool Interleave::isReadBlock( ) const throw()
		{
			return _rdTimeout < 0;
		}

		Channel::Description Interleave::getDescription() const
		{
			Channel::Description rt;
			rt.type = Channel::Shared;
			rt.ports = make_pair( _channel, _channel );

			return rt;
		}

		// ******************************************************************************************************************

		Socket::Socket( auto_ptr< KGD::Socket::Tcp > sk, const string & parentLogName )
		: _cseq( 0 )
		, _logName( parentLogName + " SOCKET" )
		{
			(*_sock).reset( sk.release() );
			(*_sock)->setReadBlock( true );
			(*_sock)->setWriteBlock( true );
			(*_sock)->setWriteTimeout( KGD::Socket::WRITE_TIMEOUT );
			(*_sock)->setWriteBufferSize( KGD::Socket::WRITE_BUFFER_SIZE );
		}

		Socket::~Socket()
		{
			Log::debug("%s: socket shutting down", getLogName());
			this->close();
			Log::verbose("%s: destroyed", getLogName());
		}

		void Socket::close() throw()
		{
			TcpTunnel::Lock lkS( _sock );
			if ( *_sock )
			{
				this->stopInterleaving();
				{
					Lock lk( _muxPorts );
					if ( ! _iChans.empty() )
						_condNoInterleave.wait( lk );
				}
				Log::debug("%s: no more interleaves", getLogName());

	// 			{
	// 				TcpTunnel::Lock lk( _sock );
					(*_sock)->close();
	// 			}
				Log::debug("%s: closed", getLogName());
			}
		}

		void Socket::stopInterleaving() throw()
		{
			Lock lk( _muxPorts );
			// stop all interleaved channels
			BOOST_FOREACH( ChannelMap::iterator::reference ch, _iChans )
				ch.second->close();
		}

		void Socket::stopInterleaving( const TSessionID & ssid ) throw()
		{
			Lock lk( _muxPorts );
			BOOST_FOREACH( ChannelMap::iterator::reference ch, _iChans )
				// more interleaves can be on a single session
				if ( ch.second->getSessionID() == ssid )
					ch.second->close();					
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

				Log::verbose( "%s: %d interleaved channels remaining", getLogName(), _iChans.size() );

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
								Log::error( "%s: unrecognized inbound packet, dequeuing %lu bytes", getLogName(), msgSz );
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
			Log::message( "%s: connection loop terminated", getLogName() );
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
			TcpTunnel::Lock lk( _sock );
			return (*_sock)->writeSome( data, sz );
		}

		size_t Socket::writeLast( void const * data, size_t sz ) throw( KGD::Socket::Exception )
		{
			TcpTunnel::Lock lk( _sock );
			(*_sock)->setLastPacket( true );
			return this->writeSome( data, sz );
		}

		bool Socket::isWriteBlock( ) const throw()
		{
			return (*_sock)->isWriteBlock();
		}

		void Socket::setWriteTimeout( double sec ) throw( KGD::Socket::Exception )
		{
			(*_sock)->setWriteTimeout( sec );
		}

		void Socket::setWriteBufferSize( size_t sz ) throw()
		{
			(*_sock)->setWriteBufferSize( sz );
		}

		void Socket::setWriteBlock( bool b ) throw()
		{
			(*_sock)->setWriteBlock( b );
		}

		void Socket::reply( const ostringstream & o ) throw( KGD::Socket::Exception )
		{
			string msg = o.str();
			Log::reply( msg );

			{
				TcpTunnel::Lock lk( _sock );

				size_t sent = 0, len = msg.size();
				const char * data = msg.c_str();

				while( sent < len )
				{
					try
					{
						size_t wrote = (*_sock)->writeSome( data, len - sent );
						data += wrote;
						sent += wrote;
					}
					catch( KGD::Socket::Exception const & e )
					{
						if ( e.wouldBlock() )
							Log::debug( "%s: socket would block in writeAll (%s)", getLogName(), e.what() );
						else
							throw;
					}
				}
			}
		}


		void Socket::reply(const Error::Definition & error) throw( KGD::Socket::Exception )
		{
			ostringstream s;
			s << VER << " " << error.getCode() << " " << error.getDescription() << EOL
				<< "CSeq: " << _cseq << EOL
				<< "Server: " << KGD::Daemon::getName() << EOL
				<< EOL;

			this->reply( s );
		}

		void Socket::reply( Method::Base & m ) throw( KGD::Socket::Exception )
		{
			try
			{
				// prapare data
				m.prepare();

				// prepend common header
				ostringstream s;
				s << VER << " " << Error::Ok.getCode() << " " << Error::Ok.getDescription() << EOL
					<< "CSeq: " << _cseq << EOL
					<< "Server: " << KGD::Daemon::getName() << EOL
					<< m.getReply();

				// reply
				this->reply( s );

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
