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

#ifndef SCANRECORD_HPP_
#define SCANRECORD_HPP_

#pragma once

#include "osi/include/log.h"
#include "utils/include/uuid.h"
#include <unordered_map>
#include <sstream>
#include <typeinfo>
#include <vector>
#include <cstring>
using namespace std;
using std::string;
using btapp::Uuid;
namespace gatt{

/**
 * Represents a scan record from Bluetooth LE scan.
 */
class ScanRecord final {
  private:
    // The following data type values are assigned by Bluetooth SIG.
    // For more details refer to Bluetooth 4.1 specification, Volume 3, Part C, Section 18.
    static const int DATA_TYPE_FLAGS = 0x01;
    static const int DATA_TYPE_SERVICE_UUIDS_16_BIT_PARTIAL = 0x02;
    static const int DATA_TYPE_SERVICE_UUIDS_16_BIT_COMPLETE = 0x03;
    static const int DATA_TYPE_SERVICE_UUIDS_32_BIT_PARTIAL = 0x04;
    static const int DATA_TYPE_SERVICE_UUIDS_32_BIT_COMPLETE = 0x05;
    static const int DATA_TYPE_SERVICE_UUIDS_128_BIT_PARTIAL = 0x06;
    static const int DATA_TYPE_SERVICE_UUIDS_128_BIT_COMPLETE = 0x07;
    static const int DATA_TYPE_LOCAL_NAME_SHORT = 0x08;
    static const int DATA_TYPE_LOCAL_NAME_COMPLETE = 0x09;
    static const int DATA_TYPE_TX_POWER_LEVEL = 0x0A;
    static const int DATA_TYPE_SERVICE_DATA_16_BIT = 0x16;
    static const int DATA_TYPE_SERVICE_DATA_32_BIT = 0x20;
    static const int DATA_TYPE_SERVICE_DATA_128_BIT = 0x21;
    static const int DATA_TYPE_SERVICE_SOLICITATION_UUIDS_16_BIT = 0x14;
    static const int DATA_TYPE_SERVICE_SOLICITATION_UUIDS_32_BIT = 0x1F;
    static const int DATA_TYPE_SERVICE_SOLICITATION_UUIDS_128_BIT = 0x15;
    static const int DATA_TYPE_MANUFACTURER_SPECIFIC_DATA = 0xFF;

    // Flags of the advertising data.
    const int mAdvertiseFlags;

    const std::vector<Uuid> mServiceUuids;

    const std::vector<Uuid> mServiceSolicitationUuids;

    const std::unordered_map<int, std::vector<uint8_t> > mManufacturerSpecificData;

    const std::unordered_map<Uuid, std::vector<uint8_t> > mServiceData;

    // Transmission power level(in dB).
    const int mTxPowerLevel;

    // Local name of the Bluetooth LE device.
    const string mDeviceName;

    // Raw bytes of scan record.
    const std::vector<uint8_t>& mBytes;

  public:
    /**
     * Returns the advertising flags indicating the discoverable mode and capability of the device.
     * Returns -1 if the flag field is not set.
     */
    int getAdvertiseFlags();

    /**
     * Returns a list of service UUIDs within the advertisement that are used to identify the
     * bluetooth GATT services.
     */
    std::vector<Uuid> getServiceUuids();

    /**
     * Returns a list of service solicitation UUIDs within the advertisement that are used to
     * identify the bluetooth GATT services.
     * @hide
     */
    std::vector<Uuid> getServiceSolicitationUuids();

    /**
     * Returns a sparse array of manufacturer identifier and its corresponding manufacturer specific
     * data.
     */
    std::unordered_map<int, std::vector<uint8_t> > getManufacturerSpecificData();

    /**
     * Returns the manufacturer specific data associated with the manufacturer id. Returns
     * null if the manufacturerId is not found.
     */

    std::vector<uint8_t> getManufacturerSpecificData(int manufacturerId);

    /**
     * Returns a map of service UUID and its corresponding service data.
     */
    std::unordered_map<Uuid, std::vector<uint8_t> > getServiceData();

    /**
     * Returns the service data byte array associated with the {@code serviceUuid}. Returns
     * null if the serviceDataUuid is not found.
     */

    std::vector<uint8_t> getServiceData(Uuid serviceDataUuid);

    /**
     * Returns the transmission power level of the packet in dBm. Returns Integer#MIN_VALUE
     * if the field is not set. This value can be used to calculate the path loss of a received
     * packet using the following equation:
     * <p>
     * <code>pathloss = txPowerLevel - rssi</code>
     */
    int getTxPowerLevel();

    /**
     * Returns the local name of the BLE device. The is a UTF-8 encoded string.
     */

    string getDeviceName();

    /**
     * Returns raw bytes of scan record.
     */
    std::vector<uint8_t> getBytes();

  private:
    ScanRecord(std::vector<Uuid> serviceUuids,
            std::vector<Uuid> serviceSolicitationUuids,
            std::unordered_map<int, std::vector<uint8_t> >& manufacturerData,
            std::unordered_map<Uuid, std::vector<uint8_t> >& serviceData,
            int advertiseFlags, int txPowerLevel,
            string localName, std::vector<uint8_t>& bytes);

  public:
    /**
     * Parse scan record bytes to ScanRecord.
     * <p>
     * The format is defined in Bluetooth 4.1 specification, Volume 3, Part C, Section 11 and 18.
     * <p>
     * All numerical multi-byte entities and values shall use little-endian <strong>byte</strong>
     * order.
     *
     * @param scanRecord The scan record of Bluetooth LE advertisement and/or scan response.
     * @hide
     */
    static ScanRecord* parseFromBytes(std::vector<uint8_t> scanRecord);

    std::string ToString() const;

  private:
    /**
    * Parse service UUIDs.
    */
    static int parseServiceUuid(std::vector<uint8_t> scanRecord, int currentPos, int dataLength,
            int uuidLength, std::vector<Uuid> &serviceUuids);

    /**
     * Parse service Solicitation UUIDs.
     * @hide
     */
    static int parseServiceSolicitationUuid(std::vector<uint8_t> scanRecord, int currentPos,
            int dataLength, int uuidLength, std::vector<Uuid> &serviceSolicitationUuids);

    /**
    * Helper method to extract bytes from byte array.
    */
    static std::vector<uint8_t> extractBytes(std::vector<uint8_t> scanRecord,
                                                                          int start, int length);
};

}//namespace gatt
#endif
