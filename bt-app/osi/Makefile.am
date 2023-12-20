# sources and intermediate files are separated

AM_CFLAGS = -Wall \
            -Wundef \
            -Wstrict-prototypes \
            -Wno-trigraphs \
            -std=c++11 \
            -fpermissive \
            -DHAS_NO_BDROID_BUILDCFG \
            -D__GLIBC__

AM_CPPFLAGS = -I${BT_SOURCE}/btapp/bt-app \
              -I${BT_SOURCE}/btapp/bt-app/osi/include \
              -I${BT_SOURCE}/stack/system/bt \
              -I${BT_SOURCE}/stack/system/bt/include \
							-I${BT_SOURCE}/stack/system/bt/include/hardware \
              -I${BT_SOURCE}/stack/system/bt/types \
              -I${BT_SOURCE}/bt_audio/hal/include

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
libgstbtapposi_la_CPPFLAGS =$(AM_CPPFLAGS) -std=c++11 -fpermissive -DHAS_NO_BDROID_BUILDCFG -D__GLIBC__
libgstbtapposi_la_LIBADD = $(requiredlibs)
libgstbtapposi_la_CFLAGS = $(AM_CFLAGS) -fPIC -D_GNU_SOURCE -DUSE_GLIB
libgstbtapposi_la_LDFLAGS = -static -lbtosi -lbtutils
else
lib_LTLIBRARIES = libbtapposi.la
libbtapposi_la_CC = @CC@
libbtapposi_la_SOURCES = $(c_sources)
libbtapposi_la_CPPFLAGS =$(AM_CPPFLAGS) -std=c++11 -fpermissive
libbtapposi_la_LIBADD = $(requiredlibs)
libbtapposi_la_CFLAGS = $(AM_CFLAGS) -fPIC -D_GNU_SOURCE -DUSE_GLIB -DHAS_NO_BDROID_BUILDCFG -D__GLIBC__
libbtapposi_la_LDFLAGS = -static -lbtosi -lbtutils
endif
