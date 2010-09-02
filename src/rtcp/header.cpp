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
 * File name: ./rtcp/header.cpp
 * First submitted: 2010-01-16
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


#include "rtcp/header.h"

namespace KGD
{
	namespace RTCP
	{

		Header::Header( PacketType::code c, uint16_t len )
#if (BYTE_ORDER == LITTLE_ENDIAN)
		: count(0)
		, padding(0)
		, version(2)
#else
		: version(2)
		, padding(0)
		, count(0)
#endif
		, pt(c)
		, length(htons(( (sizeof(Header) + len) / 4 ) - 1))
		{
		}

		void Header::setLength( uint16_t len )
		{
			length = htons((len >> 2) - 1);
		}
		void Header::incLength( uint16_t len )
		{
			setLength( (( length + 1 ) * 4) + len );
		}
	}
}
