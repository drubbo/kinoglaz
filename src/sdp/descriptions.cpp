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
 * File name: ./sdp/descriptions.cpp
 * First submitted: 2010-02-20
 * First submitter: Emiliano Leporati <emiliano.leporati@gmail.com>
 * Contributor(s) so far - 2010-07-30 :
 *     Emiliano Leporati <emiliano.leporati@gmail.com>
 *
 * Last changes :
 *     Kinoglaz birth: pervasive bug fixing, improvements and client support
 *     fixed some bug in Log and in insertMedia; comments
 *     log messages refactor; shared descriptor are optional now; spot insertion support
 *
 **/



#include "sdp/descriptions.h"
#include "rtsp/common.h"
#include "daemon.h"
#include "lib/log.h"
#include "lib/utils/container.hpp"
#include "lib/utils/virtual.hpp"

#include <sstream>
#include <iomanip>
#include <iostream>

extern "C"
{
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
}

namespace KGD
{

	namespace SDP
	{
		Descriptions::Descriptions()
		{
		}

		Descriptions::~Descriptions()
		{
			_descriptions.clear();
		}

		SDP::Container & Descriptions::loadDescription( const string & file ) throw( SDP::Exception::Generic )
		{
			RLock lk( _mux );
			Log::debug("SDP Pool: loading description for %s", file.c_str() );
			if ( ! _descriptions.has( file ) )
			{
				Log::debug("SDP Pool: %s not existent", file.c_str() );
				Container * s = new SDP::Container( file );
				{
					_descriptions( file ) = s;
					_count( file ) = 1;
				}
			}
			else
			{
				++ _count( file );
				Log::debug("SDP Pool: %s exists, reference count to %llu", file.c_str(), _count[ file ] );
			}

			return *_descriptions( file );
		}

		SDP::Container & Descriptions::getDescription( const string & file ) throw( KGD::Exception::NotFound )
		{
			RLock lk( _mux );
			Log::debug("SDP Pool: getting description for %s", file.c_str() );
			if ( _descriptions.has( file ) )
				return *(_descriptions( file ) );
			else
				throw KGD::Exception::NotFound( "SDP description for " + file );
		}

		void Descriptions::releaseDescription( const string & file ) throw( )
		{
			RLock lk( _mux );
			if ( _descriptions.has( file ) )
			{
				Log::debug("SDP Pool: releasing description for %s with count %llu", file.c_str(), _count[ file ] );
				if ( ( -- _count( file ) ) <= 0 )
				{
					_descriptions.erase( file );
					_count.erase( file );
					Log::debug("SDP Pool: removed %s", file.c_str());
				}
			}
			else
				Log::debug( "SDP Pool: releasing unmanaged description for %s", file.c_str() );
		}
	}
}
