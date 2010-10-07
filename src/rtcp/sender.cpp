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
 * File name: ./rtcp/sender.cpp
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


#include "rtcp/sender.h"
#include "rtcp/header.h"
#include "rtp/chrono.h"
#include "lib/clock.h"
#include "rtp/session.h"
#include "lib/log.h"
#include <iostream>
#include "daemon.h"

namespace KGD
{
	Channel::Out& operator<< ( Channel::Out & s, RTCP::Sender & rtcp ) throw( KGD::Socket::Exception )
	{
		Buffer & b = rtcp._buffer;
		for(;;)
		{
			try
			{
// 				Log::message("RTCP %d sending data", s.getDescription().ports.first );
				ssize_t wrote = s.writeSome( b.getDataBegin(), b.getDataLength() );
				b.dequeue( wrote );
				break;
			}
			catch( Socket::Exception const & e )
			{
				if (e.getErrcode() != EAGAIN )
					throw;
			}
		}

		return s;
	}

	namespace RTCP
	{

		long Sender::SR_INTERVAL = 5;

		Sender::Sender( RTP::Session & s, const Ptr::Shared< Channel::Bi > & sock )
		: _rtp(s)
		, _sock(sock)
		, _sync( 2 )
		, _running(false)
		, _paused(false)
		, _RTPsync(false)
		, _logName( s.getLogName() + string(" RTCP Sender") )
		{
		}

		Sender::~Sender()
		{
			this->stop();

			if ( _th )
			{
				Log::message( "%s: waiting thread", getLogName() );
				_th->join();
				_th.destroy();
			}

			Log::message( "%s: destroyed", getLogName() );
		}

		const char * Sender::getLogName() const throw()
		{
			return _logName.c_str();
		}

		void Sender::registerPacketSent( size_t sz ) throw()
		{
			RLock lk( _muxStats );
			_stats.pktCount ++;
			_stats.octetCount += sz;
		}

		void Sender::wait()
		{
// 			Log::debug("BARRIER sync");
			_sync.wait();
		}

		void Sender::releaseRTP()
		{
			if ( _RTPsync )
			{
				Log::debug( "%s: release barrier", getLogName() );
				this->wait();
				_RTPsync = false;
			}
		}

		void Sender::sendLoop()
		{
			Log::debug( "%s: started", getLogName() );

			// lock the clock
			_lkClock = new TLock( _muxClock );
			Log::debug( "%s: got lock", getLogName() );

			try
			{
				while( _running )
				{
// 					Log::debug("RTCP Sender %d about to ...", _sock->getDescription().ports.first );
					// send SR and SDES
					*_sock << enqueueReport().enqueueDescription();
					// give RTP way if needed
					this->releaseRTP();


					while ( _paused )
					{
						if ( ! _lkClock )
							_lkClock = new TLock( _muxClock );
						_condUnPause.wait( *_lkClock );
					}
					if ( _running )
					{
						// try a lock for at most "sending interval"
						Ptr::Scoped< TLock > lkT = new TLock( _muxClock, boost::posix_time::seconds( SR_INTERVAL ) );
						// if i own the lock some else should have given us free way destroying clock lock
						if ( lkT->owns_lock() )
						{
							Lock lk( _muxLk );
							_lkClock = lkT.release();
// 							Log::debug("RTCP lock owned");
						}
// 						else
// 							Log::debug("RTCP lock timeout");
					}
				}

				*_sock << enqueueReport().enqueueBye();
				Log::debug( "%s: sent BYE", getLogName() );
			}
			catch( const KGD::Socket::Exception & e )
			{
				Log::error( "%s: socket error: %s", getLogName(), e.what() );
			}

			this->releaseRTP();

			_running = false;
			_paused = false;

			Log::debug( "%s: stopped", getLogName() );

			this->getStats().log( "Sender" );
		}

		void Sender::start()
		{
			_RTPsync = true;
			if ( !_running )
			{
				_stats.RRcount = 0;
				_stats.SRcount = 0;

				_running = true;
				_th = new Thread(boost::bind(&Sender::sendLoop,this));
			}
			else if ( _paused )
				this->unpause();
		}

