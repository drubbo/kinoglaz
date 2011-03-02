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
 * File name: src/lib/utils/ref_container.hpp
 * First submitted: 2010-10-18
 * First submitter: Emiliano Leporati <emiliano.leporati@gmail.com>
 * Contributor(s) so far - 2010-11-04 :
 *     Emiliano Leporati <emiliano.leporati@gmail.com>
 *
 * Last changes :
 *     boosted
 *
 **/

#ifndef __KGD_REF_CTR_H
#define __KGD_REF_CTR_H

#include "lib/common.h"
#include "lib/exceptions.h"

namespace KGD
{
	//! bidirectional iterator over a container of references

	//! adapts a common iterator over a container of pointers
	//! template arguments:
	//! T - the type of the returned reference ( int, const string, etc ),
	//!     the same owned by the reference container returning this iterator
	//! I - the type of the adapted iterator
	template< class T, class I >
	class bidi_ref_iterator
	{
		private:
			I _iterator;
		public:
			typedef typename I::iterator_category iterator_category;
			typedef T value_type;
			typedef typename I::difference_type difference_type;
			typedef T* pointer;
			typedef T& reference;

			bidi_ref_iterator( const I & i )
			: _iterator( i ) {}

			bidi_ref_iterator( const bidi_ref_iterator & i )
			: _iterator( i._iterator ) {}

			reference operator*()
			{ return **_iterator; }

			pointer operator->()
			{ return *_iterator; }

			bidi_ref_iterator & operator++()
			{
				++ _iterator;
				return *this;
			}
			bidi_ref_iterator operator++( int )
			{
				bidi_ref_iterator rt( *this );
				++ _iterator;
				return rt;
			}

			bidi_ref_iterator & operator--()
			{
				-- _iterator;
				return *this;
			}
			bidi_ref_iterator operator--( int )
			{
				bidi_ref_iterator rt( *this );
				-- _iterator;
				return rt;
			}

			bool operator==( const bidi_ref_iterator i ) const
			{ return _iterator == i._iterator; }
			bool operator!=( const bidi_ref_iterator i ) const
			{ return _iterator != i._iterator; }
	};


	//! random access iterator over a container of references

	//! adapts a common iterator over a container of pointers
	//! template arguments:
	//! T - the type of the returned reference ( int, const string, etc ),
	//!     the same owned by the reference container returning this iterator
	//! I - the type of the adapted iterator
	template< class T, class I >
	class random_ref_iterator
	{
		private:
			I _iterator;
		public:
			typedef typename I::iterator_category iterator_category;
			typedef T value_type;
			typedef typename I::difference_type difference_type;
			typedef T* pointer;
			typedef T& reference;

			random_ref_iterator( const I & i )
			: _iterator( i )
			{}
			random_ref_iterator( const random_ref_iterator & i )
			: _iterator( i._iterator )
			{}

			reference operator*()
			{ return **_iterator; }

			pointer operator->()
			{ return *_iterator; }

			random_ref_iterator & operator++()
			{
				++ _iterator;
				return *this;
			}
			random_ref_iterator operator++( int )
			{
				random_ref_iterator rt( *this );
				++ _iterator;
				return rt;
			}

			random_ref_iterator & operator--()
			{
				-- _iterator;
				return *this;
			}
			random_ref_iterator operator--( int )
			{
				random_ref_iterator rt( *this );
				-- _iterator;
				return rt;
			}
			random_ref_iterator operator+( const difference_type & n )
			{ return random_ref_iterator( _iterator + n ); }
			random_ref_iterator & operator+=( const difference_type & n )
			{
				_iterator += n;
				return *this;
			}

			random_ref_iterator operator-( const difference_type & n )
			{ return random_ref_iterator( _iterator - n ); }
			random_ref_iterator & operator-=( const difference_type & n )
			{
				_iterator -= n;
				return *this;
			}

			reference operator[]( difference_type i )
			{ return *(_iterator[ i ]); }

			bool operator==( const random_ref_iterator i ) const
			{ return _iterator == i._iterator; }
			bool operator!=( const random_ref_iterator i ) const
			{ return _iterator != i._iterator; }
			bool operator<( const random_ref_iterator i ) const
			{ return _iterator < i._iterator; }
			bool operator<=( const random_ref_iterator i ) const
			{ return _iterator <= i._iterator; }
			bool operator>( const random_ref_iterator i ) const
			{ return _iterator > i._iterator; }
			bool operator>=( const random_ref_iterator i ) const
			{ return _iterator >= i._iterator; }

	};

	//! base class for sequence containers of references

	//! encapsulates a container of pointers but doesn't have their ownership
	//! no modification of any kind is allowed on the container itself, like
	//! clear, erasion or insertion; at this level only a const interface is
	//! exposed, as only const iterators
	//! template arguments:
	//! T    - the type of exposed const references - is always a const type
	//! Tc   - the type of the contained pointers - can be a const type or not
	//! Cont - the type of the std:: adapted sequence container ( list, vector, etc. )
	//! I    - the type of the iterator adapter
    template<
		class T,
		class Tc,
		template< class Tp , class A = std::allocator< Tp > > class Cont,
		template< class Ti, class It > class I >
    class const_ref_sequence_container
    {
	protected:
		//! the wrapped container
		Cont< Tc* > _data;

