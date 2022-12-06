/*
 * Copyright (c) 2018, The Linux Foundation. All rights reserved.
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

#ifndef GATT_SERVER_HPP_
#define GATT_SERVER_HPP_

#pragma once

#include "GattServerCallback.hpp"
#include "GattCharacteristic.hpp"
#include "GattDescriptor.hpp"
#include "GattClient.hpp"
#include "GattLibService.hpp"

#include "IServerCallback.hpp"
#include <vector>
#include <list>
#include <mutex>
#include <string>
#include <algorithm>
#include <condition_variable>
#include "utils/Log.h"
#include "utils/include/uuid.h"

using namespace std;
using std::string;
using btapp::Uuid;
namespace gatt{

/**
 * Public API for the Bluetooth GATT Profile server role.
 *
 * <p>This class provides Bluetooth GATT server role functionality,
 * allowing applications to create services and
 * characteristics.
 *
 */
class GattServer : public IServerCallback, public GattServerCallback {
  private:
    static const bool DBG = true;
    static const bool VDBG = true;

    GattLibService *mService = NULL;
    GattServerCallback *mCallback = NULL;
    IServerCallback *mServerCallback = NULL;

    std::condition_variable mServerCV;
    std::mutex mServerIfLock;
    int mServerIf;
    int mTransport;
    GattService *mPendingService = NULL;
    std::vector<GattService*> mServices;

    static const int CALLBACK_REG_TIMEOUT_MS = 10000;

    /**
     * Unregister the current application and callbacks.
     */
    void unregisterCallback() ;

  public:
    GattServer();
      /**
     * Create a GattServer proxy object.
     */
    GattServer(GattLibService *gatt,int transport);
      /**
      * Destructor destroy GattServer object when used.
      */
    ~GattServer();
    /**
     * Returns a characteristic with given handle.
     *
     */
    GattCharacteristic* getCharacteristicByHandle(int handle);

    /**
     * Returns a descriptor with given handle.
     *
     */
     GattDescriptor* getDescriptorByHandle(int handle);

    /**
     * Close this GATT server instance.
     *
     * Application should call this method as early as possible after it is done with
     * this GATT server.
     */
    void close();

    /**
     * Initiate a connection to a Bluetooth GATT capable device.
     *
     * <p>The connection may not be established right away, but will be
     * completed when the remote device is available. A
     * GattServerCallback#onConnectionStateChange callback will be
     * invoked when the connection state changes as a result of this function.
     *
     * <p>The autoConnect paramter determines whether to actively connect to
     * the remote device, or rather passively scan and constize the connection
     * when the remote device is in range/available. Generally, the first ever
     * connection to a device should be direct (autoConnect set to false) and
     * subsequent connections to known devices should be invoked with the
     * autoConnect parameter set to true.
     *
     * @param autoConnect Whether to directly connect to the remote device (false) or to
     * automatically connect as soon as the remote device becomes available (true).
     * @return true, if the connection attempt was initiated successfully
     */
    bool connect(string deviceAddress, bool autoConnect);

    /**
     * Disconnects an established connection, or cancels a connection attempt
     * currently in progress.
     *
     * @param device Remote device
     */
    void cancelConnection(string deviceAddress);

    /**
     * Set the preferred connection PHY for this app. Please note that this is just a
     * recommendation, whether the PHY change will happen depends on other applications peferences,
     * local and remote controller capabilities. Controller can override these settings. <p> {
     * GattServerCallback#onPhyUpdate will be triggered as a result of this call, even if
     * no PHY change happens. It is also triggered when remote device updates the PHY.
     *
     * @param device The remote device to send this response to
     * @param txPhy preferred transmitter PHY. Bitwise OR of any of
     * GattDevice#PHY_LE_1M_MASK, GattDevice#PHY_LE_2M_MASK, and GattDevice#PHY_LE_CODED_MASK.
     * @param rxPhy preferred receiver PHY. Bitwise OR of any of GattDevice#PHY_LE_1M_MASK,
     * GattDevice#PHY_LE_2M_MASK, and GattDevice#PHY_LE_CODED_MASK.
     * @param phyOptions preferred coding to use when transmitting on the LE Coded PHY. Can be one
     * of GattDevice#PHY_OPTION_NO_PREFERRED, GattDevice#PHY_OPTION_S2 or GattDevice#PHY_OPTION_S8
     */
    void setPreferredPhy(string deviceAddress, int txPhy, int rxPhy, int phyOptions);

    /**
     * Read the current transmitter PHY and receiver PHY of the connection. The values are returned
     * in GattServerCallback#onPhyRead
     *
     * @param device The remote device to send this response to
     */
     void readPhy(string deviceAddress);

