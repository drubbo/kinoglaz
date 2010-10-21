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
#include "lib/utils/virtual.hpp"
#include "lib/array.hpp"
#include "rtsp/method.h"

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

			Base::FrameData::FrameData( int64_t n )
			: count( n )
			{}
			
			Base::It::It( )
			: count( 0 )
			{}

			Base::Base( MediaType::kind mt, Payload::type pt )
			: _type( mt )
			, _pt( pt )
			, _rate( 90000 )
			, _index( 0 )
			, _duration( 0 )
			, _frameTimeShift( 0 )
			, _timeBase( 0 )
			, _freqBase( 0 )
			, _extraData( 0 )
			, _frame( -1 )
			{
				_it.model.reset( new Iterator::Default( *this ) );
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
			, _frame( 0 )
			{
				_it.model.reset( new Iterator::Default( *this ) );
			}

			Base::~Base()
			{
				FrameData::Lock lk( _frame );

				_it.model.reset();

				Log::debug( "%s: waiting for %u iterators", getLogName(), long( _it.count ) );
				while( _it.count > 0 )
					_it.released.wait( lk );

				_frame.list.clear();
			}

			Iterator::Base * Base::newFrameIterator() const throw()
			{
				It::Lock lk( _it );
				++ _it.count;
				return _it.model->getClone();
			}

			void Base::releaseIterator( ) throw()
			{
				_it.lock();
				if ( -- _it.count <= 0 )
				{
					_it.unlock();
					_it.released.notify_all();
				}
				else
					_it.unlock();
			}

			void Base::setFrameIteratorModel( Iterator::Base * it ) throw()
			{
				It::Lock lk( _it );
				_it.model.reset( it );
			}

			double Base::getIterationDuration() const throw()
			{
				It::Lock lk( _it );
				return _it.model->duration();
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
			Payload::type Base::getPayloadType() const
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
			MediaType::kind Base::getType() const
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
			void Base::setPayloadType( Payload::type x )
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
			void Base::setType( MediaType::kind x )
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
					FrameData::Lock lk( _frame );
					_frame.count = _frame.list.size();
				}
				_frame.available.notify_all();
			}

			void Base::addFrame( Frame::Base * f ) throw()
			{
				{
					FrameData::Lock lk( _frame );
					f->setPayloadType( _pt );
					f->addTime( _frameTimeShift );
					f->setMediumPos( _frame.list.size() );
					_frame.list.push_back( f );
				}
				_frame.available.notify_all();
			}

			size_t Base::getFrameCount( ) const throw( )
			{
				FrameData::Lock lk( _frame );
				while( _frame.count < 0 )
					_frame.available.wait( lk );

				return _frame.count;
			}

			Base::FrameList Base::getFrames( double from, double to ) const throw( )
			{
				FrameData::Lock lk( _frame );
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
						if ( _type == SDP::MediaType::Video && toPos > 0 )
							-- toPos;
					}
					catch( const KGD::Exception::OutOfBounds & e )
					{
						Log::warning( "%s: %s during getFrames, cropping stop to frame count", getLogName(), e.what() );
					}
				}

				// copy
				FrameList rt;
				rt.reserve( toPos - fromPos + 1 );
				for( size_t i = fromPos; i <= toPos; ++i )
				{
					if ( ! _frame.list.is_null( i ) )
					{
						auto_ptr< Frame::Base > f( _frame.list[i].getClone() );
						f->addTime( -from );
						rt.push_back( f );
					}
				}

				return rt;
			}


			void Base::releaseFrame( size_t pos ) throw()
			{
				FrameData::Lock lk( _frame );
				It::Lock ilk( _it );
				if ( ! RTSP::Method::SUPPORT_SEEK
					&& pos < _frame.list.size()
					&& ! _frame.list.is_null( pos )
					&& ! _it.model->hasType< Medium::Iterator::Loop >() )
				{
					_frame.list.replace( pos, 0 );
				}
			}

			const Frame::Base & Base::getFrame( size_t pos ) const throw( KGD::Exception::OutOfBounds, KGD::Exception::NullPointer )
			{
				FrameData::Lock lk( _frame );
				while ( pos >= _frame.list.size() )
				{
					if ( _frame.count < 0 )
						_frame.available.wait( lk );
					else
						throw KGD::Exception::OutOfBounds( pos, 0, _frame.list.size() );
				}

				if ( ! _frame.list.is_null( pos ) )
					return _frame.list[ pos ];
				else
					throw KGD::Exception::NullPointer( "Frame at position " + KGD::toString( pos ) + " out of " + KGD::toString( _frame.list.size() ) );
			}

			size_t Base::getFramePos( double t ) const throw( KGD::Exception::OutOfBounds )
			{
				FrameData::Lock lk( _frame );
				size_t pos = 0;
				for( ; ; )
				{
					// wait for more frames if needed
					if( pos >= _frame.list.size() )
					{
						if ( _frame.count < 0 )
							_frame.available.wait( lk );
						else
							throw KGD::Exception::OutOfBounds( pos, 0, _frame.list.size() );
					}
					// frame time >= searched time
					// if video frame, must be key
					else if (
						! _frame.list.is_null( pos )
						&& _frame.list[pos].getTime() >= t
						&& ( (_type == SDP::MediaType::Video && _frame.list[pos].asPtrUnsafe< Frame::MediaFile >()->isKey() )
							|| _type != SDP::MediaType::Video ) )
						return pos;
					else
						++ pos;
				}
			}

			void Base::insert( Iterator::Base & otherFrames, double start ) throw( KGD::Exception::OutOfBounds )
			{
				FrameData::Lock lk( _frame );
				double duration = otherFrames.duration();
				// guess pos
				size_t pos = this->getFramePos( start );
				Log::debug( "%s: media insert: found insert position %lu for time %lf, shifting next frames by %lf", getLogName(), pos, start, duration );
				// shift successive frames by new medium duration
				FrameList::iterator insIt = _frame.list.begin() + pos;
				Log::debug( "%s: media insert: first frame to shift at time %lf (previous at %lf)", getLogName(), insIt->getTime(), (insIt - 1)->getTime() );
				{
					FrameList::iterator it = insIt, ed = _frame.list.end();
					for( ; it != ed; ++it )
						it->addTime( duration );

				}
				// shift new frames by offset time
				FrameList toInsert;
				try
				{
					for(;;)
					{
						auto_ptr< Frame::Base > newFrame( otherFrames.next().getClone() );
						newFrame->addTime( start );
						toInsert.push_back( newFrame );
					}
				}
				catch( KGD::Exception::OutOfBounds )
				{
					// and insert
					Log::debug( "%s: media insert: insert %lu new frames", getLogName(), toInsert.size() );
					_frame.list.insert( insIt, toInsert.begin(), toInsert.end() );
					_duration += duration;
					_frameTimeShift += duration;
				}
			}

			void Base::append( Iterator::Base & otherFrames ) throw( )
			{
				FrameData::Lock lk( _frame );
				// we have to wait full fill
				while( _frame.count < 0 )
					_frame.available.wait( lk );

				// shift new frames by offset time
				FrameList toInsert;
				try
				{
					for(;;)
					{
						auto_ptr< Frame::Base > newFrame( otherFrames.next().getClone() );
						newFrame->addTime( _duration );
						toInsert.push_back( newFrame );
					}
				}
				catch( KGD::Exception::OutOfBounds )
				{
					// and insert
					Log::debug( "%s: media append: append %lu new frames", getLogName(), toInsert.size() );
					_frame.list.insert( _frame.list.end(), toInsert.begin(), toInsert.end() );
					_duration += otherFrames.duration();
				}				
			}


			void Base::insert( double duration, double start ) throw( KGD::Exception::OutOfBounds )
			{
				FrameData::Lock lk( _frame );
				// guess pos
				size_t pos = this->getFramePos( start );
				Log::debug( "%s: media insert: found insert position %lu", getLogName(), pos );
				// shift successive frames by new medium duration
				FrameList::iterator insIt = _frame.list.begin() + pos;
				{
					FrameList::iterator it = insIt, ed = _frame.list.end();
					for( ; it != ed; ++it )
						it->addTime( duration );
				}
				_duration += duration;
				_frameTimeShift += duration;
			}

			void Base::loop( uint8_t times ) throw()
			{
				It::Lock lk( _it );
				_it.model.reset( new Iterator::Loop( _it.model.release(), times ) );
			}
		}
	}
}
