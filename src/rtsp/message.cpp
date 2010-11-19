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
 * File name: src/rtsp/message.cpp
 * First submitted: 2010-07-30
 * First submitter: Emiliano Leporati <emiliano.leporati@gmail.com>
 * Contributor(s) so far - 2010-11-04 :
 *     Emiliano Leporati <emiliano.leporati@gmail.com>
 *
 * Last changes :
 *     fixed some SSRC issues; added support for client-hinted ssrc; fixed SIGTERM shutdown when serving
 *     boosted
 *     Some cosmetics about enums and RTCP library
 *     source import
 *
 **/


#include "rtsp/message.h"
#include "rtsp/common.h"
#include "rtsp/header.h"
#include "rtsp/exceptions.h"
#include "lib/log.h"
#include "lib/urlencode.h"
#include "lib/socket.h"
#include <cstring>
#include <cstdlib>
#include <cstdio>

#include <boost/regex.hpp>
#include <boost/algorithm/string.hpp>


using namespace std;

namespace KGD
{
	namespace RTSP
	{
		namespace Message
		{

			// ***********************************************************************************************************************************

			Parser::Parser( const char * data , size_t sz ) throw( )
			: CharArray( sz + 1 )
			, _cseq( 0 )
			{
				CharArray::set( data, sz, 0 );
				CharArray::set< char >( 0, sz );
			}

			pair< vector< string >, size_t > Parser::extractData( string::const_iterator bg, string::const_iterator ed, const string & rx ) const throw( KGD::Exception::NotFound )
			{
				boost::regex rxSearch( rx );
				boost::match_results< string::const_iterator > match;
				if (boost::regex_search(bg, ed, match, rxSearch))
				{
					vector< string > rt;
					for( size_t i = 1; i < match.size(); ++i )
						rt.push_back( match.str(i) );

					return make_pair( rt, match.position() + match.length() );
				}
				else
					throw KGD::Exception::NotFound( rx );
			}
			vector< string > Parser::extractData( const string & rx ) const throw( KGD::Exception::NotFound )
			{
				string data( _ptr );
				return this->extractData( data.begin(), data.end(), rx ).first;
			}

			vector< string > Parser::extractHeaderData( const string & header, const string & rx ) const throw( KGD::Exception::NotFound )
			{
				return this->extractData( EOL + header + ": " + rx + EOL );
			}

			pair< vector< string >, size_t > Parser::extractHeaderData( const string & header, size_t offset , const string & rx) const throw( KGD::Exception::NotFound )
			{
				string data( _ptr );
				pair< vector< string >, size_t > rt = this->extractData( data.begin() + offset, data.end(), EOL + header + ": " + rx + EOL );
				return make_pair( rt.first, rt.second + offset );
			}

			void Parser::checkVersion( const string & v ) const throw( RTSP::Exception::ManagedError )
			{
				if ( v != "1.0")
				{
					if ( ! v.empty() )
						Log::error("RTSP: unsupported version %s", v.c_str());
					throw RTSP::Exception::ManagedError( Error::VersionNotSupported );
				}
			}

			TCseq Parser::getCseq() const throw()
			{
				return _cseq;
			}

			// ***********************************************************************************************************************************
			
			Request::Request( const char * data, size_t sz, const string & remoteHost ) throw( RTSP::Exception::ManagedError, KGD::Exception::NotFound )
			: Parser( data, sz )
			{
				// first row must contain method, url and rtsp version
				// <method> <url> RTSP/1.0
				vector< string > requestData = this->extractData( "^\\s*(\\w+) (.+) RTSP/(\\d+\\.\\d+)" + EOL );
				this->checkVersion( requestData[2] );
				// check method
				try
				{
					_method = Method::getIDfromName( requestData[0] );
				}
				catch( const KGD::Exception::NotFound & e )
				{
					Log::error("RTSP: unknown method %s", e.what() );
					throw RTSP::Exception::ManagedError( Error::BadRequest );
				}

				// scan other rows to get CSeq
				try
				{
					string cseqData = this->extractHeaderData( Header::Cseq, "(\\d+)" ).at(0);
					_cseq = fromString< TCseq >( cseqData );

					// ok, update request URL
					this->loadUrl( Url::decode( requestData[1] ), remoteHost );
				}
				catch( KGD::Exception::NotFound )
				{
					Log::error("RTSP: no Cseq header");
					throw RTSP::Exception::ManagedError( Error::BadRequest );
				}
			}