    /**
     * Send a response to a read or write request to a remote device.
     *
     * <p>This function must be invoked in when a remote read/write request
     * is received by one of these callback methods:
     *
     * GattServerCallback#onCharacteristicReadRequest
     * GattServerCallback#onCharacteristicWriteRequest
     * GattServerCallback#onDescriptorReadRequest
     * GattServerCallback#onDescriptorWriteRequest
     * @param device The remote device address to send this response to
     * @param requestId The ID of the request that was received with the callback
     * @param status The status of the request to be sent to the remote devices
     * @param offset Value offset for partial read/write response
     * @param value The value of the attribute that was read/written (optional)
     */
    bool sendResponse(string deviceAddress, int requestId,
                          int status, int offset, uint8_t *value);
    /**
     * Send a response to a read or write request to a remote device.
     *
     * <p>This function must be invoked in when a remote read/write request
     * is received by one of these callback methods:
     *
     * GattServerCallback#onCharacteristicReadRequest
     * GattServerCallback#onCharacteristicWriteRequest
     * GattServerCallback#onDescriptorReadRequest
     * GattServerCallback#onDescriptorWriteRequest
     * @param device The remote device address to send this response to
     * @param requestId The ID of the request that was received with the callback
     * @param status The status of the request to be sent to the remote devices
     * @param offset Value offset for partial read/write response
     * @param value The value of the attribute that was read/written (optional)
     * @param length The value length of the attribute that was read/written (optional)
     */
    bool sendResponse(string deviceAddress, int requestId,
                          int status, int offset, uint8_t *value, int length);
    /**
     * Send a notification or indication that a local characteristic has been
     * updated.
     *
     * <p>A notification or indication is sent to the remote device to signal
     * that the characteristic has been updated. This function should be invoked
     * for every client that requests notifications/indications by writing
     * to the "Client Configuration" descriptor for the given characteristic.
     *
     * @param device The remote device address to receive the notification/indication
     * @param characteristic The local characteristic that has been updated
     * @param confirm true to request confirmation from the client (indication), false to send a
     * notification
     * @return true, if the notification has been triggered successfully
     * @throw std::invalid_argument Chracteristic value is empty
     */
    bool notifyCharacteristicChanged(string deviceAddress,
                                              GattCharacteristic &characteristic,
                                              bool confirm);

    /**
     * Add a service to the list of services to be hosted.
     *
     * <p>Once a service has been addded to the the list, the service and its
     * included characteristics will be provided by the local device.
     *
     * <p>If the local device has already exposed services when this function
     * is called, a service update notification will be sent to all clients.
     *
     * <p>The GattServerCallback#onServiceAdded callback will indicate
     * whether this service has been added successfully. Do not add another service
     * before this callback.
     *
     * @param service Service to be added to the list of services provided by this device.
     * @return true, if the request to add service has been initiated
     */
    bool addService(GattService &service);
    /**
     * Removes a service from the list of services to be provided.
     *
     * @param service Service to be removed.
     * @return true, if the service has been removed
     */
     bool removeService(GattService &service);

    /**
     * Remove all services from the list of provided services.
     */
    void clearServices();

    /**
     * Returns a list of GATT services offered by this device.
     *
     * <p>An application must call GattServer#addService to add a serice to the
     * list of services offered by this device.
     *
     * @return List of services. Returns an empty list if no services have been added yet.
     */
     std::vector<GattService*> getServices();

    /**
     * Returns a GattService from the list of services offered
     * by this device.
     *
     * <p>If multiple instances of the same service (as identified by Uuid)
     * exist, the first instance of the service is returned.
     *
     * @param uuid Uuid of the requested service
     * @return GattService if supported, or null if the requested service is not offered by
     * this device.
     */
    GattService* getService(Uuid uuid);

    /**
     * Register an application callback to start using GattServer.
     *
     * <p>The callback is used to notify
     * success or failure if the function returns true.
     *
     * @param callback GATT callback handler that will receive asynchronous callbacks.
     * @return true, the callback will be called to notify success or failure, false on immediate
     * error
     */
    bool registerCallback(GattServerCallback& callback);

    /**
     * Returns a service by Uuid, instance and type.
     *
     */
     GattService* getService(Uuid uuid, int instanceId, int type);

    /**
    *Inherted Callback
    */
    void onServerRegistered(int status, int serverIf);
    void onConnectionState(int status, int serverIf,
                                     bool connected, string address);
    void onServiceAdded(int status, GattService *service);
    void onCharacteristicReadRequest(string address, int transId, int offset,
                                         bool isLong, int handle);
    void onDescriptorReadRequest(string address, int transId,
                                        int offset, bool isLong,
                                        int handle);
    void onCharacteristicWriteRequest(string address, int transId, int offset,
                                      int length, bool isPrep, bool needRsp,
                                      int handle, uint8_t *value);
    void onDescriptorWriteRequest(string address, int transId, int offset,
                                       int length, bool isPrep, bool needRsp,
                                        int handle, uint8_t *value);
    void onExecuteWrite(string address, int transId, bool execWrite);
    void onNotificationSent(string address, int status);
    void onMtuChanged(string address, int mtu);
    void onPhyUpdate(string address, int txPhy, int rxPhy, int status);
    void onPhyRead(string address, int txPhy, int rxPhy, int status);
    void onConnectionUpdated(string address, int interval, int latency,
                                int timeout, int status);
};
}//namespace gatt
#endif
