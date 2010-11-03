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
 * File name: ./lib/socket.cpp
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
 *     pre-interleave
 *
 **/


#include <iostream>

#include "lib/socket.h"
#include "lib/common.h"
#include "lib/log.h"

#include <cerrno>
#include <cstring>
#include <cmath>

extern "C" {
#include <netdb.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
}

namespace KGD
{
	namespace Socket
	{
		double READ_TIMEOUT = 0.1;
		double WRITE_TIMEOUT = 0.1;
		size_t WRITE_BUFFER_SIZE = 2048;
		
		Exception::Exception() throw()
		: KGD::Exception::Generic( errno )
		{
		}

		Exception::Exception( const string & fun ) throw()
		: KGD::Exception::Generic( fun, errno )
		{
		}

		Exception::Exception( const string & fun, int errcode ) throw()
		: KGD::Exception::Generic( fun, errcode )
		{
		}

		Exception::Exception( const string & fun, const string & msg ) throw()
		: KGD::Exception::Generic( fun + ": " + msg )
		{
		}

		// ****************************************************************************************************************

		Abstract::Abstract( const int fileDescriptor ) throw( Socket::Exception )
		: _fileDescriptor( fileDescriptor )
		{
			socklen_t sz = sizeof(sockaddr_in);
			if (_fileDescriptor >= 0)
			{
				if (::getsockname(_fileDescriptor, (sockaddr *) &_local, &sz) < 0)
				{
					throw Socket::Exception( "getsockname" );
				}
			}
			else
			{
				memset(&_local, 0, sz);
			}
		}

		Abstract::Abstract( Type::type t, const TPort bindPort, const string& bindIP ) throw( Socket::Exception )
		{
			memset(&_local, 0, sizeof(sockaddr_in));
			// open socket
			if ( (_fileDescriptor = ::socket(Domain::INET, t, 0)) < 0)
			{
				throw Socket::Exception( "socket" );
			}

			// bind if told to
			if ( bindPort > 0 )
			{
				// reuse
				int boolVal = 1;
				if (::setsockopt(_fileDescriptor, SOL_SOCKET, SO_REUSEADDR, &boolVal, sizeof(int)) < 0)
					throw Socket::Exception( "setsockopt - SO_REUSEADDR" );
				
				_local = this->getAddress( bindPort, bindIP );
				if (::bind(_fileDescriptor, (sockaddr *) &_local, sizeof(sockaddr_in)) < 0)
					throw Socket::Exception( "bind" );
			}
			// else fill up _local with getsockname
			else
			{
				socklen_t sz = sizeof(sockaddr_in);
				if (getsockname(_fileDescriptor, (sockaddr *) &_local, &sz) < 0)
				{
					throw Socket::Exception( "getsockname" );
				}
			}
		}

		Abstract::~Abstract() throw()
		{
			Log::debug( "a socket is being destroyed");
			this->close();
		}

		void Abstract::close() throw( )
		{
			if (_fileDescriptor > -1)
			{
				Log::debug( "a socket is closing");

				if ( ::shutdown(_fileDescriptor, SHUT_RDWR ) < 0 || ::close(_fileDescriptor) < 0 )
				{
					Log::error( Socket::Exception( "close" ) );
				}
				else
					_fileDescriptor = -1;
			}
		}

		sockaddr_in Abstract::getAddress( const TPort port, const string& host ) const throw( Socket::Exception )
		{
			sockaddr_in addr;
			memset(&addr, 0, sizeof(sockaddr_in));

			if (host.compare("*") == 0)
			{
				addr.sin_addr.s_addr = INADDR_ANY;
			}
			else
			{
				//TODO gethostbyname is deprecated
				hostent * hostInfo = gethostbyname( host.c_str() );
				if ( hostInfo == NULL )
					throw Socket::Exception( "gethostbyname", h_errno );
				else
					memcpy( &addr.sin_addr, hostInfo->h_addr_list[0], hostInfo->h_length);
			}

			addr.sin_family = Family::INET;
			addr.sin_port   = htons(port);

			return addr;
		}


