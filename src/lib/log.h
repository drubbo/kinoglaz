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
 * File name: src/lib/log.h
 * First submitted: 2010-07-30
 * First submitter: Emiliano Leporati <emiliano.leporati@gmail.com>
 * Contributor(s) so far - 2010-11-04 :
 *     Emiliano Leporati <emiliano.leporati@gmail.com>
 *
 * Last changes :
 *     boosted
 *     added configure option support; fixed signature in log request / reply
 *     source import
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
	namespace Log
	{
		void error( const char *fmt, ... ) throw();
		void error( Exception::Generic const &e ) throw();
		void warning( const char *fmt, ... ) throw();
		void warning( Exception::Generic const &e ) throw();
		void message( const char *fmt, ... ) throw();
		//! log RTSP sending reply
		void reply( const string & s ) throw();
		//! log RTSP received request
		void request( const string & s ) throw();
		void debug( const char* fmt, ... ) throw();
		void verbose( const char* fmt, ... ) throw();
	};
}

#endif
