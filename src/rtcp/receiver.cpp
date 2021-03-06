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
 * File name: src/rtcp/receiver.cpp
 * First submitted: 2010-07-30
 * First submitter: Emiliano Leporati <emiliano.leporati@gmail.com>
 * Contributor(s) so far - 2010-11-04 :
 *     Emiliano Leporati <emiliano.leporati@gmail.com>
 *
 * Last changes :
 *     fixed some SSRC issues; added support for client-hinted ssrc; fixed SIGTERM shutdown when serving
 *     break loop when received END in SDES
 *     "would block" cleanup
 *     testing against a "speed crash"
 *     introduced keep alive on control socket (me dumb)
 *
 **/


#include "rtcp/receiver.h"
#include "rtcp/header.h"
#include "rtp/chrono.h"
#include "lib/clock.h"
#include "rtp/session.h"
#include "lib/log.h"

namespace KGD
{
	namespace RTCP
	{
		double Receiver::POLL_INTERVAL = 5.0;

		namespace
		{
			const size_t HEADER_SIZE = sizeof( Header );
			const size_t HEADER_SIZE_SR = sizeof( SenderReport::Header );
			const size_t HEADER_SIZE_RR = sizeof( ReceiverReport::Header );
			const size_t HEADER_SIZE_SDES = sizeof( SourceDescription::Header );
			const size_t PAYLOAD_SIZE_SR = sizeof( ReceiverReport::Payload );
			const size_t PAYLOAD_SIZE_RR = sizeof( ReceiverReport::Payload );
			const size_t PAYLOAD_SIZE_SDES = sizeof( SourceDescription::Payload );
		}

		Receiver::Receiver( RTP::Session & s, const boost::shared_ptr< Channel::Bi > & chan )
		: Thread( s, chan, "Receiver" )
		, _poll( POLL_INTERVAL )
		{
			
		}

		Receiver::~Receiver()
		{
			Log::verbose( "%s: destroying", getLogName() );
		}

		void Receiver::updateStats( const ReceiverReport::Payload & pRR )
		{
			SafeStats::Lock lk( _stats );

			(*_stats).fractLost = pRR.fractLost;
			(*_stats).pktLost = ntohl(pRR.pktLost);
			(*_stats).highestSeqNo = ntohl(pRR.highestSeqNo);
			(*_stats).jitter = ntohl( pRR.jitter );
			(*_stats).lastSR = ntohl( pRR.lastSR );
			(*_stats).delaySinceLastSR = ntohl( pRR.delaySinceLastSR );

// 			_stats.log( "Receiver" );
		}

		bool Receiver::handleSenderReport( char const * data, size_t size )
		{
			Log::debug( "%s: SR", getLogName() );

			size_t pos = 0;

			// get common header data
			if ( size < HEADER_SIZE ) return false;
			const Header & h = reinterpret_cast< const Header & >( *data );
			data += HEADER_SIZE; pos += HEADER_SIZE;

			// get sender report data
			if ( size - pos < HEADER_SIZE_SR ) return false;
			const SenderReport::Header & hSR = reinterpret_cast< const SenderReport::Header & >( *data );
			data += HEADER_SIZE_SR; pos += HEADER_SIZE_SR;

			// get payload(s)
			list< const ReceiverReport::Payload * > reports;
			for (size_t i = 0; i < h.count; ++i)
			{
				if ( size - pos < PAYLOAD_SIZE_SR ) return false;
				reports.push_back( reinterpret_cast< const ReceiverReport::Payload * >( data ) );
				data += PAYLOAD_SIZE_SR; pos += PAYLOAD_SIZE_SR;
			}
			// packet complete, update stats
			{
				SafeStats::Lock lk( _stats );
				(*_stats).SRcount ++;
				(*_stats).pktCount   = ntohl(hSR.pktCount);
				(*_stats).octetCount = ntohl(hSR.octetCount);
			}
			BOOST_FOREACH( const ReceiverReport::Payload * rpt, reports )
				this->updateStats( *rpt );

			return true;
		}

