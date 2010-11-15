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
 * File name: src/sdp/session.cpp
 * First submitted: 2010-07-30
 * First submitter: Emiliano Leporati <emiliano.leporati@gmail.com>
 * Contributor(s) so far - 2010-11-04 :
 *     Emiliano Leporati <emiliano.leporati@gmail.com>
 *
 * Last changes :
 *     threads terminate with wait + join
 *     english comments; removed leak with connection serving threads
 *     introduced keep alive on control socket (me dumb)
 *     testing interrupted connections
 *     testing interrupted connections
 *
 **/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "sdp/sdp.h"
#include "sdp/frame.h"
#include "rtsp/common.h"
#include "rtsp/method.h"
#include "daemon.h"
#include "lib/log.h"
#include "lib/array.hpp"
#include "lib/utils/virtual.hpp"
#include "lib/pls.h"

#include "formats/audio/aac.h"

#include <sstream>
#include <iomanip>
#include <iostream>

#include <boost/foreach.hpp>

extern "C"
{
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavdevice/avdevice.h>
#include <libswscale/swscale.h>
#include <arpa/inet.h>
}

namespace KGD
{

	namespace SDP
	{
		// ****************************************************************************************************************

		string Container::BASE_DIR = "./";
		bool Container::AGGREGATE_CONTROL = true;

		Container::Container( const string & fileName ) throw( SDP::Exception::Generic )
		: _fileName( fileName )
		, _description( fileName )
		, _running( false )
		, _logName( "SDP " + fileName )
		, _uuid( KGD::newUUID() )
		{
			Log::verbose( "%s: creating descriptor", getLogName() );

			if ( _fileName.substr( _fileName.size() - 4) == ".kls" )
				this->loadPlayList();
			else if ( _fileName.substr(0,9) == "dev.video" )
				this->loadLiveCast();
			else
				this->loadMediaContainer();

		}

		Container::~Container()
		{
			this->stop();
			_media.clear();
			Log::verbose( "%s: destroyed", getLogName() );
		}

		void Container::stop()
		{
			OwnThread::Lock lk( _th );
			_running = false;
			if ( _th )
			{
				Log::verbose( "%s: waiting loop termination", getLogName() );

				lk.unlock();
				_th.reset();
			}
		}

		const char * Container::getLogName() const throw()
		{
			return _logName.c_str();
		}

		const string & Container::getUniqueIdentifier() const throw()
		{
			return _uuid;
		}

		void Container::loadPlayList() throw( SDP::Exception::Generic )
		{
			try
			{
				PlayList pl( this->getFilePath() );
				list< string > files = pl.getMediaList();
				list< string >::const_iterator it = files.begin(), ed = files.end();
				if ( it != ed )
				{
					Container c( *it++ );
					this->assign( c );
				}
				while( it != ed )
				{
					Container c( *it++ );
					this->append( c );
				}

				this->loop( pl.getLoops() );
			}
			catch( KGD::Exception::NotFound const & e )
			{
				Log::error( e );
				throw SDP::Exception::Generic( "invalid playlist " + _fileName );
			}
		}

