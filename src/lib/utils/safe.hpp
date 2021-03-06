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
 * File name: src/lib/utils/safe.hpp
 * First submitted: 2010-07-30
 * First submitter: Emiliano Leporati <emiliano.leporati@gmail.com>
 * Contributor(s) so far - 2010-11-04 :
 *     Emiliano Leporati <emiliano.leporati@gmail.com>
 *
 * Last changes :
 *     boosted
 *     boosted
 *     source import
 *
 **/

#ifndef _USS_SAFE_VARS_HPP
#define _USS_SAFE_VARS_HPP

#include "lib/utils/safe.h"

namespace KGD
{
	namespace Safe
	{
		// ********************************************************************************

		template< class M >
		Flag< M >::Flag( bool b ) : _bit( b ) {}

		template< class M >
		Flag< M >::operator bool() const
		{
			typename Flag< M >::Lock lk( *this );
			return _bit;
		}

		template< class M >
		Flag< M >& Flag< M >::operator=( bool b )
		{
			typename Flag< M >::Lock lk( *this );
			_bit = b;
			return *this;
		}
	}
}

#endif
