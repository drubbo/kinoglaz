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
 * File name: ./lib/utils/safe.h
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
 *     added licence disclaimer
 *
 **/


#ifndef _USS_SAFE_VARS_H
#define _USS_SAFE_VARS_H

#include "lib/common.h"

namespace KGD
{
	//! Thread-safe utilities
	namespace Safe
	{
		//! Thread-safe number with desired precision
		template< class T >
		class Number
		{
		protected:
			T _val;
			mutable Mutex _mux;
		public:
			Number( const Number< T >& );
			template< class S >
			Number( const Number< S >& );
			template< class S >
			Number( const S & );

			Number< T >& operator++( );
			Number< T > operator++( int );
			Number< T >& operator--( );
			Number operator--( int );

			template< class S >
			Number< T >& operator=( const S & );
			template< class S >
			Number< T >& operator+=( const S & );
			template< class S >
			Number< T >& operator-=( const S & );
			template< class S >
			Number< T >& operator*=( const S & );
			template< class S >
			Number< T >& operator/=( const S & );

			T getValue() const;
			operator T () const;
		};

		//! Thread-safe boolean flag
		class Flag
		{
		protected:
			bool _val;
			mutable Mutex _mux;
		public:
			Flag( const Flag& );
			Flag( bool );

			Flag& operator=( bool );

			bool getValue() const;
			operator bool() const;
		};
	}
}

#endif

