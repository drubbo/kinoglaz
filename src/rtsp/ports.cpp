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
 * File name: ./rtsp/ports.cpp
 * First submitted: 2010-02-07
 * First submitter: Emiliano Leporati <emiliano.leporati@gmail.com>
 * Contributor(s) so far - 2010-07-30 :
 *     Emiliano Leporati <emiliano.leporati@gmail.com>
 *
 * Last changes :
 *     Kinoglaz birth: pervasive bug fixing, improvements and client support
 *     sdp debugged
 *     interleave ok
 *     pre-interleave
 *
 **/


#include "rtsp/ports.h"

namespace KGD
{
	namespace RTSP
	{
		namespace Port
		{
			Pool::Pool( TPort first, TPort last )
			: _first( first )
			, _last( last )
			{
				for( TPort p = _first; p <= _last; ++p )
					_free.insert( p );
			}

			void Pool::reset( TPort first, TPort last )
			{
				// update range
				_first = first;
				_last = last;
				// rebuild free list
				_free.clear();
				for( TPort p = _first; p <= _last; ++p )
					// do not put used ports in free list
					if ( _used.find( p ) == _used.end() )
						_free.insert( p );
			}

			TPort Pool::getOne() throw( KGD::Exception::NotFound )
			{
				if ( _free.empty() )
					throw KGD::Exception::NotFound("free port in [" + toString( _first ) + "-" + toString( _last ) + "]");
				else
				{
					TPort rt = *_free.begin();
					_free.erase( rt );
					_used.insert( rt );
					return rt;
				}
			}

			TPortPair Pool::getPair() throw( KGD::Exception::NotFound )
			{
				TPortPair rt;
				set< TPort >::iterator
					it = _free.begin(),
					ed = _free.end();
				for(;;)
				{
					// search an even port
					while( it != ed && *it % 2 != 0 )
						++ it;
					if ( it == ed )
						throw KGD::Exception::NotFound("free port pair in [" + toString( _first ) + "-" + toString( _last ) + "]");
					// save even and advance
					rt.first = *(it ++);
					if ( it == ed )
						throw KGD::Exception::NotFound("free port pair in [" + toString( _first ) + "-" + toString( _last ) + "]");
					// if contiguous, return
					if ( *it == rt.first + 1 )
					{
						rt.second = *it;
						_free.erase( rt.first );
						_free.erase( rt.second );
						_used.insert( rt.first );
						_used.insert( rt.second );
						return rt;
					}
				}
			}

			void Pool::release( TPort p )
			{
				_used.erase( p );
				if ( p >= _first && p <= _last )
					_free.insert( p );
			}
			void Pool::release( const TPortPair p )
			{
				_used.erase( p.first );
				_used.erase( p.second );
				if ( p.first >= _first && p.first <= _last )
					_free.insert( p.first );
				if ( p.second >= _first && p.second <= _last )
					_free.insert( p.second );
			}

			// ***************************************************************************************************************

			Interleave::Interleave()
			: Pool( 0, 255 )
			{
			}

			// ***************************************************************************************************************

			TPort Udp::FIRST = 30000;
			TPort Udp::LAST = 40000;

			Udp::Udp()
			: Pool( FIRST, LAST )
			{
			}

			TPort Udp::getOne() throw( KGD::Exception::NotFound )
			{
				RLock lk( Udp::mux() );
				return Pool::getOne();
			}
			TPortPair Udp::getPair() throw( KGD::Exception::NotFound )
			{
				RLock lk( Udp::mux() );
				return Pool::getPair();
			}
			void Udp::release( TPort p )
			{
				RLock lk( Udp::mux() );
				Pool::release( p );
			}
			void Udp::release( const TPortPair p )
			{
				RLock lk( Udp::mux() );
				Pool::release( p );
			}


		}
	}
}
