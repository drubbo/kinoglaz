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
 * File name: ./lib/log.cpp
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
 *     interleave ok
 *
 **/


#include "lib/log.h"
#include "lib/common.h"

#include <config.h>

extern "C"
{
#include <syslog.h>
#include <stdarg.h>
#include <rtsp/common.h>
}


#define VB_ERROR   0
#define VB_WARNING 1
#define VB_NOTICE  2
#define VB_DEBUG   3
#define VB_VERBOSE 4

namespace KGD
{
	void Log::verbose( const char* fmt, ... ) throw()
	{
#if ( LOG_VERBOSITY > VB_VERBOSE )
		va_list args;
		va_start( args, fmt );
		::vsyslog( LOG_DAEMON | LOG_DEBUG, fmt, args );
		va_end( args );
#endif
	}

	void Log::debug( const char* fmt, ... ) throw()
	{
#if ( LOG_VERBOSITY > VB_DEBUG )
		va_list args;
		va_start( args, fmt );
		::vsyslog( LOG_DAEMON | LOG_DEBUG, fmt, args );
		va_end( args );
#endif
	}

	void Log::message( const char* fmt, ... ) throw()
	{
#if ( LOG_VERBOSITY > VB_NOTICE )
		va_list args;
		va_start( args, fmt );
		::vsyslog( LOG_DAEMON | LOG_INFO, fmt, args );
		va_end( args );
#endif
	}

	void Log::reply( string s ) throw()
	{
#if ( LOG_VERBOSITY > VB_NOTICE )
		vector< string > v = split( RTSP::EOL, s );
		for( size_t i = 0; i < v.size(); ++i )
			::syslog( LOG_DAEMON | LOG_INFO, "RTSP Reply: %s", v[i].c_str() );
#endif		
	}

	void Log::request( string s ) throw()
	{
#if ( LOG_VERBOSITY > VB_NOTICE )
		vector< string > v = split( RTSP::EOL, s );
		for( size_t i = 0; i < v.size(); ++i )
			::syslog( LOG_DAEMON | LOG_INFO, "RTSP Request: %s", v[i].c_str() );
#endif		
	}

	void Log::error( const char* fmt, ... ) throw()
	{
#if ( LOG_VERBOSITY > VB_ERROR )
		va_list args;
		va_start( args, fmt );
		::vsyslog( LOG_DAEMON | LOG_ERR, fmt, args );
		va_end( args );
#endif
	}

	void Log::warning( const char* fmt, ... ) throw()
	{
#if ( LOG_VERBOSITY > VB_WARNING )
		va_list args;
		va_start( args, fmt );
		::vsyslog( LOG_DAEMON | LOG_WARNING, fmt, args );
		va_end( args );
#endif
	}

	void Log::error( const Exception::Generic &e ) throw()
	{
#if ( LOG_VERBOSITY > VB_ERROR )
		::syslog( LOG_DAEMON | LOG_ERR, "%s", e.what() );
#endif		
	}

	void Log::warning( const Exception::Generic &e ) throw()
	{
#if ( LOG_VERBOSITY > VB_WARNING )
		::syslog( LOG_DAEMON | LOG_WARNING, "%s", e.what() );
#endif
	}

}
