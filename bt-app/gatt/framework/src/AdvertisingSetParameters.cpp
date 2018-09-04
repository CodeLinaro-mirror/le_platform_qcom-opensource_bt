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

#include "AdvertisingSetParameters.hpp"
#include <stdexcept>
#include <iostream>

using namespace std;
namespace gatt {

AdvertisingSetParameters::AdvertisingSetParameters(bool connectable, bool scannable,
                                                           bool isLegacy, bool isAnonymous,
                                                           bool includeTxPower, int primaryPhy,
                                                           int secondaryPhy, int interval,
                                                           int txPowerLevel):
mConnectable(connectable),
mScannable(scannable),
mIsLegacy(isLegacy),
mIsAnonymous(isAnonymous),
mIncludeTxPower(includeTxPower),
mPrimaryPhy(primaryPhy),
mSecondaryPhy(secondaryPhy),
mInterval(interval),
mTxPowerLevel(txPowerLevel)
{}

bool AdvertisingSetParameters::isConnectable()
{
  return mConnectable;
}

bool AdvertisingSetParameters::isScannable()
{
  return mScannable;
}

bool AdvertisingSetParameters::isLegacy()
{
  return mIsLegacy;
}

bool AdvertisingSetParameters::isAnonymous()
{
  return mIsAnonymous;
}

bool AdvertisingSetParameters::includeTxPower()
{
  return mIncludeTxPower;
}

int AdvertisingSetParameters::getPrimaryPhy()
{
  return mPrimaryPhy;
}

int AdvertisingSetParameters::getSecondaryPhy()
{
  return mSecondaryPhy;
}

int AdvertisingSetParameters::getInterval()
{
  return mInterval;
}

int AdvertisingSetParameters::getTxPowerLevel()
{
  return mTxPowerLevel;
}

AdvertisingSetParameters::Builder AdvertisingSetParameters::Builder::setConnectable
                                                                          (bool connectable)
{
  mConnectable = connectable;
  return *this;
}

AdvertisingSetParameters::Builder AdvertisingSetParameters::Builder::setScannable
                                                                         (bool scannable)
{
  mScannable = scannable;
  return *this;
}

AdvertisingSetParameters::Builder AdvertisingSetParameters::Builder::setLegacyMode(bool isLegacy)
{
  mIsLegacy = isLegacy;
  return *this;
}

AdvertisingSetParameters::Builder AdvertisingSetParameters::Builder::setAnonymous(bool isAnonymous)
{
  mIsAnonymous = isAnonymous;
  return *this;
}

AdvertisingSetParameters::Builder AdvertisingSetParameters::Builder::setIncludeTxPower
                                                                              (bool includeTxPower)
{
  mIncludeTxPower = includeTxPower;
  return *this;
}

AdvertisingSetParameters::Builder AdvertisingSetParameters::Builder::setPrimaryPhy(int primaryPhy)
{
  if (primaryPhy != PHY_LE_1M && primaryPhy != PHY_LE_CODED) {
    throw std::invalid_argument("bad primaryPhy " + primaryPhy);
  }

  mPrimaryPhy = primaryPhy;
  return *this;
}

AdvertisingSetParameters::Builder AdvertisingSetParameters::Builder::setSecondaryPhy
                                                                              (int secondaryPhy)
{
  if (secondaryPhy != PHY_LE_1M && secondaryPhy != PHY_LE_2M && secondaryPhy != PHY_LE_CODED) {
     throw std::invalid_argument("bad secondaryPhy " + secondaryPhy);
  }

  mSecondaryPhy = secondaryPhy;
  return *this;
}

AdvertisingSetParameters::Builder AdvertisingSetParameters::Builder::setInterval(int interval)
{
  if (interval < INTERVAL_MIN || interval > INTERVAL_MAX) {
     throw std::invalid_argument("unknown interval " + interval);
  }

  mInterval = interval;
  return *this;
}

AdvertisingSetParameters::Builder AdvertisingSetParameters::Builder::setTxPowerLevel
                                                                            (int txPowerLevel)
{
  if (txPowerLevel < TX_POWER_MIN || txPowerLevel > TX_POWER_MAX) {
    throw std::invalid_argument("unknown txPowerLevel " + txPowerLevel);
  }

  mTxPowerLevel = txPowerLevel;
  return *this;
}

AdvertisingSetParameters* AdvertisingSetParameters::Builder::build()
{
  if (mIsLegacy) {
    if (mIsAnonymous) {
      throw std::invalid_argument("Legacy advertising can't be anonymous");
    }

    if (mConnectable && !mScannable) {
      throw std::invalid_argument("Legacy advertisement can't be connectable and non-scannable");
    }

    if (mIncludeTxPower) {
      throw std::invalid_argument("Legacy advertising can't include TX power level in header");
    }
  } else {
      if (mConnectable && mScannable) {
        throw std::invalid_argument("Advertising can't be both connectable and scannable");
      }

      if (mIsAnonymous && mConnectable) {
        throw std::invalid_argument("Advertising can't be both connectable and anonymous");
      }
  }

  return new AdvertisingSetParameters(mConnectable, mScannable, mIsLegacy, mIsAnonymous,
     mIncludeTxPower, mPrimaryPhy, mSecondaryPhy, mInterval, mTxPowerLevel);
}
}//namespace gatt
