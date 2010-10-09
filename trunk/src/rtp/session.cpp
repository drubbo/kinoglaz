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
#include "lib/utils/container.hpp"
#include "rtsp/ports.h"

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

namespace KGD
{

	namespace RTP
	{
		Session::Session
		( const Url & url,
		  SDP::Medium::Base & sdp,
		  Channel::Out * rtp,
		  Channel::Bi * rtcp,
		  const string & parentLogName,
		  RTSP::UserAgent::type agent
		)
		: _time( Factory::ClassRegistry< Timeline::Medium >::newInstance( agent ) )
		, _paused( false )
		, _stopped( true )
		, _playing( false )
		, _seeked( false )
		, _url( url )
		, _medium( sdp )
		, _RTPsock( rtp )
		, _RTCPsock( rtcp )
		, _frameBuf( Factory::ClassRegistry< Buffer::Base >::newInstance( sdp.getPayloadType() ) )
		, _timeEnd( HUGE_VAL )
		, _seqStart(0)
		, _seqCur(0)
		, _ssrc( TSSrc(random()) )
		, _logName( parentLogName + " PT " + toString( sdp.getPayloadType() ) )
		{
			_frameBuf->setMediumDescriptor( sdp );
			_frameBuf->setParentLogName( _logName );

			_time->setRate( sdp.getRate() );

			_RTCPsender = new RTCP::Sender( *this, _RTCPsock );
			_RTCPreceiver = new RTCP::Receiver( *this, _RTCPsock );

			this->seqRestart();
		}

		Session::~Session()
		{
			this->teardown( RTSP::PlayRequest() );

			// release UDP ports if needed
			Channel::Description
				rtpDesc = _RTPsock->getDescription(),
				rtcpDesc = _RTCPsock->getDescription();
			if ( rtpDesc.type == Channel::Owned && rtcpDesc.type == Channel::Owned )
			{
				RTSP::Port::Udp::getInstance().release( TPortPair( rtpDesc.ports.first, rtcpDesc.ports.first ) );
				Log::debug( "%s: releasing port pair (%u, %u)", getLogName(), rtpDesc.ports.first, rtcpDesc.ports.first );
			}
			else if ( rtpDesc.type == Channel::Owned || rtcpDesc.type == Channel::Owned )
			{
				Log::error( "%s: RTP and RTCP channel not of same type !?!", getLogName() );
			}

			Log::debug( "%s: destroyed", getLogName() );
		}

		const char * Session::getLogName() const throw()
		{
			return _logName.c_str();
		}

		bool Session::isPlaying() const throw()
		{
			return _playing;
		}

		const SDP::Medium::Base & Session::getDescription() const
		{
			return *_medium;
		}
		SDP::Medium::Base & Session::getDescription()
		{
			return *_medium;
		}

		double Session::fetchNextFrame( Ptr::Scoped< Lock > & lk) throw( RTP::Eof )
		{

			try
			{
				Ptr::Scoped< RTP::Frame::Base > tmp;

				if ( _seeked )
				{
					Log::debug( "%s: seeked, skip unordered frames", getLogName() );

					double
						now = _time->getPresentationTime(),
						spd = _time->getSpeed(),
						fTime = HUGE_VAL,
						sendWorse = HUGE_VAL;

					lk.destroy();

					for(;;)
					{
						tmp = _frameBuf->getNextFrame();
						fTime = tmp->getTime();
						double sendIn = (fTime - now) / spd;

						if ( sendIn > 1 )
						{
							if ( sendIn > sendWorse )
								break;
							else
							{
								Log::debug( "%s: skip strange frame at %lf to send in %lf", getLogName(), fTime, sendIn );
								now = _time->getPresentationTime();
								sendWorse = sendIn;
							}
						}
						else
							break;
					}
					lk = new Lock( _mux );
					_seeked = false;
				}
				else
				{
					lk.destroy();
					tmp = _frameBuf->getNextFrame();
					lk = new Lock( _mux );
				}

				// release sent frame
				if ( _frameNext )
					_medium->releaseFrame( _frameNext->getMediumPos() );
				// update frame to send
				_frameNext = tmp.release();

				return _frameNext->getTime();
			}
			catch( ... )
			{
				_frameNext.destroy();
				throw;
			}
		}