	public:
		typedef  T value_type;
		typedef  T& const_reference;
		typedef  T& reference;
        typedef  I< T, typename Cont< Tc* >::const_iterator > iterator;
        typedef  I< T, typename Cont< Tc* >::const_iterator > const_iterator;
        typedef  typename Cont< Tc* >::difference_type difference_type;
        typedef  typename Cont< Tc* >::size_type size_type;
        typedef  typename Cont< Tc* >::allocator_type allocator_type;
		typedef  std::reverse_iterator< const_iterator > reverse_iterator;
        typedef  std::reverse_iterator< const_iterator > const_reverse_iterator;

	protected:
		//! build empty
		const_ref_sequence_container() {}

		//! copy construction - no clone is peformed
        const_ref_sequence_container( const const_ref_sequence_container& c )
        : _data( c._data ) {}

		//! construction from a container of pointers - no clone is peformed
        const_ref_sequence_container( const Cont< Tc* > & c )
        {
// 			_data.reserve( c.size() );
			BOOST_FOREACH( Tc* ptr, c )
			{
				_data.push_back( ptr );
			}
		}

	public:
		const_reference front() const
		{ return *_data.front(); }
		const_reference back() const
		{ return *_data.back(); }
        const_iterator begin() const
        { return const_iterator(_data.begin()); }
        const_iterator end() const
        { return const_iterator(_data.end()); }
        const_reverse_iterator rbegin() const
        { return const_reverse_iterator(_data.rbegin()); }
        const_reverse_iterator rend() const
        { return const_reverse_iterator(_data.rend()); }

		size_type size() const
		{ return _data.size(); }
        bool empty() const
        { return _data.empty(); }
	};

	//! base class for sequence containers of mutable references

	//! extends the const interface allowing non-const iteration
	//! so contained pointer's type is non-const
	//! T itself must be non-const
    template<
		class T,
		template< class Tp , class A = std::allocator< Tp > > class Cont,
		template< class Ti, class It > class I >
    class mutable_ref_sequence_container
    : public const_ref_sequence_container< const T, T, Cont, I >
    {
	private:
		typedef const_ref_sequence_container< const T, T, Cont, I > base_class;
	public:
		typedef  T& reference;
        typedef  I< T, typename Cont< T* >::iterator > iterator;
        typedef  std::reverse_iterator< iterator > reverse_iterator;
	protected:
		//! build empty
		mutable_ref_sequence_container() : base_class() {}

		//! copy construction - no clone is peformed
        mutable_ref_sequence_container( const mutable_ref_sequence_container& c )
        : base_class( c ) {}

		//! construction from a container of pointers - no clone is peformed
        mutable_ref_sequence_container( const Cont< T* > & c )
        : base_class( c ) {}

	public:
		reference front()
		{ return *base_class::_data.front(); }
		reference back()
		{ return *base_class::_data.back(); }
        iterator begin()
        { return iterator(base_class::_data.begin()); }
        iterator end()
        { return iterator(base_class::_data.end()); }
        reverse_iterator rbegin()
        { return reverse_iterator(base_class::_data.rbegin()); }
        reverse_iterator rend()
        { return reverse_iterator(base_class::_data.rend()); }
	};

	//! vector of non-const references

	//! extends the mutable sequence base class
	//! adds vector-specific accessors
	template< class T >
	class ref_vector
	: public mutable_ref_sequence_container< T, std::vector, random_ref_iterator >
	{
		private:
			typedef mutable_ref_sequence_container< T, std::vector, random_ref_iterator > base_class;

		public:
			//! build empty
			ref_vector()
			: base_class() {}

			//! copy
			ref_vector( const ref_vector& c )
			: base_class( c ) {}

			//! build from a vector of pointers
			ref_vector( const std::vector< T* > & c )
			: base_class( c ) { }

			//! build from a ptr vector
			ref_vector( const boost::ptr_vector< T > & c )
			: base_class( )
			{
				base_class::_data.reserve( c.size() );
				BOOST_FOREACH( const T & ptr, c )
				{
					base_class::_data.push_back( &ptr );
				}
			}

			//! unchecked vector element access
			typename base_class::const_reference operator[]( const typename base_class::difference_type & i ) const
			{ return base_class::_data[i]; }
			//! checked vector element access
			typename base_class::const_reference at( const typename base_class::difference_type & i ) const
			{ return base_class::_data.at(i); }

			//! unchecked vector element access
			typename base_class::reference operator[]( const typename base_class::difference_type & i )
			{ return base_class::_data[i]; }
			//! checked vector element access
			typename base_class::reference at( const typename base_class::difference_type & i )
			{ return base_class::_data.at(i); }

		//! let the const version be friend to ease construction from non-const version
		friend class ref_vector< const T >;
	};

	//! specialization for vector of const references

