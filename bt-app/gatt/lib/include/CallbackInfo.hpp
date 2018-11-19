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
#ifndef CALLBACK_INFO_HPP_
#define CALLBACK_INFO_HPP_
#pragma once

#include<string>

using namespace std;
using std::string;

namespace gatt {

/**
 * Helper class that keeps track of callback parameters for app callbacks.
 * These are held during congestion and reported when congestion clears.
 * @hide
 */
/*package*/

class CallbackInfo {
  public:
    string address;
    int status;
    int handle;

    CallbackInfo(string address, int status, int handle);
    CallbackInfo(string address, int status);
};
}
#endif
