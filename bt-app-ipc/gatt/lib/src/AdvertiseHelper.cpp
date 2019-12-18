/*
* Copyright (c) 2018, The Linux Foundation. All rights reserved.
* Not a Contribution.
* Copyright (C) 2016 The Android Open Source Project
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

#include "AdvertiseHelper.hpp"

#include <sstream>
#include <iostream>
#include <cstring>
#include <map>
#include <exception>

#define LOGTAG "AdvertiseHelper"

using namespace std;
namespace gatt {

std::vector<uint8_t> AdvertiseHelper::advertiseDataToBytes(AdvertiseData *data, string name)
{
  if (data == NULL) {
    return {};
  }

  // Flags are added by lower layers of the stack, only if needed;
  // no need to add them here.
  std::vector<uint8_t> vec;

  if (data->getIncludeDeviceName()) {
    try {
      const uint8_t *nameBytes = reinterpret_cast<const uint8_t*>(name.c_str());

      int nameLength = static_cast<int>(strlen((char*)nameBytes));
      uint8_t type;

      if (nameLength > DEVICE_NAME_MAX) {
        nameLength = DEVICE_NAME_MAX;
        type = SHORTENED_LOCAL_NAME;
      } else {
          type = COMPLETE_LOCAL_NAME;
      }

      vec.push_back(nameLength + 1);
      vec.push_back(type);
      for (int i = 0; i < nameLength; ++i)
        vec.push_back(nameBytes[i]);
    } catch (std::exception& e) {
        ALOGE(LOGTAG " Can't include name - encoding error! %s", e.what());
    }
  }

  if (!data->getManufacturerSpecificData().empty()) {
    std::map<int, std::vector<uint8_t> > manufacturerSpecificDataMap =
                                                            data->getManufacturerSpecificData();
    for (std::map<int, std::vector<uint8_t>>::iterator it = manufacturerSpecificDataMap.begin();
                    it != manufacturerSpecificDataMap.end(); ++it) {
      int manufacturerId = it->first;
      std::vector<uint8_t> manufacturerData = it->second;
      int manufacturerDataLen = static_cast<int>(manufacturerData.size());

      int dataLen = 2 + (manufacturerData.empty() ? 0 : manufacturerDataLen);

      std::vector<uint8_t> concated(dataLen);
      // First two bytes are manufacturer id in little-endian.
      concated[0] =(uint8_t) ((manufacturerId & 0xFF));
      concated[1] = (uint8_t)(((manufacturerId >> 8) & 0xFF));

      if (!manufacturerData.empty()) {
          for (int i = 2;i < manufacturerDataLen+2; ++i)
            concated[i] = manufacturerData[i-2];
          string tmmp(&concated[2],&concated[manufacturerDataLen+2]);
      }

      int concateLen = static_cast<int>(concated.size());

      vec.push_back(concateLen+ 1);
      vec.push_back(MANUFACTURER_SPECIFIC_DATA);
      for (int i = 0 ;i < concateLen; ++i)
          vec.push_back(concated[i]);
    }
  }
  if (data->getIncludeTxPowerLevel()) {
    vec.push_back(2);/* Length */;
    vec.push_back(TX_POWER_LEVEL);
    vec.push_back(0); // lower layers will fill this value.
  }

  if (!(data->getServiceUuids().empty())) {
    const int noOfUuids = data->getServiceUuids().size();

    std::vector< std::vector <uint8_t> >serviceUuids16
                                        (noOfUuids, std::vector<uint8_t>(Uuid::kNumBytes16));
    std::vector< std::vector <uint8_t> >serviceUuids32
                                        (noOfUuids, std::vector<uint8_t>(Uuid::kNumBytes32));
    std::vector< std::vector <uint8_t> >serviceUuids128
                                        (noOfUuids, std::vector<uint8_t>(Uuid::kNumBytes128));

    int i16 = 0, i32 = 0, i128 = 0;
    for (Uuid pUuid : data->getServiceUuids()) {
      std::vector<uint8_t> uu = Uuid::uuidToByte(pUuid);

      int uuidLen = uu.size();
      if (uuidLen == Uuid::kNumBytes16) {
         for (int i = 0; i< uuidLen; ++i) {
            serviceUuids16[i16][i] = (uu[i]);
          }
         ++i16;
      } else if (uuidLen == Uuid::kNumBytes32) {
          for (int i = 0; i< uuidLen; ++i) {
            serviceUuids32[i32][i] = (uu[i]);
          }
          ++i32;
      } else /*Uuid::kNumBytes128*/ {
          for (int i = 0; i< uuidLen; ++i) {
            serviceUuids128[i128][i] = (uu[i]);
            }
          ++i128;
      }
    }

    if (i16 != 0) {
      vec.push_back((int)(Uuid::kNumBytes16 * (i16)) + 1);
       vec.push_back(COMPLETE_LIST_16_BIT_SERVICE_UUIDS);
      for (int i =0; i < i16; i++) {
        for (int j = 0; j < Uuid::kNumBytes16; ++j) {
          vec.push_back(serviceUuids16[i][j]);
          }
      }
    }

    if (i32 != 0) {
     vec.push_back((Uuid::kNumBytes32 * (i32)) + 1);
     vec.push_back(COMPLETE_LIST_32_BIT_SERVICE_UUIDS);
      for (int i = 0; i < i32; i++){
        for (int j = 0; j < Uuid::kNumBytes32; ++j) {
          vec.push_back(serviceUuids32[i][j]);
          }
      }
    }

    if (i128 != 0) {
      vec.push_back((Uuid::kNumBytes128 * (i128)) + 1);
      vec.push_back(COMPLETE_LIST_128_BIT_SERVICE_UUIDS);
      for (int i = 0; i < i128; i++){
        for (int j = 0; j < Uuid::kNumBytes128; ++j)
          vec.push_back(serviceUuids128[i][j]);
      }
    }
  }

  if (!data->getServiceData().empty()) {
    std::map<Uuid, std::vector<uint8_t>> serviceDataMap = data->getServiceData();
    for(std::map<Uuid, std::vector<uint8_t>>::iterator it = serviceDataMap.begin();
                                            it != serviceDataMap.end(); ++it) {
      Uuid pUuid = it->first;
      std::vector<uint8_t> serviceData = it->second;
      int serviceDataLen = static_cast<int>(serviceData.size());

      std::vector<uint8_t> uu = Uuid::uuidToByte(pUuid);
      int uuidLen = uu.size();

      int dataLen = uuidLen + (serviceData.empty() ? 0 : serviceDataLen);
      std::vector<uint8_t> concated(dataLen);
      for (int i =0;i<uuidLen;i++)
        concated[i] = uu[i];

      if (!serviceData.empty()) {
        for (int i =uuidLen; i < serviceDataLen+uuidLen; ++i) {
          concated[i] = serviceData[i-uuidLen];
        }
      }

      int concatedLen = static_cast<int>(concated.size());
      if (uuidLen == Uuid::kNumBytes16) {
        vec.push_back(concatedLen+1);
        vec.push_back(SERVICE_DATA_16_BIT_UUID);
        for (int i = 0; i < concatedLen; ++i)
          vec.push_back(concated[i]);

      } else if (uuidLen == Uuid::kNumBytes32) {
        vec.push_back(concatedLen+1);
        vec.push_back(SERVICE_DATA_32_BIT_UUID);
        for (int i = 0; i < concatedLen; ++i)
          vec.push_back(concated[i]);

      } else /*Uuid::kNumBytes128*/ {
        vec.push_back(concatedLen+1);
        vec.push_back(SERVICE_DATA_128_BIT_UUID);
        for (int i = 0; i < concatedLen; ++i)
          vec.push_back(concated[i]);
      }
    }
  }

  return vec;
}
}
