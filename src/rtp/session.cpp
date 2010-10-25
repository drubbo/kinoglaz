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
 * File name: ./rtp/session.cpp
 * First submitted: 2010-01-16
 * First submitter: Emiliano Leporati <emiliano.leporati@gmail.com>
 * Contributor(s) so far - 2010-07-30 :
 *     Emiliano Leporati <emiliano.leporati@gmail.com>
 *
 * Last changes :
 *     removed RTSP::Session state concept, conflicting with non-aggregate control
 *     Kinoglaz birth: pervasive bug fixing, improvements and client support
 *     log messages refactor; shared descriptor are optional now; spot insertion support
 *     wimtv key stream
 *     frame / other stuff refactor
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

		const SDP::Medium::Base & Session::getDescription() const
		{
			return _medium;
		}
		SDP::Medium::Base & Session::getDescription()
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

		void Session::loop()
		{
			{
				OwnThread::Lock lk( _th );
				Log::debug( "%s: loop start, for %lf s", getLogName(), _timeEnd );

				try
				{
					uint64_t slp = 0;
					double now = 0, spd = 0;
					double ft = this->fetchNextFrame( lk ) ;

	// 				Log::verbose("%s: loop got frame at %lf s", getLogName(), ft );

					while (!_status.bag[ Status::STOPPED ])
					{
						Log::verbose("%s: main loop entered", getLogName() );
						while( _status.bag[ Status::PAUSED ] )
						{
							Log::debug( "%s: go pause", getLogName() );
							_frame.rate.stop();
							_frame.time->pause( Clock::getSec() );
							// signal "going to pause"
							if ( _pause.sync )
							{
								OwnThread::UnLock ulk( lk );
								_pause.asleep.wait();
							}
							// wait wakeup
							_pause.wakeup.wait( lk );
						}

						try
						{
							// exit loop if stopped, paused, or stream time has ended
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
										OwnThread::UnLock ulk( lk );
										KGD::Clock::sleepNano( slp );
									}
									now = _frame.time->getPresentationTime();
									spd = _frame.time->getSpeed();
								}
							}
							while( !( _status.bag[ Status::STOPPED ] || _status.bag[ Status::PAUSED ] || ( _timeEnd - now ) * sign( spd ) <= 0.0 ) );
						}
						catch ( KGD::Socket::Exception const & e )
						{
							Log::error( "%s: %s", getLogName(), e.what() );
							{
								RTSP::Session::TryLock rtspLk( _rtsp );
								if ( rtspLk.owns_lock() )
								{
									OwnThread::UnLock ulk( lk );
									Log::debug( "%s: %s, asking to remove RTP session", getLogName(), e.what() );
									_rtsp.removeSession( _url.track );
								}									
							}
							throw;
						}
						catch ( RTP::Eof )
						{
							Log::message("%s: reached EOF", getLogName() );
						}
						catch ( KGD::Exception::Generic const & e)
						{
							Log::error( "%s: %s", getLogName(), e.what() );
						}
						// reached EOF or told to pause
						if ( !_status.bag[ Status::STOPPED ] )
						{
							if ( !_status.bag[ Status::PAUSED ] )
							{
								Log::message( "%s: self pausing", getLogName() );
								_status.bag[ Status::PAUSED ] = true;
								_pause.sync = false;
							}
						}
					}
				}
				catch ( const KGD::Exception::Generic & e)
				{
					Log::error( "%s: loop interrupted: %s", getLogName(), e.what() );
				}
				Log::verbose( "%s: main loop exited", getLogName() );

				_status.bag[ Status::STOPPED ] = true;
				_status.bag[ Status::PAUSED ]  = false;

				Log::debug( "%s: stopping rtcp sender", getLogName() );
				_rtcp.sender->stop();
				Log::debug( "%s: stopping buffer", getLogName() );
				_frame.buf->stop();
			}

			Log::debug( "%s: thread loop term sync", getLogName() );
			_th.wait();
		}

		void Session::sendNextFrame( ) throw( KGD::Socket::Exception )
		{
			if ( _frame.next )
			{
				try
				{
					RTP::TTimestamp rtp = _frame.time->getRTPtime( _frame.next->getTime() );
					auto_ptr< Packet::List > pkts = _frame.next->getPackets( rtp, _ssrc, _seqCur );

					BOOST_FOREACH( Packet & pkt, *pkts )
					{
						*_sock << pkt;
						_rtcp.sender->registerPacketSent( pkt.data.size() );
					}
					_frame.rate.tick();
				}
				catch( KGD::Socket::Exception )
				{
					throw;
				}
				catch( const Exception::Generic & e )
				{
					Log::error( "%s: %s", getLogName(), e.what() );
				}
			}
		}


		uint16_t Session::getStartSeq() const
		{
			return _seqStart;
		}

		int Session::getRate() const
		{
			return _medium.getRate();
		}
		TSSrc Session::getSsrc() const
		{
			return _ssrc;
		}

		const Url& Session::getUrl() const
		{
			return _url;
		}

		Channel::Description Session::RTPgetDescription() const
		{
			return _sock->getDescription();
		}
		Channel::Description Session::RTCPgetDescription() const
		{
			return _rtcp.sock->getDescription();
		}

	}

}
