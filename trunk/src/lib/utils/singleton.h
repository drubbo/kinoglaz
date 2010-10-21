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
 * File name: ./lib/utils/singleton.h
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


#ifndef __KGD_UTILS_SINGLETON_H
#define __KGD_UTILS_SINGLETON_H

#include "lib/common.h"
#include "lib/utils/safe.hpp"

using namespace std;

namespace KGD
{
	//! Singleton pattern tools
	namespace Singleton
	{

		//! singleton base class
		template< class T >
		class Class
		: public boost::noncopyable
		{
		public:
			typedef boost::shared_ptr< T > Reference;
			typedef KGD::RMutex Mutex;
			typedef KGD::RLock Lock;
		private:
			//! the class implementing singleton pattern must not be copyable
			Class( const Class< T > & );

		protected:
			typedef Safe::Lockable< Reference > Instance;
			//! the instance pointer
			static Instance _instance;

			//! just derived classes can create a new instance
			Class();
			//! just derived classes can get new instance references
			static Reference newInstanceRef();

		public:

			//! basic instance retrieve, using default T constructor
			static Reference getInstance();

			//! destroy last instance
			static void destroyInstance();
			
			//! lock static mux
			static void lock();
			//! unlock static mux
			static void unlock();
			//! get static mux
			static RMutex & mux();
		};
	}
}

#endif
