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
 * File name: src/rtp/buffer.h
 * First submitted: 2010-07-30
 * First submitter: Emiliano Leporati <emiliano.leporati@gmail.com>
 * Contributor(s) so far - 2010-11-04 :
 *     Emiliano Leporati <emiliano.leporati@gmail.com>
 *
 * Last changes :
 *     repo content is fine and working again
 *     boosted
 *     source import
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

				class Frame
				: public Safe::LockableBase< RMutex >
				{
				public:
					typedef boost::scoped_ptr< SDP::Medium::Iterator::Base > Index;
					typedef boost::ptr_list< RTP::Frame::Base > List;
					typedef ref< const SDP::Frame::Base > Fetch;
					//! frame iterator
					Index idx;
					//! out frame buffer
					struct Buffer
					{
						List data;
						Condition full;
						Condition empty;
					} buf;
				} _frame;
				
				//! speed
				double _scale;  

				//! log identifier
				string _logName;
				
				//! clear out buffer
				virtual void clear();
				//! clear out buffer from a certain presentation time
				virtual void clear( double from );
				//! returns size in second of out buffer
				virtual double getOutBufferTimeSize() const;
				//! tells if buffer is under the "low"
				virtual bool isBufferLow() const;
				//! tells if buffer size is above the "full"
				virtual bool isBufferFull() const;
				//! get next frame
				virtual Frame::Fetch fetchNextFrame();

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
			protected:
				typedef Safe::ThreadBarrier OwnThread;
				//! fetch thread
				OwnThread _th;
				//! running status indicator
				bool _running;

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
				virtual void seek(double t, double scale) throw( KGD::Exception::OutOfBounds );
				virtual RTP::Frame::AVMedia * getNextFrame() throw( RTP::Eof );
				//!@}
			};

			namespace Audio
			{
				//! audio frame buffer
				class Base
				: public AVFrame
				{
				protected:
					//! yes on negative speed
					virtual bool isBufferLow() const;
					//! yes on negative speed
					virtual bool isBufferFull() const;
					//! one every scale, only for positive speed
					virtual Frame::Fetch fetchNextFrame();

					//! only derived classes can build without params - factory constraint
					Base();

				public:
					//! construct from a track descriptor
					Base( SDP::Medium::Base & );
				};
			}

			namespace Video
			{
				//! video frame buffer
				class Base
				: public AVFrame
				{
				protected:
					double _lastKeyTime;
					//! get next frame: only key frames when speedy
					virtual Frame::Fetch fetchNextFrame();
					//! reset last key time
					virtual void clear();
					//! only derived classes can build without params - factory constraint
					Base();

				public:
					//! construct from a track descriptor
					Base( SDP::Medium::Base & );
				};
			}
		}
	}
}

#endif
