/*
 * Copyright (c) 2021 The Linux Foundation. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 *       copyright notice, this list of conditions and the following
 *       disclaimer in the documentation and/or other materials provided
 *       with the distribution.
 *     * Neither the name of The Linux Foundation nor the names of its
 *       contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
 * BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 * OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 * IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "Gatt.hpp"
#include "GaiaTest.hpp"
#include "utils.h"
#include "ipc.h"
#include "gaia_ad.h"
#include "scan_handler.h"
#include "connection_handler.h"
#include "gaia_types.h"
#include "gaia_client_service.h"
#include "byte_utils.h"
#include "gaia_otau_client_api.h"

#define UNUSED

GaiaTest *gaiatest = NULL;
bt_uuid_t gaia_uuid;
gaia_scan_result_callback gaia_scan_cb= NULL;
gaia_open_callback gaia_open_cb = NULL;
gaia_close_callback gaia_close_cb = NULL;
gaia_notify_callback gaia_notify_cb = NULL;
gaia_mtu_callback gaia_mtu_cb = NULL;
gaia_write_characteristic_callback gaia_write_cb = NULL;
gaia_search_callback gaia_search_cb = NULL;
bt_bdaddr_t gaia_bda;

#ifdef __cplusplus
extern "C"
{
#endif
#define COPYMAXLEN 200
//gaia adapter functions
void gaia_pair(bt_bdaddr_t *bda)
{
    ALOGE(LOGTAG  "GaiaTest (%s)", __func__);
    fprintf(stdout, "GaiaTest (%s)\n", __func__);
    BtEvent *event = NULL;
    event = new BtEvent;
    event->event_id = GAP_API_CREATE_BOND;
    memcpy(&event->bond_device.bd_addr, bda, sizeof(bt_bdaddr_t));
    event->bond_device.transport = BT_TRANSPORT_LE;
    PostMessage(THREAD_ID_GAP, event);
}

void gaia_reset(void)
{
    if(gaiatest)
    {
        fprintf(stdout, "===========================\n");
        ALOGE(LOGTAG  "GaiaTest (%s)", __func__);
        fprintf(stdout, "GaiaTest (%s)\n", __func__);
        fprintf(stdout, "===========================\n");
        gaiatest->started = false;
        memset(&gaiatest->bda, 0, sizeof(bt_bdaddr_t));
    }
}

void gaia_start_scan(void* cb)
{
    if(gaiatest)
    {
        ALOGE(LOGTAG  "GaiaTest (%s)", __func__);
        fprintf(stdout, "GaiaTest (%s)\n", __func__);
        gaia_scan_cb = (gaia_scan_result_callback) cb;
        gaiatest->StartScan();
    } else {
        fprintf(stdout, "GaiaTest (%s): not init\n", __func__);
        ALOGE(LOGTAG  "GaiaTest (%s): not init", __func__);
    }
}
void gaia_stop_scan()
{
    if(gaiatest)
    {
        fprintf(stdout, "GaiaTest (%s)\n", __func__);
        ALOGE(LOGTAG  "GaiaTest (%s)", __func__);
        //gaia_scan_cb = NULL;
        gaiatest->StopScan();
    } else {
        fprintf(stdout, "GaiaTest (%s): not init\n", __func__);
        ALOGE(LOGTAG  "GaiaTest (%s): not init", __func__);
    }
}
void gaia_connect(bt_bdaddr_t* bda, gaia_open_callback open_cb, gaia_close_callback close_cb, gaia_notify_callback notify_cb, gaia_mtu_callback mtu_cb)
{
    bdstr_t bd_str;
    bdaddr_to_string(bda, &bd_str[0], sizeof(bd_str));
    memcpy(&gaia_bda, bda, sizeof(bt_bdaddr_t));
    gaia_open_cb = open_cb;
    gaia_close_cb = close_cb;
    gaia_notify_cb = notify_cb;
    gaia_mtu_cb = mtu_cb;
    if(gaiatest)
    {
        fprintf(stdout, "GaiaTest (%s): bda:%s\n", __func__, bd_str);
        ALOGE(LOGTAG  "GaiaTest (%s): bda:%s", __func__, bd_str);
        gaiatest->Connect(bda);
    } else {
        fprintf(stdout, "GaiaTest (%s): not init\n", __func__);
        ALOGE(LOGTAG  "GaiaTest (%s): not init", __func__);
    }
}

void gaia_disconnect(bt_bdaddr_t* bda)
{
    bdstr_t bd_str;
    bdaddr_to_string(bda, &bd_str[0], sizeof(bd_str));
    memset(&gaia_bda, 0, sizeof(bt_bdaddr_t));
    if(gaiatest)
    {
        fprintf(stdout, "GaiaTest (%s) bda:%s\n", __func__, bd_str);
        ALOGE(LOGTAG  "GaiaTest (%s) bda:%s",__func__, bd_str);
        gaiatest->Disconnect(bda);
    } else {
        fprintf(stdout, "GaiaTest (%s): not init\n", __func__);
        ALOGE(LOGTAG  "GaiaTest (%s): not init", __func__);
    }

}

void GATT_Write_Request(int conn_id, uint16_t handle, int write_type, uint16_t len, int auth_req, void *value, gaia_write_characteristic_callback ClientEventCallback)
{
    if(gaiatest)
    {
        ALOGE(LOGTAG "GaiaTest(%s) handle:0x%x len:%d", __func__, handle, len);
        fprintf(stdout, "GaiaTest(%s) handle:0x%x len:%d\n", __func__, handle, len);
        gaia_write_cb = ClientEventCallback;
        gaiatest->WriteCharacteristic(conn_id, handle, write_type, len, auth_req, (char*)value);
    } else {
        fprintf(stdout, "GaiaTest (%s): not init\n", __func__);
        ALOGE(LOGTAG  "GaiaTest (%s): not init", __func__);
    }
}

void GATT_Start_Service_Discovery(int conn_id,GATT_UUID_t *UUID, gaia_search_callback search_cb)
{
    if(gaiatest)
    {
        fprintf(stdout, "GaiaTest (%s)\n", __func__);
        ALOGE(LOGTAG  "GaiaTest(%s)",__func__);
        gaia_search_cb= search_cb;
        gaiatest->SearchService(conn_id, UUID);
    } else {
        fprintf(stdout, "GaiaTest (%s) not init\n", __func__);
        ALOGE(LOGTAG  "GaiaTest (%s) not init", __func__);
    }

}

void GATT_refresh(void)
{
    fprintf(stdout, "GaiaTest (%s)\n", __func__);
    ALOGE(LOGTAG  "GaiaTest(%s)",__func__);
    gaiatest->Refresh();
}

void GATT_register_ntf(int client_if, const bt_bdaddr_t *bda, uint16_t handle)
{
    bdstr_t bd_str;
    bdaddr_to_string(bda, &bd_str[0], sizeof(bd_str));
    fprintf(stdout, "GaiaTest(%s) client_if:%d addr:%s handle:%d\n", __func__, client_if, bd_str, handle);
    ALOGE(LOGTAG "GaiaTest(%s) client_if:%d addr:%s handle:%d\n", __func__, client_if, bd_str, handle);
    gaiatest->RegisterNotify(client_if, bda, handle);
}

#ifdef __cplusplus
}
#endif


/*****************************/
class gaiatestCallback : public BluetoothGattClientCallback
{
    public:

