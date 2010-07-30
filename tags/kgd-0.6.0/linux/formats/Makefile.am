METASOURCES = AUTO

pkglib_LTLIBRARIES = libkgd_formats.la

noinst_HEADERS = \
	../../src/formats/audio/mp2.h \
	../../src/formats/audio/mp3.h \
	../../src/formats/video/mp4.h

libkgd_formats_la_SOURCES = \
	../../src/formats/audio/mp2.cpp \
	../../src/formats/audio/mp3.cpp \
	../../src/formats/video/mp4.cpp

libkgd_formats_la_LDFLAGS = -L../sdp -L../rtp

libkgd_formats_la_LIBADD = \
	-lkgd_sdp -lkgd_rtp