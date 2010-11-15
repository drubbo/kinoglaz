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
 * File name: src/rtp/frame.h
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


#ifndef __KGD_RTP_FRAME_H
#define __KGD_RTP_FRAME_H

#include "rtp/chrono.h"
#include "rtp/header.h"
#include "rtp/packet.h"
#include "sdp/sdp.h"

extern "C"
{
#include <libavcodec/avcodec.h>
}

namespace KGD
{
	namespace RTP
	{
		//! RTP frame types
		namespace Frame
		{
			//! basic frame, with header, time and data
			class Base
			: virtual public Factory::Base
			{
			protected:
				//! reference to frame data
				ref< const SDP::Frame::Base > _frame;
				//! shift time
				double _shift;
			public:
				//! empty ctor
				Base();
				//! construct from a frame description
				Base( const SDP::Frame::Base & );
				//! packetize at a certain rtp time, for a ssrc, and update cseq
				virtual auto_ptr< Packet::List > getPackets( RTP::TTimestamp , uint32_t , TCseq & ) throw( KGD::Exception::Generic );
				//! get frame time
				double getTime() const throw( KGD::Exception::NullPointer );
				//! get frame position in medium array
				size_t getMediumPos() const throw( KGD::Exception::NullPointer );
				//! set reference to a frame description
				virtual void setFrame( const SDP::Frame::Base & ) throw( KGD::Exception::InvalidType );
				//! set reference to a frame description
				virtual void setTimeShift( double ) throw( );
				//! get frame data if any
				virtual const ByteArray & getData() const throw( KGD::Exception::NotFound );
			};

			//! audio / video medium frame
			class AVMedia
			: public Base
			{
			protected:
				//! reflect property of a SDP::Frame::MediaFile
				bool _isKey;
			public:
				//! empty ctor
				AVMedia( );
				//! construct from a frame description
				AVMedia( const SDP::Frame::Base & );
				//! set reference to a frame description and set _isKey
				virtual void setFrame( const SDP::Frame::Base & ) throw( KGD::Exception::InvalidType );
				//! tell if referenced frame is key
				bool isKey() const;
			};
		}

	}
}

#endif
