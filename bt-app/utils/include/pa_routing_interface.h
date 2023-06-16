/*
 * Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef PA_ROUTING_INTERFACE_
#define PA_ROUTING_INTERFACE_

#include <string>
#include <systemd/sd-bus.h>

/**
 * Provides interfaces to control PulseAudio for BT-app
 * @hide
 */

/** Preferred QAHW devices for BT */
typedef enum {
  PA_A2DP_SINK_DEVICE,
  PA_A2DP_SOURCE_DEVICE,
  PA_SCO_SINK_DEVICE,
  PA_SCO_SOURCE_DEVICE
} pa_qahw_devices_t;

class PaRountingInterface {
private:
  static std::string getQahwObjectPath(pa_qahw_devices_t bt_device);

public:

   static int CreatePaQahwDevice(pa_qahw_devices_t bt_device);
   static int CreatePaQahwDevice(pa_qahw_devices_t bt_device, const char *encoding, uint32_t rate, const char *format, const char *channel_map);
   static int RemovePaQahwDevice(pa_qahw_devices_t bt_device);
   static int SetParamPAQahwDevice(pa_qahw_devices_t bt_device, const char *param);
};

#endif //PA_ROUTING_INTERFACE_
