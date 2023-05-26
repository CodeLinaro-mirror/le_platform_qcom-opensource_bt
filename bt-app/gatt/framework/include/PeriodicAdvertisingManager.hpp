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

#ifndef PERIODIC_ADVERTISING_MANAGER_HPP_
#define PERIODIC_ADVERTISING_MANAGER_HPP_

#pragma once
#include "IPeriodicAdvertisingCallback.hpp"
#include "PeriodicAdvertisingCallback.hpp"
#include "ScanResult.hpp"
#include <unordered_map>

using namespace std;
using std::string;
namespace gatt {


/**
 * This class provides methods to perform periodic advertising related
 * operations. An application can register for periodic advertisements using
 * PeriodicAdvertisingManager#registerSync.
 * <p>
 * Use getPeriodicAdvertisingManager() to get an
 * instance of PeriodicAdvertisingManager.
 * <p>
 *
 * @hide
 */
class PeriodicAdvertisingManager final : public IPeriodicAdvertisingCallback {
  private:

    static const int SKIP_MIN = 0;
    static const int SKIP_MAX = 499;
    static const int TIMEOUT_MIN = 10;
    static const int TIMEOUT_MAX = 16384;

    static const int SYNC_STARTING = -1;

    static PeriodicAdvertisingManager *pAdvManager;

    PeriodicAdvertisingManager();

public:
    /* maps callback, to callback wrapper and sync handle */

    std::unordered_map<PeriodicAdvertisingCallback*,
            IPeriodicAdvertisingCallback*> mCallbackMap;

    static PeriodicAdvertisingManager* getPeriodicAdvertisingManager();

    /**
     * Synchronize with periodic advertising pointed to by the scanResult.
     * The scanResult used must contain a valid advertisingSid. First
     * call to registerSync will use the skip and timeout provided.
     * Subsequent calls from other apps, trying to sync with same set will reuse
     * existing sync, thus skip and timeout values will not take
     * effect. The values in effect will be returned in
     * PeriodicAdvertisingCallback#onSyncEstablished.
     *
     * @param scanResult Scan result containing advertisingSid.
     * @param skip The number of periodic advertising packets that can be skipped after a successful
     * receive. Must be between 0 and 499.
     * @param timeout Synchronization timeout for the periodic advertising. One unit is 10ms. Must
     * be between 10 (100ms) and 16384 (163.84s).
     * @param callback Callback used to deliver all operations status.
     * @throws std::invalid_argument if scanResult is null or skip is invalid or
     * timeout is invalid or callback is null.
     */
    void registerSync(ScanResult *scanResult, int skip, int timeout,
            PeriodicAdvertisingCallback *callback);
    /**
     * Cancel pending attempt to create sync, or terminate existing sync.
     *
     * @param callback Callback used to deliver all operations status.
     * @throws std::invalid_argument if callback is null, or not a properly registered
     * callback.
     */
    void unregisterSync(PeriodicAdvertisingCallback *callback);
    /**
     * Filter or disable periodic advertising reports.
     *
     * @param enable bit value of enable pa adv and filter duplicate pa adv.
     * @param callback Callback used to deliver all operations status.
     * @throws std::invalid_argument if callback is null, or not a properly registered
     * callback.
     */
    void filterPaAdvReport(uint8_t enable, PeriodicAdvertisingCallback *callback);


    void onSyncEstablished(int syncHandle, string device,
            int advertisingSid, int skip, int timeout, int status);
    void onPeriodicAdvertisingReport(PeriodicAdvertisingReport *report);
    void onSyncLost(int syncHandle);
    ~PeriodicAdvertisingManager();
};
}
#endif
