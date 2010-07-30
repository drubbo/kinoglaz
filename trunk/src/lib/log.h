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
 * File name: ./lib/log.h
 * First submitted: 2010-01-16
 * First submitter: Emiliano Leporati <emiliano.leporati@gmail.com>
 * Contributor(s) so far - 2010-07-30 :
 *     Emiliano Leporati <emiliano.leporati@gmail.com>
 *
 * Last changes :
 *     Kinoglaz birth: pervasive bug fixing, improvements and client support
 *     fixed some bug in Log and in insertMedia; comments
 *     sdp debugged
 *     interleave ok
 *     added licence disclaimer
 *
 **/


#ifndef __KGD_LOG_H
#define __KGD_LOG_H

#include "lib/common.h"
#include "lib/exceptions.h"
#include <string>

using namespace std;

namespace KGD
{
	//! static log facility class
	class Log
	{
	public:
		static void error( const char *fmt, ... ) throw();
		static void error( Exception::Generic const &e ) throw();
		static void warning( const char *fmt, ... ) throw();
		static void warning( Exception::Generic const &e ) throw();
		static void message( const char *fmt, ... ) throw();
		//! log RTSP sending reply
		static void reply( string s ) throw();
		//! log RTSP received request
		static void request( string s ) throw();
		static void debug( const char* fmt, ... ) throw();
		static void verbose( const char* fmt, ... ) throw();
	};
}

#endif
