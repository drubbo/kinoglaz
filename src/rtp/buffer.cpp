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
			double Base::SCALE_LIMIT = RTSP::PlayRequest::LINEAR_SCALE;
			double Base::SCALE_STEP = 0;

			Base::Base ( SDP::Medium::Base & sdp )
			: _medium( sdp )
			, _scale( RTSP::PlayRequest::LINEAR_SCALE )
			{
				_frame.idx.reset( _medium->newFrameIterator() );
			}

			Base::Base( )
			: _scale( RTSP::PlayRequest::LINEAR_SCALE )
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
				Frame::Lock lk( _frame );
				_medium = sdp;
				_frame.idx.reset( sdp.newFrameIterator() );
			}

			Base::~Base()
			{
				this->clear();
			}

			void Base::clear()
			{
				Log::debug("%s: flushing whole buffer", getLogName() );

				_frame.buf.data.clear();
			}

			void Base::clear( double from )
			{
				Log::debug("%s: flushing buffer from %lf", getLogName(), from );

				Frame::List & b = _frame.buf.data;
				
				while( ! b.empty() && b.back().getTime() >= from )
				{
					Log::verbose("%s: removing frame at %lf", getLogName(), b.back().getTime() );
					b.erase( b.rbegin().base() );
				}
			}

			double Base::getFirstFrameTime() const throw( KGD::Exception::OutOfBounds )
			{
				Frame::Lock lk( _frame );
				return _frame.idx->at( 0 ).getTime();
			}

			double Base::getOutBufferTimeSize() const
			{
				Frame::List const & b = _frame.buf.data;
				if ( b.size() <= 0 )
					return 0.0;
				else
					return fabs ( (b.back().getTime() - b.front().getTime()) * _scale );
			}


			void Base::insertMedium( SDP::Medium::Base & m, double t ) throw( KGD::Exception::OutOfBounds )
			{
				Frame::Lock lk( _frame );
				this->clear( t );
				_frame.idx->insert( m, t );
			}
			void Base::insertTime( double duration, double t ) throw( KGD::Exception::OutOfBounds )
			{
				Frame::Lock lk( _frame );
				this->clear( t );
				_frame.idx->insert( duration, t );
			}

			double Base::drySeek( double t, double ) throw( KGD::Exception::OutOfBounds )
			{
				Frame::Lock lk( _frame );

				size_t pos = _frame.idx->pos();
				double rt = _frame.idx->seek( t ).getTime();
				_frame.idx->seek( pos );

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
				_frame.lock();
				_eof = false;

				if ( _th )
				{
					_frame.buf.empty.notify_all();
					_frame.unlock();
				}
				else
				{
					_running = true;
					_frame.unlock();
					_th.reset( new boost::thread( boost::bind ( &AVFrame::fetch, this ) ) );
				}

			}

			void AVFrame::stop()
			{
				{
					Frame::Lock lk( _frame );
					_running = false;
				}
				// awake all waiting threads
				_frame.buf.empty.notify_all();
				_frame.buf.full.notify_all();
				// wait for termination
				if ( _th )
				{
					Log::debug("%s: waiting thread", getLogName() );
					_th.wait();
					_th.reset();
				}
			}

			AVFrame::~AVFrame()
			{
				Log::debug("%s: shutting down", getLogName() );
				this->stop();
				Log::verbose("%s: destroyed", getLogName() );
			}

			void AVFrame::fetch()
			{
				Log::debug("%s: thread started", getLogName() );

				for(;;)
				{
					struct Full { const bool brk; Full( bool b ) : brk( b ) {} };
					try
					{
						// lock zone
						Frame::Lock lk( _frame );

						// if at end of file
						// or buffer "full"
						// or unsupported scale
						// then go pause
						while ( _running && ( _eof
									|| ( this->isBufferFull() )
									|| ( _medium->getType() == SDP::MediaType::Audio && _scale > SCALE_LIMIT ) ) )
						{
							_frame.buf.empty.wait ( lk );
						}
							
						
						// if told to stop, break loop
						if ( _running )
						{
							// fetch
							try
							{
								auto_ptr< RTP::Frame::Base > newFrame;
								{
									lk.unlock();
									{
										Lock lkSeek( _seekMux );
										lk.lock();
										// advance iterator
										const SDP::Frame::Base & next = _frame.idx->next();
										try
										{
											// seek lock
											newFrame.reset( Factory::ClassRegistry< RTP::Frame::Base >::newInstance( next.getPayloadType() ) );
											newFrame->as< RTP::Frame::AVMedia >().setFrame( next );
										}
										catch( const KGD::Exception::Generic & e )
										{
											Log::error( "%s: %s", getLogName(), e.what() );
										}
									}
								}
								_frame.buf.data.push_back ( newFrame );

								if ( !this->isBufferLow() )
								{
									throw Full( false );
								}									
							}
							catch( const KGD::Exception::OutOfBounds & e )
							{
								Log::warning( "%s: %s", getLogName(), e.what() );
								_eof = true;
								_running = false;
								throw Full( true );
							}
						}
						else
						{
							throw Full( true );
						}
					}
					catch( Full f )
					{
// 						Log::verbose( "%s: full", getLogName() );
						_frame.buf.full.notify_all();
						if ( f.brk )
							break;
					}

					_th->yield();
				}

				Log::debug( "%s: thread term sync", getLogName() );
				_th.wait();
			}

			Frame::AVMedia* AVFrame::getNextFrame() throw( RTP::Eof )
			{
				Frame::Lock lk( _frame );

				// let's wait some data
				while ( this->isBufferLow() && _running && !_eof )
					_frame.buf.full.wait ( lk );

				if ( _frame.buf.data.empty() )
					throw RTP::Eof();
				else
				{
					Frame::List::auto_type result = _frame.buf.data.pop_front();
					// if buffer size is low, fetch thread must be awakened
					// but only if video, or audio at supported speed
					SDP::MediaType::kind mType = _medium->getType();
					if ( ! _eof
							&& ( ( mType == SDP::MediaType::Video ) || ( ( mType == SDP::MediaType::Audio ) && ( _scale <= SCALE_LIMIT ) ) )
							&& this->isBufferLow() )
					{
						Frame::UnLock ulk( lk );
						_frame.buf.empty.notify_all();
					}

// 					Log::verbose("%s: giving out frame key: %d", getLogName(), result->isKey() );

					return result.release()->asPtrUnsafe< RTP::Frame::AVMedia >();
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
					Frame::Lock lk( _frame );

					this->clear();

					_frame.idx->seek( t );
					_scale  = fabs ( scale );

					Log::debug("%s: seeked at %lf x %0.2lf", getLogName(), t, scale );
				}
				this->start();
			}
		}
	}
}
