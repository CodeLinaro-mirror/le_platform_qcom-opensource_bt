/*
 * * Copyright (c) 2018, The Linux Foundation. All rights reserved.
 * Not a Contribution.
 * Copyright (C) 2009 The Android Open Source Project
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

#ifndef GATTDEVICE_HPP_
#define GATTDEVICE_HPP_

#pragma once

#include "ipc.hpp"
#include "osi/include/log.h"
#include <string>

using namespace std;
using std::string;

/**
* This class provide common used option types.
*/
namespace gatt{
class GattDevice {
  private:

    static const bool DBG = true;
    static const int MIN_ADVT_INSTANCES_FOR_MA = 5;
    static const int MIN_OFFLOADED_FILTERS = 10;
    static const int MIN_OFFLOADED_SCAN_STORAGE_BYTES = 1024;

  public:
    /**
     * No preferrence of physical transport for GATT connections to remote dual-mode devices
     */
    static const int TRANSPORT_AUTO = 0;

    /**
     * Prefer BR/EDR transport for GATT connections to remote dual-mode devices
     */
    static const int TRANSPORT_BREDR = 1;

    /**
     * Prefer LE transport for GATT connections to remote dual-mode devices
     */
    static const int TRANSPORT_LE = 2;

    /**
     * Bluetooth LE 1M PHY. Used to refer to LE 1M Physical Channel for advertising, scanning or
     * connection.
     */
    static const int PHY_LE_1M = 1;

    /**
     * Bluetooth LE 2M PHY. Used to refer to LE 2M Physical Channel for advertising, scanning or
     * connection.
     */
    static const int PHY_LE_2M = 2;

    /**
     * Bluetooth LE Coded PHY. Used to refer to LE Coded Physical Channel for advertising, scanning
     * or connection.
     */
    static const int PHY_LE_CODED = 3;

    /**
     * Bluetooth LE 1M PHY mask. Used to specify LE 1M Physical Channel as one of many available
     * options in a bitmask.
     */
    static const int PHY_LE_1M_MASK = 1;

    /**
     * Bluetooth LE 2M PHY mask. Used to specify LE 2M Physical Channel as one of many available
     * options in a bitmask.
     */
    static const int PHY_LE_2M_MASK = 2;

    /**
     * Bluetooth LE Coded PHY mask. Used to specify LE Coded Physical Channel as one of many
     * available options in a bitmask.
     */
    static const int PHY_LE_CODED_MASK = 4;

    /**
     * No preferred coding when transmitting on the LE Coded PHY.
     */
    static const int PHY_OPTION_NO_PREFERRED = 0;

    /**
     * Prefer the S=2 coding to be used when transmitting on the LE Coded PHY.
     */
    static const int PHY_OPTION_S2 = 1;

    /**
     * Prefer the S=8 coding to be used when transmitting on the LE Coded PHY.
     */
    static const int PHY_OPTION_S8 = 2;

    /** The profile is in disconnected state */
    static const int STATE_DISCONNECTED = 0;

    /** The profile is in connecting state */
    static const int STATE_CONNECTING = 1;

    /** The profile is in connected state */
    static const int STATE_CONNECTED = 2;

    /** The profile is in disconnecting state */
    static const int STATE_DISCONNECTING = 3;

    GattDevice();
};
}//namespace gatt
#endif
