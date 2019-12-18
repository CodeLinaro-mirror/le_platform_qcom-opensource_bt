AM_CPPFLAGS = -O2 \
            -D_GNU_SOURCE \
            -std=c++11

AM_CFLAGS = -Wall \
            -Wundef \
            -Wstrict-prototypes \
            -Wno-trigraphs \
            -std=c++11

if USE_BT_OBEX
AM_CFLAGS += -DOI_CPU_TYPE=19
endif
ACLOCAL_AMFLAGS = -I m4

AM_CPPFLAGS += -I${WORKSPACE}/qcom-opensource/bt/bt-app-ipc \
               -I${WORKSPACE}/qcom-opensource/bt/bt-app-ipc/utils/include \
               -I${WORKSPACE}/qcom-opensource/bt/bt-app-ipc/audio_manager/include \
               -I${WORKSPACE}/qcom-opensource/bt/bt-app-ipc/a2dp_sink/include \
               -I${WORKSPACE}/qcom-opensource/bt/bt-app-ipc/avrcp/include \
               -I${WORKSPACE}/qcom-opensource/bt/bt-app-ipc/a2dp_src/include \
               -I${WORKSPACE}/qcom-opensource/bt/bt-app-ipc/osi/include \
               -I${WORKSPACE}/qcom-opensource/bt/bt-app-ipc/gap/include \
               -I${WORKSPACE}/qcom-opensource/bt/bt-app-ipc/pan/include \
               -I${WORKSPACE}/qcom-opensource/bt/bt-app-ipc/hid/include \
               -I${WORKSPACE}/qcom-opensource/bt/bt-app-ipc/rsp/include \
               -I${WORKSPACE}/qcom-opensource/bt/bt-app-ipc/gatt/native/include \
               -I${WORKSPACE}/qcom-opensource/bt/bt-app-ipc/gatt/framework/include \
               -I${WORKSPACE}/qcom-opensource/bt/bt-app-ipc/gatt/lib/include \
               -I${WORKSPACE}/qcom-opensource/bt/bt-app-ipc/gattctest/include \
               -I${WORKSPACE}/qcom-opensource/bt/bt-app-ipc/gattstest/include \
               -I${WORKSPACE}/qcom-opensource/bt/bt-app-ipc/main/include \
               -I${WORKSPACE}/qcom-opensource/bt/bt-app-ipc/hfp_client/include \
               -I${WORKSPACE}/qcom-opensource/bt/bt-app-ipc/hfp_ag/include \
               -I${WORKSPACE}/qcom-opensource/bt/bt-app-ipc/include \
               -I${WORKSPACE}/qcom-opensource/bt/bt-app-ipc/sdp_client/include \
               -I${WORKSPACE}/system/media/audio/include \
               -I${WORKSPACE}/hardware/qcom/audio/qahw_api/inc \
               -I${WORKSPACE}/hardware/libhardware/include \
               -I${WORKSPACE}/vendor/qcom/opensource/bluetooth/hal/include \
               -I${WORKSPACE}/vendor/qcom/opensource/bluetooth_ext/vhal/include \
               -I${WORKSPACE}/mcm-api/api \
               -I${WORKSPACE}/vendor/qcom/opensource/system/bt \
               -I${WORKSPACE}/vendor/qcom/opensource/system/bt/include \
               -I${WORKSPACE}/vendor/qcom/opensource/system/bt/types \
               -I${WORKSPACE}/qcom-opensource/bt/bt-app-ipc/utils/include

if USE_BT_OBEX
AM_CPPFLAGS += -DOI_CPU_TYPE=19 \
               -I${WORKSPACE}/qcom-opensource/bt/bt-app-ipc/pbap_client/include \
               -I${WORKSPACE}/qcom-opensource/bt/bt-app-ipc/opp/include \
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

cpp_sources = ../utils/src/utils.cc \
            src/ipc.cpp \
            src/Main.cpp \
            ../hfp_ag/src/HfpAG.cpp \
            ../hfp_ag/src/hfpag_interface_b.cpp \
            ../gattctest/src/GattcTest.cpp \
            ../gattstest/src/GattsTest.cpp

requiredlibs = ../gatt/libgengatt.la ../osi/libbtapposi.la ../utils/libbtappuuid.la -lpthread -lm -ldl -lrt -lbtvendorhal -lbttypes -lcutils

if AUDIO_HAL_SUPPORTED
requiredlibs += -lqahw
endif

if USE_BT_OBEX
requiredlibs += -lbtobex
endif

if USE_GLIB

if USE_GST
 gstbtapp_CFLAGS  = $(AM_CFLAGS) -DUSE_GLIB @GLIB_CFLAGS@
 gstbtapp_CPPFLAGS  = $(AM_CPPFLAGS) -DUSE_GLIB @GLIB_CFLAGS@
 gstbtapp_LDFLAGS = $(requiredlibs) @GLIB_LIBS@
 gstbtapp_CPPFLAGS += $(GSTREAMER_CFLAGS) -DUSE_GST
else
 btapp_CFLAGS  = $(AM_CFLAGS) -DUSE_GLIB @GLIB_CFLAGS@
 btapp_CPPFLAGS  = $(AM_CPPFLAGS) -DUSE_GLIB @GLIB_CFLAGS@
 btapp_LDFLAGS = $(requiredlibs) @GLIB_LIBS@
endif

else

if USE_GST
 gstbtapp_CFLAGS = $(AM_CFLAGS)
 gstbtapp_LDFLAGS = $(requiredlibs)
else
 btapp_CFLAGS = $(AM_CFLAGS)
 btapp_LDFLAGS = $(requiredlibs)
endif

endif

if USE_GST
gstbtapp_CFLAGS += $(GSTREAMER_CFLAGS)
gstbtapp_LDFLAGS += $(GSTREAMER_LIBS) -lgstaudio-1.0 -lgstapp-1.0 -lgstbthelper
endif

AM_LDFLAGS = $(LDFLAFGS)

if USE_GST
bin_PROGRAMS = gstbtapp
gstbtapp_CC = @CC@
gstbtapp_SOURCES = $(cpp_sources)
else
bin_PROGRAMS = btapp
btapp_CC = @CC@
btapp_SOURCES = $(cpp_sources)
endif
pkgconfigdir = $(libdir)/pkgconfig
