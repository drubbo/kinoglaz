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
 * File name: ./lib/utils/map.hpp
 * First submitted: 2010-07-13
 * First submitter: Emiliano Leporati <emiliano.leporati@gmail.com>
 * Contributor(s) so far - 2010-07-30 :
 *     Emiliano Leporati <emiliano.leporati@gmail.com>
 *
 * Last changes :
 *     
 *
 **/


#ifndef __KGD_MAP_HPP
#define __KGD_MAP_HPP

#include "lib/utils/map.h"

namespace KGD
{
	namespace Ctr
	{
		// ************************************************************************************************************
		// Ctr::Map

		template< class K, class T >
		Map< K, T >::Map( const TValueMap & m )
			: _data( m )
		{
		}

		template< class K, class T >
		Map< K, T >::Map( )
		{
		}

		template< class K, class T >
		Map< K, T >::Map( const Map< K, T > & m )
			: _data( m._data )
		{

		}

		template< class K, class T >
		Map< K, T >::~Map( )
		{
			Ctr::clear( _data );
		}

		template< class K, class T >
		T & Map< K, T >::operator()( const K & k ) throw()
		{
			return _data[ k ];
		}

		template< class K, class T >
		const T & Map< K, T >::operator[]( const K & k ) const throw( Exception::NotFound )
		{
			typename TValueMap::const_iterator it = _data.find( k );
			if ( it == _data.end() )
				throw Exception::NotFound ( "Key: " + toString( k ) );
			else
				return it->second;
		}

		template< class K, class T >
		T Map< K, T >::operator()( const K & k, const T & def ) const throw()
		{
			typename TValueMap::const_iterator it = _data.find( k );
			if ( it == _data.end() )
				return def;
			else
				return it->second;
		}

		template< class K, class T >
		const T & Map< K, T >::operator()( const K & k, const Exception::NotFound & e ) const throw( Exception::NotFound )
		{
			typename TValueMap::const_iterator it = _data.find( k );
			if ( it == _data.end() )
				throw e;
			else
				return it->second;
		}

		template< class K, class T >
		T & Map< K, T >::operator()( const K & k, const Exception::NotFound & e ) throw( Exception::NotFound )
		{
			typename TValueMap::iterator it = _data.find( k );
			if ( it == _data.end() )
				throw e;
			else
				return it->second;
		}

		template< class K, class T >
		template< template< class KK, class A = allocator< KK > > class C >
		C< K > Map< K, T >::keys() const throw()
		{
			return Ctr::getKeysCont< C >( _data );
		}

		template< class K, class T >
		template< template< class TT, class A = allocator< TT > > class C >
		C< T > Map< K, T >::values() const throw()
		{
			return Ctr::getValuesCont< C >( _data );
		}

		template< class K, class T >
		void Map< K, T >::clear() throw()
		{
			Ctr::clear( _data );
		}

		template< class K, class T >
		void Map< K, T >::erase( const K & k ) throw( Exception::NotFound )
		{
			this->erase( k, Exception::NotFound( "Key: " + toString( k ) ) );
		}

		template< class K, class T >
		void Map< K, T >::erase( const K & k, const Exception::NotFound & e  ) throw( Exception::NotFound )
		{
			typename TValueMap::iterator it = _data.find( k );
			if ( it == _data.end() )
				throw e;
			else
				_data.erase( it );
		}

		template< class K, class T >
		bool Map< K, T >::has( const K & k ) const throw()
		{
			return _data.find( k ) != _data.end();
		}

		template< class K, class T >
		bool Map< K, T >::empty( ) const throw()
		{
			return _data.empty();
		}

		template< class K, class T >
		size_t Map< K, T >::size( ) const throw()
		{
			return _data.size();
		}

		// ************************************************************************************************************
		// Ctr::Map::IteratorBase

		template< class K, class T >
		template< class IT, class Ret >
		Map< K, T >::IteratorBase< IT, Ret >::IteratorBase( IT bg, IT ed, size_t sz, const typename Map< K, T >::TValueMap & m )
			: Ctr::IteratorSeekBase<
				typename Map< K, T >::TValueMap,
				IT,
				Ret,
				K
			>( bg, ed, sz )
			, _data( m )
		{
		}

		template< class K, class T >
		template< class IT, class Ret >
		typename Map< K, T >::template IteratorBase< IT, Ret > & Map< K, T >::IteratorBase< IT, Ret >::find( const K & k ) throw( )
		{
			this->_it = this->_data.find( k );
			return *this;
		}