		bool Receiver::handleReceiverReport( char const * data, size_t size )
		{
			Log::debug( "%s: RR", getLogName() );
			size_t pos = 0;

			// get common header data
			if ( size < HEADER_SIZE ) return false;
			const Header & h = reinterpret_cast< const Header & >( *data );
			data += HEADER_SIZE; pos += HEADER_SIZE;

			// get receiver report data
			if ( size - pos < HEADER_SIZE_RR ) return false;
			data += HEADER_SIZE_RR; pos += HEADER_SIZE_RR;

			// get payload(s)
			list< const ReceiverReport::Payload * > reports;
			for (size_t i = 0; i < h.count; ++i)
			{
				if ( size - pos < PAYLOAD_SIZE_RR ) return false;
				reports.push_back( reinterpret_cast< const ReceiverReport::Payload * >( data ) );
				data += PAYLOAD_SIZE_RR; pos += PAYLOAD_SIZE_RR;
			}
			// packet complete, update stats
			{
				SafeStats::Lock lk( _stats );
				(*_stats).RRcount ++;
			}
			
			BOOST_FOREACH( const ReceiverReport::Payload * rpt, reports )
				this->updateStats( *rpt );

			return true;
		}

		bool Receiver::handleSourceDescription( char const * data, size_t size )
		{
			Log::debug( "%s: SDES", getLogName() );
			size_t pos = 0;

			// get common header data
			if ( size < HEADER_SIZE ) return false;
			const Header & h = reinterpret_cast< const Header & >( *data );
			data += HEADER_SIZE; pos += HEADER_SIZE;
			// for each source
			for( size_t n = 0; n < h.count; ++n )
			{
				// get source description header data
				if ( size - pos < HEADER_SIZE_SDES ) return false;
				const SourceDescription::Header & hSD = reinterpret_cast< const SourceDescription::Header & >( *data );
				data += HEADER_SIZE_SDES; pos += HEADER_SIZE_SDES;

				Log::verbose( "%s: parsing SDES, ssrc %lX, count %u", getLogName(), ntohl( hSD.ssrc ), h.count );

				while( pos < size )
				{
					if ( size - pos < PAYLOAD_SIZE_SDES ) return false;
					const SourceDescription::Payload & pSD = reinterpret_cast< const SourceDescription::Payload & >( *data );
					data += PAYLOAD_SIZE_SDES; pos += PAYLOAD_SIZE_SDES;

					Log::verbose( "%s: parsing SDES item, size %u, attribute %u", getLogName(), pSD.length, pSD.attributeName );

					CharArray tmp( pSD.length + 1 );
					tmp.set( data, pSD.length, 0 );
					data += pSD.length; pos += pSD.length;

					switch ( pSD.attributeName )
					{
					case SourceDescription::Payload::Attribute::END:
						Log::verbose( "%s: END of SDES packet", getLogName() );
						pos = size;
						break;
					case SourceDescription::Payload::Attribute::CNAME:
						Log::verbose( "%s: CNAME is %s", getLogName(), tmp.get() );
						break;
					case SourceDescription::Payload::Attribute::NAME:
						Log::verbose( "%s: NAME is %s", getLogName(), tmp.get() );
						break;
					case SourceDescription::Payload::Attribute::EMAIL:
						Log::verbose( "%s: EMAIL is %s", getLogName(), tmp.get() );
						break;
					case SourceDescription::Payload::Attribute::PHONE:
						Log::verbose( "%s: PHONE is %s", getLogName(), tmp.get() );
						break;
					case SourceDescription::Payload::Attribute::LOC:
						Log::verbose( "%s: GEO LOCATION is %s", getLogName(), tmp.get() );
						break;
					case SourceDescription::Payload::Attribute::TOOL:
						Log::verbose( "%s: TOOL is %s", getLogName(), tmp.get() );
						break;
					case SourceDescription::Payload::Attribute::NOTE:
						Log::verbose( "%s: NOTEs are %s", getLogName(), tmp.get() );
						break;
					case SourceDescription::Payload::Attribute::PRIV:
						Log::verbose( "%s: PRIVATE EXTENSION is %s", getLogName(), tmp.get() );
						break;
					default:
						Log::warning( "%s: unhandled sender description field %u with content %s", getLogName(), pSD.attributeName, tmp.get() );
					}
				}

				// update stats
				{
					SafeStats::Lock lk( _stats );
					(*_stats).destSsrc = ntohl( hSD.ssrc );
				}
			}
			return true;
		}