		void Container::loadLiveCast() throw( SDP::Exception::Generic )
		{
			AVFormatContext *iFmtCtx = 0;
			AVCodecContext *oCtx = 0;
			AVInputFormat *iFmt = 0;
			AVCodec *vEnc;
			// ff start
			avdevice_register_all();
			avcodec_register_all();
			av_register_all();

			// open device
			string device = "/" + _fileName;
			std::replace( device.begin(), device.end(), '.', '/' );
			Log::debug( "%s: opening device %s", getLogName(), device.c_str() );

			if ( !(iFmt = av_find_input_format( "video4linux2" )) )
				throw SDP::Exception::Generic( "unable to find video4linux2 input format" );

			if ( av_open_input_file(&iFmtCtx, device.c_str(), iFmt, 0, 0) != 0 )
				throw SDP::Exception::Generic( "unable to open " + _fileName + " in " + BASE_DIR );
			// ff load stream info
			if( av_find_stream_info(iFmtCtx) < 0 )
				throw SDP::Exception::Generic( "unable to find streams in " + _fileName );

			_bitRate = iFmtCtx->bit_rate;
			_duration = HUGE_VAL;


			BOOST_ASSERT( iFmtCtx->nb_streams == 1 );

			AVStream *iStr = iFmtCtx->streams[0];
			AVCodecContext *iCdcCtx = iStr->codec;
			AVCodec *vDec = avcodec_find_decoder( CODEC_ID_RAWVIDEO );
			if ( !vDec )
				throw SDP::Exception::Generic( "unable to find RAW VIDEO decoder" );
			if ( avcodec_open( iCdcCtx, vDec ) < 0 )
				throw SDP::Exception::Generic( "unable to open decoder in context" );


			

			// alloc encoder
			Log::debug( "%s: opening encoder", getLogName() );
			if ( !( oCtx = avcodec_alloc_context() ) )
				throw SDP::Exception::Generic( "unable to alloc codec context" );

			Log::debug( "%s: image size %dx%d", getLogName(), iCdcCtx->width, iCdcCtx->height );
			oCtx->height = iCdcCtx->height;
			oCtx->width = iCdcCtx->width;
			oCtx->time_base.num = 1;
			oCtx->time_base.den = 25;
			oCtx->gop_size = 10;
			oCtx->max_b_frames = 1;
			oCtx->bit_rate = 400000;
			oCtx->pix_fmt = PIX_FMT_YUV420P;

			if ( ! (vEnc = avcodec_find_encoder( CODEC_ID_MPEG4 )) )
				throw SDP::Exception::Generic( "unable to find MPEG4 encoder" );

			if ( avcodec_open( oCtx, vEnc ) < 0 )
				throw SDP::Exception::Generic( "unable to open encoder in context" );


			auto_ptr< Medium::Base > m( Factory::ClassRegistry< Medium::Base >::newInstance( CODEC_ID_MPEG4 ) );
			m->setContainer( *this );
			m->setIndex( 0 );
			// 0x00 0x00 0x01 0xB0 0x03 0x00 0x00 0x01 
			// 0xB5 0x09 0x00 0x00 0x01 0x00 0x00 0x00 
			// 0x01 0x20 0x00 0xBC 0x04 0x06 0xC4 0x00 
			// 0x67 0x0C 0x50 0x10 0xF0 0x51 0x8F 0x00 
			// 0x00 0x01 0xB2 0x58 0x76 0x69 0x44 0x30 
			// 0x30 0x34 0x36
			m->setExtraData( oCtx->extradata, oCtx->extradata_size );
			m->setFileName( this->getFileName() );
			m->setDuration( _duration );
			m->setTimeBase( double(oCtx->time_base.num) / oCtx->time_base.den );

			_media.insert( 0, m );
			Log::debug( "%s: video stream: time base = %d / %d", getLogName(), iStr->time_base.num, iStr->time_base.den );

			{
				OwnThread::Lock lk( _th );
				_running = true;
				_th.reset( new boost::thread( boost::bind( &Container::liveCastLoop, this, iFmtCtx, oCtx ) ));
			}
		}
// static void pgm_save(unsigned char *buf, int wrap, int xsize, int ysize, size_t n)
// {
// 	char filename[1024];
// 	snprintf( filename, 1024, "/tmp/frame%u.pgm", n);
//     FILE *f;
//     int i;
// 
//     f=fopen(filename,"w");
//     fprintf(f,"P5\n%d %d\n%d\n",xsize,ysize,255);
//     for(i=0;i<ysize;i++)
//         fwrite(buf + i * wrap,1,xsize,f);
//     fclose(f);
// }

