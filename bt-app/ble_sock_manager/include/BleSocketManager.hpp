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

#ifndef BLE_SOCKET_MANAGER_H
#define BLE_SOCKET_MANAGER_H
#pragma once

#include <stdio.h>
#include <string.h>
#include "BleSockIf.hpp"
#include "osi/include/log.h"
#include "osi/include/thread.h"
#include "osi/include/config.h"
#include "osi/include/reactor.h"
#include <hardware/bluetooth.h>
#include <condition_variable>

#ifdef USE_GLIB
#include <glib.h>
#define strlcpy g_strlcpy
#endif

void WBDSSocketListenHandler(void *context);
void WBDSSocketDataHandler(void *context);

extern const char *BT_LE_SOCKET_MANAGER_ENABLED;

class BleSocketManager {
  private:
    int WBDSSocketCreate();

  public:
    static const int SOCKET_MANAGER_CLEANUP_TIMEOUT_MS = 2000;
    thread_t *wbds_thread_obj_;
    reactor_object_t *wbds_listen_reactor_;
    reactor_object_t *wbds_accept_reactor_;
    int wbds_listen_socket_local_;
    int wbds_client_socket_;
    std::condition_variable socket_manager_cleanup_;
    std::mutex socket_manager_cleanup_lock_;
    bool cleanup_due_to_deinit_;
    BleSocketManager(const bt_interface_t *bt_interface, config_t *config);
    ~BleSocketManager();
    int init();
    void deinit();
    void cleanup();
    void WBDSSocketWriteHandler(ble_ipc_msg_t *ipc_msg);
};

#endif
