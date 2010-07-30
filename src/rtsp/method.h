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
 * File name: ./rtsp/method.h
 * First submitted: 2010-01-16
 * First submitter: Emiliano Leporati <emiliano.leporati@gmail.com>
 * Contributor(s) so far - 2010-07-30 :
 *     Emiliano Leporati <emiliano.leporati@gmail.com>
 *
 * Last changes :
 *     formats on their own; better Factory elements naming; separate library for every component
 *     Kinoglaz birth: pervasive bug fixing, improvements and client support
 *     log messages refactor; shared descriptor are optional now; spot insertion support
 *     ns refactor over messages; seek exception blobbed up
 *     sdp debugged
 *
 **/


#ifndef __KGD_RTSP_METHODS_H
#define __KGD_RTSP_METHODS_H

#include "lib/urlencode.h"
#include "lib/utils/safe.hpp"
#include "rtsp/common.h"
#include "rtsp/exceptions.h"
#include "lib/utils/pointer.hpp"
#include "lib/utils/factory.hpp"

#include <string>

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
		class Session;

		//! RTSP methods implementation
		namespace Method
		{
			//! RTSP method identifier
			enum ID
			{
				OPTIONS,
				DESCRIBE,
				SETUP,
				PLAY,
				PAUSE,
				TEARDOWN,

				ANNOUNCE,
				REDIRECT,
				RECORD,
				GET_PARAMETER,
				SET_PARAMETER
			};

			//! tells if random-access play is supported
			extern bool SUPPORT_SEEK;

			//! RTSP method count
			extern const uint8_t count;
			//! RTSP method strings
			extern const string name[];

			//! abstract RTSP method
			class Base
			: virtual public Factory::Base
			{
			protected:
				//! reference to the RTSP connection through which the method-request arrived
				Ptr::Ref< Connection > _conn;

				//! returns current timestamp formatted as RFC requires
				static string getTimestamp( double t = HUGE_VAL ) throw();
				//! ctor
				Base( );
			public:
				//! dtor
				virtual ~Base();
				//! sets a reference to RTSP Connection
				void setConnection( Connection & );
				//! load parameters and do necessary actions to prepare method execution
				virtual void prepare() throw( RTSP::Exception::ManagedError ) = 0;
				//! get RTSP reply to this method
				virtual string getReply( ) throw( RTSP::Exception::ManagedError ) = 0;
				//! do the method implied action over request
				virtual void execute() throw( RTSP::Exception::ManagedError );
			};

			//! base class for methods with url specification
			class Url
			: public Method::Base
			{
			protected:
				//! requested url
				Ptr::Ref< const KGD::Url > _url;

				const KGD::Url & urlGet() const throw( RTSP::Exception::ManagedError );
				//! throw error if file not exists
				void urlCheckValid() const throw( RTSP::Exception::ManagedError );
				//! throw error if track not exists
				void urlCheckTrack() const throw( RTSP::Exception::ManagedError );

				//! ctor
				Url(  );
			public:
				//! extract url and check its validity
				virtual void prepare() throw( RTSP::Exception::ManagedError );
			};

			//! base class for methods with session identification
			class Session
			: virtual public Url
			{
			protected:
				//! requested RTSP session ID
				TSessionID _sessionID;
				//! RTSP session related to ID
				Ptr::Ref< RTSP::Session > _rtsp;

				//! extracts session ID from request
				virtual TSessionID getSessionID() const throw( RTSP::Exception::ManagedError );
				//! retrieves the session from the connection 
				virtual RTSP::Session & getSession() throw( RTSP::Exception::ManagedError );

				//! ctor
				Session();
			public:
				//! loads session ID and session object
				virtual void prepare() throw( RTSP::Exception::ManagedError );
			};

			//! base class for methods with time range specification
			class Time
			: public Session
			{
			protected:
				//! get requested time range
				PlayRequest getTimeRange() const throw( );
				//! ctor
				Time(  );
			};





			//! OPTIONS method handler
			class Options 
			: public Url
			, public Factory::Multiton< Method::Base, Options, OPTIONS >
			{
			protected:
				//! ctor
				Options(  );
				friend class Factory::Multi< Method::Base, Options >;
			public:
				//! reply supported options
				virtual string getReply() throw( RTSP::Exception::ManagedError );
				//! set user agent in use in this connection
				virtual void execute() throw( RTSP::Exception::ManagedError );
			};

			//! DESCRIBE method handler
			class Describe
			: public Url
			, public Factory::Multiton< Method::Base, Describe, DESCRIBE >
			{
			protected:
				//! sdp description
				string _description;

				//! ctor
				Describe(  );
				friend class Factory::Multi< Method::Base, Describe >;
			public:
				//! check Accept and Require header; load media descriptors
				virtual void prepare() throw( RTSP::Exception::ManagedError );
				//! DESCRIBE reply with SDP content
				virtual string getReply() throw( RTSP::Exception::ManagedError );
			};

			//! SETUP method handler
			class Setup
			: public Session
			, public Factory::Multiton< Method::Base, Setup, SETUP >
			{
			protected:
				//! reference to RTP session setting up
				Ptr::Ref< RTP::Session > _rtp;
				//! return requested RTSP session ID or generate one if none provided
				virtual TSessionID getSessionID() const throw( RTSP::Exception::ManagedError );
				//! return RTSP session related to ID or create new if none existent
				virtual RTSP::Session & getSession() throw( RTSP::Exception::ManagedError );

				//! ctor
				Setup(  );
				friend class Factory::Multi< Method::Base, Setup >;
			public:
				//! loads transport settings from request and creates new RTP session
				virtual void prepare() throw( RTSP::Exception::ManagedError );
				//! SETUP reply
				virtual string getReply() throw( RTSP::Exception::ManagedError );
// 				virtual void execute() throw( RTSP::Exception::ManagedError );
			};

			//! PLAY method handler
			class Play
			: public Time
			, public Factory::Multiton< Method::Base, Play, PLAY >
			{
			protected:
				PlayRequest _rqRange;
				PlayRequest _rplRange;

				//! ctor
				Play(  );
				friend class Factory::Multi< Method::Base, Play >;
			public:
				virtual void prepare() throw( RTSP::Exception::ManagedError );
				virtual string getReply() throw( RTSP::Exception::ManagedError );
				virtual void execute() throw( RTSP::Exception::ManagedError );
			};

			//! PAUSE method handler
			class Pause
			: public Time
			, public Factory::Multiton< Method::Base, Pause, PAUSE >
			{
			protected:
				//! ctor
				Pause(  );
				friend class Factory::Multi< Method::Base, Pause >;
			public:
				virtual void prepare() throw( RTSP::Exception::ManagedError );
				virtual string getReply() throw( RTSP::Exception::ManagedError );
// 				virtual void execute() throw( RTSP::Exception::ManagedError );
			};

			//! TEARDOWN method handler
			class Teardown
			: public Session
			, public Factory::Multiton< Method::Base, Teardown, TEARDOWN >
			{
			protected:
				//! ctor
				Teardown(  );
				friend class Factory::Multi< Method::Base, Teardown >;
			public:				
				virtual void prepare() throw( RTSP::Exception::ManagedError );
				virtual string getReply() throw( RTSP::Exception::ManagedError );
				virtual void execute() throw( RTSP::Exception::ManagedError );
			};
		}
	}
}

#endif
