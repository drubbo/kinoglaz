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
 * File name: ./sdp/medium.cpp
 * First submitted: 2010-02-20
 * First submitter: Emiliano Leporati <emiliano.leporati@gmail.com>
 * Contributor(s) so far - 2010-07-30 :
 *     Emiliano Leporati <emiliano.leporati@gmail.com>
 *
 * Last changes :
 *     formats on their own; better Factory elements naming; separate library for every component
 *     implemented (unused) support for MP3 ADU
 *     Kinoglaz birth: pervasive bug fixing, improvements and client support
 *     fixed some bug in Log and in insertMedia; comments
 *     log messages refactor; shared descriptor are optional now; spot insertion support
 *
 **/



#include "sdp/medium.h"
#include "sdp/frameiterator.h"
#include "sdp/frame.h"
#include "lib/log.h"
#include "lib/utils/container.hpp"
#include "lib/utils/virtual.hpp"
#include "lib/array.hpp"

#include <sstream>
#include <iomanip>
#include <iostream>

extern "C"
{
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
}

namespace KGD
{

	namespace SDP
	{
		namespace Medium
		{
			// ****************************************************************************************************************

			Base::Base( MediaType mt, PayloadType pt )
			: _type( mt )
			, _pt( pt )
			, _rate( 90000 )
			, _index( 0 )
			, _duration( 0 )
			, _frameTimeShift( 0 )
			, _timeBase( 0 )
			, _freqBase( 0 )
			, _extraData( 0 )
			, _frameCount( -1 )
			, _itCount( 0 )
			, _itModel( new Iterator::Default( *this ) )
			{

			}

			Base::Base( const Base & b )
			: _type( b._type )
			, _pt( b._pt )
			, _rate( b._rate )
			, _index( b._index )
			, _duration( 0 )
			, _frameTimeShift( 0 )
			, _timeBase( b._timeBase )
			, _freqBase( b._freqBase )
			, _extraData( b._extraData )
			, _frameCount( 0 )
			, _itCount( 0 )
			, _itModel( new Iterator::Default( *this ) )
			{

			}

			Base::~Base()
			{
				RLock lk( _fMux );

				_itModel.destroy();

				Log::debug( "%s: waiting for %u iterators", getLogName(), _itCount.getValue() );
				while( _itCount > 0 )
					_condItReleased.wait( lk );

				Ctr::clear( _frames );
			}

			Iterator::Base * Base::newFrameIterator() throw()
			{
				return _itModel->getClone();
			}

			void Base::setFrameIteratorModel( Iterator::Base * it ) throw()
			{
				_itModel = it;
			}

			double Base::getIterationDuration() throw()
			{
				return _itModel->duration();
			}

			const char * Base::getLogName() const throw()
			{
				return _logName.c_str();
			}

			char Base::toNibble( unsigned char in )
			{
				return ( in < 10 ? '0' + in : 'A' + in - 10 );
			}

			double Base::getDuration() const
			{
				return _duration;
			}
			const string & Base::getFileName() const
			{
				return _fileName;
			}
			const string & Base::getTrackName() const
			{
				return _trackName;
			}
			const ByteArray & Base::getExtraData() const
			{
				return _extraData;
			}
			PayloadType Base::getPayloadType() const
			{
				return _pt;
			}
			int Base::getRate() const
			{
				return _rate;
			}
			unsigned char Base::getIndex() const
			{
				return _index;
			}
			MediaType Base::getType() const
			{
				return _type;
			}
			double Base::getTimeBase() const
			{
				return _timeBase;
			}

			void Base::setDuration( double x )
			{
				_duration = x;
			}
			void Base::setFileName( const string & x )
			{
				_fileName = x;
				_trackName = x + "[" + toString< int >(_index) + "]";
				_logName = "SDP " + _trackName;
			}
			void Base::setPayloadType( PayloadType x )
			{
				_pt = x;
			}
			void Base::setRate( int x )
			{
				_rate = x;
			}
			void Base::setIndex( uint8_t x )
			{
				_index = x;
				_trackName = _fileName + "[" + toString< int >(x) + "]";
				_logName = "SDP " + _trackName;
			}
			void Base::setType( MediaType x )
			{
				_type = x;
			}
			void Base::setExtraData( void const * const data, size_t sz )
			{
				_extraData.set( data, sz, 0 );
			}
			void Base::setTimeBase( double x )
			{
				_timeBase = x;
				_freqBase = 1 / x;
				Log::debug( "%s: Tbase %lf Fbase %lf", getLogName(), _timeBase, _freqBase );
			}

			void Base::finalizeFrameCount( ) throw()
			{
				{
					RLock lk(_fMux);
					_frameCount = _frames.size();
				}
				_condMoreFrames.notify_all();
			}

			void Base::addFrame( Frame::Base * f ) throw()
			{
				{
					RLock lk(_fMux);
					f->setPayloadType( _pt );
					f->addTime( _frameTimeShift );
					_frames.push_back( f );
				}
				_condMoreFrames.notify_all();
			}

			size_t Base::getFrameCount( ) const throw( )
			{
				RLock lk(_fMux);
				while( _frameCount < 0 )
					_condMoreFrames.wait( lk );

				return _frameCount;
			}

