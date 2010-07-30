METASOURCES = AUTO

pkglib_LTLIBRARIES = libkgd_lib.la

noinst_HEADERS = \
	../../src/lib/buffer.h \
	../../src/lib/clock.h \
	../../src/lib/common.h \
	../../src/lib/daemon.h \
	../../src/lib/exceptions.h \
	../../src/lib/ini.h \
	../../src/lib/pls.h \
	../../src/lib/array.h \
	../../src/lib/array.hpp \
	../../src/lib/log.h \
	../../src/lib/socket.h \
	../../src/lib/urlencode.h \
	../../src/lib/utils/container.h \
	../../src/lib/utils/container.hpp \
	../../src/lib/utils/iterator.h \
	../../src/lib/utils/iterator.hpp \
	../../src/lib/utils/map.h \
	../../src/lib/utils/map.hpp \
	../../src/lib/utils/pointer.h \
	../../src/lib/utils/pointer.hpp \
	../../src/lib/utils/singleton.h \
	../../src/lib/utils/singleton.hpp \
	../../src/lib/utils/sharedptr.h \
	../../src/lib/utils/sharedptr.hpp \
	../../src/lib/utils/safe.h \
	../../src/lib/utils/safe.hpp \
	../../src/lib/utils/factory.h \
	../../src/lib/utils/factory.hpp \
	../../src/lib/utils/virtual.h \
	../../src/lib/utils/virtual.hpp


libkgd_lib_la_SOURCES = \
	../../src/lib/buffer.cpp \
	../../src/lib/clock.cpp \
	../../src/lib/common.cpp \
	../../src/lib/daemon.cpp \
	../../src/lib/exceptions.cpp \
	../../src/lib/ini.cpp \
	../../src/lib/pls.cpp \
	../../src/lib/log.cpp \
	../../src/lib/array.cpp \
	../../src/lib/socket.cpp \
	../../src/lib/urlencode.cpp \
	../../src/lib/utils/safe.cpp \
	../../src/lib/utils/factory.cpp \
	../../src/lib/utils/virtual.cpp

libkgd_lib_la_LIBADD = \
	-lrt -luuid \
	-lboost_regex \
	-lboost_thread