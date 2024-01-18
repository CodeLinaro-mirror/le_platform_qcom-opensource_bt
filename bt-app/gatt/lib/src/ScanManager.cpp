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

#include "ScanManager.hpp"
#include <deque>
#include <unordered_set>
#include <unordered_map>
#include <climits>
#include <algorithm>
#include <exception>
#include <chrono>
#include <utility>
#include <vector>
#include <functional>
#define LOGTAG "ScanManager"

using namespace std;
namespace gatt{

std::mutex lock;
std::condition_variable cv;
bool countDown = false;
ScanManager::ScanManager(GattNativeInterfaceV2 *mGattIf)
{
  mNative = mGattIf;
  mCurUsedTrackableAdvertisements = 0;
}

void ScanManager::start()
{
  mScanNative = new ScanNative(this);
  batch_scan_timer = alarm_new();
  if(batch_scan_timer == NULL) {
    ALOGE(LOGTAG " batch scan timer not set");
  }
}

void ScanManager::cleanup()
{
  mRegularScanClients.clear();
  mBatchClients.clear();
  if(mScanNative != NULL) {
    delete(mScanNative);
    mScanNative = NULL;
  }
  mNative = NULL;
  alarm_free(batch_scan_timer);
  batch_scan_timer = NULL;
}

ScanManager::~ScanManager()
{
  this->cleanup();
}

void ScanManager::registerScanner(Uuid uuid)
{
  ALOGD(LOGTAG " registerScanner()");
  if(mNative)
    mNative->registerScannerNative(uuid);
}

void ScanManager::unregisterScanner(int scannerId)
{
  if(mNative)
    mNative->unregisterScannerNative(scannerId);
}

/**
* Returns the regular scan queue.
*/
std::unordered_set<ScanClient*> ScanManager::getRegularScanQueue()
{
  return mRegularScanClients;
}

/**
* Returns batch scan queue.
*/
std::unordered_set<ScanClient*>& ScanManager::getBatchScanQueue()
{
  return mBatchClients;
}

/**
* Returns a set of full batch scan clients.
*/
std::unordered_set<ScanClient*> ScanManager::getFullBatchScanQueue()
{
  // TODO: split full batch scan clients and truncated batch clients so we don't need to
  // construct this every time.
  std::unordered_set<ScanClient*> fullBatchClients;
  for (ScanClient *client : mBatchClients) {
    if (client->settings->getScanResultType() == ScanSettings::SCAN_RESULT_TYPE_FULL) {
      fullBatchClients.insert(client);
    }
  }
  return fullBatchClients;
}

void ScanManager::startScan(ScanClient *client)
{
  handleStartScan( client);
}

void ScanManager::stopScan(ScanClient *client)
{
  handleStopScan( client);
}

void ScanManager::flushBatchScanResults(ScanClient *client)
{
  handleFlushBatchResults( client);
}

void ScanManager::callbackDone(int scannerId, int status)
{
  if (DBG) {
    ALOGD(LOGTAG " callback done for scannerId %d status %d ", scannerId, status);
  }
  if( status == 0)
  {
      std::lock_guard<std::mutex> lk(lock);
      countDown = true;
      ALOGD(LOGTAG " callbackDone countDown: %d , status :%d", countDown, status);
      cv.notify_all();
  }
}

void ScanManager::resetCountDownLatch()
{
  ALOGE(LOGTAG "resetCountDownLatch");
  std::lock_guard<std::mutex> lk(lock);
  countDown = false;
}
void ScanManager::waitForCallback()
{
  std::unique_lock<std::mutex> lk(lock);
  if (cv.wait_for(lk, std::chrono::milliseconds(OPERATION_TIME_OUT_MILLIS), [] {return countDown;})) {
    ALOGD(LOGTAG "waitForCallback() : LatchDown countDown is true");
  } else {
    ALOGE(LOGTAG "waitForCallback() : LatchDown Timeout");
  }
}

int ScanManager::millsToUnit(int milliseconds)
{
  std::chrono::milliseconds ms(milliseconds);
  std::chrono::microseconds us = std::chrono::duration_cast<std::chrono::microseconds> (ms);
  return (int) (us.count() / MICROS_PER_UNIT);
}

bool ScanManager::isFilteringSupported() {
  GattLibService *mGatt = GattLibService::getGatt();
  if(mGatt == NULL) return false;
  return mGatt->isOffloadedFilteringSupported();
}

void ScanManager::handleStartScan(ScanClient *client)
{
  bool isFiltered = !client->filters.empty();
  if (DBG) {
    ALOGD(LOGTAG " handling starting scan");
  }

  if (!isScanSupported(client)) {
    ALOGE(LOGTAG " Scan settings not supported");
    return;
  }

  if (mRegularScanClients.count(client) || mBatchClients.count(client)) {
    ALOGE(LOGTAG " Scan already started");
    return;
  }

  // Begin scan operations.
  if (isBatchClient(client)) {
    mBatchClients.insert(client);
    if(mScanNative != NULL)
      mScanNative->startBatchScan(client);
  } else {
    mRegularScanClients.insert(client);
    if (mScanNative != NULL) {
      bool ret = mScanNative->startRegularScan(client);
      if (!ret) {
        mRegularScanClients.erase(client);
        return;
      }
    }

  if (mScanNative != NULL && !mScanNative->isOpportunisticScanClient(client)) {
    mScanNative->configureRegularScanParams();

    if (mScanNative != NULL && !mScanNative->isExemptFromScanDowngrade(client)) {
      mScanNative->regularScanTimeout(client);
    }
  }
  }
}

void ScanManager:: handleStopScan(ScanClient *client)
{
  bool appDied;
  int scannerId;
  if (client == NULL) {
    return;
  }

  // The caller may pass a dummy client with only clientIf
  // and appDied status. Perform the operation on the
  // actual client in that case.
  appDied = client->appDied;
  scannerId = client->scannerId;
  if(mScanNative != NULL)
    client = mScanNative->getRegularScanClient(scannerId);
  if (client == NULL) {
    client = mScanNative->getBatchScanClient(scannerId);

    if(client == NULL) {
      if (DBG) ALOGD(LOGTAG " batch client is null");
      return;
    }
  }

  if (mRegularScanClients.count(client) && mScanNative != NULL) {
    mScanNative->stopRegularScan(client);

    if (!mScanNative->isOpportunisticScanClient(client)) {
      mScanNative->configureRegularScanParams();
    }
  } else if (mBatchClients.count(client) && mScanNative != NULL) {
      mScanNative->stopBatchScan(client);
  }
  if (appDied) {
    if (DBG) {
      ALOGD(LOGTAG " app died, unregister scanner - %d", client->scannerId);
    }
    unregisterScanner(client->scannerId);
  }

}

void ScanManager:: handleFlushBatchResults(ScanClient *client)
{
  if (!mBatchClients.count(client)) {
    return;
  }
  if (mScanNative != NULL)
    mScanNative->flushBatchResults(client->scannerId);
}

bool ScanManager:: isBatchClient(ScanClient *client)
{
  if (client == NULL || client->settings == NULL) {
    return false;
  }

  return client->settings->getCallbackType() == ScanSettings::CALLBACK_TYPE_ALL_MATCHES
          && client->settings->getReportDelayMillis() != 0;
}

bool ScanManager:: isScanSupported(ScanClient *client)
{
   if (client == NULL || client->settings == NULL) {
     return true;
   }

   if (isFilteringSupported()) {
     return true;
   }
   return client->settings->getCallbackType() == ScanSettings::CALLBACK_TYPE_ALL_MATCHES
           && client->settings->getReportDelayMillis() == 0;
}

ScanManager::BatchScanParams::BatchScanParams()
{
  scanMode = -1;
  fullScanscannerId = -1;
  truncatedScanscannerId = -1;
}

bool ScanManager::BatchScanParams::operator==(const BatchScanParams& obj) const
{
  return (*this == obj);
}

bool ScanManager::BatchScanParams::operator!=(const BatchScanParams& obj) const
{
  if (*this == obj) {
    return true;
  }

  return scanMode == obj.scanMode && fullScanscannerId == obj.fullScanscannerId
          && truncatedScanscannerId == obj.truncatedScanscannerId;
}

ScanManager::PhyInfo::PhyInfo(int scanPhy, int scanModeLE1M, int scanModeLECoded)
{
  this->scanPhy = scanPhy;
  this->scanModeLE1M = scanModeLE1M;
  this->scanModeLECoded =scanModeLECoded;
}

int ScanManager::getCurrentUsedTrackingAdvertisement()
{
  return mCurUsedTrackableAdvertisements;
}

void ScanManager::ScanNative::cleanup()
{
  mClientFilterIndexMap.clear();
  mAllPassRegularClients.clear();
  mAllPassBatchClients.clear();
  mFilterIndexStack.clear();
  sManager = NULL;

}

ScanManager::ScanNative::~ScanNative()
{
  this->cleanup();
}

ScanManager::ScanNative::ScanNative(ScanManager *scanManager)
{
  sManager = scanManager;
  if (sManager->mBatchClients.empty()) {
    return;
  }
  // Note this actually flushes all pending batch data.
  for(std::unordered_set<ScanClient*>::iterator it = scanManager->mBatchClients.begin();
            it != scanManager->mBatchClients.end(); ++it){
    sManager->flushBatchScanResults(*it);
  }

}

void ScanManager::ScanNative::configureRegularScanParams()
{
  if (DBG) {
    ALOGD(LOGTAG " configureRegularScanParams() - queue %d",
                        static_cast<int>(sManager->mRegularScanClients.size()));
  }
  int curScanSettingLE1M = INT_MIN;
  int curScanSettingLECoded = INT_MIN;;
  int scanPhy = GattDevice::PHY_LE_1M;
  ScanClient *client = getAggressiveClient(sManager->mRegularScanClients);
  PhyInfo *phyInfoResult = getPhyInfo(sManager->mRegularScanClients);
  bool scanModeChanged = false;
  int scanWindowLE1M = INT_MIN;
  int scanIntervalLE1M = INT_MIN;
  int scanWindowLECoded = INT_MIN;
  int scanIntervalLECoded = INT_MIN;
  int scanInterval[2];
  int scanWindow[2];
  int phyCnt = 0;


  if (phyInfoResult != NULL) {
    curScanSettingLE1M = phyInfoResult->scanModeLE1M;
    curScanSettingLECoded = phyInfoResult->scanModeLECoded;
    scanPhy = phyInfoResult->scanPhy;
    delete phyInfoResult;
  }

  if (DBG) {
    ALOGD(LOGTAG " configureRegularScanParams() - ScanSetting LE 1M Scan mode %d \
            ScanSetting LE Coded Scan mode %d mLastConfiguredScanSettingLE1M %d \
            mLastConfiguredScanSettingLECoded %d scanPhy %d",
            curScanSettingLE1M,
            curScanSettingLECoded, sManager->mLastConfiguredScanSettingLE1M,
            sManager->mLastConfiguredScanSettingLECoded,
            scanPhy);
  }

  /* Check whether scan mode for LE 1M PHY has changed and compute the appropriate
   * scan interval and scan window values
   */
  if ((curScanSettingLE1M != INT_MIN
          && curScanSettingLE1M != ScanSettings::SCAN_MODE_OPPORTUNISTIC)) {
    if ((curScanSettingLE1M != sManager->mLastConfiguredScanSettingLE1M) ||
             (curScanSettingLECoded != sManager->mLastConfiguredScanSettingLECoded)) {
      scanModeChanged = true;
      ScanSettings *settings = ScanSettings::Builder().setScanMode(curScanSettingLE1M).build();
      scanWindowLE1M = getScanWindowMillis(settings);
      scanIntervalLE1M = getScanIntervalMillis(settings);
      // convert scanWindow and scanInterval from ms to LE scan units(0.625ms)
      scanWindow[phyCnt] = millsToUnit(scanWindowLE1M);
      scanInterval[phyCnt] = millsToUnit(scanIntervalLE1M);
      phyCnt++;
      delete settings;
    }
  }
  else {
    sManager->mLastConfiguredScanSettingLE1M = curScanSettingLE1M;
    if (DBG) {
      ALOGD(LOGTAG " configureRegularScanParams() - queue emtpy, scan stopped");
    }
  }

  /* Check whether scan mode for LE Coded PHY has changed and compute the appropriate
   * scan interval and scan window values
   */
  if((curScanSettingLECoded != INT_MIN
          && curScanSettingLECoded != ScanSettings::SCAN_MODE_OPPORTUNISTIC)) {
    if ((curScanSettingLECoded != sManager->mLastConfiguredScanSettingLECoded) ||
         (curScanSettingLE1M != sManager->mLastConfiguredScanSettingLE1M)) {
      scanModeChanged = true;
      ScanSettings *settings = ScanSettings::Builder().setScanMode(curScanSettingLECoded).build();
      scanWindowLECoded = getScanWindowMillis(settings);
      scanIntervalLECoded = getScanIntervalMillis(settings);
      // convert scanWindow and scanInterval from ms to LE scan units(0.625ms)
      scanWindow[phyCnt] = millsToUnit(scanWindowLECoded);
      scanInterval[phyCnt] = millsToUnit(scanIntervalLECoded);
      delete settings;
    }
  }
  else {
    sManager->mLastConfiguredScanSettingLECoded = curScanSettingLECoded;
    if (DBG) {
      ALOGD(LOGTAG " configureRegularScanParams() - queue emtpy, scan stopped");
    }
  }
  if(scanModeChanged) {
    sManager->mNative->gattClientScanNative(false);
    if (DBG) {
      ALOGD(LOGTAG " configureRegularScanParams - scanInterval LE 1M = %d \
               configureRegularScanParams - scanWindow LE 1M= %d \
               configureRegularScanParams - scanInterval LE Coded=%d \
               configureRegularScanParams - scanWindow LE Coded= %d",
               scanIntervalLE1M, scanWindowLE1M,
               scanIntervalLECoded, scanWindowLECoded);
    }
    std::vector<uint32_t> v1(std::begin(scanInterval), std::end(scanInterval));
    std::vector<uint32_t> v2(std::begin(scanWindow), std::end(scanWindow));
    if(client != NULL) {
        sManager->mNative->gattSetScanParametersNative(client->scannerId, scanPhy, v1, v2);
    }
    sManager->mNative->gattClientScanNative(true);
    sManager->mLastConfiguredScanSettingLE1M = curScanSettingLE1M;
    sManager->mLastConfiguredScanSettingLECoded = curScanSettingLECoded;
  }
}

ScanClient* ScanManager::ScanNative::getAggressiveClient(std::unordered_set<ScanClient*> cList)
{
  ScanClient *result = NULL;
  int curScanSetting = INT_MIN;
  for (ScanClient *client : cList) {
    // ScanClient scan settings are assumed to be monotonically increasing in value for
    // more power hungry(higher duty cycle) operation.
    if (client->settings->getScanMode() > curScanSetting) {
      result = client;
      curScanSetting = client->settings->getScanMode();
    }
  }
  return result;
}

ScanManager::PhyInfo* ScanManager::ScanNative::getPhyInfo(std::unordered_set<ScanClient*> cList)
{
  PhyInfo *result = NULL;
  int curScanSettingLE1M = INT_MIN;
  int curScanSettingLECoded = INT_MIN;
  int curScanPhy = GattDevice::PHY_LE_1M;
  int aggregateScanPhy = GattDevice::PHY_LE_1M;
  for (ScanClient *client : cList) {
    // Get the most aggresive scan mode for each PHY
    curScanPhy = client->settings->getPhy();
    if (((curScanPhy & GattDevice::PHY_LE_1M)== GattDevice::PHY_LE_1M) &&
            (client->settings->getScanMode() > curScanSettingLE1M)) {
      curScanSettingLE1M = client->settings->getScanMode();
    }
    if (((curScanPhy & GattDevice::PHY_LE_CODED)== GattDevice::PHY_LE_CODED) &&
            (client->settings->getScanMode() > curScanSettingLECoded)) {
      curScanSettingLECoded = client->settings->getScanMode();
    }
    aggregateScanPhy |= client->settings->getPhy();
  }
  result = new PhyInfo(aggregateScanPhy, curScanSettingLE1M, curScanSettingLECoded);
  return result;
}

bool ScanManager::ScanNative::startRegularScan(ScanClient *client)
{
  if (sManager->isFilteringSupported() && mFilterIndexStack.empty()
          && mClientFilterIndexMap.empty()) {
   initFilterIndexStack();
  }

  if (isRoutingScanClient(client) && (client->filters.size() > mFilterIndexStack.size())) {
    GattLibService *mService = GattLibService::getGatt();
    if (mService == NULL) {
      ALOGE(LOGTAG "Gatt not instantiated");
      return false;
    }
    try {
      mService->onScanManagerErrorCallback(client->scannerId,
                  ScanCallback::SCAN_FAILED_OUT_OF_HARDWARE_RESOURCES);
      ALOGE(LOGTAG " startRegularScan, routing scan out of HARDWARE_RESOURCES");
    } catch (std::exception& e) {
      ALOGE(LOGTAG " startRegularScan, routing scan failed on onScanManagerCallback %s", e.what());
    }
    return false;
  }

  if (sManager->isFilteringSupported()) {
    configureScanFilters(client);
  }
  // Start scan native only for the first client.
  if (numRegularScanClients() == 1) {
    sManager->mNative->gattClientScanNative(true);
  }
  return true;
}

int ScanManager::ScanNative::numRegularScanClients()
{
  int num = 0;
   for (ScanClient *client : sManager->mRegularScanClients) {
     if (client->settings->getScanMode() != ScanSettings::SCAN_MODE_OPPORTUNISTIC) {
       num++;
     }
   }
   return num;
}

void ScanManager::ScanNative::startBatchScan(ScanClient *client)
{
  if (mFilterIndexStack.empty() && sManager->isFilteringSupported()) {
    initFilterIndexStack();
  }
  configureScanFilters(client);
  if (!isOpportunisticScanClient(client)) {
    // Reset batch scan. May need to stop the existing batch scan and update scan
    // params.
    resetBatchScan(client);
  }
}

bool ScanManager::ScanNative::isExemptFromScanDowngrade(ScanClient *client)
{
  return isOpportunisticScanClient(client) || isFirstMatchScanClient(client)
                    || !shouldUseAllPassFilter(client) || isRoutingScanClient(client);
}

bool ScanManager::ScanNative::isOpportunisticScanClient(ScanClient *client)
{
  return client->settings->getScanMode() == ScanSettings::SCAN_MODE_OPPORTUNISTIC;
}

bool ScanManager::ScanNative::isFirstMatchScanClient(ScanClient *client)
{
  return (client->settings->getCallbackType() & ScanSettings::CALLBACK_TYPE_FIRST_MATCH)
                    != 0;
}

bool ScanManager::ScanNative::isRoutingScanClient(ScanClient *client) {
  return client->settings->getCallbackType() == ScanSettings::CALLBACK_TYPE_SENSOR_ROUTING;
}

void ScanManager::ScanNative::resetBatchScan(ScanClient *client)
{
  int scannerId = client->scannerId;
  BatchScanParams *batchScanParams = getBatchScanParams();
  // Stop batch if batch scan params changed and previous params is not null.
  if (sManager->mBatchScanParms != NULL && sManager->mBatchScanParms != batchScanParams) {
    if (DBG) {
      ALOGD(LOGTAG " stopping BLe Batch");
    }
    sManager->resetCountDownLatch();
    sManager->mNative->gattClientStopBatchScanNative(scannerId);
    sManager->waitForCallback();
    // Clear pending results as it's illegal to config storage if there are still
    // pending results.
    flushBatchResults(scannerId);
  }
  // Start batch if batchScanParams changed and current params is not null.
  if (batchScanParams != NULL && batchScanParams != sManager->mBatchScanParms) {
      int notifyThreshold = 95;
    if (DBG) {
      ALOGD(LOGTAG " Starting BLE batch scan");
    }
    int resultType = getResultType(batchScanParams);
    int fullScanPercent = getFullScanStoragePercent(resultType);
    sManager->resetCountDownLatch();
    if (DBG) {
      ALOGD(LOGTAG " configuring batch scan storage, appIf %d ", client->scannerId);
    }
    sManager->mNative->gattClientConfigBatchScanStorageNative(client->scannerId, fullScanPercent,
            100 - fullScanPercent, notifyThreshold);
    sManager->waitForCallback();
    sManager->resetCountDownLatch();
    int scanInterval =
            millsToUnit(getBatchScanIntervalMillis(batchScanParams->scanMode));
    int scanWindow =
            millsToUnit(getBatchScanWindowMillis(batchScanParams->scanMode));
    sManager->mNative->gattClientStartBatchScanNative(scannerId, resultType, scanInterval,
            scanWindow, 0, DISCARD_OLDEST_WHEN_BUFFER_FULL);
    sManager->waitForCallback();
  }
  sManager->mBatchScanParms = batchScanParams;
  setBatchAlarm();
}

int ScanManager::ScanNative::getFullScanStoragePercent(int resultType)
{
  switch (resultType) {
    case SCAN_RESULT_TYPE_FULL:
      return 100;
    case SCAN_RESULT_TYPE_TRUNCATED:
      return 0;
    case SCAN_RESULT_TYPE_BOTH:
      return 50;
    default:
      return 50;
  }
}

ScanManager::BatchScanParams* ScanManager::ScanNative::getBatchScanParams()
{
  if (sManager->mBatchClients.empty()) {
    return NULL;
  }
  BatchScanParams *params = new BatchScanParams();
  // TODO: split full batch scan results and truncated batch scan results to different
  // collections.
  for (ScanClient *client : sManager->mBatchClients) {
    params->scanMode = std::max(params->scanMode, client->settings->getScanMode());
    if (client->settings->getScanResultType() == ScanSettings::SCAN_RESULT_TYPE_FULL) {
      params->fullScanscannerId = client->scannerId;
    } else {
      params->truncatedScanscannerId = client->scannerId;
    }
  }
  return params;
}

int ScanManager::ScanNative::getBatchScanWindowMillis(int scanMode)
{
  switch (scanMode) {
    case ScanSettings::SCAN_MODE_LOW_LATENCY:
      return SCAN_MODE_BATCH_LOW_LATENCY_WINDOW_MS;
    case ScanSettings::SCAN_MODE_BALANCED:
      return SCAN_MODE_BATCH_BALANCED_WINDOW_MS;
    case ScanSettings::SCAN_MODE_LOW_POWER:
      return SCAN_MODE_BATCH_LOW_POWER_WINDOW_MS;
    default:
      return SCAN_MODE_BATCH_LOW_POWER_WINDOW_MS;
  }
}

int ScanManager::ScanNative::getBatchScanIntervalMillis(int scanMode)
{
  switch (scanMode) {
     case ScanSettings::SCAN_MODE_LOW_LATENCY:
       return SCAN_MODE_BATCH_LOW_LATENCY_INTERVAL_MS;
     case ScanSettings::SCAN_MODE_BALANCED:
       return SCAN_MODE_BATCH_BALANCED_INTERVAL_MS;
     case ScanSettings::SCAN_MODE_LOW_POWER:
       return SCAN_MODE_BATCH_LOW_POWER_INTERVAL_MS;
     default:
       return SCAN_MODE_BATCH_LOW_POWER_INTERVAL_MS;
   }
}

void ScanManager::ScanNative::batchScanTimeoutCb(void *context)
{
  BtEvent *event = new BtEvent;
  CHECK_PARAM_VOID(event);

  event->event_id = BLESCANNER_BATCHSCAN_TIMEOUT_EVENT;
  event->BleScanner_batchscan_timeout_Event.scanmanager = context;

  PostMessage(THREAD_ID_GATT, event);
}

void ScanManager::ScanNative::setBatchAlarm()
{
  alarm_cancel(sManager->batch_scan_timer);
  if(sManager->mBatchClients.empty())
    return;
  long batchTriggerIntervalMillis = getBatchTriggerIntervalMillis();
  long TimeOutMillis = batchTriggerIntervalMillis;
  alarm_set(sManager->batch_scan_timer, TimeOutMillis,
                        (alarm_callback_t)batchScanTimeoutCb, sManager);
}

void ScanManager::ScanNative::stopRegularScan(ScanClient *client)
{
  // Remove scan filters and recycle filter indices.
  if (client == NULL) {
    return;
  }
  int deliveryMode = getDeliveryMode(client);
  if (deliveryMode == DELIVERY_MODE_ON_FOUND_LOST) {
    for (ScanFilter *filter : client->filters) {
      int entriesToFree = getNumOfTrackingAdvertisements(client->settings);
      if (!manageAllocationOfTrackingAdvertisement(entriesToFree, false)) {
        ALOGE(LOGTAG " Error freeing for onfound/onlost filter resources %d",
                entriesToFree);
        GattLibService *mService = GattLibService::getGatt();
        if (mService == NULL) {
          ALOGE(LOGTAG "Gatt not instantiated");
          return;
        }
        try {
          mService->onScanManagerErrorCallback(client->scannerId,
                    ScanCallback::SCAN_FAILED_INTERNAL_ERROR);
        } catch (std::exception& e) {
          ALOGE(LOGTAG " failed on onScanManagerCallback at freeing %s", e.what());
        }
      }
    }
  }
  sManager->mRegularScanClients.erase(client);
  if (numRegularScanClients() == 0) {
    if (DBG) {
      ALOGD(LOGTAG " stop scan");
    }
    sManager->mNative->gattClientScanNative(false);
  }
  removeScanFilters(client->scannerId);
  // Remove if ALL_PASS filters are used.
  if (mAllPassRegularClients.count(client->scannerId)) {
    mAllPassRegularClients.erase(client->scannerId);
  }
  removeFilterIfExisits(mAllPassRegularClients, client->scannerId,
          ALL_PASS_FILTER_INDEX_REGULAR_SCAN);

}

void ScanManager::ScanNative::regularScanTimeout(ScanClient *client)
{
  // The scan should continue for background scans
  configureRegularScanParams();
  if (numRegularScanClients() == 0) {
      if (DBG) {
        ALOGD(LOGTAG " stop scan");
      }
      sManager->mNative->gattClientScanNative(false);
  }
}

void ScanManager::ScanNative::setOpportunisticScanClient(ScanClient *client)
{
  // TODO: Add constructor to ScanSettings.Builder
  // that can copy values from an existing ScanSettings object
  ScanSettings::Builder builder = ScanSettings::Builder();
  ScanSettings *settings = client->settings;
  builder.setScanMode(ScanSettings::SCAN_MODE_OPPORTUNISTIC);
  builder.setCallbackType(settings->getCallbackType());
  builder.setScanResultType(settings->getScanResultType());
  builder.setReportDelay(settings->getReportDelayMillis());
  builder.setNumOfMatches(settings->getNumOfMatches());
  client->settings = builder.build();
}

// Find the regular scan client information.
ScanClient* ScanManager::ScanNative::getRegularScanClient(int scannerId)
{
  for (ScanClient *client : sManager->mRegularScanClients) {
    if (client->scannerId == scannerId) {
      return client;
    }
  }
  return NULL;
}

void ScanManager::ScanNative::stopBatchScan(ScanClient *client)
{
  sManager->mBatchClients.erase(client);
  removeScanFilters(client->scannerId);
  // Remove if ALL_PASS filters are used.
  if (mAllPassBatchClients.count(client->scannerId)) {
    mAllPassBatchClients.erase(client->scannerId);
  }
  removeFilterIfExisits(mAllPassBatchClients, client->scannerId,
          ALL_PASS_FILTER_INDEX_BATCH_SCAN);
  if (!isOpportunisticScanClient(client)) {
    resetBatchScan(client);
  }
}

void ScanManager::ScanNative::flushBatchResults(int scannerId)
{
  if (DBG) {
    ALOGD(LOGTAG " flushPendingBatchResults() - scannerId = %d", scannerId);
  }
  if (sManager->mBatchScanParms->fullScanscannerId != -1) {
    sManager->resetCountDownLatch();
    sManager->mNative->gattClientReadScanReportsNative(
            sManager->mBatchScanParms->fullScanscannerId,
            SCAN_RESULT_TYPE_FULL);
    sManager->waitForCallback();
  }
  if (sManager->mBatchScanParms->truncatedScanscannerId != -1) {
    sManager->resetCountDownLatch();
    sManager->mNative->gattClientReadScanReportsNative(
            sManager->mBatchScanParms->truncatedScanscannerId,
            SCAN_RESULT_TYPE_TRUNCATED);
    sManager->waitForCallback();
  }
  setBatchAlarm();
}

long ScanManager::ScanNative::getBatchTriggerIntervalMillis()
{
  long intervalMillis = LONG_MAX;
  for (ScanClient *client : sManager->mBatchClients) {
    if (client->settings != NULL && client->settings->getReportDelayMillis() > 0) {
      intervalMillis =
                std::min(intervalMillis, client->settings->getReportDelayMillis());
    }
  }
  return intervalMillis;
}

std::vector<apcf_command_t> ScanManager::ScanNative::parseScanFilterToApcfCommand
                                      (std::vector<ScanFilterQueue::Entry*> entries)
{

  std::vector<apcf_command_t> native_filters;
  int numFilters = entries.size();

  for (int i = 0; i < numFilters; ++i) {
    apcf_command_t curr;

    ScanFilterQueue::Entry* current = entries.at(i);

    curr.type = current->type;
    if (current->address.length() != 0)
      curr.address = current->address;

    curr.addr_type = current->addr_type;

    if (current->uuid != Uuid::kEmpty) {
      curr.uuid = current->uuid;
    }

    if (current->uuid_mask != Uuid::kEmpty) {
      curr.uuid_mask = current->uuid_mask;
    }

    if (!current->name.empty()) {
      curr.name = std::vector<uint8_t>(current->name.begin(), current->name.end());
    }

    curr.company = current->company;
    curr.company_mask = current->company_mask;

    if (!current->data.empty()) {
      curr.data.assign(current->data.begin(), current->data.end());
    }

    if (!current->data_mask.empty()) {
      curr.data_mask.assign(current->data_mask.begin(), current->data_mask.end());
    }
    delete current;
    native_filters.push_back(curr);
  }
  return native_filters;
}

// Add scan filters. The logic is:
// If no offload filter can/needs to be set, set ALL_PASS filter.
// Otherwise offload all filters to hardware and enable all filters.
void ScanManager::ScanNative::configureScanFilters(ScanClient *client)
{
  int scannerId = client->scannerId;
  int deliveryMode = getDeliveryMode(client);
  int trackEntries = 0;

  // Do not add any filters set by opportunistic scan clients
  if (isOpportunisticScanClient(client)) {
    return;
  }

  if (!shouldAddAllPassFilterToController(client, deliveryMode)) {
    return;
  }

  sManager->resetCountDownLatch();
  sManager->mNative->gattClientScanFilterEnableNative(scannerId, true);
  sManager->waitForCallback();

  if (shouldUseAllPassFilter(client)) {
    int filterIndex =
            (deliveryMode == DELIVERY_MODE_BATCH) ? ALL_PASS_FILTER_INDEX_BATCH_SCAN
                    : ALL_PASS_FILTER_INDEX_REGULAR_SCAN;
    sManager->resetCountDownLatch();
    // Don't allow Onfound/onlost with all pass
    configureFilterParamter(scannerId, client, ALL_PASS_FILTER_SELECTION, filterIndex,
            0);
    sManager->waitForCallback();
  } else {
    std::deque<int> clientFilterIndices;
    for (ScanFilter *filter : client->filters) {
      ScanFilterQueue *queue = new ScanFilterQueue();
      queue->addScanFilter(filter);
      std::vector<ScanFilterQueue::Entry*> entries = queue->toArray();
      int featureSelection = queue->getFeatureSelection();
      int filterIndex = mFilterIndexStack.front();
      mFilterIndexStack.pop_front();

      if (!entries.empty() && entries.size() > 0) {
        std::vector<apcf_command_t> filters = parseScanFilterToApcfCommand(entries);
        entries.clear();
        sManager->resetCountDownLatch();
        sManager->mNative->gattClientScanFilterAddNative(scannerId, filterIndex, filters);
        sManager->waitForCallback();
      }

      sManager->resetCountDownLatch();
      if (deliveryMode == DELIVERY_MODE_ON_FOUND_LOST) {
        trackEntries = getNumOfTrackingAdvertisements(client->settings);
        if (!manageAllocationOfTrackingAdvertisement(trackEntries, true)) {
          ALOGE(LOGTAG " No hardware resources for onfound/onlost filter %d",
                   trackEntries);
          GattLibService *mService = GattLibService::getGatt();
          if (mService == NULL) {
            ALOGE(LOGTAG "Gatt not instantiated");
            return;
          }
          try {
            mService->onScanManagerErrorCallback(scannerId,
                    ScanCallback::SCAN_FAILED_INTERNAL_ERROR);
          } catch (std::exception& e) {
            ALOGE(LOGTAG " failed on onScanManagerCallback %s", e.what());
          }
        }
      }

      configureFilterParamter(scannerId, client, featureSelection, filterIndex,
              trackEntries);
      sManager->waitForCallback();
      clientFilterIndices.push_back(filterIndex);
      delete queue;
    }

    mClientFilterIndexMap.insert({{scannerId, clientFilterIndices}});
  }
}

// Check whether the filter should be added to controller.
// Note only on ALL_PASS filter should be added.
bool ScanManager::ScanNative::shouldAddAllPassFilterToController(ScanClient *client,
                                                                              int deliveryMode)
{
  // Not an ALL_PASS client, need to add filter.
  if (!shouldUseAllPassFilter(client)) {
    return true;
  }

  if (deliveryMode == DELIVERY_MODE_BATCH) {
    mAllPassBatchClients.insert(client->scannerId);
    return mAllPassBatchClients.size() == 1;
  } else {
    mAllPassRegularClients.insert(client->scannerId);
    return mAllPassRegularClients.size() == 1;
  }
}

void ScanManager::ScanNative::removeScanFilters(int scannerId)
{
  auto itr = mClientFilterIndexMap.find(scannerId);
  if (itr != mClientFilterIndexMap.end()) {
    for (int filterIndex : itr->second)
      mFilterIndexStack.push_back(filterIndex);
    for (int filterIndex : itr->second) {
      sManager->resetCountDownLatch();
      sManager->mNative->gattClientScanFilterParamDeleteNative(scannerId, filterIndex);
      sManager->waitForCallback();
    }
  }

  unordered_map<int, std::deque<int>>::iterator it = mClientFilterIndexMap.begin();
  for(; it != mClientFilterIndexMap.end(); ++it) {
    if( it->first == scannerId) {
      mClientFilterIndexMap.erase(it);
      break;
   }
  }
}

void ScanManager::ScanNative::removeFilterIfExisits(std::unordered_set<int> clients,
                                                          int scannerId, int filterIndex)
{
  // Remove ALL_PASS filter iff no app is using it.
  if (clients.empty()) {
    sManager->resetCountDownLatch();
    sManager->mNative->gattClientScanFilterParamDeleteNative(scannerId, filterIndex);
    sManager->waitForCallback();
  }
}

ScanClient* ScanManager::ScanNative::getBatchScanClient(int scannerId)
{
  for (ScanClient *client : sManager->mBatchClients) {
    if (client->scannerId == scannerId) {
      return client;
    }
  }
  return NULL;
}

/**
* Return batch scan result type value defined in bt stack.
*/
int ScanManager::ScanNative::getResultType(BatchScanParams* params)
{
  if (params->fullScanscannerId != -1 && params->truncatedScanscannerId != -1) {
    return SCAN_RESULT_TYPE_BOTH;
  }
  if (params->truncatedScanscannerId != -1) {
    return SCAN_RESULT_TYPE_TRUNCATED;
  }
  if (params->fullScanscannerId != -1) {
    return SCAN_RESULT_TYPE_FULL;
  }
  return -1;
}

// Check if ALL_PASS filter should be used for the client.
bool ScanManager::ScanNative::shouldUseAllPassFilter(ScanClient *client)
{
  if (client == NULL) {
    return true;
  }

  if (isRoutingScanClient(client)) {
    return false;
  }

  if (client->filters.empty()) {
    return true;
  }

  return client->filters.size() > mFilterIndexStack.size();

}

void ScanManager::ScanNative::initFilterIndexStack()
{
  GattLibService *mGatt = GattLibService::getGatt();
  if(mGatt == NULL) return;
  int maxFiltersSupported =
          mGatt->getNumOfOffloadedScanFilterSupported();
  // Start from index 3 as:
  // index 0 is reserved for ALL_PASS filter in Settings app.
  // index 1 is reserved for ALL_PASS filter for regular scan apps.
  // index 2 is reserved for ALL_PASS filter for batch scan apps.
  for (int i = 3; i < maxFiltersSupported; ++i) {
    mFilterIndexStack.push_back(i);
  }
}

template<typename T, typename... Args>
static std::unique_ptr<T> make_unique(Args&&... args)
{
  return std::unique_ptr<T>(new T(std::forward<Args>(args)...));
}

// Configure filter parameters.
void ScanManager::ScanNative::configureFilterParamter(int scannerId, ScanClient *client,
                        int featureSelection, int filterIndex, int numOfTrackingEntries)
{
  int deliveryMode = getDeliveryMode(client);
  int rssiThreshold = SCHAR_MIN;
  ScanSettings *settings = client->settings;
  int onFoundTimeout = getOnFoundOnLostTimeoutMillis(settings, true);
  int onLostTimeout = getOnFoundOnLostTimeoutMillis(settings, false);
  int onFoundCount = getOnFoundOnLostSightings(settings);
  onLostTimeout = 10000;
  if (DBG) {
    ALOGD(LOGTAG " configureFilterParamter onFoundTimeout %d onLostTimeout %d \
                      onFoundCount %d numOfTrackingEntries %d deliveryMode %d ",
                      onFoundTimeout, onLostTimeout, onFoundCount,
                      numOfTrackingEntries, deliveryMode);
  }
  FilterParams *filtValue =
          new FilterParams(scannerId, filterIndex, featureSelection, LIST_LOGIC_TYPE,
                  FILTER_LOGIC_TYPE, rssiThreshold, rssiThreshold, deliveryMode,
                  onFoundTimeout, onLostTimeout, onFoundCount, numOfTrackingEntries);

  auto filt_params = make_unique<btgatt_filt_param_setup_t>();

  uint8_t client_if = filtValue->getClientIf();
  uint8_t filt_index = filtValue->getFiltIndex();
  filt_params->feat_seln = filtValue->getFeatSeln();
  filt_params->list_logic_type = filtValue->getListLogicType();
  filt_params->filt_logic_type = filtValue->getFiltLogicType();
  filt_params->dely_mode = filtValue->getDelyMode();
  filt_params->found_timeout = filtValue->getFoundTimeout();
  filt_params->lost_timeout = filtValue->getLostTimeout();
  filt_params->found_timeout_cnt = filtValue->getFoundTimeOutCnt();
  filt_params->num_of_tracking_entries = filtValue->getNumOfTrackEntries();
  filt_params->rssi_high_thres = filtValue->getRSSIHighValue();
  filt_params->rssi_low_thres = filtValue->getRSSILowValue();
  delete filtValue;
  sManager->mNative->gattClientScanFilterParamAddNative(client_if, filt_index, std::move(filt_params));
}

// Get delivery mode based on scan settings.
int ScanManager::ScanNative::getDeliveryMode(ScanClient *client)
{
  if (client == NULL) {
    return DELIVERY_MODE_IMMEDIATE;
  }

  ScanSettings *settings = client->settings;
  if (settings == NULL) {
    return DELIVERY_MODE_IMMEDIATE;
  }

  if ((settings->getCallbackType() & ScanSettings::CALLBACK_TYPE_FIRST_MATCH) != 0
          || (settings->getCallbackType() & ScanSettings::CALLBACK_TYPE_MATCH_LOST) != 0) {
    return DELIVERY_MODE_ON_FOUND_LOST;
  }
  if (isRoutingScanClient(client)) {
    return DELIVERY_MODE_ROUTE;
  }
  return settings->getReportDelayMillis() == 0 ? DELIVERY_MODE_IMMEDIATE
          : DELIVERY_MODE_BATCH;
}

int ScanManager::ScanNative::getScanWindowMillis(ScanSettings *settings)
{
  if (settings == NULL) {
    return SCAN_MODE_LOW_POWER_WINDOW_MS;
  }

  switch (settings->getScanMode()) {
    case ScanSettings::SCAN_MODE_LOW_LATENCY:
      return SCAN_MODE_LOW_LATENCY_WINDOW_MS;
    case ScanSettings::SCAN_MODE_BALANCED:
      return SCAN_MODE_BALANCED_WINDOW_MS;
    case ScanSettings::SCAN_MODE_LOW_POWER:
      return SCAN_MODE_LOW_POWER_WINDOW_MS;
    default:
      return SCAN_MODE_LOW_POWER_WINDOW_MS;
  }
}

int ScanManager::ScanNative::getScanIntervalMillis(ScanSettings *settings)
{
  if (settings == NULL) {
    return SCAN_MODE_LOW_POWER_INTERVAL_MS;
  }
  switch (settings->getScanMode()) {
    case ScanSettings::SCAN_MODE_LOW_LATENCY:
      return SCAN_MODE_LOW_LATENCY_INTERVAL_MS;
    case ScanSettings::SCAN_MODE_BALANCED:
      return SCAN_MODE_BALANCED_INTERVAL_MS;
    case ScanSettings::SCAN_MODE_LOW_POWER:
      return SCAN_MODE_LOW_POWER_INTERVAL_MS;
    default:
      return SCAN_MODE_LOW_POWER_INTERVAL_MS;
  }
}

int ScanManager::ScanNative::getOnFoundOnLostTimeoutMillis(ScanSettings *settings, bool onFound)
{
  int factor;
  int timeout = ONLOST_ONFOUND_BASE_TIMEOUT_MS;

  if (settings->getMatchMode() == ScanSettings::MATCH_MODE_AGGRESSIVE) {
    factor = MATCH_MODE_AGGRESSIVE_TIMEOUT_FACTOR;
  } else {
    factor = MATCH_MODE_STICKY_TIMEOUT_FACTOR;
  }
  if (!onFound) {
    factor = factor * ONLOST_FACTOR;
  }
  return (timeout * factor);
}

int ScanManager::ScanNative::getOnFoundOnLostSightings(ScanSettings *settings)
{
  if (settings == NULL) {
    return ONFOUND_SIGHTINGS_AGGRESSIVE;
  }
  if (settings->getMatchMode() == ScanSettings::MATCH_MODE_AGGRESSIVE) {
    return ONFOUND_SIGHTINGS_AGGRESSIVE;
  } else {
    return ONFOUND_SIGHTINGS_STICKY;
  }
}

int ScanManager::ScanNative::getNumOfTrackingAdvertisements(ScanSettings *settings)
{
  if (settings == NULL) {
    return 0;
  }
  int val = 0;
  GattLibService *mGatt = GattLibService::getGatt();
  if(mGatt == NULL) {
    if (DBG) {
      ALOGE(LOGTAG "Default int value. Gatt not instantiated");
    }
    return 0;
  }
  int maxTotalTrackableAdvertisements =
          mGatt->getTotalNumOfTrackableAdvertisements();
  // controller based onfound onlost resources are scarce commodity; the
  // assignment of filters to num of beacons to track is configurable based
  // on hw capabilities. Apps give an intent and allocation of onfound
  // resources or failure there of is done based on availibility - FCFS model
  switch (settings->getNumOfMatches()) {
    case ScanSettings::MATCH_NUM_ONE_ADVERTISEMENT:
      val = 1;
      break;
    case ScanSettings::MATCH_NUM_FEW_ADVERTISEMENT:
      val = 2;
      break;
    case ScanSettings::MATCH_NUM_MAX_ADVERTISEMENT:
      val = maxTotalTrackableAdvertisements / 2;
      break;
    default:
      val = 1;
      if (DBG) {
        ALOGD(LOGTAG " Invalid setting for getNumOfMatches() %d",
                settings->getNumOfMatches());
      }
  }
  return val;
}

bool ScanManager::ScanNative::manageAllocationOfTrackingAdvertisement(
                            int numOfTrackableAdvertisement,
                            bool allocate)
{
  GattLibService *mGatt = GattLibService::getGatt();
  if(mGatt == NULL) {
    if (DBG) {
      ALOGE(LOGTAG "Gatt not instantiated");
    }
    return false;
  }

  int maxTotalTrackableAdvertisements = mGatt->getTotalNumOfTrackableAdvertisements();
  int availableEntries =
        maxTotalTrackableAdvertisements - sManager->mCurUsedTrackableAdvertisements;
  if (allocate) {
    if (availableEntries >= numOfTrackableAdvertisement) {
      sManager->mCurUsedTrackableAdvertisements += numOfTrackableAdvertisement;
      return true;
    } else {
      return false;
    }
    } else {
    if (numOfTrackableAdvertisement > sManager->mCurUsedTrackableAdvertisements) {
      return false;
    } else {
      sManager->mCurUsedTrackableAdvertisements -= numOfTrackableAdvertisement;
      return true;
    }
  }

}

}
