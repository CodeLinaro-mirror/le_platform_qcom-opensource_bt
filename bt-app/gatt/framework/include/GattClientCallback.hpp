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

#ifndef GATTCLIENTCALLBACK_HPP_
#define GATTCLIENTCALLBACK_HPP_

#pragma once

#include "GattService.hpp"

using namespace std;

namespace gatt{
class GattClient;
class GattClientCallback {
  public:
    /**
    * Callback triggered as result of GattClient#setPreferredPhy, or as a result of
    * remote device changing the PHY.
    *
    * @param gatt GATT client
    * @param txPhy the transmitter PHY in use. One of GattDevice#PHY_LE_1M,
    * GattDevice#PHY_LE_2M, and GattDevice#PHY_LE_CODED.
    * @param rxPhy the receiver PHY in use. One of GattDevice#PHY_LE_1M,
    * GattDevice#PHY_LE_2M, and GattDevice#PHY_LE_CODED.
    * @param status Status of the PHY update operation.  GattClient#GATT_SUCCESS if the
    * operation succeeds.
    */
    virtual void onPhyUpdate(GattClient *gatt, int txPhy, int rxPhy, int status) {}

    /**
    * Callback triggered as result of GattClient#readPhy
    *
    * @param gatt GATT client
    * @param txPhy the transmitter PHY in use. One of GattDevice#PHY_LE_1M,
    * GattDevice#PHY_LE_2M, and GattDevice#PHY_LE_CODED.
    * @param rxPhy the receiver PHY in use. One of GattDevice#PHY_LE_1M,
    * GattDevice#PHY_LE_2M, and GattDevice#PHY_LE_CODED.
    * @param status Status of the PHY read operation. GattClient#GATT_SUCCESS if the
    * operation succeeds.
    */
    virtual void onPhyRead(GattClient *gatt, int txPhy, int rxPhy, int status) {}


    /**
    * Callback indicating when GATT client has connected/disconnected to/from a remote
    * GATT server.
    *
    * @param gatt GATT client
    * @param status Status of the connect or disconnect operation.
    * GattClient#GATT_SUCCESS if the operation succeeds.
    * @param newState Returns the new connection state. Can be one of
    * GattDevice::STATE_DISCONNECTED or GattDevice::STATE_CONNECTED
    */
    virtual void onConnectionStateChange(GattClient *gatt, int status,int newState) {}

    /**
    * Callback invoked when the list of remote services, characteristics and descriptors
    * for the remote device have been updated, ie new services have been discovered.
    *
    * @param gatt GATT client invoked GattClient#discoverServices
    * @param status GattClient#GATT_SUCCESS if the remote device has been explored
    * successfully.
    */
    virtual void onServicesDiscovered(GattClient *gatt, int status) {}

    /**
    * Callback reporting the result of a characteristic read operation.
    *
    * @param gatt GATT client invoked GattClient#readCharacteristic
    * @param characteristic Characteristic that was read from the associated remote device.
    * @param status GattClient#GATT_SUCCESS if the read operation was completed
    * successfully.
    */
    virtual void onCharacteristicRead(GattClient *gatt, GattCharacteristic *characteristic,
    int status) {}

    /**
    * Callback indicating the result of a characteristic write operation.
    *
    * <p>If this callback is invoked while a reliable write transaction is
    * in progress, the value of the characteristic represents the value
    * reported by the remote device. An application should compare this
    * value to the desired value to be written. If the values don't match,
    * the application must abort the reliable write transaction.
    *
    * @param gatt GATT client invoked GattClient#writeCharacteristic
    * @param characteristic Characteristic that was written to the associated remote device.
    * @param status The result of the write operation GattClient#GATT_SUCCESS if the
    * operation succeeds.
    */
    virtual void onCharacteristicWrite(GattClient *gatt,
                                              GattCharacteristic *characteristic, int status) {}

    /**
    * Callback triggered as a result of a remote characteristic notification.
    *
    * @param gatt GATT client the characteristic is associated with
    * @param characteristic Characteristic that has been updated as a result of a remote
    * notification event.
    */
    virtual void onCharacteristicChanged(GattClient *gatt,
                                                GattCharacteristic *characteristic) {}

