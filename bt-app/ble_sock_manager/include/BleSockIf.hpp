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

#ifndef BLE_SOCK_IF_H
#define BLE_SOCK_IF_H
#pragma once

#include <stdint.h>

#define BLE_WBDS_SOCKET_NAME "/data/misc/bluetooth/ble_wbds_socket"
#define MAX_BOOTSTRAP_ADV_DATA_LEN 200
#define MAX_SERVICE_DATA_LEN 200

#define BD_ADDR_STR_LEN 18
#define WCS_CHAR_VALUE_MAX_LEN 32
#define DISABLE_NOTIFICATION 0x00
#define ENABLE_NOTIFICATION 0x01
// Mask for checking whether event type represents connectable advertisement.
#define CONNECTABLE_MASK 0x01

#define MAX_SERVICE_DATA_SCAN_FILTER_LEN 24

typedef enum {
  BLE_IPC_STATUS_SUCCESS,
  BLE_IPC_STATUS_FAILED,
  BLE_IPC_STATUS_WLAN_DPP_BOOTSTRAP_MODE_ALREADY_ENABLED,
  BLE_IPC_STATUS_WLAN_DPP_BOOTSTRAP_MODE_ALREADY_DISABLED,
  BLE_IPC_STATUS_WLAN_DPP_BOOTSTRAP_MODE_ADV_DATA_TOO_BIG,
  BLE_IPC_STATUS_WCS_NOT_REGISTERED,
  BLE_IPC_STATUS_WCS_PEER_DISCOVERY_SERVICE_DATA_SCAN_FILTER_TOO_BIG,
  BLE_IPC_STATUS_WCS_PEER_DISCOVERY_ALREADY_ENABLED,
  BLE_IPC_STATUS_WCS_PEER_DISCOVERY_ALREADY_DISABLED,
  BLE_IPC_STATUS_WCS_PEER_ALREADY_CONNECTED,
  BLE_IPC_STATUS_WCS_PEER_ALREADY_DISCONNECTED,
  BLE_IPC_STATUS_WCS_PEER_ABNORMALLY_DISCONNECTED
} BleIpcStatus;

typedef enum {
  WCS_WIFI_STATE,
  WCS_DEVICE_ID,
  WCS_PAP_CONFIG_ID,
  WCS_PAP_SSID,
  WCS_PAP_BSSID,
  WCS_PAP_COUNTRY_STRING,
  WCS_PAP_OPERATING_CLASS,
  WCS_PAP_CHANNEL_NUMBER,
} WCSCharacteristicType;

