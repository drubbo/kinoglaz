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
 * File name: ./lib/exceptions.h
 * First submitted: 2010-01-16
 * First submitter: Emiliano Leporati <emiliano.leporati@gmail.com>
 * Contributor(s) so far - 2010-07-30 :
 *     Emiliano Leporati <emiliano.leporati@gmail.com>
 *
 * Last changes :
 *     Kinoglaz birth: pervasive bug fixing, improvements and client support
 *     fixed some bug in Log and in insertMedia; comments
 *     log messages refactor; shared descriptor are optional now; spot insertion support
 *     sdp debugged
 *     sdp threaded loader. sync pause
 *
 **/


#ifndef __KGD_EXCEPTIONS_H
#define __KGD_EXCEPTIONS_H

#include <stdexcept>
#include <cerrno>

using namespace std;

namespace KGD
{
	//! Basic exceptions
	namespace Exception
	{
		//! Generic exception
		class Generic :
			public runtime_error
		{
			protected:
				int _errno;
			public:
				//! construct from errno code
				Generic( int errcode );
				//! construct from string message
				Generic( const string & );
				//! construct mixing string message and error from errno code
				Generic( const string &, int errcode );
				//! get errno code
				int getErrcode() const;
		};

		//! Something was missing
		class NotFound :
			public Generic
		{
			public:
				NotFound( const string & );
		};

		//! Invalid object state for request op
		class InvalidState :
			public Generic
		{
			public:
				InvalidState( const string & );
		};

		//! Invalid runtime type
		class InvalidType :
			public Generic
		{
			public:
				InvalidType( const string & );
		};

		//! Data out of valid bounds
		class OutOfBounds :
			public NotFound
		{
			public:
				OutOfBounds( double i, double min, double max );
		};

		//! Dereferencing NULL smart pointer
		class NullPointer :
			public Generic
		{
			public:
				NullPointer( );
				NullPointer( const string & );
		};
	}

}

#endif

