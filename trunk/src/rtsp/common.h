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
 * File name: ./rtsp/common.h
 * First submitted: 2010-01-16
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


#ifndef __KGD_RTSP_COMMON_H
#define __KGD_RTSP_COMMON_H

#include "lib/common.h"
#include <string>

using namespace std;

namespace KGD
{
	namespace RTSP
	{
		//! RTSP session id
		typedef uint32_t TSessionID;
		//! RTSP / RTP type of sequence numbers
		typedef uint16_t TCseq;

		//! RTSP request to play a certain range at some speed
		struct PlayRequest
		{
			//! normal speed
			static const double LINEAR_SCALE = 1.0;
			//! Range header was present
			bool hasRange;
			//! Scale header was present
			bool hasScale;
			//! time of request (absolute)
			double time;
			//! presentation time to play from
			double from;
			//! presentation time to stop at
			double to;
			//! play speed
			double speed;

			//! construct with default and empty values
			PlayRequest();
			//! merges current request with another one: keeps the strictest range and the latest speed, if any
			PlayRequest& merge( const PlayRequest& );
			//! get string representation
			string toString() const throw();
		};

		//! known user agents
		namespace UserAgent
		{
			enum type
			{
				//! generic
				Generic,
				//! VLC GoldenEye
				VLC_1_0_2,
				VLC_1_0_6
			};

			extern string name[];
		}

		//! end of RTSP message lines
		const string EOL = "\r\n";
		//! supported RTSP version
		const string VER = "RTSP/1.0";
	}
}

#endif
