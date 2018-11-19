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

#ifndef ADVERTISING_SET_PARAMETERS_HPP_
#define ADVERTISING_SET_PARAMETERS_HPP_

#pragma once
#include "GattDevice.hpp"
using namespace std;
using std::string;

namespace gatt{

/**
 * The AdvertisingSetParameters provide a way to adjust advertising
 * preferences for each Bluetooth LE advertising set.
 * Use AdvertisingSetParameters::Builder to create an instance of this class.
 */

class AdvertisingSetParameters final {
  public:
   /**
     * Advertise on low frequency, around every 1000ms. This is the default and
     * preferred advertising mode as it consumes the least power.
     */
    static const int INTERVAL_HIGH = 1600;

    /**
     * Advertise on medium frequency, around every 250ms. This is balanced
     * between advertising frequency and power consumption.
     */
    static const int INTERVAL_MEDIUM = 400;

    /**
     * Perform high frequency, low latency advertising, around every 100ms. This
     * has the highest power consumption and should not be used for continuous
     * background advertising.
     */
    static const int INTERVAL_LOW = 160;

    /**
     * Minimum value for advertising interval.
     */
    static const int INTERVAL_MIN = 160;

    /**
     * Maximum value for advertising interval.
     */
    static const int INTERVAL_MAX = 16777215;

    /**
     * Advertise using the lowest transmission (TX) power level. Low transmission
     * power can be used to restrict the visibility range of advertising packets.
     */
    static const int TX_POWER_ULTRA_LOW = -21;

    /**
     * Advertise using low TX power level.
     */
    static const int TX_POWER_LOW = -15;

    /**
     * Advertise using medium TX power level.
     */
    static const int TX_POWER_MEDIUM = -7;

    /**
     * Advertise using high TX power level. This corresponds to largest visibility
     * range of the advertising packet.
     */
    static const int TX_POWER_HIGH = 1;

    /**
     * Minimum value for TX power.
     */
    static const int TX_POWER_MIN = -127;

    /**
     * Maximum value for TX power.
     */
    static const int TX_POWER_MAX = 1;

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

  private:
    /**
     * The maximum limited advertisement duration as specified by the Bluetooth
     * SIG
     */
    static const int LIMITED_ADVERTISING_MAX_MILLIS = 180 * 1000;

    const bool mIsLegacy;
    const bool mIsAnonymous;
    const bool mIncludeTxPower;
    const int mPrimaryPhy;
    const int mSecondaryPhy;
    const bool mConnectable;
    const bool mScannable;
    const int mInterval;
    const int mTxPowerLevel;

    AdvertisingSetParameters(bool connectable, bool scannable, bool isLegacy,
            bool isAnonymous, bool includeTxPower,
            int primaryPhy, int secondaryPhy,
            int interval, int txPowerLevel);


  public:
    /**
     * Returns whether the advertisement will be connectable.
     */
    bool isConnectable();

    /**
     * Returns whether the advertisement will be scannable.
     */
    bool isScannable();

    /**
     * Returns whether the legacy advertisement will be used.
     */
    bool isLegacy();

    /**
     * Returns whether the advertisement will be anonymous.
     */
    bool isAnonymous();

    /**
     * Returns whether the TX Power will be included.
     */
    bool includeTxPower();

    /**
     * Returns the primary advertising phy.
     */
    int getPrimaryPhy();

    /**
     * Returns the secondary advertising phy.
     */
    int getSecondaryPhy();
    /**
     * Returns the advertising interval.
     */
    int getInterval();

    /**
     * Returns the TX power level for advertising.
     */
    int getTxPowerLevel();


    /**
     * Builder class for AdvertisingSetParameters.
     */

    class Builder final{
      private:
        bool mConnectable = false;
        bool mScannable = false;
        bool mIsLegacy = false;
        bool mIsAnonymous = false;
        bool mIncludeTxPower = false;
        int mPrimaryPhy = GattDevice::PHY_LE_1M;
        int mSecondaryPhy = GattDevice::PHY_LE_1M;
        int mInterval = INTERVAL_LOW;
        int mTxPowerLevel = TX_POWER_MEDIUM;

