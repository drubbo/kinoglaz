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
 * File name: src/rtsp/connection.cpp
 * First submitted: 2010-07-30
 * First submitter: Emiliano Leporati <emiliano.leporati@gmail.com>
 * Contributor(s) so far - 2010-11-04 :
 *     Emiliano Leporati <emiliano.leporati@gmail.com>
 *
 * Last changes :
 *     fixed some SSRC issues; added support for client-hinted ssrc; fixed SIGTERM shutdown when serving
 *     english comments; removed leak with connection serving threads
 *     testing interrupted connections
 *     boosted
 *     Some cosmetics about enums and RTCP library
 *
 **/


#include "rtsp/connection.h"
#include "rtsp/session.h"
#include "rtsp/method.h"
#include "rtsp/server.h"
#include "lib/log.h"
#include "lib/common.h"

#include <sstream>
#include <boost/regex.hpp>

namespace KGD
{
	namespace RTSP
	{
		bool Connection::SHARE_DESCRIPTORS = false;

		Connection::Connection( auto_ptr< KGD::Socket::Tcp > socket, RTSP::Server & svr )
		: _svr( svr )
		, _id( random() )
		, _agent( UserAgent::Generic )
		, _logName( "CONN " + socket->getRemoteHost() + "#" + toString( _id ) )
		, _socket( new RTSP::Socket( socket, _logName ) )
		{
			Log::verbose( "%s: created", getLogName() );
		}

		Connection::~Connection()
		{
			Connection::Lock lk( *this );
			
			Log::debug("%s: shutting down", getLogName() );

			_sessions.clear();

			_socket.reset();

			// release descriptors
			SDP::Descriptions::Reference sdpool = SDP::Descriptions::getInstance();
			BOOST_FOREACH( DescriptorMap::iterator::reference it, _descriptors )
			{
				Log::debug( "%s: releasing SDP description %s", getLogName(), it.first.c_str() );
				// maybe they were shared, inform the pool
				sdpool->releaseDescription( it.first );
				it.second.invalidate();
			}
			// release local descriptors
			Log::debug( "%s: releasing SDP local description", getLogName() );
			_descriptorInstances.clear();
			
			Log::verbose( "%s: destroyed", getLogName() );
		}

		void Connection::setServingThreadID( const boost::thread::id& id ) throw()
		{
			_threadID = id;
		}
		const boost::thread::id& Connection::getServingThreadID() const throw()
		{
			return _threadID;
		}

		const char * Connection::getLogName() const throw()
		{
			return _logName.c_str();
		}

		UserAgent::type Connection::getUserAgent(  ) const throw()
		{
			return _agent;
		}

		void Connection::setUserAgent( UserAgent::type ua ) throw()
		{
			_agent = ua;
			Log::message( "%s: setting user agent '%s'", getLogName(), UserAgent::name[ ua ].c_str() );
		}

		uint32_t Connection::getID() const
		{
			return _id;
		}

		bool Connection::hasSessions() const
		{
			Connection::Lock lk( *this );
			return ! _sessions.empty();
		}

		const Message::Request & Connection::getLastRequest() const throw( KGD::Exception::NullPointer )
		{
			Connection::Lock lk( *this );
			return *_lastRq;
		}
		const Message::Response & Connection::getLastResponse() const throw( KGD::Exception::NullPointer )
		{
			Connection::Lock lk( *this );
			return *_lastResp;
		}

		
		void Connection::reply( const Error::Definition & error ) throw( KGD::Socket::Exception )
		{
			Connection::Lock lk( *this );
			_socket->reply( error );
		}

		RTSP::Socket & Connection::getSocket() throw( )
		{
			return *_socket;
		}

		const SDP::Container & Connection::loadDescription( const string & file ) throw( RTSP::Exception::ManagedError )
		{
			Connection::Lock lk( *this );
			try
			{
				if ( SHARE_DESCRIPTORS )
				{
					// get mutable instance
					SDP::Descriptions::Reference sdpool = SDP::Descriptions::getInstance();
					// so description is loaded if needed
					ref< SDP::Container > rt( sdpool->loadDescription( file ) );
					_descriptors.insert( make_pair(file, rt) );
					return *rt;
				}
				else
				{
					// create new instance
					auto_ptr< SDP::Container > cnt( new SDP::Container( file ) );
					ref< SDP::Container > rt( *cnt );
					{
						string tmpFile( file );
						_descriptorInstances.insert( tmpFile, cnt );
					}
					_descriptors.insert( make_pair(file, rt) );
					return *rt;
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
			Connection::Lock lk( *this );
			DescriptorMap::const_iterator it = _descriptors.find( file );
			if ( it != _descriptors.end( ) )
				return *it->second;
			else
				throw RTSP::Exception::ManagedError( Error::NotFound );
		}
		SDP::Container & Connection::getDescription( const string & file ) throw( RTSP::Exception::ManagedError )
		{
			Connection::Lock lk( *this );
			DescriptorMap::iterator it = _descriptors.find( file );
			if ( it != _descriptors.end( ) )
				return *it->second;
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
					Connection::Lock lk( *this );
					_lastRq.reset( rq );

					// process message and reply
					try
					{
						// get message instance
						boost::scoped_ptr< Method::Base > message(
							Factory::ClassRegistry< Method::Base >::newInstance( rq->getMethodID() ) );
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
					Connection::Lock lk( *this );
					_lastResp.reset( resp );
				}
			}
		}

		Session & Connection::createSession( const TSessionID & sessionID ) throw( RTSP::Exception::ManagedError )
		{
			Connection::Lock lk( *this );
			if ( _sessions.find( sessionID ) == _sessions.end() )
			{
				auto_ptr< Session > s( new Session( sessionID, *this ) );
				pair< SessionMap::iterator, bool > ins = _sessions.insert( sessionID, s );
				return *ins.first->second;
			}
			else
				throw RTSP::Exception::ManagedError( Error::Conflict );
		}

		Session & Connection::getSession( const TSessionID & sessionID ) throw( RTSP::Exception::ManagedError )
		{
			Connection::Lock lk( *this );
			try
			{
				return _sessions.at( sessionID );
			}
			catch( boost::bad_ptr_container_operation )
			{
				throw RTSP::Exception::ManagedError( Error::SessionNotFound );
			}
		}

		void Connection::removeSession( const TSessionID & sessionID ) throw( RTSP::Exception::ManagedError )
		{
			Connection::Lock lk( *this );
			if ( _sessions.find( sessionID ) != _sessions.end() )
			{
				_socket->stopInterleaving( sessionID );
				_sessions.erase( sessionID );
				if ( _sessions.empty() )
				{
					Log::debug( "%s: no more sessions, tearing down", getLogName() );
					_socket->close();
				}
			}
			else
				throw RTSP::Exception::ManagedError( Error::SessionNotFound );
		}
	}
}
