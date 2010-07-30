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
 * File name: ./lib/utils/container.hpp
 * First submitted: 2010-01-16
 * First submitter: Emiliano Leporati <emiliano.leporati@gmail.com>
 * Contributor(s) so far - 2010-07-30 :
 *     Emiliano Leporati <emiliano.leporati@gmail.com>
 *
 * Last changes :
 *     Kinoglaz birth: pervasive bug fixing, improvements and client support
 *     interleave ok
 *     first import
 *
 **/

#ifndef __KGD_UTILS_CONTAINER_HPP
#define __KGD_UTILS_CONTAINER_HPP

#include "lib/utils/container.h"

using namespace std;

namespace KGD
{
	// ******************************************************************************************
	// ******************************************************************************************
	namespace Ptr
	{
		using namespace KGD::Ctr;

		// ******************************************************************************************

		template < class T, template< class T, class A = allocator< T > > class C1, template< class T, class A = allocator< T > > class C2 >
		void copy( const C1< T* > & from, C2< T* > & to )
		{
			typename C1< T* >::const_iterator it = from.begin(), ed = from.end();
			for(; it != ed; ++it )
				to.push_back( clone( *it ) );

		}

		template < class K, class T >
		void copy( const map< K, T* > & from, map< K, T* > & to )
		{
			typename map< K, T* >::const_iterator it = from.begin(), ed = from.end();
			for(; it != ed; ++it )
				to[ it->first ] = clone( *( it->second ) );
		}

		// ******************************************************************************************

		template < class T, template< class T, class A = allocator< T > > class C >
		void clear ( C < T* > & ctr )
		{
			typename C< T* >::iterator it = ctr.begin(), ed = ctr.end();
			for(; it != ed; ++it )
				clear( *it );

			ctr.clear();
		}

		template < class K, class T >
		void clear ( map< K, T* > & ctr )
		{
			typename map< K, T* >::iterator it = ctr.begin(), ed = ctr.end();
			for(; it != ed; ++it )
				clear( it->second );

			ctr.clear();
		}

		// ******************************************************************************************

		template < class T, template< class T, class A = allocator< T > > class C >
		C< T* > clone ( const C< T* > & ctr )
		{
			C< T* > rt;
			typename C< T* >::const_iterator it = ctr.begin(), ed = ctr.end();
			for(; it != ed; ++it )
				rt.push_back( clone( *it ) );

			return rt;
		}

		template < class K, class T >
		map< K, T* > clone ( const map< K, T* > & ctr )
		{
			map< K, T* > rt;
			typename map< K, T* >::const_iterator it = ctr.begin(), ed = ctr.end();
			for(; it != ed; ++it )
				rt[ it->first ] = clone( it->second );

			return rt;
		}


		// ******************************************************************************************

		template < class T, template< class T, class A = allocator< T > > class C >
		C< Ref< T > > toRef ( C < T* > & ctr )
		{
			C< Ref< T > > rt;
			typename C< T* >::iterator it = ctr.begin(), ed = ctr.end();
			for(; it != ed; ++it )
				rt.push_back( toRef( *it ) );

			return rt;
		}

		template < class T, template< class T, class A = allocator< T > > class C >
		C< Ref< const T > > toConstRef ( const C < T* > & ctr )
		{
			C< Ref< const T > > rt;
			typename C< T* >::const_iterator it = ctr.begin(), ed = ctr.end();
			for( ; it != ed; ++it )
				rt.push_back( toConstRef( *it ) );
			return rt;
		}


		template < class K, class T >
		map< K, Ref< T > > toRef ( map< K, T* > & ctr )
		{
			map< K, Ref< T > > rt;
			typename map< K, T* >::const_iterator it = ctr.begin(), ed = ctr.end();
			for(; it != ed; ++it )
				rt[ it->first ] = toRef( it->second );

			return rt;
		}

		template < class K, class T >
		map< K, Ref< const T > > toConstRef ( map< K, T* > & ctr )
		{
			map< K, Ref< T > > rt;
			typename map< K, T* >::const_iterator it = ctr.begin(), ed = ctr.end();
			for(; it != ed; ++it )
				rt[ it->first ] = toConstRef( it->second );

			return rt;
		}

	}

	// ******************************************************************************************
	// ******************************************************************************************

	namespace Ctr
	{
		using namespace KGD::Ptr;

		// ******************************************************************************************

		template< class C1, class C2 >
		void copy( const C1 & from, C2 & to )
		{
			typename C1::const_iterator it = from.begin(), ed = from.end();
			for(; it != ed; ++it )
				to.push_back( *it );
		}

		// ******************************************************************************************

		template < class T, template< class T, class A = allocator< T > > class C2, template< class W = C2< T >, class A = allocator< W > > class C1 >
		void clear ( C1< C2 < T > > & ctr )
		{
			typename C1< C2 < T > >::iterator it = ctr.begin(), ed = ctr.end();
			for(; it != ed; ++it )
				clear( *it );

			ctr.clear();
		}

		template < class K, class T, template< class T, class A = allocator< T > > class C >
		void clear ( C< map< K, T > > & ctr )
		{
			typename C< map< K, T > >::iterator it = ctr.begin(), ed = ctr.end();
			for(; it != ed; ++it )
				clear( *it );

			ctr.clear();
		}

		template < class T, template< class T, class A = allocator< T > > class C >
		void clear ( C < T > & ctr )
		{
			ctr.clear();
		}

		// ******************************************************************************************

		template < class K, class T, template< class T, class A = allocator< T > > class C >
		void clear ( map< K, C< T > > & ctr )
		{
			typename map< K, C< T > >::iterator it = ctr.begin(), ed = ctr.end();
			for(; it != ed; ++it )
				clear( it->second );

			ctr.clear();
		}

