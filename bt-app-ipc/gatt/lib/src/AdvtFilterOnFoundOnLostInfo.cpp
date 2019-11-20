/*
 * Copyright (c) 2018, The Linux Foundation. All rights reserved.
 * Not a Contribution.
 * Copyright (C) 2015 The Android Open Source Project
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

#include "AdvtFilterOnFoundOnLostInfo.hpp"

using namespace std;
namespace gatt {

AdvtFilterOnFoundOnLostInfo::AdvtFilterOnFoundOnLostInfo(int clientIf, int advPktLen,
                                std::vector<uint8_t> advPkt, int scanRspLen,
                                std::vector<uint8_t> scanRsp, int filtIndex,
                                int advState, int advInfoPresent, string address, int addrType,
                                int txPower, int rssiValue, int timeStamp)
{
  mClientIf = clientIf;
  mAdvPktLen = advPktLen;
  mAdvPkt = advPkt;
  mScanRspLen = scanRspLen;
  mScanRsp = scanRsp;
  mFiltIndex = filtIndex;
  mAdvState = advState;
  mAdvInfoPresent = advInfoPresent;
  mAddress = address;
  mAddrType = addrType;
  mTxPower = txPower;
  mRssiValue = rssiValue;
  mTimeStamp = timeStamp;
}

int AdvtFilterOnFoundOnLostInfo::getClientIf()
{
  return mClientIf;
}

int AdvtFilterOnFoundOnLostInfo::getFiltIndex()
{
  return mFiltIndex;
}

int AdvtFilterOnFoundOnLostInfo::getAdvState()
{
  return mAdvState;
}

int AdvtFilterOnFoundOnLostInfo::getTxPower()
{
  return mTxPower;
}

int AdvtFilterOnFoundOnLostInfo::getTimeStamp()
{
  return mTimeStamp;
}

int AdvtFilterOnFoundOnLostInfo::getRSSIValue()
{
  return mRssiValue;
}

int AdvtFilterOnFoundOnLostInfo::getAdvInfoPresent()
{
  return mAdvInfoPresent;
}

string AdvtFilterOnFoundOnLostInfo::getAddress()
{
  return mAddress;
}

int AdvtFilterOnFoundOnLostInfo::getAddressType()
{
  return mAddrType;
}

std::vector<uint8_t> AdvtFilterOnFoundOnLostInfo::getAdvPacketData()
{
  return mAdvPkt;
}

int AdvtFilterOnFoundOnLostInfo::getAdvPacketLen()
{
  return mAdvPktLen;
}

std::vector<uint8_t> AdvtFilterOnFoundOnLostInfo::getScanRspData()
{
  return mScanRsp;
}

int AdvtFilterOnFoundOnLostInfo::getScanRspLen()
{
  return mScanRspLen;
}

std::vector<uint8_t> AdvtFilterOnFoundOnLostInfo::getResult()
{
  int resultLength = mAdvPktLen + (mScanRsp.empty() ? 0 : mScanRspLen);
  std::vector<uint8_t> result(resultLength);

  if ((!mAdvPkt.empty()))
      result.insert(result.begin(), mAdvPkt.begin(), mAdvPkt.end());
  if ( !mScanRsp.empty()) {
      result.insert(result.begin()+mAdvPktLen, mScanRsp.begin(), mScanRsp.end());
  }

  return result;
}
}
