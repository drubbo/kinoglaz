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
 * File name: ./formats/video/mp4.cpp
 * First submitted: 2010-07-29
 * First submitter: Emiliano Leporati <emiliano.leporati@gmail.com>
 * Contributor(s) so far - 2010-07-30 :
 *     Emiliano Leporati <emiliano.leporati@gmail.com>
 *
 * Last changes :
 *     formats on their own; better Factory elements naming; separate library for every component
 *
 **/

#include "formats/video/mp4.h"

namespace KGD
{
	namespace SDP
	{
		namespace Medium
		{
			// ****************************************************************************************************************

			namespace Video
			{
				MP4::MP4( )
				: Base( MediaType::Video, Payload::VideoMPEG4 )
				{
				}

				MP4::MP4( const MP4 & m )
				: Base( m )
				{

				}

				MP4* MP4::getInfoClone() const throw()
				{
					return new MP4( *this );
				}

				string MP4::getReply( const Url & u ) const throw()
				{
					int iFmt = int(_pt);
					int iIdx = int(_index);
					Url myUrl( u );
					myUrl.track = KGD::toString( iIdx );

					ostringstream s;
					s
						<< "m=video " << 0 << " RTP/AVP " << iFmt << EOL
						<< "a=control:" << myUrl.toString() << EOL
						<< "a=rtpmap:" << iFmt << " MP4V-ES/" << _rate << EOL
						<< "a=fmtp:" << iFmt << " profile-level-id=1;config=";
					for( size_t i = 0; i < _extraData.size(); ++i )
						s << toNibble(_extraData[i] >> 4) << toNibble(_extraData[i] & 0x0F);
					s << EOL;

					return s.str();
				}
			}
		}
	}

	namespace RTP
	{
		namespace Buffer
		{
			namespace Video
			{
				MP4::MP4()
				{
				}
			}
		}
		
		namespace Frame
		{
			namespace Video
			{
				MP4::MP4()
				{
				}

				auto_ptr< Packet::List > MP4::getPackets( RTP::TTimestamp rtp, TSSrc ssrc, TCseq & seq ) throw()
				{
					Header h;
					h.pt = _frame->getPayloadType();
					h.timestamp = htonl(rtp);
					h.ssrc = htonl(ssrc);

					auto_ptr< Packet::List > rt( new Packet::List );

					const ByteArray & myData = this->getData();

					size_t payloadSize = Packet::MTU - Header::SIZE;
					size_t packetized = 0, tot = myData.size();
					const unsigned char* payload = myData.get();

					while( packetized < tot )
					{
						// next payload size
						size_t copySize = min( payloadSize, tot - packetized );
						// alloc packet
						auto_ptr< Packet > pkt( new Packet( copySize + Header::SIZE ) );
						// make packet
						h.seqNo = htons(++ seq);
						// RFC 3016: The marker bit is set to one to indicate the last RTP
						// packet (or only RTP packet) of a VOP
						h.marker = (packetized + copySize >= tot ? 1 : 0 );
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
			}
		}
	}
}
