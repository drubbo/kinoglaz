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
 * File name: ./rtsp/session.cpp
 * First submitted: 2010-01-16
 * First submitter: Emiliano Leporati <emiliano.leporati@gmail.com>
 * Contributor(s) so far - 2010-07-30 :
 *     Emiliano Leporati <emiliano.leporati@gmail.com>
 *
 * Last changes :
 *     interleaved channels shutdown fixed
 *     removed RTSP::Session state concept, conflicting with non-aggregate control
 *     Kinoglaz birth: pervasive bug fixing, improvements and client support
 *     NOTIFY needs lowercase registration names
 *     log messages refactor; shared descriptor are optional now; spot insertion support
 *
 **/


#include <sstream>
#include <cstdlib>
#include <algorithm>

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "rtsp/session.h"
#include "rtsp/connection.h"
#include "rtp/session.h"
#include "rtcp/receiver.h"

namespace KGD
{
	namespace RTSP
	{		
		Session::Session(const TSessionID & ID, RTSP::Connection & conn)
		: _id( ID )
		, _conn( conn )
		, _playIssued( false )
		, _logName( conn.getLogName() + string(" SESS #") + toString( ID ) )
		{
		}

		Session::~Session()
		{
			Session::Lock lk( *this );
			_sessions.clear();
		}

		bool Session::hasPlayed() const throw()
		{
			Session::Lock lk( *this );
			return _playIssued;
		}

		const char * Session::getLogName() const throw()
		{
			return _logName.c_str();
		}

		void Session::reply( const Error::Definition & d ) throw( KGD::Socket::Exception )
		{
			_conn.reply( d );
		}

		RTP::Session & Session::createSession(const Url &url, const Channel::Description & remote ) throw( RTSP::Exception::ManagedError )
		{
			try
			{
				Session::Lock lk( *this );
				// setup channel
				boost::shared_ptr< Channel::Bi > rtpChan, rtcpChan;
				TPortPair local;

				// udp
				if ( remote.type == Channel::Owned )
				{
					local = Port::Udp::getInstance()->getPair();

					Log::debug("%s: creating udp socket locally bound to %d / %d", getLogName(), local.first, local.second );
					auto_ptr< KGD::Socket::Udp >
						rtp( new KGD::Socket::Udp( local.first, url.host ) ),
						rtcp( new KGD::Socket::Udp( local.second, url.host ) );

					Log::debug("%s: connecting udp socket to remote %d / %d", getLogName(), remote.ports.first, remote.ports.second);

					try
					{
						rtp->connectTo( remote.ports.first, url.remoteHost );
						rtcp->connectTo( remote.ports.second, url.remoteHost );
					}
					catch( const KGD::Socket::Exception & e )
					{
						Log::debug( "%s: socket error: %s", getLogName(), e.what() );
						throw RTSP::Exception::ManagedError( Error::UnsupportedTransport );
					}

					rtp->setWriteTimeout( 1.0 );
					rtcp->setWriteTimeout( 1.0 );
					rtcp->setReadBlock( true );

					rtpChan = rtp;
					rtcpChan = rtcp;
				}
				// tcp interleaved
				else
				{
					Socket & rtsp = _conn.getSocket();
					local = rtsp.addInterleavePair( remote.ports, _id );

					rtpChan = rtsp.getInterleave( local.first );
					rtcpChan = rtsp.getInterleave( local.second );
				}

				// get description
				int mediumIndex = fromString< int >( url.track );
				SDP::Medium::Base & med = _conn.getDescription( url.file ).getMedium( mediumIndex );

				// create session
				auto_ptr< RTP::Session > s( new RTP::Session( *this, url, med, rtpChan, rtcpChan, _conn.getUserAgent() ) );
				RTP::Session * sPtr = s.get();
				{
					string tmpTrack( url.track );
					_sessions.insert( tmpTrack, s );
				}
				

				Log::debug("%s: RTP session created / track: %s / ports: RTP %d %d - RTCP %d %d"
					, getLogName()
					, url.track.c_str()
					, local.first
					, remote.ports.first
					, local.second
					, remote.ports.second );

				return *sPtr;
			}
			catch( const KGD::Socket::Exception & e )
			{
				Log::debug( "%s: socket error: %s", getLogName(), e.what() );
				throw RTSP::Exception::ManagedError( Error::InternalServerError );
			}
			// no more ports
			catch( const KGD::Exception::NotFound & e )
			{
				Log::debug( "%s: %s", getLogName(), e.what() );
				throw RTSP::Exception::ManagedError( Error::NotEnoughBandwidth );
			}
		}

		PlayRequest Session::play( const PlayRequest & rq ) throw( RTSP::Exception::ManagedError )
		{
			try
			{
				Session::Lock lk( *this );
				Log::debug( "%s: pre-play", getLogName() );
				// guess common parameters
				PlayRequest rt( rq );
				BOOST_FOREACH( SessionMap::iterator::reference sess, _sessions )
					rt.merge( sess->second->eval( rq ) );
					

				Log::debug( "%s: play setup with %s", getLogName(), rt.toString().c_str() );
				// set it up
				BOOST_FOREACH( SessionMap::iterator::reference sess, _sessions )
					sess->second->play( rt );

				_playIssued = true;
				return rt;
			}
			catch( const KGD::Exception::OutOfBounds & e )
			{
				Log::error( "%s: %s", getLogName(), e.what() );
				throw RTSP::Exception::ManagedError( Error::BadRequest );
			}
		}

