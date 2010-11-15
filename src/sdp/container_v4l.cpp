
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "sdp/sdp.h"

#include <boost/foreach.hpp>

extern "C"
{
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavdevice/avdevice.h>
#include <libswscale/swscale.h>
}


namespace KGD
{
	namespace SDP
	{
		void Container::loadLiveCast() throw( SDP::Exception::Generic )
		{
			AVFormatContext *iFmtCtx = 0;
			AVCodecContext *oCtx = 0;
			AVInputFormat *iFmt = 0;
			AVCodec *vEnc;

			// ff open capture device
			string device = "/" + _fileName;
			std::replace( device.begin(), device.end(), '.', '/' );
			Log::debug( "%s: opening device %s", getLogName(), device.c_str() );

			if ( !(iFmt = av_find_input_format( "video4linux2" )) )
				throw SDP::Exception::Generic( "unable to find video4linux2 input format" );

			if ( av_open_input_file(&iFmtCtx, device.c_str(), iFmt, 0, 0) != 0 )
			{
				av_free( iFmt );
				throw SDP::Exception::Generic( "unable to open " + _fileName + " in " + BASE_DIR );
			}

			// ff load stream info
			if( av_find_stream_info(iFmtCtx) < 0 )
			{
				av_close_input_file( iFmtCtx );
				throw SDP::Exception::Generic( "unable to find streams in " + _fileName );
			}
			BOOST_ASSERT( iFmtCtx->nb_streams == 1 );

			_bitRate = iFmtCtx->bit_rate;
			_duration = HUGE_VAL;

			AVStream *iStr = iFmtCtx->streams[0];
			AVCodecContext *iCdcCtx = iStr->codec;

			// ff load input decoder
			if ( ! iCdcCtx->codec )
			{
				AVCodec *vDec = avcodec_find_decoder( iCdcCtx->codec_id );
				if ( !vDec )
					throw SDP::Exception::Generic( "unable to find RAW VIDEO decoder" );

				if ( avcodec_open( iCdcCtx, vDec ) < 0 )
				{
					av_free( vDec );
					av_close_input_file( iFmtCtx );
					throw SDP::Exception::Generic( "unable to open decoder in context" );
				}
			}
			else
				Log::debug( "%s: decoder already present" );

			// ff load output encoder
			Log::debug( "%s: opening encoder", getLogName() );
			if ( !( oCtx = avcodec_alloc_context() ) )
			{
				av_close_input_file( iFmtCtx );
				throw SDP::Exception::Generic( "unable to alloc codec context" );
			}

			Log::debug( "%s: image size %dx%d", getLogName(), iCdcCtx->width, iCdcCtx->height );
			oCtx->height = iCdcCtx->height / 2;
			oCtx->width = iCdcCtx->width / 2;
			oCtx->time_base.num = 1;
			oCtx->time_base.den = 25;
			oCtx->gop_size = 10;
			oCtx->max_b_frames = 1;
			oCtx->bit_rate = 400000;
			oCtx->pix_fmt = PIX_FMT_YUV420P;

			if ( ! (vEnc = avcodec_find_encoder( CODEC_ID_MPEG4 )) )
			{
				avcodec_close( oCtx );
				av_close_input_file( iFmtCtx );
				throw SDP::Exception::Generic( "unable to find MPEG4 encoder" );
			}

			if ( avcodec_open( oCtx, vEnc ) < 0 )
			{
				avcodec_close( oCtx );
				av_free( vEnc );
				av_close_input_file( iFmtCtx );
				throw SDP::Exception::Generic( "unable to open encoder in context" );
			}

			// create medium
			auto_ptr< Medium::Base > m( Factory::ClassRegistry< Medium::Base >::newInstance( CODEC_ID_MPEG4 ) );
			m->setContainer( *this );
			m->setIndex( 0 );
			// 0x00 0x00 0x01 0xB0 0x03 0x00 0x00 0x01
			// 0xB5 0x09 0x00 0x00 0x01 0x00 0x00 0x00
			// 0x01 0x20 0x00 0xBC 0x04 0x06 0xC4 0x00
			// 0x67 0x0C 0x50 0x10 0xF0 0x51 0x8F 0x00
			// 0x00 0x01 0xB2 0x58 0x76 0x69 0x44 0x30
			// 0x30 0x34 0x36

			uint8_t extradata[43] =
				{ 0x00, 0x00, 0x01, 0xB0, 0x03, 0x00, 0x00, 0x01
				, 0xB5, 0x09, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00
				, 0x01, 0x20, 0x00, 0xBC, 0x04, 0x06, 0xC4, 0x00
				, 0x67, 0x0C, 0x50, 0x10, 0xF0, 0x51, 0x8F, 0x00
				, 0x00, 0x01, 0xB2, 0x58, 0x76, 0x69, 0x44, 0x30
				, 0x30, 0x34, 0x36 };
			m->setExtraData( extradata, 43 );
			m->setFileName( this->getFileName() );
			m->setDuration( _duration );
			m->setTimeBase( double(oCtx->time_base.num) / oCtx->time_base.den );
			Log::debug( "%s: video stream: time base = %d / %d", getLogName(), iStr->time_base.num, iStr->time_base.den );

			_media.insert( 0, m );

			// start fetch thread
			{
				OwnThread::Lock lk( _th );
				_running = true;
				_th.reset( new boost::thread( boost::bind( &Container::liveCastLoop, this, iFmtCtx, oCtx ) ));
			}
		}

		void Container::liveCastLoop(AVFormatContext* iFmtCtx, AVCodecContext* oCtx)
		{
			AVCodecContext *iCdcCtx = iFmtCtx->streams[0]->codec;
			// alloc image conversion contenxt
			SwsContext *sws = sws_getContext(
							iCdcCtx->width,
							iCdcCtx->height,
                            iCdcCtx->pix_fmt,
                            oCtx->width,
                            oCtx->height,
                            oCtx->pix_fmt,
                            SWS_FAST_BILINEAR, NULL, NULL, NULL);
			BOOST_ASSERT( sws );

			// input picture
			AVFrame *picture = avcodec_alloc_frame();
			// converted picture
			AVFrame *planar = avcodec_alloc_frame();
			// size of converted picture
			int planarSz = avpicture_get_size(oCtx->pix_fmt, oCtx->width, oCtx->height);
			// conversion buffer
			ByteArray planarBuf( planarSz );

			// frame cache to push encoded frames in pts order
			typedef map< int64_t, Frame::Base * > FrameCache;
			FrameCache cache;
			// prev pts to force pts order
			int64_t prevPts = -1;

			Medium::Base & medium = _media.at( 0 );
			int gotPicture = 0;
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

			avcodec_close( oCtx );
			av_close_input_file( iFmtCtx );

			// sync termination
			Log::verbose( "%s: sync loop termination", getLogName() );
			_th.wait();
		}
		
	}
}
