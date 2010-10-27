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
 * File name: ./rtp/chrono.cpp
 * First submitted: 2010-01-16
 * First submitter: Emiliano Leporati <emiliano.leporati@gmail.com>
 * Contributor(s) so far - 2010-07-30 :
 *     Emiliano Leporati <emiliano.leporati@gmail.com>
 *
 * Last changes :
 *     Kinoglaz birth: pervasive bug fixing, improvements and client support
 *     log messages refactor; shared descriptor are optional now; spot insertion support
 *     wimtv key stream
 *     sdp debugged
 *     interleave ok
 *
 **/


#include "rtp/chrono.h"
#include "lib/clock.h"
#include <cmath>
#include <lib/log.h>

namespace KGD
{
	namespace RTP
	{
		TTimestamp sec2ts( double sec, int rate ) throw()
		{
			return TTimestamp( sec * rate );
		}
		
		namespace Timeline
		{
			Segment::Segment( )
			: _start( HUGE_VAL )
			, _stop( HUGE_VAL )
			, _speed( RTSP::PlayRequest::LINEAR_SCALE )
			{
			}

			Segment::Segment( double start, double spd )
			: _start( start )
			, _stop( HUGE_VAL )
			, _speed( spd )
			{
			}

			double Segment::getSpeed() const throw()
			{
				return _speed;
			}

			bool Segment::isRunning() const throw()
			{
				return (_start < HUGE_VAL && _stop == HUGE_VAL);
			}
			double Segment::getBegin() const throw()
			{
				return _start;
			}
			double Segment::getElapsed( double t ) const throw()
			{
				if ( _start < HUGE_VAL )
					return (min( _stop, t ) - _start) * _speed;
				else
					return 0;
			}
			void Segment::start( double t, double spd ) throw()
			{
				if ( _start == HUGE_VAL )
				{
					_start = t;
					if ( spd < HUGE_VAL )
						_speed = spd;
				}
			}
			void Segment::stop( double t ) throw()
			{
				if ( _start < HUGE_VAL && _stop == HUGE_VAL )
				{
					_stop = t;
				}
			}

			// **************************************************************************************************************

			MultiSegment::MultiSegment( )
			: _current( )
			, _speed( RTSP::PlayRequest::LINEAR_SCALE )
			, _start( HUGE_VAL )
			, _tot( 0 )
			, _last( 0 )
			{
			}
			MultiSegment::MultiSegment( double start, double spd )
			: _current( new Segment( start, spd ) )
			, _speed( spd )
			, _start( start )
			, _tot( 0 )
			, _last( 0 )
			{
			}
			double MultiSegment::getCurrentSpeed() const throw()
			{
				return _speed;
			}
			double MultiSegment::getBegin() const throw()
			{
				return _start;
			}
			double MultiSegment::getElapsed( double t ) const throw()
			{
				if ( this->isRunning() )
					return _tot + _current->getElapsed( t );
				else
					return _tot;
			}
			const Segment & MultiSegment::getCurrentSegment() throw( KGD::Exception::NullPointer )
			{
				return *_current;
			}
			double MultiSegment::getCurrent( double t ) const throw()
			{
				if ( this->isRunning() )
					return _current->getElapsed( t );
				else
					return 0;
			}
			double MultiSegment::getLast( double t ) const throw()
			{
				if ( this->isRunning() )
					return _current->getElapsed( t );
				else
					return _last;
			}
			double MultiSegment::getPrevious( ) const throw()
			{
				return _tot;
			}
			bool MultiSegment::isRunning() const throw()
			{
				return _current && _current->isRunning();
			}
			void MultiSegment::stop( double t ) throw( )
			{
				if ( this->isRunning() )
				{
					_current->stop( t );
					_last = _current->getElapsed();
					_tot += _last;
				}
			}
			void MultiSegment::start( double t, double spd ) throw( KGD::Exception::InvalidState )
			{
				if ( _start < HUGE_VAL )
					throw KGD::Exception::InvalidState( "MultiSegment already started" );
				else
				{
					_start = t;
					if ( spd < HUGE_VAL )
						_speed = spd;

					_current.reset( new Segment( t, _speed ) );
				}
			}

			void MultiSegment::next( double t, double spd ) throw()
			{
				if ( this->isRunning() )
					this->stop();
				else if ( _start == HUGE_VAL )
					_start = t;
				if ( spd < HUGE_VAL )
					_speed = spd;

				_current.reset( new Segment( t, _speed ) );
			}

			// **************************************************************************************************************

			Seek::Seek()
			: absolute(0)
			, relative(0)
			, left(0)
			, right(0)
			{
			}

			Seek & Seek::operator+=( double delta )
			{
				absolute += fabs( delta );
				relative += delta;
				if ( delta > 0 )
					right += delta;
				else
					left -= delta;

				return *this;
			}

			// **************************************************************************************************************

			Medium::Medium( int rate )
			: _rtpStart( 0 )
			, _rate( rate )
			{
				this->restartRTPtime();
			}

			Medium::Medium( )
			: _rtpStart( 0 )
			, _rate( 90000 )
			{
				this->restartRTPtime();
			}

			Medium::~Medium()
			{
			}

