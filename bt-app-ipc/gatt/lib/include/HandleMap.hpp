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

#ifndef HANDLE_MAP_HPP_
#define HANDLE_MAP_HPP_
#pragma once

#include "utils/Log.h"
#include "utils/include/uuid.h"
#include<unordered_map>
#include<vector>
#include<string>

using namespace std;
using std::string;
using btapp::Uuid;
namespace gatt{

class HandleMap {
  public:
    class Entry {
      public:
        int serverIf = 0;
        int type = 0;//TYPE_UNDEFINED;
        int handle = 0;
        Uuid uuid = Uuid::kEmpty;
        int instance = 0;
        int serviceType = 0;
        int serviceHandle = 0;
        int charHandle = 0;
        bool started = false;
        bool advertisePreferred = false;

        Entry(int serverIf, int handle, Uuid uuid, int serviceType, int instance);

        Entry(int serverIf, int handle, Uuid uuid, int serviceType, int instance,
                bool advertisePreferred);

        Entry(int serverIf, int type, int handle, Uuid uuid, int serviceHandle);

        Entry(int serverIf, int type, int handle, Uuid uuid, int serviceHandle, int charHandle);
    };

  private:
    static const bool DBG = false;
    std::vector<Entry*> mEntries;
    std::unordered_map<int, int> mRequestMap;
    int mLastCharacteristic = 0;

  public:
    static const int TYPE_UNDEFINED = 0;
    static const int TYPE_SERVICE = 1;
    static const int TYPE_CHARACTERISTIC = 2;
    static const int TYPE_DESCRIPTOR = 3;

    void clear();
    void addService(int serverIf, int handle, Uuid uuid, int serviceType, int instance,
            bool advertisePreferred);
    void addCharacteristic(int serverIf, int handle, Uuid uuid, int serviceHandle);
    void addDescriptor(int serverIf, int handle, Uuid uuid, int serviceHandle);
    void setStarted(int serverIf, int handle, bool started);
    Entry* getByHandle(int handle);
    bool checkServiceExists(Uuid uuid, int handle);
    void deleteService(int serverIf, int serviceHandle);
    std::vector<Entry*> getEntries() ;
    void addRequest(int requestId, int handle);
    void deleteRequest(int requestId);
    Entry* getByRequestId(int requestId);
};
}
#endif
