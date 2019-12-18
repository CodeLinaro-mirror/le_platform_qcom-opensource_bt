/*
 * Copyright (c) 2018, The Linux Foundation. All rights reserved.
 * Not a Contribution.
 * Copyright (C) 2013 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License"){}
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

#ifndef GATTCHARACTERISTIC_HPP_
#define GATTCHARACTERISTIC_HPP_

#pragma once

#include <list>
#include <string>
#include <vector>
#include <GattDescriptor.hpp>
#include "utils/include/uuid.h"

using namespace std;
using std::string;
using btapp::Uuid;

namespace gatt{
class GattService;
/**
* Represents a Bluetooth GATT Characteristic
*
* <p>A GATT characteristic is a basic data element used to construct a GATT service.
* The characteristic contains a value as well as
* additional information and optional GATT descriptors.
*/
class GattCharacteristic{
  private:
    static const int INVALID_INT = -1;
    static const float INVALID_FLOAT;
    void initCharacteristic(GattService *service,
                                  Uuid uuid, int instanceId,
                                  int properties, int permissions);

  protected:
    /**
     * The UUID of this characteristic.
     *
     * @hide
     */
    Uuid mUuid;

    /**
     * Instance ID for this characteristic.
     *
     * @hide
     */
    int mInstance;

    /**
     * Characteristic properties.
     *
     * @hide
     */
    int mProperties;

    /**
     * Characteristic permissions.
     *
     * @hide
     */
    int mPermissions;

    /**
     * Key size (default = 16).
     *
     * @hide
     */
    int mKeySize = 16;

    /**
     * Write type for this characteristic.
     *
     * @hide
     */
    int mWriteType;

    /**
     * Back-reference to the service this characteristic belongs to.
     *
     * @hide
     */
    GattService *mService = NULL;

    /**
     * The cached value of this characteristic.
     *
     * @hide
     */
    uint8_t *mValue = NULL;

    /**
     * List of descriptors included in this characteristic.
     */
    std::vector<GattDescriptor*> mDescriptors;

  public:
    /**
     * Characteristic proprty: Characteristic is broadcastable.
     */
    static const int PROPERTY_BROADCAST = 0x01;

    /**
     * Characteristic property: Characteristic is readable.
     */
    static const int PROPERTY_READ = 0x02;

    /**
     * Characteristic property: Characteristic can be written without response.
     */
    static const int PROPERTY_WRITE_NO_RESPONSE = 0x04;

    /**
     * Characteristic property: Characteristic can be written.
     */
    static const int PROPERTY_WRITE = 0x08;

    /**
     * Characteristic property: Characteristic supports notification
     */
    static const int PROPERTY_NOTIFY = 0x10;

    /**
     * Characteristic property: Characteristic supports indication
     */
    static const int PROPERTY_INDICATE = 0x20;

    /**
     * Characteristic property: Characteristic supports write with signature
     */
    static const int PROPERTY_SIGNED_WRITE = 0x40;

    /**
     * Characteristic property: Characteristic has extended properties
     */
    static const int PROPERTY_EXTENDED_PROPS = 0x80;

    /**
     * Characteristic read permission
     */
    static const int PERMISSION_READ = 0x01;

    /**
     * Characteristic permission: Allow encrypted read operations
     */
    static const int PERMISSION_READ_ENCRYPTED = 0x02;

    /**
     * Characteristic permission: Allow reading with man-in-the-middle protection
     */
    static const int PERMISSION_READ_ENCRYPTED_MITM = 0x04;

    /**
     * Characteristic write permission
     */
    static const int PERMISSION_WRITE = 0x10;

    /**
     * Characteristic permission: Allow encrypted writes
     */
    static const int PERMISSION_WRITE_ENCRYPTED = 0x20;

    /**
     * Characteristic permission: Allow encrypted writes with man-in-the-middle
     * protection
     */
    static const int PERMISSION_WRITE_ENCRYPTED_MITM = 0x40;

    /**
     * Characteristic permission: Allow signed write operations
     */
    static const int PERMISSION_WRITE_SIGNED = 0x80;

    /**
     * Characteristic permission: Allow signed write operations with
     * man-in-the-middle protection
     */
    static const int PERMISSION_WRITE_SIGNED_MITM = 0x100;

