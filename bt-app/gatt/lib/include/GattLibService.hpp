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
#ifndef GATT_LIB_SERVICE_HPP_
#define GATT_LIB_SERVICE_HPP_
#pragma once

#include <stdint.h>
#include "log.h"
#include "ipc.hpp"
#include "GattCharacteristic.hpp"
#include "GattDescriptor.hpp"
#include "GattClientCallback.hpp"
#include "GattServerCallback.hpp"
#include "HandleMap.hpp"
#include "AdvertiseData.hpp"
#include "AdvertisingSetParameters.hpp"
#include "PeriodicAdvertiseParameters.hpp"
#include "ResultStorageDescriptor.hpp"
#include "ScanCallback.hpp"
#include "ScanRecord.hpp"
#include "ScanSettings.hpp"
#include "ScanResult.hpp"
#include "ScanManager.hpp"
#include "GattDbElement.hpp"
#include "ScanFilter.hpp"
#include "ScanClient.hpp"
#include "AdvertisingSetCallback.hpp"
#include "AdvertiserManager.hpp"
#include "AdvtFilterOnFoundOnLostInfo.hpp"
#include "IServerCallback.hpp"
#include "IScannerCallback.hpp"
#include "IClientCallback.hpp"
#include "IPeriodicAdvertisingCallback.hpp"
#include "PeriodicScanManager.hpp"
#include "ContextMap.hpp"
#include "utils/include/uuid.h"
#include "GattNativeDefines.hpp"

#include <vector>
#include <map>
#include <set>
#include <list>
#include <unordered_map>
#include <unordered_set>
#include <iomanip>
#include <sstream>

using namespace std;
using std::string;
using btapp::Uuid;
namespace gatt{
class PeriodicAdvertisingCallback;
class ScanCallback;
class ScanClient;
class AdvertisingSetCallback;
class ScanManager;
class PeriodicScanManager;
/**
 * Provides Bluetooth Gatt profile, as a service in
 * the Bluetooth application.
 * @hide
 */

#define CHECK_PARAM_VOID(x)                                                   \
if (!x) {                                                                     \
    ALOGE("'%s' Void Param is NULL - exiting from function ", __FUNCTION__);  \
    return ;                                                                  \
}
class GattLibService           {
  private:
    static const bool DBG = true;
    static const bool VDBG = true;

    static const int SCAN_FILTER_ENABLED = 1;
    static const int SCAN_FILTER_MODIFIED = 2;

    static const int MAC_ADDRESS_LENGTH = 6;
    // Batch scan related constants.
    static const int TRUNCATED_RESULT_SIZE = 11;
    static const int TIME_STAMP_LENGTH = 2;

    // onFoundLost related constants
    static const int ADVT_STATE_ONFOUND = 0;
    static const int ADVT_STATE_ONLOST = 1;

    static const int ET_LEGACY_MASK = 0x10;

    static const int GATT_HIGH_PRIORITY_MIN_INTERVAL = 9;
    static const int GATT_HIGH_PRIORITY_MAX_INTERVAL = 12;
    static const int GATT_BALANCED_PRIORITY_MIN_INTERVAL = 24;
    static const int GATT_BALANCED_PRIORITY_MAX_INTERVAL =40;
    static const int GATT_LOW_POWER_MIN_INTERVAL = 80;
    static const int GATT_LOW_POWER_MAX_INTERVAL =100;
    static const int GATT_HIGH_PRIORITY_LATENCY = 0;
    static const int GATT_BALANCED_PRIORITY_LATENCY = 0;
    static const int GATT_LOW_POWER_LATENCY = 2;

    static const int CONNECTION_PRIORITY_HIGH = 1;
    static const int CONNECTION_PRIORITY_LOW_POWER = 2;
    static const int GATT_SUCCESS = 0;
    static const int GATT_CONNECTION_CONGESTED = 0x8f;

    /**
     * List of our registered scanners.
     */
    class ScannerMap : public ContextMap<IScannerCallback*> {};
    ScannerMap *mScannerMap = new ScannerMap();

    /**
     * List of our registered clients.
     */
    class ClientMap : public ContextMap<IClientCallback*> {};
    ClientMap *mClientMap = new ClientMap();
    /**
     * List of our registered server apps.
     */
    class ServerMap : public ContextMap<IServerCallback*> {};
    ServerMap *mServerMap = new ServerMap();

