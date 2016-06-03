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

/**
 * @file ipc.h
 *
 * @brief It is common header file which contains all event related structures
 */

#define MAIN_MSG_BASE           (0)
#define GAP_MSG_BASE            (1000)
#define PAN_MSG_BASE            (2000)
#define A2DP_SINK_MSG_BASE      (300)
#define MAX_BD_STR_LEN          (18)
#define BT_IPC_MSG_LEN 2

#define CMD_ID_PLAY             0x44;
#define CMD_ID_STOP             0x45;
#define CMD_ID_PAUSE            0x46;
#define CMD_ID_REWIND           0x48;
#define CMD_ID_FF               0x49;
#define CMD_ID_FORWARD          0x4B;
#define CMD_ID_BACKWARD         0x4C;

#define KEY_PRESSED             0;
#define KEY_RELEASED            1;

/**
 *   Threads info
 */
typedef enum {
    THREAD_ID_MAIN = 0,
    THREAD_ID_GAP,
    THREAD_ID_A2DP_SINK,
    THREAD_ID_PAN,
    THREAD_ID_MAX,
} ThreadIdType;

/**
 *   Profiles info
 */
typedef enum {
    PROFILE_ID_A2DP_SINK = 0,
    PROFILE_ID_PAN,
    PROFILE_ID_MAX
} ProfileIdType;

typedef void (*ThreadHandler) (void *context);

typedef struct {
    thread_t *thread_id;
    ThreadIdType thread_type;
    ThreadHandler thread_handler;
    char thread_name[50];
} ThreadInfo;

/**
 *  list of EVENTS used by GAP and MAIN thread
 */
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
    MAIN_EVENT_SSP_REQUEST,
    MAIN_EVENT_PIN_REQUEST,

    MAIN_MSG_DISCOVER_DEVICES,
    MAIN_MSG_BOND_DEVICE,
    MAIN_MSG_CONNECT_DEVICE,
    MAIN_MSG_DISCONNECT_DEVICE,

    A2DP_SINK_API_CONNECT_REQ = A2DP_SINK_MSG_BASE,
    A2DP_SINK_API_DISCONNECT_REQ,
    A2DP_SINK_DISCONNECTED_CB,
    A2DP_SINK_CONNECTING_CB,
    A2DP_SINK_CONNECTED_CB,
    A2DP_SINK_DISCONNECTING_CB,
    A2DP_SINK_FOCUS_REQUEST_CB,
    A2DP_SINK_AUDIO_SUSPENDED,
    A2DP_SINK_AUDIO_STOPPED,
    A2DP_SINK_AUDIO_STARTED,
    AVRCP_CTRL_CONNECTED_CB,
    AVRCP_CTRL_DISCONNECTED_CB,
    AVRCP_CTRL_PASS_THRU_CMD_REQ,

    GAP_API_ENABLE = GAP_MSG_BASE,
    GAP_API_DISABLE,
    GAP_API_START_INQUIRY,
    GAP_API_STOP_INQUIRY,
    GAP_API_CREATE_BOND,
    GAP_API_SSP_REPLY,
    GAP_API_PIN_REPLY,

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
    GAP_EVENT_PROFILE_START_TIMEOUT,
    GAP_EVENT_PROFILE_STOP_TIMEOUT,
    GAP_EVENT_ENABLE_TIMEOUT,
    GAP_EVENT_DISABLE_TIMEOUT,
    SKT_API_START_LISTENER,
    SKT_API_IPC_MSG_WRITE,
    SKT_API_IPC_MSG_READ,

    PROFILE_API_START,
    PROFILE_API_STOP,
    PROFILE_EVENT_START_DONE,
    PROFILE_EVENT_STOP_DONE,

    PAN_EVENT_CONTROL_STATE_CHANGED = PAN_MSG_BASE,
    PAN_EVENT_CONNECTION_STATE_CHANGED,
    PAN_EVENT_SET_TETHERING_REQ,
    PAN_EVENT_DEVICE_CONNECT_REQ,
    PAN_EVENT_DEVICE_DISCONNECT_REQ,
    PAN_EVENT_DEVICE_CONNECTED_LIST_REQ,
} BluetoothEventId;

typedef struct {
    bt_bdaddr_t address;
    char name[248];
    int bluetooth_class;
    short rssi;
    bt_uuid_t uuids[16];
    int device_type;
    int ret_value;
    char alias[64];
    int bond_state;
    bool broadcast;
} DeviceProperties;

typedef enum {
    BT_ADAPTER_STATE_OFF,
    BT_ADAPTER_STATE_ON,
    BT_ADAPTER_STATE_TURNING_ON,
    BT_ADAPTER_STATE_TURNING_OFF
} AdapterState;

