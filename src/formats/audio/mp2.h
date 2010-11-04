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
 * File name: src/formats/audio/mp2.h
 * First submitted: 2010-07-30
 * First submitter: Emiliano Leporati <emiliano.leporati@gmail.com>
 * Contributor(s) so far - 2010-11-04 :
 *     Emiliano Leporati <emiliano.leporati@gmail.com>
 *
 * Last changes :
 *     boosted
 *     Some cosmetics about enums and RTCP library
 *     source import
 *
 **/

#ifndef __KGD_FMT_A_MP2
#define __KGD_FMT_A_MP2

#include "sdp/medium.h"
#include "rtp/frame.h"
#include "rtp/buffer.h"

namespace KGD
{
	namespace SDP
	{
		namespace Medium
		{
			namespace Audio
			{
				//! Mpeg2 audio medium descriptor
				class MP2
				: public Base
				, public Factory::Multiton< Base, MP2, CODEC_ID_MP2 >
#if !MP3_ADU
				, public Factory::Multiton< Base, MP2, CODEC_ID_MP3 >
#endif
				{
				protected:
					MP2( );
					MP2( const MP2 & );
					friend class Factory::Multi< Base, MP2 >;
				public:
					//! clone informations
					virtual MP2* getInfoClone() const throw();
					//! returns specific protocol reply
					virtual string getReply( const Url & ) const throw();
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
				class MP2
				: public AVFrame
				, public Factory::Multiton< Base, MP2, SDP::Payload::AudioMPA >
				{
				protected:
					//! factory ctor
					MP2();
					friend class Factory::Multi< Base, MP2 >;
				};
			}
		}
		
		namespace Frame
		{
			namespace Audio
			{
				//! mpeg2 audio packetization
				class MP2
				: public AVMedia
				, public Factory::Multiton< Base, MP2, SDP::Payload::AudioMPA >
				{
				protected:
					MP2();
					friend class Factory::Multi< Base, MP2 >;
				public:
					//! mpeg2 audio packetization
					virtual auto_ptr< Packet::List > getPackets( RTP::TTimestamp , TSSrc , TCseq & ) throw( Exception::OutOfBounds );
				};
			}
		}
	}
}

#endif
