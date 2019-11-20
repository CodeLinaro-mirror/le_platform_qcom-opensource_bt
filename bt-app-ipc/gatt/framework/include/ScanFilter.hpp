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

#ifndef SCANFILTER_HPP_
#define SCANFILTER_HPP_

#pragma once

#include <vector>
#include <exception>
#include <cstring>

#include "ScanResult.hpp"
#include "ScanRecord.hpp"
#include "utils/Log.h"
#include "utils/include/uuid.h"

using namespace std;
using std::string;
using btapp::Uuid;
namespace gatt{

/**
 * Criteria for filtering result from Bluetooth LE scans. A ScanFilter allows clients to
 * restrict scan results to only those that are of interest to them.
 * <p>
 * Current filtering on the following fields are supported:
 * <li>Service UUIDs which identify the bluetooth gatt services running on the device.
 * <li>Name of remote Bluetooth LE device.
 * <li>Mac address of the remote device.
 * <li>Service data which is the data associated with a service.
 * <li>Manufacturer specific data which is the data associated with a particular manufacturer.
 *
 */
 class ScanFilter final {
  private:

    const string mDeviceName;
    const string mDeviceAddress;
    const Uuid mServiceUuid;
    const Uuid mServiceUuidMask;
    const Uuid mServiceSolicitationUuid;
    const Uuid mServiceSolicitationUuidMask;
    const Uuid mServiceDataUuid;
    const std::vector<uint8_t> mServiceData;
    const std::vector<uint8_t> mServiceDataMask;
    const int mManufacturerId;
    const std::vector<uint8_t> mManufacturerData;
    const std::vector<uint8_t> mManufacturerDataMask;

    ScanFilter(string name, string deviceAddress, Uuid uuid,
            Uuid uuidMask, Uuid solicitationUuid,
            Uuid solicitationUuidMask, Uuid serviceDataUuid,
            std::vector<uint8_t> serviceData, std::vector<uint8_t> serviceDataMask,
            int manufacturerId, std::vector<uint8_t> manufacturerData,
            std::vector<uint8_t> manufacturerDataMask);

  public:
    /**
     * Returns the filter set the device name field of Bluetooth advertisement data.
     */
    string getDeviceName();

    /**
     * Returns the filter set on the service uuid.
     */
    Uuid getServiceUuid();
    /**
     * Returns the filter set on the service uuid mask.
     */
    Uuid getServiceUuidMask();
    /**
    * Returns the filter set on the service Solicitation uuid.
    * @hide
    */
    Uuid getServiceSolicitationUuid();

    /**
    * Returns the filter set on the service Solicitation uuid mask.
    * @hide
    */
    Uuid getServiceSolicitationUuidMask();

    /**
    * Returns the filter set on the service data uuid.
    * @hide
    */
    Uuid getServiceDataUuid();

    /**
    * Returns the filter set on the device BDA.
    * @hide
    */
    string getDeviceAddress();

    /**
    * Returns the filter set on the service data.
    * @hide
    */
    const std::vector<uint8_t> getServiceData();

    /**
    * Returns the filter set on the service data mask.
    * @hide
    */
    const std::vector<uint8_t> getServiceDataMask();

    /**
     * Returns the manufacturer id. -1 if the manufacturer filter is not set.
     */
    int getManufacturerId();

    const std::vector<uint8_t> getManufacturerData();
    const std::vector<uint8_t> getManufacturerDataMask();

    /**
     * Check if the scan filter matches a scanResult. A scan result is considered as a match
     * if it matches all the field filters.
     */
    bool matches(ScanResult *scanResult);

    /**
    * Check if the uuid pattern matches the particular service uuid.
    */
    bool matchesServiceUuid(Uuid uuid, Uuid mask, Uuid data);

    /**
     * Check if the uuid pattern is contained in a list of parcel uuids.
     */
    bool matchesServiceUuids(Uuid uuid, Uuid UuidMask,
            std::vector<Uuid> uuids);

     /**
     * Check if the solicitation uuid pattern is contained in a list of parcel uuids.
     */
    bool matchesServiceSolicitationUuids(Uuid solicitationUuid,
            Uuid parcelSolicitationUuidMask, std::vector<Uuid> solicitationUuids);

    /**
     * Check if the solicitation uuid pattern matches the particular service solicitation uuid.
     */
    bool matchesServiceSolicitationUuid(Uuid solicitationUuid,
            Uuid solicitationUuidMask, Uuid data);

    /**
     *Check whether the data pattern matches the parsed data.
     */
    bool matchesPartialData(const std::vector<uint8_t> data, const std::vector<uint8_t> dataMask,
                                                                   std::vector<uint8_t> parsedData);

