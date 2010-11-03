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
 * File name: ./rtp/session_methods.cpp
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

			_timeEnd = min( _medium.getIterationDuration(), rq.to );

			if ( _status.bag[ Status::STOPPED ] )
				ret = this->doFirstPlay(rq);
			else
				ret = this->doSeekScale(rq);

			ret.hasRange = true;
			ret.hasScale = true;

			this->logTimes();

			return ret;
		}

		void Session::play() throw()
		{
			OwnThread::Lock lk( _th );

			// start only for video, or audio at most 1x
			if (_medium.getType() == SDP::MediaType::Video || fabs( _frame.time->getSpeed() ) <= RTSP::PlayRequest::LINEAR_SCALE )
			{
// 				_rtcp.sender->restart();
				_frame.rate.start();

				Log::debug( "%s: waiting RTCP sender", getLogName() );
				_rtcp.sender->wait();

				Log::message( "%s: start play", getLogName() );
				_status.bag[ Status::PAUSED ] = false;

				lk.unlock();
				_pause.wakeup.notify_all();
			}
		}

		RTSP::PlayRequest Session::doFirstPlay( const RTSP::PlayRequest & rq ) throw( KGD::Exception::OutOfBounds )
		{
			RTSP::PlayRequest ret( rq );
			if ( !rq.hasScale )
				ret.speed = RTSP::PlayRequest::LINEAR_SCALE;
			if ( rq.from == HUGE_VAL )
				ret.from = 0.0;

			Log::message( "%s: play %s", getLogName(), ret.toString().c_str() );

			_seqStart = _seqCur + 1;
			_frame.time->restartRTPtime();

			_status.bag[ Status::STOPPED ] = false;
			_status.bag[ Status::PAUSED ] = true;

			_frame.time->seek( ret.time, ret.from, ret.speed );
			_frame.buf->seek( ret.from, ret.speed );

			_th.reset( new boost::thread(boost::bind(&RTP::Session::loop, this)) );
			_rtcp.receiver->start();
			_rtcp.sender->restart();

			return ret;
		}

		RTSP::PlayRequest Session::doSeekScale( const RTSP::PlayRequest & rq ) throw( KGD::Exception::OutOfBounds )
		{
			RTSP::PlayRequest ret( rq );
			if ( !rq.hasScale )
				ret.speed = _frame.time->getSpeed();
			if ( rq.from == HUGE_VAL )
				ret.from = _frame.time->getPresentationTime();

			// leave this session paused with audio at more than 1x
			if ( _medium.getType() == SDP::MediaType::Video || fabs( ret.speed ) <= RTSP::PlayRequest::LINEAR_SCALE )
			{
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
					_rtcp.sender->restart();
					_rtcp.receiver->unpause();
				}
				else
					Log::warning( "%s: no mutations in play status", getLogName() );
			}
			else
			{
				Log::warning( "%s: was asked to seek at unsupported speed %0.2f, going pause", getLogName(), ret.speed );
				_frame.time->seek( rq.time, ret.from, ret.speed );
				_status.bag[ Status::PAUSED ] = true;
			}

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
				
				_frame.rate.stop();
				_frame.time->pause( rq.time );
				_rtcp.receiver->pause();

				Log::message( "%s: start pause at media time %lf", getLogName(), _frame.time->getPresentationTime() );
				this->logTimes();

				_pause.sync = true;
				{
					OwnThread::UnLock ulk( lk );
					_pause.asleep.wait();
				}
				Log::debug( "%s: effectively paused", getLogName() );
			}
		}

		void Session::unpause( const RTSP::PlayRequest & rq ) throw()
		{
			OwnThread::Lock lk( _th );

			if ( _status.bag[ Status::PAUSED ] )
			{
				if ( _medium.getType() == SDP::MediaType::Video || fabs( _frame.time->getSpeed() ) <= RTSP::PlayRequest::LINEAR_SCALE )
				{
					_rtcp.sender->restart();
					_frame.rate.start();

					Log::debug( "%s: waiting RTCP sender", getLogName() );
					_rtcp.sender->wait();

					Log::message( "%s: unpause", getLogName() );
					_status.bag[ Status::PAUSED ] = false;
					_frame.time->unpause( rq.time, _frame.time->getSpeed() );
					_rtcp.receiver->unpause();

					lk.unlock();
					_pause.wakeup.notify_all();
				}
				else
				{
					_frame.time->unpause( rq.time, _frame.time->getSpeed() );
					_rtcp.receiver->unpause();
					_rtcp.sender->restart();
					_rtcp.sender->wait();
				}
			}
			else
			{
				Log::warning( "%s: already playing", getLogName() );
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
			}
			if ( _th )
			{
				Log::debug( "%s: waiting loop termination", getLogName() );
				OwnThread::UnLock ulk( lk );
				_th.wait();
				_th.join();
				_th.reset();
				Log::debug( "%s: loop joined terminate", getLogName() );
			}

			Log::debug( "%s: stopping RTCP Receiver", getLogName() );
			_rtcp.receiver->stop();

			_frame.rate.stop();
			_frame.time->stop( rq.time );
			this->logTimes();

			Log::debug( "%s: teardown completed", getLogName() );
		}

	}
}
