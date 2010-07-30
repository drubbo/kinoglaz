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
 * File name: ./lib/utils/factory.cpp
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
 *     termination, log, method execution refactor
 *
 **/


#include "lib/utils/factory.h"
#include <lib/log.h>

namespace KGD
{
	namespace Factory
	{
		map< string, void * > * GlobalMappings = 0;

		bool Mappings::isRegistryRegistered( const string & className ) throw( )
		{
			return ( GlobalMappings && GlobalMappings->find( className ) != GlobalMappings->end() );
		}


		void Mappings::registerRegistry( const string & className, void * mappings ) throw( Exception::InvalidState )
		{
			if ( ! isRegistryRegistered( className ) )
			{
				Log::debug( "registering factory mappings for tree %s",  className.c_str() );

				if ( !GlobalMappings )
				{
					Log::debug( "creating new global registry");
					GlobalMappings = new map< string, void * >();
				}

				(*GlobalMappings)[ className ] = mappings;
			}
			else
				throw Exception::InvalidState("already registered factory mappings for tree " + className );
		}

	}
}
