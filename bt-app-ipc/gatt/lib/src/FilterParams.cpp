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

#include "FilterParams.hpp"

namespace gatt {

FilterParams::FilterParams(int clientIf, int filtIndex, int featSeln, int listLogicType,
        int filtLogicType, int rssiHighThres, int rssiLowThres, int delyMode, int foundTimeout,
        int lostTimeout, int foundTimeoutCnt, int numOfTrackingEntries)
{

  mClientIf = clientIf;
  mFiltIndex = filtIndex;
  mFeatSeln = featSeln;
  mListLogicType = listLogicType;
  mFiltLogicType = filtLogicType;
  mRssiHighValue = rssiHighThres;
  mRssiLowValue = rssiLowThres;
  mDelyMode = delyMode;
  mFoundTimeOut = foundTimeout;
  mLostTimeOut = lostTimeout;
  mFoundTimeOutCnt = foundTimeoutCnt;
  mNumOfTrackEntries = numOfTrackingEntries;
}

int FilterParams::getClientIf()
{
  return mClientIf;
}

int FilterParams::getFiltIndex()
{
  return mFiltIndex;
}

int FilterParams::getFeatSeln()
{
  return mFeatSeln;
}

int FilterParams::getDelyMode()
{
  return mDelyMode;
}

int FilterParams::getListLogicType()
{
  return mListLogicType;
}

int FilterParams::getFiltLogicType()
{
  return mFiltLogicType;
}

int FilterParams::getRSSIHighValue()
{
  return mRssiHighValue;
}

int FilterParams::getRSSILowValue()
{
  return mRssiLowValue;
}

int FilterParams::getFoundTimeout()
{
  return mFoundTimeOut;
}

int FilterParams::getFoundTimeOutCnt()
{
  return mFoundTimeOutCnt;
}

int FilterParams::getLostTimeout()
{
  return mLostTimeOut;
}

int FilterParams::getNumOfTrackEntries()
{
  return mNumOfTrackEntries;
}

}
