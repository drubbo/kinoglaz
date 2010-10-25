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
 * File name: ./rtsp/method.cpp
 * First submitted: 2010-01-16
 * First submitter: Emiliano Leporati <emiliano.leporati@gmail.com>
 * Contributor(s) so far - 2010-07-30 :
 *     Emiliano Leporati <emiliano.leporati@gmail.com>
 *
 * Last changes :
 *     removed RTSP::Session state concept, conflicting with non-aggregate control
 *     Kinoglaz birth: pervasive bug fixing, improvements and client support
 *     fixed some bug in Log and in insertMedia; comments
 *     log messages refactor; shared descriptor are optional now; spot insertion support
 *     ns refactor over messages; seek exception blobbed up
 *
 **/


#include <iomanip>

#include "rtsp/method.h"
#include "daemon.h"
#include "lib/clock.h"
#include "lib/log.h"
#include "lib/urlencode.h"
#include "rtsp/connection.h"
#include "rtsp/session.h"
#include "rtp/session.h"

#include <cmath>
#include <ctime>


namespace KGD
{
	namespace RTSP
	{

		namespace Method
		{
			bool SUPPORT_SEEK = false;

			//! RTSP method count
			static const uint8_t _count = 11;

			const string name[ _count ] =
			{
				"OPTIONS",
				"DESCRIBE",
				"SETUP",
				"PLAY",
				"PAUSE",
				"TEARDOWN",

				"ANNOUNCE",
				"REDIRECT",
				"RECORD",
				"GET_PARAMETER",
				"SET_PARAMETER"
			};

			ID getIDfromName( const string& n ) throw( KGD::Exception::NotFound )
			{
				for( size_t i = 0; i < _count; ++i )
				{
					if ( name[i] == n )
						return ID(i);
				}

				throw KGD::Exception::NotFound( n );
			}

			
			// ****************************************************************************************************************

			Base::Base( )
			{
			}

			void Base::setConnection( Connection & conn )
			{
				_conn = conn;
				_conn->lock();
			}

			Base::~Base()
			{
				if ( _conn )
					_conn->unlock();
			}

			void Base::execute() throw( RTSP::Exception::ManagedError )
			{
			}

			string Base::getTimestamp( double t ) throw()
			{

				time_t now;
				if ( t < HUGE_VAL )
					now = time_t( t );
				else
					now = time(NULL);

				struct tm *formattable = (struct tm*)gmtime(&now);
				char rt[36];
				strftime(rt, 36, "Date: %a, %d %b %Y %H:%M:%S GMT", formattable);

				return rt;
			}

			// ****************************************************************************************************************

			Url::Url( )
			{
			}

			void Url::urlCheckValid( ) const throw( RTSP::Exception::ManagedError )
			{
				if ( ! fileExists( (SDP::Container::BASE_DIR + _url->file).c_str() ) )
					throw RTSP::Exception::ManagedError( Error::NotFound );
			}

			void Url::urlCheckTrack( ) const throw( RTSP::Exception::ManagedError )
			{
				if ( ! _url->track.empty() )
				{
					const SDP::Container & s = _conn->getDescription( _url->file );
					s.getMedium( fromString< size_t >( _url->track ) );
				}
			}

			void Url::prepare() throw( RTSP::Exception::ManagedError )
			{
				_url = _conn->getLastRequest().getUrl();
				Log::message("%s: request URL: %s", _conn->getLogName(), _url->toString().c_str() );

				this->urlCheckValid( );
				this->urlCheckTrack( );
			}

			// ****************************************************************************************************************

			Time::Time( )
			{
			}

			PlayRequest Time::getTimeRange() const throw( )
			{
				PlayRequest rt;
				rt.speed = _conn->getLastRequest().getScale();
				rt.hasScale = (rt.speed < HUGE_VAL);

				try
				{
					pair< double, double > range = _conn->getLastRequest().getTimeRange();

					rt.from = range.first;
					rt.to = range.second;
					rt.hasRange = true;
				}
				catch( RTSP::Exception::ManagedError const & e )
				{
					Log::warning( "%s: %s", _conn->getLogName(), e.what() );
				}

				Log::message( "%s: request to play %s", _conn->getLogName(), rt.toString().c_str() );

				return rt;
			}

			// ****************************************************************************************************************

			Session::Session()
			: _sessionID( 0 )
			{
			}

			TSessionID Session::getSessionID() const throw( RTSP::Exception::ManagedError )
			{
				return _conn->getLastRequest().getSessionID();
			}

			RTSP::Session & Session::getSession() throw( RTSP::Exception::ManagedError )
			{
				return _conn->getSession( _sessionID );
			}

			void Session::prepare() throw( RTSP::Exception::ManagedError )
			{
				Url::prepare();

				_sessionID = this->getSessionID();
				_rtsp = this->getSession();
				_rtsp->lock();
			}

			Session::~Session()
			{
				if ( _rtsp )
					_rtsp->unlock();
			}