		void Receiver::push( char const * const buffer, ssize_t len )
		{
			_buffer.enqueue( buffer, len );
			char const * data = _buffer.getDataBegin();
			size_t i = 0;
			// while we have at least four bytes
			while ( i + 3u < _buffer.getDataLength() )
			{
				bool (Receiver::* fPtr) ( char const *, size_t )  = 0;
				PacketType::code type = PacketType::code(uint8_t(data[i + 1 ]));
				size_t size = (ntohs(*((short *) &(data[i + 2]))) + 1) * 4;
				// if we have received the whole packet
				if ( i + size <= _buffer.getDataLength() )
				{
					switch (type)
					{
					// SR
					case PacketType::SenderReport:
						fPtr = &Receiver::handleSenderReport;
						break;
					// RR
					case PacketType::ReceiverReport:
						fPtr = &Receiver::handleReceiverReport;
						break;
					// SDES
					case PacketType::SourceDescription:
						fPtr = &Receiver::handleSourceDescription;
						break;
					// BYE, break the loop
					case PacketType::Bye:
						Log::message( "%s: BYE", getLogName() );
						i += size;
						_flags.bag[ Status::RUNNING ] = false;
						break;
					// APP, ignore
					case PacketType::Application:
						Log::warning( "%s: APP received and ignored", getLogName(), type );
						i += size;
						break;
					// malformed
					default:
						Log::warning( "%s: unknown packet type %u", getLogName(), type );
						++ i;
						continue;
					}
					if ( fPtr )
					{
						if ( (this->*fPtr)( & data[ i ], size ) )
							i += size;
						else
						{
							Log::warning( "%s: failed to parse packet type %u", getLogName(), type );
							++i;
						}
					}
				}
				else
					break;
			}

			Log::verbose( "%s: dequeuing %u bytes", getLogName(), i );
			_buffer.dequeue(i);
		}

		void Receiver::waitMore() throw()
		{
			if ( ! _flags.bag[ Status::PAUSED ] )
			{
				Log::warning( "%s: increment read timeout to %lf", getLogName(), _poll * 1.2 );
				_poll *= 1.2;
				_sock->setReadTimeout( _poll );
			}
		}

		void Receiver::waitLess() throw()
		{
			Log::warning( "%s: decrement read timeout to %lf", getLogName(), _poll / 1.2 );
			_poll /= 1.2;
			_sock->setReadTimeout( _poll );
		}

		void Receiver::run()
		{
			Log::debug( "%s: started", getLogName() );
			CharArray buffer( 1024 );

			{
				OwnThread::Lock lk( _th );

				while( _flags.bag[ Status::RUNNING ] )
				{
					this->doPause( lk );

					if ( _flags.bag[ Status::RUNNING ] )
					{
						try
						{
							ssize_t read = 0;
							Log::verbose( "%s waiting message", getLogName() );
							OwnThread::Interruptible intr( _th );
							read = _sock->readSome( lk, buffer.get(), buffer.size() ) ;
							if ( read > 0 )
							{
								this->push( buffer.get(), read );
								this->waitLess();
							}
							else
								Log::warning( "%s: no data read", getLogName() );
							
						}
						catch( const Socket::Exception & e )
						{
							if ( e.wouldBlock() )
							{
								Log::warning( "%s: %s", getLogName(), e.what() );
								this->waitMore();
							}
							else
							{
								Log::error( "%s: %s, stopping", getLogName(), e.what() );
								_flags.bag[ Status::RUNNING ] = false;
							}
						}
					}
				}

				Log::debug( "%s: loop terminated", getLogName() );

				_flags.bag[ Status::PAUSED ] = false;

				this->getStats().log( "Receiver" );
			}

			_th.wait();
		}

	}
}
