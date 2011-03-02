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
 * File name: src/lib/utils/virtual.h
 * First submitted: 2010-07-30
 * First submitter: Emiliano Leporati <emiliano.leporati@gmail.com>
 * Contributor(s) so far - 2010-11-04 :
 *     Emiliano Leporati <emiliano.leporati@gmail.com>
 *
 * Last changes :
 *     Added AAC support
 *     source import
 *
 **/


#ifndef __KGD_VIRTUAL_H
#define __KGD_VIRTUAL_H

#include <typeinfo>

using namespace std;

namespace KGD
{
	//! can type test and type cast itself polymorphically
	class Virtual
	{
	public:
		//! to ensure class is virtual
		virtual ~Virtual();

		//! returns true if object has dynamic type T
		template< class T > bool hasType() const;
		//! Tries a dynamic cast to a const reference to T
		template< class T > const T & as() const throw ( bad_cast );
		//! Tries a dynamic cast to a reference to T
		template< class T > T & as() throw ( bad_cast );

		//! Tries a dynamic cast to a pointer to T, raising exception on failure
		template< class T > const T * asPtr() const throw ( bad_cast );
		//! Tries a dynamic cast to a pointer to T, raising exception on failure
		template< class T > T * asPtr() throw ( bad_cast );

		//! Tries a dynamic cast to a pointer to T, returning 0 on failure
		template< class T > const T * asPtrUnsafe() const throw ( );
		//! Tries a dynamic cast to a pointer to T, returning 0 on failure
		template< class T > T * asPtrUnsafe() throw ( );
	};
}

#endif

