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
 * File name: src/rtp/session.cpp
 * First submitted: 2010-07-30
 * First submitter: Emiliano Leporati <emiliano.leporati@gmail.com>
 * Contributor(s) so far - 2010-11-04 :
 *     Emiliano Leporati <emiliano.leporati@gmail.com>
 *
 * Last changes :
 *     fixed some SSRC issues; added support for client-hinted ssrc; fixed SIGTERM shutdown when serving
 *     fixed RTSP buffer enqueue
 *     "would block" cleanup
 *     minor cleanup and more robust Range / Scale support during PLAY
 *     introduced keep alive on control socket (me dumb)
 *
 **/


#include "rtp/session.h"
#include "rtp/frame.h"
#include "lib/log.h"
#include "lib/clock.h"
#include "rtsp/session.h"
#include "rtcp/rtcp.h"
#include "rtsp/ports.h"

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

namespace KGD
{

	namespace RTP
	{
		Session::Pause::Pause()
		: asleep(2)
		, sync(false)
		{
		}

		Session::Rtcp::Rtcp( const boost::shared_ptr< Channel::Bi > s )
		: sock( s )
		{
		}

		void Session::Rtcp::start( Session & s )
		{
			sender.reset( new RTCP::Sender( s, sock ));
			receiver.reset( new RTCP::Receiver( s, sock ));
		}

		Session::Session
		( RTSP::Session & parent, const Url & url,
		  SDP::Medium::Base & sdp,
		  const boost::shared_ptr< Channel::Out > rtp,
		  const boost::shared_ptr< Channel::Bi > rtcp,
		  RTSP::UserAgent::type agent
		)
		: _rtsp( parent )
		, _url( url )
		, _medium( sdp )
		, _sock( rtp )
		, _rtcp( rtcp )
		, _th( 2 )
		, _timeEnd( HUGE_VAL )
		, _seqStart(0)
		, _seqCur(0)
		, _ssrc( TSSrc(random()) )
		, _logName( parent.getLogName() + string(" PT ") + toString( sdp.getPayloadType() ) )
		{
			_status.bag[ Status::PAUSED ] = false;
			_status.bag[ Status::STOPPED ] = true;
			_status.bag[ Status::SEEKED ] = false;

			_frame.firstLost = HUGE_VAL;
			_frame.time.reset( Factory::ClassRegistry< Timeline::Medium >::newInstance( agent ) );
			_frame.buf.reset( Factory::ClassRegistry< Buffer::Base >::newInstance( sdp.getPayloadType() ) );
			
			_frame.buf->setMediumDescriptor( sdp );
			_frame.buf->setParentLogName( _logName );

			_frame.time->setRate( sdp.getRate() );

			_rtcp.start( *this );

			this->seqRestart();
		}

		Session::~Session()
		{
			Log::verbose( "%s: destroying", getLogName() );
			
			this->teardown( RTSP::PlayRequest() );

			// release UDP ports if needed
			Channel::Description
				rtpDesc = _sock->getDescription(),
				rtcpDesc = _rtcp.sock->getDescription();
			if ( rtpDesc.type == Channel::Owned && rtcpDesc.type == Channel::Owned )
			{
				RTSP::Port::Udp::getInstance()->release( TPortPair( rtpDesc.ports.first, rtcpDesc.ports.first ) );
				Log::debug( "%s: releasing port pair (%u, %u)", getLogName(), rtpDesc.ports.first, rtcpDesc.ports.first );
			}
			else if ( rtpDesc.type == Channel::Owned || rtcpDesc.type == Channel::Owned )
			{
				Log::error( "%s: RTP and RTCP channel not of same type !?!", getLogName() );
			}

			Log::verbose( "%s: destroyed", getLogName() );
		}

		const char * Session::getLogName() const throw()
		{
			return _logName.c_str();
		}

		bool Session::isPlaying() const throw()
		{
			OwnThread::Lock lk( _th );
			return !_status.bag[ Status::STOPPED ] && !_status.bag[ Status::PAUSED ] ;
		}

