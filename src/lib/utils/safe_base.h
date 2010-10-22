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


#ifndef _USS_SAFE_BASE_H
#define _USS_SAFE_BASE_H

#include "lib/common.h"

namespace KGD
{
	//! Thread-safe utilities
	namespace Safe
	{
		//! thread-safe reverse scope
		template< class boost_lock >
		class Unlocker
		: public boost::noncopyable
		{
			boost_lock & _lk;
		public:
			Unlocker( boost_lock & l ) 					: _lk( l ) { l.unlock(); }
			~Unlocker() 								{ _lk.lock(); }
		};

		//! base lockable class
		template< class M = RMutex >
		class LockableBase
		: public boost::noncopyable
		{
		private:
			mutable M _mux;
		public:
			typedef M Mutex;
			typedef typename M::scoped_lock Lock;
			typedef Unlocker< typename M::scoped_lock > UnLock;

			void lock() const							{ _mux.lock(); }
			void unlock() const							{ _mux.unlock(); }

			M & mux() const								{ return _mux; }
			operator M &() const						{ return _mux; }
		};

		//! Generic lockable
		template< class T, class M = RMutex >
		class Lockable
		: public LockableBase< M >
		{
		protected:
			T _obj;
		public:			
			T& operator*()								{ return _obj; }
			T const & operator*() const					{ return _obj; }
		};

		//! thread with termination barrier
		class ThreadBarrier
		{
			KGD::Thread _th;
			KGD::Barrier _term;
		public:
			ThreadBarrier( unsigned int count = 2 ) 	: _term( count ) { }

			void wait() 								{ _term.wait(); }

			boost::thread * operator->() 				{ return _th.get(); }
			boost::thread const * operator->() const 	{ return _th.get(); }
			boost::thread & operator*() 				{ return *_th; }
			boost::thread const & operator*() const 	{ return *_th; }

			operator bool() const 						{ return bool( _th ); }

			void reset( boost::thread * t = 0 ) 		{ _th.reset( t ); }
		};

	}
}

#endif