		template< class K, class T >
		template< class IT, class Ret >
		bool Map< K, T >::IteratorBase< IT, Ret >::has( const K & k ) const throw( )
		{
			IT it = this->_data.find( k );
			return it =! this->_ed;
		}

		template< class K, class T >
		template< class IT, class Ret >
		const K & Map< K, T >::IteratorBase< IT, Ret >::key() const throw( Exception::NullPointer )
		{
			if ( this->_it == this->_ed )
				throw Exception::NullPointer( "iterator for a " + string(typeid( Map< K, T > ).name() ) );
			else
				return this->_it->first;
		}

		template< class K, class T >
		template< class IT, class Ret >
		Ret & Map< K, T >::IteratorBase< IT, Ret >::val() const throw( Exception::NullPointer )
		{
			if ( this->_it == this->_ed )
				throw Exception::NullPointer( "iterator for a " + string(typeid( Map< K, T > ).name() ) );
			else
				return this->_it->second;
		}


		// ************************************************************************************************************
		// Ctr::Map::Iterator

		template< class K, class T >
		Map< K, T >::Iterator::Iterator( Map< K, T > & m )
			: IteratorBase< typename Map< K, T >::TValueMap::iterator, T >( m._data.begin(), m._data.end(), m._data.size(), m._data )
		{
		}

		template< class K, class T >
		Map< K, T >::Iterator::Iterator( typename Map< K, T >::TValueMap & m )
			: IteratorBase< typename Map< K, T >::TValueMap::iterator, T >( m.begin(), m.end(), m.size(), m )
		{
		}

		template< class K, class T >
		typename Map< K, T >::Iterator & Map< K, T >::Iterator::first() throw()
		{
			IteratorBase< typename Map< K, T >::TValueMap::iterator, T >::first();
			return *this;
		}

		template< class K, class T >
		typename Map< K, T >::Iterator & Map< K, T >::Iterator::next() throw( KGD::Exception::OutOfBounds )
		{
			IteratorBase< typename Map< K, T >::TValueMap::iterator, T >::next();
			return *this;
		}

		// Ctr::MapIterator

		template< class K, class T >
		MapIterator< K, T >::MapIterator( typename Map< K, T >::TValueMap & m )
			: Map< K, T >::Iterator( m )
		{

		}
		template< class K, class T >
		MapIterator< K, T >::MapIterator( Map< K, T > & m )
			: Map< K, T >::Iterator( m )
		{

		}

		// Ctr::MapTypeIterator

		template< class M >
		MapTypeIterator< M >::MapTypeIterator( M & m )
			: MapIterator< typename M::key_type, typename M::mapped_type >( m )
		{

		}

		// ************************************************************************************************************
		// Ctr::Map::ConstIterator

		template< class K, class T >
		Map< K, T >::ConstIterator::ConstIterator( const Map< K, T > & m )
			: IteratorBase< typename Map< K, T >::TValueMap::const_iterator, const T >( m._data.begin(), m._data.end(), m._data.size(), m._data )
		{
		}

		template< class K, class T >
		Map< K, T >::ConstIterator::ConstIterator( const typename Map< K, T >::TValueMap & m )
			: IteratorBase< typename Map< K, T >::TValueMap::const_iterator, const T >( m.begin(), m.end(), m.size(), m )
		{
		}

		template< class K, class T >
		typename Map< K, T >::ConstIterator & Map< K, T >::ConstIterator::first() throw()
		{
			IteratorBase< typename Map< K, T >::TValueMap::const_iterator, const T >::first();
			return *this;
		}

		template< class K, class T >
		typename Map< K, T >::ConstIterator & Map< K, T >::ConstIterator::next() throw( KGD::Exception::OutOfBounds )
		{
			IteratorBase< typename Map< K, T >::TValueMap::const_iterator, const T >::next();
			return *this;
		}

		// Ctr::ConstMapIterator

		template< class K, class T >
		ConstMapIterator< K, T >::ConstMapIterator( const typename Map< K, T >::TValueMap & m )
			: Map< K, T >::ConstIterator( m )
		{

		}
		template< class K, class T >
		ConstMapIterator< K, T >::ConstMapIterator( const Map< K, T > & m )
			: Map< K, T >::ConstIterator( m )
		{

		}

		// Ctr::ConstMapTypeIterator

		template< class M >
		ConstMapTypeIterator< M >::ConstMapTypeIterator( const M & m )
			: MapIterator< typename M::key_type, typename M::mapped_type >( m )
		{

		}
	}

	// ************************************************************************************************************
	// Ptr::Map

	namespace Ptr
	{
		template< class K, class T >
		Map< K, T >::Map( )
		{
		}

