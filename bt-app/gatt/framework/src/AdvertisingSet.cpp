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

#include "AdvertisingSet.hpp"
#include <exception>

#define LOGTAG "AdvertisingSet"

using namespace std;
namespace gatt {

AdvertisingSet::~AdvertisingSet()
{
  mGatt = NULL;
}

AdvertisingSet::AdvertisingSet(int advertiserId)
{
  mAdvertiserId = advertiserId;
  mGatt = GattLibService::getGatt();
  if ( mGatt == NULL) {
    ALOGE(LOGTAG " Failed to get Bluetooth gatt");
  }
}

void AdvertisingSet::setAdvertiserId(int advertiserId)
{
  mAdvertiserId = advertiserId;
}

void AdvertisingSet::enableAdvertising(bool enable, int duration,
                                            int maxExtendedAdvertisingEvents)
{
  try {
    mGatt->enableAdvertisingSet(mAdvertiserId, enable, duration,
            maxExtendedAdvertisingEvents);
  } catch (std::exception& e) {
      ALOGE(LOGTAG " remote exception - %s", e.what());
  }
}

void AdvertisingSet::setAdvertisingData(AdvertiseData& advertiseData)
{
  try {
    mGatt->setAdvertisingData(mAdvertiserId, &advertiseData);
  } catch (std::exception& e) {
      ALOGE(LOGTAG " remote exception - %s", e.what());
  }
}

void AdvertisingSet::setScanResponseData(AdvertiseData& scanResponse)
{
  try {
       mGatt->setScanResponseData(mAdvertiserId, &scanResponse);
   } catch (std::exception& e) {
       ALOGE(LOGTAG " remote exception - %s", e.what());
   }
}

void AdvertisingSet::setAdvertisingParameters(AdvertisingSetParameters& parameters)
{
  try {
    mGatt->setAdvertisingParameters(mAdvertiserId, &parameters);
  } catch (std::exception& e) {
      ALOGE(LOGTAG " remote exception - %s", e.what());
  }
}

void AdvertisingSet::setPeriodicAdvertisingParameters(PeriodicAdvertiseParameters& parameters)
{
  try {
    mGatt->setPeriodicAdvertisingParameters(mAdvertiserId, &parameters);
  } catch (std::exception& e) {
      ALOGE(LOGTAG " remote exception - %s", e.what());
  }
}

void AdvertisingSet::setPeriodicAdvertisingData(AdvertiseData& periodicData)
{
  try {
    mGatt->setPeriodicAdvertisingData(mAdvertiserId, &periodicData);
  } catch (std::exception& e) {
      ALOGE(LOGTAG " remote exception - %s", e.what());
  }
}

void AdvertisingSet::setPeriodicAdvertisingEnabled(uint8_t enable)
{
  try {
    mGatt->setPeriodicAdvertisingEnable(mAdvertiserId, enable);
  } catch (std::exception& e) {
      ALOGE(LOGTAG " remote exception - %s", e.what());
  }
}

void AdvertisingSet::getOwnAddress()
{
  try {
    mGatt->getOwnAddress(mAdvertiserId);
  } catch (std::exception& e) {
      ALOGE(LOGTAG " remote exception - %s", e.what());
  }
}

int AdvertisingSet::getAdvertiserId()
{
  return mAdvertiserId;
}
}//namespace gatt
