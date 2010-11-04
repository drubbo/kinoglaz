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
 * File name: ./rtsp/message.h
 * First submitted: 2010-02-07
 * First submitter: Emiliano Leporati <emiliano.leporati@gmail.com>
 * Contributor(s) so far - 2010-07-30 :
 *     Emiliano Leporati <emiliano.leporati@gmail.com>
 *
 * Last changes :
 *     Kinoglaz birth: pervasive bug fixing, improvements and client support
 *     ns refactor over messages; seek exception blobbed up
 *     sdp debugged
 *     interleave ok
 *     pre-interleave
 *
 **/


#ifndef __KGD_RTSP_MESSAGE_H
#define __KGD_RTSP_MESSAGE_H

#include "rtsp/common.h"
#include "rtsp/exceptions.h"
#include "rtsp/method.h"
#include "lib/array.hpp"
#include "lib/socket.h"

#include <utility>

using namespace std;

namespace KGD
{
	namespace RTSP
	{
		//! RTSP message types and parsers
		namespace Message
		{
			//! RTSP message parser
			class Parser
			: public CharArray
			{
			protected:
				TCseq _cseq;

				Parser( const char *, size_t ) throw( );

				void checkVersion( const string & version ) const throw( RTSP::Exception::ManagedError );

				pair< vector< string >, size_t > extractData( string::const_iterator bg, string::const_iterator ed, const string & rx ) const throw( KGD::Exception::NotFound );
				vector< string > extractData( const string & rx ) const throw( KGD::Exception::NotFound );

				pair< vector< string >, size_t > extractHeaderData( const string & header, size_t offset , const string & rx = "(.+?)") const throw( KGD::Exception::NotFound );
				vector< string > extractHeaderData( const string & header, const string & rx = "(.+?)" ) const throw( KGD::Exception::NotFound );
			public:
				TCseq getCseq() const throw();
			};

			//! RTSP request message
			class Request
			: public Parser
			{
			protected:
				
				Method::ID _method;
				boost::scoped_ptr< Url > _url;

				void loadUrl( const string & url, const string & remoteHost ) throw();
			public:
				Request( const char * data, size_t sz, const string & remoteHost ) throw( RTSP::Exception::ManagedError, KGD::Exception::NotFound );
				//! throws exception if present a Require header, unimplemented
				void checkRequireHeader() const throw( RTSP::Exception::ManagedError );
				//! throws exception if Accept header is present and does not containt application/sdp
				void checkAcceptHeader() const throw( RTSP::Exception::ManagedError );

				//! returns method id
				Method::ID getMethodID() const throw();
				//! returns message sequence number
				TCseq getCseq() const throw();
				//! returns identified user agent
				UserAgent::type getUserAgent() const throw( );

				//! returns decoded url of last request
				const Url & getUrl() const throw( RTSP::Exception::ManagedError );
				//! returns session id of last request
				TSessionID getSessionID() const throw( RTSP::Exception::ManagedError );
				//! returns request time range
				pair< double, double > getTimeRange() const throw( RTSP::Exception::ManagedError );
				//! returns request speed
				double getScale() const throw( );


				//! returns RTP / RTCP port pair
				pair< Channel::Description, boost::optional< TSSrc > > getTransport( ) const throw( RTSP::Exception::ManagedError );
			};

			//! RTSP response message
			class Response
			: public Parser
			{
			protected:
				//! error code
				Error::TCode _code;
			public:
				Response( const char *, size_t ) throw( RTSP::Exception::CSeq, KGD::Exception::NotFound );
				//! returns error code
				Error::TCode getCode() const throw();
			};
			
		}
	}
}


#endif