			void Request::loadUrl( const string & url, const string & remoteHost ) throw()
			{
				auto_ptr< Url > u( new Url() );
				u->remoteHost = remoteHost;

				if ( url.find( "rtsp://" ) == 0 )
				{
					string urlData = url.substr( 7 );
					vector< string > urlParts = split("/", urlData), pathParts;
					pair< string, string > hostPort = split2(":", urlParts[0]);
					u->host = hostPort.first;
					u->port = (hostPort.second == "" ? 554 : fromString< TPort >( hostPort.second ));

					for( size_t i = 1; i < urlParts.size(); ++i )
					{
						// skip null path components
						if ( urlParts[i].size() )
						{
							string::const_iterator start = urlParts[i].begin(), end = urlParts[i].end();
							boost::regex rxTrack("^tk=(\\d+)$");
							boost::match_results< string::const_iterator > match;
							if (boost::regex_search(start, end, match, rxTrack))
								u->track = match.str(1);
							else
								pathParts.push_back( urlParts[i] );
						}
					}
					u->file = join("/", pathParts);
				}

				_url.reset( u.release() );
			}


			Method::ID Request::getMethodID() const throw()
			{
				return _method;
			}

			TCseq Request::getCseq() const throw()
			{
				return _cseq;
			}

			const Url & Request::getUrl( ) const throw( RTSP::Exception::ManagedError )
			{
				if ( _url )
					return *_url;
				else
					throw RTSP::Exception::ManagedError( Error::BadRequest );
			}

			// ***********************************************************************************************************************************

			void Request::checkRequireHeader() const throw( RTSP::Exception::ManagedError )
			{
				if ( string( _ptr ).find( Header::Require ) != string::npos )
				{
					Log::error("RTSP: header \"Require\" not supported");
					throw RTSP::Exception::ManagedError( Error::NotImplemented );
				}
			}

			void Request::checkAcceptHeader() const throw( RTSP::Exception::ManagedError )
			{
				try
				{
					string mimeData = this->extractHeaderData( Header::Accept ).at(0);
// 					Log::debug("RTSP: client accepts %s", mimeData.c_str());
					vector< string > mimes = split( ", ", mimeData );
					bool found = false;
					for( size_t i = 0; i < mimes.size(); ++i )
					{
						if ( mimes[i] == "application/sdp" )
						{
							found = true;
							break;
						}
					}
					if ( !found )
					{
						Log::error("RTSP: accepted mime types does not includes \"application/sdp\", no other is supported");
						throw RTSP::Exception::ManagedError( Error::NotImplemented );
					}
				}
				// nothing to do
				catch( KGD::Exception::NotFound ) { }
			}

			// ***********************************************************************************************************************************

			TSessionID Request::getSessionID() const throw( RTSP::Exception::ManagedError )
			{
				try
				{
					string sessionData = this->extractHeaderData( Header::Session ).at(0);
					return fromString< TSessionID >( sessionData );
				}
				catch( KGD::Exception::NotFound )
				{
					Log::error("RTSP: no Session header");
					throw RTSP::Exception::ManagedError( Error::BadRequest );
				}
			}

			double Request::getScale() const throw( )
			{
				try
				{
					double rt;
					vector< string > scaleData = this->extractHeaderData( Header::Scale, "(\\+|-)?\\s*(\\d+\\.\\d+)" );
					rt = fromString< double >( scaleData.at(1) );
					if ( scaleData.at(0) == "-" )
						rt = -rt;
// 					Log::debug("RTSP: Scale: %lf", rt);
					return rt;
				}
				catch( KGD::Exception::NotFound )
				{
					return HUGE_VAL;
				}
			}

			UserAgent::type Request::getUserAgent() const throw( )
			{
				try
				{
					string ua = this->extractHeaderData( Header::UserAgent ).at(0);
					if ( ua == "VLC media player (LIVE555 Streaming Media v2008.07.24)" )
						return UserAgent::VLC_1_0_2;
					else if ( ua == "VLC media player (LIVE555 Streaming Media v2010.02.10)" )
						return UserAgent::VLC_1_0_6;
					else if ( ua == "LibVLC/1.1.4 (LIVE555 Streaming Media v2010.04.09)")
						return UserAgent::VLC_1_1_4;
					else
						return UserAgent::Generic;
				}
				catch( KGD::Exception::NotFound )
				{
					return UserAgent::Generic;
				}
			}