			vector< Frame::Base * > Base::getFrames( double from, double to ) const throw( )
			{
				RLock lk(_fMux);
				size_t
					fromPos = 0,
					toPos = this->getFrameCount() - 1;
				// guess exact start
				try
				{
					fromPos = this->getFramePos( from );
				}
				catch( const KGD::Exception::OutOfBounds & e )
				{
					Log::warning( "%s: %s during getFrames, cropping start to 0", getLogName(), e.what() );
				}
				// guess exact stop
				if ( to < HUGE_VAL )
				{
					try
					{
						toPos = this->getFramePos( to );
						// stop before the last key frame if video
						if ( _type == SDP::VIDEO && toPos > 0 )
							-- toPos;
					}
					catch( const KGD::Exception::OutOfBounds & e )
					{
						Log::warning( "%s: %s during getFrames, cropping stop to frame count", getLogName(), e.what() );
					}
				}

				vector< Frame::Base * > rt;
				rt.reserve( toPos - fromPos + 1 );
				for( size_t i = fromPos; i <= toPos; ++i )
				{
					Frame::Base * f = _frames[i]->getClone();
					f->addTime( -from );
					rt.push_back( f );
				}

				return rt;
			}



			const Frame::Base & Base::getFrame( size_t pos ) const throw( KGD::Exception::OutOfBounds )
			{
				RLock lk(_fMux);
				if ( pos >= _frames.size() )
				{
					if ( _frameCount < 0 )
						_condMoreFrames.wait( lk );
					else
						throw KGD::Exception::OutOfBounds( pos, 0, _frames.size() );
				}

				return * _frames[ pos ];
			}

			size_t Base::getFramePos( double t ) const throw( KGD::Exception::OutOfBounds )
			{
				RLock lk(_fMux);
				size_t pos = 0;
				for( ; ; )
				{
					// wait for more frames if needed
					if( pos >= _frames.size() )
					{
						if ( _frameCount < 0 )
							_condMoreFrames.wait( lk );
						else
							throw KGD::Exception::OutOfBounds( pos, 0, _frames.size() );
					}
					// frame time >= searched time
					// if video frame, must be key
					else if (
						_frames[pos]->getTime() >= t
						&& ( (_type == SDP::VIDEO && _frames[pos]->asPtr< Frame::MediaFile >()->isKey() )
							|| _type != SDP::VIDEO ) )
						return pos;
					else
						++ pos;
				}
			}

			void Base::insert( Iterator::Base & otherFrames, double start ) throw( KGD::Exception::OutOfBounds )
			{
				RLock lk(_fMux);
				double duration = otherFrames.duration();
				// guess pos
				size_t pos = this->getFramePos( start );
				Log::debug( "%s: media insert: found insert position %lu for time %lf, shifting next frames by %lf", getLogName(), pos, start, duration );
				// shift successive frames by new medium duration
				TFrameList::iterator insIt = _frames.begin() + pos;
				Log::debug( "%s: media insert: first frame to shift at time %lf (previous at %lf)", getLogName(), (*insIt)->getTime(), (*(insIt - 1))->getTime() );
				{
					TFrameList::iterator it = insIt, ed = _frames.end();
					for( ; it != ed; ++it )
						(*it)->addTime( duration );

				}
				// shift new frames by offset time
				TFrameList toInsert;
				try
				{
					for(;;)
					{
						Frame::Base * newFrame = otherFrames.next().getClone();
						newFrame->addTime( start );
						toInsert.push_back( newFrame );
					}
				}
				catch( KGD::Exception::OutOfBounds )
				{
					// and insert
					Log::debug( "%s: media insert: insert %lu new frames", getLogName(), toInsert.size() );
					_frames.insert( insIt, toInsert.begin(), toInsert.end() );
					_duration += duration;
					_frameTimeShift += duration;
				}
			}

			void Base::append( Iterator::Base & otherFrames ) throw( )
			{
				RLock lk(_fMux);
				// we have to wait full fill
				while( _frameCount < 0 )
					_condMoreFrames.wait( lk );

				// shift new frames by offset time
				TFrameList toInsert;
				try
				{
					for(;;)
					{
						Frame::Base * newFrame = otherFrames.next().getClone();
						newFrame->addTime( _duration );
						toInsert.push_back( newFrame );
					}
				}
				catch( KGD::Exception::OutOfBounds )
				{
					// and insert
					Log::debug( "%s: media append: append %lu new frames", getLogName(), toInsert.size() );
					_frames.insert( _frames.end(), toInsert.begin(), toInsert.end() );
					_duration += otherFrames.duration();
				}				
			}


			void Base::insert( double duration, double start ) throw( KGD::Exception::OutOfBounds )
			{
				RLock lk(_fMux);
				// guess pos
				size_t pos = this->getFramePos( start );
				Log::debug( "%s: media insert: found insert position %lu", getLogName(), pos );
				// shift successive frames by new medium duration
				TFrameList::iterator insIt = _frames.begin() + pos;
				{
					TFrameList::iterator it = insIt, ed = _frames.end();
					for( ; it != ed; ++it )
						(*it)->addTime( duration );
				}
				_duration += duration;
				_frameTimeShift += duration;
			}

			void Base::loop( uint8_t times ) throw()
			{
				_itModel = new Iterator::Loop( _itModel.release(), times );
			}
		}
	}
}
