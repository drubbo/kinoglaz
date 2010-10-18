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
 * File name: ./formats/video/mp4.h
 * First submitted: 2010-07-29
 * First submitter: Emiliano Leporati <emiliano.leporati@gmail.com>
 * Contributor(s) so far - 2010-07-30 :
 *     Emiliano Leporati <emiliano.leporati@gmail.com>
 *
 * Last changes :
 *     formats on their own; better Factory elements naming; separate library for every component
 *
 **/

#ifndef __KGD_FMT_V_MP4
#define __KGD_FMT_V_MP4

#include "sdp/medium.h"
#include "rtp/frame.h"
#include "rtp/buffer.h"

namespace KGD
{
	namespace SDP
	{
		namespace Medium
		{
			namespace Video
			{
				//! Mpeg4 video medium descriptor
				class MP4
				: public Base
				, public Factory::Multiton< Base, MP4, CODEC_ID_MPEG4 >
				{
				protected:
					MP4( );
					MP4( const MP4 & );
					friend class Factory::Multi< Base, MP4 >;
				public:
					//! clone informations
					virtual MP4* getInfoClone() const throw();
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
			namespace Video
			{
				class MP4
				: public AVFrame
				, public Factory::Multiton< Base, MP4, SDP::Payload::VideoMPEG4 >
				{
				protected:
					//! factory ctor
					MP4();
					friend class Factory::Multi< Base, MP4 >;
				};
			}
		}
		
		namespace Frame
		{
			//! mpeg4 video packetization
			namespace Video
			{
				//! mpeg4 video
				class MP4
				: public AVMedia
				, public Factory::Multiton< Base, MP4, SDP::Payload::VideoMPEG4 >
				{
				protected:
					//! factory ctor
					MP4();
					friend class Factory::Multi< Base, MP4 >;
				public:
					//! mpeg4 video packetization
					virtual auto_ptr< Packet::List > getPackets( RTP::TTimestamp , TSSrc , TCseq & ) throw();
				};
			}
		}
	}
}

#endif
