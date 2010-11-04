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
 * File name: src/lib/utils/ref.hpp
 * First submitted: 2010-10-18
 * First submitter: Emiliano Leporati <emiliano.leporati@gmail.com>
 * Contributor(s) so far - 2010-11-04 :
 *     Emiliano Leporati <emiliano.leporati@gmail.com>
 *
 * Last changes :
 *     boosted
 *
 **/

#ifndef __KGD_UTILS_REF_HPP
#define __KGD_UTILS_REF_HPP

#include "lib/utils/ref.h"

using namespace std;

namespace KGD
{
	template < class T >
	ref< T >::ref()
	: _ptr( 0 )
	{
	}


	template < class T >
	ref< T >::ref( T & obj )
	: _ptr( & obj )
	{
	}

	template < class T >
	template< class S >
	ref< T >::ref( ref< S > & obj )
	: _ptr( 0 )
	{
		if ( obj.isValid() )
		{
			if ( ! (_ptr = dynamic_cast< T* >( obj.getPtr() ) ) )
				throw Exception::InvalidType( typeid( S ).name() + string(" not related to ") + typeid( T ).name() );
		}
	}

	template < class T >
	ref< T >::operator bool() const { return _ptr != 0; }

	template < class T >
	bool ref< T >::isValid() const { return _ptr != 0; }

	template < class T >
	void ref< T >::invalidate() { _ptr = 0; }

	template < class T >
	ref< T > & ref< T >::operator=( T & obj )
	{
		_ptr = & obj;
		return *this;
	}

	template < class T >
	T& ref< T >::get() throw( Exception::NullPointer )
	{
		if ( ! this->_ptr )
			throw Exception::NullPointer();
		return *_ptr;
	}

	template < class T >
	const T& ref< T >::get() const throw( Exception::NullPointer )
	{
		if ( ! this->_ptr )
			throw Exception::NullPointer();
		return *_ptr;
	}

	template < class T >
	T* ref< T >::getPtr() throw( Exception::NullPointer )
	{
		if ( ! this->_ptr )
			throw Exception::NullPointer();
		return _ptr;
	}

	template < class T >
	const T* ref< T >::getPtr() const throw( Exception::NullPointer )
	{
		if ( ! this->_ptr )
			throw Exception::NullPointer();
		return _ptr;
	}

	template < class T >
	T* ref< T >::operator->() throw( Exception::NullPointer )
	{
		if ( ! this->_ptr )
			throw Exception::NullPointer();
		return _ptr;
	}

	template < class T >
	const T* ref< T >::operator->() const throw( Exception::NullPointer )
	{
		if ( ! this->_ptr )
			throw Exception::NullPointer();
		return _ptr;
	}

	template < class T >
	T& ref< T >::operator*() throw( Exception::NullPointer )
	{
		if ( ! this->_ptr )
			throw Exception::NullPointer();
		return *_ptr;
	}

	template < class T >
	const T& ref< T >::operator*() const throw( Exception::NullPointer )
	{
		if ( ! this->_ptr )
			throw Exception::NullPointer();
		return *_ptr;
	}
}
#endif