        void btgattc_client_register_app_cb(int status,int client_if,bt_uuid_t *uuid)
        {
            ALOGE(LOGTAG  "GaiaTest (%s)",__func__);
            fprintf(stdout,"GaiaTest (btgattc_client_register_app_cb)\n ");

            if(!gaiatest) {
                fprintf(stdout,"GaiaTest(%s): not init\n", __func__);
                ALOGE(LOGTAG  "GaiaTest (%s): not init",__func__);
            return;
            }

            GaiaRegisterAppEvent event;
            event.event_id = BTGAIA_REGISTER_APP_EVENT;
            event.status = status;
            event.clientIf = client_if;
            memcpy(&event.app_uuid,uuid,sizeof(bt_uuid_t));

            gaiatest->SetGaiaTestAppData(&event);
        }

        void btgattc_scan_result_cb(bt_bdaddr_t* bda, int rssi, uint8_t* adv_data, uint16_t adv_data_len)
        {
            bdstr_t bd_str;
            bdaddr_to_string(bda, &bd_str[0], sizeof(bd_str));
            ALOGE(LOGTAG  "GaiaTest (%s) bda:%s",__func__, bd_str);
            fprintf(stdout,"GaiaTest (btgattc_scan_result_cb) bda(%s)\n", bd_str);
            if(gaia_scan_cb)
            {
                gaia_scan_cb(bda, adv_data, adv_data_len);
            }
        }

