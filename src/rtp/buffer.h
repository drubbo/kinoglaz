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
 * File name: ./rtp/buffer.h
 * First submitted: 2010-01-16
 * First submitter: Emiliano Leporati <emiliano.leporati@gmail.com>
 * Contributor(s) so far - 2010-07-30 :
 *     Emiliano Leporati <emiliano.leporati@gmail.com>
 *
 * Last changes :
 *     formats on their own; better Factory elements naming; separate library for every component
 *     implemented (unused) support for MP3 ADU
 *     Kinoglaz birth: pervasive bug fixing, improvements and client support
 *     log messages refactor; shared descriptor are optional now; spot insertion support
 *     wimtv key stream
 *
 **/


#ifndef __KGD_RTP_BUFFER
#define __KGD_RTP_BUFFER

#include "lib/exceptions.h"
#include "lib/utils/factory.hpp"
#include "sdp/sdp.h"
#include "rtp/frame.h"

#include <fstream>
#include <deque>
#include <stdexcept>

namespace KGD
{
	namespace RTP
	{
		//! Stream ended
		class Eof: public Exception::Generic
		{
		public:
			Eof() : Exception::Generic("stream data terminated")  {}
		};

		//! pre-buffers
		namespace Buffer
		{
			//! Base class for RTP pre-buffers
			class Base
			: virtual public Factory::Base
			, public boost::noncopyable
			{
			public:
				//! Dimensione massima in secondi del buffer
				static double SIZE_FULL;
				//! Dimensione minima del buffer in secondi
				static double SIZE_LOW;
				//! Limite di velocita' oltre il quale si cominciano a mandare solo iframe
				static double SCALE_LIMIT;
				//! Distanza minima fra due iframe successivi una volta superato scale_limit
				static double SCALE_STEP;
					
			protected:
				//! medium descriptor
				ref< SDP::Medium::Base > _medium;
				//! frame iterator
				boost::scoped_ptr< SDP::Medium::Iterator::Base > _frameIndex;
				//! speed
				double _scale;  

				//! mutex to out buffer
				mutable Mutex _mux;
				//! data available in out buffer
				Condition _condFull;
				//! data requested in out buffer
				Condition _condEmpty;
				typedef boost::ptr_list< RTP::Frame::Base > FrameList;
				//! out frame buffer
				FrameList _bufferOut;

				//! log identifier
				string _logName;
				
				//! clear out buffer
				virtual void clear();
				//! clear out buffer from a certain presentation time
				virtual void clear( double from );
				//! returns size in second of out buffer
				virtual double getOutBufferTimeSize() const;
				//! tells if buffer is under the "low"
				virtual double isBufferLow() const;
				//! tells if buffer size is above the "full"
				virtual double isBufferFull() const;

				//! void ctor
				Base();
				//! construct from a track descriptor
				Base( SDP::Medium::Base & );

			public:
				virtual ~Base();

				//! set medium descriptor after creation
				void setMediumDescriptor( SDP::Medium::Base & );
				//! set parent log identifier after creation
				void setParentLogName( const string & );
				//! get log identifier
				const char * getLogName() const throw();

				//! inserts another medium in the current one, starting not before time t
				virtual void insertMedium( SDP::Medium::Base &, double t ) throw( KGD::Exception::OutOfBounds );
				//! inserts a delay in the current medium, starting not before time t
				virtual void insertTime( double duration, double t ) throw( KGD::Exception::OutOfBounds );

				//! tells actual seek time without doing anything
				virtual double drySeek(double t, double scale) throw( KGD::Exception::OutOfBounds );
				//! seek to new position / speed
				virtual void seek( double t, double scale ) throw( KGD::Exception::OutOfBounds ) = 0;
				//! get next frame in out buffer
				virtual RTP::Frame::Base * getNextFrame() throw( RTP::Eof ) = 0;

				//! get time of first frame
				virtual double getFirstFrameTime() const throw( KGD::Exception::OutOfBounds );

				virtual void start() = 0;
				virtual void stop() = 0;
			};

			//! Frame threaded buffer
			class AVFrame
			: public Base
			{
			private:
				//! fetch thread
				Thread _th;
				//! running status indicator
				bool _running;
				//! mutex to cleanly seek
				Mutex _seekMux;
				//! end-of-file indicator
				bool _eof;

			protected:
				//! fetch loop
				void fetch();

				//! only derived classes can build without params - factory constraint
				AVFrame();

			public:
				//! construct from a track descriptor
				AVFrame( SDP::Medium::Base & );
				//! dtor
				virtual ~AVFrame();

				//! start fetch loop
				virtual void start();
				//! stop fetch loop
				virtual void stop();

				//!@{
				//! base buffer implementation
				virtual void insertMedium( SDP::Medium::Base &, double t ) throw( KGD::Exception::OutOfBounds );
				virtual void insertTime( double duration, double t ) throw( KGD::Exception::OutOfBounds );
				virtual double drySeek(double t, double scale) throw( KGD::Exception::OutOfBounds );
				virtual void seek(double t, double scale) throw( KGD::Exception::OutOfBounds );
				virtual RTP::Frame::AVMedia * getNextFrame() throw( RTP::Eof );
				//!@}
			};
		}
	}
}

#endif
