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

#ifndef SCANRESULT_HPP_
#define SCANRESULT_HPP_

#pragma once
#include <string>
#include "GattDevice.hpp"
using namespace std;
using std::string;

namespace gatt {
class ScanRecord;

/**
 * ScanResult for Bluetooth LE scan.
 */
class ScanResult final {
  public:
    /**
     * For chained advertisements, inidcates tha the data contained in this
     * scan result is complete.
     */
    static const int DATA_COMPLETE = 0x00;

    /**
     * For chained advertisements, indicates that the controller was
     * unable to receive all chained packets and the scan result contains
     * incomplete truncated data.
     */
    static const int DATA_TRUNCATED = 0x02;

    /**
     * Indicates that the secondary physical layer was not used.
     */
    static const int PHY_UNUSED = 0x00;

    /**
     * Advertising Set ID is not present in the packet.
     */
    static const int SID_NOT_PRESENT = 0xFF;

    /**
     * TX power is not present in the packet.
     */
    static const int TX_POWER_NOT_PRESENT = 0x7F;

    /**
     * Periodic advertising interval is not present in the packet.
     */
    static const int PERIODIC_INTERVAL_NOT_PRESENT = 0x00;

  private:
    /**
     * Mask for checking whether event type represents legacy advertisement.
     */
    static const int ET_LEGACY_MASK = 0x10;

    /**
     * Mask for checking whether event type represents connectable advertisement.
     */
    static const int ET_CONNECTABLE_MASK = 0x01;

    /**
     * Scan record, including advertising data and scan response data.
     */
    ScanRecord *mScanRecord;

    /**
     * Received signal strength.
     */
    int mRssi;

    string mDeviceAddress;
    int mEventType;
    int mPrimaryPhy;
    int mSecondaryPhy;
    int mAdvertisingSid;
    int mTxPower;
    int mPeriodicAdvertisingInterval;

  public:
    /**
     * Constructs a new ScanResult.
     *
     * @param device Remote Bluetooth device found.
     * @param eventType Event type.
     * @param primaryPhy Primary advertising phy.
     * @param secondaryPhy Secondary advertising phy.
     * @param advertisingSid Advertising set ID.
     * @param txPower Transmit power.
     * @param rssi Received signal strength.
     * @param periodicAdvertisingInterval Periodic advertising interval.
     * @param scanRecord Scan record including both advertising data and scan response data.
     */
    ScanResult(string deviceAddress, int eventType, int primaryPhy, int secondaryPhy,
            int advertisingSid, int txPower, int rssi, int periodicAdvertisingInterval,
            ScanRecord *scanRecord);
    /**
    * Constructs a new ScanResult.
    *
    * @param device Remote Bluetooth device found.
    * @param scanRecord Scan record including both advertising data and scan response data.
    * @param rssi Received signal strength.
    * @deprecated use {@link #ScanResult(BluetoothDevice, int, int, int, int, int, int, int,
    * ScanRecord, long)}
    * @hide
    */
    ScanResult(string deviceAddress, ScanRecord *scanRecord, int rssi);
    /**
     * Returns the remote Bluetooth device identified by the Bluetooth device address.
     */
    string getDevice();

    /**
     * Returns the scan record, which is a combination of advertisement and scan response.
     */

    ScanRecord* getScanRecord();

    /**
     * Returns the received signal strength in dBm. The valid range is [-127, 126].
     */
    int getRssi();

    /**
     * Returns true if this object represents legacy scan result.
     * Legacy scan results do not contain advanced advertising information
     * as specified in the Bluetooth Core Specification v5.
     */
    bool isLegacy();

    /**
     * Returns true if this object represents connectable scan result.
     */
    bool isConnectable();

    /**
     * Returns the data status.
     * Can be one of ScanResult#DATA_COMPLETE or
     * ScanResult#DATA_TRUNCATED.
     */
    int getDataStatus();

    /**
     * Returns the primary Physical Layer
     * on which this advertisment was received.
     * Can be one of GattDevice#PHY_LE_1M or
     * GattDevice#PHY_LE_CODED.
     */
    int getPrimaryPhy();

    /**
     * Returns the secondary Physical Layer
     * on which this advertisment was received.
     * Can be one of GattDevice#PHY_LE_1M,
     * GattDevice#PHY_LE_2M, GattDevice#PHY_LE_CODED
     * or ScanResult#PHY_UNUSED - if the advertisement
     * was not received on a secondary physical channel.
     */
    int getSecondaryPhy();

    /**
     * Returns the advertising set id.
     * May return ScanResult#SID_NOT_PRESENT if
     * no set id was is present.
     */
    int getAdvertisingSid();

    /**
     * Returns the transmit power in dBm.
     * Valid range is [-127, 126]. A value of ScanResult#TX_POWER_NOT_PRESENT
     * indicates that the TX power is not present.
     */
    int getTxPower();
    /**
     * Returns the periodic advertising interval in units of 1.25ms.
     * Valid range is 6 (7.5ms) to 65536 (81918.75ms). A value of
     * ScanResult#PERIODIC_INTERVAL_NOT_PRESENT means periodic
     * advertising interval is not present.
     */
    int getPeriodicAdvertisingInterval();

    std::string ToString() const;

};
}//namespace gatt
#endif
