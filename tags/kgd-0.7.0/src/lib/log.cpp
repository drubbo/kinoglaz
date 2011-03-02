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
 * File name: src/lib/log.cpp
 * First submitted: 2010-07-30
 * First submitter: Emiliano Leporati <emiliano.leporati@gmail.com>
 * Contributor(s) so far - 2010-11-04 :
 *     Emiliano Leporati <emiliano.leporati@gmail.com>
 *
 * Last changes :
 *     RTCP poll times in ini file; adaptive RTCP receiver poll interval; uniform EAGAIN handling, also thrown by Interleave
 *     added configure option support; fixed signature in log request / reply
 *     source import
 *
 **/


#include "lib/log.h"
#include "lib/common.h"

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif


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
		::vsyslog( LOG_DAEMON | LOG_DEBUG, (string("VRB - ") + fmt).c_str(), args );
		va_end( args );
#endif
	}

	void Log::debug( const char* fmt, ... ) throw()
	{
#if ( LOG_VERBOSITY > VB_DEBUG )
		va_list args;
		va_start( args, fmt );
		::vsyslog( LOG_DAEMON | LOG_DEBUG, (string("DBG - ") + fmt).c_str(), args );
		va_end( args );
#endif
	}

	void Log::message( const char* fmt, ... ) throw()
	{
#if ( LOG_VERBOSITY > VB_NOTICE )
		va_list args;
		va_start( args, fmt );
		::vsyslog( LOG_DAEMON | LOG_INFO, (string("NFO - ") + fmt).c_str(), args );
		va_end( args );
#endif
	}

	void Log::reply( const string & s ) throw()
	{
#if ( LOG_VERBOSITY > VB_NOTICE )
		vector< string > v = split( RTSP::EOL, s );
		for( size_t i = 0; i < v.size(); ++i )
			::syslog( LOG_DAEMON | LOG_INFO, "RTSP Reply: %s", v[i].c_str() );
#endif		
	}

	void Log::request( const string & s ) throw()
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
		::vsyslog( LOG_DAEMON | LOG_ERR, (string("ERR - ") + fmt).c_str(), args );
		va_end( args );
#endif
	}

	void Log::warning( const char* fmt, ... ) throw()
	{
#if ( LOG_VERBOSITY > VB_WARNING )
		va_list args;
		va_start( args, fmt );
		::vsyslog( LOG_DAEMON | LOG_WARNING, (string("WRN - ") + fmt).c_str(), args );
		va_end( args );
#endif
	}

	void Log::error( const Exception::Generic &e ) throw()
	{
#if ( LOG_VERBOSITY > VB_ERROR )
		::syslog( LOG_DAEMON | LOG_ERR, "ERR - %s", e.what() );
#endif		
	}

	void Log::warning( const Exception::Generic &e ) throw()
	{
#if ( LOG_VERBOSITY > VB_WARNING )
		::syslog( LOG_DAEMON | LOG_WARNING, "WRN - %s", e.what() );
#endif
	}

}
