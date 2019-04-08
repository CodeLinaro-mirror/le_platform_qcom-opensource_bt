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

#ifndef GATT_NATIVE_INTERFACE_V2_B_HPP
#define GATT_NATIVE_INTERFACE_V2_B_HPP

#pragma once
#include "GattNativeDefines.hpp"
#include <hardware/bluetooth.h>
#include <hardware/bt_gatt.h>

/*extenstion callbacks of GattNativeInterfaceV2b*/

typedef void (*register_scanner_cb_t)(const bluetooth::Uuid& app_uuid, uint8_t scannerId, uint8_t status);
typedef void (*readClientPhyCb_t) (uint8_t clientIf, RawAddress bda, uint8_t tx_phy, uint8_t rx_phy, uint8_t status);
typedef void (*scan_params_cmpl_cb_t)(uint8_t client_if, uint8_t status);
typedef void (*getOwnAddressCb_t)(uint8_t advertiser_id, uint8_t address_type, RawAddress address);
typedef void (*scan_filter_param_cb_t)(uint8_t client_if, uint8_t avbl_space, uint8_t action, uint8_t status);
typedef void (*scan_filter_cfg_cb_t)(uint8_t client_if, uint8_t filt_type, uint8_t avbl_space, uint8_t action, uint8_t status);
typedef void (*scan_filter_status_cb_t)(uint8_t client_if, uint8_t action, uint8_t status);
typedef void (*batchscan_cfg_storage_cb_t)(uint8_t client_if, uint8_t status);
typedef void (*batchscan_start_cb_t)(uint8_t client_if, uint8_t status);
typedef void (*batchscan_stop_cb_t)(uint8_t client_if, uint8_t status);
typedef void (*readServerPhyCb_t)(uint8_t serverIf, RawAddress bda, uint8_t tx_phy, uint8_t rx_phy, uint8_t status);
typedef void (*ble_advertising_set_started_cb_t)(int reg_id, uint8_t advertiser_id, int8_t tx_power, uint8_t status);
typedef void (*ble_advertising_set_timeout_cb_t)(uint8_t advertiser_id, uint8_t status);
typedef void (*ble_advertising_set_enable_Cb_t)(uint8_t advertiser_id, bool enable, uint8_t status);
typedef void (*ble_advertising_parameters_updated_cb_t)(uint8_t advertiser_id,uint8_t status, int8_t tx_power);
typedef void (*ble_periodic_advertising_set_enable_Cb_t)(uint8_t advertiser_id, bool enable, uint8_t status);
typedef void (*onSetAdvertisingData_t)(uint8_t advertiser_id, uint8_t status);
typedef void (*onSetScanResponseData_t)(uint8_t advertiser_id,uint8_t status);
typedef void (*onSetPeriodicAdvertisingParameters_t)(uint8_t advertiser_id, uint8_t status);
typedef void (*onSetPeriodicAdvertisingData_t)(uint8_t advertiser_id, uint8_t status);
typedef void (*onSyncStarted_t)(int reg_id, uint8_t status, uint16_t sync_handle, uint8_t sid, uint8_t address_type, RawAddress address, uint8_t phy, uint16_t interval);
typedef void (*onSyncReport_t)(uint16_t sync_handle, int8_t tx_power, int8_t rssi, uint8_t data_status, std::vector<uint8_t> data);
typedef void (*onSyncLost_t)(uint16_t sync_handle);

