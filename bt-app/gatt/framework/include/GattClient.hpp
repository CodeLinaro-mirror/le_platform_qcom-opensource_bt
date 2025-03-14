/*
* * Copyright (c) 2018, The Linux Foundation. All rights reserved.
* Not a Contribution.
* Copyright (C) 2013 The Android Open Source Project
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

#ifndef GATTCLIENT_HPP_
#define GATTCLIENT_HPP_

#pragma once

#include "GattLibService.hpp"
#include "GattClientCallback.hpp"
#include "IClientCallback.hpp"
#include "GattDescriptor.hpp"
#include "GattCharacteristic.hpp"
#include "GattService.hpp"

#include<iostream>
#include <mutex>
#include<list>
#include "log.h"
#include "utils/include/uuid.h"

using namespace std;
using std::string;
using btapp::Uuid;

namespace gatt{
/**
 * Public API for the Bluetooth GATT Cient Profile.
 *
 * <p>This class provides Bluetooth GATT Client functionality to enable communication
 * with remote device
 *
 */
class GattClient : public IClientCallback, public GattClientCallback {
  private:
    static const bool DBG = true;
    static const bool VDBG = true;

    GattLibService *mService = NULL;
    IClientCallback *mGattCallback = NULL;
    GattClientCallback *mCallback = NULL;

    int mClientIf;
    string mDeviceAddress;
    bool mAutoConnect;
    int mAuthRetryState;
    int mConnState;
    std::mutex mStateLock;
    std::mutex mDeviceBusyLock;
    bool mDeviceBusy = false;
    int mTransport;
    int mPhy;
    bool mOpportunistic;

    static const int AUTH_RETRY_STATE_IDLE = 0;
    static const int AUTH_RETRY_STATE_NO_MITM = 1;
    static const int AUTH_RETRY_STATE_MITM = 2;

    static const int CONN_STATE_IDLE = 0;
    static const int CONN_STATE_CONNECTING = 1;
    static const int CONN_STATE_CONNECTED = 2;
    static const int CONN_STATE_DISCONNECTING = 3;
    static const int CONN_STATE_CLOSED = 4;

    std::vector<GattService*> mServices;

   /**
   * Register an application callback to start using GATT.
   *
   * <p>The callback {GattClientCallback#onAppRegistered}
   * is used to notify success or failure if the function returns true.
   *
   * @param callback GATT callback handler that will receive callbacks.
   * @return If true, the callback will be called to notify success or failure, false on immediate
   * error
   */
    bool registerApp(GattClientCallback& callback);

    /**
     * Unregister the current application and callbacks.
     */
    void unregisterApp();

  public:

     /** A GATT operation completed successfully */
     static const int GATT_SUCCESS = 0;

    /** GATT read/write operation is invalide handle */
    static const int GATT_INVALID_HANDLE = 1;

    /** GATT read operation is not permitted */
    static const int GATT_READ_NOT_PERMITTED = 0x2;

    /** GATT write operation is not permitted */
    static const int GATT_WRITE_NOT_PERMITTED = 0x3;

    /** Insufficient authentication for a given operation */
    static const int GATT_INSUFFICIENT_AUTHENTICATION = 0x5;

    /** The given request is not supported */
    static const int GATT_REQUEST_NOT_SUPPORTED = 0x6;

    /** Insufficient encryption for a given operation */
    static const int GATT_INSUFFICIENT_ENCRYPTION = 0xf;

    /** A read or write operation was requested with an invalid offset */
    static const int GATT_INVALID_OFFSET = 0x7;

    /** A write operation exceeds the maximum length of the attribute */
    static const int GATT_INVALID_ATTRIBUTE_LENGTH = 0xd;

    /** A remote device connection is congested. */
    static const int GATT_CONNECTION_CONGESTED = 0x8f;

    /** A GATT operation failed, errors other than the above */
    static const int GATT_FAILURE = 0x101;

    /**
    * Connection parameter update - Use the connection parameters recommended by the
    * Bluetooth SIG. This is the default value if no connection parameter update
    * is requested.
    */
    static const int CONNECTION_PRIORITY_BALANCED = 0;