		TPort Abstract::getLocalPort() const throw()
		{
			return htons(_local.sin_port);
		}

		string Abstract::getLocalHost() const throw( Socket::Exception )
		{
			char buffer[INET_ADDRSTRLEN];
			if (::inet_ntop( Family::INET, &(_local.sin_addr), buffer, INET_ADDRSTRLEN) == NULL)
			{
				throw Socket::Exception( "inet_ntop" );
			}
			return buffer;
		}

		void Abstract::setTimeout( double sec, int param ) throw( Socket::Exception )
		{
			timeval timeout;
			timeout.tv_sec  = long(floor(sec));
			timeout.tv_usec = long((sec - floor(sec)) * 1000000);

			if (::setsockopt(_fileDescriptor, SOL_SOCKET, param, &timeout, sizeof(timeval)) < 0)
				throw Socket::Exception( "setsockopt" );
		}

		// ****************************************************************************************************************

		Reader::Reader() throw()
		: _rdBlock( true )
		{
		}

		Reader::~Reader() throw()
		{
		}


		void Reader::setReadTimeout( double sec ) throw( Socket::Exception )
		{
			Abstract::setTimeout(sec, SO_RCVTIMEO);
		}

		void Reader::setReadBlock( bool b )
		{
			_rdBlock = b;
		}
		bool Reader::isReadBlock( ) const
		{
			return _rdBlock;
		}

		ssize_t Reader::recvFromSocket( void* data, size_t len, sockaddr_in* peerAddress ) throw()
		{
			int flags = 0;

			if ( !_rdBlock )
				flags |= MSG_DONTWAIT;

			if ( peerAddress )
			{
				socklen_t sz = sizeof(sockaddr_in);
				return ::recvfrom( _fileDescriptor, data, len, flags, (sockaddr*)peerAddress, &sz );
			}
			else
			{
				return ::recvfrom( _fileDescriptor, data, len, flags, NULL, 0 );
			}
		}

		size_t Reader::readSome( void * data, size_t len ) throw( Socket::Exception )
		{
			ssize_t readBytes = this->recvFromSocket(data, len);

			if ( readBytes < 0 )
				throw Socket::Exception( "readSome" );
			else if (readBytes == 0)
				throw Socket::Exception( "readSome", "connection reset by peer");
			else
				return readBytes;
		}

		size_t Reader::readAll( void * data, size_t len ) throw( Socket::Exception )
		{
			size_t readBytes = 0;
			uint8_t *tmp = reinterpret_cast< uint8_t* >( data );

			while (readBytes < len)
				tmp += this->readSome(tmp, len - readBytes);

			return readBytes;
		}

		size_t Reader::readAll( char * data, size_t len ) throw( Socket::Exception )
		{
			// recursive

			if ( len <= 0 )
				return 0;

			size_t readBytes = this->readSome(data, len);
			if (data[readBytes - 1] != 0)
			{
				data += readBytes;
				return readBytes + this->readAll( data, len - readBytes );
			}
			else
				return readBytes;
		}

		// ****************************************************************************************************************

		Writer::Writer() throw()
		: _wrLastPacket( false )
		, _wrBlock( true )
		, _connected(false)
		{
			memset(&_remote, 0, sizeof(sockaddr_in));
		}

		Writer::~Writer() throw()
		{
		}

		void Writer::setLastPacket( bool l )
		{
			_wrLastPacket = l;
		}
		bool Writer::isLastPacket( ) const
		{
			return _wrLastPacket;
		}
		void Writer::setWriteBlock( bool b )
		{
			_wrBlock = b;
		}
		bool Writer::isWriteBlock( ) const
		{
			return _wrBlock;
		}

