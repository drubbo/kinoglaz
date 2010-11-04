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
 * File name: src/rtcp/stats.cpp
 * First submitted: 2010-07-30
 * First submitter: Emiliano Leporati <emiliano.leporati@gmail.com>
 * Contributor(s) so far - 2010-11-04 :
 *     Emiliano Leporati <emiliano.leporati@gmail.com>
 *
 * Last changes :
 *     threads terminate with wait + join
 *     testing against a "speed crash"
 *     testing against a "speed crash"
 *     source import
 *
 **/


#include "rtcp/stats.h"
#include "lib/log.h"

namespace KGD
{
	namespace RTCP
	{
		Stats::Stats()
		: RRcount(0)
		, SRcount(0)
		, destSsrc(0)
		, pktCount(0)
		, octetCount(0)
		, pktLost(0)
		, fractLost(0)
		, highestSeqNo(0)
		, jitter(0)
		, lastSR(0)
		, delaySinceLastSR(0)
		{
		}

		void Stats::log( const string & lbl ) const
		{
			Log::message("RTCP %s Stats: %d RR, %d SR, %u packets, %u packets lost, %u fraction lost, %u jitter"
				, lbl.c_str()
				, RRcount
				, SRcount
				, pktCount
				, pktLost
				, fractLost
				, jitter);
		}


		Thread::Thread( RTP::Session & s, const boost::shared_ptr< Channel::Bi > & sock )
		: _rtp( s )
		, _sock( sock )
		{
			_flags.bag[ Status::RUNNING ] = false;
			_flags.bag[ Status::PAUSED ] = false;
		}

		Thread::~Thread()
		{
			this->stop();
		}

		void Thread::start()
		{
			_th.lock();

			this->_start();

			if ( !_flags.bag[ Status::RUNNING ] )
			{
				_flags.bag[ Status::RUNNING ] = true;
				_th.unlock();
				_th.reset( new boost::thread(boost::bind(&Thread::run,this)) );
			}
			else if ( _flags.bag[ Status::PAUSED ] )
			{
				_flags.bag[ Status::PAUSED ] = false;
				_th.unlock();
				_wakeup.notify_all();
			}
		}

		void Thread::pause()
		{
			OwnThread::Lock lk( _th );
			_flags.bag[ Status::PAUSED ] = true;
		}

		void Thread::unpause()
		{
			_th.lock();
			if ( _flags.bag[ Status::PAUSED ] )
			{
				_flags.bag[ Status::PAUSED ] = false;
				_th.unlock();
				_wakeup.notify_all();
			}
			else if ( _flags.bag[ Status::RUNNING ] )
			{
				_th.unlock();
				_th->interrupt();
			}
		}

		void Thread::stop()
		{
			if ( _th )
			{
				_th.lock();
				if ( _flags.bag[ Status::RUNNING ] )
				{
					_flags.bag[ Status::RUNNING ] = false;

					if ( _flags.bag[ Status::PAUSED ] )
					{
						_flags.bag[ Status::PAUSED ] = false;
						_th.unlock();
						_wakeup.notify_all();
					}
					else
					{
						_th.unlock();
						_th->interrupt();
					}
				}
				else
					_th.unlock();

				_th.reset();
			}
		}


		Stats Thread::getStats() const throw()
		{
			SafeStats::Lock lk( _stats );
			return *_stats;
		}

	}
}
