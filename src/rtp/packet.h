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
 * File name: src/rtp/packet.h
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


#ifndef __KGD_RTP_PACKET_H
#define __KGD_RTP_PACKET_H

#include "lib/socket.h"
#include "lib/array.hpp"

namespace KGD
{
	namespace RTP
	{
		//! RTP packet
		struct Packet
		{
			typedef boost::ptr_list< Packet > List;
			
			//! Maximum Transfer Unit of the network, so maximum size of a packet
			static size_t MTU;
			//! data of this packet
			ByteArray data;
			//! last packet of sequence
			bool isLastOfSequence;
			//! construct allocating a certain size
			Packet( size_t = 0 );
			//! construct with data
			Packet( void const *, size_t );
		};
	}

	//! send rtp packet over an out channel
	Channel::Out & operator<<( Channel::Out &, const RTP::Packet & ) throw( Socket::Exception );
}

#endif
 
