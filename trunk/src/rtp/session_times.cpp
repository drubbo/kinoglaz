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
 * File name: ./rtp/session_times.cpp
 * First submitted: 2010-01-16
 * First submitter: Emiliano Leporati <emiliano.leporati@gmail.com>
 * Contributor(s) so far - 2010-07-30 :
 *     Emiliano Leporati <emiliano.leporati@gmail.com>
 *
 * Last changes :
 *     Kinoglaz birth: pervasive bug fixing, improvements and client support
 *     fixed some bug in Log and in insertMedia; comments
 *     log messages refactor; shared descriptor are optional now; spot insertion support
 *     sdp debugged
 *     interleave ok
 *
 **/


#include "rtp/session.h"
#include "rtcp/receiver.h"
#include "rtsp/method.h"
#include "lib/log.h"
#include "lib/clock.h"
#include <cmath>

namespace KGD
{
	namespace RTP
	{

		TCseq Session::seqRestart()
		{
			while (0 == (_seqStart  = TCseq(random() % 0xFFFF)));
			_seqCur = _seqStart - 1;
			return _seqStart;
		}

		const Timeline::Medium & Session::getTimeline() const
		{
			return *_time;
		}

		RTSP::PlayRequest Session::eval( const RTSP::PlayRequest & rq ) throw( KGD::Exception::OutOfBounds )
		{
			Lock flk( _frameMux );
			Lock lk(_mux);

			RTSP::PlayRequest ret( rq );
			if ( _stopped )
			{
				if ( !rq.hasScale )
				{
					ret.hasScale = true;
					ret.speed = 1.0;
				}
					
				if ( rq.from == HUGE_VAL )
					ret.from = 0.0;
			}
			else
			{
				if ( !rq.hasScale )
				{
					ret.hasScale = true;
					ret.speed = _time->getSpeed();
				}
				if ( rq.from == HUGE_VAL )
					ret.from = _time->getPresentationTime();

				ret.from = _frameBuf->drySeek( ret.from, ret.speed );
			}

			ret.hasRange = true;

			if ( RTSP::Method::SUPPORT_SEEK )
				ret.to = _medium->getDuration();

			return ret;
		}

		double Session::evalMediumInsertion( double t ) throw( KGD::Exception::OutOfBounds )
		{
			assert( _paused );
			// look a bit forward
			double seekTime = min( t, _time->getPresentationTime() + 1 );
			return _frameBuf->drySeek( seekTime, _time->getSpeed() );
		}

		void Session::insertMedium( SDP::Medium::Base & m, double t ) throw( KGD::Exception::OutOfBounds )
		{
			assert( _paused );
			_frameBuf->insertMedium( m, t );
			_timeEnd += m.getIterationDuration();
		}

		void Session::insertTime( double duration, double t ) throw( KGD::Exception::OutOfBounds )
		{
			assert( _paused );
			_frameBuf->insertTime( duration, t );
			_timeEnd += duration;
		}

		void Session::logTimes() const
		{
			Log::message("%s: Media time %f"
				, getLogName()
				, _time->getPresentationTime()
			);
			Log::message("%s: Life time %f"
				, getLogName()
				, _time->getLifeTime()
			);
			Log::message("%s: Play time %f"
				, getLogName()
				, _time->getPlayTime()
			);
			Log::message("%s: Paused for %f"
				, getLogName()
				, _time->getPauseTime()
			);
			Log::message("%s: Seeked by %f"
				, getLogName()
				, _time->getSeekTimes().absolute
			);
			Log::message("%s: Speed %0.2f"
				, getLogName()
				, _time->getSpeed()
			);
			Log::message("%s: Frame rate %f"
				, getLogName()
				, _fRate.getFrequency()
			);
		}

	}
}
