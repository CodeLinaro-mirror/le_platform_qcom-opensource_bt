/******************************************************************************
 *
 *  Copyright (c) 2016, The Linux Foundation. All rights reserved.
 *  Not a Contribution.
 *  Copyright (C) 2014 Google, Inc.
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at:
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 *
 ******************************************************************************/

#pragma once

#include "osi/include/thread.h"
#include <hardware/bluetooth.h>

extern thread_t *g_gap_thread;
extern thread_t *g_main_thread;


#define MAIN_MSG_BASE           (0)
#define GAP_MSG_BASE            (1000)
#define MAX_BD_STR_LEN          (18)

typedef enum {
    THREAD_ID_MAIN,
    THREAD_ID_GAP,
} ThreadIdType;


typedef enum {
    MAIN_API_INIT = (MAIN_MSG_BASE + 1),
    MAIN_API_DEINIT,
    MAIN_EVENT_ACL_CONNECTED,
    MAIN_EVENT_ACL_DISCONNECTED,
    MAIN_EVENT_DEVICE_FOUND,
    MAIN_EVENT_INQUIRY_STATUS,
    MAIN_EVENT_BOND_STATE,
    MAIN_EVENT_ENABLED,
    MAIN_EVENT_DISABLED,

    MAIN_MSG_DISCOVER_DEVICES,
    MAIN_MSG_BOND_DEVICE,
    MAIN_MSG_CONNECT_DEVICE,
    MAIN_MSG_DISCONNECT_DEVICE,

    GAP_API_ENABLE = GAP_MSG_BASE,
    GAP_API_DISABLE,
    GAP_API_START_INQUIRY,
    GAP_API_STOP_INQUIRY,
    GAP_API_CREATE_BOND,

    GAP_EVENT_ADAPTER_STATE,
    GAP_EVENT_ACL_STATE_CHANGED,
    GAP_EVENT_DISCOVERY_STATE_CHANGED,
    GAP_EVENT_DEVICE_FOUND_INT,
    GAP_EVENT_DEVICE_FOUND,
    GAP_EVENT_PIN_REQUEST,
    GAP_EVENT_SSP_REQUEST,
    GAP_EVENT_REMOTE_DEVICE_PROPERTIES,
    GAP_EVENT_ADAPTER_PROPERTIES,
    GAP_EVENT_BOND_STATE_INT,
    GAP_EVENT_BOND_STATE,

} BluetoothEventId;

typedef struct {
    bt_bdaddr_t address;
    char mName[248];
    int mBluetoothClass;
    short mRssi;
    bt_uuid_t mUuids[16];
    int mDeviceType;
    int retValue;
    char mAlias[64];
    int mBondState;
} DeviceProperties;

typedef enum {
    BT_ADAPTER_STATE_OFF,
    BT_ADAPTER_STATE_ON,
    BT_ADAPTER_STATE_TURNING_ON,
    BT_ADAPTER_STATE_TURNING_OFF
} AdapterState;

/* Events to statemachine */

typedef struct {
    BluetoothEventId event_id;
    bt_state_t          status;
} GapAppEvent;

typedef struct {
    BluetoothEventId event_id;
    bt_bdaddr_t         bd_addr;
    int                 num_properties;
    bt_property_t       *properties;
} RemotePropertiesEvent;

typedef struct {
    BluetoothEventId event_id;
    int                 num_properties;
    bt_property_t       *properties;
} AdapterPropertiesEvent;

typedef struct {
    BluetoothEventId event_id;
    int                 num_properties;
    bt_property_t       *properties;
} DeviceFoundEventInt;

typedef struct {
    BluetoothEventId event_id;
    DeviceProperties remoteDevice;
} DeviceFoundEvent;

typedef struct {
    BluetoothEventId event_id;
    bt_bond_state_t     state;
    bt_bdaddr_t         bd_addr;
} DeviceBondStateEventInt;

typedef struct {
    BluetoothEventId event_id;
    bt_bond_state_t     state;
} DeviceBondStateEvent;

typedef struct {
    BluetoothEventId event_id;
    bt_status_t status;
    bt_bdaddr_t bd_addr;
    bt_acl_state_t state;
} ACLStateEvent;

typedef struct {
    BluetoothEventId event_id;
    bt_discovery_state_t state;
} DiscoveryStateEvent;

typedef struct {
    BluetoothEventId event_id;
    bt_bdaddr_t         bd_addr;
    bt_bdname_t         bd_name;
    uint32_t            cod;
    bool             secure;
} PINRequestEvent;

typedef struct {
    BluetoothEventId event_id;
    bt_bdaddr_t         bd_addr;
    bt_bdname_t         bd_name;
    uint32_t            cod;
    bt_ssp_variant_t    pairing_variant;
    uint32_t            pass_key;
} SSPRequestEvent;

typedef struct {
    BluetoothEventId event_id;
} DeviceDiscoverRequest;

typedef struct {
    BluetoothEventId event_id;
    bt_bdaddr_t         bd_addr;
} DeviceBondRequest;

typedef struct {
    BluetoothEventId    event_id;
    bt_bdaddr_t         bd_addr;
} DeviceConnectRequest;

typedef struct {
    BluetoothEventId event_id;
    bt_bdaddr_t         bd_addr;
} DeviceDisconnectRequest;

union BtEvent {
    BluetoothEventId        event_id;
    GapAppEvent             state_event;
    SSPRequestEvent         ssp_request_event;
    PINRequestEvent         pin_request_event;
    DiscoveryStateEvent     discovery_state_event;
    ACLStateEvent           acl_state_event;
    DeviceBondStateEvent    bond_state_event;
    DeviceBondStateEventInt bond_state_event_int;
    DeviceFoundEvent        device_found_event;
    DeviceFoundEventInt     device_found_event_int;
    RemotePropertiesEvent   remote_properties_event;
    AdapterPropertiesEvent  adapater_properties_event;
    DeviceDiscoverRequest   discover_request;
    DeviceBondRequest       bond_device;
};

#ifdef __cplusplus
extern "C" {
#endif

typedef char bdstr_t[MAX_BD_STR_LEN];
void PostMessage(ThreadIdType thread_id, void *msg);
const char *BdAddr2Str(const bt_bdaddr_t *bd_addr, char *bd_str);
void BtGapMsgHandler(void *context);
void BtMainMsgHandler(void *context);

#ifdef __cplusplus
}
#endif
