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

#ifndef GATT_NATIVE_INTERFACE_V2_HPP
#define GATT_NATIVE_INTERFACE_V2_HPP

#pragma once
#include "GattNativeDefines.hpp"
#include <hardware/bluetooth.h>
#include <hardware/bt_gatt.h>

namespace gatt {

class GattNativeInterfaceV2{
  private:
    const bt_interface_t * bluetooth_interface = NULL;
    btgatt_interface_t *sGattIf = NULL;
  public:
     GattNativeInterfaceV2(const bt_interface_t *bt_interface);
    ~GattNativeInterfaceV2();
    int gattClientGetDeviceTypeNative(string address);
    void gattClientRegisterAppNative(btapp::Uuid uuid);
    void gattClientUnregisterAppNative(int clientIf);
    void registerScannerNative(btapp::Uuid uuid);
    void unregisterScannerNative(int scanner_id);
    void gattClientScanNative(bool start);
    void gattClientConnectNative(int clientif, string address, bool isDirect,
        int transport, bool opportunistic, int initiating_phys);
    void gattClientDisconnectNative(int clientIf, string address, int conn_id);
    void gattClientSetPreferredPhyNative(int clientIf, string address,
        int tx_phy, int rx_phy, int phy_options);
    void gattClientReadPhyNative(int clientIf, string address);
    void gattClientRefreshNative(int clientIf, string address);
    void gattClientSearchServiceNative(int conn_id, bool search_all, btapp::Uuid uuid);
    void gattClientDiscoverServiceByUuidNative(int conn_id, btapp::Uuid uuid);
    void gattClientGetGattDbNative(int conn_id);
    void gattClientReadCharacteristicNative(int conn_id, int handle, int authReq);
    void gattClientReadUsingCharacteristicUuidNative(
        int conn_id, btapp::Uuid uuid, int s_handle, int e_handle, int authReq);
    void gattClientReadDescriptorNative(int conn_id, int handle, int authReq);
    void gattClientWriteCharacteristicNative(int conn_id, int handle,
        int write_type, int auth_req, std::vector<uint8_t> vect_val);
    void gattClientExecuteWriteNative(int conn_id, bool execute);
    void gattClientWriteDescriptorNative(int conn_id, int handle, int auth_req,
        std::vector<uint8_t> vect_val);
    void gattClientRegisterForNotificationsNative(
        int clientIf, string address, int handle, bool enable);
    void gattClientReadRemoteRssiNative(int clientif, string address);
    void gattSetScanParametersNative(int client_if, int scan_phy,
        std::vector<uint32_t> scan_interval, std::vector<uint32_t> scan_window);
    void getOwnAddressNative(int advertiser_id);
    void gattClientScanFilterParamAddNative(uint8_t client_if, uint8_t filt_index,
         std::unique_ptr<btgatt_filt_param_setup_t> filt_params);
    void gattClientScanFilterParamDeleteNative(uint8_t client_if, uint8_t filt_index);
    void gattClientScanFilterParamClearAllNative(uint8_t client_if);
    void gattClientScanFilterAddNative(int client_if,int filter_index,
        std::vector<apcf_command_t> filters);
    void gattClientScanFilterClearNative(int client_if, int filt_index);
    void gattClientScanFilterEnableNative(int client_if, bool enable);
    void gattClientConfigureMTUNative(int conn_id, int mtu);
    void gattConnectionParameterUpdateNative(int client_if, string address,
        int min_interval,int max_interval, int latency, int timeout, int min_ce_len,
        int max_ce_len);
    void gattClientConfigBatchScanStorageNative(int client_if, int max_full_reports_percent,
        int max_trunc_reports_percent, int notify_threshold_level_percent);
    void gattClientStartBatchScanNative(int client_if, int scan_mode,
        int scan_interval_unit, int scan_window_unit, int addr_type, int discard_rule);
    void gattClientStopBatchScanNative(int client_if);
    void gattClientReadScanReportsNative(int client_if, int scan_type);
    void gattServerRegisterAppNative(btapp::Uuid uuid);
    void gattServerUnregisterAppNative(int serverIf);
    void gattServerConnectNative(int server_if, string address, bool is_direct,
        int transport);
    void gattServerDisconnectNative(int serverIf, string address, int conn_id);
    void gattServerSetPreferredPhyNative(int serverIf, string address,
        int tx_phy, int rx_phy, int phy_options);
    void gattServerReadPhyNative(int serverIf, string address);
    void gattServerAddServiceNative(int server_if, std::vector<gatt_db_element_t> service);
    void gattServerStopServiceNative(int server_if, int svc_handle);
    void gattServerDeleteServiceNative(int server_if, int svc_handle);
    void gattServerSendIndicationNative(int server_if, int attr_handle,
       int conn_id, std::vector<uint8_t> vect_val);
    void gattServerSendNotificationNative(int server_if, int attr_handle,
      int conn_id, std::vector<uint8_t> vect_val);
    void gattServerSendResponseNative(int server_if, int conn_id,
        int trans_id, int status, int handle, int offset,
        std::vector<uint8_t> vect_val, int auth_req);
    void startAdvertisingSetNative(advertise_parameters_t params, std::vector<uint8_t> adv_data,
        std::vector<uint8_t> scan_resp, periodic_advertising_parameters_t periodic_params,
        std::vector<uint8_t> periodic_data, int duration, int maxExtAdvEvents, int reg_id);
    void stopAdvertisingSetNative(int advertiser_id);
    void enableAdvertisingSetNative(int advertiser_id, bool enable, int duration, int maxExtAdvEvents);
    void setAdvertisingDataNative(int advertiser_id, std::vector<uint8_t> data);
    void setScanResponseDataNative(int advertiser_id, std::vector<uint8_t> data);
    void setAdvertisingParametersNative(int advertiser_id, advertise_parameters_t parameters);
    void setPeriodicAdvertisingParametersNative(int advertiser_id, periodic_advertising_parameters_t periodic_parameters);
    void setPeriodicAdvertisingDataNative(int advertiser_id,std::vector<uint8_t> data);
    void setPeriodicAdvertisingEnableNative(int advertiser_id, bool enable);
    void startSyncNative(int sid, string address, int skip, int timeout, int reg_id);
    void stopSyncNative(int sync_handle);
    void gattTestNative(int command, btapp::Uuid uuid1, string bda1, int p1, int p2, int p3, int p4, int p5);
};
}  // namespace gatt

#endif