/**
 * Generic Event for GAP
 */
typedef struct {
    BluetoothEventId event_id;
    bt_state_t          status;
} GapAppEvent;

/**
 * Event for notifying Remote Device properties
 */
typedef struct {
    BluetoothEventId event_id;
    bt_bdaddr_t         bd_addr;
    int                 num_properties;
    bt_property_t       *properties;
} RemotePropertiesEvent;

/**
 * Event for notifying Adapter Properties
 */
typedef struct {
    BluetoothEventId event_id;
    int                 num_properties;
    bt_property_t       *properties;
} AdapterPropertiesEvent;

/**
 * Event for notifying Device found, It used only for internall threads
 */
typedef struct {
    BluetoothEventId event_id;
    int                 num_properties;
    bt_property_t       *properties;
} DeviceFoundEventInt;

/**
 * Event for notifying Device found
 */
typedef struct {
    BluetoothEventId event_id;
    DeviceProperties remoteDevice;
} DeviceFoundEvent;

/**
 * Event for notifying Device Bond state
 */
typedef struct {
    BluetoothEventId event_id;
    bt_bond_state_t     state;
    bt_bdaddr_t         bd_addr;
} DeviceBondStateEventInt;

/**
 * Event for notifying Device Bond state
 */
typedef struct {
    BluetoothEventId event_id;
    bt_bond_state_t     state;
    bt_bdaddr_t         bd_addr;
    bt_bdname_t         bd_name;
} DeviceBondStateEvent;

/**
 * Event for notifying ACL state
 */
typedef struct {
    BluetoothEventId event_id;
    bt_status_t status;
    bt_bdaddr_t bd_addr;
    bt_acl_state_t state;
} ACLStateEvent;

/**
 * Event for notifying Discovery state
 */
typedef struct {
    BluetoothEventId event_id;
    bt_discovery_state_t state;
} DiscoveryStateEvent;

/**
 * Event for notifying pin request
 */
typedef struct {
    BluetoothEventId event_id;
    bt_bdaddr_t         bd_addr;
    bt_bdname_t         bd_name;
    uint32_t           cod;
    bool               secure;
} PINRequestEvent;

/**
 * Event to post pin reply
 */
typedef struct {
    BluetoothEventId event_id;
    bt_bdaddr_t         bd_addr;
    bt_bdname_t         bd_name;
    uint8_t            pin_len;
    bool               secure;
    bt_pin_code_t       pincode;
} PINReplyEvent;

/**
 * Event for notifying pin request
 */
typedef struct {
    BluetoothEventId event_id;
    bt_bdaddr_t         bd_addr;
    bt_bdname_t         bd_name;
    uint32_t           cod;
    bt_ssp_variant_t    pairing_variant;
    uint32_t           pass_key;
} SSPRequestEvent;

/**
 * Event to post ssp reply
 */
typedef struct {
    BluetoothEventId event_id;
    bt_bdaddr_t         bd_addr;
    bt_bdname_t         bd_name;
    uint32_t           cod;
    bt_ssp_variant_t    pairing_variant;
    uint32_t           pass_key;
    uint8_t            accept;
} SSPReplyEvent;

/**
 * Event for notifying Device Discover
 */
typedef struct {
    BluetoothEventId event_id;
} DeviceDiscoverRequest;

/**
 * Event for notifying Device Bond
 */
typedef struct {
    BluetoothEventId event_id;
    bt_bdaddr_t         bd_addr;
} DeviceBondRequest;

/**
 * Event for notifying Device connect
 */
typedef struct {
    BluetoothEventId    event_id;
    bt_bdaddr_t         bd_addr;
} DeviceConnectRequest;

/**
 * Event for notifying Device disconnect
 */
typedef struct {
    BluetoothEventId event_id;
    bt_bdaddr_t         bd_addr;
} DeviceDisconnectRequest;

/**
 * API to start Profile
 */
typedef struct {
    BluetoothEventId event_id;
} ProfileStartRequest;

/**
 * Event for notifying Profile start status
 */
typedef struct {
    BluetoothEventId event_id;
    ProfileIdType    profile_id;
    bool             status;
} ProfileStartEvent;


/**
 * API to stop Profile
 */
typedef struct {
    BluetoothEventId event_id;
} ProfileStopRequest;

/**
 * Event for notifying Profile stop status
 */
typedef struct {
    BluetoothEventId event_id;
    ProfileIdType    profile_id;
    bool             status;
} ProfileStopEvent;

typedef struct {
    BluetoothEventId   event_id;
    bt_bdaddr_t         bd_addr;
} A2dpSinkEvent;

