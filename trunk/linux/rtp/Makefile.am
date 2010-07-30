METASOURCES = AUTO

pkglib_LTLIBRARIES = libkgd_rtp.la

noinst_HEADERS = \
	../../src/rtp/chrono.h \
	../../src/rtp/packet.h \
	../../src/rtp/header.h \
	../../src/rtp/frame.h \
	../../src/rtp/session.h \
	../../src/rtp/buffer.h


libkgd_rtp_la_SOURCES = \
	../../src/rtp/chrono.cpp \
	../../src/rtp/header.cpp \
	../../src/rtp/packet.cpp \
	../../src/rtp/frame.cpp \
	../../src/rtp/session.cpp \
	../../src/rtp/session_methods.cpp \
	../../src/rtp/session_times.cpp \
	../../src/rtp/buffer.cpp

libkgd_rtp_la_LDFLAGS = -L../lib

libkgd_rtp_la_LIBADD = -lkgd_lib