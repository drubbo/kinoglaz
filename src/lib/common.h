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
 * File name: ./lib/common.h
 * First submitted: 2010-01-16
 * First submitter: Emiliano Leporati <emiliano.leporati@gmail.com>
 * Contributor(s) so far - 2010-07-30 :
 *     Emiliano Leporati <emiliano.leporati@gmail.com>
 *
 * Last changes :
 *     Kinoglaz birth: pervasive bug fixing, improvements and client support
 *     fixed some bug in Log and in insertMedia; comments
 *     log messages refactor; shared descriptor are optional now; spot insertion support
 *     safe strerror
 *     sdp debugged
 *
 **/


#ifndef __KGD_COMMON_H
#define __KGD_COMMON_H

extern "C"
{
#include <inttypes.h>
}

#include <utility>
#include <vector>
#include <string>
#include <sstream>
#include <bitset>


#include <boost/scoped_ptr.hpp>
#include <boost/thread/thread.hpp>
#include <boost/thread/xtime.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/recursive_mutex.hpp>
#include <boost/thread/condition.hpp>
#include <boost/thread/barrier.hpp>
#include <boost/bind.hpp>
#include <boost/foreach.hpp>
#include <boost/ptr_container/ptr_map.hpp>
#include <boost/ptr_container/ptr_list.hpp>
#include <boost/ptr_container/ptr_vector.hpp>

using namespace std;

namespace KGD
{
	//! boost thread
	typedef boost::scoped_ptr< boost::thread > Thread;
	//! boost condition
	typedef boost::condition Condition;
	//! boost barrier
	typedef boost::barrier Barrier;

	//! boost mutex
	typedef boost::mutex Mutex;
	//! boost mutex lock
	typedef boost::mutex::scoped_lock Lock;

	//! boost recursive mutex
	typedef boost::recursive_mutex RMutex;
	//! boost recursive lock
	typedef boost::recursive_mutex::scoped_lock RLock;

	//! network port type
	typedef ushort TPort;
	//! network port pair type
	typedef pair< TPort, TPort > TPortPair;

	//! synchronization source identifier type
	typedef uint32_t TSSrc;

	//! thread with term barrier
	template< class MutexType >
	class SyncThread
	: public MutexType
	{
		Thread _th;
		Barrier _term;
	public:
		SyncThread( unsigned int count )
		: _term( count )
		{
		}

		void wait() { _term.wait(); }
		boost::thread * operator->() { return _th.operator->(); }
		boost::thread const * operator->() const { return _th.operator->(); }
		boost::thread & operator*() { return *_th; }
		boost::thread const & operator*() const { return *_th; }
		operator bool() const { return bool( _th ); }
		operator MutexType &() { return *this; }
		operator MutexType const &() const { return *this; }
		void unset() { _th.reset(); }
		void set( boost::thread * t ) { _th.reset( t ); }
	};
	
	//! common string -> string map
	typedef map< string, string > KeyValueMap;
	//! common string -> string pair
	typedef pair< string, string > KeyValuePair;

	
	//! search string begin operator
	class BeginsWith :
		public unary_function< string, bool >
	{
	protected:
		string _bg;
	public:
		BeginsWith( const string & );
		bool operator()( const string & ) const throw();
	};

	//! pid zero
	const pid_t PID_Z = pid_t( 0 );
	//! pid non-zero
	const pid_t PID_NZ = pid_t( -1 );

	//! returns sign of a double (-1 | 0 | +1)
	int sign(double v) throw();

	//! returns the "max with sign" between a and b, i.e., if sign is < 0, returns the min of them (the one farthest from 0)
	double signedMax( double a, double b, int sign );
	//! returns the "min with sign" between a and b, i.e., if sign is < 0, returns the max of them (the one closer to 0)
	double signedMin( double a, double b, int sign );

	//! tells if file exists
	bool fileExists ( const char* fpath ) throw();

	//! any type to string
	template < typename T > string toString ( T value ) throw();

	//! from string to any type
	template < typename T > T fromString ( const string &s ) throw();

	//! returns the chunks of the string chopped with separator
	vector< string > split ( const string & sep, const string &s ) throw();

	//! splits the string in two parts with the first occurrence of separator; if separator is not found, sencod of pair will be an empty string
	pair< string, string > split2 ( const string &sep, const string &s ) throw();

	//! joins the strings in the container with glue
	template< class Container >
	string join ( const string &glue, const Container & ss ) throw();

	//! trims both sides of string from specified char
	string trim( const string &, char = ' ');

	//! retuns safe strerror_r value
	string cStrError( int e );

	//! creates ansi string like "XXXXXXXX-XXXX-XXXX-XXXX-XXXXXXXXXXXX"
	string newUUID();

	// **************************************************************************************************************************
	
	template < typename T > string toString ( T value ) throw()
	{
		stringstream s;
		s << value;
		return s.str();
	}

	template < typename T > T fromString ( const string &s ) throw()
	{
		istringstream ss ( s );
		T value;
		ss >> value;
		return value;
	}


	template< class Container >
	string join ( const string &glue, const Container & ss ) throw()
	{
		ostringstream result;
		typename Container::const_iterator it = ss.begin();
		while ( it != ss.end() )
		{
			result << * ( it ++ );
			if ( it != ss.end() )
				result << glue;
		}
		return result.str();
	}

}

#endif
