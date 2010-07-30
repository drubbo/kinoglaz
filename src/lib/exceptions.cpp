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
 * File name: ./lib/exceptions.cpp
 * First submitted: 2010-01-16
 * First submitter: Emiliano Leporati <emiliano.leporati@gmail.com>
 * Contributor(s) so far - 2010-07-30 :
 *     Emiliano Leporati <emiliano.leporati@gmail.com>
 *
 * Last changes :
 *     Kinoglaz birth: pervasive bug fixing, improvements and client support
 *     log messages refactor; shared descriptor are optional now; spot insertion support
 *     safe strerror
 *     sdp debugged
 *     sdp threaded loader. sync pause
 *
 **/


#include "lib/exceptions.h"
#include "lib/common.h"
#include <cstring>

namespace KGD
{
	namespace Exception
	{
		Generic::Generic( const string & msg )
		: runtime_error( msg )
		, _errno( -1 )
		{
		}

		Generic::Generic( int errcode )
		: runtime_error( cStrError( errcode ) )
		, _errno( errcode )
		{
		}

		Generic::Generic( const string & msg, int errcode )
		: runtime_error( msg + ": " + cStrError( errcode ) )
		, _errno( errcode )
		{
		}

		int Generic::getErrcode() const
		{
			return _errno;
		}

		NotFound::NotFound( const string & what )
		: Generic( "Not found: " + what )
		{
		}

		NullPointer::NullPointer( )
		: Generic( "Dereferencing NULL pointer" )
		{
		}

		NullPointer::NullPointer( const string & what )
			: Generic( "Dereferencing NULL pointer: " + what )
		{
		}

		InvalidState::InvalidState( const string & msg )
		: Generic( "Invalid state: " + msg )
		{
		}

		InvalidType::InvalidType( const string & msg )
		: Generic( "Invalid type: " + msg )
		{
		}

		OutOfBounds::OutOfBounds( double  i, double  min, double  max )
		: NotFound( "Index " + toString(i) + " outside [" + toString(min) + "," + toString(max) + "]" )
		{
		}

	}

}
