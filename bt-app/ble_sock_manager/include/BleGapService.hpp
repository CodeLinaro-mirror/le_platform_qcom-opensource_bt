/*
 * Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef BLE_GAP_SERVICE_H
#define BLE_GAP_SERVICE_H
#pragma once

#include "BleSockIf.hpp"
#include "GattLeScanner.hpp"

class BleGapService {
  public:
    BleGapService();
    ~BleGapService();
    Uuid service_data_uuid_;
    string peer_device_addr_;
    bool scan_in_progress_;
    bool adv_in_progress_;
    void CleanupClientRequests();
    void ScanEnableReq(BleGapScanEnableReqEvent *evt);
    void ScanEnableRsp(BleIpcStatus status);
    void ScanResult(int advertise_flags,
                            uint16_t service_data_uuid16,
                            uint8_t service_data_len,
                            const uint8_t *service_data,
                            string bdaddr,
                            const vector<uint8_t> & raw_adv_data,
                            const vector<uint8_t> & dev_name,
                            bool is_connectable,
                            bool is_legacy,
                            int16_t rssi,
                            int16_t tx_power,
                            uint8_t sid,
                            uint16_t periodic_adv_int,
                            uint8_t primary_phy,
                            uint8_t secondary_phy);
    void ScanDisableReq();
    void ScanDisableRsp(BleIpcStatus status);
    void AdvertiseEnableReq(BleGapAdvertiseInfo *info);
    void AdvertiseEnableRsp(BleIpcStatus status);
    void AdvertiseDisableReq();
    void AdvertiseDisableRsp(BleIpcStatus status);
};

#endif

