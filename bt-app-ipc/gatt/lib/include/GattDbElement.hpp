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

#ifndef GATTDBELEMENT_HPP_
#define GATTDBELEMENT_HPP_

#pragma once

#include "utils/include/uuid.h"

using namespace std;
using std::string;
using btapp::Uuid;
namespace gatt{

/**
 * Helper class for passing gatt db elements between java and JNI, equal to
 * native btgatt_db_element_t.
 * @hide
 */
class GattDbElement {
  public:
    static const int TYPE_PRIMARY_SERVICE = 0;
    static const int TYPE_SECONDARY_SERVICE = 1;
    static const int TYPE_INCLUDED_SERVICE = 2;
    static const int TYPE_CHARACTERISTIC = 3;
    static const int TYPE_DESCRIPTOR = 4;

    int id;
    Uuid uuid;
    int type;
    int attributeHandle;

    /*
     * If type is TYPE_PRIMARY_SERVICE, or TYPE_SECONDARY_SERVICE,
     * this contains the start and end attribute handles.
     */
    int startHandle;
    int endHandle;

    /*
     * If type is TYPE_CHARACTERISTIC, this contains the properties of
     * the characteristic.
     */
    int properties;
    int permissions;

    GattDbElement(){};
    static GattDbElement* createPrimaryService(Uuid uuid);
    static GattDbElement* createSecondaryService(Uuid uuid);
    static GattDbElement* createCharacteristic(Uuid uuid, int properties, int permissions);
    static GattDbElement* createDescriptor(Uuid uuid, int permissions);
    static GattDbElement* createIncludedService(int attributeHandle);
};
}//namespace gatt
#endif