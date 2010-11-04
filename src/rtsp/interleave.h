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
 * File name: src/rtsp/interleave.h
 * First submitted: 2010-07-30
 * First submitter: Emiliano Leporati <emiliano.leporati@gmail.com>
 * Contributor(s) so far - 2010-11-04 :
 *     Emiliano Leporati <emiliano.leporati@gmail.com>
 *
 * Last changes :
 *     "would block" cleanup
 *     added param for tcp socket send buffer
 *     english comments; removed leak with connection serving threads
 *     introduced keep alive on control socket (me dumb)
 *     repo content is fine and working again
 *
 **/


#ifndef __KGD_RSTP_INTERLEAVE_H
#define __KGD_RSTP_INTERLEAVE_H

#include "lib/socket.h"
#include "rtsp/buffer.h"
#include "rtsp/ports.h"

namespace KGD
{
	namespace RTSP
	{
		typedef Safe::Lockable< boost::scoped_ptr< KGD::Socket::Tcp > > TcpTunnel;

		class Socket;
		
		//! a shell that allows RTSP interleave on the control tcp socket.

		//! output is done on socket prepended by channel identifier
		//! input comes from a buffer fed by the RTSP socket itself
		class Interleave
		: public boost::noncopyable
		, public Channel::Bi
		{
		protected:
			//! interleave channel local identifier
			TPort _channel;
			//! interleave channel remote identifier
			TPort _remote;
			//! RTSP session this interleaved channel is used by
			TSessionID _ssid;
			//! shared tcp socket
			TcpTunnel & _sock;
			//! ref to overall interleave channel
			Socket & _rtspSocket;
			//! safe receiver buffer
			typedef Safe::Lockable< boost::ptr_list< ByteArray > > InputBuffer;
			//! receive buffer
			InputBuffer _recv;
			//! read timeout
			double _rdTimeout;
			//! receive data ready
			Condition _condNotEmpty;
			//! running flag
			bool _running;
			//! log identifier
			const string _logName;

			Interleave( TPort local, TPort remote, TSessionID, TcpTunnel &, Socket & );
			friend class Socket;
		public:
			~Interleave();
			//! returns the ID of RTSP session this interleaved channel is used by
			TSessionID getSessionID() const throw();
			//! returns log identifier for this interleaved channel
			const char * getLogName() const throw();
			//! write to socket
			virtual size_t writeSome( void const *, size_t ) throw( KGD::Socket::Exception );
			//! write to socket
			virtual size_t writeLast( void const *, size_t ) throw( KGD::Socket::Exception );
			//! read from buffer
			virtual size_t readSome( void *, size_t ) throw( KGD::Socket::Exception );

			//! invalid operation
			virtual void setWriteBufferSize( size_t ) throw( );
			//! invalid operation
			virtual void setWriteBlock( bool ) throw( );
			//! invalid operation
			virtual void setWriteTimeout( double ) throw( );
			//! is write blocking
			virtual bool isWriteBlock( ) const throw();
			//! invalid operation
			virtual void setReadBlock( bool ) throw( );
			//! set read timeout
			virtual void setReadTimeout( double sec ) throw( );
			//! is read blocking
			virtual bool isReadBlock( ) const throw();
			//! get description
			virtual Channel::Description getDescription() const;
			//! push to buffer
			void pushToRead( void const *, size_t ) throw( );
			//! push to buffer
			void pushToRead( const ByteArray & ) throw( );
			//! stops using the underlying socket
			void close( ) throw( );
		};

		//! the RTSP socket abstraction. 

		//! dispatches inbound interleaves and throws new requests / responses 
		//! when told to listen
		class Socket
		: public boost::noncopyable
		, public Channel::Out
		{
		protected:
			typedef map< TPort, ref< Interleave > > ChannelMap;
			//! shared tcp socket
			TcpTunnel _sock;
			//! mutex for port assign / release
			Mutex _muxPorts;
			//! interleave shutdown wait condition
			Condition _condNoInterleave;
			//! interleave port source
			Port::Interleave _iPorts;
			//! interleave channels map
			ChannelMap _iChans;
			//! rtsp input buffer
			InputBuffer _inBuf;
			//! current message sequence
			TCseq _cseq;
			//! log identifier
			const string _logName;

			//! internal utility
			void reply( const ostringstream & o ) throw( KGD::Socket::Exception );
			
			//! will be called by Interleave destructor
			void release( TPort );

			//! local utils to get channel from port
			Interleave & getInterleaveRef( TPort p ) throw( KGD::Exception::NotFound );

			friend class Interleave;
			
		public:
			//! constructs shared socket
			Socket( auto_ptr< KGD::Socket::Tcp >, const string & );
			//! stops interleaves and destroyes socket
			~Socket();
			//! returns log identifier for this rtsp socket
			const char * getLogName() const throw();
			//! adds interleave channel
			TPort addInterleave( TPort remote, const TSessionID & ) throw( KGD::Exception::NotFound );
			//! adds interleave channel
			TPortPair addInterleavePair( const TPortPair & remote, const TSessionID & ) throw( KGD::Exception::NotFound );
			//! get interleave channel p
			boost::shared_ptr< Interleave > getInterleave( TPort p ) throw( KGD::Exception::NotFound );
			//! listen for incoming messages
			void listen() throw( Message::Request *, Message::Response *, KGD::Exception::Generic );
			//! begins shutdown of all interleaved channels
			void stopInterleaving() throw();
			//! begins shutdown of all interleaved channels for a specific RTSP session
			void stopInterleaving( const TSessionID & ) throw();
			//! write to socket
			virtual size_t writeSome( void const *, size_t ) throw( KGD::Socket::Exception );
			//! write to socket
			virtual size_t writeLast( void const *, size_t ) throw( KGD::Socket::Exception );

			//! sets write buffer size
			virtual void setWriteBufferSize( size_t ) throw( );
			//! is write blocking
			virtual bool isWriteBlock( ) const throw();
			//! sets write timeout
			virtual void setWriteTimeout( double sec ) throw( KGD::Socket::Exception );
			//! sets the socket in write-blocking mode (thread will be suspended until write has been successfully done)
			virtual void setWriteBlock( bool ) throw();

			//! reply an error
			void reply( const Error::Definition & error ) throw( KGD::Socket::Exception );
			//! reply to a method
			void reply( Method::Base & m ) throw( KGD::Socket::Exception );

			//! returns local host
			string getLocalHost() const;
			//! returns remote host
			string getRemoteHost() const;

			//! get description
			virtual Channel::Description getDescription() const;
			//! close
			virtual void close() throw();
		};
	}
}

#endif
