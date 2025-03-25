AM_CPPFLAGS = -O2 \
            -D_GNU_SOURCE \
            -std=c++11

AM_CFLAGS = -Wall \
            -Wundef \
            -Wstrict-prototypes \
            -Wno-trigraphs \
            -std=c++11 \
            -fpermissive \
            -D__GLIBC__

if USE_GEN_GATT
AM_CPPFLAGS += -DUSE_GEN_GATT
endif

if USE_BT_OBEX
AM_CFLAGS += -DOI_CPU_TYPE=19
AM_CPPFLAGS += -DUSE_BT_OBEX
endif
ACLOCAL_AMFLAGS = -I m4

AM_CPPFLAGS += -I${WORKSPACE}/qcom-opensource/bt/bt-app \
               -I${WORKSPACE}/qcom-opensource/bt/bt-app/utils/include \
               -I${WORKSPACE}/qcom-opensource/bt/bt-app/audio_manager/include \
               -I${WORKSPACE}/qcom-opensource/bt/bt-app/a2dp_sink/include \
               -I${WORKSPACE}/qcom-opensource/bt/bt-app/avrcp/include \
               -I${WORKSPACE}/qcom-opensource/bt/bt-app/a2dp_src/include \
               -I${WORKSPACE}/qcom-opensource/bt/bt-app/osi/include \
               -I${WORKSPACE}/qcom-opensource/bt/bt-app/gap/include \
               -I${WORKSPACE}/qcom-opensource/bt/bt-app/pan/include \
               -I${WORKSPACE}/qcom-opensource/bt/bt-app/hid/include \
               -I${WORKSPACE}/qcom-opensource/bt/bt-app/rsp/include \
               -I${WORKSPACE}/qcom-opensource/bt/bt-app/gatt/native/include \
               -I${WORKSPACE}/qcom-opensource/bt/bt-app/gatt/framework/include \
               -I${WORKSPACE}/qcom-opensource/bt/bt-app/gatt/lib/include \
               -I${WORKSPACE}/qcom-opensource/bt/bt-app/gattctest/include \
               -I${WORKSPACE}/qcom-opensource/bt/bt-app/gattstest/include \
               -I${WORKSPACE}/qcom-opensource/bt/bt-app/main/include \
               -I${WORKSPACE}/qcom-opensource/bt/bt-app/hfp_client/include \
               -I${WORKSPACE}/qcom-opensource/bt/bt-app/hfp_ag/include \
               -I${WORKSPACE}/qcom-opensource/bt/bt-app/include \
               -I${WORKSPACE}/qcom-opensource/bt/bt-app/sdp_client/include \
               -I${WORKSPACE}/system/media/audio/include \
               -I${WORKSPACE}/hardware/qcom/audio/qahw_api/inc \
               -I${WORKSPACE}/vendor/qcom/opensource/bluetooth/hal/include \
               -I${WORKSPACE}/vendor/qcom/opensource/bluetooth_ext/vhal/include \
               -I${WORKSPACE}/vendor/qcom/opensource/system/bt \
               -I${WORKSPACE}/vendor/qcom/opensource/system/bt/include \
               -I${WORKSPACE}/vendor/qcom/opensource/system/bt/types \
               -I${WORKSPACE}/qcom-opensource/bt/bt-app/utils/include

if RB5_AUDIO_HAL
else
AM_CPPFLAGS += -I${WORKSPACE}/hardware/libhardware/include
endif

if USE_BT_OBEX
AM_CPPFLAGS += -DOI_CPU_TYPE=19 \
               -I${WORKSPACE}/qcom-opensource/bt/bt-app/pbap_client/include \
               -I${WORKSPACE}/qcom-opensource/bt/bt-app/opp/include \
               -I${WORKSPACE}/qcom-opensource/bt/obex_profiles/include \
               -I${WORKSPACE}/qcom-opensource/bt/obex_profiles/include/profiles \
               -I${WORKSPACE}/qcom-opensource/bt/obex_profiles/profiles/obex \
               -I${WORKSPACE}/qcom-opensource/bt/obex_profiles/sdk/include \
               -I${WORKSPACE}/qcom-opensource/bt/obex_profiles/sdk/include/profiles \
               -I${WORKSPACE}/qcom-opensource/bt/obex_profiles/sdk/oem
endif

if AUDIO_HAL_SUPPORTED
AM_CPPFLAGS += -DBT_AUDIO_HAL_INTEGRATION
endif


cpp_sources = src/Gap.cpp \
              src/AdapterProperties.cpp \
              src/RemoteDevices.cpp

requiredlibs = -lchrome

library_includedir = $(pkgincludedir)
library_include_HEADERS = $(h_sources)
if USE_GST
lib_LTLIBRARIES = libgstbtappgap.la
libgstbtappgap_la_CC = @CC@
libgstbtappgap_la_SOURCES = $(cpp_sources)
libgstbtappgap_la_CPPFLAGS =$(AM_CPPFLAGS) -std=c++11 -fpermissive -D__GLIBC__ -DUSE_GLIB @GLIB_CFLAGS@
libgstbtappgap_la_CFLAGS = $(AM_CFLAGS) -fPIC -D_GNU_SOURCE -DUSE_GLIB @GLIB_CFLAGS@
libgstbtappgap_la_LDFLAGS = -static $(requiredlibs)
else
lib_LTLIBRARIES = libbtappgap.la
libbtappgap_la_CC = @CC@
libbtappgap_la_SOURCES = $(cpp_sources)
libbtappgap_la_CPPFLAGS =$(AM_CPPFLAGS) -std=c++11 -fpermissive -D__GLIBC__ -DUSE_GLIB @GLIB_CFLAGS@
libbtappgap_la_CFLAGS = $(AM_CFLAGS) -fPIC -D_GNU_SOURCE -DUSE_GLIB @GLIB_CFLAGS@
libbtappgap_la_LDFLAGS = -static $(requiredlibs)
endif
