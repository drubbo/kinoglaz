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
 * File name: src/lib/common.cpp
 * First submitted: 2010-07-30
 * First submitter: Emiliano Leporati <emiliano.leporati@gmail.com>
 * Contributor(s) so far - 2010-11-04 :
 *     Emiliano Leporati <emiliano.leporati@gmail.com>
 *
 * Last changes :
 *     source import
 *
 **/


#include "lib/common.h"
#include <cstring>
#include <cerrno>

extern "C"
{
#include <uuid/uuid.h>
}


extern "C"
{
#include <sys/stat.h>
#include <sys/types.h>
}

namespace KGD
{
	BeginsWith::BeginsWith( const string & bg)
	: _bg( bg )
	{
	}
	bool BeginsWith::operator()( const string & s ) const throw()
	{
		return (s.substr( 0, _bg.size() ) == _bg);
	}

	double signedMax( double a, double b, int sign )
	{
		if ( a < HUGE_VAL && b < HUGE_VAL )
			return (sign > 0 ? std::max(a, b) : std::min(a, b));
		else
			return std::min( a, b );
	}
	double signedMin( double a, double b, int sign )
	{
		if ( a < HUGE_VAL && b < HUGE_VAL )
			return (sign < 0 ? std::max(a, b) : std::min(a, b));
		else
			return std::min( a, b );
	}
	
	bool fileExists ( const char* fpath ) throw()
	{
		struct stat buf;
		return ( stat ( fpath, &buf ) == 0 );
	}
	
	int sign(double v) throw()
	{
		if (v == 0.0)
			return 0;
		else if (v > 0.0)
			return 1;
		else
			return -1;
	}

	string cStrError( int e )
	{
		char buf[4096];
#if (_POSIX_C_SOURCE >= 200112L || _XOPEN_SOURCE >= 600) && ! _GNU_SOURCE
		if ( strerror_r( e, buf, sizeof( buf ) ) < 0 )
			return Std::toString( e );
		else
			return buf;
#else
		return strerror_r( e, buf, sizeof( buf ) );
#endif
	}

	string newUUID()
	{
		uuid_t uuid;
		uuid_generate_random ( uuid );
		char s[37];
		uuid_unparse ( uuid, s );
		return s;
	}

	vector< string > split ( const string & sep, const string &s ) throw()
	{
		vector<string> result;
		size_t found = 0;
		size_t start = 0;

		found = s.find_first_of ( sep, start );
		while ( found != string::npos )
		{
			result.push_back ( s.substr ( start, found - start ) );
			start = found + sep.size();
			found = s.find_first_of ( sep, start );
		}
		result.push_back ( s.substr ( start ) );

		return result;
	}

	pair< string, string > split2 ( const string & sep, const string &s ) throw()
	{
		size_t found = 0;

		found = s.find_first_of ( sep );
		if ( found == string::npos )
			return make_pair ( s, "" );
		else
			return make_pair ( s.substr ( 0, found ), s.substr ( found + sep.size() ) );
	}

	string trim( const string & s, char c )
	{
		size_t bg = 0, ed = s.size() - 1;
		while( s[bg] == c )
			++ bg;
		while( s[ed] == c )
			-- ed;
		return s.substr( bg, ed - bg + 1 );
	}
}