		const SDP::Medium::Base & Session::getDescription() const throw()
		{
			return _medium;
		}
		SDP::Medium::Base & Session::getDescription() throw()
		{
			return _medium;
		}

		double Session::fetchNextFrame( OwnThread::Lock & lk) throw( RTP::Eof )
		{

			try
			{
				auto_ptr< RTP::Frame::Base > tmp;

				if ( _status.bag[ Status::SEEKED ] )
				{
					Log::debug( "%s: seeked, skip unordered frames", getLogName() );

					double
						now = _frame.time->getPresentationTime(),
						spd = _frame.time->getSpeed(),
						fTime = HUGE_VAL,
						sendWorse = HUGE_VAL;

					{
						OwnThread::UnLock ulk( lk );
						for(;;)
						{
							tmp.reset( _frame.buf->getNextFrame() );
							fTime = tmp->getTime();
							double sendIn = (fTime - now) / spd;

							if ( sendIn > 1 )
							{
								if ( sendIn > sendWorse )
									break;
								else
								{
									Log::debug( "%s: skip far future frame at %lf to send in %lf", getLogName(), fTime, sendIn );
									now = _frame.time->getPresentationTime();
									sendWorse = sendIn;
								}
							}
							else
								break;
						}
					}

					_status.bag[ Status::SEEKED ] = false;
				}
				else
				{
					OwnThread::UnLock ulk( lk );
					tmp.reset( _frame.buf->getNextFrame() );
				}

				// release sent frame
				if ( _frame.next )
					_medium.releaseFrame( _frame.next->getMediumPos() );
				// update frame to send
				_frame.next.reset( tmp.release() );

				return _frame.next->getTime();
			}
			catch( KGD::Exception::Generic const & e )
			{
				Log::error( "%s: %s", getLogName(), e.what() );
				_frame.next.reset();

				throw;
			}
		}

		void Session::run() throw()
		{
			{
				OwnThread::Lock lk( _th );
				Log::debug( "%s: loop start, for %lf s", getLogName(), _timeEnd );

				uint64_t slp = 0;
				double now = 0, spd = 0;
				double ft = this->fetchNextFrame( lk ) ;
				_rtcp.sender->restart();
				_rtcp.sender->wait();

				// main loop
				while (!_status.bag[ Status::STOPPED ])
				{
					Log::verbose("%s: main loop entered", getLogName() );
					// pause
					while( _status.bag[ Status::PAUSED ] )
					{
						Log::debug( "%s: go pause", getLogName() );
						_frame.rate.stop();
						_rtcp.sender->pause();
						_rtcp.receiver->pause();
						// signal "going to pause"
						if ( _pause.sync )
						{
							OwnThread::UnLock ulk( lk );
							Log::debug( "%s: pause sync", getLogName() );
							_pause.asleep.wait();
						}

						// wait wakeup
						_pause.wakeup.wait( lk );

						Log::verbose( "%s: wakeup from pause", getLogName() );
						if ( ! _status.bag[ Status::STOPPED ] )
						{
							Log::verbose( "%s: awaking RTCP receiver", getLogName() );
							_rtcp.receiver->unpause();
							_rtcp.sender->restart();
							Log::verbose( "%s: waiting RTCP sender", getLogName() );
							_rtcp.sender->wait();
							_frame.rate.start();
						}
					}

					try
					{
						// send loop
						// exit if stopped, paused, or stream time has ended
						do
						{
							// calc sleep while holding last fetched frame
							{
								// send frames while their time is before now
								do
								{
									this->sendNextFrame( );
									ft = this->fetchNextFrame( lk );

									now = _frame.time->getPresentationTime();
									spd = _frame.time->getSpeed();
								}
								while ( ( ft - now ) * sign( spd ) <= 0.0 );
								// this will always be positive
								slp = Clock::secToNano( ( ft - now ) / spd );
							}
							// don't sleep if nothing to wait for
							if ( _frame.next )
							{
								{
									OwnThread::Interruptible intr( _th );
									try
									{
// 										Log::verbose( "%s sleeping for %lf", getLogName(), Clock::nanoToSec(slp) );
										_th.sleepNano( lk, slp );
									}
									catch( boost::thread_interrupted )
									{
										Log::debug("%s was awakened during inter-frame sleep", getLogName());
									}
								}
								now = _frame.time->getPresentationTime();
								spd = _frame.time->getSpeed();
							}
						}
						while( !( _status.bag[ Status::STOPPED ] || _status.bag[ Status::PAUSED ] || ( _timeEnd - now ) * sign( spd ) <= 0.0 ) );
					}
					catch ( RTP::Eof )
					{
						Log::message("%s: reached EOF", getLogName() );
						_rtcp.sender->stop();
					}
					catch ( KGD::Exception::Generic const & e)
					{
						Log::error( "%s: %s", getLogName(), e.what() );
					}
					// out of send loop, go pause
					if ( !_status.bag[ Status::STOPPED ] )
					{
						if ( !_status.bag[ Status::PAUSED ] )
						{
							Log::message( "%s: self pausing", getLogName() );
							_status.bag[ Status::PAUSED ] = true;
							_pause.sync = false;
							_frame.time->pause( Clock::getSec() );
						}
					}
				}

				Log::verbose( "%s: main loop exited", getLogName() );

				_status.bag[ Status::STOPPED ] = true;
				_status.bag[ Status::PAUSED ]  = false;

				Log::verbose( "%s: stopping RTCP sender", getLogName() );
				_rtcp.sender->stop();
				Log::verbose( "%s: stopping RTCP Receiver", getLogName() );
				_rtcp.receiver->stop();
				Log::verbose( "%s: stopping buffer", getLogName() );
				_frame.buf->stop();
			}

			Log::debug( "%s: thread loop term sync", getLogName() );
			_th.wait();
		}

