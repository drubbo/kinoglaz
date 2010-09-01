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
 * File name: ./sdp/frame.cpp
 * First submitted: 2010-03-09
 * First submitter: Emiliano Leporati <emiliano.leporati@gmail.com>
 * Contributor(s) so far - 2010-07-30 :
 *     Emiliano Leporati <emiliano.leporati@gmail.com>
 *
 * Last changes :
 *     implemented (unused) support for MP3 ADU
 *     Kinoglaz birth: pervasive bug fixing, improvements and client support
 *     fixed some bug in Log and in insertMedia; comments
 *     log messages refactor; shared descriptor are optional now; spot insertion support
 *     wimtv key stream
 *
 **/



#include "sdp/frame.h"
#include "lib/array.hpp"

namespace KGD
{
	namespace SDP
	{
		namespace Frame
		{			
			Base::Base( const AVPacket & pkt, double timebase )
			: _time( pkt.dts * timebase )
			, _displace( 0 )
			{
			}
				
			Base::Base( double t )
			: _time( t )
			, _displace( 0 )
			{
			}

			Base::Base( const Base & b )
			: _time( b._time )
			, _displace( 0 )
			, _pt( b._pt )
			{
			}

			void Base::addTime( double delta )
			{
				_time += delta;
			}

			void Base::setDisplace( double t ) const
			{
				_displace = t;
			}

			Base* Base::getClone() const
			{
				return new Base( *this );
			}

			double Base::getTime() const
			{
				return _time + _displace;
			}

			Payload::type Base::getPayloadType() const
			{
				return _pt;
			}

			void Base::setPayloadType( Payload::type p )
			{
				_pt = p;
			}

			// ***********************************************************************************************

			MediaFile::MediaFile( const AVPacket & pkt, double timebase )
			: Base( pkt, timebase )
			, _size( pkt.size )
			, _isKey( pkt.flags & PKT_FLAG_KEY )
			, _pos( pkt.pos )
			, data( pkt.data, pkt.size )
			{
			}

			MediaFile::MediaFile( const ByteArray & pkt, double t )
			: Base( t )
			, _size( pkt.size() )
			, _isKey( false )
			, _pos( 0 )
			, data( pkt )
			{
			}

			MediaFile::MediaFile( const MediaFile & m )
			: Base( m )
			, _size( m._size )
			, _isKey( m._isKey )
			, _pos( m._pos )
			, data( m.data )
			{
			}

			MediaFile* MediaFile::getClone() const
			{
				return new MediaFile( *this );
			}
			
			int MediaFile::getSize() const
			{
				return _size;
			}
			uint64_t MediaFile::getFilePos() const
			{
				return _pos;
			}
			bool MediaFile::isKey() const
			{
				return _isKey;
			}
		}
	}
}
