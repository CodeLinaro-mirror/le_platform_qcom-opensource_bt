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

#include <iostream>
#include "ScanSettings.hpp"
#include <stdexcept>

namespace gatt{
ScanSettings::ScanSettings(int scanMode, int callbackType, int scanResultType,
                              long reportDelayMillis, int matchMode,
                              int numOfMatchesPerFilter, bool legacy, int phy)
{
  mScanMode = scanMode;
  mCallbackType = callbackType;
  mScanResultType = scanResultType;
  mReportDelayMillis = reportDelayMillis;
  mNumOfMatchesPerFilter = numOfMatchesPerFilter;
  mMatchMode = matchMode;
  mLegacy = legacy;
  mPhy = phy;
}

int ScanSettings::getMatchMode()
{
  return mMatchMode;
}

int ScanSettings::getNumOfMatches()
{
  return mNumOfMatchesPerFilter;
}

int ScanSettings::getScanMode()
{
  return mScanMode;
}

int ScanSettings::getCallbackType()
{
  return mCallbackType;
}

int ScanSettings::getScanResultType()
{
  return mScanResultType;
}

bool ScanSettings::getLegacy()
{
  return mLegacy;
}

int ScanSettings::getPhy()
{
  return mPhy;
}

long ScanSettings::getReportDelayMillis()
{
  return mReportDelayMillis;
}

bool ScanSettings::Builder::isValidCallbackType(int callbackType)
{
  if (callbackType == CALLBACK_TYPE_ALL_MATCHES
    || callbackType == CALLBACK_TYPE_FIRST_MATCH
    || callbackType == CALLBACK_TYPE_MATCH_LOST
    || callbackType == CALLBACK_TYPE_SENSOR_ROUTING) {
    return true;
  }
  return callbackType == (CALLBACK_TYPE_FIRST_MATCH | CALLBACK_TYPE_MATCH_LOST);
}

ScanSettings::Builder ScanSettings::Builder::setScanResultType(int scanResultType)
{
  if (scanResultType < SCAN_RESULT_TYPE_FULL
    || scanResultType > SCAN_RESULT_TYPE_ABBREVIATED) {
    throw std::invalid_argument(
                   "invalid scanResultType - " + scanResultType);
  }
  mScanResultType = scanResultType;
  return *this;
}

ScanSettings::Builder ScanSettings::Builder::setScanMode(int scanMode)
{
  if (scanMode < SCAN_MODE_OPPORTUNISTIC || scanMode > SCAN_MODE_LOW_LATENCY) {
    throw std::invalid_argument("invalid scan mode " + scanMode);
  }
  mScanMode = scanMode;
  return *this;
}

ScanSettings::Builder ScanSettings::Builder::setCallbackType(int callbackType)
{
  if (!isValidCallbackType(callbackType)) {
    throw std::invalid_argument("invalid callback type - " + callbackType);
  }
  mCallbackType = callbackType;
  return *this;
}

ScanSettings::Builder ScanSettings::Builder::setReportDelay(long reportDelayMillis)
{
  if (reportDelayMillis < 0) {
    throw std::invalid_argument("reportDelay must be > 0");
  }
  mReportDelayMillis = reportDelayMillis;
  return *this;
}

ScanSettings::Builder ScanSettings::Builder::setNumOfMatches(int numOfMatches)
{
  if (numOfMatches < MATCH_NUM_ONE_ADVERTISEMENT
    || numOfMatches > MATCH_NUM_MAX_ADVERTISEMENT) {
    throw std::invalid_argument("invalid numOfMatches " + numOfMatches);
  }
  mNumOfMatchesPerFilter = numOfMatches;
  return *this;
}

ScanSettings::Builder ScanSettings::Builder::setMatchMode(int matchMode)
{
  if (matchMode < MATCH_MODE_AGGRESSIVE
    || matchMode > MATCH_MODE_STICKY) {
    throw std::invalid_argument("invalid matchMode " + matchMode);
  }
  mMatchMode = matchMode;
  return *this;
}

ScanSettings::Builder ScanSettings::Builder::setLegacy(bool legacy)
{
  mLegacy = legacy;
  return *this;
}

ScanSettings::Builder ScanSettings::Builder::setPhy(int phy)
{
  mPhy = phy;
  return *this;
}

ScanSettings* ScanSettings::Builder::build()
{
  return new ScanSettings(mScanMode, mCallbackType, mScanResultType,
                         mReportDelayMillis, mMatchMode,
                         mNumOfMatchesPerFilter, mLegacy, mPhy);
}
}//namespace gatt
