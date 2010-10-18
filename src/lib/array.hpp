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
 * File name: ./lib/array.hpp
 * First submitted: 2010-01-16
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

#ifndef __KGD_ARRAY_HPP
#define __KGD_ARRAY_HPP

#include "lib/array.h"
#include "lib/log.h"

#include <iomanip>

namespace KGD
{
	template< class T >
	Array< T >::Array( size_t sz )
	: _ptr( 0 )
	, _size( sz )
	{
		if ( this->_size )
		{
			this->_ptr = new T[ this->_size ];
			memset( this->_ptr, 0, this->_size );
		}
	}

	template< class T >
	Array< T >::Array( Array const & a )
	: _ptr( 0 )
	, _size( a._size )
	{
		if ( this->_size )
		{
			this->_ptr = new T[ this->_size ];
			memcpy( this->_ptr, a._ptr, this->_size );
		}
	}

	template< class T >
	Array< T >::Array( void const * data, size_t sz )
	: _ptr( 0 )
	, _size( sz )
	{
		if ( this->_size )
		{
			this->_ptr = new T[ this->_size ];
			memcpy( this->_ptr, data, this->_size );
		}
	}

	template< class T >
	Array< T >::~Array( ) throw()
	{
		this->clear();
	}

	template< class T >
	void Array< T >::clear( ) throw()
	{
		if ( this->_ptr )
			delete [] this->_ptr, this->_ptr = 0;
	}


	template< class T >
	void Array< T >::swap( Array & a ) throw()
	{
		std::swap( this->_size, a._size );
		std::swap( this->_ptr, a._ptr );
	}
	
	template< class T >
	Array< T > & Array< T >::operator=( Array const & a )
	{
		Array< T > tmp( a );
		this->swap( tmp );
		return *this;
	}

	template< class T >
	size_t Array< T >::size() const
	{
		return _size;
	}
	template< class T >
	bool Array< T >::empty() const
	{
		return _size == 0;
	}
	template< class T >
	T * Array< T >::get() throw()
	{
		return this->_ptr;
	}
	template< class T >
	T const * Array< T >::get() const throw()
	{
		return const_cast< T const * const >( this->_ptr );
	}

	template< class T >
	T& Array< T >::operator[](size_t i) throw( KGD::Exception::OutOfBounds )
	{
		if ( i >= _size )
			throw KGD::Exception::OutOfBounds(i, 0, _size - 1);
		return this->_ptr[i];
	}
	template< class T >
	const T& Array< T >::operator[](size_t i) const throw( KGD::Exception::OutOfBounds )
	{
		if ( i >= _size )
			throw KGD::Exception::OutOfBounds(i, 0, _size - 1);
		return this->_ptr[i];
	}

	template< class T >
	template< class S >
	Array< T >& Array< T >::append( const S & data ) throw( )
	{
		return this->append( &data, sizeof(S) );
	}

	template< class T >
	template< class S >
	Array< T >& Array< T >::appendArray( const Array< S > & data ) throw( )
	{
		return this->append( data.get(), data.size() );
	}

	template< class T >
	Array< T >& Array< T >::append( void const * const data, size_t sz ) throw( )
	{
		size_t curSz = _size;
		this->resize( _size + sz );
		memcpy( &(this->_ptr[curSz]), data, sz );
		return *this;
	}


	template< class T >
	template< class S >
	Array< T >& Array< T >::insert( const S & data, size_t pos ) throw( KGD::Exception::OutOfBounds )
	{
		return this->insert( &data, sizeof(S), pos );
	}

	template< class T >
	template< class S >
	Array< T >& Array< T >::insertArray( const Array< S > & data, size_t pos ) throw( KGD::Exception::OutOfBounds )
	{
		return this->insert( data.get(), data.size(), pos );
	}

	template< class T >
	Array< T >& Array< T >::insert( void const * const data, size_t sz, size_t pos ) throw( KGD::Exception::OutOfBounds )
	{
		if ( pos >= _size )
			throw KGD::Exception::OutOfBounds( pos, 0, _size );


		size_t curSz = _size;
		this->resize( _size + sz );
		memcpy( &(this->_ptr[curSz]), data, sz );
		return *this;
	}

	template< class T >
	template< class S >
	Array< T >& Array< T >::set( const S & data, size_t pos ) throw( KGD::Exception::OutOfBounds )
	{
		return this->set( &data, sizeof(S), pos );
	}
	template< class T >
	template< class S >
	Array< T >& Array< T >::setArray( const Array< S > & data, size_t pos ) throw( KGD::Exception::OutOfBounds )
	{
		return this->set( data.get(), data.size(), pos );
	}
	template< class T >
	Array< T >& Array< T >::set( void const * const data, size_t sz, size_t pos ) throw( KGD::Exception::OutOfBounds )
	{
		if ( pos == _size )
			return this->append( data, sz );
		else if ( pos > _size )
			throw KGD::Exception::OutOfBounds( pos, 0, _size );
		else
		{
			if ( pos + sz > _size )
				this->resize( pos + sz );
			memcpy( &(this->_ptr[ pos ] ), data, sz );
			return *this;
		}
	}

