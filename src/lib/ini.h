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
 * File name: ./lib/ini.h
 * First submitted: 2010-01-16
 * First submitter: Emiliano Leporati <emiliano.leporati@gmail.com>
 * Contributor(s) so far - 2010-07-30 :
 *     Emiliano Leporati <emiliano.leporati@gmail.com>
 *
 * Last changes :
 *     Kinoglaz birth: pervasive bug fixing, improvements and client support
 *     fixed some bug in Log and in insertMedia; comments
 *     frame / other stuff refactor
 *     sdp debugged
 *     interleave ok
 *
 **/


#ifndef __KGD_INI_H
#define __KGD_INI_H

#include <map>

#include "lib/exceptions.h"
#include "lib/utils/singleton.hpp"

namespace KGD
{
	//! .ini file parser
	class Ini
	: public Singleton::Class< Ini >
	{
	public:
		//! The set of entries under a section
		class Entries
		{
		private:
			KeyValueMap _entries;
			string _section;
		public:
			Entries();
			Entries( const string & section );
			//! get existing value
			string & operator[]( const string & key ) throw();
			//! get existing value
			const string & operator[]( const string & key ) const throw( Exception::NotFound );
			//! get value, or default if missing
			string operator()( const string & key, const string & defaultValue ) const throw();
		};

	public:
		//! reloads the data
		void reload() throw( Exception::NotFound );
		//! returns file name
		const string& getFileName() const throw();
		//! get existing value
		string operator()( const string & section, const string & key ) const throw( Exception::NotFound );
		//! get value, or default if missing
		string operator()( const string & section, const string & key, const string & defaultValue ) const throw( );
		//! get existing section entries
		const Entries & operator[]( const string & section ) const throw( Exception::NotFound );

		//! get already existent instance
		static Reference getInstance() throw( Exception::InvalidState );
		//! get new instance of Ini parsing fileName
		static Reference getInstance( const string & fileName ) throw( Exception::InvalidState, Exception::NotFound );
	protected:
		typedef map< string, Entries > SectionsMap;
		//! section / value map
		SectionsMap _sections;
		//! ini file name
		string _fileName;

		//! ctor
		Ini( const string & fileName ) throw(Exception::NotFound);
		friend class Singleton::Class< Ini >;
	};
}

#endif
