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

#include "GattNativeInterfaceV2_b.hpp"
#include <hardware/bt_gatt.h>
#include <hardware/bt_gatt_types.h>
#include <hardware/bluetooth.h>
#include <base/bind.h>
#include <poll.h>

#include "sdbus_ipc.h"

#define LOG_TAG "bt_gatt"                   // Logcat TAG
#define LOGTAG "GattNativeInterfaceV2bImpl" // Log prefix
#define LOG_NDEBUG 0

/**
 * GATT client callbacks defines
 */
#define TYPE_TO_STR(...) ((const char[]){__VA_ARGS__, 0})
#define READ_SDBUS_ARRAY_TO_VECTOR(m, datatype, sdbus_type, vect_name)                            \
  {                                                                                               \
    int res;                                                                                      \
    res = sd_bus_message_enter_container(m, SD_BUS_TYPE_ARRAY, TYPE_TO_STR(sdbus_type));          \
    if (res < 0)                                                                                  \
    {                                                                                             \
      ALOGE(LOGTAG "::%s Failed to enter containner: %d - %s\n", __func__, -res, strerror(-res)); \
      return sd_bus_reply_method_return(m, nullptr);                                              \
    }                                                                                             \
                                                                                                  \
    for (;;)                                                                                      \
    {                                                                                             \
      datatype vect;                                                                              \
      \      
      res = sd_bus_message_read_basic(m, sdbus_type, &vect);                                      \
      if (0 == res || res < 0)                                                                    \
        break;                                                                                    \
      vect_name.push_back(vect);                                                                  \
    }                                                                                             \
    sd_bus_message_exit_container(m);                                                             \
  }

#define WRITE_SDBUS_ARRAY_FROM_VECTOR(m, datatype, sdbus_type, vect_name)                      \
  {                                                                                            \
    int res;                                                                                   \
    res = sd_bus_message_open_container(m, SD_BUS_TYPE_ARRAY, TYPE_TO_STR(sdbus_type));        \
    if (res < 0)                                                                               \
    {                                                                                          \
      ALOGE(LOGTAG "::%s Failed to open containner: %d - %s", __func__, -res, strerror(-res)); \
      goto finish;                                                                             \
    }                                                                                          \
                                                                                               \
    for (const auto it : vect_name)                                                            \
    {                                                                                          \
      res = sd_bus_message_append_basic(m, sdbus_type, &it);                                   \
      if (res < 0)                                                                             \
        break;                                                                                 \
    }                                                                                          \
    sd_bus_message_close_container(m);                                                         \
  }

namespace gatt {

class GattNativeInterfaceV2bImpl
{
public:
  GattNativeInterfaceV2bImpl() {}
  ~GattNativeInterfaceV2bImpl() {}

  static void init(const btgatt_native_interface_callbacks_t *btgatt_native_callback);
  static void deinit(void);
  static bool initSDBus();
  static void *process_dbus_request(void *ptr);

