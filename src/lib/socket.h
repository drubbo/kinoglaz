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
 * File name: src/lib/socket.h
 * First submitted: 2010-07-30
 * First submitter: Emiliano Leporati <emiliano.leporati@gmail.com>
 * Contributor(s) so far - 2010-11-04 :
 *     Emiliano Leporati <emiliano.leporati@gmail.com>
 *
 * Last changes :
 *     "would block" cleanup
 *     added param for tcp socket send buffer
 *     minor cleanup and more robust Range / Scale support during PLAY
 *     removed magic numbers in favor of constants / ini parameters
 *     introduced keep alive on control socket (me dumb)
 *
 **/


#ifndef __KGD_SOCKET_H
#define __KGD_SOCKET_H

extern "C" {
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
}
#include <iostream>
#include "lib/common.h"
#include "lib/exceptions.h"
#include "lib/array.hpp"

namespace KGD
{
	//! network communication channel abstraction interfaces
	namespace Channel
	{
		//! channel type
		enum type
		{
			//! property of owner (i.e.: UDP channel)
			Owned,
			//! shared channel (i.e.: TCP interleave)
			Shared
		};

		//! channel description
		struct Description
		{
			//! channel type
			Channel::type type;
			//! local / remote ports
			TPortPair ports;
		};
		
		//! output, writable channel
		class Out
		{
		public:
			virtual ~Out() {}
			//! an Out channel must be able to write data
			virtual size_t writeSome( void const *, size_t ) throw( KGD::Exception::Generic ) = 0;
			//! an Out channel must be able to write data with "last packet" indication
			virtual size_t writeLast( void const *, size_t ) throw( KGD::Exception::Generic ) = 0;

			//! is able to write an array
			template< class T >
			size_t writeSome( const Array< T > & ) throw( KGD::Exception::Generic );
			//! is able to write an array as last packet
			template< class T >
			size_t writeLast( const Array< T > & ) throw( KGD::Exception::Generic );

			//! sets write buffer size
			virtual void setWriteBufferSize( size_t ) = 0;
			//! sets write timeout in seconds
			virtual void setWriteTimeout( double sec ) = 0;
			//! sets the socket in write-blocking mode (thread will be suspended until write has been successfully done)
			virtual void setWriteBlock( bool ) = 0;
			//! is write a blocking operation ?
			virtual bool isWriteBlock() const = 0;

			//! stop the channel
			virtual void close() throw() = 0;

			//! an Out channel can describe itself
			virtual Description getDescription() const = 0;
		};

		//! input, readable channel
		class In
		{
		public:
			virtual ~In() {}
			//! an In channel must be able to read data
			virtual size_t readSome(void *, size_t) throw( KGD::Exception::Generic ) = 0;
			//! an In channel must be able to read data
			virtual size_t readSome(Lock &, void *, size_t) throw( KGD::Exception::Generic );
			//! an In channel must be able to read data
			virtual size_t readSome(RLock &, void *, size_t) throw( KGD::Exception::Generic );

			//! read a byte array
			template< class T >
			size_t readSome( Array< T > & ) throw( KGD::Exception::Generic );

			//! sets read timeout in seconds
			virtual void setReadTimeout( double sec ) = 0;
			//! sets the socket to be read-block (read locks the thread until data is arrived)
			virtual void setReadBlock( bool ) = 0;
			//! tells if socket is in read-blocking mode
			virtual bool isReadBlock( ) const = 0;

			//! an In channel can describe itself
			virtual Description getDescription() const = 0;
		};

		//! bidirectional channel, composed by an In and an Out channel
		class Bi
		: virtual public In
		, virtual public Out
		{
		public:
			virtual ~Bi() {}
			//! a Bi-directional channel can describe itself
			virtual Description getDescription() const = 0;
		};
	}
	
	//! classes to support TCP / UDP connectivity
	namespace Socket
	{
		//! common global value for read timeout
		extern double READ_TIMEOUT;
		//! common global value for write timeout
		extern double WRITE_TIMEOUT;
		//! common global value for write timeout
		extern size_t WRITE_BUFFER_SIZE;

		//! socket exceptions
		class Exception:
			public KGD::Exception::Generic
		{
		public:
			//! default ctor ( message is last error message )
			Exception() throw();
			//! ctor with function name causing error ( plus implied last error message )
			Exception( const string & fun ) throw();
			//! ctor with function name causing error and error number
			Exception( const string & fun, int errcode ) throw();
			//! ctor with function name and custom error message
			Exception( const string & fun, const string & msg ) throw();
			//! tells if error code is AGAIN / WOULD BLOCK
			bool wouldBlock() const throw();
		};

