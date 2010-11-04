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
 * File name: src/rtsp/buffer.cpp
 * First submitted: 2010-07-30
 * First submitter: Emiliano Leporati <emiliano.leporati@gmail.com>
 * Contributor(s) so far - 2010-11-04 :
 *     Emiliano Leporati <emiliano.leporati@gmail.com>
 *
 * Last changes :
 *     fixed RTSP buffer enqueue
 *     boosted
 *     source import
 *
 **/


#include "rtsp/buffer.h"
#include "rtsp/common.h"
#include "rtsp/header.h"
#include "rtsp/exceptions.h"
#include "lib/log.h"
#include "lib/urlencode.h"
#include <cstring>
#include <cstdlib>
#include <cstdio>

#include <boost/regex.hpp>

using namespace std;

namespace KGD
{
	namespace RTSP
	{
		InputBuffer::InputBuffer()
		: KGD::Buffer()
		{
		}


		size_t InputBuffer::getContentLength( ) const throw( )
		{
			boost::regex rxSearch( EOL + Header::ContentLength + ": (\\d+)" + EOL );
			boost::match_results< string::const_iterator > match;
			const string data( this->getDataBegin() );
			if (boost::regex_search( data.begin() , data.end(), match, rxSearch))
				return fromString< size_t >( match.str(1) );
			else
				return 0;
		}

		size_t InputBuffer::getNextPacketLength() const throw( )
		{
			size_t sz = 0;
			size_t len = this->getDataLength();
			const char * data = this->getDataBegin();
			// interleave
			// there is always at least one character in the buffer (0)
			if ( data[0] == '$' )
			{
				// $cZS
				if ( len >= 4 )
				{
					uint16_t payload = 0;
					memcpy( &payload, &data[2], 2 );
					sz = 4u + ntohs( payload );
				}
			}
			// RTSP message
			else if ( (sz = string( data ).find( EOL + EOL )) != string::npos )
			{
				sz += 2 * EOL.size() + this->getContentLength( );
			}

			// incomplete
			if ( sz > len )
				return 0;
			else
				return sz;
		}

		// ***********************************************************************************************************************************

		auto_ptr< Message::Request > InputBuffer::getNextRequest( size_t pktLen, const string & remoteHost ) throw( RTSP::Exception::ManagedError, KGD::Exception::NotFound )
		{
			if ( pktLen )
			{
				auto_ptr< Message::Request > rt( new Message::Request( this->getDataBegin(), pktLen, remoteHost ) );
				this->dequeue( pktLen );
				return rt;
			}
			else
				throw KGD::Exception::NotFound( "request" );
		}

		auto_ptr< Message::Response > InputBuffer::getNextResponse( size_t pktLen ) throw( RTSP::Exception::CSeq, KGD::Exception::NotFound )
		{
			if ( pktLen )
			{
				auto_ptr< Message::Response > rt( new Message::Response( this->getDataBegin(), pktLen ) );
				this->dequeue( pktLen );
				return rt;
			}
			else
				throw KGD::Exception::NotFound( "response" );
		}

		pair< TPort, boost::shared_ptr< RTP::Packet > > InputBuffer::getNextInterleave( size_t pktLen ) throw( KGD::Exception::NotFound )
		{
			const char * data = this->getDataBegin();
			if ( pktLen && data[0] == '$')
			{
				BOOST_ASSERT( pktLen >= 4 );
				pair< TPort, boost::shared_ptr< RTP::Packet > > rt;
				rt.first = data[1];
// 				Log::verbose("RTSP: Got interleaved on channel %d", chan);
				rt.second.reset( new RTP::Packet( &data[4], pktLen - 4) );
				this->dequeue( pktLen );
				return rt;
			}
			else
				throw KGD::Exception::NotFound( "interleave packet" );			
		}
	}
}
