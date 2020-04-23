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

#ifndef GATTLEADVERTISER_HPP_
#define GATTLEADVERTISER_HPP_

#pragma once

#include <iostream>
#include <cstring>
#include <mutex>
#include <unordered_map>
#include "AdvertisingSetCallback.hpp"
#include "AdvertiseSettings.hpp"
#include "AdvertiseData.hpp"
#include "PeriodicAdvertiseParameters.hpp"
#include "IAdvertisingSetCallback.hpp"
#include "AdvertisingSetParameters.hpp"
#include "utils/Log.h"
#include "GattLibService.hpp"
#include "AdvertisingSet.hpp"

using namespace std;
using std::string;
namespace gatt{

/**
* This class provides a way to perform Bluetooth LE advertise operations, such as starting and
* stopping advertising. An advertiser can broadcast up to 31 bytes of advertisement data
* represented by AdvertiseData.
* Use getGattLeAdvertiser() to get an instance of GattLeAdvertiser.
*/
class GattLeAdvertiser : public IAdvertisingSetCallback {
  private:
    static const int MAX_ADVERTISING_DATA_BYTES = 1650;
    static const int MAX_LEGACY_ADVERTISING_DATA_BYTES = 31;
    // Each fields need one byte for field length and another byte for field type.
    static const int OVERHEAD_BYTES_PER_FIELD = 2;
    // Flags field will be set by system.
    static const int FLAGS_FIELD_BYTES = 3;
    static const int MANUFACTURER_SPECIFIC_DATA_LENGTH = 2;

    static GattLeAdvertiser *sGattLeAdvertiser;

    // Compute the size of advertisement data or scan resp
    int totalBytes(AdvertiseData *data, bool isFlagsIncluded);
    int byteLength(std::vector<uint8_t> array);

    AdvertisingSetCallback *mCb = NULL;
    GattLibService *mGattLibService = NULL;

    std::unordered_map<const AdvertisingSetCallback*, IAdvertisingSetCallback*> mCallback;
    std::unordered_map<int, AdvertisingSet*> mAdvertisingSets;

    static std::mutex singletonLock;

    /**
    * Create GattLeAdvertiser object
    * @hide
    */

    GattLeAdvertiser();
    /**
     * Creates a new advertising set. If operation succeed, device will start advertising. This
     * method returns immediately, the operation status is delivered through
     * callback.onAdvertisingSetStarted().
     * <p>
     *
     * @param parameters advertising set parameters.
     * @param advertiseData Advertisement data to be broadcasted.
     * @param scanResponse Scan response associated with the advertisement data.
     * @param periodicParameters periodic advertisng parameters. If null, periodic advertising will
     * not be started.
     * @param periodicData Periodic advertising data.
     * @param duration advertising duration, in 10ms unit. Valid range is from 1 (10ms) to 65535
     * (655,350 ms). 0 means advertising should continue until stopped.
     * @param maxExtendedAdvertisingEvents maximum number of extended advertising events the
     * controller shall attempt to send prior to terminating the extended advertising, even if the
     * duration has not expired. Valid range is from 1 to 255. 0 means no maximum.
     * @param callback Callback for advertising set..
     * @ throw std::invalid_argument exception if :
     * callback is NULL (OR)
     * AdvertisingData/ScanResponseData/PeriodicAdvertisingData is too big (OR)
     * maxExtendedAdvertisingEvents out of range (OR)
     * Unsupported primary/secondary PHY selected (OR)
     * Conroller doesnt support LE Extended Advertising (OR)
     * Duration is out of range.
     */
    void startAdvertisingSet(AdvertisingSetParameters *parameters,
            AdvertiseData *advertiseData, AdvertiseData *scanResponse,
            PeriodicAdvertiseParameters *periodicParameters,
            AdvertiseData *periodicData, int duration,
            int maxExtendedAdvertisingEvents,
            AdvertisingSetCallback *callback);
  public:
    /**
    * Use getGattLeAdvertiser() to get GattLeAdvertiser object
    */
    static GattLeAdvertiser* getGattLeAdvertiser();
    /**
    * Destructor
    */
    ~GattLeAdvertiser();

