/*
 * Copyright (c) 2018, The Linux Foundation. All rights reserved.
 * Not a Contribution.
 * Copyright (C) 2014 The Android Open Source Project
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

#include "GattLeScanner.hpp"
#include <thread>

#define LOGTAG "GattLeScanner"

using namespace std;
namespace gatt {

std::mutex mtx;
std::unique_lock<std::mutex> mLock(mtx, std::defer_lock);

GattLeScanner *GattLeScanner::sGattLeScanner = new GattLeScanner();

void GattLeScanner::startTruncatedScan(std::vector<TruncatedFilter*> truncatedFilters,
                                             ScanSettings *settings,
                                             ScanCallback *callback)
{
  int filterSize = truncatedFilters.size();
  std::vector<ScanFilter*> scanFilters(filterSize);
  std::vector<std::vector<ResultStorageDescriptor*>> scanStorages(filterSize);
  for (TruncatedFilter *filter : truncatedFilters) {
    scanFilters.push_back(filter->getFilter());
    scanStorages.push_back(filter->getStorageDescriptors());
  }
  startScan(scanFilters, settings, callback, scanStorages);
}

void GattLeScanner::cleanup()
{
  if (sGattLeScanner != NULL) {
    sGattLeScanner = NULL;
  }

  if (mGattDevice!=NULL) {
    //delete(mGattDevice);
    mGattDevice = NULL;
  }

  if (wrapper!=NULL) {
    delete(wrapper);
    wrapper = NULL;
  }

  std::unordered_map<ScanCallback*, BleScanCallbackWrapper*>::iterator it = mLeScanClients.begin();
  for(it ; it != mLeScanClients.end(); ++it) {
    if(it->second != NULL)
      delete it->second;
  }
  mLeScanClients.clear();

}

GattLeScanner* GattLeScanner::getGattLeScanner()
{
  return sGattLeScanner;
}

GattLeScanner::GattLeScanner()
{
  //mGattDevice = new GattDevice();
}

GattLeScanner::~GattLeScanner()
{
  if(sGattLeScanner != NULL) {
    sGattLeScanner->cleanup();
  }
}

void GattLeScanner::startScan(ScanCallback *callback)
{
  startScan(std::vector<ScanFilter*>(), ScanSettings::Builder().build(), callback);
}

void GattLeScanner::startScan(std::vector<ScanFilter*> filters, ScanSettings *settings,
                                ScanCallback *callback)
{
  startScan(filters, settings,callback, std::vector<std::vector<ResultStorageDescriptor*>>());
}

int GattLeScanner::startScan(std::vector<ScanFilter*> filters, ScanSettings *settings,
                                ScanCallback *callback,
                                std::vector<std::vector<ResultStorageDescriptor*>> resultStorages)
{
  if (callback == NULL) {
    throw std::invalid_argument("callback is NULL");
  }

  if (settings == NULL) {
    throw std::invalid_argument("settings is NULL");
  }

  if (callback != NULL && mLeScanClients.find(callback) != mLeScanClients.end()) {
    return postCallbackErrorOrReturn(callback, ScanCallback::SCAN_FAILED_ALREADY_STARTED);
  }

  try {
    if (mGattLibService == NULL) {
      mGattLibService = GattLibService::getGatt();
    }
  } catch (std::exception &e) {
    mGattLibService = NULL;
  }

  if (mGattLibService == NULL) {
    return postCallbackErrorOrReturn(callback, ScanCallback::SCAN_FAILED_INTERNAL_ERROR);
  }

  if ((settings->getCallbackType() == ScanSettings::CALLBACK_TYPE_SENSOR_ROUTING)
        && (filters.empty())) {
    ScanFilter *filter = ScanFilter::Builder().build();
  }

  if (!isSettingsConfigAllowedForScan(settings)) {
    return postCallbackErrorOrReturn(callback,
            ScanCallback::SCAN_FAILED_FEATURE_UNSUPPORTED);
  }

  if (!isHardwareResourcesAvailableForScan(settings)) {
    return postCallbackErrorOrReturn(callback,
            ScanCallback::SCAN_FAILED_OUT_OF_HARDWARE_RESOURCES);
  }

  if (!isSettingsAndFilterComboAllowed(settings, filters)) {
    return postCallbackErrorOrReturn(callback,
            ScanCallback::SCAN_FAILED_FEATURE_UNSUPPORTED);
  }

  if (!isRoutingAllowedForScan(settings)) {
    return postCallbackErrorOrReturn(callback,
            ScanCallback::SCAN_FAILED_FEATURE_UNSUPPORTED);
  }

  if (callback != NULL) {
    wrapper = new BleScanCallbackWrapper(mGattLibService, this, filters,
            settings, callback, resultStorages);
    wrapper->startRegistration();
  } else {
    return ScanCallback::SCAN_FAILED_INTERNAL_ERROR;
  }

  return ScanCallback::NO_ERROR;
}

void GattLeScanner::stopScan(ScanCallback *callback)
{
  auto wrapper = mLeScanClients.find(callback);
  if (wrapper == mLeScanClients.end()) {
    if (DBG) ALOGD(LOGTAG " could not find callback wrapper");
    return;
  }
  mLeScanClients.erase(wrapper);
  wrapper->second->stopLeScan();
}

void GattLeScanner::flushPendingScanResults(ScanCallback *callback)
{
  if (callback == NULL) {
      throw std::invalid_argument("callback cannot be null!");
  }

  auto wrapper = mLeScanClients.find(callback);
  if (wrapper == mLeScanClients.end()) {
    return;
  }
  wrapper->second->flushPendingBatchResults();

}

void GattLeScanner::BleScanCallbackWrapper::cleanup()
{
  mGatt = NULL;
  mOuterScanner = NULL;
  mFilters.clear();
  mSettings = NULL;
  mScanCallback = NULL;
  mResultStorages.clear();
}

GattLeScanner::BleScanCallbackWrapper::~BleScanCallbackWrapper()
{
  cleanup();
}

GattLeScanner::BleScanCallbackWrapper::BleScanCallbackWrapper(GattLibService *gatt,
                GattLeScanner *sScanner,
                std::vector<ScanFilter*> filters, ScanSettings *settings,
                ScanCallback *scanCallback,
                std::vector<std::vector<ResultStorageDescriptor*>> resultStorages)
{
  mGatt = gatt;
  mOuterScanner = sScanner;
  mFilters = filters;
  mSettings = settings;
  mScanCallback = scanCallback;
  mScannerId = 0;
  mResultStorages = resultStorages;
}

void GattLeScanner::BleScanCallbackWrapper::startRegistration()
{
  // Scan stopped.
  if (mScannerId == -1 || mScannerId == -2) return;
  try {
    mGatt->registerScanner(this);
    std::this_thread::sleep_for(std::chrono::milliseconds(REGISTRATION_CALLBACK_TIMEOUT_MILLIS));
  } catch (std::exception& e) {
    ALOGE(LOGTAG " application registeration exception %s", e.what());
    mOuterScanner->postCallbackError(const_cast<ScanCallback *>(mScanCallback),
                                                        ScanCallback::SCAN_FAILED_INTERNAL_ERROR);
  }
  if (mScannerId > 0) {
    mOuterScanner->mLeScanClients.insert({{mScanCallback, this}});
  } else {
    // Registration timed out or got exception, reset RscannerId to -1 so no
    // subsequent operations can proceed.
    if (mScannerId == 0) mScannerId = -1;

    // If scanning too frequently, don't report anything to the app.
    if (mScannerId == -2) return;

    mOuterScanner->postCallbackError(const_cast<ScanCallback *>(mScanCallback),
            ScanCallback::SCAN_FAILED_APPLICATION_REGISTRATION_FAILED);
  }
}

void GattLeScanner::BleScanCallbackWrapper::stopLeScan()
{
   if (mScannerId <= 0) {
     ALOGE(LOGTAG " Error state, mLeHandle: %d ", mScannerId);
     return;
   }

   try {
     mGatt->stopScan(mScannerId);
     mGatt->unregisterScanner(mScannerId);
   } catch (std::exception& e) {
       ALOGE(LOGTAG " Failed to stop scan and unregister %s", e.what());
   }
   mScannerId = -1;
}

void GattLeScanner::BleScanCallbackWrapper::flushPendingBatchResults()
{
   if (mScannerId <= 0) {
     ALOGE(LOGTAG " Error state, mLeHandle: %d ", mScannerId);
     return;
   }

   try {
     mGatt->flushPendingBatchResults(mScannerId);
   } catch (std::exception& e) {
       ALOGE(LOGTAG " Failed to get pending scan results %s", e.what());
   }
}

 void GattLeScanner::BleScanCallbackWrapper::onScannerRegistered(int status, int scannerId)
{
  ALOGD(LOGTAG " onScannerRegistered() - status %d scannerId %d ",
                             status, scannerId);

  if (status == 0) {
    try {
     if (mScannerId == -1) {
       // Registration succeeds after timeout, unregister client.
       mGatt->unregisterClient(scannerId);
     } else {
       mScannerId = scannerId;
       mGatt->startScan(mScannerId, mSettings, mFilters,
               mResultStorages);
     }
    } catch (std::exception& e) {
       ALOGE(LOGTAG " fail to start le scan:%s ", e.what());
       mScannerId = -1;
    }
  } else if (status == ScanCallback::SCAN_FAILED_SCANNING_TOO_FREQUENTLY) {
     // applicaiton was scanning too frequently
     mScannerId = -2;
  } else {
     // registration failed
     mScannerId = -1;
  }

}

void GattLeScanner::BleScanCallbackWrapper::onScanResult(ScanResult *scanResult)
{
  if (VDBG) ALOGD(LOGTAG " onScanResult() - %s", scanResult->ToString().c_str());

  // Check null in case the scan has been stopped
  if (mScannerId <= 0) return;

  mScanCallback->onScanResult(ScanSettings::CALLBACK_TYPE_ALL_MATCHES, scanResult);
}

void GattLeScanner::BleScanCallbackWrapper::onBatchScanResults(
                                      std::vector<ScanResult*> results)
{
  mScanCallback->onBatchScanResults(results);
}

void GattLeScanner::BleScanCallbackWrapper::onFoundOrLost(const bool onFound,
                                                ScanResult *scanResult)
{
  if (VDBG) {
    ALOGD(LOGTAG " onFoundOrLost) - onFound %d scanResult %s ",
                      onFound, scanResult->ToString().c_str());
  }

  // Check null in case the scan has been stopped
  if (mScannerId <= 0) {
    ALOGE(LOGTAG " scannerId not valid");
    return;
  }

  if (onFound) {
    mScanCallback->onScanResult(ScanSettings::CALLBACK_TYPE_FIRST_MATCH,
           scanResult);
  } else {
     mScanCallback->onScanResult(ScanSettings::CALLBACK_TYPE_MATCH_LOST,
             scanResult);
    }
}

void GattLeScanner::BleScanCallbackWrapper::onScanManagerErrorCallback(const int errorCode)
{
  if (VDBG) {
     ALOGD(LOGTAG " onScanManagerErrorCallback() - errorCode %d ", errorCode);
  }

  if (mScannerId <= 0) {
    return;
  }

  mOuterScanner->postCallbackError(const_cast<ScanCallback *>(mScanCallback), errorCode);
}

int GattLeScanner::postCallbackErrorOrReturn(        ScanCallback *callback, const int errorCode)
{
  if (callback == NULL) {
    return errorCode;
  } else {
     postCallbackError(callback, errorCode);
     return ScanCallback::NO_ERROR;
  }
}

void GattLeScanner::postCallbackError(      ScanCallback *callback, const int errorCode)
{
  callback->onScanFailed(errorCode);
}

bool GattLeScanner::isSettingsConfigAllowedForScan(          ScanSettings *settings)
{
  if (mGattLibService == NULL) {
    ALOGE(LOGTAG "Gatt not instantiated");
    return false;
  }

  if (mGattLibService->isOffloadedFilteringSupported()) {
     return true;
  }

  const int callbackType = settings->getCallbackType();
  // Only support regular scan if no offloaded filter support.
  if (callbackType == ScanSettings::CALLBACK_TYPE_ALL_MATCHES
         && settings->getReportDelayMillis() == 0) {
     return true;
  }

  return false;
}

bool GattLeScanner::isSettingsAndFilterComboAllowed(          ScanSettings *settings,
                                                  std::vector<ScanFilter*> filterList)
{
  const int callbackType = settings->getCallbackType();
  // If onlost/onfound is requested, a non-empty filter is expected
  if ((callbackType & (ScanSettings::CALLBACK_TYPE_FIRST_MATCH
         | ScanSettings::CALLBACK_TYPE_MATCH_LOST)) != 0) {
    if (filterList.empty()) {
      return false;
    }

    for (ScanFilter *filter : filterList) {
      if (filter->isAllFieldsEmpty()) {
        return false;
      }
    }
  }
  return true;
}

bool GattLeScanner::isHardwareResourcesAvailableForScan(            ScanSettings *settings)
{
  const int callbackType = settings->getCallbackType();
  if (mGattLibService == NULL) {
    ALOGE(LOGTAG "Gatt not instantiated");
    return false;
  }

  if ((callbackType & ScanSettings::CALLBACK_TYPE_FIRST_MATCH) != 0
         || (callbackType & ScanSettings::CALLBACK_TYPE_MATCH_LOST) != 0) {
     // For onlost/onfound, we required hw support be available
    return (mGattLibService->isOffloadedFilteringSupported()
           && isHardwareTrackingFiltersAvailable());
  }
  return true;
}

bool GattLeScanner::isRoutingAllowedForScan(ScanSettings *settings)
{
  const int callbackType = settings->getCallbackType();

  if (callbackType == ScanSettings::CALLBACK_TYPE_SENSOR_ROUTING
         && settings->getScanMode() == ScanSettings::SCAN_MODE_OPPORTUNISTIC) {
    return false;
  }
  return true;
}

bool GattLeScanner::isHardwareTrackingFiltersAvailable()
{
  GattLibService *gatt = GattLibService::getGatt();
  if (gatt == NULL) {
    ALOGE(LOGTAG "Gatt not instantiated");
    return false;
  }
  return (gatt->numHwTrackFiltersAvailable() != 0);
}
}
