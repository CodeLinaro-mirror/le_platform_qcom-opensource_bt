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

#include <iostream>
#include "GattDescriptor.hpp"

namespace gatt{

const uint8_t GattDescriptor::ENABLE_NOTIFICATION_VALUE[2] = {0x01, 0x00};
const uint8_t GattDescriptor::ENABLE_INDICATION_VALUE[2] = {0x02, 0x00};
const uint8_t GattDescriptor::DISABLE_NOTIFICATION_VALUE[2] = {0x00, 0x00};

GattDescriptor::~GattDescriptor()
{
  if ( mCharacteristic != NULL ) {
    mCharacteristic = NULL;
  }

  if (mValue != NULL) {
    delete[] mValue;
    mValue = NULL;
  }
}

GattDescriptor::GattDescriptor(Uuid uuid, int permissions)
{
  initDescriptor(NULL, uuid, 0, permissions);
}

GattDescriptor::GattDescriptor(Uuid uuid, int instance, int permissions)
{
  initDescriptor(NULL, uuid, instance, permissions);
}

void GattDescriptor::initDescriptor(GattCharacteristic *characteristic, Uuid uuid,
           int instance, int permissions)
{
  mCharacteristic = characteristic;
  mUuid = uuid;
  mInstance = instance;
  mPermissions = permissions;
}

GattCharacteristic* GattDescriptor::getCharacteristic()
{
  return mCharacteristic;
}

void GattDescriptor::setCharacteristic(GattCharacteristic *characteristic)
{
  mCharacteristic = characteristic;
}

Uuid GattDescriptor::getUuid()
{
  return mUuid;
}

int GattDescriptor::getInstanceId()
{
  return mInstance;
}

void GattDescriptor::setInstanceId(int instanceId)
{
  mInstance = instanceId;
}

int GattDescriptor::getPermissions()
{
  return mPermissions;
}

uint8_t* GattDescriptor::getValue()
{
  return mValue;
}

int GattDescriptor::getValueLength()
{
  return mValueLength;
}

bool GattDescriptor::setValue(uint8_t *value, int length)
{
  if (value == NULL || length == 0) {
    return false;
  }

  if(mValue != NULL)
    delete [] mValue;

  mValueLength = length;
  mValue = new uint8_t[mValueLength];
  if (!mValue)
    return false;

  std::memcpy(mValue, value, mValueLength);

  return true;
}
}
