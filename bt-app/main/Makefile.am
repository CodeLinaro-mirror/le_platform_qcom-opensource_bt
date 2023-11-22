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

AM_CPPFLAGS += -I${WORKSPACE}/bluetooth/btapp/bt-app \
               -I${WORKSPACE}/bluetooth/btapp/bt-app/utils/include \
               -I${WORKSPACE}/bluetooth/btapp/bt-app/audio_manager/include \
               -I${WORKSPACE}/bluetooth/btapp/bt-app/a2dp_sink/include \
               -I${WORKSPACE}/bluetooth/btapp/bt-app/avrcp/include \
               -I${WORKSPACE}/bluetooth/btapp/bt-app/a2dp_src/include \
               -I${WORKSPACE}/bluetooth/btapp/bt-app/osi/include \
               -I${WORKSPACE}/bluetooth/btapp/bt-app/gap/include \
               -I${WORKSPACE}/bluetooth/btapp/bt-app/pan/include \
               -I${WORKSPACE}/bluetooth/btapp/bt-app/hid/include \
               -I${WORKSPACE}/bluetooth/btapp/bt-app/rsp/include \
               -I${WORKSPACE}/bluetooth/btapp/bt-app/gatt/native/include \
               -I${WORKSPACE}/bluetooth/btapp/bt-app/gatt/framework/include \
               -I${WORKSPACE}/bluetooth/btapp/bt-app/gatt/lib/include \
               -I${WORKSPACE}/bluetooth/btapp/bt-app/gattctest/include \
               -I${WORKSPACE}/bluetooth/btapp/bt-app/gattstest/include \
               -I${WORKSPACE}/bluetooth/btapp/bt-app/main/include \
               -I${WORKSPACE}/bluetooth/btapp/bt-app/hfp_client/include \
               -I${WORKSPACE}/bluetooth/btapp/bt-app/hfp_ag/include \
               -I${WORKSPACE}/bluetooth/btapp/bt-app/include \
               -I${WORKSPACE}/bluetooth/btapp/bt-app/sdp_client/include \
               -I${WORKSPACE}/bluetooth/btapp/bt-app/spp_client/include \
               -I${WORKSPACE}/bluetooth/btapp/bt-app/spp_server/include \
               -I${WORKSPACE}/system/media/audio/include \
               -I${WORKSPACE}/hardware/qcom/audio/qahw_api/inc \
							 -I${WORKSPACE}/vendor/qcom/opensource/audio-hal/primary-hal/qahw_api/inc \
               -I${WORKSPACE}/hardware/libhardware/include \
               -I${WORKSPACE}/bluetooth/bt_audio/hal/include \
               -I${WORKSPACE}/bluetooth/stack/bluetooth_ext/vhal/include \
               -I${WORKSPACE}/mcm-api/api \
               -I${WORKSPACE}/bluetooth/stack/system/bt \
               -I${WORKSPACE}/bluetooth/stack/system/bt/include \
							 -I${WORKSPACE}/bluetooth/stack/system/bt/include/hardware \
               -I${WORKSPACE}/bluetooth/stack/system/bt/types \
               -I${WORKSPACE}/bluetooth/btapp/bt-app/utils/include

if USE_BT_OBEX
AM_CPPFLAGS += -DOI_CPU_TYPE=19 \
               -I${WORKSPACE}/bluetooth/btapp/bt-app/pbap_client/include \
               -I${WORKSPACE}/bluetooth/btapp/bt-app/opp/include \
               -I${WORKSPACE}/bluetooth/btapp/obex_profiles/include \
               -I${WORKSPACE}/bluetooth/btapp/obex_profiles/include/profiles \
               -I${WORKSPACE}/bluetooth/btapp/obex_profiles/profiles/obex \
               -I${WORKSPACE}/bluetooth/btapp/obex_profiles/sdk/include \
               -I${WORKSPACE}/bluetooth/btapp/obex_profiles/sdk/include/profiles \
               -I${WORKSPACE}/bluetooth/btapp/obex_profiles/sdk/oem
endif

if AUDIO_HAL_SUPPORTED
AM_CPPFLAGS += -DBT_AUDIO_HAL_INTEGRATION
endif

cpp_sources = ../utils/src/utils.cc \
            src/ipc.cpp \
            src/Main.cpp \
            ../hid/src/Hid.cpp \
            ../gap/src/Gap.cpp \
            ../gap/src/AdapterProperties.cpp \
            ../gap/src/RemoteDevices.cpp \
            ../sdp_client/src/SdpClient.cpp\
            ../a2dp_sink/src/A2dp_Sink.cpp \
            ../a2dp_sink/src/A2dp_Sink_Split.cpp \
            ../a2dp_src/src/A2dp_Src.cpp \
            ../a2dp_sink/src/A2dp_Sink_Streaming.cpp \
            ../avrcp/src/Avrcp.cpp \
            ../audio_manager/src/Audio_Manager.cpp \
            ../hfp_client/src/HfpClient.cpp \
            ../hfp_ag/src/HfpAG.cpp \
            ../gattctest/src/GattcTest.cpp \
            ../pan/src/Pan.cpp \
            ../spp_server/src/spp_server.cpp \
            ../spp_client/src/spp_client.cpp \
            ../gattstest/src/GattsTest.cpp

if USE_GEN_GATT
cpp_sources += ../rsp/src/Rsp.cpp
endif


if USE_BT_OBEX
cpp_sources +=  ../pbap_client/src/PbapClient.cpp \
              ../opp/src/Opp.cpp
endif

if USE_GST
requiredlibs = ../osi/libgstbtapposi.la ../utils/libgstbtappuuid.la -lpthread -lm -ldl -lrt -lbtvendorhal -lbttypes -lcutils
else
requiredlibs = ../osi/libbtapposi.la ../utils/libbtappuuid.la -lpthread -lbttypes -lbtvendorhal
endif

if USE_GEN_GATT
requiredlibs += ../gatt/libgengatt.la
endif

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
 btapp_LDFLAGS = $(requiredlibs) @GLIB_LIBS@ -lpropertyvault
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
