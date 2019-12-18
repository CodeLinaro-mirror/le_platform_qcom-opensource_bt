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

#ifndef PERIODIC_ADVERTISING_CALLBACK_HPP_
#define PERIODIC_ADVERTISING_CALLBACK_HPP_

#pragma once
namespace gatt {

/**
 * Bluetooth LE periodic advertising callbacks, used to deliver periodic
 * advertising operation status.
 *
 * @hide
 */
class PeriodicAdvertisingCallback {
  public:

    /**
     * The requested operation was successful.
     *
     * @hide
     */
    static const int SYNC_SUCCESS = 0;

    /**
     * Sync failed to be established because remote device did not respond.
     */
    static const int SYNC_NO_RESPONSE = 1;

    /**
     * Sync failed to be established because controller can't support more syncs.
     */
    static const int SYNC_NO_RESOURCES = 2;


    /**
     * Callback when synchronization was established.
     *
     * @param syncHandle handle used to identify this synchronization.
     * @param device remote device.
     * @param advertisingSid synchronized advertising set id.
     * @param skip The number of periodic advertising packets that can be skipped after a successful
     * receive in force.
     * @param timeout Synchronization timeout for the periodic advertising in force. One unit is
     * 10ms.
     * @param timeout
     * @param status operation status.
     */
    virtual void onSyncEstablished(int syncHandle, string device,
            int advertisingSid, int skip, int timeout,
            int status) {}

    /**
     * Callback when periodic advertising report is received.
     *
     * @param report periodic advertising report.
     */
    virtual void onPeriodicAdvertisingReport(PeriodicAdvertisingReport *report) {}

    /**
     * Callback when periodic advertising synchronization was lost.
     *
     * @param syncHandle handle used to identify this synchronization.
     */
    virtual void onSyncLost(int syncHandle) {}
};
}
#endif
