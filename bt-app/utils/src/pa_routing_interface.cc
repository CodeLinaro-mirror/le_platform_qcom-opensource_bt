/*
 * Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include "osi/include/log.h"
#include "utils/include/pa_routing_interface.h"

#define LOGTAG "BT_PA_INTF "

std::string PaRountingInterface::getQahwObjectPath(pa_qahw_devices_t bt_device)
{
    std::string dev_path("/org/pulseaudio/ext/qahw/port/");

    switch (bt_device)
    {
    case PA_A2DP_SINK_DEVICE:
        dev_path += "bta2dp_in";
        break;
    case PA_A2DP_SOURCE_DEVICE:
        dev_path += "bta2dp_out";
        break;
    case PA_SCO_SINK_DEVICE:
        dev_path += "btsco_in";
        break;
    case PA_SCO_SOURCE_DEVICE:
        dev_path += "btsco_out";
        break;
    default:
        return std::string(""); // Unsupported device
    }

    return dev_path;
}

int PaRountingInterface::CreatePaQahwDevice(pa_qahw_devices_t bt_device)
{
    switch (bt_device)
    {
    case PA_A2DP_SINK_DEVICE:
    case PA_A2DP_SOURCE_DEVICE:
        return CreatePaQahwDevice(bt_device, "pcm", 48000, "s16le", "front-left,front-right");
    case PA_SCO_SINK_DEVICE:
    case PA_SCO_SOURCE_DEVICE:
        return CreatePaQahwDevice(bt_device, "pcm", 8000, "s16le", "front-left,front-right");
    default:
        break;
    }

    return -1; // Unsupported device
}

int PaRountingInterface::CreatePaQahwDevice(pa_qahw_devices_t bt_device, const char *encoding, uint32_t rate, const char *format, const char *channel_map)
{

    // Call Start stream config on the qahw Dbus interface and path
    ALOGD(LOGTAG "%s", __func__);
    int r;
    sd_bus_error error = SD_BUS_ERROR_NULL;
    sd_bus_message *m = NULL;
    sd_bus *pa_add_bus;
    std::string dev_path;

    dev_path = getQahwObjectPath(bt_device);
    if (dev_path.length() == 0)
        return -1;

    // Create an Address bus to communitate with PA dbus interface directly
    do
    {
        r = sd_bus_new(&pa_add_bus);
        if (r < 0)
        {
            ALOGE(LOGTAG "PA Address bus Creation Failed with %d - %s", r, strerror(-r));
            break;
        }
        r = sd_bus_set_address(pa_add_bus, "unix:path=/var/run/pulse/dbus-socket");
        if (r < 0)
        {
            ALOGE(LOGTAG "Set PA bus address Failed %d - %s", r, strerror(-r));
            break;
        }
        if (sd_bus_start(pa_add_bus) < 0)
        {
            ALOGE(LOGTAG "Couldn't Start the PA Address Bus\n");
            break;
        }

        r = sd_bus_message_new_method_call(pa_add_bus, &m,
                                           "org.pulseaudio.Server",
                                           dev_path.c_str(),
                                           "org.PulseAudio.Ext.Qahw.Module", "StartStream");

        if (r < 0)
        {
            ALOGE(LOGTAG "Failed to create method call message object for StartStream error %d - %s\n", r, strerror(-r));
            break;
        }

        ALOGD(LOGTAG "Path : %s", dev_path.c_str());
        ALOGD(LOGTAG "Encoding : %s , Rate : %d , Format :%s , Mapping : %s", encoding, rate, format, channel_map);

        r = sd_bus_message_append(m, "(suss)", encoding, rate, format, channel_map);
        if (r < 0)
        {
            ALOGE(LOGTAG "%s failed to append args to StartStream call, error %d - %s\n", __func__, r, strerror(-r));
            break;
        }

        r = sd_bus_call(pa_add_bus, m, 0, &error, NULL);

        if (r < 0)
        {
            ALOGE(LOGTAG "PaRoutingInterface::%s sd_bus_call_method failed error %d - %s:%s\n", __func__, r, strerror(-r), error.message);
            break;
        }
        else
        {
            fprintf(stdout, "Created PA device - Path : %s\n", dev_path.c_str());
            fprintf(stdout, "Encoding : %s , Rate : %d , Format :%s , Mapping : %s\n", encoding, rate, format, channel_map);
        }
    } while (0);

    if (NULL != m)
        sd_bus_message_unref(m);
    sd_bus_error_free(&error);
    sd_bus_flush_close_unref(pa_add_bus);
    return r;
}

int PaRountingInterface::RemovePaQahwDevice(pa_qahw_devices_t bt_device)
{
    // Call Stop stream config on the qahw Dbus interface and path
    ALOGD(LOGTAG "%s", __func__);

    int r;
    sd_bus_error error = SD_BUS_ERROR_NULL;
    sd_bus_message *m = NULL;
    sd_bus *pa_add_bus;
    std::string dev_path;

    dev_path = getQahwObjectPath(bt_device);
    if (dev_path.length() == 0)
        return -1;

    // Create an Address bus to communitate with PA dbus interface directly
    do
    {
        r = sd_bus_new(&pa_add_bus);
        if (r < 0)
        {
            ALOGE(LOGTAG "PA Address bus Creation Failed with %d - %s", r, strerror(-r));
            break;
        }
        r = sd_bus_set_address(pa_add_bus, "unix:path=/var/run/pulse/dbus-socket");
        if (r < 0)
        {
            ALOGE(LOGTAG "Set PA bus address Failed %d - %s", r, strerror(-r));
            break;
        }
        if (sd_bus_start(pa_add_bus) < 0)
        {
            ALOGE(LOGTAG "Couldn't Start the PA Address Bus\n");
            break;
        }

        r = sd_bus_call_method(pa_add_bus, NULL,
                               dev_path.c_str(),
                               "org.PulseAudio.Ext.Qahw.Module", "StopStream", &error, &m,
                               NULL);
        if (r < 0)
        {
            ALOGE(LOGTAG " A2dpSplit::%s sd_bus_call_method failed error %d - %s:%s\n", __func__, r, strerror(-r), error.message);
        }
        else
        {
            fprintf(stdout, "Removed PA device - Path : %s\n", dev_path.c_str());
        }

    } while (0);

    sd_bus_error_free(&error);
    if (NULL != m)
        sd_bus_message_unref(m);
    sd_bus_flush_close_unref(pa_add_bus);
    return r;
}

int PaRountingInterface::SetParamPAQahwDevice(pa_qahw_devices_t bt_device, const char *param)
{
    // Call Start stream config on the qahw Dbus interface and path
    ALOGE(LOGTAG "%s", __func__);
    int r;
    sd_bus_error error = SD_BUS_ERROR_NULL;
    sd_bus_message *m = NULL;
    sd_bus *pa_add_bus;
    std::string dev_path;

    dev_path = getQahwObjectPath(bt_device);
    if (dev_path.length() == 0)
        return -1;

    // Create an Address bus to communitate with PA dbus interface directly
    do
    {
        r = sd_bus_new(&pa_add_bus);
        if (r < 0)
        {
            ALOGE(LOGTAG "PA Address bus Creation Failed with %d - %s", r, strerror(-r));
            break;
        }
        r = sd_bus_set_address(pa_add_bus, "unix:path=/var/run/pulse/dbus-socket");
        if (r < 0)
        {
            ALOGE(LOGTAG "Set PA bus address Failed %d - %s", r, strerror(-r));
            break;
        }
        if (sd_bus_start(pa_add_bus) < 0)
        {
            ALOGE(LOGTAG "Couldn't Start the PA Address Bus\n");
            break;
        }

        r = sd_bus_message_new_method_call(pa_add_bus, &m,
                                           "org.pulseaudio.Server",
                                           dev_path.c_str(),
                                           "org.PulseAudio.Ext.Qahw.Module", "SetParam");

        if (r < 0)
        {
            ALOGE(LOGTAG "Failed to create method call message object for StartStream error %d - %s\n", r, strerror(-r));
            break;
        }

        ALOGD(LOGTAG "Path : %s", dev_path.c_str());
        ALOGD(LOGTAG "Param : %s", param);

        r = sd_bus_message_append(m, "s", param);
        if (r < 0)
        {
            ALOGE(LOGTAG "%s failed to append args to StartStream call, error %d - %s\n", __func__, r, strerror(-r));
            break;
        }

        r = sd_bus_call(pa_add_bus, m, 0, &error, NULL);
        if (r < 0)
        {
            ALOGE(LOGTAG "A2dpSplit::%s sd_bus_call_method failed error %d - %s:%s\n", __func__, r, strerror(-r), error.message);
            break;
        }
    } while (0);

    if (NULL != m)
        sd_bus_message_unref(m);
    sd_bus_error_free(&error);
    sd_bus_flush_close_unref(pa_add_bus);
    return r;
}
