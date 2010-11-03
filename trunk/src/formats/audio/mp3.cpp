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
 * File name: ./formats/audio/mp3.cpp
 * First submitted: 2010-07-29
 * First submitter: Emiliano Leporati <emiliano.leporati@gmail.com>
 * Contributor(s) so far - 2010-07-30 :
 *     Emiliano Leporati <emiliano.leporati@gmail.com>
 *
 * Last changes :
 *     formats on their own; better Factory elements naming; separate library for every component
 *
 **/

#include "formats/audio/mp3.h"

#if MP3_ADU

namespace KGD
{
	namespace SDP
	{
		namespace Medium
		{
			namespace Audio
			{
				MP3::Segment::Segment()
				: header( 0 )
				, payload( 0 )
				, t( 0 )
				{
				}
				
				void MP3::Segment::setFrame( const Frame::MediaFile& f, size_t split )
				{
					header.set( &f.data[ 0 ], split, 0 );
					header.resize( split );
					payload.set( &f.data[ split ], f.data.size() - split, 0 );
					payload.resize( f.data.size() - split );
					t = f.getTime();
				}

				MP3::MP3( )
				: Base( MediaType::Audio, Payload::AudioMP3 )
				{
				}

				MP3::MP3( const MP3 & m )
				: Base( m )
				{

				}

				MP3* MP3::getInfoClone() const throw()
				{
					return new MP3( *this );
				}

				void MP3::finalizeFrameCount() throw()
				{
					FrameData::Lock lk( _frame );

					this->addADU();

					Base::finalizeFrameCount();
				}

				void MP3::addADU( ) throw()
				{
					if ( ! _prev.header.empty() )
					{
						ByteArray prevADU( _prev.header );
						prevADU.append( _prev.payload );
						Base::addFrame( new Frame::MediaFile( prevADU, _prev.t ) );
					}
				}

				void MP3::addFrame( Frame::MediaFile * f ) throw()
				{
					FrameData::Lock lk( _frame );
					
					// calc side info start
					size_t sideInfoStart = 4;
					bool hasCRC = ( ( f->data[1] & 0x01 ) == 0x00 );
					if ( hasCRC )
						sideInfoStart += 2;
					// calc side info size
					uint8_t mode = ( ( f->data[3] & 0xC0 ) >> 6 );
					size_t sideInfoSize = ( mode < 3 ? 17 : 9 );
					size_t headerSize = sideInfoStart + sideInfoSize;
					// get back pointer - 9 bits
					uint16_t backPtr =
							( uint16_t( f->data[ sideInfoStart ] ) << 1 ) |
							( ( f->data[ sideInfoStart + 1 ] & 0x80 ) >> 7 );

					// this frame starts here; previous frame is complete
					if ( backPtr == 0 )
					{
						this->addADU();
						// reset buffer
						_prev.setFrame( *f, headerSize );
					}
					// this frame starts in the buffer, previous frame is complete
					else
					{
						// if enough data, push
						if ( backPtr <= _prev.payload.size() )
						{
							boost::scoped_ptr< ByteArray > rem( _prev.payload.popBack( backPtr ) );
							this->addADU();
							// reset buffer + remainder
							_prev.setFrame( *f, headerSize );
							_prev.payload.insert( *rem, 0 );
						}
						// if not, previous data is unuseful so dropping is fine
						else
							// reset buffer
							_prev.setFrame( *f, headerSize );
					}
				}

				string MP3::getReply( const Url & u ) const throw()
				{
					int iFmt = int(_pt);
					int iIdx = int(_index);
					Url myUrl( u );
					myUrl.track = KGD::toString( iIdx );

					ostringstream s;
					s
						<< "m=audio " << 0 << " RTP/AVP " << iFmt << EOL
						<< "a=rtpmap:" << iFmt << " mpa-robust/" << _rate << EOL
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
				MP3::MP3()
				{
				}
			}
		}

		namespace Frame
		{
			namespace Audio
			{
				MP3::MP3()
				{
				}

				auto_ptr< Packet::List > MP3::getPackets( RTP::TTimestamp rtp, TSSrc ssrc, TCseq & seq ) throw( Exception::OutOfBounds )
				{
					Header h;
					h.pt = _frame->getPayloadType();
					h.timestamp = htonl(rtp);
					h.ssrc = htonl(ssrc);
					// RFC 3119: This payload format defines no use for this bit.
					// Senders SHOULD set this bit to zero in each outgoing packet.
					h.marker = 0;

					auto_ptr< Packet::List > rt( new Packet::List );

					const ByteArray & myData = this->getData();

					size_t payloadSize = Packet::MTU - Header::SIZE - 2;
					size_t packetized = 0, tot = myData.size();
					const unsigned char* payload = myData.get();

					if ( tot >= 0x40000 )
						throw Exception::OutOfBounds( tot, 0, 0x40000 );

					while( packetized < tot )
					{
						// next payload size
						size_t copySize = min( payloadSize, tot - packetized );
						// alloc packet
						auto_ptr< Packet > pkt( new Packet( copySize + Header::SIZE + 2 ) );

						// make packet
						h.seqNo = htons(++ seq);

						ByteArray aduHeader( 2 );
						aduHeader[ 0 ] = ( packetized > 0 ? 0xC0 : 0x40 ) | ( ( tot & 0x3F00 ) >> 8 );
						aduHeader[ 1 ] = ( tot & 0xFF );
//						string tmp = aduHeader.toString();
//						Log::debug("Mp3 sending size: %lu - %s", tot, tmp.c_str() );

						pkt->data
							.set< Header >( h, 0 )
							.setArray( aduHeader, Header::SIZE )
							.set( &payload[packetized], copySize, Header::SIZE + 2 );

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

#endif
