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
 * File name: src/rtp/buffer.cpp
 * First submitted: 2010-07-30
 * First submitter: Emiliano Leporati <emiliano.leporati@gmail.com>
 * Contributor(s) so far - 2010-11-04 :
 *     Emiliano Leporati <emiliano.leporati@gmail.com>
 *
 * Last changes :
 *     threads terminate with wait + join
 *     testing against a "speed crash"
 *     english comments; removed leak with connection serving threads
 *     removed magic numbers in favor of constants / ini parameters
 *     testing interrupted connections
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
				//this->clear();
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
					b.pop_back();
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
					return (b.back().getTime() - b.front().getTime()) * _scale;
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

			Base::Frame::Fetch Base::fetchNextFrame()
			{
				return ( _scale >= 0 ? _frame.idx->next() : _frame.idx->prev() );
			}

			bool Base::isBufferLow() const
			{
				return this->getOutBufferTimeSize() < SIZE_LOW;
			}
			bool Base::isBufferFull() const
			{
				return this->getOutBufferTimeSize() >= SIZE_FULL;
			}

			// ****************************************************************************************************************

			AVFrame::AVFrame()
			: Buffer::Base( )
			, _running( false )
			{
			}

			AVFrame::AVFrame ( SDP::Medium::Base & sdp )
			: Buffer::Base ( sdp )
			, _running( false )
			{
			}

			AVFrame::~AVFrame()
			{
				Log::debug("%s: shutting down", getLogName() );
				this->stop();
				Log::verbose("%s: destroyed", getLogName() );
			}

			void AVFrame::start()
			{
				_frame.lock();

				if ( _th )
				{
					if ( _running )
					{
						_frame.unlock();
						_frame.buf.empty.notify_all();
						return;
					}
					else
					{
						_frame.unlock();
						_th.reset();
						_frame.lock();
					}
				}

				_running = true;
				_frame.unlock();
				_th.reset( new boost::thread( boost::bind ( &AVFrame::fetch, this ) ) );
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
					_th.reset();
				}
			}
				
			void AVFrame::fetch()
			{
				Log::debug("%s: thread started", getLogName() );

				try
				{
					for(;;)
					{
						// lock zone
						Frame::Lock lk( _frame );

						// if buffer "full"
						// wait empty signal
						while ( _running && this->isBufferFull() )
							_frame.buf.empty.wait ( lk );

						if ( _running )
						{
							auto_ptr< RTP::Frame::Base > newFrame;
							// seek lock / give way
							_th.yield( lk );
							// fetch
							Frame::Fetch next = this->fetchNextFrame();
							if ( next )
								try
								{
									newFrame.reset( Factory::ClassRegistry< RTP::Frame::Base >::newInstance( next->getPayloadType() ) );
									newFrame->setFrame( *next );
									newFrame->setTimeShift( _frame.idx->getTimeShift() );
									_frame.buf.data.push_back ( newFrame );
								}
								catch( const KGD::Exception::Generic & e )
								{
									Log::error( "%s: %s", getLogName(), e.what() );
								}

							// signal frames
							if ( !this->isBufferLow() )
							{
								Frame::UnLock ulk( lk );
								_frame.buf.full.notify_all();
							}
						}
						else
							throw RTP::Eof();
					}
				}
				catch( const KGD::Exception::OutOfBounds & e )
				{
					Log::warning( "%s: %s", getLogName(), e.what() );
					_running = false;
				}
				catch( RTP::Eof )
				{
					Log::warning( "%s: EOF", getLogName() );
				}

				Log::debug( "%s: thread term sync", getLogName() );
				_frame.buf.full.notify_all();
				_th.wait();
			}

			Frame::AVMedia* AVFrame::getNextFrame() throw( RTP::Eof )
			{
				Frame::Lock lk( _frame );

				try
				{
					// let's wait some data
					while ( _running && this->isBufferLow() )
						_frame.buf.full.wait ( lk );

					if ( _frame.buf.data.empty() )
						throw RTP::Eof();
					else
					{
						Frame::List::auto_type result = _frame.buf.data.pop_front();
						// if buffer size is low, fetch thread must be awakened
						if ( _running && this->isBufferLow() )
						{
							Frame::UnLock ulk( lk );
							_frame.buf.empty.notify_all();
						}

						return result.release()->asPtrUnsafe< RTP::Frame::AVMedia >();
					}
				}
				catch( boost::thread_interrupted )
				{
					throw RTP::Eof();
				}
			}

			void AVFrame::seek ( double t, double scale ) throw( KGD::Exception::OutOfBounds )
			{
				{
					Frame::Lock lk( _frame );

					this->clear();

					_frame.idx->seek( t );
					_scale  = scale;

					Log::debug("%s: seeked at %lf x %0.2lf", getLogName(), t, scale );
				}
				this->start();
			}


			// ***********************************************************************************************

			namespace Audio
			{
				Base::Base()
				: AVFrame::AVFrame( )
				{
				}

				Base::Base ( SDP::Medium::Base & sdp )
				: AVFrame::AVFrame ( sdp )
				{
				}

				bool Base::isBufferLow() const
				{
					return ( _running && _scale < 0.0 )|| Buffer::Base::isBufferLow();
				}
				bool Base::isBufferFull() const
				{
					return ( _running && _scale < 0.0 ) || Buffer::Base::isBufferFull();
				}

				Base::Frame::Fetch Base::fetchNextFrame()
				{
					ref< const SDP::Frame::Base > rt;

					if ( _scale > 0.0 )
					{
						size_t n = 0;
						do
							rt = Buffer::Base::fetchNextFrame();
						while ( _scale > 2.0 && ++ n < _scale );
					}

					return rt;
				}
			}
			
			// ***********************************************************************************************

			namespace Video
			{
				Base::Base()
				: AVFrame::AVFrame( )
				, _lastKeyTime( -1 )
				{
				}

				Base::Base ( SDP::Medium::Base & sdp )
				: AVFrame::AVFrame ( sdp )
				, _lastKeyTime( -1 )
				{
				}

				void Base::clear()
				{
					Buffer::Base::clear();
					_lastKeyTime = -1;
				}

				Base::Frame::Fetch Base::fetchNextFrame()
				{
					ref< const SDP::Frame::MediaFile > rt;
					size_t n = 0;
					do
						rt = Buffer::Base::fetchNextFrame()->as< SDP::Frame::MediaFile >();
						// in speed (0,1] everything is fetched
						// above or below, one every scale
						// above or below, only distanced key frames
					while ( !
						(	( _scale > 0 && _scale <= 1.0 )
						|| 	( rt->isKey() && ++ n < _scale )
// 						|| 	( rt->isKey() && ( _lastKeyTime < 0 || ( rt->getTime() - _lastKeyTime ) / _scale >= 0.2 ) )
						)
					);
					
// 					if ( rt->isKey() )
// 						_lastKeyTime = rt->getTime();

					return rt;
				}
			}
		}
	}
}
