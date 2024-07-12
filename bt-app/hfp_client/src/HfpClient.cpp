/* Copyright (c) 2016, The Linux Foundation. All rights reserved.
 * Not a Contribution.
 * Copyright (C) 2012-2014 The Android Open Source Project
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
 *
 * Changes from Qualcomm Innovation Center are provided under the following license:
 *
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include <list>
#include <map>
#include <iostream>
#include <string.h>
#include <hardware/bluetooth.h>
#include <hardware/hardware.h>
#include <hardware/bt_hf_client.h>
#include <dlfcn.h>

//#include "Audio_Manager.hpp"
#include "HfpClient.hpp"
#include "hardware/bt_hf_client_vendor.h"

#if (defined USE_GST)
#ifdef __cplusplus
extern "C" {
#endif
#include <gst/gstbthelper.h>
#ifdef __cplusplus
}
#endif
#endif

#define LOGTAG "HFP_CLIENT"

using namespace std;
using std::list;
using std::string;

Hfp_Client *pHfpClient = NULL;
extern BT_Audio_Manager *pBTAM;
#ifdef BT_AUDIO_PAL_INTEGRATION
static bool pa_hfp_is_connected = false;
#endif

#if (defined USE_GST)

gstbt gstbtringtoneobj;

#endif

char ring_tone[] =
{
    0x52, 0x49, 0x46, 0x46, 0xa0, 0x0d, 0x00, 0x00, 0x57, 0x41, 0x56, 0x45, 0x66, 0x6d, 0x74, 0x20,
    0x10, 0x00, 0x00, 0x00, 0x01, 0x00, 0x01, 0x00, 0x40, 0x1f, 0x00, 0x00, 0x80, 0x3e, 0x00, 0x00,
    0x02, 0x00, 0x10, 0x00, 0x64, 0x61, 0x74, 0x61, 0x7c, 0x0d, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0xf8, 0xff, 0x00, 0x00, 0xf8, 0xff, 0x08, 0x00, 0xe8, 0xff, 0x20, 0x00,
    0xe0, 0xff, 0x30, 0x00, 0xe0, 0xff, 0x28, 0x00, 0xe0, 0xff, 0x80, 0xf4, 0x80, 0xe7, 0x80, 0xe9,
    0x80, 0xec, 0x80, 0xf8, 0x80, 0x1e, 0x80, 0x37, 0x80, 0x37, 0x80, 0x2f, 0x80, 0x21, 0x80, 0x13,
    0x80, 0x05, 0x80, 0xf7, 0x80, 0xef, 0x80, 0xd6, 0x81, 0xb4, 0x81, 0xb0, 0x81, 0xb8, 0x80, 0xc4,
    0x80, 0xda, 0x80, 0xec, 0xb0, 0xff, 0x00, 0x0e, 0x80, 0x1e, 0x80, 0x3f, 0x7f, 0x5b, 0x7f, 0x57,
    0x7f, 0x4f, 0x80, 0x3d, 0x80, 0x27, 0x80, 0x15, 0xc0, 0x02, 0x00, 0xf7, 0x80, 0xdc, 0x81, 0xb8,
    0x81, 0xac, 0x81, 0xb0, 0x80, 0xc0, 0x80, 0xd2, 0x80, 0xe2, 0x00, 0xf6, 0x80, 0x04, 0x80, 0x13,
    0x80, 0x39, 0x7f, 0x53, 0x7f, 0x4f, 0x7f, 0x47, 0x80, 0x39, 0x80, 0x25, 0x80, 0x13, 0x00, 0x01,
    0x00, 0xf6, 0x80, 0xdc, 0x81, 0xb8, 0x81, 0xac, 0x81, 0xb0, 0x80, 0xc0, 0x80, 0xd2, 0x80, 0xe2,
    0x80, 0xf6, 0x00, 0x05, 0x80, 0x12, 0x80, 0x39, 0x7f, 0x53, 0x7f, 0x53, 0x7f, 0x47, 0x80, 0x3b,
    0x80, 0x25, 0x80, 0x13, 0xa0, 0x00, 0x00, 0xf5, 0x80, 0xdc, 0x81, 0xb8, 0x81, 0xa8, 0x81, 0xb0,
    0x81, 0xbc, 0x80, 0xce, 0x80, 0xe0, 0x00, 0xf3, 0xa0, 0x02, 0x80, 0x0f, 0x80, 0x35, 0x7f, 0x4f,
    0x7f, 0x4f, 0x7f, 0x47, 0x80, 0x39, 0x80, 0x25, 0x80, 0x13, 0xf0, 0x00, 0x80, 0xf5, 0x80, 0xde,
    0x81, 0xb8, 0x81, 0xac, 0x81, 0xb0, 0x81, 0xbc, 0x80, 0xce, 0x80, 0xe0, 0x00, 0xf3, 0x80, 0x03,
    0x80, 0x10, 0x80, 0x35, 0x7f, 0x4f, 0x7f, 0x53, 0x7f, 0x4b, 0x80, 0x3d, 0x80, 0x27, 0x80, 0x16,
    0x80, 0x03, 0x00, 0xf8, 0x80, 0xe1, 0x81, 0xbc, 0x81, 0xac, 0x81, 0xb0, 0x81, 0xbc, 0x80, 0xce,
    0x80, 0xe0, 0x80, 0xf4, 0x80, 0x04, 0x80, 0x10, 0x80, 0x31, 0x7f, 0x4f, 0x7f, 0x53, 0x7f, 0x4b,
    0x80, 0x3f, 0x80, 0x29, 0x80, 0x18, 0x80, 0x05, 0x00, 0xf9, 0x80, 0xe3, 0x80, 0xc0, 0x81, 0xac,
    0x81, 0xb0, 0x81, 0xbc, 0x80, 0xce, 0x80, 0xe0, 0x00, 0xf3, 0x00, 0x04, 0x80, 0x0f, 0x80, 0x2f,
    0x7f, 0x4f, 0x7f, 0x53, 0x7f, 0x4b, 0x80, 0x3f, 0x80, 0x29, 0x80, 0x18, 0x80, 0x05, 0xc0, 0xf8,
    0x80, 0xe4, 0x80, 0xc0, 0x81, 0xac, 0x81, 0xb0, 0x81, 0xbc, 0x80, 0xca, 0x80, 0xde, 0x00, 0xf1,
    0x20, 0x02, 0x80, 0x0e, 0x80, 0x2d, 0x7f, 0x4f, 0x7f, 0x53, 0x7f, 0x4b, 0x80, 0x3f, 0x80, 0x29,
    0x80, 0x17, 0x00, 0x05, 0x00, 0xf8, 0x80, 0xe4, 0x80, 0xc0, 0x81, 0xac, 0x81, 0xac, 0x81, 0xb8,
    0x80, 0xc8, 0x80, 0xdc, 0x80, 0xef, 0x80, 0x00, 0x00, 0x0c, 0x80, 0x29, 0x7f, 0x4b, 0x7f, 0x53,
    0x7f, 0x4b, 0x80, 0x3f, 0x80, 0x29, 0x80, 0x18, 0x40, 0x05, 0x80, 0xf8, 0x80, 0xe5, 0x80, 0xc0,
    0x81, 0xac, 0x81, 0xac, 0x81, 0xb8, 0x80, 0xc8, 0x80, 0xdc, 0x80, 0xee, 0x40, 0x00, 0x80, 0x0b,
    0x80, 0x27, 0x7f, 0x4b, 0x7f, 0x53, 0x7f, 0x4b, 0x80, 0x3f, 0x80, 0x2b, 0x80, 0x1b, 0x40, 0x07,
    0x40, 0xf9, 0x80, 0xe9, 0x80, 0xc0, 0x81, 0xac, 0x81, 0xb0, 0x81, 0xb8, 0x80, 0xc8, 0x80, 0xdc,
    0x80, 0xef, 0xc0, 0x00, 0x00, 0x0c, 0x80, 0x27, 0x7f, 0x4b, 0x7f, 0x53, 0x7f, 0x4f, 0x80, 0x3f,
    0x80, 0x2d, 0x80, 0x1c, 0x00, 0x09, 0x00, 0xfb, 0x80, 0xeb, 0x80, 0xc4, 0x81, 0xac, 0x81, 0xb0,
    0x81, 0xb8, 0x80, 0xc8, 0x80, 0xdc, 0x80, 0xee, 0x90, 0x00, 0x00, 0x0c, 0x80, 0x25, 0x7f, 0x4b,
    0x7f, 0x53, 0x7f, 0x4f, 0x80, 0x3f, 0x80, 0x2f, 0x80, 0x1d, 0x80, 0x09, 0x40, 0xfb, 0x80, 0xec,
    0x80, 0xc6, 0x81, 0xac, 0x81, 0xac, 0x81, 0xb8, 0x80, 0xc6, 0x80, 0xda, 0x80, 0xed, 0x70, 0xff,
    0x80, 0x0a, 0x80, 0x23, 0x7f, 0x47, 0x7f, 0x53, 0x7f, 0x4f, 0x80, 0x3f, 0x80, 0x2f, 0x80, 0x1d,
    0x80, 0x0a, 0x80, 0xfa, 0x80, 0xed, 0x80, 0xc6, 0x81, 0xac, 0x81, 0xac, 0x81, 0xb4, 0x80, 0xc4,
    0x81, 0xb0, 0x80, 0xe8, 0x7f, 0x4b, 0x7f, 0x7b, 0x7f, 0x6f, 0x00, 0x07, 0x80, 0xc2, 0x81, 0xb0,
    0xc0, 0xfd, 0x7f, 0x4f, 0x7f, 0x6f, 0x80, 0x3b, 0x80, 0xd4, 0x81, 0xa4, 0x81, 0xac, 0x80, 0x0f,
    0x7f, 0x4f, 0x7f, 0x5b, 0x80, 0x0b, 0x81, 0xb8, 0x81, 0x94, 0x80, 0xc2, 0x80, 0x27, 0x7f, 0x57,
    0x7f, 0x4b, 0x80, 0xe9, 0x81, 0xa8, 0x81, 0x9c, 0x80, 0xeb, 0x80, 0x3f, 0x7f, 0x5f, 0x80, 0x35,
    0x80, 0xd0, 0x81, 0xa4, 0x81, 0xb0, 0x80, 0x11, 0x7f, 0x53, 0x7f, 0x5f, 0x80, 0x11, 0x81, 0xbc,
    0x81, 0x9c, 0x80, 0xca, 0x80, 0x2f, 0x7f, 0x5b, 0x7f, 0x4f, 0x80, 0xee, 0x81, 0xac, 0x81, 0xa0,
    0x80, 0xee, 0x7f, 0x43, 0x7f, 0x63, 0x80, 0x37, 0x80, 0xd0, 0x81, 0xa4, 0x81, 0xb0, 0x80, 0x11,
    0x7f, 0x53, 0x7f, 0x5f, 0x80, 0x11, 0x81, 0xbc, 0x81, 0x9c, 0x80, 0xc8, 0x80, 0x2f, 0x7f, 0x5b,
    0x7f, 0x4f, 0x80, 0xee, 0x81, 0xb0, 0x81, 0xa0, 0x80, 0xef, 0x7f, 0x43, 0x7f, 0x63, 0x80, 0x37,
    0x80, 0xd2, 0x81, 0xa4, 0x81, 0xb0, 0x80, 0x13, 0x7f, 0x53, 0x7f, 0x5f, 0x80, 0x12, 0x80, 0xc0,
    0x81, 0xa0, 0x80, 0xca, 0x80, 0x2f, 0x7f, 0x5f, 0x7f, 0x53, 0x80, 0xef, 0x81, 0xb0, 0x81, 0xa0,
    0x80, 0xef, 0x7f, 0x43, 0x7f, 0x63, 0x80, 0x37, 0x80, 0xd2, 0x81, 0xa4, 0x81, 0xb0, 0x80, 0x12,
    0x7f, 0x53, 0x7f, 0x5f, 0x80, 0x11, 0x81, 0xbc, 0x81, 0x9c, 0x80, 0xc8, 0x80, 0x2f, 0x7f, 0x5b,
    0x7f, 0x4f, 0x80, 0xee, 0x81, 0xac, 0x81, 0xa0, 0x80, 0xee, 0x7f, 0x43, 0x7f, 0x63, 0x80, 0x35,
    0x80, 0xd0, 0x81, 0xa4, 0x81, 0xb0, 0x80, 0x10, 0x7f, 0x4f, 0x7f, 0x5f, 0x80, 0x10, 0x81, 0xbc,
    0x81, 0x9c, 0x80, 0xc8, 0x80, 0x2d, 0x7f, 0x5b, 0x7f, 0x4f, 0x80, 0xec, 0x81, 0xac, 0x81, 0xa0,
    0x80, 0xed, 0x7f, 0x43, 0x7f, 0x5f, 0x80, 0x35, 0x80, 0xd0, 0x81, 0xa0, 0x81, 0xb0, 0x80, 0x10,
    0x7f, 0x4f, 0x7f, 0x5f, 0x80, 0x10, 0x81, 0xbc, 0x81, 0x9c, 0x80, 0xc8, 0x80, 0x2d, 0x7f, 0x5b,
    0x7f, 0x4f, 0x80, 0xed, 0x81, 0xac, 0x81, 0xa0, 0x80, 0xee, 0x7f, 0x43, 0x7f, 0x63, 0x80, 0x37,
    0x80, 0xd0, 0x81, 0xa4, 0x81, 0xb0, 0x80, 0x11, 0x7f, 0x53, 0x7f, 0x5f, 0x80, 0x11, 0x81, 0xbc,
    0x81, 0x9c, 0x80, 0xca, 0x80, 0x2f, 0x7f, 0x5b, 0x7f, 0x4f, 0x80, 0xef, 0x81, 0xb0, 0x81, 0xa0,
    0x80, 0xef, 0x7f, 0x43, 0x7f, 0x63, 0x80, 0x37, 0x80, 0xd2, 0x81, 0xa4, 0x81, 0xb0, 0x80, 0x12,
    0x7f, 0x53, 0x7f, 0x5f, 0x80, 0x12, 0x81, 0xbc, 0x81, 0x9c, 0x80, 0xca, 0x80, 0x2f, 0x7f, 0x5b,
    0x7f, 0x4f, 0x80, 0xef, 0x81, 0xb0, 0x81, 0xa0, 0x80, 0xef, 0x7f, 0x43, 0x7f, 0x63, 0x80, 0x37,
    0x80, 0xd2, 0x81, 0xa4, 0x81, 0xb0, 0x80, 0x12, 0x7f, 0x53, 0x7f, 0x5f, 0x80, 0x11, 0x81, 0xbc,
    0x81, 0x9c, 0x80, 0xc8, 0x80, 0x2f, 0x7f, 0x5b, 0x7f, 0x4f, 0x80, 0xee, 0x81, 0xac, 0x81, 0xa0,
    0x80, 0xee, 0x7f, 0x43, 0x7f, 0x63, 0x80, 0x37, 0x80, 0xd0, 0x81, 0xa4, 0x81, 0xb0, 0x80, 0x11,
    0x7f, 0x4f, 0x7f, 0x5f, 0x80, 0x11, 0x81, 0xbc, 0x81, 0x9c, 0x80, 0xc8, 0x80, 0x2d, 0x7f, 0x5b,
    0x7f, 0x4f, 0x80, 0xed, 0x81, 0xac, 0x81, 0xa0, 0x80, 0xed, 0x7f, 0x43, 0x7f, 0x63, 0x80, 0x35,
    0x80, 0xd0, 0x81, 0xa4, 0x81, 0xb0, 0x80, 0x10, 0x7f, 0x4f, 0x7f, 0x5f, 0x80, 0x10, 0x81, 0xbc,
    0x81, 0x9c, 0x80, 0xc8, 0x80, 0x2d, 0x7f, 0x5b, 0x7f, 0x4f, 0x80, 0xee, 0x81, 0xac, 0x81, 0xa0,
    0x80, 0xee, 0x7f, 0x43, 0x7f, 0x63, 0x80, 0x37, 0x80, 0xd0, 0x81, 0xa4, 0x81, 0xb0, 0x80, 0x11,
    0x7f, 0x53, 0x7f, 0x5f, 0x80, 0x11, 0x81, 0xbc, 0x81, 0x9c, 0x80, 0xc8, 0x80, 0x2f, 0x7f, 0x5b,
    0x7f, 0x53, 0x80, 0xec, 0x81, 0xb8, 0x80, 0xc8, 0x80, 0xd2, 0x80, 0xe5, 0x80, 0xf0, 0x80, 0x14,
    0x80, 0x39, 0x80, 0x3f, 0x80, 0x3d, 0x80, 0x2f, 0x80, 0x1f, 0x80, 0x0f, 0x18, 0x00, 0x00, 0xf6,
    0x80, 0xe3, 0x80, 0xc0, 0x81, 0xb0, 0x81, 0xb4, 0x80, 0xc0, 0x80, 0xd2, 0x80, 0xe3, 0x80, 0xf7,
    0x80, 0x08, 0x80, 0x13, 0x80, 0x35, 0x7f, 0x53, 0x7f, 0x57, 0x7f, 0x4f, 0x80, 0x3f, 0x80, 0x2d,
    0x80, 0x1c, 0x80, 0x07, 0x40, 0xfa, 0x80, 0xe7, 0x80, 0xc0, 0x81, 0xac, 0x81, 0xb0, 0x81, 0xb8,
    0x80, 0xc8, 0x80, 0xde, 0x80, 0xf0, 0x00, 0x01, 0x00, 0x0d, 0x80, 0x29, 0x7f, 0x4b, 0x7f, 0x53,
    0x7f, 0x4b, 0x80, 0x3f, 0x80, 0x2b, 0x80, 0x1a, 0xc0, 0x06, 0x00, 0xf9, 0x80, 0xe7, 0x80, 0xc0,
    0x81, 0xac, 0x81, 0xb0, 0x81, 0xb8, 0x80, 0xc8, 0x80, 0xde, 0x80, 0xef, 0x00, 0x01, 0x00, 0x0c,
    0x80, 0x29, 0x7f, 0x4b, 0x7f, 0x53, 0x7f, 0x4b, 0x80, 0x3f, 0x80, 0x2b, 0x80, 0x1a, 0xc0, 0x06,
    0x80, 0xf8, 0x80, 0xe8, 0x80, 0xc0, 0x81, 0xac, 0x81, 0xac, 0x81, 0xb8, 0x80, 0xc6, 0x80, 0xda,
    0x80, 0xed, 0x00, 0xff, 0x00, 0x0a, 0x80, 0x25, 0x7f, 0x47, 0x7f, 0x53, 0x7f, 0x4b, 0x80, 0x3f,
    0x80, 0x2b, 0x80, 0x1a, 0x40, 0x07, 0x00, 0xf9, 0x80, 0xe9, 0x80, 0xc2, 0x81, 0xac, 0x81, 0xac,
    0x81, 0xb8, 0x80, 0xc6, 0x80, 0xda, 0x80, 0xed, 0x60, 0xff, 0x80, 0x0a, 0x80, 0x25, 0x7f, 0x47,
    0x7f, 0x53, 0x7f, 0x4f, 0x80, 0x3f, 0x80, 0x2d, 0x80, 0x1d, 0x80, 0x09, 0x00, 0xfb, 0x80, 0xec,
    0x80, 0xc6, 0x81, 0xac, 0x81, 0xb0, 0x81, 0xb8, 0x80, 0xc6, 0x80, 0xdc, 0x80, 0xed, 0xf0, 0xff,
    0x00, 0x0b, 0x80, 0x23, 0x7f, 0x47, 0x7f, 0x53, 0x7f, 0x4f, 0x7f, 0x43, 0x80, 0x2f, 0x80, 0x1f,
    0x00, 0x0b, 0x40, 0xfc, 0x80, 0xee, 0x80, 0xc8, 0x81, 0xb0, 0x81, 0xb0, 0x81, 0xb8, 0x80, 0xc6,
    0x80, 0xda, 0x80, 0xec, 0x50, 0xff, 0x80, 0x0a, 0x80, 0x21, 0x7f, 0x47, 0x7f, 0x53, 0x7f, 0x4f,
    0x7f, 0x43, 0x80, 0x2f, 0x80, 0x1f, 0x80, 0x0b, 0x40, 0xfc, 0x80, 0xee, 0x80, 0xca, 0x81, 0xb0,
    0x81, 0xac, 0x81, 0xb4, 0x80, 0xc4, 0x80, 0xd8, 0x80, 0xeb, 0x80, 0xfd, 0x00, 0x09, 0x80, 0x1f,
    0x7f, 0x43, 0x7f, 0x53, 0x7f, 0x4f, 0x7f, 0x43, 0x80, 0x2f, 0x80, 0x1f, 0x80, 0x0b, 0x80, 0xfb,
    0x80, 0xee, 0x80, 0xca, 0x81, 0xb0, 0x81, 0xac, 0x81, 0xb4, 0x80, 0xc2, 0x80, 0xd8, 0x80, 0xe9,
    0x00, 0xfc, 0x80, 0x07, 0x80, 0x1e, 0x7f, 0x43, 0x7f, 0x53, 0x7f, 0x4f, 0x7f, 0x43, 0x80, 0x31,
    0x80, 0x1f, 0x00, 0x0c, 0x00, 0xfc, 0x80, 0xef, 0x80, 0xce, 0x81, 0xb0, 0x81, 0xac, 0x81, 0xb4,
    0x80, 0xc0, 0x80, 0xd6, 0x80, 0xe8, 0x80, 0xfb, 0x80, 0x07, 0x80, 0x1d, 0x7f, 0x43, 0x7f, 0x53,
    0x7f, 0x4f, 0x7f, 0x43, 0x80, 0x31, 0x80, 0x1f, 0x00, 0x0e, 0x20, 0xfd, 0x80, 0xf0, 0x80, 0xd0,
    0x81, 0xb0, 0x81, 0xac, 0x81, 0xb4, 0x80, 0xc0, 0x80, 0xd6, 0x80, 0xe8, 0x80, 0xfb, 0x00, 0x08,
    0x80, 0x1c, 0x80, 0x3f, 0x7f, 0x53, 0x7f, 0x4f, 0x7f, 0x43, 0x80, 0x35, 0x80, 0x21, 0x80, 0x0f,
    0x60, 0xfe, 0x00, 0xf2, 0x80, 0xd4, 0x81, 0xb4, 0x81, 0xac, 0x81, 0xb4, 0x80, 0xc0, 0x80, 0xd6,
    0x80, 0xe8, 0x00, 0xfb, 0x80, 0x07, 0x80, 0x1b, 0x80, 0x3f, 0x7f, 0x53, 0x7f, 0x4f, 0x7f, 0x47,
    0x80, 0x37, 0x80, 0x21, 0x80, 0x0f, 0xd0, 0xfe, 0x00, 0xf3, 0x80, 0xd6, 0x81, 0xb4, 0x81, 0xac,
    0x81, 0xb4, 0x80, 0xc0, 0x80, 0xd4, 0x80, 0xe5, 0x80, 0xf9, 0x40, 0x07, 0x80, 0x18, 0x80, 0x3f,
    0x7f, 0x53, 0x7f, 0x4f, 0x7f, 0x47, 0x80, 0x37, 0x80, 0x21, 0x80, 0x10, 0x10, 0xff, 0x80, 0xf2,
    0x80, 0xd8, 0x81, 0xb4, 0x81, 0xac, 0x81, 0xb0, 0x80, 0xc2, 0x81, 0xb8, 0x80, 0xc6, 0x80, 0x2f,
    0x7f, 0x73, 0x7f, 0x7b, 0x80, 0x2b, 0x80, 0xd4, 0x81, 0xb0, 0x80, 0xdc, 0x80, 0x3d, 0x7f, 0x6b,
    0x7f, 0x57, 0x00, 0xf2, 0x81, 0xb0, 0x81, 0xa0, 0x80, 0xec, 0x80, 0x3f, 0x7f, 0x5f, 0x80, 0x2f,
    0x80, 0xc8, 0x81, 0x9c, 0x81, 0xa8, 0x80, 0x0b, 0x7f, 0x4b, 0x7f, 0x5b, 0x00, 0x0d, 0x81, 0xb8,
    0x81, 0x94, 0x80, 0xc6, 0x80, 0x2b, 0x7f, 0x5b, 0x7f, 0x4f, 0x80, 0xed, 0x81, 0xac, 0x81, 0xa0,
    0x80, 0xee, 0x7f, 0x43, 0x7f, 0x63, 0x80, 0x37, 0x80, 0xd2, 0x81, 0xa4, 0x81, 0xb0, 0x80, 0x11,
    0x7f, 0x53, 0x7f, 0x5f, 0x80, 0x11, 0x81, 0xbc, 0x81, 0x9c, 0x80, 0xc8, 0x80, 0x2d, 0x7f, 0x5b,
    0x7f, 0x4f, 0x80, 0xee, 0x81, 0xb0, 0x81, 0xa0, 0x80, 0xee, 0x7f, 0x43, 0x7f, 0x63, 0x80, 0x37,
    0x80, 0xd2, 0x81, 0xa4, 0x81, 0xb0, 0x80, 0x11, 0x7f, 0x53, 0x7f, 0x5f, 0x80, 0x11, 0x81, 0xbc,
    0x81, 0xa0, 0x80, 0xca, 0x80, 0x2f, 0x7f, 0x5b, 0x7f, 0x4f, 0x80, 0xef, 0x81, 0xb0, 0x81, 0xa0,
    0x80, 0xef, 0x7f, 0x43, 0x7f, 0x63, 0x80, 0x37, 0x80, 0xd2, 0x81, 0xa4, 0x81, 0xb0, 0x80, 0x12,
    0x7f, 0x53, 0x7f, 0x5f, 0x80, 0x12, 0x80, 0xc0, 0x81, 0xa0, 0x80, 0xca, 0x80, 0x2f, 0x7f, 0x5b,
    0x7f, 0x4f, 0x80, 0xef, 0x81, 0xb0, 0x81, 0xa0, 0x80, 0xef, 0x7f, 0x43, 0x7f, 0x63, 0x80, 0x37,
    0x80, 0xd2, 0x81, 0xa4, 0x81, 0xb0, 0x80, 0x11, 0x7f, 0x4f, 0x7f, 0x5f, 0x80, 0x11, 0x81, 0xbc,
    0x81, 0x9c, 0x80, 0xc8, 0x80, 0x2d, 0x7f, 0x5b, 0x7f, 0x4f, 0x80, 0xee, 0x81, 0xac, 0x81, 0xa0,
    0x80, 0xed, 0x7f, 0x43, 0x7f, 0x5f, 0x80, 0x35, 0x80, 0xd0, 0x81, 0xa4, 0x81, 0xb0, 0x80, 0x10,
    0x7f, 0x4f, 0x7f, 0x5f, 0x80, 0x10, 0x81, 0xbc, 0x81, 0x9c, 0x80, 0xc8, 0x80, 0x2d, 0x7f, 0x5b,
    0x7f, 0x4f, 0x80, 0xed, 0x81, 0xac, 0x81, 0xa0, 0x80, 0xed, 0x80, 0x3f, 0x7f, 0x5f, 0x80, 0x35,
    0x80, 0xd0, 0x81, 0xa4, 0x81, 0xb0, 0x80, 0x10, 0x7f, 0x4f, 0x7f, 0x5f, 0x80, 0x10, 0x81, 0xbc,
    0x81, 0x9c, 0x80, 0xc8, 0x80, 0x2d, 0x7f, 0x5b, 0x7f, 0x4f, 0x80, 0xee, 0x81, 0xb0, 0x81, 0xa0,
    0x80, 0xee, 0x7f, 0x43, 0x7f, 0x63, 0x80, 0x37, 0x80, 0xd2, 0x81, 0xa4, 0x81, 0xb0, 0x80, 0x11,
    0x7f, 0x4f, 0x7f, 0x5f, 0x80, 0x11, 0x80, 0xc0, 0x81, 0xa0, 0x80, 0xca, 0x80, 0x2f, 0x7f, 0x5b,
    0x7f, 0x4f, 0x80, 0xef, 0x81, 0xb0, 0x81, 0xa0, 0x80, 0xef, 0x7f, 0x43, 0x7f, 0x63, 0x80, 0x37,
    0x80, 0xd2, 0x81, 0xa4, 0x81, 0xb0, 0x80, 0x11, 0x7f, 0x4f, 0x7f, 0x5f, 0x80, 0x12, 0x80, 0xc0,
    0x81, 0xa0, 0x80, 0xca, 0x80, 0x2f, 0x7f, 0x5b, 0x7f, 0x4f, 0x80, 0xef, 0x81, 0xb0, 0x81, 0xa0,
    0x80, 0xef, 0x7f, 0x43, 0x7f, 0x5f, 0x80, 0x37, 0x80, 0xd2, 0x81, 0xa4, 0x81, 0xb0, 0x80, 0x11,
    0x7f, 0x4f, 0x7f, 0x5f, 0x80, 0x11, 0x81, 0xbc, 0x81, 0xa0, 0x80, 0xca, 0x80, 0x2d, 0x7f, 0x5b,
    0x7f, 0x4f, 0x80, 0xee, 0x81, 0xb0, 0x81, 0xa0, 0x80, 0xee, 0x80, 0x3f, 0x7f, 0x5f, 0x80, 0x35,
    0x80, 0xd2, 0x81, 0xa4, 0x81, 0xb0, 0x80, 0x10, 0x7f, 0x4f, 0x7f, 0x5f, 0x80, 0x10, 0x81, 0xbc,
    0x81, 0xa0, 0x80, 0xc8, 0x80, 0x2d, 0x7f, 0x5b, 0x7f, 0x4f, 0x80, 0xee, 0x81, 0xb0, 0x81, 0xa0,
    0x80, 0xee, 0x80, 0x3f, 0x7f, 0x5f, 0x80, 0x35, 0x80, 0xd2, 0x81, 0xa4, 0x81, 0xb0, 0x80, 0x10,
    0x7f, 0x4f, 0x7f, 0x5f, 0x80, 0x10, 0x81, 0xbc, 0x81, 0xa0, 0x80, 0xca, 0x80, 0x2d, 0x7f, 0x5b,
    0x7f, 0x4f, 0x80, 0xee, 0x81, 0xb0, 0x81, 0xa0, 0x80, 0xee, 0x80, 0x3f, 0x7f, 0x5f, 0x80, 0x35,
    0x80, 0xd2, 0x81, 0xa4, 0x81, 0xb0, 0x80, 0x11, 0x7f, 0x4f, 0x7f, 0x5b, 0x80, 0x13, 0x80, 0xc0,
    0x80, 0xc4, 0x80, 0xd0, 0x80, 0xe0, 0x80, 0xec, 0x80, 0x05, 0x80, 0x2d, 0x80, 0x3f, 0x80, 0x3d,
    0x80, 0x31, 0x80, 0x23, 0x80, 0x14, 0x40, 0x05, 0x80, 0xf8, 0x80, 0xed, 0x80, 0xce, 0x81, 0xb4,
    0x81, 0xb4, 0x81, 0xbc, 0x80, 0xca, 0x80, 0xe0, 0x00, 0xf1, 0xc0, 0x03, 0x80, 0x0f, 0x80, 0x25,
    0x7f, 0x47, 0x7f, 0x57, 0x7f, 0x53, 0x7f, 0x43, 0x80, 0x35, 0x80, 0x1f, 0x00, 0x0e, 0xc0, 0xfd,
    0x80, 0xf0, 0x80, 0xce, 0x81, 0xb0, 0x81, 0xb0, 0x81, 0xb8, 0x80, 0xc4, 0x80, 0xd8, 0x80, 0xea,
    0x40, 0xfc, 0x80, 0x07, 0x80, 0x1e, 0x7f, 0x43, 0x7f, 0x53, 0x7f, 0x4b, 0x7f, 0x43, 0x80, 0x2f,
    0x80, 0x1f, 0x00, 0x0d, 0xa0, 0xfc, 0x80, 0xf0, 0x80, 0xd0, 0x81, 0xb0, 0x81, 0xb0, 0x81, 0xb8,
    0x80, 0xc4, 0x80, 0xd8, 0x80, 0xe9, 0x40, 0xfc, 0x00, 0x08, 0x80, 0x1c, 0x80, 0x3f, 0x7f, 0x4f,
    0x7f, 0x4f, 0x7f, 0x43, 0x80, 0x31, 0x80, 0x1f, 0x00, 0x0d, 0x80, 0xfc, 0x80, 0xf0, 0x80, 0xd0,
    0x81, 0xb0, 0x81, 0xac, 0x81, 0xb4, 0x80, 0xc0, 0x80, 0xd6, 0x80, 0xe7, 0x00, 0xfa, 0x00, 0x06,
    0x80, 0x18, 0x80, 0x3f, 0x7f, 0x4f, 0x7f, 0x4b, 0x7f, 0x43, 0x80, 0x31, 0x80, 0x1f, 0x80, 0x0d,
    0xc0, 0xfc, 0x00, 0xf1, 0x80, 0xd2, 0x81, 0xb4, 0x81, 0xac, 0x81, 0xb4, 0x80, 0xc0, 0x80, 0xd6,
    0x80, 0xe7, 0x40, 0xfa, 0x00, 0x07, 0x80, 0x18, 0x80, 0x3f, 0x7f, 0x4f, 0x7f, 0x4f, 0x7f, 0x43,
    0x80, 0x35, 0x80, 0x21, 0x80, 0x0f, 0xd0, 0xfe, 0x00, 0xf3, 0x80, 0xd6, 0x81, 0xb4, 0x81, 0xb0,
    0x81, 0xb4, 0x80, 0xc2, 0x80, 0xd6, 0x80, 0xe7, 0x80, 0xfa, 0x80, 0x07, 0x80, 0x18, 0x80, 0x3f,
    0x7f, 0x53, 0x7f, 0x4f, 0x7f, 0x47, 0x80, 0x37, 0x80, 0x23, 0x80, 0x11, 0x18, 0x00, 0x00, 0xf5,
    0x80, 0xd8, 0x81, 0xb8, 0x81, 0xb0, 0x81, 0xb4, 0x80, 0xc0, 0x80, 0xd6, 0x80, 0xe7, 0x80, 0xf9,
    0x40, 0x07, 0x80, 0x16, 0x80, 0x3d, 0x7f, 0x53, 0x7f, 0x4f, 0x7f, 0x47, 0x80, 0x37, 0x80, 0x23,
    0x80, 0x11, 0x28, 0x00, 0x00, 0xf5, 0x80, 0xda, 0x81, 0xb8, 0x81, 0xac, 0x81, 0xb4, 0x80, 0xc0,
    0x80, 0xd4, 0x80, 0xe4, 0x00, 0xf8, 0x80, 0x05, 0x80, 0x14, 0x80, 0x3b, 0x7f, 0x4f, 0x7f, 0x4f,
    0x7f, 0x47, 0x80, 0x37, 0x80, 0x23, 0x80, 0x11, 0xc0, 0xff, 0x00, 0xf5, 0x80, 0xda, 0x81, 0xb8,
    0x81, 0xac, 0x81, 0xb4, 0x80, 0xc0, 0x80, 0xd2, 0x80, 0xe2, 0x80, 0xf6, 0x40, 0x04, 0x80, 0x12,
    0x80, 0x37, 0x7f, 0x4f, 0x7f, 0x4f, 0x7f, 0x47, 0x80, 0x37, 0x80, 0x23, 0x80, 0x11, 0x28, 0x00,
    0x00, 0xf5, 0x80, 0xdc, 0x81, 0xb8, 0x81, 0xac, 0x81, 0xb4, 0x80, 0xc0, 0x80, 0xd2, 0x80, 0xe2,
    0x80, 0xf5, 0x40, 0x04, 0x80, 0x11, 0x80, 0x37, 0x7f, 0x4f, 0x7f, 0x4f, 0x7f, 0x47, 0x80, 0x39,
    0x80, 0x25, 0x80, 0x13, 0x80, 0x01, 0x80, 0xf6, 0x80, 0xde, 0x81, 0xbc, 0x81, 0xac, 0x81, 0xb4,
    0x80, 0xc0, 0x80, 0xd2, 0x80, 0xe2, 0x80, 0xf5, 0x80, 0x04, 0x80, 0x11, 0x80, 0x35, 0x7f, 0x4f,
    0x7f, 0x4f, 0x7f, 0x47, 0x80, 0x3b, 0x80, 0x25, 0x80, 0x14, 0xe0, 0x02, 0x80, 0xf7, 0x80, 0xe0,
    0x81, 0xbc, 0x81, 0xb0, 0x81, 0xb4, 0x80, 0xc0, 0x80, 0xd0, 0x80, 0xe1, 0x00, 0xf5, 0x40, 0x04,
    0x80, 0x10, 0x80, 0x31, 0x7f, 0x4f, 0x7f, 0x4f, 0x7f, 0x47, 0x80, 0x3b, 0x80, 0x27, 0x80, 0x15,
    0x80, 0x03, 0x00, 0xf8, 0x80, 0xe1, 0x81, 0xbc, 0x81, 0xac, 0x81, 0xb4, 0x81, 0xbc, 0x80, 0xd0,
    0x80, 0xe0, 0x00, 0xf4, 0x80, 0x03, 0x80, 0x0f, 0x80, 0x2f, 0x7f, 0x4f, 0x7f, 0x4f, 0x7f, 0x47,
    0x80, 0x3d, 0x80, 0x27, 0x80, 0x16, 0x80, 0x04, 0x00, 0xf7, 0x80, 0xe4, 0x81, 0xbc, 0x81, 0xb0,
    0x81, 0xb0, 0x80, 0xc0, 0x80, 0xc0, 0x81, 0xb8, 0x80, 0x0d, 0x7f, 0x5b, 0x7f, 0x7b, 0x7f, 0x4b,
    0x80, 0xea, 0x81, 0xbc, 0x80, 0xc0, 0x80, 0x1f, 0x7f, 0x5b, 0x7f, 0x63, 0x80, 0x15, 0x80, 0xc0,
    0x81, 0xa0, 0x80, 0xc8, 0x80, 0x29, 0x7f, 0x57, 0x7f, 0x47, 0x80, 0xe9, 0x81, 0xa8, 0x81, 0x9c,
    0x80, 0xe8, 0x80, 0x3d, 0x7f, 0x5b, 0x80, 0x2f, 0x80, 0xce, 0x81, 0xa0, 0x81, 0xac, 0x00, 0x0f,
    0x7f, 0x4b, 0x7f, 0x5b, 0x80, 0x0f, 0x81, 0xbc, 0x81, 0xa0, 0x80, 0xca, 0x80, 0x2d, 0x7f, 0x5b,
    0x7f, 0x4f, 0x80, 0xef, 0x81, 0xb0, 0x81, 0xa4, 0x80, 0xef, 0x7f, 0x43, 0x7f, 0x5f, 0x80, 0x37,
    0x80, 0xd2, 0x81, 0xa4, 0x81, 0xb0, 0x80, 0x10, 0x7f, 0x4f, 0x7f, 0x5f, 0x80, 0x11, 0x80, 0xc0,
    0x81, 0xa0, 0x80, 0xca, 0x80, 0x2d, 0x7f, 0x5b, 0x7f, 0x4f, 0x80, 0xef, 0x81, 0xb0, 0x81, 0xa0,
    0x80, 0xee, 0x7f, 0x43, 0x7f, 0x5f, 0x80, 0x37, 0x80, 0xd2, 0x81, 0xa4, 0x81, 0xb0, 0x80, 0x11,
    0x7f, 0x4f, 0x7f, 0x5f, 0x80, 0x12, 0x80, 0xc0, 0x81, 0xa0, 0x80, 0xca, 0x80, 0x2f, 0x7f, 0x5b,
    0x7f, 0x4f, 0x80, 0xf0, 0x81, 0xb0, 0x81, 0xa4, 0x80, 0xf0, 0x7f, 0x43, 0x7f, 0x63, 0x80, 0x37,
    0x80, 0xd4, 0x81, 0xa8, 0x81, 0xb0, 0x80, 0x12, 0x7f, 0x4f, 0x7f, 0x5f, 0x80, 0x12, 0x80, 0xc0,
    0x81, 0xa0, 0x80, 0xca, 0x80, 0x2d, 0x7f, 0x5b, 0x7f, 0x4f, 0x80, 0xef, 0x81, 0xb0, 0x81, 0xa0,
    0x80, 0xee, 0x80, 0x3f, 0x7f, 0x5f, 0x80, 0x35, 0x80, 0xd2, 0x81, 0xa4, 0x81, 0xb0, 0x80, 0x10,
    0x7f, 0x4f, 0x7f, 0x5b, 0x80, 0x10, 0x81, 0xbc, 0x81, 0x9c, 0x80, 0xc8, 0x80, 0x2d, 0x7f, 0x5b,
    0x7f, 0x4f, 0x80, 0xee, 0x81, 0xb0, 0x81, 0xa0, 0x80, 0xed, 0x80, 0x3f, 0x7f, 0x5f, 0x80, 0x35,
    0x80, 0xd0, 0x81, 0xa4, 0x81, 0xb0, 0x80, 0x0f, 0x7f, 0x4f, 0x7f, 0x5b, 0x80, 0x10, 0x81, 0xbc,
    0x81, 0x9c, 0x80, 0xc8, 0x80, 0x2d, 0x7f, 0x5b, 0x7f, 0x4f, 0x80, 0xee, 0x81, 0xb0, 0x81, 0xa0,
    0x80, 0xed, 0x80, 0x3f, 0x7f, 0x5f, 0x80, 0x35, 0x80, 0xd2, 0x81, 0xa4, 0x81, 0xb0, 0x80, 0x10,
    0x7f, 0x4f, 0x7f, 0x5f, 0x80, 0x11, 0x81, 0xbc, 0x81, 0xa0, 0x80, 0xc8, 0x80, 0x2d, 0x7f, 0x5b,
    0x7f, 0x4f, 0x80, 0xef, 0x81, 0xb0, 0x81, 0xa0, 0x80, 0xee, 0x7f, 0x43, 0x7f, 0x63, 0x80, 0x37,
    0x80, 0xd2, 0x81, 0xa4, 0x81, 0xb0, 0x80, 0x11, 0x7f, 0x4f, 0x7f, 0x5f, 0x80, 0x12, 0x80, 0xc0,
    0x81, 0xa0, 0x80, 0xca, 0x80, 0x2d, 0x7f, 0x5b, 0x7f, 0x4f, 0x80, 0xef, 0x81, 0xb0, 0x81, 0xa0,
    0x80, 0xef, 0x7f, 0x43, 0x7f, 0x63, 0x80, 0x37, 0x80, 0xd2, 0x81, 0xa4, 0x81, 0xb0, 0x80, 0x11,
    0x7f, 0x4f, 0x7f, 0x5f, 0x80, 0x12, 0x80, 0xc0, 0x81, 0xa0, 0x80, 0xca, 0x80, 0x2d, 0x7f, 0x5b,
    0x7f, 0x4f, 0x80, 0xef, 0x81, 0xb0, 0x81, 0xa0, 0x80, 0xee, 0x7f, 0x43, 0x7f, 0x63, 0x80, 0x37,
    0x80, 0xd2, 0x81, 0xa4, 0x81, 0xb0, 0x80, 0x10, 0x7f, 0x4f, 0x7f, 0x5f, 0x80, 0x11, 0x81, 0xbc,
    0x81, 0x9c, 0x80, 0xc8, 0x80, 0x2d, 0x7f, 0x5b, 0x7f, 0x4f, 0x80, 0xee, 0x81, 0xb0, 0x81, 0xa0
};

#ifdef __cplusplus
extern "C" {
#endif

void BtHfpClientMsgHandler(void *msg) {
    BtEvent* pEvent = NULL;
    if(!msg) {
        printf("Msg is NULL, return.\n");
        return;
    }

    pEvent = ( BtEvent *) msg;

    ALOGD(LOGTAG " BtHfpClientMsgHandler event = %d", pEvent->event_id);
    switch(pEvent->event_id) {
        case PROFILE_API_START:
            if (pHfpClient) {
                pHfpClient->HandleEnableClient();
            }
            break;
        case PROFILE_API_STOP:
            if (pHfpClient) {
                pHfpClient->HandleDisableClient();
            }
            break;
        default:
            if(pHfpClient) {
               pHfpClient->ProcessEvent(( BtEvent *) msg);
            }
            break;
    }
    delete pEvent;
}

#ifdef __cplusplus
}
#endif

static void connection_state_cb(const bt_bdaddr_t* bd_addr, bthf_client_connection_state_t state, unsigned int peer_feat, unsigned chld_feat) {
    ALOGD(LOGTAG " Connection State CB");
    BtEvent *pEvent = new BtEvent;
    memcpy(&pEvent->hfp_client_event.bd_addr, bd_addr, sizeof(bt_bdaddr_t));
    pEvent->hfp_client_event.peer_feat = peer_feat;
    pEvent->hfp_client_event.chld_feat = chld_feat;
    switch( state ) {
        case BTHF_CLIENT_CONNECTION_STATE_DISCONNECTED:
            pEvent->hfp_client_event.event_id = HFP_CLIENT_DISCONNECTED_CB;
        break;
        case BTHF_CLIENT_CONNECTION_STATE_CONNECTING:
            pEvent->hfp_client_event.event_id = HFP_CLIENT_CONNECTING_CB;
        break;
        case BTHF_CLIENT_CONNECTION_STATE_CONNECTED:
            pEvent->hfp_client_event.event_id = HFP_CLIENT_CONNECTED_CB;
        break;
        case BTHF_CLIENT_CONNECTION_STATE_SLC_CONNECTED:
            pEvent->hfp_client_event.event_id = HFP_CLIENT_SLC_CONNECTED_CB;
        break;
        case BTHF_CLIENT_CONNECTION_STATE_DISCONNECTING:
            pEvent->hfp_client_event.event_id = HFP_CLIENT_DISCONNECTING_CB;
        break;
        default:
        break;
    }
    PostMessage(THREAD_ID_HFP_CLIENT, pEvent);
}

static void audio_state_cb(const bt_bdaddr_t* bd_addr, bthf_client_audio_state_t state) {
    ALOGD(LOGTAG " Audio State CB");
    BtEvent *pEvent = new BtEvent;
    memcpy(&pEvent->hfp_client_event.bd_addr, bd_addr, sizeof(bt_bdaddr_t));
    switch( state ) {
        case BTHF_CLIENT_AUDIO_STATE_DISCONNECTED:
            pEvent->hfp_client_event.event_id = HFP_CLIENT_AUDIO_STATE_DISCONNECTED_CB;
        break;
        case BTHF_CLIENT_AUDIO_STATE_CONNECTING:
            pEvent->hfp_client_event.event_id = HFP_CLIENT_AUDIO_STATE_CONNECTING_CB;
        break;
        case BTHF_CLIENT_AUDIO_STATE_CONNECTED:
            pEvent->hfp_client_event.event_id = HFP_CLIENT_AUDIO_STATE_CONNECTED_CB;
        break;
        case BTHF_CLIENT_AUDIO_STATE_CONNECTED_MSBC:
            pEvent->hfp_client_event.event_id = HFP_CLIENT_AUDIO_STATE_CONNECTED_MSBC_CB;
        break;
    }
    PostMessage(THREAD_ID_HFP_CLIENT, pEvent);
}

void vr_cmd_cb(const bt_bdaddr_t* bd_addr, bthf_client_vr_state_t state) {
    ALOGD(LOGTAG "VR state is %s",(state == BTHF_CLIENT_VR_STATE_STOPPED) ? "stopped": "started");
    fprintf(stdout, "VR state is %s\n",(state == BTHF_CLIENT_VR_STATE_STOPPED) ? "stopped": "started");
}

void network_state_cb (const bt_bdaddr_t* bd_addr, bthf_client_network_state_t state) {
    ALOGD(LOGTAG "network state is %s", (state == BTHF_CLIENT_NETWORK_STATE_NOT_AVAILABLE) ? "not available": "available");
    fprintf(stdout, "network state is %s\n", (state == BTHF_CLIENT_NETWORK_STATE_NOT_AVAILABLE) ? "not available": "available");
}

void network_roaming_cb (const bt_bdaddr_t* bd_addr, bthf_client_service_type_t type) {
    ALOGD(LOGTAG "AG is in %s", (type == BTHF_CLIENT_SERVICE_TYPE_HOME) ? "home network": "roaming");
    fprintf(stdout, "AG is in %s\n", (type == BTHF_CLIENT_SERVICE_TYPE_HOME) ? "home network": "roaming");
}

void network_signal_cb (const bt_bdaddr_t* bd_addr, int signal) {
    ALOGD(LOGTAG "signal level is %d", signal);
    fprintf(stdout, "signal level is %d\n", signal);
}

void battery_level_cb (const bt_bdaddr_t* bd_addr, int level) {
    ALOGD(LOGTAG "battery level is %d", level);
    fprintf(stdout, "battery level is %d\n", level);
}

void current_operator_cb (const bt_bdaddr_t* bd_addr, const char *name) {
    ALOGD(LOGTAG "operator name is %s", name);
    fprintf(stdout, "operator name is %s\n", name);
}

void call_cb (const bt_bdaddr_t* bd_addr, bthf_client_call_t call) {
    ALOGD(LOGTAG "%s call is in progress", (call == BTHF_CLIENT_CALL_NO_CALLS_IN_PROGRESS) ? "no": "a");
    fprintf(stdout, "%s call is in progress\n", (call == BTHF_CLIENT_CALL_NO_CALLS_IN_PROGRESS) ? "no": "a");
}

void callsetup_cb (const bt_bdaddr_t* bd_addr, bthf_client_callsetup_t callsetup) {
   BtEvent *pEvent = new BtEvent;
   if (callsetup == BTHF_CLIENT_CALLSETUP_NONE) {
      fprintf(stdout, "no call in setup\n");
      ALOGD(LOGTAG "no call is setup");
      // TODO: post this only when MT call ends
      pEvent->hfp_client_event.event_id = HFP_CLIENT_STOP_RINGTONE_REQ;
      PostMessage(THREAD_ID_HFP_CLIENT, pEvent);
   }
   else if(callsetup == BTHF_CLIENT_CALLSETUP_INCOMING) {
      fprintf(stdout, "Incoming call is in setup\n");
      ALOGD(LOGTAG "Incoming call is in setup");
   }
   else if(callsetup == BTHF_CLIENT_CALLSETUP_OUTGOING) {
      fprintf(stdout, "Outgoing call is in setup\n");
      ALOGD(LOGTAG "Outgoing call is in setup");
   }
   else if(callsetup == BTHF_CLIENT_CALLSETUP_ALERTING) {
      fprintf(stdout, "Outgoing call in alerting state\n");
      ALOGD(LOGTAG "Outgoing call in alerting state");
   }
}

void callheld_cb (const bt_bdaddr_t* bd_addr, bthf_client_callheld_t callheld) {
   if (callheld == BTHF_CLIENT_CALLHELD_NONE) {
      fprintf(stdout, "no held call\n");
      ALOGD(LOGTAG "no held call");
   }
   else if (callheld == BTHF_CLIENT_CALLHELD_HOLD_AND_ACTIVE) {
      fprintf(stdout,"a call is placed on hold or calls are swapped\n");
      ALOGD(LOGTAG "a call is placed on hold or calls are swapped");
   }
   else if (callheld == BTHF_CLIENT_CALLHELD_HOLD) {
      fprintf(stdout,"a call is on hold, no active calls\n");
      ALOGD(LOGTAG "a call is on hold, no active calls");
   }
}

void resp_and_hold_cb (const bt_bdaddr_t* bd_addr, bthf_client_resp_and_hold_t resp_and_hold) {
   if (resp_and_hold == BTHF_CLIENT_RESP_AND_HOLD_HELD) {
      fprintf(stdout,"incoming call put on held\n");
      ALOGD(LOGTAG "incoming call put on held");
   }
   else if (resp_and_hold == BTRH_CLIENT_RESP_AND_HOLD_ACCEPT) {
      fprintf(stdout,"held incoming call accepted\n");
      ALOGD(LOGTAG "held incoming call accepted");
   }
   else if (resp_and_hold == BTRH_CLIENT_RESP_AND_HOLD_REJECT) {
      fprintf(stdout,"held incoming call rejected\n");
      ALOGD(LOGTAG "held incoming call rejected");
   }
}

void clip_cb (const bt_bdaddr_t* bd_addr, const char *number) {
   ALOGD(LOGTAG "CLIP number is %s", number);
   fprintf(stdout, "CLIP number is %s\n", number);
}

void call_waiting_cb (const bt_bdaddr_t* bd_addr, const char *number) {
   ALOGD(LOGTAG "a call is waiting from number %s", number);
   fprintf(stdout, "a call is waiting from number %s\n", number);
}

void current_calls_cb (const bt_bdaddr_t* bd_addr, int index, bthf_client_call_direction_t dir,
                                            bthf_client_call_state_t state,
                                            bthf_client_call_mpty_type_t mpty,
                                            const char *number) {
   ALOGD(LOGTAG "%s: index %d, call direction %d, call state %d, multiparty %d, number %s",
        __func__, index, dir, state, mpty, number);
   fprintf(stdout, "%s: index %d, call direction %d, call state %d, multiparty %d, number %s\n",
    __func__, index, dir, state, mpty, number);
}

void volume_change_cb (const bt_bdaddr_t* bd_addr, bthf_client_volume_type_t type, int volume) {
   BtEvent *pEvent = new BtEvent;
   ALOGD(LOGTAG "%s : %s volume is %d\n", __func__,
          (type == BTHF_CLIENT_VOLUME_TYPE_SPK) ? "speaker": "mic", volume);
   fprintf(stdout,"%s : %s volume is %d\n", __func__,
          (type == BTHF_CLIENT_VOLUME_TYPE_SPK) ? "speaker": "mic", volume);

   if (type == BTHF_CLIENT_VOLUME_TYPE_SPK)
       pEvent->hfp_client_event.event_id = HFP_CLIENT_API_SPK_VOL_CTRL_REQ;
   else if (type == BTHF_CLIENT_VOLUME_TYPE_MIC)
       pEvent->hfp_client_event.event_id = HFP_CLIENT_API_MIC_VOL_CTRL_REQ;

   pEvent->hfp_client_event.arg1 = volume;
   /* Trigger is from Remote side no need to send AT+VGS,
    * so second argument set to flase */
   pEvent->hfp_client_event.arg2 = false;
   PostMessage(THREAD_ID_HFP_CLIENT, pEvent);
}

