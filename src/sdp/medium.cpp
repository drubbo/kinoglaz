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
 * File name: src/sdp/medium.cpp
 * First submitted: 2010-07-30
 * First submitter: Emiliano Leporati <emiliano.leporati@gmail.com>
 * Contributor(s) so far - 2010-11-04 :
 *     Emiliano Leporati <emiliano.leporati@gmail.com>
 *
 * Last changes :
 *     introduced keep alive on control socket (me dumb)
 *     testing interrupted connections
 *     Lockables in timers and medium; refactorized iterator release
 *     boosted
 *     removed deadlock issue in RTCP receiver; unloading sent frames from memory when appliable
 *
 **/



#include "sdp/medium.h"
#include "sdp/container.h"
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
			, timeShift( 0 )
			{}
			
			Base::It::It( )
// 			: count( 0 )
			{}

			Base::Base( MediaType::kind mt, Payload::type pt )
			: _type( mt )
			, _pt( pt )
			, _rate( 90000 )
			, _index( 0 )
			, _duration( 0 )
			, _timeBase( 0 )
			, _freqBase( 0 )
			, _extraData( 0 )
			, _frame( -1 )
			{
				_it.model.reset( new Iterator::Default( *this ) );
			}

			Base::Base( const Base & b )
			: _container( b._container )
			, _type( b._type )
			, _pt( b._pt )
			, _rate( b._rate )
			, _index( b._index )
			, _duration( 0 )
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

				while( ! _it.instances.empty() )
				{
					Log::debug( "%s: waiting for %u iterators", getLogName(), _it.instances.size() );
					_it.released.wait( lk );
				}

				_frame.list.clear();
			}

			Iterator::Base * Base::newFrameIterator() throw()
			{
				It::Lock lk( _it );
				Iterator::Base * rt = _it.model->getClone();
				_it.instances.push_back( rt );
				Log::verbose( "%s: new iterator %p", getLogName(), rt );
				return rt;
			}

			void Base::releaseIterator( Iterator::Base & i ) throw()
			{
				Log::debug( "%s: releasing iterator %p", getLogName(), &i );
				_it.lock();
				IteratorList::iterator toRemove = find( _it.instances.begin(), _it.instances.end(), &i );
				if ( toRemove != _it.instances.end() )
				{
					Log::verbose( "%s: iterator %p found", getLogName(), &i );
					_it.instances.erase( toRemove );

					if ( _it.instances.empty() )
					{
						_it.unlock();
						_it.released.notify_all();
						return;
					}
				}
				_it.unlock();
			}

			void Base::setContainer( SDP::Container & cnt ) throw()
			{
				_container = cnt;
			}

			void Base::setFrameIteratorModel( Iterator::Base * it ) throw()
			{
				It::Lock lk( _it );
				_it.model.reset( it );
			}

			bool Base::isLiveCast() const throw()
			{
				BOOST_ASSERT( _container );
				return _container->isLiveCast();
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

			double Base::getDuration() const throw()
			{
				return _duration;
			}
			const string & Base::getFileName() const throw()
			{
				return _fileName;
			}
			const string & Base::getTrackName() const throw()
			{
				return _trackName;
			}
			const ByteArray & Base::getExtraData() const throw()
			{
				return _extraData;
			}
			Payload::type Base::getPayloadType() const throw()
			{
				return _pt;
			}
			int Base::getRate() const throw()
			{
				return _rate;
			}
			unsigned char Base::getIndex() const throw()
			{
				return _index;
			}
			MediaType::kind Base::getType() const throw()
			{
				return _type;
			}
			double Base::getTimeBase() const throw()
			{
				return _timeBase;
			}

			void Base::setDuration( double x ) throw()
			{
				_duration = x;
			}
			void Base::setFileName( const string & x ) throw()
			{
				_fileName = x;
				_trackName = x + "[" + toString< int >(_index) + "]";
				_logName = "SDP " + _trackName;
			}
			void Base::setPayloadType( Payload::type x ) throw()
			{
				_pt = x;
			}
			void Base::setRate( int x ) throw()
			{
				_rate = x;
			}
			void Base::setIndex( uint8_t x ) throw()
			{
				_index = x;
				_trackName = _fileName + "[" + toString< int >(x) + "]";
				_logName = "SDP " + _trackName;
			}
			void Base::setType( MediaType::kind x ) throw()
			{
				_type = x;
			}
			void Base::setExtraData( void const * const data, size_t sz ) throw()
			{
				_extraData.set( data, sz, 0 );
			}
			void Base::setTimeBase( double x ) throw()
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
					Log::debug("%s: %lld frames", getLogName(), _frame.count );
				}
				_frame.available.notify_all();
			}

			void Base::addFrame( Frame::Base * f ) throw()
			{
				{
					FrameData::Lock lk( _frame );
					f->setPayloadType( _pt );
					f->addTime( _frame.timeShift );
					f->setMediumPos( _frame.list.size() );
					BOOST_ASSERT( _frame.list.empty() || f->getTime() > _frame.list.back().getTime() );
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
				// we won't release frames on non-live casts
				// since we may want to seek back
				if ( _container->isLiveCast()
					&& pos < _frame.list.size()
					&& ! _frame.list.is_null( pos )
					&& ! _it.model->hasType< Medium::Iterator::Loop >() )
				{
					// effectively release when every iterator has released the frame
					if ( _frame.list[ pos ].release() >= _it.instances.size() )
						_frame.list.replace( pos, 0 );
/*					{
						_frame.list.erase( _frame.list.begin() + pos );
						for_each( _it.instances.begin(), _it.instances.end(), boost::bind( &Iterator::Base::frameRemoved, _1, pos ) );
					}*/
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
				double otherDuration = otherFrames.duration();
				// guess pos
				size_t pos = this->getFramePos( start );
				Log::debug( "%s: media insert: found insert position %lu for time %lf, shifting next frames by %lf", getLogName(), pos, start, otherDuration );
				// shift successive frames by new medium duration
				FrameList::iterator insIt = _frame.list.begin() + pos;
				Log::debug( "%s: media insert: first frame to shift at time %lf (previous at %lf)", getLogName(), insIt->getTime(), (insIt - 1)->getTime() );
				{
					FrameList::iterator it = insIt, ed = _frame.list.end();
					for( ; it != ed; ++it )
						it->addTime( otherDuration );
				}

				this->insert( insIt, start, otherDuration, otherFrames );
			}

			void Base::append( Iterator::Base & otherFrames ) throw( )
			{
				FrameData::Lock lk( _frame );
				// we have to wait full fill
				while( _frame.count < 0 )
					_frame.available.wait( lk );

				this->insert( _frame.list.end(), _duration, 0, otherFrames );
			}

			void Base::insert( FrameList::iterator at, double offset, double shift, Iterator::Base & otherFrames )
			{
				FrameList toInsert;
				try
				{
					// clone and shift by offset time
					for(;;)
					{
						auto_ptr< Frame::Base > newFrame( otherFrames.next().getClone() );
						newFrame->addTime( offset );
						toInsert.push_back( newFrame );
					}
				}
				catch( KGD::Exception::OutOfBounds )
				{
					// insert and adjust internal state
					Log::debug( "%s: media append: append %lu new frames", getLogName(), toInsert.size() );
					_frame.list.insert( at, toInsert.begin(), toInsert.end() );
					_frame.count += toInsert.size();
					_duration += otherFrames.duration();
					_frame.timeShift += shift;
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
				_frame.timeShift += duration;
			}

			void Base::loop( uint8_t times ) throw()
			{
				It::Lock lk( _it );
				_it.model.reset( new Iterator::Loop( _it.model.release(), times ) );
			}
		}
	}
}
