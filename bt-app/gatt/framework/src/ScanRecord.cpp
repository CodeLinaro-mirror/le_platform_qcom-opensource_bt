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
 *
 * Changes from Qualcomm Innovation Center are provided under the following license:
 * Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include "ScanRecord.hpp"
#include <iostream>
#include <climits>
#include <unordered_map>
#include <vector>
#include <exception>
#include <utils/Log.h>

#define LOGTAG "ScanRecord"

using namespace std;
namespace gatt {

static Uuid uuidFromByte(uint8_t* uuidBytes, int uuidLen)
{
  if(uuidLen == Uuid::kNumBytes16){
    //convert uint8_t* to uint16_t
    std::array<uint8_t, Uuid::kNumBytes16>tmp;
    std::memcpy(&tmp[0], uuidBytes, uuidLen);
    uint16_t uuidBytes16 = ((uint16_t)tmp[1] << 8) | tmp[0];
    return Uuid::From16Bit(uuidBytes16);

  } else if (uuidLen == Uuid::kNumBytes32){
    //convert uint8_t* to uint32_t
    std::array<uint8_t, Uuid::kNumBytes32>tmp;
    std::memcpy(&tmp[0], uuidBytes, uuidLen);
    uint32_t uuidBytes32 = ((uint32_t)tmp[3] << 24) | ((uint32_t)tmp[2] << 16)
                                  | ((uint32_t)tmp[1] << 8) | tmp[0];
    return Uuid::From32Bit(uuidBytes32);
  } else {
    Uuid::UUID128Bit tmp;
    std::memcpy(&tmp[0], uuidBytes, uuidLen);
    return Uuid::From128BitBE(uuidBytes);
  }
}

static std::string vectoString(std::vector<Uuid> vec)
{
  if(vec.empty()) {
    return "{}";
  }

  std::stringstream ss;
  ss << '{';
  for( std::vector<Uuid>::iterator it = vec.begin();
            it != vec.end(); ++it) {
    Uuid uu = *it;
    ss << uu.ToString() << ", ";
  }

  ss << '}';
  return ss.str();
}

static std::string mapUuidtoString(std::unordered_map<Uuid, std::vector<uint8_t>> map)
{
  if(map.empty())
    return "{}";

  std:: stringstream ss;
  ss << "{";
  for(std::unordered_map<Uuid, std::vector<uint8_t>>::iterator it = map.begin();
                it != map.end(); ++it) {
    ss << '{';
    Uuid uu = it->first;
    ss << uu.ToString() << ", ";
    std::string str(it->second.begin(),it->second.end());
    ss << str  << "}, ";
  }

  ss << "}";

  return ss.str();
}

static std::string mapInttoString(std::unordered_map<int, std::vector<uint8_t> > map)
{
  if(map.empty())
    return "{}";

  std:: stringstream ss;
  ss << "{";
  for(std::unordered_map<int, std::vector<uint8_t>>::iterator it = map.begin();
                it != map.end(); ++it) {
    ss << '{';
    ss << it->first << ", ";
    std::string str(it->second.begin(),it->second.end());
    ss << str  << "}, ";
  }

  ss << "}";

  return ss.str();
}

int ScanRecord::getAdvertiseFlags()
{
  return mAdvertiseFlags;
}

std::vector<Uuid> ScanRecord::getServiceUuids()
{
  return mServiceUuids;
}

std::vector<Uuid> ScanRecord::getServiceSolicitationUuids()
{
  return mServiceSolicitationUuids;
}

std::unordered_map<int, std::vector<uint8_t> > ScanRecord::getManufacturerSpecificData()
{
  return mManufacturerSpecificData;
}

std::vector<uint8_t> ScanRecord::getManufacturerSpecificData(int manufacturerId)
{
  auto map_ptr = mManufacturerSpecificData.find(manufacturerId);
  if(map_ptr == mManufacturerSpecificData.end()) {
    ALOGE(LOGTAG " getManufacturerSpecificData - manufacturerId not found");
  }
  return map_ptr->second;
}

std::unordered_map<Uuid, std::vector<uint8_t> > ScanRecord::getServiceData()
{
  return mServiceData;
}

std::vector<uint8_t> ScanRecord::getServiceData(Uuid serviceDataUuid)
{
  if(serviceDataUuid == Uuid::kEmpty) {
    return {};
  }
  auto map_ptr = mServiceData.find(serviceDataUuid);
  if(map_ptr == mServiceData.end()) {
    ALOGE(LOGTAG " getServiceData - Service Data Uuid not found");
    return {};
  }
  return map_ptr->second;
}

int ScanRecord::getTxPowerLevel()
{
  return mTxPowerLevel;
}

string ScanRecord::getDeviceName()
{
  return mDeviceName;
}

std::vector<uint8_t> ScanRecord::getBytes()
{
  return mBytes;
}

ScanRecord::ScanRecord(std::vector<Uuid> serviceUuids,
        std::vector<Uuid> serviceSolicitationUuids,
        std::unordered_map<int, std::vector<uint8_t>> manufacturerData,
        std::unordered_map<Uuid, std::vector<uint8_t>> serviceData,
        int advertiseFlags, int txPowerLevel,
        string localName, std::vector<uint8_t> bytes):
  mServiceSolicitationUuids(serviceSolicitationUuids),
  mServiceUuids(serviceUuids),
  mManufacturerSpecificData(manufacturerData),
  mServiceData(serviceData),
  mDeviceName(localName),
  mAdvertiseFlags(advertiseFlags),
  mTxPowerLevel(txPowerLevel),
  mBytes(bytes)
{}

ScanRecord* ScanRecord::parseFromBytes(std::vector<uint8_t> scanRecord)
{
  if (scanRecord.empty()) {
    return NULL;
  }

  std::string t(scanRecord.begin(), scanRecord.end());
  int currentPos = 0;
  int advertiseFlag = -1;
  std::vector<Uuid> serviceUuids;
  std::vector<Uuid> serviceSolicitationUuids;
  string localName= "";
  int txPowerLevel = INT_MIN;

  std::unordered_map<int, std::vector<uint8_t> >manufacturerData;
  std::unordered_map<Uuid, std::vector<uint8_t> > serviceData;

  try {
    while (currentPos < scanRecord.size()) {
      // length is unsigned int.
      int length = scanRecord[currentPos++] & 0xFF;
      if (length == 0) {
        break;
      }
      // Note the length includes the length of the field type itself.
      int dataLength = length - 1;
      // fieldType is unsigned int.
      int fieldType = scanRecord[currentPos++] & 0xFF;
      ALOGE(LOGTAG " parseFromBytes() fieldType (%d)", fieldType);
      switch (fieldType) {
        case DATA_TYPE_FLAGS:
          advertiseFlag = scanRecord[currentPos] & 0xFF;
          break;
        case DATA_TYPE_SERVICE_UUIDS_16_BIT_PARTIAL:
        case DATA_TYPE_SERVICE_UUIDS_16_BIT_COMPLETE:
          parseServiceUuid(scanRecord, currentPos,
                                   dataLength, Uuid::kNumBytes16, serviceUuids);
          break;
        case DATA_TYPE_SERVICE_UUIDS_32_BIT_PARTIAL:
        case DATA_TYPE_SERVICE_UUIDS_32_BIT_COMPLETE:
          parseServiceUuid(scanRecord, currentPos, dataLength,
                        Uuid::kNumBytes32, serviceUuids);
          break;
        case DATA_TYPE_SERVICE_UUIDS_128_BIT_PARTIAL:
        case DATA_TYPE_SERVICE_UUIDS_128_BIT_COMPLETE:
          parseServiceUuid(scanRecord, currentPos, dataLength,
                                    Uuid::kNumBytes128, serviceUuids);
          break;
        case DATA_TYPE_SERVICE_SOLICITATION_UUIDS_16_BIT:
          parseServiceSolicitationUuid(scanRecord, currentPos, dataLength,
                                    Uuid::kNumBytes16, serviceSolicitationUuids);
          break;
        case DATA_TYPE_SERVICE_SOLICITATION_UUIDS_32_BIT:
           parseServiceSolicitationUuid(scanRecord, currentPos, dataLength,
                                  Uuid::kNumBytes32, serviceSolicitationUuids);
           break;
        case DATA_TYPE_SERVICE_SOLICITATION_UUIDS_128_BIT:
          parseServiceSolicitationUuid(scanRecord, currentPos, dataLength,
                                   Uuid::kNumBytes128, serviceSolicitationUuids);
          break;
        case DATA_TYPE_LOCAL_NAME_SHORT:
        case DATA_TYPE_LOCAL_NAME_COMPLETE:
          {
          // convert to string
          std::vector<uint8_t> temp = extractBytes(scanRecord, currentPos, dataLength);
          std::string t (temp.begin(),temp.end());
          localName = t;
          temp.clear();
          }
          break;
        case DATA_TYPE_TX_POWER_LEVEL:
          txPowerLevel = scanRecord[currentPos];
          break;
        case DATA_TYPE_SERVICE_DATA_16_BIT:
        case DATA_TYPE_SERVICE_DATA_32_BIT:
        case DATA_TYPE_SERVICE_DATA_128_BIT:
          {
            int serviceUuidLength = Uuid::kNumBytes16;
            if (fieldType == DATA_TYPE_SERVICE_DATA_32_BIT) {
              serviceUuidLength = Uuid::kNumBytes32;
            } else if (fieldType == DATA_TYPE_SERVICE_DATA_128_BIT) {
              serviceUuidLength = Uuid::kNumBytes128;
            }

            std::vector<uint8_t> serviceDataUuidBytes = extractBytes(scanRecord, currentPos,
                                                serviceUuidLength);
            if (!serviceDataUuidBytes.empty()) {
              ALOGE(LOGTAG "serviceUuidBytes not empty");
              Uuid serviceDataUuid = uuidFromByte(serviceDataUuidBytes.data(),serviceUuidLength);
              std::vector<uint8_t> serviceDataBytes = extractBytes(scanRecord,
                      currentPos + serviceUuidLength, dataLength - serviceUuidLength);
              serviceData.insert({{serviceDataUuid, serviceDataBytes}});
            }
          }
          break;
        case DATA_TYPE_MANUFACTURER_SPECIFIC_DATA:
          // The first two bytes of the manufacturer specific data are
          // manufacturer ids in little endian.
          // As spec defined, manufacturer ID must be 2 bytes.
          if (dataLength < 2) {
            ALOGE(LOGTAG "manufacturerId field truncated or not present.");
          }
          else
          {
            int manufacturerId = ((scanRecord[currentPos + 1] & 0xFF) << 8)
                                  + (scanRecord[currentPos] & 0xFF);
            std::vector<uint8_t> manufacturerDataBytes = extractBytes(scanRecord, currentPos + 2,
                                     dataLength - 2);
            if (!manufacturerDataBytes.empty()) {
              ALOGE(LOGTAG "manufacturerDataBytes not empty");
              manufacturerData.insert({{manufacturerId, manufacturerDataBytes}});
            }
          }
          break;
        default:
          // Just ignore, we don't handle such data type.
          break;
        }
      currentPos += dataLength;
  }
  if (serviceUuids.empty()) {
    serviceUuids = {};
  }
  if (serviceSolicitationUuids.empty()) {
    serviceSolicitationUuids = {};
  }
  return new ScanRecord(serviceUuids, serviceSolicitationUuids, manufacturerData,
                    serviceData, advertiseFlag, txPowerLevel, localName, scanRecord);
 } catch (exception& e) {
   std::string str(scanRecord.begin(), scanRecord.end());
   ALOGE(LOGTAG " unable to parse scan record: %s", str.c_str());
    // As the record is invalid, ignore all the parsed results for this packet
    // and return an empty record with raw scanRecord bytes in results
    return new ScanRecord(std::vector<Uuid>(), std::vector<Uuid>(),
                          std::unordered_map<int, std::vector<uint8_t>>(),
                          std::unordered_map<Uuid, std::vector<uint8_t>>(),
                          -1, INT_MIN, "", scanRecord);
 }
}

std::string ScanRecord::ToString() const
{

  std::string str = "ScanRecord [mAdvertiseFlags= " +
                std::to_string(mAdvertiseFlags) +
                ", mServiceUuids= " +
                vectoString(mServiceUuids) +
                ", mServiceSolicitationUuids=" +
                vectoString(mServiceSolicitationUuids) +
                ", mManufacturerSpecificData=" +
                mapInttoString(mManufacturerSpecificData) +
                ", mServiceData=" +
                mapUuidtoString(mServiceData) +
                ", mTxPowerLevel=" +
                std::to_string(mTxPowerLevel) +
                ", mDeviceName=" +
                mDeviceName +
                "]";
  return str;
}

int ScanRecord::parseServiceUuid(std::vector<uint8_t> scanRecord, int currentPos, int dataLength,
        int uuidLength, std::vector<Uuid> &serviceUuids)
{
  while (dataLength > 0 && uuidLength > 0) {
    std::vector<uint8_t> uuidBytes = extractBytes(scanRecord, currentPos, uuidLength);
    int uuidLen = static_cast<int>(uuidBytes.size());
    if(uuidLen == 0)
      break;
    serviceUuids.push_back(uuidFromByte(uuidBytes.data(), uuidLen));
    dataLength -= uuidLength;
    currentPos += uuidLength;
  }
  return currentPos;
}

int ScanRecord::parseServiceSolicitationUuid(std::vector<uint8_t> scanRecord, int currentPos,
        int dataLength, int uuidLength, std::vector<Uuid> &serviceSolicitationUuids)
{
  while (dataLength > 0 && uuidLength > 0) {
    std::vector<uint8_t> uuidBytes = extractBytes(scanRecord, currentPos, uuidLength);
    int uuidLen = static_cast<int>(uuidBytes.size());
    if(uuidLen == 0)
      break;
    serviceSolicitationUuids.push_back(uuidFromByte(uuidBytes.data(), uuidLen));
    dataLength -= uuidLength;
    currentPos += uuidLength;
  }
  return currentPos;
}

std::vector<uint8_t> ScanRecord::extractBytes(std::vector<uint8_t> scanRecord, int start, int length)
{
  std::vector<uint8_t> bytes(length);
  /*length+srcPos is less than src.length, the length of the source array.*/
  ALOGE(LOGTAG "start %d length %d scanRecord %d", start, length,scanRecord.size());
  if ((length > 0) && (length+start < scanRecord.size())) {
    bytes.assign(scanRecord.begin()+start, scanRecord.begin()+start+length);
  }
  return bytes;
}
}//namespace gatt