void cmd_complete_cb (const bt_bdaddr_t* bd_addr, bthf_client_cmd_complete_t type, int cme) {
   ALOGD(LOGTAG "cmd_complete_cb, type %d, error %d", (int)type, cme);
}

void subscriber_info_cb (const bt_bdaddr_t* bd_addr, const char *name, bthf_client_subscriber_service_type_t type) {
   if (type == BTHF_CLIENT_SERVICE_UNKNOWN) {
       ALOGD(LOGTAG "subscriber name is %s type is unknown", name);
       fprintf(stdout, "subscriber name is %s type is unknown\n", name);
   }
   else if (type == BTHF_CLIENT_SERVICE_VOICE) {
       ALOGD(LOGTAG "subscriber name is %s type is voice", name);
       fprintf(stdout, "subscriber name is %s type is voice\n", name);
   }
   else if (type == BTHF_CLIENT_SERVICE_FAX) {
       ALOGD(LOGTAG "subscriber name is %s type is fax", name);
       fprintf(stdout, "subscriber name is %s type is fax\n", name);
   }
}

void in_band_ring_cb (const bt_bdaddr_t* bd_addr, bthf_client_in_band_ring_state_t in_band) {
   if (in_band == BTHF_CLIENT_IN_BAND_RINGTONE_NOT_PROVIDED) {
       ALOGD(LOGTAG " in-band ringtone not provided");
       fprintf(stdout, "in-band ringtone not provided\n");
   }
   else if (in_band == BTHF_CLIENT_IN_BAND_RINGTONE_PROVIDED) {
       ALOGD(LOGTAG " in-band ringtone provided");
       fprintf(stdout, "in-band ringtone provided\n");
   }
}

