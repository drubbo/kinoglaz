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
 * File name: ./lib/utils/sharedptr.h
 * First submitted: 2010-01-16
 * First submitter: Emiliano Leporati <emiliano.leporati@gmail.com>
 * Contributor(s) so far - 2010-07-30 :
 *     Emiliano Leporati <emiliano.leporati@gmail.com>
 *
 * Last changes :
 *     Kinoglaz birth: pervasive bug fixing, improvements and client support
 *     sdp debugged
 *     sdp threaded loader. sync pause
 *     should not need interlocking on socket write (should)
 *     interleave ok
 *
 **/


#ifndef __KGD_UTILS_SHPOINTER_H
#define __KGD_UTILS_SHPOINTER_H

#include "lib/utils/pointer.hpp"
#include "lib/utils/safe.hpp"
#include "lib/exceptions.h"

#include <cstring>
#include <stdexcept>

using namespace std;

namespace KGD
{
	//! Pointer management utils
	namespace Ptr
	{
		//! a shared smart pointer: successive copies will increment reference count; destruction of last copy will destroy first delegated pointer
		template< class T >
		class Shared
		{
		protected:
			//! this class represents the actually shared pointer
			class Common
			{
			private:
				//! actually shared pointer
				T * _ptr;
				//! number of references to this common pointer
				Safe::Number< long > _refCount;
				//! copy prohibited
				Common( const Common & );
				//! copy prohibited
				Common& operator=( const Common & );
			public:
				mutable RMutex mux;
				//! ctor: delegates the pointer (NULL is prohibited)
				Common( T* ) throw( KGD::Exception::Generic );
				//! dtor: deletes the pointer
				~Common();
				//! increment reference count
				Common& operator++();
				//! decrement reference count
				Common& operator--();
				//! get reference count value
				operator long() const;
				//! get pointer
				T* get();
				//! get pointer
				const T* get() const;
			};

			//! the shared object between copies
			Common * _ptr;
		public:
			//! creates the first Common instance
			Shared( T * ) throw( KGD::Exception::Generic );
			//! copyies the Common instance incrementing reference count
			Shared( const Shared< T > & );
			//! decrements reference count of Common instance and destroyies it if last copy
			~Shared();

			//! get pointer
			T * get();
			//! get pointer
			const T * get() const;

			//! dereference
			T * operator->();
			//! dereference
			const T * operator->() const;
			//! dereference
			T & operator*();
			//! dereference
			const T & operator*() const;

			//! lock
			void lock() const;
			//! unlock
			void unlock() const;
		};
	}
}

#endif