typedef struct {
    BluetoothEventId   event_id;
    bt_bdaddr_t         bd_addr;
    uint8_t             key_id;
} AvrcpCtrlPassThruCmdReq;

/**
 * Event for notifying Pan control state
 */
typedef struct {
    BluetoothEventId event_id;
    uint8_t local_role;
    uint8_t state;
    uint8_t error;
    const char *ifname;
} PanControlStateEvent;

/**
 * Event for notifying Pan connection state
 */
typedef struct {
    BluetoothEventId event_id;
    bt_bdaddr_t bd_addr;
    uint8_t local_role;
    uint8_t remote_role;
    uint8_t state;
    uint8_t error;
} PanConnectionStateEvent;

/**
 * Event for notifying tethering on/off from UI
 */
typedef struct {
    BluetoothEventId event_id;
    bool is_tethering_on;
} PanSetTetheringEvent;

/**
 * Event for notifying Pan disconnect
 */
typedef struct {
    BluetoothEventId event_id;
    bt_bdaddr_t bd_addr;
} PanDeviceDisconnectEvent;

/**
 * Event for notifying Pan connect
 */
typedef struct {
    BluetoothEventId event_id;
    bt_bdaddr_t bd_addr;
} PanDeviceConnectEvent;

/**
 * Event for notifying Pan connected device list
 */
typedef struct {
    BluetoothEventId event_id;
} PanDeviceConnectedListEvent;

/**
  * @brief BT IPC message between qcbtdaemon & btapp
  */
typedef struct{
    /**
     * It can be any value of bt_ipc_type
     */
    uint8_t type;
    /**
     * It can be any value of bt_ipc_status
     */
    uint8_t status;
} BtIpcMsg;

typedef struct {
    BluetoothEventId event_id;
    BtIpcMsg ipc_msg;
} BtIpcMsgEvent;

typedef union {
    BluetoothEventId                event_id;
    GapAppEvent                     state_event;
    SSPRequestEvent                 ssp_request_event;
    SSPReplyEvent                   ssp_reply_event;
    PINRequestEvent                 pin_request_event;
    PINReplyEvent                   pin_reply_event;
    DiscoveryStateEvent             discovery_state_event;
    ACLStateEvent                   acl_state_event;
    DeviceBondStateEvent            bond_state_event;
    DeviceBondStateEventInt         bond_state_event_int;
    DeviceFoundEvent                device_found_event;
    DeviceFoundEventInt             device_found_event_int;
    RemotePropertiesEvent           remote_properties_event;
    AdapterPropertiesEvent          adapater_properties_event;
    DeviceDiscoverRequest           discover_request;
    DeviceBondRequest               bond_device;
    ProfileStartRequest             profile_start_request;
    ProfileStopRequest              profile_stop_request;
    ProfileStartEvent               profile_start_event;
    ProfileStopEvent                profile_stop_event;
    A2dpSinkEvent                   a2dpSinkEvent;
    AvrcpCtrlPassThruCmdReq         avrcpCtrlEvent;
    PanControlStateEvent            pan_control_state_event;
    PanConnectionStateEvent         pan_connection_state_event;
    PanSetTetheringEvent            pan_set_tethering_event;
    PanDeviceDisconnectEvent        pan_device_disconnect_event;
    PanDeviceConnectEvent           pan_device_connect_event;
    PanDeviceConnectedListEvent     pan_device_connected_list_event;
    BtIpcMsgEvent                   bt_ipc_msg_event;
} BtEvent;

/**
  * @brief BT IPC message type
  */
typedef enum{
    /**
     * ipc message to enable tethering
     */
    BT_IPC_ENABLE_TETHERING = 0x01,
    /**
     * ipc message to disable tethering
     */
    BT_IPC_DISABLE_TETHERING,
    /**
     * ipc message to start WLAN
     */
    BT_IPC_REMOTE_START_WLAN,
    BT_IPC_INAVALID = 0xFF
} bt_ipc_type;

/**
 * @brief BT IPC message status
 */
typedef enum{
    SUCCESS = 0x00,
    FAILED,
    INITIATED,
    INVALID = 0xFF
} bt_ipc_status;

#ifdef __cplusplus
extern "C" {
#endif

typedef char bdstr_t[MAX_BD_STR_LEN];
void PostMessage(ThreadIdType thread_id, void *msg);
void BtGapMsgHandler(void *context);
void BtMainMsgHandler(void *context);
void BtSocketMsgHandler (void *context);
void BtA2dpSinkMsgHandler(void *msg);
void BtPanMsgHandler(void *context);

#ifdef __cplusplus
}
#endif