    /**
     * Write characteristic, requesting acknoledgement by the remote device
     */
    static const int WRITE_TYPE_DEFAULT = 0x02;

    /**
     * Write characteristic without requiring a response by the remote device
     */
    static const int WRITE_TYPE_NO_RESPONSE = 0x01;

    /**
     * Write characteristic including authentication signature
     */
    static const int WRITE_TYPE_SIGNED = 0x04;

    /**
     * Characteristic value format type uint8
     */
    static const int FORMAT_UINT8 = 0x11;

    /**
     * Characteristic value format type uint16
     */
    static const int FORMAT_UINT16 = 0x12;

    /**
     * Characteristic value format type uint32
     */
    static const int FORMAT_UINT32 = 0x14;

    /**
     * Characteristic value format type sint8
     */
    static const int FORMAT_SINT8 = 0x21;

    /**
     * Characteristic value format type sint16
     */
    static const int FORMAT_SINT16 = 0x22;

    /**
     * Characteristic value format type sint32
     */
    static const int FORMAT_SINT32 = 0x24;

    /**
     * Characteristic value format type sfloat (16-bit float)
     */
    static const int FORMAT_SFLOAT = 0x32;

    /**
     * Characteristic value format type float (32-bit float)
     */
    static const int FORMAT_FLOAT = 0x34;

    /**
     * Create a new GattCharacteristic.
     *
     * @param uuid The UUID for this characteristic
     * @param properties Properties of this characteristic
     * @param permissions Permissions for this characteristic
     */
    GattCharacteristic(Uuid uuid, int properties, int permissions);

    GattCharacteristic(Uuid uuid, int instanceId,
                            int properties, int permissions);
    /**
    * Destructor
    */
    ~GattCharacteristic();
    /**
     * Adds a descriptor to this characteristic.
     *
     * @param descriptor Descriptor to be added to this characteristic.
     * @return true, if the descriptor was added to the characteristic
     */
    bool addDescriptor(GattDescriptor *descriptor);

    /**
     * Get a descriptor by UUID and instance id.
     *
     * @hide
     */
    GattDescriptor* getDescriptor(Uuid uuid, int instanceId);

    /**
     * Returns the service this characteristic belongs to.
     *
     * @return The asscociated service
     */
    GattService* getService();

    /**
    * Sets service this characteristic belongs to.
    */
    void setService(GattService *service);

    /**
     * Returns the UUID of this characteristic
     *
     * @return UUID of this characteristic
     */
    Uuid getUuid();

    /**
     * Returns the instance ID for this characteristic.
     *
     * <p>If a remote device offers multiple characteristics with the same UUID,
     * the instance ID is used to distinguish between characteristics.
     *
     * @return Instance ID of this characteristic
     */
    int getInstanceId();

    /**
     * Force the instance ID.
     *
     * @hide
     */
    void setInstanceId(int instanceId);

    /**
     * Returns the properties of this characteristic.
     *
     * <p>The properties contain a bit mask of property flags indicating
     * the features of this characteristic.
     *
     * @return Properties of this characteristic
     */
    int getProperties();

    /**
     * Returns the permissions for this characteristic.
     *
     * @return Permissions of this characteristic
     */
    int getPermissions();

    /**
     * Gets the write type for this characteristic.
     *
     * @return Write type for this characteristic
     */
    int getWriteType();

    /**
     * Set the write type for this characteristic
     *
     * <p>Setting the write type of a characteristic determines how the
     * {GattClient#writeCharacteristic} function write this
     * characteristic.
     *
     * @param writeType The write type to for this characteristic.
     * Can be one of: {WRITE_TYPE_DEFAULT},
     * {WRITE_TYPE_NO_RESPONSE} or {WRITE_TYPE_SIGNED}.
     */
    void setWriteType(int writeType);

    /**
     * Set the desired key size.
     *
     * @hide
     */
    void setKeySize(int keySize);

    /**
    * Returns the desired key size.
    *
    * @hide
    */
    int getKeySize();

    /**
     * Returns a list of descriptors for this characteristic.
     *
     * @return Descriptors for this characteristic
     */
    std::vector<GattDescriptor*> getDescriptors();