		template < class K1, class K2, class T >
		void clear ( map< K1, map< K2, T > > & ctr )
		{
			typename map< K1, map< K2, T > >::iterator it = ctr.begin(), ed = ctr.end();
			for(; it != ed; ++it )
				clear( it->second );

			ctr.clear();
		}

		template < class K, class T >
		void clear ( map< K, T > & ctr )
		{
			ctr.clear();
		}


		template < class K, class T >
		list< K > getKeys( const map< K, T > & ctr )
		{
			return getKeysCont< list >( ctr );
		}

		template < template< class K, class A = allocator< K > > class C, class K, class T >
		C< K > getKeysCont( const map< K, T > & ctr )
		{
			typename map< K, T >::const_iterator it = ctr.begin(), ed = ctr.end();
			C< K > rt;
			for(; it != ed; ++it )
				rt.push_back( it->first );

			return rt;
		}

		template < class K, class T >
		list< T > getValues( const map< K, T > & ctr )
		{
			return getValuesCont< list >( ctr );
		}

		template < template< class T, class A = allocator< T > > class C, class K, class T >
		C< T > getValuesCont( const map< K, T > & ctr )
		{
			typename map< K, T >::const_iterator it = ctr.begin(), ed = ctr.end();
			C< T > rt;
			for(; it != ed; ++it )
				rt.push_back( it->second );

			return rt;
		}

		// ******************************************************************************************

		template < class T, template< class T, class A = allocator< T > > class C2, template< class W = C2< T >, class A = allocator< W > > class C1 >
		C1< C2< T > > clone ( const C1< C2< T > > & ctr )
		{
			C1< C2< T > > rt;
			typename C1< C2 < T > >::const_iterator it = ctr.begin(), ed = ctr.end();
			for(; it != ed; ++it )
				rt.push_back( clone( *it ) );

			return rt;
		}

		template < class K, class T, template< class T, class A = allocator< T > > class C >
		C< map< K, T > > clone ( const C< map< K, T > > & ctr )
		{
			C< map< K, T > > rt;
			typename C< map< K, T > >::const_iterator it = ctr.begin(), ed = ctr.end();
			for(; it != ed; ++it )
				rt.push_back( clone( *it ) );

			return rt;
		}

		template < class T, template< class T, class A = allocator< T > > class C >
		C< T > clone ( const C< T > & ctr )
		{
			C< T > rt;
			typename C< T >::const_iterator it = ctr.begin(), ed = ctr.end();
			for(; it != ed; ++it )
				rt.push_back( *it );

			return rt;
		}

		// ******************************************************************************************

		template < class K, class T, template< class T, class A = allocator< T > > class C >
		map< K, C< T > > clone ( const map< K, C< T > > & ctr )
		{
			map< K, C< T > > rt;
			typename map< K, C< T > >::const_iterator it = ctr.begin(), ed = ctr.end();
			for(; it != ed; ++it )
				rt[ it->first ] = clone( it->second );

			return rt;
		}

		template < class K1, class K2, class T >
		map< K1, map< K2, T > > clone ( const map< K1, map< K2, T > > & ctr )
		{
			map< K1, map< K2, T > > rt;
			typename map< K1, map< K2, T > >::const_iterator it = ctr.begin(), ed = ctr.end();
			for(; it != ed; ++it )
				rt[ it->first ] = clone( it->second );

			return rt;
		}

		template < class K, class T >
		map< K, T > clone ( const map< K, T > & ctr )
		{
			map< K, T > rt;
			typename map< K, T >::const_iterator it = ctr.begin(), ed = ctr.end();
			for(; it != ed; ++it )
				rt[ it->first ] = it->second;

			return rt;
		}
	}

	// ******************************************************************************************
	// ******************************************************************************************

	namespace Ctr
	{

		template < class T, template< class T, class A = allocator< T > > class C1, template< class T, class A = allocator< T > > class C2 >
		void copy( const C1< T* > & from, const C2< T* > & to )
		{
			Ptr::copy( from, to );
		}

		template < class K, class T >
		void copy( const map< K, T* > & from, map< K, T* > & to )
		{
			Ptr::copy( from, to );
		}


		template < class T, template< class T, class A = allocator< T > > class C >
		void clear ( C < T* > & ctr )
		{
			Ptr::clear( ctr );
		}

		template < class K, class T >
		void clear ( map< K, T* > & ctr )
		{
			Ptr::clear( ctr );
		}

		template < class T, template< class T, class A = allocator< T > > class C >
		C< T* > clone ( const C< T* > & ctr )
		{
			return Ptr::clone( ctr );
		}

		template < class K, class T >
		map< K, T* > clone ( const map< K, T* > & ctr )
		{
			return Ptr::clone( ctr );
		}


		template < class T, template< class T, class A = allocator< T > > class C >
		C< Ptr::Ref< T > > toRef ( C < T* > & ctr )
		{
			return Ptr::toRef( ctr );
		}


		template < class K, class T >
		map< K, Ptr::Ref< T > > toRef ( map< K, T* > & ctr )
		{
			return Ptr::toRef( ctr );
		}


		template < class T, template< class T, class A = allocator< T > > class C >
		C< Ptr::Ref< const T > > toConstRef ( const C < T* > & ctr )
		{
			return Ptr::toConstRef( ctr );
		}


		template < class K, class T >
		map< K, Ptr::Ref< const T > > toConstRef ( const map< K, T* > & ctr )
		{
			return Ptr::toConstRef( ctr );
		}
	}
}
#endif
