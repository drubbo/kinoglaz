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
 * File name: ./rtcp/header.h
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


#ifndef __KGD_RTCP_HEADER_H
#define __KGD_RTCP_HEADER_H

#include <config.h>

#include "lib/utils/pointer.hpp"
#include "lib/utils/sharedptr.hpp"
#include "lib/utils/safe.hpp"

#include "lib/common.h"
#include "lib/buffer.h"
#include "lib/socket.h"

#include "lib/common.h"

namespace KGD
{
	//! RTCP classes
	namespace RTCP
	{
		//! packet types
		enum PacketType
		{
			SR = 200,
			RR = 201,
			SDES = 202,
			BYE = 203,
			APP = 204
		};

		//! RTCP packet header
		struct Header
		{
#if (BYTE_ORDER == LITTLE_ENDIAN)
			uint8_t count:5;
			uint8_t padding:1;
			uint8_t version:2;
#else
			uint8_t version:2;
			uint8_t padding:1;
			uint8_t count:5;
#endif
			uint8_t pt;
			uint16_t length;

			//! build for a packet type with a len
			Header( PacketType, uint16_t );
			//! reset len
			void setLength( uint16_t );
			//! add len
			void incLength( uint16_t );
		};

		//! Sender Report
		struct HeaderSR
		{
			TSSrc ssrc;
			uint32_t NTPtimestampH;
			uint32_t NTPtimestampL;
			uint32_t RTPtimestamp;
			uint32_t pktCount;
			uint32_t octetCount;
		};

		//! Receiver Report
		struct HeaderRR
		{
			TSSrc ssrc;
		};

		//! a packet following RR header
		struct PacketRR
		{
			TSSrc ssrc;
			uint8_t fractLost;
			uint32_t pktLost:16;
			uint32_t highestSeqNo;
			uint32_t jitter;
			uint32_t lastSR;
			uint32_t delaySinceLastSR;
		};

		//! Sender Description packet
		struct HeaderSDES
		{
			//! fields in SDES
			enum Field
			{
				CNAME = 1,
				NAME = 2,
				EMAIL = 3,
				PHONE = 4,
				LOC = 5,
				TOOL = 6,
				NOTE = 7,
				PRIV = 8
			};

			TSSrc ssrc;
			uint8_t attr_name;
			uint8_t length;

		} __attribute__((__packed__));

		//! BYE packet
		struct HeaderBYE
		{
			TSSrc ssrc;
			uint8_t length;

		} __attribute__((__packed__));
	}
}

#endif

