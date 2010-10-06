METASOURCES = AUTO

SUBDIRS = lib sdp rtcp rtp rtsp formats

bin_PROGRAMS = kgd

kgd_SOURCES = \
	../src/main.cpp \
	../src/daemon.cpp

noinst_HEADERS = \
	../src/daemon.h

kgd_LDFLAGS = -Llib -Lsdp -Lrtp -Lrtcp -Lrtsp -Lformats

kgd_LDADD = \
	-lkgd_sdp -lkgd_rtcp -lkgd_rtp -lkgd_rtsp -lkgd_formats \
	-lboost_thread -lboost_regex
