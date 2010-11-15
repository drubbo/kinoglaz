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
 * File name: src/formats/audio/aac.cpp
 * First submitted: 2010-09-02
 * First submitter: Emiliano Leporati <emiliano.leporati@gmail.com>
 * Contributor(s) so far - 2010-11-04 :
 *     Emiliano Leporati <emiliano.leporati@gmail.com>
 *
 * Last changes :
 *     boosted
 *     fixed bug related to AAC sample rate
 *     Added AAC support
 *
 **/

#include "formats/audio/aac.h"

namespace KGD
{
	namespace SDP
	{
		namespace Medium
		{
			namespace Audio
			{
				AAC::AAC( )
				: Medium::Base( MediaType::Audio, Payload::AudioAAC )
				, _nChannels( 0 )
				{
				}

				AAC::AAC( const AAC & m )
				: Medium::Base( m )
				, _nChannels( m._nChannels )
				{

				}

				AAC* AAC::getInfoClone() const throw()
				{
					return new AAC( *this );
				}

				void AAC::setChannels( int n ) throw()
				{
					_nChannels = n;
				}

				string AAC::getReply( const Url & u ) const throw()
				{
					int iFmt = int(_pt);
					int iIdx = int(_index);
					Url myUrl( u );
					myUrl.track = KGD::toString( iIdx );

					ostringstream s;
					s
						<< "m=audio " << 0 << " RTP/AVP " << iFmt << EOL
						<< "a=control:" << myUrl.toString() << EOL
						<< "a=rtpmap:" << iFmt << " mpeg4-generic/" << _rate << "/" << _nChannels  << EOL
						<< "a=fmtp:" << iFmt << " profile-level-id=1;mode=AAC-hbr;sizeLength=13;indexLength=3;indexDeltaLength=3;config=";
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
			namespace Audio
			{
				AAC::AAC()
				{
				}
			}
		}

		namespace Frame
		{
			namespace Audio
			{
				AAC::AAC()
				{
				}

				auto_ptr< Packet::List > AAC::getPackets( RTP::TTimestamp rtp, TSSrc ssrc, TCseq & seq ) throw( Exception::OutOfBounds )
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

					// The maximum size of an AAC frame in this mode (AAC-hbr) is 8191 octets.
					if ( tot > 8191 )
						throw Exception::OutOfBounds( tot, 0, 8191 );

					while( packetized < tot )
					{
						// next payload size
						size_t copySize = min( payloadSize, tot - packetized );
						// alloc packet
						auto_ptr< Packet > pkt( new Packet( copySize + Header::SIZE + 4 ) );

						// make packet
						h.seqNo = htons(++ seq);
						// mark if: last fragment
						h.marker = ( copySize >= ( tot - packetized ) ? 1 : 0 );

						uint16_t AU_HS_L = htons( 16 );
						// 13 bit size + 3 bit index (000)
						uint16_t AU_size_index = htons( (uint16_t) copySize << 3 );

						pkt->data
							.set< Header >( h, 0 )
							.set< uint16_t >( AU_HS_L, Header::SIZE )
							.set< uint16_t >( AU_size_index, Header::SIZE + 2 )
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
