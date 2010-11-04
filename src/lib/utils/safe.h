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
 * File name: src/lib/utils/safe.h
 * First submitted: 2010-07-30
 * First submitter: Emiliano Leporati <emiliano.leporati@gmail.com>
 * Contributor(s) so far - 2010-11-04 :
 *     Emiliano Leporati <emiliano.leporati@gmail.com>
 *
 * Last changes :
 *     boosted
 *     boosted
 *     source import
 *
 **/


#ifndef _USS_SAFE_VARS_H
#define _USS_SAFE_VARS_H

#include "lib/common.h"
#include "lib/utils/safe_base.h"
#include <bitset>

namespace KGD
{
	//! Thread-safe utilities
	namespace Safe
	{
		//! Lockable flag set
		template< size_t N, class M = RMutex >
		class FlagSet
		: public LockableBase< M >
		{
		private:
			bitset< N > _bits;
		public:

			bool operator[]( size_t pos ) const							{ return _bits[pos]; }
			typename bitset< N >::reference operator[]( size_t pos )	{ return _bits[pos]; }
		};

		//! Lockable flag
		template< class M = Mutex >
		class Flag
		: private LockableBase< M >
		{
		private:
			bool _bit;
		public:

			Flag( bool b );

			operator bool() const;
			Flag& operator=( bool b );
		};

		//! Lockable thread with term barrier
		template< class M = RMutex >
		class Thread
		: public LockableBase< M >
		, public ThreadBarrier
		{
		public:
			Thread( unsigned int count = 2 ) 			: ThreadBarrier( count ) { }
		};

		//! thread-safe boolean
		typedef Flag<> Bool;
		//! unlocker for unique lock
		typedef Unlocker< KGD::Lock > UnLock;
		//! unlocker for recursive lock
		typedef Unlocker< KGD::RLock > UnRLock;
	}
}

#endif

