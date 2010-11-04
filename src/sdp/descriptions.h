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
 * File name: src/sdp/descriptions.h
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



#ifndef __KGD_SDP_DESCRIPTIONS_H
#define __KGD_SDP_DESCRIPTIONS_H

#include "lib/common.h"
#include "lib/utils/singleton.hpp"
#include "sdp/session.h"

#include <string>
#include <map>

using namespace std;

namespace KGD
{
	namespace SDP
	{
		//! session description cache
		class Descriptions
		: public Singleton::Class< Descriptions >
		{
		private:
			typedef boost::ptr_map< string, Container > ContainerMap;
			//! loaded media container descriptors
			ContainerMap _descriptions;
			//! reference count for every described media
			map< string, int64_t > _count;
			//! ctor
			Descriptions();
			friend class Singleton::Class< Descriptions >;
		public:
			~Descriptions();
			//! returns description of file or creates if needed
			SDP::Container & loadDescription( const string & ) throw( SDP::Exception::Generic );
			//! returns description of file if exists
			SDP::Container & getDescription( const string & ) throw( KGD::Exception::NotFound );
			//! returns description of file if exists
			void releaseDescription( const string & ) throw( );

		};
	}
}

#endif