typedef struct
{
	register_scanner_cb_t                    register_scanner_cb;
	readClientPhyCb_t                        readClientPhyCb;                             
	scan_params_cmpl_cb_t                    scan_params_cmpl_cb;
	getOwnAddressCb_t                        getOwnAddressCb;
	scan_filter_param_cb_t                   scan_filter_param_cb;
	scan_filter_cfg_cb_t                     scan_filter_cfg_cb;
	scan_filter_status_cb_t                  scan_filter_status_cb;
	batchscan_cfg_storage_cb_t               batchscan_cfg_storage_cb;
	batchscan_start_cb_t                     batchscan_start_cb;
	batchscan_stop_cb_t                      batchscan_stop_cb;
	readServerPhyCb_t                        readServerPhyCb;
	ble_advertising_set_started_cb_t         ble_advertising_set_started_cb;
	ble_advertising_set_timeout_cb_t         ble_advertising_set_timeout_cb;
	ble_advertising_set_enable_Cb_t          ble_advertising_set_enable_Cb;
	ble_advertising_parameters_updated_cb_t  ble_advertising_parameters_updated_cb;
	ble_periodic_advertising_set_enable_Cb_t ble_periodic_advertising_set_enable_Cb;
	onSetAdvertisingData_t                   onSetAdvertisingData;
	onSetScanResponseData_t                  onSetScanResponseData;
	onSetPeriodicAdvertisingParameters_t     onSetPeriodicAdvertisingParameters;
	onSetPeriodicAdvertisingData_t           onSetPeriodicAdvertisingData;
	onSyncStarted_t                          onSyncStarted;
	onSyncReport_t                           onSyncReport;
	onSyncLost_t                             onSyncLost;
	
}btgatt_ext_callbacks_t;

typedef struct
{
	const btgatt_callbacks_t *btgatt_callbacks; // defined in bt_gatt.h
	const btgatt_ext_callbacks_t *extension;
}btgatt_native_interface_callbacks_t;

