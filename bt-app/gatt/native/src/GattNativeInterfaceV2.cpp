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

#include "GattNativeInterfaceV2.hpp"
#include <hardware/bt_gatt.h>
#include <hardware/bt_gatt_types.h>
#include "ipc.hpp"
#include <hardware/bluetooth.h>
#include <base/bind.h>

namespace gatt {

#define LOGTAG "GattNativeInterfaceV2"

#define LOG_NDEBUG 0

static RawAddress str2addr(string address) {
  RawAddress bd_addr;

  RawAddress::FromString(std::string(address), bd_addr);

  return bd_addr;
}

static string* addr2Str(RawAddress address) {
  return new string(address.ToString());
}

static btapp::Uuid bluetoothUuid2btAppUuid(bluetooth::Uuid uuid) {
  return btapp::Uuid::From128BitBE(uuid.To128BitBE());
}

static bluetooth::Uuid btAppUuid2bluetoothUuid(btapp::Uuid uuid) {
  return bluetooth::Uuid::From128BitBE(uuid.To128BitBE());
}
/**
 * Static variables
 */

static const btgatt_interface_t* sGattIf = NULL;


/**
 * BTA client callbacks
 */

static void btgattc_register_app_cb(int status, int clientIf, const bluetooth::Uuid& app_uuid) {
    ALOGD(LOGTAG " (%s) status = %d, clientIf = %d, app_uuid = %s",
      __FUNCTION__, status, clientIf, app_uuid.ToString().c_str());

  BtEvent *event = new BtEvent;
  CHECK_PARAM_VOID(event);

  event->event_id = BTGATTC_REGISTER_APP_EVENT;
  event->gattc_register_app_event.status = status;
  event->gattc_register_app_event.clientIf = clientIf;
  event->gattc_register_app_event.app_uuid = bluetoothUuid2btAppUuid(app_uuid);

  PostMessage(THREAD_ID_GATT, event);
}

static void btgattc_open_cb(int conn_id, int status, int clientIf,
                     const RawAddress& bda) {
  ALOGD(LOGTAG "(%s) conn_id: %d status: %d client_if: %d",
    __FUNCTION__, conn_id, status, clientIf);

  BtEvent *event = new BtEvent;
  CHECK_PARAM_VOID(event);

  event->event_id = BTGATTC_OPEN_EVENT;
  event->gattc_open_event.status= status;
  event->gattc_open_event.clientIf= clientIf;
  event->gattc_open_event.conn_id= conn_id;
  event->gattc_open_event.bda = addr2Str(bda);

  PostMessage(THREAD_ID_GATT, event);
}

static void btgattc_close_cb(int conn_id, int status, int clientIf,
                      const RawAddress& bda) {
  ALOGD(LOGTAG "(%s) conn_id: %d status: %d client_if: %d",
      __FUNCTION__, conn_id, status, clientIf);

  BtEvent *event = new BtEvent;
  CHECK_PARAM_VOID(event);

  event->event_id = BTGATTC_CLOSE_EVENT;
  event->gattc_close_event.status= status;
  event->gattc_close_event.clientIf= clientIf;
  event->gattc_close_event.conn_id= conn_id;
  event->gattc_close_event.bda = addr2Str(bda);

  PostMessage(THREAD_ID_GATT, event);
}

static void btgattc_search_complete_cb(int conn_id, int status) {
  ALOGD(LOGTAG "(%s) status: %d conn_id: %d",__FUNCTION__,status, conn_id);

  BtEvent *event = new BtEvent;
  CHECK_PARAM_VOID(event);

  event->event_id = BTGATTC_SEARCH_COMPLETE_EVENT;
  event->gattc_search_complete_event.status= status;
  event->gattc_search_complete_event.conn_id= conn_id;

  PostMessage(THREAD_ID_GATT, event);

}

static void btgattc_register_for_notification_cb(int conn_id, int registered,
                                          int status, uint16_t handle) {
  ALOGD(LOGTAG "(%s) conn_id: %d status: %d registered: %d, handle: %d",
      __FUNCTION__, conn_id, status, registered, handle);

  BtEvent *event = new BtEvent;
  CHECK_PARAM_VOID(event);

  event->event_id = BTGATTC_REGISTER_FOR_NOTIFICATION_EVENT;
  event->gattc_register_for_notification_event.status= status;
  event->gattc_register_for_notification_event.conn_id= conn_id;
  event->gattc_register_for_notification_event.registered= registered;
  event->gattc_register_for_notification_event.handle= handle;

  PostMessage(THREAD_ID_GATT, event);

}

static void btgattc_notify_cb(int conn_id, const btgatt_notify_params_t& p_data) {

  ALOGD(LOGTAG "(%s) conn_id : %d",__FUNCTION__,conn_id);

  BtEvent *event = new BtEvent;
  CHECK_PARAM_VOID(event);

  event->event_id = BTGATTC_NOTIFY_EVENT;
  event->gattc_notify_event.conn_id= conn_id;
  std::memcpy(&event->gattc_notify_event.p_data, &p_data,sizeof(btgatt_notify_params_t));

  PostMessage(THREAD_ID_GATT, event);

}

static void btgattc_read_characteristic_cb(int conn_id, int status,
                                    btgatt_read_params_t* p_data) {
  ALOGD(LOGTAG "(%s) status: %d conn_id: %d",__FUNCTION__,status, conn_id);

  BtEvent *event = new BtEvent;
  CHECK_PARAM_VOID(event);

  event->event_id = BTGATTC_READ_CHARACTERISTIC_EVENT;
  event->gattc_read_characteristic_event.status= status;
  event->gattc_read_characteristic_event.conn_id= conn_id;

  if (status == 0 && p_data != NULL) {
    std::memcpy(&event->gattc_read_characteristic_event.p_data,
        p_data,sizeof(btgatt_read_params_t));
  }

  PostMessage(THREAD_ID_GATT, event);
}

static void btgattc_write_characteristic_cb(int conn_id, int status, uint16_t handle) {
  ALOGD(LOGTAG "(%s) status : %d conn_id : %d, handle: %d",__FUNCTION__,status, conn_id, handle);

  BtEvent *event = new BtEvent;
  CHECK_PARAM_VOID(event);

  event->event_id = BTGATTC_WRITE_CHARACTERISTIC_EVENT;
  event->gattc_write_characteristic_event.status= status;
  event->gattc_write_characteristic_event.conn_id= conn_id;
  event->gattc_write_characteristic_event.handle= handle;

  PostMessage(THREAD_ID_GATT, event);
}

static void btgattc_execute_write_cb(int conn_id, int status) {
  ALOGD(LOGTAG "(%s) status : %d conn_id : %d",__FUNCTION__,status, conn_id);

  BtEvent *event = new BtEvent;
  CHECK_PARAM_VOID(event);

  event->event_id = BTGATTC_EXECUTE_WRITE_EVENT;
  event->gattc_execute_write_event.status= status;
  event->gattc_execute_write_event.conn_id= conn_id;

  PostMessage(THREAD_ID_GATT, event);
}

static void btgattc_read_descriptor_cb(int conn_id, int status,
                                const btgatt_read_params_t& p_data) {
  ALOGD(LOGTAG "(%s) status : %d conn_id : %d",__FUNCTION__, status, conn_id);

  BtEvent *event = new BtEvent;
  CHECK_PARAM_VOID(event);

  event->event_id = BTGATTC_READ_DESCRIPTOR_EVENT;
  event->gattc_read_descriptor_event.status= status;
  event->gattc_read_descriptor_event.conn_id= conn_id;

  if (status == 0 && p_data.value.len != 0) {
    std::memcpy(&event->gattc_read_descriptor_event.p_data, &p_data,sizeof(btgatt_read_params_t));
  }

  PostMessage(THREAD_ID_GATT, event);
}

static void btgattc_write_descriptor_cb(int conn_id, int status, uint16_t handle) {
  ALOGD(LOGTAG "(%s) status : %d conn_id : %d, handle: %d",__FUNCTION__,status, conn_id, handle);

  BtEvent *event = new BtEvent;
  CHECK_PARAM_VOID(event);

  event->event_id = BTGATTC_WRITE_DESCRIPTOR_EVENT;
  event->gattc_write_descriptor_event.status= status;
  event->gattc_write_descriptor_event.conn_id= conn_id;
  event->gattc_write_descriptor_event.handle= handle;

  PostMessage(THREAD_ID_GATT, event);
}

static void btgattc_remote_rssi_cb(int client_if, const RawAddress& bda, int rssi,
                            int status) {
  ALOGD(LOGTAG "(%s) status : %d client_if : %d, rssi: %d",
    __FUNCTION__,status, client_if, rssi);

  BtEvent *event = new BtEvent;
  CHECK_PARAM_VOID(event);

  event->event_id = BTGATTC_REMOTE_RSSI_EVENT;
  event->gattc_remote_rssi_event.status = status;
  event->gattc_remote_rssi_event.client_if= client_if;
  event->gattc_remote_rssi_event.rssi= rssi;
  event->gattc_remote_rssi_event.bda = addr2Str(bda);

  PostMessage(THREAD_ID_GATT, event);
}

static void btgattc_configure_mtu_cb(int conn_id, int status, int mtu) {
  ALOGD(LOGTAG "(%s) status : %d conn_id : %d, mtu: %d",
    __FUNCTION__, status, conn_id, mtu);

  BtEvent *event = new BtEvent;
  CHECK_PARAM_VOID(event);

  event->event_id = BTGATTC_CONFIGURE_MTU_EVENT;
  event->gattc_configure_mtu_event.status= status;
  event->gattc_configure_mtu_event.conn_id= conn_id;
  event->gattc_configure_mtu_event.mtu= mtu;

  PostMessage(THREAD_ID_GATT, event);
}

static void btgattc_congestion_cb(int conn_id, bool congested) {
  ALOGD(LOGTAG "(%s) conn_id : %d congested : %d",__FUNCTION__, conn_id, congested);

  BtEvent *event = new BtEvent;
  CHECK_PARAM_VOID(event);

  event->event_id = BTGATTC_CONGESTION_EVENT;
  event->gattc_congestion_event.conn_id= conn_id;
  event->gattc_congestion_event.congested= congested;

  PostMessage(THREAD_ID_GATT, event);

}

static void btgattc_get_gatt_db_cb(int conn_id, const btgatt_db_element_t* db,
                            int count) {
  ALOGD(LOGTAG "(%s) conn_id: %d, count: %d",__FUNCTION__,conn_id, count);

  BtEvent *event = new BtEvent;
  CHECK_PARAM_VOID(event);

  btgatt_db_element_t *tmp_db = new btgatt_db_element_t[count];
  if (!tmp_db)
    return;
  std::memcpy(tmp_db, db, sizeof(btgatt_db_element_t) * count);

  event->event_id = BTGATTC_GET_GATT_DB_EVENT;
  event->gattc_get_gatt_db_event.conn_id = conn_id;
  event->gattc_get_gatt_db_event.count = count;
  event->gattc_get_gatt_db_event.db = (gatt::gatt_db_element_t *)tmp_db;

  PostMessage(THREAD_ID_GATT, event);
}

static void btgattc_phy_updated_cb(int conn_id, uint8_t tx_phy, uint8_t rx_phy,
                            uint8_t status) {
  ALOGD(LOGTAG "(%s) conn_id: %d tx_phy: %d, rx_phy: %d, status: %d",
      __FUNCTION__,conn_id, tx_phy, rx_phy, status);

  BtEvent *event = new BtEvent;
  CHECK_PARAM_VOID(event);

  event->event_id = BTGATTC_PHY_UPDATED_EVENT;
  event->gattc_phy_updated_event.conn_id = conn_id;
  event->gattc_phy_updated_event.tx_phy = tx_phy;
  event->gattc_phy_updated_event.rx_phy = rx_phy;
  event->gattc_phy_updated_event.status= status;

  PostMessage(THREAD_ID_GATT, event);
}

static void btgattc_conn_updated_cb(int conn_id, uint16_t interval, uint16_t latency,
                             uint16_t timeout, uint8_t status) {
  ALOGD(LOGTAG "(%s) conn_id: %d interval: %d, latency: %d, timeout: %d status: %d",
      __FUNCTION__,conn_id, interval, latency, timeout, status);

  BtEvent *event = new BtEvent;
  CHECK_PARAM_VOID(event);

  event->event_id = BTGATTC_CONN_UPDATED_EVENT;
  event->gattc_conn_updated_event.conn_id = conn_id;
  event->gattc_conn_updated_event.interval = interval;
  event->gattc_conn_updated_event.latency = latency;
  event->gattc_conn_updated_event.timeout = timeout;
  event->gattc_conn_updated_event.status = status;

  PostMessage(THREAD_ID_GATT, event);
}

static void readClientPhyCb(uint8_t clientIf, RawAddress bda, uint8_t tx_phy,
                            uint8_t rx_phy, uint8_t status) {
  ALOGD(LOGTAG "(%s) clientIf: %d, bda: %s, tx_phy: %d, rx_phy: %d, status: %d",
      __FUNCTION__,clientIf,  bda.ToString().c_str(), tx_phy, rx_phy, status);

  BtEvent *event = new BtEvent;
  CHECK_PARAM_VOID(event);

  event->event_id = BTGATTC_READ_PHY_EVENT;
  event->gattc_read_phy_event.clientIf= clientIf;
  event->gattc_read_phy_event.tx_phy = tx_phy;
  event->gattc_read_phy_event.rx_phy = rx_phy;
  event->gattc_read_phy_event.status= status;
  event->gattc_read_phy_event.bda = addr2Str(bda);

  PostMessage(THREAD_ID_GATT, event);
}


/**
 * BTA server callbacks
 */

static void btgatts_register_app_cb(int status, int server_if, const bluetooth::Uuid& uuid) {
  ALOGD(LOGTAG "(%s) server_if: %d status: %d", __FUNCTION__,server_if, status);

  BtEvent *event = new BtEvent;
  CHECK_PARAM_VOID(event);

  event->event_id = BTGATTS_REGISTER_APP_EVENT;
  event->gatts_register_app_event.status = status;
  event->gatts_register_app_event.server_if = server_if;
  event->gatts_register_app_event.uuid = bluetoothUuid2btAppUuid(uuid);

  PostMessage(THREAD_ID_GATT, event);
}

static void btgatts_connection_cb(int conn_id, int server_if, int connected,
                           const RawAddress& bda) {
  ALOGD(LOGTAG "(%s) connid : %d server_if : %d status : %d bda (%s)",__FUNCTION__, conn_id,
      server_if, connected, bda.ToString().c_str());

  BtEvent *event = new BtEvent;
  CHECK_PARAM_VOID(event);

  event->event_id = BTGATTS_CONNECTION_EVENT;
  event->gatts_connection_event.conn_id = conn_id;
  event->gatts_connection_event.server_if = server_if;
  event->gatts_connection_event.connected = connected;
  event->gatts_connection_event.bda = addr2Str(bda);

  PostMessage(THREAD_ID_GATT, event);
}

static void btgatts_service_added_cb(int status, int server_if,
                              std::vector<btgatt_db_element_t> service) {
  ALOGD(LOGTAG "(%s) status : %d server_if : %d",__FUNCTION__,
      status, server_if);

  BtEvent *event = new BtEvent;
  CHECK_PARAM_VOID(event);

  event->event_id = BTGATTS_SERVICE_ADDED_EVENT;
  event->gatts_service_added_event.status = status;
  event->gatts_service_added_event.server_if = server_if;
  event->gatts_service_added_event.service = new std::vector<gatt_db_element_t>();
  for(size_t i = 0; i < service.size(); i++)
  {
    btgatt_db_element_t temp = service.at(i);
    gatt_db_element_t temp1;
    temp1.id = temp.id;
    temp1.type = (gatt_db_attribute_type_t)temp.type;
    temp1.permissions = temp.permissions;
    temp1.properties = temp.properties;
    temp1.attribute_handle = temp.attribute_handle;
    temp1.start_handle = temp.start_handle;
    temp1.end_handle = temp.end_handle;
    temp1.uuid = bluetoothUuid2btAppUuid(temp.uuid);
    event->gatts_service_added_event.service->push_back(temp1);
  }

  PostMessage(THREAD_ID_GATT, event);
}

static void btgatts_service_stopped_cb(int status, int server_if, int srvc_handle) {
  ALOGD(LOGTAG "(%s) status:: %d, server_if: %d, srvc_handle: %d",
      __FUNCTION__, status, server_if, srvc_handle);

  BtEvent *event = new BtEvent;
  CHECK_PARAM_VOID(event);

  event->event_id = BTGATTS_SERVICE_STOPPED_EVENT;
  event->gatts_service_stopped_event.status = status;
  event->gatts_service_stopped_event.server_if = server_if;
  event->gatts_service_stopped_event.srvc_handle = srvc_handle ;

  PostMessage(THREAD_ID_GATT, event);
}

static void  btgatts_service_deleted_cb(int status, int server_if, int srvc_handle) {
  ALOGD(LOGTAG "(%s) status:: %d, server_if: %d, srvc_handle: %d",
      __FUNCTION__, status, server_if, srvc_handle);

  BtEvent *event = new BtEvent;
  CHECK_PARAM_VOID(event);

  event->event_id = BTGATTS_SERVICE_DELETED_EVENT;
  event->gatts_service_deleted_event.status = status;
  event->gatts_service_deleted_event.server_if = server_if;
  event->gatts_service_deleted_event.srvc_handle = srvc_handle ;

  PostMessage(THREAD_ID_GATT, event);
}

static void btgatts_request_read_characteristic_cb(int conn_id, int trans_id,
                                            const RawAddress& bda,
                                            int attr_handle, int offset,
                                            bool is_long) {
  ALOGD(LOGTAG "(%s) connid: %d trans_id: %d, bda: %s, attr_handle: %d, offset:%d is_long: %d",
      __FUNCTION__, conn_id, trans_id, bda.ToString().c_str(), attr_handle, offset, is_long);

  BtEvent *event = new BtEvent;
  CHECK_PARAM_VOID(event);

  event->event_id = BTGATTS_REQUEST_READ_CHARACTERISTIC_EVENT;
  event->gatts_request_read_characteristic_event.conn_id = conn_id;
  event->gatts_request_read_characteristic_event.trans_id = trans_id;
  event->gatts_request_read_characteristic_event.bda = addr2Str(bda);
  event->gatts_request_read_characteristic_event.attr_handle = attr_handle;
  event->gatts_request_read_characteristic_event.offset = offset;
  event->gatts_request_read_characteristic_event.is_long = is_long ;

  PostMessage(THREAD_ID_GATT, event);
}

static void btgatts_request_read_descriptor_cb(int conn_id, int trans_id,
                                        const RawAddress& bda, int attr_handle,
                                        int offset, bool is_long) {
  ALOGD(LOGTAG "(%s) connid: %d trans_id: %d, bda: %s, attr_handle: %d, offset:%d is_long: %d",
      __FUNCTION__, conn_id, trans_id, bda.ToString().c_str(), attr_handle, offset, is_long);

  BtEvent *event = new BtEvent;
  CHECK_PARAM_VOID(event);

  event->event_id = BTGATTS_REQUEST_READ_DESCRIPTOR_EVENT;
  event->gatts_request_read_descriptor_event.conn_id = conn_id;
  event->gatts_request_read_descriptor_event.trans_id = trans_id;
  event->gatts_request_read_descriptor_event.bda = addr2Str(bda);
  event->gatts_request_read_descriptor_event.attr_handle = attr_handle;
  event->gatts_request_read_descriptor_event.offset = offset;
  event->gatts_request_read_descriptor_event.is_long = is_long ;

  PostMessage(THREAD_ID_GATT, event);
}

static void btgatts_request_write_characteristic_cb(int conn_id, int trans_id,
                                             const RawAddress& bda,
                                             int attr_handle, int offset,
                                             bool need_rsp, bool is_prep,
                                             std::vector<uint8_t> value) {
   ALOGD(LOGTAG "(%s) connid: %d trans_id: %d, bda: %s, attr_handle: %d, offset:%d need_rsp: %d,"
      " is_prep:%d", __FUNCTION__, conn_id, trans_id, bda.ToString().c_str(), attr_handle, offset,
      need_rsp, is_prep);

  BtEvent *event = new BtEvent;
  CHECK_PARAM_VOID(event);

  event->event_id = BTGATTS_REQUEST_WRITE_CHARACTERISTIC_EVENT;
  event->gatts_request_write_characteristic_event.conn_id = conn_id;
  event->gatts_request_write_characteristic_event.trans_id = trans_id;
  event->gatts_request_write_characteristic_event.bda = addr2Str(bda);
  event->gatts_request_write_characteristic_event.attr_handle = attr_handle;
  event->gatts_request_write_characteristic_event.offset = offset;
  event->gatts_request_write_characteristic_event.need_rsp = need_rsp;
  event->gatts_request_write_characteristic_event.is_prep = is_prep;
  event->gatts_request_write_characteristic_event.value = new std::vector<uint8_t>(value);

  PostMessage(THREAD_ID_GATT, event);
}

static void btgatts_request_write_descriptor_cb(int conn_id, int trans_id,
                                         const RawAddress& bda, int attr_handle,
                                         int offset, bool need_rsp,
                                         bool is_prep,
                                         std::vector<uint8_t> value) {
  ALOGD(LOGTAG "(%s) connid: %d trans_id: %d, bda: %s, attr_handle: %d, offset:%d need_rsp: %d,"
      " is_prep:%d", __FUNCTION__, conn_id, trans_id, bda.ToString().c_str(), attr_handle, offset,
      need_rsp, is_prep);

  BtEvent *event = new BtEvent;
  CHECK_PARAM_VOID(event);

  event->event_id = BTGATTS_REQUEST_WRITE_DESCRIPTOR_EVENT;
  event->gatts_request_write_descriptor_event.conn_id = conn_id;
  event->gatts_request_write_descriptor_event.trans_id = trans_id;
  event->gatts_request_write_descriptor_event.bda = addr2Str(bda);
  event->gatts_request_write_descriptor_event.attr_handle = attr_handle;
  event->gatts_request_write_descriptor_event.offset = offset;
  event->gatts_request_write_descriptor_event.need_rsp = need_rsp;
  event->gatts_request_write_descriptor_event.is_prep = is_prep;
  event->gatts_request_write_descriptor_event.value = new std::vector<uint8_t>(value);

  PostMessage(THREAD_ID_GATT, event);
}

static void btgatts_request_exec_write_cb(int conn_id, int trans_id,
                                   const RawAddress& bda, int exec_write) {
  ALOGD(LOGTAG "(%s) connid: %d trans_id: %d, bda: %s, exec_write: %d",
      __FUNCTION__, conn_id, trans_id, bda.ToString().c_str(), exec_write);

  BtEvent *event = new BtEvent;
  CHECK_PARAM_VOID(event);

  event->event_id = BTGATTS_REQUEST_EXEC_WRITE_EVENT;
  event->gatts_request_exec_write_event.conn_id = conn_id;
  event->gatts_request_exec_write_event.trans_id = trans_id;
  event->gatts_request_exec_write_event.bda = addr2Str(bda);;
  event->gatts_request_exec_write_event.exec_write = exec_write;

  PostMessage(THREAD_ID_GATT, event);
}

static void btgatts_response_confirmation_cb(int status, int handle) {
  ALOGD(LOGTAG "(%s) status: %d handle: %d",__FUNCTION__, status, handle);

  BtEvent *event = new BtEvent;
  CHECK_PARAM_VOID(event);

  event->event_id = BTGATTS_RESPONSE_CONFIRMATION_EVENT;
  event->gatts_response_confirmation_event.status = status;
  event->gatts_response_confirmation_event.handle = handle ;

  PostMessage(THREAD_ID_GATT, event);
}

static void btgatts_indication_sent_cb(int conn_id, int status) {
  ALOGD(LOGTAG "(%s) conn_id : %d status:: %d",__FUNCTION__, conn_id, status);

  BtEvent *event = new BtEvent;
  CHECK_PARAM_VOID(event);

  event->event_id = BTGATTS_INDICATION_SENT_EVENT;
  event->gatts_indication_sent_event.status = status;
  event->gatts_indication_sent_event.conn_id = conn_id ;

  PostMessage(THREAD_ID_GATT, event);
}

static void btgatts_congestion_cb(int conn_id, bool congested) {
  ALOGD(LOGTAG "(%s) contested: %d conn_id: %d",__FUNCTION__, congested, conn_id);

  BtEvent *event = new BtEvent;
  CHECK_PARAM_VOID(event);

  event->event_id = BTGATTS_CONGESTION_EVENT;
  event->gatts_congestion_event.congested = congested;
  event->gatts_congestion_event.conn_id = conn_id ;

  PostMessage(THREAD_ID_GATT, event);
}

static void btgatts_mtu_changed_cb(int conn_id, int mtu) {
  ALOGD(LOGTAG "(%s) conn_id: %d Mtu: %d",__FUNCTION__, conn_id, mtu);

  BtEvent *event = new BtEvent;
  CHECK_PARAM_VOID(event);

  event->event_id = BTGATTS_MTU_CHANGED_EVENT;
  event->gatts_mtu_changed_event.conn_id = conn_id;
  event->gatts_mtu_changed_event.mtu = mtu ;

  PostMessage(THREAD_ID_GATT, event);
}

static void btgatts_phy_updated_cb(int conn_id, uint8_t tx_phy, uint8_t rx_phy,
                            uint8_t status) {
  ALOGD(LOGTAG "(%s) conn_id: %d tx_phy: %d, rx_phy: %d, status: %d",
      __FUNCTION__,conn_id, tx_phy, rx_phy, status);

  BtEvent *event = new BtEvent;
  CHECK_PARAM_VOID(event);

  event->event_id = BTGATTS_PHY_UPDATED_EVENT;
  event->gatts_phy_updated_event.conn_id = conn_id;
  event->gatts_phy_updated_event.tx_phy = tx_phy;
  event->gatts_phy_updated_event.rx_phy = rx_phy;
  event->gatts_phy_updated_event.status= status;

  PostMessage(THREAD_ID_GATT, event);
}

static void btgatts_conn_updated_cb(int conn_id, uint16_t interval, uint16_t latency,
                             uint16_t timeout, uint8_t status) {
  ALOGD(LOGTAG "(%s) conn_id: %d interval: %d, latency: %d, timeout: %d status: %d",
      __FUNCTION__,conn_id, interval, latency, timeout, status);

  BtEvent *event = new BtEvent;
  CHECK_PARAM_VOID(event);

  event->event_id = BTGATTS_CONN_UPDATED_EVENT;
  event->gatts_conn_updated_event.conn_id = conn_id;
  event->gatts_conn_updated_event.interval = interval;
  event->gatts_conn_updated_event.latency = latency;
  event->gatts_conn_updated_event.timeout = timeout;
  event->gatts_conn_updated_event.status = status;

  PostMessage(THREAD_ID_GATT, event);
}

static void readServerPhyCb(uint8_t serverIf, RawAddress bda, uint8_t tx_phy,
                            uint8_t rx_phy, uint8_t status) {
  ALOGD(LOGTAG "(%s) serverIf: %d, bda: %s, tx_phy: %d, rx_phy: %d, status: %d",
      __FUNCTION__,serverIf,  bda.ToString().c_str(), tx_phy, rx_phy, status);

  BtEvent *event = new BtEvent;
  CHECK_PARAM_VOID(event);

  event->event_id = BTGATTS_READ_PHY_EVENT;
  event->gatts_read_phy_event.serverIf= serverIf;
  event->gatts_read_phy_event.tx_phy = tx_phy;
  event->gatts_read_phy_event.rx_phy = rx_phy;
  event->gatts_read_phy_event.status= status;
  event->gatts_read_phy_event.bda = addr2Str(bda);

  PostMessage(THREAD_ID_GATT, event);
}

//scanner callbacks
static void register_scanner_cb(const bluetooth::Uuid& app_uuid, uint8_t scannerId,
                                 uint8_t status) {
  ALOGD(LOGTAG "(%s) scannerId: %d status: %d",__FUNCTION__, scannerId, status);

  BtEvent *event = new BtEvent;
  CHECK_PARAM_VOID(event);

  event->event_id = BLESCANNER_REGISTER_SCANNER_EVENT;
  event->blescanner_register_scanner_event.app_uuid = bluetoothUuid2btAppUuid(app_uuid);
  event->blescanner_register_scanner_event.scannerId = scannerId;
  event->blescanner_register_scanner_event.status = status;

  PostMessage(THREAD_ID_GATT, event);
}

static void scan_params_cmpl_cb(uint8_t client_if, uint8_t status) {
  ALOGD(LOGTAG "(%s) client_if: %d status: %d",__FUNCTION__, client_if, status);

  BtEvent *event = new BtEvent;
  CHECK_PARAM_VOID(event);

  event->event_id = BLESCANNER_SCAN_PARAMS_COMPLETE_EVENT;
  event->blescanner_scan_param_complete_event.client_if = client_if;
  event->blescanner_scan_param_complete_event.status = status;

  PostMessage(THREAD_ID_GATT, event);
}

static void scan_result_cb(uint16_t event_type, uint8_t addr_type,
                            RawAddress* bda, uint8_t primary_phy,
                            uint8_t secondary_phy, uint8_t advertising_sid,
                            int8_t tx_power, int8_t rssi,
                            uint16_t periodic_adv_int,
                            std::vector<uint8_t> adv_data) {

  ALOGD(LOGTAG "(%s) event_type:%d, addr_type: %d, bda: %s, primary_phy: %d, "
      "secondary_phy: %d, advertising_sid: %d, tx_power: %d, rssi: %d, periodic_adv_int:%d"
      ,__FUNCTION__, event_type, addr_type, (*bda).ToString().c_str(), primary_phy,
      secondary_phy, advertising_sid, tx_power, rssi, periodic_adv_int);

  BtEvent *event = new BtEvent;
  CHECK_PARAM_VOID(event);

  event->event_id = BLESCANNER_SCAN_RESULT_EVENT;
  event->blescanner_scan_result_event.bda = addr2Str(*bda);
  event->blescanner_scan_result_event.rssi= rssi;
  event->blescanner_scan_result_event.event_type = event_type;
  event->blescanner_scan_result_event.addr_type = addr_type;
  event->blescanner_scan_result_event.primary_phy = primary_phy;
  event->blescanner_scan_result_event.secondary_phy = secondary_phy;
  event->blescanner_scan_result_event.tx_power = tx_power;
  event->blescanner_scan_result_event.periodic_adv_int = periodic_adv_int;
  event->blescanner_scan_result_event.adv_data = new std::vector<uint8_t>(adv_data);

  PostMessage(THREAD_ID_GATT, event);
}

static void batchscan_reports_cb(int client_if, int status, int report_format,
                                  int num_records, std::vector<uint8_t> data) {
  ALOGD(LOGTAG "(%s) status : %d client_if : %d",__FUNCTION__,status, client_if);

  BtEvent *event = new BtEvent;
  CHECK_PARAM_VOID(event);

  event->event_id = BLESCANNER_BATCHSCAN_REPORTS_EVENT;
  event->blescanner_batchscan_reports_event.status= status;
  event->blescanner_batchscan_reports_event.client_if= client_if;
  event->blescanner_batchscan_reports_event.report_format = report_format;
  event->blescanner_batchscan_reports_event.num_records= num_records;
  event->blescanner_batchscan_reports_event.data = new std::vector<uint8_t>(data);

  PostMessage(THREAD_ID_GATT, event);
}

static void batchscan_threshold_cb(int client_if) {
  ALOGD(LOGTAG "(%s) client_if : %d", __FUNCTION__, client_if);

  BtEvent *event = new BtEvent;
  CHECK_PARAM_VOID(event);

  event->event_id = BLESCANNER_BATCHSCAN_THRESHOLD_EVENT;
  event->blescanner_batchscan_threshold_event.client_if= client_if;

  PostMessage(THREAD_ID_GATT, event);
}

static void track_adv_event_cb(btgatt_track_adv_info_t* p_adv_track_info) {
  ALOGD(LOGTAG "(%s) ",__FUNCTION__);

  BtEvent *event = new BtEvent;
  CHECK_PARAM_VOID(event);

  event->event_id = BLESCANNER_TRACK_ADV_EVENT_EVENT;
  event->blescanner_track_adv_event.p_adv_track_info.client_if = p_adv_track_info->client_if;
  event->blescanner_track_adv_event.p_adv_track_info.filt_index = p_adv_track_info->filt_index;
  event->blescanner_track_adv_event.p_adv_track_info.advertiser_state
      = p_adv_track_info->advertiser_state;
  event->blescanner_track_adv_event.p_adv_track_info.advertiser_info_present
      = p_adv_track_info->advertiser_info_present;
  event->blescanner_track_adv_event.p_adv_track_info.addr_type = p_adv_track_info->addr_type;
  event->blescanner_track_adv_event.p_adv_track_info.tx_power = p_adv_track_info->tx_power;
  event->blescanner_track_adv_event.p_adv_track_info.rssi_value = p_adv_track_info->rssi_value;
  event->blescanner_track_adv_event.p_adv_track_info.time_stamp = p_adv_track_info->time_stamp;
  event->blescanner_track_adv_event.p_adv_track_info.bd_addr = addr2Str(p_adv_track_info->bd_addr);

  if (p_adv_track_info->adv_pkt_len != 0) {
    event->blescanner_track_adv_event.p_adv_track_info.p_adv_pkt_data
        = new uint8_t[p_adv_track_info->adv_pkt_len];
    std::memcpy(event->blescanner_track_adv_event.p_adv_track_info.p_adv_pkt_data,
        p_adv_track_info->p_adv_pkt_data, p_adv_track_info->adv_pkt_len);
  } else {
    event->blescanner_track_adv_event.p_adv_track_info.p_adv_pkt_data = NULL;
    ALOGE(LOGTAG "(%s) adv_pkt_len is 0",__FUNCTION__);
  }

  if (p_adv_track_info->scan_rsp_len != 0) {
    event->blescanner_track_adv_event.p_adv_track_info.p_scan_rsp_data
        = new uint8_t[p_adv_track_info->scan_rsp_len];
    std::memcpy(event->blescanner_track_adv_event.p_adv_track_info.p_scan_rsp_data,
        p_adv_track_info->p_scan_rsp_data , p_adv_track_info->scan_rsp_len);
  } else {
    event->blescanner_track_adv_event.p_adv_track_info.p_scan_rsp_data = NULL;
    ALOGE(LOGTAG "(%s) scan_rsp_len is 0",__FUNCTION__);
  }

  PostMessage(THREAD_ID_GATT, event);
}

static void scan_filter_cfg_cb(uint8_t client_if, uint8_t filt_type,
                               uint8_t avbl_space, uint8_t action,
                               uint8_t status) {
  ALOGD(LOGTAG "(%s) status : %d client_if : %d, filt_type: %d, avbl_space: %d, action:%d",
      __FUNCTION__, status, client_if, filt_type, avbl_space, action);

  BtEvent *event = new BtEvent;

  event->event_id = BLESCANNER_SCAN_FILTER_CFG_EVENT;
  event->blescanner_scan_filter_cfg_event.status= status;
  event->blescanner_scan_filter_cfg_event.action= action;
  event->blescanner_scan_filter_cfg_event.client_if= client_if;
  event->blescanner_scan_filter_cfg_event.filt_type=filt_type;
  event->blescanner_scan_filter_cfg_event.avbl_space=avbl_space;

  PostMessage(THREAD_ID_GATT, event);
}

static void scan_filter_param_cb(uint8_t client_if, uint8_t avbl_space, uint8_t action,
                          uint8_t status) {
  ALOGD(LOGTAG "(%s) status : %d client_if : %d, avbl_space: %d, action:%d",
      __FUNCTION__, status, client_if, avbl_space, action);

  BtEvent *event = new BtEvent;
  CHECK_PARAM_VOID(event);

  event->event_id = BLESCANNER_SCAN_FILTER_PARAM_EVENT;
  event->blescanner_scan_filter_param_event.status= status;
  event->blescanner_scan_filter_param_event.action= action;
  event->blescanner_scan_filter_param_event.client_if= client_if;
  event->blescanner_scan_filter_param_event.avbl_space=avbl_space;

  PostMessage(THREAD_ID_GATT, event);
}

static void scan_filter_status_cb(uint8_t client_if, uint8_t action, uint8_t status) {
  ALOGD(LOGTAG "(%s) status : %d client_if : %d, action:%d",
      __FUNCTION__, status, client_if, action);

  BtEvent *event = new BtEvent;
  CHECK_PARAM_VOID(event);

  event->event_id = BLESCANNER_SCAN_FILTER_STATUS_EVENT;
  event->blescanner_scan_filter_status_event.status= status;
  event->blescanner_scan_filter_status_event.action= action;
  event->blescanner_scan_filter_status_event.client_if= client_if;

  PostMessage(THREAD_ID_GATT, event);
}

static void batchscan_cfg_storage_cb(uint8_t client_if, uint8_t status) {
  ALOGD(LOGTAG "(%s) status : %d client_if : %d", __FUNCTION__, status, client_if);

  BtEvent *event = new BtEvent;
  CHECK_PARAM_VOID(event);

  event->event_id = BLESCANNER_BATCHSCAN_CFG_STORAGE_EVENT;
  event->blescanner_batchscan_cfg_storage_event.status= status;
  event->blescanner_batchscan_cfg_storage_event.client_if= client_if;

  PostMessage(THREAD_ID_GATT, event);
}

static void  batchscan_start_cb(uint8_t client_if, uint8_t status) {
  ALOGD(LOGTAG "(%s) status : %d client_if : %d", __FUNCTION__, status, client_if);

  BtEvent *event = new BtEvent;
  CHECK_PARAM_VOID(event);

  event->event_id = BLESCANNER_BATCHSCAN_START_EVENT;
  event->blescanner_batchscan_start_event.status= status;
  event->blescanner_batchscan_start_event.client_if= client_if;

  PostMessage(THREAD_ID_GATT, event);
}

static void  batchscan_stop_cb(uint8_t client_if, uint8_t status) {
  ALOGD(LOGTAG "(%s) status : %d client_if : %d",__FUNCTION__, status, client_if);

  BtEvent *event = new BtEvent;
  CHECK_PARAM_VOID(event);

  event->event_id = BLESCANNER_BATCHSCAN_STOP_EVENT;
  event->blescanner_batchscan_stop_event.status= status;
  event->blescanner_batchscan_stop_event.client_if= client_if;

  PostMessage(THREAD_ID_GATT, event);
}

static void onSyncStarted(int reg_id, uint8_t status, uint16_t sync_handle,
                          uint8_t sid, uint8_t address_type, RawAddress address,
                          uint8_t phy, uint16_t interval) {
  BtEvent *event = new BtEvent;
  CHECK_PARAM_VOID(event);

  event->event_id = BLESCANNER_PERIODIC_ADVERTISING_SYNC_START_EVENT;
  event->blescanner_periodic_adv_sync_start_event.reg_id = reg_id;
  event->blescanner_periodic_adv_sync_start_event.status = status;
  event->blescanner_periodic_adv_sync_start_event.sync_handle = sync_handle;
  event->blescanner_periodic_adv_sync_start_event.sid = sid;
  event->blescanner_periodic_adv_sync_start_event.address_type = address_type;
  event->blescanner_periodic_adv_sync_start_event.bda = addr2Str(address);
  event->blescanner_periodic_adv_sync_start_event.phy = phy;
  event->blescanner_periodic_adv_sync_start_event.interval = interval;

  PostMessage(THREAD_ID_GATT, event);
}

static void  onSyncLost(uint16_t sync_handle) {
  BtEvent *event = new BtEvent;
  CHECK_PARAM_VOID(event);

  event->event_id = BLESCANNER_PERIODIC_ADVERTISING_SYNC_LOST_EVENT;
  event->blescanner_periodic_adv_sync_lost_event.sync_handle= sync_handle;

  PostMessage(THREAD_ID_GATT, event);
}


static void onSyncReport(uint16_t sync_handle, int8_t tx_power, int8_t rssi,
                         uint8_t data_status, std::vector<uint8_t> data) {
  BtEvent *event = new BtEvent;
  CHECK_PARAM_VOID(event);

  event->event_id = BLESCANNER_PERIODIC_ADVERTISING_SYNC_REPORT_EVENT;
  event->blescanner_periodic_adv_sync_report_event.sync_handle = sync_handle;
  event->blescanner_periodic_adv_sync_report_event.tx_power = tx_power;
  event->blescanner_periodic_adv_sync_report_event.rssi = rssi;
  event->blescanner_periodic_adv_sync_report_event.data_status = data_status;
  event->blescanner_periodic_adv_sync_report_event.data = new std::vector<uint8_t>(data);

  PostMessage(THREAD_ID_GATT, event);
}



static void onSetAdvertisingData(uint8_t advertiser_id,
                            uint8_t status) {
  ALOGD(LOGTAG "(%s) advertiser_id: %d status: %d",__FUNCTION__, advertiser_id, status);

  BtEvent *event = new BtEvent;
  CHECK_PARAM_VOID(event);

  event->event_id = BLEADVERTISER_SET_ADVERTISING_DATA_EVENT;
  event->bleadverister_set_adv_data_event.advertiser_id = advertiser_id;
  event->bleadverister_set_adv_data_event.status = status;

  PostMessage(THREAD_ID_GATT, event);
}

static void onSetScanResponseData(uint8_t advertiser_id,
                            uint8_t status) {
  ALOGD(LOGTAG "(%s) advertiser_id: %d status: %d",__FUNCTION__, advertiser_id, status);

  BtEvent *event = new BtEvent;
  CHECK_PARAM_VOID(event);

  event->event_id = BLEADVERTISER_SET_SCAN_RESPONSE_DATA_EVENT;
  event->bleadverister_set_scan_resp_event.advertiser_id = advertiser_id;
  event->bleadverister_set_scan_resp_event.status = status;

  PostMessage(THREAD_ID_GATT, event);
}

static void onSetPeriodicAdvertisingParameters(uint8_t advertiser_id,
                            uint8_t status) {
  ALOGD(LOGTAG "(%s) advertiser_id: %d status: %d",__FUNCTION__, advertiser_id, status);

  BtEvent *event = new BtEvent;
  CHECK_PARAM_VOID(event);

  event->event_id = BLEADVERTISER_SET_PERIODIC_ADVERTISING_PARAMETER_EVENT;
  event->bleadverister_set_periodic_adv_param_event.advertiser_id = advertiser_id;
  event->bleadverister_set_periodic_adv_param_event.status = status;

  PostMessage(THREAD_ID_GATT, event);
}

static void onSetPeriodicAdvertisingData(uint8_t advertiser_id,
                            uint8_t status) {
  ALOGD(LOGTAG "(%s) advertiser_id: %d status: %d",__FUNCTION__, advertiser_id, status);

  BtEvent *event = new BtEvent;
  CHECK_PARAM_VOID(event);

  event->event_id = BLEADVERTISER_SET_PERIODIC_ADVERTISING_DATA_EVENT;
  event->bleadverister_set_periodic_adv_data_event.advertiser_id = advertiser_id;
  event->bleadverister_set_periodic_adv_data_event.status = status;

  PostMessage(THREAD_ID_GATT, event);
}

static void getOwnAddressCb(uint8_t advertiser_id, uint8_t address_type,
                            RawAddress address) {
  ALOGD(LOGTAG "(%s) advertiser_id: %d address_type: %d, address: %s",
      __FUNCTION__, advertiser_id, address_type, address.ToString().c_str());

  BtEvent *event = new BtEvent;
  CHECK_PARAM_VOID(event);

  event->event_id = BLEDAVERTISER_GET_OWN_ADDRESS_EVENT;
  event->bleadvertiser_get_own_address_event.advertiser_id = advertiser_id;
  event->bleadvertiser_get_own_address_event.address_type = address_type;
  event->bleadvertiser_get_own_address_event.bda = addr2Str(address);

  PostMessage(THREAD_ID_GATT, event);
}

static void ble_advertising_set_started_cb(int reg_id, uint8_t advertiser_id,
                                           int8_t tx_power, uint8_t status) {
  ALOGD(LOGTAG "(%s) reg_id: %d, advertiser_id: %d, tx_power: %d, status: %d,",
      __FUNCTION__, reg_id, advertiser_id, tx_power, status);

  BtEvent *event = new BtEvent;
  CHECK_PARAM_VOID(event);

  event->event_id = BLEDAVERTISER_ADVERTISING_SET_START_EVENT;
  event->bleadvertiser_adv_set_start_event.reg_id = reg_id;
  event->bleadvertiser_adv_set_start_event.advertiser_id = advertiser_id;
  event->bleadvertiser_adv_set_start_event.tx_power = tx_power;
  event->bleadvertiser_adv_set_start_event.status = status;

  PostMessage(THREAD_ID_GATT, event);

}

static void ble_advertising_set_timeout_cb(uint8_t advertiser_id,
                                           uint8_t status) {
  ALOGD(LOGTAG "(%s) advertiser_id: %d status: %d",__FUNCTION__, advertiser_id, status);

  BtEvent *event = new BtEvent;
  CHECK_PARAM_VOID(event);

  event->event_id = BLEDAVERTISER_ADVERTISING_SET_ENABLE_EVENT;
  event->bleadvertiser_adv_set_enable_event.advertiser_id = advertiser_id;
  event->bleadvertiser_adv_set_enable_event.status = status;
  event->bleadvertiser_adv_set_enable_event.isEnabled = false;

  PostMessage(THREAD_ID_GATT, event);
}

static void ble_advertising_set_enable_Cb(uint8_t advertiser_id, bool enable, uint8_t status) {
  ALOGD(LOGTAG "(%s) advertiser_id: %d enable: %d, status: %d",__FUNCTION__, advertiser_id, enable, status);

  BtEvent *event = new BtEvent;
  CHECK_PARAM_VOID(event);

  event->event_id = BLEDAVERTISER_ADVERTISING_SET_ENABLE_EVENT;
  event->bleadvertiser_adv_set_enable_event.advertiser_id = advertiser_id;
  event->bleadvertiser_adv_set_enable_event.status = status;
  event->bleadvertiser_adv_set_enable_event.isEnabled = enable;

  PostMessage(THREAD_ID_GATT, event);
}

static void ble_advertising_parameters_updated_cb(uint8_t advertiser_id,
                                             uint8_t status, int8_t tx_power) {
  ALOGD(LOGTAG "(%s) advertiser_id: %d, tx_power: %d, status: %d,",
    __FUNCTION__, advertiser_id, tx_power, status);

  BtEvent *event = new BtEvent;
  CHECK_PARAM_VOID(event);

  event->event_id = BLEADVERTISER_ADVERTISING_PARAMETER_UPDATED_EVENT;
  event->bleadvertiser_adv_set_param_update_event.advertiser_id = advertiser_id;
  event->bleadvertiser_adv_set_param_update_event.status = status;
  event->bleadvertiser_adv_set_param_update_event.tx_power= tx_power;

  PostMessage(THREAD_ID_GATT, event);
}

static void ble_periodic_advertising_set_enable_Cb(uint8_t advertiser_id, bool enable,
                                uint8_t status) {
  ALOGD(LOGTAG "(%s) advertiser_id: %d enable: %d, status: %d",
      __FUNCTION__, advertiser_id, enable, status);

  BtEvent *event = new BtEvent;
  CHECK_PARAM_VOID(event);

  event->event_id = BLEDAVERTISER_PERIODIC_ADVERTISING_SET_ENABLE_EVENT;
  event->bleadverister_periodic_adv_set_enable_event.advertiser_id = advertiser_id;
  event->bleadverister_periodic_adv_set_enable_event.status = status;
  event->bleadverister_periodic_adv_set_enable_event.isEnabled = enable;

  PostMessage(THREAD_ID_GATT, event);
}


static const btgatt_scanner_callbacks_t sGattScannerCallbacks = {
    scan_result_cb,
    batchscan_reports_cb,
    batchscan_threshold_cb,
    track_adv_event_cb,
};

static const btgatt_client_callbacks_t sGattClientCallbacks = {
    btgattc_register_app_cb,
    btgattc_open_cb,
    btgattc_close_cb,
    btgattc_search_complete_cb,
    btgattc_register_for_notification_cb,
    btgattc_notify_cb,
    btgattc_read_characteristic_cb,
    btgattc_write_characteristic_cb,
    btgattc_read_descriptor_cb,
    btgattc_write_descriptor_cb,
    btgattc_execute_write_cb,
    btgattc_remote_rssi_cb,
    btgattc_configure_mtu_cb,
    btgattc_congestion_cb,
    btgattc_get_gatt_db_cb,
    NULL, /* services_removed_cb */
    NULL, /* services_added_cb */
    btgattc_phy_updated_cb,
    btgattc_conn_updated_cb
};


static const btgatt_server_callbacks_t sGattServerCallbacks = {
    btgatts_register_app_cb,
    btgatts_connection_cb,
    btgatts_service_added_cb,
    btgatts_service_stopped_cb,
    btgatts_service_deleted_cb,
    btgatts_request_read_characteristic_cb,
    btgatts_request_read_descriptor_cb,
    btgatts_request_write_characteristic_cb,
    btgatts_request_write_descriptor_cb,
    btgatts_request_exec_write_cb,
    btgatts_response_confirmation_cb,
    btgatts_indication_sent_cb,
    btgatts_congestion_cb,
    btgatts_mtu_changed_cb,
    btgatts_phy_updated_cb,
    btgatts_conn_updated_cb
};


/**
 * GATT callbacks
 */

static const btgatt_callbacks_t sGattCallbacks = {
    sizeof(btgatt_callbacks_t), &sGattClientCallbacks, &sGattServerCallbacks,
    &sGattScannerCallbacks,
};


GattNativeInterfaceV2 :: GattNativeInterfaceV2(const bt_interface_t *bt_interface) {
  if (bluetooth_interface) return;

  bluetooth_interface = bt_interface;
  if (bluetooth_interface == NULL) {
    ALOGE(LOGTAG "Bluetooth module is not loaded");
    return;
  }

  if (sGattIf != NULL) {
    ALOGW(LOGTAG "Cleaning up Bluetooth GATT Interface before initializing...");
    sGattIf->cleanup();
    sGattIf = NULL;
  }


  sGattIf =
      (btgatt_interface_t*)bluetooth_interface->get_profile_interface(BT_PROFILE_GATT_ID);
  if (sGattIf == NULL) {
    ALOGE(LOGTAG "Failed to get Bluetooth GATT Interface");
    return;
  }

  bt_status_t status = sGattIf->init(&sGattCallbacks);
  if (status != BT_STATUS_SUCCESS) {
    ALOGE(LOGTAG "Failed to initialize Bluetooth GATT, status: %d", status);
    sGattIf = NULL;
    return;
  }
}

GattNativeInterfaceV2 :: ~GattNativeInterfaceV2() {

  if (!bluetooth_interface) return;

  if (sGattIf != NULL) {
    sGattIf->cleanup();
    sGattIf = NULL;
  }

  bluetooth_interface = NULL;
}

/**
 * Native Client functions
 */

int GattNativeInterfaceV2 :: gattClientGetDeviceTypeNative(string address) {
  if (!sGattIf) return 0;

  return sGattIf->client->get_device_type(str2addr(address));
}

void GattNativeInterfaceV2 :: gattClientRegisterAppNative(btapp::Uuid uuid) {
  if (!sGattIf) return;

  sGattIf->client->register_client(btAppUuid2bluetoothUuid(uuid));
}

void GattNativeInterfaceV2 :: gattClientUnregisterAppNative(int clientIf) {
  if (!sGattIf) return;

  sGattIf->client->unregister_client(clientIf);
}

void GattNativeInterfaceV2 :: registerScannerNative(btapp::Uuid uuid) {
  if (!sGattIf) return;

  sGattIf->scanner->RegisterScanner(
      base::Bind(&register_scanner_cb, btAppUuid2bluetoothUuid(uuid)));
}

void GattNativeInterfaceV2 :: unregisterScannerNative(int scanner_id) {
  if (!sGattIf) return;

  sGattIf->scanner->Unregister(scanner_id);
}

void GattNativeInterfaceV2 :: gattClientScanNative(bool start) {
  if (!sGattIf) return;

  sGattIf->scanner->Scan(start);
}

void GattNativeInterfaceV2 :: gattClientConnectNative(int clientif, string address, bool isDirect,
                                    int transport, bool opportunistic,
                                    int initiating_phys) {
  if (!sGattIf) return;

  sGattIf->client->connect(clientif, str2addr(address), isDirect,
                           transport, opportunistic, initiating_phys);
}

void GattNativeInterfaceV2 :: gattClientDisconnectNative(int clientIf, string address,
                                       int conn_id) {
  if (!sGattIf) return;

  sGattIf->client->disconnect(clientIf, str2addr(address), conn_id);
}

void GattNativeInterfaceV2 :: gattClientSetPreferredPhyNative(int clientIf, string address,
                                            int tx_phy, int rx_phy,
                                            int phy_options) {
  if (!sGattIf) return;

  sGattIf->client->set_preferred_phy(str2addr(address), tx_phy, rx_phy,
                                     phy_options);
}

void GattNativeInterfaceV2 :: gattClientReadPhyNative(int clientIf, string address) {
  if (!sGattIf) return;

  RawAddress bda = str2addr(address);
  sGattIf->client->read_phy(bda, base::Bind(&readClientPhyCb, clientIf, bda));
}

void GattNativeInterfaceV2 :: gattClientRefreshNative(int clientIf, string address) {
  if (!sGattIf) return;

  sGattIf->client->refresh(clientIf, str2addr(address));
}

void GattNativeInterfaceV2 :: gattClientSearchServiceNative(int conn_id, bool search_all,
                                          btapp::Uuid uuid) {
  if (!sGattIf) return;

  bluetooth::Uuid uuid1 = btAppUuid2bluetoothUuid(uuid);
  sGattIf->client->search_service(conn_id, search_all ? 0 : &uuid1);
}

void GattNativeInterfaceV2 :: gattClientDiscoverServiceByUuidNative(int conn_id, btapp::Uuid uuid) {
  if (!sGattIf) return;

  sGattIf->client->btif_gattc_discover_service_by_uuid(conn_id, btAppUuid2bluetoothUuid(uuid));
}

void GattNativeInterfaceV2 :: gattClientGetGattDbNative(int conn_id) {
  if (!sGattIf) return;

  sGattIf->client->get_gatt_db(conn_id);
}

void GattNativeInterfaceV2 :: gattClientReadCharacteristicNative(int conn_id, int handle,
                                               int authReq) {
  if (!sGattIf) return;

  sGattIf->client->read_characteristic(conn_id, handle, authReq);
}

void GattNativeInterfaceV2 :: gattClientReadUsingCharacteristicUuidNative(
    int conn_id, btapp::Uuid uuid, int s_handle, int e_handle, int authReq) {
  if (!sGattIf) return;

  sGattIf->client->read_using_characteristic_uuid(conn_id, btAppUuid2bluetoothUuid(uuid), s_handle,
                                                  e_handle, authReq);
}

void GattNativeInterfaceV2 :: gattClientReadDescriptorNative(int conn_id, int handle,
                                           int authReq) {
  if (!sGattIf) return;

  sGattIf->client->read_descriptor(conn_id, handle, authReq);
}

void GattNativeInterfaceV2 :: gattClientWriteCharacteristicNative(int conn_id, int handle,
                                                int write_type, int auth_req,
                                                std::vector<uint8_t> vect_val) {
  if (!sGattIf) return;

  if (vect_val.size() == 0) {
    ALOGW("gattClientWriteCharacteristicNative() ignoring NULL array");
    return;
  }

  sGattIf->client->write_characteristic(conn_id, handle, write_type, auth_req,
                                        std::move(vect_val));
}

void GattNativeInterfaceV2 :: gattClientExecuteWriteNative(int conn_id, bool execute) {
  if (!sGattIf) return;

  sGattIf->client->execute_write(conn_id, execute ? 1 : 0);
}

void GattNativeInterfaceV2 :: gattClientWriteDescriptorNative(int conn_id, int handle,
                                            int auth_req, std::vector<uint8_t> vect_val) {
  if (!sGattIf) return;

  if (vect_val.size() == 0) {
    ALOGW("gattClientWriteDescriptorNative() ignoring NULL array");
    return;
  }

  sGattIf->client->write_descriptor(conn_id, handle, auth_req,
                                    std::move(vect_val));
}

void GattNativeInterfaceV2 :: gattClientRegisterForNotificationsNative(
    int clientIf, string address, int handle, bool enable) {
  if (!sGattIf) return;

  RawAddress bd_addr = str2addr(address);
  if (enable)
    sGattIf->client->register_for_notification(clientIf, bd_addr, handle);
  else
    sGattIf->client->deregister_for_notification(clientIf, bd_addr, handle);
}

void GattNativeInterfaceV2 :: gattClientReadRemoteRssiNative(int clientif, string address) {
  if (!sGattIf) return;

  sGattIf->client->read_remote_rssi(clientif, str2addr(address));
}

void GattNativeInterfaceV2 :: gattSetScanParametersNative(int client_if, int scan_phy,
                                        std::vector<uint32_t> scan_interval,
                                        std::vector<uint32_t> scan_window) {
  if (!sGattIf) return;

  sGattIf->scanner->SetScanParameters(
      scan_phy, scan_interval, scan_window,
      base::Bind(&scan_params_cmpl_cb, client_if));
}

void GattNativeInterfaceV2 :: getOwnAddressNative(
                                int advertiser_id) {
  if (!sGattIf) return;

  sGattIf->advertiser->GetOwnAddress(
      advertiser_id, base::Bind(&getOwnAddressCb, advertiser_id));
}

void GattNativeInterfaceV2 :: gattClientScanFilterParamAddNative(
      uint8_t client_if, uint8_t filt_index,
      std::unique_ptr<btgatt_filt_param_setup_t> filt_params) {
  if (!sGattIf) return;

  const int add_scan_filter_params_action = 0;

  sGattIf->scanner->ScanFilterParamSetup(
      client_if, add_scan_filter_params_action, filt_index,
      std::move(filt_params), base::Bind(&scan_filter_param_cb, client_if));
}

void GattNativeInterfaceV2 :: gattClientScanFilterParamDeleteNative(uint8_t client_if, uint8_t filt_index) {
  if (!sGattIf) return;

  const int delete_scan_filter_params_action = 1;

  sGattIf->scanner->ScanFilterParamSetup(
      client_if, delete_scan_filter_params_action, filt_index, nullptr,
      base::Bind(&scan_filter_param_cb, client_if));
}

void GattNativeInterfaceV2 :: gattClientScanFilterParamClearAllNative(uint8_t client_if) {
  if (!sGattIf) return;

  const int clear_scan_filter_params_action = 2;

  sGattIf->scanner->ScanFilterParamSetup(
      client_if, clear_scan_filter_params_action, 0 /* index, unused */,
      nullptr, base::Bind(&scan_filter_param_cb, client_if));
}

void GattNativeInterfaceV2 :: gattClientScanFilterAddNative(int client_if,int filter_index,
    std::vector<apcf_command_t> filters) {
  if (!sGattIf) return;

  int numFilters = filters.size();
  std::vector<ApcfCommand> apcf_filters;

  for(size_t i = 0; i < numFilters; i++)
  {
    apcf_command_t temp = filters.at(i);
    ApcfCommand temp1;
    temp1.address = str2addr(temp.address);
    temp1.addr_type = temp.addr_type;
    temp1.company = temp.company;
    temp1.data = temp.data;
    temp1.data_mask = temp.data_mask;
    temp1.name = temp.name;
    temp1.type = temp.type;
    temp1.uuid = btAppUuid2bluetoothUuid(temp.uuid);
    temp1.uuid_mask = btAppUuid2bluetoothUuid(temp.uuid_mask);

    apcf_filters.push_back(temp1);
  }

  sGattIf->scanner->ScanFilterAdd(filter_index, apcf_filters,
                                  base::Bind(&scan_filter_cfg_cb, client_if));
}

void GattNativeInterfaceV2 :: gattClientScanFilterClearNative(int client_if, int filt_index) {
  if (!sGattIf) return;

  sGattIf->scanner->ScanFilterClear(filt_index,
                                    base::Bind(&scan_filter_cfg_cb, client_if));
}

void GattNativeInterfaceV2 :: gattClientScanFilterEnableNative(int client_if, bool enable) {
  if (!sGattIf) return;

  sGattIf->scanner->ScanFilterEnable(enable,
                                     base::Bind(&scan_filter_status_cb, client_if));
}

void GattNativeInterfaceV2 :: gattClientConfigureMTUNative(int conn_id, int mtu) {
  if (!sGattIf) return;

  sGattIf->client->configure_mtu(conn_id, mtu);
}

void GattNativeInterfaceV2 :: gattConnectionParameterUpdateNative(
                                                int client_if, string address,
                                                int min_interval,
                                                int max_interval, int latency,
                                                int timeout, int min_ce_len,
                                                int max_ce_len) {
  if (!sGattIf) return;

  sGattIf->client->conn_parameter_update(
      str2addr(address), min_interval, max_interval, latency, timeout);
}

void GattNativeInterfaceV2 :: gattClientConfigBatchScanStorageNative(
    int client_if, int max_full_reports_percent,
    int max_trunc_reports_percent, int notify_threshold_level_percent) {
  if (!sGattIf) return;

  sGattIf->scanner->BatchscanConfigStorage(
      client_if, max_full_reports_percent, max_trunc_reports_percent,
      notify_threshold_level_percent,
      base::Bind(&batchscan_cfg_storage_cb, client_if));
}

void GattNativeInterfaceV2 :: gattClientStartBatchScanNative(
                                           int client_if, int scan_mode,
                                           int scan_interval_unit,
                                           int scan_window_unit,
                                           int addr_type, int discard_rule) {
  if (!sGattIf) return;

  sGattIf->scanner->BatchscanEnable(
      scan_mode, scan_interval_unit, scan_window_unit, addr_type, discard_rule,
      base::Bind(&batchscan_start_cb, client_if));
}

void GattNativeInterfaceV2 :: gattClientStopBatchScanNative(int client_if) {
  if (!sGattIf) return;

  sGattIf->scanner->BatchscanDisable(
      base::Bind(&batchscan_stop_cb, client_if));
}

void GattNativeInterfaceV2 :: gattClientReadScanReportsNative(int client_if, int scan_type) {
  if (!sGattIf) return;

  sGattIf->scanner->BatchscanReadReports(client_if, scan_type);
}

/**
 * Native server functions
 */
void GattNativeInterfaceV2 ::  gattServerRegisterAppNative(btapp::Uuid uuid) {
  if (!sGattIf) return;

  sGattIf->server->register_server(btAppUuid2bluetoothUuid(uuid));
}

void GattNativeInterfaceV2 :: gattServerUnregisterAppNative(int serverIf) {
  if (!sGattIf) return;

  sGattIf->server->unregister_server(serverIf);
}

void GattNativeInterfaceV2 :: gattServerConnectNative(int server_if,
                                    string address, bool is_direct,
                                    int transport) {
  if (!sGattIf) return;

  RawAddress bd_addr = str2addr(address);
  sGattIf->server->connect(server_if, bd_addr, is_direct, transport);
}

void GattNativeInterfaceV2 :: gattServerDisconnectNative(int serverIf, string address,
                                       int conn_id) {
  if (!sGattIf) return;
  sGattIf->server->disconnect(serverIf, str2addr(address), conn_id);
}

void GattNativeInterfaceV2 :: gattServerSetPreferredPhyNative(int serverIf, string address,
                                            int tx_phy, int rx_phy,
                                            int phy_options) {
  if (!sGattIf) return;
  RawAddress bda = str2addr(address);
  sGattIf->server->set_preferred_phy(bda, tx_phy, rx_phy, phy_options);
}

void GattNativeInterfaceV2 :: gattServerReadPhyNative(int serverIf, string address) {
  if (!sGattIf) return;

  RawAddress bda = str2addr(address);
  sGattIf->server->read_phy(bda, base::Bind(&readServerPhyCb, serverIf, bda));
}

void GattNativeInterfaceV2 :: gattServerAddServiceNative(int server_if,
                                       std::vector<gatt_db_element_t> service) {
  if (!sGattIf) return;

  if(service.size() > 0)
  {
    std::vector<btgatt_db_element_t> gatt_service;
    for(size_t i = 0; i < service.size(); i++)
    {
      gatt_db_element_t temp = service.at(i);
      btgatt_db_element_t temp1;

      temp1.id = temp.id;
      temp1.type = (bt_gatt_db_attribute_type_t)temp.type;
      temp1.permissions = temp.permissions;
      temp1.properties = temp.properties;
      temp1.attribute_handle = temp.attribute_handle;
      temp1.start_handle = temp.start_handle;
      temp1.end_handle = temp.end_handle;
      temp1.uuid = btAppUuid2bluetoothUuid(temp.uuid);
      gatt_service.push_back(temp1);
    }
    sGattIf->server->add_service(server_if, gatt_service);
  }
}

void GattNativeInterfaceV2 :: gattServerStopServiceNative(int server_if, int svc_handle) {
  if (!sGattIf) return;

  sGattIf->server->stop_service(server_if, svc_handle);
}

void GattNativeInterfaceV2 :: gattServerDeleteServiceNative(int server_if, int svc_handle) {
  if (!sGattIf) return;

  sGattIf->server->delete_service(server_if, svc_handle);
}

void GattNativeInterfaceV2 :: gattServerSendIndicationNative(int server_if, int attr_handle,
                                           int conn_id, std::vector<uint8_t> vect_val) {
  if (!sGattIf) return;

  sGattIf->server->send_indication(server_if, attr_handle, conn_id,
                                   /*confirm*/ 1, std::move(vect_val));
}

void GattNativeInterfaceV2 :: gattServerSendNotificationNative(
                                             int server_if, int attr_handle,
                                             int conn_id, std::vector<uint8_t> vect_val) {
  if (!sGattIf) return;

  sGattIf->server->send_indication(server_if, attr_handle, conn_id,
                                   /*confirm*/ 0, std::move(vect_val));
}

void GattNativeInterfaceV2 :: gattServerSendResponseNative(
                                         int server_if, int conn_id,
                                         int trans_id, int status,
                                         int handle, int offset,
                                         std::vector<uint8_t> vect_val, int auth_req) {
  if (!sGattIf) return;

  btgatt_response_t response;

  response.attr_value.handle = handle;
  response.attr_value.auth_req = auth_req;
  response.attr_value.offset = offset;
  response.attr_value.len = 0;

  if (vect_val.size() > 0) {
    if (vect_val.size() < BTGATT_MAX_ATTR_LEN) {
      response.attr_value.len = (uint16_t)vect_val.size();
    } else {
      response.attr_value.len = BTGATT_MAX_ATTR_LEN;
    }
    uint8_t *data = vect_val.data();
    std::memcpy(response.attr_value.value, data, response.attr_value.len);
  }

  sGattIf->server->send_response(conn_id, trans_id, status, response);
}

void GattNativeInterfaceV2 :: startAdvertisingSetNative(
                                      advertise_parameters_t params, std::vector<uint8_t> adv_data,
                                      std::vector<uint8_t> scan_resp,
                                      periodic_advertising_parameters_t periodic_params,
                                      std::vector<uint8_t> periodic_data, int duration,
                                      int maxExtAdvEvents, int reg_id) {
  if (!sGattIf) return;
  PeriodicAdvertisingParameters periodicParams;
  AdvertiseParameters adv_params;

  periodicParams.enable = periodic_params.enable;
  periodicParams.max_interval = periodic_params.max_interval;
  periodicParams.min_interval = periodic_params.min_interval;
  periodicParams.periodic_advertising_properties = periodic_params.periodic_advertising_properties;

  adv_params.advertising_event_properties = params.advertising_event_properties;
  adv_params.channel_map = params.channel_map;
  adv_params.max_interval = params.max_interval;
  adv_params.min_interval = params.min_interval;
  adv_params.primary_advertising_phy = params.primary_advertising_phy;
  adv_params.scan_request_notification_enable = params.scan_request_notification_enable;
  adv_params.secondary_advertising_phy = params.secondary_advertising_phy;
  adv_params.tx_power = params.tx_power;

  sGattIf->advertiser->StartAdvertisingSet(
      base::Bind(&ble_advertising_set_started_cb, reg_id), adv_params, adv_data,
      scan_resp, periodicParams, periodic_data, duration,
      maxExtAdvEvents, base::Bind(ble_advertising_set_timeout_cb));
}

void GattNativeInterfaceV2 :: stopAdvertisingSetNative(
                                     int advertiser_id) {
  if (!sGattIf) return;

  sGattIf->advertiser->Unregister(advertiser_id);
}

void GattNativeInterfaceV2 :: enableAdvertisingSetNative(
                                       int advertiser_id, bool enable,
                                       int duration, int maxExtAdvEvents) {
  if (!sGattIf) return;

  sGattIf->advertiser->Enable(advertiser_id, enable,
                              base::Bind(&ble_advertising_set_enable_Cb, advertiser_id, enable),
                              duration, maxExtAdvEvents,
                              base::Bind(&ble_advertising_set_enable_Cb, advertiser_id, false));
}

void GattNativeInterfaceV2 :: setAdvertisingDataNative(
                                     int advertiser_id, std::vector<uint8_t> data) {
  if (!sGattIf) return;

  sGattIf->advertiser->SetData(
      advertiser_id, false, data,
      base::Bind(&onSetAdvertisingData, advertiser_id));
}

void GattNativeInterfaceV2 :: setScanResponseDataNative(
                                      int advertiser_id, std::vector<uint8_t> data) {
  if (!sGattIf) return;

  sGattIf->advertiser->SetData(
      advertiser_id, true, data,
      base::Bind(&onSetScanResponseData,
                 advertiser_id));
}

void GattNativeInterfaceV2 :: setAdvertisingParametersNative(
                                           int advertiser_id,
                                           advertise_parameters_t params) {
  if (!sGattIf) return;

  AdvertiseParameters adv_params;

  adv_params.advertising_event_properties = params.advertising_event_properties;
  adv_params.channel_map = params.channel_map;
  adv_params.max_interval = params.max_interval;
  adv_params.min_interval = params.min_interval;
  adv_params.primary_advertising_phy = params.primary_advertising_phy;
  adv_params.scan_request_notification_enable = params.scan_request_notification_enable;
  adv_params.secondary_advertising_phy = params.secondary_advertising_phy;
  adv_params.tx_power = params.tx_power;

  sGattIf->advertiser->SetParameters(
      advertiser_id, adv_params,
      base::Bind(&ble_advertising_parameters_updated_cb, advertiser_id));
}

void GattNativeInterfaceV2 :: setPeriodicAdvertisingParametersNative(int advertiser_id,
    periodic_advertising_parameters_t periodic_params) {
  if (!sGattIf) return;

  PeriodicAdvertisingParameters periodicParams;

  periodicParams.enable = periodic_params.enable;
  periodicParams.max_interval = periodic_params.max_interval;
  periodicParams.min_interval = periodic_params.min_interval;
  periodicParams.periodic_advertising_properties = periodic_params.periodic_advertising_properties;

  sGattIf->advertiser->SetPeriodicAdvertisingParameters(
      advertiser_id, periodicParams,
      base::Bind(&onSetPeriodicAdvertisingParameters, advertiser_id));
}

void GattNativeInterfaceV2 :: setPeriodicAdvertisingDataNative(
                                             int advertiser_id,
                                             std::vector<uint8_t> data) {
  if (!sGattIf) return;

  sGattIf->advertiser->SetPeriodicAdvertisingData(
      advertiser_id, data,
      base::Bind(&onSetPeriodicAdvertisingData,
                 advertiser_id));
}

void GattNativeInterfaceV2 :: setPeriodicAdvertisingEnableNative(
                                               int advertiser_id,
                                               bool enable) {
  if (!sGattIf) return;

  sGattIf->advertiser->SetPeriodicAdvertisingEnable(
      advertiser_id, enable,
      base::Bind(&ble_periodic_advertising_set_enable_Cb, advertiser_id, enable));
}

void GattNativeInterfaceV2 :: startSyncNative(int sid,
                            string address, int skip, int timeout,
                            int reg_id) {
  if (!sGattIf) return;

  sGattIf->scanner->StartSync(sid, str2addr(address), skip, timeout,
                              base::Bind(&onSyncStarted, reg_id),
                              base::Bind(&onSyncReport),
                              base::Bind(&onSyncLost));
}

void GattNativeInterfaceV2 :: stopSyncNative(int sync_handle) {
  if (!sGattIf) return;

  sGattIf->scanner->StopSync(sync_handle);
}

void GattNativeInterfaceV2 :: gattTestNative(int command,
                           btapp::Uuid uuid, string bda1,
                           int p1, int p2, int p3, int p4, int p5) {
  if (!sGattIf) return;

  bluetooth::Uuid uuid1 = btAppUuid2bluetoothUuid(uuid);
  RawAddress bt_bda1 = str2addr(bda1);

  btgatt_test_params_t params;
  params.bda1 = &bt_bda1;
  params.uuid1 = &uuid1;
  params.u1 = p1;
  params.u2 = p2;
  params.u3 = p3;
  params.u4 = p4;
  params.u5 = p5;
  sGattIf->client->test_command(command, params);
}

}
