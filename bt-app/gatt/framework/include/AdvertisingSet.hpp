/*
** Copyright (c) 2018, The Linux Foundation. All rights reserved.
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

#ifndef ADVERTISINGSET_HPP_
#define ADVERTISINGSET_HPP_

#pragma once

#include "GattLibService.hpp"
#include "osi/include/log.h"
#include <string>

using namespace std;
using std::string;

namespace gatt{

class AdvertiseData;
class AdvertisingSetParameters;
class PeriodicAdvertisingParameters;
/**
 * This class provides a way to control single Bluetooth LE advertising instance.
 * To get an instance of AdvertisingSet, call the GattLeAdvertiser#startAdvertisingSet method.
 */
class AdvertisingSet {
  private:
    GattLibService *mGatt;
    int mAdvertiserId;

  public:
    AdvertisingSet(int advertiserId);
    ~AdvertisingSet();
    void setAdvertiserId(int advertiserId);
    /**
     * Enables Advertising. This method returns immediately, the operation status is
     * delivered through AdvertisingSetCallback#onAdvertisingEnabled().
     *
     * @param enable whether the advertising should be enabled (true), or disabled (false)
     * @param duration advertising duration, in 10ms unit. Valid range is from 1 (10ms) to 65535
     * (655,350 ms)
     * @param maxExtendedAdvertisingEvents maximum number of extended advertising events the
     * controller shall attempt to send prior to terminating the extended advertising, even if the
     * duration has not expired. Valid range is from 1 to 255.
     */
    void enableAdvertising(bool enable, int duration,
            int maxExtendedAdvertisingEvents);

    /**
     * Set/update data being Advertised. Make sure that data doesn't exceed the size limit for
     * specified AdvertisingSetParameters. This method returns immediately, the operation status is
     * delivered through AdvertisingSetCallback#onAdvertisingDataSet callback method.
     * <p>
     * Advertising data must be empty if non-legacy scannable advertising is used.
     *
     * @param advertiseData Advertisement data to be broadcasted. Size must not exceed
     * MaximumAdvertisingDataLength. If the advertisement is connectable,
     * three bytes will be added for flags. If the update takes place when the advertising set is
     * enabled, the data can be maximum 251 bytes long.
     */
    void setAdvertisingData(AdvertiseData& advertiseData);

    /**
     * Set/update scan response data. Make sure that data doesn't exceed the size limit for
     * specified AdvertisingSetParameters. This method returns immediately, the operation status
     * is delivered through AdvertisingSetCallback#onScanResponseDataSet callback method.
     *
     * @param scanResponse Scan response associated with the advertisement data. Size must not
     * exceed MaximumAdvertisingDataLength. If the update takes place
     * when the advertising set is enabled, the data can be maximum 251 bytes long.
     */
    void setScanResponseData(AdvertiseData& scanResponse);

    /**
     * Update advertising parameters associated with this AdvertisingSet. Must be called when
     * advertising is not active. This method returns immediately, the operation status is delivered
     * through AdvertisingSetCallback#onAdvertisingParametersUpdated callback method.
     *
     * @param parameters advertising set parameters.
     */
    void setAdvertisingParameters(AdvertisingSetParameters& parameters);
    /**
     * Update periodic advertising parameters associated with this set. Must be called when
     * periodic advertising is not enabled. This method returns immediately, the operation
     * status is delivered through AdvertisingSetCallback#onPeriodicAdvertisingParametersUpdated
     * callback method.
     */
    void setPeriodicAdvertisingParameters(PeriodicAdvertiseParameters& parameters);

    /**
     * Used to set periodic advertising data, must be called after setPeriodicAdvertisingParameters,
     * or after advertising was started with periodic advertising data set. This method returns
     * immediately, the operation status is delivered through
     * AdvertisingSetCallback#onPeriodicAdvertisingDataSet callback method.
     *
     * @param periodicData Periodic advertising data. Size must not exceed
     * MaximumAdvertisingDataLength. If the update takes place when the
     * periodic advertising is enabled for this set, the data can be maximum 251 bytes long.
     */
    void setPeriodicAdvertisingData(AdvertiseData& periodicData);

    /**
     * Used to enable/disable periodic advertising. This method returns immediately, the operation
     * status is delivered through AdvertisingSetCallback#onPeriodicAdvertisingEnable callback
     * method.
     *
     * @param enable whether the periodic advertising should be enabled (true), or disabled
     * (false).
     */
    void setPeriodicAdvertisingEnabled(bool enable);

    /**
     * Returns address associated with this advertising set.
     * @hide
     */
    void getOwnAddress();

    /**
     * Returns advertiserId associated with this advertising set.
     * @hide
     */
    int getAdvertiserId();
};
}//namespace gatt
#endif
