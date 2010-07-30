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
 * File name: ./rtsp/error.h
 * First submitted: 2010-01-16
 * First submitter: Emiliano Leporati <emiliano.leporati@gmail.com>
 * Contributor(s) so far - 2010-07-30 :
 *     Emiliano Leporati <emiliano.leporati@gmail.com>
 *
 * Last changes :
 *     Kinoglaz birth: pervasive bug fixing, improvements and client support
 *     fixed some bug in Log and in insertMedia; comments
 *     sdp debugged
 *     interleave ok
 *     added licence disclaimer
 *
 **/


#ifndef __KGD_RTSP_ERROR_H
#define __KGD_RTSP_ERROR_H

#include "lib/common.h"
#include "lib/exceptions.h"
#include "lib/utils/map.hpp"
#include "lib/utils/pointer.hpp"
#include "rtsp/common.h"
#include <map>
#include <string>

using namespace std;

namespace KGD
{
	namespace RTSP
	{
		//! RTSP standard error and response codes
		namespace Error
		{
			//! type of error codes
			typedef uint16_t TCode;
			
			//! RTSP error definition
			class Definition
			{
			private:
				//! static map to keep all definitions
				static Mutex _allMux;
				//! all definitions as reference
				static Ptr::Scoped< Ctr::Map< TCode, Ptr::Ref< Definition > > > _all;
				//! error code
				TCode _code;
				//! error description
				string _description;
			public:
				//! ctor by code and description
				Definition( const TCode, const string & );
				//! returns error code
				TCode getCode() const throw();
				//! returns error description
				const string & getDescription() const throw();
				//! tells if this is really an error ( 200 OK is not )
				bool isError() const throw();

				//! looks up for a definition responding to passed code
				static Definition & getDefinition( const TCode ) throw( KGD::Exception::NotFound );
			};

			//! code for success
			const TCode OK_CODE = 200;

			extern const Definition Continue;
			extern const Definition Ok;
			extern const Definition Created;
			extern const Definition Accepted;
			extern const Definition NonAuthInfo;
			extern const Definition NoContent;
			extern const Definition ResetContent;
			extern const Definition PartialContent;
			extern const Definition MultipleChoices;
			extern const Definition MovedPermanently;
			extern const Definition MovedTemporarily;
			extern const Definition BadRequest;
			extern const Definition Unauthorized;
			extern const Definition PaymentRequired;
			extern const Definition Forbidden;
			extern const Definition NotFound;
			extern const Definition MethodNotAllowed;
			extern const Definition NotAcceptable;
			extern const Definition ProxyAuthRequired;
			extern const Definition RequestTimeOut;
			extern const Definition Conflict;
			extern const Definition Gone;
			extern const Definition LengthRequired;
			extern const Definition PreconditionFailed;
			extern const Definition RequestEntityTooLarge;
			extern const Definition RequestURITooLarge;
			extern const Definition UnsupportedMediaType;
			extern const Definition BadExtension;
			extern const Definition InvalidParameter;
			extern const Definition ParameterNotUnderstood;
			extern const Definition ConferenceNotFound;
			extern const Definition NotEnoughBandwidth;
			extern const Definition SessionNotFound;
			extern const Definition InvalidMethodInState;
			extern const Definition HeaderFieldNotValidForResource;
			extern const Definition InvalidRange;
			extern const Definition ParameterIsReadOnly;
			extern const Definition AggregateNotAllowed;
			extern const Definition AggregateOnly;
			extern const Definition UnsupportedTransport;
			extern const Definition InternalServerError;
			extern const Definition NotImplemented;
			extern const Definition BadGateway;
			extern const Definition ServiceUnavailable;
			extern const Definition GatewayTimeOut;
			extern const Definition VersionNotSupported;
			extern const Definition OptionNotSupported;
			extern const Definition RequireTransportSettings;

/*

Status-Code  =     "100"      ; Continue
                |     "200"      ; OK
                |     "201"      ; Created
                |     "250"      ; Low on Storage Space
                |     "300"      ; Multiple Choices
                |     "301"      ; Moved Permanently
                |     "302"      ; Moved Temporarily
                |     "303"      ; See Other
                |     "304"      ; Not Modified
                |     "305"      ; Use Proxy
                |     "400"      ; Bad Request
                |     "401"      ; Unauthorized
                |     "402"      ; Payment Required
                |     "403"      ; Forbidden
                |     "404"      ; Not Found
                |     "405"      ; Method Not Allowed
                |     "406"      ; Not Acceptable
                |     "407"      ; Proxy Authentication Required
                |     "408"      ; Request Time-out
                |     "410"      ; Gone
                |     "411"      ; Length Required
                |     "412"      ; Precondition Failed
                |     "413"      ; Request Entity Too Large
                |     "414"      ; Request-URI Too Large
                |     "415"      ; Unsupported Media Type
                |     "451"      ; Parameter Not Understood
                |     "452"      ; Conference Not Found
                |     "453"      ; Not Enough Bandwidth
                |     "454"      ; Session Not Found
                |     "455"      ; Method Not Valid in This State
                |     "456"      ; Header Field Not Valid for Resource
                |     "457"      ; Invalid Range
                |     "458"      ; Parameter Is Read-Only
                |     "459"      ; Aggregate operation not allowed
                |     "460"      ; Only aggregate operation allowed
                |     "461"      ; Unsupported transport
                |     "462"      ; Destination unreachable
                |     "500"      ; Internal Server Error
                |     "501"      ; Not Implemented
                |     "502"      ; Bad Gateway
                |     "503"      ; Service Unavailable
                |     "504"      ; Gateway Time-out
                |     "505"      ; RTSP Version not supported
                |     "551"      ; Option not supported
                |     extension-code
*/
		}

	}


}

#endif