  static int gattClientGetDeviceTypeNative(string address);
  static void gattClientRegisterAppNative(btapp::Uuid uuid);
  static void gattClientUnregisterAppNative(int clientIf);
  static void registerScannerNative(btapp::Uuid uuid);
  static void unregisterScannerNative(int scanner_id);
  static void gattClientScanNative(bool start);
  static void gattClientConnectNative(int clientif, string address, bool isDirect, int transport, bool opportunistic, int initiating_phys);
  static void gattClientDisconnectNative(int clientIf, string address, int conn_id);
  static void gattClientSetPreferredPhyNative(int clientIf, string address, int tx_phy, int rx_phy, int phy_options);
  static void gattClientReadPhyNative(int clientIf, string address);
  static void gattClientRefreshNative(int clientIf, string address);
  static void gattClientSearchServiceNative(int conn_id, bool search_all, btapp::Uuid uuid);
  static void gattClientDiscoverServiceByUuidNative(int conn_id, btapp::Uuid uuid);
  static void gattClientGetGattDbNative(int conn_id);
  static void gattClientReadCharacteristicNative(int conn_id, int handle, int authReq);
  static void gattClientReadUsingCharacteristicUuidNative(int conn_id, btapp::Uuid uuid, int s_handle, int e_handle, int authReq);
  static void gattClientReadDescriptorNative(int conn_id, int handle, int authReq);
  static void gattClientWriteCharacteristicNative(int conn_id, int handle, int write_type, int auth_req, std::vector<uint8_t> vect_val);
  static void gattClientExecuteWriteNative(int conn_id, bool execute);
  static void gattClientWriteDescriptorNative(int conn_id, int handle, int auth_req,
                                              std::vector<uint8_t> vect_val);
  static void gattClientRegisterForNotificationsNative(
      int clientIf, string address, int handle, bool enable);
  static void gattClientReadRemoteRssiNative(int clientif, string address);
  static void gattSetScanParametersNative(int client_if, int scan_phy,
                                          std::vector<uint32_t> scan_interval, std::vector<uint32_t> scan_window);
  static void getOwnAddressNative(int advertiser_id);
  static void gattClientScanFilterParamAddNative(uint8_t client_if, uint8_t filt_index,
                                                 std::unique_ptr<btgatt_filt_param_setup_t> filt_params);
  static void gattClientScanFilterParamDeleteNative(uint8_t client_if, uint8_t filt_index);
  static void gattClientScanFilterParamClearAllNative(uint8_t client_if);
  static void gattClientScanFilterAddNative(int client_if, int filter_index,
                                            std::vector<apcf_command_t> filters);
  static void gattClientScanFilterClearNative(int client_if, int filt_index);
  static void gattClientScanFilterEnableNative(int client_if, bool enable);
  static void gattClientConfigureMTUNative(int conn_id, int mtu);
  static void gattConnectionParameterUpdateNative(int client_if, string address,
                                                  int min_interval, int max_interval, int latency, int timeout, int min_ce_len,
                                                  int max_ce_len);
  static void gattClientConfigBatchScanStorageNative(int client_if, int max_full_reports_percent,
                                                     int max_trunc_reports_percent, int notify_threshold_level_percent);
  static void gattClientStartBatchScanNative(int client_if, int scan_mode,
                                             int scan_interval_unit, int scan_window_unit, int addr_type, int discard_rule);
  static void gattClientStopBatchScanNative(int client_if);
  static void gattClientReadScanReportsNative(int client_if, int scan_type);
  static void gattServerRegisterAppNative(btapp::Uuid uuid);
  static void gattServerUnregisterAppNative(int serverIf);
  static void gattServerConnectNative(int server_if, string address, bool is_direct,
                                      int transport);
  static void gattServerDisconnectNative(int serverIf, string address, int conn_id);
  static void gattServerSetPreferredPhyNative(int serverIf, string address,
                                              int tx_phy, int rx_phy, int phy_options);
  static void gattServerReadPhyNative(int serverIf, string address);
  static void gattServerAddServiceNative(int server_if, std::vector<gatt_db_element_t> service);
  static void gattServerStopServiceNative(int server_if, int svc_handle);
  static void gattServerDeleteServiceNative(int server_if, int svc_handle);
  static void gattServerSendIndicationNative(int server_if, int attr_handle,
                                             int conn_id, std::vector<uint8_t> vect_val);
  static void gattServerSendNotificationNative(int server_if, int attr_handle,
                                               int conn_id, std::vector<uint8_t> vect_val);
  static void gattServerSendResponseNative(int server_if, int conn_id,
                                           int trans_id, int status, int handle, int offset,
                                           std::vector<uint8_t> vect_val, int auth_req);
  static void startAdvertisingSetNative(advertise_parameters_t params, std::vector<uint8_t> adv_data,
                                        std::vector<uint8_t> scan_resp, periodic_advertising_parameters_t periodic_params,
                                        std::vector<uint8_t> periodic_data, int duration, int maxExtAdvEvents, int reg_id);
  static void stopAdvertisingSetNative(int advertiser_id);
  static void enableAdvertisingSetNative(int advertiser_id, bool enable, int duration, int maxExtAdvEvents);
  static void setAdvertisingDataNative(int advertiser_id, std::vector<uint8_t> data);
  static void setScanResponseDataNative(int advertiser_id, std::vector<uint8_t> data);
  static void setAdvertisingParametersNative(int advertiser_id, advertise_parameters_t parameters);
  static void setPeriodicAdvertisingParametersNative(int advertiser_id, periodic_advertising_parameters_t periodic_parameters);
  static void setPeriodicAdvertisingDataNative(int advertiser_id, std::vector<uint8_t> data);
  static void setPeriodicAdvertisingEnableNative(int advertiser_id, bool enable);
  static void startSyncNative(int sid, string address, int skip, int timeout, int reg_id);
  static void stopSyncNative(int sync_handle);
  static void gattTestNative(int command, btapp::Uuid uuid1, string bda1, int p1, int p2, int p3, int p4, int p5);
};

static RawAddress str2addr(string address)
{
  RawAddress bd_addr;

  RawAddress::FromString(std::string(address), bd_addr);

  return bd_addr;
}

static string *addr2Str(RawAddress address)
{
  return new string(address.ToString());
}

static btapp::Uuid bluetoothUuid2btAppUuid(bluetooth::Uuid uuid)
{
  return btapp::Uuid::From128BitBE(uuid.To128BitBE());
}

static bluetooth::Uuid btAppUuid2bluetoothUuid(btapp::Uuid uuid)
{
  return bluetooth::Uuid::From128BitBE(uuid.To128BitBE());
}

/**
 * GATT client callbacks
 */
const btgatt_native_interface_callbacks_t *btgatt_native_interface_callbacks = NULL;

const btgatt_scanner_callbacks_t *sGattScannerCallbacks = NULL;
const btgatt_client_callbacks_t *sGattClientCallbacks = NULL;
const btgatt_server_callbacks_t *sGattServerCallbacks = NULL;
const btgatt_ext_callbacks_t *sGattExtCallbacks = NULL;

//scanner callbacks
static int register_scanner_cb(sd_bus_message *m, void *userdata, sd_bus_error *ret_error)
{
  bluetooth::Uuid app_uuid;
  uint8_t scannerId;
  uint8_t status;
  ////////////////////////////
  ALOGD(LOGTAG "(%s) enter", __func__);
  const char *str_get;
  bool is_valid;
  int rtn, res;
  res = sd_bus_reply_method_return(m, nullptr);
  rtn = sd_bus_message_read(m, "syy", &str_get, &scannerId, &status);
  if (rtn < 0)
  {
    ALOGE(LOGTAG "::%s Failed to read parameters: %d - %s\n", __func__, -rtn, strerror(-rtn));
  }
  else
  {
    app_uuid = bluetooth::Uuid::FromString(str_get, &is_valid);
    if (sGattExtCallbacks)
      sGattExtCallbacks->register_scanner_cb(app_uuid, scannerId, status);

    ALOGD(LOGTAG "(%s) exit", __func__);
  }
  return res;
}

static int scan_params_cmpl_cb(sd_bus_message *m, void *userdata, sd_bus_error *ret_error)
{
  uint8_t client_if;
  uint8_t status;
  ////////////////////////////
  ALOGD(LOGTAG "(%s) enter", __func__);
  int rtn, res;
  res = sd_bus_reply_method_return(m, nullptr);
  rtn = sd_bus_message_read(m, "yy", &client_if, &status);
  if (rtn < 0)
  {
    ALOGE(LOGTAG "::%s Failed to read parameters: %d - %s\n", __func__, -rtn, strerror(-rtn));
  }
  else
  {
    if (sGattExtCallbacks)
      sGattExtCallbacks->scan_params_cmpl_cb(client_if, status);
  }
  return res;
}

static int scan_result_cb(sd_bus_message *m, void *userdata, sd_bus_error *ret_error)
{
  uint16_t event_type;
  uint8_t addr_type;
  RawAddress bda;
  uint8_t primary_phy;
  uint8_t secondary_phy;
  uint8_t advertising_sid;
  int8_t tx_power;
  int8_t rssi;
  uint16_t periodic_adv_int;
  std::vector<uint8_t> adv_data;
  ////////////////////////////
  ALOGD(LOGTAG "(%s) enter", __func__);
  const char *str_get;
  int rtn, res;
  res = sd_bus_reply_method_return(m, nullptr);
  rtn = sd_bus_message_read(m, "qysyyyiiq", &event_type, &addr_type, &str_get, &primary_phy,
                            &secondary_phy, &advertising_sid, &tx_power, &rssi, &periodic_adv_int);
  if (rtn < 0)
  {
    ALOGE(LOGTAG "::%s Failed to read parameters: %d - %s\n", __func__, -rtn, strerror(-rtn));
  }
  else
  {
    bda = str2addr(string(str_get));
    READ_SDBUS_ARRAY_TO_VECTOR(m, uint8_t, SD_BUS_TYPE_BYTE, adv_data);
    if (adv_data.size() == 0)
    {
      ALOGE(LOGTAG "::%s ignoring NULL array", __func__);
    }

    if (sGattScannerCallbacks)
      sGattScannerCallbacks->scan_result_cb(event_type, addr_type, &bda, primary_phy, secondary_phy,
                                            advertising_sid, tx_power, rssi, periodic_adv_int, adv_data);
  }
  return res;
}

static int batchscan_reports_cb(sd_bus_message *m, void *userdata, sd_bus_error *ret_error)
{
  int client_if;
  int status;
  int report_format;
  int num_records;
  std::vector<uint8_t> data;
  ////////////////////////////
  ALOGD(LOGTAG "(%s) enter", __func__);
  int rtn, res;
  res = sd_bus_reply_method_return(m, nullptr);
  rtn = sd_bus_message_read(m, "iiii", &status, &client_if, &status, &report_format, &num_records, data);
  if (rtn < 0)
  {
    ALOGE(LOGTAG "::%s Failed to read parameters: %d - %s\n", __func__, -rtn, strerror(-rtn));
  }
  else
  {
    READ_SDBUS_ARRAY_TO_VECTOR(m, uint8_t, SD_BUS_TYPE_BYTE, data);
    if (data.size() == 0)
    {
      ALOGE(LOGTAG "::%s ignoring NULL array", __func__);
    }
    else
    {
      ALOGD(LOGTAG "(%s) status : %d client_if : %d", __func__, client_if, status, report_format, num_records, data);
      if (sGattScannerCallbacks)
        sGattScannerCallbacks->batchscan_reports_cb(client_if, status, report_format, num_records, data);
    }
  }
  return res;
}

static int batchscan_threshold_cb(sd_bus_message *m, void *userdata, sd_bus_error *ret_error)
{
  int client_if;
  ////////////////////////////
  ALOGD(LOGTAG "(%s) enter", __func__);
  int rtn, res;
  res = sd_bus_reply_method_return(m, nullptr);
  rtn = sd_bus_message_read(m, "i", &client_if);
  if (rtn < 0)
  {
    ALOGE(LOGTAG "::%s Failed to read parameters: %d - %s\n", __func__, -rtn, strerror(-rtn));
  }
  else
  {
    ALOGD(LOGTAG "(%s) client_if : %d", __func__, client_if);
    if (sGattScannerCallbacks)
      sGattScannerCallbacks->batchscan_threshold_cb(client_if);
  }
  return res;
}

static int track_adv_event_cb(sd_bus_message *m, void *userdata, sd_bus_error *ret_error)
{
  btgatt_track_adv_info_t adv_track_info;

  /* signature : yyyyyyiqsyayyay  */
  ALOGD(LOGTAG "(%s) enter", __func__);

  int rtn, res;
  bool is_valid;
  const char *str_get;
  uint8_t *p_adv_pkt_data, *p_scan_rsp_data;
  int rssi_value;
  size_t size;

  res = sd_bus_reply_method_return(m, nullptr);
  rtn = sd_bus_message_read(m, "yyyyyyiqs", &adv_track_info.client_if,
                            &adv_track_info.filt_index,
                            &adv_track_info.advertiser_state,
                            &adv_track_info.advertiser_info_present,
                            &adv_track_info.addr_type,
                            &adv_track_info.tx_power,
                            &rssi_value,
                            &adv_track_info.time_stamp,
                            &str_get);
  if (rtn < 0)
  {
    ALOGE(LOGTAG "::%s Failed to read parameters: %d - %s\n", __func__, -rtn, strerror(-rtn));
    return res;
  }
  adv_track_info.rssi_value = (int8_t)rssi_value;
  RawAddress::FromString(str_get, adv_track_info.bd_addr);

  rtn = sd_bus_message_read(m, "y", &adv_track_info.adv_pkt_len);
  if (rtn < 0)
  {
    ALOGE(LOGTAG "::%s Failed to read parameters: %d - %s\n", __func__, -rtn, strerror(-rtn));
    return res;
  }

  rtn = sd_bus_message_read_array(m, 'y', reinterpret_cast<const void **>((const uint8_t **)&p_adv_pkt_data), &size);
  if (rtn < 0)
  {
    ALOGE(LOGTAG "::%s Failed to read array: %d - %s\n", __func__, -rtn, strerror(-rtn));
    return res;
  }
  adv_track_info.p_adv_pkt_data = p_adv_pkt_data;

  rtn = sd_bus_message_read(m, "y", &adv_track_info.scan_rsp_len);
  if (rtn < 0)
  {
    ALOGE(LOGTAG "::%s Failed to read parameters: %d - %s\n", __func__, -rtn, strerror(-rtn));
    return res;
  }

  rtn = sd_bus_message_read_array(m, 'y', reinterpret_cast<const void **>((const uint8_t **)&p_scan_rsp_data), &size);
  if (rtn < 0)
  {
    ALOGE(LOGTAG "::%s Failed to read array: %d - %s\n", __func__, -rtn, strerror(-rtn));
    return res;
  }
  adv_track_info.p_scan_rsp_data = p_scan_rsp_data;

  ALOGD(LOGTAG "(%s) ", __func__);
  if (sGattScannerCallbacks)
    sGattScannerCallbacks->track_adv_event_cb((btgatt_track_adv_info_t *)&adv_track_info);

  return res;
}

//sGattClientCallbacks
static int btgattc_register_app_cb(sd_bus_message *m, void *userdata, sd_bus_error *ret_error)
{
  int status;
  int clientIf;
  bluetooth::Uuid app_uuid;
  ////////////////////////////
  ALOGD(LOGTAG "(%s) enter", __func__);
  bool is_valid;
  const char *str_get;
  int rtn, res;
  res = sd_bus_reply_method_return(m, nullptr);
  rtn = sd_bus_message_read(m, "iis", &status, &clientIf, &str_get);
  if (rtn < 0)
  {
    ALOGE(LOGTAG "::%s Failed to read parameters: %d - %s\n", __func__, -rtn, strerror(-rtn));
  }
  else
  {
    app_uuid = bluetooth::Uuid::FromString(str_get, &is_valid);
    if (is_valid)
    {
      ALOGD(LOGTAG " (%s) status = %d, clientIf = %d, app_uuid = %s", __func__, status, clientIf, app_uuid.ToString().c_str());
      if (sGattClientCallbacks)
        sGattClientCallbacks->register_client_cb(status, clientIf, app_uuid);
    }
  }
  return res;
}

static int btgattc_open_cb(sd_bus_message *m, void *userdata, sd_bus_error *ret_error)
{
  int conn_id;
  int status;
  int clientIf;
  RawAddress bda;
  ////////////////////////////
  ALOGD(LOGTAG "(%s) enter", __func__);
  const char *str_get;
  int rtn, res;
  res = sd_bus_reply_method_return(m, nullptr);
  rtn = sd_bus_message_read(m, "iiis", &conn_id, &status, &clientIf, &str_get);
  if (rtn < 0)
  {
    ALOGE(LOGTAG "::%s Failed to read parameters: %d - %s\n", __func__, -rtn, strerror(-rtn));
  }
  else
  {
    bda = str2addr(string(str_get));
    ALOGD(LOGTAG "(%s) conn_id: %d status: %d client_if: %d", __func__, conn_id, status, clientIf);
    if (sGattClientCallbacks)
      sGattClientCallbacks->open_cb(conn_id, status, clientIf, bda);
  }
  return res;
}

static int btgattc_close_cb(sd_bus_message *m, void *userdata, sd_bus_error *ret_error)
{
  int conn_id;
  int status;
  int clientIf;
  RawAddress bda;
  ////////////////////////////
  ALOGD(LOGTAG "(%s) enter", __func__);
  const char *str_get;
  int rtn, res;
  res = sd_bus_reply_method_return(m, nullptr);
  rtn = sd_bus_message_read(m, "iiis", &conn_id, &status, &clientIf, &str_get);
  if (rtn < 0)
  {
    ALOGE(LOGTAG "::%s Failed to read parameters: %d - %s\n", __func__, -rtn, strerror(-rtn));
  }
  else
  {
    bda = str2addr(string(str_get));
    ALOGD(LOGTAG "(%s) conn_id: %d status: %d client_if: %d", __func__, conn_id, status, clientIf);
    if (sGattClientCallbacks)
      sGattClientCallbacks->close_cb(conn_id, status, clientIf, bda);
  }
  return res;
}
static int btgattc_search_complete_cb(sd_bus_message *m, void *userdata, sd_bus_error *ret_error)
{
  int conn_id;
  int status;
  ////////////////////////////
  ALOGD(LOGTAG "(%s) enter", __func__);
  int rtn, res;
  res = sd_bus_reply_method_return(m, nullptr);
  rtn = sd_bus_message_read(m, "ii", &conn_id, &status);
  if (rtn < 0)
  {
    ALOGE(LOGTAG "::%s Failed to read parameters: %d - %s\n", __func__, -rtn, strerror(-rtn));
  }
  else
  {
    ALOGD(LOGTAG "(%s) status: %d conn_id: %d", __func__, status, conn_id);
    if (sGattClientCallbacks)
      sGattClientCallbacks->search_complete_cb(conn_id, status);
  }
  return res;
}

static int btgattc_register_for_notification_cb(sd_bus_message *m, void *userdata, sd_bus_error *ret_error)
{
  int conn_id;
  int registered;
  int status;
  uint16_t handle;
  ////////////////////////////
  ALOGD(LOGTAG "(%s) enter", __func__);
  int rtn, res;
  res = sd_bus_reply_method_return(m, nullptr);
  rtn = sd_bus_message_read(m, "iiiq", &conn_id, &registered, &status, &handle);
  if (rtn < 0)
  {
    ALOGE(LOGTAG "::%s Failed to read parameters: %d - %s\n", __func__, -rtn, strerror(-rtn));
  }
  else
  {
    ALOGD(LOGTAG "(%s) conn_id: %d status: %d registered: %d, handle: %d", __func__, conn_id, status, registered, handle);
    if (sGattClientCallbacks)
      sGattClientCallbacks->register_for_notification_cb(conn_id, registered, status, handle);
  }
  return res;
}

static int btgattc_notify_cb(sd_bus_message *m, void *userdata, sd_bus_error *ret_error)
{
  int conn_id;
  btgatt_notify_params_t p_data = {
      0,
  };

  /* signature : iaysqqy  */
  ALOGD(LOGTAG "(%s) enter", __func__);
  int rtn, res;
  const uint8_t *ptr = NULL;
  const char *str_get;
  size_t size;

  res = sd_bus_reply_method_return(m, nullptr);
  rtn = sd_bus_message_read(m, "i", &conn_id);
  if (rtn < 0)
  {
    ALOGE(LOGTAG "::%s Failed to read parameters: %d - %s\n", __func__, -rtn, strerror(-rtn));
    return res;
  }

  rtn = sd_bus_message_read_array(m, 'y', reinterpret_cast<const void **>(&ptr), &size);
  if (rtn < 0)
  {
    ALOGE(LOGTAG "::%s Failed to read array: %d - %s\n", __func__, -rtn, strerror(-rtn));
    return res;
  }
  memcpy(p_data.value, ptr, size);

  rtn = sd_bus_message_read(m, "sqqy", &str_get, &p_data.handle, &p_data.len, &p_data.is_notify);
  if (rtn < 0)
  {
    ALOGE(LOGTAG "::%s Failed to read parameters: %d - %s\n", __func__, -rtn, strerror(-rtn));
    return res;
  }

  ALOGD(LOGTAG "(%s) conn_id : %d, len : %d", __func__, conn_id, p_data.len);
  if (sGattClientCallbacks)
    sGattClientCallbacks->notify_cb(conn_id, p_data);

  return res;
}

static int btgattc_read_characteristic_cb(sd_bus_message *m, void *userdata, sd_bus_error *ret_error)
{
  int conn_id;
  int status;
  btgatt_read_params_t data = {
      0,
  };

  /* signature : iiqayqqy  */
  ALOGD(LOGTAG "(%s) enter", __func__);

  int rtn, res;
  const uint8_t *ptr = NULL;
  size_t size;

  res = sd_bus_reply_method_return(m, nullptr);
  rtn = sd_bus_message_read(m, "iiq", &conn_id, &status, &data.handle);
  if (rtn < 0)
  {
    ALOGE(LOGTAG "::%s Failed to read parameters: %d - %s\n", __func__, -rtn, strerror(-rtn));
    return res;
  }

  rtn = sd_bus_message_read_array(m, 'y', reinterpret_cast<const void **>(&ptr), &size);
  if (rtn < 0)
  {
    ALOGE(LOGTAG "::%s Failed to read array: %d - %s\n", __func__, -rtn, strerror(-rtn));
    return res;
  }
  memcpy(data.value.value, ptr, size);

  rtn = sd_bus_message_read(m, "qqy", &data.value.len, &data.value_type, &data.status);
  if (rtn < 0)
  {
    ALOGE(LOGTAG "::%s Failed to read parameters: %d - %s\n", __func__, -rtn, strerror(-rtn));
    return res;
  }

  ALOGD(LOGTAG "(%s) status: %d conn_id: %d", __func__, status, conn_id);
  if (sGattClientCallbacks)
    sGattClientCallbacks->read_characteristic_cb(conn_id, status, &data);

  return res;
}

static int btgattc_write_characteristic_cb(sd_bus_message *m, void *userdata, sd_bus_error *ret_error)
{
  int conn_id;
  int status;
  uint16_t handle;
  ////////////////////////////
  ALOGD(LOGTAG "(%s) enter", __func__);
  int rtn, res;
  res = sd_bus_reply_method_return(m, nullptr);
  rtn = sd_bus_message_read(m, "iiq", &conn_id, &status, &handle);
  if (rtn < 0)
  {
    ALOGE(LOGTAG "::%s Failed to read parameters: %d - %s\n", __func__, -rtn, strerror(-rtn));
  }
  else
  {
    ALOGD(LOGTAG "(%s) status : %d conn_id : %d, handle: %d", __func__, status, conn_id, handle);
    if (sGattClientCallbacks)
      sGattClientCallbacks->write_characteristic_cb(conn_id, status, handle);
  }
  return res;
}

static int btgattc_execute_write_cb(sd_bus_message *m, void *userdata, sd_bus_error *ret_error)
{
  int conn_id;
  int status;
  ////////////////////////////
  ALOGD(LOGTAG "(%s) enter", __func__);
  int rtn, res;
  res = sd_bus_reply_method_return(m, nullptr);
  rtn = sd_bus_message_read(m, "ii", &conn_id, &status);
  if (rtn < 0)
  {
    ALOGE(LOGTAG "::%s Failed to read parameters: %d - %s\n", __func__, -rtn, strerror(-rtn));
  }
  else
  {
    ALOGD(LOGTAG "(%s) status : %d conn_id : %d", __func__, status, conn_id);
    if (sGattClientCallbacks)
      sGattClientCallbacks->execute_write_cb(conn_id, status);
  }
  return res;
}

static int btgattc_read_descriptor_cb(sd_bus_message *m, void *userdata, sd_bus_error *ret_error)
{
  int conn_id;
  int status;
  btgatt_read_params_t data;

  /* signature : iiqayqqy  */
  ALOGD(LOGTAG "(%s) enter", __func__);

  int rtn, res;
  const uint8_t *ptr = NULL;
  size_t size;

  res = sd_bus_reply_method_return(m, nullptr);
  rtn = sd_bus_message_read(m, "iiq", &conn_id, &status, &data.handle);
  if (rtn < 0)
  {
    ALOGE(LOGTAG "::%s Failed to read parameters: %d - %s\n", __func__, -rtn, strerror(-rtn));
    return res;
  }

  rtn = sd_bus_message_read_array(m, 'y', reinterpret_cast<const void **>(&ptr), &size);
  if (rtn < 0)
  {
    ALOGE(LOGTAG "::%s Failed to read array: %d - %s\n", __func__, -rtn, strerror(-rtn));
    return res;
  }
  memcpy(data.value.value, ptr, size);

  rtn = sd_bus_message_read(m, "qqy", &data.value.len, &data.value_type, &data.status);
  if (rtn < 0)
  {
    ALOGE(LOGTAG "::%s Failed to read parameters: %d - %s\n", __func__, -rtn, strerror(-rtn));
    return res;
  }

  ALOGD(LOGTAG "(%s) status : %d conn_id : %d", __func__, status, conn_id);
  if (sGattClientCallbacks)
    sGattClientCallbacks->read_descriptor_cb(conn_id, status, data);

  return res;
}

static int btgattc_write_descriptor_cb(sd_bus_message *m, void *userdata, sd_bus_error *ret_error)
{
  int conn_id;
  int status;
  uint16_t handle;
  ////////////////////////////
  ALOGD(LOGTAG "(%s) enter", __func__);
  int rtn, res;
  res = sd_bus_reply_method_return(m, nullptr);
  rtn = sd_bus_message_read(m, "iiq", &conn_id, &status, &handle);
  if (rtn < 0)
  {
    ALOGE(LOGTAG "::%s Failed to read parameters: %d - %s\n", __func__, -rtn, strerror(-rtn));
  }
  else
  {
    ALOGD(LOGTAG "(%s) status : %d conn_id : %d, handle: %d", __func__, status, conn_id, handle);

    if (sGattClientCallbacks)
      sGattClientCallbacks->write_descriptor_cb(conn_id, status, handle);
  }
  return res;
}

static int btgattc_remote_rssi_cb(sd_bus_message *m, void *userdata, sd_bus_error *ret_error)
{
  int client_if;
  RawAddress bda;
  int rssi;
  int status;
  ////////////////////////////
  ALOGD(LOGTAG "(%s) enter", __func__);
  const char *str_get;
  int rtn, res;
  res = sd_bus_reply_method_return(m, nullptr);
  rtn = sd_bus_message_read(m, "isii", &client_if, &str_get, &rssi, &status);
  if (rtn < 0)
  {
    ALOGE(LOGTAG "::%s Failed to read parameters: %d - %s\n", __func__, -rtn, strerror(-rtn));
  }
  else
  {
    bda = str2addr(string(str_get));
    ALOGD(LOGTAG "(%s) status : %d client_if : %d, rssi: %d", __func__, status, client_if, rssi);
    if (sGattClientCallbacks)
      sGattClientCallbacks->read_remote_rssi_cb(client_if, bda, rssi, status);
  }
  return res;
}

static int btgattc_configure_mtu_cb(sd_bus_message *m, void *userdata, sd_bus_error *ret_error)
{
  int conn_id;
  int status;
  int mtu;
  ////////////////////////////
  ALOGD(LOGTAG "(%s) enter", __func__);
  int rtn, res;
  res = sd_bus_reply_method_return(m, nullptr);
  rtn = sd_bus_message_read(m, "iii", &conn_id, &status, &mtu);
  if (rtn < 0)
  {
    ALOGE(LOGTAG "::%s Failed to read parameters: %d - %s\n", __func__, -rtn, strerror(-rtn));
  }
  else
  {
    ALOGD(LOGTAG "(%s) status : %d conn_id : %d, mtu: %d", __func__, status, conn_id, mtu);
    if (sGattClientCallbacks)
      sGattClientCallbacks->configure_mtu_cb(conn_id, status, mtu);
  }
  return res;
}

static int btgattc_congestion_cb(sd_bus_message *m, void *userdata, sd_bus_error *ret_error)
{
  int conn_id;
  int congested;
  ////////////////////////////
  ALOGD(LOGTAG "(%s) enter", __func__);
  int rtn, res;
  res = sd_bus_reply_method_return(m, nullptr);
  rtn = sd_bus_message_read(m, "ii", &conn_id, &congested);
  if (rtn < 0)
  {
    ALOGE(LOGTAG "::%s Failed to read parameters: %d - %s\n", __func__, -rtn, strerror(-rtn));
  }
  else
  {
    ALOGD(LOGTAG "(%s) conn_id : %d congested : %d", __func__, conn_id, congested);
    if (sGattClientCallbacks)
      sGattClientCallbacks->congestion_cb(conn_id, congested);
  }
  return res;
}

static int btgattc_get_gatt_db_cb(sd_bus_message *m, void *userdata, sd_bus_error *ret_error)
{
  int conn_id;
  int count;
  btgatt_db_element_t *db = nullptr;

  /* signature : iia(qsiqqqyq)    */
  ALOGD(LOGTAG "(%s) enter", __func__);

  int rtn, res;
  const char *str_get;
  bool is_valid;
  int type;
  btapp::Uuid uuid;

  res = sd_bus_reply_method_return(m, nullptr);
  rtn = sd_bus_message_read(m, "ii", &conn_id, &count);
  if (rtn < 0)
  {
    ALOGE(LOGTAG "::%s Failed to read parameters: %d - %s\n", __func__, -rtn, strerror(-rtn));
    return res;
  }

  db = new btgatt_db_element_t[count];

  rtn = sd_bus_message_enter_container(m, SD_BUS_TYPE_ARRAY, "(qsiqqqyq)");
  if (rtn < 0)
  {
    ALOGE(LOGTAG "::%s Failed to open container : %d - %s\n", __func__, -rtn, strerror(-rtn));
    delete db;
    return res;
  }

  for (int i=0; i<count; i++)
  {
    btgatt_db_element_t *element = (btgatt_db_element_t *)&db[i];

    sd_bus_message_enter_container(m, 'r', "qsiqqqyq");
    rtn = sd_bus_message_read(m, "qsiqqqyq", &element->id,
                              &str_get,
                              &type,
                              &element->attribute_handle,
                              &element->start_handle,
                              &element->end_handle,
                              &element->properties,
                              &element->permissions);
    if (0 == rtn || rtn < 0)  {
      ALOGE(LOGTAG "::%s Array size mismatched. count:%d, size:%d", __func__, count, i);
      delete db;     
      return res;
    }

    uuid = btapp::Uuid::FromString(str_get, &is_valid);
    if (is_valid)
      element->uuid = btAppUuid2bluetoothUuid(uuid);
    element->type = (bt_gatt_db_attribute_type_t)type;

    sd_bus_message_exit_container(m);
  }

  sd_bus_message_exit_container(m);

  ALOGD(LOGTAG "(%s) conn_id: %d, count: %d", __func__, conn_id, count);
  if (sGattClientCallbacks)
    sGattClientCallbacks->get_gatt_db_cb(conn_id, (const btgatt_db_element_t *)db, count);

  delete db;
  return res;
}

static int btgattc_phy_updated_cb(sd_bus_message *m, void *userdata, sd_bus_error *ret_error)
{
  int conn_id;
  uint8_t tx_phy;
  uint8_t rx_phy;
  uint8_t status;
  ////////////////////////////
  ALOGD(LOGTAG "(%s) enter", __func__);
  int rtn, res;
  res = sd_bus_reply_method_return(m, nullptr);
  rtn = sd_bus_message_read(m, "iyyy", &conn_id, &tx_phy, &rx_phy, &status);
  if (rtn < 0)
  {
    ALOGE(LOGTAG "::%s Failed to read parameters: %d - %s\n", __func__, -rtn, strerror(-rtn));
  }
  else
  {
    ALOGD(LOGTAG "(%s) conn_id: %d tx_phy: %d, rx_phy: %d, status: %d", __func__, conn_id, tx_phy, rx_phy, status);
    if (sGattClientCallbacks)
      sGattClientCallbacks->phy_updated_cb(conn_id, tx_phy, rx_phy, status);
  }
  return res;
}

static int btgattc_conn_updated_cb(sd_bus_message *m, void *userdata, sd_bus_error *ret_error)
{
  int conn_id;
  uint16_t interval;
  uint16_t latency;
  uint16_t timeout;
  uint8_t status;
  ////////////////////////////
  ALOGD(LOGTAG "(%s) enter", __func__);
  int rtn, res;
  res = sd_bus_reply_method_return(m, nullptr);
  rtn = sd_bus_message_read(m, "iqqqy", &conn_id, &interval, &latency, &timeout, &status);
  if (rtn < 0)
  {
    ALOGE(LOGTAG "::%s Failed to read parameters: %d - %s\n", __func__, -rtn, strerror(-rtn));
  }
  else
  {
    ALOGD(LOGTAG "(%s) conn_id: %d interval: %d, latency: %d, timeout: %d status: %d", __func__, conn_id, interval, latency, timeout, status);
    if (sGattClientCallbacks)
      sGattClientCallbacks->conn_updated_cb(conn_id, interval, latency, timeout, status);
  }
  return res;
}
//
// extension
//
static int readClientPhyCb(sd_bus_message *m, void *userdata, sd_bus_error *ret_error)
{
  uint8_t clientIf;
  RawAddress bda;
  uint8_t tx_phy;
  uint8_t rx_phy;
  uint8_t status;
  ////////////////////////////
  ALOGD(LOGTAG "(%s) enter", __func__);
  const char *str_get;
  int rtn, res;
  res = sd_bus_reply_method_return(m, nullptr);
  rtn = sd_bus_message_read(m, "ysyyy", &clientIf, &str_get, &tx_phy, &rx_phy, &status);
  if (rtn < 0)
  {
    ALOGE(LOGTAG "::%s Failed to read parameters: %d - %s\n", __func__, -rtn, strerror(-rtn));
  }
  else
  {
    bda = str2addr(string(str_get));
    ALOGD(LOGTAG "(%s) clientIf: %d, bda: %s, tx_phy: %d, rx_phy: %d, status: %d",
          __func__, clientIf, addr2Str(bda)->c_str(), tx_phy, rx_phy, status);
    if (sGattClientCallbacks)
      sGattExtCallbacks->readClientPhyCb(clientIf, bda, tx_phy, rx_phy, status);
  }
  return res;
}

/**
 * GATT server callbacks
 */
static int btgatts_register_app_cb(sd_bus_message *m, void *userdata, sd_bus_error *ret_error)
{
  int status;
  int server_if;
  btapp::Uuid uuid;
  ////////////////////////////
  ALOGD(LOGTAG "(%s) enter", __func__);
  bool is_valid;
  const char *str_get;
  int rtn, res;
  res = sd_bus_reply_method_return(m, nullptr);
  rtn = sd_bus_message_read(m, "iis", &status, &server_if, &str_get);
  if (rtn < 0)
  {
    ALOGE(LOGTAG "::%s Failed to read parameters: %d - %s\n", __func__, -rtn, strerror(-rtn));
  }
  else
  {
    uuid = btapp::Uuid::FromString(str_get, &is_valid);
    if (is_valid)
    {
      ALOGD(LOGTAG "::%s status:%d, server_if:%d, uuid:%s", __func__, status, server_if, uuid.ToString().c_str());
      if (sGattServerCallbacks)
        sGattServerCallbacks->register_server_cb(status, server_if, btAppUuid2bluetoothUuid(uuid));
      ALOGD(LOGTAG "::%s register_server_cb finished status:%d, server_if:%d, uuid:%s", __func__, status, server_if, uuid.ToString().c_str());
    }
  }
  return res;
}

static int btgatts_connection_cb(sd_bus_message *m, void *userdata, sd_bus_error *ret_error)
{
  int conn_id;
  int server_if;
  int connected;
  RawAddress bda;
  ////////////////////////////
  ALOGD(LOGTAG "(%s) enter", __func__);
  const char *str_get;
  int rtn, res;
  res = sd_bus_reply_method_return(m, nullptr);
  rtn = sd_bus_message_read(m, "iiis", &conn_id, &server_if, &connected, &str_get);
  if (rtn < 0)
  {
    ALOGE(LOGTAG "::%s Failed to read parameters: %d - %s\n", __func__, -rtn, strerror(-rtn));
  }
  else
  {
    bda = str2addr(string(str_get));
    ALOGD(LOGTAG "(%s) connid : %d server_if : %d status : %d bda (%s)", __func__, conn_id,
          server_if, connected, addr2Str(bda)->c_str());
    if (sGattServerCallbacks)
    {
      sGattServerCallbacks->connection_cb(conn_id, server_if, connected, bda);
    }
    ALOGD(LOGTAG "(%s) btgatts_connection_cb finished!!", __func__);
  }
  return res;
}

static int btgatts_service_added_cb(sd_bus_message *m, void *userdata, sd_bus_error *ret_error)
{
  int status;
  int server_if;
  std::vector<btgatt_db_element_t> service;

  /* signature : iia(qsiqqqyq) */
  int rtn, res;
  const char *str_get;
  bool is_valid;
  int type;
  btapp::Uuid uuid;

  res = sd_bus_reply_method_return(m, nullptr);
  rtn = sd_bus_message_read(m, "ii", &status, &server_if);
  if (rtn < 0)
  {
    ALOGE(LOGTAG "::%s Failed to read parameters: %d - %s\n", __func__, -rtn, strerror(-rtn));
    return res;
  }

  rtn = sd_bus_message_enter_container(m, SD_BUS_TYPE_ARRAY, "(qsiqqqyq)");
  if (rtn < 0)
  {
    ALOGE(LOGTAG "::%s Failed to open container : %d - %s\n", __func__, -rtn, strerror(-rtn));
    return res;
  }

  for (;;)
  {
    btgatt_db_element_t data;

    sd_bus_message_enter_container(m, 'r', "qsiqqqyq");
    rtn = sd_bus_message_read(m, "qsiqqqyq", &data.id,
                              &str_get,
                              &type,
                              &data.attribute_handle,
                              &data.start_handle,
                              &data.end_handle,
                              &data.properties,
                              &data.permissions);
    if (0 == rtn || rtn < 0)
      break;

    uuid = btapp::Uuid::FromString(str_get, &is_valid);
    if (is_valid)
      data.uuid = btAppUuid2bluetoothUuid(uuid);
    data.type = (bt_gatt_db_attribute_type_t)type;

    service.push_back(data);
    sd_bus_message_exit_container(m);
  }

  sd_bus_message_exit_container(m);

  ALOGD(LOGTAG "(%s) status : %d server_if : %d, service size:%d", __func__, status, server_if, service.size());
  if (sGattServerCallbacks)
  {
    sGattServerCallbacks->service_added_cb(status, server_if, service);
  }

  return res;
}

static int btgatts_service_stopped_cb(sd_bus_message *m, void *userdata, sd_bus_error *ret_error)
{
  int status;
  int server_if;
  int srvc_handle;
  ////////////////////////////
  ALOGD(LOGTAG "(%s) enter", __func__);
  int rtn, res;
  res = sd_bus_reply_method_return(m, nullptr);
  rtn = sd_bus_message_read(m, "iii", &status, &server_if, &srvc_handle);
  if (rtn < 0)
  {
    ALOGE(LOGTAG "::%s Failed to read parameters: %d - %s\n", __func__, -rtn, strerror(-rtn));
  }
  else
  {
    ALOGD(LOGTAG "(%s) status:: %d, server_if: %d, srvc_handle: %d",
          __func__, status, server_if, srvc_handle);
    if (sGattServerCallbacks)
    {
      sGattServerCallbacks->service_stopped_cb(status, server_if, srvc_handle);
    }
  }
  return res;
}

static int btgatts_service_deleted_cb(sd_bus_message *m, void *userdata, sd_bus_error *ret_error)
{
  int status;
  int server_if;
  int srvc_handle;
  ////////////////////////////
  ALOGD(LOGTAG "(%s) enter", __func__);
  int rtn, res;
  res = sd_bus_reply_method_return(m, nullptr);
  rtn = sd_bus_message_read(m, "iii", &status, &server_if, &srvc_handle);
  if (rtn < 0)
  {
    ALOGE(LOGTAG "::%s Failed to read parameters: %d - %s\n", __func__, -rtn, strerror(-rtn));
  }
  else
  {
    ALOGD(LOGTAG "(%s) status:: %d, server_if: %d, srvc_handle: %d",
          __func__, status, server_if, srvc_handle);

    if (sGattServerCallbacks)
    {
      sGattServerCallbacks->service_deleted_cb(status, server_if, srvc_handle);
    }
  }
  return res;
}

static int btgatts_request_read_characteristic_cb(sd_bus_message *m, void *userdata, sd_bus_error *ret_error)
{
  int conn_id;
  int trans_id;
  RawAddress bda;
  int attr_handle;
  int offset;
  int is_long; // bool
  ////////////////////////////
  ALOGD(LOGTAG "(%s) enter", __func__);
  const char *str_get;
  int rtn, res;
  res = sd_bus_reply_method_return(m, nullptr);
  rtn = sd_bus_message_read(m, "iisiii", &conn_id, &trans_id, &str_get, &attr_handle, &offset, &is_long);
  if (rtn < 0)
  {
    ALOGE(LOGTAG "::%s Failed to read parameters: %d - %s\n", __func__, -rtn, strerror(-rtn));
  }
  else
  {
    bda = str2addr(string(str_get));
    ALOGD(LOGTAG "(%s) connid: %d trans_id: %d, bda: %s, attr_handle: %d, offset:%d is_long: %d",
          __func__, conn_id, trans_id, addr2Str(bda)->c_str(), attr_handle, offset, is_long);
    if (sGattServerCallbacks)
    {
      sGattServerCallbacks->request_read_characteristic_cb(conn_id, trans_id, bda, attr_handle, offset, (bool)is_long);
    }
  }
  return res;
}

static int btgatts_request_read_descriptor_cb(sd_bus_message *m, void *userdata, sd_bus_error *ret_error)
{
  int conn_id;
  int trans_id;
  RawAddress bda;
  int attr_handle;
  int offset;
  int is_long; //bool
  ////////////////////////////
  ALOGD(LOGTAG "(%s) enter", __func__);
  const char *str_get;
  int rtn, res;
  res = sd_bus_reply_method_return(m, nullptr);
  rtn = sd_bus_message_read(m, "iisiii", &conn_id, &trans_id, &str_get, &attr_handle, &offset, &is_long);
  if (rtn < 0)
  {
    ALOGE(LOGTAG "::%s Failed to read parameters: %d - %s\n", __func__, -rtn, strerror(-rtn));
  }
  else
  {
    bda = str2addr(string(str_get));
    ALOGD(LOGTAG "(%s) connid: %d trans_id: %d, bda: %s, attr_handle: %d, offset:%d is_long: %d",
          __func__, conn_id, trans_id, addr2Str(bda)->c_str(), attr_handle, offset, is_long);

    if (sGattServerCallbacks)
    {
      sGattServerCallbacks->request_read_descriptor_cb(conn_id, trans_id, bda, attr_handle, offset, (bool)is_long);
    }
  }
  return res;
}

static int btgatts_request_write_characteristic_cb(sd_bus_message *m, void *userdata, sd_bus_error *ret_error)
{
  int conn_id;
  int trans_id;
  RawAddress bda;
  int attr_handle;
  int offset;
  int need_rsp; //bool
  int is_prep;  //bool
  std::vector<uint8_t> value;
  ////////////////////////////
  ALOGD(LOGTAG "(%s) enter", __func__);
  const char *str_get;
  int rtn, res;
  res = sd_bus_reply_method_return(m, nullptr);
  rtn = sd_bus_message_read(m, "iisiiii", &conn_id, &trans_id, &str_get, &attr_handle, &offset, &need_rsp, &is_prep);
  if (rtn < 0)
  {
    ALOGE(LOGTAG "::%s Failed to read parameters: %d - %s\n", __func__, -rtn, strerror(-rtn));
  }
  else
  {
    READ_SDBUS_ARRAY_TO_VECTOR(m, uint8_t, SD_BUS_TYPE_BYTE, value);
    if (value.size() == 0)
    {
      ALOGE(LOGTAG "::%s ignoring NULL array", __func__);
    }

    bda = str2addr(str_get); 
    ALOGD(LOGTAG "(%s) connid: %d trans_id: %d, bda: %s, attr_handle: %d, offset:%d need_rsp: %d,"
                 " is_prep:%d",
          __func__, conn_id, trans_id, addr2Str(bda)->c_str(), attr_handle, offset,
          need_rsp, is_prep);
    if (sGattServerCallbacks)
    {
      sGattServerCallbacks->request_write_characteristic_cb(conn_id, trans_id, bda, attr_handle, offset, (bool)need_rsp, (bool)is_prep, value);
    }
  }
  return res;
}

static int btgatts_request_write_descriptor_cb(sd_bus_message *m, void *userdata, sd_bus_error *ret_error)
{
  int conn_id;
  int trans_id;
  RawAddress bda;
  int attr_handle;
  int offset;
  int need_rsp; //bool
  int is_prep;  //bool
  std::vector<uint8_t> value;
  ////////////////////////////
  ALOGD(LOGTAG "(%s) enter", __func__);
  const char *str_get;
  int rtn, res;
  res = sd_bus_reply_method_return(m, nullptr);
  rtn = sd_bus_message_read(m, "iisiiii", &conn_id, &trans_id, &str_get, &attr_handle, &offset, &need_rsp, &is_prep);
  if (rtn < 0)
  {
    ALOGE(LOGTAG "::%s Failed to read parameters: %d - %s\n", __func__, -rtn, strerror(-rtn));
  }
  else
  {
    bda = str2addr(string(str_get));
    READ_SDBUS_ARRAY_TO_VECTOR(m, uint8_t, SD_BUS_TYPE_BYTE, value);
    if (value.size() == 0)
    {
      ALOGE(LOGTAG "::%s ignoring NULL array", __func__);
    }

    ALOGD(LOGTAG "(%s) connid: %d trans_id: %d, bda: %s, attr_handle: %d, offset:%d need_rsp: %d,"
                 " is_prep:%d",
          __func__, conn_id, trans_id, addr2Str(bda)->c_str(), attr_handle, offset,
          need_rsp, is_prep);

    if (sGattServerCallbacks)
    {
      sGattServerCallbacks->request_write_descriptor_cb(conn_id, trans_id, bda, attr_handle, offset, (bool)need_rsp, (bool)is_prep, value);
    }
  }
  return res;
}

static int btgatts_request_exec_write_cb(sd_bus_message *m, void *userdata, sd_bus_error *ret_error)
{
  int conn_id;
  int trans_id;
  RawAddress bda;
  int exec_write;
  ////////////////////////////
  ALOGD(LOGTAG "(%s) enter", __func__);
  const char *str_get;
  int rtn, res;
  res = sd_bus_reply_method_return(m, nullptr);
  rtn = sd_bus_message_read(m, "iisi", &conn_id, &trans_id, &str_get, &exec_write);
  if (rtn < 0)
  {
    ALOGE(LOGTAG "::%s Failed to read parameters: %d - %s\n", __func__, -rtn, strerror(-rtn));
  }
  else
  {
    bda = str2addr(string(str_get));
    ALOGD(LOGTAG "(%s) connid: %d trans_id: %d, bda: %s, exec_write: %d",
          __func__, conn_id, trans_id, addr2Str(bda)->c_str(), exec_write);

    if (sGattServerCallbacks)
    {
      sGattServerCallbacks->request_exec_write_cb(conn_id, trans_id, bda, exec_write);
    }
  }
  return res;
}

static int btgatts_response_confirmation_cb(sd_bus_message *m, void *userdata, sd_bus_error *ret_error)
{
  int status;
  int handle;
  ////////////////////////////
  ALOGD(LOGTAG "(%s) enter", __func__);
  int rtn, res;
  res = sd_bus_reply_method_return(m, nullptr);
  rtn = sd_bus_message_read(m, "ii", &status, &handle);
  if (rtn < 0)
  {
    ALOGE(LOGTAG "::%s Failed to read parameters: %d - %s\n", __func__, -rtn, strerror(-rtn));
  }
  else
  {
    ALOGD(LOGTAG "(%s) status: %d handle: %d", __func__, status, handle);
    if (sGattServerCallbacks)
    {
      sGattServerCallbacks->response_confirmation_cb(status, handle);
    }
  }
  return res;
}

static int btgatts_indication_sent_cb(sd_bus_message *m, void *userdata, sd_bus_error *ret_error)
{
  int conn_id;
  int status;
  ////////////////////////////
  ALOGD(LOGTAG "(%s) enter", __func__);
  int rtn, res;
  res = sd_bus_reply_method_return(m, nullptr);
  rtn = sd_bus_message_read(m, "ii", &conn_id, &status);
  if (rtn < 0)
  {
    ALOGE(LOGTAG "::%s Failed to read parameters: %d - %s\n", __func__, -rtn, strerror(-rtn));
  }
  else
  {
    ALOGD(LOGTAG "(%s) conn_id : %d status:: %d", __func__, conn_id, status);

    if (sGattServerCallbacks)
    {
      sGattServerCallbacks->indication_sent_cb(conn_id, status);
    }
  }
  return res;
}

static int btgatts_congestion_cb(sd_bus_message *m, void *userdata, sd_bus_error *ret_error)
{
  int conn_id;
  int congested; // bool
  ////////////////////////////
  ALOGD(LOGTAG "(%s) enter", __func__);
  int rtn, res;
  res = sd_bus_reply_method_return(m, nullptr);
  rtn = sd_bus_message_read(m, "ii", &conn_id, &congested);
  if (rtn < 0)
  {
    ALOGE(LOGTAG "::%s Failed to read parameters: %d - %s\n", __func__, -rtn, strerror(-rtn));
  }
  else
  {
    ALOGD(LOGTAG "(%s) contested: %d conn_id: %d", __func__, congested, conn_id);

    if (sGattServerCallbacks)
    {
      sGattServerCallbacks->congestion_cb(conn_id, (bool)congested);
    }
  }
  return res;
}

static int btgatts_mtu_changed_cb(sd_bus_message *m, void *userdata, sd_bus_error *ret_error)
{
  int conn_id;
  int mtu;
  ////////////////////////////
  ALOGD(LOGTAG "(%s) enter", __func__);
  int rtn, res;
  res = sd_bus_reply_method_return(m, nullptr);
  rtn = sd_bus_message_read(m, "ii", &conn_id, &mtu);
  if (rtn < 0)
  {
    ALOGE(LOGTAG "::%s Failed to read parameters: %d - %s\n", __func__, -rtn, strerror(-rtn));
  }
  else
  {
    ALOGD(LOGTAG "(%s) conn_id: %d Mtu: %d", __func__, conn_id, mtu);

    if (sGattServerCallbacks)
    {
      sGattServerCallbacks->mtu_changed_cb(conn_id, mtu);
    }
  }
  return res;
}

static int btgatts_phy_updated_cb(sd_bus_message *m, void *userdata, sd_bus_error *ret_error)
{
  int conn_id;
  uint8_t tx_phy;
  uint8_t rx_phy;
  uint8_t status;
  ////////////////////////////
  ALOGD(LOGTAG "(%s) enter", __func__);
  int rtn, res;
  res = sd_bus_reply_method_return(m, nullptr);
  rtn = sd_bus_message_read(m, "iyyy", &conn_id, &tx_phy, &rx_phy, &status);
  if (rtn < 0)
  {
    ALOGE(LOGTAG "::%s Failed to read parameters: %d - %s\n", __func__, -rtn, strerror(-rtn));
  }
  else
  {

    ALOGD(LOGTAG "(%s) conn_id: %d tx_phy: %d, rx_phy: %d, status: %d",
          __func__, conn_id, tx_phy, rx_phy, status);

    if (sGattServerCallbacks)
    {
      sGattServerCallbacks->phy_updated_cb(conn_id, tx_phy, rx_phy, status);
    }
  }

  return res;
}

static int btgatts_conn_updated_cb(sd_bus_message *m, void *userdata, sd_bus_error *ret_error)
{
  int conn_id;
  uint16_t interval;
  uint16_t latency;
  uint16_t timeout;
  uint8_t status;
  ////////////////////////////
  ALOGD(LOGTAG "(%s) enter", __func__);
  int rtn, res;
  res = sd_bus_reply_method_return(m, nullptr);
  rtn = sd_bus_message_read(m, "iqqqy", &conn_id, &interval, &latency, &timeout, &status);
  if (rtn < 0)
  {
    ALOGE(LOGTAG "::%s Failed to read parameters: %d - %s\n", __func__, -rtn, strerror(-rtn));
  }
  else
  {
    ALOGD(LOGTAG "(%s) conn_id: %d interval: %d, latency: %d, timeout: %d status: %d",
          __func__, conn_id, interval, latency, timeout, status);

    if (sGattServerCallbacks)
    {
      sGattServerCallbacks->conn_updated_cb(conn_id, interval, latency, timeout, status);
    }
  }
  return res;
}
//
//extension
//
static int readServerPhyCb(sd_bus_message *m, void *userdata, sd_bus_error *ret_error)
{
  uint8_t serverIf;
  RawAddress bda;
  uint8_t tx_phy;
  uint8_t rx_phy;
  uint8_t status;
  ////////////////////////////
  ALOGD(LOGTAG "(%s) enter", __func__);
  const char *str_get;
  int rtn, res;
  res = sd_bus_reply_method_return(m, nullptr);
  rtn = sd_bus_message_read(m, "ysyyy", &serverIf, &str_get, &tx_phy, &rx_phy, &status);
  if (rtn < 0)
  {
    ALOGE(LOGTAG "::%s Failed to read parameters: %d - %s\n", __func__, -rtn, strerror(-rtn));
  }
  else
  {
    bda = str2addr(string(str_get));
    ALOGD(LOGTAG "(%s) serverIf: %d, bda: %s, tx_phy: %d, rx_phy: %d, status: %d",
          __func__, serverIf, addr2Str(bda)->c_str(), tx_phy, rx_phy, status);

    if (sGattExtCallbacks)
    {
      sGattExtCallbacks->readServerPhyCb(serverIf, bda, tx_phy, rx_phy, status);
    }
  }
  return res;
}
static int scan_filter_param_cb(sd_bus_message *m, void *userdata, sd_bus_error *ret_error)
{
  uint8_t client_if;
  uint8_t avbl_space;
  uint8_t action;
  uint8_t status;
  ////////////////////////////
  ALOGD(LOGTAG "(%s) enter", __func__);
  int rtn, res;
  res = sd_bus_reply_method_return(m, nullptr);
  rtn = sd_bus_message_read(m, "yyyy", &client_if, &avbl_space, &action, &status);
  if (rtn < 0)
  {
    ALOGE(LOGTAG "::%s Failed to read parameters: %d - %s\n", __func__, -rtn, strerror(-rtn));
  }
  else
  {
    ALOGD(LOGTAG "(%s) status : %d client_if : %d, avbl_space: %d, action:%d",
          __func__, status, client_if, avbl_space, action);
    if (sGattExtCallbacks)
    {
      sGattExtCallbacks->scan_filter_param_cb(client_if, avbl_space, action, status);
    }
  }
  ALOGD(LOGTAG "(%s) exit", __func__);
  return res;
}

static int scan_filter_cfg_cb(sd_bus_message *m, void *userdata, sd_bus_error *ret_error)
{
  uint8_t client_if;
  uint8_t filt_type;
  uint8_t avbl_space;
  uint8_t action;
  uint8_t status;
  ////////////////////////////
  ALOGD(LOGTAG "(%s) enter", __func__);
  int rtn, res;
  res = sd_bus_reply_method_return(m, nullptr);
  rtn = sd_bus_message_read(m, "yyyyy", &client_if, &filt_type, &avbl_space, &action, &status);
  if (rtn < 0)
  {
    ALOGE(LOGTAG "::%s Failed to read parameters: %d - %s\n", __func__, -rtn, strerror(-rtn));
  }
  else
  {
    ALOGD(LOGTAG "(%s) status : %d client_if : %d",
          __func__, status, client_if);
    if (sGattExtCallbacks)
    {
      sGattExtCallbacks->scan_filter_cfg_cb(client_if, filt_type, avbl_space, action, status);
    }
  }
  return res;
}

static int scan_filter_status_cb(sd_bus_message *m, void *userdata, sd_bus_error *ret_error)
{
  uint8_t client_if;
  uint8_t action;
  uint8_t status;
  ////////////////////////////
  ALOGD(LOGTAG "(%s) enter", __func__);
  int rtn, res;
  res = sd_bus_reply_method_return(m, nullptr);
  rtn = sd_bus_message_read(m, "yyy", &client_if, &action, &status);
  if (rtn < 0)
  {
    ALOGE(LOGTAG "::%s Failed to read parameters: %d - %s\n", __func__, -rtn, strerror(-rtn));
  }
  else
  {
    ALOGD(LOGTAG "(%s) status : %d client_if : %d, action:%d",
          __func__, status, client_if, action);
    if (sGattExtCallbacks)
    {
      sGattExtCallbacks->scan_filter_status_cb(client_if, action, status);
    }
  }
  return res;
}

static int batchscan_cfg_storage_cb(sd_bus_message *m, void *userdata, sd_bus_error *ret_error)
{
  uint8_t client_if;
  uint8_t status;
  ////////////////////////////
  ALOGD(LOGTAG "(%s) enter", __func__);
  int rtn, res;
  res = sd_bus_reply_method_return(m, nullptr);
  rtn = sd_bus_message_read(m, "yy", &client_if, &status);
  if (rtn < 0)
  {
    ALOGE(LOGTAG "::%s Failed to read parameters: %d - %s\n", __func__, -rtn, strerror(-rtn));
  }
  else
  {
    ALOGD(LOGTAG "(%s) status : %d client_if : %d", __func__, status, client_if);

    if (sGattExtCallbacks)
    {
      sGattExtCallbacks->batchscan_cfg_storage_cb(client_if, status);
    }
  }
  return res;
}

static int batchscan_start_cb(sd_bus_message *m, void *userdata, sd_bus_error *ret_error)
{
  uint8_t client_if;
  uint8_t status;
  ////////////////////////////
  ALOGD(LOGTAG "(%s) enter", __func__);
  int rtn, res;
  res = sd_bus_reply_method_return(m, nullptr);
  rtn = sd_bus_message_read(m, "yy", &client_if, &status);
  if (rtn < 0)
  {
    ALOGE(LOGTAG "::%s Failed to read parameters: %d - %s\n", __func__, -rtn, strerror(-rtn));
  }
  else
  {
    ALOGD(LOGTAG "(%s) status : %d client_if : %d", __func__, status, client_if);

    if (sGattExtCallbacks)
    {
      sGattExtCallbacks->batchscan_start_cb(client_if, status);
    }
  }
  return res;
}
static int batchscan_stop_cb(sd_bus_message *m, void *userdata, sd_bus_error *ret_error)
{
  uint8_t client_if;
  uint8_t status;
  ////////////////////////////
  ALOGD(LOGTAG "(%s) enter", __func__);
  int rtn, res;
  res = sd_bus_reply_method_return(m, nullptr);
  rtn = sd_bus_message_read(m, "yy", &client_if, &status);
  if (rtn < 0)
  {
    ALOGE(LOGTAG "::%s Failed to read parameters: %d - %s\n", __func__, -rtn, strerror(-rtn));
  }
  else
  {
    ALOGD(LOGTAG "(%s) status : %d client_if : %d", __func__, status, client_if);
    if (sGattExtCallbacks)
    {
      sGattExtCallbacks->batchscan_stop_cb(client_if, status);
    }
  }
  return res;
}
static int onSyncStarted(sd_bus_message *m, void *userdata, sd_bus_error *ret_error)
{
  int reg_id;
  uint8_t status;
  uint16_t sync_handle;
  uint8_t sid;
  uint8_t address_type;
  RawAddress address;
  uint8_t phy;
  uint16_t interval;
  ////////////////////////////
  ALOGD(LOGTAG "(%s) enter", __func__);
  const char *str_get;
  int rtn, res;
  res = sd_bus_reply_method_return(m, nullptr);
  rtn = sd_bus_message_read(m, "iyqyysyq", &reg_id, &status, &sync_handle, &sid, &address_type, &str_get, &phy, &interval);
  if (rtn < 0)
  {
    ALOGE(LOGTAG "::%s Failed to read parameters: %d - %s\n", __func__, -rtn, strerror(-rtn));
  }
  else
  {
    address = str2addr(string(str_get));
    if (sGattExtCallbacks)
    {
      sGattExtCallbacks->onSyncStarted(reg_id, status, sync_handle, sid, address_type, address, phy, interval);
    }
  }
  return res;
}
static int onSyncLost(sd_bus_message *m, void *userdata, sd_bus_error *ret_error)
{
  uint16_t sync_handle;
  ////////////////////////////
  ALOGD(LOGTAG "(%s) enter", __func__);
  int rtn, res;
  res = sd_bus_reply_method_return(m, nullptr);
  rtn = sd_bus_message_read(m, "q", &sync_handle);
  if (rtn < 0)
  {
    ALOGE(LOGTAG "::%s Failed to read parameters: %d - %s\n", __func__, -rtn, strerror(-rtn));
  }
  else
  {
    if (sGattExtCallbacks)
    {
      sGattExtCallbacks->onSyncLost(sync_handle);
    }
  }
  return res;
}

static int onSyncReport(sd_bus_message *m, void *userdata, sd_bus_error *ret_error)
{
  uint16_t sync_handle;
  int tx_power;
  int rssi;
  uint8_t data_status;
  std::vector<uint8_t> data;
  ////////////////////////////
  ALOGD(LOGTAG "(%s) enter", __func__);
  int rtn, res;
  res = sd_bus_reply_method_return(m, nullptr);
  rtn = sd_bus_message_read(m, "qiiy", &sync_handle, &tx_power, &rssi, &data_status);
  if (rtn < 0)
  {
    ALOGE(LOGTAG "::%s Failed to read parameters: %d - %s\n", __func__, -rtn, strerror(-rtn));
  }
  else
  {
    READ_SDBUS_ARRAY_TO_VECTOR(m, uint8_t, SD_BUS_TYPE_BYTE, data);
    if (data.size() == 0)
    {
      ALOGE(LOGTAG "::%s ignoring NULL array", __func__);
    }

    if (sGattExtCallbacks)
    {
      sGattExtCallbacks->onSyncReport(sync_handle, tx_power, rssi, data_status, data);
    }
  }
  return res;
}

static int onSetAdvertisingData(sd_bus_message *m, void *userdata, sd_bus_error *ret_error)
{
  uint8_t advertiser_id;
  uint8_t status;
  ////////////////////////////
  ALOGD(LOGTAG "(%s) enter", __func__);
  int rtn, res;
  res = sd_bus_reply_method_return(m, nullptr);
  rtn = sd_bus_message_read(m, "yy", &advertiser_id, &status);
  if (rtn < 0)
  {
    ALOGE(LOGTAG "::%s Failed to read parameters: %d - %s\n", __func__, -rtn, strerror(-rtn));
  }
  else
  {
    ALOGD(LOGTAG "(%s) advertiser_id: %d status: %d", __func__, advertiser_id, status);

    if (sGattExtCallbacks)
    {
      sGattExtCallbacks->onSetAdvertisingData(advertiser_id, status);
    }
  }
  return res;
}
static int onSetScanResponseData(sd_bus_message *m, void *userdata, sd_bus_error *ret_error)
{
  uint8_t advertiser_id;
  uint8_t status;
  ////////////////////////////
  ALOGD(LOGTAG "(%s) enter", __func__);
  int rtn, res;
  res = sd_bus_reply_method_return(m, nullptr);
  rtn = sd_bus_message_read(m, "yy", &advertiser_id, &status);
  if (rtn < 0)
  {
    ALOGE(LOGTAG "::%s Failed to read parameters: %d - %s\n", __func__, -rtn, strerror(-rtn));
  }
  else
  {
    ALOGD(LOGTAG "(%s) advertiser_id: %d status: %d", __func__, advertiser_id, status);

    if (sGattExtCallbacks)
    {
      sGattExtCallbacks->onSetScanResponseData(advertiser_id, status);
    }
  }
  return res;
}
static int onSetPeriodicAdvertisingParameters(sd_bus_message *m, void *userdata, sd_bus_error *ret_error)
{
  uint8_t advertiser_id;
  uint8_t status;
  ////////////////////////////
  ALOGD(LOGTAG "(%s) enter", __func__);
  int rtn, res;
  res = sd_bus_reply_method_return(m, nullptr);
  rtn = sd_bus_message_read(m, "yy", &advertiser_id, &status);
  if (rtn < 0)
  {
    ALOGE(LOGTAG "::%s Failed to read parameters: %d - %s\n", __func__, -rtn, strerror(-rtn));
  }
  else
  {
    ALOGD(LOGTAG "(%s) advertiser_id: %d status: %d", __func__, advertiser_id, status);

    if (sGattExtCallbacks)
    {
      sGattExtCallbacks->onSetPeriodicAdvertisingParameters(advertiser_id, status);
    }
  }
  return res;
}
static int onSetPeriodicAdvertisingData(sd_bus_message *m, void *userdata, sd_bus_error *ret_error)
{
  uint8_t advertiser_id;
  uint8_t status;
  ////////////////////////////
  ALOGD(LOGTAG "(%s) enter", __func__);
  int rtn, res;
  res = sd_bus_reply_method_return(m, nullptr);
  rtn = sd_bus_message_read(m, "yy", &advertiser_id, &status);
  if (rtn < 0)
  {
    ALOGE(LOGTAG "::%s Failed to read parameters: %d - %s\n", __func__, -rtn, strerror(-rtn));
  }
  else
  {
    ALOGD(LOGTAG "(%s) advertiser_id: %d status: %d", __func__, advertiser_id, status);

    if (sGattExtCallbacks)
    {
      sGattExtCallbacks->onSetPeriodicAdvertisingData(advertiser_id, status);
    }
  }
  return res;
}
static int getOwnAddressCb(sd_bus_message *m, void *userdata, sd_bus_error *ret_error)
{
  uint8_t advertiser_id;
  uint8_t address_type;
  RawAddress address;
  ////////////////////////////
  ALOGD(LOGTAG "(%s) enter", __func__);
  const char *str_get;
  int rtn, res;
  res = sd_bus_reply_method_return(m, nullptr);
  rtn = sd_bus_message_read(m, "yys", &advertiser_id, &address_type, &str_get);
  if (rtn < 0)
  {
    ALOGE(LOGTAG "::%s Failed to read parameters: %d - %s\n", __func__, -rtn, strerror(-rtn));
  }
  else
  {
    address = str2addr(string(str_get));
    ALOGD(LOGTAG "(%s) advertiser_id: %d address_type: %d, address: %s",
          __func__, advertiser_id, address_type, addr2Str(address)->c_str());

    if (sGattExtCallbacks)
    {
      sGattExtCallbacks->getOwnAddressCb(advertiser_id, address_type, address);
    }
  }
  return res;
}

static int ble_advertising_set_started_cb(sd_bus_message *m, void *userdata, sd_bus_error *ret_error)
{
  int reg_id;
  uint8_t advertiser_id;
  int tx_power;
  uint8_t status;
  ////////////////////////////
  ALOGD(LOGTAG "(%s) enter", __func__);
  int rtn, res;
  res = sd_bus_reply_method_return(m, nullptr);
  rtn = sd_bus_message_read(m, "iyiy", &reg_id, &advertiser_id, &tx_power, &status);
  if (rtn < 0)
  {
    ALOGE(LOGTAG "::%s Failed to read parameters: %d - %s\n", __func__, -rtn, strerror(-rtn));
  }
  else
  {
    ALOGD(LOGTAG "(%s) reg_id: %d, advertiser_id: %d, tx_power: %d, status: %d,",
          __func__, reg_id, advertiser_id, tx_power, status);

    if (sGattExtCallbacks)
    {
      sGattExtCallbacks->ble_advertising_set_started_cb(reg_id, advertiser_id, (int8_t)tx_power, status);
    }
    ALOGD(LOGTAG "(%s) ble_advertising_set_started_cb() finished!!", __func__);
  }
  return res;
}

static int ble_advertising_set_timeout_cb(sd_bus_message *m, void *userdata, sd_bus_error *ret_error)
{
  uint8_t advertiser_id;
  uint8_t status;
  ////////////////////////////
  ALOGD(LOGTAG "(%s) enter", __func__);
  int rtn, res;
  res = sd_bus_reply_method_return(m, nullptr);
  rtn = sd_bus_message_read(m, "yy", &advertiser_id, &status);
  if (rtn < 0)
  {
    ALOGE(LOGTAG "::%s Failed to read parameters: %d - %s\n", __func__, -rtn, strerror(-rtn));
  }
  else
  {
    ALOGD(LOGTAG "(%s) advertiser_id: %d status: %d", __func__, advertiser_id, status);

    if (sGattExtCallbacks)
    {
      sGattExtCallbacks->ble_advertising_set_timeout_cb(advertiser_id, status);
    }
  }
  return res;
}
static int ble_advertising_set_enable_Cb(sd_bus_message *m, void *userdata, sd_bus_error *ret_error)
{
  uint8_t advertiser_id;
  int enable; //bool
  uint8_t status;
  ////////////////////////////
  ALOGD(LOGTAG "(%s) enter", __func__);
  int rtn, res;
  res = sd_bus_reply_method_return(m, nullptr);
  rtn = sd_bus_message_read(m, "yiy", &advertiser_id, &enable, &status);
  if (rtn < 0)
  {
    ALOGE(LOGTAG "::%s Failed to read parameters: %d - %s\n", __func__, -rtn, strerror(-rtn));
  }
  else
  {
    ALOGD(LOGTAG "(%s) advertiser_id: %d enable: %d, status: %d", __func__, advertiser_id, enable, status);

    if (sGattExtCallbacks)
    {
      sGattExtCallbacks->ble_advertising_set_enable_Cb(advertiser_id, (bool)enable, status);
    }
  }
  return res;
}
static int ble_advertising_parameters_updated_cb(sd_bus_message *m, void *userdata, sd_bus_error *ret_error)
{
  uint8_t advertiser_id;
  uint8_t status;
  int tx_power;
  ////////////////////////////
  ALOGD(LOGTAG "(%s) enter", __func__);
  int rtn, res;
  res = sd_bus_reply_method_return(m, nullptr);
  rtn = sd_bus_message_read(m, "yyi", &advertiser_id, &status, &status);
  if (rtn < 0)
  {
    ALOGE(LOGTAG "::%s Failed to read parameters: %d - %s\n", __func__, -rtn, strerror(-rtn));
  }
  else
  {
    ALOGD(LOGTAG "(%s) advertiser_id: %d, tx_power: %d, status: %d,",
          __func__, advertiser_id, tx_power, status);

    if (sGattExtCallbacks)
    {
      sGattExtCallbacks->ble_advertising_parameters_updated_cb(advertiser_id, status, tx_power);
    }
  }
  return res;
}
static int ble_periodic_advertising_set_enable_Cb(sd_bus_message *m, void *userdata, sd_bus_error *ret_error)
{
  uint8_t advertiser_id;
  int enable; //bool
  uint8_t status;
  ////////////////////////////
  ALOGD(LOGTAG "(%s) enter", __func__);
  int rtn, res;
  res = sd_bus_reply_method_return(m, nullptr);
  rtn = sd_bus_message_read(m, "yiy", &advertiser_id, &enable, &status);
  if (rtn < 0)
  {
    ALOGE(LOGTAG "::%s Failed to read parameters: %d - %s\n", __func__, -rtn, strerror(-rtn));
  }
  else
  {
    ALOGD(LOGTAG "(%s) advertiser_id: %d enable: %d, status: %d",
          __func__, advertiser_id, enable, status);

    if (sGattExtCallbacks)
    {
      sGattExtCallbacks->ble_periodic_advertising_set_enable_Cb(advertiser_id, (bool)enable, status);
    }
  }
  return res;
}
/**
 * GattNativeInterfaceV2bImpl implementation for layer B
 */


/**
 * Native Client functions
 */
void GattNativeInterfaceV2bImpl::init(const btgatt_native_interface_callbacks_t *btgatt_native_interface_callbacks_a)
{
  ALOGD(LOGTAG "(%s) enter", __func__);

  if (btgatt_native_interface_callbacks_a)
  {
    ALOGD(LOGTAG "(%s) set callbacks", __func__);
    btgatt_native_interface_callbacks = btgatt_native_interface_callbacks_a;
    if (btgatt_native_interface_callbacks->btgatt_callbacks)
    {
      ALOGD(LOGTAG "(%s) set callbacks - scanner", __func__);
      sGattScannerCallbacks = btgatt_native_interface_callbacks->btgatt_callbacks->scanner;
      ALOGD(LOGTAG "(%s) set callbacks - client", __func__);
      sGattClientCallbacks = btgatt_native_interface_callbacks->btgatt_callbacks->client;
      ALOGD(LOGTAG "(%s) set callbacks - server", __func__);
      sGattServerCallbacks = btgatt_native_interface_callbacks->btgatt_callbacks->server;
    }
    ALOGD(LOGTAG "(%s) set callbacks - extension", __func__);
    sGattExtCallbacks = btgatt_native_interface_callbacks->extension;
  }

  // initialize dbus
  if (!open_sdbus_ipc())
  {
    ALOGE(LOGTAG "(%s) Failed to open sd-bus", __func__);
    return;
  }

  if (!initSDBus())
  {
    ALOGE(LOGTAG "(%s) Failed to init sd-bus!!", __func__);
    return;
  }
  ALOGD(LOGTAG "(%s) Success to setup sd-bus", __func__);
}

void GattNativeInterfaceV2bImpl::deinit(void)
{
  // deinitialize dbus
  close_sdbus_ipc();

  ALOGD(LOGTAG "(%s) Success to cleanup sd-bus", __func__);
}

bool GattNativeInterfaceV2bImpl::initSDBus(void)
{
  pthread_t t_id;

  // Create DBus interface for Adapter functionalities
  static const sd_bus_vtable dbus_vtable[] = {
      SD_BUS_VTABLE_START(0),

      SD_BUS_METHOD("register_scanner_cb", "syy", nullptr, register_scanner_cb, SD_BUS_VTABLE_UNPRIVILEGED),
      SD_BUS_METHOD("scan_params_cmpl_cb", "yy", nullptr, scan_params_cmpl_cb, SD_BUS_VTABLE_UNPRIVILEGED),

      SD_BUS_METHOD("scan_result_cb", "qysyyyiiqay", nullptr, scan_result_cb, SD_BUS_VTABLE_UNPRIVILEGED),
      SD_BUS_METHOD("batchscan_reports_cb", "iiiiay", nullptr, batchscan_reports_cb, SD_BUS_VTABLE_UNPRIVILEGED),
      SD_BUS_METHOD("batchscan_threshold_cb", "i", nullptr, batchscan_threshold_cb, SD_BUS_VTABLE_UNPRIVILEGED),
      SD_BUS_METHOD("track_adv_event_cb", "yyyyyyiqsyayyay", nullptr, track_adv_event_cb, SD_BUS_VTABLE_UNPRIVILEGED),

      SD_BUS_METHOD("btgattc_register_app_cb", "iis", nullptr, btgattc_register_app_cb, SD_BUS_VTABLE_UNPRIVILEGED),
      SD_BUS_METHOD("btgattc_open_cb", "iiis", nullptr, btgattc_open_cb, SD_BUS_VTABLE_UNPRIVILEGED),
      SD_BUS_METHOD("btgattc_close_cb", "iiis", nullptr, btgattc_close_cb, SD_BUS_VTABLE_UNPRIVILEGED),
      SD_BUS_METHOD("btgattc_search_complete_cb", "ii", nullptr, btgattc_search_complete_cb, SD_BUS_VTABLE_UNPRIVILEGED),
      SD_BUS_METHOD("btgattc_register_for_notification_cb", "iiiq", nullptr, btgattc_register_for_notification_cb, SD_BUS_VTABLE_UNPRIVILEGED),
      SD_BUS_METHOD("btgattc_notify_cb", "iaysqqy", nullptr, btgattc_notify_cb, SD_BUS_VTABLE_UNPRIVILEGED),
      SD_BUS_METHOD("btgattc_read_characteristic_cb", "iiqayqqy", nullptr, btgattc_read_characteristic_cb, SD_BUS_VTABLE_UNPRIVILEGED),
      SD_BUS_METHOD("btgattc_write_characteristic_cb", "iiq", nullptr, btgattc_write_characteristic_cb, SD_BUS_VTABLE_UNPRIVILEGED),
      SD_BUS_METHOD("btgattc_execute_write_cb", "ii", nullptr, btgattc_execute_write_cb, SD_BUS_VTABLE_UNPRIVILEGED),
      SD_BUS_METHOD("btgattc_read_descriptor_cb", "iiqayqqy", nullptr, btgattc_read_descriptor_cb, SD_BUS_VTABLE_UNPRIVILEGED),
      SD_BUS_METHOD("btgattc_write_descriptor_cb", "iiq", nullptr, btgattc_write_descriptor_cb, SD_BUS_VTABLE_UNPRIVILEGED),
      SD_BUS_METHOD("btgattc_remote_rssi_cb", "isii", nullptr, btgattc_remote_rssi_cb, SD_BUS_VTABLE_UNPRIVILEGED),
      SD_BUS_METHOD("btgattc_configure_mtu_cb", "iii", nullptr, btgattc_configure_mtu_cb, SD_BUS_VTABLE_UNPRIVILEGED),
      SD_BUS_METHOD("btgattc_congestion_cb", "ii", nullptr, btgattc_congestion_cb, SD_BUS_VTABLE_UNPRIVILEGED),
      SD_BUS_METHOD("btgattc_get_gatt_db_cb", "iia(qsiqqqyq)", nullptr, btgattc_get_gatt_db_cb, SD_BUS_VTABLE_UNPRIVILEGED),
      SD_BUS_METHOD("btgattc_phy_updated_cb", "iyyy", nullptr, btgattc_phy_updated_cb, SD_BUS_VTABLE_UNPRIVILEGED),
      SD_BUS_METHOD("btgattc_conn_updated_cb", "iqqqy", nullptr, btgattc_conn_updated_cb, SD_BUS_VTABLE_UNPRIVILEGED),
      SD_BUS_METHOD("readClientPhyCb", "ysyyy", nullptr, readClientPhyCb, SD_BUS_VTABLE_UNPRIVILEGED),
      SD_BUS_METHOD("btgatts_register_app_cb", "iis", nullptr, btgatts_register_app_cb, SD_BUS_VTABLE_UNPRIVILEGED),
      SD_BUS_METHOD("btgatts_connection_cb", "iiis", nullptr, btgatts_connection_cb, SD_BUS_VTABLE_UNPRIVILEGED),
      SD_BUS_METHOD("btgatts_service_added_cb", "iia(qsiqqqyq)", nullptr, btgatts_service_added_cb, SD_BUS_VTABLE_UNPRIVILEGED),
      SD_BUS_METHOD("btgatts_service_stopped_cb", "iii", nullptr, btgatts_service_stopped_cb, SD_BUS_VTABLE_UNPRIVILEGED),
      SD_BUS_METHOD("btgatts_service_deleted_cb", "iii", nullptr, btgatts_service_deleted_cb, SD_BUS_VTABLE_UNPRIVILEGED),
      SD_BUS_METHOD("btgatts_request_read_characteristic_cb", "iisiii", nullptr, btgatts_request_read_characteristic_cb, SD_BUS_VTABLE_UNPRIVILEGED),
      SD_BUS_METHOD("btgatts_request_read_descriptor_cb", "iisiii", nullptr, btgatts_request_read_descriptor_cb, SD_BUS_VTABLE_UNPRIVILEGED),
      SD_BUS_METHOD("btgatts_request_write_characteristic_cb", "iisiiiiay", nullptr, btgatts_request_write_characteristic_cb, SD_BUS_VTABLE_UNPRIVILEGED),
      SD_BUS_METHOD("btgatts_request_write_descriptor_cb", "iisiiiiay", nullptr, btgatts_request_write_descriptor_cb, SD_BUS_VTABLE_UNPRIVILEGED),
      SD_BUS_METHOD("btgatts_request_exec_write_cb", "iisi", nullptr, btgatts_request_exec_write_cb, SD_BUS_VTABLE_UNPRIVILEGED),
      SD_BUS_METHOD("btgatts_response_confirmation_cb", "ii", nullptr, btgatts_response_confirmation_cb, SD_BUS_VTABLE_UNPRIVILEGED),
      SD_BUS_METHOD("btgatts_indication_sent_cb", "ii", nullptr, btgatts_indication_sent_cb, SD_BUS_VTABLE_UNPRIVILEGED),
      SD_BUS_METHOD("btgatts_congestion_cb", "ii", nullptr, btgatts_congestion_cb, SD_BUS_VTABLE_UNPRIVILEGED),
      SD_BUS_METHOD("btgatts_mtu_changed_cb", "ii", nullptr, btgatts_mtu_changed_cb, SD_BUS_VTABLE_UNPRIVILEGED),
      SD_BUS_METHOD("btgatts_phy_updated_cb", "iyyy", nullptr, btgatts_phy_updated_cb, SD_BUS_VTABLE_UNPRIVILEGED),
      SD_BUS_METHOD("btgatts_conn_updated_cb", "iqqqy", nullptr, btgatts_conn_updated_cb, SD_BUS_VTABLE_UNPRIVILEGED),
      SD_BUS_METHOD("readServerPhyCb", "ysyyy", nullptr, readServerPhyCb, SD_BUS_VTABLE_UNPRIVILEGED),
      SD_BUS_METHOD("scan_filter_param_cb", "yyyy", nullptr, scan_filter_param_cb, SD_BUS_VTABLE_UNPRIVILEGED),
      SD_BUS_METHOD("scan_filter_status_cb", "yyy", nullptr, scan_filter_status_cb, SD_BUS_VTABLE_UNPRIVILEGED),
      SD_BUS_METHOD("batchscan_cfg_storage_cb", "yy", nullptr, batchscan_cfg_storage_cb, SD_BUS_VTABLE_UNPRIVILEGED),
      SD_BUS_METHOD("batchscan_start_cb", "yy", nullptr, batchscan_start_cb, SD_BUS_VTABLE_UNPRIVILEGED),
      SD_BUS_METHOD("batchscan_stop_cb", "yy", nullptr, batchscan_stop_cb, SD_BUS_VTABLE_UNPRIVILEGED),
      SD_BUS_METHOD("onSyncStarted", "iyqyysyq", nullptr, onSyncStarted, SD_BUS_VTABLE_UNPRIVILEGED),
      SD_BUS_METHOD("onSyncLost", "q", nullptr, onSyncLost, SD_BUS_VTABLE_UNPRIVILEGED),
      SD_BUS_METHOD("onSyncReport", "qiiyay", nullptr, onSyncReport, SD_BUS_VTABLE_UNPRIVILEGED),
      SD_BUS_METHOD("onSetAdvertisingData", "yy", nullptr, onSetAdvertisingData, SD_BUS_VTABLE_UNPRIVILEGED),
      SD_BUS_METHOD("onSetScanResponseData", "yy", nullptr, onSetScanResponseData, SD_BUS_VTABLE_UNPRIVILEGED),
      SD_BUS_METHOD("onSetPeriodicAdvertisingParameters", "yy", nullptr, onSetPeriodicAdvertisingParameters, SD_BUS_VTABLE_UNPRIVILEGED),
      SD_BUS_METHOD("onSetPeriodicAdvertisingData", "yy", nullptr, onSetPeriodicAdvertisingData, SD_BUS_VTABLE_UNPRIVILEGED),
      SD_BUS_METHOD("getOwnAddressCb", "yys", nullptr, getOwnAddressCb, SD_BUS_VTABLE_UNPRIVILEGED),
      SD_BUS_METHOD("ble_advertising_set_started_cb", "iyiy", nullptr, ble_advertising_set_started_cb, SD_BUS_VTABLE_UNPRIVILEGED),
      SD_BUS_METHOD("ble_advertising_set_timeout_cb", "yy", nullptr, ble_advertising_set_timeout_cb, SD_BUS_VTABLE_UNPRIVILEGED),
      SD_BUS_METHOD("ble_advertising_set_enable_Cb", "yiy", nullptr, ble_advertising_set_enable_Cb, SD_BUS_VTABLE_UNPRIVILEGED),
      SD_BUS_METHOD("ble_advertising_parameters_updated_cb", "yyi", nullptr, ble_advertising_parameters_updated_cb, SD_BUS_VTABLE_UNPRIVILEGED),
      SD_BUS_METHOD("ble_periodic_advertising_set_enable_Cb", "yiy", nullptr, ble_periodic_advertising_set_enable_Cb, SD_BUS_VTABLE_UNPRIVILEGED),
      SD_BUS_VTABLE_END};

  if (g_sdbus)
  {
    int r = sd_bus_add_object_vtable(g_sdbus,
                                     NULL,
                                     DBUS_OBJ_PATH, // object path
                                     DBUS_IF_NAME,  // interface name
                                     dbus_vtable,
                                     NULL);
    if (r < 0)
    {
      ALOGE(LOGTAG "::%s VTable creation failed and %d - %s", __func__, -r, strerror(-r));
      return false;
    }
  }
  else
  {
    ALOGE(LOGTAG "::%s No dbus connection.", __func__);
    return false;
  }

  pthread_create(&t_id, NULL, process_dbus_request, NULL);
  ALOGD(LOGTAG "::%s Start a thread to process dbus request", __func__);

  return true;
}

void *GattNativeInterfaceV2bImpl::process_dbus_request(void *ptr)
{
  int rtn;
  struct pollfd fds[2];

  fds[0].fd = g_stop_dbus_fd;
  fds[0].events = POLLIN;

  fds[1].fd = sd_bus_get_fd(g_sdbus);
  fds[1].events = POLLIN;

  while (g_dbus_running)
  {
    rtn = sd_bus_process(g_sdbus, NULL);
    if (rtn < 0)
    {
      ALOGE(LOGTAG "::%s Error : sd_bus_process() : %d", __func__, rtn);
      break;
    }

    if (rtn > 0)
      continue;

    poll(fds, 2, -1);
    if (fds[0].revents & POLLIN)
    {
      ALOGD(LOGTAG "::%s Stop triggered, exiting process system bus", __func__);
      break;
    }
  }
  ALOGD(LOGTAG "::%s exit", __func__);
}

/////////////////////////////////////////////////
//MACROS for quick coding
/////////////////////////////////////////////////

const char *__gatt_ipc_service =        "com.qualcomm.qti.adk.btipc.daemon";
const char *__gatt_ipc_service_object = "/com/qualcomm/qti/adk/btipc/daemon/gatt";
const char *__gatt_ipc_service_name =   "com.qualcomm.qti.adk.btipc.daemon.gatt";

#define __gatt_message_release(rtn, m)                                                      \
  {                                                                                         \
    if (rtn < 0)                                                                            \
    {                                                                                       \
      ALOGE(LOGTAG "(%s) Failed to call method : %d - %s", __func__, -rtn, strerror(-rtn)); \
    }                                                                                       \
    if (m != NULL)                                                                          \
      sd_bus_message_unref(m);                                                              \
  }

#define __primary_params                                  \
  g_sdbus_call,                                           \
      __gatt_ipc_service,        /* service to contact */ \
      __gatt_ipc_service_object, /* object path */        \
      __gatt_ipc_service_name    /* interface name */

////////////////////////////////////////////////////

int GattNativeInterfaceV2bImpl ::gattClientGetDeviceTypeNative(string address)
{
  ALOGD(LOGTAG "(%s) enter!", __func__);
  // implement IPC call here by passing the parameters unchanged
  sd_bus_message *m = NULL;
  int rtn, result = -1;
  rtn = sd_bus_call_method(__primary_params, __func__,
                           NULL,             /* object to return error in */
                           &m,               /* return message on success */
                           "s",              /* input signature */
                           address.c_str()); /* parameters */

  if (rtn < 0)
  {
    ALOGD(LOGTAG "(%s) rtn:%d", __func__, rtn);
    goto finish;
  }

  rtn = sd_bus_message_read(m, "i", &result);
  if (rtn < 0)
  {
    ALOGE(LOGTAG "::%s Failed to read parameters: %d - %s\n", __func__, -rtn, strerror(-rtn));
  }

finish:
  if (m != NULL)
    sd_bus_message_unref(m);
  return result;
}

void GattNativeInterfaceV2bImpl ::gattClientRegisterAppNative(btapp::Uuid uuid)
{
  ALOGD(LOGTAG "(%s) enter!", __func__);
  // implement IPC call here by passing the parameters unchanged
  sd_bus_message *m = NULL;
  int rtn;
  rtn = sd_bus_call_method(__primary_params, __func__,
                           NULL,                     /* object to return error in */
                           &m,                       /* return message on success */
                           "s",                      /* input signature */
                           uuid.ToString().c_str()); /* parameters */

  __gatt_message_release(rtn, m);
}

void GattNativeInterfaceV2bImpl ::gattClientUnregisterAppNative(int clientIf)
{

  ALOGD(LOGTAG "(%s) enter!", __func__);
  // implement IPC call here by passing the parameters unchanged
  sd_bus_message *m = NULL;
  int rtn;
  rtn = sd_bus_call_method(__primary_params, __func__,
                           NULL,      /* object to return error in */
                           &m,        /* return message on success */
                           "i",       /* input signature */
                           clientIf); /* parameters */

  __gatt_message_release(rtn, m);
}

void GattNativeInterfaceV2bImpl ::registerScannerNative(btapp::Uuid uuid)
{

  ALOGD(LOGTAG "(%s) enter!, uuid: %s", __func__, uuid.ToString().c_str());
  // implement IPC call here by passing the parameters unchanged
  sd_bus_message *m = NULL;
  int rtn;
  rtn = sd_bus_call_method(__primary_params, __func__,
                           NULL,                     /* object to return error in */
                           &m,                       /* return message on success */
                           "s",                      /* input signature */
                           uuid.ToString().c_str()); /* parameters */

  ALOGD(LOGTAG "(%s) exit!, uuid: %s", __func__, uuid.ToString().c_str());
  __gatt_message_release(rtn, m);
}

void GattNativeInterfaceV2bImpl ::unregisterScannerNative(int scanner_id)
{
  ALOGD(LOGTAG "(%s) enter!", __func__);
  // implement IPC call here by passing the parameters unchanged
  sd_bus_message *m = NULL;
  int rtn;
  rtn = sd_bus_call_method(__primary_params, __func__,
                           NULL,        /* object to return error in */
                           &m,          /* return message on success */
                           "i",         /* input signature */
                           scanner_id); /* parameters */

  __gatt_message_release(rtn, m);
}

void GattNativeInterfaceV2bImpl ::gattClientScanNative(bool start)
{
  ALOGD(LOGTAG "(%s) enter!", __func__);
  // implement IPC call here by passing the parameters unchanged
  sd_bus_message *m = NULL;
  int rtn;
  rtn = sd_bus_call_method(__primary_params, __func__,
                           NULL,   /* object to return error in */
                           &m,     /* return message on success */
                           "i",    /* input signature */
                           (int)start); /* parameters */

  __gatt_message_release(rtn, m);
}

void GattNativeInterfaceV2bImpl ::gattClientConnectNative(int clientif, string address, bool isDirect,
                                                          int transport, bool opportunistic,
                                                          int initiating_phys)
{
  ALOGD(LOGTAG "(%s) enter!", __func__);
  // implement IPC call here by passing the parameters unchanged
  sd_bus_message *m = NULL;
  int rtn;
  rtn = sd_bus_call_method(__primary_params, __func__,
                           NULL,                                                                            /* object to return error in */
                           &m,                                                                              /* return message on success */
                           "isiiii",                                                                        /* input signature */
                           clientif, address.c_str(), (int)isDirect, transport, (int)opportunistic, initiating_phys); /* parameters */

  __gatt_message_release(rtn, m);
}

void GattNativeInterfaceV2bImpl ::gattClientDisconnectNative(int clientIf, string address,
                                                             int conn_id)
{
  ALOGD(LOGTAG "(%s) enter!", __func__);
  // implement IPC call here by passing the parameters unchanged
  sd_bus_message *m = NULL;
  int rtn;
  rtn = sd_bus_call_method(__primary_params, __func__,
                           NULL,                                /* object to return error in */
                           &m,                                  /* return message on success */
                           "isi",                               /* input signature */
                           clientIf, address.c_str(), conn_id); /* parameters */

  __gatt_message_release(rtn, m);
}

void GattNativeInterfaceV2bImpl ::gattClientSetPreferredPhyNative(int clientIf, string address,
                                                                  int tx_phy, int rx_phy,
                                                                  int phy_options)
{
  ALOGD(LOGTAG "(%s) enter!", __func__);
  // implement IPC call here by passing the parameters unchanged
  sd_bus_message *m = NULL;
  int rtn;
  rtn = sd_bus_call_method(__primary_params, __func__,
                           NULL,                                                    /* object to return error in */
                           &m,                                                      /* return message on success */
                           "isiii",                                                 /* input signature */
                           clientIf, address.c_str(), tx_phy, rx_phy, phy_options); /* parameters */

  __gatt_message_release(rtn, m);
}

void GattNativeInterfaceV2bImpl ::gattClientReadPhyNative(int clientIf, string address)
{
  ALOGD(LOGTAG "(%s) enter!", __func__);
  // implement IPC call here by passing the parameters unchanged
  sd_bus_message *m = NULL;
  int rtn;
  rtn = sd_bus_call_method(__primary_params, __func__,
                           NULL,                       /* object to return error in */
                           &m,                         /* return message on success */
                           "is",                       /* input signature */
                           clientIf, address.c_str()); /* parameters */

  __gatt_message_release(rtn, m);
}

void GattNativeInterfaceV2bImpl ::gattClientRefreshNative(int clientIf, string address)
{
  ALOGD(LOGTAG "(%s) enter!", __func__);
  // implement IPC call here by passing the parameters unchanged
  sd_bus_message *m = NULL;
  int rtn;
  rtn = sd_bus_call_method(__primary_params, __func__,
                           NULL,                       /* object to return error in */
                           &m,                         /* return message on success */
                           "is",                       /* input signature */
                           clientIf, address.c_str()); /* parameters */

  __gatt_message_release(rtn, m);
}

void GattNativeInterfaceV2bImpl ::gattClientSearchServiceNative(int conn_id, bool search_all,
                                                                btapp::Uuid uuid)
{
  ALOGD(LOGTAG "(%s) enter!", __func__);
  // implement IPC call here by passing the parameters unchanged
  sd_bus_message *m = NULL;
  int rtn;
  rtn = sd_bus_call_method(__primary_params, __func__,
                           NULL,                                          /* object to return error in */
                           &m,                                            /* return message on success */
                           "iis",                                         /* input signature */
                           conn_id, (int)search_all, uuid.ToString().c_str()); /* parameters */

  __gatt_message_release(rtn, m);
}

void GattNativeInterfaceV2bImpl ::gattClientDiscoverServiceByUuidNative(int conn_id, btapp::Uuid uuid)
{
  ALOGD(LOGTAG "(%s) enter!", __func__);
  // implement IPC call here by passing the parameters unchanged
  sd_bus_message *m = NULL;
  int rtn;
  rtn = sd_bus_call_method(__primary_params, __func__,
                           NULL,                              /* object to return error in */
                           &m,                                /* return message on success */
                           "is",                              /* input signature */
                           conn_id, uuid.ToString().c_str()); /* parameters */

  __gatt_message_release(rtn, m);
}

void GattNativeInterfaceV2bImpl ::gattClientGetGattDbNative(int conn_id)
{
  ALOGD(LOGTAG "(%s) enter!", __func__);
  // implement IPC call here by passing the parameters unchanged
  sd_bus_message *m = NULL;
  int rtn;
  rtn = sd_bus_call_method(__primary_params, __func__,
                           NULL,     /* object to return error in */
                           &m,       /* return message on success */
                           "i",      /* input signature */
                           conn_id); /* parameters */

  __gatt_message_release(rtn, m);
}

void GattNativeInterfaceV2bImpl ::gattClientReadCharacteristicNative(int conn_id, int handle,
                                                                     int authReq)
{
  ALOGD(LOGTAG "(%s) enter!", __func__);
  // implement IPC call here by passing the parameters unchanged
  sd_bus_message *m = NULL;
  int rtn;
  rtn = sd_bus_call_method(__primary_params, __func__,
                           NULL,                      /* object to return error in */
                           &m,                        /* return message on success */
                           "iii",                     /* input signature */
                           conn_id, handle, authReq); /* parameters */

  __gatt_message_release(rtn, m);
}

void GattNativeInterfaceV2bImpl ::gattClientReadUsingCharacteristicUuidNative(
    int conn_id, btapp::Uuid uuid, int s_handle, int e_handle, int authReq)
{
  ALOGD(LOGTAG "(%s) enter!", __func__);
  // implement IPC call here by passing the parameters unchanged
  sd_bus_message *m = NULL;
  int rtn;
  rtn = sd_bus_call_method(__primary_params, __func__,
                           NULL,                                                           /* object to return error in */
                           &m,                                                             /* return message on success */
                           "isiii",                                                        /* input signature */
                           conn_id, uuid.ToString().c_str(), s_handle, e_handle, authReq); /* parameters */

  __gatt_message_release(rtn, m);
}

void GattNativeInterfaceV2bImpl ::gattClientReadDescriptorNative(int conn_id, int handle,
                                                                 int authReq)
{
  ALOGD(LOGTAG "(%s) enter!", __func__);
  // implement IPC call here by passing the parameters unchanged
  sd_bus_message *m = NULL;
  int rtn;
  rtn = sd_bus_call_method(__primary_params, __func__,
                           NULL,                      /* object to return error in */
                           &m,                        /* return message on success */
                           "iii",                     /* input signature */
                           conn_id, handle, authReq); /* parameters */

  __gatt_message_release(rtn, m);
}

void GattNativeInterfaceV2bImpl ::gattClientWriteCharacteristicNative(int conn_id, int handle,
                                                                      int write_type, int auth_req,
                                                                      std::vector<uint8_t> vect_val)
{

  ALOGD(LOGTAG "(%s) enter", __func__);
  // implement IPC call here by passing the parameters unchanged
  int rtn;
  sd_bus_message *m = NULL;

  rtn = sd_bus_message_new_method_call(g_sdbus_call,
                                       &m,
                                       __gatt_ipc_service,        /* service to contact */
                                       __gatt_ipc_service_object, /* object path */
                                       __gatt_ipc_service_name,   /* interface name */
                                       __func__);                 /* method name */
  if (rtn < 0)
  {
    ALOGE(LOGTAG "(%s) Failed to create new method call : %d - %s", __func__, -rtn, strerror(-rtn));
    goto finish;
  }

  rtn = sd_bus_message_append(m, "iiii", conn_id, handle, write_type, auth_req);
  if (rtn < 0)
  {
    ALOGE(LOGTAG "(%s) Failed to append data: %d - %s", __func__, -rtn, strerror(-rtn));
    goto finish;
  }

  WRITE_SDBUS_ARRAY_FROM_VECTOR(m, uint8_t, SD_BUS_TYPE_BYTE, vect_val);

  rtn = sd_bus_call(g_sdbus_call, m, -1, nullptr, nullptr);
  if (rtn < 0)
  {
    ALOGE(LOGTAG "(%s) Failed to call: %d - %s", __func__, -rtn, strerror(-rtn));
  }

finish:
  if (m != NULL)
    sd_bus_message_unref(m);
  return;
}

void GattNativeInterfaceV2bImpl ::gattClientExecuteWriteNative(int conn_id, bool execute)
{
  ALOGD(LOGTAG "(%s) enter!", __func__);
  // implement IPC call here by passing the parameters unchanged
  sd_bus_message *m = NULL;
  int rtn;
  rtn = sd_bus_call_method(__primary_params, __func__,
                           NULL,              /* object to return error in */
                           &m,                /* return message on success */
                           "ii",              /* input signature */
                           conn_id, (int)execute); /* parameters */

  __gatt_message_release(rtn, m);
}

void GattNativeInterfaceV2bImpl ::gattClientWriteDescriptorNative(int conn_id, int handle,
                                                                  int auth_req, std::vector<uint8_t> vect_val)
{
  ALOGD(LOGTAG "(%s) enter", __func__);
  // implement IPC call here by passing the parameters unchanged
  int rtn;
  sd_bus_message *m = NULL;

  rtn = sd_bus_message_new_method_call(g_sdbus_call,
                                       &m,
                                       __gatt_ipc_service,        /* service to contact */
                                       __gatt_ipc_service_object, /* object path */
                                       __gatt_ipc_service_name,   /* interface name */
                                       __func__);                 /* method name */
  if (rtn < 0)
  {
    ALOGE(LOGTAG "(%s) Failed to create new method call : %d - %s", __func__, -rtn, strerror(-rtn));
    goto finish;
  }

  rtn = sd_bus_message_append(m, "iii", conn_id, handle, auth_req);
  if (rtn < 0)
  {
    ALOGE(LOGTAG "(%s) Failed to append data: %d - %s", __func__, -rtn, strerror(-rtn));
    goto finish;
  }

  WRITE_SDBUS_ARRAY_FROM_VECTOR(m, uint8_t, SD_BUS_TYPE_BYTE, vect_val);

  rtn = sd_bus_call(g_sdbus_call, m, -1, nullptr, nullptr);
  if (rtn < 0)
  {
    ALOGE(LOGTAG "(%s) Failed to call: %d - %s", __func__, -rtn, strerror(-rtn));
  }

finish:
  if (m != NULL)
    sd_bus_message_unref(m);
  return;
}

void GattNativeInterfaceV2bImpl ::gattClientRegisterForNotificationsNative(
    int clientIf, string address, int handle, bool enable)
{

  ALOGD(LOGTAG "(%s) enter!", __func__);
  // implement IPC call here by passing the parameters unchanged
  sd_bus_message *m = NULL;
  int rtn;
  rtn = sd_bus_call_method(__primary_params, __func__,
                           NULL,                                       /* object to return error in */
                           &m,                                         /* return message on success */
                           "isii",                                     /* input signature */
                           clientIf, address.c_str(), handle, (int)enable); /* parameters */

  __gatt_message_release(rtn, m);
}

void GattNativeInterfaceV2bImpl ::gattClientReadRemoteRssiNative(int clientif, string address)
{

  ALOGD(LOGTAG "(%s) enter!", __func__);
  // implement IPC call here by passing the parameters unchanged
  sd_bus_message *m = NULL;
  int rtn;
  rtn = sd_bus_call_method(__primary_params, __func__,
                           NULL,                       /* object to return error in */
                           &m,                         /* return message on success */
                           "is",                       /* input signature */
                           clientif, address.c_str()); /* parameters */

  __gatt_message_release(rtn, m);
}

void GattNativeInterfaceV2bImpl ::gattSetScanParametersNative(int client_if, int scan_phy,
                                                              std::vector<uint32_t> scan_interval,
                                                              std::vector<uint32_t> scan_window)
{

  ALOGD(LOGTAG "(%s) enter", __func__);
  // implement IPC call here by passing the parameters unchanged
  int rtn;
  sd_bus_message *m = NULL;

  rtn = sd_bus_message_new_method_call(g_sdbus_call,
                                       &m,
                                       __gatt_ipc_service,        /* service to contact */
                                       __gatt_ipc_service_object, /* object path */
                                       __gatt_ipc_service_name,   /* interface name */
                                       __func__);                 /* method name */
  if (rtn < 0)
  {
    ALOGE(LOGTAG "(%s) Failed to create new method call : %d - %s", __func__, -rtn, strerror(-rtn));
    goto finish;
  }

  rtn = sd_bus_message_append(m, "ii", client_if, scan_phy);
  if (rtn < 0)
  {
    ALOGE(LOGTAG "(%s) Failed to append data: %d - %s", __func__, -rtn, strerror(-rtn));
    goto finish;
  }

  WRITE_SDBUS_ARRAY_FROM_VECTOR(m, uint32_t, SD_BUS_TYPE_UINT32, scan_interval);
  WRITE_SDBUS_ARRAY_FROM_VECTOR(m, uint32_t, SD_BUS_TYPE_UINT32, scan_window);

  rtn = sd_bus_call(g_sdbus_call, m, -1, nullptr, nullptr);
  if (rtn < 0)
  {
    ALOGE(LOGTAG "(%s) Failed to call: %d - %s", __func__, -rtn, strerror(-rtn));
  }

finish:
  if (m != NULL)
    sd_bus_message_unref(m);
  return;
}

void GattNativeInterfaceV2bImpl ::getOwnAddressNative(
    int advertiser_id)
{
  ALOGD(LOGTAG "(%s) enter!", __func__);
  // implement IPC call here by passing the parameters unchanged
  sd_bus_message *m = NULL;
  int rtn;
  rtn = sd_bus_call_method(__primary_params, __func__,
                           NULL,           /* object to return error in */
                           &m,             /* return message on success */
                           "i",            /* input signature */
                           advertiser_id); /* parameters */

  __gatt_message_release(rtn, m);
}

void GattNativeInterfaceV2bImpl ::gattClientScanFilterParamAddNative(uint8_t client_if,
                                                                     uint8_t filt_index, std::unique_ptr<btgatt_filt_param_setup_t> filt_params)
{

  ALOGD(LOGTAG "(%s) enter", __func__);
  // implement IPC call here by passing the parameters unchanged
  sd_bus_message *m = NULL;
  int rtn;
  rtn = sd_bus_call_method(__primary_params, __func__,
                           NULL,                  /* object to return error in */
                           &m,                    /* return message on success */
                           "yyqqyyyyqqyq",        /* input signature */
                           client_if, filt_index, /* parameters */
                           filt_params->feat_seln,
                           filt_params->list_logic_type,
                           filt_params->filt_logic_type,
                           filt_params->rssi_high_thres,
                           filt_params->rssi_low_thres,
                           filt_params->dely_mode,
                           filt_params->found_timeout,
                           filt_params->lost_timeout,
                           filt_params->found_timeout_cnt,
                           filt_params->num_of_tracking_entries);

  __gatt_message_release(rtn, m);
}

void GattNativeInterfaceV2bImpl ::gattClientScanFilterParamDeleteNative(uint8_t client_if, uint8_t filt_index)
{

  ALOGD(LOGTAG "(%s) enter!", __func__);
  // implement IPC call here by passing the parameters unchanged
  sd_bus_message *m = NULL;
  int rtn;
  rtn = sd_bus_call_method(__primary_params, __func__,
                           NULL,                   /* object to return error in */
                           &m,                     /* return message on success */
                           "yy",                   /* input signature */
                           client_if, filt_index); /* parameters */

  __gatt_message_release(rtn, m);
}

void GattNativeInterfaceV2bImpl ::gattClientScanFilterParamClearAllNative(uint8_t client_if)
{
  ALOGD(LOGTAG "(%s) enter!", __func__);
  // implement IPC call here by passing the parameters unchanged
  sd_bus_message *m = NULL;
  int rtn;
  rtn = sd_bus_call_method(__primary_params, __func__,
                           NULL,       /* object to return error in */
                           &m,         /* return message on success */
                           "y",        /* input signature */
                           client_if); /* parameters */

  __gatt_message_release(rtn, m);
}

void GattNativeInterfaceV2bImpl ::gattClientScanFilterAddNative(int client_if, int filter_index, std::vector<apcf_command_t> filters)
{

  ALOGD(LOGTAG "(%s) enter", __func__);
  // implement IPC call here by passing the parameters unchanged
  sd_bus_message *m = NULL;
  int rtn;

  rtn = sd_bus_message_new_method_call(g_sdbus_call,
                                       &m,
                                       __gatt_ipc_service,        /* service to contact */
                                       __gatt_ipc_service_object, /* object path */
                                       __gatt_ipc_service_name,   /* interface name */
                                       __func__);                 /* method name */
  if (rtn < 0)
  {
    ALOGE(LOGTAG "(%s) Failed to create new method call : %d - %s", __func__, -rtn, strerror(-rtn));
    goto finish;
  }

  rtn = sd_bus_message_append(m, "ii", client_if, filter_index);
  if (rtn < 0)
  {
    ALOGE(LOGTAG "(%s) Failed to append data: %d - %s", __func__, -rtn, strerror(-rtn));
    goto finish;
  }

  rtn = sd_bus_message_open_container(m, SD_BUS_TYPE_ARRAY, "(ysyssayqqayay)");
  if (rtn < 0)
  {
    ALOGE(LOGTAG "::%s Failed to open containner: %d - %s", __func__, -rtn, strerror(-rtn));
    goto finish;
  }

  for (const auto it : filters)
  {
    sd_bus_message_open_container(m, 'r', "ysyssayqqayay");

    rtn = sd_bus_message_append(m, "ysyss", it.type,
                                it.address.c_str(),
                                it.addr_type,
                                it.uuid.ToString().c_str(),
                                it.uuid_mask.ToString().c_str());
    if (rtn < 0)
    {
      ALOGE(LOGTAG "(%s) Failed to append data: %d - %s", __func__, -rtn, strerror(-rtn));
      goto finish;
    }

    WRITE_SDBUS_ARRAY_FROM_VECTOR(m, uint8_t, SD_BUS_TYPE_BYTE, it.name);

    rtn = sd_bus_message_append(m, "yy", it.company, it.company_mask);
    if (rtn < 0)
    {
      ALOGE(LOGTAG "(%s) Failed to append data: %d - %s", __func__, -rtn, strerror(-rtn));
      goto finish;
    }

    WRITE_SDBUS_ARRAY_FROM_VECTOR(m, uint8_t, SD_BUS_TYPE_BYTE, it.data);
    WRITE_SDBUS_ARRAY_FROM_VECTOR(m, uint8_t, SD_BUS_TYPE_BYTE, it.data_mask);

    sd_bus_message_close_container(m);
  }

  sd_bus_message_close_container(m);

  rtn = sd_bus_call(g_sdbus_call, m, -1, nullptr, nullptr);
  if (rtn < 0)
    ALOGE(LOGTAG "(%s) Failed to call: %d - %s", __func__, -rtn, strerror(-rtn));

finish:
  if (m != NULL)
    sd_bus_message_unref(m);
  return;
}

void GattNativeInterfaceV2bImpl ::gattClientScanFilterClearNative(int client_if, int filt_index)
{

  ALOGD(LOGTAG "(%s) enter!", __func__);
  // implement IPC call here by passing the parameters unchanged
  sd_bus_message *m = NULL;
  int rtn;
  rtn = sd_bus_call_method(__primary_params, __func__,
                           NULL,                   /* object to return error in */
                           &m,                     /* return message on success */
                           "ii",                   /* input signature */
                           client_if, filt_index); /* parameters */

  __gatt_message_release(rtn, m);
}

void GattNativeInterfaceV2bImpl ::gattClientScanFilterEnableNative(int client_if, bool enable)
{

  ALOGD(LOGTAG "(%s) enter!", __func__);
  // implement IPC call here by passing the parameters unchanged
  sd_bus_message *m = NULL;
  int rtn;
  rtn = sd_bus_call_method(__primary_params, __func__,
                           NULL,               /* object to return error in */
                           &m,                 /* return message on success */
                           "ii",               /* input signature */
                           client_if, (int)enable); /* parameters */

  __gatt_message_release(rtn, m);
}

void GattNativeInterfaceV2bImpl ::gattClientConfigureMTUNative(int conn_id, int mtu)
{
  ALOGD(LOGTAG "(%s) enter!", __func__);
  // implement IPC call here by passing the parameters unchanged
  sd_bus_message *m = NULL;
  int rtn;
  rtn = sd_bus_call_method(__primary_params, __func__,
                           NULL,          /* object to return error in */
                           &m,            /* return message on success */
                           "ii",          /* input signature */
                           conn_id, mtu); /* parameters */

  __gatt_message_release(rtn, m);
}

void GattNativeInterfaceV2bImpl ::gattConnectionParameterUpdateNative(
    int client_if, string address,
    int min_interval,
    int max_interval, int latency,
    int timeout, int min_ce_len,
    int max_ce_len)
{
  ALOGD(LOGTAG "(%s) enter!", __func__);
  // implement IPC call here by passing the parameters unchanged
  sd_bus_message *m = NULL;
  int rtn;
  rtn = sd_bus_call_method(__primary_params, __func__,
                           NULL,                                                                                              /* object to return error in */
                           &m,                                                                                                /* return message on success */
                           "isiiiiii",                                                                                        /* input signature */
                           client_if, address.c_str(), min_interval, max_interval, latency, timeout, min_ce_len, max_ce_len); /* parameters */

  __gatt_message_release(rtn, m);
}

void GattNativeInterfaceV2bImpl ::gattClientConfigBatchScanStorageNative(
    int client_if, int max_full_reports_percent,
    int max_trunc_reports_percent, int notify_threshold_level_percent)
{

  ALOGD(LOGTAG "(%s) enter!", __func__);
  // implement IPC call here by passing the parameters unchanged
  sd_bus_message *m = NULL;
  int rtn;
  rtn = sd_bus_call_method(__primary_params, __func__,
                           NULL,                                                                                            /* object to return error in */
                           &m,                                                                                              /* return message on success */
                           "iiii",                                                                                          /* input signature */
                           client_if, max_full_reports_percent, max_trunc_reports_percent, notify_threshold_level_percent); /* parameters */

  __gatt_message_release(rtn, m);
}

void GattNativeInterfaceV2bImpl ::gattClientStartBatchScanNative(
    int client_if, int scan_mode,
    int scan_interval_unit,
    int scan_window_unit,
    int addr_type, int discard_rule)
{
  ALOGD(LOGTAG "(%s) enter!", __func__);
  // implement IPC call here by passing the parameters unchanged
  sd_bus_message *m = NULL;
  int rtn;
  rtn = sd_bus_call_method(__primary_params, __func__,
                           NULL,                                                                                 /* object to return error in */
                           &m,                                                                                   /* return message on success */
                           "iiiiii",                                                                             /* input signature */
                           client_if, scan_mode, scan_interval_unit, scan_window_unit, addr_type, discard_rule); /* parameters */

  __gatt_message_release(rtn, m);
}

void GattNativeInterfaceV2bImpl ::gattClientStopBatchScanNative(int client_if)
{
  ALOGD(LOGTAG "(%s) enter!", __func__);
  // implement IPC call here by passing the parameters unchanged
  sd_bus_message *m = NULL;
  int rtn;
  rtn = sd_bus_call_method(__primary_params, __func__,
                           NULL,       /* object to return error in */
                           &m,         /* return message on success */
                           "i",        /* input signature */
                           client_if); /* parameters */

  __gatt_message_release(rtn, m);
}

void GattNativeInterfaceV2bImpl ::gattClientReadScanReportsNative(int client_if, int scan_type)
{
  ALOGD(LOGTAG "(%s) enter!", __func__);
  // implement IPC call here by passing the parameters unchanged
  sd_bus_message *m = NULL;
  int rtn;
  rtn = sd_bus_call_method(__primary_params, __func__,
                           NULL,                  /* object to return error in */
                           &m,                    /* return message on success */
                           "ii",                  /* input signature */
                           client_if, scan_type); /* parameters */

  __gatt_message_release(rtn, m);
}

/**
 * Native server functions
 */

void GattNativeInterfaceV2bImpl ::gattServerRegisterAppNative(btapp::Uuid uuid)
{
  ALOGD(LOGTAG "(%s) enter!!", __func__);
  // implement IPC call here by passing the parameters unchanged
  int rtn;
  sd_bus_message *m = NULL;
  rtn = sd_bus_call_method(__primary_params, __func__,
                           NULL,                     /* object to return error in */
                           &m,                       /* return message on success */
                           "s",                      /* input signature */
                           uuid.ToString().c_str()); /* parameters */

  if (rtn < 0)
    ALOGD(LOGTAG "(%s) send uuid:%s, rtn:%d - %s", __func__, uuid.ToString().c_str(), -rtn, strerror(-rtn));
  else
    ALOGD(LOGTAG "(%s) send uuid:%s, rtn:%d", __func__, uuid.ToString().c_str(), rtn);

  __gatt_message_release(rtn, m);
}

void GattNativeInterfaceV2bImpl ::gattServerUnregisterAppNative(int serverIf)
{
  ALOGD(LOGTAG "(%s) enter!", __func__);
  // implement IPC call here by passing the parameters unchanged
  sd_bus_message *m = NULL;
  int rtn;
  rtn = sd_bus_call_method(__primary_params, __func__,
                           NULL,      /* object to return error in */
                           &m,        /* return message on success */
                           "i",       /* input signature */
                           serverIf); /* parameters */

  __gatt_message_release(rtn, m);
}

void GattNativeInterfaceV2bImpl ::gattServerConnectNative(int server_if,
                                                          string address, bool is_direct,
                                                          int transport)
{
  ALOGD(LOGTAG "(%s) enter!", __func__);
  // implement IPC call here by passing the parameters unchanged
  sd_bus_message *m = NULL;
  int rtn;
  rtn = sd_bus_call_method(__primary_params, __func__,
                           NULL,                                              /* object to return error in */
                           &m,                                                /* return message on success */
                           "isii",                                            /* input signature */
                           server_if, address.c_str(), (int)is_direct, transport); /* parameters */

  __gatt_message_release(rtn, m);
}

void GattNativeInterfaceV2bImpl ::gattServerDisconnectNative(int serverIf, string address,
                                                             int conn_id)
{

  ALOGD(LOGTAG "(%s) enter!", __func__);
  // implement IPC call here by passing the parameters unchanged
  sd_bus_message *m = NULL;
  int rtn;
  rtn = sd_bus_call_method(__primary_params, __func__,
                           NULL,                                /* object to return error in */
                           &m,                                  /* return message on success */
                           "isi",                               /* input signature */
                           serverIf, address.c_str(), conn_id); /* parameters */

  __gatt_message_release(rtn, m);
}

void GattNativeInterfaceV2bImpl ::gattServerSetPreferredPhyNative(int serverIf, string address,
                                                                  int tx_phy, int rx_phy,
                                                                  int phy_options)
{
  ALOGD(LOGTAG "(%s) enter!", __func__);
  // implement IPC call here by passing the parameters unchanged
  sd_bus_message *m = NULL;
  int rtn;
  rtn = sd_bus_call_method(__primary_params, __func__,
                           NULL,                                                    /* object to return error in */
                           &m,                                                      /* return message on success */
                           "isiii",                                                 /* input signature */
                           serverIf, address.c_str(), tx_phy, rx_phy, phy_options); /* parameters */

  __gatt_message_release(rtn, m);
}

void GattNativeInterfaceV2bImpl ::gattServerReadPhyNative(int serverIf, string address)
{

  ALOGD(LOGTAG "(%s) enter!", __func__);
  // implement IPC call here by passing the parameters unchanged
  sd_bus_message *m = NULL;
  int rtn;
  rtn = sd_bus_call_method(__primary_params, __func__,
                           NULL,                       /* object to return error in */
                           &m,                         /* return message on success */
                           "is",                       /* input signature */
                           serverIf, address.c_str()); /* parameters */

  __gatt_message_release(rtn, m);
}
void GattNativeInterfaceV2bImpl ::gattServerAddServiceNative(int server_if, std::vector<gatt_db_element_t> service)
{

  ALOGD(LOGTAG "(%s) enter : service size - %d", __func__, service.size());

  sd_bus_message *m = NULL;
  int rtn;

  rtn = sd_bus_message_new_method_call(g_sdbus_call,
                                       &m,
                                       __gatt_ipc_service,        /* service to contact */
                                       __gatt_ipc_service_object, /* object path */
                                       __gatt_ipc_service_name,   /* interface name */
                                       __func__);                 /* method name */
  if (rtn < 0)
  {
    ALOGE(LOGTAG "(%s) Failed to create new method call : %d - %s", __func__, -rtn, strerror(-rtn));
    goto finish;
  }

  rtn = sd_bus_message_append(m, "i", server_if);
  if (rtn < 0)
  {
    ALOGE(LOGTAG "(%s) Failed to append data: %d - %s", __func__, -rtn, strerror(-rtn));
    goto finish;
  }

  rtn = sd_bus_message_open_container(m, SD_BUS_TYPE_ARRAY, "(qsiqqqyq)");
  if (rtn < 0)
  {
    ALOGE(LOGTAG "::%s Failed to open containner!: %d - %s", __func__, -rtn, strerror(-rtn));
    goto finish;
  }

  for (const auto it : service)
  {
    rtn = sd_bus_message_open_container(m, 'r', "qsiqqqyq");
    if (rtn < 0)
    {
      ALOGE(LOGTAG "(%s) Failed to open container2: %d - %s", __func__, -rtn, strerror(-rtn));
      goto finish;
    }

    ALOGD(LOGTAG "(%s) id:%d, uuid:%s", __func__, it.id, it.uuid.ToString().c_str());
    rtn = sd_bus_message_append(m, "qsiqqqyq", it.id,
                                it.uuid.ToString().c_str(),
                                it.type,
                                it.attribute_handle,
                                it.start_handle,
                                it.end_handle,
                                it.properties,
                                it.permissions);
    if (rtn < 0)
    {
      ALOGE(LOGTAG "(%s) Failed to append data: %d - %s", __func__, -rtn, strerror(-rtn));
      goto finish;
    }
    sd_bus_message_close_container(m);
  }

  sd_bus_message_close_container(m);
  ALOGD(LOGTAG "(%s) before sd_bus_call() server_if:%d, size:%d", __func__, server_if, service.size());

  rtn = sd_bus_call(g_sdbus_call, m, -1, nullptr, nullptr);
  if (rtn < 0)
    ALOGE(LOGTAG "(%s) Failed to call: %d - %s", __func__, -rtn, strerror(-rtn));

  ALOGD(LOGTAG "(%s) after sd_bus_call() server_if:%d, size:%d", __func__, server_if, service.size());

finish:
  if (m != NULL)
    sd_bus_message_unref(m);
  return;
}

void GattNativeInterfaceV2bImpl ::gattServerStopServiceNative(int server_if, int svc_handle)
{

  ALOGD(LOGTAG "(%s) enter!", __func__);
  // implement IPC call here by passing the parameters unchanged
  sd_bus_message *m = NULL;
  int rtn;
  rtn = sd_bus_call_method(__primary_params, __func__,
                           NULL,                   /* object to return error in */
                           &m,                     /* return message on success */
                           "ii",                   /* input signature */
                           server_if, svc_handle); /* parameters */

  __gatt_message_release(rtn, m);
}
void GattNativeInterfaceV2bImpl ::gattServerDeleteServiceNative(int server_if, int svc_handle)
{
  ALOGD(LOGTAG "(%s) enter!", __func__);
  // implement IPC call here by passing the parameters unchanged
  sd_bus_message *m = NULL;
  int rtn;
  rtn = sd_bus_call_method(__primary_params, __func__,
                           NULL,                   /* object to return error in */
                           &m,                     /* return message on success */
                           "ii",                   /* input signature */
                           server_if, svc_handle); /* parameters */

  __gatt_message_release(rtn, m);
}

void GattNativeInterfaceV2bImpl ::gattServerSendIndicationNative(int server_if, int attr_handle,
                                                                 int conn_id, std::vector<uint8_t> vect_val)
{

  ALOGD(LOGTAG "(%s) enter", __func__);
  // implement IPC call here by passing the parameters unchanged
  int rtn;
  sd_bus_message *m = NULL;

  rtn = sd_bus_message_new_method_call(g_sdbus_call,
                                       &m,
                                       __gatt_ipc_service,        /* service to contact */
                                       __gatt_ipc_service_object, /* object path */
                                       __gatt_ipc_service_name,   /* interface name */
                                       __func__);                 /* method name */
  if (rtn < 0)
  {
    ALOGE(LOGTAG "(%s) Failed to create new method call : %d - %s", __func__, -rtn, strerror(-rtn));
    goto finish;
  }

  rtn = sd_bus_message_append(m, "iii", server_if, attr_handle, conn_id);
  if (rtn < 0)
  {
    ALOGE(LOGTAG "(%s) Failed to append data: %d - %s", __func__, -rtn, strerror(-rtn));
    goto finish;
  }

  WRITE_SDBUS_ARRAY_FROM_VECTOR(m, uint8_t, SD_BUS_TYPE_BYTE, vect_val);

  rtn = sd_bus_call(g_sdbus_call, m, -1, nullptr, nullptr);
  if (rtn < 0)
  {
    ALOGE(LOGTAG "(%s) Failed to call: %d - %s", __func__, -rtn, strerror(-rtn));
  }

finish:
  if (m != NULL)
    sd_bus_message_unref(m);
  return;
}

void GattNativeInterfaceV2bImpl ::gattServerSendNotificationNative(
    int server_if, int attr_handle,
    int conn_id, std::vector<uint8_t> vect_val)
{

  ALOGD(LOGTAG "(%s) enter", __func__);
  // implement IPC call here by passing the parameters unchanged
  int rtn;
  sd_bus_message *m = NULL;

  rtn = sd_bus_message_new_method_call(g_sdbus_call,
                                       &m,
                                       __gatt_ipc_service,        /* service to contact */
                                       __gatt_ipc_service_object, /* object path */
                                       __gatt_ipc_service_name,   /* interface name */
                                       __func__);                 /* method name */
  if (rtn < 0)
  {
    ALOGE(LOGTAG "(%s) Failed to create new method call : %d - %s", __func__, -rtn, strerror(-rtn));
    goto finish;
  }

  rtn = sd_bus_message_append(m, "iii", server_if, attr_handle, conn_id);
  if (rtn < 0)
  {
    ALOGE(LOGTAG "(%s) Failed to append data: %d - %s", __func__, -rtn, strerror(-rtn));
    goto finish;
  }

  WRITE_SDBUS_ARRAY_FROM_VECTOR(m, uint8_t, SD_BUS_TYPE_BYTE, vect_val);

  rtn = sd_bus_call(g_sdbus_call, m, -1, nullptr, nullptr);
  if (rtn < 0)
  {
    ALOGE(LOGTAG "(%s) Failed to call: %d - %s", __func__, -rtn, strerror(-rtn));
  }

finish:
  if (m != NULL)
    sd_bus_message_unref(m);
  return;
}

void GattNativeInterfaceV2bImpl ::gattServerSendResponseNative(
    int server_if, int conn_id,
    int trans_id, int status,
    int handle, int offset,
    std::vector<uint8_t> vect_val, int auth_req)
{

//signature: "iiiiii ayi"

	ALOGD(LOGTAG "(%s) enter", __func__);
	// implement IPC call here by passing the parameters unchanged
	int rtn;
	sd_bus_message *m = NULL;
  
	rtn = sd_bus_message_new_method_call(g_sdbus_call,
										 &m,
										 __gatt_ipc_service,		/* service to contact */
										 __gatt_ipc_service_object, /* object path */
										 __gatt_ipc_service_name,	/* interface name */
										 __func__); 				/* method name */
	if (rtn < 0)
	{
	  ALOGE(LOGTAG "(%s) Failed to create new method call : %d - %s", __func__, -rtn, strerror(-rtn));
	  goto finish;
	}
  
	rtn = sd_bus_message_append(m, "iiiiii", server_if, conn_id, trans_id, status, handle, offset);
	if (rtn < 0)
	{
	  ALOGE(LOGTAG "(%s) Failed to append data: %d - %s", __func__, -rtn, strerror(-rtn));
	  goto finish;
	}
  
	WRITE_SDBUS_ARRAY_FROM_VECTOR(m, uint8_t, SD_BUS_TYPE_BYTE, vect_val);

	rtn = sd_bus_message_append(m, "i", auth_req);
	if (rtn < 0)
	{
	  ALOGE(LOGTAG "(%s) Failed to append data: %d - %s", __func__, -rtn, strerror(-rtn));
	  goto finish;
	}
  
	rtn = sd_bus_call(g_sdbus_call, m, -1, nullptr, nullptr);
	if (rtn < 0)
	{
	  ALOGE(LOGTAG "(%s) Failed to call: %d - %s", __func__, -rtn, strerror(-rtn));
	}
  
  finish:
	if (m != NULL)
	  sd_bus_message_unref(m);
	return;

}

void GattNativeInterfaceV2bImpl ::startAdvertisingSetNative(
    advertise_parameters_t params, std::vector<uint8_t> adv_data,
    std::vector<uint8_t> scan_resp,
    periodic_advertising_parameters_t periodic_params,
    std::vector<uint8_t> periodic_data, int duration,
    int maxExtAdvEvents, int reg_id)
{

  /* signature : quuyiyyy ayay yqqqayiii  */

  ALOGD(LOGTAG "(%s) enter adv_data size:%d scan_resp sie:%d, periodic_data size:%d", __func__, adv_data.size(), scan_resp.size(), periodic_data.size());
  // implement IPC call here by passing the parameters unchanged
  sd_bus_message *m = NULL;
  int rtn;

  rtn = sd_bus_message_new_method_call(g_sdbus_call,
                                       &m,
                                       __gatt_ipc_service,        /* service to contact */
                                       __gatt_ipc_service_object, /* object path */
                                       __gatt_ipc_service_name,   /* interface name */
                                       __func__);                 /* method name */
  if (rtn < 0)
  {
    ALOGE(LOGTAG "(%s) Failed to create new method call : %d - %s", __func__, -rtn, strerror(-rtn));
    goto finish;
  }

  /* "quuyiyyy" - advertise_parameters_t  */
  rtn = sd_bus_message_append(m, "quuyiyyy", params.advertising_event_properties,
                              params.min_interval,
                              params.max_interval,
                              params.channel_map,
                              params.tx_power,
                              params.primary_advertising_phy,
                              params.secondary_advertising_phy,
                              params.scan_request_notification_enable);
  if (rtn < 0)
  {
    ALOGE(LOGTAG "(%s) Failed to append data: %d - %s", __func__, -rtn, strerror(-rtn));
    goto finish;
  }

  /* vector : adv_data, scan_resp */
  WRITE_SDBUS_ARRAY_FROM_VECTOR(m, uint8_t, SD_BUS_TYPE_BYTE, adv_data);
  WRITE_SDBUS_ARRAY_FROM_VECTOR(m, uint8_t, SD_BUS_TYPE_BYTE, scan_resp);

  /* "yqqq" - periodic_advertising_parameters_t  */
  rtn = sd_bus_message_append(m, "yqqq", 
                              periodic_params.enable,
                              periodic_params.min_interval,
                              periodic_params.max_interval,
                              periodic_params.periodic_advertising_properties);
  if (rtn < 0)
  {
    ALOGE(LOGTAG "(%s) Failed to append data: %d - %s", __func__, -rtn, strerror(-rtn));
    goto finish;
  }

  /* vector : periodic_data */
  WRITE_SDBUS_ARRAY_FROM_VECTOR(m, uint8_t, SD_BUS_TYPE_BYTE, periodic_data);

  /* "i" - duration, maxExtAdvEvents, reg_id */
  rtn = sd_bus_message_append(m, "iii", duration, maxExtAdvEvents, reg_id);
  if (rtn < 0)
  {
    ALOGE(LOGTAG "(%s) Failed to append data: %d - %s", __func__, -rtn, strerror(-rtn));
    goto finish;
  }

  rtn = sd_bus_call(g_sdbus_call, m, -1, nullptr, nullptr);
  if (rtn < 0)
    ALOGE(LOGTAG "(%s) Failed to call: %d - %s", __func__, -rtn, strerror(-rtn));

finish:
  if (m != NULL)
    sd_bus_message_unref(m);
  return;
}

void GattNativeInterfaceV2bImpl ::stopAdvertisingSetNative(int advertiser_id)
{

  ALOGD(LOGTAG "(%s) enter!", __func__);
  // implement IPC call here by passing the parameters unchanged
  sd_bus_message *m = NULL;
  int rtn;
  rtn = sd_bus_call_method(__primary_params, __func__,
                           NULL,           /* object to return error in */
                           &m,             /* return message on success */
                           "i",            /* input signature */
                           advertiser_id); /* parameters */

  __gatt_message_release(rtn, m);
}

void GattNativeInterfaceV2bImpl ::enableAdvertisingSetNative(
    int advertiser_id, bool enable,
    int duration, int maxExtAdvEvents)
{
  ALOGD(LOGTAG "(%s) enter!", __func__);
  // implement IPC call here by passing the parameters unchanged
  sd_bus_message *m = NULL;
  int rtn;
  rtn = sd_bus_call_method(__primary_params, __func__,
                           NULL,                                              /* object to return error in */
                           &m,                                                /* return message on success */
                           "iiii",                                            /* input signature */
                           advertiser_id, (int)enable, duration, maxExtAdvEvents); /* parameters */

  __gatt_message_release(rtn, m);
}

void GattNativeInterfaceV2bImpl ::setAdvertisingDataNative(
    int advertiser_id, std::vector<uint8_t> data)
{

  ALOGD(LOGTAG "(%s) enter", __func__);
  // implement IPC call here by passing the parameters unchanged
  int rtn;
  sd_bus_message *m = NULL;

  rtn = sd_bus_message_new_method_call(g_sdbus_call,
                                       &m,
                                       __gatt_ipc_service,        /* service to contact */
                                       __gatt_ipc_service_object, /* object path */
                                       __gatt_ipc_service_name,   /* interface name */
                                       __func__);                 /* method name */
  if (rtn < 0)
  {
    ALOGE(LOGTAG "(%s) Failed to create new method call : %d - %s", __func__, -rtn, strerror(-rtn));
    goto finish;
  }

  rtn = sd_bus_message_append(m, "i", advertiser_id);
  if (rtn < 0)
  {
    ALOGE(LOGTAG "(%s) Failed to append data: %d - %s", __func__, -rtn, strerror(-rtn));
    goto finish;
  }

  WRITE_SDBUS_ARRAY_FROM_VECTOR(m, uint8_t, SD_BUS_TYPE_BYTE, data);

  rtn = sd_bus_call(g_sdbus_call, m, -1, nullptr, nullptr);
  if (rtn < 0)
  {
    ALOGE(LOGTAG "(%s) Failed to call: %d - %s", __func__, -rtn, strerror(-rtn));
  }

finish:
  if (m != NULL)
    sd_bus_message_unref(m);
  return;
}

void GattNativeInterfaceV2bImpl ::setScanResponseDataNative(
    int advertiser_id, std::vector<uint8_t> data)
{

  ALOGD(LOGTAG "(%s) enter", __func__);
  // implement IPC call here by passing the parameters unchanged
  int rtn;
  sd_bus_message *m = NULL;

  rtn = sd_bus_message_new_method_call(g_sdbus_call,
                                       &m,
                                       __gatt_ipc_service,        /* service to contact */
                                       __gatt_ipc_service_object, /* object path */
                                       __gatt_ipc_service_name,   /* interface name */
                                       __func__);                 /* method name */
  if (rtn < 0)
  {
    ALOGE(LOGTAG "(%s) Failed to create new method call : %d - %s", __func__, -rtn, strerror(-rtn));
    goto finish;
  }

  rtn = sd_bus_message_append(m, "i", advertiser_id);
  if (rtn < 0)
  {
    ALOGE(LOGTAG "(%s) Failed to append data: %d - %s", __func__, -rtn, strerror(-rtn));
    goto finish;
  }

  WRITE_SDBUS_ARRAY_FROM_VECTOR(m, uint8_t, SD_BUS_TYPE_BYTE, data);

  rtn = sd_bus_call(g_sdbus_call, m, -1, nullptr, nullptr);
  if (rtn < 0)
  {
    ALOGE(LOGTAG "(%s) Failed to call: %d - %s", __func__, -rtn, strerror(-rtn));
  }

finish:
  if (m != NULL)
    sd_bus_message_unref(m);
  return;
}

void GattNativeInterfaceV2bImpl ::setAdvertisingParametersNative(
    int advertiser_id,
    advertise_parameters_t params)
{

  ALOGD(LOGTAG "(%s) enter", __func__);
  // implement IPC call here by passing the parameters unchanged
  sd_bus_message *m = NULL;
  int rtn;

  rtn = sd_bus_message_new_method_call(g_sdbus_call,
                                       &m,
                                       __gatt_ipc_service,        /* service to contact */
                                       __gatt_ipc_service_object, /* object path */
                                       __gatt_ipc_service_name,   /* interface name */
                                       __func__);                 /* method name */
  if (rtn < 0)
  {
    ALOGE(LOGTAG "(%s) Failed to create new method call : %d - %s", __func__, -rtn, strerror(-rtn));
    goto finish;
  }

  rtn = sd_bus_message_append(m, "i", advertiser_id);
  if (rtn < 0)
  {
    ALOGE(LOGTAG "(%s) Failed to append data: %d - %s", __func__, -rtn, strerror(-rtn));
    goto finish;
  }

  rtn = sd_bus_message_append(m, "quuyiyyy", params.advertising_event_properties,
                              params.min_interval,
                              params.max_interval,
                              params.channel_map,
                              params.tx_power,
                              params.primary_advertising_phy,
                              params.secondary_advertising_phy,
                              params.scan_request_notification_enable);
  if (rtn < 0)
  {
    ALOGE(LOGTAG "(%s) Failed to append data: %d - %s", __func__, -rtn, strerror(-rtn));
    goto finish;
  }

  rtn = sd_bus_call(g_sdbus_call, m, -1, nullptr, nullptr);
  if (rtn < 0)
    ALOGE(LOGTAG "(%s) Failed to call: %d - %s", __func__, -rtn, strerror(-rtn));

finish:
  if (m != NULL)
    sd_bus_message_unref(m);
  return;
}

void GattNativeInterfaceV2bImpl ::setPeriodicAdvertisingParametersNative(int advertiser_id,
                                                                         periodic_advertising_parameters_t periodic_params)
{
  ALOGD(LOGTAG "(%s) enter", __func__);
  // implement IPC call here by passing the parameters unchanged
  sd_bus_message *m = NULL;
  int rtn;

  rtn = sd_bus_message_new_method_call(g_sdbus_call,
                                       &m,
                                       __gatt_ipc_service,        /* service to contact */
                                       __gatt_ipc_service_object, /* object path */
                                       __gatt_ipc_service_name,   /* interface name */
                                       __func__);                 /* method name */
  if (rtn < 0)
  {
    ALOGE(LOGTAG "(%s) Failed to create new method call : %d - %s", __func__, -rtn, strerror(-rtn));
    goto finish;
  }

  rtn = sd_bus_message_append(m, "i", advertiser_id);
  if (rtn < 0)
  {
    ALOGE(LOGTAG "(%s) Failed to append data: %d - %s", __func__, -rtn, strerror(-rtn));
    goto finish;
  }

  rtn = sd_bus_message_append(m, "yqqq",  &periodic_params.enable,
                                          &periodic_params.min_interval,
                                          &periodic_params.max_interval,
                                          &periodic_params.periodic_advertising_properties);
  if (rtn < 0)
  {
    ALOGE(LOGTAG "(%s) Failed to append data: %d - %s", __func__, -rtn, strerror(-rtn));
    goto finish;
  }

  rtn = sd_bus_call(g_sdbus_call, m, -1, nullptr, nullptr);
  if (rtn < 0)
    ALOGE(LOGTAG "(%s) Failed to call: %d - %s", __func__, -rtn, strerror(-rtn));

finish:
  if (m != NULL)
    sd_bus_message_unref(m);
  return;
}

void GattNativeInterfaceV2bImpl ::setPeriodicAdvertisingDataNative(
    int advertiser_id,
    std::vector<uint8_t> data)
{

  ALOGD(LOGTAG "(%s) enter", __func__);
  // implement IPC call here by passing the parameters unchanged
  int rtn;
  sd_bus_message *m = NULL;

  rtn = sd_bus_message_new_method_call(g_sdbus_call,
                                       &m,
                                       __gatt_ipc_service,        /* service to contact */
                                       __gatt_ipc_service_object, /* object path */
                                       __gatt_ipc_service_name,   /* interface name */
                                       __func__);                 /* method name */
  if (rtn < 0)
  {
    ALOGE(LOGTAG "(%s) Failed to create new method call : %d - %s", __func__, -rtn, strerror(-rtn));
    goto finish;
  }

  rtn = sd_bus_message_append(m, "i", advertiser_id);
  if (rtn < 0)
  {
    ALOGE(LOGTAG "(%s) Failed to append data: %d - %s", __func__, -rtn, strerror(-rtn));
    goto finish;
  }

  WRITE_SDBUS_ARRAY_FROM_VECTOR(m, uint8_t, SD_BUS_TYPE_BYTE, data);

  rtn = sd_bus_call(g_sdbus_call, m, -1, nullptr, nullptr);
  if (rtn < 0)
  {
    ALOGE(LOGTAG "(%s) Failed to call: %d - %s", __func__, -rtn, strerror(-rtn));
  }

finish:
  if (m != NULL)
    sd_bus_message_unref(m);
  return;
}

void GattNativeInterfaceV2bImpl ::setPeriodicAdvertisingEnableNative(
    int advertiser_id,
    bool enable)
{

  ALOGD(LOGTAG "(%s) enter!", __func__);
  // implement IPC call here by passing the parameters unchanged
  sd_bus_message *m = NULL;
  int rtn;
  rtn = sd_bus_call_method(__primary_params, __func__,
                           NULL,                   /* object to return error in */
                           &m,                     /* return message on success */
                           "ii",                   /* input signature */
                           advertiser_id, (int)enable); /* parameters */

  __gatt_message_release(rtn, m);
}

void GattNativeInterfaceV2bImpl ::startSyncNative(int sid,
                                                  string address, int skip, int timeout,
                                                  int reg_id)
{
  ALOGD(LOGTAG "(%s) enter!", __func__);
  // implement IPC call here by passing the parameters unchanged
  sd_bus_message *m = NULL;
  int rtn;
  rtn = sd_bus_call_method(__primary_params, __func__,
                           NULL,                                         /* object to return error in */
                           &m,                                           /* return message on success */
                           "isiii",                                      /* input signature */
                           sid, address.c_str(), skip, timeout, reg_id); /* parameters */

  __gatt_message_release(rtn, m);
}

void GattNativeInterfaceV2bImpl ::stopSyncNative(int sync_handle)
{

  ALOGD(LOGTAG "(%s) enter!", __func__);
  // implement IPC call here by passing the parameters unchanged
  sd_bus_message *m = NULL;
  int rtn;
  rtn = sd_bus_call_method(__primary_params, __func__,
                           NULL,         /* object to return error in */
                           &m,           /* return message on success */
                           "i",          /* input signature */
                           sync_handle); /* parameters */

  __gatt_message_release(rtn, m);
}

void GattNativeInterfaceV2bImpl ::gattTestNative(int command,
                                                 btapp::Uuid uuid, string bda1,
                                                 int p1, int p2, int p3, int p4, int p5)
{
  ALOGD(LOGTAG "(%s) enter!", __func__);
  // implement IPC call here by passing the parameters unchanged
  int rtn;
  sd_bus_message *m = NULL;
  rtn = sd_bus_call_method(__primary_params, __func__,
                           NULL,                                                                /* object to return error in */
                           &m,                                                                  /* return message on success */
                           "issiiiii",                                                          /* input signature */
                           command, uuid.ToString().c_str(), bda1.c_str(), p1, p2, p3, p4, p5); /* parameters */

  __gatt_message_release(rtn, m);
}
}

