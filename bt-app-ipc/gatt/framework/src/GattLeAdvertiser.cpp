/*
 * Copyright (c) 2018, The Linux Foundation. All rights reserved.
 * Not a Contribution.
 * Copyright (C) 2014 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "GattLeAdvertiser.hpp"
#include <exception>
#include <stdexcept>

#define LOGTAG "GattLeAdvertiser"

using namespace std;
namespace gatt {

GattLeAdvertiser *GattLeAdvertiser::sGattLeAdvertiser = NULL;

GattLeAdvertiser* GattLeAdvertiser::getGattLeAdvertiser()
{
  if(sGattLeAdvertiser == NULL) {
    ALOGE(LOGTAG " GattLeAdvertiser singleTon instance destroyed");
    sGattLeAdvertiser = new GattLeAdvertiser();
  }
  return sGattLeAdvertiser;
}

GattLeAdvertiser::GattLeAdvertiser()
{
}

GattLeAdvertiser::~GattLeAdvertiser()
{
  if (sGattLeAdvertiser != NULL) {
    cleanup();
  }
  sGattLeAdvertiser = NULL;
}

int GattLeAdvertiser::totalBytes(AdvertiseData *data, bool isFlagsIncluded)
{
  if (data == NULL) return 0;
  // Flags field is omitted if the advertising is not connectable.
  int size = (isFlagsIncluded) ? FLAGS_FIELD_BYTES : 0;

  if (!(data->getServiceUuids().empty())) {
    int num16BitUuids = 0;
    int num32BitUuids = 0;
    int num128BitUuids = 0;
    for (Uuid uuid : data->getServiceUuids()) {
      if (uuid.GetShortestRepresentationSize() == 2) {
        ++num16BitUuids;
      } else if (uuid.GetShortestRepresentationSize() == 4) {
        ++num32BitUuids;
      } else {
        ++num128BitUuids;
      }
    }

    // 16 bit service uuids are grouped into one field when doing advertising.
    if (num16BitUuids != 0) {
      size += OVERHEAD_BYTES_PER_FIELD + (num16BitUuids * Uuid::kNumBytes16);
    }
    // 32 bit service uuids are grouped into one field when doing advertising.
    if (num32BitUuids != 0) {
      size += OVERHEAD_BYTES_PER_FIELD + (num32BitUuids * Uuid::kNumBytes32);
    }
    // 128 bit service uuids are grouped into one field when doing advertising.
    if (num128BitUuids != 0) {
      size += OVERHEAD_BYTES_PER_FIELD + (num128BitUuids * Uuid::kNumBytes128);
    }
  }

  std::map<Uuid, std::vector<uint8_t> > map_ptr = data->getServiceData();
  for (std::map<Uuid, std::vector<uint8_t> >::iterator it = map_ptr.begin();
                                        it != map_ptr.end(); ++it) {
    Uuid uuid = it->first;
    std::vector<uint8_t> uu = Uuid::uuidToByte(uuid);
    int uuidLen = uu.size();

    if (!(data->getServiceData().count(uuid))) {
      ALOGE(LOGTAG " No Service Data");
      continue;
    }
    auto m = data->getServiceData().find(uuid);
    size += OVERHEAD_BYTES_PER_FIELD + uuidLen + byteLength(m->second);
  }

  std::map<int, std::vector<uint8_t>> map_ptr_2 = data->getManufacturerSpecificData();
  for (std::map<int, std::vector<uint8_t>>::iterator it = map_ptr_2.begin();
          it!= map_ptr_2.end(); ++it) {
    size += OVERHEAD_BYTES_PER_FIELD + MANUFACTURER_SPECIFIC_DATA_LENGTH
          + byteLength(it->second);
  }

  if (data->getIncludeTxPowerLevel()) {
    size += OVERHEAD_BYTES_PER_FIELD + 1; // tx power level value is one byte.
  }

  if (data->getIncludeDeviceName() && !mGattLibService->getDeviceName().empty()) {
    size += OVERHEAD_BYTES_PER_FIELD + mGattLibService->getDeviceName().length();
  }

  ALOGE(LOGTAG "totalBytes() total_size (%d)", size);
  return size;
}

int GattLeAdvertiser::byteLength(std::vector<uint8_t> array)
{
  return array.empty() ? 0 : array.size();
}

void GattLeAdvertiser::startAdvertising(AdvertiseSettings *settings,
                        AdvertiseData *advertiseData, AdvertisingSetCallback *callback)
{
  startAdvertising(settings, advertiseData, NULL, callback);
}

void GattLeAdvertiser::startAdvertising(AdvertiseSettings* settings,
                        AdvertiseData* advertiseData, AdvertiseData* scanResponse,
                        AdvertisingSetCallback* callback)
{
  if (callback == NULL) {
    throw std::invalid_argument("callback cannot be null");
  }

  if (mGattLibService == NULL) {
    mGattLibService = GattLibService::getGatt();
    if (mGattLibService == NULL) {
      ALOGE(LOGTAG " Failed to get Bluetooth gatt - NULL ");
      postStartSetFailure(callback,
             AdvertisingSetCallback::ADVERTISE_FAILED_INTERNAL_ERROR);
      return;
    }
  }
  bool isConnectable = settings->isConnectable();

  if (totalBytes(advertiseData, isConnectable) > MAX_LEGACY_ADVERTISING_DATA_BYTES
          || totalBytes(scanResponse, false) > MAX_LEGACY_ADVERTISING_DATA_BYTES) {
          ALOGE(LOGTAG "startAdvertising() :: ADVERTISE_FAILED_DATA_TOO_LARGE ");
    postStartFailure(callback, AdvertisingSetCallback::ADVERTISE_FAILED_DATA_TOO_LARGE);
    return;
  }

  int interval = 0;
  if (settings->getMode() == AdvertiseSettings::ADVERTISE_MODE_LOW_POWER) {
    interval = 1600; //1s
  } else if (settings->getMode() == AdvertiseSettings::ADVERTISE_MODE_BALANCED) {
    interval = 400; //250ms
  } else if (settings->getMode() == AdvertiseSettings::ADVERTISE_MODE_LOW_LATENCY) {
    interval = 160; //100ms
  }

  int txPowerLevel = 0;
  if (settings->getTxPowerLevel() == AdvertiseSettings::ADVERTISE_TX_POWER_ULTRA_LOW) {
    txPowerLevel = -21;
  } else if (settings->getTxPowerLevel() == AdvertiseSettings::ADVERTISE_TX_POWER_LOW) {
    txPowerLevel = -15;
  } else if (settings->getTxPowerLevel() == AdvertiseSettings::ADVERTISE_TX_POWER_MEDIUM) {
    txPowerLevel = -7;
  } else if (settings->getTxPowerLevel() == AdvertiseSettings::ADVERTISE_TX_POWER_HIGH) {
    txPowerLevel = 1;
  }

  AdvertisingSetParameters *parameters =
                            AdvertisingSetParameters::Builder()
                            .setLegacyMode(true)
                            .setConnectable(isConnectable)
                            .setScannable(true)
                            .setInterval(interval)
                            .setTxPowerLevel(txPowerLevel)
                            .build();

  int duration = 0;
  int timeoutMillis = settings->getTimeout();
  if (timeoutMillis > 0) {
    duration = (timeoutMillis < 10) ? 1 : timeoutMillis / 10;
  }

  startAdvertisingSet(parameters, advertiseData, scanResponse, NULL, NULL,
          duration, 0, callback);

  delete parameters;
}

void GattLeAdvertiser::stopAdvertising(AdvertisingSetCallback *callback)
{
  if (callback == NULL) {
    throw std::invalid_argument("callback cannot be null");
  }

  stopAdvertisingSet(callback);
}

void GattLeAdvertiser::startAdvertisingSet(AdvertisingSetParameters *parameters,
                        AdvertiseData *advertiseData, AdvertiseData *scanResponse,
                        PeriodicAdvertiseParameters *periodicParameters,
                        AdvertiseData *periodicData, AdvertisingSetCallback *callback)
{
  startAdvertisingSet(parameters, advertiseData, scanResponse, periodicParameters,
            periodicData, 0, 0, callback);
}

void GattLeAdvertiser::startAdvertisingSet(AdvertisingSetParameters *parameters,
                                        AdvertiseData *advertiseData, AdvertiseData *scanResponse,
                                        PeriodicAdvertiseParameters *periodicParameters,
                                        AdvertiseData *periodicData, int duration,
                                        int maxExtendedAdvertisingEvents,
                                        AdvertisingSetCallback *callback)
{
  if (callback == NULL) {
    throw std::invalid_argument("callback cannot be null");
  }

  if (mGattLibService == NULL) {
    mGattLibService = GattLibService::getGatt();
    if (mGattLibService == NULL) {
      ALOGE(LOGTAG " Failed to get Bluetooth gatt - NULL ");
      postStartSetFailure(callback,
             AdvertisingSetCallback::ADVERTISE_FAILED_INTERNAL_ERROR);
      return;
    }
  }

  bool isConnectable = parameters->isConnectable();
  if (parameters->isLegacy()) {
    if (totalBytes(advertiseData, isConnectable) > MAX_LEGACY_ADVERTISING_DATA_BYTES) {
      ALOGE(LOGTAG "startAdvertisingSet() Legacy advertising data too big");
      throw std::invalid_argument("Legacy advertising data too big");
    }

    if (totalBytes(scanResponse, false) > MAX_LEGACY_ADVERTISING_DATA_BYTES) {
      ALOGE(LOGTAG "startAdvertisingSet() Legacy scan response data too big");
      throw std::invalid_argument("Legacy scan response data too big");
    }
  } else {
      bool supportCodedPhy = mGattLibService->isLeCodedPhySupported();
      bool support2MPhy = mGattLibService->isLe2MPhySupported();
      int pphy = parameters->getPrimaryPhy();
      int sphy = parameters->getSecondaryPhy();
      if (pphy == GattDevice::PHY_LE_CODED && !supportCodedPhy) {
        ALOGE(LOGTAG "startAdvertisingSet():: Unsupported primary PHY selected ");
        throw std::invalid_argument("Unsupported primary PHY selected");
      }

      if ((sphy == GattDevice::PHY_LE_CODED && !supportCodedPhy)
             || (sphy == GattDevice::PHY_LE_2M && !support2MPhy)) {
             ALOGE(LOGTAG " startAdvertisingSet():: Unsupported secondary PHY selected ");
         throw std::invalid_argument("Unsupported secondary PHY selected");
      }

      int maxData = mGattLibService->getLeMaximumAdvertisingDataLength();
      ALOGD(LOGTAG " startAdvertisingSet() maxData %d",
                                    mGattLibService->getLeMaximumAdvertisingDataLength());
      if (totalBytes(advertiseData, isConnectable) > maxData) {
         throw std::invalid_argument("Advertising data too big");
      }

      if (totalBytes(scanResponse, false) > maxData) {
         throw std::invalid_argument("Scan response data too big");
      }

      if (totalBytes(periodicData, false) > maxData) {
         throw std::invalid_argument("Periodic advertising data too big");
      }

      bool supportPeriodic = mGattLibService->isLePeriodicAdvertisingSupported();
      if (periodicParameters != NULL && !supportPeriodic) {
         throw std::invalid_argument(
                 "Controller does not support LE Periodic Advertising");
      }
  }

  if (maxExtendedAdvertisingEvents < 0 || maxExtendedAdvertisingEvents > 255) {
    throw new std::invalid_argument(
           "maxExtendedAdvertisingEvents out of range: " + maxExtendedAdvertisingEvents);
  }

  if (maxExtendedAdvertisingEvents != 0
     && !mGattLibService->isLePeriodicAdvertisingSupported()) {
    throw std::invalid_argument(
           "Can't use maxExtendedAdvertisingEvents with controller that don't support \
                   LE Extended Advertising");
  }

  if (duration < 0 || duration > 65535) {
    throw new std::invalid_argument("duration out of range: " + duration);
  }

  IAdvertisingSetCallback *cb = this;
  mCb = callback;

  auto m = mCallback.find(callback);
  if (m == mCallback.end()) {
    mCallback.insert({{callback,this}});
    cb = this;
  }

  try {
    mGattLibService->startAdvertisingSet(parameters, advertiseData, scanResponse,
      periodicParameters, periodicData, duration, maxExtendedAdvertisingEvents,cb);
  } catch (std::exception& e) {
    ALOGE(LOGTAG " Failed to start advertising set - %s ", e.what());
    postStartSetFailure(callback,
           AdvertisingSetCallback::ADVERTISE_FAILED_INTERNAL_ERROR);
    return;
    }
}

void GattLeAdvertiser::stopAdvertisingSet(AdvertisingSetCallback *callback)
{
 if (callback == NULL) {
    throw std::invalid_argument("callback cannot be null");
  }

  auto tCb = mCallback.find(callback);
  if (tCb == mCallback.end()) {
  ALOGE(LOGTAG " stopAdvertisingSet() No callback ");
  return;
  }

  mCallback.erase(callback);
  GattLibService *gatt;
  try {
    gatt = GattLibService::getGatt();
    gatt->stopAdvertisingSet(tCb->second);
  } catch (std::exception& e) {
    ALOGE(LOGTAG " Failed to stop advertising - %s ", e.what());
  }
}

void GattLeAdvertiser::cleanup()
{
  mCb = NULL;
  mCallback.clear();
  for (auto adv_map:mAdvertisingSets) {
    delete adv_map.second;
  }
  mAdvertisingSets.clear();
  sGattLeAdvertiser = NULL;
}

void GattLeAdvertiser::postStartSetFailure(AdvertisingSetCallback *callback,const int error)
{
   callback->onAdvertisingSetStarted(NULL, 0, error);
}

void GattLeAdvertiser::postStartFailure(AdvertisingSetCallback *callback, const int error)
{
  callback->onStartFailure(error);
}

void GattLeAdvertiser::postStartSuccess(AdvertisingSetCallback *callback,
                                                                    AdvertiseSettings* settings)
{
  callback->onStartSuccess(settings);
}

void GattLeAdvertiser::onAdvertisingSetStarted(int advertiserId, int txPower, int status) {

  if (status != AdvertisingSetCallback::ADVERTISE_SUCCESS) {
    mCb->onAdvertisingSetStarted(NULL, 0, status);
    //mCallback.erase(callback);
    return;
  }

  AdvertisingSet *advertisingSet = new AdvertisingSet(advertiserId);

  mAdvertisingSets.insert({{advertiserId, advertisingSet}});
  mCb->onAdvertisingSetStarted(advertisingSet, txPower, status);
  delete advertisingSet;
}

void GattLeAdvertiser::onOwnAddressRead(int advertiserId, int addressType, string address)
{
  auto tAdvId = mAdvertisingSets.find(advertiserId);
  if (tAdvId == mAdvertisingSets.end()) {
    ALOGE(LOGTAG " onOwnAddressRead() Advertising Set not found");
    return;
  }

  AdvertisingSet *advertisingSet = tAdvId->second;
  mCb->onOwnAddressRead(advertisingSet, addressType, address);
}

void GattLeAdvertiser::onAdvertisingSetStopped(int advertiserId)
{
  auto tAdvId = mAdvertisingSets.find(advertiserId);
  if (tAdvId == mAdvertisingSets.end()) {
    ALOGE(LOGTAG " onAdvertisingSetStopped() Advertising Set not found");
    return;
  }

  AdvertisingSet *advertisingSet = tAdvId->second;
  mCb->onAdvertisingSetStopped(advertisingSet);
  mAdvertisingSets.erase(advertiserId);
  //mCallback.erase(callback);
}

void GattLeAdvertiser::onAdvertisingEnabled(int advertiserId, bool enabled, int status)
{
  auto tAdvId = mAdvertisingSets.find(advertiserId);
  if (tAdvId == mAdvertisingSets.end()) {
    ALOGE(LOGTAG " onAdvertisingEnabled() Advertising Set not found");
    return;
  }

  AdvertisingSet *advertisingSet = tAdvId->second;
  mCb->onAdvertisingEnabled(advertisingSet, enabled, status);
}

void GattLeAdvertiser::onAdvertisingDataSet(int advertiserId, int status)
{
  auto tAdvId = mAdvertisingSets.find(advertiserId);
  if (tAdvId == mAdvertisingSets.end()) {
    ALOGE(LOGTAG " onAdvertisingDataSet() Advertising Set not found");
    return;
  }

  AdvertisingSet *advertisingSet = tAdvId->second;
  mCb->onAdvertisingDataSet(advertisingSet, status);
}

void GattLeAdvertiser::onScanResponseDataSet(int advertiserId, int status)
{
  auto tAdvId = mAdvertisingSets.find(advertiserId);
  if (tAdvId == mAdvertisingSets.end()) {
    ALOGE(LOGTAG " onScanResponseDataSet() Advertising Set not found");
    return;
  }

  AdvertisingSet *advertisingSet = tAdvId->second;
  mCb->onScanResponseDataSet(advertisingSet, status);
}

void GattLeAdvertiser::onAdvertisingParametersUpdated(int advertiserId, int txPower, int status)
{
  auto tAdvId = mAdvertisingSets.find(advertiserId);
  if (tAdvId == mAdvertisingSets.end()) {
    ALOGE(LOGTAG " onAdvertisingParametersUpdated() Advertising Set not found");
    return;
  }

  AdvertisingSet *advertisingSet = tAdvId->second;
  mCb->onAdvertisingParametersUpdated(advertisingSet, txPower, status);
}

void GattLeAdvertiser::onPeriodicAdvertisingParametersUpdated(int advertiserId, int status)
{
  auto tAdvId = mAdvertisingSets.find(advertiserId);
  if (tAdvId == mAdvertisingSets.end()) {
    ALOGE(LOGTAG " onPeriodicAdvertisingParametersUpdated() Advertising Set not found");
    return;
  }

  AdvertisingSet *advertisingSet = tAdvId->second;
  mCb->onPeriodicAdvertisingParametersUpdated(advertisingSet, status);
}

void GattLeAdvertiser::onPeriodicAdvertisingDataSet(int advertiserId, int status)
{
  auto tAdvId = mAdvertisingSets.find(advertiserId);
  if (tAdvId == mAdvertisingSets.end()) {
    ALOGE(LOGTAG " onPeriodicAdvertisingDataSet() Advertising Set not found");
    return;
  }

  AdvertisingSet *advertisingSet = tAdvId->second;
  mCb->onPeriodicAdvertisingDataSet(advertisingSet, status);
}

void GattLeAdvertiser::onPeriodicAdvertisingEnabled(int advertiserId, bool enable, int status)
{
  auto tAdvId = mAdvertisingSets.find(advertiserId);
  if (tAdvId == mAdvertisingSets.end()) {
    ALOGE(LOGTAG " onPeriodicAdvertisingEnabled() Advertising Set not found");
    return;
  }

  AdvertisingSet *advertisingSet = tAdvId->second;
  mCb->onPeriodicAdvertisingEnabled(advertisingSet, enable, status);
}
}