		void Session::sendNextFrame( ) throw( KGD::Socket::Exception )
		{
			if ( _frame.next )
			{
				size_t sendingSz;
				RTP::TTimestamp rtp = _frame.time->getRTPtime( _frame.next->getTime() );
				auto_ptr< Packet::List > pkts = _frame.next->getPackets( rtp, _ssrc, _seqCur );
				try
				{
					BOOST_FOREACH( Packet & pkt, *pkts )
					{
						sendingSz = pkt.data.size();
						*_sock << pkt;
						_rtcp.sender->registerPacketSent( sendingSz );
						_frame.firstLost = HUGE_VAL;
					}
					_frame.rate.tick();
				}
				catch( KGD::Socket::Exception const & e )
				{
					if ( e.wouldBlock() )
					{
						_rtcp.sender->registerPacketLost( sendingSz );
						Log::debug( "%s: packet lost %d / %d, %lf, %lf - %lu bytes", getLogName(), _rtcp.sender->getStats().pktLost, _medium.getFrameCount(), Clock::getSec() - _frame.firstLost, _frame.next->getTime(), sendingSz );
						// more than 10 s of lost packets, drop connection
						if ( _frame.firstLost == HUGE_VAL )
							_frame.firstLost = Clock::getSec();
						else if ( Clock::getSec() - _frame.firstLost >= 5 )
						{
							Log::warning( "%s: 5s packet loss, stopping", getLogName(), e.what() );
							throw;
						}
					}
					else
					{
						Log::warning( "%s: packet lost: %s", getLogName(), e.what() );
						throw;
					}
				}
				catch( const Exception::Generic & e )
				{
					Log::error( "%s: %s", getLogName(), e.what() );
				}
			}
		}


		uint16_t Session::getStartSeq() const throw()
		{
			return _seqStart;
		}

		int Session::getRate() const throw()
		{
			return _medium.getRate();
		}
		void Session::setSsrc( TSSrc ssrc ) throw()
		{
			_ssrc = ssrc;
		}
		TSSrc Session::getSsrc() const throw()
		{
			return _ssrc;
		}

		const Url& Session::getUrl() const throw()
		{
			return _url;
		}

		Channel::Description Session::RTPgetDescription() const throw()
		{
			return _sock->getDescription();
		}
		Channel::Description Session::RTCPgetDescription() const throw()
		{
			return _rtcp.sock->getDescription();
		}

	}

}
