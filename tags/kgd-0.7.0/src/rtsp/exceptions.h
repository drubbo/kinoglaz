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
 * File name: src/rtsp/exceptions.h
 * First submitted: 2010-07-30
 * First submitter: Emiliano Leporati <emiliano.leporati@gmail.com>
 * Contributor(s) so far - 2010-11-04 :
 *     Emiliano Leporati <emiliano.leporati@gmail.com>
 *
 * Last changes :
 *     source import
 *
 **/


#ifndef __KGD_RTSP_EXCEPTIONS_H
#define __KGD_RTSP_EXCEPTIONS_H

#include "lib/exceptions.h"
#include "rtsp/common.h"
#include "rtsp/error.h"

#include <string>

using namespace std;
using namespace KGD::RTSP;

namespace KGD
{
	namespace RTSP
	{
		namespace Exception
		{
			//! Generic RTSP exception
			class Generic :
				public KGD::Exception::Generic
			{
			public:
				//! error from a function with system message
				Generic( const string & fun );
				//! error from a function with user message
				Generic( const string & fun, const string & msg );
			};

			//! protocol-managed RTSP error
			class ManagedError :
				public Generic
			{
			private:
				//! Reference to error definition
				const Error::Definition & _definition;
			public:
				//! ctor from error code
				ManagedError( const uint16_t code ) throw( KGD::Exception::NotFound );
				//! ctor from explicit error definition
				ManagedError( const Error::Definition & ) throw();
				//! return reference to error definition
				const Error::Definition & getError() const throw();
			};

			//! RTSP cseq out of sequence
			class CSeq :
				public Generic
			{
			public:
				CSeq();
			};

		}
	}
}

#endif