    /**
     * Server handle map.
     */
    HandleMap *mHandleMap = new HandleMap();;
    std::vector<Uuid> mAdvertisingServiceUuids;

    int mMaxScanFilters;

    std::unordered_map<int, std::vector<GattService*>> mGattClientDatabases;

    AdvertiserManager *mAdvertiserManager = NULL;
    ScanManager *mScanManager = NULL;
    PeriodicScanManager *mPeriodicScanManager = NULL;

    static GattLibService* volatile sGattService;
    GattNativeInterfaceV2 *mNative = NULL;
    const bt_interface_t * bluetooth_interface;
    btgatt_interface_t *gatt_interface;

    string mAddress;
    string mDeviceName;

    static const int MIN_ADVT_INSTANCES_FOR_MA = 5;
    static const int MIN_OFFLOADED_FILTERS = 10;
    static const int MIN_OFFLOADED_SCAN_STORAGE_BYTES = 1024;

    int mNumOfAdvertisementInstancesSupported;
    bool mRpaOffloadSupported;
    int mNumOfOffloadedIrkSupported;
    int mNumOfOffloadedScanFilterSupported;
    int mOffloadedScanResultStorageBytes;
    int mVersSupported;
    int mTotNumOfTrackableAdv;
    bool mIsExtendedScanSupported;
    bool mIsDebugLogSupported;
    bool mIsActivityAndEnergyReporting;
    bool mIsLe2MPhySupported;
    bool mIsLeCodedPhySupported;
    bool mIsLeExtendedAdvertisingSupported;
    bool mIsLePeriodicAdvertisingSupported;
    int mLeMaximumAdvertisingDataLength;
    /**
     * Reliable write queue
     */
    std::unordered_set<string> mReliableQueue;

    bool isRestrictedCharUuid(const Uuid charUuid);
    bool isRestrictedSrvcUuid(const Uuid srvcUuid);
    bool isHidUuid(const Uuid uuid);
    bool isFidoUUID(const Uuid uuid);
    bool permissionCheck(Uuid uuid);
    bool permissionCheck(int connId, int handle);
    void stopNextService(int serverIf, int status);
    void deleteServices(int serverIf);
    std::vector<Uuid> parseUuids(std::vector<uint8_t> advData);
    void start();
    void cleanup();
    GattLibService(const bt_interface_t *bt_interface);
    /**###################################################################
     * Handlers for incoming service calls
     *####################################################################
     */

  public:
    ~GattLibService();
    static GattLibService* getInstance(const bt_interface_t *bt_interface);
    static GattLibService* getGatt();

