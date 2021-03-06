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
 * File name: src/rtsp/common.cpp
 * First submitted: 2010-07-30
 * First submitter: Emiliano Leporati <emiliano.leporati@gmail.com>
 * Contributor(s) so far - 2010-11-04 :
 *     Emiliano Leporati <emiliano.leporati@gmail.com>
 *
 * Last changes :
 *     boosted
 *     Some cosmetics about enums and RTCP library
 *     source import
 *
 **/


#include "rtsp/common.h"
#include "rtsp/connection.h"
#include "lib/clock.h"
#include <cmath>

namespace KGD
{
	namespace RTSP
	{
		PlayRequest::PlayRequest()
		: hasRange( false )
		, hasScale( false )
		, time( Clock::getSec() )
		, from( HUGE_VAL )
		, to( HUGE_VAL )
		, speed( HUGE_VAL )
		{
		}

		PlayRequest& PlayRequest::merge( const PlayRequest & rq )
		{
			if ( rq.hasScale )
			{
				speed = rq.speed;
				hasScale = true;
			}
			if ( rq.hasRange )
			{
				hasRange = true;
				if ( ! mediaType || *mediaType != SDP::MediaType::Video )
				{
					if ( rq.mediaType && *rq.mediaType == SDP::MediaType::Video )
						from = rq.from;
					else
						from = signedMin( from, rq.from, sign( speed ) );
					mediaType = rq.mediaType;
				}
				
				to = signedMax( to, rq.to, sign( speed ) );
			}

			return *this;
		}

		string PlayRequest::toString() const throw()
		{
			ostringstream rt;
			rt << "[" << from << "-" << to << "] x " << speed << " ( wr: " << hasRange << " | ws: " << hasScale << " )";
			return rt.str();
		}

		namespace UserAgent
		{
			string name[4] = {
				"Generic",
				"VLC 1.0.2",
				"VLC 1.0.6",
				"VLC 1.1.4"
			};
		}
	}
}