void last_voice_tag_number_cb (const bt_bdaddr_t* bd_addr, const char *number) {
   ALOGD(LOGTAG "last_voice_tag_number_cb: number is %s", number);
}

void ring_indication_cb (const bt_bdaddr_t* bd_addr) {
   BtEvent *pEvent = new BtEvent;
   ALOGD(LOGTAG "ring_indication");
   fprintf(stdout, "RING indication for incoming call\n");
   pEvent->hfp_client_event.event_id = HFP_CLIENT_PLAY_RINGTONE_REQ;
   PostMessage(THREAD_ID_HFP_CLIENT, pEvent);
}

void cgmi_vendor_cb (const char *str) {
   ALOGD(LOGTAG " cgmi_vendor_cb %s", str);
}

void cgmm_vendor_cb (const char *str) {
   ALOGD(LOGTAG " cgmm_vendor_cb %s", str);
}


static bthf_client_callbacks_t sBluetoothHfpClientCallbacks = {
    sizeof(sBluetoothHfpClientCallbacks),
    connection_state_cb,
    audio_state_cb,
    vr_cmd_cb,
    network_state_cb,
    network_roaming_cb,
    network_signal_cb,
    battery_level_cb,
    current_operator_cb,
    call_cb,
    callsetup_cb,
    callheld_cb,
    resp_and_hold_cb,
    clip_cb,
    call_waiting_cb,
    current_calls_cb,
    volume_change_cb,
    cmd_complete_cb,
    subscriber_info_cb,
    in_band_ring_cb,
    last_voice_tag_number_cb,
    ring_indication_cb,
};

