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
 * File name: src/lib/utils/ref.h
 * First submitted: 2010-10-18
 * First submitter: Emiliano Leporati <emiliano.leporati@gmail.com>
 * Contributor(s) so far - 2010-11-04 :
 *     Emiliano Leporati <emiliano.leporati@gmail.com>
 *
 * Last changes :
 *     boosted
 *
 **/

#ifndef __KGD_UTILS_REF_H
#define __KGD_UTILS_REF_H

#include <lib/exceptions.h>
#include <typeinfo>

using namespace std;

namespace KGD
{

	//! a reference adaptor to safely return container of references, copy references, etc.
	template < class T > class ref
	{
	private:
		//! the reference is managed here by its pointer, that won't be deallocated
		T * _ptr;
	public:
		//! construct an invalid reference to NULL
		ref();
		//! construct from a reference
		ref( T & obj );
		//! construct from a Ref
		template< class S >
		ref( ref< S > & obj );
		//! tells if reference is to an object or to NULL
		operator bool() const;
		//! tells if reference is to an object or to NULL
		bool isValid() const;
		//! puts reference to NULL
		void invalidate();
		//! change reference to another object
		ref & operator=( T & obj );

		//! get reference
		T& get() throw( Exception::NullPointer );
		//! get const reference
		const T& get() const throw( Exception::NullPointer );

		//! get hidden pointer
		T* getPtr() throw( Exception::NullPointer );

		//! get hidden pointer
		const T* getPtr() const throw( Exception::NullPointer );

		//! deref
		T* operator->() throw( Exception::NullPointer );
		//! const deref
		const T* operator->() const throw( Exception::NullPointer );
		//! deref
		T& operator*() throw( Exception::NullPointer );
		//! const deref
		const T& operator*() const throw( Exception::NullPointer );
	};
}
#endif

