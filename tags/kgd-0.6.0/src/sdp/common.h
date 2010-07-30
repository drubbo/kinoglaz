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
 * File name: ./sdp/common.h
 * First submitted: 2010-02-20
 * First submitter: Emiliano Leporati <emiliano.leporati@gmail.com>
 * Contributor(s) so far - 2010-07-30 :
 *     Emiliano Leporati <emiliano.leporati@gmail.com>
 *
 * Last changes :
 *     Kinoglaz birth: pervasive bug fixing, improvements and client support
 *     wimtv key stream
 *     sig HUP supported
 *     frame / other stuff refactor
 *
 **/



#ifndef __KGD_SDP_COMMON_H
#define __KGD_SDP_COMMON_H

#include "lib/exceptions.h"
#include "lib/common.h"
#include "config.h"

#include <string>

using namespace std;

namespace KGD
{
	//! Media container descriptors
	namespace SDP
	{
		namespace Exception
		{
			//! SDP generic exception
			class Generic :
				public KGD::Exception::Generic
			{
			public:
				Generic( const string & );
			};
		}

		//! SDP END OF LINE
		const string EOL = "\r\n";
		//! SDP VERSION
		const string VER = "0";

		//! known payload types
		enum PayloadType
		{
			A_MPA = 14,
			A_MP3 = 97,
			V_MPEG4 = 96
		};

		//! known media types
		enum MediaType
		{
			AUDIO = 'A',
			VIDEO = 'V',
			APPLICATION = 'a'
		};
	}
}

#endif
