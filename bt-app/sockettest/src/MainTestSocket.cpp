/******************************************************************************
 *
 * Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 *
 ******************************************************************************/

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <netinet/in.h>
#include "../bt-app/ble_sock_manager/include/BleSockIf.hpp"

#ifdef USE_GLIB
#include <glib.h>
#define strlcpy g_strlcpy
#endif

#ifdef __cplusplus
extern "C"
{
#endif

static int fd = -1;

void SetupSocket() {

  struct sockaddr_un addr;
  char sockName[200] = "/data/misc/bluetooth/ble_gap_socket";
  printf("%s", __func__);

  if ((fd = socket(AF_UNIX, SOCK_STREAM, 0)) < 0) {
     printf("%s:socket create failed, %s", __func__, strerror(errno));
     return;
  }

  memset(&addr, 0, sizeof(addr));
  addr.sun_family = AF_UNIX;
  strlcpy(addr.sun_path, sockName, sizeof(addr.sun_path));

  if(connect(fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
    printf("%s: connect failed = %s", __func__, strerror(errno));
    close(fd);
    fd = -1;
    return;
  }
}

void TearDownSocket() {
  close(fd);
}

static void RxProcessing(ble_ipc_msg_t * evt) {
  switch (evt->eventId) {
    case BLE_IPC_MSG_GAP_ADVERTISE_ENABLE_RSP:
      printf("%s: adv enable rsp = %d\n", __func__, evt->bleGapAdvertiseEnableRspEvent.status);
    break;
    case BLE_IPC_MSG_GAP_ADVERTISE_DISABLE_RSP:
      printf("%s: adv disable rsp = %d\n", __func__, evt->bleGapAdvertiseDisableRspEvent.status);
    break;
    case BLE_IPC_MSG_GAP_SCAN_ENABLE_RSP:
      printf("%s: scan enable rsp = %d\n", __func__, evt->bleGapScanEnableRspEvent.status);
    break;
    case BLE_IPC_MSG_GAP_SCAN_RESULT:
      printf("%s: scan result\n", __func__);
      printf("%s: legacy=%d, connectable=%d, rssi=%d, txpower=%d, adv_sid=%d, periodic_adv_int=%d, p_phy=%d, s_phy=%d\n", __func__,
        evt->bleGapScanResultEvent.is_legacy, evt->bleGapScanResultEvent.is_connectable,
        evt->bleGapScanResultEvent.rssi, evt->bleGapScanResultEvent.tx_power,
        evt->bleGapScanResultEvent.adv_sid, evt->bleGapScanResultEvent.periodic_adv_int,
        evt->bleGapScanResultEvent.primary_phy, evt->bleGapScanResultEvent.secondary_phy);
      printf("%s: dev_name_len = %d, dev_name=\"", __func__, evt->bleGapScanResultEvent.device_name_len);
      for(int i=0; i<evt->bleGapScanResultEvent.device_name_len; i++)
	    printf("%c", evt->bleGapScanResultEvent.device_name[i]);
      printf("\"\n");
    break;
    case BLE_IPC_MSG_GAP_SCAN_DISABLE_RSP:
      printf("%s: scan disable rsp = %d\n", __func__, evt->bleGapScanDisableRspEvent.status);
    break;
    default:
    break;
  }
}

static void SendMsgToSocket(ble_ipc_msg_t * ipc_msg) {
    if (write(fd, ipc_msg, sizeof(ble_ipc_msg_t)) != sizeof(ble_ipc_msg_t)) {
      printf("%s: write failed\n", __func__);
    }
}

static void *ThreadRxHandler(void * ptr) {
  char buf[BLE_IPC_MSG_LEN];
  int i, readCnt;
  if (fd > 0) {
    while ((readCnt=read(fd,buf,sizeof(buf))) > 0) {
      printf("%s: Read, bytes = %d\n", __func__, readCnt);
      if(readCnt != BLE_IPC_MSG_LEN) {
        printf ("%s: Error, recv diff than IPC LEN, len = %d\n", __func__, readCnt);
      } else {
        ble_ipc_msg_t evt;
        memcpy(&evt, buf, sizeof(ble_ipc_msg_t));
        RxProcessing (&evt);
      }
    }
    if (readCnt < 0){
      printf("%s: read error = %s\n", __func__, strerror);
    }
    if (readCnt == 0){
      printf("%s: Remote closed socket\n", __func__);
    }
  } else {
    printf("%s: Socket error\n", __func__);
  }
  return NULL;
}

void InitReadThread() {
  printf("%s\n", __func__);
  static pthread_t thread_test_config;
  int ret = pthread_create(&thread_test_config, NULL, ThreadRxHandler, NULL);
}

void ExecuteTestCases() {
  ble_ipc_msg_t msg;
  printf("%s\n", __func__);
  // Start Adv
  printf("%s: start back to back adv\n", __func__);
  printf("%s: start adv 1\n", __func__);
  memset(&msg, 0, sizeof(ble_ipc_msg_t));
  msg.eventId = BLE_IPC_MSG_GAP_ADVERTISE_ENABLE_REQ;
  msg.bleGapAdvertiseEnableReqEvent.info.adv_data.service_data_uuid16 = 0x1812;
  msg.bleGapAdvertiseEnableReqEvent.info.adv_data.service_data_len = 2;
  msg.bleGapAdvertiseEnableReqEvent.info.adv_data.service_data[0] = 2;
  msg.bleGapAdvertiseEnableReqEvent.info.adv_data.service_data[1] = 3;
  msg.bleGapAdvertiseEnableReqEvent.info.adv_data.include_device_name = true;
  msg.bleGapAdvertiseEnableReqEvent.info.adv_data.include_tx_power_level = true;
  msg.bleGapAdvertiseEnableReqEvent.info.interval = 160;
  msg.bleGapAdvertiseEnableReqEvent.info.tx_power_level = 1;
  msg.bleGapAdvertiseEnableReqEvent.info.duration_msec = 30*1000;
  SendMsgToSocket(&msg);
  // Stop Adv
  printf("%s: stop adv 1\n", __func__);
  memset(&msg, 0, sizeof(ble_ipc_msg_t));
  msg.eventId = BLE_IPC_MSG_GAP_ADVERTISE_DISABLE_REQ;
  SendMsgToSocket(&msg);
  // Start Adv
  printf("%s: start adv 2\n", __func__);
  memset(&msg, 0, sizeof(ble_ipc_msg_t));
  msg.eventId = BLE_IPC_MSG_GAP_ADVERTISE_ENABLE_REQ;
  msg.bleGapAdvertiseEnableReqEvent.info.adv_data.service_data_uuid16 = 0x1812;
  msg.bleGapAdvertiseEnableReqEvent.info.adv_data.service_data_len = 2;
  msg.bleGapAdvertiseEnableReqEvent.info.adv_data.service_data[0] = 2;
  msg.bleGapAdvertiseEnableReqEvent.info.adv_data.service_data[1] = 3;
  msg.bleGapAdvertiseEnableReqEvent.info.adv_data.include_device_name = true;
  msg.bleGapAdvertiseEnableReqEvent.info.adv_data.include_tx_power_level = true;
  msg.bleGapAdvertiseEnableReqEvent.info.interval = 160;
  msg.bleGapAdvertiseEnableReqEvent.info.tx_power_level = 1;
  msg.bleGapAdvertiseEnableReqEvent.info.duration_msec = 30*1000;
  SendMsgToSocket(&msg);
  // Stop Adv
  printf("%s: stop adv 2\n", __func__);
  memset(&msg, 0, sizeof(ble_ipc_msg_t));
  msg.eventId = BLE_IPC_MSG_GAP_ADVERTISE_DISABLE_REQ;
  SendMsgToSocket(&msg);
  // Start Adv
  printf("%s: start adv 3\n", __func__);
  memset(&msg, 0, sizeof(ble_ipc_msg_t));
  msg.eventId = BLE_IPC_MSG_GAP_ADVERTISE_ENABLE_REQ;
  msg.bleGapAdvertiseEnableReqEvent.info.adv_data.service_data_uuid16 = 0x1812;
  msg.bleGapAdvertiseEnableReqEvent.info.adv_data.service_data_len = 2;
  msg.bleGapAdvertiseEnableReqEvent.info.adv_data.service_data[0] = 2;
  msg.bleGapAdvertiseEnableReqEvent.info.adv_data.service_data[1] = 3;
  msg.bleGapAdvertiseEnableReqEvent.info.adv_data.include_device_name = true;
  msg.bleGapAdvertiseEnableReqEvent.info.adv_data.include_tx_power_level = true;
  msg.bleGapAdvertiseEnableReqEvent.info.interval = 160;
  msg.bleGapAdvertiseEnableReqEvent.info.tx_power_level = 1;
  msg.bleGapAdvertiseEnableReqEvent.info.duration_msec = 30*1000;
  SendMsgToSocket(&msg);
  // Stop Adv
  printf("%s: stop adv 3\n", __func__);
  memset(&msg, 0, sizeof(ble_ipc_msg_t));
  msg.eventId = BLE_IPC_MSG_GAP_ADVERTISE_DISABLE_REQ;
  SendMsgToSocket(&msg);
  // Start Adv
  printf("%s: start adv 4\n", __func__);
  memset(&msg, 0, sizeof(ble_ipc_msg_t));
  msg.eventId = BLE_IPC_MSG_GAP_ADVERTISE_ENABLE_REQ;
  msg.bleGapAdvertiseEnableReqEvent.info.adv_data.service_data_uuid16 = 0x1812;
  msg.bleGapAdvertiseEnableReqEvent.info.adv_data.service_data_len = 2;
  msg.bleGapAdvertiseEnableReqEvent.info.adv_data.service_data[0] = 2;
  msg.bleGapAdvertiseEnableReqEvent.info.adv_data.service_data[1] = 3;
  msg.bleGapAdvertiseEnableReqEvent.info.adv_data.include_device_name = true;
  msg.bleGapAdvertiseEnableReqEvent.info.adv_data.include_tx_power_level = true;
  msg.bleGapAdvertiseEnableReqEvent.info.interval = 160;
  msg.bleGapAdvertiseEnableReqEvent.info.tx_power_level = 1;
  msg.bleGapAdvertiseEnableReqEvent.info.duration_msec = 30*1000;
  SendMsgToSocket(&msg);
  // Stop Adv
  printf("%s: stop adv 4\n", __func__);
  memset(&msg, 0, sizeof(ble_ipc_msg_t));
  msg.eventId = BLE_IPC_MSG_GAP_ADVERTISE_DISABLE_REQ;
  SendMsgToSocket(&msg);
  // Start Adv
  printf("%s: start adv 5\n", __func__);
  memset(&msg, 0, sizeof(ble_ipc_msg_t));
  msg.eventId = BLE_IPC_MSG_GAP_ADVERTISE_ENABLE_REQ;
  msg.bleGapAdvertiseEnableReqEvent.info.adv_data.service_data_uuid16 = 0x1812;
  msg.bleGapAdvertiseEnableReqEvent.info.adv_data.service_data_len = 2;
  msg.bleGapAdvertiseEnableReqEvent.info.adv_data.service_data[0] = 2;
  msg.bleGapAdvertiseEnableReqEvent.info.adv_data.service_data[1] = 3;
  msg.bleGapAdvertiseEnableReqEvent.info.adv_data.include_device_name = true;
  msg.bleGapAdvertiseEnableReqEvent.info.adv_data.include_tx_power_level = true;
  msg.bleGapAdvertiseEnableReqEvent.info.interval = 160;
  msg.bleGapAdvertiseEnableReqEvent.info.tx_power_level = 1;
  msg.bleGapAdvertiseEnableReqEvent.info.duration_msec = 30*1000;
  SendMsgToSocket(&msg);
  // Stop Adv
  printf("%s: stop adv 5\n", __func__);
  memset(&msg, 0, sizeof(ble_ipc_msg_t));
  msg.eventId = BLE_IPC_MSG_GAP_ADVERTISE_DISABLE_REQ;
  SendMsgToSocket(&msg);
  printf("%s: stop back to back adv \n", __func__);
  sleep(10);

  printf("%s: start back to back scan\n", __func__);
  // Start Scanning
  printf("%s: start scan 1\n", __func__);
  memset(&msg, 0, sizeof(ble_ipc_msg_t));
  msg.eventId = BLE_IPC_MSG_GAP_SCAN_ENABLE_REQ;
  msg.bleGapScanEnableReqEvent.service_data_uuid16 = 0x1812;
  msg.bleGapScanEnableReqEvent.service_data_len = 2;
  msg.bleGapScanEnableReqEvent.service_data[0] = 2;
  msg.bleGapScanEnableReqEvent.service_data[1] = 3;
  msg.bleGapScanEnableReqEvent.service_data_mask_len = 2;
  msg.bleGapScanEnableReqEvent.service_data_mask[0] = 0xFF;
  msg.bleGapScanEnableReqEvent.service_data_mask[1] = 0xFF;
  msg.bleGapScanEnableReqEvent.scan_mode = 2;
  msg.bleGapScanEnableReqEvent.legacy = false;

  SendMsgToSocket(&msg);
  // Stop Scanning
  printf("%s: stop scan 1\n", __func__);
  memset(&msg, 0, sizeof(ble_ipc_msg_t));
  msg.eventId = BLE_IPC_MSG_GAP_SCAN_DISABLE_REQ;
  SendMsgToSocket(&msg);
  // Start Scanning
  printf("%s: start scan 2\n", __func__);
  memset(&msg, 0, sizeof(ble_ipc_msg_t));
  msg.eventId = BLE_IPC_MSG_GAP_SCAN_ENABLE_REQ;
  msg.bleGapScanEnableReqEvent.service_data_uuid16 = 0x1812;
  msg.bleGapScanEnableReqEvent.service_data_len = 2;
  msg.bleGapScanEnableReqEvent.service_data[0] = 2;
  msg.bleGapScanEnableReqEvent.service_data[1] = 3;
  msg.bleGapScanEnableReqEvent.service_data_mask_len = 2;
  msg.bleGapScanEnableReqEvent.service_data_mask[0] = 0xFF;
  msg.bleGapScanEnableReqEvent.service_data_mask[1] = 0xFF;
  msg.bleGapScanEnableReqEvent.scan_mode = 2;
  msg.bleGapScanEnableReqEvent.legacy = false;

  SendMsgToSocket(&msg);
  // Stop Scanning
  printf("%s: stop scan 2\n", __func__);
  memset(&msg, 0, sizeof(ble_ipc_msg_t));
  msg.eventId = BLE_IPC_MSG_GAP_SCAN_DISABLE_REQ;
  SendMsgToSocket(&msg);
  // Start Scanning
  printf("%s: start scan 3\n", __func__);
  memset(&msg, 0, sizeof(ble_ipc_msg_t));
  msg.eventId = BLE_IPC_MSG_GAP_SCAN_ENABLE_REQ;
  msg.bleGapScanEnableReqEvent.service_data_uuid16 = 0x1812;
  msg.bleGapScanEnableReqEvent.service_data_len = 2;
  msg.bleGapScanEnableReqEvent.service_data[0] = 2;
  msg.bleGapScanEnableReqEvent.service_data[1] = 3;
  msg.bleGapScanEnableReqEvent.service_data_mask_len = 2;
  msg.bleGapScanEnableReqEvent.service_data_mask[0] = 0xFF;
  msg.bleGapScanEnableReqEvent.service_data_mask[1] = 0xFF;
  msg.bleGapScanEnableReqEvent.scan_mode = 2;
  msg.bleGapScanEnableReqEvent.legacy = false;

  SendMsgToSocket(&msg);
  // Stop Scanning
  printf("%s: stop scan 3\n", __func__);
  memset(&msg, 0, sizeof(ble_ipc_msg_t));
  msg.eventId = BLE_IPC_MSG_GAP_SCAN_DISABLE_REQ;
  SendMsgToSocket(&msg);
  // Start Scanning
  printf("%s: start scan 4\n", __func__);
  memset(&msg, 0, sizeof(ble_ipc_msg_t));
  msg.eventId = BLE_IPC_MSG_GAP_SCAN_ENABLE_REQ;
  msg.bleGapScanEnableReqEvent.service_data_uuid16 = 0x1812;
  msg.bleGapScanEnableReqEvent.service_data_len = 2;
  msg.bleGapScanEnableReqEvent.service_data[0] = 2;
  msg.bleGapScanEnableReqEvent.service_data[1] = 3;
  msg.bleGapScanEnableReqEvent.service_data_mask_len = 2;
  msg.bleGapScanEnableReqEvent.service_data_mask[0] = 0xFF;
  msg.bleGapScanEnableReqEvent.service_data_mask[1] = 0xFF;
  msg.bleGapScanEnableReqEvent.scan_mode = 2;
  msg.bleGapScanEnableReqEvent.legacy = false;

  SendMsgToSocket(&msg);
  // Stop Scanning
  printf("%s: stop scan 4\n", __func__);
  memset(&msg, 0, sizeof(ble_ipc_msg_t));
  msg.eventId = BLE_IPC_MSG_GAP_SCAN_DISABLE_REQ;
  SendMsgToSocket(&msg);
  // Start Scanning
  printf("%s: start scan 5\n", __func__);
  memset(&msg, 0, sizeof(ble_ipc_msg_t));
  msg.eventId = BLE_IPC_MSG_GAP_SCAN_ENABLE_REQ;
  msg.bleGapScanEnableReqEvent.service_data_uuid16 = 0x1812;
  msg.bleGapScanEnableReqEvent.service_data_len = 2;
  msg.bleGapScanEnableReqEvent.service_data[0] = 2;
  msg.bleGapScanEnableReqEvent.service_data[1] = 3;
  msg.bleGapScanEnableReqEvent.service_data_mask_len = 2;
  msg.bleGapScanEnableReqEvent.service_data_mask[0] = 0xFF;
  msg.bleGapScanEnableReqEvent.service_data_mask[1] = 0xFF;
  msg.bleGapScanEnableReqEvent.scan_mode = 2;
  msg.bleGapScanEnableReqEvent.legacy = false;

  SendMsgToSocket(&msg);
  // Stop Scanning
  printf("%s: stop scan 5\n", __func__);
  memset(&msg, 0, sizeof(ble_ipc_msg_t));
  msg.eventId = BLE_IPC_MSG_GAP_SCAN_DISABLE_REQ;
  SendMsgToSocket(&msg);

  printf("%s: stop back to back scan\n", __func__);
  sleep(10);

  // Start Adv
  memset(&msg, 0, sizeof(ble_ipc_msg_t));
  msg.eventId = BLE_IPC_MSG_GAP_ADVERTISE_ENABLE_REQ;
  msg.bleGapAdvertiseEnableReqEvent.info.adv_data.service_data_uuid16 = 0x1812;
  msg.bleGapAdvertiseEnableReqEvent.info.adv_data.service_data_len = 2;
  msg.bleGapAdvertiseEnableReqEvent.info.adv_data.service_data[0] = 2;
  msg.bleGapAdvertiseEnableReqEvent.info.adv_data.service_data[1] = 3;
  msg.bleGapAdvertiseEnableReqEvent.info.adv_data.include_device_name = true;
  msg.bleGapAdvertiseEnableReqEvent.info.adv_data.include_tx_power_level = true;
  msg.bleGapAdvertiseEnableReqEvent.info.interval = 160;
  msg.bleGapAdvertiseEnableReqEvent.info.tx_power_level = 1;
  msg.bleGapAdvertiseEnableReqEvent.info.duration_msec = 30*1000;
  SendMsgToSocket(&msg);
  sleep(3);

  // Start Scanning
  memset(&msg, 0, sizeof(ble_ipc_msg_t));
  msg.eventId = BLE_IPC_MSG_GAP_SCAN_ENABLE_REQ;
  msg.bleGapScanEnableReqEvent.service_data_uuid16 = 0x1812;
  msg.bleGapScanEnableReqEvent.service_data_len = 2;
  msg.bleGapScanEnableReqEvent.service_data[0] = 2;
  msg.bleGapScanEnableReqEvent.service_data[1] = 3;
  msg.bleGapScanEnableReqEvent.service_data_mask_len = 2;
  msg.bleGapScanEnableReqEvent.service_data_mask[0] = 0xFF;
  msg.bleGapScanEnableReqEvent.service_data_mask[1] = 0xFF;
  msg.bleGapScanEnableReqEvent.scan_mode = 2;
  msg.bleGapScanEnableReqEvent.legacy = false;

  SendMsgToSocket(&msg);
  sleep(10);

  // Stop Adv
  memset(&msg, 0, sizeof(ble_ipc_msg_t));
  msg.eventId = BLE_IPC_MSG_GAP_ADVERTISE_DISABLE_REQ;
  SendMsgToSocket(&msg);
  sleep(3);

  // Start Adv again
  memset(&msg, 0, sizeof(ble_ipc_msg_t));
  msg.eventId = BLE_IPC_MSG_GAP_ADVERTISE_ENABLE_REQ;
  msg.bleGapAdvertiseEnableReqEvent.info.adv_data.service_data_uuid16 = 0x1812;
  msg.bleGapAdvertiseEnableReqEvent.info.adv_data.service_data_len = 2;
  msg.bleGapAdvertiseEnableReqEvent.info.adv_data.service_data[0] = 4;
  msg.bleGapAdvertiseEnableReqEvent.info.adv_data.service_data[1] = 5;
  msg.bleGapAdvertiseEnableReqEvent.info.adv_data.include_device_name = true;
  msg.bleGapAdvertiseEnableReqEvent.info.adv_data.include_tx_power_level = true;
  msg.bleGapAdvertiseEnableReqEvent.info.interval = 160;
  msg.bleGapAdvertiseEnableReqEvent.info.tx_power_level = 1;
  msg.bleGapAdvertiseEnableReqEvent.info.duration_msec = 5*1000;
  SendMsgToSocket(&msg);
  sleep(3);

  // Stop Scanning
  memset(&msg, 0, sizeof(ble_ipc_msg_t));
  msg.eventId = BLE_IPC_MSG_GAP_SCAN_DISABLE_REQ;
  SendMsgToSocket(&msg);
  sleep(3);

  // Start Adv again
  memset(&msg, 0, sizeof(ble_ipc_msg_t));
  msg.eventId = BLE_IPC_MSG_GAP_ADVERTISE_ENABLE_REQ;
  msg.bleGapAdvertiseEnableReqEvent.info.adv_data.service_data_uuid16 = 0x1812;
  msg.bleGapAdvertiseEnableReqEvent.info.adv_data.service_data_len = 2;
  msg.bleGapAdvertiseEnableReqEvent.info.adv_data.service_data[0] = 6;
  msg.bleGapAdvertiseEnableReqEvent.info.adv_data.service_data[1] = 7;
  msg.bleGapAdvertiseEnableReqEvent.info.adv_data.include_device_name = true;
  msg.bleGapAdvertiseEnableReqEvent.info.adv_data.include_tx_power_level = true;
  msg.bleGapAdvertiseEnableReqEvent.info.interval = 160;
  msg.bleGapAdvertiseEnableReqEvent.info.tx_power_level = 1;
  msg.bleGapAdvertiseEnableReqEvent.info.duration_msec = 10*1000;
  SendMsgToSocket(&msg);
  sleep(3);
}

int main (int argc, char *argv[]) {
  ble_ipc_msg_t msg;
  printf("%s\n", __func__);
  SetupSocket();
  InitReadThread();
  ExecuteTestCases();
  TearDownSocket();
  return 0;
}

#ifdef __cplusplus
}
#endif


