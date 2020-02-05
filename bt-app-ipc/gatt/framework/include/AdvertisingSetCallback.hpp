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
#ifndef ADVERTISINGSETCALLBACK_HPP_
#define ADVERTISINGSETCALLBACK_HPP_

#pragma once

#include <string>

using namespace std;
using std::string;

namespace gatt{
class AdvertisingSet;
class AdvertiseSettings;

/**
 * Bluetooth LE advertising set callbacks, used to deliver advertising operation
 * status.
 */
class AdvertisingSetCallback {
  public:
    /**
     * The requested operation was successful.
     */
    static const int ADVERTISE_SUCCESS = 0;

    /**
     * Failed to start advertising as the advertise data to be broadcasted is too
     * large.
     */
    static const int ADVERTISE_FAILED_DATA_TOO_LARGE = 1;

    /**
     * Failed to start advertising because no advertising instance is available.
     */
    static const int ADVERTISE_FAILED_TOO_MANY_ADVERTISERS = 2;

    /**
     * Failed to start advertising as the advertising is already started.
     */
    static const int ADVERTISE_FAILED_ALREADY_STARTED = 3;

    /**
     * Operation failed due to an internal error.
     */
    static const int ADVERTISE_FAILED_INTERNAL_ERROR = 4;

    /**
     * This feature is not supported on this platform.
     */
    static const int ADVERTISE_FAILED_FEATURE_UNSUPPORTED = 5;

    virtual ~AdvertisingSetCallback() = default;
    /**
     * Callback triggered in response to GattLeAdvertiser#startAdvertisingSets
     * indicating result of the operation. If status is ADVERTISE_SUCCESS, then advertisingSet
     * contains the started set and it is advertising. If error occured, advertisingSet is
     * null, and status will be set to proper error code.
     *
     * @param advertisingSet The advertising set that was started or null if error.
     * @param txPower tx power that will be used for this set.
     * @param status Status of the operation.
     */
    virtual void onAdvertisingSetStarted(AdvertisingSet *advertisingSet, int txPower, int status){};

    /**
     * Callback triggered in response to GattLeAdvertiser#stopAdvertisingSet
     * indicating advertising set is stopped.
     *
     * @param advertisingSet The advertising set.
     */
    virtual void onAdvertisingSetStopped(AdvertisingSet *advertisingSet){};

    /**
     * Callback triggered in response to GattLeAdvertiser#startAdvertisingSet
     * indicating result of the operation. If status is ADVERTISE_SUCCESS, then advertising set is
     * advertising.
     *
     * @param advertisingSet The advertising set.
     * @param status Status of the operation.
     */
    virtual void onAdvertisingEnabled(AdvertisingSet *advertisingSet, bool enable, int status){};

    /**
     * Callback triggered in response to AdvertisingSet#setAdvertisingData indicating
     * result of the operation. If status is ADVERTISE_SUCCESS, then data was changed.
     *
     * @param advertisingSet The advertising set.
     * @param status Status of the operation.
     */
    virtual void onAdvertisingDataSet(AdvertisingSet *advertisingSet, int status){};

    /**
     * Callback triggered in response to AdvertisingSet#setAdvertisingData indicating
     * result of the operation.
     *
     * @param advertisingSet The advertising set.
     * @param status Status of the operation.
     */
    virtual void onScanResponseDataSet(AdvertisingSet *advertisingSet, int status){};

    /**
     * Callback triggered in response to AdvertisingSet#setAdvertisingParameters
     * indicating result of the operation.
     *
     * @param advertisingSet The advertising set.
     * @param txPower tx power that will be used for this set.
     * @param status Status of the operation.
     */
    virtual void onAdvertisingParametersUpdated(AdvertisingSet *advertisingSet,
            int txPower, int status){};

    /**
     * Callback triggered in response to AdvertisingSet#setPeriodicAdvertisingParameters
     * indicating result of the operation.
     *
     * @param advertisingSet The advertising set.
     * @param status Status of the operation.
     */
    virtual void onPeriodicAdvertisingParametersUpdated(AdvertisingSet *advertisingSet,
                    int status){};
    /**
     * Callback triggered in response to AdvertisingSet#setPeriodicAdvertisingData
     * indicating result of the operation.
     *
     * @param advertisingSet The advertising set.
     * @param status Status of the operation.
     */
    virtual void onPeriodicAdvertisingDataSet(AdvertisingSet *advertisingSet,
            int status){};

    /**
     * Callback triggered in response to AdvertisingSet#setPeriodicAdvertisingEnabled
     * indicating result of the operation.
     *
     * @param advertisingSet The advertising set.
     * @param status Status of the operation.
     */
    virtual void onPeriodicAdvertisingEnabled(AdvertisingSet *advertisingSet, bool enable,
            int status){};

    /**
     * Callback triggered in response to AdvertisingSet#getOwnAddress()
     * indicating result of the operation.
     *
     * @param advertisingSet The advertising set.
     * @param addressType type of address.
     * @param address advertising set bluetooth address.
     * @hide
     */
    virtual void onOwnAddressRead(AdvertisingSet *advertisingSet, int addressType,
      string address){};

     /**
     * Callback triggered in response to {@link BluetoothLeAdvertiser#startAdvertising} indicating
     * that the advertising has been started successfully.
     *
     * @param settingsInEffect The actual settings used for advertising, which may be different from
     * what has been requested.
     */
    virtual void onStartSuccess(AdvertiseSettings *settingsInEffect){};

    /**
     * Callback when advertising could not be started.
     *
     * @param errorCode Error code (see ADVERTISE_FAILED_* constants) for advertising start
     * failures.
     */
    virtual void onStartFailure(int errorCode){};
};
}//namespace gatt
#endif
