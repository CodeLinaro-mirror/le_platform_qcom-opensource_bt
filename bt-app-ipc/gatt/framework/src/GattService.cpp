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

#include "GattService.hpp"
#include<iostream>
#include<list>

using namespace std;

namespace gatt {

GattService::GattService(Uuid uuid, int serviceType)
{
  mDevice = "";
  mUuid = uuid;
  mInstanceId = 0;
  mServiceType = serviceType;
}

GattService::GattService(Uuid uuid, int instanceId, int serviceType)
{
  mDevice = "";
  mUuid = uuid;
  mInstanceId = instanceId;
  mServiceType = serviceType;
}

GattService::~GattService()
{
  for (GattCharacteristic *svc_char : mCharacteristics)
      delete svc_char;
  mCharacteristics.clear();

  for (GattService *svc_include : mIncludedServices)
      delete svc_include;
  mIncludedServices.clear();
}

string GattService::getDevice()
{
   return mDevice;
}

void GattService::setDevice(string device)
{
  mDevice = device;
}

bool GattService::addService(GattService *service)
{
  mIncludedServices.push_back(service);
  return true;
}

bool GattService::addCharacteristic(GattCharacteristic *characteristic)
{
  mCharacteristics.push_back(characteristic);
  characteristic->setService(this);
  return true;
}

GattCharacteristic* GattService::getCharacteristic(Uuid uuid, int instanceId)
{
  for (GattCharacteristic *characteristic : mCharacteristics) {
    if (uuid == characteristic->getUuid()
            && characteristic->getInstanceId() == instanceId) {
      return characteristic;
    }
  }
  return NULL;
}

void GattService::setInstanceId(int instanceId)
{
  mInstanceId = instanceId;
}

void GattService::addIncludedService(GattService *includedService)
{
  mIncludedServices.push_back(includedService);
}

Uuid GattService::getUuid()
{
  return mUuid;
}

int GattService::getInstanceId()
{
  return mInstanceId;
}

int GattService::getType()
{
  return mServiceType;
}

std::vector<GattService*> GattService::getIncludedServices()
{
  return mIncludedServices;
}

std::vector<GattCharacteristic*> GattService::getCharacteristics()
{
  return mCharacteristics;
}

GattCharacteristic* GattService::getCharacteristic(Uuid uuid)
{
  for (GattCharacteristic *characteristic : mCharacteristics) {
     if (uuid == characteristic->getUuid()) {
       return characteristic;
     }
   }
   return NULL;
}

bool GattService::isAdvertisePreferred()
{
  return mAdvertisePreferred;
}

void GattService::setAdvertisePreferred(bool advertisePreferred)
{
  mAdvertisePreferred = advertisePreferred;
}
}