		//! base socket class
		class Abstract
		: public boost::noncopyable
		{
		public:
			//! valid socket types
			struct Type
			{
				typedef int type;
				static const type TCP = SOCK_STREAM;
				static const type UDP = SOCK_DGRAM;
			};
			//! valid socket domains
			struct Domain
			{
				typedef int type;
				static const type INET = PF_INET;
			};
			//! valid socket families
			struct Family
			{
				typedef int type;
				static const type INET = AF_INET;
				static const type LOCAL = AF_LOCAL;
			};

		protected:

			//! socket file descriptor
			int  _fileDescriptor;
			//! local address
			sockaddr_in _local;

			//! default ctor

			//! if file_descriptor is -1, then the socket is disconnected
			//! else is connected and we can load local address info
			Abstract( int fileDescriptor = -1 ) throw( Socket::Exception );

			//! opens a new socket potentially bound to a local address
			Abstract( Type::type t, TPort bindPort, const string& bindIP) throw( Socket::Exception );

			//! converts a port / ip to a sockaddr_in structure; ip = "*" means "INADDR_ANY"
			sockaddr_in getAddress( TPort port, const string& hostName ) const throw( Socket::Exception );

			//! sets socket timeout for read or write depending on param ( see setsockopt )
			void setTimeout( double sec, int param) throw( Socket::Exception );

		public:
			//! dtor, closes the socket
			virtual ~Abstract() throw();
			//! closes the socket
			virtual void close() throw( );

			//! returns local bind port
			TPort getLocalPort() const throw();
			//! returns local bind IP / hostname
			string getLocalHost() const throw( Socket::Exception );
		};


		//! Socket reader: abstract class, implements read logic and practices
		class Reader
		: virtual public Abstract
		, virtual public Channel::In
		{
		public:
			static const unsigned int MAX_READABLE_STRING_LEN = 4096;

		protected:
			//! socket in read-blocking mode ?
			bool _rdBlock;
			//! ctor
			Reader() throw();

			//! 'recvfrom' wrapper
			ssize_t recvFromSocket( void *, size_t, sockaddr_in * peer_addr = 0) throw();

		public:
			//! dtor
			virtual ~Reader() throw();

			//! read implementation
			virtual size_t readSome( void *, size_t ) throw( Socket::Exception );
			//! reads a variant number of bytes, all of specified
			virtual size_t readAll( void *, size_t ) throw( Socket::Exception);
			//! reads a variant number of string characters, at most those specified, or to first \0
			virtual size_t readAll( char *, size_t ) throw( Socket::Exception );

			//! read all a byte array
			template< class T >
			size_t readAll( Array< T > & ) throw( Socket::Exception);

			//! sets read timeout in seconds
			virtual void setReadTimeout( double sec ) throw( Socket::Exception );
			//! sets the socket to be read-block (read locks the thread until data is arrived)
			virtual void setReadBlock( bool );
			//! tells if socket is in read-blocking mode
			virtual bool isReadBlock( ) const;
		};


		//! Socket writer: abstract class, implements write logic and practices
		class Writer
		: virtual public Abstract
		, virtual public Channel::Out
		{
		protected:
			//! next packet will be last of a sequence ?
			bool _wrLastPacket;
			//! is socket in write-blocking mode ?
			bool _wrBlock;
			//! tells if socket is connected to remote end-point
			bool _connected;
			//! remote end-point address
			sockaddr_in _remote;

			//! ctor
			Writer() throw();

			//! connects to an end-point
			void connectTo( const sockaddr_in * addr ) throw( Socket::Exception );

		public:
			//! dtor
			virtual ~Writer() throw();
			//! closes the socket, both ways
			virtual void close() throw( );

			//! connects to an end-point
			void connectTo( TPort port, const string & host = "127.0.0.1" ) throw( Socket::Exception );

			//! 'send' wrapper
			virtual size_t writeSome(void const *, size_t) throw( Socket::Exception );
			//! 'send' wrapper
			virtual size_t writeLast(void const *, size_t) throw( Socket::Exception );
			//! sends generic data to the connected end-point, all of them
			virtual size_t writeAll( void const * , size_t ) throw( Socket::Exception );

			//! send a whole byte array
			template< class T >
			size_t writeAll( const Array< T > & ) throw( Socket::Exception );

			//! returns remote end-point port
			TPort getRemotePort() const throw();
			//! returns remote end-point IP / hostname
			string getRemoteHost() const throw( Socket::Exception );

			//! tells if socket is actually connected to an end-point
			bool isConnected() const throw();

			//! sets write buffer size
			void setWriteBufferSize( size_t sz ) throw( Socket::Exception );
			//! sets write timeout
			virtual void setWriteTimeout( double sec ) throw( Socket::Exception );
			//! sets the socket in write-blocking mode (thread will be suspended until write has been successfully done)
			virtual void setWriteBlock( bool );
			//! tells if the socket is in write-blocking mode
			virtual bool isWriteBlock( ) const;
			//! sets "last packet of sequence" flag
			virtual void setLastPacket( bool = true );
			//! tells if next packet will be last in sequence
			virtual bool isLastPacket() const;
		};

