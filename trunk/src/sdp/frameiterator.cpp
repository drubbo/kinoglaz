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
 * File name: ./sdp/frameiterator.cpp
 * First submitted: 2010-05-12
 * First submitter: Emiliano Leporati <emiliano.leporati@gmail.com>
 * Contributor(s) so far - 2010-07-30 :
 *     Emiliano Leporati <emiliano.leporati@gmail.com>
 *
 * Last changes :
 *     
 *
 **/



#include "sdp/frameiterator.h"
#include "lib/utils/container.hpp"

namespace KGD
{
	namespace SDP
	{
		namespace Medium
		{
			namespace Iterator
			{
				Base::~Base( )
				{
				}

				void Base::insert( Medium::Base & m, double t ) throw( KGD::Exception::OutOfBounds )
				{
					Ptr::Scoped< Iterator::Base > otherFrames = m.newFrameIterator();
					this->insert( *otherFrames, t );
				}

				// ***************************************************************************************************

				Default::Default( Medium::Base & m ) throw()
				: _med( m )
				, _pos( 0 )
				{
					++ _med->_itCount;
				}

				Default::Default( const Default & it ) throw()
				: _med( it._med )
				, _pos( 0 )
				{
					++ _med->_itCount;
				}

				Default::~Default( )
				{
					-- _med->_itCount;
					_med->_condItReleased.notify_all();
				}

				Default * Default::getClone() const throw()
				{
					return new Default( *this );
				}

				const SDP::Frame::Base & Default::at( size_t pos ) const throw( KGD::Exception::OutOfBounds )
				{
					return _med->getFrame( pos );
				}
				const SDP::Frame::Base & Default::curr() const throw( KGD::Exception::OutOfBounds )
				{
					return _med->getFrame( _pos );
				}
				const SDP::Frame::Base & Default::next() throw( KGD::Exception::OutOfBounds )
				{
					return _med->getFrame( _pos ++ );
				}
				const SDP::Frame::Base & Default::seek( double t ) throw( KGD::Exception::OutOfBounds )
				{
					_pos = _med->getFramePos( t );
					return _med->getFrame( _pos );
				}

				const SDP::Frame::Base & Default::seek( size_t p ) throw( KGD::Exception::OutOfBounds )
				{
					return _med->getFrame( _pos = p );
				}

				size_t Default::pos( ) const throw( )
				{
					return _pos;
				}

				size_t Default::size( ) const throw( )
				{
					return _med->getFrameCount();
				}

				double Default::duration() const throw()
				{
					return _med->getDuration();
				}

				void Default::insert( Iterator::Base & it, double t ) throw( KGD::Exception::OutOfBounds )
				{
					_med->insert( it, t );
				}

				void Default::insert( double duration, double t ) throw( KGD::Exception::OutOfBounds )
				{
					_med->insert( duration, t );
				}

				// ***************************************************************************************************

				Slice::Slice( vector< Frame::Base * > & fs, MediaType::kind t ) throw()
				: _frames( fs )
				, _pos( 0 )
				, _duration( fs.back()->getTime() - fs.front()->getTime() )
				, _type( t )
				{
				}

				Slice::Slice( const Slice & it ) throw()
				: _pos( 0 )
				, _duration( it.duration() )
				, _type( it._type )
				{
					_frames.reserve( it.size() );
					for( size_t i = 0; i < it.size(); ++i )
						_frames.push_back( it.at( i ).getClone() );
				}

				Slice::~Slice( )
				{
					Ctr::clear( _frames );
				}

				Slice * Slice::getClone() const throw()
				{
					return new Slice( *this );
				}

				const SDP::Frame::Base & Slice::at( size_t pos ) const throw( KGD::Exception::OutOfBounds )
				{
					if ( pos < _frames.size() )
						return *_frames[ pos ];
					else
						throw KGD::Exception::OutOfBounds( pos, 0, _frames.size() );
				}
				const SDP::Frame::Base & Slice::curr() const throw( KGD::Exception::OutOfBounds )
				{
					if ( _pos < _frames.size() )
						return *_frames[ _pos ];
					else
						throw KGD::Exception::OutOfBounds( _pos, 0, _frames.size() );
				}
				const SDP::Frame::Base & Slice::next() throw( KGD::Exception::OutOfBounds )
				{
					if ( _pos < _frames.size() )
						return *_frames[ _pos ++ ];
					else
						throw KGD::Exception::OutOfBounds( _pos, 0, _frames.size() );
				}
				const SDP::Frame::Base & Slice::seek( double t ) throw( KGD::Exception::OutOfBounds )
				{
					size_t i = 0;
					while( i < _frames.size() )
					{
						if ( _frames[i]->getTime() >= t
							&& ( ( _type == SDP::MediaType::Video && _frames[i]->asPtr< SDP::Frame::MediaFile >()->isKey() )
								|| _type != SDP::MediaType::Video ) )
							break;
						else
							++i;
					}
					if ( i < _frames.size() )
						return *_frames[ _pos = i ];
					else
						throw KGD::Exception::OutOfBounds( t, 0, _duration );
				}

				const SDP::Frame::Base & Slice::seek( size_t p ) throw( KGD::Exception::OutOfBounds )
				{
					_pos = p;
					return curr();
				}

				size_t Slice::pos( ) const throw( )
				{
					return _pos;
				}

				size_t Slice::size( ) const throw( )
				{
					return _frames.size();
				}

