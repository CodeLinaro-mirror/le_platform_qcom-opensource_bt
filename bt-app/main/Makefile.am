AM_CPPFLAGS = -O2 \
            -D_GNU_SOURCE \
            -std=c++11

AM_CFLAGS = -Wall \
            -Wundef \
            -Wstrict-prototypes \
            -Wno-trigraphs \
            -std=c++11


AM_CPPFLAGS += @LIBCHROME_CFLAGS@
AM_CPPFLAGS += -DUSE_ANDROID_LOGGING -DUSE_BT_OBEX -DUSE_LIBHW_AOSP -DUSE_GEN_GATT

if USE_BT_OBEX
AM_CFLAGS += -DOI_CPU_TYPE=19
endif
ACLOCAL_AMFLAGS = -I m4

AM_CPPFLAGS += -I$(top_srcdir)/ \
               -I$(top_srcdir)/utils/include \
               -I$(top_srcdir)/audio_manager/include \
               -I$(top_srcdir)/a2dp_sink/include \
               -I$(top_srcdir)/avrcp/include \
               -I$(top_srcdir)/a2dp_src/include \
               -I$(top_srcdir)/osi/include \
               -I$(top_srcdir)/gap/include \
               -I$(top_srcdir)/pan/include \
               -I$(top_srcdir)/hid/include \
               -I$(top_srcdir)/rsp/include \
               -I$(top_srcdir)/gatt/native/include \
               -I$(top_srcdir)/gatt/framework/include \
               -I$(top_srcdir)/gatt/lib/include \
               -I$(top_srcdir)/gattctest/include \
               -I$(top_srcdir)/gattstest/include \
               -I$(top_srcdir)/main/include \
               -I$(top_srcdir)/hfp_client/include \
               -I$(top_srcdir)/hfp_ag/include \
               -I$(top_srcdir)/include \
               -I$(top_srcdir)/sdp_client/include \
               -I${WORKSPACE}/hardware/qcom/audio/qahw_api/inc \
               -I$(top_srcdir)/utils/include \
               -I$(top_srcdir)/ble_sock_manager/include

AM_CPPFLAGS += @BLUETOOTHEXT_CFLAGS@

if RB5_AUDIO_HAL
else
#AM_CPPFLAGS += -I${WORKSPACE}/hardware/libhardware/include
endif

if USE_BT_OBEX
AM_CPPFLAGS += -DOI_CPU_TYPE=19 \
               -I$(top_srcdir)/pbap_client/include \
               -I$(top_srcdir)/opp/include \
               -I../../obex_profiles/include \
               -I../../obex_profiles/include/profiles \
               -I../../obex_profiles/profiles/obex \
               -I../../obex_profiles/sdk/include \
               -I../../obex_profiles/sdk/include/profiles \
               -I../../obex_profiles/sdk/oem

endif

if AUDIO_HAL_SUPPORTED
AM_CPPFLAGS += -DBT_AUDIO_HAL_INTEGRATION
endif
if AUDIO_PAL_SUPPORTED
AM_CPPFLAGS += -DBT_AUDIO_PAL_INTEGRATION
AM_CPPFLAGS += @PABTAUDIO_CFLAGS@
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
            ../gattstest/src/GattsTest.cpp

if USE_GEN_GATT
cpp_sources += ../rsp/src/Rsp.cpp
if USE_BLE_SOCKET_MANAGER
cpp_sources += ../ble_sock_manager/src/BleSocketManager.cpp
cpp_sources += ../ble_sock_manager/src/BleWlanDppBootstrap.cpp
cpp_sources += ../ble_sock_manager/src/BleWifiControlService.cpp
cpp_sources += ../ble_sock_manager/src/BleGapService.cpp
AM_CPPFLAGS += -DUSE_BLE_SOCKET_MANAGER
endif
endif


if USE_BT_OBEX
cpp_sources +=  ../pbap_client/src/PbapClient.cpp \
              ../opp/src/Opp.cpp
endif

if USE_GST
requiredlibs = ../osi/libgstbtapposi.la ../utils/libgstbtappuuid.la -lpthread -lm -ldl -lrt -lbtvendorhal -lbttypes -lcutils -lsystemd
else
requiredlibs = ../osi/libbtapposi.la ../utils/libbtappuuid.la -lpthread -lm -ldl -lrt -lbtvendorhal -lbttypes -lcutils -lsystemd
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

if USE_GEN_SOCK_TEST

if USE_GLIB
bttestsocket_CFLAGS  = $(AM_CFLAGS) -DUSE_GLIB @GLIB_CFLAGS@
bttestsocket_CPPFLAGS  = $(AM_CPPFLAGS) -DUSE_GLIB @GLIB_CFLAGS@
bttestwifisocket_CFLAGS  = $(AM_CFLAGS) -DUSE_GLIB @GLIB_CFLAGS@
bttestwifisocket_CPPFLAGS  = $(AM_CPPFLAGS) -DUSE_GLIB @GLIB_CFLAGS@
endif

bin_PROGRAMS += bttestsocket bttestwifisocket
bttestsocket_CC = @CC@
bttestsocket_SOURCES = ../sockettest/src/MainTestSocket.cpp
bttestsocket_LDFLAGS = -lpthread @GLIB_LIBS@

bttestwifisocket_CC = @CC@
bttestwifisocket_SOURCES = ../sockettest/src/MainTestWifiSocket.cpp
bttestwifisocket_LDFLAGS = -lpthread @GLIB_LIBS@
endif

pkgconfigdir = $(libdir)/pkgconfig
