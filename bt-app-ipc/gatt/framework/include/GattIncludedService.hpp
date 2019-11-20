/*
 * Copyright (c) 2018, The Linux Foundation. All rights reserved.
 * Not a Contribution.
 * Copyright (C) 2016 The Android Open Source Project
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

#ifndef GATTINCLUDESERVICE_HPP_
#define GATTINCLUDESERVICE_HPP_
#pragma once
#include <iostream>
#include "utils/include/uuid.h"

using namespace std;
using std::string;
using btapp::Uuid;
namespace gatt{

/**
 * Represents a Bluetooth GATT Included Service
 *
 * @hide
 */
class GattIncludedService {

  protected:
    /**
     * The Uuid of this service.
     */
    Uuid mUuid;

    /**
     * Instance ID for this service.
     */
    int mInstanceId;

    /**
     * Service type (Primary/Secondary).
     */
    int mServiceType;


  public:
     /**
     * Create a new GattIncludedService
     */
    GattIncludedService(Uuid uuid, int instanceId, int serviceType);

    /**
     * Returns the Uuid of this service
     *
     * @return Uuid of this service
     */
    Uuid getUuid();

    /**
     * Returns the instance ID for this service
     *
     * <p>If a remote device offers multiple services with the same Uuid
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
};
}//namespace gatt
#endif
