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
 * File name: ./rtp/chrono.h
 * First submitted: 2010-01-16
 * First submitter: Emiliano Leporati <emiliano.leporati@gmail.com>
 * Contributor(s) so far - 2010-07-30 :
 *     Emiliano Leporati <emiliano.leporati@gmail.com>
 *
 * Last changes :
 *     formats on their own; better Factory elements naming; separate library for every component
 *     Kinoglaz birth: pervasive bug fixing, improvements and client support
 *     fixed some bug in Log and in insertMedia; comments
 *     sdp debugged
 *     interleave ok
 *
 **/


#ifndef __KGD_RTP_CHRONO
#define __KGD_RTP_CHRONO

#include "lib/common.h"
#include "lib/utils/factory.hpp"

#include <lib/clock.h>
#include <lib/utils/pointer.hpp>
#include <rtsp/common.h>

namespace KGD
{
	namespace RTP
	{
		//! type of RTP timestamps
		typedef uint32_t TTimestamp;

		//! converts seconds to a RTP timestamp given a scale
		TTimestamp sec2ts( double sec, int rate ) throw();

		//! timeline management
		namespace Timeline
		{
			//! represents a portion of time elapsed between two points, at variable speed
			class Segment
			{
			protected:
				//! reference start time, usually absolute
				double _start;
				//! stop time, absolute relatively to the timeline of start
				double _stop;
				//! in this segment the time can run at a different speed from ours
				double _speed;
			public:
				//! void ctor, segment not running
				Segment( );
				//! constructs a running segment
				Segment( double start, double spd = 1.0 );
				//! starts the segment from time t at speed spd; successive start will have no effect, even after stops. HUGE_VAL for spd means "use default", 1.0
				void start( double t = Clock::getSec(), double spd = HUGE_VAL ) throw();
				//! stops the segment at time t; successive stop will have no effect
				void stop( double t = Clock::getSec() ) throw();
				//! tells if the segment has started but not stopped
				bool isRunning() const throw();
				//! tells segment time speed
				double getSpeed() const throw();
				//! tells the begin time of the segment
				double getBegin() const throw();
				//! tells the elapsed seconds from start at segment time speed, or 0 if not started
				double getElapsed( double t = Clock::getSec() ) const throw();
				//! tells the elapsed seconds from start at a different time speed, or 0 if not started
				double getScaledElapsed( double speed = 1.0, double t = Clock::getSec() ) const throw();
			};
			
			//! represents the result of successive segments elapsed at different speeds
			class MultiSegment
			{
			protected:
				//! the current segment, can be null before start
				Ptr::Scoped< Segment > _current;
				//! current / last speed, is default speed for new segments
				double _speed;
				//! start time
				double _start;
				//! total time elapsed in terminated segment (i.e. without the elapsed of _current)
				double _tot;
				//! total linear time ( speed 1 ) elapsed in terminated segment (i.e. without the elapsed of _current)
				double _linear;
				//! the elapsed time of the segment before _current (i.e. calculated at last stop)
				double _last;
			public:
				//! void ctor
				MultiSegment( );
				//! starts the first segment with specified params
				MultiSegment( double start, double spd = 1.0 );
				//! get speed of _current, or HUGE_VAL
				double getCurrentSpeed() const throw();
				//! get the absolute begin, or HUGE_VAL
				double getBegin() const throw();
				//! get the elapsed time, i.e. tot + current (0 is fine)
				double getElapsed( double t = Clock::getSec() ) const throw();
				//! get the last elapsed time (0 is fine)
				double getLast( double t = Clock::getSec() ) const throw();
				//! get current segment, if any
				const Segment & getCurrentSegment() throw( KGD::Exception::NullPointer );
				//! get the elapsed time in current segment
				double getCurrent( double t = Clock::getSec() ) const throw();
				//! get the stored total value: previous + current = elapsed
				double getPrevious( ) const throw();
				//! tells if there's a current segment and it's counting time (is not stopped)
				bool isRunning() const throw();
				//! first start (must never have been started). HUGE_VAL for speed means "use last". default is 1.0
				void start( double t = Clock::getSec(), double spd = HUGE_VAL ) throw( KGD::Exception::InvalidState );
				//! stops current segment
				void stop( double t = Clock::getSec() ) throw( );
				//! start next segment after a stop; can be used instead of start
				void next( double t = Clock::getSec(), double spd = HUGE_VAL) throw( );
			};

