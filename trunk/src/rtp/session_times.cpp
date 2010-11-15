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
 * File name: src/rtp/session_times.cpp
 * First submitted: 2010-07-30
 * First submitter: Emiliano Leporati <emiliano.leporati@gmail.com>
 * Contributor(s) so far - 2010-11-04 :
 *     Emiliano Leporati <emiliano.leporati@gmail.com>
 *
 * Last changes :
 *     minor cleanup and more robust Range / Scale support during PLAY
 *     removed magic numbers in favor of constants / ini parameters
 *     introduced keep alive on control socket (me dumb)
 *     testing interrupted connections
 *     fixed bug during PAUSE toggle
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

		TCseq Session::seqRestart() throw()
		{
			while (0 == (_seqStart  = TCseq(random() % 0xFFFF)));
			_seqCur = _seqStart - 1;
			return _seqStart;
		}

		const Timeline::Medium & Session::getTimeline() const throw()
		{
			return *_frame.time;
		}

		RTSP::PlayRequest Session::getPlayRange() const throw()
		{
			OwnThread::Lock lk(_th);
			
			RTSP::PlayRequest ret;
			ret.speed = _frame.time->getSpeed();
			ret.from = _frame.time->getPresentationTime();
			ret.to = _timeEnd;// medium.getIterationDuration();
			ret.hasRange = true;
			ret.hasScale = true;
			ret.mediaType = _medium.getType();

			return ret;
		}

		RTSP::PlayRequest Session::eval( const RTSP::PlayRequest & rq ) throw( KGD::Exception::OutOfBounds )
		{
			OwnThread::Lock lk(_th);

			Log::debug( "%s: in: %s", getLogName(), rq.toString().c_str() );

			RTSP::PlayRequest ret( rq );

			if ( _status.bag[ Status::STOPPED ] )
			{
				if ( !rq.hasScale )
				{
					ret.hasScale = true;
					ret.speed = RTSP::PlayRequest::LINEAR_SCALE;
				}
					
				if ( rq.from == HUGE_VAL )
					ret.from = signedMin( 0.0, _medium.getIterationDuration(), sign( ret.speed ) );
			}
			else
			{
				if ( !rq.hasScale )
				{
					ret.hasScale = true;
					ret.speed = _frame.time->getSpeed();
				}
				if ( rq.from == HUGE_VAL )
					ret.from = _frame.time->getPresentationTime();

				ret.from = _frame.buf->drySeek( ret.from, ret.speed );
			}

			ret.hasRange = true;
			ret.mediaType = _medium.getType();

			if ( _medium.isLiveCast() )
				ret.to = HUGE_VAL;
			else
				ret.to = signedMax( 0.0, _medium.getIterationDuration(), sign( ret.speed ) );

			Log::debug( "%s: out: %s", getLogName(), ret.toString().c_str() );
			return ret;
		}

		double Session::evalMediumInsertion( double t ) throw( KGD::Exception::OutOfBounds )
		{
			BOOST_ASSERT( _status.bag[ Status::PAUSED ] );
			double spd = _frame.time->getSpeed();
			if ( t == HUGE_VAL )
				return _frame.buf->drySeek( _frame.time->getPresentationTime() + 2 / fabs(spd) , spd);
			else
				return _frame.buf->drySeek( t, _frame.time->getSpeed() );
		}

		void Session::insertMedium( SDP::Medium::Base & m, double t ) throw( KGD::Exception::OutOfBounds )
		{
			BOOST_ASSERT( _status.bag[ Status::PAUSED ] );
			_frame.buf->insertMedium( m, t );
			_timeEnd += m.getIterationDuration();
		}

		void Session::insertTime( double duration, double t ) throw( KGD::Exception::OutOfBounds )
		{
			BOOST_ASSERT( _status.bag[ Status::PAUSED ] );
			_frame.buf->insertTime( duration, t );
			_timeEnd += duration;
		}

		void Session::logTimes() const throw()
		{
			Log::message("%s: Media time %lf | Life time %lf | Play time %lf | Paused for %lf | Seeked by %lf | CurSpd %0.2lf | Frame rate %lf"
				, getLogName()
				, _frame.time->getPresentationTime()
				, _frame.time->getLifeTime()
				, _frame.time->getPlayTime()
				, _frame.time->getPauseTime()
				, _frame.time->getSeekTimes().absolute
				, _frame.time->getSpeed()
				, _frame.rate.getFrequency()
			);
		}
	}
}
