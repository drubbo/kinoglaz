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
 * File name: ./lib/utils/pointer.h
 * First submitted: 2010-01-16
 * First submitter: Emiliano Leporati <emiliano.leporati@gmail.com>
 * Contributor(s) so far - 2010-07-30 :
 *     Emiliano Leporati <emiliano.leporati@gmail.com>
 *
 * Last changes :
 *     Kinoglaz birth: pervasive bug fixing, improvements and client support
 *     fixed some bug in Log and in insertMedia; comments
 *     sdp debugged
 *     interleave ok
 *     pre-interleave
 *
 **/


#ifndef __KGD_UTILS_POINTER_H
#define __KGD_UTILS_POINTER_H

#include <lib/exceptions.h>
#include <cstring>
#include <stdexcept>

using namespace std;

namespace KGD
{
	//! Pointer management utils
	namespace Ptr
	{
		// ******************************************************************************************

		//! deletes pointed memory if any and sets ptr to 0
		template < class T > void clear ( T* & ptr );

		//! clones a ptr using its class copy ctor
		template < class T > T * clone ( T const * const ptr );

		// ******************************************************************************************

		//! a reference adaptor to safely return container of references, copy references, etc.
		template < class T > class Ref
		{
		private:
			//! the reference is managed here by its pointer, that won't be deallocated
			T * _ref;
		public:
			//! construct an invalid reference to NULL
			Ref();
			//! construct from a reference
			Ref( T & obj );
			//! construct from a Ref
			template< class S >
			Ref( Ref< S > & obj );
			//! tells if reference is to an object or to NULL
			bool isValid() const;
			//! puts reference to NULL
			void invalidate();
			//! change reference to another object
			virtual Ref< T > & operator=( T & obj );

			//! cast operator
			operator T &() throw( Exception::NullPointer );
			//! const cast operator
			operator const T &() const throw( Exception::NullPointer );

			//! cast alias
			T& get() throw( Exception::NullPointer );
			//! const cast alias
			const T& get() const throw( Exception::NullPointer );

			//! get hidden pointer
			T* getPtr() throw( Exception::NullPointer );
			//! get hidden pointer
			const T* getPtr() const throw( Exception::NullPointer );

			//! deref
			T* operator->() throw( Exception::NullPointer );
			//! const deref
			const T* operator->() const throw( Exception::NullPointer );

			//! deref
			T& operator*() throw( Exception::NullPointer );
			//! const deref
			const T& operator*() const throw( Exception::NullPointer );
		};

		//! converts a pointer to a reference
		template< class T > Ref< T > toRef( T * p );
		template< class T > Ref< const T > toConstRef( T const * p );

		// ******************************************************************************************

		//! A class implementing this interface is declaring a delegate-memory pointer behaviour: she will become responsible of his destruction after assignement
		template < class T > class Delegate
		{
		public:
			virtual Delegate& operator= ( T * ) = 0;
		};

		//! A class implementing this interface can be delegated of more than one pointer
		template < class T > class MultiDelegate
		{
		public:
			virtual MultiDelegate& operator+= ( T * ) = 0;
		};

		// ******************************************************************************************

		//! A delegate shell for a pointer with flexible capabilities; supports template-polymorphic deletion and cloning
		template < class T > class Shell :
				public Delegate< T >
		{
		protected:
			//! the encapsulated pointer
			T* _ptr;
			//! deletion flag
			bool _attached;

		public:
			//! void shell
			Shell( );
			//! owns the ptr
			Shell( T* p );
			//! destroy the ptd memory
			virtual ~Shell();

			//! delegate operator= implementation
			virtual Shell& operator= ( T * p );

			//! keeps ptr reference but stops managing its deallocation
			T * detach();

			//! the pointer is released in the outside world with this call, object is clean
			T * release();

			//! destroy pointed content, if attached
			void destroy();

			//! special dereference to let trusted routines change internal pointer (openssl)
			T * & get();
			//! special dereference to let trusted routines change internal pointer (openssl)
			const T * get() const;

			//! tells if ptr is != 0
			operator bool() const;
		};

		// ******************************************************************************************

		//! this class encapsulates a pointer living in a scope - ideal for temporary ptrs in routine code
		template< class T > class Scoped :
				public Shell< T >
		{
		public:
			//! void ctor
			Scoped( );
			//! delegate ctor, from a ptr
			Scoped( T * p );
			//! ctor, for byte arrays, initialized to 0
			explicit Scoped( const size_t sz );
			
			//! delegate assign
			virtual Scoped & operator=( T * p );

			//! dereference
			T * operator->() throw( Exception::NullPointer );
			//! dereference
			const T * operator->() const throw( Exception::NullPointer );
			//! dereference
			T & operator*() throw( Exception::NullPointer );
			//! dereference
			const T & operator*() const throw( Exception::NullPointer );
		};

		// ******************************************************************************************

		//! A clonable pointer: T must be copiable (via copy ctor)
		template< class T > class Copyable :
				public Scoped< T >
		{
		public:
			//! void ctor
			Copyable( );
			//! delegate ctor, from a ptr
			Copyable( T * p );
			//! copy ctor
			Copyable( const T & p );
			//! copy ctor
			Copyable( const Copyable< T > & p );
			//! ctor, for byte arrays, initialized to 0
			explicit Copyable( const size_t sz );


			//! delegate assign
			virtual Copyable & operator=( T * p );
			//! assignable implementation
			virtual Copyable & operator=( const T & p );
			//! assignable implementation
			virtual Copyable & operator=( const Copyable< T > & p );
		};
	}
}
#endif

