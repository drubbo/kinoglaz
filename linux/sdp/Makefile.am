METASOURCES = AUTO

pkglib_LTLIBRARIES = libkgd_sdp.la

noinst_HEADERS = \
	../../src/sdp/sdp.h \
	../../src/sdp/common.h \
	../../src/sdp/session.h \
	../../src/sdp/medium.h \
	../../src/sdp/descriptions.h \
	../../src/sdp/frameiterator.h \
	../../src/sdp/frame.h


libkgd_sdp_la_SOURCES = \
	../../src/sdp/common.cpp \
	../../src/sdp/session.cpp \
	../../src/sdp/medium.cpp \
	../../src/sdp/frameiterator.cpp \
	../../src/sdp/descriptions.cpp \
	../../src/sdp/frame.cpp

libkgd_sdp_la_LDFLAGS = -L../lib

libkgd_sdp_la_LIBADD = \
	-lkgd_lib \
	-lswscale -lavformat -lavcodec -lavutil