static bthf_client_vendor_callbacks_t sBluetoothHfpClientVendorCallbacks = {
    sizeof(sBluetoothHfpClientVendorCallbacks),
    cgmi_vendor_cb,
    cgmm_vendor_cb,
};

void Hfp_Client::HandleEnableClient(void) {
    if (bluetooth_interface != NULL)
    {
        sBtHfpClientInterface = (bthf_client_interface_t *)bluetooth_interface->
                get_profile_interface(BT_PROFILE_HANDSFREE_CLIENT_ID);

        if (sBtHfpClientInterface == NULL)
        {
            // TODO: sent message to indicate failure for profile init
            ALOGE(LOGTAG "get profile interface failed, returning");
            return;
        }
        memset(&mConnectedDevice, 0, sizeof(bt_bdaddr_t));
        change_state(HFP_CLIENT_STATE_DISCONNECTED);
        sBtHfpClientInterface->init(&sBluetoothHfpClientCallbacks);
        BtEvent *pEvent = new BtEvent;
        pEvent->profile_start_event.event_id = PROFILE_EVENT_START_DONE;
        pEvent->profile_start_event.profile_id = PROFILE_ID_HFP_CLIENT;
        pEvent->profile_start_event.status = true;
        PostMessage(THREAD_ID_GAP, pEvent);
    }
}

