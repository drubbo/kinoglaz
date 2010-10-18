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
 * File name: ./lib/daemon.cpp
 * First submitted: 2010-01-16
 * First submitter: Emiliano Leporati <emiliano.leporati@gmail.com>
 * Contributor(s) so far - 2010-07-30 :
 *     Emiliano Leporati <emiliano.leporati@gmail.com>
 *
 * Last changes :
 *     Kinoglaz birth: pervasive bug fixing, improvements and client support
 *     sig HUP supported
 *     safe strerror
 *     frame / other stuff refactor
 *     sdp debugged
 *
 **/


#include <iostream>
#include <fstream>
#include <cerrno>
#include "common.h"
#include "daemon.h"

extern "C" {
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <signal.h>
}

namespace KGD
{

	AbstractDaemon::AbstractDaemon( const Ini::Reference & params ) throw( Exception::NotFound )
	: _ini( params )
	, _pid_filename( (*_ini)("DAEMON", "pidfile") )
	{

	}

	AbstractDaemon::~AbstractDaemon()
	{
	}

	bool AbstractDaemon::detach( )
	{
		// umask
		umask(0);
		// session id
		if ( setsid() < PID_Z )
		{
			cerr << "Error during setsid: " << cStrError(errno) << endl;
			return false;
		}

		return true;
	}

	pid_t AbstractDaemon::checkPidfile() const throw()
	{
		ifstream pid_file(_pid_filename.c_str());

		cout << "Checking pidfile " << _pid_filename << " ..." << endl;

		if (pid_file.good())
		{
			pid_t pid = 0;
			pid_file >> pid;
			// alive test
			if (::kill(pid, 0) == 0 || errno == EPERM)
			{
				return pid;
			}
		}

		return PID_Z;
	}

	void AbstractDaemon::setupParameters() throw( Exception::NotFound )
	{
	}

	const Ini & AbstractDaemon::getParameters() throw()
	{
		return *_ini;
	}


	bool AbstractDaemon::start( bool doFork ) throw()
	{
		cout << endl << "Spawning " << this->getFullName() << endl << endl;

		// check pid file
		pid_t pid = this->checkPidfile();
		if (pid != PID_Z)
		{
			cerr << "Daemon already running with pid " << pid << endl;
			return false;
		}

		// fork
		if ( doFork )
		{
			cout << "Forking the daemon ..." << endl;
			pid = fork();
		}
		else
			pid = getpid();

		// fork error
		if ( pid < PID_Z )
		{
			cerr << "Daemon fork failed: " << cStrError(errno) << endl;
			return false;

		}
		// active child (or self if no fork)
		else if ( !doFork || pid == PID_Z )
		{
			if ( doFork )
			{
				cout << "Daemon forked successfully with pid " << getpid() << ", detaching" << endl;
				if ( !AbstractDaemon::detach() )
					return EXIT_FAILURE;
			}
			else
				cout << "Daemon starting with pid " << pid << endl;

			// write pid
			ofstream pid_file( _pid_filename.c_str() );
			pid_file << getpid();
			pid_file.close();

			// close standard streams
			if ( doFork )
			{
				close(STDIN_FILENO);
				close(STDOUT_FILENO);
				close(STDERR_FILENO);
			}

			// setup global params
			this->setupParameters();
			// run
			bool rt = this->run();
			// remove pid file
			remove( _pid_filename.c_str() );

			return rt;
		}
		// parent process (only when forking)
		else
		{
			return true;
		}
	}
}