		template< class K, class T >
		Map< K, T >::Map( const Map< K, T > & m )
			: Map< K, T * >( Ctr::clone( m._data ) )
		{
		}

		template< class K, class T >
		Map< K, T > & Map< K, T >::operator=( const Map< K, T > & m ) throw()
		{
			Ctr::clear( this->_data );
			this->_data = Ctr::clone( m._data );
			return *this;
		}

		template< class K, class T >
		void Map< K, T >::erase( const K & k, const Exception::NotFound &e ) throw( Exception::NotFound )
		{
			typename Ctr::Map< K, T * >::TValueMap::iterator it = this->_data.find( k );
			if ( it == this->_data.end() )
				throw e;
			else
			{
				T * ptr = it->second;
				this->_data.erase( k );
				Ptr::clear( ptr );
			}
		}

		template< class K, class T >
		T * & Map< K, T >::operator()( const K & k ) throw()
		{
			return Ctr::Map< K, T * >::operator()( k );
		}

		template< class K, class T >
		const T & Map< K, T >::operator[]( const K & k ) const throw( Exception::NotFound )
		{
			return *(Ctr::Map< K, T * >::operator[]( k ));
		}

		template< class K, class T >
		const T & Map< K, T >::operator()( const K & k, const Exception::NotFound &e ) const throw( Exception::NotFound )
		{
			return *(Ctr::Map< K, T * >::operator()( k, e ));
		}

		template< class K, class T >
		T & Map< K, T >::operator()( const K & k, const Exception::NotFound &e ) throw( Exception::NotFound )
		{
			return *(Ctr::Map< K, T * >::operator()( k, e ));
		}

		// ************************************************************************************************************
		// Ptr::Map::Iterator

		template< class K, class T >
		Map< K, T >::Iterator::Iterator( typename Map< K, T >::TValueMap & m )
			: Ctr::Map< K, T* >::Iterator( m )
		{
		}
		template< class K, class T >
		Map< K, T >::Iterator::Iterator( Map< K, T > & m )
			: Ctr::Map< K, T* >::Iterator( m )
		{
		}

		template< class K, class T >
		typename Map< K, T >::Iterator & Map< K, T >::Iterator::first() throw()
		{
			Ctr::Map< K, T* >::Iterator::first();
			return *this;
		}

		template< class K, class T >
		typename Map< K, T >::Iterator & Map< K, T >::Iterator::next() throw( KGD::Exception::OutOfBounds )
		{
			Ctr::Map< K, T* >::Iterator::next();
			return *this;
		}

		template< class K, class T >
		T & Map< K, T >::Iterator::val() throw( Exception::NullPointer )
		{
			return *( Ctr::Map< K, T* >::Iterator::val() );
		};

		// Ptr::MapIterator

		template< class K, class T >
		MapIterator< K, T >::MapIterator( typename Map< K, T >::TValueMap & m )
			: Map< K, T >::Iterator( m )
		{

		}
		template< class K, class T >
		MapIterator< K, T >::MapIterator( Map< K, T > & m )
			: Map< K, T >::Iterator( m )
		{

		}

		// ************************************************************************************************************
		// Ptr::Map::ConstIterator

		template< class K, class T >
		Map< K, T >::ConstIterator::ConstIterator( const typename Map< K, T >::TValueMap & m )
			: Ctr::Map< K, T* >::ConstIterator( m )
		{
		}
		template< class K, class T >
		Map< K, T >::ConstIterator::ConstIterator( const Map< K, T > & m )
			: Ctr::Map< K, T* >::ConstIterator( m )
		{
		}


		template< class K, class T >
		typename Map< K, T >::ConstIterator & Map< K, T >::ConstIterator::first() throw()
		{
			Ctr::Map< K, T* >::ConstIterator::first();
			return *this;
		}

		template< class K, class T >
		typename Map< K, T >::ConstIterator & Map< K, T >::ConstIterator::next() throw( KGD::Exception::OutOfBounds )
		{
			Ctr::Map< K, T* >::ConstIterator::next();
			return *this;
		}

		template< class K, class T >
		const T & Map< K, T >::ConstIterator::val() const throw( Exception::NullPointer )
		{
			return *( Ctr::Map< K, T* >::ConstIterator::val() );
		};

		// Ptr::ConstMapIterator

		template< class K, class T >
		ConstMapIterator< K, T >::ConstMapIterator( const typename Map< K, T >::TValueMap & m )
			: Map< K, T >::ConstIterator( m )
		{

		}
		template< class K, class T >
		ConstMapIterator< K, T >::ConstMapIterator( const Map< K, T > & m )
			: Map< K, T >::ConstIterator( m )
		{

		}
	}
}

#endif
