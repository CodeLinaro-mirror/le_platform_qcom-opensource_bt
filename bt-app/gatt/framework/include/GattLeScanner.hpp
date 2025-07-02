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

#ifndef GATTLESCANNER_HPP_
#define GATTLESCANNER_HPP_

#pragma once
#include "GattDevice.hpp"
#include "ScanSettings.hpp"
#include "ScanCallback.hpp"
#include "ScanResult.hpp"
#include "TruncatedFilter.hpp"
#include "ScanFilter.hpp"
#include "ResultStorageDescriptor.hpp"
#include "IScannerCallback.hpp"
#include "GattLibService.hpp"

#include "osi/include/log.h"
#include <vector>
#include <mutex>
#include <unordered_map>
#include <string>

using namespace std;
using std::string;

namespace gatt {
/**
 * This class provides methods to perform scan related operations for Bluetooth LE devices. An
 * application can scan for a particular type of Bluetooth LE devices using ScanFilter. It
 * can also request different types of callbacks for delivering the result.
 * <p>
 * Use getGattLeScanner() to get an instance of GattLeScanner.
 * <p>
 */
class GattLeScanner final {
  private :
    /**
     * Bluetooth GATT interface callbacks
     */
    class BleScanCallbackWrapper : public IScannerCallback {
      private:
        static const int REGISTRATION_CALLBACK_TIMEOUT_MILLIS = 2000;
        ScanCallback *mScanCallback = NULL;
        std::vector<ScanFilter*> mFilters;
        ScanSettings *mSettings = NULL;
        GattLibService *mGatt = NULL;
        GattLeScanner *mOuterScanner = NULL;

        std::vector<std::vector<ResultStorageDescriptor*>> mResultStorages;

        int mScannerId;

      public:
        BleScanCallbackWrapper(GattLibService *gatt,GattLeScanner *sScanner,
                std::vector<ScanFilter*> filters, ScanSettings *settings,
                ScanCallback *scanCallback,
                std::vector<std::vector<ResultStorageDescriptor*>> resultStorages);
        void startRegistration();
        void stopLeScan();
        /**
         *Application interface registered - app is ready to go
         *@Override
         */
        void onScannerRegistered(int status, int scannerId);
        /**
         * Callback reporting an LE scan result.
         *
         */
        void onScanResult(ScanResult *scanResult);
        void onBatchScanResults(std::vector<ScanResult*> results);
        void onFoundOrLost(const bool onFound, ScanResult *scanResult);
        void onScanManagerErrorCallback(const int errorCode);
        void flushPendingBatchResults();
        void cleanup();
        ~BleScanCallbackWrapper();
      };

    static const bool DBG = true;
    static const bool VDBG = false;
    static GattLeScanner *sGattLeScanner;
    static std::mutex singletonLock;
    GattLibService *mGattLibService = NULL;
    BleScanCallbackWrapper *wrapper = NULL;

    std::unordered_map<ScanCallback*, BleScanCallbackWrapper*> mLeScanClients;
    /**
     * Start truncated scan.
     *
     * @hide
     */
    void startTruncatedScan(std::vector<TruncatedFilter*> truncatedFilters, ScanSettings *settings,
            ScanCallback *callback);
    /**
     * Cleans up scan clients. Should be called when bluetooth is down.
     *
     * @hide
     */
    void cleanup();
    GattLeScanner();
    int startScan(std::vector<ScanFilter*> filters, ScanSettings *settings,
        ScanCallback *callback,
        std::vector<std::vector<ResultStorageDescriptor*>> resultStorages);

    bool isSettingsConfigAllowedForScan(ScanSettings *settings);
    bool isSettingsAndFilterComboAllowed(ScanSettings *settings,
                                  std::vector<ScanFilter*> filterList);
    bool isHardwareResourcesAvailableForScan(ScanSettings *settings);
    bool isRoutingAllowedForScan(ScanSettings *settings);
    int postCallbackErrorOrReturn(ScanCallback *callback, const int errorCode);
    void postCallbackError(ScanCallback *callback, const int errorCode);

     /**
     * Return true if hardware has entries available for matching beacons
     *
     * @return true if there are hw entries available for matching beacons
     * @hide
     */
    static bool isHardwareTrackingFiltersAvailable();

  public:
    /**
     * Use to get GattLeScanner object
     *
     */
    static GattLeScanner* getGattLeScanner();
    /**
     * Start Bluetooth LE scan with default parameters and no filters. The scan results will be
     * delivered through callback. Use startScan(List, ScanSettings, ScanCallback)
     * with desired ScanFilter.
     *
     * @param callback Callback used to deliver scan results.
     * @throws std::invalid_argument If callback is null.
     */
    void startScan(ScanCallback *callback);
    /**
     * Start Bluetooth LE scan. The scan results will be delivered through callback.
     * Use filetered scanning by using proper ScanFilter.
     *
     * @param filters ScanFilters for finding exact BLE devices.
     * @param settings Settings for the scan.
     * @param callback Callback used to deliver scan results.
     * @throws std::invalid_argument If settings or callback is null.
     */
    void startScan(std::vector<ScanFilter*> filters, ScanSettings *settings,
            ScanCallback *callback);
    /**
     * Stops an ongoing Bluetooth LE scan.
     *
     * @param callback
     */
    void stopScan(ScanCallback *callback);
    /**
     * Flush pending batch scan results stored in Bluetooth controller. This will return Bluetooth
     * LE scan results batched on bluetooth controller. Returns immediately, batch scan results data
     * will be delivered through the callback.
     *
     * @param callback Callback of the Bluetooth LE Scan, it has to be the same instance as the one
     * used to start scan.
     */
    void flushPendingScanResults(ScanCallback *callback);
    /**
    * Destructor : Call when GattLeScanner done
    */
    ~GattLeScanner();
};
}
#endif
