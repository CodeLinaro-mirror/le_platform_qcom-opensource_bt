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

#ifndef PERIODIC_ADVERTISING_REPORT_HPP_
#define PERIODIC_ADVERTISING_REPORT_HPP_

#pragma once

namespace gatt {
class ScanRecord;

/**
 * PeriodicAdvertisingReport for Bluetooth LE synchronized advertising.
 *
 * @hide
 */
class PeriodicAdvertisingReport final {
  private:
    int mSyncHandle;
    int mTxPower;
    int mRssi;
    int mDataStatus;

    // periodic advertising data.
    ScanRecord *mData;

  public:
    /**
     * The data returned is complete
     */
    static const int DATA_COMPLETE = 0;

    /**
     * The data returned is incomplete. The controller was unsuccessfull to
     * receive all chained packets, returning only partial data.
     */
    static const int DATA_INCOMPLETE_TRUNCATED = 2;

    /**
     * Constructor of periodic advertising result.
     */
    PeriodicAdvertisingReport(int syncHandle, int txPower, int rssi,
            int dataStatus, ScanRecord *data);

    /**
     * Returns the synchronization handle.
     */
    int getSyncHandle();

    /**
     * Returns the transmit power in dBm. The valid range is [-127, 126]. Value
     * of 127 means information was not available.
     */
    int getTxPower();

    /**
     * Returns the received signal strength in dBm. The valid range is [-127, 20].
     */
    int getRssi();

    /**
     * Returns the data status. Can be one of {@link PeriodicAdvertisingReport#DATA_COMPLETE}
     * or {@link PeriodicAdvertisingReport#DATA_INCOMPLETE_TRUNCATED}.
     */
    int getDataStatus();

    /**
     * Returns the data contained in this periodic advertising report.
     */
    ScanRecord* getData();
    ~PeriodicAdvertisingReport();
};
}
#endif
