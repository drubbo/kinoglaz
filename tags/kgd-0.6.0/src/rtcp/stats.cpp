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
 * File name: ./rtcp/stats.cpp
 * First submitted: 2010-01-17
 * First submitter: Emiliano Leporati <emiliano.leporati@gmail.com>
 * Contributor(s) so far - 2010-07-30 :
 *     Emiliano Leporati <emiliano.leporati@gmail.com>
 *
 * Last changes :
 *     Kinoglaz birth: pervasive bug fixing, improvements and client support
 *     sdp debugged
 *     interleave ok
 *     added licence disclaimer
 *     termination, log, method execution refactor
 *
 **/


#include "rtcp/stats.h"
#include "lib/log.h"

namespace KGD
{
	namespace RTCP
	{
		Stat::Stat()
		: RRcount(0)
		, SRcount(0)
		, destSsrc(0)
		, pktCount(0)
		, octetCount(0)
		, pktLost(0)
		, fractLost(0)
		, highestSeqNo(0)
		, jitter(0)
		, lastSR(0)
		, delaySinceLastSR(0)
		{
		}

		void Stat::log( const string & lbl ) const
		{
			Log::message("RTCP %s Stats: %d RR, %d SR, %u packets, %u packets lost, %u fraction lost, %u jitter"
				, lbl.c_str()
				, RRcount
				, SRcount
				, pktCount
				, pktLost
				, fractLost
				, jitter);
		}

	}
}