        void btgattc_open_cb(int conn_id, int status, int clientIf, bt_bdaddr_t* bda)
        {
            bdstr_t bd_str;
            bdaddr_to_string(bda, &bd_str[0], sizeof(bd_str));
            fprintf(stdout,"GaiaTest(%s) bda:%s conn_id:%d status:%d clientIf%d\n ",__func__, bd_str, conn_id, status, clientIf);
            ALOGV(LOGTAG "GaiaTest(%s) bda:%s conn_id:%d status:%d clientIf%d\n ",__func__, bd_str, conn_id, status, clientIf);

            GattcOpenEvent event;
            event.event_id = BTGATTC_OPEN_EVENT;
            event.conn_id = conn_id;
            event.status = status;
            event.clientIf = clientIf;
            memcpy(&event.bda, bda, sizeof(bt_bdaddr_t));

            if (gaiatest) {
                gaiatest->SetGaiaTestConnectionData(&event);
                //gaiatest->ConfigMtu(conn_id, BTGATT_MAX_ATTR_LEN);
                if (status == 0)
                {
                    if(gaia_open_cb)
                        (gaia_open_cb)(conn_id, status, clientIf, bda);
                }
            }
        }

        void btgattc_close_cb(int conn_id, int status, int clientIf, bt_bdaddr_t* bda)
        {
            bdstr_t bd_str;
            bdaddr_to_string(bda, &bd_str[0], sizeof(bd_str));
            fprintf(stdout, "GaiaTest(%s) conn_id:%d status:%d clientIf:%d bda:%s\n", __func__, conn_id, status, clientIf, bd_str);
            ALOGV(LOGTAG "GaiaTest(%s) conn_id:%d status:%d clientIf:%d bda:%s\n", __func__, conn_id, status, clientIf, bd_str);
            if(gaia_close_cb)
            {
                (gaia_close_cb)(conn_id, status, clientIf, bda);
            }
        }

        void btgattc_search_complete_cb(int conn_id, int status)
        {
            fprintf(stdout,"GaiaTest(%s) conn_id %d status %d\n", __func__, conn_id,status);
            ALOGV(LOGTAG "GaiaTest(%s) conn_id %d status %d", __func__, conn_id,status);
            if(status == 0)
            {
                gaiatest->app_gatt->get_gatt_db(conn_id);
            }
        }

        void btgattc_register_for_notification_cb(int conn_id, int registered, int status, uint16_t handle)
        {
            fprintf(stdout, "GaiaTest (%s) conn_id:%d, registered:%d, status:%d, handle:%d\n", __func__, conn_id, registered, status, handle);
            ALOGV( LOGTAG "GaiaTest (%s) conn_id:%d, registered:%d, status:%d, handle:%d\n", __func__, conn_id, registered, status, handle);
        }

        void btgattc_notify_cb(int conn_id, btgatt_notify_params_t *p_data)
        {
            ALOGV( LOGTAG "GaiaTest(%s)", __func__);
            fprintf(stdout, "GaiaTest(%s)\n", __func__);
            GATT_Server_Notification_Data_t notify_data;
            notify_data.AttributeValueLength = p_data->len;
            notify_data.AttributeHandle = p_data->handle;
            notify_data.AttributeValue = p_data->value;
            if (gaia_notify_cb)
            {
                (gaia_notify_cb)(&notify_data);
            }
        }

        void btgattc_read_characteristic_cb(int conn_id, int status, btgatt_read_params_t *p_data)
        {
            UNUSED
        }

