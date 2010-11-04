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
 * File name: src/rtsp/connection.h
 * First submitted: 2010-07-30
 * First submitter: Emiliano Leporati <emiliano.leporati@gmail.com>
 * Contributor(s) so far - 2010-11-04 :
 *     Emiliano Leporati <emiliano.leporati@gmail.com>
 *
 * Last changes :
 *     english comments; removed leak with connection serving threads
 *     testing interrupted connections
 *     boosted
 *     Some cosmetics about enums and RTCP library
 *     source import
 *
 **/


#ifndef __KGD_RTSP_CONNECTION_H
#define __KGD_RTSP_CONNECTION_H

#include "rtsp/interleave.h"
#include "rtsp/common.h"
#include "rtsp/buffer.h"
#include "rtsp/error.h"
#include "rtsp/exceptions.h"
#include "rtsp/ports.h"
#include "sdp/sdp.h"

#include <vector>
#include <string>
#include <set>

using namespace std;

namespace KGD
{
	namespace RTSP
	{
		class Server;
		
		//! RTSP client connection management
		class Connection
		: public Safe::LockableBase< RMutex >
		{
		public:
			static bool SHARE_DESCRIPTORS;
		private:
			typedef boost::ptr_map< TSessionID, Session > SessionMap;
			typedef map< string, ref< SDP::Container > > DescriptorMap;
			typedef boost::ptr_map< string, SDP::Container > LocalDescriptorMap;

			//! ref to server
			RTSP::Server & _svr;
			//! unique random id
			uint32_t _id;
			//! user agent
			UserAgent::type _agent;
			//! log name
			const string _logName;
			//! control socket
			boost::scoped_ptr< RTSP::Socket > _socket;
			//! last request received
			boost::scoped_ptr< Message::Request > _lastRq;
			//! last response received
			boost::scoped_ptr< Message::Response > _lastResp;
			//! RTSP sessions
			SessionMap _sessions;
			//! used media descriptors, shared or not
			DescriptorMap _descriptors;
			//! local instanced descriptors
			LocalDescriptorMap _descriptorInstances;
			//! serving thread id
			boost::thread::id _threadID;

		public:
			//! ctor
			Connection( auto_ptr< KGD::Socket::Tcp > socket, RTSP::Server & );
			//! dtor - closes all sessions and shuts down the socket
			~Connection();

			//! get log identifier for this connection
			const char * getLogName() const throw();

			//! set thread ID to ease removal
			void setServingThreadID( const boost::thread::id & ) throw();
			//! get serving thread ID
			const boost::thread::id & getServingThreadID() const throw();
			//! sets user agent, to build specialized timelines in rtp sessions
			void setUserAgent( UserAgent::type ) throw();
			//! get user agent operating on this connection
			UserAgent::type getUserAgent(  ) const throw();
			
			//! socket-event-driven main request loop
			void listen() throw( KGD::Exception::Generic );

			//! returns this id
			uint32_t getID() const;

			//! reply an error
			void reply( const Error::Definition & error ) throw( KGD::Socket::Exception );

			//! returns last request received
			const Message::Request & getLastRequest() const throw( KGD::Exception::NullPointer );
			//! returns last response received (actually unused)
			const Message::Response & getLastResponse() const throw( KGD::Exception::NullPointer );
			
			//! creates a new session if none with given ID is present
			Session & createSession( const TSessionID & sessionID ) throw( RTSP::Exception::ManagedError );
			//! removes RTSP session given its ID
			void removeSession( const TSessionID & sessionID ) throw( RTSP::Exception::ManagedError );
			//! returns RTSP session given the ID
			Session & getSession( const TSessionID & sessionID ) throw( RTSP::Exception::ManagedError );

			//! load SDP description and bind it to this connection
			const SDP::Container & loadDescription( const string & file ) throw( RTSP::Exception::ManagedError );
			//! get previously bound SDP description
			const SDP::Container & getDescription( const string & file ) const throw( RTSP::Exception::ManagedError );
			//! get previously bound SDP description
			SDP::Container & getDescription( const string & file ) throw( RTSP::Exception::ManagedError );

			//! returns RTSP socket shell
			RTSP::Socket & getSocket() throw( );

			//! tells if this request has active RTSP sessions
			bool hasSessions() const;
		};
	}
}

#endif
