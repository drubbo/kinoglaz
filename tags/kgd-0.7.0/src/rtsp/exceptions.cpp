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
 * File name: src/rtsp/exceptions.cpp
 * First submitted: 2010-07-30
 * First submitter: Emiliano Leporati <emiliano.leporati@gmail.com>
 * Contributor(s) so far - 2010-11-04 :
 *     Emiliano Leporati <emiliano.leporati@gmail.com>
 *
 * Last changes :
 *     source import
 *
 **/


#include "rtsp/exceptions.h"

using namespace std;

namespace KGD
{
	namespace RTSP
	{
		namespace Exception
		{
			//! error from a function with system message
			Generic::Generic( const string & fun )
			: KGD::Exception::Generic( fun, errno )
			{
			}
			//! error from a function with user message
			Generic::Generic( const string & fun, const string & msg )
			: KGD::Exception::Generic( fun + ": " + msg )
			{
			}

			ManagedError::ManagedError( const uint16_t code ) throw( KGD::Exception::NotFound )
			: Generic( "RTSP", Error::Definition::getDefinition( code ).getDescription() ), _definition( Error::Definition::getDefinition( code ) )
			{
			}

			ManagedError::ManagedError( const Error::Definition & def ) throw()
			: Generic( "RTSP", def.getDescription() ), _definition( def )
			{
			}

			const Error::Definition & ManagedError::getError() const throw()
			{
				return _definition;
			}

			CSeq::CSeq()
			: Generic( "RTSP", "invalid cseq")
			{
			}
		}
	}
}
