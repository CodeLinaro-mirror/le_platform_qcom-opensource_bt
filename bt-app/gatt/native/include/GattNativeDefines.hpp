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

#ifndef GATT_NATIVE_DEFINES_HPP
#define GATT_NATIVE_INTERFACE_HPP

#pragma once
#include <map>
#include <string>
#include <vector>
#include <array>
#include <memory>
#include <iostream>
#include "utils/Log.h"
#include "uuid.h"

namespace gatt {

#define CHECK_PARAM(x)                                                      \
   if (!x) {                                                                \
       ALOGE("'%s' Param is NULL - exiting from function ", __FUNCTION__);  \
       return false;                                                        \
   }

#define CHECK_PARAM_VOID(x)                                                      \
   if (!x) {                                                                     \
       ALOGE("'%s' Void Param is NULL - exiting from function ", __FUNCTION__);  \
       return ;                                                                  \
   }

/**
 * GATT Service types
 */
#define GATT_SERVICE_TYPE_PRIMARY 0
#define GATT_SERVICE_TYPE_SECONDARY 1
#define GATT_MAX_ATTR_LEN 600

using std::string;

/** GATT ID adding instance id tracking to the UUID */
typedef struct {
  btapp::Uuid uuid;
  uint8_t inst_id;
} gatt_id_t;

/** GATT Service ID also identifies the service type (primary/secondary) */
typedef struct {
  gatt_id_t id;
  uint8_t is_primary;
} gatt_srvc_id_t;

/** Preferred physical Transport for GATT connection */
typedef enum {
  GATT_TRANSPORT_AUTO,
  GATT_TRANSPORT_BREDR,
  GATT_TRANSPORT_LE
} gatt_transport_t;

typedef struct {
  uint8_t client_if;
  uint8_t filt_index;
  uint8_t advertiser_state;
  uint8_t advertiser_info_present;
  uint8_t addr_type;
  uint8_t tx_power;
  int8_t rssi_value;
  uint16_t time_stamp;
  string *bd_addr;
  uint8_t adv_pkt_len;
  uint8_t* p_adv_pkt_data;
  uint8_t scan_rsp_len;
  uint8_t* p_scan_rsp_data;
} gatt_track_adv_info_t;

typedef enum {
  GATT_DB_PRIMARY_SERVICE,
  GATT_DB_SECONDARY_SERVICE,
  GATT_DB_INCLUDED_SERVICE,
  GATT_DB_CHARACTERISTIC,
  GATT_DB_DESCRIPTOR,
} gatt_db_attribute_type_t;

typedef struct {
  uint16_t id;
  btapp::Uuid uuid;
  gatt_db_attribute_type_t type;
  uint16_t attribute_handle;

  /*
   * If |type| is |BTGATT_DB_PRIMARY_SERVICE|, or
   * |BTGATT_DB_SECONDARY_SERVICE|, this contains the start and end attribute
   * handles.
   */
  uint16_t start_handle;
  uint16_t end_handle;

  /*
   * If |type| is |BTGATT_DB_CHARACTERISTIC|, this contains the properties of
   * the characteristic.
   */
  uint8_t properties;
  uint16_t permissions;
} gatt_db_element_t;

/** Buffer type for unformatted reads/writes */
typedef struct {
  uint8_t value[GATT_MAX_ATTR_LEN];
  uint16_t len;
} gatt_unformatted_value_t;

/** Parameters for GATT read operations */
typedef struct {
  uint16_t handle;
  gatt_unformatted_value_t value;
  uint16_t value_type;
  uint8_t status;
} gatt_read_params_t;

/** Parameters for GATT write operations */
typedef struct {
  gatt_srvc_id_t srvc_id;
  gatt_id_t char_id;
  gatt_id_t descr_id;
  uint8_t status;
} gatt_write_params_t;

typedef struct {
  uint8_t value[GATT_MAX_ATTR_LEN];
  string *bda;
  uint16_t handle;
  uint16_t len;
  uint8_t is_notify;
} gatt_notify_params_t;

typedef struct {
  uint16_t feat_seln;
  uint16_t list_logic_type;
  uint8_t filt_logic_type;
  uint8_t rssi_high_thres;
  uint8_t rssi_low_thres;
  uint8_t dely_mode;
  uint16_t found_timeout;
  uint16_t lost_timeout;
  uint8_t found_timeout_cnt;
  uint16_t num_of_tracking_entries;
} gatt_filt_param_setup_t;

// Advertising Packet Content Filter
typedef struct {
  uint8_t type;
  string address;
  uint8_t addr_type;
  btapp::Uuid uuid;
  btapp::Uuid uuid_mask;
  std::vector<uint8_t> name;
  uint16_t company;
  uint16_t company_mask;
  std::vector<uint8_t> data;
  std::vector<uint8_t> data_mask;
}apcf_command_t;


typedef struct {
  uint16_t advertising_event_properties;
  uint32_t min_interval;
  uint32_t max_interval;
  uint8_t channel_map;
  int8_t tx_power;
  uint8_t primary_advertising_phy;
  uint8_t secondary_advertising_phy;
  uint8_t scan_request_notification_enable;
} advertise_parameters_t;

typedef struct {
  uint8_t enable;
  uint16_t min_interval;
  uint16_t max_interval;
  uint16_t periodic_advertising_properties;
}periodic_advertising_parameters_t;
}  // namespace gatt

#endif
