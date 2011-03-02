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
 * File name: src/sdp/medium.h
 * First submitted: 2010-07-30
 * First submitter: Emiliano Leporati <emiliano.leporati@gmail.com>
 * Contributor(s) so far - 2010-11-04 :
 *     Emiliano Leporati <emiliano.leporati@gmail.com>
 *
 * Last changes :
 *     Lockables in timers and medium; refactorized iterator release
 *     boosted
 *     removed deadlock issue in RTCP receiver; unloading sent frames from memory when appliable
 *     Some cosmetics about enums and RTCP library
 *     source import
 *
 **/



#ifndef __KGD_SDP_MEDIUM_H
#define __KGD_SDP_MEDIUM_H

#include "sdp/common.h"
#include "sdp/frame.h"
#include "lib/array.h"
#include "lib/urlencode.h"
#include "lib/utils/factory.hpp"
#include "lib/utils/safe.hpp"
#include "lib/utils/ref.hpp"

#include <boost/detail/atomic_count.hpp>
#include <string>
#include <map>

extern "C"
{
#include <libavformat/avformat.h>
}
using namespace std;

namespace KGD
{
	namespace SDP
	{
		class Container;

		//! Track data and metadata
		namespace Medium
		{
			namespace Iterator
			{
				class Base;
				class Default;
			}
			
			//! Resource Medium metadata and stream informations
			class Base
			: public boost::noncopyable
			, virtual public Factory::Base
			{
			public:
				typedef boost::ptr_vector< boost::nullable< Frame::Base > > FrameList;
				typedef vector< Iterator::Base * > IteratorList;
				friend class SDP::Container;
			protected:
				//! ref to container
				ref< SDP::Container > _container;
				//! file name
				string _fileName;
				//! track name (file name with track index)
				string _trackName;
				//! A(udio), V(ideo), ...
				MediaType::kind _type;
				//! 96, 14, ....
				Payload::type _pt;
				//! 90000, ...
				int _rate;
				//! track index
				uint8_t _index;
				//! duration
				double _duration;
				//! frame time base
				double _timeBase;
				//! frame frequence base
				double _freqBase;

				//! extra informations
				ByteArray _extraData;

				//! frame stuff in medium descriptor
				struct FrameData
				: public Safe::LockableBase< RMutex >
				{
					FrameData( int64_t );
					//! number of frames; -1 means no effective value has been determined
					int64_t count;
					//! frame list
					FrameList list;
					//! condition for iterators to wait for more frames
					mutable Condition available;
					//! time shift to add to new added frames if insertions were made
					double timeShift;
					//! time of first valid frame
					double timeFirst;
					//! time of last valid frame
					double timeLast;
				} _frame;

				//! iterator stuff in medium descriptor
				struct It
				: public Safe::LockableBase< RMutex >
				{
					It( );
					//! instance references
					IteratorList instances;
// 					mutable boost::detail::atomic_count count;
					//! frame iterator model
					auto_ptr< Iterator::Base > model;
					//! condition for dtor to wait for no more iterator instances
					Condition released;
				} _it;


				//! log identifier
				string _logName;
				
			protected:
				//! ctor
				Base( MediaType::kind, Payload::type );
				//! copy informations (not frames)
				Base( const Base & );
				//! set frame count to determined, actual, effective value
				virtual void finalizeFrameCount( ) throw();
				//! adds a frame
				virtual void cacheFrame( Frame::Base * ) throw();
				//! adds a frame and notifies waiting threads
				virtual void addFrame( Frame::Base * ) throw();
				//! retrieves a frame at a given position
				const Frame::Base & getFrame( size_t ) const throw( KGD::Exception::OutOfBounds, KGD::Exception::NullPointer );
				//! tells the position of the first valid frame at or immediately after the given time in seconds - this means a key frame for video media
				size_t getFramePos( double ) const throw( KGD::Exception::OutOfBounds );


				//!@{
				//! set medium information - just the container can
				void setContainer(SDP::Container&) throw();
				void setPayloadType(Payload::type) throw();
				void setRate(int) throw();
				void setIndex(uint8_t) throw();
				void setType(MediaType::kind) throw();
				void setTimeBase(double) throw();
				void setDuration(double) throw();
				void setFileName(const string &) throw();
				void setExtraData( void const * const, size_t ) throw();
				//!@}

				//! util for extradata
				static char toNibble( unsigned char );
			public:
				//! clone informations
				virtual Base* getInfoClone() const throw() = 0;
				//! dtor
				virtual ~Base();
				//! returns protocol reply
				virtual string getReply( const Url & ) const = 0;

				//! returns log identifier for this medium
				const char * getLogName() const throw();

				//!@{
				//! retrieve medium information
				Payload::type getPayloadType() const throw();
				int getRate() const throw();
				uint8_t getIndex() const throw();
				MediaType::kind getType() const throw();
				double getTimeBase() const throw();
				double getDuration() const throw();
				const ByteArray & getExtraData() const throw();
				const string & getFileName() const throw();
				const string & getTrackName() const throw();
				//!@}

				//! is this medium referred to a live cast ?
				bool isLiveCast() const throw();

				//! set a new iterator model
				void setFrameIteratorModel( Iterator::Base * ) throw();
				//! returns a new frame iterator for this medium' frames, cloning the model
				Iterator::Base * newFrameIterator() throw();
				//! release iterator
				void releaseIterator( Iterator::Base & ) throw();
				//! returns the duration of this medium wrt its iterator model - HUGE_VAL is fine
				double getIterationDuration() const throw();
				//! returns the number of seconds between the first and the last valid frames currently stored
				double getStoredDuration( ) const throw();
				//! returns effective frame count, waiting until one has been determined
				size_t getFrameCount( ) const throw( );
				//! returns a cloned portion of all frames based on time; limits are cropped if out of bounds
				FrameList getFrames( double from, double to = HUGE_VAL ) const throw( );
				
				//! frees the memory of a frame at given position
				void releaseFrame( size_t pos ) throw();

				//! insert other frames for a total duration at a defined time position
				void insert( Iterator::Base & otherFrames, double start ) throw( KGD::Exception::OutOfBounds );
				//! insert a void, silence duration at a defined time position
				void insert( double duration, double start ) throw( KGD::Exception::OutOfBounds );
				//! append frames to the end
				void append( Iterator::Base & otherFrames ) throw();
				//! loop current frames a number of times ( 0 = infinite )
				void loop( uint8_t = 0 ) throw();

			private:
				//! internal insert utility
				void insert( FrameList::iterator at, double offset, double shift, Iterator::Base & otherFrames );

				friend class Iterator::Default;
			};
		}
	}
}

#endif
