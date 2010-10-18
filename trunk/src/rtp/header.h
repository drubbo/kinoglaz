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
 * File name: ./rtp/header.h
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
 *     sdp threaded loader. sync pause
 *
 **/


#ifndef __KGD_RTP_HEADER_H
#define __KGD_RTP_HEADER_H

#include "sdp/frame.h"

#ifdef HAVE_CONFIG_H
#include "config.h"
#else
#error "Please use platform configuration file."
#endif

namespace KGD
{
	//! RTP classes
	namespace RTP
	{
		//! RTP header
		struct Header
		{
#ifdef WORDS_BIGENDIAN
			uint8_t version:2;
			uint8_t padding:1;
			uint8_t extension:1;
			uint8_t csrcLen:4;
#else
			uint8_t csrcLen:4;
			uint8_t extension:1;
			uint8_t padding:1;
			uint8_t version:2;
#endif
#ifdef WORDS_BIGENDIAN
			uint8_t marker:1;
			uint8_t pt:7;
#else
			uint8_t pt:7;
			uint8_t marker:1;
#endif
			uint16_t seqNo;
			uint32_t timestamp;
			uint32_t ssrc;

			//! default ctor
			Header( );
			//! load data from a frame descriptor
			void setupFrom( const SDP::Frame::Base & );

			static const size_t SIZE;
		};

	}
}
#endif
