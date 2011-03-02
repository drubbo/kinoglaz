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
 * File name: src/lib/array.h
 * First submitted: 2010-07-30
 * First submitter: Emiliano Leporati <emiliano.leporati@gmail.com>
 * Contributor(s) so far - 2010-11-04 :
 *     Emiliano Leporati <emiliano.leporati@gmail.com>
 *
 * Last changes :
 *     boosted
 *     source import
 *
 **/


#ifndef __KGD_ARRAY_H
#define __KGD_ARRAY_H

#include <memory>
#include "lib/exceptions.h"

namespace KGD
{
	//! variable sized memory area
	template< class T >
	class Array
	{
	protected:
		//! allocated memory
		T * _ptr;
		//! allocated size
		size_t _size;
	public:
		//! create with allocated size
		Array( size_t );
		//! create from existing data
		Array( void const *, size_t );
		//! copy ctor
		Array( Array const & );
		//! dtor
		~Array() throw();

		//! swap
		void swap( Array & ) throw();
		//! cleanup
		void clear() throw();
		
		//! assign
		Array & operator=( Array const & );
		
		//! returns max size
		size_t size() const;
		//! tells if there is some data
		bool empty() const;
		//! returns inner ptr
		T * get() throw();
		//! returns inner ptr
		T const * get() const throw();

		//! returns element at position i
		T& operator[](size_t i) throw( KGD::Exception::OutOfBounds );
		//! returns element at position i
		const T& operator[](size_t i) const throw( KGD::Exception::OutOfBounds );

		//! appends data
		template< class S >
		Array< T >& append( const S & ) throw( );
		//! appends data
		Array< T >& append( void const * const, size_t ) throw( );
		//! append another array
		template< class S >
		Array< T >& appendArray( const Array< S > & ) throw( );

		//! inserts data at a specified position
		template< class S >
		Array< T >& insert( const S &, size_t pos ) throw( KGD::Exception::OutOfBounds );
		//! inserts data at a specified position
		Array< T >& insert( void const * const, size_t sz, size_t pos) throw( KGD::Exception::OutOfBounds );
		//! insert another array
		template< class S >
		Array< T >& insertArray( const Array< S > &, size_t pos ) throw( KGD::Exception::OutOfBounds );

		//! overwrite data at specified position resizing if needed
		template< class S >
		Array< T >& set( const S &, size_t pos ) throw( KGD::Exception::OutOfBounds );
		//! overwrite data at specified position resizing if needed
		Array< T >& set( void const * const, size_t sz, size_t pos ) throw( KGD::Exception::OutOfBounds );
		//! overwrite data at specified position resizing if needed
		template< class S >
		Array< T >& setArray( const Array< S > &, size_t pos ) throw( KGD::Exception::OutOfBounds );

		//! extracts data from array and puts to dest; returns number of bytes copied
		size_t copyTo( void * dest, size_t sz, size_t from = 0 ) throw( );

		//! pops n bytes from beginning of array, returning them
		Array< T > * popFront( size_t ) throw();
		//! removes n bytes from beginning of array
		void chopFront( size_t ) throw();
		//! pops n bytes from end of array, returning them
		Array< T > * popBack( size_t ) throw();
		//! removes n bytes from end of array
		void chopBack( size_t ) throw();

		//! resize to a new size mantaining old data; new data is set to 0
		Array< T > & resize( size_t ) throw();

		//! get string as if array contains a const char *
		string toStdString() const throw();
		//! get bytes of this array in hex format
		string toString() const throw();
	};

	//! our char array
	typedef Array< char > CharArray;
	//! our byte array
	typedef Array< unsigned char > ByteArray;

	//! Base64 encoding / decoding facilities
	namespace Base64
	{
		//! decode a string to array of desired type
		template< class T > auto_ptr< Array< T > > decode( const string& b );
		//! decode an array to another
		template< class T > auto_ptr< Array< T > > decode( const Array< T >& b );
		//! encode an array to another
		template< class T > auto_ptr< Array< T > > encode( const Array< T >& b );
	};
}

#endif
