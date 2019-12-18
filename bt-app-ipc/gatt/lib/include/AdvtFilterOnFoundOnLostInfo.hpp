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

#ifndef ADV_FILTER_ON_FOUND_ON_LOST_INFO_HPP__
#define ADV_FILTER_ON_FOUND_ON_LOST_INFO_HPP__

#pragma once

#include <cstring>
#include <iostream>
#include <vector>

using namespace std;
using std::string;

namespace gatt {

class AdvtFilterOnFoundOnLostInfo {
  private:
    int mClientIf;

    int mAdvPktLen;
    std::vector<uint8_t> mAdvPkt;

    int mScanRspLen;
    std::vector<uint8_t> mScanRsp;

    int mFiltIndex;
    int mAdvState;
    int mAdvInfoPresent;
    string mAddress;
    int mAddrType;
    int mTxPower;
    int mRssiValue;
    int mTimeStamp;

  public:
    AdvtFilterOnFoundOnLostInfo(int clientIf, int advPktLen,
                                        std::vector<uint8_t> advPkt,
                                        int scanRspLen,
                                        std::vector<uint8_t> scanRsp,
                                        int filtIndex, int advState,
                                        int advInfoPresent, string address,
                                        int addrType, int txPower,
                                        int rssiValue, int timeStamp);
    int getClientIf();
    int getFiltIndex();
    int getAdvState();
    int getTxPower();
    int getTimeStamp();
    int getRSSIValue();
    int getAdvInfoPresent();
    string getAddress();
    int getAddressType();
    std::vector<uint8_t> getAdvPacketData();
    int getAdvPacketLen();
    std::vector<uint8_t> getScanRspData();
    int getScanRspLen();
    std::vector<uint8_t> getResult();
};
}
#endif
