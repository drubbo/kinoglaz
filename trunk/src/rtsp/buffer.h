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
 * File name: ./rtsp/buffer.h
 * First submitted: 2010-01-16
 * First submitter: Emiliano Leporati <emiliano.leporati@gmail.com>
 * Contributor(s) so far - 2010-07-30 :
 *     Emiliano Leporati <emiliano.leporati@gmail.com>
 *
 * Last changes :
 *     Kinoglaz birth: pervasive bug fixing, improvements and client support
 *     sdp debugged
 *     interleave ok
 *     pre-interleave
 *     added licence disclaimer
 *
 **/


#ifndef __KGD_RTSP_BUFFER_H
#define __KGD_RTSP_BUFFER_H

#include "lib/buffer.h"
#include "rtsp/common.h"
#include "rtsp/exceptions.h"
#include "rtsp/method.h"
#include "rtsp/message.h"
#include "rtp/frame.h"

#include <utility>

using namespace std;

namespace KGD
{
	namespace RTSP
	{

		//! Input RTSP buffer, with methods to inspect request
		class InputBuffer :
			public KGD::Buffer
		{
		protected:
			size_t getContentLength( ) const throw( );
		public:
			InputBuffer();

			//! returns next packet' length, or 0 if incomplete
			size_t getNextPacketLength() const throw( );
			//! returns next request
			Message::Request * getNextRequest( size_t pktLen, const string & remoteHost ) throw( RTSP::Exception::ManagedError, KGD::Exception::NotFound );
			//! returns next response code
			Message::Response * getNextResponse( size_t pktLen ) throw( RTSP::Exception::CSeq, KGD::Exception::NotFound );
			//! returns next interleaved packet
			pair< TPort, RTP::Packet * > getNextInterleave( size_t pktLen ) throw( KGD::Exception::NotFound );
		};
	}
}


#endif
