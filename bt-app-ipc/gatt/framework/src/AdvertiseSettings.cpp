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

#include "AdvertiseSettings.hpp"

#include <stdexcept>
#include <iostream>

using namespace std;
namespace gatt {

AdvertiseSettings::AdvertiseSettings(int advertiseMode, int advertiseTxPowerLevel,
                                          bool advertiseConnectable, int advertiseTimeout):
  mAdvertiseMode(advertiseMode),
  mAdvertiseTxPowerLevel(advertiseTxPowerLevel),
  mAdvertiseTimeoutMillis(advertiseTimeout),
  mAdvertiseConnectable(advertiseConnectable)
{}

int AdvertiseSettings::getMode()
{
  return mAdvertiseMode;
}

int AdvertiseSettings::getTxPowerLevel()
{
  return mAdvertiseTxPowerLevel;
}

bool AdvertiseSettings::isConnectable()
{
  return mAdvertiseConnectable;
}

int AdvertiseSettings::getTimeout()
{
  return mAdvertiseTimeoutMillis;
}

AdvertiseSettings::Builder AdvertiseSettings::Builder::setAdvertiseMode(int advertiseMode)
{
  if (advertiseMode < ADVERTISE_MODE_LOW_POWER || advertiseMode > ADVERTISE_MODE_LOW_LATENCY) {
    throw std::invalid_argument("unknown mode " + advertiseMode);
  }

  mMode = advertiseMode;
  return *this;
}

AdvertiseSettings::Builder AdvertiseSettings::Builder::setTxPowerLevel(int txPowerLevel)
{
  if (txPowerLevel < ADVERTISE_TX_POWER_ULTRA_LOW || txPowerLevel > ADVERTISE_TX_POWER_HIGH) {
    throw std::invalid_argument("unknown tx power level " + txPowerLevel);
  }

  mTxPowerLevel = txPowerLevel;
  return *this;
}

AdvertiseSettings::Builder AdvertiseSettings::Builder::setConnectable(bool connectable)
{
    mConnectable = connectable;
    return *this;
}

AdvertiseSettings::Builder AdvertiseSettings::Builder::setTimeout(int timeoutMillis)
{
  if (timeoutMillis < 0 || timeoutMillis > LIMITED_ADVERTISING_MAX_MILLIS) {
    throw std::invalid_argument("timeoutMillis invalid (must be 0-"
                  +std::to_string(LIMITED_ADVERTISING_MAX_MILLIS)+" milliseconds");
  }

  mTimeoutMillis = timeoutMillis;
  return *this;
}

AdvertiseSettings* AdvertiseSettings::Builder::build()
{
  return new AdvertiseSettings(mMode, mTxPowerLevel, mConnectable, mTimeoutMillis);
}
}//namespace gatt
