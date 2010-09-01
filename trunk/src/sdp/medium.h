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
 * File name: ./sdp/medium.h
 * First submitted: 2010-02-20
 * First submitter: Emiliano Leporati <emiliano.leporati@gmail.com>
 * Contributor(s) so far - 2010-07-30 :
 *     Emiliano Leporati <emiliano.leporati@gmail.com>
 *
 * Last changes :
 *     formats on their own; better Factory elements naming; separate library for every component
 *     implemented (unused) support for MP3 ADU
 *     interleaved channels shutdown fixed
 *     Kinoglaz birth: pervasive bug fixing, improvements and client support
 *     fixed some bug in Log and in insertMedia; comments
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
			: virtual public Factory::Base
			{
				friend class SDP::Container;
			protected:
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
				//! time shift to add to new added frames if insertions were made
				double _frameTimeShift;
				//! frame time base
				double _timeBase;
				//! frame frequence base
				double _freqBase;

				//! extra informations
				ByteArray _extraData;

				//! frame list mutex
				mutable RMutex _fMux;
				//! number of frames; -1 means no effective value has been determined
				int64_t _frameCount;

				typedef vector< Frame::Base * > TFrameList;
				//! frame list
				TFrameList _frames;

				//! number of instanced iterators
				mutable Safe::Number< size_t > _itCount;
				//! frame iterator model
				Ptr::Scoped< Iterator::Base > _itModel;

				//! condition for dtor to wait for no more iterator instances
				mutable Condition _condItReleased;
				//! condition for iterators to wait for more frames
				mutable Condition _condMoreFrames;

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
				virtual void addFrame( Frame::Base * ) throw();
				//! retrieves a frame at a given position
				const Frame::Base & getFrame( size_t ) const throw( KGD::Exception::OutOfBounds );
				//! tells the position of the first valid frame at or immediately after the given time in seconds - this means a key frame for video media
				size_t getFramePos( double ) const throw( KGD::Exception::OutOfBounds );


				//!@{
				//! set medium information - just the container can
				void setPayloadType(Payload::type);
				void setRate(int);
				void setIndex(uint8_t);
				void setType(MediaType::kind);
				void setTimeBase(double);
				void setDuration(double);
				void setFileName(const string &);
				void setExtraData( void const * const, size_t );
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
				Payload::type getPayloadType() const;
				int getRate() const;
				uint8_t getIndex() const;
				MediaType::kind getType() const;
				double getTimeBase() const;
				double getDuration() const;
				const ByteArray & getExtraData() const;
				const string & getFileName() const;
				const string & getTrackName() const;
				//!@}

				//! set a new iterator model
				void setFrameIteratorModel( Iterator::Base * ) throw();
				//! returns a new frame iterator for this medium' frames, cloning the model
				Iterator::Base * newFrameIterator() throw();
				//! returns the duration of this medium wrt its iterator model - HUGE_VAL is fine
				double getIterationDuration() throw();
				//! returns effective frame count, waiting until one has been determined
				size_t getFrameCount( ) const throw( );
				//! returns a portion of all frames based on time; limits are cropped if out of bounds
				vector< Frame::Base * > getFrames( double from, double to = HUGE_VAL ) const throw( );
				

				//! insert other frames for a total duration at a defined time position
				void insert( Iterator::Base & otherFrames, double start ) throw( KGD::Exception::OutOfBounds );
				//! insert a void, silence duration at a defined time position
				void insert( double duration, double start ) throw( KGD::Exception::OutOfBounds );
				//! append frames to the end
				void append( Iterator::Base & otherFrames ) throw();
				//! loop current frames a number of times ( 0 = infinite )
				void loop( uint8_t = 0 ) throw();

				friend class Iterator::Default;
			};
		}
	}
}

#endif