    /**
    * Connection parameter update - Request a high priority, low latency connection.
    * An application should only request high priority connection parameters to transfer large
    * amounts of data over LE quickly. Once the transfer is complete, the application should
    * request {GattClient#CONNECTION_PRIORITY_BALANCED} connection parameters to reduce
    * energy use.
    */
    static const int CONNECTION_PRIORITY_HIGH = 1;

    /** Connection parameter update - Request low power, reduced data rate connection parameters. */
    static const int CONNECTION_PRIORITY_LOW_POWER = 2;

    /**
    * No authentication required.
    */
    static const int AUTHENTICATION_NONE = 0;

    /**
    * Authentication requested; no man-in-the-middle protection required.
    */
    static const int AUTHENTICATION_NO_MITM = 1;

    /**
    * Authentication with man-in-the-middle protection requested.
    */
    static const int AUTHENTICATION_MITM = 2;

    /**
    * @hide
    */
    GattClient(   );

    /**
    * Create GattClient object
    * @param gatt GattLibService single instance
    * @param deviceAddress
    * @param transport Transport to be used for GATT connection.
    * Types of transport GattDevice#TRANSPORT_AUTO(0)
    * GattDevice#TRANSPORT_BREDR(1) GattDevice#TRANSPORT_LE(2)
    * @param opportunistic wether the connection shall be opportunistic,
    * and don't impact the disconnection timer
    * @param phy LE PHY to use.
    */
    GattClient(   GattLibService *gatt,string deviceAddress, int transport,
                                                  bool opportunistic, int phy);

