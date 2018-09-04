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


#ifndef PERIODIC_SCAN_MANAGER_HPP_
#define PERIODIC_SCAN_MANAGER_HPP_

#pragma once

#include <unordered_map>
#include <vector>
#include "IPeriodicAdvertisingCallback.hpp"
using namespace std;
namespace gatt {
class GattNativeInterfaceV2;
class ScanResult;
/**
 * Manages Bluetooth LE Periodic scans
 *
 * @hide
 */
class PeriodicScanManager {
  private:
    static const bool DBG = true;
    GattNativeInterfaceV2 *mNative;
  public:
    /** When id_sync_handle is negative, the registration is ongoing.
    * When the registration finishes, id
    * becomes equal to sync_handle
    */
    int id_sync_handle;
    IPeriodicAdvertisingCallback *callback;
    std::unordered_map<int, IPeriodicAdvertisingCallback*> mSyncs;

    static int sTempRegistrationId;

    /**
     * Constructor of {@link SyncManager}.
     */
    PeriodicScanManager(GattNativeInterfaceV2 *native);
    ~PeriodicScanManager();
    void start();
    void cleanup();
    IPeriodicAdvertisingCallback* findSync(int syncHandle);
    void onSyncStarted(int regId, int syncHandle, int sid, int addressType,
                string address, int phy, int interval, int status);
    void onSyncReport(int syncHandle, int txPower, int rssi, int dataStatus,
                          std::vector<uint8_t> data);
    void onSyncLost(int syncHandle);
    void startSync(ScanResult *scanResult, int skip, int timeout,
            IPeriodicAdvertisingCallback *callback);
    void stopSync(IPeriodicAdvertisingCallback *callback);
};
}
#endif