    /**
     * Checks if the scanfilter is empty
     */
    bool isAllFieldsEmpty();
    /** @hide */
    static const ScanFilter *EMPTY;

    bool operator==(const ScanFilter& rhs) const;
    ~ScanFilter();

    /**
     * Builder class for ScanFilter.
     */
    class Builder {
      private:
        string mDeviceName = "";
        string mDeviceAddress = "";

        Uuid mServiceUuid = Uuid::kEmpty;
        Uuid mUuidMask = Uuid::kEmpty;

        Uuid mServiceSolicitationUuid = Uuid::kEmpty;
        Uuid mServiceSolicitationUuidMask = Uuid::kEmpty;

        Uuid mServiceDataUuid = Uuid::kEmpty;
        std::vector<uint8_t> mServiceData;
        std::vector<uint8_t> mServiceDataMask;

        int mManufacturerId = -1;
        std::vector<uint8_t> mManufacturerData;
        std::vector<uint8_t> mManufacturerDataMask;

      public:
        /**
         * Set filter on device name.
         */
        Builder setDeviceName(string deviceName);

        /**
         * Set filter on device address.
         *
         * @param deviceAddress The device Bluetooth address for the filter. It needs to be in the
         * format of "01:02:03:AB:CD:EF". The device address can be validated using
         * checkBluetoothAddress.
         * @throws std::invalid_argument If the deviceAddress is invalid.
         */
        Builder setDeviceAddress(string deviceAddress);

        /**
         * Set filter on service uuid.
         */
        Builder setServiceUuid(Uuid serviceUuid);

        /**
         * Set filter on partial service uuid. The uuidMas} is the bit mask for the
         * serviceUuid. Set any bit in the mask to 1 to indicate a match is needed for the
         * bit in serviceUuid, and 0 to ignore that bit.
         *
         * @throws std::invalid_argument If serviceUuid is null but
         * uuidMask is not null.
         */
        Builder setServiceUuid(Uuid serviceUuid, Uuid uuidMask);

        /**
         * Set filter on service solicitation uuid.
         * @hide
         */
        Builder setServiceSolicitationUuid(Uuid serviceSolicitationUuid);

        /**
         * Set filter on partial service Solicitation uuid. The SolicitationUuidMask is the
         * bit mask for the serviceSolicitationUuid. Set any bit in the mask to 1 to
         * indicate a match is needed for the bit in serviceSolicitationUuid, and 0 to
         * ignore that bit.
         *
         * @throws std::invalid_argument If serviceSolicitationUuid is null but
         *         serviceSolicitationUuidMask is not null.
         * @hide
         */
        Builder setServiceSolicitationUuid(Uuid serviceSolicitationUuid,
                Uuid solicitationUuidMask);

        /**
         * Set filtering on service data.
         *
         * @throws std::invalid_argument If serviceDataUuid is null.
         */
        Builder setServiceData(Uuid serviceDataUuid, std::vector<uint8_t> serviceData);

        /**
         * Set partial filter on service data. For any bit in the mask, set it to 1 if it needs to
         * match the one in service data, otherwise set it to 0 to ignore that bit.
         * <p>
         * The serviceDataMask must have the same length of the serviceData.
         *
         * @throws std::invalid_argument If serviceDataUuid is null or
         * serviceDataMask is null while serviceData is not or
         * serviceDataMask and serviceData has different length.
         */
        Builder setServiceData(Uuid serviceDataUuid,
                 std::vector<uint8_t> serviceData,  std::vector<uint8_t> serviceDataMask);

        /**
         * Set filter on on manufacturerData. A negative manufacturerId is considered as invalid id.
         * <p>
         * Note the first two bytes of the manufacturerData is the manufacturerId.
         *
         * @throws std::invalid_argument If the manufacturerId is invalid.
         */
        Builder setManufacturerData(int manufacturerId,  std::vector<uint8_t> manufacturerData);

        /**
         * Set filter on partial manufacture data. For any bit in the mask, set it the 1 if it needs
         * to match the one in manufacturer data, otherwise set it to 0.
         * <p>
         * The manufacturerDataMask must have the same length of manufacturerData.
         *
         * @throws std::invalid_argument If the manufacturerId is invalid, or
         * manufacturerData is null while manufacturerDataMask is not, or
         * manufacturerData and  manufacturerDataMask have different length.
         */
        Builder setManufacturerData(int manufacturerId,  std::vector<uint8_t> manufacturerData,
                 std::vector<uint8_t> manufacturerDataMask);

        /**
         * Build ScanFilter.
         *
         * @throws std::invalid_argument If the filter cannot be built.
         */
        ScanFilter* build();
    };

};
}//namespace gatt
#endif
