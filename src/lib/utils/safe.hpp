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
		template< class L >
		template< class Lk >
		Locker< L >::Locker( Lk const & l )
		: _lk( new typename Lk::LockType( l.mux() ) )
		{ }

		template< class L >
		L & Locker< L >::getLock()
		{ return *_lk; }

		// ********************************************************************************

		template< class L >
		UnLocker< L >::UnLocker( Locker< L > & l )
		: _lk( l )
		{ _lk.getLock().unlock(); }

		template< class L >
		UnLocker< L >::~UnLocker( )
		{ _lk.getLock().lock(); }

		// ********************************************************************************

		template< class M, class L >
		void MutexLockable< M, L >::lock() const
		{ _mux.lock(); }

		template< class M, class L >
		void MutexLockable< M, L >::unlock() const
		{ _mux.unlock(); }

		template< class M, class L >
		M & MutexLockable< M, L >::mux() const
		{ return _mux; }

		// ********************************************************************************

		template< size_t N, class M, class L >
		bool FlagSet< N, M, L >::operator[]( size_t pos ) const
		{ return _bits[ pos ]; }

		template< size_t N, class M, class L >
		typename bitset< N >::reference FlagSet< N, M, L >::operator[]( size_t pos )
		{ return _bits[ pos ]; }

		// ********************************************************************************

		template< class M, class L >
		Flag< M, L >::Flag( bool b ) : _bit( b ) {}

		template< class M, class L >
		Flag< M, L >::operator bool() const
		{
			LockerType lk( *this );
			return _bit;
		}

		template< class M, class L >
		Flag< M, L >& Flag< M, L >::operator=( bool b )
		{
			LockerType lk( *this );
			_bit = b;
			return *this;
		}

		// ********************************************************************************

		template< class T, class M, class L >
		T& Lockable< T, M, L >::operator*()
		{ return _obj; }

		template< class T, class M, class L >
		T const & Lockable< T, M, L >::operator*() const
		{ return _obj; }
	}
}

#endif