void Hfp_Client::HandleDisableClient(void) {
   change_state(HFP_CLIENT_STATE_NOT_STARTED);
   if(sBtHfpClientInterface != NULL) {
       sBtHfpClientInterface->cleanup();
       sBtHfpClientInterface = NULL;
   }
   ConfigureAudio(false);
   BtEvent *pEvent = new BtEvent;
   pEvent->profile_stop_event.event_id = PROFILE_EVENT_STOP_DONE;
   pEvent->profile_stop_event.profile_id = PROFILE_ID_HFP_CLIENT;
   pEvent->profile_stop_event.status = true;
   PostMessage(THREAD_ID_GAP, pEvent);
}

void Hfp_Client::ProcessEvent(BtEvent* pEvent) {
    ALOGD(LOGTAG " Processing event %d in state %d", pEvent->event_id, mClientState);
    fprintf(stdout, " Processing event %d in state %d\n", pEvent->event_id, mClientState);
    switch(mClientState) {
        case HFP_CLIENT_STATE_DISCONNECTED:
            state_disconnected_handler(pEvent);
            break;
        case HFP_CLIENT_STATE_CONNECTING:
            state_connecting_handler(pEvent);
            break;
        case HFP_CLIENT_STATE_CONNECTED:
            state_connected_handler(pEvent);
            break;
        case HFP_CLIENT_STATE_AUDIO_ON:
            state_audio_on_handler(pEvent);
            break;
        case HFP_CLIENT_STATE_NOT_STARTED:
            ALOGE(LOGTAG," STATE UNINITIALIZED, return");
            break;
    }
}

void Hfp_Client::state_disconnected_handler(BtEvent* pEvent) {
    char str[18];
    ALOGD(LOGTAG "state_disconnected_handler Processing event %d", pEvent->event_id);
    switch(pEvent->event_id) {
        case HFP_CLIENT_API_CONNECT_REQ:
            memcpy(&mConnectingDevice, &pEvent->hfp_client_event.bd_addr, sizeof(bt_bdaddr_t));
            if (sBtHfpClientInterface != NULL) {
                sBtHfpClientInterface->connect(&pEvent->hfp_client_event.bd_addr);
            }
            bdaddr_to_string(&mConnectingDevice, str, 18);
            fprintf(stdout, "connecting with device %s\n", str);
            ALOGD(LOGTAG "connecting with device %s", str);
            change_state(HFP_CLIENT_STATE_CONNECTING);
            break;
        case HFP_CLIENT_CONNECTING_CB: // TODO: check if this state is needed
            // intentional fall through
        case HFP_CLIENT_CONNECTED_CB:
            memcpy(&mConnectingDevice, &pEvent->hfp_client_event.bd_addr, sizeof(bt_bdaddr_t));
            change_state(HFP_CLIENT_STATE_CONNECTING);
            break;
        case HFP_CLIENT_SLC_CONNECTED_CB: // TODO: shoud not happen. cross check
            memset(&mConnectingDevice, 0, sizeof(bt_bdaddr_t));
            memcpy(&mConnectedDevice, &pEvent->hfp_client_event.bd_addr, sizeof(bt_bdaddr_t));
            peer_feat = pEvent->hfp_client_event.peer_feat;
            chld_feat = pEvent->hfp_client_event.chld_feat;
            mAudioWbs = false;

            bdaddr_to_string(&mConnectedDevice, str, 18);
            fprintf(stdout, "SLC connected with device %s\n", str);
            ALOGD(LOGTAG "SLC connected with device %s", str);
            change_state(HFP_CLIENT_STATE_CONNECTED);
            break;
        default:
            ALOGD(LOGTAG," event not handled %d ", pEvent->event_id);
            break;
    }
}
void Hfp_Client::state_connecting_handler(BtEvent* pEvent) {
    char str[18];
    ALOGD(LOGTAG "state_connecting_handler Processing event %d", pEvent->event_id);
    switch(pEvent->event_id) {
        case HFP_CLIENT_API_CONNECT_REQ:
            ALOGE(LOGTAG,"Already in connecting_handler,Ignore request");
            fprintf(stdout,"Already in connecting_handler,Ignore request\n");
            break;
        case HFP_CLIENT_CONNECTING_CB:
        // intentional fall through
        case HFP_CLIENT_CONNECTED_CB:
            break;
        case HFP_CLIENT_SLC_CONNECTED_CB:
            memcpy(&mConnectedDevice, &pEvent->hfp_client_event.bd_addr, sizeof(bt_bdaddr_t));
            memset(&mConnectingDevice, 0, sizeof(bt_bdaddr_t));
            peer_feat = pEvent->hfp_client_event.peer_feat;
            chld_feat = pEvent->hfp_client_event.chld_feat;
            mAudioWbs = false;

            bdaddr_to_string(&mConnectedDevice, str, 18);
            fprintf(stdout, "SLC connected with device %s\n", str);
            ALOGD(LOGTAG "SLC connected with device %s", str);
            change_state(HFP_CLIENT_STATE_CONNECTED);
            break;
        case HFP_CLIENT_DISCONNECTED_CB:
            bdaddr_to_string(&pEvent->hfp_client_event.bd_addr, str, 18);
            fprintf(stdout, "Disconnected from or Unable to connect with device %s\n", str);
            ALOGD(LOGTAG "Disconnected from or Unable to connect with device %s", str);

            memset(&mConnectedDevice, 0, sizeof(bt_bdaddr_t));
            memset(&mConnectingDevice, 0, sizeof(bt_bdaddr_t));
            peer_feat = 0;
            chld_feat = 0;
            mAudioWbs = false;
            change_state(HFP_CLIENT_STATE_DISCONNECTED);
            break;
        default:
            ALOGD(LOGTAG," event not handled %d ", pEvent->event_id);
            break;
    }
}

