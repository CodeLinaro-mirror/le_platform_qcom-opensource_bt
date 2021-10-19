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

#include "AdvertiseData.hpp"
#include <cstring>
#define LOGTAG "AdvertiseData"
using namespace std;
namespace gatt {

AdvertiseData::AdvertiseData(std::vector<Uuid> serviceUuids,
                                std::map<int, std::vector<uint8_t>> manufacturerData,
                                std::map<Uuid, std::vector<uint8_t>> serviceData,
                                bool includeTxPowerLevel,
                                bool includeDeviceName, std::string deviceName) :
  mServiceUuids(serviceUuids),
  mManufacturerSpecificData(manufacturerData),
  mServiceData(serviceData),
  mIncludeTxPowerLevel(includeTxPowerLevel),
  mIncludeDeviceName(includeDeviceName),
  mDeviceName(deviceName)
{}

AdvertiseData::~AdvertiseData()
{
  mServiceUuids.clear();
  mManufacturerSpecificData.clear();
  mServiceData.clear();
}

std::vector<Uuid> AdvertiseData::getServiceUuids()
{
  return mServiceUuids;
}

std::map<int, std::vector<uint8_t>> AdvertiseData::getManufacturerSpecificData()
{
  return mManufacturerSpecificData;
}

std::map<Uuid, std::vector<uint8_t>> AdvertiseData::getServiceData()
{
  return mServiceData;
}

bool AdvertiseData::getIncludeTxPowerLevel()
{
  return mIncludeTxPowerLevel;
}

bool AdvertiseData::getIncludeDeviceName()
{
  return mIncludeDeviceName;
}

std::string AdvertiseData::getBleDeviceName()
{
  return mDeviceName;
}

AdvertiseData::Builder AdvertiseData::Builder::addServiceUuid(Uuid serviceUuid)
{
  if (serviceUuid == Uuid::kEmpty) {
    throw std::invalid_argument("serivceUuids are NULL");
  }

  mServiceUuids.push_back(serviceUuid);

  return *this;
}

AdvertiseData::Builder AdvertiseData::Builder::addServiceData(Uuid serviceDataUuid,
                                                                   std::vector<uint8_t> serviceData)
{
  if (serviceDataUuid == Uuid::kEmpty || serviceData.empty()) {
    throw std::invalid_argument("serviceDataUuid or serviceData is NULL");
  }

  mServiceData.insert(std::pair<Uuid,std::vector<uint8_t>>(serviceDataUuid, serviceData));

  return *this;
}

AdvertiseData::Builder AdvertiseData::Builder::addManufacturerData(int manufacturerId,
                                                    std::vector<uint8_t> manufacturerSpecificData)
{
  if (manufacturerId < 0) {
    throw std::invalid_argument("invalid manufacturerId - " + manufacturerId);
  }

  if (manufacturerSpecificData.empty()) {
    throw std::invalid_argument("addManufacturerData() - manufacturerSpecificData is NULL");
  }

  mManufacturerSpecificData.insert(
          std::pair<int,std::vector<uint8_t>>(manufacturerId, manufacturerSpecificData));

  return *this;
}

AdvertiseData::Builder AdvertiseData::Builder::setIncludeTxPowerLevel(bool includeTxPowerLevel)
{
  mIncludeTxPowerLevel = includeTxPowerLevel;
  return *this;
}

AdvertiseData::Builder AdvertiseData::Builder::setIncludeDeviceName(bool includeDeviceName)
{
  mIncludeDeviceName = includeDeviceName;
  return *this;
}

AdvertiseData::Builder AdvertiseData::Builder::setBleName(std::string DeviceName)
{
  mDeviceName = DeviceName;
  return *this;
}

AdvertiseData* AdvertiseData::Builder::build()
{
  return new AdvertiseData(mServiceUuids, mManufacturerSpecificData, mServiceData,
         mIncludeTxPowerLevel, mIncludeDeviceName, mDeviceName);
}
}//namespace gatt
