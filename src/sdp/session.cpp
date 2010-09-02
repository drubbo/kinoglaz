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
 * File name: ./sdp/session.cpp
 * First submitted: 2010-02-20
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

#include "config.h"

#include "sdp/sdp.h"
#include "sdp/frame.h"
#include "rtsp/common.h"
#include "rtsp/method.h"
#include "daemon.h"
#include "lib/log.h"
#include "lib/array.hpp"
#include "lib/utils/container.hpp"
#include "lib/utils/virtual.hpp"
#include "lib/pls.h"

#include "formats/audio/aac.h"

#include <sstream>
#include <iomanip>
#include <iostream>

extern "C"
{
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
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
		, _running( true )
		, _logName( "SDP " + fileName )
		, _uuid( KGD::newUUID() )
		{
			Log::debug( "%s: creating descriptor", getLogName() );

			if ( _fileName.substr( _fileName.size() - 4) == ".kls" )
				this->loadPlayList();
			else
				this->loadMediaContainer();

		}

		Container::~Container()
		{
			if ( _th )
			{
				_running = false;
				_th->join();
				_th.destroy();
			}
			_media.clear();
			Log::debug( "%s: destroyed", getLogName() );
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

				bool first = true;
				for( Ctr::ConstIterator< list< string > > it( pl.getMediaList() ); it.isValid(); it.next() )
				{
					Container c( *it );
					if ( first )
					{
						this->assign( c );
						first = false;
					}
					else
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

		void Container::loop( uint8_t times ) throw()
		{
			for( TMediumMap::Iterator it( _media ); it.isValid(); it.next() )
				it.val().loop( times );
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
				AVRational ratBase = str->time_base;
				try
				{
					Medium::Base * m = Factory::ClassRegistry< Medium::Base >::newInstance( cdc->codec_id );
					m->setIndex( i );
					m->setExtraData( cdc->extradata, cdc->extradata_size );
					m->setFileName( this->getFileName() );
					m->setDuration( _duration );
					m->setTimeBase( double(ratBase.num) / ratBase.den );

					// set specific data
					Medium::Audio::AAC * ma = m->asPtrUnsafe< Medium::Audio::AAC >();
					if ( ma )
					{
						ma->setSampleRate( cdc->sample_rate );
						ma->setChannels( cdc->channels );
					}

					_media( i ) = m;
				}
				catch( const KGD::Exception::NotFound & e )
				{
					Log::error( "%s: %s", getLogName(), e.what() );
					_media.clear();
					av_close_input_file( fctx );
					throw SDP::Exception::Generic( "unsupported codec " + string(cdc->codec_name) );
				}
			}

			_th = new Thread( boost::bind( &Container::loadFrameIndex, this, fctx ) );
			Thread::yield();
		}

		void Container::loadFrameIndex( AVFormatContext *fctx )
		{
			while( _running )
			{
				// load frame
				AVPacket pkt;
				av_init_packet( &pkt );
				int rdRes = av_read_frame( fctx, &pkt );
				// err
				if ( rdRes < 0 )
				{
					// break cycle
					if ( rdRes == AVERROR_EOF )
						break;
					else
						Log::warning( "%s: av_read_frame error %d", getLogName(), rdRes );
				}
				else
				{
					if ( _media.has( pkt.stream_index ) && pkt.size > 0 )
					{
						Medium::Base & m = *_media( pkt.stream_index );
						Frame::MediaFile * f = new Frame::MediaFile( pkt, m.getTimeBase() );
						m.addFrame( f );
					}
					else
						Log::warning( "%s: skipping frame stream %d sz %d", getLogName(), pkt.stream_index, pkt.size );
				}
				av_free_packet( &pkt );
			}

			av_close_input_file( fctx );

			// finalize sizes
			for( TMediumMap::Iterator it( _media ); it.isValid(); it.next() )
				it.val().finalizeFrameCount();

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
			if ( _media.has( i ) )
				return _media[ i ];
			else
				throw RTSP::Exception::ManagedError( RTSP::Error::NotFound );
		}
		Medium::Base & Container::getMedium( size_t i ) throw( RTSP::Exception::ManagedError )
		{
			if ( _media.has( i ) )
				return *_media( i );
			else
				throw RTSP::Exception::ManagedError( RTSP::Error::NotFound );
		}

		list< Ptr::Ref< const Medium::Base > > Container::getMedia() const throw()
		{
			list< Medium::Base * > meds = _media.values< list >( );
			return Ctr::toConstRef( meds );
		}
		list< Ptr::Ref< Medium::Base > > Container::getMedia() throw()
		{
			list< Medium::Base * > meds = _media.values< list >( );
			return Ctr::toRef( meds );
		}

		void Container::insert( SDP::Container & other, double insertTime ) throw( KGD::Exception::OutOfBounds )
		{
			// insert mediums with correspondent payload type
			// get media to insert
			list< Ptr::Ref< SDP::Medium::Base > > ml = other.getMedia();
			// for every local medium
			for( TMediumMap::Iterator myIt( _media ); myIt.isValid(); myIt.next() )
			{
				Log::debug( "%s: media insert: look for payload type %u", getLogName(), myIt.val().getPayloadType() );
				// search correspondent medium to insert
				bool found = false;
				for( Ctr::Iterator< list< Ptr::Ref< SDP::Medium::Base > > > it( ml ); !found && it.isValid(); it.next() )
				{
					// if same PT, insert
					if ( myIt.val().getPayloadType() == it->get().getPayloadType() )
					{
						Log::debug("%s: media insert: found payload type %u", getLogName(), it->get().getPayloadType() );
						Ptr::Scoped< Medium::Iterator::Base > frameIt = it->get().newFrameIterator();
						myIt.val().insert( *frameIt, insertTime );
						found = true;
					}
				}
				// no correspondence found, add void
				if ( !found )
				{
					Log::debug( "%s: media insert: add void to payload type %u", getLogName(), myIt.val().getPayloadType() );
					myIt.val().insert( other.getDuration(), insertTime );
				}
			}

			_duration += other.getDuration();
		}

		void Container::append( SDP::Container & other ) throw()
		{
			// insert mediums with correspondent payload type
			// get media to insert
			list< Ptr::Ref< SDP::Medium::Base > > ml = other.getMedia();
			// for every local medium
			for( TMediumMap::Iterator myIt( _media ); myIt.isValid(); myIt.next() )
			{
				Log::debug( "%s: media append: look for payload type %u", getLogName(), myIt.val().getPayloadType() );
				// search correspondent medium to insert
				for( Ctr::Iterator< list< Ptr::Ref< SDP::Medium::Base > > > it( ml ); it.isValid(); it.next() )
				{
					// if same PT, insert
					if ( myIt.val().getPayloadType() == it->get().getPayloadType() )
					{
						Log::debug("%s: media append: found payload type %u", getLogName(), it->get().getPayloadType() );
						Ptr::Scoped< Medium::Iterator::Base > frameIt = it->get().newFrameIterator();
						myIt.val().append( *frameIt );
						break;
					}
				}
			}
			_duration += other.getDuration();
		}

		void Container::assign( SDP::Container & other ) throw()
		{
			if ( _th )
			{
				_running = false;
				_th->join();
				_th.destroy();
			}
			_media.clear();

			list< Ptr::Ref< SDP::Medium::Base > > ml = other.getMedia();
			for( Ctr::Iterator< list< Ptr::Ref< SDP::Medium::Base > > > it( ml ); it.isValid(); it.next() )
			{
				uint8_t medIdx = it->get().getIndex();
				Ptr::Scoped< SDP::Medium::Base > newMed = it->get().getInfoClone();
				newMed->setFileName( this->getFileName() );
				Ptr::Scoped< Medium::Iterator::Base > frameIt = it->get().newFrameIterator();
				newMed->append( *frameIt );
				_media( medIdx ) = newMed.release();
			}

			_duration = other._duration;
			_bitRate = other._bitRate;
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
			if ( RTSP::Method::SUPPORT_SEEK )
				s << setprecision(3) << _duration << EOL;
			else
				s << EOL;

			for( TMediumMap::ConstIterator it( _media ); it.isValid(); it.next() )
				s << it.val().getReply( u );

			return s.str();
		}

		string Container::getReply( const Url & u, const RTSP::TSessionID & sessionID ) const throw()
		{
			return this->getReply( u, "Session #" + toString( sessionID ) );
		}
	}
}