void Hfp_Client::state_connected_handler(BtEvent* pEvent) {
    ALOGD(LOGTAG "state_connected_handler Processing event %d", pEvent->event_id);
    char str[18];
    BtEvent *pControlRequest, *pReleaseControlReq;
    switch(pEvent->event_id) {
        case HFP_CLIENT_API_CONNECT_REQ:
            ALOGE(LOGTAG,"Already in connected_handler,Ignore Request");
            fprintf(stdout,"Already in connected_handler,Ignore Request\n");
            break;
        case HFP_CLIENT_API_DISCONNECT_REQ:
            if (memcmp(&pEvent->hfp_client_event.bd_addr,&mConnectedDevice,sizeof(bt_bdaddr_t))) {
                bdaddr_to_string(&pEvent->hfp_client_event.bd_addr, str, 18);
                ALOGE(LOGTAG, "%s, Device not connected: %s", __func__,str);
                fprintf(stdout, "Device not connected: %s\n", str);
                break;
            }

            if (mAudioMode == HFP_CLIENT_MODE_RINGTONE)
            {
                ALOGD("Audio Mode is ring tone, stop the ring tone");
                fprintf(stdout, "Audio Mode is ring tone, stop the ring tone\n");
                change_mode(HFP_CLIENT_MODE_NORMAL);
                StopRingTone();
            }
            // release control
            pReleaseControlReq = new BtEvent;
            pReleaseControlReq->btamControlRelease.event_id = BT_AM_RELEASE_CONTROL;
            pReleaseControlReq->btamControlRelease.profile_id = PROFILE_ID_HFP_CLIENT;
            PostMessage(THREAD_ID_BT_AM, pReleaseControlReq);

            mcontrolStatus = STATUS_LOSS_TRANSIENT;
            memset(&mConnectedDevice, 0, sizeof(bt_bdaddr_t));
            memset(&mConnectingDevice, 0, sizeof(bt_bdaddr_t));
            if (sBtHfpClientInterface != NULL) {
                sBtHfpClientInterface->disconnect(&pEvent->hfp_client_event.bd_addr);
            }

            bdaddr_to_string(&pEvent->hfp_client_event.bd_addr, str, 18);
            fprintf(stdout, "Disconnecting with device %s\n", str);
            ALOGD(LOGTAG "Disconnecting with device %s", str);
            change_state(HFP_CLIENT_STATE_CONNECTING);
            break;
        case HFP_CLIENT_DISCONNECTED_CB:
            // release control
            pReleaseControlReq = new BtEvent;
            pReleaseControlReq->btamControlRelease.event_id = BT_AM_RELEASE_CONTROL;
            pReleaseControlReq->btamControlRelease.profile_id = PROFILE_ID_HFP_CLIENT;
            PostMessage(THREAD_ID_BT_AM, pReleaseControlReq);

            mcontrolStatus = STATUS_LOSS_TRANSIENT;
            bdaddr_to_string(&pEvent->hfp_client_event.bd_addr, str, 18);
            fprintf(stdout, "Disconnected with device %s\n", str);
            ALOGD(LOGTAG "Disconnected with device %s", str);

            memset(&mConnectedDevice, 0, sizeof(bt_bdaddr_t));
            memset(&mConnectingDevice, 0, sizeof(bt_bdaddr_t));
            peer_feat = 0;
            chld_feat = 0;
            mAudioWbs = false;
            if (mAudioMode == HFP_CLIENT_MODE_RINGTONE)
            {
                ALOGD("Audio Mode is ring tone, stop the ring tone");
                fprintf(stdout, "Audio Mode is ring tone, stop the ring tone\n");;
                StopRingTone();
            }
            change_mode(HFP_CLIENT_MODE_NORMAL);
            change_state(HFP_CLIENT_STATE_DISCONNECTED);
            break;
        case HFP_CLIENT_DISCONNECTING_CB:
            // release control
            pReleaseControlReq = new BtEvent;
            pReleaseControlReq->btamControlRelease.event_id = BT_AM_RELEASE_CONTROL;
            pReleaseControlReq->btamControlRelease.profile_id = PROFILE_ID_HFP_CLIENT;
            PostMessage(THREAD_ID_BT_AM, pReleaseControlReq);
            mcontrolStatus = STATUS_LOSS_TRANSIENT;

            break;
        case HFP_CLIENT_API_CONNECT_AUDIO_REQ:
            if (memcmp(&pEvent->hfp_client_event.bd_addr,&mConnectedDevice,sizeof(bt_bdaddr_t))) {
                bdaddr_to_string(&pEvent->hfp_client_event.bd_addr, str, 18);
                ALOGE(LOGTAG, "%s, Device not connected: %s", __func__,str);
                fprintf(stdout, "Device not connected: %s\n", str);
                break;
            }
            bdaddr_to_string(&pEvent->hfp_client_event.bd_addr, str, 18);
            fprintf(stdout, "Connecting SCO/eSCO with device %s\n", str);
            ALOGD(LOGTAG "Connecting SCO/eSCO with device %s", str);

            if (sBtHfpClientInterface != NULL) {
                sBtHfpClientInterface->connect_audio(&mConnectedDevice);
            }
            break;
        case HFP_CLIENT_AUDIO_STATE_CONNECTED_MSBC_CB:
            mAudioWbs = true;
            // intentional fall through.
        case HFP_CLIENT_AUDIO_STATE_CONNECTED_CB:
            bdaddr_to_string(&pEvent->hfp_client_event.bd_addr, str, 18);
            fprintf(stdout, "SCO/eSCO connected with device %s\n", str);
            ALOGD(LOGTAG " SCO/eSCO connected with device %s", str);

            if (mcontrolStatus == STATUS_LOSS || mcontrolStatus == STATUS_LOSS_TRANSIENT)
            {
                fprintf(stdout, "Connected state: Requesting for focus\n");
                ALOGD(LOGTAG  " Connected state: Requesting for focus");
                pControlRequest = new BtEvent;
                pControlRequest->btamControlReq.event_id = BT_AM_REQUEST_CONTROL;
                pControlRequest->btamControlReq.profile_id = PROFILE_ID_HFP_CLIENT;
                pControlRequest->btamControlReq.request_type = REQUEST_TYPE_TRANSIENT;
                PostMessage(THREAD_ID_BT_AM, pControlRequest);
            }
            else
            {
                // we already have the focus, configure for SCO connection. TODO: cross check
                fprintf(stdout, "Connected state: already have focus, configure audio for SCO\n");
                ALOGD(LOGTAG " Connected state: already have focus, configure audio for SCO, current mode %d", mAudioMode);

                // for MT call, stop ring tone if it is playing before connecting sco
                if (mAudioMode == HFP_CLIENT_MODE_RINGTONE)
                {
                    ALOGD("Audio Mode is ring tone, stop the ring tone");
                    fprintf(stdout, "Audio Mode is ring tone, stop the ring tone\n");
                    change_mode(HFP_CLIENT_MODE_NORMAL);
                    StopRingTone();
                }

                ConfigureAudio(true);
            }

            change_mode(HFP_CLIENT_MODE_IN_CALL);
            change_state(HFP_CLIENT_STATE_AUDIO_ON);
            break;
        case HFP_CLIENT_PLAY_RINGTONE_REQ:
            if (mAudioMode != HFP_CLIENT_MODE_RINGTONE)
            {
                change_mode(HFP_CLIENT_MODE_RINGTONE);

                pControlRequest = new BtEvent;
                pControlRequest->btamControlReq.event_id = BT_AM_REQUEST_CONTROL;
                pControlRequest->btamControlReq.profile_id = PROFILE_ID_HFP_CLIENT;
                pControlRequest->btamControlReq.request_type = REQUEST_TYPE_TRANSIENT;
                PostMessage(THREAD_ID_BT_AM, pControlRequest);
            }
            else
            {
                ALOGD("Audio Mode is ring tone, play the ring tone");
                fprintf(stdout, "Audio Mode is ring tone, play the ring tone\n");
                PlayRingTone();
            }
            break;
        case HFP_CLIENT_STOP_RINGTONE_REQ:
            if (mAudioMode == HFP_CLIENT_MODE_RINGTONE)
            {
                ALOGD("Audio Mode is ring tone, stop the ring tone");
                fprintf(stdout, "Audio Mode is ring tone, stop the ring tone\n");

                StopRingTone();
                // release control
                pReleaseControlReq = new BtEvent;
                pReleaseControlReq->btamControlRelease.event_id = BT_AM_RELEASE_CONTROL;
                pReleaseControlReq->btamControlRelease.profile_id = PROFILE_ID_HFP_CLIENT;
                PostMessage(THREAD_ID_BT_AM, pReleaseControlReq);

                mcontrolStatus = STATUS_LOSS_TRANSIENT;

                change_mode(HFP_CLIENT_MODE_NORMAL);
            }
            break;
        case BT_AM_CONTROL_STATUS:
            ALOGD(LOGTAG "earlier status = %d  new status = %d", mcontrolStatus,
                                   pEvent->btamControlStatus.status_type);
            fprintf(stdout, "earlier status = %d  new status = %d\n", mcontrolStatus,
                                   pEvent->btamControlStatus.status_type);

            mcontrolStatus = pEvent->btamControlStatus.status_type;

            switch(mcontrolStatus) {
                 case STATUS_LOSS:
                    // this should not occur.
                    break;
                 case STATUS_LOSS_TRANSIENT:
                    // this should not occur.
                    break;
                 case STATUS_GAIN:
                    // this should not occur.
                    break;
                 case STATUS_GAIN_TRANSIENT:
                    if (mAudioMode == HFP_CLIENT_MODE_RINGTONE)
                    {
                        ALOGD(LOGTAG " in Ringtone mode, configure and play ringtone");
                        fprintf(stdout, "in Ringtone mode, configure and play ringtone\n");
                        ConfigureRingTonePlayback();
                        PlayRingTone();
                    }
                    else
                    {
                        ALOGD(LOGTAG " Configure audio for SCO");
                        fprintf(stdout, "Configure audio for SCO\n");
                        ConfigureAudio(true);
                    }
                    break;
            }
            break;
        case HFP_CLIENT_API_ACCEPT_CALL_REQ:
            if (sBtHfpClientInterface != NULL) {
                sBtHfpClientInterface->handle_call_action(&mConnectedDevice, BTHF_CLIENT_CALL_ACTION_ATA, 0);
            }
            break;
        case HFP_CLIENT_API_RELEASE_HELD_CALL_REQ:
            if (sBtHfpClientInterface != NULL) {
                sBtHfpClientInterface->handle_call_action(&mConnectedDevice, BTHF_CLIENT_CALL_ACTION_CHLD_0, 0);
            }
            break;
        case HFP_CLIENT_API_REJECT_CALL_REQ:
            if (mAudioMode == HFP_CLIENT_MODE_RINGTONE)
            {
                ALOGD("Audio Mode is ring tone, stop the ring tone");
                fprintf(stdout, "Audio Mode is ring tone, stop the ring tone\n");

                StopRingTone();
                // release control
                pReleaseControlReq = new BtEvent;
                pReleaseControlReq->btamControlRelease.event_id = BT_AM_RELEASE_CONTROL;
                pReleaseControlReq->btamControlRelease.profile_id = PROFILE_ID_HFP_CLIENT;
                PostMessage(THREAD_ID_BT_AM, pReleaseControlReq);

                mcontrolStatus = STATUS_LOSS_TRANSIENT;

                change_mode(HFP_CLIENT_MODE_NORMAL);
            }
            // intentional fall through. TODO: cross check
        case HFP_CLIENT_API_END_CALL_REQ:
            if (sBtHfpClientInterface != NULL) {
                sBtHfpClientInterface->handle_call_action(&mConnectedDevice, BTHF_CLIENT_CALL_ACTION_CHUP, 0);
            }
            break;
        case HFP_CLIENT_API_RELEASE_ACTIVE_ACCEPT_WAITING_OR_HELD_CALL_REQ:
            if (sBtHfpClientInterface != NULL) {
                sBtHfpClientInterface->handle_call_action(&mConnectedDevice, BTHF_CLIENT_CALL_ACTION_CHLD_1, 0);
            }
            break;
        case HFP_CLIENT_API_HOLD_CALL_REQ:
            // intentional fall through
        case HFP_CLIENT_API_SWAP_CALLS_REQ:
            if (sBtHfpClientInterface != NULL) {
                sBtHfpClientInterface->handle_call_action(&mConnectedDevice, BTHF_CLIENT_CALL_ACTION_CHLD_2, 0);
            }
            break;
        case HFP_CLIENT_API_ADD_HELD_CALL_TO_CONF_REQ:
            if (sBtHfpClientInterface != NULL) {
                sBtHfpClientInterface->handle_call_action(&mConnectedDevice, BTHF_CLIENT_CALL_ACTION_CHLD_3, 0);
            }
            break;
        case HFP_CLIENT_API_RELEASE_SPECIFIED_ACTIVE_CALL_REQ:
            if (sBtHfpClientInterface != NULL) {
                sBtHfpClientInterface->handle_call_action(&mConnectedDevice, BTHF_CLIENT_CALL_ACTION_CHLD_1x,
                                           pEvent->hfp_client_event.arg1);
            }
            break;
        case HFP_CLIENT_API_PRIVATE_CONSULTATION_MODE_REQ:
            if (sBtHfpClientInterface != NULL) {
                sBtHfpClientInterface->handle_call_action(&mConnectedDevice, BTHF_CLIENT_CALL_ACTION_CHLD_2x,
                                           pEvent->hfp_client_event.arg1);
            }
            break;
        case HFP_CLIENT_API_PUT_INCOMING_CALL_ON_HOLD_REQ:
            if (sBtHfpClientInterface != NULL) {
                sBtHfpClientInterface->handle_call_action(&mConnectedDevice, BTHF_CLIENT_CALL_ACTION_BTRH_0, 0);
            }
            break;
        case HFP_CLIENT_API_ACCEPT_HELD_INCOMING_CALL_REQ:
            if (sBtHfpClientInterface != NULL) {
                sBtHfpClientInterface->handle_call_action(&mConnectedDevice, BTHF_CLIENT_CALL_ACTION_BTRH_1, 0);
            }
            break;
        case HFP_CLIENT_API_REJECT_HELD_INCOMING_CALL_REQ:
            if (sBtHfpClientInterface != NULL) {
                sBtHfpClientInterface->handle_call_action(&mConnectedDevice, BTHF_CLIENT_CALL_ACTION_BTRH_2, 0);
            }
            break;
        case HFP_CLIENT_API_DIAL_REQ:
            if (sBtHfpClientInterface != NULL) {
                sBtHfpClientInterface->dial(&mConnectedDevice, pEvent->hfp_client_event.str);
            }
            break;
        case HFP_CLIENT_API_REDIAL_REQ:
            if (sBtHfpClientInterface != NULL) {
                sBtHfpClientInterface->dial(&mConnectedDevice, NULL);
            }
            break;
        case HFP_CLIENT_API_DIAL_MEMORY_REQ:
            if (sBtHfpClientInterface != NULL) {
                sBtHfpClientInterface->dial_memory(&mConnectedDevice, pEvent->hfp_client_event.arg1);
            }
            break;
        case HFP_CLIENT_API_START_VR_REQ:
            if (sBtHfpClientInterface != NULL) {
                sBtHfpClientInterface->start_voice_recognition(&mConnectedDevice);
            }
            break;
        case HFP_CLIENT_API_STOP_VR_REQ:
            if (sBtHfpClientInterface != NULL) {
                sBtHfpClientInterface->stop_voice_recognition(&mConnectedDevice);
            }
            break;
        case HFP_CLIENT_API_CALL_ACTION_REQ:
            if (sBtHfpClientInterface != NULL) {
                sBtHfpClientInterface->handle_call_action(&mConnectedDevice, (bthf_client_call_action_t)(pEvent->hfp_client_event.arg1),
                                         pEvent->hfp_client_event.arg2);
            }
            break;
        case HFP_CLIENT_API_QUERY_CURRENT_CALLS_REQ:
            if (sBtHfpClientInterface != NULL) {
                sBtHfpClientInterface->query_current_calls(&mConnectedDevice);
            }
            break;
        case HFP_CLIENT_API_QUERY_OPERATOR_NAME_REQ:
            if (sBtHfpClientInterface != NULL) {
                sBtHfpClientInterface->query_current_operator_name(&mConnectedDevice);
            }
            break;
        case HFP_CLIENT_API_QUERY_SUBSCRIBER_INFO_REQ:
            if (sBtHfpClientInterface != NULL) {
                sBtHfpClientInterface->retrieve_subscriber_info(&mConnectedDevice);
            }
            break;
        case HFP_CLIENT_API_SCO_VOL_CTRL_REQ:
            if (sBtHfpClientInterface != NULL) {
                sBtHfpClientInterface->volume_control(&mConnectedDevice, (bthf_client_volume_type_t)(pEvent->hfp_client_event.arg1),
                                         pEvent->hfp_client_event.arg2);
            }
            break;
        case HFP_CLIENT_API_SPK_VOL_CTRL_REQ:
            if (sBtHfpClientInterface != NULL && pEvent->hfp_client_event.arg2 == true) {
                sBtHfpClientInterface->volume_control(&mConnectedDevice, BTHF_CLIENT_VOLUME_TYPE_SPK,
                                          pEvent->hfp_client_event.arg1);
            }
            break;
        case HFP_CLIENT_API_MIC_VOL_CTRL_REQ:
            if (sBtHfpClientInterface != NULL) {
                sBtHfpClientInterface->volume_control(&mConnectedDevice, BTHF_CLIENT_VOLUME_TYPE_MIC,
                                          pEvent->hfp_client_event.arg1);
            }
            break;
        case HFP_CLIENT_API_SEND_DTMF_REQ:
            if (sBtHfpClientInterface != NULL) {
                int dtmf_code = pEvent->hfp_client_event.str[0];
                sBtHfpClientInterface->send_dtmf(&mConnectedDevice, dtmf_code);
            }
            break;
        case HFP_CLIENT_API_DISABLE_NREC_ON_AG_REQ:
            if (sBtHfpClientInterface != NULL) {
                // 15 is NREC command
                sBtHfpClientInterface->send_at_cmd(&mConnectedDevice, 15, 1, 0, NULL);
            }
            break;
        default:
            ALOGD(LOGTAG," event not handled %d ", pEvent->event_id);
            break;
    }
}

