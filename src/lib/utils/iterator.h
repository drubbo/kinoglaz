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
 * File name: ./lib/utils/iterator.h
 * First submitted: 2010-07-13
 * First submitter: Emiliano Leporati <emiliano.leporati@gmail.com>
 * Contributor(s) so far - 2010-07-30 :
 *     Emiliano Leporati <emiliano.leporati@gmail.com>
 *
 * Last changes :
 *     
 *
 **/

#ifndef __KGD_ITERATOR
#define __KGD_ITERATOR

#include "lib/common.h"
#include "lib/utils/container.hpp"

namespace KGD
{
	namespace Ctr
	{
		//! generic iterator operations
		template< class C, class IT, class Ret >
		class IteratorBase
		{
		protected:
			//! iterator over data
			IT _it;
			//! begin of data
			IT _bg;
			//! end of data
			IT _ed;
			//! number of elements
			size_t _n;
			//! build from a range
			IteratorBase( IT, IT, size_t );
		public:
			//! rewind
			virtual IteratorBase & first() throw();
			//! advance
			virtual IteratorBase & next() throw( Exception::OutOfBounds );
			//! tells if iterator is still before end
			bool isValid() const throw( );
			//! tells how many records we're iterating over
			size_t size() const throw();
			//! to std
			operator IT() const throw();

			//! get current val
			Ret & val() const throw( Exception::NullPointer );
			//! dereference
			Ret & operator*() const throw( Exception::NullPointer );
			//! member access
			Ret * operator->() const throw( Exception::NullPointer );
		};


		//! iterator supporting value seek
		template< class C, class IT, class Ret, class Srch >
		class IteratorSeekBase
			: public IteratorBase< C, IT, Ret >
		{
		protected:
			//! build from a range
			IteratorSeekBase( IT, IT, size_t );
		public:

			//! find
			bool has( const Srch & ) const throw( );
			//! find
			IteratorSeekBase & find( const Srch & ) throw( );
		};

		//! iterator over a std container
		template< class C >
		class Iterator
			: public IteratorBase<
				C, typename C::iterator,
				typename C::value_type
			>
		{
		public:
			Iterator( C & );

			//! rewind
			virtual Iterator & first() throw();
			//! advance
			virtual Iterator & next() throw( Exception::OutOfBounds );
		};


		//! const iterator over a std container
		template< class C >
		class ConstIterator
			: public IteratorBase<
				C, typename C::const_iterator,
				const typename C::value_type
			>
		{
		public:
			//! build from a const map
			ConstIterator( const C & );

			//! rewind
			virtual ConstIterator & first() throw();
			//! advance
			virtual ConstIterator & next() throw( Exception::OutOfBounds );
		};


		//! iterator over a std container
		template< class C >
		class SeekableIterator
			: public IteratorSeekBase<
				C, typename C::iterator,
				typename C::value_type,
				typename C::value_type
			>
		{
		public:
			SeekableIterator( C & );

			//! rewind
			virtual SeekableIterator & first() throw();
			//! advance
			virtual SeekableIterator & next() throw( Exception::OutOfBounds );
		};


		//! const iterator over a std container
		template< class C >
		class ConstSeekableIterator
			: public IteratorSeekBase<
				C, typename C::const_iterator,
				const typename C::value_type,
				typename C::value_type
			>
		{
		public:
			//! build from a const map
			ConstSeekableIterator( const C & );

			//! rewind
			virtual ConstSeekableIterator & first() throw();
			//! advance
			virtual ConstSeekableIterator & next() throw( Exception::OutOfBounds );
		};
	}
}

#endif

