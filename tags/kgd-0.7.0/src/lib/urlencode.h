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
 * File name: src/lib/urlencode.h
 * First submitted: 2010-07-30
 * First submitter: Emiliano Leporati <emiliano.leporati@gmail.com>
 * Contributor(s) so far - 2010-11-04 :
 *     Emiliano Leporati <emiliano.leporati@gmail.com>
 *
 * Last changes :
 *     source import
 *
 **/


#ifndef __KGD_URLENCODE_H
#define __KGD_URLENCODE_H

#include "lib/common.h"
#include <string>

using namespace std;

namespace KGD
{
	//! URL decomposition
	struct Url
	{
		//! remote host
		string remoteHost;
		//! target (server) host
		string host;
		//! target file
		string file;
		//! target track
		string track;

		//! target port
		TPort port;

		//! ctor
		Url();
		//! re-compose
		string toString() const;

		//! encode an URL as of RFC 1738
		static string encode(const string &);
		//! decode an URL as of RFC 1738
		static string decode(const string &);
	};

}


#endif
