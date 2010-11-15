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
#include "rtsp/method.h"
#include "daemon.h"
#include "lib/pls.h"

#include <sstream>
#include <iomanip>
#include <iostream>

#include <boost/foreach.hpp>

extern "C"
{
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
