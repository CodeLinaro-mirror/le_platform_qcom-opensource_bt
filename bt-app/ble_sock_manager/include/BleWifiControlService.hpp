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

#ifndef BLE_WIFI_CONTROL_SERVICE_H
#define BLE_WIFI_CONTROL_SERVICE_H
#pragma once

#include "BleSockIf.hpp"
#include "GattClient.hpp"
#include "GattServer.hpp"
#include "GattDescriptor.hpp"
#include "GattCharacteristic.hpp"
#include "GattService.hpp"
#include "GattLeScanner.hpp"


using namespace gatt;

class BleWifiControlService {
  private:
    const Uuid WCS_UUID = Uuid::FromString("20c59b66-49b3-44f5-beec-b895257bb763");
    const Uuid WIFI_STATE_UUID = Uuid::FromString("bf5a1235-b421-43b7-86c3-1047c11f75bd");
    const Uuid DEVICE_ID_UUID = Uuid::FromString("94da529d-8458-46fc-98bb-fae2ec07a959");
    const Uuid PAP_CONFIG_ID_UUID = Uuid::FromString("a4270a71-cd25-4995-a14f-2c3dcc538ab8");
    const Uuid PAP_SSID_UUID = Uuid::FromString("85bc587a-5a98-42f9-8514-d43fbd2574a7");
    const Uuid PAP_BSSID_UUID = Uuid::FromString("d399825a-c206-4378-bbb2-31f260dc6917");
    const Uuid PAP_COUNTRY_STRING_UUID = Uuid::FromString("bd26f136-f238-475b-aae5-d4283534dae0");
    const Uuid PAP_OPERATING_CLASS_UUID = Uuid::FromString("cd543a1b-9a99-4e4d-b19b-38bd671f336b");
    const Uuid PAP_CHANNEL_NUMBER_UUID = Uuid::FromString("3a1ec80a-c597-4da7-82b1-04a200f9f4ad");

    GattServer *mServer = NULL;
    GattClient *mClient = NULL;
    GattLeScanner* mScanner = NULL;
    GattService *mService = NULL;
    GattCharacteristic *mWifiStateCharacteristic = NULL;
    GattCharacteristic *mDeviceIdCharacteristic = NULL;
    GattCharacteristic *mPAPConfigIDCharacteristic = NULL;
    GattCharacteristic *mPAPSSIDCharacteristic = NULL;
    GattCharacteristic *mPAPBSSIDCharacteristic = NULL;
    GattCharacteristic *mPAPCountryStringCharacteristic = NULL;
    GattCharacteristic *mPAPOperatingClassCharacteristic = NULL;
    GattCharacteristic *mPAPChannelNumberCharacteristic = NULL;
    GattDescriptor *mCCCDDescriptor = NULL;

    bool is_discovery_enabled_;

  public:
    const Uuid CCCD_UUID = Uuid::FromString("00002902-0000-1000-8000-00805f9b34fb");
    Uuid service_data_uuid_;
    bool is_wcs_added_;
    string peer_device_addr_;
    int conn_state_;

    BleWifiControlService();
    ~BleWifiControlService();
    void AddWCS();
    void RemoveWCS();
    bool isWCSRegistered();
    bool isPeerDiscoveryEnabled();
    bool isPeerDeviceConnectedOrConnecting();
    void PeerDiscoveryEnableReq(WCSPeerDiscoveryEnableReqEvent *evt);
    void PeerDiscoveryEnableRsp(BleIpcStatus status);
    void PeerDiscoveryDisableReq();
    void PeerDiscoveryDisableRsp(BleIpcStatus status);
    void PeerDiscoveryResult(int advertise_flags,
                            uint16_t service_data_uuid,
                            uint8_t service_data_len,
                            uint8_t *service_data,
                            string bdaddr);
    void ConnectPeerReq(WCSConnectPeerReqEvent *evt);
    void ConnectPeerRsp(string bdaddr, BleIpcStatus status);
    void DisconnectPeerReq(WCSDisconnectPeerReqEvent *evt);
    void DisconnectPeerRsp(string bdaddr, BleIpcStatus status);
    void DisconnectPeerInd(string bdaddr, BleIpcStatus status);
    void SendNotificationReq(WCSSendNotificationReqEvent *evt);
    void SendNotificationRsp(string bdaddr, BleIpcStatus status);
    void CharacteristicReadReq(string bdaddr,
                              int requestId,
                              int offset, GattCharacteristic *characteristic);
    void CharacteristicReadRsp(WCSCharacteristicReadRspEvent *evt);
    void CharacteristicWriteReq(string bdaddr,
                              int requestId, GattCharacteristic *characteristic,
                              bool preparedWrite,bool responseNeeded,
                              int offset, uint8_t* value, int len);
    void CharacteristicWriteRsp(WCSCharacteristicWriteRspEvent *evt);
    void CCCDReadReq(string bdaddr, int requestId,
                           int offset);
    void CCCDReadRsp(WCSCCCDReadRspEvent *evt);
    void CCCDWriteReq(string bdaddr, int requestId,
                          bool responseNeeded, int offset, uint8_t value);
    void CCCDWriteRsp(WCSCCCDWriteRspEvent *evt);
};

#endif

