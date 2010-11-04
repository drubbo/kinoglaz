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
	namespace
	{
		const size_t BLOCK_SIZE = 4096;
	}
	
	Buffer::Buffer()
	{
		_data.reserve( BLOCK_SIZE );
		// ensure string termination for later uses
		_data.push_back( 0 );
	}

	Buffer::~Buffer()
	{
	}

	const char * Buffer::getDataBegin( size_t pos ) const throw( KGD::Exception::OutOfBounds )
	{
		if ( pos > _data.size() - 2 )
			throw KGD::Exception::OutOfBounds( pos, 0, _data.size() - 2 );
		return &_data[pos];
	}

	size_t Buffer::getDataLength() const throw()
	{
		return _data.size() - 1;
	}

	void Buffer::enqueue( const void * s, uint16_t len ) throw( KGD::Exception::Generic )
	{
		while( _data.size() + len > _data.capacity() )
			_data.reserve( _data.capacity() + BLOCK_SIZE );

		const char * c = reinterpret_cast< const char * >( s );		
		_data.insert( _data.end() - 1, c, c + len );
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
		if (len > 0 && _data.size() > len)
		{
			_data.erase( _data.begin(), _data.begin() + len );
		}
		else if ( len > 0 )
			throw KGD::Exception::Generic( "buffer underrun " + toString( len ) + " on " + toString( _data.size() - 1 ) );
	}
}
