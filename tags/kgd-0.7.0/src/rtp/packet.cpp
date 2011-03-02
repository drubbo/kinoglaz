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
 * File name: src/rtp/packet.cpp
 * First submitted: 2010-07-30
 * First submitter: Emiliano Leporati <emiliano.leporati@gmail.com>
 * Contributor(s) so far - 2010-11-04 :
 *     Emiliano Leporati <emiliano.leporati@gmail.com>
 *
 * Last changes :
 *     "would block" cleanup
 *     testing interrupted connections
 *     RTCP poll times in ini file; adaptive RTCP receiver poll interval; uniform EAGAIN handling, also thrown by Interleave
 *     source import
 *
 **/


#include "rtp/packet.h"

namespace KGD
{
	namespace RTP
	{
		size_t Packet::MTU = 1440;

		Packet::Packet( size_t sz )
		: data( sz )
		, isLastOfSequence( false )
		{
		}

		Packet::Packet( void const * bytes, size_t sz )
		: data( bytes, sz )
		, isLastOfSequence( false )
		{
		}
	}

	Channel::Out & operator<<( Channel::Out & s, const RTP::Packet & pkt ) throw( KGD::Socket::Exception )
	{
		ssize_t wrote;
		size_t tot = 0, rem = pkt.data.size();
		while( rem > 0 )
		{
			if ( pkt.isLastOfSequence )
				wrote = s.writeLast( &(pkt.data[tot]), rem );
			else
				wrote = s.writeSome( &(pkt.data[tot]), rem );
			tot += wrote;
			rem -= wrote;
		}

		return s;
	}

}	
 
