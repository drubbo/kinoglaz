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
 * File name: src/sdp/frameiterator.h
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

#ifndef __KGD_FRAME_ITERATOR
#define __KGD_FRAME_ITERATOR

#include "sdp/common.h"
#include "sdp/medium.h"

namespace KGD
{
	namespace SDP
	{
		namespace Medium
		{
			//! iterators over medium descriptors
			namespace Iterator
			{
				class Base;
				class Default;
				class Loop;

				
				//! Medium frame iterator interface
				class Base
				: public boost::noncopyable
				, public Virtual
				{
				public:
					virtual ~Base();
					//! obtain clone of iterator
					virtual Base* getClone() const throw() = 0;
					//! returns frame at position. does not change current position
					virtual const SDP::Frame::Base & at( size_t ) const throw( KGD::Exception::OutOfBounds, KGD::Exception::NullPointer  ) = 0;

					//! returns frame at current position
					virtual const SDP::Frame::Base & curr() const throw( KGD::Exception::OutOfBounds, KGD::Exception::NullPointer  ) = 0;
					//! returns frame at current position then advances position by 1
					virtual const SDP::Frame::Base & next() throw( KGD::Exception::OutOfBounds, KGD::Exception::NullPointer  ) = 0;
					//! returns frame at current position then backwards position by 1
					virtual const SDP::Frame::Base & prev() throw( KGD::Exception::OutOfBounds, KGD::Exception::NullPointer ) = 0;

					//! seeks first frame at specified time. changes current position
					virtual const SDP::Frame::Base & seek( double ) throw( KGD::Exception::OutOfBounds, KGD::Exception::NullPointer  ) = 0;
					//! seeks to a position
					virtual const SDP::Frame::Base & seek( size_t ) throw( KGD::Exception::OutOfBounds, KGD::Exception::NullPointer  ) = 0;

					//! returns current position
					virtual size_t pos() const throw() = 0;
					//! returns number of frames this iterator referers to
					virtual size_t size() const throw() = 0;
					//! returns duration of this iteration in seconds
					virtual double duration() const throw() = 0;
					//! returns time shift to be applied to RTP frames
					virtual double getTimeShift() const throw() = 0;

					//! inserts another iterator
					virtual void insert( Iterator::Base &, double t ) throw( KGD::Exception::OutOfBounds ) = 0;
					//! inserts another medium
					void insert( Medium::Base &, double t ) throw( KGD::Exception::OutOfBounds );
					//! inserts a void space
					virtual void insert( double duration, double t ) throw( KGD::Exception::OutOfBounds ) = 0;

					//! get medium this iterator works on
					virtual Medium::Base & getMedium() throw() = 0;
				};

				//! Medium frame iterator
				class Default
				: public Base
				{
				protected:
					//! medium whose frames we're iterating over
					Medium::Base & _med;
					//! current position in the frame vector
					size_t _pos;

					//! copy another iterator
					Default( const Default & ) throw();
				public:
					//! construct from an abstract medium
					Default( Medium::Base & ) throw();
					//! get clone of this iterator
					virtual Default* getClone() const throw();
					//! dtor, release count from medium
					virtual ~Default();
					//! returns frame at position. does not change current position
					virtual const SDP::Frame::Base & at( size_t ) const throw( KGD::Exception::OutOfBounds, KGD::Exception::NullPointer );
					//! returns frame at current position
					virtual const SDP::Frame::Base & curr() const throw( KGD::Exception::OutOfBounds, KGD::Exception::NullPointer );
					//! returns frame at current position then advances position by 1
					virtual const SDP::Frame::Base & next() throw( KGD::Exception::OutOfBounds, KGD::Exception::NullPointer );
					//! returns frame at current position then backwards position by 1
					virtual const SDP::Frame::Base & prev() throw( KGD::Exception::OutOfBounds, KGD::Exception::NullPointer );
					//! seeks first frame at specified time. changes current position
					virtual const SDP::Frame::Base & seek( double ) throw( KGD::Exception::OutOfBounds, KGD::Exception::NullPointer );
					//! seeks to a position
					virtual const SDP::Frame::Base & seek( size_t ) throw( KGD::Exception::OutOfBounds, KGD::Exception::NullPointer );
					//! returns current position
					virtual size_t pos() const throw();
					//! returns number of effective frames in this medium
					virtual size_t size() const throw();
					//! returns duration of this iteration in seconds
					virtual double duration() const throw();
					//! no time shift
					virtual double getTimeShift() const throw();
					//! inserts another medium
					virtual void insert( Iterator::Base &, double t ) throw( KGD::Exception::OutOfBounds );
					//! inserts a void space
					virtual void insert( double duration, double t ) throw( KGD::Exception::OutOfBounds );

					//! get medium this iterator works on
					virtual Medium::Base & getMedium() throw();
				};

