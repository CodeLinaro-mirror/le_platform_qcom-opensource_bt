/*
 * Copyright (c) 2018, The Linux Foundation. All rights reserved.
 * Not a Contribution.
 * Copyright (C) 2013 The Android Open Source Project
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

/*******************************************************************************
 *
 *  Filename:      sdbus_ipc.cc
 *
 *  Description:   SD-Bus IPC for Bluetooth HAL Interfaces
 *
 ******************************************************************************/

#define LOG_TAG "bt_ipc"

#include "sdbus_ipc.h"

#include <stdio.h>
#include <pthread.h>

#include <iostream>
#include <fstream>

#include "GattNativeInterfaceV2_b.hpp"

#define LOGTAG "sdbus_ipc"

#define DBUS_SESSION_FILE "/tmp/dbus-session"

sd_bus *g_sdbus = nullptr;
sd_bus *g_sdbus_call = nullptr;
int g_stop_dbus_fd = -1;
bool g_dbus_running = false;

bool open_sdbus_ipc()
{
    // Set dBus-session
    std::ifstream dBusfile(DBUS_SESSION_FILE);
    std::string file_data;
    if (dBusfile.is_open())
    {
        while (getline(dBusfile, file_data))
        {
            std::string var = file_data.substr(0, file_data.find("="));
            std::string value = file_data.substr(file_data.find("=") + 1);
            const char *env = var.c_str();
            const char *set = value.c_str();
            setenv(env, set, 0);
        }
        dBusfile.close();
    }
    else
    {
        ALOGE(LOGTAG "::%s Unable to open file - %s , export DBUS_SESSION_BUS_ADDRESS manually", __func__, DBUS_SESSION_FILE);
        return false;
    }

    if (sd_bus_open_system(&g_sdbus) < 0)
    {
        ALOGE(LOGTAG "::%s D-Bus is not Initialised.", __func__);
        return false;
    }

    if (sd_bus_request_name(g_sdbus, DBUS_SVC_NAME, 0) < 0)
    {
        ALOGE(LOGTAG "::%s Failed to acquire name on user bus.", __func__);
        return false;
    }

    if (sd_bus_open_system(&g_sdbus_call) < 0)
    {
        ALOGE(LOGTAG "::%s D-Bus is not Initialised.", __func__);
        return false;
    }

    if (sd_bus_request_name(g_sdbus_call, DBUS_SVC_NAME_SENDER, 0) < 0)
    {
        ALOGE(LOGTAG "::%s Failed to acquire name on user bus.", __func__);
        return false;
    }

    g_stop_dbus_fd = eventfd(0, 0);
    if (g_stop_dbus_fd < 0) {
        ALOGE(LOGTAG "sdbusInit failed to to create eventfd");
        return false;
    }

    ALOGD(LOGTAG "::%s Successed to open bus!! : service - %s", __func__, DBUS_SVC_NAME);

    g_dbus_running = true;

    return true;
}

void close_sdbus_ipc()
{
    // Make a thread stop
    eventfd_write(g_stop_dbus_fd, 1);
    close(g_stop_dbus_fd);

    g_dbus_running = false;

    if (g_sdbus != nullptr)
    {
        sd_bus_flush_close_unref(g_sdbus);
        g_sdbus = nullptr;
    }

    if (g_sdbus_call != nullptr)
    {
        sd_bus_flush_close_unref(g_sdbus_call);
        g_sdbus_call = nullptr;
    }
}

