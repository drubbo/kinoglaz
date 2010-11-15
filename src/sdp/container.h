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
 * File name: src/sdp/session.h
 * First submitted: 2010-07-30
 * First submitter: Emiliano Leporati <emiliano.leporati@gmail.com>
 * Contributor(s) so far - 2010-11-04 :
 *     Emiliano Leporati <emiliano.leporati@gmail.com>
 *
 * Last changes :
 *     introduced keep alive on control socket (me dumb)
 *     testing interrupted connections
 *     boosted
 *     added parameter to control aggregate / per track control; fixed parse and send routine of SDES RTCP packets
 *     Some cosmetics about enums and RTCP library
 *
 **/



#ifndef __KGD_SDP_SESSION_H
#define __KGD_SDP_SESSION_H

#include "sdp/common.h"
#include "rtsp/common.h"
#include "rtsp/exceptions.h"
#include "lib/common.h"
#include "lib/array.hpp"
#include "lib/utils/safe.hpp"
#include "lib/urlencode.h"
#include "lib/utils/ref.hpp"
#include "lib/utils/ref_container.hpp"

#include <string>
#include <map>
#include <list>

extern "C"
{
#include <libavformat/avformat.h>
}


using namespace std;

namespace cv
{
	class VideoCapture;
}

namespace KGD
{
	namespace SDP
	{

		namespace Medium
		{
			class Base;
		}

		//! session description
		class Container
		: public boost::noncopyable
		{
		protected:
			typedef boost::ptr_map< uint8_t, Medium::Base > MediaMap;
			typedef Safe::Thread< RMutex > OwnThread;

			//! resource name
			string _fileName;
			//! resource description
			string _description;
			//! duration in seconds
			double _duration;
			//! medium bit rate
			int _bitRate;

			//! medium list
			MediaMap _media;

			//! load frame thread
			OwnThread _th;
			//! is load frame thread running ?
			bool _running;

			//! constantly fetches frames from video device
			void loadLiveCast() throw( SDP::Exception::Generic );
			//! live cast thread loop
			void liveCastLoop( AVFormatContext*, AVCodecContext* );
			//! loads a kinoglaz playlist
			void loadPlayList() throw( SDP::Exception::Generic );
			//! loads a media container
			void loadMediaContainer() throw( SDP::Exception::Generic );
			//! load index thread loop
			void mediaContainerLoop( AVFormatContext* );

			//! log identifier
			const string _logName;

			//! unique identifier
			const string _uuid;

			//! returns time in NTP format
			static double getNtpTime( const time_t & t ) throw();

			//! stop the thread
			void stop();
		public:
			//! loads file metadata
			Container( const string & fileName ) throw( SDP::Exception::Generic );
			//! dtor
			~Container();
			//! returns log identifier
			const char * getLogName() const throw();
			//! returns unique identifier for this descriptor
			const string & getUniqueIdentifier() const throw();
			//! returns duration - HUGE_VAL for livecasts
			double getDuration() const;
			//! tells if this description refers to a livecast
			bool isLiveCast() const;
			//! returns protocol reply using session ID as description
			string getReply( const Url &, const RTSP::TSessionID & ) const throw();
			//! returns protocol reply with a given description for the session; internal description is used if none given
			string getReply( const Url &, const string & description = "") const throw();
			//!@{
			//! returns medium by index / track name
			const Medium::Base & getMedium( size_t ) const throw( RTSP::Exception::ManagedError );
			Medium::Base & getMedium( size_t ) throw( RTSP::Exception::ManagedError );
			//!@}
			//!@{
			//! returns media list
			ref_list< const Medium::Base > getMedia() const throw();
			ref_list< Medium::Base > getMedia() throw();
			//!@}

			//! get full path of source media container
			string getFilePath() const throw();
			//! get base name of source media container
			const string & getFileName() const throw();
			//! get default description (default media container base name)
			const string & getDescription() const throw();
			//! set different description
			void setDescription( const string & ) throw();

			//! insert another descriptor at specified time
			void insert( SDP::Container & other, double insertTime ) throw( KGD::Exception::OutOfBounds );
			//! append another descriptor to the end
			void append( SDP::Container & other ) throw();
			//! transfer media from the other container to this, overwriting any existent data
			void assign( SDP::Container & other ) throw();
			//! loop current description a number of times ( 0 = infinite )
			void loop( uint8_t = 0 ) throw();

			//! global parameter: directory where media containers lie
			static string BASE_DIR;
			//! global parameter: when true, aggregate track control is announced
			static bool AGGREGATE_CONTROL;
		};
	}
}

#endif