typedef enum{
  /**
   * ipc message is used enable wlan bootstrap mode using BLE
   */
  BLE_IPC_MSG_WLAN_DPP_BOOTSTRAP_MODE_ENABLE_REQ,
  /**
   * ipc message is used to reply to an BLE_IPC_MSG_WLAN_DPP_BOOTSTRAP_MODE_ENABLE_REQ event
   */
  BLE_IPC_MSG_WLAN_DPP_BOOTSTRAP_MODE_ENABLE_RSP,
  /**
   * ipc message is used to disable wlan bootstrap mode
   */
  BLE_IPC_MSG_WLAN_DPP_BOOTSTRAP_MODE_DISABLE_REQ,
  /**
   * ipc message is used to reply to an BLE_IPC_MSG_WLAN_DPP_BOOTSTRAP_MODE_DISABLE_REQ event
   */
  BLE_IPC_MSG_WLAN_DPP_BOOTSTRAP_MODE_DISABLE_RSP,
    /**
   * ipc message is used to start peer discovery using BLE
   */
  BLE_IPC_MSG_WCS_PEER_DISCOVERY_ENABLE_REQ,
  /**
   * ipc message is used to reply to an BLE_IPC_MSG_WCS_PEER_DISCOVERY_ENABLE_REQ event
   */
  BLE_IPC_MSG_WCS_PEER_DISCOVERY_ENABLE_RSP,
  /**
   * ipc message is used to stop peer discovery using BLE
   */
  BLE_IPC_MSG_WCS_PEER_DISCOVERY_DISABLE_REQ,
  /**
   * ipc message is used to reply to an BLE_IPC_MSG_WCS_PEER_DISCOVERY_DISABLE_REQ event
   */
  BLE_IPC_MSG_WCS_PEER_DISCOVERY_DISABLE_RSP,
  /**
   * ipc message is used to deliver peer discovery result to app
   */
  BLE_IPC_MSG_WCS_PEER_DISCOVERY_RESULT,
    /**
   * ipc message is used to connect particular peer device using BLE
   */
  BLE_IPC_MSG_WCS_CONNECT_PEER_REQ,
  /**
   * ipc message is used to reply to an BLE_IPC_MSG_WCS_CONNECT_PEER_REQ event
   */
  BLE_IPC_MSG_WCS_CONNECT_PEER_RSP,
  /**
   * ipc message is used to disconnect particular peer device using BLE
   */
  BLE_IPC_MSG_WCS_DISCONNECT_PEER_REQ,
  /**
   * ipc message is used to reply to an BLE_IPC_MSG_WCS_DISCONNECT_PEER_REQ event
   */
  BLE_IPC_MSG_WCS_DISCONNECT_PEER_RSP,
  /**
   * ipc message is used to indicate peer disconnection, which is not initiated by app
   */
  BLE_IPC_MSG_WCS_PEER_DISCONNECT_IND,
  /**
   * ipc message is used to notify particular characteristic value change
   */
  BLE_IPC_MSG_WCS_SEND_NOTIFICATION_REQ,
  /**
   * ipc message is used to reply to an BLE_IPC_MSG_WCS_SEND_NOTIFICATION_REQ event
   */
  BLE_IPC_MSG_WCS_SEND_NOTIFICATION_RSP,
  /**
   * ipc message is used to read the value of a given characteristic
   */
  BLE_IPC_MSG_WCS_CHARACTERISTIC_READ_REQ,
  /**
   * ipc message is used to reply to an BLE_IPC_MSG_WCS_CHARACTERISTIC_READ_REQ event
   */
  BLE_IPC_MSG_WCS_CHARACTERISTIC_READ_RSP,
  /**
   * ipc message is used to write the value of a given characteristic
   */
  BLE_IPC_MSG_WCS_CHARACTERISTIC_WRITE_REQ,
  /**
   * ipc message is used to reply to an BLE_IPC_MSG_WCS_CHARACTERISTIC_WRITE_REQ event
   */
  BLE_IPC_MSG_WCS_CHARACTERISTIC_WRITE_RSP,
  /**
   * ipc message is used to read the notification config value using CCCD, which is  associated
   * with particular characteristic
   */
  BLE_IPC_MSG_WCS_CCCD_READ_REQ,
  /**
   * ipc message is used to reply to an BLE_IPC_MSG_WCS_CCCD_READ_REQ event
   */
  BLE_IPC_MSG_WCS_CCCD_READ_RSP,
  /**
   * ipc message is used to update the notification config value using CCCD, which is  associated
   * with particular characteristic
   */
  BLE_IPC_MSG_WCS_CCCD_WRITE_REQ,
  /**
   * ipc message is used to reply to an BLE_IPC_MSG_WCS_CCCD_WRITE_REQ event
   */
  BLE_IPC_MSG_WCS_CCCD_WRITE_RSP,

  BLE_IPC_MSG_INAVALID = 0xFF
} BleIpcEventId;

typedef struct
{
  //16 bit service data uuid
  uint16_t service_data_uuid;
  // adv data len
  uint8_t adv_data_len;
  //adv data must include type, uriLength & uriInfo
  uint8_t adv_data[MAX_BOOTSTRAP_ADV_DATA_LEN];
} __attribute__((packed)) WlanDppBootstrapInfo;

typedef struct {
  BleIpcEventId eventId;
  WlanDppBootstrapInfo bootstrapInfo;
} __attribute__((packed)) WlanDppBootstrapModeEnableReqEvent;

typedef struct {
  BleIpcEventId eventId;
  BleIpcStatus status;
} __attribute__((packed)) WlanDppBootstrapModeEnableRspEvent;

typedef struct {
  BleIpcEventId eventId;
} __attribute__((packed)) WlanDppBootstrapModeDisableReqEvent;

