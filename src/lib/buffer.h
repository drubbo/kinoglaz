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
 * File name: src/lib/buffer.h
 * First submitted: 2010-07-30
 * First submitter: Emiliano Leporati <emiliano.leporati@gmail.com>
 * Contributor(s) so far - 2010-11-04 :
 *     Emiliano Leporati <emiliano.leporati@gmail.com>
 *
 * Last changes :
 *     boosted
 *     source import
 *
 **/


#ifndef __KGD_BUFFER_H
#define __KGD_BUFFER_H

#include "lib/common.h"
#include "lib/exceptions.h"
#include "lib/array.hpp"

using namespace std;

namespace KGD
{
	//! generic buffer
	class Buffer
	: public boost::noncopyable
	{
	private:
		//! data as char
		vector< char > _data;
	public:
		//! build empty
		Buffer();
		//! destroy
		virtual ~Buffer();
		//! returns a pointer to begin of data
		const char * getDataBegin( size_t = 0 ) const throw( KGD::Exception::OutOfBounds );
		//! returns actual buffer length
		size_t getDataLength() const throw();
		//! enqueues data
		void enqueue( const void * s, uint16_t len ) throw( KGD::Exception::Generic );
		//! enqueues data
		void enqueue( const ByteArray & ) throw( KGD::Exception::Generic );
		//! enqueues data
		void enqueue( const string & s ) throw( KGD::Exception::Generic );
		//! dequeues data
		void dequeue( uint16_t len ) throw( KGD::Exception::Generic );
	};
}


#endif
