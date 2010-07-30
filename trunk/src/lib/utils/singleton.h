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
#include "lib/utils/pointer.hpp"

using namespace std;

namespace KGD
{
	//! Singleton pattern tools
	namespace Singleton
	{
		template< class T > class Class;

		//! this class represents a reference to a singleton instance of class T; the destruction of last reference causes instance destruction
		template< class T > class InstanceRef
		{
		private:
			//! thread safer
			static Mutex _mux;
			//! counter of references to T* instance
			static long _count;

		public:
			//! constructs an instance reference
			InstanceRef( );

			//! everybody can copy the reference
			InstanceRef( const InstanceRef< T >& );

			//! deletes cascade the instance if last reference
			~InstanceRef( );

			T * operator->();
			const T * operator->() const;

			T & operator*();
			const T & operator*() const;

			//! let Class< T > access private members
			friend class Class< T >;
		};

		//! singleton base class
		template< class T > class Class
		{
		private:
			//! static thread-safe delete
			static void destroyInstance();

			//! the class implementing singleton pattern must not be copyable
			Class( const Class< T > & );

		protected:
			//! thread safer
			static Mutex _mux;
			//! the instance pointer
			static T* _instance;

			//! just derived classes can create a new instance
			Class();
			//! just derived classes can get new instance references
			static InstanceRef< T > newInstanceRef();

		public:

			//! basic instance retrieve, using default T constructor
			static InstanceRef< T > getInstance();

			//! reference should access private members
			friend class InstanceRef< T >;
		};

		//! a class implementing persistent pattern will be instantiated at first request and never destroyied
		template< class T > class Persistent
		{
		private:
			//! the class implementing singleton persistent pattern must not be copyable
			Persistent( const Persistent< T > & );
			//! the class implementing singleton persistent pattern must not be copyable
			Persistent< T >& operator=( const Persistent< T > & );

		protected:
			//! thread safer
			static Mutex _mux;
			//! the instance pointer
			static Ptr::Scoped< T > _instance;

			//! just derived classes can create a new instance
			Persistent();

		public:
			//! basic instance initializer and retriever
			static T& getInstance();
			//! static thread-safe delete
			static void destroyInstance();
		};
	}
}

#endif
