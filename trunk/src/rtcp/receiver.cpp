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
 * File name: ./rtcp/receiver.cpp
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

		static const size_t hSize = sizeof( Header );
		static const size_t hSizeSR = sizeof( SenderReport::Header );
		static const size_t hSizeRR = sizeof( ReceiverReport::Header );
		static const size_t hSizeSDES = sizeof( SourceDescription::Header );
		static const size_t pSizeSR = sizeof( ReceiverReport::Payload );
		static const size_t pSizeRR = sizeof( ReceiverReport::Payload );
		static const size_t pSizeSDES = sizeof( SourceDescription::Payload );

		Receiver::Receiver( RTP::Session & s, const boost::shared_ptr< Channel::Bi > & chan )
		: Thread( s, chan )
		, _logName( s.getLogName() + string(" RTCP Receiver") )
		, _poll( POLL_INTERVAL )
		{
			
		}

		Receiver::~Receiver()
		{
			this->stop();
			Log::verbose( "%s: destroyed", getLogName() );
		}

		const char * Receiver::getLogName() const throw()
		{
			return _logName.c_str();
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
			if ( size < hSize ) return false;
			const Header & h = reinterpret_cast< const Header & >( *data );
			data += hSize; pos += hSize;

			// get sender report data
			if ( size - pos < hSizeSR ) return false;
			const SenderReport::Header & hSR = reinterpret_cast< const SenderReport::Header & >( *data );
			data += hSizeSR; pos += hSizeSR;

			// get payload(s)
			list< const ReceiverReport::Payload * > reports;
			for (size_t i = 0; i < h.count; ++i)
			{
				if ( size - pos < pSizeSR ) return false;
				reports.push_back( reinterpret_cast< const ReceiverReport::Payload * >( data ) );
				data += pSizeSR; pos += pSizeSR;
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
			if ( size < hSize ) return false;
			const Header & h = reinterpret_cast< const Header & >( *data );
			data += hSize; pos += hSize;

			// get receiver report data
			if ( size - pos < hSizeRR ) return false;
			data += hSizeRR; pos += hSizeRR;

			// get payload(s)
			list< const ReceiverReport::Payload * > reports;
			for (size_t i = 0; i < h.count; ++i)
			{
				if ( size - pos < pSizeRR ) return false;
				reports.push_back( reinterpret_cast< const ReceiverReport::Payload * >( data ) );
				data += pSizeRR; pos += pSizeRR;
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
			if ( size < hSize ) return false;
			const Header & h = reinterpret_cast< const Header & >( *data );
			data += hSize; pos += hSize;
			// for each source
			for( size_t n = 0; n < h.count; ++n )
			{
				// get source description header data
				if ( size - pos < hSizeSDES ) return false;
				const SourceDescription::Header & hSD = reinterpret_cast< const SourceDescription::Header & >( *data );
				data += hSizeSDES; pos += hSizeSDES;

				Log::verbose( "%s: parsing SDES, ssrc %X", getLogName(), ntohs( hSD.ssrc ) );

				while( pos < size )
				{
					if ( size - pos < pSizeSDES ) return false;
					const SourceDescription::Payload & pSD = reinterpret_cast< const SourceDescription::Payload & >( *data );
					data += pSizeSDES; pos += pSizeSDES;

					Log::verbose( "%s: parsing SDES item, size %u, attribute %u", getLogName(), pSD.length, pSD.attributeName );

					CharArray tmp( pSD.length + 1 );
					tmp.set( data, pSD.length, 0 );
					data += pSD.length; pos += pSD.length;

					switch ( pSD.attributeName )
					{
					case SourceDescription::Payload::Attribute::END:
						Log::verbose( "%s: END of SDES packet", getLogName() );
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
					(*_stats).destSsrc = ntohs( hSD.ssrc );
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
							++i;
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
			Log::warning( "%s: increment read timeout to %lf", getLogName(), _poll * 1.2 );
			_poll *= 1.2;
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
					while( _flags.bag[ Status::PAUSED ] )
					{
						Log::message( "%s: paused", getLogName() );
						_wakeup.wait( lk );
					}

					if ( _flags.bag[ Status::RUNNING ] )
					{
						try
						{
							ssize_t read = 0;
							Log::verbose( "%s waiting message", getLogName() );
							{
								OwnThread::UnLock ulk( lk );
								read = _sock->readSome( buffer ) ;
							}
							if ( read > 0 )
								this->push( buffer.get(), read );
							else
								Log::warning( "%s: no data read", getLogName() );
							
						}
						catch( const Socket::Exception & e )
						{
							if ( e.getErrcode() == EAGAIN || e.getErrcode() == EWOULDBLOCK )
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
