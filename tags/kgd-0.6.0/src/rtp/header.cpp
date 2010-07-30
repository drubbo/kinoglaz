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
 * File name: ./rtp/header.cpp
 * First submitted: 2010-01-16
 * First submitter: Emiliano Leporati <emiliano.leporati@gmail.com>
 * Contributor(s) so far - 2010-07-30 :
 *     Emiliano Leporati <emiliano.leporati@gmail.com>
 *
 * Last changes :
 *     formats on their own; better Factory elements naming; separate library for every component
 *     Kinoglaz birth: pervasive bug fixing, improvements and client support
 *     log messages refactor; shared descriptor are optional now; spot insertion support
 *     sdp debugged
 *     interleave ok
 *
 **/


#include "rtp/header.h"
#include "sdp/sdp.h"



namespace KGD
{
	namespace RTP
	{
		const size_t Header::SIZE = sizeof( Header );

		Header::Header( )
#ifdef WORDS_BIGENDIAN
		: version(2)
		, padding(0)
		, extension(0)
		, csrcLen(0)
		, marker(0)
		, pt( ptp )
#else
		: csrcLen(0)
		, extension(0)
		, padding(0)
		, version(2)
		, pt( 0 )
		, marker(0)
#endif
		, seqNo(0)
		, timestamp(0)
		, ssrc(0)
		{
		}

		void Header::setupFrom( const SDP::Frame::Base & )
		{
		}
	}
}
 