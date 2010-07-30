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
 * File name: ./lib/urlencode.cpp
 * First submitted: 2010-01-16
 * First submitter: Emiliano Leporati <emiliano.leporati@gmail.com>
 * Contributor(s) so far - 2010-07-30 :
 *     Emiliano Leporati <emiliano.leporati@gmail.com>
 *
 * Last changes :
 *     Kinoglaz birth: pervasive bug fixing, improvements and client support
 *     sdp debugged
 *     interleave ok
 *     added licence disclaimer
 *     first import
 *
 **/


#include "lib/urlencode.h"

extern "C"
{
#include <ctype.h>
#include <string.h>
#include <stdlib.h>
}

namespace KGD
{
	Url::Url()
	: port( 554 )
	{
	}
	
	/* Converts a hex character to its integer value */
	char fromHex( char ch )
	{
		return isdigit(ch) ? ch - '0' : tolower(ch) - 'a' + 10;
	}

	/* Converts an integer value to its hex character*/
	char toHex( char code )
	{
		static char hex[] = "0123456789abcdef";
		return hex[code & 15];
	}

	string Url::toString() const
	{
		string rt = "rtsp://" + host + ":" + KGD::toString( port ) + "/" + encode( file );
		if ( track.size() )
			rt += "/tk=" + track;
		return rt;
	}

	string Url::encode(const string & str)
	{
		const char *pstr = str.c_str();
		char *buf = (char*)malloc(str.size() * 3 + 1), *pbuf = buf;
		while (*pstr)
		{
			if (isalnum(*pstr) || *pstr == '-' || *pstr == '_' || *pstr == '.' || *pstr == '~')
				*pbuf++ = *pstr;
			else if (*pstr == ' ')
				*pbuf++ = '+';
			else
				*pbuf++ = '%', *pbuf++ = toHex(*pstr >> 4), *pbuf++ = toHex(*pstr & 15);
			pstr++;
		}
		*pbuf = '\0';

		string rt = buf;
		free( buf );
		return rt;
	}

	string Url::decode(const string & str)
	{
		const char *pstr = str.c_str();
		char *buf = (char*)malloc(str.size() + 1), *pbuf = buf;
		while (*pstr)
		{
			if (*pstr == '%')
			{
				if (pstr[1] && pstr[2])
				{
					*pbuf++ = fromHex(pstr[1]) << 4 | fromHex(pstr[2]);
					pstr += 2;
				}
			}
			else if (*pstr == '+')
			{
				*pbuf++ = ' ';
			}
			else
			{
				*pbuf++ = *pstr;
			}
			pstr++;
		}
		*pbuf = '\0';

		string rt = buf;
		free( buf );
		return rt;
	}
}