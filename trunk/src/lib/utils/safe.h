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
#include <boost/detail/atomic_count.hpp>
#include <bitset>

namespace KGD
{
	//! Thread-safe utilities
	namespace Safe
	{

		template< class boost_lock >
		class BoostUnlocker
		: public boost::noncopyable
		{
			boost_lock & _lk;
		public:
			BoostUnlocker( boost_lock & l ) : _lk( l ) { l.unlock(); }
			~BoostUnlocker() { _lk.lock(); }
		};

		typedef BoostUnlocker< Lock > UnLock;
		typedef BoostUnlocker< RLock > UnRLock;
		
		//! Locker for any MutexLockable
		template< class L >
		class Locker
		: public boost::noncopyable
		{
			boost::scoped_ptr< L > _lk;
		public:
			template< class Lk >
			Locker( Lk const & l );

			L & getLock();
		};

		//! Temporarily unlocks a previous acquired lock
		template< class L >
		class UnLocker
		: public boost::noncopyable
		{
			Locker< L > & _lk;
		public:
			UnLocker( Locker< L > & l );
			~UnLocker( );
		};

		//! Mutex-based lockable class
		template< class M = RMutex, class L = RLock >
		class MutexLockable
		: public boost::noncopyable
		{
		private:
			mutable M _mux;
		public:
			typedef M MutexType;
			typedef L LockType;

			typedef Locker< L > LockerType;
			typedef UnLocker< L > UnLockerType;

			void lock() const;
			void unlock() const;
			M & mux() const;
		};

		//! Lockable flag set
		template< size_t N, class M = RMutex, class L = RLock >
		class FlagSet
		: public MutexLockable< M, L >
		{
		private:
			bitset< N > _bits;
		public:

			bool operator[]( size_t pos ) const;
			typename bitset< N >::reference operator[]( size_t pos );
		};

		//! Lockable single flag
		template< class M = RMutex, class L = RLock >
		class Flag
		: public MutexLockable< M, L >
		{
		private:
			bool _bit;
		public:

			Flag( bool b );

			operator bool() const;
			Flag& operator=( bool b );
		};

		//! Lockable single flag typedef sugar
		typedef Flag<> Bool;

		//! Generic lockable
		template< class T, class M = RMutex, class L = RLock >
		class Lockable
		: public MutexLockable< M, L >
		{
		protected:
			T _obj;
		public:
			
			T& operator*();
			T const & operator*() const;
		};
	}
}

#endif

