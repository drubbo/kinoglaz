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
 * File name: ./formats/audio/mp3.h
 * First submitted: 2010-07-29
 * First submitter: Emiliano Leporati <emiliano.leporati@gmail.com>
 * Contributor(s) so far - 2010-07-30 :
 *     Emiliano Leporati <emiliano.leporati@gmail.com>
 *
 * Last changes :
 *     formats on their own; better Factory elements naming; separate library for every component
 *
 **/

#include "sdp/medium.h"
#include "rtp/frame.h"
#include "rtp/buffer.h"

#if MP3_ADU

namespace KGD
{
	namespace SDP
	{
		namespace Medium
		{
			namespace Audio
			{
				//! mpeg2 layer III audio medium descriptor
				class MP3
				: public Base
				, public Factory::Multiton< Base, MP3, CODEC_ID_MP3 >
				{
				protected:
					struct Segment
					{
						ByteArray header;
						ByteArray payload;
						//! frame time
						double t;
						//! set header and payload splitting by size
						void setFrame( const Frame::MediaFile &, size_t );

						Segment();
					};

					Segment _prev;

					MP3( );
					MP3( const MP3 & );
					friend class Factory::Multi< Base, MP3 >;

					void addADU( ) throw();
				public:
					//! clone informations
					virtual MP3* getInfoClone() const throw();
					//! adds last ADU
					virtual void finalizeFrameCount( ) throw();
					//! add frame and build ADUs
					virtual void addFrame( Frame::MediaFile * ) throw();
					//! returns specific protocol reply
					virtual string getReply( const Url & ) const;
				};
			}
		}
	}

	namespace RTP
	{
		namespace Buffer
		{
			namespace Audio
			{
				class MP3
				: public AVFrame
				, public Factory::Multiton< Base, MP3, SDP::A_MP3 >
				{
				protected:
					//! factory ctor
					MP3();
					friend class Factory::Multi< Base, MP3 >;
				};
			}
		}
		
		namespace Frame
		{
			namespace Audio
			{
				//! mpeg2 layer III audio packetization - ADU conversion
				class MP3
				: public AVMedia
				, public Factory::Multiton< Base, MP3, SDP::A_MP3 >
				{
				protected:
					MP3();
					friend class Factory::Multi< Base, MP3 >;
				public:
					//! mp3 audio packetization
					virtual list< Packet* > getPackets( RTP::TTimestamp , TSSrc , TCseq & ) throw( Exception::OutOfBounds );
				};
			}
		}
	}

}
 
#endif