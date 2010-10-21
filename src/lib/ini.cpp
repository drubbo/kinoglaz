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
 * File name: ./lib/ini.cpp
 * First submitted: 2010-01-16
 * First submitter: Emiliano Leporati <emiliano.leporati@gmail.com>
 * Contributor(s) so far - 2010-07-30 :
 *     Emiliano Leporati <emiliano.leporati@gmail.com>
 *
 * Last changes :
 *     Kinoglaz birth: pervasive bug fixing, improvements and client support
 *     frame / other stuff refactor
 *     sdp debugged
 *     interleave ok
 *     added licence disclaimer
 *
 **/


#include <iostream>
#include <fstream>

#include <string>
#include <boost/regex.hpp>

#include "lib/ini.h"

namespace KGD
{

	Ini::Reference Ini::getInstance() throw( Exception::InvalidState )
	{
		Instance::Lock lk( _instance );
		if ( *_instance )
			return newInstanceRef();
		else
			throw Exception::InvalidState( "ini instance not initialized" );
	}

	Ini::Reference Ini::getInstance( const string & fileName ) throw( Exception::InvalidState, Exception::NotFound )
	{
		Instance::Lock lk( _instance );
		if ( *_instance )
		{
			if ( (*_instance)->_fileName != fileName )
				throw Exception::InvalidState( "ini instance already initialized with file name " + (*_instance)->_fileName );
			else
				return newInstanceRef();
		}
		else
		{
			(*_instance).reset( new Ini( fileName ) );
			return newInstanceRef();
		}
	}

	Ini::Ini(const string & fileName ) throw( Exception::NotFound )
	: _fileName( fileName )
	{
		this->reload();
	}

	void Ini::reload() throw( Exception::NotFound )
	{
		// checked file open
		ifstream f;
		f.exceptions( ios_base::goodbit );
		f.open(_fileName.c_str(), ifstream::in);
		if ( ! f.good() )
			throw Exception::NotFound( _fileName );

		// current section during file scan
		string currentSection;
		// search regular expressions
		boost::regex rxSection("^\\[(.+)\\]");
		boost::regex rxParams("^([^;].+?)=(.+)");
		boost::match_results< string::const_iterator > match;
		// for every line
		char line[2048];
		while( f.good() && !f.eof() )
		{
			f.getline( line, 2048 );
			// setup rx iterators
			string data( line );
			string::const_iterator bg = data.begin(), ed = data.end();
			// if new section, add
			if (boost::regex_search(bg, ed, match, rxSection))
			{
				currentSection = match.str(1);
				_sections[ currentSection ] = Entries( currentSection );
			}
			// if params, add to current section
			else if (boost::regex_search(bg, ed, match, rxParams))
			{
				_sections[ currentSection ][ match.str(1) ] = match.str(2);
			}
		}
		// close file
		f.close();
	}


	const string& Ini::getFileName() const throw()
	{
		return _fileName;
	}

	const Ini::Entries & Ini::operator[]( const string & section ) const throw( Exception::NotFound )
	{
		SectionsMap::const_iterator it = _sections.find( section );
		if ( it == _sections.end() )
			throw KGD::Exception::NotFound( _fileName + "::[" + section + "]" );
		else
			return it->second;
	}

	string Ini::operator()( const string & section, const string & key ) const throw( Exception::NotFound )
	{
		SectionsMap::const_iterator it = _sections.find( section );
		if ( it == _sections.end() )
			throw KGD::Exception::NotFound( _fileName + "::[" + section + "]" );
		else
			return it->second[ key ];
	}

	string Ini::operator()( const string & section, const string & key, const string & defaultValue ) const throw( )
	{
		SectionsMap::const_iterator it = _sections.find( section );
		if ( it == _sections.end() )
			return defaultValue;
		else
			return it->second( key, defaultValue );
	}



	string & Ini::Entries::operator[]( const string & key ) throw( )
	{
		return _entries[ key ];
	}

	const string & Ini::Entries::operator[]( const string & key ) const throw( Exception::NotFound )
	{
		KeyValueMap::const_iterator it = _entries.find( key );
		if ( it == _entries.end() )
			throw Exception::NotFound( "[" + _section + "]" + "::" + key );
		else
			return it->second;
	}

	string Ini::Entries::operator()( const string & key, const string & defaultValue ) const throw( )
	{
		KeyValueMap::const_iterator it = _entries.find( key );
		if ( it == _entries.end() )
			return defaultValue;
		else
			return it->second;
	}

	Ini::Entries::Entries()
	{
	}

	Ini::Entries::Entries( const string & section )
	: _section( section )
	{
	}

}