		void Session::insertMedia( SDP::Container & other, double curMediaTime ) throw( KGD::Exception::OutOfBounds )
		{
			Session::Lock lk( *this );
			if ( _sessions.empty() )
			{
				Log::error( "%s: media insert: no sessions to add to", getLogName() );
				return;
			}
			else if ( RTSP::Method::SUPPORT_SEEK )
			{
				Log::error( "%s: media insert: unsupported operation with seek support active", getLogName() );
				return;
			}

			SessionMap::iterator sessIt = _sessions.begin();
			// list of sessions forced to pause
			list< string > pausedSessions;
			PlayRequest pauseRq;
			// pause all RTP sessions
			for( ; sessIt != _sessions.end(); ++sessIt )
			{
				if ( sessIt->second->isPlaying() )
				{
					Log::message( "%s: media insert: pause %s", getLogName(), sessIt->first.c_str() );
					pausedSessions.push_back( sessIt->first );
					sessIt->second->pause( pauseRq );
				}
			}

			// find the suitable time to insertion
			double insertTime = HUGE_VAL;
			// video medium will lead
			Log::debug( "%s: media insert: search video track", getLogName() );
			BOOST_FOREACH( SessionMap::iterator::reference sess, _sessions )
			{
				if ( sess->second->getDescription().getType() == SDP::MediaType::Video )
				{
					insertTime = sess->second->evalMediumInsertion( curMediaTime );
					break;
				}
			}
			// not found, use first medium
			if ( insertTime == HUGE_VAL )
				insertTime = _sessions.begin()->second->evalMediumInsertion( curMediaTime );
			Log::debug( "%s: media insert: time to insert at is %lf", getLogName(), insertTime );

			// insert mediums with correspondent payload type
			// get media to insert
			ref_list< SDP::Medium::Base > otherMedia = other.getMedia();
			// for every local medium
			BOOST_FOREACH( SessionMap::iterator::reference sess, _sessions )
			{
				SDP::Medium::Base & sessMed = sess->second->getDescription();
				Log::debug( "%s: media insert: look for payload type %u", getLogName(), sessMed.getPayloadType() );
				// search correspondent medium to insert
				bool found = false;
				BOOST_FOREACH( SDP::Medium::Base & otherMedium, otherMedia )
				{
					// if same PT, insert
					if ( sessMed.getPayloadType() == otherMedium.getPayloadType() )
					{
						Log::debug("%s: media insert: found payload type %u", getLogName(), otherMedium.getPayloadType() );
						sess->second->insertMedium( otherMedium, insertTime );
						found = true;
						break;
					}
				}
				// no correspondence found, add void
				if ( !found )
				{
					Log::debug( "%s: media insert: add void to payload type %u", getLogName(), sessMed.getPayloadType() );
					sess->second->insertTime( other.getDuration(), insertTime );
				}
			}

			// restart paused sessions
			BOOST_FOREACH( const string & sessID, pausedSessions )
			{
				Log::message( "%s: media insert: unpause %s", getLogName(), sessID.c_str() );
				_sessions.at( sessID ).unpause( pauseRq );
			}
		}

		void Session::play() throw()
		{
			Session::Lock lk( *this );
			Log::debug( "%s: play effective", getLogName() );

			BOOST_FOREACH( SessionMap::iterator::reference sess, _sessions )
				sess->second->play();
		}

		void Session::pause( const PlayRequest & rq ) throw()
		{
			Session::Lock lk( *this );
			Log::debug( "%s: pause", getLogName() );

			BOOST_FOREACH( SessionMap::iterator::reference sess, _sessions )
				sess->second->pause( rq );
		}

		void Session::unpause( const PlayRequest & rq ) throw()
		{
			Session::Lock lk( *this );
			Log::debug( "%s: unpause", getLogName() );

			BOOST_FOREACH( SessionMap::iterator::reference sess, _sessions )
				sess->second->unpause( rq );
		}

		ref_list< RTP::Session > Session::getSessions() throw()
		{
			Session::Lock lk( *this );
			list< RTP::Session * > rt;
			BOOST_FOREACH( SessionMap::iterator::reference sess, _sessions )
				rt.push_back( sess->second );

			return rt;
		}

		ref_list< const RTP::Session > Session::getSessions() const throw()
		{
			Session::Lock lk( *this );
			list< RTP::Session const * > rt;
			BOOST_FOREACH( SessionMap::const_iterator::reference sess, _sessions )
				rt.push_back( sess->second );

			return rt;
		}

		RTP::Session & Session::getSession( const string & track ) throw( RTSP::Exception::ManagedError )
		{
			try
			{
				Session::Lock lk( *this );
				return _sessions.at( track );
			}
			catch( boost::bad_ptr_container_operation )
			{
				throw RTSP::Exception::ManagedError( Error::NotFound );
			}
		}

		const RTP::Session & Session::getSession( const string & track ) const throw( RTSP::Exception::ManagedError )
		{
			try
			{
				Session::Lock lk( *this );
				return _sessions.at( track );
			}
			catch( boost::bad_ptr_container_operation )
			{
				throw RTSP::Exception::ManagedError( Error::NotFound );
			}
		}

		void Session::removeSession( const string & track ) throw( RTSP::Exception::ManagedError )
		{
			Session::Lock lk( *this );
			Log::debug( "%s: removing RTP session %s", getLogName(), track.c_str() );
			if ( ! _sessions.erase( track ) )
				throw RTSP::Exception::ManagedError( Error::NotFound );
			else if ( _sessions.empty() )
			{
				Connection::TryLock connLk( _conn );
				if ( connLk.owns_lock() )
				{
					Log::debug( "%s: asking to remove RTSP session", getLogName() );
					_conn.removeSession( _id );
				}
			}
		}

		const TSessionID & Session::getID() const throw()
		{
			return _id;
		}
		RTSP::Connection & Session::getConnection() throw( )
		{
			return _conn;
		}
		const RTSP::Connection & Session::getConnection() const throw( )
		{
			return _conn;
		}
	}

}
