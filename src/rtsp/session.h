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
 * File name: src/rtsp/session.h
 * First submitted: 2010-07-30
 * First submitter: Emiliano Leporati <emiliano.leporati@gmail.com>
 * Contributor(s) so far - 2010-11-04 :
 *     Emiliano Leporati <emiliano.leporati@gmail.com>
 *
 * Last changes :
 *     fixed some SSRC issues; added support for client-hinted ssrc; fixed SIGTERM shutdown when serving
 *     minor cleanup and more robust Range / Scale support during PLAY
 *     removed magic numbers in favor of constants / ini parameters
 *     introduced keep alive on control socket (me dumb)
 *     boosted
 *
 **/


#ifndef __KGD_RTSP_SESSION_H
#define __KGD_RTSP_SESSION_H

#include "lib/urlencode.h"
#include "rtsp/common.h"
#include "lib/socket.h"
#include "lib/exceptions.h"
#include "rtsp/error.h"
#include "sdp/sdp.h"

#include <vector>
#include <string>

#ifdef HAVE_CONFIG_H
#include "config.h"
#else
#error "Please use platform configuration file."
#endif

using namespace std;

namespace KGD
{
	namespace RTP
	{
		class Session;
	}

	namespace RTSP
	{
		class Connection;

		//! RTSP session
		class Session
		: public Safe::LockableBase< RMutex >
		{
		private:
			typedef boost::ptr_map< string, RTP::Session > SessionMap;
			//! RTSP session id
			TSessionID _id;
			//! RTP sessions
			SessionMap _sessions;
			//! Originating request
			Connection & _conn;
			//! registers if a play has been ever issued
			bool _playIssued;
			//! log identifier
			const string _logName;
		public:
			//! ctor
			Session( const TSessionID &, Connection & );
			//! dtor - closes all RTP sessions
			~Session();
			//! returns log identifier for this session
			const char * getLogName() const throw();
			//! creates a new RTP session returning it
			RTP::Session & createSession( const Url & url, const Channel::Description & remote, const boost::optional< TSSrc > & ssrc ) throw( RTSP::Exception::ManagedError );
			//! returns RTP session list
			ref_list< RTP::Session > getSessions() throw();
			//! returns RTP session list
			ref_list< const RTP::Session > getSessions() const throw();
			//! returns the RTP session relative to the track specified
			RTP::Session & getSession( const string & track ) throw( RTSP::Exception::ManagedError );
			//! returns the RTP session relative to the track specified
			const RTP::Session & getSession( const string & track ) const throw( RTSP::Exception::ManagedError );
			//! destroies a single RTP session
			void removeSession( const string & track ) throw( RTSP::Exception::ManagedError );
			
			//! returns this session' ID
			const TSessionID & getID() const throw();
			//! returns this session' originating Connection
			Connection & getConnection() throw( );
			//! returns this session' originating Connection
			const Connection & getConnection() const throw( );

			//! adds another media in the current running sessions at specified media time (oo = asap)
			void insertMedia( SDP::Container & media, double curMediaTime = HUGE_VAL ) throw( KGD::Exception::OutOfBounds );

			//! returns current (average) play range
			PlayRequest getPlayRange() const throw();
			//! returns current play range of given RTP session
			PlayRequest getPlayRange( const string & track ) const throw( RTSP::Exception::ManagedError );
			//! aggregate pre-play: sets up time range on every RTP session
			PlayRequest play( const PlayRequest & ) throw( RTSP::Exception::ManagedError );
			//! aggregate post-play: invokes play on every RTP session
			void play() throw();
			//! aggregate pause: invokes pause on every RTP session
			void pause( const PlayRequest & ) throw();
			//! aggregate unpause: invokes unpause on every RTP session
			void unpause( const PlayRequest & ) throw();

			//! tells if a play has been ever issued
			bool hasPlayed() const throw();
			
			//! send message to client
			void reply( const Error::Definition & ) throw( KGD::Socket::Exception );
		};
	}
}

#endif
