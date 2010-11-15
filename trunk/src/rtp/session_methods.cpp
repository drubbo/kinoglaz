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
 * File name: src/rtp/session_methods.cpp
 * First submitted: 2010-07-30
 * First submitter: Emiliano Leporati <emiliano.leporati@gmail.com>
 * Contributor(s) so far - 2010-11-04 :
 *     Emiliano Leporati <emiliano.leporati@gmail.com>
 *
 * Last changes :
 *     fixed RTSP buffer enqueue
 *     threads terminate with wait + join
 *     testing against a "speed crash"
 *     english comments; removed leak with connection serving threads
 *     removed magic numbers in favor of constants / ini parameters
 *
 **/


#include "rtp/session.h"
#include "lib/clock.h"
#include "lib/log.h"
#include "rtcp/rtcp.h"

namespace KGD
{
	namespace RTP
	{
		RTSP::PlayRequest Session::play( const RTSP::PlayRequest & rq ) throw( KGD::Exception::OutOfBounds )
		{
			OwnThread::Lock lk(_th);

			RTSP::PlayRequest ret;

			// setup end time according to scale
			if ( rq.speed > 0 )
				_timeEnd = min( _medium.getIterationDuration(), rq.to );
			else if ( rq.to < HUGE_VAL )
				_timeEnd = rq.to;
			else
				_timeEnd = 0;

			// setup to play
			if ( _status.bag[ Status::STOPPED ] )
				ret = this->doFirstPlay(rq);
			else
				ret = this->doSeekScale(rq);

			ret.to = _timeEnd;
			ret.hasRange = true;
			ret.hasScale = true;
			ret.mediaType = _medium.getType();

			this->logTimes();

			return ret;
		}

		void Session::play() throw()
		{
			{
				OwnThread::Lock lk( _th );
				Log::message( "%s: start play", getLogName() );
				_status.bag[ Status::PAUSED ] = false;
				if ( ! _th )
				{
					_rtcp.receiver->start();
					_rtcp.sender->reset();
					_th.reset( new boost::thread(boost::bind(&RTP::Session::run, this)) );
				}
			}

			_pause.wakeup.notify_all();
		}

		RTSP::PlayRequest Session::doFirstPlay( const RTSP::PlayRequest & rq ) throw( KGD::Exception::OutOfBounds )
		{
			RTSP::PlayRequest ret( rq );
			if ( !rq.hasScale )
				ret.speed = RTSP::PlayRequest::LINEAR_SCALE;
			if ( rq.from == HUGE_VAL )
				ret.from = signedMin( 0.0, _medium.getIterationDuration(), sign( ret.speed ) );

			Log::message( "%s: play %s", getLogName(), ret.toString().c_str() );

			_seqStart = _seqCur + 1;
			_frame.time->restartRTPtime();

			_status.bag[ Status::STOPPED ] = false;
			_status.bag[ Status::PAUSED ] = true;

			_frame.time->seek( ret.time, ret.from, ret.speed );
			_frame.buf->seek( ret.from, ret.speed );

			return ret;
		}

		RTSP::PlayRequest Session::doSeekScale( const RTSP::PlayRequest & rq ) throw( KGD::Exception::OutOfBounds )
		{
			RTSP::PlayRequest ret( rq );
			if ( !rq.hasScale )
				ret.speed = _frame.time->getSpeed();
			if ( rq.from == HUGE_VAL )
				ret.from = _frame.time->getPresentationTime();

			_status.bag[ Status::SEEKED ] = ret.hasRange;

			// pause
			if ( _status.bag[ Status::PAUSED ] )
			{
				Log::message( "%s: unpause medium after %f s", getLogName(), _frame.time->getLastPause() );
			}
			// seek
			if ( ret.hasRange )
			{
				Log::message( "%s: seek medium at %f", getLogName(), ret.from );
			}
			// scale
			if ( ret.hasScale )
			{
				Log::message( "%s: scale medium at %0.2f x", getLogName(), ret.speed );
			}

			if ( _status.bag[ Status::PAUSED ] || ret.hasRange || ret.hasScale )
			{
				_frame.next.reset();
				_frame.buf->seek( ret.from, ret.speed );
				_seqStart = _seqCur + 1;
				_frame.time->seek( rq.time, ret.from, ret.speed );
			}
			else
				Log::warning( "%s: no changes in play status", getLogName() );

			return ret;
		}

		void Session::pause( const RTSP::PlayRequest & rq ) throw()
		{
			OwnThread::Lock lk( _th );

			if ( _status.bag[ Status::STOPPED ] )
			{
				Log::warning( "%s: session is stopped", getLogName() );
			}
			else if ( _status.bag[ Status::PAUSED ] )
			{
				Log::warning( "%s: already paused", getLogName() );
			}
			else
			{
				_status.bag[ Status::PAUSED ] = true;

				_frame.time->pause( rq.time );

				Log::message( "%s: start pause at media time %lf", getLogName(), _frame.time->getPresentationTime() );
				this->logTimes();

				_pause.sync = true;
				{
					OwnThread::UnLock ulk( lk );
					_th.interrupt();
					_pause.asleep.wait();
				}
				Log::debug( "%s: effectively paused", getLogName() );
			}
		}

		void Session::unpause( const RTSP::PlayRequest & rq ) throw()
		{
			_th.lock();

			if ( _status.bag[ Status::PAUSED ] )
			{

				Log::message( "%s: unpause", getLogName() );
				_status.bag[ Status::PAUSED ] = false;
				_frame.time->unpause( rq.time, _frame.time->getSpeed() );
				_th.unlock();

				Log::verbose( "%s: wakeup", getLogName() );
				_pause.wakeup.notify_all();
			}
			else
			{
				Log::warning( "%s: already playing", getLogName() );
				_th.unlock();
			}

		}
		void Session::teardown( const RTSP::PlayRequest & rq ) throw()
		{
			OwnThread::Lock lk(_th);

			Log::debug( "%s: tearing down", getLogName() );

			if ( !_status.bag[ Status::STOPPED ] )
			{
				_status.bag[ Status::STOPPED ] = true;
				if ( _status.bag[ Status::PAUSED ] )
				{
					Log::verbose( "%s: awaking paused send loop", getLogName() );
					_status.bag[ Status::PAUSED ] = false;
					{
						OwnThread::UnLock ulk( lk );
						_pause.wakeup.notify_all();
					}
				}
				_frame.buf->stop();
			}
			if ( _th )
			{
				Log::verbose( "%s: waiting loop termination", getLogName() );
				OwnThread::UnLock ulk( lk );
				_th.reset();
				Log::verbose( "%s: loop joined terminate", getLogName() );
			}

			_frame.time->stop( rq.time );
			this->logTimes();

			Log::debug( "%s: teardown completed", getLogName() );
		}

	}
}