void Hfp_Client::state_audio_on_handler(BtEvent* pEvent) {
    char str[18];
    BtEvent *pControlRequest, *pReleaseControlReq;
    ALOGD(LOGTAG "state_audio_on_handler Processing event %d", pEvent->event_id);
    switch(pEvent->event_id) {
        case HFP_CLIENT_API_CONNECT_REQ:
            ALOGE(LOGTAG,"Already in audio_on_handler,Ignore request");
            fprintf(stdout,"Already in audio_on_handler,Ignore request\n");
            break;
        case HFP_CLIENT_API_DISCONNECT_REQ:
            if (memcmp(&pEvent->hfp_client_event.bd_addr,&mConnectedDevice,sizeof(bt_bdaddr_t))) {
                bdaddr_to_string(&pEvent->hfp_client_event.bd_addr, str, 18);
                ALOGE(LOGTAG, "%s, Device not connected: %s", __func__,str);
                fprintf(stdout, "Device not connected: %s\n", str);
                break;
            }

            if (sBtHfpClientInterface != NULL) {
                sBtHfpClientInterface->disconnect_audio(&pEvent->hfp_client_event.bd_addr);
            }

            bdaddr_to_string(&pEvent->hfp_client_event.bd_addr, str, 18);
            fprintf(stdout, "Disconnecting SCO/eSCO with device %s\n", str);
            ALOGD(LOGTAG "Disconnecting SCO/eSCO with device %s", str);
             // release control
             pReleaseControlReq = new BtEvent;
             pReleaseControlReq->btamControlRelease.event_id = BT_AM_RELEASE_CONTROL;
             pReleaseControlReq->btamControlRelease.profile_id = PROFILE_ID_HFP_CLIENT;
             PostMessage(THREAD_ID_BT_AM, pReleaseControlReq);

             mcontrolStatus = STATUS_LOSS_TRANSIENT;
             mAudioWbs = false;
             // setting client mode
             change_mode(HFP_CLIENT_MODE_NORMAL);
             ConfigureAudio(false);
             memset(&mConnectedDevice, 0, sizeof(bt_bdaddr_t));
             memset(&mConnectingDevice, 0, sizeof(bt_bdaddr_t));
             if (sBtHfpClientInterface != NULL) {
                 sBtHfpClientInterface->disconnect(&pEvent->hfp_client_event.bd_addr);
             }

             bdaddr_to_string(&pEvent->hfp_client_event.bd_addr, str, 18);
             fprintf(stdout, "Disconnecting with device %s\n", str);
             ALOGD(LOGTAG "Disconnecting with device %s", str);
              // changing client state
             change_state(HFP_CLIENT_STATE_CONNECTING);
             break;
        case HFP_CLIENT_API_DISCONNECT_AUDIO_REQ:
            if (memcmp(&pEvent->hfp_client_event.bd_addr,&mConnectedDevice,sizeof(bt_bdaddr_t))) {
                bdaddr_to_string(&pEvent->hfp_client_event.bd_addr, str, 18);
                ALOGE(LOGTAG, "%s, Device not connected: %s", __func__,str);
                fprintf(stdout, "Device not connected: %s\n", str);
                break;
            }
            if (sBtHfpClientInterface != NULL) {
                sBtHfpClientInterface->disconnect_audio(&pEvent->hfp_client_event.bd_addr);
            }

            bdaddr_to_string(&pEvent->hfp_client_event.bd_addr, str, 18);
            fprintf(stdout, "Disconnecting SCO/eSCO with device %s\n", str);
            ALOGD(LOGTAG "Disconnecting SCO/eSCO with device %s", str);
            break;
        case HFP_CLIENT_AUDIO_STATE_DISCONNECTED_CB:

            mAudioWbs = false;

            // release control
            pReleaseControlReq = new BtEvent;
            pReleaseControlReq->btamControlRelease.event_id = BT_AM_RELEASE_CONTROL;
            pReleaseControlReq->btamControlRelease.profile_id = PROFILE_ID_HFP_CLIENT;
            PostMessage(THREAD_ID_BT_AM, pReleaseControlReq);

            mcontrolStatus = STATUS_LOSS_TRANSIENT;
            // changing client mode
            change_mode(HFP_CLIENT_MODE_NORMAL);
            change_state(HFP_CLIENT_STATE_CONNECTED);

            bdaddr_to_string(&pEvent->hfp_client_event.bd_addr, str, 18);
            ConfigureAudio(false);
            fprintf(stdout, "Disconnected SCO connection with device %s\n", str);
            ALOGD(LOGTAG "Disconnected SCO connection with device %s", str);
            break;
        case BT_AM_CONTROL_STATUS:
            ALOGD(LOGTAG "earlier status = %d  new status = %d", mcontrolStatus,
                                   pEvent->btamControlStatus.status_type);
            fprintf(stdout, "earlier status = %d  new status = %d\n", mcontrolStatus,
                                   pEvent->btamControlStatus.status_type);
            mcontrolStatus = pEvent->btamControlStatus.status_type;

            switch(mcontrolStatus) {
                 case STATUS_LOSS:
                    // this should not occur.
                    break;
                 case STATUS_LOSS_TRANSIENT:
                    // this should not occur.
                    break;
                 case STATUS_GAIN:
                    // this should not occur.
                    break;
                 case STATUS_GAIN_TRANSIENT:
                    ALOGD(LOGTAG " Configure audio for SCO");
                    fprintf(stdout, "Configure audio for SCO\n");
                    ConfigureAudio(true);
                    break;
            }
            break;
        case HFP_CLIENT_API_ACCEPT_CALL_REQ:
            if (sBtHfpClientInterface != NULL) {
                sBtHfpClientInterface->handle_call_action(&mConnectedDevice, BTHF_CLIENT_CALL_ACTION_ATA, 0);
            }
            break;
        case HFP_CLIENT_API_RELEASE_HELD_CALL_REQ:
            if (sBtHfpClientInterface != NULL) {
                sBtHfpClientInterface->handle_call_action(&mConnectedDevice, BTHF_CLIENT_CALL_ACTION_CHLD_0, 0);
            }
            break;
        case HFP_CLIENT_API_REJECT_CALL_REQ:
                // intentional fall through. TODO: cross check
        case HFP_CLIENT_API_END_CALL_REQ:
            if (sBtHfpClientInterface != NULL) {
                sBtHfpClientInterface->handle_call_action(&mConnectedDevice, BTHF_CLIENT_CALL_ACTION_CHUP, 0);
            }
            break;
        case HFP_CLIENT_API_RELEASE_ACTIVE_ACCEPT_WAITING_OR_HELD_CALL_REQ:
            if (sBtHfpClientInterface != NULL) {
                sBtHfpClientInterface->handle_call_action(&mConnectedDevice, BTHF_CLIENT_CALL_ACTION_CHLD_1, 0);
            }
            break;
        case HFP_CLIENT_API_HOLD_CALL_REQ:
            // intentional fall through
        case HFP_CLIENT_API_SWAP_CALLS_REQ:
            if (sBtHfpClientInterface != NULL) {
                sBtHfpClientInterface->handle_call_action(&mConnectedDevice, BTHF_CLIENT_CALL_ACTION_CHLD_2, 0);
            }
            break;
        case HFP_CLIENT_API_ADD_HELD_CALL_TO_CONF_REQ:
            if (sBtHfpClientInterface != NULL) {
                sBtHfpClientInterface->handle_call_action(&mConnectedDevice, BTHF_CLIENT_CALL_ACTION_CHLD_3, 0);
            }
            break;
        case HFP_CLIENT_API_RELEASE_SPECIFIED_ACTIVE_CALL_REQ:
            if (sBtHfpClientInterface != NULL) {
                sBtHfpClientInterface->handle_call_action(&mConnectedDevice, BTHF_CLIENT_CALL_ACTION_CHLD_1x,
                                           pEvent->hfp_client_event.arg1);
            }
            break;
        case HFP_CLIENT_API_PRIVATE_CONSULTATION_MODE_REQ:
            if (sBtHfpClientInterface != NULL) {
                sBtHfpClientInterface->handle_call_action(&mConnectedDevice, BTHF_CLIENT_CALL_ACTION_CHLD_2x,
                                           pEvent->hfp_client_event.arg1);
            }
            break;
        case HFP_CLIENT_API_PUT_INCOMING_CALL_ON_HOLD_REQ:
            if (sBtHfpClientInterface != NULL) {
                sBtHfpClientInterface->handle_call_action(&mConnectedDevice, BTHF_CLIENT_CALL_ACTION_BTRH_0, 0);
            }
            break;
        case HFP_CLIENT_API_ACCEPT_HELD_INCOMING_CALL_REQ:
            if (sBtHfpClientInterface != NULL) {
                sBtHfpClientInterface->handle_call_action(&mConnectedDevice, BTHF_CLIENT_CALL_ACTION_BTRH_1, 0);
            }
            break;
        case HFP_CLIENT_API_REJECT_HELD_INCOMING_CALL_REQ:
            if (sBtHfpClientInterface != NULL) {
                sBtHfpClientInterface->handle_call_action(&mConnectedDevice, BTHF_CLIENT_CALL_ACTION_BTRH_2, 0);
            }
            break;
        case HFP_CLIENT_API_DIAL_REQ:
            if (sBtHfpClientInterface != NULL) {
                sBtHfpClientInterface->dial(&mConnectedDevice, pEvent->hfp_client_event.str);
            }
            break;
        case HFP_CLIENT_API_REDIAL_REQ:
            if (sBtHfpClientInterface != NULL) {
                sBtHfpClientInterface->dial(&mConnectedDevice, NULL);
            }
            break;
        case HFP_CLIENT_API_DIAL_MEMORY_REQ:
            if (sBtHfpClientInterface != NULL) {
                sBtHfpClientInterface->dial_memory(&mConnectedDevice, pEvent->hfp_client_event.arg1);
            }
            break;
        case HFP_CLIENT_API_START_VR_REQ:
            if (sBtHfpClientInterface != NULL) {
                sBtHfpClientInterface->start_voice_recognition(&mConnectedDevice);
            }
            break;
        case HFP_CLIENT_API_STOP_VR_REQ:
            if (sBtHfpClientInterface != NULL) {
                sBtHfpClientInterface->stop_voice_recognition(&mConnectedDevice);
            }
            break;
        case HFP_CLIENT_API_CALL_ACTION_REQ:
            if (sBtHfpClientInterface != NULL) {
                sBtHfpClientInterface->handle_call_action(&mConnectedDevice, (bthf_client_call_action_t)(pEvent->hfp_client_event.arg1),
                                         pEvent->hfp_client_event.arg2);
            }
            break;
        case HFP_CLIENT_API_QUERY_CURRENT_CALLS_REQ:
            if (sBtHfpClientInterface != NULL) {
                sBtHfpClientInterface->query_current_calls(&mConnectedDevice);
            }
            break;
        case HFP_CLIENT_API_QUERY_OPERATOR_NAME_REQ:
            if (sBtHfpClientInterface != NULL) {
                sBtHfpClientInterface->query_current_operator_name(&mConnectedDevice);
            }
            break;
        case HFP_CLIENT_API_QUERY_SUBSCRIBER_INFO_REQ:
            if (sBtHfpClientInterface != NULL) {
                sBtHfpClientInterface->retrieve_subscriber_info(&mConnectedDevice);
            }
            break;
        case HFP_CLIENT_API_SCO_VOL_CTRL_REQ:
            if (sBtHfpClientInterface != NULL) {
                sBtHfpClientInterface->volume_control(&mConnectedDevice, (bthf_client_volume_type_t)(pEvent->hfp_client_event.arg1),
                                         pEvent->hfp_client_event.arg2);
            }
            break;
        case HFP_CLIENT_API_SPK_VOL_CTRL_REQ:
            if (sBtHfpClientInterface != NULL) {
                ConfigureVolume(BTHF_CLIENT_VOLUME_TYPE_SPK, pEvent->hfp_client_event.arg1, false);
                /* AT+VGS will be sent to remote only while processing API */
                if (pEvent->hfp_client_event.arg2 == true) {
                    sBtHfpClientInterface->volume_control(&mConnectedDevice, BTHF_CLIENT_VOLUME_TYPE_SPK,
                                    pEvent->hfp_client_event.arg1);
                }
            }
            break;
        case HFP_CLIENT_API_MIC_VOL_CTRL_REQ:
            if (sBtHfpClientInterface != NULL) {
                if (pEvent->hfp_client_event.arg1 == 0)
                    ConfigureVolume(BTHF_CLIENT_VOLUME_TYPE_MIC, pEvent->hfp_client_event.arg1, true);
                else
                    ConfigureVolume(BTHF_CLIENT_VOLUME_TYPE_MIC, pEvent->hfp_client_event.arg1, false);

                sBtHfpClientInterface->volume_control(&mConnectedDevice, BTHF_CLIENT_VOLUME_TYPE_MIC,
                                          pEvent->hfp_client_event.arg1);
            }
            break;
        case HFP_CLIENT_API_SEND_DTMF_REQ:
            if (sBtHfpClientInterface != NULL) {
                int dtmf_code = pEvent->hfp_client_event.str[0];
                sBtHfpClientInterface->send_dtmf(&mConnectedDevice, dtmf_code);
            }
            break;
        case HFP_CLIENT_API_DISABLE_NREC_ON_AG_REQ:
            if (sBtHfpClientInterface != NULL) {
                // 15 is NREC command
                sBtHfpClientInterface->send_at_cmd(&mConnectedDevice, 15, 1, 0, NULL);
            }
            break;

        default:
            ALOGD(LOGTAG," event not handled %d ", pEvent->event_id);
            break;
    }

}

