/*
* Copyright (c) 2021, The Linux Foundation. All rights reserved.
*
* Redistribution and use in source and binary forms, with or without
* modification, are permitted provided that the following conditions are
* met:
*     * Redistributions of source code must retain the above copyright
*       notice, this list of conditions and the following disclaimer.
*     * Redistributions in binary form must reproduce the above
*       copyright notice, this list of conditions and the following
*       disclaimer in the documentation and/or other materials provided
*       with the distribution.
*     * Neither the name of The Linux Foundation nor the names of its
*       contributors may be used to endorse or promote products derived
*       from this software without specific prior written permission.
*
* THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESS OR IMPLIED
* WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT
* ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS
* BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
* CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
* SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
* BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
* WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
* OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
* IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include "BleWifiControlService.hpp"
#include "BleSocketManager.hpp"
#include "uuid.h"
#include "GattClientCallback.hpp"
#include "uuid.h"
#include "ScanSettings.hpp"
#include "ScanCallback.hpp"
#include "GattLeScanner.hpp"

#define LOGTAG "BleWifiControlService: "


extern BleWifiControlService *g_ble_wifi_control_service;
extern BleSocketManager *g_ble_socket_manager;
extern GattLibService *g_gatt;

class WCSScannerCallback : public ScanCallback
{
  public:
    void onScanResult(int callbackType, ScanResult *result)
    {
      if (g_ble_wifi_control_service == NULL) {
        ALOGW(LOGTAG "%s:  g_ble_wifi_control_service is NULL", __FUNCTION__);
        return;
      }

      ScanRecord *sr = result->getScanRecord();
      std::vector<uint8_t> sData =
          sr->getServiceData(g_ble_wifi_control_service->service_data_uuid_);
      int advertise_flags = sr->getAdvertiseFlags();
      string bd_addr = result->getDevice();
      ALOGD(LOGTAG "The scanned device is %s", bd_addr.c_str());
      uint8_t service_data_len = sData.size();
      if (service_data_len <= MAX_SERVICE_DATA_LEN) {
        uint16_t service_data_uuid = g_ble_wifi_control_service->service_data_uuid_.As16Bit();
        uint8_t service_data[MAX_SERVICE_DATA_LEN];
        std::copy(sData.begin(), sData.end(), service_data);
        g_ble_wifi_control_service->PeerDiscoveryResult(advertise_flags,service_data_uuid,
            service_data_len, service_data,bd_addr);
      }
    }

    void onScanFailed (int errorCode)
    {
      ALOGE(LOGTAG "%s:  Scan Failed due to error %d", __FUNCTION__, errorCode);
      g_ble_wifi_control_service->PeerDiscoveryEnableRsp(BLE_IPC_STATUS_FAILED);
    }
};

class WCSClientCallback:public GattClientCallback
{
  public:
    void onConnectionStateChange(GattClient *gatt, int status,int newState) {
      ALOGD(LOGTAG "%s: status: %d, newState: %d , bdaddr = %s",
          __FUNCTION__, status, newState, gatt->getDeviceAddress().c_str());

      if (g_ble_wifi_control_service == NULL) {
        ALOGW(LOGTAG "%s:  g_ble_wifi_control_service is NULL", __FUNCTION__);
        return;
      }

      if (status == GattClient::GATT_SUCCESS) {
        if (newState == GattDevice::STATE_CONNECTED){
          g_ble_wifi_control_service->conn_state_ = GattDevice::STATE_CONNECTED;
          gatt->requestMtu(64);
        } else if(newState == GattDevice::STATE_DISCONNECTED) {
         if (g_ble_wifi_control_service->conn_state_ == GattDevice::STATE_DISCONNECTING) {
            g_ble_wifi_control_service->DisconnectPeerRsp(gatt->getDeviceAddress(),
                BLE_IPC_STATUS_SUCCESS);
          } else if(g_ble_wifi_control_service->conn_state_ ==  GattDevice::STATE_CONNECTED) {
             g_ble_wifi_control_service->ConnectPeerRsp(gatt->getDeviceAddress(),
               BLE_IPC_STATUS_FAILED);
          }
        }
      } else {
        if (newState == GattDevice::STATE_DISCONNECTED) {
          if (g_ble_wifi_control_service->conn_state_ == GattDevice::STATE_CONNECTING) {
            g_ble_wifi_control_service->ConnectPeerRsp(gatt->getDeviceAddress(),
                BLE_IPC_STATUS_FAILED);
          } else {
            g_ble_wifi_control_service ->DisconnectPeerInd(gatt->getDeviceAddress(),
            BLE_IPC_STATUS_WCS_PEER_ABNORMALLY_DISCONNECTED);
          }
        }
      }
   }

    void onMtuChanged(GattClient *gatt, int mtu, int status) {
      ALOGD(LOGTAG "%s: status: %d, mtu: %d", __FUNCTION__, status, mtu);
      if (status == GattClient::GATT_SUCCESS) {
        g_ble_wifi_control_service ->ConnectPeerRsp(gatt->getDeviceAddress(),
          BLE_IPC_STATUS_SUCCESS);
      } else {
        //as per the design, Disconnect the link, if MTU change is failed
        gatt->disconnect();
        g_ble_wifi_control_service ->ConnectPeerRsp(gatt->getDeviceAddress(),
          BLE_IPC_STATUS_FAILED);
      }
    }

    void onConnectionUpdated(GattClient *gatt, int interval, int latency,
                                  int timeout, int status) {
      ALOGD(LOGTAG "%s: status: %d, interval: %d latency: %d, timeout: %d",
          __FUNCTION__, status, interval, latency, timeout);
    }
};

class WCSServerCallback  :public GattServerCallback{
  public:
  void onServiceAdded(int status,GattService *service) {
    ALOGD(LOGTAG" %s status: %d", __FUNCTION__, status);
    if (status == GattClient::GATT_SUCCESS) {
      g_ble_wifi_control_service->is_wcs_added_ = true;
    }
  }

  void onCharacteristicReadRequest(string deviceAddress, int requestId,
                              int offset, GattCharacteristic *characteristic) {
    ALOGD(LOGTAG" %s", __FUNCTION__);
    g_ble_wifi_control_service->CharacteristicReadReq(deviceAddress,requestId,
        offset, characteristic);
  }

  void onCharacteristicWriteRequest(string deviceAddress,int requestId,
        GattCharacteristic *characteristic,bool preparedWrite,bool responseNeeded,
        int offset, uint8_t* value, int length) {
    ALOGD(LOGTAG" %s", __FUNCTION__);
    g_ble_wifi_control_service->CharacteristicWriteReq(deviceAddress, requestId, characteristic,
        preparedWrite,responseNeeded, offset, value, length);

    delete value;
  }

  void onDescriptorReadRequest(string deviceAddress, int requestId,
                                       int offset, GattDescriptor *descriptor) {
    ALOGD(LOGTAG" %s" ,__FUNCTION__);
    Uuid uuid = descriptor->getUuid();

    if (uuid == g_ble_wifi_control_service->CCCD_UUID)
      g_ble_wifi_control_service->CCCDReadReq(deviceAddress, requestId, offset);
  }

  void onDescriptorWriteRequest(string deviceAddress, int requestId,
                           GattDescriptor *descriptor,bool preparedWrite,
                           bool responseNeeded, int offset, uint8_t* value, int length) {
    ALOGD(LOGTAG" %s", __FUNCTION__);
    Uuid uuid = descriptor->getUuid();

    if (uuid == g_ble_wifi_control_service->CCCD_UUID)
      g_ble_wifi_control_service->CCCDWriteReq(deviceAddress, requestId,
          responseNeeded, offset,*value);

    delete value;
  }

  void onExecuteWrite(string deviceAddress, int requestId, bool execute) {
    ALOGD(LOGTAG"%s", __FUNCTION__);
  }

  void onNotificationSent(string deviceAddress, int status) {
    ALOGD(LOGTAG"%s", __FUNCTION__);
    g_ble_wifi_control_service->SendNotificationRsp(deviceAddress,
        (status == GattClient::GATT_SUCCESS) ? BLE_IPC_STATUS_SUCCESS : BLE_IPC_STATUS_FAILED);
  }
};

static WCSServerCallback mWCSServerCb;
static WCSClientCallback mWCSClientCb;
static WCSScannerCallback mWCSScannerCb;

BleWifiControlService :: BleWifiControlService() {
  is_wcs_added_ = false;
  is_discovery_enabled_ = false;
  conn_state_ = GattDevice::STATE_DISCONNECTED;
}

BleWifiControlService :: ~BleWifiControlService() {
  is_wcs_added_ = false;
  is_discovery_enabled_ = false;
  conn_state_ = GattDevice::STATE_DISCONNECTED;
}

void BleWifiControlService :: AddWCS() {
  ALOGE(LOGTAG "%s",__FUNCTION__);
  mServer = new GattServer(g_gatt, GattDevice::TRANSPORT_LE);
  mScanner = GattLeScanner::getGattLeScanner();

  mServer->registerCallback(mWCSServerCb);

  mService  = new GattService(WCS_UUID, GattService::SERVICE_TYPE_PRIMARY);
  mCCCDDescriptor = new GattDescriptor(CCCD_UUID, GattCharacteristic::PERMISSION_READ
      | GattCharacteristic::PERMISSION_WRITE);

  mWifiStateCharacteristic = new GattCharacteristic(WIFI_STATE_UUID,
      GattCharacteristic::PROPERTY_READ | GattCharacteristic::PROPERTY_WRITE
      | GattCharacteristic::PROPERTY_NOTIFY, GattCharacteristic::PERMISSION_READ
      | GattCharacteristic::PERMISSION_WRITE);
  mWifiStateCharacteristic->addDescriptor(mCCCDDescriptor);
  mDeviceIdCharacteristic = new GattCharacteristic(DEVICE_ID_UUID,
      GattCharacteristic::PROPERTY_READ | GattCharacteristic::PROPERTY_WRITE,
      GattCharacteristic::PERMISSION_READ
      | GattCharacteristic::PERMISSION_WRITE);

  mPAPConfigIDCharacteristic = new GattCharacteristic(PAP_CONFIG_ID_UUID,
      GattCharacteristic::PROPERTY_READ | GattCharacteristic::PROPERTY_WRITE,
      GattCharacteristic::PERMISSION_READ
      | GattCharacteristic::PERMISSION_WRITE);

  mPAPSSIDCharacteristic = new GattCharacteristic(PAP_SSID_UUID,
      GattCharacteristic::PROPERTY_READ | GattCharacteristic::PROPERTY_WRITE,
      GattCharacteristic::PERMISSION_READ
      | GattCharacteristic::PERMISSION_WRITE);

  mPAPBSSIDCharacteristic = new GattCharacteristic(PAP_BSSID_UUID,
      GattCharacteristic::PROPERTY_READ | GattCharacteristic::PROPERTY_WRITE,
      GattCharacteristic::PERMISSION_READ
      | GattCharacteristic::PERMISSION_WRITE);

  mPAPCountryStringCharacteristic = new GattCharacteristic(PAP_COUNTRY_STRING_UUID,
      GattCharacteristic::PROPERTY_READ | GattCharacteristic::PROPERTY_WRITE,
      GattCharacteristic::PERMISSION_READ
      | GattCharacteristic::PERMISSION_WRITE);

  mPAPOperatingClassCharacteristic= new GattCharacteristic(PAP_OPERATING_CLASS_UUID,
      GattCharacteristic::PROPERTY_READ | GattCharacteristic::PROPERTY_WRITE,
      GattCharacteristic::PERMISSION_READ
      | GattCharacteristic::PERMISSION_WRITE);

  mPAPChannelNumberCharacteristic = new GattCharacteristic(PAP_CHANNEL_NUMBER_UUID,
      GattCharacteristic::PROPERTY_READ | GattCharacteristic::PROPERTY_WRITE,
      GattCharacteristic::PERMISSION_READ
      | GattCharacteristic::PERMISSION_WRITE);

  mService->addCharacteristic(mWifiStateCharacteristic);
  mService->addCharacteristic(mDeviceIdCharacteristic);
  mService->addCharacteristic(mPAPConfigIDCharacteristic);
  mService->addCharacteristic(mPAPSSIDCharacteristic);
  mService->addCharacteristic(mPAPBSSIDCharacteristic);
  mService->addCharacteristic(mPAPCountryStringCharacteristic);
  mService->addCharacteristic(mPAPOperatingClassCharacteristic);
  mService->addCharacteristic(mPAPChannelNumberCharacteristic);
  mServer->addService(*mService);
}

void BleWifiControlService :: RemoveWCS() {
  ALOGD(LOGTAG "%s ", __FUNCTION__);
  mServer->close();
  delete mServer;
  mServer = NULL;
  mService = NULL;
  mCCCDDescriptor = NULL;
  mWifiStateCharacteristic = NULL;
  mPAPConfigIDCharacteristic = NULL;
  mPAPSSIDCharacteristic = NULL;
  mPAPBSSIDCharacteristic = NULL;
  mPAPCountryStringCharacteristic = NULL;
  mPAPOperatingClassCharacteristic = NULL;
  mPAPChannelNumberCharacteristic = NULL;
  is_wcs_added_ = false;
}

bool BleWifiControlService :: isWCSRegistered() {
  return is_wcs_added_;
}

bool BleWifiControlService :: isPeerDiscoveryEnabled() {
  return is_discovery_enabled_;
}

bool BleWifiControlService :: isPeerDeviceConnectedOrConnecting() {
  return ((conn_state_ == GattDevice::STATE_CONNECTED)
          || (conn_state_ == GattDevice::STATE_CONNECTING));
}

void BleWifiControlService :: PeerDiscoveryEnableReq(WCSPeerDiscoveryEnableReqEvent *evt) {

  ALOGD(LOGTAG "%s : service_data_len: %d, service_data_mask_len: %d", __FUNCTION__,
      evt->service_data_len, evt->service_data_mask_len);

  if (evt->service_data_len > MAX_SERVICE_DATA_SCAN_FILTER_LEN
      || evt->service_data_len > MAX_SERVICE_DATA_SCAN_FILTER_LEN)
  {
    PeerDiscoveryEnableRsp(BLE_IPC_STATUS_WCS_PEER_DISCOVERY_SERVICE_DATA_SCAN_FILTER_TOO_BIG);
    return;
  } else if (isPeerDiscoveryEnabled()) {
    PeerDiscoveryEnableRsp(BLE_IPC_STATUS_WCS_PEER_DISCOVERY_ALREADY_ENABLED);
    return;
  }

  if (!isWCSRegistered()) {
    AddWCS();
  }

  std::vector<uint8_t> service_data(evt->service_data,
                                    (evt->service_data) + (evt->service_data_len));
  std::vector<uint8_t> service_data_mask(evt->service_data_mask,
      (evt->service_data_mask) + (evt->service_data_mask_len));
  service_data_uuid_ = Uuid::From16Bit(evt->service_data_uuid);

  ScanSettings *settings = ScanSettings::Builder()
                            .setScanMode(ScanSettings::SCAN_MODE_LOW_LATENCY)
                            .setLegacy(false).build();
  ScanFilter *filter =
      ScanFilter::Builder().setServiceData(service_data_uuid_,
                                           service_data, service_data_mask).build();
  vector < ScanFilter*> filters;

  filters.push_back(filter);
  mScanner->startScan(filters, settings, &mWCSScannerCb);
  is_discovery_enabled_ = true;
  PeerDiscoveryEnableRsp(BLE_IPC_STATUS_SUCCESS);
}

void BleWifiControlService :: PeerDiscoveryEnableRsp(BleIpcStatus status) {
  ble_ipc_msg_t ipc_msg;

  ALOGD(LOGTAG "%s : status: %d", __FUNCTION__, status);

  if (status == BLE_IPC_STATUS_FAILED)
    is_discovery_enabled_ = false;

  ipc_msg.eventId = BLE_IPC_MSG_WCS_PEER_DISCOVERY_ENABLE_RSP;
  ipc_msg.wcsPeerDiscoveryEnableRspEvent.status = status;
  g_ble_socket_manager->WBDSSocketWriteHandler(&ipc_msg);
}

void BleWifiControlService :: PeerDiscoveryDisableReq() {
  ALOGD(LOGTAG "%s ", __FUNCTION__);

  if (!isWCSRegistered()) {
    PeerDiscoveryDisableRsp(BLE_IPC_STATUS_WCS_NOT_REGISTERED);
    return;
  }

  if (!isPeerDiscoveryEnabled()) {
    PeerDiscoveryDisableRsp(BLE_IPC_STATUS_WCS_PEER_DISCOVERY_ALREADY_DISABLED);
  } else {
    mScanner->stopScan(&mWCSScannerCb);
    PeerDiscoveryDisableRsp(BLE_IPC_STATUS_SUCCESS);
  }
}

void BleWifiControlService :: PeerDiscoveryDisableRsp(BleIpcStatus status) {
  ble_ipc_msg_t ipc_msg;

  ALOGD(LOGTAG "%s : status: %d", __FUNCTION__, status);

  is_discovery_enabled_ = false;

  ipc_msg.eventId = BLE_IPC_MSG_WCS_PEER_DISCOVERY_DISABLE_RSP;
  ipc_msg.wcsPeerDiscoveryDisableRspEvent.status = status;
  g_ble_socket_manager->WBDSSocketWriteHandler(&ipc_msg);
}

void BleWifiControlService :: PeerDiscoveryResult(int advertise_flags,
                                                      uint16_t service_data_uuid,
                                                      uint8_t service_data_len,
                                                      uint8_t *service_data,
                                                      string bdaddr) {
  ble_ipc_msg_t ipc_msg;

  ALOGD(LOGTAG "%s: bdaddr: %s", __FUNCTION__, bdaddr.c_str());
  ipc_msg.eventId = BLE_IPC_MSG_WCS_PEER_DISCOVERY_RESULT;
  ipc_msg.wcsPeerDiscoveryResultEvent.advertise_flags = advertise_flags;
  ipc_msg.wcsPeerDiscoveryResultEvent.service_data_uuid = service_data_uuid;
  ipc_msg.wcsPeerDiscoveryResultEvent.service_data_len = service_data_len;
  memcpy(ipc_msg.wcsPeerDiscoveryResultEvent.service_data, service_data, service_data_len);
  strlcpy(ipc_msg.wcsPeerDiscoveryResultEvent.bd_addr, bdaddr.c_str(), BD_ADDR_STR_LEN);
  g_ble_socket_manager->WBDSSocketWriteHandler(&ipc_msg);

}

void BleWifiControlService :: ConnectPeerReq(WCSConnectPeerReqEvent *evt) {
  string bdaddr(evt->bd_addr);

  ALOGD(LOGTAG "%s: bdaddr: %s", __FUNCTION__, bdaddr.c_str());

  if (!isWCSRegistered()) {
    ConnectPeerRsp(bdaddr, BLE_IPC_STATUS_WCS_NOT_REGISTERED);
    return;
  }

  mClient = new GattClient(g_gatt, bdaddr,
      GattDevice::TRANSPORT_LE, false, GattDevice::PHY_LE_1M);
  bool status = mClient->connect(false, mWCSClientCb);

  if (status) {
    conn_state_ = GattDevice::STATE_CONNECTING;
    peer_device_addr_ = bdaddr;
  } else {
    ConnectPeerRsp(bdaddr, BLE_IPC_STATUS_FAILED);
    delete mClient;
  }
}

void BleWifiControlService :: ConnectPeerRsp(string bdaddr, BleIpcStatus status) {
  ble_ipc_msg_t ipc_msg;

  ALOGD(LOGTAG "%s: bdaddr: %s, status: %d", __FUNCTION__, bdaddr.c_str(), status);

  if (status == BLE_IPC_STATUS_SUCCESS) {
    conn_state_ = GattDevice::STATE_CONNECTED;
  } else {
    conn_state_ = GattDevice::STATE_DISCONNECTED;
  }

  ipc_msg.eventId = BLE_IPC_MSG_WCS_CONNECT_PEER_RSP;
  ipc_msg.wcsConnectPeerRspEvent.status = status;
  strlcpy(ipc_msg.wcsConnectPeerRspEvent.bd_addr, bdaddr.c_str(), BD_ADDR_STR_LEN);
  g_ble_socket_manager->WBDSSocketWriteHandler(&ipc_msg);
}

void BleWifiControlService :: DisconnectPeerReq(WCSDisconnectPeerReqEvent *evt) {
  string bdaddr(evt->bd_addr);

  ALOGD(LOGTAG "%s: bdaddr: %s", __FUNCTION__, bdaddr.c_str());

  if (!isWCSRegistered()) {
    DisconnectPeerRsp(bdaddr, BLE_IPC_STATUS_WCS_NOT_REGISTERED);
    return;
  }

  if (mClient != NULL && isPeerDeviceConnectedOrConnecting()) {
    conn_state_ = GattDevice::STATE_DISCONNECTING;
    mClient->disconnect();
  } else {
    DisconnectPeerRsp(bdaddr, BLE_IPC_STATUS_WCS_PEER_ALREADY_DISCONNECTED);
  }
}

void BleWifiControlService :: DisconnectPeerRsp(string bdaddr, BleIpcStatus status) {
  ble_ipc_msg_t ipc_msg;

  ALOGD(LOGTAG "%s: bdaddr: %s, status: %d", __FUNCTION__, bdaddr.c_str(), status);

  mClient->close();

  peer_device_addr_ = "";
  conn_state_ = GattDevice::STATE_DISCONNECTED;
  ipc_msg.eventId = BLE_IPC_MSG_WCS_DISCONNECT_PEER_RSP;
  ipc_msg.wcsDisconnectPeerRspEvent.status = status;
  strlcpy(ipc_msg.wcsDisconnectPeerRspEvent.bd_addr, bdaddr.c_str(), BD_ADDR_STR_LEN);
  g_ble_socket_manager->WBDSSocketWriteHandler(&ipc_msg);

  if (g_ble_socket_manager->cleanup_due_to_deinit_) {
    std::unique_lock<std::mutex> lck(g_ble_socket_manager->socket_manager_cleanup_lock_);
    g_ble_socket_manager->socket_manager_cleanup_.notify_all();
    ALOGW(LOGTAG "%s:notify to BLE SM", __FUNCTION__);
  }

  delete mClient;
}

void BleWifiControlService :: DisconnectPeerInd(string bdaddr, BleIpcStatus status) {
  ble_ipc_msg_t ipc_msg;

  ALOGD(LOGTAG "%s: bdaddr: %s, status: %d", __FUNCTION__, bdaddr.c_str(), status);

  mClient->close();

  peer_device_addr_ = "";
  conn_state_ = GattDevice::STATE_DISCONNECTED;

  ipc_msg.eventId = BLE_IPC_MSG_WCS_PEER_DISCONNECT_IND;
  ipc_msg.wcsPeerDisconnectIndEvent.status = status;
  strlcpy(ipc_msg.wcsPeerDisconnectIndEvent.bd_addr, bdaddr.c_str(), BD_ADDR_STR_LEN);
  g_ble_socket_manager->WBDSSocketWriteHandler(&ipc_msg);

  if (g_ble_socket_manager->cleanup_due_to_deinit_) {
    std::unique_lock<std::mutex> lck(g_ble_socket_manager->socket_manager_cleanup_lock_);
    g_ble_socket_manager->socket_manager_cleanup_.notify_all();
    ALOGW(LOGTAG "%s: notify to BLE SM", __FUNCTION__);
  }

  delete mClient;
}

void BleWifiControlService :: SendNotificationReq(WCSSendNotificationReqEvent *evt) {
  string bdaddr(evt->bd_addr);

  ALOGD(LOGTAG "%s: bdaddr: %s, char_type: %d", __FUNCTION__, bdaddr.c_str(), evt->char_type);

  if (!isWCSRegistered()) {
    SendNotificationRsp(bdaddr, BLE_IPC_STATUS_WCS_NOT_REGISTERED);
    return;
  }

  if(evt->char_type == WCS_WIFI_STATE) {
    mWifiStateCharacteristic->setValue(evt->value, evt->len);
    mServer->notifyCharacteristicChanged(bdaddr,*mWifiStateCharacteristic, false);
  }
}

void BleWifiControlService :: SendNotificationRsp(string bdaddr, BleIpcStatus status) {
  ble_ipc_msg_t ipc_msg;

  ALOGD(LOGTAG "%s: bdaddr: %s, status: %d", __FUNCTION__, bdaddr.c_str(), status);

  ipc_msg.eventId = BLE_IPC_MSG_WCS_SEND_NOTIFICATION_RSP;
  ipc_msg.wcsSendNotificationRspEvent.status = status;
  ipc_msg.wcsSendNotificationRspEvent.char_type = WCS_WIFI_STATE;
  strlcpy(ipc_msg.wcsSendNotificationRspEvent.bd_addr, bdaddr.c_str(), BD_ADDR_STR_LEN);
  g_ble_socket_manager->WBDSSocketWriteHandler(&ipc_msg);
}

void BleWifiControlService :: CharacteristicReadReq(string bdaddr,
                              int requestId,
                              int offset, GattCharacteristic *characteristic) {

  ble_ipc_msg_t ipc_msg;

  ALOGD(LOGTAG "%s: bdaddr: %s", __FUNCTION__, bdaddr.c_str());

  ipc_msg.eventId = BLE_IPC_MSG_WCS_CHARACTERISTIC_READ_REQ;

  if (characteristic->getUuid()== WIFI_STATE_UUID) {
    ipc_msg.wcsCharacteristicReadReqEvent.char_type = WCS_WIFI_STATE;
  } else if(characteristic->getUuid()== DEVICE_ID_UUID) {
    ipc_msg.wcsCharacteristicReadReqEvent.char_type = WCS_DEVICE_ID;
  } else if(characteristic->getUuid()== PAP_CONFIG_ID_UUID) {
    ipc_msg.wcsCharacteristicReadReqEvent.char_type = WCS_PAP_CONFIG_ID;
  } else if(characteristic->getUuid()== PAP_SSID_UUID) {
    ipc_msg.wcsCharacteristicReadReqEvent.char_type = WCS_PAP_SSID;
  } else if(characteristic->getUuid()== PAP_BSSID_UUID) {
    ipc_msg.wcsCharacteristicReadReqEvent.char_type = WCS_PAP_BSSID;
  } else if(characteristic->getUuid()== PAP_COUNTRY_STRING_UUID) {
    ipc_msg.wcsCharacteristicReadReqEvent.char_type = WCS_PAP_COUNTRY_STRING;
  } else if(characteristic->getUuid()== PAP_OPERATING_CLASS_UUID) {
    ipc_msg.wcsCharacteristicReadReqEvent.char_type = WCS_PAP_OPERATING_CLASS;
  } else if(characteristic->getUuid()== PAP_CHANNEL_NUMBER_UUID) {
    ipc_msg.wcsCharacteristicReadReqEvent.char_type = WCS_PAP_CHANNEL_NUMBER;
  }

  ipc_msg.wcsCharacteristicReadReqEvent.offset = offset;
  ipc_msg.wcsCharacteristicReadReqEvent.request_id = requestId;
  strlcpy(ipc_msg.wcsCharacteristicReadReqEvent.bd_addr, bdaddr.c_str(), BD_ADDR_STR_LEN);
  g_ble_socket_manager->WBDSSocketWriteHandler(&ipc_msg);

}

void BleWifiControlService :: CharacteristicReadRsp(WCSCharacteristicReadRspEvent *evt) {
  string bdaddr(evt->bd_addr);

  ALOGD(LOGTAG "%s: bdaddr: %s", __FUNCTION__, bdaddr.c_str());

  if (!isWCSRegistered()) {
    return;
  }

  uint8_t *value = new uint8_t[evt->len];
  if(value){
    int status = (evt->status == BLE_IPC_STATUS_SUCCESS) ? (GattClient::GATT_SUCCESS)
            : (GattClient::GATT_FAILURE);

    memcpy(value, evt->value, evt->len);
    mServer->sendResponse(bdaddr, evt->request_id, status, evt->offset, value, evt->len);
    delete value;
  }
}

void BleWifiControlService :: CharacteristicWriteReq(string bdaddr,
                              int requestId, GattCharacteristic *characteristic,
                              bool preparedWrite, bool responseNeeded,
                              int offset, uint8_t* value, int len) {
  ble_ipc_msg_t ipc_msg;

  ALOGD(LOGTAG "%s: bdaddr: %s, len = %d, preparedWrite = %d",
      __FUNCTION__, bdaddr.c_str(), len, preparedWrite);

  if ((len > WCS_CHAR_VALUE_MAX_LEN) || (preparedWrite == true)) {
    //send error response, if len is larger than expected len or it's prepate write request
    mServer->sendResponse(bdaddr, requestId, GattClient::GATT_FAILURE, offset, value, len);
    return;
  }

  ipc_msg.eventId = BLE_IPC_MSG_WCS_CHARACTERISTIC_WRITE_REQ;

  if (characteristic->getUuid()== WIFI_STATE_UUID) {
    ipc_msg.wcsCharacteristicWriteReqEvent.char_type = WCS_WIFI_STATE;
  } else if(characteristic->getUuid()== DEVICE_ID_UUID) {
    ipc_msg.wcsCharacteristicWriteReqEvent.char_type = WCS_DEVICE_ID;
  } else if(characteristic->getUuid()== PAP_CONFIG_ID_UUID) {
    ipc_msg.wcsCharacteristicWriteReqEvent.char_type = WCS_PAP_CONFIG_ID;
  } else if(characteristic->getUuid()== PAP_SSID_UUID) {
    ipc_msg.wcsCharacteristicWriteReqEvent.char_type = WCS_PAP_SSID;
  } else if(characteristic->getUuid()== PAP_BSSID_UUID) {
    ipc_msg.wcsCharacteristicWriteReqEvent.char_type = WCS_PAP_BSSID;
  } else if(characteristic->getUuid()== PAP_COUNTRY_STRING_UUID) {
    ipc_msg.wcsCharacteristicWriteReqEvent.char_type = WCS_PAP_COUNTRY_STRING;
  } else if(characteristic->getUuid()== PAP_OPERATING_CLASS_UUID) {
    ipc_msg.wcsCharacteristicWriteReqEvent.char_type = WCS_PAP_OPERATING_CLASS;
  } else if(characteristic->getUuid()== PAP_CHANNEL_NUMBER_UUID) {
    ipc_msg.wcsCharacteristicWriteReqEvent.char_type = WCS_PAP_CHANNEL_NUMBER;
  }

  memcpy(ipc_msg.wcsCharacteristicWriteReqEvent.value, value, len);
  ipc_msg.wcsCharacteristicWriteReqEvent.offset = offset;
  ipc_msg.wcsCharacteristicWriteReqEvent.request_id = requestId;
  ipc_msg.wcsCharacteristicWriteReqEvent.rsp_needed = responseNeeded;
  ipc_msg.wcsCharacteristicWriteReqEvent.len = len;
  strlcpy(ipc_msg.wcsCharacteristicWriteReqEvent.bd_addr, bdaddr.c_str(), BD_ADDR_STR_LEN);
  g_ble_socket_manager->WBDSSocketWriteHandler(&ipc_msg);
}

void BleWifiControlService :: CharacteristicWriteRsp(WCSCharacteristicWriteRspEvent *evt) {
  string bdaddr(evt->bd_addr);

  ALOGD(LOGTAG "%s: bdaddr: %s, rsp_needed: %d", __FUNCTION__, bdaddr.c_str(), evt->rsp_needed);

  if (!isWCSRegistered()) {
    return;
  }

  if (evt->rsp_needed) {
    uint8_t *value = new uint8_t[evt->len];
    if(value){
      int status = ((evt->status == BLE_IPC_STATUS_SUCCESS) ? (GattClient::GATT_SUCCESS)
              : (GattClient::GATT_FAILURE));
      memcpy(value, evt->value, evt->len);
      mServer->sendResponse(bdaddr, evt->request_id, status, evt->offset, value, evt->len);
      delete value;
    }
  }
}

void BleWifiControlService :: CCCDReadReq(string bdaddr, int requestId,
                                       int offset) {
  ble_ipc_msg_t ipc_msg;

  ALOGD(LOGTAG "%s: bdaddr: %s", __FUNCTION__, bdaddr.c_str());

  ipc_msg.eventId = BLE_IPC_MSG_WCS_CCCD_READ_REQ;

  strlcpy(ipc_msg.wcsCharacteristicWriteReqEvent.bd_addr, bdaddr.c_str(), BD_ADDR_STR_LEN);
  ipc_msg.wcsCCCDWriteReqEvent.char_type = WCS_WIFI_STATE;
  ipc_msg.wcsCCCDWriteReqEvent.offset = offset;
  ipc_msg.wcsCCCDWriteReqEvent.request_id = requestId;

  g_ble_socket_manager->WBDSSocketWriteHandler(&ipc_msg);
}

void BleWifiControlService :: CCCDReadRsp(WCSCCCDReadRspEvent *evt) {
  string bdaddr(evt->bd_addr);
  int status = ((evt->status == BLE_IPC_STATUS_SUCCESS) ? (GattClient::GATT_SUCCESS)
              : (GattClient::GATT_FAILURE));

  ALOGD(LOGTAG "%s: bdaddr: %s", __FUNCTION__, bdaddr.c_str());

  if (!isWCSRegistered()) {
    return;
  }

  mServer->sendResponse(bdaddr, evt->request_id, status, evt->offset, &(evt->value), 1);
}

void BleWifiControlService :: CCCDWriteReq(string bdaddr, int requestId,
                                        bool responseNeeded, int offset, uint8_t value) {

  ble_ipc_msg_t ipc_msg;

  ALOGD(LOGTAG "%s: bdaddr: %s", __FUNCTION__, bdaddr.c_str());

  ipc_msg.eventId = BLE_IPC_MSG_WCS_CCCD_WRITE_REQ;

  strlcpy(ipc_msg.wcsCharacteristicWriteReqEvent.bd_addr, bdaddr.c_str(), BD_ADDR_STR_LEN);
  ipc_msg.wcsCCCDWriteReqEvent.char_type = WCS_WIFI_STATE;
  ipc_msg.wcsCCCDWriteReqEvent.offset = offset;
  ipc_msg.wcsCCCDWriteReqEvent.request_id = requestId;
  ipc_msg.wcsCCCDWriteReqEvent.rsp_needed = responseNeeded;
  ipc_msg.wcsCCCDWriteReqEvent.value = value;

  g_ble_socket_manager->WBDSSocketWriteHandler(&ipc_msg);

}

void BleWifiControlService :: CCCDWriteRsp(WCSCCCDWriteRspEvent *evt) {
  string bdaddr(evt->bd_addr);

  ALOGD(LOGTAG "%s: bdaddr: %s", __FUNCTION__, bdaddr.c_str());

  if (!isWCSRegistered()) {
    return;
  }

  if (evt->rsp_needed) {
    int status = ((evt->status == BLE_IPC_STATUS_SUCCESS) ? (GattClient::GATT_SUCCESS)
              : (GattClient::GATT_FAILURE));
    mServer->sendResponse(bdaddr, evt->request_id, status, evt->offset, &(evt->value), 1);
  }
}
