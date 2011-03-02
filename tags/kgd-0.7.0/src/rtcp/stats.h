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
 * File name: src/rtcp/stats.h
 * First submitted: 2010-07-30
 * First submitter: Emiliano Leporati <emiliano.leporati@gmail.com>
 * Contributor(s) so far - 2010-11-04 :
 *     Emiliano Leporati <emiliano.leporati@gmail.com>
 *
 * Last changes :
 *     testing against a "speed crash"
 *     testing against a "speed crash"
 *     introduced keep alive on control socket (me dumb)
 *     boosted
 *     source import
 *
 **/


#ifndef __KGD_RTCP_STATS_H
#define __KGD_RTCP_STATS_H

#include "lib/common.h"
#include "lib/socket.h"
#include "lib/utils/safe.hpp"

#define NO_INTERRUPT( x ) 	{try { x; } catch( boost::thread_interrupted ) { Log::debug( "%s: interrupt !", getLogName() ); }}

namespace KGD
{
	namespace RTP
	{
		class Session;
	}

	namespace RTCP
	{
		//! RTCP stats
		struct Stats
		{
			uint RRcount;
			uint SRcount;
			TSSrc destSsrc;
			uint pktCount;
			uint octetCount;
			uint pktLost;
			uint8_t fractLost;
			uint highestSeqNo;
			uint jitter;
			uint lastSR;
			uint delaySinceLastSR;

			Stats();
			void log( const string & lbl ) const;
		};

		//! stats with own lock
		typedef Safe::Lockable< Stats > SafeStats;

		//! base RTCP thread (sender, receiver)
		class Thread
		: public boost::noncopyable
		{
		protected:
			//! ref to rtp session
			RTP::Session & _rtp;
			//! socket abstraction
			boost::shared_ptr< Channel::Bi > _sock;

			typedef Safe::Thread< Mutex > OwnThread;
			//! thread
			OwnThread _th;

			//! pause condition
			Condition _wakeup;
			
			//! status flags
			struct Status
			{
				enum Flags { RUNNING, PAUSED };
				bitset< 2 > bag;
			} _flags;

			//! stats
			SafeStats _stats;
			//! log identifier
			const string _logName;

			
			//! thread loop to implement
			virtual void run() = 0;
			//! additional actions to perform when starting
			virtual void _start() {};
			//! pause loop
			void doPause( OwnThread::Lock &lk );


			Thread( RTP::Session & s, const boost::shared_ptr< Channel::Bi > & sock, const char * name );

		public:
			virtual ~Thread();

			//! get log identifier
			const char * getLogName() const throw();

			//! start the thread
			void start();
			//! stop the thread
			void stop();
			//! pause the loop
			void pause();
			//! restart the loop
			void unpause();

			//! return sender stats
			Stats getStats() const throw();
		};

		
	}
}

#endif