			// ****************************************************************************************************************

			Options::Options( )
			{
			}

			string Options::getReply() throw( RTSP::Exception::ManagedError )
			{
				ostringstream reply;
				reply
					<< "Public: OPTIONS,DESCRIBE,SETUP,PLAY,PAUSE,TEARDOWN" << EOL
					<< "Accept-Charset: ISO-8859-1;q=1" << EOL
					<< EOL;

				return reply.str();
			}

			void Options::execute() throw( RTSP::Exception::ManagedError )
			{
				_conn->setUserAgent( _conn->getLastRequest().getUserAgent() );
			}

			// ****************************************************************************************************************

			Describe::Describe( )
			: _description( "" )
			{
			}

			void Describe::prepare() throw( RTSP::Exception::ManagedError )
			{
				Url::prepare();
				_conn->getLastRequest().checkAcceptHeader();
				_conn->getLastRequest().checkRequireHeader();

				const SDP::Container & s = _conn->loadDescription( _url->file );
				_description = s.getReply( *_url );
			}

			string Describe::getReply() throw( RTSP::Exception::ManagedError )
			{

				ostringstream reply;
				reply
					<< this->getTimestamp() << EOL
					<< "Content-Type: application/sdp" << EOL
					<< "Content-Base: " << _url->toString() << EOL
					<< "Content-Length: " << _description.size() << EOL
// 					<< "Accept-Charset: ISO-8859-1;q=1" << EOL
					<< EOL
					<< _description;

				return reply.str();
			}

			// ****************************************************************************************************************

			Setup::Setup( )
			{
			}

			TSessionID Setup::getSessionID() const throw( RTSP::Exception::ManagedError )
			{
				try
				{
					// session already set up
					return Session::getSessionID();
				}
				catch( RTSP::Exception::ManagedError )
				{
					// generate random
					TSessionID result;
					while( (result = TSessionID(random()) ) == 0 );
					return result;
				}
			}

			RTSP::Session & Setup::getSession() throw( RTSP::Exception::ManagedError )
			{
				try
				{
					return _conn->getSession( _sessionID );
				}
				catch( RTSP::Exception::ManagedError const & )
				{
					return _conn->createSession( _sessionID );
				}
			}

			void Setup::prepare() throw( RTSP::Exception::ManagedError )
			{
				Session::prepare();
				// create RTP session
				Channel::Description cDesc = _conn->getLastRequest().getTransport();
				_rtp = _rtsp->createSession( *_url, cDesc );
			}


			string Setup::getReply() throw( RTSP::Exception::ManagedError )
			{
				ostringstream reply, ssrc;

				ssrc << setfill('0') << setw(8) << _rtp->getSsrc();

				Channel::Description rtp = _rtp->RTPgetDescription();
				Channel::Description rtcp = _rtp->RTCPgetDescription();

				Socket & sock = _conn->getSocket();
				switch( rtp.type )
				{
				case Channel::Owned:
					reply
						<< this->getTimestamp() << EOL
						<< "Session: " << _sessionID << EOL
						<< "Transport: RTP/AVP;unicast;"
						<< "source=" <<  sock.getLocalHost() << ";"
						<< "destination=" << sock.getRemoteHost() << ";"
						<< "client_port=" << rtp.ports.second << "-" << rtcp.ports.second << ";"
						<< "server_port=" << rtp.ports.first << "-" << rtcp.ports.first << ";"
						<< "ssrc=" << ssrc << EOL
						<< EOL;
					break;

				case Channel::Shared:
					reply
						<< this->getTimestamp() << EOL
						<< "Session: " << _sessionID << EOL
						<< "Transport: RTP/AVP/TCP;"
						<< "source=" << sock.getLocalHost() << ";"
						<< "destination=" << sock.getRemoteHost() << ";"
						<< "interleaved=" << rtp.ports.first << "-" << rtcp.ports.first << ";"
						<< "ssrc=" << ssrc << EOL
						<< EOL;
					break;
				}

				return reply.str();
			}

// 			void Setup::execute() throw( RTSP::Exception::ManagedError )
// 			{
// 			}

			// ****************************************************************************************************************

			Play::Play( )
			{
			}

