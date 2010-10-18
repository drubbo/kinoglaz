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
 * File name: ./rtp/session.h
 * First submitted: 2010-01-16
 * First submitter: Emiliano Leporati <emiliano.leporati@gmail.com>
 * Contributor(s) so far - 2010-07-30 :
 *     Emiliano Leporati <emiliano.leporati@gmail.com>
 *
 * Last changes :
 *     removed RTSP::Session state concept, conflicting with non-aggregate control
 *     Kinoglaz birth: pervasive bug fixing, improvements and client support
 *     log messages refactor; shared descriptor are optional now; spot insertion support
 *     ns refactor over messages; seek exception blobbed up
 *     wimtv key stream
 *
 **/


#ifndef __KGD_RTP_SESSION_H
#define __KGD_RTP_SESSION_H

#include "rtp/buffer.h"
#include "rtp/header.h"
#include "rtp/chrono.h"
#include "sdp/sdp.h"
#include "lib/socket.h"
#include "lib/utils/safe.hpp"

namespace KGD
{
	namespace RTCP
	{
		class Sender;
		class Receiver;
	}

	namespace RTSP
	{
		class Session;
	}

	namespace RTP
	{
		//! Sessione RTP
		class Session
		: public boost::noncopyable
		{
		private:
			//! mutex
			mutable Mutex _mux;
			//! condition to wait to exit from pause
			Condition _condUnPause;
			//! condition to wait to enter pause
			Condition _condPaused;
			//! send thread
			Thread _th;

			//! medium combined timeline
			boost::scoped_ptr< Timeline::Medium > _time;
			//! is session paused ?
			bool _paused;
			//! is session stopped ?
			bool _stopped;
			//! playing indicator (not stopped and not paused)
			Safe::Bool _playing;
			//! seek request indicator
			bool _seeked;

			//! url, with track, to play
			Url _url;
			//! medium description
			ref< SDP::Medium::Base > _medium;
			//! frame rate evaluator
			FrameRate _fRate;

			//! RTP socket
			boost::shared_ptr< Channel::Out > _RTPsock;
			//! RTCP socket
			boost::shared_ptr< Channel::Bi > _RTCPsock;
			//! RTCP receiver for this session
			boost::scoped_ptr< RTCP::Receiver > _RTCPreceiver;
			//! RTCP sender for this session
			boost::scoped_ptr< RTCP::Sender > _RTCPsender;

			//! frame access mutex
			Mutex _frameMux;
			//! cross-method frame mutex locker
			boost::scoped_ptr< Lock > _frameLk;
			//! frame buffer
			boost::scoped_ptr< Buffer::Base > _frameBuf;
			//! next frame to send
			boost::scoped_ptr< RTP::Frame::Base > _frameNext;

			//! time elapsed on media timeline when to stop play
			double _timeEnd;

			//! random start packet sequence number
			TCseq _seqStart;
			//! current packet sequence number
			TCseq _seqCur;
			//! random ssrc
			TSSrc _ssrc;

			//! log identifier
			const string _logName;

			//! starts a new random sequence
			TCseq seqRestart();

			//! sets up the session to do first PLAY request
			RTSP::PlayRequest doFirstPlay( const RTSP::PlayRequest & ) throw( KGD::Exception::OutOfBounds );
			//! sets up the session to do successive PLAY requests
			RTSP::PlayRequest doSeekScale( const RTSP::PlayRequest & ) throw( KGD::Exception::OutOfBounds );

			//! retrieves next frame from frame buffer
			double fetchNextFrame( boost::scoped_ptr< Lock > & ) throw( RTP::Eof );

			//! packetized and sends a frame on the RTP socket, marking the frame with the specified time on media timeline
			void sendNextFrame( ) throw( KGD::Socket::Exception );

			//! logs some informations
			void logTimes() const;

			//! main loop
			void loop();
		public:
			//! ctor: given the request URL, the track descriptor, RTP / RTCP channels and the user agent
			Session( const Url &, SDP::Medium::Base &,
					 const boost::shared_ptr< Channel::Out > rtp,
					 const boost::shared_ptr< Channel::Bi > rtcp,
					 const string & parentLogName,
					 RTSP::UserAgent::type = RTSP::UserAgent::Generic );
			//! dtor
			~Session();

			//! returns log identifier for this rtp session
			const char * getLogName() const throw();

			//! tells if session is playing
			bool isPlaying() const throw();

			//! returns the description this session is streaming
			const SDP::Medium::Base & getDescription() const;
			//! returns the description this session is streaming
			SDP::Medium::Base & getDescription();
			//! returns the timeline of this session
			const Timeline::Medium & getTimeline() const;
			//! returns start sequence
			uint16_t getStartSeq() const;
			//! returns medium rate
			int getRate() const;
			//! returns SSRC
			TSSrc getSsrc() const;
			//! returns requested url
			const Url& getUrl() const;
			//! returns RTP channel description
			Channel::Description RTPgetDescription() const;
			//! returns RTCP channel description
			Channel::Description RTCPgetDescription() const;

			//! returns the time after t when another medium can be inserted
			double evalMediumInsertion( double t ) throw( KGD::Exception::OutOfBounds );
			//! inserts a new medium into current, starting not before time t
			void insertMedium( SDP::Medium::Base &, double t ) throw( KGD::Exception::OutOfBounds );
			//! inserts a delay into current medium, starting not before time t
			void insertTime( double duration, double t ) throw( KGD::Exception::OutOfBounds );

			//! evaluates a play request returning the "real" values that will be adopted by the session
			RTSP::PlayRequest eval( const RTSP::PlayRequest & ) throw( KGD::Exception::OutOfBounds );
			//! setup to play
			RTSP::PlayRequest play( const RTSP::PlayRequest & ) throw( KGD::Exception::OutOfBounds );
			//! play with last setup
			void play() throw();

			//! pause
			void pause( const RTSP::PlayRequest & ) throw();
			//! un-pause
			void unpause( const RTSP::PlayRequest & ) throw();

			//! stop
			void teardown( const RTSP::PlayRequest & ) throw();
		};
	}
}

#endif
