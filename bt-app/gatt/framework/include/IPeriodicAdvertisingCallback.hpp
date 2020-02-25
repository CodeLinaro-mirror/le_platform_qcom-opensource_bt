/*
 * Copyright (c) 2018, The Linux Foundation. All rights reserved.
 * Not a Contribution.
 * Copyright (C) 2017 The Android Open Source Project
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

#ifndef I_PERIODIC_ADV_CALLBACK_HPP_
#define I_PERIODIC_ADV_CALLBACK_HPP_

#pragma once

#include <iostream>
using namespace std;
using std::string;

namespace gatt{
  /**
  *@hide
  */
class PeriodicAdvertisingReport;
class IPeriodicAdvertisingCallback {
  public:
    virtual ~IPeriodicAdvertisingCallback() = default;
    virtual void onSyncEstablished(int syncHandle, string device,
            int advertisingSid, int skip, int timeout,
            int status) {}

    virtual void onPeriodicAdvertisingReport(PeriodicAdvertisingReport *report) {}

    virtual void onSyncLost(int syncHandle) {}
};
}
#endif