				//! Loops another iterator
				class Loop
				: public Base
				{
				protected:
					//! iterator we're looping
					boost::scoped_ptr< Iterator::Base > _it;
					//! number of iterations to do ( 0 = oo )
					uint8_t _times;
					//! current iteration
					size_t _cur;

					//! copy another iterator
					Loop( const Loop & ) throw();

					//! returns the input position normalized wrt to number of frames; updates _cur
					size_t seekPos( size_t ) throw( KGD::Exception::OutOfBounds );
					//! returns the input position normalized wrt to number of frames
					size_t normalizePos( size_t ) const throw( KGD::Exception::OutOfBounds );
					//! returns the input time normalized wrt to base iteration duration; updates _cur
					double seekTime( double ) throw( KGD::Exception::OutOfBounds );
					//! returns the input time normalized wrt to base iteration duration; updates _cur
					double normalizeTime( double ) const throw( KGD::Exception::OutOfBounds );
				public:
					//! construct from another iterator, with loop count
					Loop( Iterator::Base *, uint8_t = 0 ) throw();
					//! get clone of this iterator
					virtual Loop* getClone() const throw();
					//! dtor
					virtual ~Loop();
					//! returns frame at position. does not change current position
					virtual const SDP::Frame::Base & at( size_t ) const throw( KGD::Exception::OutOfBounds, KGD::Exception::NullPointer );
					//! returns frame at current position
					virtual const SDP::Frame::Base & curr() const throw( KGD::Exception::OutOfBounds, KGD::Exception::NullPointer );
					//! returns frame at current position then advances position by 1
					virtual const SDP::Frame::Base & next() throw( KGD::Exception::OutOfBounds, KGD::Exception::NullPointer );
					//! returns frame at current position then backwards position by 1
					virtual const SDP::Frame::Base & prev() throw( KGD::Exception::OutOfBounds, KGD::Exception::NullPointer );
					//! seeks first frame at specified time. changes current position
					virtual const SDP::Frame::Base & seek( double ) throw( KGD::Exception::OutOfBounds, KGD::Exception::NullPointer );
					//! seeks to a position
					virtual const SDP::Frame::Base & seek( size_t ) throw( KGD::Exception::OutOfBounds, KGD::Exception::NullPointer );
					//! returns current position
					virtual size_t pos() const throw();
					//! returns number of effective frames in this medium
					virtual size_t size() const throw();
					//! returns duration of this iteration in seconds
					virtual double duration() const throw();
					//! time shift is duration * every performed iteration
					virtual double getTimeShift() const throw();
					//! inserts another medium
					virtual void insert( Iterator::Base &, double t ) throw( KGD::Exception::OutOfBounds );
					//! inserts a void space
					virtual void insert( double duration, double t ) throw( KGD::Exception::OutOfBounds );

					//! get medium this iterator works on
					virtual Medium::Base & getMedium() throw();
				};

/*
				//! Iterator over a detached frame set
				class Slice
				: public Base
				{
				protected:
					//! frames we're iterating over
					boost::ptr_vector< Frame::Base > _frames;
					//! current position in the frame vector
					size_t _pos;
					//! time extension of frame vector
					double _duration;
					//! type of frames
					MediaType::kind _type;

					//! copy another iterator
					Slice( const Slice & ) throw();
				public:
					//! construct from a frame sequence
					Slice( vector< Frame::Base * > &, MediaType::kind ) throw();
					//! get clone of this iterator
					virtual Slice* getClone() const throw();
					//! dtor
					virtual ~Slice();
					//! returns frame at position. does not change current position
					virtual const SDP::Frame::Base & at( size_t ) const throw( KGD::Exception::OutOfBounds );
					//! returns frame at current position
					virtual const SDP::Frame::Base & curr() const throw( KGD::Exception::OutOfBounds );
					//! returns frame at current position then advances position by 1
					virtual const SDP::Frame::Base & next() throw( KGD::Exception::OutOfBounds );
					//! seeks first frame at specified time. changes current position
					virtual const SDP::Frame::Base & seek( double ) throw( KGD::Exception::OutOfBounds );
					//! seeks to a position
					virtual const SDP::Frame::Base & seek( size_t ) throw( KGD::Exception::OutOfBounds );
					//! returns current position
					virtual size_t pos() const throw();
					//! returns number of effective frames in this medium
					virtual size_t size() const throw();
					//! returns duration of this iteration in seconds
					virtual double duration() const throw();
					//! inserts another medium
					virtual void insert( Iterator::Base &, double t ) throw( KGD::Exception::OutOfBounds );
					//! inserts a void space
					virtual void insert( double duration, double t ) throw( KGD::Exception::OutOfBounds );
				};
*/

			}
		}
	}
}

#endif
