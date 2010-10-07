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
 * File name: ./lib/utils/singleton.hpp
 * First submitted: 2010-01-16
 * First submitter: Emiliano Leporati <emiliano.leporati@gmail.com>
 * Contributor(s) so far - 2010-07-30 :
 *     Emiliano Leporati <emiliano.leporati@gmail.com>
 *
 * Last changes :
 *     Kinoglaz birth: pervasive bug fixing, improvements and client support
 *     frame / other stuff refactor
 *     interleave ok
 *     first import
 *
 **/

#ifndef __KGD_UTILS_SINGLETON_HPP
#define __KGD_UTILS_SINGLETON_HPP

#include "lib/utils/singleton.h"

using namespace std;

namespace KGD
{
	namespace Singleton
	{
		template< typename T >
		InstanceRef< T >::InstanceRef( )
		{
			RLock lk( _mux );
			++ _count;
		}

		template< typename T >
		InstanceRef< T >::InstanceRef( const InstanceRef< T >& )
		{
			RLock lk( _mux );
			++ this->_count;
		}

		template< typename T >
		InstanceRef< T >::~InstanceRef( )
		{
			RLock lk( this->_mux );
			if ( (-- this->_count) <= 0 )
				T::destroyInstance();
		}

		template< typename T > T * InstanceRef< T >::operator->()
		{
			if ( ! T::_instance )
				throw runtime_error( "dereferencing NULL ptr" );
			return T::_instance;
		}
		template< typename T > const T * InstanceRef< T >::operator->() const
		{
			if ( ! T::_instance )
				throw runtime_error( "dereferencing NULL ptr" );
			return T::_instance;
		}

		template< typename T > T & InstanceRef< T >::operator*()
		{
			if ( ! T::_instance )
				throw runtime_error( "dereferencing NULL ptr" );
			return *T::_instance;
		}
		template< typename T > const T & InstanceRef< T >::operator*() const
		{
			if ( ! T::_instance )
				throw runtime_error( "dereferencing NULL ptr" );
			return *T::_instance;
		}


		template< typename T >
		Class< T >::Class()
		{
		}

		template< typename T >
		void Class< T >::destroyInstance()
		{
			RLock lk( _mux );
			if ( _instance )
				Ptr::clear< T >( _instance );
		}

		template< typename T >
		InstanceRef< T > Class< T >::newInstanceRef()
		{
			return InstanceRef< T >();
		}

		template< typename T >
		InstanceRef< T > Class< T >::getInstance()
		{
			RLock lk( _mux );
			if ( !_instance )
				_instance = new T;

			return newInstanceRef();
		}


		template< typename T >
		Persistent< T >::Persistent()
		{
		}

		template< class T >
		void Persistent< T >::destroyInstance()
		{
			_instance.destroy();
		}

		template< typename T >
		T& Persistent< T >::getInstance()
		{
			RLock lk( _mux );

			if ( !_instance )
				_instance = new T;

			return *_instance;
		}

		template< typename T > RMutex InstanceRef< T >::_mux;
		template< typename T > long InstanceRef< T >::_count = 0;
		template< typename T > RMutex Class< T >::_mux;
		template< typename T > T* Class< T >::_instance = 0;


		template< typename T > RMutex Persistent< T >::_mux;
		template< typename T > Ptr::Scoped< T > Persistent< T >::_instance = 0;
	}
}


#endif