        void btgattc_write_characteristic_cb(int conn_id, int status, uint16_t handle)
        {
            ALOGV(LOGTAG "GaiaTest(%s)",__func__);
            fprintf(stdout, "GaiaTest(%s)\n",__func__);
            if (gaia_write_cb)
            {
                (gaia_write_cb)(conn_id, status, handle);
            }
        }

        void btgattc_read_descriptor_cb(int conn_id, int status, btgatt_read_params_t *p_data)
        {
            UNUSED
        }

        void btgattc_write_descriptor_cb(int conn_id, int status, uint16_t handle)
        {
            UNUSED
        }

        void btgattc_execute_write_cb(int conn_id, int status)
        {
            UNUSED
        }

        void btgattc_remote_rssi_cb(int client_if,bt_bdaddr_t* bda, int rssi, int status)
        {
            UNUSED
        }

        void btgattc_advertise_cb(int status, int client_if)
        {
            UNUSED
        }

        void btgattc_configure_mtu_cb(int conn_id, int status, int mtu)
        {
            int gaia_mtu = 0;
            ALOGE(LOGTAG "GaiaTest(%s) conn_id:%d status:%d mtu:%d", __func__, conn_id, status, mtu);
            fprintf(stdout, "GaiaTest(%s) conn_id:%d status:%d mtu:%d\n", __func__, conn_id, status, mtu);
            if(status == GATT_SUCCESS) {
                gaia_mtu = (mtu > BTGATT_MAX_ATTR_LEN ? BTGATT_MAX_ATTR_LEN: mtu);
                if(gaia_mtu_cb) {
                    (gaia_mtu_cb)(gaia_mtu);
                }
            }

        }

       void btgattc_scan_filter_cfg_cb(int action, int client_if, int status, int filt_type, int avbl_space)
        {
            UNUSED
        }

        void btgattc_scan_filter_param_cb(int action, int client_if, int status, int avbl_space)
        {
            UNUSED
        }

        void btgattc_scan_filter_status_cb(int action, int client_if, int status)
        {
            UNUSED
        }

        void btgattc_multiadv_enable_cb(int client_if, int status)
        {
            UNUSED
        }

        void btgattc_multiadv_update_cb(int client_if, int status)
        {
            UNUSED
        }

        void btgattc_multiadv_setadv_data_cb(int client_if, int status)
        {
            UNUSED
        }

        void btgattc_multiadv_disable_cb(int client_if, int status)
        {
            UNUSED
        }

        void btgattc_congestion_cb(int conn_id, bool congested)
        {
            UNUSED
        }

        void btgattc_batchscan_cfg_storage_cb(int client_if, int status)
        {
            UNUSED
        }

        void btgattc_batchscan_startstop_cb(int startstop_action, int client_if, int status)
        {
            UNUSED
        }

        void btgattc_batchscan_reports_cb(int client_if, int status, int report_format,
                                                int num_records, int data_len, uint8_t *p_rep_data)
        {
            UNUSED
        }
        void btgattc_batchscan_threshold_cb(int client_if)
        {
            UNUSED
        }

        void btgattc_track_adv_event_cb(btgatt_track_adv_info_t *p_adv_track_info)
        {
            UNUSED
        }