			pair< double, double > Request::getTimeRange() const throw( RTSP::Exception::ManagedError )
			{
				try
				{
					vector< string > rangeData = this->extractHeaderData( Header::Range, "(.+)=(now|\\d+|\\d+\\.\\d+)-(\\d+\\.\\d+)?" );

					if ( rangeData[0] != "npt" )
						throw RTSP::Exception::ManagedError( Error::NotImplemented );

					pair< double, double > result( HUGE_VAL, HUGE_VAL );
					if ( rangeData[1] != "now" )
						result.first = fromString< double >( rangeData[1] );
					if ( rangeData.size() > 2 && rangeData[2].size() )
						result.second = fromString< double >( rangeData[2] );

					return result;
				}
				catch( KGD::Exception::NotFound )
				{
					Log::error("RTSP: no Range header");
					throw RTSP::Exception::ManagedError( Error::BadRequest );
				}
			}

			pair< Channel::Description, boost::optional< TSSrc > > Request::getTransport( ) const throw( RTSP::Exception::ManagedError )
			{
				pair< Channel::Description, boost::optional< TSSrc > > rt;
				try
				{
					string data = this->extractHeaderData( Header::Transport ).at(0);
					vector< string > sets = split( ",", data );
					for( size_t s = 0; s < sets.size(); ++s )
					{
						vector< string > parts = split( ";", sets[s] );
						if ( parts.empty() )
							continue;

						string portParam;
						// RTP/AVP(/UDP)
						if ( parts[0] == "RTP/AVP" || parts[0] == "RTP/AVP/UDP" )
						{
							// unicast needed
							if ( find( parts.begin(), parts.end(), "unicast") != parts.end() )
							{
								rt.first.type = Channel::Owned;
								portParam = "client_port";
							}
							else
							{
								Log::warning("RTSP: unsupported non-unicast transport");
								continue;
							}
								
						}
						// TCP interleaved - last type supported
						else if ( parts[0] == "RTP/AVP/TCP" )
						{
							rt.first.type = Channel::Shared;
							portParam = "interleaved";
						}
						else
						{
							Log::warning("RTSP: unsupported transport %s", parts[0].c_str());
							continue;
						}

						// client ports
						vector< string >::const_iterator cPorts =
							find_if( parts.begin(), parts.end(), boost::bind( &boost::starts_with< string, string >, _1, portParam ) );
						if ( cPorts == parts.end() )
						{
							Log::warning("RTSP: no port/channel specification");
							continue;
						}

						boost::regex rxPorts( ( portParam + "=(\\d+)-(\\d+)" ).c_str() );
						boost::match_results< string::const_iterator > match;
						if (boost::regex_search(cPorts->begin(), cPorts->end(), match, rxPorts))
						{
							rt.first.ports.first = fromString< TPort >( match.str(1) );
							rt.first.ports.second = fromString< TPort >( match.str(2) );
							// hint ssrc
							vector< string >::const_iterator cSSRC =
								find_if( parts.begin(), parts.end(), boost::bind( & boost::starts_with< string, string >, _1, "ssrc=" ) );
							if ( cSSRC != parts.end() )
								rt.second = fromString< TSSrc >(cSSRC->substr( 5 ));

							return rt;
						}
						else
							throw RTSP::Exception::ManagedError( Error::BadRequest );
					}

					throw RTSP::Exception::ManagedError( Error::UnsupportedTransport );
				}
				catch( KGD::Exception::NotFound )
				{
					Log::error("RTSP: no Transport header");
					throw RTSP::Exception::ManagedError( Error::BadRequest );
				}
			}


			// ***********************************************************************************************************************************

			Response::Response( const char * data, size_t sz ) throw( RTSP::Exception::CSeq, KGD::Exception::NotFound )
			: Parser( data, sz )
			{
				// first row contains
				// RTSP/1.0 <code> <description>
				vector< string > requestData = this->extractData( "^\\s*RTSP/(\\d+\\.\\d+) (\\d+) ([\\w|\\s]+?)" + EOL );
				this->checkVersion( requestData[0] );
				// it's a response, check cseq
				try
				{
					string cseqData = this->extractHeaderData( Header::Cseq, "(\\d+)" ).at(0);
					_cseq = fromString< TCseq >( cseqData );
					_code = fromString< uint16_t >( requestData[1] );
				}
				catch( KGD::Exception::NotFound )
				{
					throw RTSP::Exception::CSeq();
				}

			}

			Error::TCode Response::getCode() const throw()
			{
				return _code;
			}

		}
	}
}
