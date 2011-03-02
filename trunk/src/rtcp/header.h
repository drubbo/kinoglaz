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
 * File name: src/rtcp/header.h
 * First submitted: 2010-07-30
 * First submitter: Emiliano Leporati <emiliano.leporati@gmail.com>
 * Contributor(s) so far - 2010-11-04 :
 *     Emiliano Leporati <emiliano.leporati@gmail.com>
 *
 * Last changes :
 *     boosted
 *     added parameter to control aggregate / per track control; fixed parse and send routine of SDES RTCP packets
 *     Some cosmetics about enums and RTCP library
 *     source import
 *
 **/


#ifndef __KGD_RTCP_HEADER_H
#define __KGD_RTCP_HEADER_H

#include "lib/utils/safe.hpp"

#include "lib/common.h"
#include "lib/buffer.h"
#include "lib/socket.h"

#include "lib/common.h"

#ifdef HAVE_CONFIG_H
#include "config.h"
#else
#error "Please use platform configuration file."
#endif

namespace KGD
{
	//! RTCP classes
	namespace RTCP
	{
		//! RTCP packet type codes
		namespace PacketType
		{
			enum code
			{
				SenderReport = 200,
				ReceiverReport = 201,
				SourceDescription = 202,
				Bye = 203,
				Application = 204
			};
		}

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
			Header( PacketType::code, uint16_t );
			//! reset len
			void setLength( uint16_t );
			//! add len
			void incLength( uint16_t );
		};

		//! Sender Report declarations
		namespace SenderReport
		{
			//! Sender Report header
			struct Header
			{
				TSSrc ssrc;
				uint32_t NTPtimestampH;
				uint32_t NTPtimestampL;
				uint32_t RTPtimestamp;
				uint32_t pktCount;
				uint32_t octetCount;
			};
		}

		//! Receiver Report declarations
		namespace ReceiverReport
		{
			//! Receiver Report header
			struct Header
			{
				TSSrc ssrc;
			};
			//! Receiver Report payload
			struct Payload
			{
				TSSrc ssrc;
				uint8_t fractLost;
				uint32_t pktLost:16;
				uint32_t highestSeqNo;
				uint32_t jitter;
				uint32_t lastSR;
				uint32_t delaySinceLastSR;
			};
		}

		//! Source Description declarations
		namespace SourceDescription
		{
			//! Source Description header
			struct Header
			{
				TSSrc ssrc;
			} __attribute__((__packed__));

			//! Source Description item
			struct Payload
			{
				//! valid Source Description items
				struct Attribute
				{
					enum type
					{
						END   = 0,
						CNAME = 1,
						NAME  = 2,
						EMAIL = 3,
						PHONE = 4,
						LOC   = 5,
						TOOL  = 6,
						NOTE  = 7,
						PRIV  = 8
					};
				};
				uint8_t attributeName;
				uint8_t length;
			} __attribute__((__packed__));

		}

		//! BYE message declarations
		namespace Bye
		{
			//! Bye header
			struct Header
			{
				TSSrc ssrc;
				uint8_t length;

			} __attribute__((__packed__));
		}
	}
}

#endif

