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
 * File name: ./rtcp/stats.h
 * First submitted: 2010-01-17
 * First submitter: Emiliano Leporati <emiliano.leporati@gmail.com>
 * Contributor(s) so far - 2010-07-30 :
 *     Emiliano Leporati <emiliano.leporati@gmail.com>
 *
 * Last changes :
 *     Kinoglaz birth: pervasive bug fixing, improvements and client support
 *     log messages refactor; shared descriptor are optional now; spot insertion support
 *     sdp debugged
 *     interleave ok
 *     added licence disclaimer
 *
 **/


#ifndef __KGD_RTCP_STATS_H
#define __KGD_RTCP_STATS_H

#include "lib/common.h"

namespace KGD
{
	namespace RTCP
	{
		//! RTCP stats
		struct Stat
		{
			uint RRcount;
			uint SRcount;
			TSSrc destSsrc;
			uint pktCount;
			uint octetCount;
			uint pktLost;
			uint8_t fractLost;
			uint highestSeqNo;
			uint jitter;
			uint lastSR;
			uint delaySinceLastSR;

			Stat();
			void log( const string & lbl ) const;
		};
	}
}

#endif

