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
