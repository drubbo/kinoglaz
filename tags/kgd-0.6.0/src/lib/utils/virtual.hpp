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
 * File name: ./lib/utils/virtual.hpp
 * First submitted: 2010-01-16
 * First submitter: Emiliano Leporati <emiliano.leporati@gmail.com>
 * Contributor(s) so far - 2010-07-30 :
 *     Emiliano Leporati <emiliano.leporati@gmail.com>
 *
 * Last changes :
 *     Kinoglaz birth: pervasive bug fixing, improvements and client support
 *     first import
 *
 **/

#ifndef __KGD_VIRTUAL_HPP
#define __KGD_VIRTUAL_HPP

#include "lib/utils/virtual.h"

namespace KGD
{

	template< class T >
	bool Virtual::hasType() const
	{
		const T * ptr = dynamic_cast< const T * > ( this );
		return ptr != 0;
	}

	template< class T >
	const T & Virtual::as() const throw ( bad_cast )
	{
		const T & ref = dynamic_cast< const T & > ( *this );
		return ref;
	}


	template< class T >
	T & Virtual::as() throw ( bad_cast )
	{
		T & ref = dynamic_cast< T & > ( *this );
		return ref;
	}

	template< class T >
	T * Virtual::asPtr() throw ( bad_cast )
	{
		T * ref = dynamic_cast< T * > ( this );
		if ( ref != 0 )
			return ref;
		else
			throw bad_cast();
	}
}

#endif
