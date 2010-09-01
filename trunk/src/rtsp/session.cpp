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

#include <config.h>

#include "lib/utils/container.hpp"
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
			_sessions.clear();
		}

		bool Session::hasPlayed() const throw()
		{
			return _playIssued;
		}

		const char * Session::getLogName() const throw()
		{
			return _logName.c_str();
		}

		void Session::reply( const Error::Definition & d ) throw( KGD::Socket::Exception )
		{
			_conn->reply( d );
		}

		RTP::Session & Session::createSession(const Url &url, const Channel::Description & remote ) throw( RTSP::Exception::ManagedError )
		{
			try
			{
				// setup channel
				Ptr::Scoped< Channel::Bi > rtpChan, rtcpChan;
				TPortPair local;

				// udp
				if ( remote.type == Channel::Owned )
				{
					local = Port::Udp::getInstance().getPair();

					Log::debug("%s: creating udp socket locally bound to %d / %d", getLogName(), local.first, local.second );
					Ptr::Scoped< KGD::Socket::Udp >
						rtp = new KGD::Socket::Udp( local.first, url.host ),
						rtcp = new KGD::Socket::Udp( local.second, url.host );

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
					rtcp->setReadTimeout( RTCP::Receiver::POLL_INTERVAL );
					rtcp->setReadBlock( true );

					rtpChan = rtp.release();
					rtcpChan = rtcp.release();
				}
				// tcp interleaved
				else
				{
					Socket & rtsp = _conn->getSocket();
					local = rtsp.addInterleavePair( remote.ports, _id );

					rtpChan = rtsp.getInterleave( local.first );
					rtcpChan = rtsp.getInterleave( local.second );
				}
				
				// get description
				int mediumIndex = fromString< int >( url.track );
				SDP::Medium::Base & med = _conn->getDescription( url.file ).getMedium( mediumIndex );

				// create session
				RTP::Session *s = new RTP::Session( url, med, rtpChan.release(), rtcpChan.release(), getLogName(), _conn->getUserAgent() );
				_sessions( url.track ) = s;

				Log::debug("%s: RTP session created / track: %s / ports: RTP %d %d - RTCP %d %d"
					, getLogName()
					, url.track.c_str()
					, local.first
					, remote.ports.first
					, local.second
					, remote.ports.second );

				return *s;
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
				Log::debug( "%s: pre-play", getLogName() );
				// guess common parameters
				PlayRequest rt( rq );
				TSessionMap::Iterator it( _sessions );
				for( ; it.isValid(); it.next() )
					rt.merge( it.val().eval( rq ) );

				Log::debug( "%s: play setup with %s", getLogName(), rt.toString().c_str() );
				// set it up
				for( it.first(); it.isValid(); it.next() )
					it.val().play( rt );

				_playIssued = true;
				return rt;
			}
			catch( const KGD::Exception::OutOfBounds & e )
			{
				Log::error( "%s: %s", getLogName(), e.what() );
				throw RTSP::Exception::ManagedError( Error::BadRequest );
			}
		}

		void Session::insertMedia( SDP::Container & media, double curMediaTime ) throw( KGD::Exception::OutOfBounds )
		{
			if ( _sessions.empty() )
			{
				Log::error( "%s: media insert: no sessions to add to", getLogName() );
				return;
			}

			TSessionMap::Iterator sessIt( _sessions );
			// list of sessions forced to pause
			list< string > pausedSessions;
			PlayRequest pauseRq;
			// pause all RTP sessions
			for( ; sessIt.isValid(); sessIt.next() )
			{
				if ( sessIt.val().isPlaying() )
				{
					Log::message( "%s: media insert: pause %s", getLogName(), sessIt.key().c_str() );
					pausedSessions.push_back( sessIt.key() );
					sessIt.val().pause( pauseRq );
				}
			}

			// find the suitable time to insertion
			double insertTime = HUGE_VAL;
			// video medium will lead
			Log::debug( "%s: media insert: search video track", getLogName() );
			for( sessIt.first(); sessIt.isValid(); sessIt.next() )
			{
				if ( sessIt.val().getDescription().getType() == SDP::MediaType::Video )
				{
					insertTime = sessIt.val().evalMediumInsertion( curMediaTime );
					break;
				}
			}
			// not found, use first medium
			if ( insertTime == HUGE_VAL )
				insertTime = sessIt.first().val().evalMediumInsertion( curMediaTime );
			Log::debug( "%s: media insert: time to insert at is %lf", getLogName(), insertTime );

			// insert mediums with correspondent payload type
			// get media to insert
			list< Ptr::Ref< SDP::Medium::Base > > ml = media.getMedia();
			// for every local medium
			for( sessIt.first(); sessIt.isValid(); sessIt.next() )
			{
				SDP::Medium::Base & sessMed = sessIt.val().getDescription();
				Log::debug( "%s: media insert: look for payload type %u", getLogName(), sessMed.getPayloadType() );
				// search correspondent medium to insert
				bool found = false;
				for( Ctr::Iterator< list< Ptr::Ref< SDP::Medium::Base > > > it( ml ); !found && it.isValid(); it.next() )
				{
					// if same PT, insert
					if ( sessMed.getPayloadType() == it->get().getPayloadType() )
					{
						Log::debug("%s: media insert: found payload type %u", getLogName(), it->get().getPayloadType() );
						sessIt.val().insertMedium( it->get(), insertTime );
						found = true;
					}
				}
				// no correspondence found, add void
				if ( !found )
				{
					Log::debug( "%s: media insert: add void to payload type %u", getLogName(), sessMed.getPayloadType() );
					sessIt.val().insertTime( media.getDuration(), insertTime );
				}
			}

			// restart paused sessions
			for( Ctr::Iterator< list< string > > pauseIt( pausedSessions ); pauseIt.isValid(); pauseIt.next() )
			{
				Log::message( "%s: media insert: unpause %s", getLogName(), pauseIt->c_str() );
				_sessions( *pauseIt )->unpause( pauseRq );
			}
		}

		void Session::play() throw()
		{
			Log::debug( "%s: play effective", getLogName() );

			for( TSessionMap::Iterator it( _sessions ); it.isValid(); it.next() )
				it.val().play();
		}

		void Session::pause( const PlayRequest & rq ) throw()
		{
			Log::debug( "%s: pause", getLogName() );

			for( TSessionMap::Iterator it( _sessions ); it.isValid(); it.next() )
				it.val().pause( rq );
		}

		void Session::unpause( const PlayRequest & rq ) throw()
		{
			Log::debug( "%s: unpause", getLogName() );

			for( TSessionMap::Iterator it( _sessions ); it.isValid(); it.next() )
				it.val().unpause( rq );
		}

		list< Ptr::Ref< RTP::Session > > Session::getSessions() throw()
		{
			list< RTP::Session * > l = _sessions.values< list >();
			return Ctr::toRef( l );
		}

		list< Ptr::Ref< const RTP::Session > > Session::getSessions() const throw()
		{
			list< RTP::Session * > l = _sessions.values< list >();
			return Ctr::toConstRef( l );
		}

		RTP::Session & Session::getSession( const string & track ) throw( RTSP::Exception::ManagedError )
		{
			if ( _sessions.has( track ) )
				return *_sessions( track );
			else
				throw RTSP::Exception::ManagedError( Error::NotFound );
		}
		
		const RTP::Session & Session::getSession( const string & track ) const throw( RTSP::Exception::ManagedError )
		{
			if ( _sessions.has( track ) )
				return _sessions[ track ];
			else
				throw RTSP::Exception::ManagedError( Error::NotFound );
		}

		void Session::removeSession( const string & track ) throw( RTSP::Exception::ManagedError )
		{
			if ( _sessions.has( track ) )
				_sessions.erase( track );
			else
				throw RTSP::Exception::ManagedError( Error::NotFound );
		}
		
		const TSessionID & Session::getID() const throw()
		{
			return _id;
		}
		RTSP::Connection & Session::getConnection() throw( KGD::Exception::NullPointer )
		{
			return _conn.get();
		}
		const RTSP::Connection & Session::getConnection() const throw( KGD::Exception::NullPointer )
		{
			return _conn.get();
		}
	}

}
