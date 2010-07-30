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
 * File name: ./lib/utils/pointer.hpp
 * First submitted: 2010-01-16
 * First submitter: Emiliano Leporati <emiliano.leporati@gmail.com>
 * Contributor(s) so far - 2010-07-30 :
 *     Emiliano Leporati <emiliano.leporati@gmail.com>
 *
 * Last changes :
 *     Kinoglaz birth: pervasive bug fixing, improvements and client support
 *     fixed some bug in Log and in insertMedia; comments
 *     interleave ok
 *     pre-interleave
 *     first import
 *
 **/

#ifndef __KGD_UTILS_POINTER_HPP
#define __KGD_UTILS_POINTER_HPP

#include "lib/utils/pointer.h"

using namespace std;
namespace KGD
{
	namespace Ptr
	{

		// ******************************************************************************************

		template < class T > void clear ( T* &ptr )
		{
			if ( ptr )
			{
				delete ptr;
				ptr = 0;
			}
		}

		template < class T > T * clone ( T const * const ptr )
		{
			if ( ptr )
				return new T( *ptr );
			else
				return 0;
		}
		// ******************************************************************************************

		template < class T >
		Ref< T >::Ref( )
		: _ref( 0 )
		{
		}

		template < class T >
		Ref< T >::Ref( T & obj )
		: _ref( & obj )
		{
		}

		template < class T >
		template < class S >
		Ref< T >::Ref( Ref< S > & obj )
		: _ref( 0 )
		{
			if ( obj.isValid() )
				_ref = reinterpret_cast< T* >( & obj.get() );
		}

		template < typename T >
		void Ref< T >::invalidate( )
		{
			_ref = 0 ;
		}

		template < class T >
		bool Ref< T >::isValid( ) const
		{
			return ( _ref != 0 );
		}

		template < class T >
		Ref< T >::operator const T &() const throw( Exception::NullPointer )
		{
			if ( ! this->_ref )
				throw Exception::NullPointer();
			return *_ref;
		}

		template < class T >
		Ref< T >::operator T &() throw( Exception::NullPointer )
		{
			if ( ! this->_ref )
				throw Exception::NullPointer();
			return *_ref;
		}

		template < class T >
		const T & Ref< T >::get() const throw( Exception::NullPointer )
		{
			if ( ! this->_ref )
				throw Exception::NullPointer();
			return *_ref;
		}

		template < class T >
		T & Ref< T >::get() throw( Exception::NullPointer )
		{
			if ( ! this->_ref )
				throw Exception::NullPointer();
			return *_ref;
		}

		template < class T >
		const T * Ref< T >::operator->() const throw( Exception::NullPointer )
		{
			if ( ! this->_ref )
				throw Exception::NullPointer();
			return _ref;
		}

		template < class T >
		T * Ref< T >::operator->() throw( Exception::NullPointer )
		{
			if ( ! this->_ref )
				throw Exception::NullPointer();
			return _ref;
		}

		template < class T >
		const T & Ref< T >::operator*() const throw( Exception::NullPointer )
		{
			if ( ! this->_ref )
				throw Exception::NullPointer();
			return *_ref;
		}

		template < class T >
		T & Ref< T >::operator*() throw( Exception::NullPointer )
		{
			if ( ! this->_ref )
				throw Exception::NullPointer();
			return *_ref;
		}

		template < class T >
		T * Ref< T >::getPtr() throw( Exception::NullPointer )
		{
			if ( ! this->_ref )
				throw Exception::NullPointer();
			return _ref;
		}

		template < class T >
		const T * Ref< T >::getPtr() const throw( Exception::NullPointer )
		{
			if ( ! this->_ref )
				throw Exception::NullPointer();
			return _ref;
		}

		template < class T >
		Ref< T > & Ref< T >::operator=( T & obj )
		{
			_ref = & obj;
			return *this;
		}

		template< class T > Ref< T > toRef( T * p )
		{
			if ( p )
				return Ref< T >( *p );
			else
				return Ref< T >();
		}

		template< class T > Ref< const T > toConstRef( T const * p )
		{
			if ( p )
				return Ref< const T >( *p );
			else
				return Ref< const T >();
		}