typedef struct {
  BleIpcEventId eventId;
  BleIpcStatus status;
} __attribute__((packed)) WlanDppBootstrapModeDisableRspEvent;

typedef struct {
  BleIpcEventId eventId;
  //16 bit service data uuid
  uint16_t service_data_uuid;
  // service data len
  uint8_t service_data_len;
  //service data must include device id
  uint8_t service_data[MAX_SERVICE_DATA_SCAN_FILTER_LEN];
  // service data msak len
  uint8_t service_data_mask_len;
  //service data mask must include mask value, if mask len is greater than 0
  uint8_t service_data_mask[MAX_SERVICE_DATA_SCAN_FILTER_LEN];
} __attribute__((packed)) WCSPeerDiscoveryEnableReqEvent;

typedef struct {
  BleIpcEventId eventId;
  BleIpcStatus status;
} __attribute__((packed)) WCSPeerDiscoveryEnableRspEvent;

typedef struct {
  BleIpcEventId eventId;
} __attribute__((packed)) WCSPeerDiscoveryDisableReqEvent;

typedef struct {
  BleIpcEventId eventId;
  BleIpcStatus status;
} __attribute__((packed)) WCSPeerDiscoveryDisableRspEvent;

typedef struct {
  BleIpcEventId eventId;
  int advertise_flags;
  //16 bit service data uuid
  uint16_t service_data_uuid;
  char bd_addr[BD_ADDR_STR_LEN];
  // service data len
  uint8_t service_data_len;
  //service data must include device id
  uint8_t service_data[MAX_SERVICE_DATA_LEN];
} __attribute__((packed)) WCSPeerDiscoveryResultEvent;

typedef struct {
  BleIpcEventId eventId;
  char bd_addr[BD_ADDR_STR_LEN];
} __attribute__((packed)) WCSConnectPeerReqEvent;

typedef struct {
  BleIpcEventId eventId;
  char bd_addr[BD_ADDR_STR_LEN];
  BleIpcStatus status;
} __attribute__((packed)) WCSConnectPeerRspEvent;

typedef struct {
  BleIpcEventId eventId;
  char bd_addr[BD_ADDR_STR_LEN];
} __attribute__((packed)) WCSDisconnectPeerReqEvent;

typedef struct {
  BleIpcEventId eventId;
  char bd_addr[BD_ADDR_STR_LEN];
  BleIpcStatus status;
} __attribute__((packed)) WCSDisconnectPeerRspEvent;

typedef struct {
  BleIpcEventId eventId;
  char bd_addr[BD_ADDR_STR_LEN];
  BleIpcStatus status;
} __attribute__((packed)) WCSPeerDisconnectIndEvent;

typedef struct {
  BleIpcEventId eventId;
  WCSCharacteristicType char_type;
  char bd_addr[BD_ADDR_STR_LEN];
  uint8_t len;
  uint8_t value[WCS_CHAR_VALUE_MAX_LEN];
} __attribute__((packed)) WCSSendNotificationReqEvent;

typedef struct {
  BleIpcEventId eventId;
  WCSCharacteristicType char_type;
  char bd_addr[BD_ADDR_STR_LEN];
  BleIpcStatus status;
} __attribute__((packed)) WCSSendNotificationRspEvent;

typedef struct {
  BleIpcEventId eventId;
  WCSCharacteristicType char_type;
  char bd_addr[BD_ADDR_STR_LEN];
  int request_id;
  int offset;
} __attribute__((packed)) WCSCharacteristicReadReqEvent;

typedef struct {
  BleIpcEventId eventId;
  WCSCharacteristicType char_type;
  char bd_addr[BD_ADDR_STR_LEN];
  int request_id;
  int offset;
  uint8_t len;
  uint8_t value[WCS_CHAR_VALUE_MAX_LEN];
  BleIpcStatus status;
} __attribute__((packed)) WCSCharacteristicReadRspEvent;

typedef struct {
  BleIpcEventId eventId;
  WCSCharacteristicType char_type;
  char bd_addr[BD_ADDR_STR_LEN];
  int request_id;
  int offset;
  uint8_t len;
  bool rsp_needed;
  uint8_t value[WCS_CHAR_VALUE_MAX_LEN];
} __attribute__((packed)) WCSCharacteristicWriteReqEvent;