/*class of GattNativeInterfaceV2b*/
namespace gatt {

typedef struct {
	
	void (*init) (const btgatt_native_interface_callbacks_t* btgatt_native_callback);
	void (*deinit) (void);
	int (*gattClientGetDeviceTypeNative)(string address);
	void (*gattClientRegisterAppNative)(btapp::Uuid uuid);
	void (*gattClientUnregisterAppNative)(int clientIf);
	void (*registerScannerNative)(btapp::Uuid uuid);
	void (*unregisterScannerNative)(int scanner_id);
	void (*gattClientScanNative)(bool start);
	void (*gattClientConnectNative)(int clientif, string address, bool isDirect, int transport, bool opportunistic, int initiating_phys);
	void (*gattClientDisconnectNative)(int clientIf, string address, int conn_id);
	void (*gattClientSetPreferredPhyNative)(int clientIf, string address, int tx_phy, int rx_phy, int phy_options);
	void (*gattClientReadPhyNative)(int clientIf, string address);
	void (*gattClientRefreshNative)(int clientIf, string address);
	void (*gattClientSearchServiceNative)(int conn_id, bool search_all, btapp::Uuid uuid);
	void (*gattClientDiscoverServiceByUuidNative)(int conn_id, btapp::Uuid uuid);
	void (*gattClientGetGattDbNative)(int conn_id);
	void (*gattClientReadCharacteristicNative)(int conn_id, int handle, int authReq);
	void (*gattClientReadUsingCharacteristicUuidNative)(int conn_id, btapp::Uuid uuid, int s_handle, int e_handle, int authReq);
	void (*gattClientReadDescriptorNative)(int conn_id, int handle, int authReq);
	void (*gattClientWriteCharacteristicNative)(int conn_id, int handle,int write_type, int auth_req, std::vector<uint8_t> vect_val);
	void (*gattClientExecuteWriteNative)(int conn_id, bool execute);
	void (*gattClientWriteDescriptorNative)(int conn_id, int handle, int auth_req, std::vector<uint8_t> vect_val);
	void (*gattClientRegisterForNotificationsNative)(int clientIf, string address, int handle, bool enable);
	void (*gattClientReadRemoteRssiNative)(int clientif, string address);
	void (*gattSetScanParametersNative)(int client_if, int scan_phy, std::vector<uint32_t> scan_interval, std::vector<uint32_t> scan_window);
	void (*getOwnAddressNative)(int advertiser_id);
	void (*gattClientScanFilterParamAddNative)(uint8_t client_if, uint8_t filt_index, std::unique_ptr<btgatt_filt_param_setup_t> filt_params);
	void (*gattClientScanFilterParamDeleteNative)(uint8_t client_if, uint8_t filt_index);
	void (*gattClientScanFilterParamClearAllNative)(uint8_t client_if);
	void (*gattClientScanFilterAddNative)(int client_if,int filter_index,std::vector<apcf_command_t> filters);
	void (*gattClientScanFilterClearNative)(int client_if, int filt_index);
	void (*gattClientScanFilterEnableNative)(int client_if, bool enable);
	void (*gattClientConfigureMTUNative)(int conn_id, int mtu);
	void (*gattConnectionParameterUpdateNative)(int client_if, string address,
        int min_interval,int max_interval, int latency, int timeout, int min_ce_len,
        int max_ce_len);
	void (*gattClientConfigBatchScanStorageNative)(int client_if, int max_full_reports_percent,
        int max_trunc_reports_percent, int notify_threshold_level_percent);
	void (*gattClientStartBatchScanNative)(int client_if, int scan_mode,
        int scan_interval_unit, int scan_window_unit, int addr_type, int discard_rule);
	void (*gattClientStopBatchScanNative)(int client_if);
	void (*gattClientReadScanReportsNative)(int client_if, int scan_type);
	void (*gattServerRegisterAppNative)(btapp::Uuid uuid);
	void (*gattServerUnregisterAppNative)(int serverIf);
	void (*gattServerConnectNative)(int server_if, string address, bool is_direct, int transport);
	void (*gattServerDisconnectNative)(int serverIf, string address, int conn_id);
	void (*gattServerSetPreferredPhyNative)(int serverIf, string address, int tx_phy, int rx_phy, int phy_options);
	void (*gattServerReadPhyNative)(int serverIf, string address);
	void (*gattServerAddServiceNative)(int server_if, std::vector<gatt_db_element_t> service);
	void (*gattServerStopServiceNative)(int server_if, int svc_handle);
	void (*gattServerDeleteServiceNative)(int server_if, int svc_handle);
	void (*gattServerSendIndicationNative)(int server_if, int attr_handle, int conn_id, std::vector<uint8_t> vect_val);
	void (*gattServerSendNotificationNative)(int server_if, int attr_handle,
      int conn_id, std::vector<uint8_t> vect_val);
	void (*gattServerSendResponseNative)(int server_if, int conn_id,
        int trans_id, int status, int handle, int offset,
        std::vector<uint8_t> vect_val, int auth_req);
	void (*startAdvertisingSetNative)(advertise_parameters_t params, std::vector<uint8_t> adv_data,
        std::vector<uint8_t> scan_resp, periodic_advertising_parameters_t periodic_params,
        std::vector<uint8_t> periodic_data, int duration, int maxExtAdvEvents, int reg_id);
	void (*stopAdvertisingSetNative)(int advertiser_id);
	void (*enableAdvertisingSetNative)(int advertiser_id, bool enable, int duration, int maxExtAdvEvents);
	void (*setAdvertisingDataNative)(int advertiser_id, std::vector<uint8_t> data);
	void (*setScanResponseDataNative)(int advertiser_id, std::vector<uint8_t> data);
	void (*setAdvertisingParametersNative)(int advertiser_id, advertise_parameters_t parameters);
	void (*setPeriodicAdvertisingParametersNative)(int advertiser_id, periodic_advertising_parameters_t periodic_parameters);
	void (*setPeriodicAdvertisingDataNative)(int advertiser_id,std::vector<uint8_t> data);
	void (*setPeriodicAdvertisingEnableNative)(int advertiser_id, bool enable);
	void (*startSyncNative)(int sid, string address, int skip, int timeout, int reg_id);
	void (*stopSyncNative)(int sync_handle);
	void (*gattTestNative)(int command, btapp::Uuid uuid1, string bda1, int p1, int p2, int p3, int p4, int p5);
}GattNativeInterfaceV2b;
}  // names

extern gatt::GattNativeInterfaceV2b GattNativeInterfaceV2bImplInst;

/*device of GattNativeInterfaceV2b*/
typedef gatt::GattNativeInterfaceV2b gatt_native_interface_v2b_t;

typedef struct 
{
    const void* (*get_bluetooth_interface)();
	const void* (*get_gatt_native_interface_v2b)();

}bluetooth_extension_interface_t; 

typedef struct {
    struct hw_device_t common;
	const bluetooth_extension_interface_t* (*get_bluetooth_extension_interface)(); 
	
}bluetooth_device_ext_t;

typedef bluetooth_device_ext_t bluetooth_ext_module_t;

#endif
