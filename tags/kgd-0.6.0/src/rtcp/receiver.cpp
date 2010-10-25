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

		Receiver::Receiver( RTP::Session & s, const Ptr::Shared< Channel::Bi > & chan )
		: _sock( chan )
		, _running( false )
		, _paused( false )
		, _logName( s.getLogName() + string(" RTCP Receiver") )
		{

		}

		Receiver::~Receiver()
		{
			this->stop();
			if ( _th /*&& _running */)
			{
				Log::debug( "%s: waiting thread join", getLogName() );
				_th->join();
				_th.destroy();
			}
			Log::debug( "%s: destroyed", getLogName() );
		}

		const char * Receiver::getLogName() const throw()
		{
			return _logName.c_str();
		}

		void Receiver::updateStats( const PacketRR & pRR )
		{
			RLock lk( _muxStats );
			_stats.fractLost = pRR.fractLost;
			_stats.pktLost = ntohl(pRR.pktLost);
			_stats.highestSeqNo = ntohl(pRR.highestSeqNo);
			_stats.jitter = ntohl( pRR.jitter );
			_stats.lastSR = ntohl( pRR.lastSR );
			_stats.delaySinceLastSR = ntohl( pRR.delaySinceLastSR );

// 			_stats.log( "Receiver" );
		}

		bool Receiver::sr( char const * data, size_t size )
		{
			Log::verbose( "%s: SR", getLogName() );

			size_t pos = 0;

			if ( size - pos < sizeof(Header) ) return false;
			const Header & h = reinterpret_cast< const Header & >( *data );
			data += sizeof(Header); pos += sizeof(Header);

			if ( size - pos < sizeof(HeaderSR) ) return false;
			const HeaderSR & hSR = reinterpret_cast< const HeaderSR & >( *data );
			data += sizeof(HeaderSR); pos += sizeof(HeaderSR);

			_stats.SRcount ++;
			_stats.pktCount   = ntohl(hSR.pktCount);
			_stats.octetCount = ntohl(hSR.octetCount);

			for (size_t i = 0; i < h.count; ++i)
			{
				if ( size - pos < sizeof(PacketRR) ) return false;
				const PacketRR & pRR = reinterpret_cast< const PacketRR & >( *data );
				data += sizeof(PacketRR); pos += sizeof(PacketRR);

				this->updateStats( pRR );
			}
			return true;
		}

		bool Receiver::rr( char const * data, size_t size )
		{
			Log::verbose( "%s: RR", getLogName() );
			size_t pos = 0;

			if ( size - pos < sizeof(Header) ) return false;
			const Header & h = reinterpret_cast< const Header & >( *data );
			data += sizeof(Header); pos += sizeof(Header);

			if ( size - pos < sizeof(HeaderRR) ) return false;
			data += sizeof(HeaderRR); pos += sizeof(HeaderRR);

			_stats.RRcount ++;
			for (size_t i = 0; i < h.count; ++i)
			{
				if ( size - pos < sizeof(PacketRR) ) return false;
				const PacketRR & pRR = reinterpret_cast< const PacketRR & >( *data );
				data += sizeof(PacketRR); pos += sizeof(PacketRR);

				this->updateStats( pRR );
			}
			return true;
		}

		bool Receiver::sdes( char const * data, size_t size )
		{
			Log::verbose( "%s: SDES", getLogName() );

			if ( size < sizeof(HeaderSDES) )
				return false;

			const HeaderSDES & hSD = reinterpret_cast< const HeaderSDES & >( *data );

			switch ( hSD.attr_name )
			{
			case HeaderSDES::CNAME:
				_stats.destSsrc = ntohs( hSD.ssrc );
				break;
			}
			return true;
		}

		void Receiver::push( char const * const buffer, ssize_t len )
		{
			_buffer.enqueue( buffer, len );
			char const * data = _buffer.getDataBegin();
			size_t i = 0;
			while (
				_buffer.getDataLength() > 1
				&& i < _buffer.getDataLength() - 2
			)
			{
				bool (Receiver::* fPtr) ( char const * const, size_t )  = 0;
				PacketType type = PacketType(uint8_t(data[i + 1 ]));
				size_t size = (ntohs(*((short *) &(data[i + 2]))) + 1) * 4;

				if ( i + size <= _buffer.getDataLength() )
				{
					switch (type)
					{
					case SR:
						fPtr = &Receiver::sr;
						break;
					case RR:
						fPtr = &Receiver::rr;
						break;
					case RTCP::SDES:
						fPtr = &Receiver::sdes;
						break;
					case RTCP::BYE:
						Log::message( "%s: BYE", getLogName() );
						i += size;
						this->stop();
						break;
					case RTCP::APP:
						Log::warning( "%s: APP received and ignored", getLogName(), type );
						i += size;
						break;
					default:
						++ i;
					}
					if ( fPtr )
					{
						if ( (this->*fPtr)( & data[ i ], size ) )
							i += size;
						else
							++i;
					}
				}
			}

			_buffer.dequeue(i);
		}


		void Receiver::recvLoop()
		{
			Log::debug( "%s: started", getLogName() );
			CharArray buffer( 1024 );

			try
			{
				while( _running )
				{
					while( _paused )
					{
						Lock lk( _mux );
						_condUnPause.wait( lk );
					}

					try
					{
						ssize_t read = 0;
						Log::debug( "%s waiting message", getLogName() );
						if ( ( read = _sock->readSome( buffer ) ) > 0 )
							this->push( buffer.get(), read );
					}
					catch( const Socket::Exception & e )
					{
						if ( e.getErrcode() != EAGAIN )
						{
							Log::error( "%s: %s, stopping", getLogName(), e.what() );
							this->stop();
						}
					}
				}
			}
			catch( const KGD::Socket::Exception & e )
			{
				Log::error( "%s: %s", getLogName(), e.what() );
			}

			_running = false;
			_paused = false;
			Log::debug( "%s: stopped", getLogName() );

			this->getStats().log( "Receiver" );
		}


		void Receiver::start()
		{
			if ( !_running )
			{
				_running = true;
				_th = new Thread(boost::bind(&Receiver::recvLoop,this));
			}
			else if ( _paused )
				this->unpause();
		}

		void Receiver::pause()
		{
			_paused = true;
		}

		void Receiver::unpause()
		{
			Lock lk( _mux );
			_paused = false;
			_condUnPause.notify_all();
		}

		void Receiver::stop()
		{
			_running = false;
			if ( _paused )
				this->unpause();
		}
		Stat Receiver::getStats() const throw()
		{
			RLock lk( _muxStats );
			return _stats;
		}


	}
}