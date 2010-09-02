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
 * File name: ./rtp/buffer.cpp
 * First submitted: 2010-01-16
 * First submitter: Emiliano Leporati <emiliano.leporati@gmail.com>
 * Contributor(s) so far - 2010-07-30 :
 *     Emiliano Leporati <emiliano.leporati@gmail.com>
 *
 * Last changes :
 *     implemented (unused) support for MP3 ADU
 *     Kinoglaz birth: pervasive bug fixing, improvements and client support
 *     log messages refactor; shared descriptor are optional now; spot insertion support
 *     wimtv key stream
 *     frame / other stuff refactor
 *
 **/


#include "rtp/buffer.h"
#include "rtsp/exceptions.h"
#include "lib/common.h"
#include "lib/log.h"
#include "lib/utils/container.hpp"

#include <iomanip>

namespace KGD
{
	namespace RTP
	{
		namespace Buffer
		{
			// ****************************************************************************************************************

			double Base::SIZE_LOW = 1.0;
			double Base::SIZE_FULL = HUGE_VAL;
			double Base::SCALE_LIMIT = 1.0;
			double Base::SCALE_STEP = 0;
			
			Base::Base ( SDP::Medium::Base & sdp )
			: _medium( sdp )
// 			, _trackName( sdp.getTrackName() )
			, _frameIndex( _medium->newFrameIterator() )
			, _scale( 1.0 )
			{
			}

			Base::Base( )
			: _scale( 1.0 )
			{
			}

			const char * Base::getLogName() const throw()
			{
				return _logName.c_str();
			}

			void Base::setParentLogName( const string & s )
			{
				_logName = s + " BUF";
			}

			void Base::setMediumDescriptor( SDP::Medium::Base & sdp )
			{
				Lock lk( _mux );
				_medium = sdp;
// 				_trackName = sdp.getTrackName();
				_frameIndex = sdp.newFrameIterator();
			}

			Base::~Base()
			{
				this->clear();
			}

			void Base::clear()
			{
				Ctr::clear( _bufferOut );
			}

			void Base::clear( double from )
			{
				while( ! _bufferOut.empty() && _bufferOut.back()->getTime() >= from )
				{
					RTP::Frame::Base * f = _bufferOut.back();
					_bufferOut.pop_back();
					Log::verbose("%s: removing frame at %lf", getLogName(), f->getTime() );
					Ptr::clear( f );
				}
			}

			double Base::getFirstFrameTime() const throw( KGD::Exception::OutOfBounds )
			{
				Lock lk( _mux );
				return _frameIndex->at( 0 ).getTime();
			}

			double Base::getOutBufferTimeSize() const
			{
				if ( _bufferOut.size() <= 0 )
					return 0.0;
				else
					return fabs ( (_bufferOut.back()->getTime() - _bufferOut.front()->getTime()) * _scale );
			}


			void Base::insertMedium( SDP::Medium::Base & m, double t ) throw( KGD::Exception::OutOfBounds )
			{
				Lock lk ( _mux );
				this->clear( t );
				_frameIndex->insert( m, t );
			}
			void Base::insertTime( double duration, double t ) throw( KGD::Exception::OutOfBounds )
			{
				Lock lk ( _mux );
				this->clear( t );
				_frameIndex->insert( duration, t );
			}

			double Base::drySeek(double t, double ) throw( KGD::Exception::OutOfBounds )
			{
				Lock lk ( _mux );

				size_t pos = _frameIndex->pos();
				double rt = _frameIndex->seek( t ).getTime();
				_frameIndex->seek( pos );

				return rt;
			}
			
			double Base::isBufferLow() const
			{
				return this->getOutBufferTimeSize() < SIZE_LOW;
			}
			double Base::isBufferFull() const
			{
				return this->getOutBufferTimeSize() >= SIZE_FULL;
			}

			// ****************************************************************************************************************

			AVFrame::AVFrame()
			: Buffer::Base( )
			, _running( false )
			, _eof( false )
			{
			}

			AVFrame::AVFrame ( SDP::Medium::Base & sdp )
			: Buffer::Base ( sdp )
			, _running( false )
			, _eof( false )
			{
			}

			void AVFrame::start()
			{
				_mux.lock();
				_eof = false;

				if ( _th )
				{
					_condEmpty.notify_all();
					_mux.unlock();
				}
				else
				{
					_running = true;
					_mux.unlock();
					_th = new Thread( boost::bind ( &AVFrame::fetch, this ) );
				}
					
			}

