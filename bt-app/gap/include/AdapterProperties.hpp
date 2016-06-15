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

/**
 * @file AdapterProperties.hpp
 * @brief Adapter Properties header file
 */

/**
 * Bluetooth Address length
*/
#define BD_ADDR_LEN      6

/**
 * @class AdapterProperties
 *
 * @brief AdapterProperties Class
 */
class AdapterProperties {

    private:
        /**
         *  object for @ref AdapterState class
         */
        AdapterState state_;
        bool discovering_;
        /**
        *  class object for @ref RemoteDevices class
        */
        RemoteDevices  *remote_devices_obj_;
        /**
         * Used to for syncronization accces
         */
        pthread_mutex_t lock_;
        /**
         *  structure object for standard Bluetooth DM interface
         */
        const bt_interface_t *bluetooth_interface_;
        void GetBondedDevicesFromPropertyList(int num_properties,
            bt_property_t *properties, bt_property_type_t type, bt_bdaddr_t *bd_addr,
            int *num_bonded_devices);

    public:
        std::list <std::string> bonded_devices;
        /**
         * @brief @ref AdapterProperties constructor
         *
         * It will initialize local @ref bluetooth_interface_ and
         * @ref remote_devices_obj_
         * @param[in] bt_interface_t bt_interface
         * @param[in] RemoteDevices remote_devices_obj
         */
        AdapterProperties(const bt_interface_t *bt_interface,
                RemoteDevices *remote_devices_obj);

        /**
         * @ref distructor for @ref AdapterProperties
         *
         * Clear the locks, remove device entries from @ref bonded_devices
         */
        ~AdapterProperties();
        /**
         * @brief FlushBondedDeviceList
         *
         *  Function will clears the paring list in @ref bonded_devices
         *
         * @param  void
         * @return none
         */
        void FlushBondedDeviceList(void);

        /**
         * @brief AdapterPropertiesUpdate
         *
         *
         *
         * @param  AdapterPropertiesEvent event
         * @return none
         */
        void AdapterPropertiesUpdate(AdapterPropertiesEvent *event);

        /**
         * @brief HandleDiscoveryStateChange
         *
         *
         *
         * @param  bt_discovery_state_t
         * @return none
         */
        void HandleDiscoveryStateChange(bt_discovery_state_t state);

        /**
         * @brief OnbondStateChanged
         *
         *
         *
         * @param bt_bdaddr_t   bd_addr
         * @param bt_bond_state_t   new_state
         * @param bool  notify
         * @return none
         */
        void OnbondStateChanged( bt_bdaddr_t bd_addr, bt_bond_state_t new_state,
            bool notify);

        /**
         * @brief SetState
         *
         * It will set the AdapterState
         *
         * @param AdapterState
         * @return none
         */
        void SetState(AdapterState state);

        /**
         * @brief GetState
         *
         * This function will returns the current BT state
         *
         * @return int
         */
        int  GetState();

        /**
         * @brief IsDiscovering
         *
         * It will check discovery state, If discovery is in progress then returns true
         * else returns false
         *
         * @return bool
         */
        bool IsDiscovering();
};

#endif