        void btgattc_scan_parameter_setup_completed_cb(int client_if, btgattc_error_t status)
        {
            UNUSED
        }
        void btgattc_get_gatt_db_cb(int conn_id, btgatt_db_element_t *db, int count)
        {
            bool found = false;
            char uuid_buf[COPYMAXLEN];
            GAIA_Client_Info_t clientInfo;
            ALOGV(LOGTAG "GaiaTest(%s) conn_id:%d count:%d\n", __func__, conn_id, count);
            fprintf(stdout, "GaiaTest(%s) conn_id:%d count:%d\n", __func__, conn_id, count);
            memset(&clientInfo, 0, sizeof(clientInfo));

            fprintf(stdout, "============================================\n",__func__);
            fprintf(stdout, "GaiaTest(%s) print all db\n",__func__);
            for(int j = 0; j < count; j++)
            {
                btgatt_db_element_t curr = db[j];
                fprintf(stdout,"%d id:%d uuid:%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x type:%d attribute_handle:%d\n", j, curr.id,
                        curr.uuid.uu[0],curr.uuid.uu[1],curr.uuid.uu[2],curr.uuid.uu[3],
                        curr.uuid.uu[4],curr.uuid.uu[5],curr.uuid.uu[6],curr.uuid.uu[7],
                        curr.uuid.uu[8],curr.uuid.uu[9],curr.uuid.uu[10],curr.uuid.uu[11],
                        curr.uuid.uu[12],curr.uuid.uu[13],curr.uuid.uu[14],curr.uuid.uu[15],
                        curr.type, curr.attribute_handle);

            }
            fprintf(stdout, "============================================\n",__func__);

            for(int i = 0; i < count; i++)
            {
                btgatt_db_element_t curr = db[i];
                if(curr.type == BTGATT_DB_CHARACTERISTIC && (curr.uuid.uu[13] == 0x11) && (curr.uuid.uu[12] == 0x01))
                {
                    clientInfo.Command_Endpoint_Characteristic = curr.attribute_handle;
                    clientInfo.Response_Endpoint_Characteristic = db[i+1].attribute_handle;
                    clientInfo.Data_Endpoint_Characteristic = db[i+3].attribute_handle;
                    clientInfo.Response_Endpoint_Client_Configuration_Descriptor = db[i+2].attribute_handle;
                    clientInfo.Data_Endpoint_Client_Configuration_Descriptor = db[i+4].attribute_handle;
                    fprintf(stdout, "GaiaTest (btgattc_get_gatt_db_cb) find GAIA service\n");
                    ALOGV(LOGTAG "GaiaTest (btgattc_get_gatt_db_cb) find GAIA service\n");
                    ALOGV(LOGTAG "GaiaTest(%s) Value Handle:1101-%d, 1102-%d, 1103-%d; Cfg Handle:1102-%d, 1103-%d",
                                  __func__,
                                  clientInfo.Command_Endpoint_Characteristic,
                                  clientInfo.Response_Endpoint_Characteristic,
                                  clientInfo.Data_Endpoint_Characteristic,
                                  clientInfo.Response_Endpoint_Client_Configuration_Descriptor,
                                  clientInfo.Data_Endpoint_Client_Configuration_Descriptor);

                    fprintf(stdout, "GaiaTest(%s)Value Handle:1101-%d, 1102-%d, 1103-%d; Cfg Handle:1102-%d, 1103-%d\n",
                                  __func__,
                                  clientInfo.Command_Endpoint_Characteristic,
                                  clientInfo.Response_Endpoint_Characteristic,
                                  clientInfo.Data_Endpoint_Characteristic,
                                  clientInfo.Response_Endpoint_Client_Configuration_Descriptor,
                                  clientInfo.Data_Endpoint_Client_Configuration_Descriptor);
                    found = true;
                    break;
                }

            }

            if(gaia_search_cb)
                (gaia_search_cb)(&clientInfo);

            if(clientInfo.Command_Endpoint_Characteristic == 0)
            {
                ALOGE(LOGTAG "GaiaTest(%s) NOT FOUND GAIA SERVICE!!!", __func__);
                fprintf(stdout, "GaiaTest(%s) NOT FOUND GAIA SERVICE!!!\n", __func__);
            }
        }
};

gaiatestCallback *gaiatestCb = NULL;


GaiaTest::GaiaTest(Gatt* gatt)
{
    fprintf(stdout,"Gaiatest new\n");
    ALOGE(LOGTAG "Gaiatest new");
    gatt_interface = gatt->GetGattInterface();
    app_gatt = gatt;
    gaiatestCb = new gaiatestCallback;
}

GaiaTest::~GaiaTest()
{
    fprintf(stdout, "GaiaTest(%s) DeInitialized\n",__func__);
    ALOGE(LOGTAG "GaiaTest(%s) DeInitialized",__func__);
    delete(gaiatestCb);
}