			//! seek counters
			struct Seek
			{
				//! total seeked
				double absolute;
				//! current shift
				double relative;
				//! total left shift
				double left;
				//! total right shift
				double right;

				Seek();
				//! add shift
				Seek & operator+=( double delta );
			};

			//! the combined medium timeline: RTP(t) = (LIFE(t) - PAUSED(t)) * rate
			class Medium
			: virtual public Factory::Base
			, public Factory::Multiton< Medium, Medium, RTSP::UA_UNDEFINED >
			{
			protected:
				mutable RMutex _mux;

				//! this is the lifetime segment, the linear time elapsed from first start
				Segment _life;
				//! multiple times, the medium will be paused, always at speed 1.0
				MultiSegment _pause;
				//! play intervals will be many, at different speeds
				MultiSegment _play;
				//! seek adjustements
				Seek _seek;

				//! a random value to start rtp timeline
				TTimestamp _rtpStart;
				//! the clock rate used to build rtp timestamp
				int _rate;

				//! factory ctor
				Medium();
				friend class Factory::Multi< Medium, Medium >;
			public:
				//! ctor
				Medium( int rate );
				//! dtor
				~Medium();
				//! returns rate
				int getRate() const throw();
				//! sets rate, for factory constructions
				void setRate( int rate ) throw();

				//! starts life and play timeline at certain time / speed. must have never been started
				void start( double t = Clock::getSec(), double spd = HUGE_VAL ) throw( );
				//! goes pause. must be running
				void pause( double t = Clock::getSec() ) throw( );
				//! unpauses or starts. must not be running
				void unpause( double t = Clock::getSec(), double spd = HUGE_VAL ) throw( );
				//! seeks. must not be running
				void seek( double t, double pt, double spd = HUGE_VAL ) throw( );
				//! stops everything
				void stop( double t = Clock::getSec() ) throw();

				//! returns the presentation time, i.e. the time of the frames that should be sent
				double getPresentationTime( double t = Clock::getSec() ) const throw();
				//! assigns a new random value to _rtpStart
				void restartRTPtime();
				//! returns _rtpStart
				TTimestamp getRTPbegin() const throw();
				//! RTP timestamp is function of the time spent playing
				virtual TTimestamp getRTPtime( double pt = HUGE_VAL, double t = Clock::getSec() ) const throw();

				//! returns current speed (HUGE_VAL is fine)
				double getSpeed() const throw();
				//! stat
				double getLifeTime( double t = Clock::getSec() ) const throw();
				//! stat
				double getLifeBegin() const throw();
				//! stat
				double getPauseTime( double t = Clock::getSec() ) const throw();
				//! stat
				double getPlayTime( double t = Clock::getSec() ) const throw();
				//! stat
				Seek getSeekTimes( ) const throw();
				//! stat
				double getLastPause( double t = Clock::getSec() ) const throw();

			};

			//! vlc medium: RTP(t) = PT(t) * rate
			class VLCMedium
			: public Medium
			, public Factory::Multiton< Medium, VLCMedium, RTSP::UA_VLC_1_0_2 >
			{
			protected:
				//! factory ctor
				VLCMedium();
				friend class Factory::Multi< Medium, VLCMedium >;

			public:
				VLCMedium( int rate );
				//! RTP timestamp is function of the current presetation time
				virtual TTimestamp getRTPtime( double pt = HUGE_VAL, double t = Clock::getSec() ) const throw();
			};
		}

		//! measures real-time frame rate of a medium
		class FrameRate
		{
		protected:
			Timeline::MultiSegment _time;
			//! number of frames sent from start time
			uint64_t _n;
		public:
			//! ctor
			FrameRate();
			//! register a sent frame
			void tick() throw();
			//! start sampling
			void start() throw();
			//! stop sampling
			void stop() throw();
			//! get medium time from two deliveries in secons
			double getInterval() const throw();
			//! get delivery frequence in Hertz
			double getFrequency() const throw();
		};		
	}
}

#endif
