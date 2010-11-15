#include "sdp/sdp.h"

#include "formats/audio/aac.h"

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
		void Container::loadMediaContainer() throw( SDP::Exception::Generic )
		{
			AVFormatContext *fctx = 0;

			// ff open file
			if ( av_open_input_file(&fctx, this->getFilePath().c_str(), NULL, 0, NULL) != 0 )
				throw SDP::Exception::Generic( "unable to open " + _fileName + " in " + BASE_DIR );
			// ff load stream info
			if( av_find_stream_info(fctx) < 0 )
			{
				av_close_input_file( fctx );
				throw SDP::Exception::Generic( "unable to find streams in " + _fileName );
			}


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
		
	}
}