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

#ifndef SCANSETTINGS_HPP_
#define SCANSETTINGS_HPP_

#pragma once
using namespace std;
using std::string;

namespace gatt{

/**
 * Bluetooth LE scan settings are passed to GattLeScanner#startScan to define the
 * parameters for the scan.
 */
class ScanSettings final {
  private:
    // Bluetooth LE scan mode.
    int mScanMode;

    // Bluetooth LE scan callback type
    int mCallbackType;

    // Bluetooth LE scan result type
    int mScanResultType;

    // Time of delay for reporting the scan result
    long mReportDelayMillis;

    int mMatchMode;

    int mNumOfMatchesPerFilter;

    // Include only legacy advertising results
    bool mLegacy;

    int mPhy;

    ScanSettings(int scanMode, int callbackType, int scanResultType,
          long reportDelayMillis, int matchMode,
        int numOfMatchesPerFilter, bool legacy, int phy);

  public:
    /**
     * A special Bluetooth LE scan mode. Applications using this scan mode will passively listen for
     * other scan results without starting BLE scans themselves.
     */
    static const int SCAN_MODE_OPPORTUNISTIC = -1;

    /**
     * Perform Bluetooth LE scan in low power mode. This is the default scan mode as it consumes the
     * least power. This mode is enforced if the scanning application is not in foreground.
     */
    static const int SCAN_MODE_LOW_POWER = 0;

    /**
     * Perform Bluetooth LE scan in balanced power mode. Scan results are returned at a rate that
     * provides a good trade-off between scan frequency and power consumption.
     */
    static const int SCAN_MODE_BALANCED = 1;

    /**
     * Scan using highest duty cycle. It's recommended to only use this mode when the application is
     * running in the foreground.
     */
    static const int SCAN_MODE_LOW_LATENCY = 2;

    /**
     * Trigger a callback for every Bluetooth advertisement found that matches the filter criteria.
     * If no filter is active, all advertisement packets are reported.
     */
    static const int CALLBACK_TYPE_ALL_MATCHES = 1;

    /**
     * A result callback is only triggered for the first advertisement packet received that matches
     * the filter criteria.
     */
    static const int CALLBACK_TYPE_FIRST_MATCH = 2;

    /**
     * Receive a callback when advertisements are no longer received from a device that has been
     * previously reported by a first match callback.
     */
    static const int CALLBACK_TYPE_MATCH_LOST = 4;

    /**
     * Provide results to sensor router instead of the apps processor
     * @hide
     */
    static const int CALLBACK_TYPE_SENSOR_ROUTING = 8;

    /**
     * Determines how many advertisements to match per filter, as this is scarce hw resource
     */
    /**
     * Match one advertisement per filter
     */
    static const int MATCH_NUM_ONE_ADVERTISEMENT = 1;

    /**
     * Match few advertisement per filter, depends on current capability and availibility of
     * the resources in hw
     */
    static const int MATCH_NUM_FEW_ADVERTISEMENT = 2;

    /**
     * Match as many advertisement per filter as hw could allow, depends on current
     * capability and availibility of the resources in hw
     */
    static const int MATCH_NUM_MAX_ADVERTISEMENT = 3;


    /**
     * In Aggressive mode, hw will determine a match sooner even with feeble signal strength
     * and few number of sightings/match in a duration.
     */
    static const int MATCH_MODE_AGGRESSIVE = 1;

    /**
     * For sticky mode, higher threshold of signal strength and sightings is required
     * before reporting by hw
     */
    static const int MATCH_MODE_STICKY = 2;

    /**
     * Use all supported PHYs for scanning.
     * This will check the controller capabilities, and start
     * the scan on 1Mbit and LE Coded PHYs if supported, or on
     * the 1Mbit PHY only.
     */
    static const int PHY_LE_ALL_SUPPORTED = 255;

    /**
      * Request full scan results which contain the device, rssi, advertising data, scan response
      * as well as the scan timestamp.
      *
      * @hide
      */
      static const int SCAN_RESULT_TYPE_FULL = 0;

       /**
      * Request abbreviated scan results which contain the device, rssi and scan timestamp.
      * <p>
      * <b>Note:</b> It is possible for an application to get more scan results than it asked for,
      * if there are multiple apps using this type.
      *
      * @hide
      */
      static const int SCAN_RESULT_TYPE_ABBREVIATED = 1;

      /**
       * @hide
       */
      int getMatchMode();

      /**
       * @hide
       */
      int getNumOfMatches();

    int getScanMode();

    int getCallbackType();

    int getScanResultType();

