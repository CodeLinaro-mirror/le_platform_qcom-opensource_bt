/*
 * Copyright (c) 2016, The Linux Foundation. All rights reserved.
 * Not a Contribution.
 * Copyright (C) 2012 The Android Open Source Project
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

#ifndef GAP_APP_HPP
#define GAP_APP_HPP

#include <map>
#include <string>
#include <hardware/bluetooth.h>

#include "osi/include/log.h"
#include "osi/include/thread.h"
#include "osi/include/config.h"
#include "ipc.h"
#include "AdapterProperties.hpp"
#include "RemoteDevices.hpp"

#define MAX_BONDED_DEVICES (20)

#define HANDLER_EVENT_COUNT (2)
#define ADAPTER_EVENT_MSG   (0)
#define ADAPTER_EVENT_STOP  (1)


const unsigned char g_audiosink_uuid[16] = {0x00, 0x00, 0x11, 0x0B, 0x00, 0x00,
                0x10, 0x00, 0x80, 0x00, 0x00, 0x80, 0x5F, 0x9B, 0x34, 0xFB};

const unsigned char g_audiosource_uuid[16] = {0x00, 0x00, 0x11, 0x0A, 0x00, 0x00,
                0x10, 0x00, 0x80, 0x00, 0x00, 0x80, 0x5F, 0x9B, 0x34, 0xFB};

const unsigned char g_advaudiodist_uuid[16] = {0x00, 0x00, 0x11, 0x0D,0x00, 0x00,
                0x10, 0x00, 0x80, 0x00, 0x00, 0x80, 0x5F, 0x9B, 0x34, 0xFB};

const unsigned char g_hsp_uuid[16] = {0x00, 0x00, 0x11, 0x08, 0x00, 0x00, 0x10,
                0x00, 0x80, 0x00, 0x00, 0x80, 0x5F, 0x9B, 0x34, 0xFB};

const unsigned char g_hsp_aguuid[16] = {0x00, 0x00, 0x11, 0x12, 0x00, 0x00, 0x10,
                 0x00, 0x80, 0x00, 0x00, 0x80, 0x5F, 0x9B, 0x34, 0xFB};

const unsigned char g_handsfree_uuid[16] = {0x00, 0x00, 0x11, 0x1E, 0x00, 0x00, 0x10,
                 0x00, 0x80, 0x00, 0x00, 0x80, 0x5F, 0x9B, 0x34, 0xFB};

const unsigned char g_handsfree_aguuid[16] = {0x00, 0x00, 0x11, 0x1f, 0x00, 0x00,
                 0x10, 0x00, 0x80, 0x00, 0x00, 0x80, 0x5F, 0x9B, 0x34, 0xFB};

const unsigned char g_avrcpcontroller_uuid[16] = {0x00, 0x00, 0x11, 0x0E, 0x00, 0x00,
                 0x10, 0x00, 0x80, 0x00, 0x00, 0x80, 0x5F, 0x9B, 0x34, 0xFB};

const unsigned char g_avrcptarget_uuid[16] = {0x00, 0x00, 0x11, 0x0C, 0x00, 0x00,
                 0x10, 0x00, 0x80, 0x00, 0x00, 0x80, 0x5F, 0x9B, 0x34, 0xFB};

enum ProfileType
{
  TYPE_AUDIO_SINK,
  TYPE_AUIDO_SOURCE,
  TYPE_ADVANCED_AUDIO,
  TYPE_HSP,
  TYPE_HSP_AG,
  TYPE_HANDSFREE,
  TYPE_HANDSFREE_AG,
  TYPE_AVRCP_CT,
  TYPE_AVRCP_TG
};

class Gap {

  private:
    config_t *config_;
    const bt_interface_t *bluetooth_interface_;
    AdapterProperties *adapter_properties_obj_;
    RemoteDevices     *remote_devices_obj_;
    void HandlePinRequestEvent(PINRequestEvent *event);
    void HandleSspRequestEvent(SSPRequestEvent *event);
    void HandleBondStateEvent(DeviceBondStateEventInt *event);
    void HandleEnable();
    void HandleDisable();
    void HandleStartDiscovery();
    void HandleStopDiscovery();

  public:
    Gap(const bt_interface_t *bt_interface, config_t *config);
    ~Gap();
    void ProcessEvent(BtEvent* event);
    int  GetState();
    bool IsEnabled();
    bool IsDiscovering();
    int GetBondState(bt_bdaddr_t bd_addr);
};

#endif