    void registerClient(Uuid uuid, IClientCallback& callback);
    void unregisterClient(int clientIf);
    void clientConnect(int clientIf, string address, bool isDirect, int transport,
                           bool opportunistic, int phy);
    void clientDisconnect(int clientIf, string address);
    void clientSetPreferredPhy(int clientIf, string address, int txPhy,
                                      int rxPhy, int phyOptions);
    void clientReadPhy(int clientIf, string address);
    void refreshDevice(int clientIf, string address);
    void discoverServices(int clientIf, string address);
    void discoverServiceByUuid(int clientIf, string address, Uuid uuid);
    void readCharacteristic(int clientIf, string address, int handle, int authReq);
    void readUsingCharacteristicUuid(int clientIf, string address, Uuid uuid,
                                              int startHandle, int endHandle, int authReq);
    void writeCharacteristic(int clientIf, string address, int handle, int writeType,
                                   int authReq, uint8_t *value, int valueLength);
    void readDescriptor(int clientIf, string address, int handle, int authReq);
    void writeDescriptor(int clientIf, string address, int handle, int authReq,
                             uint8_t *value, int valueLength);
    void beginReliableWrite(int clientIf, string address);
    void endReliableWrite(int clientIf, string address, bool execute);
    void registerForNotification(int clientIf, string address, int handle,
                                         bool enable);
    void readRemoteRssi(int clientIf, string address);
    void configureMTU(int clientIf, string address, int mtu);
    void connectionParameterUpdate(int clientIf, string address,
                                           int connectionPriority);
    void leConnectionUpdate(int clientIf, string address,
                                  int minConnectionInterval, int maxConnectionInterval,
                                  int slaveLatency, int supervisionTimeout,
                                  int minConnectionEventLen, int maxConnectionEventLen);
    void registerServer(Uuid uuid, IServerCallback& callback);
    void unregisterServer(int serverIf);
    void serverConnect(int serverIf, string address, bool isDirect, int transport);
    void serverDisconnect(int serverIf, string address);
    void serverSetPreferredPhy(int serverIf, string address, int txPhy, int rxPhy,
                                      int phyOptions);
    void serverReadPhy(int clientIf, string address);
    std::vector<gatt_db_element_t> fillGattDbStructureArray(std::vector<GattDbElement*> db,
                                                                    int count);
    std::vector<GattDbElement*> fillGattDbElementArray(gatt_db_element_t *db,
                                                              int count);
    void addService(int serverIf, GattService *svc);
    void removeService(int serverIf, int handle);
    void clearServices(int serverIf);
    void sendResponse(int serverIf, string address, int requestId, int status,
            int offset, uint8_t *value);
    void sendResponse(int serverIf, string address, int requestId, int status,
            int offset, uint8_t *value, int valueLength);
    void sendNotification(int serverIf, string address, int handle, bool confirm,
            uint8_t *value, int valueLength);
    void startAdvertisingSet(AdvertisingSetParameters *parameters,
                                    AdvertiseData *advertiseData, AdvertiseData *scanResponse,
                                    PeriodicAdvertiseParameters *periodicParameters,
                                    AdvertiseData *periodicData, int duration,
                                    int maxExtAdvEvents, IAdvertisingSetCallback *callback);
    void stopAdvertisingSet(IAdvertisingSetCallback *callback);
    void getOwnAddress(int advertiserId);
    void enableAdvertisingSet(int advertiserId, bool enable, int duration,
                                    int maxExtAdvEvents);
    void setAdvertisingData(int advertiserId, AdvertiseData *data);
    void setScanResponseData(int advertiserId, AdvertiseData *data);
    void setAdvertisingParameters(int advertiserId,
                                          AdvertisingSetParameters *parameters);
    void setPeriodicAdvertisingParameters(int advertiserId,
                                                  PeriodicAdvertiseParameters *parameters);
    void setPeriodicAdvertisingData(int advertiserId, AdvertiseData *data);
    void setPeriodicAdvertisingEnable(int advertiserId, bool enable);
    void registerSync(ScanResult *scanResult, int skip, int timeout,
                          IPeriodicAdvertisingCallback *callback);
    void unregisterSync(IPeriodicAdvertisingCallback *callback);
    void registerScanner(IScannerCallback *callback);
    void unregisterScanner(int scannerId);
    void startScan(int scannerId, ScanSettings *settings, std::vector<ScanFilter*> filters,
                      std::vector<std::vector<ResultStorageDescriptor*>>storages);
    void stopScan(int scannerId);
    void stopScan(ScanClient *client);
    void flushPendingBatchResults(int scannerId);
    void disconnectAll();
    bool isScanClient(int clientIf);
    void unregAll();
    int numHwTrackFiltersAvailable();
    string getAddress();
    string getDeviceName();
    bool isMultipleAdvertisementSupported();
    int getNumOfAdvertisementInstancesSupported();
    bool isRpaOffloadSupported();
    int getNumOfOffloadedIrkSupported();
    bool isOffloadedScanBatchingSupported();
    int getOffloadedScanResultStorage();
    bool isActivityAndEnergyReportingSupported();
    bool isLe2MPhySupported();
    bool isLeCodedPhySupported();
    bool isLeExtendedAdvertisingSupported();
    bool isLePeriodicAdvertisingSupported();
    int getLeMaximumAdvertisingDataLength();
    int getTotalNumOfTrackableAdvertisements();
    bool isOffloadedFilteringSupported();
    int getNumOfOffloadedScanFilterSupported();
    /**************************************************************************
     * Callback functions - CLIENT
     *************************************************************************/
  private:

