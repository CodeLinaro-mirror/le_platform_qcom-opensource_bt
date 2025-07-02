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

#ifndef ADVERTISE_HELPER_HPP_
#define ADVERTISE_HELPER_HPP_

#pragma once

#include "AdvertiseData.hpp"
#include "osi/include/log.h"
#include <string>

using namespace std;
using std::string;

namespace gatt {
class AdvertiseHelper {
  private:
    static const int DEVICE_NAME_MAX = 26;
    static const int COMPLETE_LIST_16_BIT_SERVICE_UUIDS = 0X03;
    static const int COMPLETE_LIST_32_BIT_SERVICE_UUIDS = 0X05;
    static const int COMPLETE_LIST_128_BIT_SERVICE_UUIDS = 0X07;
    static const int SHORTENED_LOCAL_NAME = 0X08;
    static const int COMPLETE_LOCAL_NAME = 0X09;
    static const int TX_POWER_LEVEL = 0x0A;
    static const int SERVICE_DATA_16_BIT_UUID = 0X16;
    static const int SERVICE_DATA_32_BIT_UUID = 0X20;
    static const int SERVICE_DATA_128_BIT_UUID = 0X21;
    static const int MANUFACTURER_SPECIFIC_DATA = 0XFF;

  public:
    static std::vector<uint8_t> advertiseDataToBytes(AdvertiseData *data, string name);
};
}
#endif
