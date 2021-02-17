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

typedef enum {
  BLE_IPC_STATUS_SUCCESS,
  BLE_IPC_STATUS_FAILED,
  BLE_IPC_STATUS_WLAN_DPP_BOOTSTRAP_MODE_ALREADY_ENABLED,
  BLE_IPC_STATUS_WLAN_DPP_BOOTSTRAP_MODE_ALREADY_DISABLED,
  BLE_IPC_STATUS_WLAN_DPP_BOOTSTRAP_MODE_ADV_DATA_TOO_BIG,
} BleIpcStatus;

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

typedef union {
  BleIpcEventId eventId;
  WlanDppBootstrapModeEnableReqEvent wlanDppBootstrapModeEnableReqEvent;
  WlanDppBootstrapModeEnableRspEvent wlanDppBootstrapModeEnableRspEvent;
  WlanDppBootstrapModeDisableReqEvent wlanDppBootstrapModeDisableReqEvent;
  WlanDppBootstrapModeDisableRspEvent wlanDppBootstrapModeDisableRspEvent;
} ble_ipc_msg_t;

#define BLE_IPC_MSG_LEN sizeof(ble_ipc_msg_t)

#endif
