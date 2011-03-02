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
 * File name: src/lib/utils/safe_base.h
 * First submitted: 2010-10-22
 * First submitter: Emiliano Leporati <emiliano.leporati@gmail.com>
 * Contributor(s) so far - 2010-11-04 :
 *     Emiliano Leporati <emiliano.leporati@gmail.com>
 *
 * Last changes :
 *     threads terminate with wait + join
 *     testing against a "speed crash"
 *     testing interrupted connections
 *
 **/


#ifndef _USS_SAFE_BASE_H
#define _USS_SAFE_BASE_H

#include "lib/common.h"

namespace KGD
{
	//! Thread-safe utilities
	namespace Safe
	{
		//! thread-safe reverse lock scope
		template< class boost_lock >
		class Unlocker
		: public boost::noncopyable
		{
			boost_lock & _lk;
		public:
			Unlocker( boost_lock & l ) 					: _lk( l ) { l.unlock(); }
			~Unlocker() 								{ _lk.lock(); }
		};

		//! base class for lockable classes
		template< class M = RMutex >
		class LockableBase
		: public boost::noncopyable
		{
		private:
			//! inner mutex
			mutable M _mux;
		public:
			typedef M Mutex;
			typedef typename M::scoped_lock Lock;
			typedef typename M::scoped_try_lock TryLock;
			typedef Unlocker< typename M::scoped_lock > UnLock;

			//! locks the object
			void lock() const							{ _mux.lock(); }
			//! unlocks the object
			void unlock() const							{ _mux.unlock(); }

			//! returns inner mutex
			M & mux() const								{ return _mux; }
			//! casts the object to its inner mutex type
			operator M &() const						{ return _mux; }
		};

		//! Typed wrapper for lockable classes
		template< class T, class M = RMutex >
		class Lockable
		: public LockableBase< M >
		{
		protected:
			//! inner object
			T _obj;
		public:
			//! dereference to the inner object
			T& operator*()								{ return _obj; }
			//! const dereference to the inner object
			T const & operator*() const					{ return _obj; }
		};

		//! thread with termination barrier
		class ThreadBarrier
		{
		private:
			//! tells if this thread is currently interruptible
			bool _interruptible;
		protected:
			//! thread instance
			KGD::Thread _th;
			//! barrier instance
			KGD::Barrier _term;
		public:
			//! represents a scope inside whom a ThreadBarrier is interruptible
			class Interruptible
			{
				ThreadBarrier & _th;
			public:
				Interruptible( ThreadBarrier & th ) : _th( th ) { _th._interruptible = true; }
				~Interruptible()								{ _th._interruptible = false; }
			};
			friend class Interruptible;

			//! build with a barrier count
			ThreadBarrier( unsigned int count = 2 ) 	: _term( count ) { }
			//! wait at the barrier
			void wait() 								{ _term.wait(); }
			//! tells if the thread has been instantiated or not
			operator bool() const 						{ return bool( _th ); }
			//! sets inner thread to some loop function
			void reset( boost::thread * t ) 			{ BOOST_ASSERT( !_th ); _th.reset( t ); }
			//! safely terminates and destroys the thread
			void reset()
			{
				if ( _th )
				{
					_term.wait();
					_th->join();
					_th.reset();
				}
			}
			//! interrupts the thread if it is in an Interruptible scope
			void interrupt()							{ if ( _interruptible ) _th->interrupt(); }
			//! unlocks the lock and waits at the barrier, relocking after wait has complete
			template< class L >
			void wait( L & lk )							{ Unlocker< L > ulk( lk ); _term.wait(); }
			//! unlocks the lock and yields the thread, relocking after yield has complete
			template< class L >
			void yield( L & lk )						{ Unlocker< L > ulk( lk ); _th->yield(); }
			//! unlocks the lock and sleeps, relocking at wakeup
			template< class L >
			void sleepSec( L & lk, double sec )			{ Unlocker< L > ulk( lk ); _th->sleep( boost::get_system_time() + boost::posix_time::seconds( sec ) ); }
			//! unlocks the lock and sleeps, relocking at wakeup
			template< class L >
			void sleepNano( L & lk, uint64_t nano )		{ Unlocker< L > ulk( lk ); _th->sleep( boost::get_system_time() + boost::posix_time::microseconds( nano / 1000 ) ); }
		};
	}
}

#endif

