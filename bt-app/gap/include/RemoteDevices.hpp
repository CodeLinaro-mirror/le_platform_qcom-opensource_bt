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

#ifndef REMOTE_DEV_HPP
#define REMOTE_DEV_HPP

#include <map>
#include <string>
#include <hardware/bluetooth.h>
#include "osi/include/log.h"
#include "osi/include/thread.h"
#include "ipc.h"
#include <pthread.h>

class RemoteDevices {
    private:
        const bt_interface_t *bluetooth_interface_;
        pthread_mutex_t lock_;
        bool GetValueFromPropertyList(int num_properties, bt_property_t *properties,
                     bt_property_type_t type, void* dest);
        void ClearPropertyList(int num_properties, bt_property_t *properties);

    public:
        RemoteDevices(const bt_interface_t *bt_interface);
        ~RemoteDevices();
        std::map <std::string, DeviceProperties*> remote_device_prop;
        DeviceProperties *AddDeviceProperties(bt_bdaddr_t bd_addr);
        DeviceProperties *GetDeviceProperties(bt_bdaddr_t bd_addr);

        void FlushDiscoveredDeviceList(void);
        void DeviceFound(DeviceFoundEventInt *dev_found);
        void RemoteDeviceProperties(RemotePropertiesEvent *event);
        void HandleAclStateChange(int status, bt_bdaddr_t bd_addr, int new_state);
};

#endif
