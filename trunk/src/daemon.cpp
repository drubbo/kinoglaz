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
 * File name: ./daemon.cpp
 * First submitted: 2010-01-16
 * First submitter: Emiliano Leporati <emiliano.leporati@gmail.com>
 * Contributor(s) so far - 2010-07-30 :
 *     Emiliano Leporati <emiliano.leporati@gmail.com>
 *
 * Last changes :
 *     formats on their own; better Factory elements naming; separate library for every component
 *     Kinoglaz birth: pervasive bug fixing, improvements and client support
 *     log messages refactor; shared descriptor are optional now; spot insertion support
 *     sig HUP supported
 *     frame / other stuff refactor
 *
 **/


#include "daemon.h"
#include "rtsp/ports.h"
#include "rtsp/server.h"
#include "sdp/sdp.h"
#include "lib/ini.h"
#include "lib/clock.h"
#include "lib/log.h"
#include "rtp/buffer.h"
#include "rtsp/connection.h"

#include <cstdlib>
#include <csignal>

#ifdef HAVE_CONFIG_H
#include "config.h"
#else
#error "Please use platform configuration file."
#endif

namespace KGD
{
	Daemon::Reference Daemon::getInstance() throw( KGD::Exception::InvalidState )
	{
		Instance::LockerType lk( _instance );
		if ( ! *_instance )
			throw KGD::Exception::InvalidState( "KGD: daemon instance not initialized" );
		else
			return newInstanceRef();
	}

	Daemon::Reference Daemon::getInstance( const string & fileName ) throw( KGD::Exception::InvalidState, KGD::Exception::NotFound )
	{
		Instance::LockerType lk( _instance );
		if ( *_instance )
			throw KGD::Exception::InvalidState( "KGD: daemon instance already initialized" );
		else
		{
			(*_instance).reset( new Daemon( fileName ) );
			return newInstanceRef();
		}
	}

	Daemon::Daemon( const string & fileName ) throw( KGD::Exception::NotFound )
	: KGD::AbstractDaemon( KGD::Ini::getInstance( fileName ) )
	{
	}

	void Daemon::setupParameters() throw( KGD::Exception::NotFound )
	{
		srandom(uint32_t (KGD::Clock::getNano() & 0xFFFF));

		_ini->reload();

		RTP::Buffer::Base::SIZE_LOW = fromString< double >( (*_ini)( "RTP", "buf-empty" ) );
		RTP::Buffer::Base::SIZE_FULL = fromString< double >( (*_ini)( "RTP", "buf-full" ) );
		RTP::Packet::MTU = fromString< size_t >( (*_ini)("RTP", "net-mtu") );

		RTSP::Port::Udp::FIRST = fromString< TPort >( (*_ini)("RTP", "udp-first", "30000") );
		RTSP::Port::Udp::LAST = fromString< TPort >( (*_ini)("RTP", "udp-last", "40000") );
		RTSP::Port::Udp::getInstance()->reset( RTSP::Port::Udp::FIRST, RTSP::Port::Udp::LAST );

		SDP::Container::BASE_DIR = (*_ini)( "SDP", "base-dir");
		SDP::Container::AGGREGATE_CONTROL = ( "1" == (*_ini)( "SDP", "aggregate", "1") );

		RTSP::Connection::SHARE_DESCRIPTORS = ( "1" == (*_ini)( "SDP", "share-descriptors", "0" ) );

		RTSP::Method::SUPPORT_SEEK = ( "1" == (*_ini)( "RTSP", "supp-seek", "1" ) );

		ostringstream s;
		s << "KGD: Parameters: Buffer [" << RTP::Buffer::Base::SIZE_LOW << "-" << RTP::Buffer::Base::SIZE_FULL
			<< "] | MTU " << RTP::Packet::MTU
			<< " | RTP [" << RTSP::Port::Udp::FIRST << "-" << RTSP::Port::Udp::LAST << "]"
			<< " | SDP shared descriptors " << RTSP::Connection::SHARE_DESCRIPTORS
			<< " | SDP aggregate control " << SDP::Container::AGGREGATE_CONTROL
			<< " | RTSP seek support " << RTSP::Method::SUPPORT_SEEK;
		Log::debug( "%s", s.str().c_str() );
	}

	bool Daemon::run() throw()
	{
		SigHandler::setup();

		Log::message("KGD: >-------------------------------- START --------------------------------<");

		try
		{
			{
				RTSP::Server::Reference s = RTSP::Server::getInstance( (*_ini)[ "SERVER" ] );
				s->start();
			}
			RTSP::Server::destroyInstance();
		}
		catch ( KGD::Socket::Exception & e )
		{
			Log::error( "KGD: socket error: %s", e.what() );
		}
		catch ( KGD::Exception::Generic & e )
		{
			Log::error( "KGD: %s", e.what() );
		}

		Log::message("KGD: >-------------------------------- STOP ---------------------------------<");

		return true;
	}

	string Daemon::getName() throw()
	{
		return PACKAGE_NAME + string(" version ") + PACKAGE_VERSION;
	}
	string Daemon::getFullName() const throw()
	{
		return getName();
	}

	void Daemon::SigHandler::term( int )
	{
		Log::warning("KGD: received SIGTERM");
		RTSP::Server::Reference s = RTSP::Server::getInstance( );
		s->stop();
	}

	void Daemon::SigHandler::hup( int )
	{
		try
		{
			Log::warning("KGD: received SIGHUP");
			KGD::Daemon::Reference d = KGD::Daemon::getInstance( );
			d->setupParameters();
			RTSP::Server::Reference s = RTSP::Server::getInstance( );
			s->setupParameters( d->getParameters()[ "SERVER" ] );
		}
		catch( KGD::Exception::Generic const &e )
		{
			Log::error( "KGD: %s in SIGHUP", e.what() );
		}
	}

	void Daemon::SigHandler::setup()
	{
		Log::message("KGD: signal setup");
		// blocco tutti i segnali tranne TERM
		sigset_t sig_mask;
		sigfillset(&sig_mask);
		sigdelset(&sig_mask, SIGTERM);
		sigdelset(&sig_mask, SIGINT);
		sigdelset(&sig_mask, SIGHUP);

		sigprocmask(SIG_BLOCK, &sig_mask, NULL);

		// definisco gli handler
		struct sigaction act;

		// TERM + INT
		sigfillset(&sig_mask);
		act.sa_handler = Daemon::SigHandler::term;
		act.sa_mask    = sig_mask;
		act.sa_flags   = 0; //SA_RESETHAND;
		sigaction(SIGTERM, &act, NULL);
		sigaction(SIGINT, &act, NULL);

		// HUP
		act.sa_handler = Daemon::SigHandler::hup;
		act.sa_mask    = sig_mask;
		act.sa_flags   = 0;
		sigaction(SIGHUP, &act, NULL);
	}
}
