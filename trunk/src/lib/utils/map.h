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
 * File name: ./lib/utils/map.h
 * First submitted: 2010-07-13
 * First submitter: Emiliano Leporati <emiliano.leporati@gmail.com>
 * Contributor(s) so far - 2010-07-30 :
 *     Emiliano Leporati <emiliano.leporati@gmail.com>
 *
 * Last changes :
 *     
 *
 **/




#ifndef __KGD_MAP
#define __KGD_MAP

#include "lib/exceptions.h"
#include "lib/utils/iterator.hpp"

#include <string>
#include <map>
#include <list>

using namespace std;

namespace KGD
{
	namespace Ctr
	{
		//! generic map
		template< class K, class T >
		class Map
		{
		public:
			//! inner map type
			typedef map< K, T > TValueMap;
		protected:
			//! inner data map
			TValueMap _data;
		public:
			//! type of keys
			typedef K TKey;
			//! type of values
			typedef T TValue;
			//! default ctor
			Map();
			//! build from a data map
			Map( const TValueMap & );
			//! copy ctor
			Map( const Map< K, T > & );
			//! dtor
			virtual ~Map();
			//! get reference to value
			T & operator()( const K & ) throw();
			//! get value or default if key is missing
			T operator()( const K &, const T & def = T() ) const throw();
			//! get const reference if present
			const T & operator[]( const K & ) const throw( Exception::NotFound );
			//! get reference or custom exception
			T & operator()( const K &, const Exception::NotFound &) throw( Exception::NotFound );
			//! get const reference or custom exception
			const T & operator()( const K &, const Exception::NotFound &) const throw( Exception::NotFound );
			//! tells if key is registered
			bool has( const K & ) const throw();
			//! returns key list
			template< template< class KK, class A = allocator< KK > > class C >
			C< K > keys() const throw();
			//! returns value list
			template< template< class TT, class A = allocator< TT > > class C >
			C< T > values() const throw();
			//! tells if map is empty
			bool empty() const throw();
			//! returns map size
			size_t size() const throw();
			//! clears hidden map
			void clear() throw();
			//! removes a key from hidden map
			void erase( const K & ) throw( Exception::NotFound );
			//! removes a key from hidden map
			virtual void erase( const K &, const Exception::NotFound &) throw( Exception::NotFound );

			template< class IT, class Ret >
			class IteratorBase
				: public Ctr::IteratorSeekBase<
					typename Map< K, T >::TValueMap,
					IT,
					Ret,
					K
				>
			{
			protected:
				typename Map< K, T >::TValueMap const & _data;
				//! build from a data set
				IteratorBase( IT bg, IT ed, size_t sz, const typename Map< K, T >::TValueMap & );
			public:				

				//! find redefined
				bool has( const K & ) const throw( );
				//! find redefined
				IteratorBase & find( const K & ) throw( );

				//! get current key
				const K & key() const throw( Exception::NullPointer );
				//! get val redefined
				Ret & val() const throw( Exception::NullPointer );
			};

			//! mutable iterator
			class Iterator
				: public IteratorBase< typename TValueMap::iterator, T >
			{
			public:
				//! build from a data set
				Iterator( typename Map< K, T >::TValueMap & );
				//! build from a map
				Iterator( Map< K, T > & );

				//! rewind
				virtual Iterator & first() throw();
				//! advance
				virtual Iterator & next() throw( Exception::OutOfBounds );
			};

			//! const iterator
			class ConstIterator
				: public IteratorBase< typename TValueMap::const_iterator, const T >
			{
			public:
				//! build from a data set
				ConstIterator( const typename Map< K, T >::TValueMap & );
				//! build from a map
				ConstIterator( const Map< K, T > & );

				//! rewind
				virtual ConstIterator & first() throw();
				//! advance
				virtual ConstIterator & next() throw( Exception::OutOfBounds );
			};

			friend class Iterator;
			friend class ConstIterator;
		};

		//! utility class to build a map iterator over a std::map or a Ctr::Map, given key and value type
		template< class K, class T >
		class MapIterator
			: public Map< K, T >::Iterator
		{
		public:
			//! build from a data set
			MapIterator( typename Map< K, T >::TValueMap & );
			//! build from a data set
			MapIterator( Map< K, T > & );
		};

