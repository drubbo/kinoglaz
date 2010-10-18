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
 * File name: ./formats/audio/aac.h
 * First submitted: 2010-09-02
 * First submitter: Edoardo Radica <edoardo@cedeo.net>
 * Contributor(s) so far - 2010-09-02 :
 *     Edoardo Radica <edoardo@cedeo.net>
 *     Emiliano Leporati <emiliano.leporati@gmail.com>
 *
 * Last changes :
 *
 **/

#ifndef __KGD_FMT_A_AAC
#define __KGD_FMT_A_AAC

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
				//! AAC audio medium descriptor
				class AAC
				: public Base
				, public Factory::Multiton< Base, AAC, CODEC_ID_AAC >
				{
				protected:
					int _nChannels;

					AAC( );
					AAC( const AAC & );
					friend class Factory::Multi< Base, AAC >;
				public:
					//! clone informations
					virtual AAC* getInfoClone() const throw();
					//! returns specific protocol reply
					virtual string getReply( const Url & ) const throw();
					//! set number of channels
					void setChannels( int ) throw();
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
				class AAC
				: public AVFrame
				, public Factory::Multiton< Base, AAC, SDP::Payload::AudioAAC >
				{
				protected:
					//! factory ctor
					AAC();
					friend class Factory::Multi< Base, AAC >;
				};
			}
		}

		namespace Frame
		{
			namespace Audio
			{
				//! AAC audio packetization
				class AAC
				: public AVMedia
				, public Factory::Multiton< Base, AAC, SDP::Payload::AudioAAC >
				{
				protected:
					AAC();
					friend class Factory::Multi< Base, AAC >;
				public:
					//! AAC audio packetization
					virtual auto_ptr< boost::ptr_list< Packet > > getPackets( RTP::TTimestamp , TSSrc , TCseq & ) throw( Exception::OutOfBounds );
				};
			}
		}
	}
}

#endif
