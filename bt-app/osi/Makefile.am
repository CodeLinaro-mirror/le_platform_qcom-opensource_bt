# sources and intermediate files are separated

AM_CFLAGS = -Wall \
            -Wundef \
            -Wstrict-prototypes \
            -Wno-trigraphs \
            -std=c++11 \
            -fpermissive \
            -DHAS_NO_BDROID_BUILDCFG \
            -D__GLIBC__

AM_CPPFLAGS = -I$(top_srcdir) \
              -I$(top_srcdir)/osi/include

AM_CPPFLAGS  += @SYSTEM_BT_CFLAGS@ @LIBCHROME_CFLAGS@
AM_CPPFLAGS  += -DUSE_ANDROID_LOGGING -DUSE_BT_OBEX -DUSE_LIBHW_AOSP -DUSE_GEN_GATT
c_sources = \
    src/alarm.cc \
    src/allocator.cc \
    src/allocation_tracker.cc \
    src/config.cc \
    src/fixed_queue.cc \
    src/hash_functions.cc \
    src/hash_map.cc \
    src/list.cc \
    src/reactor.cc \
    src/semaphore.cc \
    src/thread.cc \
    src/future.cc

requiredlibs = -lpthread
library_includedir = $(pkgincludedir)\
library_include_HEADERS = $(h_sources)
if USE_GST
lib_LTLIBRARIES = libgstbtapposi.la
libgstbtapposi_la_CC = @CC@
libgstbtapposi_la_SOURCES = $(c_sources)
libgstbtapposi_la_CPPFLAGS =$(AM_CPPFLAGS) -std=c++11 -fpermissive -DHAS_NO_BDROID_BUILDCFG -D__GLIBC__ -DUSE_GLIB @GLIB_CFLAGS@
libgstbtapposi_la_CPPFLAGS +=$(GLIB_CFLAGS) -Dstrlcat=g_strlcat -include glib.h
libgstbtapposi_la_LIBADD = $(requiredlibs) @GLIB_LIBS@
libgstbtapposi_la_LIBADD += $(GLIB_LIBS) -llog -lcutils
libgstbtapposi_la_CFLAGS = $(AM_CFLAGS) -fPIC -D_GNU_SOURCE -DUSE_GLIB -DUSE_GLIB @GLIB_CFLAGS@
libgstbtapposi_la_LDFLAGS = -static
else
lib_LTLIBRARIES = libbtapposi.la
libbtapposi_la_CC = @CC@
libbtapposi_la_SOURCES = $(c_sources)
libbtapposi_la_CPPFLAGS =$(AM_CPPFLAGS) -std=c++11 -fpermissive -DUSE_GLIB @GLIB_CFLAGS@
libbtapposi_la_LIBADD = $(requiredlibs)  @GLIB_LIBS@
libbtapposi_la_CFLAGS = $(AM_CFLAGS) -fPIC -D_GNU_SOURCE -DUSE_GLIB -DHAS_NO_BDROID_BUILDCFG -D__GLIBC__ -DUSE_GLIB @GLIB_CFLAGS@
libbtapposi_la_LDFLAGS = -static
libbtapposi_la_CPPFLAGS +=$(GLIB_CFLAGS) -Dstrlcat=g_strlcat -include glib.h
libbtapposi_la_LIBADD += $(GLIB_LIBS) -llog -lcutils
endif
