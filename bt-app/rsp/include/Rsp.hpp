/*
 * Copyright (c) 2016, The Linux Foundation. All rights reserved.
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

#ifndef RSP_APP_H
#define RSP_APP_H

#pragma once

#include <hardware/bluetooth.h>
#include <stdio.h>
#include <vector>


#include "osi/include/log.h"
#include "osi/include/thread.h"
#include "osi/include/config.h"
#include "osi/include/allocator.h"
#include "ipc.hpp"
#include "GattLibService.hpp"
#include "GattServer.hpp"
#include "GattServerCallback.hpp"
#include "uuid.h"
#include "AdvertiseSettings.hpp"
#include "AdvertiseData.hpp"
#include "AdvertisingSetCallback.hpp"
#include "AdvertisingSet.hpp"
#include "AdvertisingSetParameters.hpp"
#include "PeriodicAdvertiseParameters.hpp"
#include "GattLeAdvertiser.hpp"



#define RSP_MIN_CI           (100)
#define RSP_MAX_CI           (1000)

#define GATT_PROP_READ       (0x02)
#define GATT_PROP_WRITE      (0x08)

#define GATT_PERM_READ       (0x01)
#define GATT_PERM_WRITE      (0x10)

#define LOGTAG "RSP "


typedef enum
{
    WLAN_ACTIVE = 1,
    WLAN_INACTIVE,
    WLAN_TRANSACTION_PENDING,
} rsp_power_state_t;


using namespace gatt;

class Rsp {
    private:
        GattLibService *mlibservice;
        int wlan_state;

    public:
        Rsp(GattLibService*);
        ~Rsp();

        bool EnableRSP();
        inline int GetDeviceState()
        {
            fprintf(stdout, "(%s) WLAN Current State (%d) \n",__FUNCTION__, wlan_state);
            return wlan_state;
        }
        inline void SetDeviceState(int currentstate)
        {
            fprintf(stdout, "(%s) WLAN Prev State (%d) New State(%d) \n"
                   ,__FUNCTION__, wlan_state, currentstate);
            wlan_state = currentstate;
        }
        void SendResponse(string,int,int,int,uint8_t *);
        bool StartAdvertisement(void);
        void StopAdvertisement(void);
        bool AddService(void);
        bool AddCharacteristics(void);
        bool AddDescriptor(void);
        bool DeleteService(void);
        bool HandleWlanOn(void);
};
#endif

