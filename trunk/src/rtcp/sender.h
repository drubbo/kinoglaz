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
 * File name: ./rtcp/sender.h
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


#ifndef __KGD_RTCP_SENDER_H
#define __KGD_RTCP_SENDER_H

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
		class Sender;
	}

	//! delivery sender stats on a channel
	Channel::Out & operator<< ( Channel::Out &, RTCP::Sender & ) throw( KGD::Socket::Exception );

	namespace RTCP
	{
		//! RTCP sender - delivers sender stats
		class Sender
		: public boost::noncopyable
		{
		private:
			//! ref to rtp session
			RTP::Session & _rtp;
			//! socket abstraction
			boost::shared_ptr< Channel::Bi > _sock;

			//! sync barrier with RTP session
			Barrier _syncRTP;
			Barrier _syncLoopEnd;
			//! send thread
			Thread _th;
			//! un-pause condition
			Condition _condUnPause;

			//! status flags declaration
			struct Status
			{
				//! thread safe flag set
				typedef Safe::FlagSet< 3 > type;
				//! status identifiers 
				enum flag
				{
					RUNNING,
					PAUSED,
					SYNC_WITH_RTP
				};
			};
			//! sender status flags
			Status::type _status;

			//! buffer of data to send
			Buffer _buffer;
			//! sending stats
			SafeStats _stats;

			//! log identifier
			const string _logName;

			//! main loop
			void sendLoop();

			//! enque SR packet in local buffer
			Sender& enqueueReport();
			//! enque SDES packet in local buffer
			Sender& enqueueDescription();
			//! enque BYE packet in local buffer
			Sender& enqueueBye();

			//! pause the loop
			void pause();
			//! restart the loop
			void unpause();
			//! release sync barrier with RTP session
			void releaseRTP();
		public:
			//! ctor
			Sender( RTP::Session &, const boost::shared_ptr< Channel::Bi > & );
			//! dtor
			~Sender();

			//! get log identifier
			const char * getLogName() const throw();

			//! start the sender
			void start();
			//! stop the sender
			void stop();
			//! restart the sender, so next delivery will be immediate
			void restart();
			//! calling thread (RTP session) will wait for sync on a 2-input barrier
			void wait();

			//! add a packet to sender stats
			void registerPacketSent( size_t sz ) throw();
			//! return sender stats
			Stat getStats() const throw();

			friend Channel::Out & KGD::operator<<( Channel::Out &, RTCP::Sender & ) throw( KGD::Socket::Exception );

			//! seconds between two deliveries of the sender stats
			static long SR_INTERVAL;
		};
	}

}

#endif
