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
 * File name: src/rtcp/sender.cpp
 * First submitted: 2010-07-30
 * First submitter: Emiliano Leporati <emiliano.leporati@gmail.com>
 * Contributor(s) so far - 2010-11-04 :
 *     Emiliano Leporati <emiliano.leporati@gmail.com>
 *
 * Last changes :
 *     "would block" cleanup
 *     testing against a "speed crash"
 *     testing interrupted connections
 *     Lockables in timers and medium; refactorized iterator release
 *     RTCP poll times in ini file; adaptive RTCP receiver poll interval; uniform EAGAIN handling, also thrown by Interleave
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
		ssize_t wrote = s.writeSome( b.getDataBegin(), b.getDataLength() );
		b.dequeue( wrote );

		return s;
	}

	namespace RTCP
	{

		long Sender::SR_INTERVAL = 5;

		Sender::RTPSync::RTPSync()
		: _doSync( false )
		, _sync( 2 )
		{
		};

		Sender::RTPSync& Sender::RTPSync::operator=( bool b )
		{
			_doSync = b;
			return *this;
		}

		Sender::RTPSync::operator bool() const
		{
			return _doSync;
		}

		void Sender::RTPSync::wait()
		{
			_sync.wait();
		}


		
		
		Sender::Sender( RTP::Session & s, const boost::shared_ptr< Channel::Bi > & sock )
		: Thread( s, sock, "Sender" )
		{
		}

		Sender::~Sender()
		{
			Log::verbose( "%s: destroying", getLogName() );
		}

		void Sender::registerPacketSent( size_t sz ) throw()
		{
			SafeStats::Lock lk( _stats );
			(*_stats).pktCount ++;
			(*_stats).octetCount += sz;
		}
		void Sender::registerPacketLost( size_t sz ) throw()
		{
			SafeStats::Lock lk( _stats );
			(*_stats).pktLost ++;
		}

		void Sender::wait()
		{
			_syncRTP.wait();
		}

		void Sender::_start()
		{
			_syncRTP = true;
		}

		void Sender::releaseRTP( OwnThread::Lock & lk )
		{
			if ( _syncRTP )
			{
				Log::verbose( "%s: release barrier", getLogName() );
				{
					OwnThread::UnLock ulk( lk );
					bool done = false;
					do
						NO_INTERRUPT( _syncRTP.wait(); done = true; )
					while( ! done );
				}
				_syncRTP = false;
			}
		}

		void Sender::run()
		{
			{
				OwnThread::Lock lk( _th );

				Log::verbose( "%s: loop started", getLogName() );

				try
				{
					while( _flags.bag[ Status::RUNNING ] )
					{
						this->doPause( lk );

						if ( _flags.bag[ Status::RUNNING ] )
						{
							// send SR and SDES
							try
							{
								*_sock << enqueueReport().enqueueDescription();
								// give RTP way if needed
								this->releaseRTP( lk );
							}
							catch( Socket::Exception const & e )
							{
								if ( e.wouldBlock() )
									Log::warning( "%s: packet lost: %s", getLogName(), e.what() );
								else
									throw;
							}

							// sleep
							{
								OwnThread::Interruptible intr( _th );
								try
								{
									Log::verbose("%s sleeping", getLogName());
									_th.sleepSec( lk, SR_INTERVAL );
								}
								catch( boost::thread_interrupted )
								{
									Log::verbose("%s was awakened", getLogName());
								}
							}
						}
					}

					// send BYE
					*_sock << enqueueReport().enqueueBye();
					Log::debug( "%s: sent BYE", getLogName() );
				}
				catch( const KGD::Socket::Exception & e )
				{
					Log::error( "%s: closing socket: %s", getLogName(), e.what() );
					_sock->close();
				}

				this->releaseRTP( lk );

				_flags.bag[ Status::RUNNING ] = false;
				_flags.bag[ Status::PAUSED ] = false;

				Log::debug( "%s: loop terminated", getLogName() );

				RTCP::Thread::getStats().log( "Sender" );
			}

			_th.wait();
		}

		void Sender::reset()
		{
			SafeStats::Lock lk( _stats );
			(*_stats).RRcount = 0;
			(*_stats).SRcount = 0;
		}

		void Sender::restart()
		{
			Log::debug( "%s: restarting", getLogName() );

			_th.lock();
			_syncRTP = true;

			if ( _flags.bag[ Status::RUNNING ] )
			{
				_th.unlock();
				this->unpause();
			}
			else
			{
				_th.unlock();
				this->start();
			}				
		}

		Sender& Sender::enqueueReport()
		{
			//TODO keep to calc fract lost
// 			static Stats lastStats;

			SenderReport::Header h;

			// times
			{
				double curTime = Clock::getSec();
				const RTP::Timeline::Medium & tm = _rtp.getTimeline();
				double presTime = tm.getPresentationTime( curTime );
				RTP::TTimestamp timeRTP = tm.getRTPtime( presTime, curTime );

				timespec timeNTP;
				timeNTP.tv_sec  = long(trunc(curTime));
				timeNTP.tv_nsec = Clock::secToNano(curTime - trunc(curTime));
				h.NTPtimestampH = htonl( uint32_t(timeNTP.tv_sec) + 2208988800u );
				h.NTPtimestampL = htonl( Clock::nanoToSec(uint64_t(timeNTP.tv_nsec) << 32) );
				h.RTPtimestamp  = htonl( timeRTP );

				Log::debug( "%s: building SR: time %lf presentation time %lf %lu", getLogName(), curTime, presTime, timeRTP );
			}

			{
				SafeStats::Lock lk( _stats );
				++ (*_stats).SRcount;
				h.pktCount   = htonl( (*_stats).pktCount );
				h.octetCount = htonl( (*_stats).octetCount );
			}
			h.ssrc = htonl( _rtp.getSsrc() );

			Header hh( PacketType::SenderReport, sizeof(h) );
			_buffer.enqueue( &hh, sizeof(hh) );
			_buffer.enqueue( &h, sizeof(h) );


			//TODO payload for "every" source
			
			return *this;
		}

		Sender& Sender::enqueueDescription()
		{
			SourceDescription::Header h;
			SourceDescription::Payload pName, pTool;

			h.ssrc = htonl( _rtp.getSsrc() );

			pName.attributeName = SourceDescription::Payload::Attribute::CNAME;
			pName.length = _rtp.getUrl().host.size();

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
			_buffer.enqueue( _rtp.getUrl().host );
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

			h.ssrc = htonl( _rtp.getSsrc() );
			h.length = htonl( reason.size() );

			Header hh( PacketType::Bye, sizeof(h) );
			hh.count = 1;

			_buffer.enqueue( &hh, sizeof(hh) );
			_buffer.enqueue( &h, sizeof(h) );
			_buffer.enqueue(reason);

			return *this;
		}

	}
}
