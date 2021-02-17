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

#include "BleSockIf.hpp"
#include "BleWlanDppBootstrap.hpp"
#include "BleSocketManager.hpp"
#include "ipc.hpp"
#include <sys/socket.h>
#include <sys/un.h>

#define LOGTAG "BleSocketManager: "

BleSocketManager *g_ble_socket_manager;
BleWlanDppBootstrap *g_ble_wlan_dpp_bootstrap;
extern ThreadInfo threadInfo[THREAD_ID_MAX];

#ifdef __cplusplus
    extern "C"
    {
#endif

void BtLeSocketMsgHandler(void *msg)
{
  BtEvent* event = NULL;
  if(!msg) {
    ALOGE(LOGTAG "%s: Msg is null, return", __FUNCTION__);
    return;
  }

  event = ( BtEvent *) msg;

  if (event == NULL)
  {
    ALOGE(LOGTAG "%s: event is null", __FUNCTION__);
    return;
  }

  ALOGI(LOGTAG "BtLeSocketMsgHandler event = %d g_ble_socket_manager: %p",
      event->event_id, g_ble_socket_manager);
  switch(event->event_id) {
    case PROFILE_API_START:
      if (g_ble_socket_manager) {
        int status = g_ble_socket_manager->init();

        BtEvent *start_event = new BtEvent;
        start_event->profile_start_event.event_id = PROFILE_EVENT_START_DONE;
        start_event->profile_start_event.profile_id = PROFILE_ID_BLE_SM;
        start_event->profile_start_event.status = (status == 0) ? true: false;
        PostMessage(THREAD_ID_GAP, start_event);
      }
      break;

    case PROFILE_API_STOP:
      if (g_ble_socket_manager) {
        g_ble_socket_manager->deinit();

        BtEvent *stop_event = new BtEvent;
        stop_event->profile_start_event.event_id = PROFILE_EVENT_STOP_DONE;
        stop_event->profile_start_event.profile_id = PROFILE_ID_BLE_SM;
        stop_event->profile_start_event.status = true;
        PostMessage(THREAD_ID_GAP, stop_event);
      }
      break;
    default:
      break;
  }
  delete event;
}

#ifdef __cplusplus
}
#endif

BleSocketManager :: BleSocketManager(const bt_interface_t *bt_interface, config_t *config)
{

}
BleSocketManager :: ~BleSocketManager()
{
}

int BleSocketManager :: init()
{
  int status = -1;

  cleanup_due_to_deinit_ = false;
  wbds_client_socket_ = -1;
  wbds_listen_socket_local_ = -1;
  wbds_listen_reactor_ = NULL;
  wbds_accept_reactor_ = NULL;
  wbds_thread_obj_ = NULL;

  status = WBDSSocketCreate();

  if (status != -1) {
    wbds_thread_obj_ = thread_new ("BLE_SM_WBDS_TASK");

    if (wbds_thread_obj_) {
      wbds_listen_reactor_ = reactor_register (thread_get_reactor(wbds_thread_obj_),
          wbds_listen_socket_local_, NULL, WBDSSocketListenHandler, NULL);
      if (wbds_listen_reactor_ != NULL) {
        status = 0;
        g_ble_wlan_dpp_bootstrap = new BleWlanDppBootstrap();
      }
    }
  }

  ALOGD(LOGTAG "%s : wbds_listen_reactor_= %p, wbds_thread_obj_=%p, status = %d",
      __FUNCTION__, wbds_listen_reactor_, wbds_thread_obj_, status);

  if (status == -1) {
    deinit();
  }

  return status;
}
void BleSocketManager :: deinit()
{
  ALOGD(LOGTAG "%s : wbds_listen_reactor_= %p, wbds_accept_reactor_= %p, wbds_thread_obj_ = %p",
    __FUNCTION__, wbds_listen_reactor_, wbds_accept_reactor_, wbds_thread_obj_);

  cleanup_due_to_deinit_ = true;
  cleanup();

  cleanup_due_to_deinit_ = false;

  if (wbds_accept_reactor_) {
    reactor_unregister ( wbds_accept_reactor_);
    wbds_accept_reactor_ = NULL;
  }

  if (wbds_listen_reactor_) {
    reactor_unregister ( wbds_listen_reactor_);
    wbds_listen_reactor_ = NULL;
  }

  if (wbds_thread_obj_) {
    thread_free(wbds_thread_obj_);
  }

  ALOGD(LOGTAG "%s : wbds_client_socket_= %d, wbds_listen_socket_local_= %d",
    __FUNCTION__, wbds_client_socket_, wbds_listen_socket_local_);

  if (wbds_client_socket_ != -1) {
    close(wbds_client_socket_);
    wbds_client_socket_ = -1;
  }

  if (wbds_listen_socket_local_ != -1) {
    close(wbds_listen_socket_local_);
    wbds_listen_socket_local_ = -1;
  }

  if (g_ble_wlan_dpp_bootstrap) {
    delete g_ble_wlan_dpp_bootstrap;
    g_ble_wlan_dpp_bootstrap = NULL;
  }
}

void BleSocketManager :: cleanup()
{

  ALOGD(LOGTAG "%s ", __FUNCTION__);

  if (g_ble_wlan_dpp_bootstrap->isBootstrapModeEnabled()) {
    g_ble_wlan_dpp_bootstrap->WlanDppBootstrapModeDisableReq();
  }
}

