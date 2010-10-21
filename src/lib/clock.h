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
 * File name: ./lib/clock.h
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


#ifndef __KGD_CLOCK_H
#define __KGD_CLOCK_H

#include <string>

extern "C" {
#include <time.h>
#include <inttypes.h>
}

#include <boost/date_time.hpp>

using namespace std;

namespace KGD
{
	//! Time-related functions: obtain, convert, format, sleep
	namespace Clock
	{
		//! factor to get seconds from nano
		const double NSEC_2_SEC = 0.000000001;
		//! factor to get nano from seconds
		const uint64_t SEC_2_NSEC = 1000000000u;

		//! boost absolute time relative to current
		boost::posix_time::ptime boostDeltaSec( double sec ) throw();
		
		//! returns current time in sec and fills given structure
		double getSec( timespec *now ) throw();
		//! returns current time in sec 
		double getSec() throw();
		//! returns current time in nano
		uint64_t getNano() throw();

		//! converts seconds to nano
		uint64_t secToNano( double timesec ) throw();
		//! converts nano to seconds
		double nanoToSec( uint64_t nanotime ) throw();

		//! suspends current thread for given nano
		void sleepNano( uint64_t interval ) throw();

		//! returns given time in a struct suitable to strftime
		tm *getInfo(time_t epoch) throw();
		//! returns current time in a struct suitable to strftime
		tm *getInfo() throw();

		//! returns given time in a string formatted by strftime
		string getFormatted(time_t epoch, const char* fmt) throw();
		//! returns current time in a string formatted by strftime
		string getFormatted(const char* fmt) throw();
	}
}

#endif