		//! Socket reader/writer: redefines read to allow successive write without explicit connectTo calls
		class ReaderWriter
		: public Reader
		, public Writer
		, virtual public Channel::Bi
		{
		protected:

			//! ctor
			ReaderWriter() throw();
		public:
			//! dtor
			virtual ~ReaderWriter() throw();
			//! in this override, if socket is not connected, a connection is done to the sender of the received data
			virtual size_t readSome( void *, size_t ) throw( Socket::Exception );
			//! Channel implementation
			virtual Channel::Description getDescription() const;
		};

		//! TCP Socket
		class Tcp
		: public ReaderWriter
		{
		protected:
			//! protected ctor used by TcpServer during accept
			Tcp( int file_descriptor, sockaddr_in const * peerAddress ) throw( Socket::Exception );

			friend class TcpServer;
		public:
			//! opens a new socket potentially bound to a local address
			Tcp( TPort bindPort = 0, const string & bindIP = "*" ) throw( Socket::Exception );
			//! dtor
			virtual ~Tcp() throw();

			void setKeepAlive( bool ) throw( Socket::Exception );
		};
		
		//! UDP Socket
		class Udp
		: public ReaderWriter
		{
		public:
			//! opens a new socket potentially bound to a local address
			Udp( TPort bindPort = 0, const string & bindIP = "*") throw( Socket::Exception );
			//! dtor
			virtual ~Udp() throw();
		};


		//! TCP socket server: accepts inbound connections giving out TCP sockets
		class TcpServer
		: public Abstract
		{
		public:
			//! opens a socket bound to a local port and sets up listen queue
			TcpServer( TPort bind_port, const string & ip = "*", int queue = 5 ) throw( Socket::Exception );
			virtual ~TcpServer() throw();

			//! waits for a connection and returns it's correspondent socket
			auto_ptr< Tcp > accept() throw( Socket::Exception );
		};
	}

	template< class T >
	size_t Channel::Out::writeSome( const Array< T > & b ) throw( KGD::Exception::Generic )
	{
		return this->writeSome( b.get(), b.size() );
	}
	template< class T >
	size_t Channel::Out::writeLast( const Array< T > & b ) throw( KGD::Exception::Generic )
	{
		return this->writeLast( b.get(), b.size() );
	}
	template< class T >
	size_t Channel::In::readSome( Array< T > & b ) throw( KGD::Exception::Generic )
	{
		return this->readSome( b.get(), b.size() );
	}

	template< class T >
	size_t Socket::Reader::readAll( Array< T > & b ) throw( Socket::Exception )
	{
		return this->readAll( b.get(), b.size() );
	}

	template< class T >
	size_t Socket::Writer::writeAll( const Array< T > & b ) throw( Socket::Exception )
	{
		return this->writeAll( b.get(), b.size() );
	}

	//! generic extractor
	template < class T > Socket::Reader& operator>>( Socket::Reader& s, T& val ) throw( Socket::Exception )
	{
		s.readAll((void*)(&val), sizeof(T));
		return s;
	}
	//! Generic pointer extractor - prohibited
	template < class T > Socket::Reader& operator>>( Socket::Reader& s, T* val ) throw( Socket::Exception )
	{
		throw Socket::Exception( "operator>>", "invalid pointer type");
	}
	//! String extractor
	template < > Socket::Reader& operator>> < > ( Socket::Reader& s, string& val ) throw( Socket::Exception );


	//! Generic pointer insertor
	template < class T > Socket::Writer& operator<<( Socket::Writer& s, T& val ) throw( Socket::Exception )
	{
		s.writeAll((void*)(&val), sizeof(T));
		return s;
	}
	//! Generic pointer insertor - prohibited
	template < class T > Socket::Writer& operator<<( Socket::Writer& s, T* val) throw( Socket::Exception )
	{
		throw Socket::Exception( "operator<<", "invalid pointer type");
	}
	//! char* insertor
	template < > Socket::Writer& operator<< < > ( Socket::Writer& s, const char* val ) throw( Socket::Exception );
	//! std::tring insertor
	template < > Socket::Writer& operator<< < > ( Socket::Writer& s, const string& val ) throw( Socket::Exception );
}

#endif
