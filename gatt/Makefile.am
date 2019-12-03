AM_CPPFLAGS = -O2 \
              -D_GNU_SOURCE \
             -I${top_srcdir}/include \
             -I${top_srcdir}/../bt-app/include \
             -I${top_srcdir}/../bt-app

AM_CFLAGS = -Wall \
        -Wundef \
        -Wstrict-prototypes \
        -Wno-trigraphs \
        -DLINUX \
        -std=c99

ACLOCAL_AMFLAGS = -I m4

c_sources = src/Gatt.cpp

h_sources = include/Gatt.hpp

AM_LDFLAGS := $(LDFLAFGS)

library_includedir = $(pkgincludedir)
lib_LTLIBRARIES = libgengatt.la
libgengatt_la_CC = @CC@
libgengatt_la_SOURCES = $(c_sources)
libgengatt_la_includedir = $(includedir)
libgengatt_la_include_HEADERS = $(h_sources)
libgengatt_la_CPPFLAGS = $(AM_CPPFLAGS)
libgengatt_la_CFLAGS = $(AM_CFLAGS)
libgengatt_la_LDFLAGS = $(AM_LDFLAGS) -static -lpthread -lm -ldl -lrt
