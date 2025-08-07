# sources and intermediate files are separated

AM_CFLAGS = -Wall \
            -Wundef \
            -Wstrict-prototypes \
            -Wno-trigraphs \
            -std=c++11 \
            -fpermissive \
            -D__GLIBC__

AM_CPPFLAGS = -I${BT_SOURCE}/bt-app \
              -I${BT_SOURCE}/bt-app/utils/include


AM_CPPFLAGS += @LIBCHROME_CFLAGS@

if BT_AUDIO_ENABLE
#if AUDIO_PAL_SUPPORTED
AM_CPPFLAGS += -DBT_AUDIO_PAL_INTEGRATION
AM_CPPFLAGS += @PABTAUDIO_CFLAGS@
#endif
endif

cpp_sources = src/uuid.cc

if BT_AUDIO_ENABLE
cpp_sources += src/pa_routing_interface.cc
AM_CPPFLAGS += -DBT_AUDIO_ENABLE
endif

requiredlibs = -lchrome

library_includedir = $(pkgincludedir)
library_include_HEADERS = $(h_sources)
if USE_GST
lib_LTLIBRARIES = libgstbtappuuid.la
libgstbtappuuid_la_CC = @CC@
libgstbtappuuid_la_SOURCES = $(cpp_sources)
libgstbtappuuid_la_CPPFLAGS =$(AM_CPPFLAGS) -std=c++11 -fpermissive -D__GLIBC__
libgstbtappuuid_la_CFLAGS = $(AM_CFLAGS) -fPIC -D_GNU_SOURCE -DUSE_GLIB
libgstbtappuuid_la_LDFLAGS = -static $(requiredlibs) -version-number @LT_VERSION_NUMBER@
else
lib_LTLIBRARIES = libbtappuuid.la
libbtappuuid_la_CC = @CC@
libbtappuuid_la_SOURCES = $(cpp_sources)
libbtappuuid_la_CPPFLAGS =$(AM_CPPFLAGS) -std=c++11 -fpermissive -D__GLIBC__
libbtappuuid_la_CFLAGS = $(AM_CFLAGS) -fPIC -D_GNU_SOURCE -DUSE_GLIB
libbtappuuid_la_LDFLAGS = -static $(requiredlibs) -version-number @LT_VERSION_NUMBER@
endif
