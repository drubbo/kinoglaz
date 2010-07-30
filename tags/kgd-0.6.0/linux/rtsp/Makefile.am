METASOURCES = AUTO

pkglib_LTLIBRARIES = libkgd_rtsp.la

noinst_HEADERS = \
	../../src/rtsp/common.h \
	../../src/rtsp/ports.h \
	../../src/rtsp/interleave.h \
	../../src/rtsp/message.h \
	../../src/rtsp/buffer.h \
	../../src/rtsp/exceptions.h \
	../../src/rtsp/error.h \
	../../src/rtsp/header.h \
	../../src/rtsp/method.h \
	../../src/rtsp/connection.h \
	../../src/rtsp/server.h \
	../../src/rtsp/session.h


libkgd_rtsp_la_SOURCES = \
	../../src/rtsp/common.cpp \
	../../src/rtsp/buffer.cpp \
	../../src/rtsp/exceptions.cpp \
	../../src/rtsp/error.cpp \
	../../src/rtsp/ports.cpp \
	../../src/rtsp/interleave.cpp \
	../../src/rtsp/message.cpp \
	../../src/rtsp/header.cpp \
	../../src/rtsp/method.cpp \
	../../src/rtsp/connection.cpp \
	../../src/rtsp/server.cpp \
	../../src/rtsp/session.cpp

libkgd_rtsp_la_LDFLAGS = -L../lib

libkgd_rtsp_la_LIBADD = -lkgd_lib