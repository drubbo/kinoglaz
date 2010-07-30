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
 * File name: ./lib/pls.h
 * First submitted: 2010-05-12
 * First submitter: Emiliano Leporati <emiliano.leporati@gmail.com>
 * Contributor(s) so far - 2010-07-30 :
 *     Emiliano Leporati <emiliano.leporati@gmail.com>
 *
 * Last changes :
 *     
 *
 **/


#ifndef __KGD_PLS_H
#define __KGD_PLS_H

#include <list>

#include "lib/exceptions.h"

namespace KGD
{
	//! .kls file parser
	class PlayList
	{
	public:
		//! ctor
		PlayList( const string & fileName ) throw(Exception::NotFound);
		//! reloads the data
		void reload() throw( Exception::NotFound );
		//! returns file name
		const string& getFileName() const throw();
		//! get number of loops
		const ushort & getLoops() const throw();
		//! get ordered list of media containers to play
		const list< string > & getMediaList() const throw();
	protected:
		//! numer of loops; 0 means infinte
		ushort _loops;
		//! media containers
		list< string > _media;
		//! ini file name
		string _fileName;
	};
}

#endif