			void Play::prepare() throw( RTSP::Exception::ManagedError )
			{
				Session::prepare();

				// get and check range and speed
				_rqRange = this->getTimeRange();
				// Range header
				if ( _rqRange.hasRange )
				{
					// just the first play can have range if we don't support seek
					if ( ! SUPPORT_SEEK && _rtsp->hasPlayed() )
						throw RTSP::Exception::ManagedError( Error::BadRequest );
					// playing reverse should have inverted range
					else if ( (_rqRange.to - _rqRange.from) * sign( _rqRange.speed ) < 0)
					{
						Log::error( "%s: play request time range misordered: %s", _conn->getLogName(), _rqRange.toString().c_str() );
						throw RTSP::Exception::ManagedError( Error::BadRequest );
					}
				}
				// Scale header
				else if ( _rqRange.hasScale )
				{
					if ( _rqRange.speed == 0.0 )
					{
						Log::error( "%s: play request with invalid scale %f", _conn->getLogName(), _rqRange.speed );
						throw RTSP::Exception::ManagedError( Error::BadRequest );
					}
				}
				// first time we need a range
				else if ( ! _rtsp->hasPlayed() )
				{
					Log::error( "%s: play request without mandatory time range", _conn->getLogName() );
					throw RTSP::Exception::ManagedError( Error::BadRequest );
				}

				// some useful info were given, set up the new scenario
				if ( _rqRange.hasRange || _rqRange.hasScale )
				{
					// pre-play
					if ( _url->track.empty() )
						_rplRange = _rtsp->play( _rqRange );
					else
						_rplRange = _rtsp->getSession( _url->track ).play( _rqRange );
				}
			}

			string Play::getReply() throw( RTSP::Exception::ManagedError )
			{
				ostringstream reply;

				reply << this->getTimestamp( _rplRange.time ) << EOL;
				if ( _rplRange.hasRange )
				{
					reply << "Range: npt=" << setprecision(3) << fixed << _rplRange.from << "-";
					if ( SUPPORT_SEEK && _rplRange.to < HUGE_VAL )
						reply << setprecision(3) << fixed << _rplRange.to << EOL;
					else
						reply << EOL;
				}
				reply
					<< "Scale: " << setprecision(3) << fixed << _rplRange.speed << EOL
					<< "Session: " << _sessionID << EOL
					<< "RTP-Info: ";

				// aggregate
				if ( _url->track.empty() )
				{
					const RTSP::Session & rtspSess = _rtsp.get();
					ref_list< const RTP::Session > rtpSessions = rtspSess.getSessions();
					size_t i = 0, n = rtpSessions.size();
					BOOST_FOREACH( const RTP::Session & sess, rtpSessions )
					{
						reply
							<< "url=" << sess.getUrl().toString() << ";"
							<< "seq=" << sess.getStartSeq() << ";"
							<< "rtptime=" << sess.getTimeline().getRTPtime( _rplRange.from, _rplRange.time )
							<< ( i ++ < n - 1 ? "," : EOL );
					}
				}
				// single
				else
				{
					const RTP::Session & sess = _rtsp->getSession( _url->track );
					reply
						<< "url=" << sess.getUrl().toString() << ";"
						<< "seq=" << sess.getStartSeq() << ";"
						<< "rtptime=" << sess.getTimeline().getRTPtime( _rplRange.from, _rplRange.time )
						<< EOL;
				}
				reply << EOL;

				return reply.str();
			}

			void Play::execute() throw( RTSP::Exception::ManagedError )
			{
				// play effective
				if ( _rqRange.hasRange || _rqRange.hasScale )
				{
					if ( _url->track.empty() )
						_rtsp->play( );
					else
						_rtsp->getSession( _url->track ).play( );
				}
				// just toggle pause
				else
				{
					if ( _url->track.empty() )
						_rtsp->unpause( _rqRange );
					else
						_rtsp->getSession( _url->track ).unpause( _rqRange );
				}
			}

			// ****************************************************************************************************************

			Pause::Pause( )
			{
			}

			void Pause::prepare() throw( RTSP::Exception::ManagedError )
			{
				Session::prepare();

				// dummy request to set up current time
				PlayRequest rq;// = this->getTimeRange();

				if ( _url->track.empty() )
					_rtsp->pause( rq );
				else
					_rtsp->getSession( _url->track ).pause( rq );
			}

			string Pause::getReply() throw( RTSP::Exception::ManagedError )
			{

				ostringstream reply;
				reply
					<< this->getTimestamp() << EOL
					<< "Session: " << _sessionID << EOL
					<< EOL;

				return reply.str();
			}

// 			void Pause::execute() throw( RTSP::Exception::ManagedError )
// 			{
// 
// 			}

			// ****************************************************************************************************************

			Teardown::Teardown( )
			{
			}

			void Teardown::prepare() throw( RTSP::Exception::ManagedError )
			{
				if (! _conn->hasSessions() )
					throw RTSP::Exception::ManagedError( Error::SessionNotFound );

				Session::prepare();
			}

			std::string Teardown::getReply() throw( RTSP::Exception::ManagedError )
			{
				ostringstream reply;

				reply
					<< this->getTimestamp() << EOL
					<< "Session: " << _sessionID << EOL
					<< EOL;

				return reply.str();
			}

			void Teardown::execute() throw( RTSP::Exception::ManagedError )
			{
				if ( _url->track.empty() )
				{
					_rtsp->unlock();
					_rtsp.invalidate();
					_conn->removeSession( _sessionID );
				}
				else
					_rtsp->removeSession( _url->track );
			}
		}
	}
}

