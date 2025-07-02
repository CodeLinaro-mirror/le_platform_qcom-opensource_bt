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

#include <iostream>
#include "ScanClient.hpp"
#include <vector>
#include "osi/include/log.h"
#define LOGTAG "ScanCLient"
using namespace std;
namespace gatt {

static ScanSettings *DEFAULT_SCAN_SETTINGS =
         ScanSettings::Builder().setScanMode(ScanSettings::SCAN_MODE_LOW_LATENCY).build();

ScanClient::ScanClient(int scannerId, std::vector<Uuid> uuids,  ScanSettings *settings,
                          std::vector<ScanFilter*> filters,
                          std::vector<std::vector<ResultStorageDescriptor*>> storages)
{
  this->scannerId = scannerId;
  this->uuids = uuids;
  this->settings = settings;
  this->passiveSettings = NULL;
  this->filters = filters;
  this->storages = storages;
}

ScanClient::ScanClient(int scannerId)
{
  this->scannerId = scannerId;
  this->settings = DEFAULT_SCAN_SETTINGS;
  this->passiveSettings = NULL;
}

ScanClient::ScanClient(int scannerId, std::vector<Uuid> uuids)
{
  this->scannerId = scannerId;
  this->settings = DEFAULT_SCAN_SETTINGS;
  this->passiveSettings = NULL;
  this->uuids = uuids;
}

ScanClient::ScanClient(int scannerId, ScanSettings *settings, std::vector<ScanFilter*> filters)
{
  this->scannerId = scannerId;
  this->settings = settings;
  this->filters = filters;
  this->passiveSettings = NULL;
}

ScanClient::ScanClient(int scannerId, ScanSettings *settings, std::vector<ScanFilter*> filters,
        std::vector<std::vector<ResultStorageDescriptor*>> storages)
{
  this->scannerId = scannerId;
  this->settings = settings;
  this->filters = filters;
  this->passiveSettings = NULL;
  this->storages = storages;
}
}
