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
#ifndef SCAN_CLIENT_HPP_
#define SCAN_CLIENT_HPP_
#pragma once

#include "ScanSettings.hpp"
#include "utils/include/uuid.h"

#include <vector>
#include <iostream>
using namespace std;
using std::string;
using btapp::Uuid;
namespace gatt {
class ScanFilter;
class ResultStorageDescriptor;
/**
 * Helper class identifying a client that has requested LE scan results.
 *
 */
class ScanClient {
  private:

    ScanClient(int scannerId, std::vector<Uuid> uuids,    ScanSettings *settings,
                  std::vector<ScanFilter*> filters,
                  std::vector<std::vector<ResultStorageDescriptor*>> storages);

  public:
    int scannerId;
    std::vector<Uuid> uuids;
    ScanSettings *settings;
    ScanSettings *passiveSettings;
    int appUid;
    std::vector<ScanFilter*> filters;
    std::vector<std::vector<ResultStorageDescriptor*>> storages;
    // App associated with the scan client died.
    bool appDied;

    ScanClient(int scannerId);
    ScanClient(int scannerId, std::vector<Uuid> uuids);
    ScanClient(int scannerId, ScanSettings *settings, std::vector<ScanFilter*> filters);
    ScanClient(int scannerId, ScanSettings *settings, std::vector<ScanFilter*> filters,
            std::vector<std::vector<ResultStorageDescriptor*>> storages);
};
}
#endif
