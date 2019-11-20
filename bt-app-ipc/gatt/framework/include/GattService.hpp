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
#ifndef GATT_SERVICE_HPP_
#define GATT_SERVICE_HPP_
#pragma once

#include "GattCharacteristic.hpp"
#include "utils/include/uuid.h"
#include<vector>

using namespace std;
using std::string;
using btapp::Uuid;
namespace gatt{

class GattDevice;

/**
 * Represents a Bluetooth GATT Service
 *
 * <p> Gatt Service contains a collection of GattCharacteristic,
 * as well as referenced services.
 */
class GattService {

  private:
    /**
     * Whether the service uuid should be advertised.
     */
    bool mAdvertisePreferred;

  protected:
    /**
     * The remote device his service is associated with.
     * This applies to client applications only.
     *
     * @hide
     */
    string mDevice;

    /**
     * The UUID of this service.
     *
     * @hide
     */
    Uuid mUuid;

    /**
     * Instance ID for this service.
     *
     * @hide
     */
    int mInstanceId;

    /**
     * Handle counter override (for conformance testing).
     *
     * @hide
     */
    int mHandles = 0;

    /**
     * Service type (Primary/Secondary).
     *
     * @hide
     */
    int mServiceType;

    /**
     * List of characteristics included in this service.
     */
    std::vector<GattCharacteristic*> mCharacteristics;

    /**
     * List of included services for this service.
     */
    std::vector<GattService*> mIncludedServices;

  public:
    /**
     * Primary service
     */
    static const int SERVICE_TYPE_PRIMARY = 0;

    /**
     * Secondary service (included by primary services)
     */
    static const int SERVICE_TYPE_SECONDARY = 1;

    /**
     * Create a new GattService.
     *
     * @param uuid The UUID for this service
     * @param serviceType The type of this service,
     * SERVICE_TYPE_PRIMARY or SERVICE_TYPE_SECONDARY
     */
    GattService(Uuid uuid, int serviceType);
    GattService(Uuid uuid, int instanceId, int serviceType);
    ~GattService();

    /**
     * Add an included service to this service.
     *
     * @param service The service to be added
     * @return true, if the included service was added to the service
     */
    bool addService(GattService *service);

    /**
     * Add a characteristic to this service.
     *
     * @param characteristic The characteristics to be added
     * @return true, if the characteristic was added to the service
     */
    bool addCharacteristic(GattCharacteristic *characteristic);

    /**
     * Get characteristic by UUID and instanceId.
     *
     */
    GattCharacteristic* getCharacteristic(Uuid uuid, int instanceId);

    /**
     * Returns the UUID of this service
     *
     * @return UUID of this service
     */
    Uuid getUuid();

    /**
     * Returns the instance ID for this service
     *
     * If a remote device offers multiple services with the same UUID
     * (ex. multiple battery services for different batteries), the instance
     * ID is used to distuinguish services.
     *
     * @return Instance ID of this service
     */
    int getInstanceId();

    /**
     * Get the type of this service (primary/secondary)
     */
    int getType();

    /**
     * Get the list of included GATT services for this service.
     *
     * @return List of included services or empty list if no included services were discovered.
     */
    std::vector<GattService*> getIncludedServices();

    /**
     * Returns a list of characteristics included in this service.
     *
     * @return Characteristics included in this service
     */
    std::vector<GattCharacteristic*> getCharacteristics();

    /**
     * Returns a characteristic with a given UUID out of the list of
     * characteristics offered by this service.
     *
     * This is a convenience function to allow access to a given characteristic
     * without enumerating over the list returned by getCharacteristics
     * manually.
     *
     * If a remote service offers multiple characteristics with the same
     * UUID, the first instance of a characteristic with the given UUID
     * is returned.
     *
     * @return GATT characteristic object or null if no characteristic with the given UUID was
     * found.
     */
    GattCharacteristic* getCharacteristic(Uuid uuid);

     /**
     * Force the instance ID.
     *
     */
    void setInstanceId(int instanceId);

    /**
     * Add an included service to the internal map.
     *
     */
    void addIncludedService(GattService *includedService);

    /**
     * Returns the device associated with this service.
     */
    string getDevice();

    /**
     * Returns the device associated with this service.
     */
    void setDevice(string device);

    /**
     * Returns whether the uuid of the service should be advertised.
     */
    bool isAdvertisePreferred();

    /**
     * Set whether the service uuid should be advertised.
     */
    void setAdvertisePreferred(bool advertisePreferred);

};
}
#endif