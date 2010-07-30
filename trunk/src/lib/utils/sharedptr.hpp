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
 * File name: ./lib/utils/sharedptr.hpp
 * First submitted: 2010-01-16
 * First submitter: Emiliano Leporati <emiliano.leporati@gmail.com>
 * Contributor(s) so far - 2010-07-30 :
 *     Emiliano Leporati <emiliano.leporati@gmail.com>
 *
 * Last changes :
 *     Kinoglaz birth: pervasive bug fixing, improvements and client support
 *     should not need interlocking on socket write (should)
 *     pre-interleave
 *     first import
 *
 **/

#ifndef __KGD_UTILS_SHPOINTER_HPP
#define __KGD_UTILS_SHPOINTER_HPP

#include "lib/utils/sharedptr.h"

using namespace std;

namespace KGD
{
	//! Pointer management utils
	namespace Ptr
	{
		template< class T >
		Shared< T >::Common::Common( T* p ) throw( KGD::Exception::Generic )
		: _ptr( p )
		, _refCount( 1 )
		{
			if ( !p )
				throw KGD::Exception::Generic("constructing shared NULL pointer");
		}
		template< class T >
		Shared< T >::Common::~Common()
		{
			Ptr::clear( _ptr );
		}
		template< class T >
		typename Shared< T >::Common& Shared< T >::Common::operator++()
		{
			++ _refCount;
			return *this;
		}
		template< class T >
		typename Shared< T >::Common& Shared< T >::Common::operator--()
		{
			-- _refCount;
			return *this;
		}
		template< class T >
		Shared< T >::Common::operator long() const
		{
			return _refCount.getValue();
		}
		template< class T >
		T* Shared< T >::Common::get()
		{
			return _ptr;
		}
		template< class T >
		const T* Shared< T >::Common::get() const
		{
			return _ptr;
		};

		template< class T >
		Shared< T >::Shared( T * p ) throw( KGD::Exception::Generic )
		: _ptr( new Common( p ) )
		{
		}

		template< class T >
		Shared< T >::Shared( const Shared< T > & s )
		: _ptr( s._ptr )
		{
			++ (*_ptr);
		}
		template< class T >
		Shared< T >::~Shared()
		{
			if ( -- (*_ptr) <= 0 )
				Ptr::clear( _ptr );
		}

		template< class T >
		void Shared< T >::lock() const
		{
			_ptr->mux.lock();
		}

		template< class T >
		void  Shared< T >::unlock() const
		{
			_ptr->mux.unlock();
		}

		template< class T >
		T * Shared< T >::get()
		{
			return _ptr->get();
		}
		template< class T >
		const T * Shared< T >::get() const
		{
			return _ptr->get();
		}
		template< class T >
		T * Shared< T >::operator->()
		{
			return _ptr->get();
		}
		template< class T >
		const T * Shared< T >::operator->() const
		{
			return _ptr->get();
		}
		template< class T >
		T & Shared< T >::operator*()
		{
			return *(_ptr->get());
		}
		template< class T >
		const T & Shared< T >::operator*() const
		{
			return *(_ptr->get());
		}

	}
}

#endif