    /**
     * Returns a descriptor with a given UUID out of the list of
     * descriptors for this characteristic.
     *
     * @return GATT descriptor object or null if no descriptor with the given UUID was found.
     */
    GattDescriptor* getDescriptor(Uuid uuid);

    /**
     * Get the stored value for this characteristic.
     *
     * <p>This function returns the stored value for this characteristic as
     * retrieved by calling {GattClient#readCharacteristic}. The cached
     * value of the characteristic is updated as a result of a read characteristic
     * operation or if a characteristic update notification has been received.
     *
     * @return Cached value of the characteristic
     */
    uint8_t* getValue();

    /**
     * Return the stored value of this characteristic.
     *
     * <p>The formatType parameter determines how the characteristic value
     * is to be interpreted. For example, settting formatType to
     * {FORMAT_UINT16} specifies that the first two bytes of the
     * characteristic value at the given offset are interpreted to generate the
     * return value.
     *
     * @param formatType The format type used to interpret the characteristic value.
     * @param offset Offset at which the integer value can be found.
     * @return Cached value of the characteristic or INVALID_INT(-1) exceeds value size.
     */
    int getIntValue(int formatType, int offset);

    /**
     * Return the stored value of this characteristic.
     *
     * @param formatType The format type used to interpret the characteristic value.
     * @param offset Offset at which the float value can be found.
     * @return Cached value of the characteristic at a given offset or INVALID_FLOAT(-1.0)
     * if the requested offset exceeds the value size.
     */
    float getFloatValue(int formatType, int offset);

    /**
     * Return the stored value of this characteristic.
     *
     * @param offset Offset at which the string value can be found.
     * @return Cached value of the characteristic
     */
    string getStringValue(int offset);

    /**
     * Updates the locally stored value of this characteristic.
     *
     * <p>This function modifies the locally stored cached value of this
     * characteristic. To send the value to the remote device, call
     * {GattClient#writeCharacteristic} to send the value to the
     * remote device.
     *
     * @param value New value for this characteristic
     * @return true if the locally stored value has been set, false if the requested value could not
     * be stored locally.
     */
    bool setValue(uint8_t *value);

    /**
     * Set the locally stored value of this characteristic.
     *
     * @param value New value for this characteristic
     * @param formatType Integer format type used to transform the value parameter
     * @param offset Offset at which the value should be placed
     * @return true if the locally stored value has been set
     */
    bool setValue(int value, int formatType, int offset);

    /**
     * Set the locally stored value of this characteristic.
     *
     * @param mantissa Mantissa for this characteristic
     * @param exponent exponent value for this characteristic
     * @param formatType Float format type used to transform the value parameter
     * @param offset Offset at which the value should be placed
     * @return true if the locally stored value has been set
     */
    bool setValue(int mantissa, int exponent, int formatType, int offset);

    /**
     * Set the locally stored value of this characteristic.
     *
     * @param value New value for this characteristic
     * @return true if the locally stored value has been set
     */
    bool setValue(string value);

  private:
    /**
     * Returns the size of a give value type.
     */
    int getTypeLen(int formatType);

    /**
     * Convert a signed byte to an unsigned int.
     */
    int unsignedByteToInt(uint8_t b);

    /**
     * Convert signed bytes to a 16-bit unsigned int.
     */
    int unsignedBytesToInt(uint8_t b0, uint8_t b1);

    /**
     * Convert signed bytes to a 32-bit unsigned int.
     */
    int unsignedBytesToInt(uint8_t b0, uint8_t b1, uint8_t b2, uint8_t b3);

    /**
     * Convert signed bytes to a 16-bit short float value.
     */
    float bytesToFloat(uint8_t b0, uint8_t b1);

    /**
     * Convert signed bytes to a 32-bit short float value.
     */
    float bytesToFloat(uint8_t b0, uint8_t b1, uint8_t b2, uint8_t b3);

    /**
     * Convert an unsigned integer value to a two's-complement encoded
     * signed value.
     */
    int unsignedToSigned(int un_signed, int size);

    /**
     * Convert an integer into the signed bits of a given length.
     */
    int intToSignedBits(int i, int size);
};
}//namespace gatt
#endif
