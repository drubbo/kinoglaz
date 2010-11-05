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
 * File name: src/rtsp/server.cpp
 * First submitted: 2010-07-30
 * First submitter: Emiliano Leporati <emiliano.leporati@gmail.com>
 * Contributor(s) so far - 2010-11-04 :
 *     Emiliano Leporati <emiliano.leporati@gmail.com>
 *
 * Last changes :
 *     english comments; removed leak with connection serving threads
 *     removed magic numbers in favor of constants / ini parameters
 *     introduced keep alive on control socket (me dumb)
 *     testing interrupted connections
 *     boosted
 *
 **/


#include "rtsp/server.h"
#include "rtsp/connection.h"
#include "rtsp/exceptions.h"
#include "lib/ini.h"
#include "lib/common.h"
#include "lib/log.h"

namespace KGD
{
	namespace RTSP
	{
		Server::Reference Server::getInstance( const Ini::Entries & params ) throw( KGD::Exception::Generic )
		{
			Instance::Lock lk( _instance );

			if ( !*_instance )
				(*_instance).reset( new Server( params ) );
			else
				throw KGD::Exception::InvalidState( "KGD instance already initialized" );

			return newInstanceRef();
		}

		Server::Reference Server::getInstance() throw( KGD::Exception::InvalidState )
		{
			Instance::Lock lk( _instance );

			if ( !*_instance )
				throw KGD::Exception::InvalidState( "KGD instance not yet initialized" );

			return newInstanceRef();
		}


		Server::Connections::Connections( Server & s )
		: _srv( s )
		, max( 0 )
		{
		}


		void Server::Connections::start()
		{
			th.reset( new boost::thread( boost::bind( &Server::Connections::run, this )));
		}

		void Server::Connections::run()
		{
			while( _srv._running )
			{
				Server::Lock lk( Server::mux() );
				list.erase_if( ! boost::bind( &Connection::isActive, _1 ) );
				Log::debug( "KGD: active connections %u", list.size() );
				wakeup.timed_wait( lk, Clock::boostDeltaSec( 10 ) );
			}
			th.wait();
		}





		Server::Server( const Ini::Entries & params ) throw ( KGD::Exception::NotFound, KGD::Socket::Exception )
		: _ip( "" )
		, _port( 0 )
		, _conns( *this )
		, _running( false )
		{
			this->setupParameters( params );
		}

		void Server::setupParameters( const Ini::Entries & params ) throw( KGD::Exception::NotFound, KGD::Socket::Exception )
		{
			_conns.max = fromString< uint16_t >( params[ "limit" ] );
			while( _conns.max != 0 && _conns.list.size() > _conns.max )
				_conns.list.erase( _conns.list.rbegin().base() );

			TPort newPort = fromString< TPort >( params( "port", "8554" ) );
			string newIP = params( "ip", "*" );
			if ( _socket && (_port != newPort || _ip != newIP ))
			{
				Log::message( "KGD: switching from %s:%d to %s:%d", _ip.c_str(), _port, newIP.c_str(), newPort );
				_socket.reset();
			}
			if ( !_socket )
			{
				_port = newPort;
				_ip = newIP;
				_socket.reset( new KGD::Socket::TcpServer( _port, _ip, _conns.max ) );
				Log::message( "KGD: binding to %s:%d, max connections %u", _ip.c_str(), _port, _conns.max );
			}

		}


		Server::~Server()
		{			
			Log::message("KGD: stop clean thread");
			_running = false;
			_conns.wakeup.notify_all();
			_conns.th.reset();
			Log::message("KGD: tearing down %d connections", _conns.list.size());
			_conns.list.clear();
			Log::message("KGD: shut down");
		}

		void Server::handle( auto_ptr< KGD::Socket::Tcp > channel )
		{
			Server::Lock lk( Server::mux() );

			_conns.list.erase_if( ! boost::bind( &Connection::isActive, _1 ) );
			Log::debug( "KGD: active connections %u", _conns.list.size() );
			if ( _conns.max != 0 && _conns.list.size() >= _conns.max )
				throw RTSP::Exception::Generic( "handle", "KGD: connection limit reached ("+toString( _conns.max )+")" );

			// enable TCP keepalive
			channel->setKeepAlive( true );
			// creo il gestore della richiesta
			auto_ptr< Connection > conn( new Connection( channel, *this ) );
			// add connection to the basket
			_conns.list.push_back( conn );
		}

		void Server::run()
		{
			try
			{
				while( _running )
				{
					try
					{
						Log::message("KGD: waiting connections");
						this->handle( _socket->accept() );
					}
					catch( const KGD::Socket::Exception & e)
					{
						// on interrupt we continue
						if ( e.getErrcode() != EINTR )
						{
							Log::error("KGD: run: %s", e.what() );
							throw;
						}

					}
				}
			}
			catch( const KGD::Exception::Generic & e)
			{
				Log::error(e);
				_running = false;
			}
		}

		void Server::stop()
		{
			_running = false;
		}

		void Server::start()
		{
			BOOST_ASSERT( !_running );
			_running = true;
			_conns.start();
			this->run();
		}

	}
}
