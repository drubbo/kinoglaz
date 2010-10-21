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
				Log::message( "Socket error during RTCP Sender packet send: %s", e.what() );
// 				if ( s.isWriteBlock() || ( e.getErrcode() != EAGAIN && e.getErrcode() != EWOULDBLOCK ) )
				throw;
			}
		}

		return s;
	}

	namespace RTCP
	{

		long Sender::SR_INTERVAL = 5;

		Sender::Sender( RTP::Session & s, const boost::shared_ptr< Channel::Bi > & sock )
		: _rtp(s)
		, _sock(sock)
		, _syncRTP( 2 )
		, _syncLoopEnd( 2 )
		, _logName( s.getLogName() + string(" RTCP Sender") )
		{
			Status::type::LockerType lk( _status );
			_status[ Status::RUNNING ] = false;
			_status[ Status::PAUSED ] = false;
			_status[ Status::SYNC_WITH_RTP ] = false;
// 			_status[ Status::DESTROY ] = false;
		}

		Sender::~Sender()
		{
			this->stop();
			Log::message( "%s: destroyed", getLogName() );
		}

		const char * Sender::getLogName() const throw()
		{
			return _logName.c_str();
		}

		void Sender::registerPacketSent( size_t sz ) throw()
		{
			SafeStats::LockerType lk( _stats );
			(*_stats).pktCount ++;
			(*_stats).octetCount += sz;
		}

		void Sender::wait()
		{
			_syncRTP.wait();
		}

		void Sender::releaseRTP()
		{
			Status::type::LockerType lk( _status );

			if ( _status[ Status::SYNC_WITH_RTP ] )
			{
				Log::debug( "%s: release barrier", getLogName() );
				{
					Status::type::UnLockerType ulk( lk );
					_syncRTP.wait();
				}
				_status[ Status::SYNC_WITH_RTP ] = false;
			}
		}

		void Sender::sendLoop()
		{
			Log::debug( "%s: loop started", getLogName() );

			{
				Status::type::LockerType lk( _status );

				try
				{
					while( _status[ Status::RUNNING ] )
					{
						while ( _status[ Status::PAUSED ] )
							_condUnPause.wait( lk.getLock() );

						if ( _status[ Status::RUNNING ] )
						{
							// send SR and SDES
							*_sock << enqueueReport().enqueueDescription();
							// give RTP way if needed
							this->releaseRTP();

							// sleep
							{
								Status::type::UnLockerType ulk( lk );
								try
								{
									_th->sleep( Clock::boostDeltaSec( SR_INTERVAL ) );
									Log::debug("%s has sleeped enough", getLogName());
								}
								catch( boost::thread_interrupted )
								{
									Log::debug("%s was awakened", getLogName());
								}
							}
							Log::debug("%s was awakened !!", getLogName());
						}
					}

					// send BYE
					*_sock << enqueueReport().enqueueBye();
					Log::debug( "%s: sent BYE", getLogName() );
				}
				catch( const KGD::Socket::Exception & e )
				{
					Log::error( "%s: socket error: %s", getLogName(), e.what() );
				}

				{
					Status::type::UnLockerType ulk( lk );
					this->releaseRTP();
				}

				_status[ Status::RUNNING ] = false;
				_status[ Status::PAUSED ] = false;

				Log::debug( "%s: loop terminated", getLogName() );

				this->getStats().log( "Sender" );
			}

			_syncLoopEnd.wait();
		}

		void Sender::start()
		{
			_status.lock();

			_status[ Status::SYNC_WITH_RTP ] = true;

			if ( !_status[ Status::RUNNING ] )
			{
				_status[ Status::RUNNING ] = true;
				_status.unlock();
				_th.reset( new boost::thread(boost::bind(&Sender::sendLoop,this)) );
			}
			else if ( _status[ Status::PAUSED ] )
			{
				_status[ Status::PAUSED ] = false;
				_status.unlock();
				_condUnPause.notify_all();
			}
		}

		void Sender::pause()
		{
			Status::type::LockerType lk( _status );
			_status[ Status::PAUSED ] = true;
		}

		void Sender::unpause()
		{
			_status.lock();
			if ( _status[ Status::PAUSED ] )
			{
				_status[ Status::PAUSED ] = false;
				_status.unlock();
				_condUnPause.notify_all();
			}
			else if ( _status[ Status::RUNNING ] )
			{
				_status.unlock();
				_th->interrupt();
			}
		}

		void Sender::stop()
		{
			_status.lock();
			if ( _status[ Status::RUNNING ] )
			{
				_status[ Status::RUNNING ] = false;

				if ( _status[ Status::PAUSED ] )
				{
					_status[ Status::PAUSED ] = false;
					_status.unlock();
					_condUnPause.notify_all();
				}
				else
				{
					_status.unlock();
					_th->interrupt();
				}
			}
			if ( _th )
			{
				_syncLoopEnd.wait();
				_th.reset();
			}
		}

		void Sender::restart()
		{
			Log::debug( "%s: restarting", getLogName() );

			_status.lock();
			_status[ Status::SYNC_WITH_RTP ] = true;

			{
				SafeStats::LockerType lk( _stats );
				(*_stats).RRcount = 0;
				(*_stats).SRcount = 0;
			}

			if ( _status[ Status::RUNNING ] )
			{
				_status.unlock();
				this->unpause();
			}
			else
			{
				_status.unlock();
				this->start();
			}				
		}

		Sender& Sender::enqueueReport()
		{
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

				Log::message( "%s: building sr time %lf presentation time %lf %lu", getLogName(), curTime, presTime, timeRTP );
			}

			{
				SafeStats::LockerType lk( _stats );
				++ (*_stats).SRcount;
				h.pktCount   = htonl( (*_stats).pktCount );
				h.octetCount = htonl( (*_stats).octetCount );
			}
			h.ssrc = htonl( _rtp.getSsrc() );

			Header hh( PacketType::SenderReport, sizeof(h) );
			_buffer.enqueue( &hh, sizeof(hh) );
			_buffer.enqueue( &h, sizeof(h) );

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

		Stat Sender::getStats() const throw()
		{
			SafeStats::LockerType lk( _stats );
			return *_stats;
		}

	}
}
