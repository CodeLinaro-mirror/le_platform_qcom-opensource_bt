/*
 * Copyright (c) 2018, The Linux Foundation. All rights reserved.
 * Not a Contribution.
 * Copyright (C) 2017 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License"); you may not
 * use this file except in compliance with the License. You may obtain a copy of
 * the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
 * License for the specific language governing permissions and limitations under
 * the License.
 */

#include <stdexcept>
#include <exception>
#include "PeriodicAdvertisingManager.hpp"
#include <GattLibService.hpp>

#define LOGTAG "PeriodicAdvertisingManager"

namespace gatt {

PeriodicAdvertisingManager* PeriodicAdvertisingManager::pAdvManager = NULL;

PeriodicAdvertisingManager* PeriodicAdvertisingManager::getPeriodicAdvertisingManager()
{
  if(pAdvManager == NULL)
    pAdvManager = new PeriodicAdvertisingManager();

  return pAdvManager;
}

PeriodicAdvertisingManager::PeriodicAdvertisingManager() {    }

PeriodicAdvertisingManager::~PeriodicAdvertisingManager()
{
  if (pAdvManager != NULL) {
    delete pAdvManager;
    pAdvManager = NULL;
  }
  mCallbackMap.clear();
}

void PeriodicAdvertisingManager::registerSync(ScanResult *scanResult, int skip, int timeout,
      PeriodicAdvertisingCallback *callback)
{
  if (callback == NULL) {
      throw std::invalid_argument("callback can't be null");
  }

  if (scanResult == NULL) {
      throw std::invalid_argument("scanResult can't be null");
  }

  if (scanResult->getAdvertisingSid() == ScanResult::SID_NOT_PRESENT) {
      throw std::invalid_argument("scanResult must contain a valid sid");
  }

  if (skip < SKIP_MIN || skip > SKIP_MAX) {
      throw std::invalid_argument(
              "timeout must be between " + std::to_string(TIMEOUT_MIN) + " and "
                      + std::to_string(TIMEOUT_MAX));
  }

  if (timeout < TIMEOUT_MIN || timeout > TIMEOUT_MAX) {
      throw std::invalid_argument(
              "timeout must be between " + std::to_string(TIMEOUT_MIN) + " and "
                + std::to_string(TIMEOUT_MAX));
  }

  GattLibService *gatt;
  try {
      gatt = GattLibService::getGatt();
  } catch (std::exception &e) {
      ALOGE(LOGTAG "Failed to get Bluetooth gatt - %s", e.what());
      callback->onSyncEstablished(0, scanResult->getDevice(), scanResult->getAdvertisingSid(),
              skip, timeout,
              PeriodicAdvertisingCallback::SYNC_NO_RESOURCES);
      return;
  }

  mCallbackMap.insert({{callback, this}});

  try {
      gatt->registerSync(scanResult, skip, timeout, this);
  } catch (std::exception &e) {
      ALOGE(LOGTAG "Failed to register sync - %s", e.what());
      return;
  }
}

void PeriodicAdvertisingManager::unregisterSync(PeriodicAdvertisingCallback *callback)
{
  if (callback == NULL) {
      throw std::invalid_argument("callback can't be null");
  }

  GattLibService *gatt;
  try {
      gatt = GattLibService::getGatt();
  } catch (std::exception &e) {
      ALOGE(LOGTAG "Failed to get Bluetooth gatt - %s", e.what());
      return;
  }

  std::unordered_map<PeriodicAdvertisingCallback*,
      IPeriodicAdvertisingCallback*>:: iterator it = mCallbackMap.find(callback);

  if (it == mCallbackMap.end()) {
      throw std::invalid_argument("callback was not properly registered");
  }

  try {
      gatt->unregisterSync(it->second);
  } catch (std::exception &e) {
      ALOGE(LOGTAG "Failed to cancel sync creation - %s", e.what());
      return;
  }
}

void PeriodicAdvertisingManager::filterPaAdvReport(uint8_t enable, PeriodicAdvertisingCallback *callback)
{
  if (callback == NULL) {
      throw std::invalid_argument("callback can't be null");
  }
   GattLibService *gatt;
  try {
      gatt = GattLibService::getGatt();
  } catch (std::exception &e) {
      ALOGE(LOGTAG "Failed to get Bluetooth gatt - %s", e.what());
      return;
  }

  std::unordered_map<PeriodicAdvertisingCallback*,
      IPeriodicAdvertisingCallback*>:: iterator it = mCallbackMap.find(callback);

  if (it == mCallbackMap.end()) {
      throw std::invalid_argument("callback was not properly registered");
  }

  try {
      gatt->enablePaAdvReport(enable, it->second);
  } catch (std::exception &e) {
      ALOGE(LOGTAG "Failed to cancel sync creation - %s", e.what());
      return;
  }
}

 void PeriodicAdvertisingManager::onSyncEstablished(int syncHandle, string device,
        int advertisingSid, int skip, int timeout, int status)
{
  std::unordered_map<PeriodicAdvertisingCallback*,
            IPeriodicAdvertisingCallback*>::iterator it = mCallbackMap.begin();
  for (; it != mCallbackMap.end(); ++it) {
      if (it->second == this)
        break;
  }

  if (it == mCallbackMap.end()) {
    ALOGE(LOGTAG "Callback was not properly registered");
    return;
  }

  PeriodicAdvertisingCallback *cb = it->first;
  cb->onSyncEstablished(syncHandle, device, advertisingSid, skip,
                          timeout, status);

  if (status != PeriodicAdvertisingCallback::SYNC_SUCCESS) {
      // App can still unregister the sync until notified it failed. Remove
      // callback
      // after app was notifed.
      mCallbackMap.erase(it);
  }
}

void PeriodicAdvertisingManager::onPeriodicAdvertisingReport(PeriodicAdvertisingReport *report)
{
  std::unordered_map<PeriodicAdvertisingCallback*,
            IPeriodicAdvertisingCallback*>::iterator it = mCallbackMap.begin();
  for (; it != mCallbackMap.end(); ++it) {
      if (it->second == this)
        break;
  }

  if (it == mCallbackMap.end()) {
    ALOGE(LOGTAG "Callback was not properly registered");
    return;
  }

  PeriodicAdvertisingCallback *cb = it->first;
  cb->onPeriodicAdvertisingReport(report);
}

void PeriodicAdvertisingManager::onSyncLost(int syncHandle)
{
  std::unordered_map<PeriodicAdvertisingCallback*,
            IPeriodicAdvertisingCallback*>::iterator it = mCallbackMap.begin();
  for (; it != mCallbackMap.end(); ++it) {
      if (it->second == this)
        break;
  }

  if (it == mCallbackMap.end()) {
    ALOGE(LOGTAG "Callback was not properly registered");
    return;
  }

  PeriodicAdvertisingCallback *cb = it->first;
  cb->onSyncLost(syncHandle);
  // App can still unregister the sync until notified it's lost.
  // Remove callback after app was notifed.
  mCallbackMap.erase(it);
}
}
