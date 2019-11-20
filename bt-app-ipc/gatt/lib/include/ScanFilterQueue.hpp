/*
 * Copyright (c) 2018, The Linux Foundation. All rights reserved.
 * Not a Contribution.
 * Copyright (C) 2013 The Android Open Source Project
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

#ifndef SCAN_FILTER_QUEUE_HPP_
#define SCAN_FILTER_QUEUE_HPP_
#pragma once

#include "ScanFilter.hpp"
#include <unordered_set>
#include <vector>
#include <cstring>
#include "utils/include/uuid.h"

using namespace std;
using std::string;
using btapp::Uuid;
namespace gatt {

/**
 * Helper class used to manage advertisement package filters.
 *
 * @hide
 */
class ScanFilterQueue {
  public:
    static const int TYPE_DEVICE_ADDRESS = 0;
    static const int TYPE_SERVICE_DATA_CHANGED = 1;
    static const int TYPE_SERVICE_UUID = 2;
    static const int TYPE_SOLICIT_UUID = 3;
    static const int TYPE_LOCAL_NAME = 4;
    static const int TYPE_MANUFACTURER_DATA = 5;
    static const int TYPE_SERVICE_DATA = 6;

  private:
    // Max length is 31 - 3(flags) - 2 (one byte for length and one byte for type).
    static const int MAX_LEN_PER_FIELD = 26;

    // Values defined in bluedroid.
    static const uint8_t DEVICE_TYPE_ALL = 2;

  public:
    class Entry {
      public:
        uint8_t type;
        string address;
        uint8_t addr_type;
        Uuid uuid;
        Uuid uuid_mask;
        string name;
        int company;
        int company_mask;
        std::vector<uint8_t> data;
        std::vector<uint8_t> data_mask;
    };

  private:
    std::unordered_set<Entry*> mEntries ;
    std::vector<uint8_t> concate(Uuid serviceDataUuid, std::vector<uint8_t> serviceData);

  public:
    void addDeviceAddress(string address, uint8_t type);
    void addServiceChanged();
    void addUuid(Uuid uuid);
    void addUuid(Uuid uuid, Uuid uuidMask);
    void addSolicitUuid(Uuid uuid);
    void addSolicitUuid(Uuid uuid, Uuid uuidMask);
    void addName(string name);
    void addManufacturerData(int company, std::vector<uint8_t> data);
    void addManufacturerData(int company, int companyMask, std::vector<uint8_t> data,
                                   std::vector<uint8_t> dataMask);
    void addServiceData(std::vector<uint8_t> data, std::vector<uint8_t> dataMask);
    Entry* pop();

    /**
     * Compute feature selection based on the filters presented.
     */
    int getFeatureSelection();

    std::vector<ScanFilterQueue::Entry*> toArray();

    /**
     * Add ScanFilter to scan filter queue.
     */
    void addScanFilter(ScanFilter* filter);
};
}
#endif