typedef struct {
  BleIpcEventId eventId;
  WCSCharacteristicType char_type;
  char bd_addr[BD_ADDR_STR_LEN];
  int request_id;
  int offset;
  uint8_t len;
  bool rsp_needed;
  uint8_t value[WCS_CHAR_VALUE_MAX_LEN];
  BleIpcStatus status;
} __attribute__((packed)) WCSCharacteristicWriteRspEvent;

typedef struct {
  BleIpcEventId eventId;
  WCSCharacteristicType char_type;
  char bd_addr[BD_ADDR_STR_LEN];
  int request_id;
  int offset;
 } __attribute__((packed)) WCSCCCDReadReqEvent;

typedef struct {
  BleIpcEventId eventId;
  WCSCharacteristicType char_type;
  char bd_addr[BD_ADDR_STR_LEN];
  int request_id;
  int offset;
  uint8_t value;
  BleIpcStatus status;
} __attribute__((packed)) WCSCCCDReadRspEvent;

typedef struct {
  BleIpcEventId eventId;
  WCSCharacteristicType char_type;
  char bd_addr[BD_ADDR_STR_LEN];
  int request_id;
  int offset;
  bool rsp_needed;
  uint8_t value;
} __attribute__((packed)) WCSCCCDWriteReqEvent;

typedef struct {
  BleIpcEventId eventId;
  WCSCharacteristicType char_type;
  char bd_addr[BD_ADDR_STR_LEN];
  int request_id;
  int offset;
  bool rsp_needed;
  uint8_t value;
  BleIpcStatus status;
} __attribute__((packed)) WCSCCCDWriteRspEvent;

typedef union {
  BleIpcEventId eventId;
  WlanDppBootstrapModeEnableReqEvent wlanDppBootstrapModeEnableReqEvent;
  WlanDppBootstrapModeEnableRspEvent wlanDppBootstrapModeEnableRspEvent;
  WlanDppBootstrapModeDisableReqEvent wlanDppBootstrapModeDisableReqEvent;
  WlanDppBootstrapModeDisableRspEvent wlanDppBootstrapModeDisableRspEvent;
  WCSPeerDiscoveryEnableReqEvent      wcsPeerDiscoveryEnableReqEvent;
  WCSPeerDiscoveryEnableRspEvent      wcsPeerDiscoveryEnableRspEvent;
  WCSPeerDiscoveryDisableReqEvent     wcsPeerDiscoveryDisableReqEvent;
  WCSPeerDiscoveryDisableRspEvent     wcsPeerDiscoveryDisableRspEvent;
  WCSPeerDiscoveryResultEvent         wcsPeerDiscoveryResultEvent;
  WCSConnectPeerReqEvent              wcsConnectPeerReqEvent;
  WCSConnectPeerRspEvent              wcsConnectPeerRspEvent;
  WCSDisconnectPeerReqEvent           wcsDisconnectPeerReqEvent;
  WCSDisconnectPeerRspEvent           wcsDisconnectPeerRspEvent;
  WCSPeerDisconnectIndEvent           wcsPeerDisconnectIndEvent;
  WCSSendNotificationReqEvent         wcsSendNotificationReqEvent;
  WCSSendNotificationRspEvent         wcsSendNotificationRspEvent;
  WCSCharacteristicReadReqEvent       wcsCharacteristicReadReqEvent;
  WCSCharacteristicReadRspEvent       wcsCharacteristicReadRspEvent;
  WCSCharacteristicWriteReqEvent      wcsCharacteristicWriteReqEvent;
  WCSCharacteristicWriteRspEvent      wcsCharacteristicWriteRspEvent;
  WCSCCCDReadReqEvent                 wcsCCCDReadReqEvent;
  WCSCCCDReadRspEvent                 wcsCCCDReadRspEvent;
  WCSCCCDWriteReqEvent                wcsCCCDWriteReqEvent;
  WCSCCCDWriteRspEvent                wcsCCCDWriteRspEvent;
} ble_ipc_msg_t;

#define BLE_IPC_MSG_LEN sizeof(ble_ipc_msg_t)

#endif
