/*
 * Copyright (c) 2018, The Linux Foundation. All rights reserved.
 * Not a Contribution.
 * Copyright (C) 2014 The Android Open Source Project
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
#ifndef SCANCALLBACK_HPP_
#define SCANCALLBACK_HPP_

#pragma once

#include "ScanResult.hpp"
#include <vector>
using namespace std;
using std::string;

namespace gatt{

/**
 * Bluetooth LE scan callbacks. Scan results are reported using these callbacks.
 */
class ScanCallback {
  public:
    /**
     * Fails to start scan as BLE scan with the same settings is already started by the app.
     */
    static const int SCAN_FAILED_ALREADY_STARTED = 1;

    /**
     * Fails to start scan as app cannot be registered.
     */
    static const int SCAN_FAILED_APPLICATION_REGISTRATION_FAILED = 2;

    /**
     * Fails to start scan due an internal error
     */
    static const int SCAN_FAILED_INTERNAL_ERROR = 3;

    /**
     * Fails to start power optimized scan as this feature is not supported.
     */
    static const int SCAN_FAILED_FEATURE_UNSUPPORTED = 4;

    /**
     * Fails to start scan as it is out of hardware resources.
     *
     * @hide
     */
    static const int SCAN_FAILED_OUT_OF_HARDWARE_RESOURCES = 5;

    /**
     * Fails to start scan as application tries to scan too frequently.
     * @hide
     */
    static const int SCAN_FAILED_SCANNING_TOO_FREQUENTLY = 6;

    static const int NO_ERROR = 0;

    /**
     * Callback when a BLE advertisement has been found.
     *
     * @param callbackType Determines how this callback was triggered. Could be one of
     * ScanSettings#CALLBACK_TYPE_ALL_MATCHES, ScanSettings#CALLBACK_TYPE_FIRST_MATCH or
     * ScanSettings#CALLBACK_TYPE_MATCH_LOST
     * @param result A Bluetooth LE scan result.
     */
    virtual void onScanResult(int callbackType, ScanResult *result){}

    /**
     * Callback when batch results are delivered.
     *
     * @param results List of scan results that are previously scanned.
     */
    virtual void onBatchScanResults(std::vector<ScanResult*> results){}

    /**
     * Callback when scan could not be started.
     *
     * @param errorCode Error code (one of SCAN_FAILED_*) for scan failure.
     */
    virtual void onScanFailed(int errorCode){}
};

}//namespace gatt
#endif
