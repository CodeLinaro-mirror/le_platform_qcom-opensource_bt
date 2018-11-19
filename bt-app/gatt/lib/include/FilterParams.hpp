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

#ifndef FILTER_PARAMS_HPP_
#define FILTER_PARAMS_HPP_
#pragma once

namespace gatt {
class FilterParams {
  private:
    int mClientIf;
    int mFiltIndex;
    int mFeatSeln;
    int mListLogicType;
    int mFiltLogicType;
    int mRssiHighValue;
    int mRssiLowValue;
    int mDelyMode;
    int mFoundTimeOut;
    int mLostTimeOut;
    int mFoundTimeOutCnt;
    int mNumOfTrackEntries;

  public:
    FilterParams(int clientIf, int filtIndex, int featSeln, int listLogicType,
            int filtLogicType, int rssiHighThres, int rssiLowThres, int delyMode, int foundTimeout,
            int lostTimeout, int foundTimeoutCnt, int numOfTrackingEntries);
    int getClientIf();
    int getFiltIndex();
    int getFeatSeln();
    int getDelyMode();
    int getListLogicType();
    int getFiltLogicType();
    int getRSSIHighValue();
    int getRSSILowValue();
    int getFoundTimeout();
    int getFoundTimeOutCnt();
    int getLostTimeout();
    int getNumOfTrackEntries();
};
}
#endif