if USE_GEN_GATT
AM_CPPFLAGS = -O2 \
              -D_GNU_SOURCE \
              -std=c++11 \
              -I${WORKSPACE}/qcom-opensource/bt/bt-app-ipc/utils/include \
              -I${WORKSPACE}/qcom-opensource/bt/bt-app-ipc/gatt/native/include \
              -I${WORKSPACE}/qcom-opensource/bt/bt-app-ipc/gatt/lib/include \
              -I${WORKSPACE}/qcom-opensource/bt/bt-app-ipc/gatt/framework/include \
              -I${WORKSPACE}/qcom-opensource/bt/bt-app-ipc/include \
              -I${WORKSPACE}/qcom-opensource/bt/bt-app-ipc/ \
              -I${WORKSPACE}/vendor/qcom/opensource/bluetooth/hal/include/ \
              -I${WORKSPACE}/vendor/qcom/opensource/bluetooth/vhal/include \
              -I${WORKSPACE}/vendor/qcom/opensource/system/bt \
              -I${WORKSPACE}/vendor/qcom/opensource/system/bt/include \
              -I${WORKSPACE}/vendor/qcom/opensource/system/bt/types \
              -I${WORKSPACE}/system/core/include \
              @SYSTEMD_CFLAGS@

ACLOCAL_AMFLAGS = -I m4

cpp_sources = native/src/GattNativeInterfaceV2.cpp \
              native/src/GattNativeInterfaceV2_b.cpp \
              native/src/sdbus_ipc.cc \
              framework/src/AdvertiseData.cpp \
              framework/src/AdvertiseSettings.cpp \
              framework/src/AdvertisingSet.cpp \
              framework/src/AdvertisingSetParameters.cpp \
              framework/src/GattCharacteristic.cpp \
              framework/src/GattClient.cpp \
              framework/src/GattDescriptor.cpp \
              framework/src/GattDevice.cpp \
              framework/src/GattIncludedService.cpp \
              framework/src/GattLeAdvertiser.cpp \
              framework/src/GattLeScanner.cpp \
              framework/src/GattServer.cpp \
              framework/src/GattService.cpp \
              framework/src/PeriodicAdvertiseParameters.cpp \
              framework/src/PeriodicAdvertisingManager.cpp \
              framework/src/PeriodicAdvertisingReport.cpp \
              framework/src/ResultStorageDescriptor.cpp \
              framework/src/ScanFilter.cpp \
              framework/src/ScanRecord.cpp \
              framework/src/ScanResult.cpp \
              framework/src/ScanSettings.cpp \
              framework/src/TruncatedFilter.cpp \
              lib/src/AdvertiseHelper.cpp \
              lib/src/AdvertiserManager.cpp \
              lib/src/AdvtFilterOnFoundOnLostInfo.cpp \
              lib/src/CallbackInfo.cpp \
              lib/src/FilterParams.cpp \
              lib/src/GattDbElement.cpp \
              lib/src/GattLibService.cpp \
              lib/src/HandleMap.cpp \
              lib/src/PeriodicScanManager.cpp \
              lib/src/ScanClient.cpp \
              lib/src/ScanFilterQueue.cpp \
              lib/src/ScanManager.cpp

AM_LDFLAGS := -lbttypes -lchrome ../utils/libbtappuuid.la @SYSTEMD_LIBS@ 

library_includedir = $(pkgincludedir)
lib_LTLIBRARIES = libgengatt.la
libgengatt_la_CC = @CC@
libgengatt_la_SOURCES = $(cpp_sources)
libgengatt_la_CPPFLAGS = $(AM_CPPFLAGS)
libgengatt_la_LDFLAGS = $(AM_LDFLAGS) -static -lpthread -lm -ldl -lrt
endif
