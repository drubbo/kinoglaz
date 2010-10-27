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
 * File name: ./rtsp/server.cpp
 * First submitted: 2010-01-16
 * First submitter: Emiliano Leporati <emiliano.leporati@gmail.com>
 * Contributor(s) so far - 2010-07-30 :
 *     Emiliano Leporati <emiliano.leporati@gmail.com>
 *
 * Last changes :
 *     Kinoglaz birth: pervasive bug fixing, improvements and client support
 *     log messages refactor; shared descriptor are optional now; spot insertion support
 *     ns refactor over messages; seek exception blobbed up
 *     sig HUP supported
 *     sdp debugged
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

		Server::Server( const Ini::Entries & params ) throw ( KGD::Exception::NotFound, KGD::Socket::Exception )
		: _maxConnections( 0 )
		, _ip( "" )
		, _port( 0 )
		, _running( false )
		{
			this->setupParameters( params );
		}

		void Server::setupParameters( const Ini::Entries & params ) throw( KGD::Exception::NotFound, KGD::Socket::Exception )
		{
			_maxConnections = fromString< uint16_t >( params[ "limit" ] );
			while( _maxConnections != 0 && _conns.size() > _maxConnections )
				_conns.erase( _conns.rbegin().base() );

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
				_socket.reset( new KGD::Socket::TcpServer( _port, _ip, _maxConnections ) );
				Log::message( "KGD: binding to %s:%d, max connections %u", _ip.c_str(), _port, _maxConnections );
			}

		}


		Server::~Server()
		{
			Log::message("KGD: tearing down %d connections", _conns.size());
			_conns.clear();
			Log::message("KGD: shut down");
		}

		void Server::serve( Connection * conn )
		{
			try
			{
				conn->listen();
			}
			catch( const KGD::Exception::Generic & e )
			{
				Log::debug( "KGD: serve: %s", e.what() );
			}

			if ( _running )
			{
				Log::debug( "KGD: calling remove of %lu", conn->getID() );
				this->remove( *conn );
			}
			else
			{
				Log::warning( "KGD: server is not running, destruction in progress" );
				boost::this_thread::yield();
			}
		}

		void Server::handle( auto_ptr< KGD::Socket::Tcp > channel )
		{
			Server::Lock lk( Server::mux() );

			if ( _maxConnections != 0 && _conns.size() >= _maxConnections )
				throw RTSP::Exception::Generic( "handle", "KGD: CONN limit already reached" );

			// abilitazione TCP keepalive
			channel->setKeepAlive( true );
			// creo il gestore della richiesta
			auto_ptr< Connection > conn( new Connection( channel.release() ) );
			Connection * cPtr = conn.get();
			// accodo
			_conns.push_back( conn );
			// lancio il thread di gestione
			new boost::thread( boost::bind( &RTSP::Server::serve, this, cPtr ) );
		}

		void Server::remove( Connection & conn ) throw( KGD::Exception::NotFound )
		{
			Server::Lock lk( Server::mux() );
			uint32_t connID = conn.getID();

			Log::debug( "KGD: removing connection %lu", connID );

			ConnectionList::iterator it = find_if( _conns.begin(), _conns.end(), boost::bind( &Connection::getID, _1 ) == connID );
// 			for( ConnectionList::iterator it = _conns.begin(); it != _conns.end(); ++it )
			if ( it == _conns.end() )
			{
				Log::error("KGD: no connection %lu found", conn.getID() );
				throw KGD::Exception::NotFound( "KGD: CONN to remove " + toString( conn.getID() ) );
			}
			else
			{
/*				if ( it->getID() == conn.getID() )
				{*/
					_conns.erase( it );
					Log::message("KGD: CONN removed, %d remaining", _conns.size());
/*					return;
				}*/
			}

		}

		void Server::run()
		{
			try
			{
				while(_running)
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
			if (!_running)
			{
				_running = true;
				this->run();
			}
		}

	}
}
