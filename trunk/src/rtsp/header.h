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
 * File name: src/rtsp/header.h
 * First submitted: 2010-07-30
 * First submitter: Emiliano Leporati <emiliano.leporati@gmail.com>
 * Contributor(s) so far - 2010-11-04 :
 *     Emiliano Leporati <emiliano.leporati@gmail.com>
 *
 * Last changes :
 *     source import
 *
 **/


#ifndef __KGD_RTSP_HEADERS_H
#define __KGD_RTSP_HEADERS_H

#include <string>

using namespace std;

namespace KGD
{
	//! RTSP classes
	namespace RTSP
	{
		//! RTSP headers
		namespace Header
		{
			const string ContentLength    = "Content-Length";
			const string Accept           = "Accept";
			const string Allow            = "Allow";
			const string Blocksize        = "Blocksize";
			const string ContentType      = "Content-Type";
			const string Date             = "Date";
			const string Require          = "Require";
			const string TransportRequire = "Transport-Require";
			const string SequenceNo       = "SequenceNo";
			const string Cseq             = "CSeq";
			const string Stream           = "Stream";
			const string Session          = "Session";
			const string Transport        = "Transport";
			const string Range            = "Range";
			const string Scale            = "Scale";
			const string UserAgent        = "User-Agent";
		}
	}

}

#endif