/*   */
#if 1
gatt::GattNativeInterfaceV2b GattNativeInterfaceV2bImplInst = 
{
	gatt::GattNativeInterfaceV2bImpl::init,
	gatt::GattNativeInterfaceV2bImpl::deinit,
	gatt::GattNativeInterfaceV2bImpl::gattClientGetDeviceTypeNative,
	gatt::GattNativeInterfaceV2bImpl::gattClientRegisterAppNative,
	gatt::GattNativeInterfaceV2bImpl::gattClientUnregisterAppNative,
	gatt::GattNativeInterfaceV2bImpl::registerScannerNative,
	gatt::GattNativeInterfaceV2bImpl::unregisterScannerNative,
	gatt::GattNativeInterfaceV2bImpl::gattClientScanNative,
	gatt::GattNativeInterfaceV2bImpl::gattClientConnectNative,
	gatt::GattNativeInterfaceV2bImpl::gattClientDisconnectNative,
	gatt::GattNativeInterfaceV2bImpl::gattClientSetPreferredPhyNative,
	gatt::GattNativeInterfaceV2bImpl::gattClientReadPhyNative,
	gatt::GattNativeInterfaceV2bImpl::gattClientRefreshNative,
	gatt::GattNativeInterfaceV2bImpl::gattClientSearchServiceNative,
	gatt::GattNativeInterfaceV2bImpl::gattClientDiscoverServiceByUuidNative,
	gatt::GattNativeInterfaceV2bImpl::gattClientGetGattDbNative,
	gatt::GattNativeInterfaceV2bImpl::gattClientReadCharacteristicNative,
	gatt::GattNativeInterfaceV2bImpl::gattClientReadUsingCharacteristicUuidNative,
	gatt::GattNativeInterfaceV2bImpl::gattClientReadDescriptorNative,
	gatt::GattNativeInterfaceV2bImpl::gattClientWriteCharacteristicNative,
	gatt::GattNativeInterfaceV2bImpl::gattClientExecuteWriteNative,
	gatt::GattNativeInterfaceV2bImpl::gattClientWriteDescriptorNative,
	gatt::GattNativeInterfaceV2bImpl::gattClientRegisterForNotificationsNative,
	gatt::GattNativeInterfaceV2bImpl::gattClientReadRemoteRssiNative,
	gatt::GattNativeInterfaceV2bImpl::gattSetScanParametersNative,
	gatt::GattNativeInterfaceV2bImpl::getOwnAddressNative,
	gatt::GattNativeInterfaceV2bImpl::gattClientScanFilterParamAddNative,
	gatt::GattNativeInterfaceV2bImpl::gattClientScanFilterParamDeleteNative,
	gatt::GattNativeInterfaceV2bImpl::gattClientScanFilterParamClearAllNative,
	gatt::GattNativeInterfaceV2bImpl::gattClientScanFilterAddNative,
	gatt::GattNativeInterfaceV2bImpl::gattClientScanFilterClearNative,
	gatt::GattNativeInterfaceV2bImpl::gattClientScanFilterEnableNative,
	gatt::GattNativeInterfaceV2bImpl::gattClientConfigureMTUNative,
	gatt::GattNativeInterfaceV2bImpl::gattConnectionParameterUpdateNative,
	gatt::GattNativeInterfaceV2bImpl::gattClientConfigBatchScanStorageNative,
	gatt::GattNativeInterfaceV2bImpl::gattClientStartBatchScanNative,
	gatt::GattNativeInterfaceV2bImpl::gattClientStopBatchScanNative,
	gatt::GattNativeInterfaceV2bImpl::gattClientReadScanReportsNative,
	gatt::GattNativeInterfaceV2bImpl::gattServerRegisterAppNative,
	gatt::GattNativeInterfaceV2bImpl::gattServerUnregisterAppNative,
	gatt::GattNativeInterfaceV2bImpl::gattServerConnectNative,
	gatt::GattNativeInterfaceV2bImpl::gattServerDisconnectNative,
	gatt::GattNativeInterfaceV2bImpl::gattServerSetPreferredPhyNative,
	gatt::GattNativeInterfaceV2bImpl::gattServerReadPhyNative,
	gatt::GattNativeInterfaceV2bImpl::gattServerAddServiceNative,
	gatt::GattNativeInterfaceV2bImpl::gattServerStopServiceNative,
	gatt::GattNativeInterfaceV2bImpl::gattServerDeleteServiceNative,
	gatt::GattNativeInterfaceV2bImpl::gattServerSendIndicationNative,
	gatt::GattNativeInterfaceV2bImpl::gattServerSendNotificationNative,
	gatt::GattNativeInterfaceV2bImpl::gattServerSendResponseNative,
	gatt::GattNativeInterfaceV2bImpl::startAdvertisingSetNative,
	gatt::GattNativeInterfaceV2bImpl::stopAdvertisingSetNative,
	gatt::GattNativeInterfaceV2bImpl::enableAdvertisingSetNative,
	gatt::GattNativeInterfaceV2bImpl::setAdvertisingDataNative,
	gatt::GattNativeInterfaceV2bImpl::setScanResponseDataNative,
	gatt::GattNativeInterfaceV2bImpl::setAdvertisingParametersNative,
	gatt::GattNativeInterfaceV2bImpl::setPeriodicAdvertisingParametersNative,
	gatt::GattNativeInterfaceV2bImpl::setPeriodicAdvertisingDataNative,
	gatt::GattNativeInterfaceV2bImpl::setPeriodicAdvertisingEnableNative,
	gatt::GattNativeInterfaceV2bImpl::startSyncNative,
	gatt::GattNativeInterfaceV2bImpl::stopSyncNative,
	gatt::GattNativeInterfaceV2bImpl::gattTestNative
};

