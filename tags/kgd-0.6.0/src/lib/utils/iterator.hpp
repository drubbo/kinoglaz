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
 * File name: ./lib/utils/iterator.hpp
 * First submitted: 2010-07-13
 * First submitter: Emiliano Leporati <emiliano.leporati@gmail.com>
 * Contributor(s) so far - 2010-07-30 :
 *     Emiliano Leporati <emiliano.leporati@gmail.com>
 *
 * Last changes :
 *     
 *
 **/

#ifndef __KGD_ITERATOR_HPP
#define __KGD_ITERATOR_HPP

#include "lib/utils/iterator.h"
#include <algorithm>
#include <typeinfo>

namespace KGD
{
	namespace Ctr
	{
		// ************************************************************************************************************

		template< class C, class IT, class Ret >
		IteratorBase< C, IT, Ret >::IteratorBase( IT bg, IT ed, size_t sz )
			: _it( bg )
			, _bg( bg )
			, _ed( ed )
			, _n( sz )
		{

		}

		template< class C, class IT, class Ret >
		IteratorBase< C, IT, Ret > & IteratorBase< C, IT, Ret >::first() throw()
		{
			_it = _bg;
			return *this;
		}

		template< class C, class IT, class Ret >
		IteratorBase< C, IT, Ret > & IteratorBase< C, IT, Ret >::next() throw( Exception::OutOfBounds )
		{
			if ( _it == _ed )
				throw Exception::OutOfBounds( _n + 1, 0, _n );
			else
			{
				++ _it;
				return *this;
			}
		}

		template< class C, class IT, class Ret >
		bool IteratorBase< C, IT, Ret >::isValid() const throw( )
		{
			return ( _it != _ed );
		}

		template< class C, class IT, class Ret >
		IteratorBase< C, IT, Ret >::operator IT() const throw()
		{
			return _it;
		}

		template< class C, class IT, class Ret >
		size_t IteratorBase< C, IT, Ret >::size() const throw( )
		{
			return _n;
		}

		template< class C, class IT, class Ret >
		Ret & IteratorBase< C, IT, Ret >::val() const throw( Exception::NullPointer )
		{
			return *( this->operator->() );
		}

		template< class C, class IT, class Ret >
		Ret & IteratorBase< C, IT, Ret >::operator*() const throw( Exception::NullPointer )
		{
			return *( this->operator->() );
		}

		template< class C, class IT, class Ret >
		Ret * IteratorBase< C, IT, Ret >::operator->() const throw( Exception::NullPointer )
		{
			if ( this->_it == this->_ed )
				throw Exception::NullPointer( "iterator for a " + string(typeid( C ).name()) );
			else
				return &(*this->_it);
		}

		// ************************************************************************************************************

		template< class C, class IT, class Ret, class Srch >
		IteratorSeekBase< C, IT, Ret, Srch >::IteratorSeekBase( IT bg, IT ed, size_t sz )
			: IteratorBase< C, IT, Ret >( bg, ed, sz )
		{
		}


		template< class C, class IT, class Ret, class Srch >
		IteratorSeekBase< C, IT, Ret, Srch > & IteratorSeekBase< C, IT, Ret, Srch >::find( const Srch & v ) throw( )
		{
			this->_it = std::find( this->_bg, this->_ed, v );
			return *this;
		}

		template< class C, class IT, class Ret, class Srch >
		bool IteratorSeekBase< C, IT, Ret, Srch >::has( const Srch & v ) const throw( )
		{
			return ( std::find( this->_bg, this->_ed, v ) != this->_ed );
		}

		// ************************************************************************************************************

		template< class C >
		Iterator< C >::Iterator( C & c )
			: IteratorBase<
				C, typename C::iterator,
				typename C::value_type
			>( c.begin(), c.end(), c.size() )
		{
		}

		template< class C >
		Iterator< C > & Iterator< C >::first() throw()
		{
			IteratorBase< C, typename C::iterator, typename C::value_type >::first();
			return *this;
		}

		template< class C >
		Iterator< C > & Iterator< C >::next() throw( KGD::Exception::OutOfBounds )
		{
			IteratorBase< C, typename C::iterator, typename C::value_type >::next();
			return *this;
		}

		// ************************************************************************************************************

		template< class C >
		ConstIterator< C >::ConstIterator( const C & c )
			: IteratorBase<
				C, typename C::const_iterator,
				const typename C::value_type
			>( c.begin(), c.end(), c.size() )
		{
		}

		template< class C >
		ConstIterator< C > & ConstIterator< C >::first() throw()
		{
			IteratorBase< C, typename C::const_iterator, const typename C::value_type >::first();
			return *this;
		}

		template< class C >
		ConstIterator< C > & ConstIterator< C >::next() throw( KGD::Exception::OutOfBounds )
		{
			IteratorBase< C, typename C::const_iterator, const typename C::value_type >::next();
			return *this;
		}

		// ************************************************************************************************************

		template< class C >
		SeekableIterator< C >::SeekableIterator( C & c )
			: IteratorSeekBase<
				C, typename C::iterator,
				typename C::value_type,
				typename C::value_type
			>( c.begin(), c.end(), c.size() )
		{
		}

		template< class C >
		SeekableIterator< C > & SeekableIterator< C >::first() throw()
		{
			IteratorSeekBase< C, typename C::iterator, typename C::value_type, typename C::value_type >::first();
			return *this;
		}

		template< class C >
		SeekableIterator< C > & SeekableIterator< C >::next() throw( KGD::Exception::OutOfBounds )
		{
			IteratorSeekBase< C, typename C::iterator, typename C::value_type, typename C::value_type >::next();
			return *this;
		}

		// ************************************************************************************************************

		template< class C >
		ConstSeekableIterator< C >::ConstSeekableIterator( const C & c )
			: IteratorSeekBase<
				C, typename C::const_iterator,
				const typename C::value_type,
				typename C::value_type
			>( c.begin(), c.end(), c.size() )
		{
		}


		template< class C >
		ConstSeekableIterator< C > & ConstSeekableIterator< C >::first() throw()
		{
			IteratorSeekBase< C, typename C::const_iterator, const typename C::value_type, typename C::value_type >::first();
			return *this;
		}

		template< class C >
		ConstSeekableIterator< C > & ConstSeekableIterator< C >::next() throw( KGD::Exception::OutOfBounds )
		{
			IteratorSeekBase< C, typename C::const_iterator, const typename C::value_type, typename C::value_type >::next();
			return *this;
		}
	}
}

#endif
