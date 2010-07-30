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
 * File name: ./rtsp/connection.cpp
 * First submitted: 2010-01-16
 * First submitter: Emiliano Leporati <emiliano.leporati@gmail.com>
 * Contributor(s) so far - 2010-07-30 :
 *     Emiliano Leporati <emiliano.leporati@gmail.com>
 *
 * Last changes :
 *     interleaved channels shutdown fixed
 *     Kinoglaz birth: pervasive bug fixing, improvements and client support
 *     log messages refactor; shared descriptor are optional now; spot insertion support
 *     ns refactor over messages; seek exception blobbed up
 *     sdp debugged
 *
 **/


#include "rtsp/connection.h"
#include "rtsp/session.h"
#include "rtsp/method.h"
#include "lib/utils/container.hpp"
#include "lib/utils/map.hpp"
#include "lib/log.h"
#include "lib/common.h"

#include <sstream>
#include <boost/regex.hpp>

namespace KGD
{
	namespace RTSP
	{
		bool Connection::SHARE_DESCRIPTORS = false;

		Connection::Connection( KGD::Socket::Tcp * socket )
		: _id( random() )
		, _agent( UA_UNDEFINED )
		, _logName( "CONN " + socket->getRemoteHost() + "#" + toString( _id ) )
		, _socket( new RTSP::Socket( socket, _logName ) )
		{
			Log::debug( "%s: created", getLogName() );
		}

		Connection::~Connection()
		{
			Log::debug("%s: shutting down", getLogName() );

			_socket->stopInterleaving();

			_sessions.clear();

			// release descriptors
			SDP::Descriptions & sdpool = SDP::Descriptions::getInstance();
			for( TDescriptorMap::Iterator it( _descriptors ); it.isValid(); it.next() )
			{
				Log::debug( "%s: releasing SDP description %s", getLogName(), it.key().c_str() );
				it.val().invalidate();
				// maybe they were shared, inform the pool
				sdpool.releaseDescription( it.key() );
			}
			// release local descriptors
			Log::debug( "%s: releasing SDP local description", getLogName() );
			_descriptorInstances.clear();
			
			Log::debug( "%s: destroyed", getLogName() );
		}

		const char * Connection::getLogName() const throw()
		{
			return _logName.c_str();
		}

		UserAgent Connection::getUserAgent(  ) const throw()
		{
			return _agent;
		}

		void Connection::setUserAgent( UserAgent ua ) throw()
		{
			_agent = ua;
		}

		uint32_t Connection::getID() const
		{
			return _id;
		}

		bool Connection::hasSessions() const
		{
			return ! _sessions.empty();
		}

		const Message::Request & Connection::getLastRequest() const throw( KGD::Exception::NullPointer )
		{
			return *_lastRq;
		}
		const Message::Response & Connection::getLastResponse() const throw( KGD::Exception::NullPointer )
		{
			return *_lastResp;
		}

		
		void Connection::reply( const Error::Definition & error ) throw( KGD::Socket::Exception )
		{
			_socket->reply( error );
		}

		RTSP::Socket & Connection::getSocket() throw( )
		{
			return *_socket;
		}

		const SDP::Container & Connection::loadDescription( const string & file ) throw( RTSP::Exception::ManagedError )
		{
			try
			{
				if ( SHARE_DESCRIPTORS )
				{
					// get mutable instance
					SDP::Descriptions & sdpool = SDP::Descriptions::getInstance();
					// so description is loaded if needed
					SDP::Container & rt = sdpool.loadDescription( file );
					_descriptors( file ) = rt;
					return rt;
				}
				else
				{
					// create new instance
					SDP::Container * s = new SDP::Container( file );
					_descriptorInstances( file ) = s;
					_descriptors( file ) = *s;
					return *s;
				}
			}
			catch( const SDP::Exception::Generic & e )
			{
				Log::error( "%s: %s", getLogName(), e.what() );
				throw RTSP::Exception::ManagedError( Error::InternalServerError );
			}
		}

		const SDP::Container & Connection::getDescription( const string & file ) const throw( RTSP::Exception::ManagedError )
		{
			if ( _descriptors.has( file ) )
				return _descriptors[ file ];
			else
				throw RTSP::Exception::ManagedError( Error::NotFound );
		}
		SDP::Container & Connection::getDescription( const string & file ) throw( RTSP::Exception::ManagedError )
		{
			if ( _descriptors.has( file ) )
				return _descriptors( file );
			else
				throw RTSP::Exception::ManagedError( Error::NotFound );
		}

		void Connection::listen() throw( KGD::Exception::Generic )
		{
			Log::debug( "%s: listen loop start", getLogName() );

			ByteArray receiveBuffer( 1024 );

			for(;;)
			{
				try
				{
					_socket->listen();
				}
				// got request
				catch ( Message::Request * rq )
				{
					_lastRq = rq;
					try
					{
						// get message instance
						Ptr::Scoped< Method::Base > message =
							Factory::ClassRegistry< Method::Base >::newInstance( rq->getMethodID() );
						message->setConnection( *this );
						// reply to message performing requested actions
						_socket->reply( *message );
					}
					// method not found in factory
					catch( const KGD::Exception::NotFound & e )
					{
						Log::error( "%s: %s", getLogName(), e.what() );
						throw RTSP::Exception::ManagedError( Error::NotImplemented );
					}
					catch( const KGD::Socket::Exception & e )
					{
						Log::error( "%s: %s", getLogName(), e.what() );
						throw;
					}
				}
				// got response
				catch( Message::Response * resp )
				{
					_lastResp = resp;
				}
			}

			Log::debug( "%s: listen loop end", getLogName() );
		}

		Session & Connection::createSession( const TSessionID & sessionID ) throw( RTSP::Exception::ManagedError )
		{
			if ( ! _sessions.has( sessionID ) )
			{
				Session *s = new Session( sessionID, *this );
				_sessions( sessionID ) = s;
				return *s;
			}
			else
				throw RTSP::Exception::ManagedError( Error::Conflict );
		}

		Session & Connection::getSession( const TSessionID & sessionID ) throw( RTSP::Exception::ManagedError )
		{
			try
			{
				return _sessions( sessionID, KGD::Exception::NotFound( toString( sessionID ) ) );
			}
			catch( KGD::Exception::NotFound )
			{
				throw RTSP::Exception::ManagedError( Error::SessionNotFound );
			}
		}

		void Connection::removeSession( const TSessionID & sessionID ) throw( RTSP::Exception::ManagedError )
		{
			if ( _sessions.has( sessionID ) )
			{
				_socket->stopInterleaving( sessionID );
				_sessions.erase( sessionID );
			}
			else
				throw RTSP::Exception::ManagedError( Error::SessionNotFound );
		}
	}
}
