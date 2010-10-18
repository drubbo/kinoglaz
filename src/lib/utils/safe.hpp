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
 * File name: ./lib/utils/safe.hpp
 * First submitted: 2010-01-16
 * First submitter: Emiliano Leporati <emiliano.leporati@gmail.com>
 * Contributor(s) so far - 2010-07-30 :
 *     Emiliano Leporati <emiliano.leporati@gmail.com>
 *
 * Last changes :
 *     Kinoglaz birth: pervasive bug fixing, improvements and client support
 *     first import
 *
 **/

#ifndef _USS_SAFE_VARS_HPP
#define _USS_SAFE_VARS_HPP

#include "lib/utils/safe.h"

namespace KGD
{
	namespace Safe
	{
/*
		template< class T >
		Number< T >::Number( const Number< T > & i )
		: _val( i.getValue() )
		{
		}
		template< class T >
		template< class S >
		Number< T >::Number( const S & v )
		: _val( v )
		{
		}
		template< class T >
		template< class S >
		Number< T >::Number( const Number< S > & i )
		: _val( i.getValue() )
		{}

		template< class T >
		Number< T > Number< T >::operator++( int )
		{
			Lock lk( _mux );
			T cur = _val ++;
			return cur;
		}
		template< class T >
		Number< T >& Number< T >::operator++( )
		{
			Lock lk( _mux );
			++ _val;
			return *this;
		}
		template< class T >
		Number< T > Number< T >::operator--( int )
		{
			Lock lk( _mux );
			T cur = _val --;
			return cur;
		}
		template< class T >
		Number< T >& Number< T >::operator--( )
		{
			Lock lk( _mux );
			-- _val;
			return *this;
		}

		template< class T >
		template< class S >
		Number< T >& Number< T >::operator=( const S & v )
		{
			Lock lk( _mux );
			_val = v;
			return *this;
		}
		template< class T >
		template< class S >
		Number< T >& Number< T >::operator+=( const S & v )
		{
			Lock lk( _mux );
			_val += v;
			return *this;
		}
		template< class T >
		template< class S >
		Number< T >& Number< T >::operator-=( const S & v )
		{
			Lock lk( _mux );
			_val -= v;
			return *this;
		}
		template< class T >
		template< class S >
		Number< T >& Number< T >::operator*=( const S & v )
		{
			Lock lk( _mux );
			_val *= v;
			return *this;
		}
		template< class T >
		template< class S >
		Number< T >& Number< T >::operator/=( const S & v )
		{
			Lock lk( _mux );
			_val /= v;
			return *this;
		}


		template< class T >
		T Number< T >::getValue() const
		{
			Lock lk( _mux );
			return _val;
		}
		template< class T >
		Number< T >::operator T () const
		{
			Lock lk( _mux );
			return _val;
		}
*/
	}
}

#endif