    /**
     * Start Bluetooth LE Advertising. On success, the advertiseData will be broadcasted.
     * Returns immediately, the operation status is delivered through callback.
     *
     * @param settings Settings for Bluetooth LE advertising.
     * @param advertiseData Advertisement data to be broadcasted.
     * @param callback Callback for advertising status.
     */
    void startAdvertising(AdvertiseSettings *settings,
            AdvertiseData *advertiseData, AdvertisingSetCallback *callback);

    /**
     * Start Bluetooth LE Advertising. The advertiseData will be broadcasted if the
     * operation succeeds. The scanResponse is returned when a scanning device sends an
     * active scan request. This method returns immediately, the operation status is delivered
     * through callback.
     *
     * @param settings Settings for Bluetooth LE advertising.
     * @param advertiseData Advertisement data to be advertised in advertisement packet.
     * @param scanResponse Scan response associated with the advertisement data.
     * @param callback Callback for advertising status.
     * @ throw std::invalid_argument exception if callback is NULL.
     */
    void startAdvertising(AdvertiseSettings *settings,
            AdvertiseData *advertiseData, AdvertiseData *scanResponse,
            AdvertisingSetCallback *callback);
    /**
     * Stop Bluetooth LE advertising. The callback must be the same one use in
     * GattLeAdvertiser#startAdvertising.
     *
     * @param callback AdvertisingSetCallback identifies the advertising instance to stop.
     * @ throw std::invalid_argument exception if callback is NULL.
     */
    void stopAdvertising(AdvertisingSetCallback *callback);
    /**
     * Creates a new advertising set. If operation succeed, device will start advertising. This
     * method returns immediately, the operation status is delivered through
     * callback.onAdvertisingSetStarted().
     * <p>
     *
     * @param parameters advertising set parameters.
     * @param advertiseData Advertisement data to be broadcasted.
     * @param scanResponse Scan response associated with the advertisement data.
     * @param periodicParameters periodic advertisng parameters. If null, periodic advertising will
     * not be started.
     * @param periodicData Periodic advertising data.
     * @param callback Callback for advertising set.
     */
    void startAdvertisingSet(AdvertisingSetParameters *parameters,
            AdvertiseData *advertiseData, AdvertiseData *scanResponse,
            PeriodicAdvertiseParameters *periodicParameters,
            AdvertiseData *periodicData,AdvertisingSetCallback *callback);

    /**
     * Used to dispose of a AdvertisingSet object, obtained with
     * GattLeAdvertiser#startAdvertisingSet.
     */
    void stopAdvertisingSet(AdvertisingSetCallback *callback);
    /**
     * Cleans up advertisers. Should be called when bluetooth is down.
     * @hide
     */
    void cleanup();
    /**
    * Callbacks
    */
    void postStartSetFailure(AdvertisingSetCallback *callback,const int error);
    void postStartFailure(AdvertisingSetCallback *callback, const int error);
    void postStartSuccess(AdvertisingSetCallback *callback, AdvertiseSettings *settings);
    void onAdvertisingSetStarted(int advertiserId, int txPower, int status);
    void onOwnAddressRead(int advertiserId, int addressType, string address);
    void onAdvertisingSetStopped(int advertiserId);
    void onAdvertisingEnabled(int advertiserId, bool enabled, int status);
    void onAdvertisingDataSet(int advertiserId, int status);
    void onScanResponseDataSet(int advertiserId, int status);
    void onAdvertisingParametersUpdated(int advertiserId, int txPower, int status);
    void onPeriodicAdvertisingParametersUpdated(int advertiserId, int status);
    void onPeriodicAdvertisingDataSet(int advertiserId, int status);
    void onPeriodicAdvertisingEnabled(int advertiserId, bool enable, int status);

};
}
#endif