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
 * File name: ./lib/buffer.h
 * First submitted: 2010-01-16
 * First submitter: Emiliano Leporati <emiliano.leporati@gmail.com>
 * Contributor(s) so far - 2010-07-30 :
 *     Emiliano Leporati <emiliano.leporati@gmail.com>
 *
 * Last changes :
 *     Kinoglaz birth: pervasive bug fixing, improvements and client support
 *     frame / other stuff refactor
 *     sdp debugged
 *     interleave ok
 *     added licence disclaimer
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
	{
	protected:
		//! data as char
		char *_data;
		//! significant data
		uint16_t _len;
		//! allocated size
		uint16_t _size;
	public:
		Buffer();
		virtual ~Buffer();
		//! returns a pointer to begin of data
		const char * getDataBegin() const throw();
		//! returns actual buffer length
		uint16_t getDataLength() const throw();
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