    /**
     * Returns whether only legacy advertisements will be returned.
     * Legacy advertisements include advertisements as specified
     * by the Bluetooth core specification 4.2 and below.
     */
    bool getLegacy();

    /**
     * Returns the physical layer used during a scan.
     */
    int getPhy();

    /**
     * Returns report delay timestamp based on the device clock.
     */
    long getReportDelayMillis();

    /**
     * Builder for ScanSettings.
     */
    class Builder {
      private:
        int mScanMode = SCAN_MODE_LOW_POWER;
        int mCallbackType = CALLBACK_TYPE_ALL_MATCHES;
        int mScanResultType = SCAN_RESULT_TYPE_FULL;
        long mReportDelayMillis = 0;
        int mMatchMode = MATCH_MODE_AGGRESSIVE;
        int mNumOfMatchesPerFilter = MATCH_NUM_MAX_ADVERTISEMENT;
        bool mLegacy = true;
        int mPhy = PHY_LE_ALL_SUPPORTED;

        // Returns true if the callbackType is valid.
        bool isValidCallbackType(int callbackType);

      public:
        /**
         * Set scan mode for Bluetooth LE scan.
         *
         * @param scanMode The scan mode can be one of ScanSettings#SCAN_MODE_LOW_POWER,
         * ScanSettings#SCAN_MODE_BALANCED or ScanSettings#SCAN_MODE_LOW_LATENCY.
         * @throws std::invalid_argument If the scanMode is invalid.
         */
        Builder setScanMode(int scanMode);

        /**
         * Set callback type for Bluetooth LE scan.
         *
         * @param callbackType The callback type flags for the scan.
         * @throws std::invalid_argument If the callbackType is invalid.
         */
        Builder setCallbackType(int callbackType);

        /**
         * Set report delay timestamp for Bluetooth LE scan.
         *
         * @param reportDelayMillis Delay of report in milliseconds. Set to 0 to be notified of
         * results immediately. Values &gt; 0 causes the scan results to be queued up and delivered
         * after the requested delay or when the internal buffers fill up.
         * @throws std::invalid_argument If reportDelayMillis is 0.
         */
        Builder setReportDelay(long reportDelayMillis);

        /**
         * Set the number of matches for Bluetooth LE scan filters hardware match
         *
         * @param numOfMatches The num of matches can be one of
         * ScanSettings#MATCH_NUM_ONE_ADVERTISEMENT
         * or ScanSettings#MATCH_NUM_FEW_ADVERTISEMENT or
         * ScanSettings#MATCH_NUM_MAX_ADVERTISEMENT
         * @throws std::invalid_argument If the matchMode is invalid.
         */
        Builder setNumOfMatches(int numOfMatches);

        /**
         * Set match mode for Bluetooth LE scan filters hardware match
         *
         * @param matchMode The match mode can be one of ScanSettings#MATCH_MODE_AGGRESSIVE
         * or ScanSettings#MATCH_MODE_STICKY
         * @throws std::invalid_argument If the matchMode is invalid.
         */
        Builder setMatchMode(int matchMode);

        /**
         * Set whether only legacy advertisments should be returned in scan results.
         * Legacy advertisements include advertisements as specified by the
         * Bluetooth core specification 4.2 and below. This is true by default
         * for compatibility with older apps.
         *
         * @param legacy true if only legacy advertisements will be returned
         */
        Builder setLegacy(bool legacy);

        /**
         * Set the Physical Layer to use during this scan.
         * This is used only if ScanSettings.Builder#setLegacy
         * is set to false.
         * android.bluetooth.BluetoothAdapter#isLeCodedPhySupported
         * may be used to check whether LE Coded phy is supported by calling
         * android.bluetooth.BluetoothAdapter#isLeCodedPhySupported.
         * Selecting an unsupported phy will result in failure to start scan.
         *
         * @param phy Can be one of GattDevice#PHY_LE_1M,
         * GattDevice#PHY_LE_CODED or ScanSettings#PHY_LE_ALL_SUPPORTED
         */
        Builder setPhy(int phy);

        /**
         * Set scan result type for Bluetooth LE scan.
         *
         * @param scanResultType Type for scan result, could be either
         * ScanSettings#SCAN_RESULT_TYPE_FULL or ScanSettings#SCAN_RESULT_TYPE_ABBREVIATED.
         * @throws std::invalid_argument If the scanResultType is invalid.
         * @hide
         */
        Builder setScanResultType(int scanResultType);

        /**
         * Build ScanSettings.
         */
        ScanSettings* build();
    };
};
}//namespace gatt
#endif