	//! construction from non-const data is allowed at this point
	//! adds vector-specific accessors
	//! extends the const sequence base class specifying a const
	//! type for both the exposed reference and the inner pointers
	template< class T >
	class ref_vector< const T >
	: public const_ref_sequence_container< const T, const T, std::vector, random_ref_iterator >
	{
		private:
			typedef const_ref_sequence_container< const T, const T, std::vector, random_ref_iterator > base_class;

		public:
			//! build empty
			ref_vector()
			: base_class() {}

			//! build from a vector of const references (copy)
			ref_vector( const ref_vector< const T >& c )
			: base_class( c ) {}

			//! build from a vector of pointers to const objects
			ref_vector( const std::vector< const T* > & c )
			: base_class( c ) { }

			//! build from a ptr vector of const objects
			ref_vector( const boost::ptr_vector< const T > & c )
			: base_class( )
			{
				base_class::_data.reserve( c.size() );
				BOOST_FOREACH( const T & ptr, c )
				{
					base_class::_data.push_back( &ptr );
				}
			}


			//! build from a vector of references
			ref_vector( const ref_vector< T > & c )
			: base_class( )
			{
				base_class::_data.reserve( c.size() );
				BOOST_FOREACH( T* ptr, c._data )
				{
					base_class::_data.push_back( const_cast< const T* >(ptr) );
				}
			}

			//! build from a vector of pointers to objects
			ref_vector( const std::vector< T* > & c )
			: base_class( )
			{
				base_class::_data.reserve( c.size() );
				BOOST_FOREACH( T* ptr, c )
				{
					base_class::_data.push_back( const_cast< const T* >(ptr) );
				}
			}

			//! build from a ptr vector
			ref_vector( const boost::ptr_vector< T > & c )
			: base_class( )
			{
				base_class::_data.reserve( c.size() );
				BOOST_FOREACH( const T & ptr, c )
				{
					base_class::_data.push_back( &ptr );
				}
			}

			//! unchecked vector element access
			typename base_class::const_reference operator[]( const typename base_class::difference_type & i ) const
			{ return base_class::_data[i]; }
			//! checked vector element access
			typename base_class::const_reference at( const typename base_class::difference_type & i ) const
			{ return base_class::_data.at(i); }
	};

	//! list of non-const references
	template< class T >
	class ref_list
	: public mutable_ref_sequence_container< T, std::list, bidi_ref_iterator >
	{
		private:
			typedef mutable_ref_sequence_container< T, std::list, bidi_ref_iterator > base_class;
		public:
			//! build empty
			ref_list()
			: base_class() {}

			//! copy
			ref_list( const ref_list& c )
			: base_class( c ) {}

			//! build from a list of objects
			ref_list( const std::list< T* > & c )
			: base_class( c ) { }

			//! build from a ptr list
			ref_list( const boost::ptr_list< T > & c )
			: base_class( )
			{
				BOOST_FOREACH( const T & ptr, c )
				{
					base_class::_data.push_back( &ptr );
				}
			}

		//! let the const version be friend to ease construction from non-const version
		friend class ref_list< const T >;
	};

	//! specialization for list of const references

	//! construction from non-const data is allowed at this point
	//! extends the const sequence base class specifying a const
	//! type for both the exposed reference and the inner pointers
	template< class T >
	class ref_list< const T >
	: public const_ref_sequence_container< const T, const T, std::list, bidi_ref_iterator >
	{
		private:
			typedef const_ref_sequence_container< const T, const T, std::list, bidi_ref_iterator > base_class;
		public:
			//! build empty
			ref_list()
			: base_class() {}

			//! build from a list of const references (copy)
			ref_list( const ref_list< const T >& c )
			: base_class( c ) {}

			//! build from a list of pointers to const objects
			ref_list( const std::list< const T* > & c )
			: base_class( c ) { }

			//! build from a ptr list of const objects
			ref_list( const boost::ptr_list< const T > & c )
			: base_class( )
			{
				BOOST_FOREACH( const T & ptr, c )
				{
					base_class::_data.push_back( &ptr );
				}
			}

			//! build from a list of references
			ref_list( const ref_list< T > & c )
			: base_class( )
			{
				BOOST_FOREACH( T* ptr, c._data )
				{
					base_class::_data.push_back( const_cast< const T* >(ptr) );
				}
			}

			//! build from a list of pointers
			ref_list( const std::list< T* > & c )
			: base_class( )
			{
				BOOST_FOREACH( T* ptr, c )
				{
					base_class::_data.push_back( const_cast< const T* >(ptr) );
				}
			}

			//! build from a ptr list
			ref_list( const boost::ptr_list< T > & c )
			: base_class( )
			{
				BOOST_FOREACH( const T & ptr, c )
				{
					base_class::_data.push_back( &ptr );
				}
			}
	};

/*

	template< class Ctr >
	class Iterator
	{
		typename Ctr::iterator const end;
		typename Ctr::iterator curr;
	public:
		
		Iterator( Ctr & c )
		: end( c.end() )
		, curr( c.begin() )
		{
		}

		operator bool() const
		{ return curr != end; }
	};*/
}

#endif
