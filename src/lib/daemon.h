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
 * File name: ./lib/daemon.h
 * First submitted: 2010-01-16
 * First submitter: Emiliano Leporati <emiliano.leporati@gmail.com>
 * Contributor(s) so far - 2010-07-30 :
 *     Emiliano Leporati <emiliano.leporati@gmail.com>
 *
 * Last changes :
 *     Kinoglaz birth: pervasive bug fixing, improvements and client support
 *     sig HUP supported
 *     frame / other stuff refactor
 *     sdp debugged
 *     daemon out of RTSP ns
 *
 **/


#ifndef __KGD_DAEMON_BASE_H
#define __KGD_DAEMON_BASE_H

#include "lib/ini.h"
#include "lib/exceptions.h"

namespace KGD
{
	//! base deamon spawn class
	class AbstractDaemon
	: public boost::noncopyable
	{
	protected:
		//! reference to parameter file
		Ini::Reference _ini;
		//! deamon pidfile location
		string _pid_filename;

		//! returns pid_zero if deamon is not running, or its current pid
		pid_t checkPidfile() const throw();

		//! daemon main loop to implement: should return false if something wrong happend
		virtual bool run() throw() = 0;

		//! ctor
		AbstractDaemon( const Ini::Reference& params ) throw( Exception::NotFound );

		//! sets umask and new run-session id, returning true if everything's ok
		bool detach();

	public:
		virtual ~AbstractDaemon();

		//! fork, stdin-out-err close, write pidfile; returns false if something wrong happened
		bool start( bool doFork ) throw();

		//! setup global parameters
		virtual void setupParameters() throw( Exception::NotFound );
		//! get reference to parameters
		const Ini & getParameters() throw();

		//! returns app name and version
		virtual string getFullName() const throw() = 0;
	};
}

#endif
