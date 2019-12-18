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

#include "PeriodicAdvertisingReport.hpp"

namespace gatt {

PeriodicAdvertisingReport::PeriodicAdvertisingReport(int syncHandle, int txPower, int rssi,
      int dataStatus, ScanRecord *data)
{
  this->mSyncHandle = syncHandle;
  this->mTxPower = txPower;
  this->mRssi = rssi;
  this->mDataStatus = dataStatus;
  this->mData = data;
}

PeriodicAdvertisingReport::~PeriodicAdvertisingReport() {}

int PeriodicAdvertisingReport::getSyncHandle()
{
  return mSyncHandle;
}

int PeriodicAdvertisingReport::getTxPower()
{
  return mTxPower;
}

int PeriodicAdvertisingReport::getRssi()
{
  return mRssi;
}

int PeriodicAdvertisingReport::getDataStatus()
{
  return mDataStatus;
}

ScanRecord* PeriodicAdvertisingReport::getData()
{
  return mData;
}
}
