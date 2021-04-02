/*
 * Copyright (c) 2021 The Linux Foundation. All rights reserved.
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
#ifndef GAIATEST_APP_H
#define GAIATEST_APP_H

#include <hardware/bluetooth.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "osi/include/log.h"
#include "osi/include/thread.h"
#include "osi/include/config.h"
#include "ipc.h"
#include "Gatt.hpp"

#define LOGTAG "GAIATEST "
#ifdef USE_GLIB
#include <glib.h>
#define strlcpy g_strlcpy
#endif

class GaiaTest {
    private:
        GattcOpenEvent client_conn_data;
        GaiaRegisterAppEvent app_client_if;
        btgatt_interface_t *gatt_interface;
    public:
        Gatt *app_gatt;

    public:
        GaiaTest(Gatt *);
        ~GaiaTest();

        bool EnableGaiaTest(char *path);
        bool DisableGaiaTest();
        void Start(bt_bdaddr_t bda);
        char fw_path[200];
        bool started = false;
        bt_bdaddr_t bda;

        inline btgatt_interface_t* GetGattInterface()
        {
            return gatt_interface;
        }

        inline GaiaRegisterAppEvent* GetGaiaTestAppData()
        {
            return &app_client_if;
        }
        inline void SetGaiaTestAppData(GaiaRegisterAppEvent *event)
        {
            memset(&app_client_if, 0, sizeof(app_client_if));
            memcpy(&app_client_if, event, sizeof(GaiaRegisterAppEvent));
        }
        inline void SetGaiaTestConnectionData(GattcOpenEvent*event)
        {
            memset(&client_conn_data, 0, sizeof(GattcOpenEvent));
            memcpy(&client_conn_data, event, sizeof(GattcOpenEvent));
        }
        inline GattcOpenEvent* GetGaiaTestConnectionData()
        {
            return &client_conn_data;
        }
        bool CopyClientUUID(bt_uuid_t *);
        char *GaiaReadFile(char *path, int *length);
        bool StartScan(void);
        bool StopScan(void);
        bool RegisterClient();
        bool Connect(const bt_bdaddr_t*);
        bool Disconnect(const bt_bdaddr_t*);
        bool UnregisterClient(int);
        bool WriteCharacteristic(int conn_id, uint16_t handle, int write_type, int len, int auth_req, char* p_value);
        bool SearchService(int conn_id, bt_uuid_t *filter_uuid);
        void Refresh(void);
        bt_status_t ConfigMtu(int conn_id, int mtu);
        void Cleanup();
        void RegisterNotify(int client_if, const bt_bdaddr_t *bda, uint16_t handle);
};
#endif