    // Check if a scan record matches a specific filters.
    bool matchesFilters(ScanClient *client, ScanResult *scanResult);
    void sendBatchScanResults(ScannerMap::App *app, ScanClient *client,
                                    std::vector<ScanResult*> results);
    // Check and deliver scan results for different scan clients.
    void deliverBatchScan(ScanClient *client, std::unordered_set<ScanResult*> allResults);
    std::unordered_set<ScanResult*> parseBatchScanResults(int numRecords, int reportType,
                                                                 std::vector<uint8_t> batchRecord);
    std::unordered_set<ScanResult*> parseTruncatedResults(int numRecords,
                                                                 std::vector<uint8_t> batchRecord);
    long parseTimestampNanos(uint8_t * data);
    std::unordered_set<ScanResult*> parseFullResults(int numRecords,
                                                          std::vector<uint8_t> batchRecord);
    // Reverse byte array.
    void reverse(std::vector<uint8_t> &address);
    // Helper method to extract bytes from byte array.
    std::vector<uint8_t> extractBytes(std::vector<uint8_t> scanRecord, int start, int length);
    void setAddress(void *val, int len);
    void setDeviceName(void *val, int len);
    void updateFeatureSupport(void *value, int len);

    std::unordered_set<ScanClient*> mScanClients;
    ScanClient* getScanClientbyScanId(int scannerId);

  public:
    void onScanResult(int eventType, int addressType, string address, int primaryPhy,
            int secondaryPhy, int advertisingSid, int txPower, int rssi, int periodicAdvInt,
            std::vector<uint8_t> advData);
    void onScannerRegistered(int status, int scannerId, Uuid uuid);
    void onClientRegistered(int status, int clientIf, Uuid uuid);
    void onConnected(int clientIf, int connId, int status, string address);
    void onDisconnected(int clientIf, int connId, int status, string address);
    void onClientPhyUpdate(int connId, int txPhy, int rxPhy, int status);
    void onClientPhyRead(int clientIf, string address, int txPhy, int rxPhy, int status);
    void onClientConnUpdate(int connId, int interval, int latency, int timeout, int status);
    void onServerPhyUpdate(int connId, int txPhy, int rxPhy, int status);
    void onServerPhyRead(int serverIf, string address, int txPhy, int rxPhy, int status);
    void onServerConnUpdate(int connId, int interval, int latency, int timeout, int status);
    void onSearchCompleted(int connId, int status);
    GattDbElement* getSampleGattDbElement();
    void onGetGattDb(int connId, std::vector<GattDbElement*> db);
    void onRegisterForNotifications(int connId, int status, int registered, int handle);
    void onNotify(int connId, string address, int handle, bool isNotify, uint8_t *data, int length);
    void onReadCharacteristic(int connId, int status, int handle, uint8_t *data, int length);
    void onWriteCharacteristic(int connId, int status, int handle);
    void onExecuteCompleted(int connId, int status);
    void onReadDescriptor(int connId, int status, int handle, uint8_t *data, int length);
    void onWriteDescriptor(int connId, int status, int handle);
    void onReadRemoteRssi(int clientIf, string address, int rssi, int status);
    void onScanFilterEnableDisabled(int action, int status, int clientIf);
    void onScanFilterParamsConfigured(int action, int status, int clientIf, int availableSpace);
    void onScanFilterConfig(int action, int status, int clientIf, int filterType,
            int availableSpace);
    void onBatchScanStorageConfigured(int status, int clientIf);
    void onBatchScanStarted(int status, int clientIf);
    void onBatchScanStopped(int status, int clientIf);
    void onBatchScanReports(int status, int scannerId, int reportType, int numRecords,
            std::vector<uint8_t> recordData);
    void onBatchScanThresholdCrossed(int clientIf);
    AdvtFilterOnFoundOnLostInfo* createOnTrackAdvFoundLostObject(int clientIf, int advPktLen,
                                                                     std::vector<uint8_t> advPkt,
                                                                     int scanRspLen,
                                                                     std::vector<uint8_t> scanRsp,
                                                                     int filtIndex, int advState,
                                                                     int advInfoPresent,
                                                                     string address,
                                                                     int addrType, int txPower,
                                                                     int rssiValue, int timeStamp);
    void onTrackAdvFoundLost(AdvtFilterOnFoundOnLostInfo *trackingInfo);
    void onScanParamSetupCompleted(int status, int scannerId);
    // callback from ScanManager for dispatch of errors apps.
    void onScanManagerErrorCallback(int scannerId, int errorCode);
    void onConfigureMTU(int connId, int status, int mtu);
    void onClientCongestion(int connId, bool congested);
    void onServerRegistered(int status, int serverIf, Uuid uuid);
    void onServiceAdded(int status, int serverIf, std::vector<GattDbElement*> service);
    void onServiceStopped(int status, int serverIf, int srvcHandle);
    void onServiceDeleted(int status, int serverIf, int srvcHandle);
    void onClientConnected(string address, bool connected, int connId, int serverIf);
    void onServerReadCharacteristic(string address, int connId, int transId, int handle, int offset,
            bool isLong);
    void onServerReadDescriptor(string address, int connId, int transId, int handle, int offset,
            bool isLong);
    void onServerWriteCharacteristic(string address, int connId, int transId, int handle,
            int offset, int length, bool needRsp, bool isPrep, uint8_t *data);
    void onServerWriteDescriptor(string address, int connId, int transId, int handle, int offset,
            int length, bool needRsp, bool isPrep, uint8_t *data);
    void onExecuteWrite(string address, int connId, int transId, int execWrite);
    void onResponseSendCompleted(int status, int attrHandle);
    void onNotificationSent(int connId, int status);
    void onServerCongestion(int connId, bool congested);
    void onMtuChanged(int connId, int mtu);

