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
 * File name: src/rtp/frame.cpp
 * First submitted: 2010-07-30
 * First submitter: Emiliano Leporati <emiliano.leporati@gmail.com>
 * Contributor(s) so far - 2010-11-04 :
 *     Emiliano Leporati <emiliano.leporati@gmail.com>
 *
 * Last changes :
 *     boosted
 *     removed deadlock issue in RTCP receiver; unloading sent frames from memory when appliable
 *     source import
 *
 **/


#include "rtp/frame.h"
#include "sdp/sdp.h"
#include "lib/log.h"
#include <cstdio>

namespace KGD
{
	namespace RTP
	{
		namespace Frame
		{
			Base::Base( const SDP::Frame::Base &f )
			: _frame( f )
			, _shift( 0 )
			{
			}

			Base::Base()
			: _shift( 0 )
			{
			}

			void Base::setTimeShift( double s ) throw()
			{
				_shift = s;
			}

			double Base::getTime() const throw( KGD::Exception::NullPointer )
			{
				return _frame->getTime() + _shift;
			}

			size_t Base::getMediumPos() const throw( KGD::Exception::NullPointer )
			{
				return _frame->getMediumPos();
			}

			void Base::setFrame( const SDP::Frame::Base & f ) throw( KGD::Exception::InvalidType )
			{
				_frame = f;
			}

			const ByteArray & Base::getData() const throw( KGD::Exception::NotFound )
			{
				try
				{
					return _frame->as< SDP::Frame::MediaFile >().data;
				}
				catch( bad_cast )
				{
					throw KGD::Exception::NotFound( "frame payload data" );
				}
			}

			auto_ptr< Packet::List > Base::getPackets( RTP::TTimestamp rtp, TSSrc ssrc, TCseq & seq ) throw( KGD::Exception::Generic )
			{
				Header h;
				h.pt = _frame->getPayloadType();
				h.timestamp = htonl(rtp);
				h.ssrc = htonl(ssrc);

				auto_ptr< Packet::List > rt( new Packet::List );

				size_t payloadSize = Packet::MTU - Header::SIZE;
				size_t packetized = 0, tot = this->getData().size();
				const unsigned char* payload = this->getData().get();

				while( packetized < tot )
				{
					// next payload size
					size_t copySize = min( payloadSize, tot - packetized );
					// alloc packet
					auto_ptr< Packet > pkt( new Packet( copySize + Header::SIZE ) );
					// make packet
					h.seqNo = htons(++ seq);
					h.marker = 0;
					pkt->data
						.set< Header >( h, 0 )
						.set( &payload[packetized], copySize, Header::SIZE );
					// advance
					packetized += copySize;
					rt->push_back( pkt );
				}
				rt->back().isLastOfSequence = true;
				return rt;
			}

			// *************************************************************************************

			AVMedia::AVMedia( const SDP::Frame::Base &f )
			: Base( f )
			{
				try
				{
					_isKey = _frame->as< SDP::Frame::MediaFile >().isKey();
				}
				catch( bad_cast )
				{
					_isKey = false;
				}
			}

			AVMedia::AVMedia()
			: _isKey( false )
			{
			}

			bool AVMedia::isKey() const
			{
				return _isKey;
			}

			void AVMedia::setFrame( const SDP::Frame::Base & f ) throw( KGD::Exception::InvalidType )
			{
				_frame = f;

				try
				{
					_isKey = _frame->as< SDP::Frame::MediaFile >().isKey();
				}
				catch( bad_cast )
				{
					throw KGD::Exception::InvalidType( "not a MediaFile frame" );
				}
			}

		}
	}
}

