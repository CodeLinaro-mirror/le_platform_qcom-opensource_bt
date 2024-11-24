if USE_GEN_GATT
AM_CPPFLAGS = -O2 \
              -D_GNU_SOURCE \
              -std=c++11 \
              -I${BT_SOURCE}/btapp/bt-app/utils/include \
              -I${BT_SOURCE}/btapp/bt-app/gatt/native/include \
              -I${BT_SOURCE}/btapp/bt-app/gatt/lib/include \
              -I${BT_SOURCE}/btapp/bt-app/gatt/framework/include \
              -I${BT_SOURCE}/btapp/bt-app/include \
              -I${BT_SOURCE}/btapp/bt-app/ \
              -I${BT_SOURCE}/bt_audio/hal/include/ \
              -I${BT_SOURCE}/stack/bluetooth_ext/vhal/include \
              -I${BT_SOURCE}/stack/system/bt \
              -I${BT_SOURCE}/stack/system/bt/include \
              -I${BT_SOURCE}/stack/system/bt/types

ACLOCAL_AMFLAGS = -I m4

cpp_sources = native/src/GattNativeInterfaceV2.cpp \
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

AM_LDFLAGS := -lchrome ../utils/libbtappuuid.la

library_includedir = $(pkgincludedir)
lib_LTLIBRARIES = libgengatt.la
libgengatt_la_CC = @CC@
libgengatt_la_SOURCES = $(cpp_sources)
libgengatt_la_CPPFLAGS = $(AM_CPPFLAGS)
libgengatt_la_LDFLAGS = $(AM_LDFLAGS) -static -lpthread -lm -ldl -lrt -version-number @LT_VERSION_NUMBER@
endif