		void Writer::setWriteBufferSize( size_t sz ) throw( Socket::Exception )
		{
			int param = sz;
			if (::setsockopt(_fileDescriptor, SOL_SOCKET, SO_SNDBUF, &param, sizeof(int)) < 0)
				throw Socket::Exception( "setsockopt" );
		}

		void Writer::setWriteTimeout( double sec ) throw( Socket::Exception )
		{
			Abstract::setTimeout( sec, SO_SNDTIMEO );
		}

		void Writer::connectTo( sockaddr_in const * addr ) throw( Socket::Exception )
		{
			memcpy(&_remote, addr, sizeof(sockaddr_in));

			if(::connect(_fileDescriptor, (sockaddr *) &_remote, sizeof(sockaddr_in)) < 0)
			{
				throw Socket::Exception( "connectTo" );
			}
			else
			{
				_connected = true;
			}
		}

		void Writer::connectTo( const TPort port, const string & IP ) throw( Socket::Exception )
		{
			sockaddr_in addr = this->getAddress( port, IP );
			this->connectTo( &addr );
		}

		bool Writer::isConnected() const throw()
		{
			return _connected;
		}

		TPort Writer::getRemotePort() const throw()
		{
			return htons( _remote.sin_port );
		}

		string Writer::getRemoteHost() const throw( Socket::Exception )
		{
			char buffer[INET_ADDRSTRLEN];
			if (::inet_ntop( _remote.sin_family, &(_remote.sin_addr), buffer, INET_ADDRSTRLEN ) == NULL)
			{
				throw Socket::Exception( "inet_ntop" );
			}
			return buffer;
		}

		void Writer::close() throw( )
		{
			_connected = false;
			memset(&_remote, 0, sizeof(sockaddr_in));

			Abstract::close();
		}

		size_t Writer::writeSome( void const * data, size_t len ) throw( Socket::Exception )
		{
			if ( ! _connected )
				throw Socket::Exception( "writeSome", "socket is not connected to an end-point");

			int flags = 0;//MSG_NOSIGNAL;

			if ( !_wrBlock )
				flags |= MSG_DONTWAIT;

			if ( _wrLastPacket )
			{
				flags |= MSG_EOR;
				_wrLastPacket = false;
			}

			ssize_t wroteBytes = ::send( _fileDescriptor, data, len, flags );
			if ( wroteBytes < 0 )
				throw Socket::Exception( "writeSome" );
			else
				return wroteBytes;
		}

		size_t Writer::writeLast( void const * data, size_t len ) throw( Socket::Exception )
		{
			this->setLastPacket( true );
			return this->writeSome( data, len );
		}

		size_t Writer::writeAll( void const * data, size_t len ) throw( Socket::Exception )
		{
			size_t sent = 0;
			while( sent < len )
			{
				size_t wrote = this->writeSome( data, len - sent );
				data = reinterpret_cast< uint8_t const * >( data ) + wrote;
				sent += wrote;
			}

			return sent;
		}

		// ****************************************************************************************************************

		ReaderWriter::ReaderWriter() throw()
		{
		}

		ReaderWriter::~ReaderWriter() throw()
		{
		}

		Channel::Description ReaderWriter::getDescription() const
		{
			Channel::Description rt;
			rt.type = Channel::Owned;
			rt.ports = make_pair( this->getLocalPort(), this->getRemotePort() );

			return rt;
		}

		size_t ReaderWriter::readSome( void * data, size_t len ) throw( Socket::Exception )
		{
			// if connected, don't care who sends message
			if (_connected)
			{
				return Reader::readSome( data, len );
			}
			// else, first sender becomes my endpoint
			else
			{
				sockaddr_in peerAddress;

				ssize_t readBytes = this->recvFromSocket(data, len, &peerAddress);

				if ( readBytes < 0 )
					throw Socket::Exception( "readSome" );
				else if (readBytes == 0)
					throw Socket::Exception( "readSome", "connection reset by peer");

				try
				{
					this->connectTo(&peerAddress);
				}
				catch( Socket::Exception const &e)
				{
					Log::error( e );
				}

				return readBytes;
			}
		}

