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

#ifndef ADVERTISEDATA_HPP_
#define ADVERTISEDATA_HPP_

#pragma once

#include "log.h"
#include "utils/include/uuid.h"
#include <map>
#include <vector>
#include <iostream>
#include <stdexcept>

using namespace std;
using std::string;
using btapp::Uuid;
namespace gatt{

/**
* Advertise data packet container for Bluetooth LE advertising. This represents the data to be
* advertised as well as the scan response data for active scans.
* <p>
* Use AdvertiseData::Builder to create an instance of AdvertiseData to be
* advertised.
*/
class AdvertiseData final {
  private:
    std::vector<Uuid> mServiceUuids;
    std::map<int, std::vector<uint8_t>> mManufacturerSpecificData;
    std::map<Uuid, std::vector<uint8_t>> mServiceData;
    const bool mIncludeTxPowerLevel;
    const bool mIncludeDeviceName;
    const std::string mDeviceName;

    AdvertiseData(std::vector<Uuid> serviceUuids,
           std::map<int, std::vector<uint8_t>> manufacturerData,
           std::map<Uuid, std::vector<uint8_t>> serviceData,
           bool includeTxPowerLevel,
           bool includeDeviceName, std::string deviceName);

  public:
    /**
    * Destructor
    */
    ~AdvertiseData();

    /**
    * Returns a list of service UUIDs within the advertisement that are used to identify the
    * Bluetooth GATT services.
    */
    std::vector<Uuid> getServiceUuids();

    /**
    * Returns an array of manufacturer Id and the corresponding manufacturer specific data. The
    * manufacturer id is a non-negative number assigned by Bluetooth SIG.
    */
    std::map<int, std::vector<uint8_t>> getManufacturerSpecificData();

    /**
    * Returns a map of UUID and its corresponding service data.
    */
    std::map<Uuid, std::vector<uint8_t>> getServiceData();

    /**
    * Whether the transmission power level will be included in the advertisement packet.
    */
    bool getIncludeTxPowerLevel();

    /**
    * Whether the device name will be included in the advertisement packet.
    */
    bool getIncludeDeviceName();

    /**
    * Returns the BLE name set by the user as part of adv data
    */
    std::string getBleDeviceName();

    /**
    * Builder for  AdvertiseData.
    */
    class Builder final{
      private:
        std::vector<Uuid> mServiceUuids;
        std::map<int, std::vector<uint8_t>> mManufacturerSpecificData;
        std::map<Uuid, std::vector<uint8_t>> mServiceData;
        bool mIncludeTxPowerLevel = false;
        bool mIncludeDeviceName = false;
        std::string mDeviceName;

      public:
        /**
        * Add a service UUID to advertise data.
        *
        * @param serviceUuid A service UUID to be advertised.
        * @throws std::invalid_argument If the serviceUuids are null.
        */
        Builder addServiceUuid(Uuid serviceUuid);

        /**
        * Add service data to advertise data.
        *
        * @param serviceDataUuid UUID of the service the data is associated with
        * @param serviceData Service data
        * @throws std::invalid_argument If the serviceDataUuid or serviceData is
        * empty.
        */
        Builder addServiceData(Uuid serviceDataUuid, std::vector<uint8_t> serviceData);

        /**
        * Add manufacturer specific data.
        *
        * @param manufacturerId Manufacturer ID assigned by Bluetooth SIG.
        * @param manufacturerSpecificData Manufacturer specific data
        * @throws std::invalid_argument If the manufacturerId is negative or
        * manufacturerSpecificData is null.
        */
        Builder addManufacturerData(int manufacturerId,
                                          std::vector<uint8_t> manufacturerSpecificData);

        /**
        * Whether the transmission power level should be included in the advertise packet. Tx power
        * level field takes 3 bytes in advertise packet.
        */
        Builder setIncludeTxPowerLevel(bool includeTxPowerLevel);

        /**
        * Set whether the device name should be included in advertise packet.
        */
        Builder setIncludeDeviceName(bool includeDeviceName);

        /**
        * Set the BLE device name that should be included in advertise packet.
        */
        Builder setBleName(std::string DeviceName);

        /**
        * Build the AdvertiseData.
        */
        AdvertiseData* build();

      };
  };
}//namespace gatt
#endif