bool GaiaTest::CopyClientUUID(bt_uuid_t *uuid)
{
    CHECK_PARAM(uuid)
    uuid->uu[0] = 0xff;
    int i;
    for (i = 1; i < 16; i++) {
        uuid->uu[i] = 0x40;
    }
    ALOGV(LOGTAG "GaiaTest uuid:0x%2x-%2x-%2x-%2x-%2x-%2x-%2x-%2x-%2x-%2x-%2x-%2x-%2x-%2x-%2x-%2x",
            uuid->uu[0], uuid->uu[1],uuid->uu[2], uuid->uu[3],uuid->uu[4], uuid->uu[5],uuid->uu[6], uuid->uu[7],
            uuid->uu[8], uuid->uu[9],uuid->uu[10], uuid->uu[11],uuid->uu[12], uuid->uu[13],uuid->uu[14], uuid->uu[15]);
    fprintf(stdout, "GaiaTest uuid:0x%2x-%2x-%2x-%2x-%2x-%2x-%2x-%2x-%2x-%2x-%2x-%2x-%2x-%2x-%2x-%2x\n",
            uuid->uu[0], uuid->uu[1],uuid->uu[2], uuid->uu[3],uuid->uu[4], uuid->uu[5],uuid->uu[6], uuid->uu[7],
            uuid->uu[8], uuid->uu[9],uuid->uu[10], uuid->uu[11],uuid->uu[12], uuid->uu[13],uuid->uu[14], uuid->uu[15]);
    return true;
}

bool GaiaTest::EnableGaiaTest(char *path)
{
    CopyClientUUID(&gaia_uuid);
    strlcpy(fw_path, path, 200);
    fprintf(stdout, "GaiaTest(%s) fw_path:%s\n",__func__,fw_path);
    ALOGV(LOGTAG "GaiaTest(%s) fw_path:%s",__func__,fw_path);
    gaiatest->RegisterClient();
    GaiaClientOtauInit(fw_path);
    return true;
}

bool GaiaTest::RegisterClient()
{
    if (GetGattInterface() == NULL) {
        ALOGE(LOGTAG  "(%s) Gatt Interface Not present",__func__);
        return false;
    }
    fprintf(stdout, "GaiaTest(RegisterClient)\n");
    ALOGV(LOGTAG "GaiaTest(RegisterClient)\n");
    app_gatt->RegisterClientCallback(gaiatestCb,&gaia_uuid);
    return app_gatt->register_client(&gaia_uuid) == BT_STATUS_SUCCESS;
}

bool GaiaTest::UnregisterClient(int client_if)
{
    fprintf(stdout, "GaiaTest (%s)\n",__func__);
    ALOGD(LOGTAG  "GaiaTest(%s)",__func__);
    if (GetGattInterface() == NULL) {
        ALOGE(LOGTAG  "GaiaTest(%s) Gatt Interface Not present",__func__);
        return false;
    }
    app_gatt->UnRegisterClientCallback(client_if);
    return app_gatt->unregister_client(client_if) == BT_STATUS_SUCCESS;
}

bool GaiaTest::DisableGaiaTest()
{
    fprintf(stdout, "GaiaTest (%s)\n",__func__);
    ALOGD(LOGTAG  "GaiaTest(%s)",__func__);

     if (gaiatest) {
         UnregisterClient(GetGaiaTestAppData()->clientIf);
         Cleanup();
         delete gaiatest;
         gaiatest = NULL;
     }
     return true;
}
void GaiaTest::Start(bt_bdaddr_t bd_addr)
{
    bdstr_t bd_str;
    bdaddr_to_string(&bd_addr, &bd_str[0], sizeof(bd_str));
    ALOGE(LOGTAG  "GaiaTest(%s) bda:%s",__func__, bd_str);
    fprintf(stdout, "GaiaTest(%s) bda:%s\n",__func__, bd_str);
    StartLOTScan(&bd_addr);
}

bool GaiaTest::Connect(const bt_bdaddr_t *bd_addr)
{
    bdstr_t bd_str;
    bdaddr_to_string(bd_addr, &bd_str[0], sizeof(bd_str));
    if (GetGattInterface() == NULL) {
        ALOGE(LOGTAG  "(%s) Gatt Interface Not present",__func__);
        return false;
    }
    ALOGE(LOGTAG  "GaiaTest(%s) bda:%s",__func__, bd_str);
    fprintf(stdout, "GaiaTest(%s) bda:%s\n",__func__, bd_str);
    return app_gatt->clientConnect(GetGaiaTestAppData()->clientIf,bd_addr,true,GATT_TRANSPORT_LE);

}

