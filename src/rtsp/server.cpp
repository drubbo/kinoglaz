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
#include "lib/utils/container.hpp"

namespace KGD
{
	namespace RTSP
	{
		Server& Server::getInstance( const Ini::Entries & params ) throw( KGD::Exception::Generic )
		{
			RLock lk( _mux );

			if ( !_instance )
			{
				_instance = new Server( params );
			}
			else
				throw KGD::Exception::InvalidState( "KGD instance already initialized" );

			return *_instance;
		}

		Server& Server::getInstance() throw( KGD::Exception::InvalidState )
		{
			RLock lk( _mux );
			if ( !_instance )
				throw KGD::Exception::InvalidState( "KGD instance not yet initialized" );

			return *_instance;
		}

		Server::Server( const Ini::Entries & params ) throw ( KGD::Exception::NotFound, KGD::Socket::Exception )
		: _maxConnections( 0 )
		, _port( 0 )
		, _running( false )
		{
			this->setupParameters( params );
		}

		void Server::setupParameters( const Ini::Entries & params ) throw( KGD::Exception::NotFound, KGD::Socket::Exception )
		{
			_maxConnections = fromString< ushort >( params[ "limit" ] );
			while( _maxConnections != 0 && _conns.size() > _maxConnections )
			{
				Ptr::clear( _conns.back() );
				_conns.pop_back();
			}

			TPort newPort = fromString< TPort >( params[ "port" ] );
			if ( _socket && _port != newPort )
			{
				Log::message( "KGD: switching port from %d to %d", _port, newPort );
				_socket.destroy();
			}
			if ( !_socket )
			{
				_port = newPort;
				_socket = new KGD::Socket::TcpServer( _port );
				Log::message( "KGD: binding to port %d", newPort );
			}

		}


		Server::~Server()
		{
			RLock lk(_mux);

			Log::message("KGD: tearing down %d connections", _conns.size());
			Ctr::clear( _conns );

			// if still running, shut down the socket
			if ( _running )
			{
				_running = false;
				_socket.destroy();
			}

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

			Log::debug( "KGD: calling remove of %lu", conn->getID() );
			this->remove( conn );
		}

		void Server::handle( KGD::Socket::Tcp * channel )
		{
			RLock lk( _mux );

			if ( _maxConnections != 0 && _conns.size() >= _maxConnections )
				throw RTSP::Exception::Generic( "handle", "KGD: CONN limit already reached" );

			// creo il gestore della richiesta
			Connection * conn = new Connection( channel );
			// accodo
			_conns.push_back( conn );
			// lancio il thread di gestione
			new Thread(boost::bind(&RTSP::Server::serve, this, conn));
		}

		void Server::remove( Connection * conn ) throw( KGD::Exception::NotFound )
		{
			RLock lk( _mux );
			Log::debug( "KGD: removing connection %lu", conn->getID() );

			if ( !_running )
			{
				Log::debug( "KGD: server is not running" );
				return;
			}

			for( Ctr::Iterator< list< Connection * > > it( _conns ); it.isValid(); it.next() )
			{
				if ( (*it)->getID() == conn->getID() )
				{
					Ptr::clear( conn );
					_conns.erase( it );
					Log::message("KGD: CONN removed, %d remaining", _conns.size());
					return;
				}
			}

			Log::error("KGD: no connection %lu found", conn->getID() );

			throw KGD::Exception::NotFound( "KGD: CONN to remove" );
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
