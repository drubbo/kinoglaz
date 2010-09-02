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
 * File name: ./formats/audio/aac.cpp
 * First submitted: 2010-09-02
 * First submitter: Edoardo Radica <edoardo@cedeo.net>
 * Contributor(s) so far - 2010-09-02 :
 *     Edoardo Radica <edoardo@cedeo.net>
 *     Emiliano Leporati <emiliano.leporati@gmail.com>
 *
 * Last changes :
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
				: Base( MediaType::Audio, Payload::AudioAAC )
				, _sampleRate( 0 )
				, _nChannels( 0 )
				{
				}

				AAC::AAC( const AAC & m )
				: Base( m )
				, _sampleRate( m._sampleRate )
				, _nChannels( m._nChannels )
				{

				}

				AAC* AAC::getInfoClone() const throw()
				{
					return new AAC( *this );
				}

				void AAC::setSampleRate( int r ) throw()
				{
					_sampleRate = r;
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
						<< "a=rtpmap:" << iFmt << " mpeg4-generic/" << _sampleRate << "/" << _nChannels << EOL
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

				list< Packet* > AAC::getPackets( RTP::TTimestamp rtp, TSSrc ssrc, TCseq & seq ) throw( Exception::OutOfBounds )
				{
					Header h;
					h.pt = _frame->getPayloadType();
					h.timestamp = htonl(rtp);
					h.ssrc = htonl(ssrc);

					list< Packet* > rt;

					const ByteArray & myData = this->getData();

					size_t payloadSize = Packet::MTU - Header::SIZE - 4;
					size_t packetized = 0, tot = myData.size();
					const unsigned char* payload = myData.get();

// 					Log::debug(" --- (getPackets AAC) | tot = %d", tot );
// 					Log::debug(" --- (getPackets AAC) | payloadSize = %d", payloadSize );

					while( packetized < tot )
					{
						// next payload size
						size_t copySize = min( payloadSize, tot - packetized );
// 						Log::debug(" --- (getPackets AAC) | copySize + header = %d + %d ", copySize, sizeof(Header) );
						// alloc packet
						Ptr::Scoped< Packet > pkt = new Packet( copySize + Header::SIZE + 4 );

						// make packet
						h.seqNo = htons(++ seq);
						h.marker = ( copySize >= tot ? 1 : 0 );

						if ( packetized >= 8192 )
							throw Exception::OutOfBounds(packetized, 0, 8192);

// 						Log::debug(" --- (getPackets AAC) | marker bit =  %c", h.marker == 1 ? '1' : '0' );

						uint16_t AU_HS_L = htons( 16 );
// 						Log::debug(" --- (getPackets AAC) | AU_HS_L htons = %2x", AU_HS_L );
						// 13 bit size + 3 bit index (000)
// 						Log::debug(" --- (getPackets AAC) | AU_size_index = %2x", (uint16_t) copySize << 3 );
						uint16_t AU_size_index = htons( uint16_t(copySize) << 3 );

// 						Log::debug(" --- (getPackets AAC) | AU_size_index htons = %2x", AU_size_index );

// 						unsigned char* payloadNOT = (unsigned char*) malloc(copySize * sizeof(unsigned char));

// 						for(int i = 0; i < copySize; i++){
// 							payloadNOT[i] = ~payload[i];
// 						}
// 						Log::debug(" --- (getPackets AAC) | prima = %2x %2x %2x %2x %2x", payload[0], payload[1], payload[2], payload[3],payload[4] );
// 						Log::debug(" --- (getPackets AAC) | dopo  = %2x %2x %2x %2x %2x", payloadNOT[0], payloadNOT[1], payloadNOT[2], payloadNOT[3],payloadNOT[4] );

						pkt->data
							.set< Header >( h, 0 )
							.set< uint16_t >( AU_HS_L, Header::SIZE )
							.set< uint16_t >( AU_size_index, Header::SIZE + 2 )
							.set( &payload[packetized], copySize, Header::SIZE + 4 );

						// advance
						packetized += copySize;
// 						Log::debug(" --- (getPackets AAC) | packetized = %d", packetized );
						rt.push_back( pkt.release() );
					}

					rt.back()->isLastOfSequence = true;
					return rt;
				}
			}
		}
	}

}
