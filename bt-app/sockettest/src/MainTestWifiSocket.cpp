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
  char sockName[200] = "/dev/socket/ble_wbds_socket";
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
    case BLE_IPC_MSG_WLAN_DPP_BOOTSTRAP_MODE_ENABLE_RSP:
      printf("%s: wlan dpp adv enable rsp = %d\n", __func__, evt->wlanDppBootstrapModeEnableRspEvent.status);
    break;
    case BLE_IPC_MSG_WLAN_DPP_BOOTSTRAP_MODE_DISABLE_RSP:
      printf("%s: wlan dpp adv disable rsp = %d\n", __func__, evt->wlanDppBootstrapModeDisableRspEvent.status);
    break;
    case BLE_IPC_MSG_WCS_PEER_DISCOVERY_ENABLE_RSP:
      printf("%s: wcs peer disc scan enable rsp = %d\n", __func__, evt->wcsPeerDiscoveryEnableRspEvent.status);
    break;
    case BLE_IPC_MSG_WCS_PEER_DISCOVERY_RESULT:
      printf("%s: scan result\n", __func__);
    break;
    case BLE_IPC_MSG_WCS_PEER_DISCOVERY_DISABLE_RSP:
      printf("%s: wcs peer disc scan disable rsp = %d\n", __func__, evt->wcsPeerDiscoveryDisableRspEvent.status);
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
  char buf[BLE_IPC_MSG_LEN + 1];
  int i, readCnt;
  if (fd > 0) {
    while ((readCnt=read(fd,buf,sizeof(buf))) > 0) {
      printf("%s: Read, bytes = %d\n", __func__, readCnt);
      for(i=0; i<readCnt;i++) {
        // printf("%s: bytes val = %c", __func__, buf[i]);
      }
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
  memset(&msg, 0, sizeof(ble_ipc_msg_t));
  msg.eventId = BLE_IPC_MSG_WLAN_DPP_BOOTSTRAP_MODE_ENABLE_REQ;
  msg.wlanDppBootstrapModeEnableReqEvent.bootstrapInfo.service_data_uuid = 0x1812;
  msg.wlanDppBootstrapModeEnableReqEvent.bootstrapInfo.adv_data_len = 2;
  msg.wlanDppBootstrapModeEnableReqEvent.bootstrapInfo.adv_data[0] = 7;
  msg.wlanDppBootstrapModeEnableReqEvent.bootstrapInfo.adv_data[1] = 8;
  SendMsgToSocket(&msg);
  sleep(3);

  // Start Scanning
  memset(&msg, 0, sizeof(ble_ipc_msg_t));
  msg.eventId = BLE_IPC_MSG_WCS_PEER_DISCOVERY_ENABLE_REQ;
  msg.wcsPeerDiscoveryEnableReqEvent.service_data_uuid = 0x1812;
  msg.wcsPeerDiscoveryEnableReqEvent.service_data_len = 2;
  msg.wcsPeerDiscoveryEnableReqEvent.service_data[0] = 4;
  msg.wcsPeerDiscoveryEnableReqEvent.service_data[1] = 5;
  msg.wcsPeerDiscoveryEnableReqEvent.service_data_mask_len = 2;
  msg.wcsPeerDiscoveryEnableReqEvent.service_data_mask[0] = 0xFF;
  msg.wcsPeerDiscoveryEnableReqEvent.service_data_mask[1] = 0xFF;
  SendMsgToSocket(&msg);
  sleep(120);

  // Stop Scanning
  memset(&msg, 0, sizeof(ble_ipc_msg_t));
  msg.eventId = BLE_IPC_MSG_WCS_PEER_DISCOVERY_DISABLE_REQ;
  SendMsgToSocket(&msg);
  sleep(3);

  // Stop Adv
  memset(&msg, 0, sizeof(ble_ipc_msg_t));
  msg.eventId = BLE_IPC_MSG_WLAN_DPP_BOOTSTRAP_MODE_DISABLE_REQ;
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


