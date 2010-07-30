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
			_frameLk = new Lock( _frameMux );
			Lock lk(_mux);

			RTSP::PlayRequest ret;

			_timeEnd = min( _medium->getIterationDuration(), rq.to );

			if ( _stopped )
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
			// video, o audio a meno di 1x, faccio partire
			if (_medium->getType() == SDP::VIDEO || fabs( _time->getSpeed() ) <= 1.0)
			{
				_RTCPsender->restart();
				_fRate.start();

				Log::message( "%s: waiting RTCP sender", getLogName() );
				_RTCPsender->wait();

				Log::message( "%s: start play", getLogName() );
				_frameLk.destroy();
				{
					Lock lk(_mux);
					_paused = false;
				}
				_condUnPause.notify_all();
				_playing = true;
			}
			else
				_frameLk.destroy();
		}

		RTSP::PlayRequest Session::doFirstPlay( const RTSP::PlayRequest & rq ) throw( KGD::Exception::OutOfBounds )
		{
			RTSP::PlayRequest ret( rq );
			if ( !rq.hasScale )
				ret.speed = 1.0;
			if ( rq.from == HUGE_VAL )
				ret.from = 0.0;

			Log::message( "%s: play %s", getLogName(), ret.toString().c_str() );

			_seqStart = _seqCur + 1;
			_time->restartRTPtime();

			_stopped = false;
			_paused = true;

			_time->seek( ret.time, ret.from, ret.speed );
			_frameBuf->seek( ret.from, ret.speed );

			_th = new Thread(boost::bind(&RTP::Session::loop, this));
			_RTCPreceiver->start();

			return ret;
		}

		RTSP::PlayRequest Session::doSeekScale( const RTSP::PlayRequest & rq ) throw( KGD::Exception::OutOfBounds )
		{
			RTSP::PlayRequest ret( rq );
			if ( !rq.hasScale )
				ret.speed = _time->getSpeed();
			if ( rq.from == HUGE_VAL )
				ret.from = _time->getPresentationTime();

			// un audio a velocita' superiore a 1.0x non lo invio, lascio il medium in pausa
			if ( _medium->getType() == SDP::VIDEO || fabs( ret.speed ) <= 1.0 )
			{
				// pause
				if ( _paused )
				{
					Log::message( "%s: unpause medium after %f s", getLogName(), _time->getLastPause() );
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

				// posso anche avere seek + scale; in ogni caso lo scale viene passato alla funzione quindi ok
				if ( _paused || ret.hasRange || ret.hasScale )
				{
					_frameNext.destroy();
					_frameBuf->seek( ret.from, ret.speed );
					_seqStart = _seqCur + 1;
					_time->seek( rq.time, ret.from, ret.speed );
				}
				else
					Log::warning( "%s: did not do play because of no conditions", getLogName() );
			}
			else
			{
				Log::warning( "%s: was asked to seek at unsupported speed %0.2f, going pause", getLogName(), ret.speed );
				_time->seek( rq.time, ret.from, ret.speed );
				_paused = true;
			}

			return ret;
		}

		void Session::pause( const RTSP::PlayRequest & rq ) throw()
		{
			Lock flk( _frameMux );
			_mux.lock();

			if ( _paused )
			{
				_mux.unlock();
				Log::warning( "%s: already paused", getLogName() );
			}
			else
			{
				_playing = false;
				_paused = true;

				_fRate.stop();
				_time->pause( rq.time );

				Log::message( "%s: start pause at media time %lf", getLogName(), _time->getPresentationTime() );
				this->logTimes();
				_mux.unlock();

				_condPaused.wait( flk );
				Log::message( "%s: effectively paused", getLogName() );
			}
			
		}

		void Session::unpause( const RTSP::PlayRequest & rq ) throw()
		{
			Lock flk( _frameMux );
			Lock lk( _mux );

			if ( _paused )
			{
				if ( _medium->getType() == SDP::VIDEO || fabs( _time->getSpeed() ) <= 1.0 )
				{
					_RTCPsender->restart();
					_fRate.start();

					Log::message( "%s: waiting RTCP sender", getLogName() );
					_RTCPsender->wait();

					Log::message( "%s: unpause", getLogName() );
					_paused = false;
					_time->unpause( rq.time, _time->getSpeed() );

					_condUnPause.notify_all();
					_playing = true;
				}
				else
					_time->unpause( rq.time, _time->getSpeed() );
			}
			else
			{
				Log::warning( "%s: already playing", getLogName() );
			}

		}
		void Session::teardown( const RTSP::PlayRequest & rq ) throw()
		{
			Log::debug( "%s: tearing down", getLogName() );
			{
				Lock lk(_mux);
				_stopped = true;
			}

			if ( _frameBuf )
				_frameBuf->stop();

			if ( _th )
			{
				Log::debug( "%s: waiting loop termination", getLogName() );
				_condUnPause.notify_all();
				_th->join();
				_th.destroy();
			}
			_playing = false;


			_fRate.stop();

			_RTCPsender->stop();
			_RTCPreceiver->stop();
			_time->stop( rq.time );

			this->logTimes();

			Log::debug( "%s: teardown completed", getLogName() );
		}

	}
}
