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
 * File name: src/rtcp/sender.h
 * First submitted: 2010-07-30
 * First submitter: Emiliano Leporati <emiliano.leporati@gmail.com>
 * Contributor(s) so far - 2010-11-04 :
 *     Emiliano Leporati <emiliano.leporati@gmail.com>
 *
 * Last changes :
 *     testing interrupted connections
 *     boosted
 *     boosted
 *     added parameter to control aggregate / per track control; fixed parse and send routine of SDES RTCP packets
 *     source import
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
		: public RTCP::Thread
		{
		private:
			//! sync barrier with RTP session
			class RTPSync
			{
			private:
				bool _doSync;
				Barrier _sync;
			public:
				RTPSync();
				RTPSync & operator=( bool );
				operator bool() const;
				void wait();
			} _syncRTP;

			//! buffer of data to send
			Buffer _buffer;

			//! main loop
			virtual void run();

			//! enque SR packet in local buffer
			Sender& enqueueReport();
			//! enque SDES packet in local buffer
			Sender& enqueueDescription();
			//! enque BYE packet in local buffer
			Sender& enqueueBye();

			//! release sync barrier with RTP session
			void releaseRTP( OwnThread::Lock & );
			//! advice to sync with rtp
			virtual void _start();
		public:
			//! ctor
			Sender( RTP::Session &, const boost::shared_ptr< Channel::Bi > & );
			//! dtor
			~Sender();

			//! reset packet and octect count
			void reset();
			//! restart the sender requesting rtp sync
			void restart();
			//! calling thread (RTP session) will wait for sync on a 2-input barrier
			void wait();

			//! add a sent packet to sender stats
			void registerPacketSent( size_t sz ) throw();
			//! add a lost packet to sender stats
			void registerPacketLost( size_t sz ) throw();

			friend Channel::Out & KGD::operator<<( Channel::Out &, RTCP::Sender & ) throw( KGD::Socket::Exception );

			//! seconds between two deliveries of the sender stats
			static long SR_INTERVAL;
		};
	}

}

#endif
