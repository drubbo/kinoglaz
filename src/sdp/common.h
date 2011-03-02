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
 * File name: src/sdp/common.h
 * First submitted: 2010-07-30
 * First submitter: Emiliano Leporati <emiliano.leporati@gmail.com>
 * Contributor(s) so far - 2010-11-04 :
 *     Emiliano Leporati <emiliano.leporati@gmail.com>
 *
 * Last changes :
 *     boosted
 *     Added AAC support
 *     Some cosmetics about enums and RTCP library
 *     source import
 *
 **/



#ifndef __KGD_SDP_COMMON_H
#define __KGD_SDP_COMMON_H

#include "lib/exceptions.h"
#include "lib/common.h"

#include <string>

#ifdef HAVE_CONFIG_H
#include "config.h"
#else
#error "Please use platform configuration file."
#endif

using namespace std;

namespace KGD
{
	//! Media container descriptors
	namespace SDP
	{
		//! SDP specific exceptions
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

		//! supported payload types
		namespace Payload
		{
			enum type
			{
				AudioMPA = 14,
				AudioMP3 = 97,
				VideoMPEG4 = 96,
				AudioAAC = 98
			};
		}

		//! supported media types
		namespace MediaType
		{
			enum kind
			{
				Audio = 'A',
				Video = 'V',
				Application = 'a'
			};
		}
	}
}

#endif
