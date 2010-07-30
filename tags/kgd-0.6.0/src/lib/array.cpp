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
 * File name: ./lib/array.cpp
 * First submitted: 2010-05-12
 * First submitter: Emiliano Leporati <emiliano.leporati@gmail.com>
 * Contributor(s) so far - 2010-07-30 :
 *     Emiliano Leporati <emiliano.leporati@gmail.com>
 *
 * Last changes :
 *     
 *
 **/

#include "lib/array.hpp"

namespace KGD
{
	//! encode 3 8-bit binary bytes as 4 '6-bit' characters
	void Base64::encodeBlock( const unsigned char in[3], unsigned char out[4], size_t len ) throw()
	{
		static const unsigned char cb64[]="ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

		if ( len > 0 )
		{
			out[0] = cb64[ in[0] >> 2 ];
			out[1] = cb64[ ((in[0] & 0x03) << 4) | ((in[1] & 0xf0) >> 4) ];
			out[2] = (len > 1 ? cb64[ ((in[1] & 0x0f) << 2) | ((in[2] & 0xc0) >> 6) ] : '=');
			out[3] = (len > 2 ? cb64[ in[2] & 0x3f ] : '=');
		}
		else
			memset( out, 0, 4 );
	}

	//! decode 4 '6-bit' characters into 3 8-bit binary bytes
	void Base64::decodeBlock( const unsigned char in[4], unsigned char out[3] ) throw()
	{
		out[ 0 ] = (in[0] << 2 | in[1] >> 4);
		out[ 1 ] = (in[1] << 4 | in[2] >> 2);
		out[ 2 ] = (((in[2] << 6) & 0xc0) | in[3]);
	}
}