		// ****************************************************************************************************************

		Tcp::Tcp( const TPort bindPort, const string & bindIP ) throw( Socket::Exception )
		: Socket::Abstract( Type::TCP, bindPort, bindIP )
		{
		}

		Tcp::~Tcp() throw()
		{
		}


		Tcp::Tcp( const int fileDescriptor, sockaddr_in const * const peerAddress ) throw( Socket::Exception )
		: Socket::Abstract( fileDescriptor )
		{
			memcpy(&_remote, peerAddress, sizeof(sockaddr_in));
			_connected = true;
		}

		void Tcp::setKeepAlive( bool set ) throw( Socket::Exception )
		{
			int flag = ( set ? 1 : 0 );

			if (::setsockopt(_fileDescriptor, SOL_SOCKET, SO_KEEPALIVE, &flag, sizeof(int)) < 0)
				throw Socket::Exception( "setsockopt - SO_KEEPALIVE" );

			if ( set )
			{
				// after 5s idle, try 3 times every 10s
				int idle = 5, cnt = 3, intvl = 10;
				if (::setsockopt(_fileDescriptor, SOL_TCP, TCP_KEEPIDLE, &idle, sizeof(int)) < 0)
					throw Socket::Exception( "setsockopt - TCP_KEEPIDLE" );
				if (::setsockopt(_fileDescriptor, SOL_TCP, TCP_KEEPCNT, &cnt, sizeof(int)) < 0)
					throw Socket::Exception( "setsockopt - TCP_KEEPCNT" );
				if (::setsockopt(_fileDescriptor, SOL_TCP, TCP_KEEPINTVL, &intvl, sizeof(int)) < 0)
					throw Socket::Exception( "setsockopt - TCP_KEEPINTVL" );
			}
		}


		// ****************************************************************************************************************

		Udp::Udp( const TPort bindPort, const string & bindIP ) throw( Socket::Exception )
		: Socket::Abstract( Type::UDP, bindPort, bindIP )
		{ }

		Udp::~Udp() throw()
		{
		}

		// ****************************************************************************************************************

		TcpServer::TcpServer( TPort bindPort, const string & bindIP, const int queue ) throw( Socket::Exception )
		: Socket::Abstract( Type::TCP, bindPort, bindIP )
		{
			if (::listen(_fileDescriptor, queue) < 0)
			{
				throw Socket::Exception( "listen" );
			}
		}

		TcpServer::~TcpServer() throw()
		{
		}

		auto_ptr< Tcp > TcpServer::accept() throw( Socket::Exception )
		{
			int newsockfd;
			sockaddr_in peerAddress;
			socklen_t sz = sizeof(sockaddr_in);

			if ((newsockfd = ::accept(_fileDescriptor, (sockaddr *) &peerAddress, &sz)) <= 0)
				throw Socket::Exception( "accept" );
			else
				return auto_ptr< Tcp >( new Tcp( newsockfd, &peerAddress ) );
		}

	}

	using namespace Socket;

	template < > Reader& operator>> < > (Reader& s, string& val) throw( Socket::Exception )
	{
		const size_t len = Reader::MAX_READABLE_STRING_LEN;
		char buffer[ len + 1 ];
		s.readAll(buffer, len );
		buffer[ len ] = 0;
		val = buffer;

		return s;
	}

	template < > Writer& operator<< < > ( Writer& s, const char* val ) throw( Socket::Exception )
	{
		s.writeAll(val, 1 + strlen(val));
		return s;
	}

	template < > Writer& operator<< < > ( Writer& s, const string& val ) throw( Socket::Exception )
	{
		s.writeAll(val.c_str(), 1 + val.size());
		return s;
	}

}
