# sources and intermediate files are separated

AM_CFLAGS = -Wall \
            -Wundef \
            -Wstrict-prototypes \
            -Wno-trigraphs \
            -std=c++11 \
            -fpermissive \
            -D__GLIBC__

AM_CPPFLAGS = -I${WORKSPACE}/../qcom-opensource/bt/bt-app \
              -I${WORKSPACE}/../qcom-opensource/bt/bt-app/utils/include

cpp_sources = src/uuid.cc

requiredlibs = -lchrome

library_includedir = $(pkgincludedir)
library_include_HEADERS = $(h_sources)
if USE_GST
lib_LTLIBRARIES = libgstbtappuuid.la
libgstbtappuuid_la_CC = @CC@
libgstbtappuuid_la_SOURCES = $(cpp_sources)
libgstbtappuuid_la_CPPFLAGS =$(AM_CPPFLAGS) -std=c++11 -fpermissive -D__GLIBC__
libgstbtappuuid_la_CFLAGS = $(AM_CFLAGS) -fPIC -D_GNU_SOURCE -DUSE_GLIB
libgstbtappuuid_la_LDFLAGS = -static $(requiredlibs)
else
lib_LTLIBRARIES = libbtappuuid.la
libbtappuuid_la_CC = @CC@
libbtappuuid_la_SOURCES = $(cpp_sources)
libbtappuuid_la_CPPFLAGS =$(AM_CPPFLAGS) -std=c++11 -fpermissive -D__GLIBC__
libbtappuuid_la_CFLAGS = $(AM_CFLAGS) -fPIC -D_GNU_SOURCE -DUSE_GLIB
libbtappuuid_la_LDFLAGS = -static $(requiredlibs)
endif