			void AVFrame::stop()
			{
				{
					Lock lk( _mux );
					_running = false;
				}
				// awake all waiting threads
				_condEmpty.notify_all();
				_condFull.notify_all();
			}

			AVFrame::~AVFrame()
			{
				Log::message("%s: shutting down", getLogName() );
				this->stop();
				if ( _th )
				{
					Log::message("%s: joining thread", getLogName() );
					_th->join();
				}
			}

			void AVFrame::fetch()
			{
				Log::message("%s: thread started", getLogName() );

				Ptr::Scoped< Lock > lk;

				for(;;)
				{
					lk = new Lock ( _mux );
					// if told to stop, break loop
					if ( ! _running )
						break;
					// if at end of file
					// or buffer "full"
					// or unsupported scale
					// then go pause
					else if ( _eof
								|| ( this->isBufferFull() )
								|| ( _medium->getType() == SDP::MediaType::Audio && _scale > SCALE_LIMIT ) )
						_condEmpty.wait ( *lk );

					// fetch
					try
					{
						// advance iterator
						const SDP::Frame::Base & next = _frameIndex->next();
// 						Log::verbose("%s: got frame key: %d", getLogName(), next.isKey() );
						lk.destroy();

						try
						{
							Lock lkSeek( _seekMux );
							Ptr::Scoped< RTP::Frame::Base > newFrame =
								Factory::ClassRegistry< RTP::Frame::Base >::newInstance( next.getPayloadType() );
							newFrame->as< RTP::Frame::AVMedia >().setFrame( next );

							lk = new Lock ( _mux );
							_bufferOut.push_back ( newFrame.release() );
							lk.destroy();

							if ( !this->isBufferLow() )
								_condFull.notify_all();
						}
						catch( const KGD::Exception::Generic & e )
						{
							Log::error( "%s: %s", getLogName(), e.what() );
						}
					}
					catch( const KGD::Exception::OutOfBounds & e )
					{
						Log::warning( "%s: %s", getLogName(), e.what() );
						_eof = true;
						lk.destroy();
						_condFull.notify_all();
					}

					Thread::yield();
				}

				_eof = true;

				lk.destroy();
				_condFull.notify_all();
			}

			Frame::AVMedia* AVFrame::getNextFrame() throw( RTP::Eof )
			{
				Ptr::Scoped< Lock > lk = new Lock ( _mux );

				// let's wait some data
				while ( this->isBufferLow() && _running && !_eof )
					_condFull.wait ( *lk );

				if ( _bufferOut.empty() )
					throw RTP::Eof();
				else
				{
					RTP::Frame::AVMedia * result = _bufferOut.front()->asPtrUnsafe< RTP::Frame::AVMedia >();
					_bufferOut.pop_front();
					// se ci sono pochi frame, facciamo ripartire il 3d di fetch
					// ma solo se sono flusso video, oppure audio con scale buono (altrimenti dell'audio non carico niente)
					SDP::MediaType::kind mType = _medium->getType();
					if ( ! _eof
							&& ( ( mType == SDP::MediaType::Video ) || ( ( mType == SDP::MediaType::Audio ) && ( _scale <= SCALE_LIMIT ) ) )
							&& this->isBufferLow() )
					{
						lk.destroy();
						_condEmpty.notify_all();
					}

// 					Log::verbose("%s: giving out frame key: %d", getLogName(), result->isKey() );

					return result;
				}
			}

			void AVFrame::insertMedium( SDP::Medium::Base & m, double t ) throw( KGD::Exception::OutOfBounds )
			{
				Lock lkSeek( _seekMux );
				Base::insertMedium( m, t );
			}

			void AVFrame::insertTime( double duration, double t ) throw( KGD::Exception::OutOfBounds )
			{
				Lock lkSeek( _seekMux );
				Base::insertTime( duration, t );
			}

			double AVFrame::drySeek(double t, double s ) throw( KGD::Exception::OutOfBounds )
			{
				Lock lkSeek( _seekMux );
				return Base::drySeek( t, s );
			}


			void AVFrame::seek ( double t, double scale ) throw( KGD::Exception::OutOfBounds )
			{
				{
					Lock lkSeek( _seekMux );
					Lock lk ( _mux );

					this->clear();

					_frameIndex->seek( t );
					_scale  = fabs ( scale );

					Log::debug("%s: seeked at %lf x %0.2lf", getLogName(), t, scale );
				}
				this->start();
			}
		}
	}
}