    /**
    * Destructor
    */
    ~GattClient();
    /**
    * Cancels a reliable write transaction for a given device.
    *
    * <p>Calling this function will discard all queued characteristic write
    * operations for a given remote device.
    */
    void abortReliableWrite();
     /**
     * Initiates a reliable write transaction for a given remote device.
     *
     * <p>Once a reliable write transaction has been initiated, all calls
     * to writeCharacteristic are sent to the remote device for
     * verification and queued up for atomic execution. The application will
     * receive an GattClientCallback#onCharacteristicWrite callback
     * in response to every GattClient#writeCharacteristic call and is responsible
     * for verifying if the value has been transmitted accurately.
     *
     * <p>After all characteristics have been queued up and verified,
     * executeReliableWrite will execute all writes. If a characteristic
     * was not written correctly, calling abortReliableWrite will
     * cancel the current transaction without commiting any values on the
     * remote device.
     *
     * @return true, if the reliable write transaction has been initiated
     */
    bool beginReliableWrite();
    /**
     * Close this GATT client.
     *
     * Application should call this method as early as possible after it is done with
     * this GATT client.
     */
    void close();
    /**
     * Initiate a connection to a Bluetooth GATT capable device.
     *
     * <p>The connection may not be established right away, but will be
     * completed when the remote device is available. A
     * GattClientCallback#onConnectionStateChange callback will be
     * invoked when the connection state changes as a result of this function.
     *
     * <p>The autoConnect parameter determines whether to actively connect to
     * the remote device, or rather passively scan and finalize the connection
     * when the remote device is in range/available. Generally, the first ever
     * connection to a device should be direct (autoConnect set to false) and
     * subsequent connections to known devices should be invoked with the
     * autoConnect parameter set to true.
     *
     * @param autoConnect Whether to directly connect to the remote device (false) or to
     * automatically connect as soon as the remote device becomes available (true).
     * @param GattClientCallback object
     * @return true, if the connection attempt was initiated successfully
     */
    bool connect(bool autoConnect,GattClientCallback& callback);
    /**
     * Disconnects an established connection, or cancels a connection attempt
     * currently in progress.
     */
    void disconnect();
     /**
     * Discovers a service by UUID.
     * It should never be used by real applications. The service is not searched
     * for characteristics and descriptors, or returned in any callback.
     *
     * @return true, if the remote service discovery has been started
     */
    bool discoverServiceByUuid(Uuid uuid);
    /**
     * Discovers services offered by a remote device as well as their
     * characteristics and descriptors.
     *
     * <p>Once service discovery is completed, the GattClientCallback#onServicesDiscovered
     * callback is triggered. If the discovery was successful, the remote services can be
     * retrieved using the getServices function.
     *
     * @return true, if the remote service discovery has been started
     */
    bool discoverServices();
     /**
     * Executes a reliable write transaction for a given remote device.
     *
     * <p>This function will commit all queued up characteristic write
     * operations for a given remote device.
     *
     * <p>A GattClientCallback#onReliableWriteCompleted callback is
     * invoked to indicate whether the transaction has been executed correctly.
     *
     * @return true, if the request to execute the transaction has been sent
     */
    bool executeReliableWrite();
    /**
     * Returns a characteristic with id equal to instanceId.
     *
     */
    GattCharacteristic* getCharacteristicById(string deviceAddress, int instanceId);
     /**
     * Returns a descriptor with id equal to instanceId.
     *
     */
    GattDescriptor* getDescriptorById(string deviceAddress, int instanceId);
    /**
     * Return the remote bluetooth devic eaddress this GATT client targets to
     *
     * @return remote bluetooth device address
     */
    string getDeviceAddress();
    /**
     * Returns a GattService, if the requested UUID is
     * supported by the remote device.
     *
     * <p>This function requires that service discovery has been completed
     * for the given device.
     *
     * <p>If multiple instances of the same service (as identified by UUID)
     * exist, the first instance of the service is returned.
     *
     * @param uuid UUID of the requested service
     * @return GattService if supported, or null if the requested service is not offered by
     * the remote device.
     */
    GattService* getService(string deviceAddress, Uuid uuid, int instanceId);
    /**
     * Returns a list of GATT services offered by the remote device.
     *
     * <p>This function requires that service discovery has been completed
     * for the given device.
     *
     * @return List of services on the remote device.
     * Returns an empty list if service discovery has
     * not yet been performed.
     */
    std::list<GattService*> getServices();
    /**
     * Returns a GattService, if the requested UUID is
     * supported by the remote device.
     *
     * <p>This function requires that service discovery has been completed
     * for the given device.
     *
     * <p>If multiple instances of the same service (as identified by UUID)
     * exist, the first instance of the service is returned.
     *
     * @param uuid UUID of the requested service
     * @return GattService if supported, or null if the requested service is not offered by
     * the remote device.
     */
    GattService* getService(Uuid uuid);
    /**
     * Reads the requested characteristic from the associated remote device.
     *
     * <p>The result of the read operation is reported
     * by the GattClientCallback#onCharacteristicRead callback.
     *
     * @param characteristic Characteristic to read from the remote device
     * @return true, if the read operation was initiated successfully
     */
    bool readCharacteristic(GattCharacteristic &characteristic);
    /**
     * Reads the value for a given descriptor from the associated remote device.
     *
     * <p>Once the read operation has been completed, the
     * GattClientCallback#onDescriptorRead callback is
     * triggered, signaling the result of the operation.
     *
     * @param descriptor Descriptor value to read from the remote device
     * @return true, if the read operation was initiated successfully
     */
    bool readDescriptor(GattDescriptor &descriptor);
    /**
     * Read the current transmitter PHY and receiver PHY of the connection.
     * The values are returned in GattClientCallback#onPhyRead
     */
    void readPhy();
    /**
     * Read the RSSI for a connected remote device.
     *
     * <p>The GattClientCallback#onReadRemoteRssi callback will be
     * invoked when the RSSI value has been read.
     *
     * @return true, if the RSSI value has been requested successfully
     */
    bool readRemoteRssi();
     /**
     * Reads the characteristic using its UUID from the associated remote device.
     *
     * <p>The result of the read operation is reported
     * by the {GattClientCallback#onCharacteristicRead} callback.
     *
     * @param uuid UUID of characteristic to read from the remote device
     * @return true, if the read operation was initiated successfully
     * @hide
     */
    bool readUsingCharacteristicUuid(Uuid uuid, int startHandle, int endHandle);
     /**
     * Clears the internal cache and forces a refresh of the services from the
     * remote device.
     */
    bool refresh();

    /**
     * Request a connection parameter update.
     *
     * <p>This function will send a connection parameter update request to the
     * remote device.
     *
     * @param connectionPriority Request a specific connection priority. Must be one of
     * GattClient#CONNECTION_PRIORITY_BALANCED, GattClient#CONNECTION_PRIORITY_HIGH
     * or GattClient#CONNECTION_PRIORITY_LOW_POWER.
     * @throws std::invalid_argument exception If the parameters are outside of
     * their specified range.
     */
    bool requestConnectionPriority(int connectionPriority);