#else

gatt::GattNativeInterfaceV2b GattNativeInterfaceV2bImplInst = 
{
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL
};

#endif

// garbage code, but backup.

/* if you are using BtEvent...
static void btgatt_event_cb (BtEvent *event)
{
	switch(event->event_id) {
    	case BTGATTS_REGISTER_APP_EVENT:
			break;
		case BTGATTS_CONNECTION_EVENT:
			break;
		case BTGATTS_SERVICE_ADDED_EVENT:
			break;
		case BTGATTS_INCLUDED_SERVICE_ADDED_EVENT:
			break;
		case BTGATTS_CHARACTERISTIC_ADDED_EVENT:
			break;
		case BTGATTS_DESCRIPTOR_ADDED_EVENT:
			break;
		case BTGATTS_SERVICE_STARTED_EVENT:
			break;
		case BTGATTS_SERVICE_STOPPED_EVENT:
			break;
		case BTGATTS_SERVICE_DELETED_EVENT:
			break;
		case BTGATTS_REQUEST_READ_CHARACTERISTIC_EVENT:
			break;
		case BTGATTS_REQUEST_READ_DESCRIPTOR_EVENT:
			break;
		case BTGATTS_REQUEST_WRITE_CHARACTERISTIC_EVENT:
			break;
		case BTGATTS_REQUEST_WRITE_DESCRIPTOR_EVENT:
			break;
		case BTGATTS_REQUEST_EXEC_WRITE_EVENT:
			break;
		case BTGATTS_RESPONSE_CONFIRMATION_EVENT:
			break;
		case BTGATTS_INDICATION_SENT_EVENT:
			break;
		case BTGATTS_CONGESTION_EVENT:
			break;
		case BTGATTS_MTU_CHANGED_EVENT:
			break;
		case BTGATTS_PHY_UPDATED_EVENT:
			break;
		case BTGATTS_CONN_UPDATED_EVENT:
			break;
		case BTGATTS_READ_PHY_EVENT:
			break;
		case BTGATTC_REGISTER_APP_EVENT:
			break;
		case BTGATTC_OPEN_EVENT:
			break;
		case BTGATTC_CLOSE_EVENT:
			break;
		case BTGATTC_SEARCH_COMPLETE_EVENT:
			break;
		case BTGATTC_SEARCH_RESULT_EVENT:
			break;
		case BTGATTC_GET_CHARACTERISTIC_EVENT:
			break;
		case BTGATTC_GET_DESCRIPTOR_EVENT:
			break;
		case BTGATTC_GET_INCLUDED_SERVICE_EVENT:
			break;
		case BTGATTC_REGISTER_FOR_NOTIFICATION_EVENT:
			break;
		case BTGATTC_NOTIFY_EVENT:
			break;
		case BTGATTC_READ_CHARACTERISTIC_EVENT:
			break;
		case BTGATTC_WRITE_CHARACTERISTIC_EVENT:
			break;
		case BTGATTC_READ_DESCRIPTOR_EVENT:
			break;
		case BTGATTC_WRITE_DESCRIPTOR_EVENT:
			break;
		case BTGATTC_EXECUTE_WRITE_EVENT:
			break;
		case BTGATTC_REMOTE_RSSI_EVENT:
			break;
		case BTGATTC_CONFIGURE_MTU_EVENT:
			break;
		case BTGATTC_CONGESTION_EVENT:
			break;
		case BTGATTC_GET_GATT_DB_EVENT:
			break;
		case BTGATTC_PHY_UPDATED_EVENT:
			break;
		case BTGATTC_CONN_UPDATED_EVENT:
			break;
		case BTGATTC_READ_PHY_EVENT:
			break;
		case BLEADVERTISER_SET_ADVERTISING_DATA_EVENT:
			break;
		case BLEADVERTISER_SET_SCAN_RESPONSE_DATA_EVENT:
			break;
		case BLEADVERTISER_SET_PERIODIC_ADVERTISING_DATA_EVENT:
			break;
		case BLEADVERTISER_SET_PERIODIC_ADVERTISING_PARAMETER_EVENT:
			break;
		case BLEDAVERTISER_GET_OWN_ADDRESS_EVENT:
			break;
		case BLEDAVERTISER_ADVERTISING_SET_ENABLE_EVENT:
			break;
		case BLEADVERTISER_ADVERTISING_PARAMETER_UPDATED_EVENT:
			break;
		case BLEDAVERTISER_ADVERTISING_SET_START_EVENT:
			break;
		case BLEDAVERTISER_PERIODIC_ADVERTISING_SET_ENABLE_EVENT:
			break;
		case BLESCANNER_REGISTER_SCANNER_EVENT:
			break;
		case BLESCANNER_SCAN_PARAMS_COMPLETE_EVENT:
			break;
		case BLESCANNER_SCAN_RESULT_EVENT:
			break;
		case BLESCANNER_SCAN_FILTER_CFG_EVENT:
			break;
		case BLESCANNER_SCAN_FILTER_PARAM_EVENT:
			break;
		case BLESCANNER_SCAN_FILTER_STATUS_EVENT:
			break;
		case BLESCANNER_BATCHSCAN_CFG_STORAGE_EVENT:
			break;
		case BLESCANNER_BATCHSCAN_START_EVENT:
			break;
		case BLESCANNER_BATCHSCAN_STOP_EVENT:
			break;
		case BLESCANNER_BATCHSCAN_REPORTS_EVENT:
			break;
		case BLESCANNER_BATCHSCAN_THRESHOLD_EVENT:
			break;
		case BLESCANNER_TRACK_ADV_EVENT_EVENT:
			break;
		case BLESCANNER_PERIODIC_ADVERTISING_SYNC_START_EVENT:
			break;
		case BLESCANNER_PERIODIC_ADVERTISING_SYNC_LOST_EVENT:
			break;
		case BLESCANNER_PERIODIC_ADVERTISING_SYNC_REPORT_EVENT:
			break;
		case GATT_EVENT_ADAPTER_PROPERTIES:
			break;
		case RSP_ENABLE_EVENT:
			break;
		case RSP_DISABLE_EVENT:
			break;
		default:
			ALOGD(LOGTAG "(%s) no event id (%x)", __func__, event->event_id);
			break;
	};
}
*/

