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

#include "ScanFilter.hpp"

#define LOGTAG "ScanFilter"

using namespace std;
namespace gatt{

const ScanFilter *ScanFilter::EMPTY = ScanFilter::Builder().build();

static bool maskedEquals128Bit(Uuid uuid, Uuid mask)
{
  Uuid::UUID128Bit uuid128Bit = uuid.To128BitBE();
  Uuid::UUID128Bit mask128Bit = mask.To128BitBE();
  bool res = 1;
  for (int i = 0; i < Uuid::kNumBytes128; i++)
    res &= uuid128Bit[i] & mask128Bit[i];

  return res;
}

static bool maskedEquals32Bit(Uuid uuid, Uuid mask)
{
  // this returns big endian representation
  std::array<uint8_t,Uuid::kNumBytes32> uuid32Bit = uuid.As32BitBE();
  // this returns big endian representation
  std::array<uint8_t,Uuid::kNumBytes32> mask32Bit = mask.As32BitBE();
  bool res = 1;
  for (int i = 0; i < Uuid::kNumBytes32; i++)
    res &= uuid32Bit[i] & mask32Bit[i];

  return res;
}

static bool maskedEquals16Bit(Uuid uuid, Uuid mask)
{
  // this returns big endian representation
  std::array<uint8_t,Uuid::kNumBytes16> uuid16Bit= uuid.As16BitBE();
  // this returns big endian representation
  std::array<uint8_t,Uuid::kNumBytes16> mask16Bit = mask.As16BitBE();
  bool res = 1;
  for (int i = 0; i < Uuid::kNumBytes16; i++)
    res &= uuid16Bit[i] & mask16Bit[i];

  return res;
}

static bool maskedEquals(Uuid a, Uuid b, Uuid mask)
{
  if (mask == Uuid::kEmpty) {
    return (a == b);
  }

  if((a.GetShortestRepresentationSize() != mask.GetShortestRepresentationSize())
      || (b.GetShortestRepresentationSize() != mask.GetShortestRepresentationSize())) {
    ALOGE(LOGTAG "Mask and Uuid Bit not Equal");
    return false;
  }

  if (a.GetShortestRepresentationSize() == Uuid::kNumBytes16) {
    return maskedEquals16Bit(a,mask) && maskedEquals16Bit(b, mask);
  }
  else if (a.GetShortestRepresentationSize() == Uuid::kNumBytes32) {
    return maskedEquals32Bit(a,mask) && maskedEquals32Bit(b, mask);
  }
  else {
    return maskedEquals128Bit(a,mask) && maskedEquals128Bit(b, mask);
  }
  return false;
}

static const int ADDRESS_LENGTH = 17;

static bool checkBluetoothAddress(string address)
{
  if (address.empty() || address.length() != ADDRESS_LENGTH) {
    return false;
  }
  for (int i = 0; i < ADDRESS_LENGTH; i++) {
    char c = address.at(i);
    switch (i % 3) {
      case 0:
      case 1:
        if ((c >= '0' && c <= '9') || (c >= 'A' && c <= 'F') ||
            (c >= 'a' && c <= 'f')) {
          // hex character, OK
          break;
        }
          return false;
      case 2:
        if (c == ':') {
          break;  // OK
        }
        return false;
    }
  }
  return true;
}

inline bool caseInsCharCompareN(char a, char b)
{
  return(toupper(a) == toupper(b));
}
static bool caseInsCompare(const string& s1, const string& s2)
{
   return((s1.size() == s2.size()) &&
            equal(s1.begin(), s1.end(), s2.begin(), caseInsCharCompareN));
}
ScanFilter::ScanFilter(string name, string deviceAddress, Uuid uuid,
    Uuid uuidMask, Uuid solicitationUuid,
    Uuid solicitationUuidMask, Uuid serviceDataUuid,
    std::vector<uint8_t> serviceData, std::vector<uint8_t> serviceDataMask,
    int manufacturerId, std::vector<uint8_t> manufacturerData,
    std::vector<uint8_t> manufacturerDataMask) :mDeviceName(name),
  mServiceUuid(uuid), mServiceUuidMask ( uuidMask),
  mServiceSolicitationUuid(solicitationUuid),
  mServiceSolicitationUuidMask(solicitationUuidMask),
  mDeviceAddress(deviceAddress), mServiceDataUuid(serviceDataUuid),
  mServiceData(serviceData), mServiceDataMask(serviceDataMask),
  mManufacturerId(manufacturerId), mManufacturerData(manufacturerData),
  mManufacturerDataMask( manufacturerDataMask)
  {}

ScanFilter::~ScanFilter()
{
}

bool ScanFilter::matchesServiceUuid(Uuid uuid, Uuid mask, Uuid data)
{
  return maskedEquals(data, uuid, mask);
}

bool ScanFilter::matchesServiceUuids(Uuid uuid, Uuid UuidMask,
              std::vector<Uuid> uuids)
{
  if (uuid == Uuid::kEmpty) {
    return true;
  }
  if (uuids.empty()) {
    return false;
  }

  for (Uuid pUuid : uuids) {
    Uuid uuidMask = UuidMask == Uuid::kEmpty ? Uuid::kEmpty : UuidMask;
    if (matchesServiceUuid(uuid, uuidMask, pUuid)) {
        return true;
    }
  }
  return false;
}

bool ScanFilter::matchesServiceSolicitationUuids(Uuid solicitationUuid,
    Uuid solicitationUuidMask, std::vector<Uuid> solicitationUuids)
{
  if (solicitationUuid == Uuid::kEmpty) {
    return true;
  }
  if (solicitationUuids.empty()) {
    return false;
  }

  for (Uuid parcelSolicitationUuid : solicitationUuids) {
    Uuid solicitationUuidMask = solicitationUuidMask == Uuid::kEmpty
            ? Uuid::kEmpty : solicitationUuidMask;
    if (matchesServiceUuid(solicitationUuid, solicitationUuidMask,
            solicitationUuid)){
      return true;
    }
  }
  return false;
}

bool ScanFilter::matchesServiceSolicitationUuid(Uuid solicitationUuid,
    Uuid solicitationUuidMask, Uuid data)
{
  return maskedEquals(data, solicitationUuid, solicitationUuidMask);
}

bool ScanFilter::matchesPartialData(const std::vector<uint8_t> data,
                const std::vector<uint8_t> dataMask, std::vector<uint8_t> parsedData)
{
  if (parsedData.empty() || parsedData.size() < data.size()) {
    return false;
  }
  if (dataMask.empty()) {
    for (int i = 0; i < data.size(); ++i) {
      if (!caseInsCharCompareN(parsedData[i],data[i])) {
        return false;
      }
    }
    return true;
  }
  for (int i = 0; i < data.size(); ++i) {
    if (!caseInsCharCompareN((dataMask[i] & parsedData[i]), (dataMask[i] & data[i]))) {
      return false;
    }
  }
  return true;
}

bool ScanFilter::isAllFieldsEmpty()
{
  return *EMPTY == *this;
}

string ScanFilter::getDeviceName()
{
  return mDeviceName;
}

Uuid ScanFilter::getServiceUuid()
{
  return mServiceUuid;
}

Uuid ScanFilter::getServiceUuidMask()
{
  return mServiceUuidMask;
}

Uuid ScanFilter::getServiceSolicitationUuid()
{
    return mServiceSolicitationUuid;
}

Uuid ScanFilter::getServiceSolicitationUuidMask()
{
    return mServiceSolicitationUuidMask;
}

Uuid ScanFilter::getServiceDataUuid()
{
  return mServiceDataUuid;
}

string ScanFilter::getDeviceAddress()
{
  return mDeviceAddress;
}

const std::vector<uint8_t> ScanFilter::getServiceData()
{
  return mServiceData;
}

const std::vector<uint8_t> ScanFilter::getServiceDataMask()
{
  return mServiceDataMask;
}

int ScanFilter::getManufacturerId()
{
  return mManufacturerId;
}

const std::vector<uint8_t> ScanFilter::getManufacturerData()
{
  return mManufacturerData;
}

const std::vector<uint8_t> ScanFilter::getManufacturerDataMask()
{
  return mManufacturerDataMask;
}

bool ScanFilter::matches(ScanResult *scanResult)
{
  if (scanResult == NULL) {
    return false;
  }
  string device = scanResult->getDevice();
  // Device match.
  if (!mDeviceAddress.empty()  && (!device.empty() || !caseInsCompare(mDeviceAddress,device))) {
    return false;
  }

  ScanRecord *scanRecord = scanResult->getScanRecord();

  // Scan record is NULL but there exist filters on it.
  if (scanRecord == NULL
          && (!mDeviceName.empty() || mServiceUuid != Uuid::kEmpty || !mManufacturerData.empty()
          || !mServiceData.empty() || mServiceSolicitationUuid != Uuid::kEmpty)) {
    return false;
  }

  // Local name match.
  if (!mDeviceName.empty() && !(caseInsCompare(mDeviceName, scanRecord->getDeviceName()))) {
    return false;
  }

  // UUID match.
  if (mServiceUuid != Uuid::kEmpty && !matchesServiceUuids(mServiceUuid, mServiceUuidMask,
          scanRecord->getServiceUuids())) {
    return false;
  }

  // solicitation UUID match.
  if (mServiceSolicitationUuid != Uuid::kEmpty && !matchesServiceSolicitationUuids(
          mServiceSolicitationUuid, mServiceSolicitationUuidMask,
          scanRecord->getServiceSolicitationUuids())) {
    return false;
  }

  // Service data match
  if (mServiceDataUuid != Uuid::kEmpty) {
    if (!matchesPartialData(mServiceData, mServiceDataMask,
            scanRecord->getServiceData(mServiceDataUuid))) {
      return false;
    }
  }

  // Manufacturer data match.
  if (mManufacturerId >= 0) {
    if (!matchesPartialData(mManufacturerData, mManufacturerDataMask,
            scanRecord->getManufacturerSpecificData(mManufacturerId))) {
      return false;
    }
  }
  // All filters match.
  return true;
}

ScanFilter::Builder ScanFilter::Builder::setDeviceName(string deviceName)
{
  mDeviceName = deviceName;
  return *this;
}

ScanFilter::Builder ScanFilter::Builder::setDeviceAddress(string deviceAddress)
{
  if (!deviceAddress.empty() && !checkBluetoothAddress(deviceAddress)) {
    throw std::invalid_argument("invalid device address " + deviceAddress);
  }
  mDeviceAddress = deviceAddress;
  return *this;
}

ScanFilter::Builder ScanFilter::Builder::setServiceUuid(Uuid serviceUuid)
{
  mServiceUuid = serviceUuid;
  mUuidMask = Uuid::kEmpty; // clear uuid mask
  return *this;
}

ScanFilter::Builder ScanFilter::Builder::setServiceUuid(Uuid serviceUuid, Uuid uuidMask)
{
  if (mUuidMask != Uuid::kEmpty && mServiceUuid == Uuid::kEmpty) {
    throw std::invalid_argument("uuid is null while uuidMask is not null!");
  }
  mServiceUuid = serviceUuid;
  mUuidMask = uuidMask;
  return *this;
}

ScanFilter::Builder ScanFilter::Builder::setServiceSolicitationUuid(Uuid serviceSolicitationUuid)
{
  mServiceSolicitationUuid = serviceSolicitationUuid;
  return *this;
}

ScanFilter::Builder ScanFilter::Builder::setServiceSolicitationUuid(Uuid serviceSolicitationUuid,
   Uuid solicitationUuidMask)
{
  if (mServiceSolicitationUuidMask != Uuid::kEmpty && mServiceSolicitationUuid == Uuid::kEmpty) {
     throw std::invalid_argument(
             "SolicitationUuid is null while SolicitationUuidMask is not null!");
  }
  mServiceSolicitationUuid = serviceSolicitationUuid;
  mServiceSolicitationUuidMask = solicitationUuidMask;
  return *this;
}

ScanFilter::Builder ScanFilter::Builder::setServiceData(Uuid serviceDataUuid,
                                                              std::vector<uint8_t> serviceData)
{
  if (serviceDataUuid == Uuid::kEmpty) {
    throw std::invalid_argument("serviceDataUuid is null");
  }
  mServiceDataUuid = serviceDataUuid;
  mServiceData = serviceData;
  mServiceDataMask.clear(); // clear service data mask
  return *this;
}

ScanFilter::Builder ScanFilter::Builder::setServiceData(Uuid serviceDataUuid,
        std::vector<uint8_t> serviceData, std::vector<uint8_t> serviceDataMask)
{
  if (serviceDataUuid == Uuid::kEmpty) {
    throw std::invalid_argument("serviceDataUuid is null");
  }
  if (!mServiceDataMask.empty()) {
    if (mServiceData.empty()) {
      throw std::invalid_argument(
              "serviceData is null while serviceDataMask is not null");
    }
    // Since the mServiceDataMask is a bit mask for mServiceData, the lengths of the two
    // byte array need to be the same.
    if (mServiceData.size() != mServiceDataMask.size()) {
      throw std::invalid_argument(
              "size mismatch for service data and service data mask");
    }
  }
  mServiceDataUuid = serviceDataUuid;
  mServiceData = serviceData;
  mServiceDataMask = serviceDataMask;
  return *this;
}

ScanFilter::Builder ScanFilter::Builder::setManufacturerData(int manufacturerId,
                                                        std::vector<uint8_t> manufacturerData)
{
  if (!manufacturerData.empty() && manufacturerId < 0) {
    throw std::invalid_argument("invalid manufacture id");
  }
  mManufacturerId = manufacturerId;
  mManufacturerData = manufacturerData;
  mManufacturerDataMask.clear(); // clear manufacturer data mask
  return *this;
}

ScanFilter::Builder ScanFilter::Builder::setManufacturerData(int manufacturerId, std::vector<uint8_t> manufacturerData,
        std::vector<uint8_t> manufacturerDataMask)
{
  if (!manufacturerData.empty() && manufacturerId < 0) {
    throw std::invalid_argument("invalid manufacture id");
  }
  if (!mManufacturerDataMask.empty()) {
    if (mManufacturerData.empty()) {
      throw std::invalid_argument(
                "manufacturerData is null while manufacturerDataMask is not null");
    }
    // Since the mManufacturerDataMask is a bit mask for mManufacturerData, the lengths
    // of the two byte array need to be the same.
    if (mManufacturerData.size() != mManufacturerDataMask.size()) {
      throw std::invalid_argument(
                "size mismatch for manufacturerData and manufacturerDataMask");
    }
  }
  mManufacturerId = manufacturerId;
  mManufacturerData = manufacturerData;
  mManufacturerDataMask = manufacturerDataMask;
  return *this;
}

ScanFilter* ScanFilter::Builder::build()
{
  return new ScanFilter(mDeviceName, mDeviceAddress,
                     mServiceUuid, mUuidMask, mServiceSolicitationUuid,
                     mServiceSolicitationUuidMask,
                     mServiceDataUuid, mServiceData, mServiceDataMask,
                     mManufacturerId, mManufacturerData, mManufacturerDataMask);
}

bool ScanFilter::operator==(const ScanFilter& rhs) const
{
  return ( (this->mDeviceAddress == rhs.mDeviceAddress) &&
            (this->mDeviceName == rhs.mDeviceName) &&
           (this->mServiceData.size() == rhs.mServiceData.size()) &&
           (std::equal (this->mServiceData.begin(), this->mServiceData.end(),
                                                        rhs.mServiceData.begin())) &&
           (this->mServiceDataMask.size() == rhs.mServiceDataMask.size()) &&
           (std::equal (this->mServiceDataMask.begin(), this->mServiceDataMask.end(),
                                                        rhs.mServiceDataMask.begin())) &&
           (this->mServiceDataUuid == rhs.mServiceDataUuid) &&
           (this->mServiceSolicitationUuid == rhs.mServiceSolicitationUuid) &&
           (this->mServiceSolicitationUuidMask == rhs.mServiceSolicitationUuidMask) &&
           (this->mServiceUuid == rhs.mServiceUuid) &&
           (this->mServiceUuidMask == rhs.mServiceUuidMask) &&
           (this->mManufacturerData.size() == rhs.mManufacturerData.size()) &&
           (std::equal (this->mManufacturerData.begin(), this->mManufacturerData.end(),
                                                      rhs.mManufacturerData.begin())) &&
           (this->mManufacturerDataMask.size() == rhs.mManufacturerDataMask.size()) &&
           (std::equal (this->mManufacturerDataMask.begin(), this->mManufacturerDataMask.end(),
                                                     rhs.mManufacturerDataMask.begin())) &&
           (this->mManufacturerId == rhs.mManufacturerId));
}
}//namespace gatt