			void Medium::setRate( int rate ) throw()
			{
				_rate = rate;
			}
			int Medium::getRate( ) const throw()
			{
				return _rate;
			}
			
			void Medium::restartRTPtime()
			{
				while (0 == (_rtpStart = TTimestamp(random())));
			}

			void Medium::start( double t, double spd ) throw( )
			{
				Medium::Lock lk( *this );
				if ( !_life.isRunning() )
				{
					_life.start( t );
					_play.start( t, spd );
				}
			}

			void Medium::pause( double t ) throw( )
			{
				Medium::Lock lk( *this );
				if ( _play.isRunning() )
				{
					_play.stop( t );
					_pause.next( t );
				}
			}

			void Medium::unpause( double t, double spd ) throw( )
			{
				Medium::Lock lk( *this );
				if ( !_life.isRunning() )
				{
					_life.start( t );
					_play.start( t, spd );
				}
				else if ( _pause.isRunning() )
				{
					_pause.stop( t );
					_play.next( t, spd );
				}
			}

			void Medium::stop( double t ) throw()
			{
				Medium::Lock lk( *this );
				if ( _life.isRunning() )
				{
					_play.stop( t );
					_pause.stop( t );
					_life.stop( t );
				}
			}

			void Medium::seek( double t, double pt, double spd ) throw( )
			{
				Medium::Lock lk( *this );

				double delta = ( pt - this->getPresentationTime( t ) );
				_seek += delta;

				if ( _pause.isRunning() )
					_pause.stop( t );
				else if ( !_life.isRunning() )
					_life.start( t );

				_play.next( t, spd );

// 				Log::debug( "Timeline::Medium seek by %lf", delta );
			}

			double Medium::getPresentationTime( double t ) const throw()
			{
				Medium::Lock lk( *this );
				return _play.getElapsed( t ) + _seek.relative;
			}
// 			TTimestamp Medium::getRTPtime( double deltaPT, double t ) const throw()
// 			{
// 				Medium::Lock lk( *this );
// 				TTimestamp rt = TTimestamp(
// 						( 	_rtpStart
// 							+ _seek.relative	//NOTE this is for VLC, without the rtp timeline would really be monotonic
// 							+ _play.getPrevious()
// 							+ ( _play.getCurrent( t ) + deltaPT ) /*/ _play.getSpeed()*/ )
// 						* _rate );
// // 				Log::message("RTP TIME for %lf AT %lf OF %lf x %0.2lf = %lu / %lu", t, delta, _life.getElapsed() - _pause.getElapsed(), _play.getSpeed(), rt, _rtpStart );
// 				return rt;
// 			}

			TTimestamp Medium::getRTPtime( double pt, double t ) const throw()
			{
				Medium::Lock lk( *this );
				double delta = 0;
				if ( pt != HUGE_VAL )
					delta = ( pt - this->getPresentationTime(t) ) / _play.getCurrentSpeed();
				TTimestamp rt = _rtpStart + sec2ts( this->getPlayTime(t) + delta, _rate );
				return rt;
			}

			TTimestamp Medium::getRTPbegin() const throw()
			{
				Medium::Lock lk( *this );
				return _rtpStart;
			}
			double Medium::getSpeed() const throw()
			{
				Medium::Lock lk( *this );
				return _play.getCurrentSpeed();
			}
			double Medium::getLastPause( double t ) const throw()
			{
				Medium::Lock lk( *this );
				return _pause.getLast( t );
			}
			double Medium::getLifeBegin() const throw()
			{
				Medium::Lock lk( *this );
				return _life.getBegin();
			}
			double Medium::getLifeTime( double t ) const throw()
			{
				Medium::Lock lk( *this );
				return _life.getElapsed( t );
			}
			double Medium::getPauseTime( double t ) const throw()
			{
				Medium::Lock lk( *this );
				return _pause.getElapsed( t );
			}
			double Medium::getPlayTime( double t ) const throw()
			{
				Medium::Lock lk( *this );
				return _life.getElapsed( t ) - _pause.getElapsed( t );
			}
			Seek Medium::getSeekTimes() const throw()
			{
				Medium::Lock lk( *this );
				return _seek;
			}


			// **************************************************************************************************************

			VLCMedium::VLCMedium( )
			: Medium( )
			{
// 				Log::debug("Building VLC media timeline");
			}

			VLCMedium::VLCMedium( int rate )
			: Medium( rate )
			{
			}

			TTimestamp VLCMedium::getRTPtime( double pt, double t ) const throw()
			{
				Medium::Lock lk( *this );
				double curTime = this->getPresentationTime( t );
				if ( pt == HUGE_VAL )
					pt = curTime;

				TTimestamp rt = _rtpStart + sec2ts( pt, _rate );
				return rt;
			}

		}

		FrameRate::FrameRate()
		: _time()
		, _n( 0 )
		{
		}
		void FrameRate::tick() throw()
		{
			++ _n;
		}
		void FrameRate::start() throw( )
		{
			_time.next();
		}
		void FrameRate::stop() throw()
		{
			_time.stop();
		}
		double FrameRate::getInterval() const throw()
		{
			return _time.getElapsed() / _n;
		}
		double FrameRate::getFrequency() const throw()
		{
			return _n / _time.getElapsed();
		}

	}
}
