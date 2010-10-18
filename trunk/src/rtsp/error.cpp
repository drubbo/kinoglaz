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
 * File name: ./rtsp/error.cpp
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


#include "rtsp/error.h"
#include "lib/log.h"
#include "lib/common.h"

namespace KGD
{
	namespace RTSP
	{
		namespace Error
		{
			boost::scoped_ptr< map< TCode, ref< const Definition > > > Definition::_all( new map< TCode, ref< const Definition > >() );
			Mutex Definition::_allMux;

			Definition::Definition( const TCode code, const string & description )
			: _code( code ), _description( description )
			{
				Lock lk( _allMux );
				_all->insert( make_pair( code, *this ) );
			}

			TCode Definition::getCode() const throw()
			{
				return _code;
			}

			const string & Definition::getDescription() const throw()
			{
				return _description;
			}

			bool Definition::isError() const throw()
			{
				return _code > 202;
			}

			const Definition & Definition::getDefinition( const TCode code ) throw( KGD::Exception::NotFound )
			{
				Lock lk( _allMux );
				try
				{
					return *_all->at( code );
				}
				catch( boost::bad_ptr_container_operation )
				{
					throw Exception::NotFound( "RTSP error definition for code " + KGD::toString( code ) );
				}
			}


			const Definition Continue(100, "Continue");
			const Definition Ok(200, "OK");
			const Definition Created(201, "Created");
			const Definition Accepted(202, "Accepted");
			const Definition NonAuthInfo(203, "Non-Authoritative Information");
			const Definition NoContent(204, "No Content");
			const Definition ResetContent(205, "Reset Content");
			const Definition PartialContent(206, "Partial Content");
			const Definition MultipleChoices(300, "Multiple Choices");
			const Definition MovedPermanently(301, "Moved Permanently");
			const Definition MovedTemporarily(302, "Moved Temporarily");
			const Definition BadRequest(400, "Bad Request");
			const Definition Unauthorized(401, "Unauthorized");
			const Definition PaymentRequired(402, "Payment Required");
			const Definition Forbidden(403, "Forbidden");
			const Definition NotFound(404, "Not Found");
			const Definition MethodNotAllowed(405, "Method Not Allowed");
			const Definition NotAcceptable(406, "Not Acceptable");
			const Definition ProxyAuthRequired(407, "Proxy Authentication Required");
			const Definition RequestTimeOut(408, "Request Time-out");
			const Definition Conflict(409, "Conflict");
			const Definition Gone(410, "Gone");
			const Definition LengthRequired(411, "Length Required");
			const Definition PreconditionFailed(412, "Precondition Failed");
			const Definition RequestEntityTooLarge(413, "Request Entity Too Large");
			const Definition RequestURITooLarge(414, "Request-URI Too Large");
			const Definition UnsupportedMediaType(415, "Unsupported Media Type");
			const Definition BadExtension(420, "Bad Extension");
			const Definition InvalidParameter(450, "Invalid Parameter");
			const Definition ParameterNotUnderstood(451, "Parameter Not Understood");
			const Definition ConferenceNotFound(452, "Conference Not Found");
			const Definition NotEnoughBandwidth(453, "Not Enough Bandwidth");
			const Definition SessionNotFound(454, "Session Not Found");
			const Definition InvalidMethodInState(455, "Method Not Valid In This State");
			const Definition HeaderFieldNotValidForResource(456, "Header Field Not Valid for Resource");
			const Definition InvalidRange(457, "Invalid Range");
			const Definition ParameterIsReadOnly(458, "Parameter Is Read-Only");
			const Definition AggregateNotAllowed(459, "Aggregate Operation Not Allowed");
			const Definition AggregateOnly(460, "Only Aggregate Operation Allowed");
			const Definition UnsupportedTransport(461, "Unsupported Transport");
			const Definition InternalServerError(500, "Internal Server Error");
			const Definition NotImplemented(501, "Not Implemented");
			const Definition BadGateway(502, "Bad Gateway");
			const Definition ServiceUnavailable(503, "Service Unavailable");
			const Definition GatewayTimeOut(504, "Gateway Time-out");
			const Definition VersionNotSupported(505, "RTSP Version Not Supported");
			const Definition OptionNotSupported(551, "Option not supported");
			const Definition RequireTransportSettings(416, "Require Transport Settings");

		}
	}

}
