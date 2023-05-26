/*
 * Copyright (c) 2018, The Linux Foundation. All rights reserved.
 * Not a Contribution.
 * Copyright (C) 2017 The Android Open Source Project
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
#include "PeriodicScanManager.hpp"
#include "GattNativeInterfaceV2.hpp"
#include "ScanResult.hpp"
#include "ScanRecord.hpp"
#include "PeriodicAdvertisingReport.hpp"

#define LOGTAG "PeriodicScanManager"

using namespace std;
namespace gatt {

int PeriodicScanManager::sTempRegistrationId = -1;

PeriodicScanManager::PeriodicScanManager(GattNativeInterfaceV2 *native)
{
  mNative = native;
}

void PeriodicScanManager::start()
{
}

void PeriodicScanManager::cleanup()
{
  if (DBG) {
      ALOGD(LOGTAG "cleanup()");
  }
  mSyncs.clear();
  sTempRegistrationId = -1;
  mNative = NULL;
}

PeriodicScanManager::~PeriodicScanManager()
{
  this->cleanup();
}

IPeriodicAdvertisingCallback* PeriodicScanManager::findSync(int syncHandle)
{
  for (std::unordered_map<int, IPeriodicAdvertisingCallback*>::iterator it = mSyncs.begin();
            it != mSyncs.end(); ++it) {
    if (it->first == syncHandle) {
       return it->second;
    }
  }
  return NULL;
}

void PeriodicScanManager::onSyncStarted(int regId, int syncHandle, int sid, int addressType,
                                            string address, int phy, int interval, int status)
{
  if (DBG) {
    ALOGD(LOGTAG "onSyncStarted() - regId= %d, syncHandle=%d, status=%d",
                                        regId, syncHandle, status);
  }

  IPeriodicAdvertisingCallback *cb = findSync(regId);
  if (cb == NULL) {
    ALOGI(LOGTAG "onSyncStarted() - no callback found for regId %d", regId);
    // Sync was stopped before it was properly registered.
    if (status == 0) {
      mNative->stopSyncNative(syncHandle);
    }
    return;
  }

  // erase regId due to stopSync() may find it instead of syncHandle
  mSyncs.erase(regId);
  if (status == 0) {
    mSyncs.insert({{syncHandle, cb}});
  } else {
    mSyncs.erase(syncHandle);
  }
  cb->onSyncEstablished(syncHandle, address, sid, 0, 0, status);

  // TODO: fix callback arguments
  // callback->onSyncStarted(syncHandle, tx_power, status);
}

void PeriodicScanManager::onSyncReport(int syncHandle, int txPower, int rssi, int dataStatus,
                                                  std::vector<uint8_t> data)
{
  if (DBG) {
    ALOGD(LOGTAG "onSyncReport() - syncHandle=%d", syncHandle);
  }

  IPeriodicAdvertisingCallback *cb = findSync(syncHandle);
  if (cb == NULL) {
    ALOGI(LOGTAG "onSyncReport() - no callback found for syncHandle %d", syncHandle);
    return;
  }

  PeriodicAdvertisingReport *report =
          new PeriodicAdvertisingReport(syncHandle, txPower, rssi, dataStatus,
                  ScanRecord::parseFromBytes(data));
  cb->onPeriodicAdvertisingReport(report);
  if (report != NULL) {
    delete(report);
  }
}

void PeriodicScanManager::onSyncLost(int syncHandle)
{
  if (DBG) {
    ALOGD(LOGTAG "onSyncLost() - syncHandle=%d", syncHandle);
  }

  IPeriodicAdvertisingCallback *cb = findSync(syncHandle);
  if (cb == NULL) {
    ALOGI(LOGTAG "onSyncLost() - no callback found for syncHandle %d", syncHandle);
    return;
  }

  mSyncs.erase(syncHandle);
  cb->onSyncLost(syncHandle);
}


void PeriodicScanManager::startSync(ScanResult *scanResult, int skip, int timeout,
        IPeriodicAdvertisingCallback *callback)
{

  string address = scanResult->getDevice();
  int sid = scanResult->getAdvertisingSid();

  int cbId = --sTempRegistrationId;
  mSyncs.insert({{cbId,callback}});

  if (DBG) {
    ALOGD(LOGTAG "startSync() - reg_id=%d", cbId);
  }
  mNative->startSyncNative(sid, address, skip, timeout, cbId);
}

void PeriodicScanManager::stopSync(IPeriodicAdvertisingCallback *callback)
{
  if (DBG) {
    ALOGD(LOGTAG "stopSync() ");
  }
  std::unordered_map<int, IPeriodicAdvertisingCallback*>::iterator it = mSyncs.begin();
  for (; it != mSyncs.end(); ++it) {
    if (it->second == callback)
      break;
  }

  if (it == mSyncs.end()) {
    ALOGE(LOGTAG "stopSync() - no client found for callback");
    fprintf(stdout, "stopSync() - no client found for callback");
    return;
  }

  int syncHandle = it->first;

  if (syncHandle < 0) {
    ALOGD(LOGTAG "stopSync() - not finished registration yet");
    fprintf(stdout, "stopSync() - not finished registration yet");
    // Sync will be freed once initiated in onSyncStarted()
    return;
  }

  mNative->stopSyncNative(syncHandle);
}

void PeriodicScanManager::enablePaAdvReport(uint8_t enable, IPeriodicAdvertisingCallback *callback)
{
  if (DBG) {
    ALOGD(LOGTAG "enablePaAdvReport() ");
  }
  std::unordered_map<int, IPeriodicAdvertisingCallback*>::iterator it = mSyncs.begin();
  for (; it != mSyncs.end(); ++it) {
    if (it->second == callback)
      break;
  }

  if (it == mSyncs.end()) {
    ALOGE(LOGTAG "enablePaAdvReport() - no client found for callback");
    fprintf(stdout, "enablePaAdvReport() - no client found for callback");
    return;
  }

  int syncHandle = it->first;

  if (syncHandle < 0) {
    ALOGD(LOGTAG "enablePaAdvReport() - not finished registration yet");
    fprintf(stdout, "enablePaAdvReport() - not finished registration yet");
    // Sync will be freed once initiated in onSyncStarted()
    return;
  }

  mNative->enablePaScanResultNative(syncHandle, enable);
}

}
