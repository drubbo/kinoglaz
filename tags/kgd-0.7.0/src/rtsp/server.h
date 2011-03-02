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
 * File name: src/rtsp/server.h
 * First submitted: 2010-07-30
 * First submitter: Emiliano Leporati <emiliano.leporati@gmail.com>
 * Contributor(s) so far - 2010-11-04 :
 *     Emiliano Leporati <emiliano.leporati@gmail.com>
 *
 * Last changes :
 *     english comments; removed leak with connection serving threads
 *     testing interrupted connections
 *     boosted
 *     source import
 *
 **/


#ifndef __KGD_RTSP_SERVER_H
#define __KGD_RTSP_SERVER_H

#include "lib/utils/singleton.hpp"
#include "lib/common.h"
#include "lib/socket.h"
#include "lib/ini.h"
#include "lib/utils/safe.hpp"

#include <vector>
#include <string>

namespace KGD
{
	namespace RTSP
	{
		class Connection;

		//! RTSP (socket) server
		class Server
		: public Singleton::Class< Server >
		{
		protected:
			//! tcp server socket
			boost::scoped_ptr< KGD::Socket::TcpServer > _socket;
			//! listen ip
			string _ip;
			//! listen port
			TPort _port;

			//! dead connection cleaner
			class Connections
			{
				Server & _srv;
				//! dead connection cleanup loop
				void run();
			public:
				typedef boost::ptr_list< Connection > List;
				//! max servable requests
				uint16_t max;
				//! active requests
				List list;

				//! thread
				Safe::ThreadBarrier th;
				//! sleep condition
				Condition wakeup;

				Connections( Server & );
				void start();
			} _conns;

			//! is main loop running ?
			Safe::Bool _running;

			//! handle new connection starting new serve-thread if connection limit has not been reached
			void handle( auto_ptr< KGD::Socket::Tcp > channel );

			//! daemon loop: waits for TCP incoming connections and serves them
			void run();

			//! ctor
			Server( const Ini::Entries & params ) throw ( KGD::Exception::NotFound, KGD::Socket::Exception );
			friend class Singleton::Class< Server >;
			friend class Connections;
		public:
			//! dtor
			~Server();

			//! loads or reloads parameters from ini entries
			void setupParameters( const Ini::Entries & params ) throw ( KGD::Exception::NotFound, KGD::Socket::Exception );
			//! starts main loop
			void start();
			//! informs main loop to exit at next iteration
			void stop();

			//! get first instance
			static Server::Reference getInstance( const Ini::Entries & ) throw( KGD::Exception::Generic );
			//! get next instance references
			static Server::Reference getInstance() throw( KGD::Exception::InvalidState );
		};
	}
}

#endif
