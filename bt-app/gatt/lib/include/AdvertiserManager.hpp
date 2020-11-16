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

#ifndef ADVERTISER_MANAGER_HPP_
#define ADVERTISER_MANAGER_HPP_

#pragma once

#include "GattDevice.hpp"
#include "AdvertiseHelper.hpp"
#include "IAdvertisingSetCallback.hpp"
#include "AdvertisingSetParameters.hpp"
#include "PeriodicAdvertiseParameters.hpp"
#include "GattNativeInterfaceV2.hpp"
#include "utils/Log.h"
#include "hardware/bt_gatt.h"
#include "ipc.hpp"

#include <string>
#include <unordered_map>

using namespace std;
using std::string;

namespace gatt{

/**
* Manages Bluetooth LE advertising operations and interacts with bluedroid stack.
*
*/

class AdvertiseData;
class AdvertiserManager {
  private:
    static const bool DBG = true;
    static int sTempRegistrationId;
    std::unordered_map<int,IAdvertisingSetCallback&> mAdvertisers;

    GattNativeInterfaceV2 *mNative = NULL;

  public:
    /**
     * Constructor of AdvertiserManager.
     */
    AdvertiserManager(GattNativeInterfaceV2 *mGattIf);
    ~AdvertiserManager();
    void start();
    void cleanup();
    advertise_parameters_t parseParams(AdvertisingSetParameters *parameters);
    periodic_advertising_parameters_t parsePeriodicParams(PeriodicAdvertiseParameters *parameter);
    void startAdvertisingSet(AdvertisingSetParameters& parameters, AdvertiseData& advertiseData,
                                   AdvertiseData& scanResponse,
                                   PeriodicAdvertiseParameters& periodicParameters,
                                   AdvertiseData& periodicData, int duration, int maxExtAdvEvents,
                                   IAdvertisingSetCallback& callback);
    void getOwnAddress(int advertiserId);
    void stopAdvertisingSet(IAdvertisingSetCallback& callback);
    void enableAdvertisingSet(int advertiserId, bool enable, int duration, int maxExtAdvEvents);
    void setAdvertisingData(int advertiserId, AdvertiseData& data);
    void setScanResponseData(int advertiserId, AdvertiseData& data);
    void setAdvertisingParameters(int advertiserId, AdvertisingSetParameters& parameters);
    void setPeriodicAdvertisingParameters(int advertiserId,
                                                     PeriodicAdvertiseParameters& parameters);
    void setPeriodicAdvertisingData(int advertiserId, AdvertiseData& data);
    void setPeriodicAdvertisingEnable(int advertiserId, bool enable);
    void stopAdvertisingSets();
    void onAdvertisingSetStarted(int regId, int advertiserId, int txPower, int status);
    void onAdvertisingEnabled(int advertiserId, bool enable, int status);
    void onOwnAddressRead(int advertiserId, int addressType, string *address);
    void onAdvertisingDataSet(int advertiserId, int status);
    void onScanResponseDataSet(int advertiserId, int status);
    void onAdvertisingParametersUpdated(int advertiserId, int txPower, int status);
    void onPeriodicAdvertisingParametersUpdated(int advertiserId, int status);
    void onPeriodicAdvertisingDataSet(int advertiserId, int status);
    void onPeriodicAdvertisingEnabled(int advertiserId, bool enable, int status);
    std::vector<uint8_t> toVec(uint8_t *data);
};
}
#endif
