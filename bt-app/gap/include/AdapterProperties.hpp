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


#ifndef ADAPTER_PROP_HPP
#define ADAPTER_PROP_HPP

#include <map>
#include <string>
#include <hardware/bluetooth.h>
#include <pthread.h>

#include "osi/include/log.h"
#include "osi/include/thread.h"
#include "ipc.h"
#include <list>
#include "RemoteDevices.hpp"

#define BD_ADDR_LEN      6

class AdapterProperties {

    private:
        AdapterState state_;
        bool discovering_;
        RemoteDevices  *remote_devices_obj_;
        pthread_mutex_t lock_;
        const bt_interface_t *bluetooth_interface_;
        void GetBondedDevicesFromPropertyList(int num_properties,
            bt_property_t *properties, bt_property_type_t type, bt_bdaddr_t *bd_addr,
            int *num_bonded_devices);

    public:
        std::list <std::string> bonded_devices;
        AdapterProperties(const bt_interface_t *bt_interface,
                RemoteDevices *remote_devices_obj);

        ~AdapterProperties();
        void FlushBondedDeviceList(void);
        void AdapterPropertiesUpdate(AdapterPropertiesEvent *event);
        void HandleDiscoveryStateChange(bt_discovery_state_t state);
        void OnbondStateChanged( bt_bdaddr_t bd_addr, bt_bond_state_t new_state);
        void SetState(AdapterState state);
        int  GetState();
        bool IsDiscovering();
};

#endif