		//! utility class to build a map iterator over a std::map, given the map type
		template< class M >
		class MapTypeIterator
			: public MapIterator< typename M::key_type, typename M::mapped_type >
		{
		public:
			//! build from a data set
			MapTypeIterator( M & );
		};

		//! utility class to build a const map iterator over a std::map or a Ctr::Map, given key and value type
		template< class K, class T >
		class ConstMapIterator
			: public Map< K, T >::ConstIterator
		{
		public:
			//! build from a data set
			ConstMapIterator( const typename Map< K, T >::TValueMap & );
			//! build from a data set
			ConstMapIterator( const Map< K, T > & );
		};

		//! utility class to build a const map iterator over a std::map, given the map type
		template< class M >
		class ConstMapTypeIterator
			: public MapIterator< typename M::key_type, typename M::mapped_type >
		{
		public:
			//! build from a data set
			ConstMapTypeIterator( const M & );
		};

		//! key-value pair set
		typedef Map< string, string > KeyValueMap;
	}

	namespace Ptr
	{
		//! pointer map: maps keys to pointers-to-data
		template< class K, class T >
		class Map
			: public Ctr::Map< K, T* >
		{
		public:
			//! type of keys
			typedef K TKey;
			//! type of values (stored as pointers to T)
			typedef T TValue;
			//! default ctor
			Map();
			//! copy (deep copy of pointers)
			Map( const Map< K, T > & );
			//! assign with memory free and clone
			Map< K, T > & operator=( const Map< K, T > & ) throw();

			//! get reference to pointer
			T * & operator()( const K & ) throw();
			//! get const reference to value
			const T & operator[]( const K & ) const throw( Exception::NotFound );
			//! get reference to value or custom exception
			T & operator()( const K &, const Exception::NotFound &) throw( Exception::NotFound );
			//! get const reference to value or custom exception
			const T & operator()( const K &, const Exception::NotFound &) const throw( Exception::NotFound );

			using Ctr::Map< K, T* >::clear;
			using Ctr::Map< K, T* >::erase;
			//! removes a key from hidden map
			virtual void erase( const K &, const Exception::NotFound &) throw( Exception::NotFound );

			//! mutable iterator
			class Iterator
				: public Ctr::Map< K, T* >::Iterator
			{
			public:
				//! build from a data set
				Iterator( typename Map< K, T >::TValueMap & );
				//! build from a map
				Iterator( Map< K, T > & );
				//! rewind
				virtual Iterator & first() throw();
				//! advance
				virtual Iterator & next() throw( Exception::OutOfBounds );
				//! get current val, derefenced
				T & val() throw( Exception::NullPointer );
			};

			//! const iterator
			class ConstIterator
				: public Ctr::Map< K, T* >::ConstIterator
			{
			public:
				//! build from a data set
				ConstIterator( const typename Map< K, T >::TValueMap & );
				//! build from a const map
				ConstIterator( const Map< K, T > & );
				//! rewind
				virtual ConstIterator & first() throw();
				//! advance
				virtual ConstIterator & next() throw( Exception::OutOfBounds );
				//! get current val, derefenced
				const T & val() const throw( Exception::NullPointer );
			};
		};

		//! utility class to build a map iterator over a std::map or a Ptr::Map, given key and value type
		template< class K, class T >
		class MapIterator
			: public Map< K, T >::Iterator
		{
		public:
			//! build from a data set
			MapIterator( typename Map< K, T >::TValueMap & );
			//! build from a data set
			MapIterator( Map< K, T > & );
		};

		//! utility class to build a const map iterator over a std::map or a Ptr::Map, given key and value type
		template< class K, class T >
		class ConstMapIterator
			: public Map< K, T >::ConstIterator
		{
		public:
			//! build from a data set
			ConstMapIterator( const typename Map< K, T >::TValueMap & );
			//! build from a data set
			ConstMapIterator( const Map< K, T > & );
		};

	}
}

#endif