    /**
     * Request an MTU size used for a given connection.
     *
     * <p>When performing a write request operation (write without response),
     * the data sent is truncated to the MTU size. This function may be used
     * to request a larger MTU size to be able to send more data at once.
     *
     * <p>A GattClientCallback#onMtuChanged callback will indicate
     * whether this operation was successful.
     *
     * @return true, if the new MTU value has been requested successfully
     */
    bool requestMtu(int mtu);
    /**
     * Enable or disable notifications/indications for a given characteristic.
     *
     * <p>Once notifications are enabled for a characteristic, a
     * GattClientCallback#onCharacteristicChanged callback will be
     * triggered if the remote device indicates that the given characteristic
     * has changed
     *
     * @param characteristic The characteristic for which to enable notifications
     * @param enable Set to true to enable notifications/indications
     * @return true, if the requested notification status was set successfully
     */
    bool setCharacteristicNotification(GattCharacteristic &characteristic,
                                                bool enable);

     /**
     * Set the preferred connection PHY for this app. Please note that this is just a
     * recommendation, whether the PHY change will happen depends on other applications preferences,
     * local and remote controller capabilities. Controller can override these settings.
     * <p>
     * GattClientCallback#onPhyUpdate will be triggered as a result of this call, even
     * if no PHY change happens. It is also triggered when remote device updates the PHY.
     *
     * @param txPhy preferred transmitter PHY. Bitwise OR of any of GattDevice#PHY_LE_1M_MASK,
      * GattDevice#PHY_LE_2M_MASK, and GattDevice#PHY_LE_CODED_MASK.
     * @param rxPhy preferred receiver PHY. Bitwise OR of any of GattDevice#PHY_LE_1M_MASK,
     * GattDevice#PHY_LE_2M_MASK, and GattDevice#PHY_LE_CODED_MASK.
     * @param phyOptions preferred coding to use when transmitting on the LE Coded PHY. Can be one
     * of GattDevice#PHY_OPTION_NO_PREFERRED, GattDevice#PHY_OPTION_S2 or GattDevice#PHY_OPTION_S8
     */
    void setPreferredPhy(int txPhy, int rxPhy, int phyOptions);

    /**
     * Writes a given characteristic and its values to the associated remote device.
     *
     * <p>Once the write operation has been completed, the
     * GattClientCallback#onCharacteristicWrite callback is invoked,
     * reporting the result of the operation.
     *
     * @param characteristic Characteristic to write on the remote device
     * @return true, if the write operation was initiated successfully
     */
    bool writeCharacteristic(GattCharacteristic &characteristic);
    /**
     * Write the value of a given descriptor to the associated remote device.
     *
     * <p>A GattClientCallback#onDescriptorWrite callback is
     * triggered to report the result of the write operation.
     *
     * @param descriptor Descriptor to write to the associated remote device
     * @return true, if the write operation was initiated successfully
     */
    bool writeDescriptor(GattDescriptor &descriptor);

    /*Inherited Callbacks*/
    void onClientRegistered(int status, int clientIf);

    void onConnectionState(int status, int clientIf,
                                    bool connected, string address);

    void onPhyUpdate(string address, int txPhy, int rxPhy, int status);

    void onPhyRead(string address, int txPhy, int rxPhy, int status);

    void onSearchComplete(string address, std::vector<GattService*> services, int status);

    void onCharacteristicRead(string address, int status, int handle, uint8_t *value, int length);

    void onCharacteristicWrite(string address, int status, int handle);

    void onExecuteWrite(string address, int status);

    void onDescriptorRead(string address, int status, int handle, uint8_t *value, int length);

    void onDescriptorWrite(string address, int status, int handle);

    void onNotify(string address, int handle, uint8_t *value, int length);

    void onReadRemoteRssi(string address, int rssi, int status);

    void onConfigureMTU(string address, int mtu, int status);

    void onConnectionUpdated(string address, int interval, int latency,
                                int timeout, int status);
};
}//namespace gatt
#endif
