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

#ifndef I_SERVER_CALLBACK_HPP_
#define I_SERVER_CALLBACK_HPP_
#pragma once

using namespace std;
using std::string;

namespace gatt {
 /**
  *@hide
  */
class IServerCallback {
  public:
    virtual ~IServerCallback() = default;
    virtual void onServerRegistered(int status, int serverIf) {}
    virtual void onConnectionState(int status, int serverIf,
                                     bool connected, string address){}
    virtual void onServiceAdded(int status, GattService *service){}
    virtual void onCharacteristicReadRequest(string address, int transId, int offset,
                                         bool isLong, int handle){}
    virtual void onDescriptorReadRequest(string address, int transId,
                                        int offset, bool isLong,
                                        int handle){}
    virtual void onCharacteristicWriteRequest(string address, int transId, int offset,
                                      int length, bool isPrep, bool needRsp,
                                      int handle, uint8_t *value){}
    virtual void onDescriptorWriteRequest(string address, int transId, int offset,
                                       int length, bool isPrep, bool needRsp,
                                        int handle, uint8_t *value){}
    virtual void onExecuteWrite(string address, int transId, bool execWrite){}
    virtual void onNotificationSent(string address, int status){}
    virtual void onMtuChanged(string address, int mtu){}
    virtual void onPhyUpdate(string address, int txPhy, int rxPhy, int status){}
    virtual void onPhyRead(string address, int txPhy, int rxPhy, int status){}
    virtual void onConnectionUpdated(string address, int interval, int latency,
                                int timeout, int status){}

};
}
#endif
