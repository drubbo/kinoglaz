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
 * File name: ./rtcp/receiver.h
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
		: public boost::noncopyable
		{
		private:
			//! communication channel
			boost::shared_ptr< Channel::Bi > _sock;
			//! receive thread
			Thread _th;

			struct Status
			{
				typedef Safe::FlagSet< 2 > type;

				enum flag
				{
					RUNNING,
					PAUSED
				};
			};

			
			//! is thread running ?
			Status::type _status;

			//! un-pause condition
			Condition _condUnPause;
			//! data buffer
			Buffer _buffer;
			typedef Safe::Lockable< Stat > Stats;
			//! receive stats
			Stats _stats;

			//! log identifier
			const string _logName;


			//! main loop
			void recvLoop();
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

			//! pauses main loop
			void pause();
			//! unpauses main loop
			void unpause();
		public:
			//! ctor
			Receiver( RTP::Session &, const boost::shared_ptr< Channel::Bi > & );
			//! dtor
			~Receiver();
			//! log identifier
			const char * getLogName() const throw();
			//! returns receiver stats
			Stat getStats() const throw();
			//! starts receiver
			void start();
			//! stops receiver
			void stop();

			//! maximum time waiting data
			static double POLL_INTERVAL;
		};
	}
}

#endif
