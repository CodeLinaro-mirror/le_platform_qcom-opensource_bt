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

#ifndef ADVERTISING_SETTINGS_HPP_
#define ADVERTISING_SETTINGS_HPP_

#pragma once
#include "utils/include/uuid.h"
using btapp::Uuid;
namespace gatt{

/**
* The AdvertiseSettings provide a way to adjust advertising preferences for each
* Bluetooth LE advertisement instance. Use AdvertiseSettings::Builder to create an
* instance of this using namespace std;class.
*/
class AdvertiseSettings final {
  public:
    /**
    * Perform Bluetooth LE advertising in low power mode. This is the default and preferred
    * advertising mode as it consumes the least power.
    */
    static const int ADVERTISE_MODE_LOW_POWER = 0;

    /**
    * Perform Bluetooth LE advertising in balanced power mode. This is balanced between advertising
    * frequency and power consumption.
    */
    static const int ADVERTISE_MODE_BALANCED = 1;

    /**
    * Perform Bluetooth LE advertising in low latency, high power mode. This has the highest power
    * consumption and should not be used for continuous background advertising.
    */
    static const int ADVERTISE_MODE_LOW_LATENCY = 2;

    /**
    * Advertise using the lowest transmission (TX) power level. Low transmission power can be used
    * to restrict the visibility range of advertising packets.
    */
    static const int ADVERTISE_TX_POWER_ULTRA_LOW = 0;

    /**
    * Advertise using low TX power level.
    */
    static const int ADVERTISE_TX_POWER_LOW = 1;

    /**
    * Advertise using medium TX power level.
    */
    static const int ADVERTISE_TX_POWER_MEDIUM = 2;

    /**
    * Advertise using high TX power level. This corresponds to largest visibility range of the
    * advertising packet.
    */
    static const int ADVERTISE_TX_POWER_HIGH = 3;

  private:
    /**
    * The maximum limited advertisement duration as specified by the Bluetooth SIG
    */
    static const int LIMITED_ADVERTISING_MAX_MILLIS = 180 * 1000;

    const int mAdvertiseMode;
    const int mAdvertiseTxPowerLevel;
    const int mAdvertiseTimeoutMillis;
    const bool mAdvertiseConnectable;

    AdvertiseSettings(const int advertiseMode, const int advertiseTxPowerLevel,
                            bool advertiseConnectable, const int advertiseTimeout);

  public:
    /**
    * Returns the advertise mode.
    */
    int getMode();

    /**
    * Returns the TX power level for advertising.
    */
    int getTxPowerLevel();

    /**
    * Returns whether the advertisement will indicate connectable.
    */
    bool isConnectable();

    /**
    * Returns the advertising time limit in milliseconds.
    */
    int getTimeout();

    /**
    * Builder class for AdvertiseSettings.
    */

    class Builder final {
      private:
        int mMode = ADVERTISE_MODE_LOW_POWER;
        int mTxPowerLevel = ADVERTISE_TX_POWER_MEDIUM;
        int mTimeoutMillis = 0;
        bool mConnectable = true;

      public:
        /**
        * Set advertise mode to control the advertising power and latency.
        *
        * @param advertiseMode Bluetooth LE Advertising mode, can only be one of
        * AdvertiseSettings#ADVERTISE_MODE_LOW_POWER,
        * AdvertiseSettings#ADVERTISE_MODE_BALANCED,
        * or AdvertiseSettings#ADVERTISE_MODE_LOW_LATENCY.
        * @throws std::invalid_argument If the advertiseMode is invalid.
        */
        Builder setAdvertiseMode(int advertiseMode);

        /**
        * Set advertise TX power level to control the transmission power level for the advertising.
        *
        * @param txPowerLevel Transmission power of Bluetooth LE Advertising, can only be one of
        * AdvertiseSettings#ADVERTISE_TX_POWER_ULTRA_LOW,
        * AdvertiseSettings#ADVERTISE_TX_POWER_LOW,
        * AdvertiseSettings#ADVERTISE_TX_POWER_MEDIUM
        * or AdvertiseSettings#ADVERTISE_TX_POWER_HIGH.
        * @throws std::invalid_argument If the txPowerLevel is invalid.
        */
        Builder setTxPowerLevel(int txPowerLevel);

        /**
        * Set whether the advertisement type should be connectable or non-connectable.
        *
        * @param connectable Controls whether the advertisment type will be connectable (true) or
        * non-connectable (false).
        */
        Builder setConnectable(bool connectable);

        /**
        * Limit advertising to a given amount of time.
        *
        * @param timeoutMillis Advertising time limit. May not exceed 180000 milliseconds. A value
        * of 0 will disable the time limit.
        * @throws std::invalid_argument If the provided timeout is over 180000 ms.
        */
        Builder setTimeout(int timeoutMillis);

        /**
        * Build the AdvertiseSettings object.
        */
        AdvertiseSettings* build();

      };
    };
}//namespace gatt
#endif