		void Sender::pause()
		{
			if ( !_paused )
			{
				Lock lk( _muxLk );
				_paused = true;
				_lkClock.destroy();
			}
		}

		void Sender::unpause()
		{
			if ( _paused )
			{
				_paused = false;
				_condUnPause.notify_all();
			}
			else
			{
				Lock lk( _muxLk );
				_lkClock.destroy();
			}

		}

		void Sender::stop()
		{
			if ( _running )
			{
				_running = false;
				this->unpause();
			}
		}

		void Sender::restart()
		{
			Log::debug( "%s: restarting", getLogName() );

			_RTPsync = true;

			if ( _running )
				this->unpause();
			else
				this->start();
		}

		Sender& Sender::enqueueReport()
		{
			SenderReport::Header h;

			// times
			{
				double curTime = Clock::getSec();
				const RTP::Timeline::Medium & tm = _rtp->getTimeline();
				double presTime = tm.getPresentationTime( curTime );
				RTP::TTimestamp timeRTP = tm.getRTPtime( presTime, curTime );

				timespec timeNTP;
				timeNTP.tv_sec  = long(trunc(curTime));
				timeNTP.tv_nsec = Clock::secToNano(curTime - trunc(curTime));
				h.NTPtimestampH = htonl( uint32_t(timeNTP.tv_sec) + 2208988800u );
				h.NTPtimestampL = htonl( Clock::nanoToSec(uint64_t(timeNTP.tv_nsec) << 32) );
				h.RTPtimestamp  = htonl( timeRTP );

				Log::message( "%s: building sr time %lf presentation time %lf %lu", getLogName(), curTime, presTime, timeRTP );
			}

			{
				RLock lk( _muxStats );
				++ _stats.SRcount;
				h.pktCount   = htonl( _stats.pktCount );
				h.octetCount = htonl( _stats.octetCount );
			}
			h.ssrc = htonl( _rtp->getSsrc() );

			Header hh( PacketType::SenderReport, sizeof(h) );
			_buffer.enqueue( &hh, sizeof(hh) );
			_buffer.enqueue( &h, sizeof(h) );

			return *this;
		}

		Sender& Sender::enqueueDescription()
		{
			SourceDescription::Header h;
			SourceDescription::Payload pName, pTool;

			h.ssrc = htonl( _rtp->getSsrc() );

			pName.attributeName = SourceDescription::Payload::Attribute::CNAME;
			pName.length = _rtp->getUrl().host.size();

			pTool.attributeName = SourceDescription::Payload::Attribute::TOOL;
			pTool.length = KGD::Daemon::getName().size();

			size_t pktSize =
				sizeof(h) +
				sizeof(pName) + pName.length +
				sizeof(pTool) + pTool.length;

			Log::verbose( "%s sending %u bytes", getLogName(), pktSize );
			if ( pktSize % 4 != 0 )
				pktSize += 4 - (pktSize % 4);
			Log::verbose( "%s adjusted to %u bytes", getLogName(), pktSize );

			Header hh( PacketType::SourceDescription, pktSize );
			hh.count = 1;

			_buffer.enqueue( &hh, sizeof(hh) );
			_buffer.enqueue( &h, sizeof(h) );
			_buffer.enqueue( &pName, sizeof(pName) );
			_buffer.enqueue( _rtp->getUrl().host );
			_buffer.enqueue( &pTool, sizeof(pTool) );
			_buffer.enqueue( KGD::Daemon::getName() );

			char zero( 0 );
			while( _buffer.getDataLength() < pktSize )
				_buffer.enqueue( &zero, 1 );

			return *this;
		}

		Sender& Sender::enqueueBye()
		{
			Bye::Header h;
			string reason = "Stream terminated";

			h.ssrc = htonl( _rtp->getSsrc() );
			h.length = htonl( reason.size() );

			Header hh( PacketType::Bye, sizeof(h) );
			hh.count = 1;

			_buffer.enqueue( &hh, sizeof(hh) );
			_buffer.enqueue( &h, sizeof(h) );
			_buffer.enqueue(reason);

			return *this;
		}

		Stat Sender::getStats() const throw()
		{
			RLock lk( _muxStats );
			return _stats;
		}

	}
}
