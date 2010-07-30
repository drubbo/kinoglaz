METASOURCES = AUTO

pkglib_LTLIBRARIES = libkgd_rtcp.la

noinst_HEADERS = \
	../../src/rtcp/header.h \
	../../src/rtcp/stats.h \
	../../src/rtcp/rtcp.h \
	../../src/rtcp/sender.h \
	../../src/rtcp/receiver.h


libkgd_rtcp_la_SOURCES = \
	../../src/rtcp/header.cpp \
	../../src/rtcp/sender.cpp \
	../../src/rtcp/stats.cpp \
	../../src/rtcp/receiver.cpp

libkgd_rtcp_la_LDFLAGS = -L../lib

libkgd_rtcp_la_LIBADD = -lkgd_lib