		void Container::liveCastLoop(AVFormatContext* iFmtCtx, AVCodecContext* oCtx)
		{
			AVStream *iStr = iFmtCtx->streams[0];
			AVCodecContext *iCdcCtx = iStr->codec;
			SwsContext *sws = sws_getContext(
							iCdcCtx->width,
							iCdcCtx->height,
                            iCdcCtx->pix_fmt,
                            oCtx->width,
                            oCtx->height,
                            oCtx->pix_fmt,
                            SWS_FAST_BILINEAR, NULL, NULL, NULL);

			AVFrame *picture = avcodec_alloc_frame();
			AVFrame *planar = avcodec_alloc_frame();
			int planarSz = avpicture_get_size(oCtx->pix_fmt, oCtx->width, oCtx->height);
			ByteArray planarBuf( planarSz );

			typedef map< int64_t, Frame::Base * > FrameCache;
			FrameCache cache;
			int gotPicture = 0;
			int64_t prevPts = -1;

			Medium::Base & medium = _media.at( 0 );
			{
				OwnThread::Lock lk( _th );
// 				int64_t baseDTS = -1;
				while( _running )
				{
					AVPacket pkt;
					av_init_packet( &pkt );
					int rdRes = 0;
					// load frame
					{
						OwnThread::UnLock ulk( lk );
						rdRes = av_read_frame( iFmtCtx, &pkt );
					}
					
					// err
					if ( rdRes < 0 )
					{
						// break cycle
						if ( rdRes == AVERROR_EOF )
							_running = false;
						else
							Log::warning( "%s: av_read_frame error %d", getLogName(), rdRes );
					}
					else if ( pkt.size > 0 )
					{
						// decode
						iCdcCtx->reordered_opaque = pkt.pts;
						int decodeRes = avcodec_decode_video2( iCdcCtx, picture, &gotPicture, &pkt );
						if ( decodeRes < 0 )
							Log::debug( "%s: decoding failed", getLogName() );
						else if (!gotPicture )
							Log::debug( "%s: no picture yet", getLogName() );
						else
						{
							// change colorspace
							avpicture_fill((AVPicture *)planar, planarBuf.get(), oCtx->pix_fmt, oCtx->width, oCtx->height);
							sws_scale(sws, picture->data, picture->linesize, 0, iCdcCtx->height, planar->data, planar->linesize);
							// encode
							ByteArray encBuf( 65536 );
							int encSize = avcodec_encode_video( oCtx, encBuf.get(), encBuf.size(), planar );
							if ( encSize <= 0 )
								encSize = avcodec_encode_video( oCtx, encBuf.get(), encBuf.size(), 0 );

							if ( encSize > 0 )
							{
								// create frame
								auto_ptr< ByteArray > rawData( encBuf.popFront( encSize ) );
								Frame::MediaFile * f = new Frame::MediaFile( *rawData, oCtx->coded_frame->pts * medium.getTimeBase() );
								if ( oCtx->coded_frame->key_frame )
									f->setKey();

								// ordered push to medium
								cache.insert( make_pair( oCtx->coded_frame->pts, f ) );
								FrameCache::iterator itCache = cache.begin();
								while( !cache.empty() && itCache->first == prevPts + 1 )
								{
									prevPts = itCache->first;
									medium.addFrame( itCache->second );
									cache.erase( itCache );
									itCache = cache.begin();
// 										Log::verbose("%s: %lld", getLogName(), prevPts );
								}
							}
							else
								Log::debug( "%s: encoding failed", getLogName() );
						}
					}
					else
						Log::warning( "%s: skipping frame stream %d sz %d", getLogName(), pkt.stream_index, pkt.size );

					av_free_packet( &pkt );
				}

				// finalize sizes
				BOOST_FOREACH( MediaMap::iterator::reference medium, _media )
					medium->second->finalizeFrameCount();
			}
			
			av_close_input_file( iFmtCtx );

			// sync termination
			Log::verbose( "%s: sync loop termination", getLogName() );
			_th.wait();
		}


		void Container::loadMediaContainer() throw( SDP::Exception::Generic )
		{
			AVFormatContext *fctx = 0;
			// ff start
			av_register_all();
			// ff open file
			if ( av_open_input_file(&fctx, this->getFilePath().c_str(), NULL, 0, NULL) != 0 )
				throw SDP::Exception::Generic( "unable to open " + _fileName + " in " + BASE_DIR );
			// ff load stream info
			if( av_find_stream_info(fctx) < 0 )
				throw SDP::Exception::Generic( "unable to find streams in " + _fileName );

			// other info
			_bitRate = fctx->bit_rate;
			_duration = double(fctx->duration) / AV_TIME_BASE;

			// load media
			size_t i = 0;
			for( i = 0; i < fctx->nb_streams; i++)
			{
				AVStream *str = fctx->streams[i];
				AVCodecContext *cdc = str->codec;
				AVRational tBase = str->time_base;
				try
				{
					auto_ptr< Medium::Base > m( Factory::ClassRegistry< Medium::Base >::newInstance( cdc->codec_id ) );
					m->setContainer( *this );
					m->setIndex( i );
					m->setExtraData( cdc->extradata, cdc->extradata_size );
					m->setFileName( this->getFileName() );
					m->setDuration( _duration );
					m->setTimeBase( double(tBase.num) / tBase.den );

					// set specific data
					
					if ( Medium::Audio::AAC * ma = m->asPtrUnsafe< Medium::Audio::AAC >() )
					{
						ma->setRate( cdc->sample_rate );
						ma->setChannels( cdc->channels );
					}

					_media.insert( i, m );
				}
				catch( const KGD::Exception::NotFound & e )
				{
					Log::error( "%s: %s", getLogName(), e.what() );
					_media.clear();
					av_close_input_file( fctx );
					throw SDP::Exception::Generic( "unsupported codec " + string(cdc->codec_name) );
				}
			}

			{
				OwnThread::Lock lk( _th );
				_running = true;
				_th.reset( new boost::thread( boost::bind( &Container::mediaContainerLoop, this, fctx ) ));
			}
		}

