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
 * File name: ./lib/utils/container.h
 * First submitted: 2010-01-16
 * First submitter: Emiliano Leporati <emiliano.leporati@gmail.com>
 * Contributor(s) so far - 2010-07-30 :
 *     Emiliano Leporati <emiliano.leporati@gmail.com>
 *
 * Last changes :
 *     Kinoglaz birth: pervasive bug fixing, improvements and client support
 *     sdp debugged
 *     interleave ok
 *     added licence disclaimer
 *     first import
 *
 **/


#ifndef __KGD_UTILS_CONTAINER_H
#define __KGD_UTILS_CONTAINER_H

#include <map>
#include <list>
#include <memory>
#include "lib/utils/pointer.hpp"


using namespace std;

namespace KGD
{
	namespace Ptr
	{
		//! copy a container of object pointers into another

		//! \param to is not cleared before copy, so you can use successive copy call to append cloned data to a container
		template < class T, template< class T, class A = allocator< T > > class C1, template< class T, class A = allocator< T > > class C2 >
		void copy( const C1< T* > & from, C2< T* > & to );

		//! copy a map of object pointers into another
		template < class K, class T >
		void copy( const map< K, T* > & from, map< K, T* > & to );

		//! clears a container of object pointers
		template < class T, template< class T, class A = allocator< T > > class C >
		void clear ( C < T* > & );

		//! clears a map of object pointers
		template < class K, class T >
		void clear ( map< K, T* > & );

		//! clones a container of object pointers
		template < class T, template< class T, class A = allocator< T > > class C >
		C< T* > clone ( const C < T* > & );

		//! clones a map of object pointers
		template < class K, class T >
		map< K, T* > clone ( const map< K, T* > & );

		//! transforms a container of object pointers in a container of references
		template < class T, template< class T, class A = allocator< T > > class C >
		C< Ptr::Ref< T > > toRef ( C < T* > & );
		//! transforms a container of object pointers in a container of const references
		template < class T, template< class T, class A = allocator< T > > class C >
		C< Ptr::Ref< const T > > toConstRef ( const C < T* > & );
		//! transforms a map of object pointers in a map of references
		template < class K, class T >
		map< K, Ref< T > > toRef ( map< K, T* > & );
		//! transforms a map of object pointers in a map of const references
		template < class K, class T >
		map< K, Ref< const T > > toConstRef ( const map< K, T* > & );
	}

	//! Container clear and clone wrappers to fully support pointer cleanup
	namespace Ctr
	{
		//! copy a container into another
		template< class C1, class C2 >
		void copy( const C1 & from, C2 & to );

		// ******************************************************************************************

		//! clears a container of containers
		template < class T, template< class T, class A = allocator< T > > class C2, template< class W = C2< T >, class A = allocator< W > > class C1 >
		void clear ( C1< C2 < T > > & );

		//! clears a container of maps
		template < class K, class T, template< class T, class A = allocator< T > > class C >
		void clear ( C< map< K, T > > & );

		//! clears a container of objects
		template < class T, template< class T, class A = allocator< T > > class C >
		void clear ( C < T > & );

		// ******************************************************************************************

		//! clears a map of containers
		template < class K, class T, template< class T, class A = allocator< T > > class C >
		void clear ( map< K, C< T > > & );

		//! clears a map of maps
		template < class K1, class K2, class T >
		void clear ( map< K1, map< K2, T > > & );

		//! clears a map of objects
		template < class K, class T >
		void clear ( map< K, T > & );

		// ******************************************************************************************

		//! returns the list of keys in the map
		template < class K, class T >
		list< K > getKeys( const map< K, T > & );

		//! returns the list of keys in the map in a given container type
		template < template< class K, class A = allocator< K > > class C, class K, class T >
		C< K > getKeysCont( const map< K, T > & );

		//! returns the list of values in the map
		template < class K, class T >
		list< T > getValues( const map< K, T > & );

		//! returns the list of values in the map in a given container type
		template < template< class T, class A = allocator< T > > class C, class K, class T >
		C< T > getValuesCont( const map< K, T > & );

		// ******************************************************************************************

		//! clones a container of containers
		template < class T, template< class T, class A = allocator< T > > class C2, template< class W = C2< T >, class A = allocator< W > > class C1 >
		C1< C2< T > > clone ( const C1< C2< T > > & );

		//! clones a container of map
		template < class K, class T, template< class T, class A = allocator< T > > class C >
		C< map< K, T > > clone ( const C< map< K, T > > & );

		//! clones a container of objects
		template < class T, template< class T, class A = allocator< T > > class C >
		C< T > clone ( const C < T > & );

		// ******************************************************************************************

		//! clones a map of containers
		template < class K, class T, template< class T, class A = allocator< T > > class C >
		map< K, C< T > > clone ( const map< K, C< T > > & );

		//! clones a map of maps
		template < class K1, class K2, class T >
		map< K1, map< K2, T > > clone ( const map< K1, map< K2, T > > & );

		//! clones a map of objects
		template < class K, class T >
		map< K, T > clone ( const map< K, T > & );


		// ******************************************************************************************
		// wrappers to correct Ptr namespace

		//! copy a container of object pointers into another
		template < class T, template< class T, class A = allocator< T > > class C1, template< class T, class A = allocator< T > > class C2 >
		void copy( const C1< T* > & from, const C2< T* > & to );

		//! copy a map of object pointers into another
		template < class K, class T >
		void copy( const map< K, T* > & from, map< K, T* > & to );

		//! clears a container of object pointers
		template < class T, template< class T, class A = allocator< T > > class C >
		void clear ( C < T* > & );

		//! clears a map of object pointers
		template < class K, class T >
		void clear ( map< K, T* > & );

		//! clones a container of object pointers
		template < class T, template< class T, class A = allocator< T > > class C >
		C< T* > clone ( const C < T* > & );

		//! clones a map of object pointers
		template < class K, class T >
		map< K, T* > clone ( const map< K, T* > & );

		//! transforms a container of object pointers in a container of references
		template < class T, template< class T, class A = allocator< T > > class C >
		C< Ptr::Ref< T > > toRef ( C < T* > & );
		//! transforms a container of object pointers in a container of const references
		template < class T, template< class T, class A = allocator< T > > class C >
		C< Ptr::Ref< const T > > toConstRef ( const C < T* > & );

		//! transforms a map of object pointers in a map of references
		template < class K, class T >
		map< K, Ptr::Ref< T > > toRef ( const map< K, T* > & );
		//! transforms a map of object pointers in a map of const references
		template < class K, class T >
		map< K, Ptr::Ref< const T > > toConstRef ( const map< K, T* > & );
	}
}
#endif
