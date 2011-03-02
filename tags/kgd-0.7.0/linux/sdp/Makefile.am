METASOURCES = AUTO

pkglib_LTLIBRARIES = libkgd_sdp.la

noinst_HEADERS = \
	../../src/sdp/sdp.h \
	../../src/sdp/common.h \
	../../src/sdp/container.h \
	../../src/sdp/medium.h \
	../../src/sdp/descriptions.h \
	../../src/sdp/frameiterator.h \
	../../src/sdp/frame.h


libkgd_sdp_la_SOURCES = \
	../../src/sdp/common.cpp \
	../../src/sdp/container.cpp \
	../../src/sdp/container_mediafile.cpp \
	../../src/sdp/container_v4l.cpp \
	../../src/sdp/medium.cpp \
	../../src/sdp/frameiterator.cpp \
	../../src/sdp/descriptions.cpp \
	../../src/sdp/frame.cpp

libkgd_sdp_la_LDFLAGS = -L../lib

libkgd_sdp_la_LIBADD = \
	-lkgd_lib \
	-lswscale -lavdevice -lavformat -lavcodec -lavutil
