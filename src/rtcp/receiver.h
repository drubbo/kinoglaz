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
 * File name: src/rtcp/receiver.h
 * First submitted: 2010-07-30
 * First submitter: Emiliano Leporati <emiliano.leporati@gmail.com>
 * Contributor(s) so far - 2010-11-04 :
 *     Emiliano Leporati <emiliano.leporati@gmail.com>
 *
 * Last changes :
 *     RTCP poll times in ini file; adaptive RTCP receiver poll interval; uniform EAGAIN handling, also thrown by Interleave
 *     boosted
 *     boosted
 *     removed deadlock issue in RTCP receiver; unloading sent frames from memory when appliable
 *     added parameter to control aggregate / per track control; fixed parse and send routine of SDES RTCP packets
 *
 **/


#ifndef __KGD_RTCP_RECV_H
#define __KGD_RTCP_RECV_H

#include "lib/utils/safe.hpp"

#include "lib/common.h"
#include "lib/buffer.h"
#include "lib/socket.h"

#include "rtcp/header.h"
#include "rtcp/stats.h"

namespace KGD
{
	namespace RTP
	{
		class Session;
	}

	namespace RTCP
	{
		//! RTCP receiver - receives client stats
		class Receiver
		: public RTCP::Thread
		{
		private:
			//! data buffer
			Buffer _buffer;

			//! poll interval / socket adaptive read timeout
			double _poll;

			//! main loop
			virtual void run();

			//! increase read timeout
			void waitMore() throw();
			//! decrease read timeout
			void waitLess() throw();
			//! update stats from received packets
			void updateStats( const ReceiverReport::Payload & );

			//! tells if a SR has been received
			bool handleSenderReport( char const * const data, size_t size );
			//! tells if a RR has been received
			bool handleReceiverReport( char const * const data, size_t size );
			//! tells if a SDES has been received
			bool handleSourceDescription( char const * const data, size_t size );
			//! adds received data to local buffer
			void push( char const * const buffer, ssize_t len );
		public:
			//! ctor
			Receiver( RTP::Session &, const boost::shared_ptr< Channel::Bi > & );
			//! dtor
			~Receiver();

			//! maximum time waiting data
			static double POLL_INTERVAL;
		};
	}
}

#endif
