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

#include "HandleMap.hpp"

#include<map>
#include<list>
#include<string>

#define LOGTAG "HandleMap"

using namespace std;
namespace gatt {

HandleMap::Entry::Entry(int serverIf, int handle, Uuid uuid, int serviceType, int instance)
{
  this->serverIf = serverIf;
  this->type = TYPE_SERVICE;
  this->handle = handle;
  this->uuid = uuid;
  this->instance = instance;
  this->serviceType = serviceType;
}

HandleMap::Entry::Entry(int serverIf, int handle, Uuid uuid, int serviceType, int instance,
        bool advertisePreferred)
{
  this->serverIf = serverIf;
  this->type = TYPE_SERVICE;
  this->handle = handle;
  this->uuid = uuid;
  this->instance = instance;
  this->serviceType = serviceType;
  this->advertisePreferred = advertisePreferred;
}

HandleMap::Entry::Entry(int serverIf, int type, int handle, Uuid uuid, int serviceHandle)
{
  this->serverIf = serverIf;
  this->type = type;
  this->handle = handle;
  this->uuid = uuid;
  this->serviceHandle = serviceHandle;
}

HandleMap::Entry::Entry(int serverIf, int type, int handle, Uuid uuid, int serviceHandle, int charHandle)
{
  this->serverIf = serverIf;
  this->type = type;
  this->handle = handle;
  this->uuid = uuid;
  this->serviceHandle = serviceHandle;
  this->charHandle = charHandle;
}

void  HandleMap::clear()
{
  mEntries.clear();
  mRequestMap.clear();
}

void  HandleMap::addService(int serverIf, int handle, Uuid uuid, int serviceType, int instance,
                                bool advertisePreferred)
{
  mEntries.push_back(new Entry(serverIf, handle, uuid, serviceType, instance, advertisePreferred));
}

void  HandleMap::addCharacteristic(int serverIf, int handle, Uuid uuid, int serviceHandle)
{
  mLastCharacteristic = handle;
  mEntries.push_back(new Entry(serverIf, TYPE_CHARACTERISTIC, handle, uuid, serviceHandle));
}

void  HandleMap::addDescriptor(int serverIf, int handle, Uuid uuid, int serviceHandle)
{
  mEntries.push_back(new Entry(serverIf, TYPE_DESCRIPTOR, handle, uuid, serviceHandle,
                       mLastCharacteristic));
}

void  HandleMap::setStarted(int serverIf, int handle, bool started)
{
  for (Entry *entry : mEntries) {
    if (entry->type != TYPE_SERVICE || entry->serverIf != serverIf
            || entry->handle != handle) {
      continue;
    }

    entry->started = started;
    return;
  }
}

HandleMap::Entry* HandleMap::getByHandle(int handle)
{
  for (Entry *entry : mEntries) {
    if (entry->handle == handle) {
      return entry;
    }
  }
  ALOGE(LOGTAG " getByHandle() - Handle %d not found !", handle);
  return NULL;
}

bool HandleMap::checkServiceExists(Uuid uuid, int handle)
{
  for (Entry *entry : mEntries) {
    if (entry->type == TYPE_SERVICE && entry->handle == handle && entry->uuid == uuid) {
      return true;
    }
  }
  return false;
}

void HandleMap::deleteService(int serverIf, int serviceHandle)
{
  for (std::vector<Entry*>::iterator it = mEntries.begin(); it != mEntries.end(); ++it) {
    Entry *entry = *it;
    if (entry->serverIf != serverIf) {
      continue;
    }

    if (entry->handle == serviceHandle || entry->serviceHandle == serviceHandle) {
      mEntries.erase(it);
      break;
    }
  }
}

std::vector<HandleMap::Entry*> HandleMap::getEntries()
{
  return mEntries;
}

void HandleMap::addRequest(int requestId, int handle)
{
  mRequestMap.insert({{requestId, handle}});
}

void HandleMap::deleteRequest(int requestId)
{
  for ( std::unordered_map<int, int>::iterator it = mRequestMap.begin() ;
              it != mRequestMap.end(); ++it) {
    if( it->first == requestId) {
      mRequestMap.erase(it);
    }
  }
}

HandleMap::Entry* HandleMap::getByRequestId(int requestId)
{
  auto handle = mRequestMap.find(requestId);
  if (handle == mRequestMap.end()) {
    ALOGE(LOGTAG " getByRequestId() - Request ID %d not found !", requestId);
    return NULL;
  }
  return getByHandle(handle->second);
}

}