void Hfp_Client::ConfigureRingTonePlayback() {

#ifdef BT_AUDIO_PAL_INTEGRATION
   int ret = 0;
   ALOGD(LOGTAG " %s: Initializing pulseaudio sink for ringtone playback", __func__);
   ret = pa_routing_intf->pa_sink_init_fn(PA_SINK_LOW_LATENCY, 16, 8000, 1, PA_BT_PCM_FORMAT);
   if (ret)
       ALOGE(LOGTAG " %s: Failed to initialize pulseaudio sink for ringtone playback\n", __func__);
#endif
}

// plays 1 sec tone
void Hfp_Client::PlayRingTone() {
   ALOGD("%s:", __func__);
#ifdef BT_AUDIO_PAL_INTEGRATION
  bool ret = false;
  char *buffer = (char*) osi_calloc(sizeof(ring_tone));
  if (!buffer) {
      fprintf(stdout, "memory allocation for playing ringtone failed\n");
      ALOGE("%s: memory allocation for playing ringtone failed", __func__);
      return;
  }

  memcpy(buffer, ring_tone, sizeof(ring_tone));
  ret = pa_routing_intf->pa_sink_play_fn(buffer, sizeof(ring_tone));
  if (!ret)
      ALOGE(LOGTAG " %s: Failed to play ringtone on PA sink", __func__);

  osi_free(buffer);
#endif
}

void Hfp_Client::StopRingTone() {
#ifdef BT_AUDIO_PAL_INTEGRATION
    ALOGD(LOGTAG " %s:Deiniting the sink for ringtone playback", __func__);
    pa_routing_intf->pa_sink_deinit_fn();
#endif
}

void Hfp_Client::ConfigureAudio(bool enable) {
#ifdef BT_AUDIO_PAL_INTEGRATION
   if (pa_routing_intf) {
       int ret = -1;
       if (enable && !pa_hfp_is_connected) {
           ret = pa_routing_intf->pa_bt_connect_fn(PA_BT_HFP_CLIENT, true);
           if(!ret) {
               fprintf(stdout, "BT connect is success for SCO usecase\n");
               ALOGD(LOGTAG " BT connect is success for SCO usecase");
               pa_hfp_is_connected = true;
           }
           else {
               fprintf(stdout, "BT connect failed for SCO usecase !!\n");
               ALOGE(LOGTAG " BT connect failed for SCO usecase !!\n");
               return;
           }

           ret = pa_routing_intf->pa_bt_set_param_fn(PA_BT_HFP_CLIENT, "hfp_volume=10");
           if(!ret)
               ALOGD(LOGTAG " HFP set volume success\n");

           if (mAudioWbs)
               ret = pa_routing_intf->pa_bt_set_param_fn(PA_BT_HFP_CLIENT, "hfp_sample_rate=16000");
           else
               ret = pa_routing_intf->pa_bt_set_param_fn(PA_BT_HFP_CLIENT, "hfp_sample_rate=8000");

           if(!ret)
               ALOGD(LOGTAG " set param- hfp_set_sampling_rate success\n");

           ret = pa_routing_intf->pa_bt_set_param_fn(PA_BT_HFP_CLIENT, "hfp_enable=true");
           if(!ret) {
               fprintf(stdout, "set param- hfp_enable=true success\n");
               ALOGD(LOGTAG " HFP enabled successfully\n");
           }
           else {
               fprintf(stdout, "set param hfp_enable=true failed!!!\n");
               ALOGE(LOGTAG " set param hfp_enable=true failed!!!\n");
           }
       }
       else if (!enable && pa_hfp_is_connected) {
           ret = pa_routing_intf->pa_bt_set_param_fn(PA_BT_HFP_CLIENT, "hfp_enable=false");
           if(!ret) {
               fprintf(stdout, "set param hfp_enable=false success\n");
               ALOGD(LOGTAG " set param hfp_enable=false success\n");
           }
           else {
               fprintf(stdout, "set param hfp_enable=false failed!!!\n");
               ALOGE(LOGTAG " set param hfp_enable=false failed!!!\n");
           }

           ret = pa_routing_intf->pa_bt_connect_fn(PA_BT_HFP_CLIENT, false);
           if(!ret) {
               fprintf(stdout, "BT disconnect is success for SCO usecase\n");
               ALOGD(LOGTAG " BT disconnect is success for SCO usecase\n");
               pa_hfp_is_connected = false;
           }
       }
   }
#endif
}

void Hfp_Client::ConfigureVolume(bthf_client_volume_type_t vol_type, int vol, bool mute_mic) {
#if defined(BT_AUDIO_PAL_INTEGRATION)
   int ret;
   char vol_cmd[25];
   char *vol_str = NULL;

   if (vol_type == BTHF_CLIENT_VOLUME_TYPE_SPK) {
       int cmd_len = 0;
       vol_str = "hfp_volume=";
       snprintf(vol_cmd, strlen(vol_str) + 3, "%s%d", vol_str, vol);
   }
   else if (vol_type == BTHF_CLIENT_VOLUME_TYPE_MIC) {
       if (mute_mic) {
           vol_str = "hfp_mic_mute=true";
       }
       else {
           vol_str = "hfp_mic_mute=false";
       }
       strlcpy(vol_cmd, vol_str, strlen(vol_str) + 1);
   }
   else {
       fprintf(stdout, "Unsupported volume type!!\n");
       return;
   }

   ret = pa_routing_intf->pa_bt_set_param_fn(PA_BT_HFP_CLIENT, vol_cmd);
   if(!ret) {
       ALOGD(LOGTAG " set %s success\n", vol_cmd);
       fprintf(stdout, "set %s success\n", vol_cmd);
   }
   else {
       ALOGD(LOGTAG " HFP set mute failed !!\n");
       fprintf(stdout, "HFP set mute failed !!\n");
   }

#endif // BT_AUDIO_PAL_INTEGRATION
}

void Hfp_Client::change_state(HfpClientState mState) {
   ALOGD(LOGTAG " current State = %d, new state = %d", mClientState, mState);
   fprintf(stdout, " current State = %d, new state = %d\n", mClientState, mState);
   pthread_mutex_lock(&lock);
   mClientState = mState;
   ALOGD(LOGTAG " state changes to %d ", mState);
   pthread_mutex_unlock(&lock);
}

void Hfp_Client::change_mode(HfpClientMode mode) {
   ALOGD(LOGTAG " current mode = %d, new mode = %d", mAudioMode, mode);
   fprintf(stdout, " current mode = %d, new mode = %d\n", mAudioMode, mode);
   pthread_mutex_lock(&lock);
   mAudioMode = mode;
   ALOGD(LOGTAG " mode changes to %d ", mAudioMode);
   pthread_mutex_unlock(&lock);
}

Hfp_Client :: Hfp_Client(const bt_interface_t *bt_interface, config_t *config) {
    this->bluetooth_interface = bt_interface;
    sBtHfpClientInterface = NULL;
    mClientState = HFP_CLIENT_STATE_NOT_STARTED;
    mAudioMode = HFP_CLIENT_MODE_NORMAL;
    mcontrolStatus = STATUS_LOSS_TRANSIENT;
    mAudioWbs = false;
    peer_feat = 0;
    chld_feat = 0;
#if defined(BT_AUDIO_PAL_INTEGRATION)
    pa_routing_intf = pa_routing_intf_open();
    if (!pa_routing_intf) {
        ALOGE(LOGTAG " pa_routing_intf_open failed !!");
    }
    else {
        ALOGD(LOGTAG " pa_routing_intf_open success !!");
        pa_hfp_is_connected = false;
    }
#endif
    pthread_mutex_init(&this->lock, NULL);
}

Hfp_Client :: ~Hfp_Client() {
    mcontrolStatus = STATUS_LOSS_TRANSIENT;
#if defined(BT_AUDIO_PAL_INTEGRATION)
    if (pa_routing_intf) {
        pa_routing_intf_close(pa_routing_intf);
        pa_hfp_is_connected = false;
    }
    pa_routing_intf = NULL;
#endif
    pthread_mutex_destroy(&lock);
}
