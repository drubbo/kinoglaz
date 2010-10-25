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
 * File name: ./daemon.h
 * First submitted: 2010-01-16
 * First submitter: Emiliano Leporati <emiliano.leporati@gmail.com>
 * Contributor(s) so far - 2010-07-30 :
 *     Emiliano Leporati <emiliano.leporati@gmail.com>
 *
 * Last changes :
 *     Kinoglaz birth: pervasive bug fixing, improvements and client support
 *     fixed some bug in Log and in insertMedia; comments
 *     sig HUP supported
 *     frame / other stuff refactor
 *     sdp debugged
 *
 **/


#ifndef __KGD_DAEMON_H
#define __KGD_DAEMON_H

#include "rtsp/common.h"
#include "lib/daemon.h"
#include "lib/utils/singleton.hpp"

//! Kinoglaz Streaming Server
namespace KGD
{
	//! Kinoglaz Daemon
	class Daemon :
		public KGD::AbstractDaemon,
		public Singleton::Class< Daemon >
	{
	protected:
		//! instance the rtsp server and start it
		virtual bool run() throw();
		//! construct with parameter file
		Daemon( const Ini::Reference & params ) throw( KGD::Exception::NotFound );
		friend class Singleton::Class< Daemon >;
	public:
		//! daemon must not have been instanced already - first call
		static Daemon::Reference getInstance( const Ini::Reference & ) throw( KGD::Exception::InvalidState, KGD::Exception::NotFound );
		//! daemon must have been instanced already
		static Daemon::Reference getInstance() throw( KGD::Exception::InvalidState );
		//! reload param file and assign values to static variables here and there
		virtual void setupParameters() throw( KGD::Exception::NotFound );
		//! compose name with app name and version
		virtual string getFullName() const throw();
		//! compose name with app name and version
		static string getName() throw();

		//! Signal handler
		class SigHandler
		{
		private:
			//! terminates the server
			static void term(int);
			//! forces server to reload parameters
			static void hup(int);
		public:
			//! sets up signal mask and handlers
			static void setup();
		};

	};
}

#endif
