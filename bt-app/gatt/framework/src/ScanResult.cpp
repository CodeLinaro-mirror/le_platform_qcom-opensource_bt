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

#include "ScanResult.hpp"

using namespace std;
namespace gatt{

ScanResult::ScanResult(string deviceAddress, int eventType, int primaryPhy, int secondaryPhy,
        int advertisingSid, int txPower, int rssi, int periodicAdvertisingInterval,
        ScanRecord *scanRecord)
{
  mDeviceAddress = deviceAddress;
  mEventType = eventType;
  mPrimaryPhy = primaryPhy;
  mSecondaryPhy = secondaryPhy;
  mAdvertisingSid = advertisingSid;
  mTxPower = txPower;
  mRssi = rssi;
  mPeriodicAdvertisingInterval = periodicAdvertisingInterval;
  mScanRecord = scanRecord;
}

ScanResult::ScanResult(string deviceAddress, ScanRecord *scanRecord, int rssi)
{
  mDeviceAddress = deviceAddress;
  mScanRecord = scanRecord;
  mRssi = rssi;
  mEventType = (DATA_COMPLETE << 5) | ET_LEGACY_MASK | ET_CONNECTABLE_MASK;
  mPrimaryPhy = GattDevice::PHY_LE_1M;
  mSecondaryPhy = PHY_UNUSED;
  mAdvertisingSid = SID_NOT_PRESENT;
  mTxPower = 127;
  mPeriodicAdvertisingInterval = 0;
}

ScanResult::~ScanResult()
{
  delete mScanRecord;
  mScanRecord = NULL;
}

string ScanResult::getDevice()
{
  return mDeviceAddress;
}

ScanRecord* ScanResult::getScanRecord()
{
  return mScanRecord;
}

int ScanResult::getRssi()
{
  return mRssi;
}

bool ScanResult::isLegacy()
{
  return (mEventType & ET_LEGACY_MASK) != 0;
}

bool ScanResult::isConnectable()
{
  return (mEventType & ET_CONNECTABLE_MASK) != 0;
}

int ScanResult::getDataStatus()
{
  // return bit 5 and 6
  return (mEventType >> 5) & 0x03;
}

int ScanResult::getPrimaryPhy()
{
  return mPrimaryPhy;
}

int ScanResult::getSecondaryPhy()
{
  return mSecondaryPhy;
}

int ScanResult::getAdvertisingSid()
{
  return mAdvertisingSid;
}

int ScanResult::getTxPower()
{
  return mTxPower;
}

int ScanResult::getPeriodicAdvertisingInterval()
{
  return mPeriodicAdvertisingInterval;
}

string ToString(ScanResult *result) {
  return string("none");
}
}//namespace gatt
