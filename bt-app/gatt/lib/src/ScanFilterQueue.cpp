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

#include "ScanFilterQueue.hpp"

namespace gatt {

void ScanFilterQueue::addDeviceAddress(string address, uint8_t type)
{
  Entry *entry = new Entry();
  entry->type = TYPE_DEVICE_ADDRESS;
  entry->address = address;
  entry->addr_type = type;
  mEntries.insert(entry);
}

void ScanFilterQueue::addServiceChanged()
{
  Entry *entry = new Entry();
  entry->type = TYPE_SERVICE_DATA_CHANGED;
  mEntries.insert(entry);
}

void ScanFilterQueue::addUuid(Uuid uuid)
{
  Entry *entry = new Entry();
  entry->type = TYPE_SERVICE_UUID;
  entry->uuid = uuid;
  entry->uuid_mask = Uuid::kEmpty;
  mEntries.insert(entry);
}

void ScanFilterQueue::addUuid(Uuid uuid, Uuid uuidMask)
{
  Entry *entry = new Entry();
  entry->type = TYPE_SERVICE_UUID;
  entry->uuid = uuid;
  entry->uuid_mask = uuidMask;
  mEntries.insert(entry);
}

void ScanFilterQueue::addSolicitUuid(Uuid uuid)
{
  Entry *entry = new Entry();
  entry->type = TYPE_SOLICIT_UUID;
  entry->uuid = uuid;
  entry->uuid_mask = Uuid::kEmpty;
  mEntries.insert(entry);
}

void ScanFilterQueue::addSolicitUuid(Uuid uuid, Uuid uuidMask)
{
  Entry *entry = new Entry();
  entry->type = TYPE_SOLICIT_UUID;
  entry->uuid = uuid;
  entry->uuid_mask = uuidMask;
  mEntries.insert(entry);
}

void ScanFilterQueue::addName(string name)
{
  Entry *entry = new Entry();
  entry->type = TYPE_LOCAL_NAME;
  entry->name = name;
  mEntries.insert(entry);
}

void ScanFilterQueue::addManufacturerData(int company, std::vector<uint8_t> data)
{
  Entry *entry = new Entry();
  entry->type = TYPE_MANUFACTURER_DATA;
  entry->company = company;
  entry->company_mask = 0xFFFF;
  entry->data = data;
  entry->data_mask.assign('F', data.size());
  mEntries.insert(entry);
}

void ScanFilterQueue::addManufacturerData(int company, int companyMask,
                                      std::vector<uint8_t> data, std::vector<uint8_t> dataMask)
{
  Entry *entry = new Entry();
  entry->type = TYPE_MANUFACTURER_DATA;
  entry->company = company;
  entry->company_mask = companyMask;
  entry->data = data;
  entry->data_mask = dataMask;
  mEntries.insert(entry);
}

void ScanFilterQueue::addServiceData(std::vector<uint8_t> data, std::vector<uint8_t> dataMask)
{
  Entry *entry = new Entry();
  entry->type = TYPE_SERVICE_DATA;
  entry->data = data;
  entry->data_mask = dataMask;
  mEntries.insert(entry);
}

ScanFilterQueue::Entry* ScanFilterQueue::pop()
{
  if (mEntries.empty()) {
    return NULL;
  }
  std::unordered_set<Entry*>::iterator it = mEntries.begin();
  Entry *entry = *it;
  mEntries.erase(it);
  return entry;
}

/**
 * Compute feature selection based on the filters presented.
 */
int ScanFilterQueue::getFeatureSelection()
{
  int selc = 0;
  for (Entry *entry : mEntries) {
    selc |= (1 << entry->type);
  }
  return selc;
}

std::vector<ScanFilterQueue::Entry*> ScanFilterQueue::toArray()
{
  std::vector<ScanFilterQueue::Entry*> *vec =
              new std::vector<ScanFilterQueue::Entry*>(mEntries.size());
  std::copy(mEntries.begin(), mEntries.end(),vec->begin());
  return *vec;
}

/**
 * Add ScanFilter to scan filter queue.
 */
void ScanFilterQueue::addScanFilter(ScanFilter* filter)
{
  if (filter == NULL) {
    return;
  }

  if (filter->getDeviceName() != "") {
    addName(filter->getDeviceName());
  }

  if (filter->getDeviceAddress() != "") {
    addDeviceAddress(filter->getDeviceAddress(), DEVICE_TYPE_ALL);
  }

  if (filter->getServiceUuid() != Uuid::kEmpty) {
    if (filter->getServiceUuidMask() == Uuid::kEmpty) {
      addUuid(filter->getServiceUuid());
    } else {
      addUuid(filter->getServiceUuid(), filter->getServiceUuidMask());
    }
  }

  if (filter->getServiceSolicitationUuid() != Uuid::kEmpty) {
    if (filter->getServiceSolicitationUuidMask() == Uuid::kEmpty) {
      addSolicitUuid(filter->getServiceSolicitationUuid());
    } else {
      addSolicitUuid(filter->getServiceSolicitationUuid(),
              filter->getServiceSolicitationUuidMask());
    }
  }

  if (!filter->getManufacturerData().empty()) {
    if (filter->getManufacturerDataMask().empty()) {
      addManufacturerData((filter->getManufacturerId()), (filter->getManufacturerData()));
    } else {
      addManufacturerData((filter->getManufacturerId()), 0xFFFF,
             (filter->getManufacturerData()), (filter->getManufacturerDataMask()));
    }
  }

  if (filter->getServiceDataUuid() != Uuid::kEmpty && !filter->getServiceData().empty()) {
    Uuid serviceDataUuid = filter->getServiceDataUuid();
    std::vector<uint8_t> serviceData = (filter->getServiceData());
    std::vector<uint8_t> serviceDataMask = (filter->getServiceDataMask());

    if (serviceDataMask.empty()) {
      serviceDataMask.assign('F',serviceData.size());
    }

    serviceData = concate(serviceDataUuid, serviceData);
    serviceDataMask = concate(serviceDataUuid, serviceDataMask);
    if (!serviceData.empty() && !serviceDataMask.empty()) {
      addServiceData(serviceData, serviceDataMask);
    }
  }
}

std::vector<uint8_t> ScanFilterQueue::concate(Uuid serviceDataUuid,
                                                std::vector<uint8_t> serviceData)
{
  std::vector<uint8_t> uu = Uuid::uuidToByte(serviceDataUuid);
  int uuidLen =  uu.size();
  int serviceLen = serviceData.size();
  int dataLen = uuidLen + (serviceData.empty() ? 0 : serviceLen);
  // If data is too long, don't add it to hardware scan filter.
  if (dataLen > MAX_LEN_PER_FIELD) {
    return {};
  }
  std::vector<uint8_t>concated (dataLen);
  concated.clear();
  concated.assign(uu.begin(), uu.end());

  if (!serviceData.empty()) {
    concated.insert(concated.begin()+uuidLen,serviceData.begin(), serviceData.end());
  }
  return concated;
}
}
