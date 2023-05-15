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

#ifndef I_CLIENT_CALLBACK_HPP_
#define I_CLIENT_CALLBACK_HPP_

#pragma once

#include "GattService.hpp"
#include <iostream>
#include <vector>
#include <string>
using namespace std;
using std::string;

namespace gatt{
  /**
  *@hide
  */
class IClientCallback {
  public:
    virtual ~IClientCallback() = default;

    virtual void onClientRegistered(int status, int clientIf){}

    virtual void onConnectionState(int status, int clientIf,
                                     bool connected, string address){}

    virtual void onPhyUpdate(string address, int txPhy, int rxPhy, int status){}

    virtual void onPhyRead(string address, int txPhy, int rxPhy, int status){}

    virtual void onSearchComplete(string address, std::vector<GattService*> services, int status){}

    virtual void onCharacteristicRead(string address, int status, int handle, uint8_t *value, int length){}

    virtual void onCharacteristicWrite(string address, int status, int handle){}

    virtual void onExecuteWrite(string address, int status){}

    virtual void onDescriptorRead(string address, int status, int handle, uint8_t *value, int length){}

    virtual void onDescriptorWrite(string address, int status, int handle){}

    virtual void onNotify(string address, int handle, uint8_t *value, int length){}

    virtual void onReadRemoteRssi(string address, int rssi, int status){}

    virtual void onConfigureMTU(string address, int mtu, int status){}

    virtual void onConnectionUpdated(string address, int interval, int latency,
                                 int timeout, int status){}

    virtual void onServiceChanged(string address){}

};
}
#endif