bool GaiaTest::Disconnect(const bt_bdaddr_t *bd_addr)
{
    bdstr_t bd_str;
    bdaddr_to_string(bd_addr, &bd_str[0], sizeof(bd_str));
    if (GetGattInterface() == NULL) {
        ALOGE(LOGTAG  "(%s) Gatt Interface Not present",__func__);
        return false;
    }
    ALOGE(LOGTAG  "GaiaTest(%s) bda:%s",__func__, bd_str);
    fprintf(stdout, "GaiaTest(%s) bda:%s\n",__func__, bd_str);
    return app_gatt->clientDisconnect(GetGaiaTestConnectionData()->clientIf,bd_addr,GetGaiaTestConnectionData()->conn_id);
}

bool GaiaTest::StartScan()
{
    if (GetGattInterface() == NULL) {
        ALOGE(LOGTAG  "(%s) Gatt Interface Not present",__func__);
        return false;
    }
    ALOGE(LOGTAG  "GaiaTest(%s)",__func__);
    fprintf(stdout, "GaiaTest(%s)\n",__func__);
    return app_gatt->scan(true, GetGaiaTestAppData()->clientIf);
}

bool GaiaTest::StopScan()
{
    if (GetGattInterface() == NULL) {
        ALOGE(LOGTAG  "(%s) Gatt Interface Not present",__func__);
        return false;
    }
    ALOGE(LOGTAG  "GaiaTest(%s)",__func__);
    fprintf(stdout, "GaiaTest(%s)\n",__func__);
    return app_gatt->scan(false, GetGaiaTestAppData()->clientIf);
}
bool GaiaTest::WriteCharacteristic(int conn_id, uint16_t handle, int write_type, int len, int auth_req, char* p_value)
{
    if (GetGattInterface() == NULL) {
        ALOGE(LOGTAG  "(%s) Gatt Interface Not present",__func__);
        return false;
    }
    ALOGE(LOGTAG  "GaiaTest(%s) handle:0x%x len:%d\n",__func__, handle, len);
    fprintf(stdout,"GaiaTest(%s) handle:0x%x len:%d\n",__func__, handle, len);
    return app_gatt->write_characteristic(conn_id, handle, write_type, len, auth_req, p_value);

}

bool GaiaTest::SearchService(int conn_id, bt_uuid_t *filter_uuid )
{
    if (GetGattInterface() == NULL) {
        ALOGE(LOGTAG  "(%s) Gatt Interface Not present",__func__);
        return false;
    }
    ALOGE(LOGTAG  "GaiaTest(%s)",__func__);
    fprintf(stdout, "GaiaTest(%s)\n", __func__);
    return app_gatt->search_service(GetGaiaTestConnectionData()->conn_id, NULL);
}
void GaiaTest::Refresh(void)
{
    ALOGE(LOGTAG  "GaiaTest(%s)",__func__);
    fprintf(stdout, "GaiaTest(%s)\n", __func__);
    app_gatt->refresh(GetGaiaTestAppData()->clientIf, &gaia_bda);
}

bt_status_t GaiaTest::ConfigMtu(int conn_id, int mtu)
{
    fprintf(stdout,"GaiaTest(%s)\n", __func__);
    ALOGE(LOGTAG "GaiaTest(%s) mtu:%d",__func__, mtu);
    if (GetGattInterface() == NULL) {
        ALOGE(LOGTAG  "(%s) Gatt Interface Not present",__func__);
        return BT_STATUS_FAIL;
    }
    return app_gatt->configure_mtu(GetGaiaTestConnectionData()->conn_id, mtu);
}

void GaiaTest::Cleanup(void)
{
    fprintf(stdout, " GaiaTest::Cleanup\n");
    ALOGE(LOGTAG " GaiaTest::Cleanup\n");
    GaiaClientCleanup();
}

void GaiaTest::RegisterNotify(int client_if, const bt_bdaddr_t *bda, uint16_t handle)
{
    app_gatt->register_for_notification(client_if, bda, handle);
}
