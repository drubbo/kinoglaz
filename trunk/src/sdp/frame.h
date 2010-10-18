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
 * File name: ./sdp/frame.h
 * First submitted: 2010-03-09
 * First submitter: Emiliano Leporati <emiliano.leporati@gmail.com>
 * Contributor(s) so far - 2010-07-30 :
 *     Emiliano Leporati <emiliano.leporati@gmail.com>
 *
 * Last changes :
 *     implemented (unused) support for MP3 ADU
 *     Kinoglaz birth: pervasive bug fixing, improvements and client support
 *     fixed some bug in Log and in insertMedia; comments
 *     log messages refactor; shared descriptor are optional now; spot insertion support
 *     wimtv key stream
 *
 **/


#ifndef __KGD_SDP_FRAME_H
#define __KGD_SDP_FRAME_H

#include "sdp/common.h"
#include "lib/utils/virtual.hpp"
#include <lib/array.h>

#include <string>
#include <functional>

extern "C"
{
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
}

using namespace std;

namespace KGD
{
	namespace SDP
	{
		//! frame data and metadata
		namespace Frame
		{
			//! basic informations
			class Base
			: public Virtual
			{
			protected:
				//! presentation time
				double _time;
				//! time displace
				mutable double _displace;
				//! RTP payload type for this frame
				Payload::type _pt;
				//! array index in medium
				size_t _mediumPos;
			public:
				//! build from a ffmpeg packet and timebase to guess time
				Base( const AVPacket &, double timebase );
				//! build with just time
				Base( double t );
				//! copy
				Base( const Base & );

				//! set position in medium container
				void setMediumPos( size_t );
				//! get position in medium container
				size_t getMediumPos() const;

				//! permanently shifts time by delta seconds
				void addTime( double delta );
				//! set temporary time shift
				void setDisplace( double delta ) const;
				//! get frame time
				double getTime() const;

				//! get RTP payload type
				Payload::type getPayloadType() const;
				//! force different RTP payload type
				void setPayloadType( Payload::type );

				//! get fresh copy
				virtual Base* getClone() const;
			};


			//! ptr container clone support
			Base* new_clone( const Base & b );
			
			//! a frame gently described by ffmpeg, with data
			class MediaFile
			: public Base
			{
			protected:
				//! frame size
				int _size;
				//! key frame indicator (for video media)
				bool _isKey;
				//! frame position into media container
				uint64_t _pos;
			public:
				//! frame data
				ByteArray data;
				//! build from a ffmpeg packet
				MediaFile( const AVPacket &, double timebase );
				//! build from a ffmpeg packet
				MediaFile( const ByteArray &, double t );
				//! copy
				MediaFile( const MediaFile & );
				//! return frame size
				int getSize() const;
				//! return position into media container
				uint64_t getFilePos() const;
				//! return key frame indicator
				bool isKey() const;
				//! get fresh copy
				virtual MediaFile* getClone() const;
			};
		}
	}
}

#endif
