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
		template< class T >
		Class< T >::Class()
		{
		}

		template< class T >
		typename Class< T >::Reference Class< T >::newInstanceRef()
		{
			return Reference( *_instance );
		}

		template< class T >
		void Class< T >::lock()
		{
			_instance.lock();
		}
		template< class T >
		void Class< T >::unlock()
		{
			_instance.unlock();
		}
		template< class T >
		RMutex & Class< T >::mux()
		{
			return _instance.mux();
		}


		template< class T >
		typename Class< T >::Reference Class< T >::getInstance()
		{
			typename Instance::Lock lk( _instance );
			if ( !*_instance )
				(*_instance).reset( new T );

			return newInstanceRef();
		}

		template< class T >
		void Class< T >::destroyInstance()
		{
			BOOST_ASSERT( (*_instance).unique() );
			(*_instance).reset();
		}


		template< class T > typename Class< T >::Instance Class< T >::_instance;
	}
}


#endif