		// ******************************************************************************************


		template < class T >
		Shell< T >::Shell( )
		: _ptr( 0 ), _attached( false )
		{ }

		template < class T >
		Shell< T >::Shell( T* p )
		: _ptr( p ), _attached( true )
		{
		}

		template < class T >
		Shell< T >::~Shell()
		{
			this->destroy();
		}

		template < class T >
		Shell< T >::operator bool() const
		{
			return (_ptr != 0);
		}

		template < class T >
		Shell< T >& Shell< T >::operator= ( T * p )
		{
			if ( this->_ptr != p )
			{
				this->destroy();
				this->_ptr = p;
				this->_attached = true;
			}

			return *this;
		}

		template < class T >
		T * Shell< T >::detach()
		{
			this->_attached = false;
			return this->_ptr;
		}

		template < class T >
		T * Shell< T >::release()
		{
			T * tmp = this->_ptr;
			this->_ptr = 0;
			this->_attached = false;
			return tmp;
		}

		template < class T >
		void Shell< T >::destroy()
		{
			if ( this->_attached )
			{
				clear( this->_ptr );
			}
			else
				this->_ptr = 0;
		}

		template < class T >
		T * & Shell< T >::get()
		{
			return _ptr;
		}

		template < class T >
		const T * Shell< T >::get() const
		{
			return _ptr;
		}

		// ******************************************************************************************

		template < class T >
		Scoped< T >::Scoped( )
		: Shell< T >( )
		{}

		template < class T >
		Scoped< T >::Scoped( T * p )
		: Shell< T >( p )
		{}

		template < class T >
		Scoped< T >::Scoped( const size_t sz )
		: Shell< T >( sz ? new T[ sz ] : 0 )
		{
			if ( sz )
				memset( this->_ptr, 0, sz );
		}

		template < class T >
		Scoped< T > & Scoped< T >::operator=( T * p )
		{
			Shell< T >::operator=( p );
			return *this;
		}

		template < class T >
		T * Scoped< T >::operator->() throw( Exception::NullPointer )
		{
			if ( ! this->_ptr )
				throw Exception::NullPointer();
			return this->_ptr;
		}

		template < class T >
		const T * Scoped< T >::operator->() const throw( Exception::NullPointer )
		{
			if ( ! this->_ptr )
				throw Exception::NullPointer();
			return this->_ptr;
		}

		template < class T >
		T & Scoped< T >::operator*() throw( Exception::NullPointer )
		{
			if ( ! this->_ptr )
				throw Exception::NullPointer();
			return *this->_ptr;
		}

		template < class T >
		const T & Scoped< T >::operator*() const throw( Exception::NullPointer )
		{
			if ( ! this->_ptr )
				throw Exception::NullPointer();
			return *this->_ptr;
		}

		// ******************************************************************************************

		template < class T >
		Copyable< T >::Copyable( )
		: Scoped< T >( )
		{}

		template < class T >
		Copyable< T >::Copyable( T * p )
		: Scoped< T >( p )
		{}

		template < class T >
		Copyable< T >::Copyable( const T & p )
		: Scoped< T >( clone( & p ) )
		{}

		template < class T >
		Copyable< T >::Copyable( const Copyable< T > & p )
		: Scoped< T >( clone( p.get() ) )
		{}
		template < class T >
		Copyable< T >::Copyable( const size_t sz )
		: Scoped< T >( new T[ sz ] )
		{
			memset( this->_ptr, 0, sz );
		}

		template < class T >
		Copyable< T > & Copyable< T >::operator=( T * p )
		{
			Shell< T >::operator=( p );
			return *this;
		}

		template < class T >
		Copyable< T > & Copyable< T >::operator=( const T & p )
		{
			if ( &p != this->_ptr )
				Shell< T >::operator=( clone( & p ) );
			return *this;
		}

		template < class T >
		Copyable< T > & Copyable< T >::operator=( const Copyable< T > & p )
		{
			if ( p.get() != this->_ptr )
				Shell< T >::operator=( clone( p.get() ) );
			return *this;
		}
	}
}
#endif

