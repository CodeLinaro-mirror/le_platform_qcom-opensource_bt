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
#ifndef GATTDESCRIPTOR_HPP_
#define GATTDESCRIPTOR_HPP_

#pragma once

#include <cstdint>
#include <iostream>
#include "GattDevice.hpp"
#include "utils/include/uuid.h"

using namespace std;
using std::string;
using btapp::Uuid;
namespace gatt{
class GattCharacteristic;
/**
 * Represents a Bluetooth GATT Descriptor
 *
 * GATT Descriptors contain additional information and attributes of a GATT
 * characteristic. They can be used to describe
 * the characteristic's features or to control certain behaviours of the characteristic.
 */
class GattDescriptor {
  private:
    /**
    * @hide
    */
    void initDescriptor(GattCharacteristic *characteristic, Uuid uuid,
            int instance, int permissions);

  protected:
    /**
     * The Uuid of this descriptor.
     */
    Uuid mUuid;

    /**
     * Instance ID for this descriptor.
     */
    int mInstance;

    /**
     * Permissions for this descriptor
     */
    int mPermissions;

    /**
     * Back-reference to the characteristic this descriptor belongs to.
     */
    GattCharacteristic* mCharacteristic = NULL;

    /**
     * The value for this descriptor.
     */
    uint8_t *mValue = NULL;

  public:
    /**
     * Value used to enable notification for a client configuration descriptor
     */
    static const uint8_t ENABLE_NOTIFICATION_VALUE[];

    /**
     * Value used to enable indication for a client configuration descriptor
     */
    static const uint8_t ENABLE_INDICATION_VALUE[];

    /**
     * Value used to disable notifications or indicatinos
     */
    static const uint8_t DISABLE_NOTIFICATION_VALUE[];

    /**
     * Descriptor read permission
     */
    static const int PERMISSION_READ = 0x01;

    /**
     * Descriptor permission: Allow encrypted read operations
     */
    static const int PERMISSION_READ_ENCRYPTED = 0x02;

    /**
     * Descriptor permission: Allow reading with man-in-the-middle protection
     */
    static const int PERMISSION_READ_ENCRYPTED_MITM = 0x04;

    /**
     * Descriptor write permission
     */
    static const int PERMISSION_WRITE = 0x10;

    /**
     * Descriptor permission: Allow encrypted writes
     */
    static const int PERMISSION_WRITE_ENCRYPTED = 0x20;

    /**
     * Descriptor permission: Allow encrypted writes with man-in-the-middle
     * protection
     */
     static const int PERMISSION_WRITE_ENCRYPTED_MITM = 0x40;

    /**
     * Descriptor permission: Allow signed write operations
     */
    static const int PERMISSION_WRITE_SIGNED = 0x80;

    /**
     * Descriptor permission: Allow signed write operations with
     * man-in-the-middle protection
     */
    static const int PERMISSION_WRITE_SIGNED_MITM = 0x100;

    /**
     * Create a new GattDescriptor.
     *
     * @param uuid The Uuid for this descriptor
     * @param permissions Permissions for this descriptor
     */
    GattDescriptor(Uuid uuid, int permissions);

    /**
    * @hide
    */
    GattDescriptor(Uuid uuid, int instance, int permissions);

    /**
    * Destructor
    */
    ~GattDescriptor();

    /**
     * Returns the characteristic this descriptor belongs to.
     *
     * @return The characteristic.
     */
    GattCharacteristic* getCharacteristic();

    /**
     * Set the back-reference to the associated characteristic
     */
    void setCharacteristic(GattCharacteristic *characteristic);

    /**
     * Returns the Uuid of this descriptor.
     *
     * @return Uuid of this descriptor
     */
    Uuid getUuid();

    /**
     * Returns the permissions for this descriptor.
     *
     * @return Permissions of this descriptor
     */
    int getPermissions();

    /**
     * Returns the stored value for this descriptor
     *
     * This function returns the stored value for this descriptor.
     * The cached value of the descriptor is updated as a result
     * of a descriptor read operation.
     *
     * @return Cached value of the descriptor
     */
    uint8_t* getValue();

    /**
     * Updates the locally stored value of this descriptor.
     *
     * This function modifies the locally stored cached value of this
     * descriptor.
     *
     * @param value New value for this descriptor
     * @return true if the locally stored value has been set, false if the requested value could not
     * be stored locally.
     */
    bool setValue(uint8_t *value);

    /**
    * Returns the instance ID for this descriptor.
    *
    * <p>If a remote device offers multiple descriptors with the same UUID,
    * the instance ID is used to distuinguish between descriptors.
    *
    * @return Instance ID of this descriptor
    * @hide
    */
    int getInstanceId();

    /**
    * Force the instance ID.
    *
    * @hide
    */
    void setInstanceId(int instanceId);
};
}//namespace gatt
#endif
