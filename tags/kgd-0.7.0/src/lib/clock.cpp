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
 * File name: src/lib/clock.cpp
 * First submitted: 2010-07-30
 * First submitter: Emiliano Leporati <emiliano.leporati@gmail.com>
 * Contributor(s) so far - 2010-11-04 :
 *     Emiliano Leporati <emiliano.leporati@gmail.com>
 *
 * Last changes :
 *     boosted
 *     source import
 *
 **/


#include "lib/clock.h"
#include <boost/assert.hpp>

#include <cstring>
#include <cstdlib>
#include <boost/thread/thread_time.hpp>

namespace KGD
{
	namespace Clock
	{		
		double getSec( timespec *now ) throw()
		{
			clock_gettime ( CLOCK_REALTIME, now );
			return ( double ) now->tv_sec + ( double ) now->tv_nsec * NSEC_2_SEC;
		}

		double getSec() throw()
		{
			timespec tmp;
			clock_gettime ( CLOCK_REALTIME, &tmp );
			return double(tmp.tv_sec) + double(tmp.tv_nsec) * NSEC_2_SEC;
		}

		uint64_t getNano() throw()
		{
			timespec tmp;
			clock_gettime ( CLOCK_REALTIME, &tmp );
			return uint64_t(tmp.tv_sec) * SEC_2_NSEC + uint64_t(tmp.tv_nsec);
		}

		uint64_t secToNano( double timesec ) throw()
		{
			BOOST_ASSERT( timesec >= 0 );
			return uint64_t( timesec * SEC_2_NSEC );
		}

		double nanoToSec( uint64_t nanotime ) throw()
		{
			return double(nanotime) * NSEC_2_SEC;
		}


		void sleepNano( uint64_t interval ) throw()
		{
			timespec tmp;
			tmp.tv_nsec = interval % SEC_2_NSEC;
			tmp.tv_sec = ( interval - tmp.tv_nsec ) / SEC_2_NSEC;
			nanosleep ( &tmp, NULL );
		}

		tm *getInfo(time_t epoch) throw()
		{
			return localtime(&epoch);
		}

		tm *getInfo() throw()
		{
			return getInfo(::time(NULL));
		}

		string getFormatted(const char* fmt) throw()
		{
			char s[255];
			memset(s,0,255);
			tm *info = getInfo();

			strftime (s, 255, fmt, info);

			return s;
		}

		string getFormatted(time_t epoch, const char* fmt) throw()
		{
			char s[255];
			memset(s,0,255);
			tm *info = getInfo(epoch);

			strftime (s, 255, fmt, info);

			return s;
		}

	}
}