    /**
    * Callback reporting the result of a descriptor read operation.
    *
    * @param gatt GATT client invoked GattClient#readDescriptor
    * @param descriptor Descriptor that was read from the associated remote device.
    * @param status GattClient#GATT_SUCCESS if the read operation was completed
    * successfully
    */
    virtual void onDescriptorRead(GattClient *gatt,
                                       GattDescriptor *descriptor, int status) {}

    /**
    * Callback indicating the result of a descriptor write operation.
    *
    * @param gatt GATT client invoked GattClient#writeDescriptor
    * @param descriptor Descriptor that was writte to the associated remote device.
    * @param status The result of the write operation GattClient#GATT_SUCCESS if the
    * operation succeeds.
    */
    virtual void onDescriptorWrite(GattClient *gatt,
                                        GattDescriptor *descriptor, int status) {}

    /**
    * Callback invoked when a reliable write transaction has been completed.
    *
    * @param gatt GATT client invoked GattClient#executeReliableWrite
    * @param status GattClient#GATT_SUCCESS if the reliable write transaction was
    * executed successfully
    */
    virtual void onReliableWriteCompleted(GattClient *gatt,
                                                  int status) {}

    /**
    * Callback reporting the RSSI for a remote device connection.
    *
    * This callback is triggered in response to the
    * GattClient#readRemoteRssi function.
    *
    * @param gatt GATT client invoked GattClient#readRemoteRssi
    * @param rssi The RSSI value for the remote device
    * @param status GattClient#GATT_SUCCESS if the RSSI was read successfully
    */
    virtual void onReadRemoteRssi(GattClient *gatt , int rssi, int status) {}

    /**
    * Callback indicating the MTU for a given device connection has changed.
    *
    * This callback is triggered in response to the
    * GattClient#requestMtu function, or in response to a connection
    * event.
    *
    * @param gatt GATT client invoked GattClient#requestMtu
    * @param mtu The new MTU size
    * @param status GattClient#GATT_SUCCESS if the MTU has been changed successfully
    */
    virtual void onMtuChanged(GattClient *gatt, int mtu, int status) {}

    /**
    * Callback indicating the connection parameters were updated.
    *
    * @param gatt GATT client involved
    * @param interval Connection interval used on this connection, 1.25ms unit. Valid range is from
    * 6 (7.5ms) to 3200 (4000ms).
    * @param latency Slave latency for the connection in number of connection events. Valid range
    * is from 0 to 499
    * @param timeout Supervision timeout for this connection, in 10ms unit. Valid range is from 10
    * (0.1s) to 3200 (32s)
    * @param status GattClient#GATT_SUCCESS if the connection has been updated
    * successfully
    * @hide
    */
    virtual void onConnectionUpdated(GattClient *gatt, int interval, int latency,
                                                              int timeout, int status) {}

    /**
    * Callback indicating service changed event is received.
    *
    * Receiving this event means that the GATT database is out of sync with
    * the remote device. {@link BluetoothGatt#discoverServices} should be
    * called to re-discover the services.
    *
    * @param gatt GATT client involved
    */
    virtual void onServiceChanged(GattClient *gatt) {}

    /**
    * Callback indicating the connection subrate parameters were updated.
    *
    * @param gatt GATT client involved
    * @param subrateFactor Connection subrate factor used on this connection. Valid range is from
    * 1 to 500.
    * @param latency Slave latency for the connection in number of connection events. Valid range
    * is from 0 to 499
    * @param contNum Number of underlying connection intervals to remain active after a non-
    * empty packet is transmitted or received. Valid range is from 0 to 499.
    * @param timeout Supervision timeout for this connection, in 10ms unit. Valid range is from 10
    * (0.1s) to 3200 (32s)
    * @param status GattClient#GATT_SUCCESS if the connection has been updated
    * successfully
    * @hide
    */
    virtual void onSubrateChanged(GattClient *gatt, int subrateFactor, int latency, int contNum,
                                                          int timeout, int status) {}
};
}
#endif
