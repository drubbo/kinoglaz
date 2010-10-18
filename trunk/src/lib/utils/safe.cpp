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
 * File name: ./lib/utils/safe.cpp
 * First submitted: 2010-01-16
 * First submitter: Emiliano Leporati <emiliano.leporati@gmail.com>
 * Contributor(s) so far - 2010-07-30 :
 *     Emiliano Leporati <emiliano.leporati@gmail.com>
 *
 * Last changes :
 *     Kinoglaz birth: pervasive bug fixing, improvements and client support
 *     sdp debugged
 *     sdp threaded loader. sync pause
 *     interleave ok
 *     added licence disclaimer
 *
 **/


#include "lib/utils/safe.hpp"

namespace KGD
{
	namespace Safe
	{
/*
		Flag::Flag( const Flag& f )
		: _val( f.getValue() )
		{
		}

		Flag::Flag( bool f )
		: _val( f )
		{
		}

		Flag& Flag::operator=( bool b )
		{
			Lock lk( _mux );
			_val = b;
			return *this;
		}

		bool Flag::getValue() const
		{
			Lock lk( _mux );
			return _val;
		}
		Flag::operator bool() const
		{
			Lock lk( _mux );
			return _val;
		}
*/
	}
}