		void Container::mediaContainerLoop( AVFormatContext *fctx )
		{
			{
				OwnThread::Lock lk( _th );
				while( _running )
				{
					AVPacket pkt;
					av_init_packet( &pkt );
					// load frame
					int rdRes = av_read_frame( fctx, &pkt );
					// err
					if ( rdRes < 0 )
					{
						// break cycle
						if ( rdRes == AVERROR_EOF )
							_running = false;
						else
							Log::warning( "%s: av_read_frame error %d", getLogName(), rdRes );
					}
					else
					{
						MediaMap::iterator medium = _media.find( pkt.stream_index );
						if ( medium != _media.end() && pkt.size > 0 )
						{
							Medium::Base & m = *medium->second;
							Frame::MediaFile * f = new Frame::MediaFile( pkt, m.getTimeBase() );
							m.addFrame( f );
						}
						else
							Log::warning( "%s: skipping frame stream %d sz %d", getLogName(), pkt.stream_index, pkt.size );
					}
					av_free_packet( &pkt );
					_th.yield( lk );
				}

				// finalize sizes
				BOOST_FOREACH( MediaMap::iterator::reference medium, _media )
					medium->second->finalizeFrameCount();
			}
			av_close_input_file( fctx );

			// sync termination
			Log::verbose( "%s: sync loop termination", getLogName() );
			_th.wait();
		}

		string Container::getFilePath() const throw()
		{
			return BASE_DIR + _fileName;
		}

		const string & Container::getFileName() const throw()
		{
			return _fileName;
		}
		const string & Container::getDescription() const throw()
		{
			return _description;
		}
		void Container::setDescription( const string & d ) throw()
		{
			_description = d;
		}

		const Medium::Base & Container::getMedium( size_t i ) const throw( RTSP::Exception::ManagedError )
		{
			try
			{
				return _media.at( i );
			}
			catch( boost::bad_ptr_container_operation )
			{
				throw RTSP::Exception::ManagedError( RTSP::Error::NotFound );
			}				
		}
		Medium::Base & Container::getMedium( size_t i ) throw( RTSP::Exception::ManagedError )
		{
			try
			{
				return _media.at( i );
			}
			catch( boost::bad_ptr_container_operation )
			{
				throw RTSP::Exception::ManagedError( RTSP::Error::NotFound );
			}
		}

		ref_list< const Medium::Base > Container::getMedia() const throw()
		{
			list< Medium::Base const * > rt;
			BOOST_FOREACH( MediaMap::const_iterator::reference medium, _media )
				rt.push_back( medium->second );

			return rt;
		}
		ref_list< Medium::Base > Container::getMedia() throw()
		{
			list< Medium::Base * > rt;
			BOOST_FOREACH( MediaMap::iterator::reference medium, _media )
				rt.push_back( medium->second );

			return rt;
		}
		void Container::loop( uint8_t times ) throw()
		{
			BOOST_FOREACH( MediaMap::iterator::reference medium, _media )
				medium.second->loop( times );
		}