		void Session::loop()
		{
			Ptr::Scoped< Lock > lk = new Lock(_mux);
			Log::debug( "%s: loop start, for %lf s", getLogName(), _timeEnd );

			try
			{
				uint64_t slp = 0;
				double now = 0, spd = 0;
				double ft = this->fetchNextFrame( lk ) ;

// 				Log::verbose("%s: loop got frame at %lf s", getLogName(), ft );

				while (!_stopped)
				{
					Log::debug("%s: main loop entered", getLogName() );
					try
					{
						// exit loop if stopped, paused, or stream time has ended
						do
						{
							// calc sleep while holding last fetched frame
							{
								Lock fLk( _frameMux );
								// send frames while their time is before now
								do
								{
									this->sendNextFrame( );
									ft = this->fetchNextFrame( lk );

									now = _time->getPresentationTime();
									spd = _time->getSpeed();
								}
								while ( ( ft - now ) * sign( spd ) <= 0.0 );
								// this will always be positive
								slp = Clock::secToNano( ( ft - now ) / spd );
							}
							// don't sleep if nothing to wait for
							if ( _frameNext )
							{
								lk.destroy();
								KGD::Clock::sleepNano( slp );
								lk = new Lock(_mux);

								now = _time->getPresentationTime();
								spd = _time->getSpeed();
							}
						}
						while( !( _stopped || _paused || ( _timeEnd - now ) * sign( spd ) <= 0.0 ) );
					}
					catch ( RTP::Eof )
					{
						Log::message("%s: reached EOF", getLogName() );
					}
					catch ( KGD::Socket::Exception )
					{
						throw;
					}
					catch ( KGD::Exception::Generic const & e)
					{
						Log::error( "%s: %s", getLogName(), e.what() );
					}
					// reached EOF or told to pause
					if ( !_stopped )
					{
						if ( !_paused )
						{
							Log::message( "%s: self pausing", getLogName() );
							_paused = true;
							_fRate.stop();
							_time->pause( Clock::getSec() );
						}

						Log::message( "%s: go pause", getLogName() );
						_condPaused.notify_all();
						if ( !lk )
							lk = new Lock(_mux);
						_condUnPause.wait(*lk);
					}
				}
			}
			catch ( const KGD::Exception::Generic & e)
			{
				Log::error( "%s: %s", getLogName(), e.what() );
			}
			Log::debug( "%s: loop exited", getLogName() );

			_RTCPsender->stop();

			_stopped = true;
			_paused  = false;

			lk.destroy();

			Log::debug( "%s: loop terminated", getLogName() );
		}

		void Session::sendNextFrame( ) throw( KGD::Socket::Exception )
		{
			if ( _frameNext )
			{
				list< Packet * > pkts;
				try
				{
					RTP::TTimestamp rtp = _time->getRTPtime( _frameNext->getTime() );
					pkts = _frameNext->getPackets( rtp, _ssrc, _seqCur );

					for( Ctr::ConstIterator< list< Packet * > > it( pkts ); it.isValid(); it.next() )
					{
						Packet & pkt = **it;
						*_RTPsock << pkt;
						_RTCPsender->registerPacketSent( pkt.data.size() );
					}
					_fRate.tick();
				}
				catch( KGD::Socket::Exception )
				{
					Ctr::clear( pkts );
					throw;
				}
				catch( const Exception::Generic & e )
				{
					Log::error( "%s: %s", getLogName(), e.what() );
				}
				Ctr::clear( pkts );
			}
		}


		uint16_t Session::getStartSeq() const
		{
			return _seqStart;
		}

		int Session::getRate() const
		{
			return _medium->getRate();
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
			return _RTPsock->getDescription();
		}
		Channel::Description Session::RTCPgetDescription() const
		{
			return _RTCPsock->getDescription();
		}

	}

}