    /**********************************************************************
    * HANDLE CALLBACK FROM JNI
    ***********************************************************************/
     /**

     * @brief ProcessEvent
     *
     * It will handle incomming events
     *
     * @param BtEvent
     * @return none
     */
    void ProcessEvent(BtEvent* event);
    void HandleGattcRegisterAppEvent(GattcRegisterAppEvent *event);
    void HandleGattcOpenEvent(GattcOpenEvent *event);
    void HandleGattcCloseEvent(GattcCloseEvent *event);
    void HandleGattcSearchCompleteEvent(GattcSearchCompleteEvent *event);
    void HandleGattcRegisterForNotificationEvent(GattcRegisterForNotificationEvent *event);
    void HandleGattcNotifyEvent(GattcNotifyEvent *event);
    void HandleGattcReadCharacteristicEvent(GattcReadCharacteristicEvent *event);
    void HandleGattcWriteCharacterisitcEvent(GattcWriteCharacteristicEvent *event);
    void HandleGattcExecuteWriteEvent(GattcExecuteWriteEvent *event);
    void HandleGattcReadDescriptorEvent(GattcReadDescriptorEvent *event);
    void HandleGattcWriteDescriptorEvent(GattcWriteDescriptorEvent *event);
    void HandleGattcRemoteRssiEvent(GattcRemoteRssiEvent *event);
    void HandleGattcConfigureMtuEvent(GattcConfigureMtuEvent *event);
    void HandleGattcCongestionEvent(GattcCongestionEvent *event);
    void HandleGattcGetGattDbEvent(GattcGetGattDbEvent *event);
    void HandleGattcPhyUpdatedEvent(GattcPhyUpdatedEvent *event);
    void HandleGattcConnUpdatedEvent(GattcConnUpdatedEvent *event);
    void HandleGattcReadPhyEvent(GattcReadPhyEvent *event);
    void HandleGattsRegisterAppEvent(GattsRegisterAppEvent *event);
    void HandleGattsConnectionEvent(GattsConnectionEvent *event);
    void HandleGattsServiceAddedEvent(GattsServiceAddedEvent *event);
    void HandleGattsServiceStoppedEvent(GattsServiceStoppedEvent *event);
    void HandleGattsServiceDeletedEvent(GattsServiceDeletedEvent *event);
    void HandleGattsRequestReadCharacteristicEvent(GattsRequestReadCharacteristicEvent *event);
    void HandleGattsRequestReadDescriptorEvent(GattsRequestReadDescriptorEvent *event);
    void HandleGattsRequestWriteCharacteristicEvent(GattsRequestWriteCharacteristicEvent *event);
    void HandleGattsRequestWriteDescriptorEvent(GattsRequestWriteDescriptorEvent *event);
    void HandleGattsRequestExecWriteEvent(GattsRequestExecWriteEvent *event);
    void HandleGattsResponseConfirmationEvent(GattsResponseConfirmationEvent *event);
    void HandleGattsIndicationSentEvent(GattsIndicationSentEvent *event);
    void HandleGattsCongestionEvent(GattsCongestionEvent *event);
    void HandleGattsMtuChangedEvent(GattsMTUchangedEvent *event);
    void HandleGattsPhyUpdatedEvent(GattsPhyUpdatedEvent *event);
    void HandleGattsConnUpdatedEvent(GattsConnUpdatedEvent *event);
    void HandleGattsReadPhyEvent(GattsReadPhyEvent *event);
    void HandleBleScannerRegisterScannerEvent(BleScannerRegisterScannerEvent *event);
    void HandleBleScannerScanParamsCompleteEvent(BleScannerScanParamCompleteEvent *event);
    void HandleBleScannerScanResultEvent(BleScannerScanResultEvent *event);
    void HandleBleScannerBatchScanReportsEvent(BleScannerBatchscanReportsEvent *event);
    void HandleBleScannerBatchScanThresholdEvent(BleScannerBatchscanThresholdEvent *event);
    void HandleBleScannerTrackAdvEvent(BleScannerTrackAdvEvent *event);
    void HandleBleScannerScanFilterCfgEvent(BleScannerScanFilterCfgEvent *event);
    void HandleBleScannerScanFilterParamEvent(BleScannerScanFilterParamEvent *event);
    void HandleBleScannerScanFilterStatusEvent(BleScannerScanFilterStatusEvent *event);
    void HandleBleScannerBatchScanCfgStorageEvent(BleScannerBatchscanCfgStorageEvent *event);
    void HandleBleScannerBatchscanStartEvent(BleScannerBatchscanStartEvent *event);
    void HandleBleScannerBatchscanStopEvent(BleScannerBatchscanStopEvent *event);
    void HandleBleScannerPeriodicAdvertisingSyncStartEvent
                                                (BleScannerPeriodicAdvSyncStartEvent *event);
    void HandleBleScannerPeriodicAdvertsiingSyncLostEvent
                                                (BleScannerPeriodicAdvSyncLostEvent *event);
    void HandleBleScannerPeriodicAdvertisingSyncReportEvent
                                                (BleScannerPeriodicAdvSyncReportEvent *event);
    void HandleBleAdvertiserSetAdvertisingDataEvent(BleAdvertiserSetAdvDataEvent *event);
    void HandleBleAdvertiserSetScanResponseDataEvent(BleAdvertiserSetScanRespEvent *event);
    void HandleBleAdvertiserSetPeriodicAdvertisingParameterEvent
                                                (BleAdvertiserSetPeriodicAdvParamEvent *event);
    void HandleBleAdvertiserSetPeriodicAdvertisingDataEvent
                                                 (BleAdvertiserSetPeriodicAdvDataEvent *event);
    void HandleBleAdvertiserGetOwnAddressEvent(BleAdvertiserGetOwnAddressEvent *event);
    void HandleBleAdvertiserAdvSetStartEvent(BleAdvertiserAdvSetStartEvent *event);
    void HandleBleAdvertiserAdvSetEnableEvent(BleAdvertiserAdvSetEnableEvent *event);
    void HandleBleAdvertiserAdvParameterUpdatedEvent(BleAdvertiserAdvSetParamUpdateEvent *event);
    void HandleBleAdvertiserPeriodicAdvSetEnableEvent
                                                (BleAdvertiserPeriodicAdvSetEnableEvent *event);
    void HandleBleScannerPeriodicAdvSyncStartEvent(BleScannerPeriodicAdvSyncStartEvent *event);
    void HandleBleScannerPeriodicAdvSyncLostEvent(BleScannerPeriodicAdvSyncLostEvent *event);
    void HandleBleScannerPeriodicAdvSyncReportEvent(BleScannerPeriodicAdvSyncReportEvent *event);
    void HandleGattAdapterPropertyEvent(GattAdapterPropertyEvent *event);
    void HandleBleBatchScanTimeoutEvent(BleScannerBatchscantimeoutEvent *event);
};
}//namespace gatt
#endif