	template< class T >
	string Array< T >::toStdString() const throw()
	{
		string rt;
		for( size_t i = 0; i < _size && this->_ptr[i]; ++i )
			rt.push_back( this->_ptr[i] );
		if ( rt[ rt.size() - 1 ] != '\0' )
			rt.push_back( '\0' );
		return rt;
	}


	template< class T >
	string Array< T >::toString() const throw()
	{
		ostringstream rt;
		for( size_t i = 0; i < _size; ++i )
			rt << hex << setw( 2 ) << setfill('0') << int( this->_ptr[i] ) << " ";
		return rt.str();
	}

	template< class T >
	size_t Array< T >::copyTo( void * dest, size_t sz, size_t from ) throw( )
	{
		if ( from < _size )
		{
			size_t rt = min( _size - from, sz );
			memcpy( dest, &(this->_ptr[ from ]), rt );
			return rt;
		}
		else
			return 0;
	}

	template< class T >
	Array< T > & Array< T >::resize( size_t sz ) throw()
	{
		if ( sz != this->_size )
		{
			if ( sz > 0 )
			{
				this->_ptr = (T*)realloc( this->_ptr, sz );
				if ( sz > this->_size )
					memset( &this->_ptr[ this->_size ], 0, sz - this->_size );
				this->_size = sz;
			}
			else
				this->clear();
		}
		return *this;
	}

	template< class T >
	Array< T > * Array< T >::popFront( size_t n ) throw()
	{
		Array< T > * rt = new Array< T >( n );
		rt->resize( this->copyTo( rt->get(), n ) );
		this->chopFront( rt->size() );
		return rt;
	}

	template< class T >
	void Array< T >::chopFront( size_t n ) throw()
	{
		if ( n >= this->_size )
			this->resize( 0 );
		else
		{
			size_t sz = this->_size - n;
			memmove( &this->_ptr[ n ], this->_ptr, sz );
			this->resize( sz );
		}
	}

	template< class T >
	Array< T > * Array< T >::popBack( size_t n ) throw()
	{
		Array< T > * rt;
		if ( n >= this->size() )
		{
			rt = new Array< T >( *this );
			this->resize( 0 );
		}
		else
		{
			rt = new Array< T >( n );
			rt->resize( this->copyTo( rt->get(), n, this->_size - n ) );
			this->resize( this->_size - rt->size() );
		}
		return rt;
	}

	template< class T >
	void Array< T >::chopBack( size_t n ) throw()
	{
		if ( n > this->_size )
			this->resize( 0 );
		else
			this->resize( this->_size - n );
	}


	namespace Base64
	{
		extern void encodeBlock( const unsigned char in[3], unsigned char out[4], size_t len ) throw();
		extern void decodeBlock( const unsigned char in[4], unsigned char out[3] ) throw();

		template< class T >
		auto_ptr< Array< T > > encode( const Array< T >& b )
		{
			unsigned char in[3], out[4];
			auto_ptr< Array< T > > rt( new Array< T >( 0 ) );
			for( size_t i = 0; i < b.size(); i += 3 )
			{
				memset( in, 0, 3 );
				size_t cpN = min( b.size(), i + 3 ) % 3;
				if ( cpN == 0 ) cpN = 3;
				for( size_t cp = 0; cp < cpN; ++cp )
					in[cp] = b[ i + cp ];
				encodeBlock( in, out, cpN );
				rt->append( out, 4 );
			}
			return rt;
		}

		template< class T >
		auto_ptr< Array< T > > decode( const Array< T >& b )
		{
			static const char cd64[]="|$$$}rstuvwxyz{$$$$$$$>?@ABCDEFGHIJKLMNOPQRSTUVW$$$$$$XYZ[\\]^_`abcdefghijklmnopq";

			unsigned char in[4], out[3];
			auto_ptr< Array< T > > rt( new Array< T >( 0 ) );
			for( size_t i = 0; i < b.size(); i += 4 )
			{
				memset( in, 0, 4 );
				size_t cpN = min( b.size(), i + 4 ) % 4;
				if ( cpN == 0 ) cpN = 4;
				for( size_t cp = 0; cp < cpN; ++cp )
				{
					unsigned char c = b[ i + cp ];
					c = (c < 43 || c > 122) ? 0 : cd64[ c - 43 ];
					if( c ) c = (c == '$') ? 0 : c - 62;
					in[cp] = c;
				}
				decodeBlock( in, out );
				rt->append( out, 3);
			}
			return rt;
		}

		template< class T >
		auto_ptr< Array< T > > decode( const string & s )
		{
			Array< T > data( s.c_str(), s.size() );
			return decode( data );
		}
	}
}

#endif
