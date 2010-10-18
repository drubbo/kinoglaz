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
 * File name: ./main.cpp
 * First submitted: 2010-01-16
 * First submitter: Emiliano Leporati <emiliano.leporati@gmail.com>
 * Contributor(s) so far - 2010-07-30 :
 *     Emiliano Leporati <emiliano.leporati@gmail.com>
 *
 * Last changes :
 *     Kinoglaz birth: pervasive bug fixing, improvements and client support
 *     log messages refactor; shared descriptor are optional now; spot insertion support
 *     frame / other stuff refactor
 *     sdp debugged
 *     daemon out of RTSP ns
 *
 **/


#include "lib/exceptions.h"
#include "lib/array.hpp"
#include "lib/utils/singleton.hpp"
#include "daemon.h"

#include <iostream>
#include <algorithm>
#include <boost/regex.hpp>

using namespace std;
using namespace KGD;

KeyValueMap options;
list< string > switches;

string optGet( const string & opt ) throw( KGD::Exception::NotFound )
{
	KeyValueMap::const_iterator it = options.find( opt ) ;
	if ( it != options.end() )
		return it->second;
	else
		throw Exception::NotFound( "Option " + opt );
}

bool switchIsSet( const string & sw )
{
	return ( find( switches.begin(), switches.end(), sw ) != switches.end() );
}

int main(int argc, char** argv)
{
	if ( argc <= 1 )
	{
		cerr << "Usage:\n\t kgd -v|--version\n\t kgd -c config-file [-d]" << endl << endl;
		return EXIT_FAILURE;
	}
	
	for(int i = 1; i < argc; ++i )
	{
		string opt = argv[i];
		if (opt == "-v" || opt == "--version")
		{
			switches.push_back( "version" );
		}
		else if ( opt == "-d" )
		{
			switches.push_back( opt );
		}
		else if ( opt == "-c" )
		{
			if ( i + 1 < argc )
				options.insert( make_pair( opt, argv[ ++i ] ) );
			else
				cerr << "Bad option " << opt << endl;
		}
		else
		{
			cerr << "Unknown option " << opt << endl;
		}
	}


	if ( switchIsSet( "version" ) )
	{
		cout << KGD::Daemon::getName() << endl << endl;
		return EXIT_SUCCESS;
	}
	else
	{
		try
		{
			KGD::Daemon::Reference d = KGD::Daemon::getInstance( optGet( "-c" ) );
			bool runResult = d->start( switchIsSet( "-d" ) );
			return ( runResult ? EXIT_SUCCESS : EXIT_FAILURE );
		}
		catch( KGD::Exception::Generic const &e )
		{
			cerr << e.what() << endl << endl;
			return EXIT_FAILURE;
		}
	}
}