int BleSocketManager:: WBDSSocketCreate(void) {
  int conn_sk, length;
  struct sockaddr_un addr;

  ALOGI (LOGTAG "%s", __FUNCTION__);
  wbds_listen_socket_local_ = socket(AF_LOCAL, SOCK_STREAM, 0);
  if (wbds_listen_socket_local_ < 0) {
    ALOGE (LOGTAG "%s : Failed to create Local Socket: %s", __FUNCTION__, strerror(errno));
    return -1;
  }

  memset(&addr, 0, sizeof(addr));
  addr.sun_family = AF_LOCAL;
  strlcpy(addr.sun_path, BLE_WBDS_SOCKET_NAME, sizeof(addr.sun_path));
  unlink(BLE_WBDS_SOCKET_NAME);
  if (bind(wbds_listen_socket_local_, (struct sockaddr*)&addr, sizeof(addr)) == -1) {
    ALOGE (LOGTAG "%s : Failed to bind Local Socket: %s", __FUNCTION__, strerror(errno));
    close(wbds_listen_socket_local_);
    wbds_listen_socket_local_ = -1;
    return -1;
  }

  if (listen(wbds_listen_socket_local_, 1) < 0) {
    ALOGE (LOGTAG "%s : Failed to listen Local Socket: %s", __FUNCTION__, strerror(errno));
    close(wbds_listen_socket_local_);
    wbds_listen_socket_local_ = -1;
    return -1;
  }

  return 0;
}

void BleSocketManager:: WBDSSocketWriteHandler (ble_ipc_msg_t *ipc_msg) {
  int len;

  ALOGD (LOGTAG "%s", __FUNCTION__);
  if (wbds_client_socket_ != -1) {
    if((len = send(wbds_client_socket_, ipc_msg, BLE_IPC_MSG_LEN, 0)) < 0) {
      reactor_unregister (wbds_accept_reactor_);
      wbds_accept_reactor_ = NULL;
      close(wbds_client_socket_);
      wbds_client_socket_ = -1;

      cleanup();

      ALOGE (LOGTAG "%s : Local socket send fail %s", __FUNCTION__, strerror(errno));
    } else {
      ALOGI (LOGTAG "%s: sent %d bytes", __FUNCTION__, len);
    }
  }
}

void WBDSSocketListenHandler (void *context) {
  struct sockaddr_un cliaddr;
  int length;

  ALOGI (LOGTAG "%s", __FUNCTION__);

  if (g_ble_socket_manager->wbds_client_socket_ == -1) {
      g_ble_socket_manager->wbds_client_socket_ =
          accept(g_ble_socket_manager->wbds_listen_socket_local_,
          (struct sockaddr*) &cliaddr, ( socklen_t *) &length);
    if (g_ble_socket_manager->wbds_client_socket_ == -1) {
      ALOGE (LOGTAG "%s: error accepting LOCAL socket: %s",
          __FUNCTION__, strerror(errno));
    } else {
      g_ble_socket_manager->wbds_accept_reactor_ = reactor_register(
          thread_get_reactor(g_ble_socket_manager->wbds_thread_obj_),
          g_ble_socket_manager->wbds_client_socket_, NULL, WBDSSocketDataHandler, NULL);
    }
  } else {
    ALOGI (LOGTAG "%s: Accepting and closing the next connection", __FUNCTION__);

    int accept_socket = accept(g_ble_socket_manager->wbds_listen_socket_local_,
        (struct sockaddr*) &cliaddr, ( socklen_t *) &length);
    if (accept_socket)
      close(accept_socket);
  }
}

void WBDSSocketDataHandler (void *context) {
  ble_ipc_msg_t ipc_msg  = {};
  int len;

  ALOGD (LOGTAG "%s", __FUNCTION__);
  if(g_ble_socket_manager->wbds_client_socket_ != -1) {
    len = recv(g_ble_socket_manager->wbds_client_socket_, &ipc_msg, BLE_IPC_MSG_LEN, 0);
    ALOGD (LOGTAG "%s : len = %d", __FUNCTION__, len);

    if (len <= 0) {
      ALOGE (LOGTAG "%s: Not able to receive msg to remote dev: %s",
         __FUNCTION__, strerror(errno));
      reactor_unregister (g_ble_socket_manager->wbds_accept_reactor_);
      g_ble_socket_manager->wbds_accept_reactor_ = NULL;
      close(g_ble_socket_manager->wbds_client_socket_);
      g_ble_socket_manager->wbds_client_socket_ = -1;

      g_ble_socket_manager->cleanup();
    } else if(len == BLE_IPC_MSG_LEN) {
      switch (ipc_msg.eventId) {
        case BLE_IPC_MSG_WLAN_DPP_BOOTSTRAP_MODE_ENABLE_REQ:
          ALOGI (LOGTAG "%s: BLE_IPC_MSG_WLAN_DPP_BOOTSTRAP_MODE_ENABLE_REQ", __FUNCTION__);
          g_ble_wlan_dpp_bootstrap->WlanDppBootstrapModeEnableReq(
              &(ipc_msg.wlanDppBootstrapModeEnableReqEvent.bootstrapInfo));
          break;

        case BLE_IPC_MSG_WLAN_DPP_BOOTSTRAP_MODE_DISABLE_REQ:
          ALOGI (LOGTAG "%s: BLE_IPC_MSG_WLAN_DPP_BOOTSTRAP_MODE_DISABLE_REQ", __FUNCTION__);
          g_ble_wlan_dpp_bootstrap->WlanDppBootstrapModeDisableReq();
          break;

        default:
          ALOGW(LOGTAG "%s Msg not handled %d", __FUNCTION__, ipc_msg.eventId);;
          break;
      }
    }
  }
}
