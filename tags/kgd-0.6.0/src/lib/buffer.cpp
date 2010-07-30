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
 * File name: ./lib/buffer.cpp
 * First submitted: 2010-01-16
 * First submitter: Emiliano Leporati <emiliano.leporati@gmail.com>
 * Contributor(s) so far - 2010-07-30 :
 *     Emiliano Leporati <emiliano.leporati@gmail.com>
 *
 * Last changes :
 *     Kinoglaz birth: pervasive bug fixing, improvements and client support
 *     sdp debugged
 *     interleave ok
 *     pre-interleave
 *     added licence disclaimer
 *
 **/


#include "lib/log.h"
#include "lib/buffer.h"
#include <cstring>

using namespace std;

namespace KGD
{
	Buffer::Buffer()
	: _data( (char*)malloc( 4096 ) )
	, _len( 0 )
	, _size( 4096 )
	{
	}

	Buffer::~Buffer()
	{
		free( _data );
	}

	const char * Buffer::getDataBegin() const throw()
	{
		return _data;
	}

	uint16_t Buffer::getDataLength() const throw()
	{
		return _len;
	}

	void Buffer::enqueue( const void * s, uint16_t len ) throw( KGD::Exception::Generic )
	{
		if ( _len + len > _size )
		{
			_size += 1024;
			_data = (char*)realloc( _data, _size + 1 );
			if ( _data == 0 )
				throw KGD::Exception::Generic( "memory allocation error" );
		}

		memcpy(&_data[_len], s, len);
		_len += len;
		// ensure string termination for later uses
		_data[ _len ] = 0;
	}

	void Buffer::enqueue( const string &s ) throw( KGD::Exception::Generic )
	{
		this->enqueue( s.c_str(), s.size() );
	}

	void Buffer::enqueue( const ByteArray &s ) throw( KGD::Exception::Generic )
	{
		this->enqueue( s.get(), s.size() );
	}

	void Buffer::dequeue( uint16_t len ) throw( KGD::Exception::Generic )
	{
		if (len > 0 && _len >= len)
		{
			memmove( _data, _data + len, _size - len);
			_size -= len;
			_len -= len;
		}
		else if ( len > 0 )
			throw KGD::Exception::Generic( "buffer underrun " + toString( len ) + " on " + toString( _len ) );
	}
}
