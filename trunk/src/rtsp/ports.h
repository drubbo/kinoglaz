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
 * File name: ./rtsp/ports.h
 * First submitted: 2010-02-07
 * First submitter: Emiliano Leporati <emiliano.leporati@gmail.com>
 * Contributor(s) so far - 2010-07-30 :
 *     Emiliano Leporati <emiliano.leporati@gmail.com>
 *
 * Last changes :
 *     Kinoglaz birth: pervasive bug fixing, improvements and client support
 *     sdp debugged
 *     sdp threaded loader. sync pause
 *     interleave ok
 *     pre-interleave
 *
 **/


#ifndef __KGD_RTSP_PORTS_H
#define __KGD_RTSP_PORTS_H

#include "lib/exceptions.h"
#include "lib/utils/singleton.hpp"
#include "lib/utils/safe.hpp"

#include <set>

using namespace std;

namespace KGD
{
	namespace RTSP
	{
		//! UDP / Interleave port pools
		namespace Port
		{
			//! port or interleaved channels pool
			class Pool
			{
			protected:
				//! first available port in range
				TPort _first;
				//! last available port in range
				TPort _last;
				//! set of free ports
				set< TPort > _free;
				//! set of used ports
				set< TPort > _used;
			public:
				//! build with a given availability range
				Pool( TPort, TPort );
				//! get a single port
				virtual TPort getOne() throw( KGD::Exception::NotFound );
				//! get a port pair (even the first, odd the second)
				virtual TPortPair getPair() throw( KGD::Exception::NotFound );
				//! release a port, so it becomes available again
				virtual void release( TPort );
				//! release a port pair, so they become available again
				virtual void release( const TPortPair );
				//! set new range and reset free list
				void reset( TPort, TPort );
			};

			//! interleaved channels pool, one per RTSP socket; range 0-255
			class Interleave
			: public Pool
			{
			public:
				//! build with default interleave range
				Interleave();
			};

			//! udp shared port pool
			class Udp
			: public Pool
			, public Singleton::Class< Udp >
			{
			protected:
				Udp();
				friend class Singleton::Class< Udp >;
			public:
				//! first UDP available port: can be reset from parameters
				static TPort FIRST;
				//! last UDP available port: can be reset from parameters
				static TPort LAST;

				//! get one port, interlocked
				virtual TPort getOne() throw( KGD::Exception::NotFound );
				//! get port pair, interlocked
				virtual TPortPair getPair() throw( KGD::Exception::NotFound );
				//! release one port, interlocked
				virtual void release( TPort );
				//! release port pair, interlocked
				virtual void release( const TPortPair );
			};
		}
	}
}

#endif
