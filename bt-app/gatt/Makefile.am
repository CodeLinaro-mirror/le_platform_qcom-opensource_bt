if USE_GEN_GATT
AM_CPPFLAGS = -O2 \
              -D_GNU_SOURCE \
              -I${WORKSPACE}/qcom-opensource/bt/bt-app/utils/include \
              -I${WORKSPACE}/qcom-opensource/bt/bt-app/gatt/native/include \
              -I${WORKSPACE}/qcom-opensource/bt/bt-app/include \
              -I${WORKSPACE}/qcom-opensource/bt/bt-app/ \
              -I${WORKSPACE}/vendor/qcom/opensource/bluetooth/hal/include/ \
              -I${WORKSPACE}/vendor/qcom/opensource/bluetooth/vhal/include \
              -I${WORKSPACE}/vendor/qcom/opensource/system/bt \
              -I${WORKSPACE}/vendor/qcom/opensource/system/bt/include \
              -I${WORKSPACE}/vendor/qcom/opensource/system/bt/types \
              -I${WORKSPACE}/system/core/include

ACLOCAL_AMFLAGS = -I m4

cpp_sources = native/src/GattNativeInterfaceV2.cpp

AM_LDFLAGS := -lbttypes -lchrome ../utils/libbtappuuid.la

library_includedir = $(pkgincludedir)
lib_LTLIBRARIES = libgengatt.la
libgengatt_la_CC = @CC@
libgengatt_la_SOURCES = $(cpp_sources)
libgengatt_la_CPPFLAGS = $(AM_CPPFLAGS)
libgengatt_la_LDFLAGS = $(AM_LDFLAGS) -static -lpthread -lm -ldl -lrt
endif