				double Slice::duration() const throw()
				{
					return _duration;
				}

				void Slice::insert( Iterator::Base & otherFrames, double t ) throw( KGD::Exception::OutOfBounds )
				{
					size_t pos = _pos;
					// seek to guess position
					this->seek( t );
					// shift successive frames by other medium duration
					double duration = otherFrames.duration();
					vector< Frame::Base * >::iterator insIt = _frames.begin() + _pos;
					{
						vector< Frame::Base * >::iterator it = insIt, ed = _frames.end();
						for( ; it != ed; ++it )
							(*it)->addTime( duration );
					}
					// shift new frames by offset time
					vector< Frame::Base * > toInsert;
					try
					{
						for(;;)
						{
							Frame::Base * newFrame = otherFrames.next().getClone();
							newFrame->addTime( t );
							toInsert.push_back( newFrame );
						}
					}
					catch( KGD::Exception::OutOfBounds )
					{
						// and insert
						_frames.insert( insIt, toInsert.begin(), toInsert.end() );
						_duration += duration;
						// restore position
						_pos = pos;
					}
				}

				void Slice::insert( double duration, double t ) throw( KGD::Exception::OutOfBounds )
				{
					size_t pos = _pos;
					// seek to guess position
					this->seek( t );
					// shift successive frames by duration
					vector< Frame::Base * >::iterator
						it = _frames.begin() + _pos,
						ed = _frames.end();
					for( ; it != ed; ++it )
						(*it)->addTime( duration );
					// restore position
					_pos = pos;
				}

				// ***************************************************************************************************

				Loop::Loop( Iterator::Base * it, uint8_t n ) throw()
				: _it( it )
				, _times( n )
				, _cur( 0 )
				{
					Log::debug("Creating loop for %u times", n);
				}

				Loop::Loop( const Loop & it ) throw()
				: _it( it._it->getClone() )
				, _times( it._times )
				, _cur( 0 )
				{
				}

				Loop::~Loop( )
				{
					Ctr::clear( _curFrames );
				}

				Loop * Loop::getClone() const throw()
				{
					return new Loop( *this );
				}

				size_t Loop::seekPos( size_t pos ) throw( KGD::Exception::OutOfBounds )
				{
					_cur = pos / _it->size();
					return this->normalizePos( pos );
				}

				size_t Loop::normalizePos( size_t pos ) const throw( KGD::Exception::OutOfBounds )
				{
					size_t sz = _it->size();
					if ( _times == 0 || pos < sz * _times )
						return pos % sz;
					else
						throw KGD::Exception::OutOfBounds( pos, 0, sz * _times );
				}

				double Loop::seekTime( double t ) throw( KGD::Exception::OutOfBounds )
				{
					_cur = size_t( floor( t / _it->duration() ) );
					return this->normalizeTime( t );
				}

				double Loop::normalizeTime( double t ) const throw( KGD::Exception::OutOfBounds )
				{
					double duration = _it->duration();
					if ( _times == 0 || t < duration * _times )
					{
						while( t >= duration )
							t -= duration;
						return t;
					}
					else
						throw KGD::Exception::OutOfBounds( t, 0, duration * _times );
				}

				const SDP::Frame::Base & Loop::normalizeFrame( const SDP::Frame::Base & f ) const throw()
				{
					f.setDisplace( _cur * _it->duration() );
					return f;
				}

				const SDP::Frame::Base & Loop::at( size_t pos ) const throw( KGD::Exception::OutOfBounds )
				{
					return this->normalizeFrame( _it->at( this->normalizePos( pos ) ) );
				}

				const SDP::Frame::Base & Loop::curr() const throw( KGD::Exception::OutOfBounds )
				{
					return this->normalizeFrame( _it->curr() );
				}
				const SDP::Frame::Base & Loop::next() throw( KGD::Exception::OutOfBounds )
				{
					try
					{
						return this->normalizeFrame( _it->next() );
					}
					catch( KGD::Exception::OutOfBounds )
					{
						++ _cur;
						Ctr::clear( _curFrames );
						Log::debug("Loop: next %lu of %u", _cur, _times );
						if ( _times == 0 || _cur < _times )
						{
							Log::debug("Loop: reset next", _cur, _times );
							_it->seek( size_t(0) );
							return this->next();
						}
						else
							throw;
					}
				}
				const SDP::Frame::Base & Loop::seek( double t ) throw( KGD::Exception::OutOfBounds )
				{
					return this->normalizeFrame( _it->seek( this->seekTime( t ) ) );
				}

				const SDP::Frame::Base & Loop::seek( size_t p ) throw( KGD::Exception::OutOfBounds )
				{
					return this->normalizeFrame( _it->seek( this->seekPos( p ) ) );
				}

				size_t Loop::pos( ) const throw( )
				{
					return _it->pos() + (_it->size() * _cur);
				}

				size_t Loop::size( ) const throw( )
				{
					return _it->size();
				}

				double Loop::duration() const throw()
				{
					return ( _times > 0 ? _it->duration() * _times : HUGE_VAL );
				}

				void Loop::insert( Iterator::Base & otherFrames, double t ) throw( KGD::Exception::OutOfBounds )
				{
					_it->insert( otherFrames, this->seekTime( t ) );
				}

				void Loop::insert( double duration, double t ) throw( KGD::Exception::OutOfBounds )
				{
					_it->insert( duration, this->seekTime( t ) );
				}

			}
		}
	}
}
