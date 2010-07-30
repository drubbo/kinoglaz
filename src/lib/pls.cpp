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
 * File name: ./lib/pls.cpp
 * First submitted: 2010-05-12
 * First submitter: Emiliano Leporati <emiliano.leporati@gmail.com>
 * Contributor(s) so far - 2010-07-30 :
 *     Emiliano Leporati <emiliano.leporati@gmail.com>
 *
 * Last changes :
 *     
 *
 **/


#include <fstream>

#include <boost/regex.hpp>

#include "lib/pls.h"
#include "lib/log.h"

namespace KGD
{

	PlayList::PlayList( const string & fileName ) throw( Exception::NotFound )
		: _loops( 0 )
		, _fileName( fileName )
	{
		this->reload();
	}

	void PlayList::reload() throw( Exception::NotFound )
	{
		// checked file open
		ifstream f;
		f.exceptions( ios_base::goodbit );
		f.open(_fileName.c_str(), ifstream::in);
		if ( ! f.good() )
			throw Exception::NotFound( _fileName );

		// search regular expressions
		boost::regex rxHeader1("^loop\\s*$");
		boost::regex rxHeader2("^loop\\s+(\\d{1,3})\\s*$");
		boost::regex rxFile("^([^\\s]+)\\s*$");
		boost::match_results< string::const_iterator > match;
		// for every line

		char line[2048];
		size_t lineNo = 0;
		while( f.good() && !f.eof() )
		{
			f.getline( line, 2048 );
			// setup rx iterators
			string data( line );
			string::const_iterator bg = data.begin(), ed = data.end();
			// check infinite loop
			if (boost::regex_search(bg, ed, match, rxHeader1))
			{
				if ( lineNo > 0 )
					Log::warning( "%s: invalid loop declaration at line %u", _fileName.c_str(), lineNo );
				else
					_loops = 0;
			}
			// check finite loop
			else if (boost::regex_search(bg, ed, match, rxHeader2))
			{
				if ( lineNo > 0 )
					Log::warning( "%s: invalid loop declaration at line %u", _fileName.c_str(), lineNo );
				else
					_loops = fromString< ushort >( match.str(1) );
			}
			else if (boost::regex_search(bg, ed, match, rxFile))
				_media.push_back( match.str(1) );
		}
		// close file
		f.close();
	}

	const string& PlayList::getFileName() const throw()
	{
		return _fileName;
	}

	const ushort & PlayList::getLoops() const throw()
	{
		return _loops;
	}

	const list< string > & PlayList::getMediaList() const throw()
	{
		return _media;
	}
}