      public:
        /**
         * Set whether the advertisement type should be connectable or
         * non-connectable.
         * Legacy advertisements can be both connectable and scannable. Non-legacy
         * advertisements can be only scannable or only connectable.
         *
         * @param connectable Controls whether the advertisement type will be connectable (true) or
         * non-connectable (false).
         */
        Builder setConnectable(bool connectable);

        /**
         * Set whether the advertisement type should be scannable.
         * Legacy advertisements can be both connectable and scannable. Non-legacy
         * advertisements can be only scannable or only connectable.
         *
         * @param scannable Controls whether the advertisement type will be scannable (true) or
         * non-scannable (false).
         */
        Builder setScannable(bool scannable);

        /**
         * When set to true, advertising set will advertise 4.x Spec compliant
         * advertisements.
         *
         * @param isLegacy whether legacy advertising mode should be used.
         */
        Builder setLegacyMode(bool isLegacy);

        /**
         * Set whether advertiser address should be ommited from all packets. If this
         * mode is used, periodic advertising can't be enabled for this set.
         *
         * This is used only if legacy mode is not used.
         *
         * @param isAnonymous whether anonymous advertising should be used.
         */
        Builder setAnonymous(bool isAnonymous);

        /**
         * Set whether TX power should be included in the extended header.
         *
         * This is used only if legacy mode is not used.
         *
         * @param includeTxPower whether TX power should be included in extended header
         */
        Builder setIncludeTxPower(bool includeTxPower);

        /**
         * Set the primary physical channel used for this advertising set.
         *
         * This is used only if legacy mode is not used.
         *
         * Use GattLibServices#isLeCodedPhySupported to determine if LE Coded PHY is
         * supported on this device.
         *
         * @param primaryPhy Primary advertising physical channel, can only be
         * AdvertisingSetParameters#PHY_LE_1M or AdvertisingSetParameters#PHY_LE_CODED.
         * @throws std::invalid_argument If the primaryPhy is invalid.
         */
        Builder setPrimaryPhy(int primaryPhy);

        /**
         * Set the secondary physical channel used for this advertising set.
         *
         * This is used only if legacy mode is not used.
         *
         * Use GattLibService#isLeCodedPhySupported and
         * GattLibService#isLe2MPhySupported to determine if LE Coded PHY or 2M PHY is
         * supported on this device.
         *
         * @param secondaryPhy Secondary advertising physical channel, can only be one of
         * AdvertisingSetParameters#PHY_LE_1M, AdvertisingSetParameters#PHY_LE_2M or
         * AdvertisingSetParameters#PHY_LE_CODED.
         * @throws std::invalid_argument If the secondaryPhy is invalid.
         */
        Builder setSecondaryPhy(int secondaryPhy);

        /**
         * Set advertising interval.
         *
         * @param interval Bluetooth LE Advertising interval, in 0.625ms unit. Valid range is from
         * 160 (100ms) to 16777215 (10,485.759375 s). Recommended values are: {@link
         * AdvertisingSetParameters#INTERVAL_LOW, AdvertisingSetParameters#INTERVAL_MEDIUM,
         * or AdvertisingSetParameters#INTERVAL_HIGH.
         * @throws std::invalid_argument If the interval is invalid.
         */
        Builder setInterval(int interval);

        /**
         * Set the transmission power level for the advertising.
         *
         * @param txPowerLevel Transmission power of Bluetooth LE Advertising, in dBm. The valid
         * range is [-127, 1] Recommended values are:
         * AdvertisingSetParameters#TX_POWER_ULTRA_LOW,
         * AdvertisingSetParameters#TX_POWER_LOW,
         * AdvertisingSetParameters#TX_POWER_MEDIUM,
         * or AdvertisingSetParameters#TX_POWER_HIGH.
         * @throws std::invalid_argument If the txPowerLevel is invalid.
         */
        Builder setTxPowerLevel(int txPowerLevel);

        /**
         * Build the AdvertisingSetParameters} object.
         *
         * @throws std::invalid_argument if invalid combination of parameters is used.
         */
        AdvertisingSetParameters* build();

      };
  };
}//namespace gatt
#endif
