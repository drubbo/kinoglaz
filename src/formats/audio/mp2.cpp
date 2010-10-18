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
 * File name: ./formats/audio/mp2.cpp
 * First submitted: 2010-07-29
 * First submitter: Emiliano Leporati <emiliano.leporati@gmail.com>
 * Contributor(s) so far - 2010-07-30 :
 *     Emiliano Leporati <emiliano.leporati@gmail.com>
 *
 * Last changes :
 *     formats on their own; better Factory elements naming; separate library for every component
 *
 **/

#include "formats/audio/mp2.h"

namespace KGD
{
	namespace SDP
	{
		namespace Medium
		{
			namespace Audio
			{
				MP2::MP2( )
				: Base( MediaType::Audio, Payload::AudioMPA )
				{
				}

				MP2::MP2( const MP2 & m )
				: Base( m )
				{

				}

				MP2* MP2::getInfoClone() const throw()
				{
					return new MP2( *this );
				}

				string MP2::getReply( const Url & u ) const throw()
				{
					int iFmt = int(_pt);
					int iIdx = int(_index);
					Url myUrl( u );
					myUrl.track = KGD::toString( iIdx );

					ostringstream s;
					s
						<< "m=audio " << 0 << " RTP/AVP " << iFmt << EOL
						<< "a=control:" << myUrl.toString() << EOL;

					return s.str();
				}
			}			
		}
	}

	namespace RTP
	{
		namespace Buffer
		{
			namespace Audio
			{
				MP2::MP2()
				{
				}
			}
		}
		
		namespace Frame
		{
			namespace Audio
			{
				MP2::MP2()
				{
				}

				auto_ptr< Packet::List > MP2::getPackets( RTP::TTimestamp rtp, TSSrc ssrc, TCseq & seq ) throw( Exception::OutOfBounds )
				{
					Header h;
					h.pt = _frame->getPayloadType();
					h.timestamp = htonl(rtp);
					h.ssrc = htonl(ssrc);

					auto_ptr< Packet::List > rt( new Packet::List );

					const ByteArray & myData = this->getData();

					size_t payloadSize = Packet::MTU - Header::SIZE - 4;
					size_t packetized = 0, tot = myData.size();
					const unsigned char* payload = myData.get();

					if ( tot >= 0x10000 )
						throw Exception::OutOfBounds( tot, 0, 0x10000 );

					while( packetized < tot )
					{
						// next payload size
						size_t copySize = min( payloadSize, tot - packetized );
						// alloc packet
						auto_ptr< Packet > pkt( new Packet( copySize + Header::SIZE + 4 ) );

						// make packet
						h.seqNo = htons(++ seq);
						h.marker = ( copySize >= tot ? 1 : 0 );

						uint32_t fragmentOffset = htonl( packetized & 0xFFFF );
						pkt->data
							.set< Header >( h, 0 )
							.set< uint32_t >( fragmentOffset, Header::SIZE )
							.set( &payload[packetized], copySize, Header::SIZE + 4 );

						// advance
						packetized += copySize;
						rt->push_back( pkt );
					}

					rt->back().isLastOfSequence = true;
					return rt;
				}
			}
		}
	}
	
}