		void Container::insert( SDP::Container & other, double insertTime ) throw( KGD::Exception::OutOfBounds )
		{
			// insert mediums with correspondent payload type
			// get media to insert
			ref_list< Medium::Base > otherMedia = other.getMedia();
			// for every local medium
			BOOST_FOREACH( MediaMap::iterator::reference localIt, _media )
			{
				MediaMap::mapped_reference localMedium = *localIt->second;
				Log::debug( "%s: media insert: look for payload type %u", getLogName(), localMedium.getPayloadType() );
				// search correspondent medium to insert
				bool found = false;
				BOOST_FOREACH( Medium::Base & otherMedium, otherMedia )
				{
					// if same PT, insert
					if ( localMedium.getPayloadType() == otherMedium.getPayloadType() )
					{
						Log::debug("%s: media insert: found payload type %u", getLogName(), otherMedium.getPayloadType() );
						boost::scoped_ptr< Medium::Iterator::Base > frameIt( otherMedium.newFrameIterator() );
						localMedium.insert( *frameIt, insertTime );
						found = true;
						break;
					}
				}
				// no correspondence found, add void
				if ( !found )
				{
					Log::debug( "%s: media insert: add void to payload type %u", getLogName(), localMedium.getPayloadType() );
					localMedium.insert( other.getDuration(), insertTime );
				}
			}

			_duration += other.getDuration();
		}

		void Container::append( SDP::Container & other ) throw()
		{
			// insert mediums with correspondent payload type
			// get media to insert
			ref_list< SDP::Medium::Base > otherMedia = other.getMedia();
			// for every local medium
			BOOST_FOREACH( MediaMap::iterator::reference localIt, _media )
			{
				MediaMap::mapped_reference localMedium = *localIt->second;
				Log::debug( "%s: media append: look for payload type %u", getLogName(), localMedium.getPayloadType() );
				// search correspondent medium to insert
				BOOST_FOREACH( Medium::Base & otherMedium, otherMedia )
				{
					// if same PT, insert
					if ( localMedium.getPayloadType() == otherMedium.getPayloadType() )
					{
						Log::debug("%s: media append: found payload type %u", getLogName(), otherMedium.getPayloadType() );
						boost::scoped_ptr< Medium::Iterator::Base > frameIt( otherMedium.newFrameIterator() );
						localMedium.append( *frameIt );
						break;
					}
				}
			}
			_duration += other.getDuration();
		}

		void Container::assign( SDP::Container & other ) throw()
		{
			this->stop();
			_media.clear();

			ref_list< SDP::Medium::Base > otherMedia = other.getMedia();
			BOOST_FOREACH( SDP::Medium::Base & otherMedium, otherMedia )
			{
				uint8_t medIdx = otherMedium.getIndex();
				auto_ptr< SDP::Medium::Base > newMed( otherMedium.getInfoClone() );
				newMed->setFileName( this->getFileName() );
				boost::scoped_ptr< Medium::Iterator::Base > frameIt( otherMedium.newFrameIterator() );
				newMed->append( *frameIt );
				_media.erase( medIdx );
				_media.insert( medIdx, newMed );
			}

			_duration = other._duration;
			_bitRate = other._bitRate;
		}

		bool Container::isLiveCast() const
		{
			// when seek support is not active, every description is live
			// else lives are from video devices
			return !RTSP::Method::SUPPORT_SEEK || _fileName.substr(0,9) == "dev.video";
		}

		double Container::getDuration() const
		{
			return _duration;
		}

		double Container::getNtpTime( const time_t & t ) throw()
		{
			return (double(t) + 2208988800u);
		}

		string Container::getReply ( const Url & u, const string & description ) const throw()
		{
			ostringstream s;
			time_t now = time(NULL);

			s
				<< setprecision(0) << fixed << "v=" << VER << EOL
				<< "o=- " << this->getNtpTime(now) << " " << this->getNtpTime(now + (time_t)round(_duration))
				<< " IN IP4 " << u.host << EOL
				<< "s=" << ( description.empty() ? this->getDescription() : description ) << EOL
				<< "c=IN IP4 " << u.host << EOL
				<< "t=0 0" << EOL
				<< "a=type:broadcast" << EOL
				<< "a=tool:" << KGD::Daemon::getName() << EOL;

			// aggregate or per-track control
			if ( AGGREGATE_CONTROL )
				s << "a=control:*" << EOL;

			// range
			s << "a=range:npt=0-" ;
			if ( ! this->isLiveCast() )
				s << setprecision(3) << _duration << EOL;
			else
				s << EOL;

			BOOST_FOREACH( MediaMap::const_iterator::reference it, _media )
				s << it->second->getReply( u );

			return s.str();
		}

		string Container::getReply( const Url & u, const RTSP::TSessionID & sessionID ) const throw()
		{
			return this->getReply( u, "Session #" + toString( sessionID ) );
		}
	}
}
