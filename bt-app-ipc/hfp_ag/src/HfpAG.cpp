/* Copyright (c) 2016, 2019 The Linux Foundation. All rights reserved.
 * Not a Contribution.
 * Copyright (C) 2012-2014 The Android Open Source Project
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

#include <list>
#include <map>
#include <iostream>
#include <vector>
#include <string.h>
#include <hardware/bluetooth.h>
#include <hardware/hardware.h>
#include <hardware/bt_hf.h>
#include "hardware/bt_hf_vendor.h"
#include "hfpag_interface_b.hpp"

#include <unistd.h>
#include <systemdq/sd-bus.h>

#include "HfpAG.hpp"

#define LOGTAG "HFP_AG "

using namespace std;
using std::list;
using std::string;

Hfp_Ag *pHfpAG = NULL;

#ifdef __cplusplus
extern "C" {
#endif

#if defined(BT_MODEM_INTEGRATION)
const char *MCM_LIBRARY_NAME = "/usr/lib/libmcm.so.0";
#endif

vector<BtEvent> memorized_evt;

void BtHfpAgMsgHandler(void *msg) {
    BtEvent* pEvent = NULL;
    if(!msg) {
        printf("Msg is NULL, return.\n");
        return;
    }

    pEvent = ( BtEvent *) msg;

    ALOGD(LOGTAG " BtHfpAgMsgHandler event = %d", pEvent->event_id);
    fprintf(stdout, " BtHfpAgMsgHandler event = %d\n", pEvent->event_id);
    switch(pEvent->event_id) {
        case PROFILE_API_START:
            if (pHfpAG) {
                pHfpAG->HandleEnableAg();
            }
            break;
        case PROFILE_API_STOP:
            if (pHfpAG) {
                pHfpAG->HandleDisableAg();
            }
            break;
        default:
            if(pHfpAG) {
               pHfpAG->ProcessEvent(( BtEvent *) msg);
            }
            break;
    }
    delete pEvent;
}

#ifdef __cplusplus
}
#endif

static void connection_state_callback(bthf_connection_state_t state, bt_bdaddr_t* bd_addr) {
    ALOGD(LOGTAG " Connection State CB");
    BtEvent *pEvent = new BtEvent;
    memcpy(&pEvent->hfp_ag_event.bd_addr, bd_addr, sizeof(bt_bdaddr_t));
    switch( state ) {
        case BTHF_CONNECTION_STATE_DISCONNECTED:
            pEvent->hfp_ag_event.event_id = HFP_AG_DISCONNECTED_CB;
        break;
        case BTHF_CONNECTION_STATE_CONNECTING:
            pEvent->hfp_ag_event.event_id = HFP_AG_CONNECTING_CB;
        break;
        case BTHF_CONNECTION_STATE_CONNECTED:
            pEvent->hfp_ag_event.event_id = HFP_AG_CONNECTED_CB;
        break;
        case BTHF_CONNECTION_STATE_SLC_CONNECTED:
            pEvent->hfp_ag_event.event_id = HFP_AG_SLC_CONNECTED_CB;
        break;
        case BTHF_CONNECTION_STATE_DISCONNECTING:
            pEvent->hfp_ag_event.event_id = HFP_AG_DISCONNECTING_CB;
        break;
        default:
        break;
    }
    PostMessage(THREAD_ID_HFP_AG, pEvent);
}

static void audio_state_callback(bthf_audio_state_t state, bt_bdaddr_t* bd_addr) {
    ALOGD(LOGTAG " Audio State CB");
    BtEvent *pEvent = new BtEvent;
    memcpy(&pEvent->hfp_ag_event.bd_addr, bd_addr, sizeof(bt_bdaddr_t));
    switch( state ) {
        case BTHF_AUDIO_STATE_DISCONNECTED:
            pEvent->hfp_ag_event.event_id = HFP_AG_AUDIO_STATE_DISCONNECTED_CB;
        break;
        case BTHF_AUDIO_STATE_CONNECTING:
            pEvent->hfp_ag_event.event_id = HFP_AG_AUDIO_STATE_CONNECTING_CB;
        break;
        case BTHF_AUDIO_STATE_CONNECTED:
            pEvent->hfp_ag_event.event_id = HFP_AG_AUDIO_STATE_CONNECTED_CB;
        break;
        case BTHF_AUDIO_STATE_DISCONNECTING:
            pEvent->hfp_ag_event.event_id = HFP_AG_AUDIO_STATE_DISCONNECTING_CB;
        break;
    }
    PostMessage(THREAD_ID_HFP_AG, pEvent);
}

void voice_recognition_callback(bthf_vr_state_t state, bt_bdaddr_t* bd_addr) {
    BtEvent *pEvent = new BtEvent;
    ALOGD(LOGTAG "VR state is %s",(state == BTHF_VR_STATE_STOPPED) ? "stopped": "started");
    fprintf(stdout, "VR state is %s\n",(state == BTHF_VR_STATE_STOPPED) ? "stopped": "started");

    memcpy(&pEvent->hfp_ag_event.bd_addr, bd_addr, sizeof(bt_bdaddr_t));
    pEvent->hfp_ag_event.arg1 = state;
    pEvent->hfp_ag_event.event_id = HFP_AG_VR_CB;
    PostMessage(THREAD_ID_HFP_AG, pEvent);
}

void answer_call_callback(bt_bdaddr_t* bd_addr) {
    BtEvent *pEvent = new BtEvent;
    ALOGD(LOGTAG " answer_call_callback");
    fprintf(stdout, " answer_call_callback\n");

    memcpy(&pEvent->hfp_ag_event.bd_addr, bd_addr, sizeof(bt_bdaddr_t));
    pEvent->hfp_ag_event.event_id = HFP_AG_ANSWER_CALL_CB;
    PostMessage(THREAD_ID_HFP_AG, pEvent);
}

void hangup_call_callback(bt_bdaddr_t* bd_addr) {
    BtEvent *pEvent = new BtEvent;
    ALOGD(LOGTAG " hangup_call_callback");
    fprintf(stdout, " hangup_call_callback\n");

    memcpy(&pEvent->hfp_ag_event.bd_addr, bd_addr, sizeof(bt_bdaddr_t));
    pEvent->hfp_ag_event.event_id = HFP_AG_HANGUP_CALL_CB;
    PostMessage(THREAD_ID_HFP_AG, pEvent);
}

void volume_control_callback(bthf_volume_type_t type, int volume, bt_bdaddr_t* bd_addr) {
    BtEvent *pEvent = new BtEvent;
    ALOGD(LOGTAG "%s : %s volume is %d", __func__,
          (type == BTHF_VOLUME_TYPE_SPK) ? "speaker": "mic", volume);

    fprintf(stdout, "%s : %s volume is %d\n", __func__,
          (type == BTHF_VOLUME_TYPE_SPK) ? "speaker": "mic", volume);

    memcpy(&pEvent->hfp_ag_event.bd_addr, bd_addr, sizeof(bt_bdaddr_t));
    pEvent->hfp_ag_event.event_id = HFP_AG_VOL_CONTROL_CB;
    PostMessage(THREAD_ID_HFP_AG, pEvent);
}

void dial_call_callback(char *number, bt_bdaddr_t* bd_addr) {
    BtEvent *pEvent = new BtEvent;
    ALOGD(LOGTAG " dial_call_callback");
    fprintf(stdout, " dial_call_callback\n");

    memcpy(&pEvent->hfp_ag_event.bd_addr, bd_addr, sizeof(bt_bdaddr_t));

    // for AT+BLDN, number will be NULL
    if (number == NULL)
       pEvent->hfp_ag_event.str[0] = '\0';
    else
       strlcpy(pEvent->hfp_ag_event.str, number, strlen(number)+1);

    pEvent->hfp_ag_event.event_id = HFP_AG_DIAL_CALL_CB;
    PostMessage(THREAD_ID_HFP_AG, pEvent);
}

void dtmf_cmd_callback(char dtmf, bt_bdaddr_t* bd_addr) {
    BtEvent *pEvent = new BtEvent;
    ALOGD(LOGTAG " dtmf_cmd_callback");
    fprintf(stdout, " dtmf_cmd_callback\n");

    memcpy(&pEvent->hfp_ag_event.bd_addr, bd_addr, sizeof(bt_bdaddr_t));
    pEvent->hfp_ag_event.arg1 = dtmf;
    pEvent->hfp_ag_event.event_id = HFP_AG_DTMF_CB;
    PostMessage(THREAD_ID_HFP_AG, pEvent);
}

void noice_reduction_callback(bthf_nrec_t nrec, bt_bdaddr_t* bd_addr) {
    BtEvent *pEvent = new BtEvent;
    ALOGD(LOGTAG " noice_reduction_callback");
    fprintf(stdout, " noice_reduction_callback\n");

    memcpy(&pEvent->hfp_ag_event.bd_addr, bd_addr, sizeof(bt_bdaddr_t));
    pEvent->hfp_ag_event.arg1 = nrec;
    pEvent->hfp_ag_event.event_id = HFP_AG_NREC_CB;
    PostMessage(THREAD_ID_HFP_AG, pEvent);
}

void wbs_callback(bthf_wbs_config_t wbs_config, bt_bdaddr_t* bd_addr) {
    BtEvent *pEvent = new BtEvent;
    ALOGD(LOGTAG " wbs_callback");
    fprintf(stdout, " wbs_callback\n");

    memcpy(&pEvent->hfp_ag_event.bd_addr, bd_addr, sizeof(bt_bdaddr_t));
    pEvent->hfp_ag_event.arg1 = wbs_config;
    pEvent->hfp_ag_event.event_id = HFP_AG_WBS_CB;
    PostMessage(THREAD_ID_HFP_AG, pEvent);
}

void at_chld_callback(bthf_chld_type_t chld, bt_bdaddr_t* bd_addr) {
    BtEvent *pEvent = new BtEvent;
    ALOGD(LOGTAG " at_chld_callback");
    fprintf(stdout, " at_chld_callback\n");

    memcpy(&pEvent->hfp_ag_event.bd_addr, bd_addr, sizeof(bt_bdaddr_t));
    pEvent->hfp_ag_event.arg1 = (int)chld;
    pEvent->hfp_ag_event.event_id = HFP_AG_CHLD_CB;
    PostMessage(THREAD_ID_HFP_AG, pEvent);
}

void at_cnum_callback(bt_bdaddr_t* bd_addr) {
    BtEvent *pEvent = new BtEvent;
    ALOGD(LOGTAG " at_cnum_callback");
    fprintf(stdout, " at_cnum_callback\n");

    memcpy(&pEvent->hfp_ag_event.bd_addr, bd_addr, sizeof(bt_bdaddr_t));
    pEvent->hfp_ag_event.event_id = HFP_AG_SUBSCRIBER_INFO_CB;
    PostMessage(THREAD_ID_HFP_AG, pEvent);
}

void at_cind_callback(bt_bdaddr_t* bd_addr) {
    BtEvent *pEvent = new BtEvent;
    ALOGD(LOGTAG " at_cind_callback");
    fprintf(stdout, "at_cind_callback\n");

    memcpy(&pEvent->hfp_ag_event.bd_addr, bd_addr, sizeof(bt_bdaddr_t));
    pEvent->hfp_ag_event.event_id = HFP_AG_CIND_CB;
    PostMessage(THREAD_ID_HFP_AG, pEvent);
}

void at_cops_callback(bt_bdaddr_t* bd_addr) {
    BtEvent *pEvent = new BtEvent;
    ALOGD(LOGTAG " at_cops_callback");
    fprintf(stdout, "at_cops_callback\n");

    memcpy(&pEvent->hfp_ag_event.bd_addr, bd_addr, sizeof(bt_bdaddr_t));
    pEvent->hfp_ag_event.event_id = HFP_AG_COPS_CB;
    PostMessage(THREAD_ID_HFP_AG, pEvent);
}

void at_clcc_callback(bt_bdaddr_t* bd_addr) {
    BtEvent *pEvent = new BtEvent;
    ALOGD(LOGTAG " at_clcc_callback");
    fprintf(stdout, "at_clcc_callback\n");

    memcpy(&pEvent->hfp_ag_event.bd_addr, bd_addr, sizeof(bt_bdaddr_t));
    pEvent->hfp_ag_event.event_id = HFP_AG_CLCC_CB;
    PostMessage(THREAD_ID_HFP_AG, pEvent);
}

void unknown_at_callback(char *at_string, bt_bdaddr_t* bd_addr) {
    BtEvent *pEvent = new BtEvent;
    ALOGD(LOGTAG " unknown_at_callback");
    fprintf(stdout, "unknown_at_callback\n");

    memcpy(&pEvent->hfp_ag_event.bd_addr, bd_addr, sizeof(bt_bdaddr_t));
    pEvent->hfp_ag_event.event_id = HFP_AG_UNKNOWN_AT_CMD_CB;
    PostMessage(THREAD_ID_HFP_AG, pEvent);
}

void key_pressed_callback(bt_bdaddr_t* bd_addr) {
    BtEvent *pEvent = new BtEvent;
    ALOGD(LOGTAG " key_pressed_callback");
    fprintf(stdout, "key_pressed_callback\n");

    memcpy(&pEvent->hfp_ag_event.bd_addr, bd_addr, sizeof(bt_bdaddr_t));
    pEvent->hfp_ag_event.event_id = HFP_AG_KEY_PRESSED_CB;
    PostMessage(THREAD_ID_HFP_AG, pEvent);

}

void bind_callback(char *at_string, bt_bdaddr_t* bd_addr) {
    BtEvent *pEvent = new BtEvent;
    ALOGD(LOGTAG " bind_cmd_vendor_cb");
    fprintf(stdout, " bind_cmd_vendor_cb\n");

    memcpy(&pEvent->hfp_ag_event.bd_addr, bd_addr, sizeof(bt_bdaddr_t));
    strlcpy(pEvent->hfp_ag_event.str, at_string, strlen(at_string)+1);
    pEvent->hfp_ag_event.event_id = HFP_AG_BIND_CB;
    PostMessage(THREAD_ID_HFP_AG, pEvent);
}

void biev_callback(bthf_hf_ind_type_t ind_id, int ind_value,
                                        RawAddress *bd_addr) {
    BtEvent *pEvent = new BtEvent;
    ALOGD(LOGTAG " biev_cmd_vendor_cb");
    fprintf(stdout, " biev_cmd_vendor_cb\n");

    memcpy(&pEvent->hfp_ag_event.bd_addr, bd_addr, sizeof(bt_bdaddr_t));
    pEvent->hfp_ag_event.arg1 = ind_id;
    pEvent->hfp_ag_event.arg1 = ind_value;
    pEvent->hfp_ag_event.event_id = HFP_AG_BIEV_CB;
    PostMessage(THREAD_ID_HFP_AG, pEvent);
}

void bind_cmd_vendor_cb(char* hf_ind, bthf_vendor_bind_type_t type, bt_bdaddr_t* bd_addr) {
    BtEvent *pEvent = new BtEvent;
    ALOGD(LOGTAG " bind_cmd_vendor_cb");
    fprintf(stdout, " bind_cmd_vendor_cb\n");

    memcpy(&pEvent->hfp_ag_event.bd_addr, bd_addr, sizeof(bt_bdaddr_t));
    strlcpy(pEvent->hfp_ag_event.str, hf_ind, strlen(hf_ind)+1);
    pEvent->hfp_ag_event.arg1 = type;
    pEvent->hfp_ag_event.event_id = HFP_AG_BIND_CB;
    PostMessage(THREAD_ID_HFP_AG, pEvent);
}

void biev_cmd_vendor_cb(char* hf_ind_val, bt_bdaddr_t* bd_addr) {
    BtEvent *pEvent = new BtEvent;
    ALOGD(LOGTAG " biev_cmd_vendor_cb");
    fprintf(stdout, " biev_cmd_vendor_cb\n");

    memcpy(&pEvent->hfp_ag_event.bd_addr, bd_addr, sizeof(bt_bdaddr_t));
    strlcpy(pEvent->hfp_ag_event.str, hf_ind_val, strlen(hf_ind_val)+1);
    pEvent->hfp_ag_event.event_id = HFP_AG_BIEV_CB;
    PostMessage(THREAD_ID_HFP_AG, pEvent);
}

static bthf_callbacks_t sBluetoothHfpAgCallbacks = {
    sizeof(sBluetoothHfpAgCallbacks),
    connection_state_callback,
    audio_state_callback,
    voice_recognition_callback,
    answer_call_callback,
    hangup_call_callback,
    volume_control_callback,
    dial_call_callback,
    dtmf_cmd_callback,
    noice_reduction_callback,
    wbs_callback,
    at_chld_callback,
    at_cnum_callback,
    at_cind_callback,
    at_cops_callback,
    at_clcc_callback,
    unknown_at_callback,
    bind_callback,
    biev_callback,
    key_pressed_callback
};

static bthf_vendor_callbacks_t sBluetoothHfpAgVendorCallbacks = {
    sizeof(sBluetoothHfpAgVendorCallbacks),
    bind_cmd_vendor_cb,
    biev_cmd_vendor_cb,
};

#if defined(BT_MODEM_INTEGRATION)
void ril_ind_cb(mcm_client_handle_type hndl, uint32 msg_id,
                     void *ind_c_struct, uint32 ind_len) {
   BtEvent *pEvent = new BtEvent;
   fprintf(stdout, "%s: indications is %u\n",__func__, msg_id);
   ALOGD(LOGTAG "%s: indications is %u\n", __func__, msg_id);

   if (ind_c_struct == NULL) {
       ALOGE(LOGTAG "%s: indication data is NULL", __func__);
       fprintf(stdout, "indication data is NULL\n");
       return;
   }

   pEvent->hfp_ag_event.hdl = hndl;
   pEvent->hfp_ag_event.msg_id = msg_id;
   pEvent->hfp_ag_event.data_length = ind_len;
   memcpy(&pEvent->hfp_ag_event.data, ind_c_struct, ind_len);
   pEvent->hfp_ag_event.event_id = HFP_AG_RIL_IND_CB;
   PostMessage(THREAD_ID_HFP_AG, pEvent);
}

void ril_resp_cb(mcm_client_handle_type hndl, uint32 msg_id,
                      void *resp_c_struct, uint32 resp_len, void *token_id){
   BtEvent *pEvent = new BtEvent;
   fprintf(stdout, "%s: response msg %u\n", __func__, msg_id);
   ALOGD(LOGTAG "%s: response msg  is %u\n", __func__, msg_id);

   if (resp_c_struct == NULL) {
       ALOGE(LOGTAG "%s: response data is NULL", __func__);
       fprintf(stdout, "response data is NULL\n");
       return;
   }

   pEvent->hfp_ag_event.hdl = hndl;
   pEvent->hfp_ag_event.msg_id = msg_id;
   pEvent->hfp_ag_event.data_length = resp_len;
   memcpy(&pEvent->hfp_ag_event.data, resp_c_struct, resp_len);
   pEvent->hfp_ag_event.event_id = HFP_AG_RIL_RESP_CB;
   PostMessage(THREAD_ID_HFP_AG, pEvent);
}
#endif

/*
MM-Audio integration for SCO
recording session through audio HAL
playback through audio HAL
 */
void Hfp_Ag::configurescoaudio(bool enable) {
    ALOGD(LOGTAG "configurescoaudio - enable:%d",enable);
    fprintf(stdout, "configurescoaudio - enable:%d\n",enable);

#if defined(BT_AUDIO_HAL_INTEGRATION)
#else
    ALOGD("%s: BT_AUDIO_HAL_INTEGRATION needs to be defined", __func__);
    fprintf(stdout, "BT_AUDIO_HAL_INTEGRATION needs to be defined\n");
#endif
}
static void *start_playback(void *in_param) {
#if defined(BT_AUDIO_HAL_INTEGRATION)
#else
    ALOGD("%s: BT_AUDIO_HAL_INTEGRATION needs to be defined", __func__);
    fprintf(stdout, "BT_AUDIO_HAL_INTEGRATION needs to be defined\n");
#endif
    return NULL;
}
static void *start_record(void *in_param) {
    ALOGD(LOGTAG "start_record - start");
    fprintf(stdout, "start_record - start\n");

#if defined(BT_AUDIO_HAL_INTEGRATION)
#else
    ALOGD("%s: BT_AUDIO_HAL_INTEGRATION needs to be defined", __func__);
    fprintf(stdout, "BT_AUDIO_HAL_INTEGRATION needs to be defined\n");
#endif
    return NULL;
}

void Hfp_Ag::HandleEnableAg(void) {

  hfpag_interface_b_init();
  
  sBtHfpAgInterface = get_profile_interface_hfpag();
  sBtHfpAgVendorInterface = get_profile_interface_hfpag_vendor ();
  change_state(HFP_AG_STATE_DISCONNECTED);
  sBtHfpAgInterface->init(&sBluetoothHfpAgCallbacks, 1, false);
  mActiveCallsNum = 0;
  mHeldCallsNum = 0;
  sBtHfpAgVendorInterface->init_vendor(&sBluetoothHfpAgVendorCallbacks);
#if defined(BT_MODEM_INTEGRATION)
  init_modem();
#endif
  init_audio();
}

void Hfp_Ag::HandleDisableAg(void) {
   change_state(HFP_AG_STATE_NOT_STARTED);
   if(sBtHfpAgInterface != NULL) {
       sBtHfpAgInterface->cleanup();
       sBtHfpAgInterface = NULL;
   }
   if(sBtHfpAgVendorInterface != NULL) {
       sBtHfpAgVendorInterface->cleanup_vendor();
       sBtHfpAgVendorInterface = NULL;
   }

   hfpag_interface_b_deinit();
   configurescoaudio(false);
   
   mActiveCallsNum = 0;
   mHeldCallsNum = 0;
   number_vec.clear();
#if defined(BT_MODEM_INTEGRATION)
   release_modem();
#endif
   release_audio();
}

void Hfp_Ag::ProcessEvent(BtEvent* pEvent) {
    ALOGD(LOGTAG " Processing event %d", pEvent->event_id);
    fprintf(stdout, " AG: Processing event = %d\n", pEvent->event_id);
    switch(mAgState) {
        case HFP_AG_STATE_DISCONNECTED:
            state_disconnected_handler(pEvent);
            break;
        case HFP_AG_STATE_PENDING:
            state_pending_handler(pEvent);
            break;
        case HFP_AG_STATE_CONNECTED:
            state_connected_handler(pEvent);
            break;
        case HFP_AG_STATE_AUDIO_ON:
            state_audio_on_handler(pEvent);
            break;
        case HFP_AG_STATE_NOT_STARTED:
            ALOGE(LOGTAG " STATE UNINITIALIZED, return");
            break;
    }
}

void Hfp_Ag::state_disconnected_handler(BtEvent* pEvent) {
    char str[18];
    ALOGD(LOGTAG "state_disconnected_handler Processing event %d", pEvent->event_id);
    fprintf(stdout, "state_disconnected_handler Processing event %d\n", pEvent->event_id);
    switch(pEvent->event_id) {
        case HFP_AG_API_CONNECT_REQ:
            memcpy(&mConnectingDevice, &pEvent->hfp_ag_event.bd_addr, sizeof(bt_bdaddr_t));
            if (sBtHfpAgInterface != NULL) {
                sBtHfpAgInterface->connect(&pEvent->hfp_ag_event.bd_addr);
            }
            bdaddr_to_string(&mConnectingDevice, str, 18);
            fprintf(stdout, "connecting with device %s", str);
            ALOGD(LOGTAG " connecting with device %s", str);
            change_state(HFP_AG_STATE_PENDING);
            break;
        case HFP_AG_CONNECTING_CB:
            memcpy(&mConnectingDevice, &pEvent->hfp_ag_event.bd_addr, sizeof(bt_bdaddr_t));
            change_state(HFP_AG_STATE_PENDING);
            break;
        case HFP_AG_CONNECTED_CB:
            memset(&mConnectingDevice, 0, sizeof(bt_bdaddr_t));
            memcpy(&mConnectedDevice, &pEvent->hfp_ag_event.bd_addr, sizeof(bt_bdaddr_t));

            bdaddr_to_string(&mConnectedDevice, str, 18);
            fprintf(stdout, " connected with device %s", str);
            ALOGD(LOGTAG " connected with device %s", str);

            change_state(HFP_AG_STATE_CONNECTED);
            break;
        case HFP_AG_UPDATE_ACTIVE_CALL_NUM:
            update_activecall_num(pEvent->hfp_ag_event.arg1);
            break;
        case HFP_AG_UPDATE_HELD_CALL_NUM:
            update_heldcall_num(pEvent->hfp_ag_event.arg1);
            break;
        case HFP_AG_ADD_NUMBER:
            if (number_vec.size() < 2) {
              number_vec.insert(number_vec.end(), pEvent->hfp_ag_event.str);
              fprintf(stdout, "\n %s - number added ", pEvent->hfp_ag_event.str);
              ALOGD(LOGTAG "%s - number added ", pEvent->hfp_ag_event.str);
            } else {
              fprintf(stdout, "\n Can not add more than 2 numbers ");
              ALOGD(LOGTAG "Can not add more than 2 numbers ");
            }
            break;
        case HFP_AG_DELETE_NUMBER:
            if (number_vec.size() > 0) {
              number_vec.pop_back();
              fprintf(stdout, "\n number deleted ");
              ALOGD(LOGTAG "number deleted ");
            } else {
              fprintf(stdout, "\n all numbers deleted/no Number added to delete ");
              ALOGD(LOGTAG " all numbers deleted no Number added to delete ");
            }
            break;
        default:
            ALOGD(LOGTAG " event not handled %d ", pEvent->event_id);
            break;
    }
}
void Hfp_Ag::state_pending_handler(BtEvent* pEvent) {
    char str[18];
    ALOGD(LOGTAG "state_pending_handler Processing event %d", pEvent->event_id);
    fprintf(stdout, "state_pending_handler Processing event %d\n", pEvent->event_id);
    switch(pEvent->event_id) {
        case HFP_AG_CONNECTING_CB:
            break;
        case HFP_AG_CONNECTED_CB:
            memcpy(&mConnectedDevice, &pEvent->hfp_ag_event.bd_addr, sizeof(bt_bdaddr_t));
            memset(&mConnectingDevice, 0, sizeof(bt_bdaddr_t));

            bdaddr_to_string(&mConnectedDevice, str, 18);
            fprintf(stdout, "connected with device %s", str);
            ALOGD(LOGTAG "connected with device %s", str);
            change_state(HFP_AG_STATE_CONNECTED);
            break;
        case HFP_AG_DISCONNECTED_CB:
            bdaddr_to_string(&pEvent->hfp_ag_event.bd_addr, str, 18);
            fprintf(stdout, "Disconnected from or Unable to connect with device %s", str);
            ALOGD(LOGTAG "Disconnected from or Unable to connect with device %s", str);

            memset(&mConnectedDevice, 0, sizeof(bt_bdaddr_t));
            memset(&mConnectingDevice, 0, sizeof(bt_bdaddr_t));
            change_state(HFP_AG_STATE_DISCONNECTED);
            break;
        case HFP_AG_AUDIO_STATE_DISCONNECTED_CB:

            bdaddr_to_string(&pEvent->hfp_ag_event.bd_addr, str, 18);
            fprintf(stdout, "Disconnected SCO connection with device %s", str);
            ALOGD(LOGTAG "Disconnected SCO connection with device %s", str);
#if defined(BT_ALSA_AUDIO_INTEGRATION)
            teardown_sco_path();
#endif
            configurescoaudio(false);
            if(memorized_evt.empty() == true) {
                change_state(HFP_AG_STATE_CONNECTED);
            }
            else {
                if(memorized_evt[memorized_evt.size() -1].event_id == HFP_AG_API_DISCONNECT_REQ) {
                    bt_status_t ret_val = sBtHfpAgInterface->disconnect(
                             &memorized_evt[memorized_evt.size() -1].hfp_ag_event.bd_addr);
                    if (ret_val != BT_STATUS_SUCCESS) {
                       fprintf(stdout, "Failure disconnecting with device %s", str);
                       ALOGD(LOGTAG "Failure disconnecting with device %s", str);
                       break;
                    }
                    memorized_evt.pop_back();
                }
            }
            break;
        default:
            ALOGD(LOGTAG " event not handled %d ", pEvent->event_id);
            break;
    }
}

void Hfp_Ag::state_connected_handler(BtEvent* pEvent) {
    ALOGD(LOGTAG "state_connected_handler Processing event %d", pEvent->event_id);
    fprintf(stdout, "state_connected_handler Processing event = %d", pEvent->event_id);
    char str[18];
    BtEvent *pControlRequest, *pReleaseControlReq;
    switch(pEvent->event_id) {
        case HFP_AG_API_CONNECT_REQ: // TODO: handle connections to another device
            break;
        case HFP_AG_API_DISCONNECT_REQ:
            bdaddr_to_string(&pEvent->hfp_ag_event.bd_addr, str, 18);
            if (sBtHfpAgInterface != NULL) {
                bt_status_t ret_val;
                ret_val = sBtHfpAgInterface->disconnect(&pEvent->hfp_ag_event.bd_addr);
                if (ret_val != BT_STATUS_SUCCESS) {
                    fprintf(stdout, "Failure disconnecting with device %s", str);
                    ALOGD(LOGTAG "Failure disconnecting with device %s", str);
                    break;
                }
            }

            fprintf(stdout, "Disconnecting with device %s", str);
            ALOGD(LOGTAG "Disconnecting with device %s", str);
            memset(&mConnectedDevice, 0, sizeof(bt_bdaddr_t));
            memset(&mConnectingDevice, 0, sizeof(bt_bdaddr_t));
            change_state(HFP_AG_STATE_PENDING);
            break;
        case HFP_AG_SLC_CONNECTED_CB:
            bdaddr_to_string(&pEvent->hfp_ag_event.bd_addr, str, 18);
            fprintf(stdout, "SLC connected with device %s", str);
            ALOGD(LOGTAG " SLC connected with device %s", str);
            bt_status_t ret_val;
            ret_val = sBtHfpAgInterface->set_active_device(&pEvent->hfp_ag_event.bd_addr);
            if (ret_val != BT_STATUS_SUCCESS) {
                fprintf(stdout, "Failure setting active device %s", str);
                ALOGD(LOGTAG "Failure setting active device %s", str);
                break;
            }
#if defined(BT_MODEM_INTEGRATION)
            processSlcConnected();
#endif
            break;
        case HFP_AG_DISCONNECTED_CB:
            bdaddr_to_string(&pEvent->hfp_ag_event.bd_addr, str, 18);
            fprintf(stdout, "Disconnected with device %s", str);
            ALOGD(LOGTAG "Disconnected with device %s", str);

            memset(&mConnectedDevice, 0, sizeof(bt_bdaddr_t));
            memset(&mConnectingDevice, 0, sizeof(bt_bdaddr_t));
            change_state(HFP_AG_STATE_DISCONNECTED);
            break;
        case HFP_AG_DISCONNECTING_CB:
            break;
        case HFP_AG_VOIP_CALL_INDICATION:
            VoipCallInd(&pEvent->hfp_ag_event.bd_addr);
            break;
        case HFP_AG_VOIP_CALL_TERMINATION:
            EndVoipCall(&pEvent->hfp_ag_event.bd_addr);
            break;
        case HFP_AG_VOIP_CALL_INCOMING_INDICATION:
            VoipCallIncomingInd(&pEvent->hfp_ag_event.bd_addr,pEvent->hfp_ag_event.str,
                                pEvent->hfp_ag_event.arg1);
            break;
        case HFP_AG_VOIP_CALL_ACCEPT:
            AcceptVoipCall(&pEvent->hfp_ag_event.bd_addr);
            break;
        case HFP_AG_VOIP_CALL_SWAP:
            SwapVoipCall(&pEvent->hfp_ag_event.bd_addr);
            break;
        case HFP_AG_UPDATE_ACTIVE_CALL_NUM:
            update_activecall_num(pEvent->hfp_ag_event.arg1);
            break;
        case HFP_AG_UPDATE_HELD_CALL_NUM:
            update_heldcall_num(pEvent->hfp_ag_event.arg1);
            break;
        case HFP_AG_ADD_NUMBER:
            if (number_vec.size() < 2) {
              number_vec.insert(number_vec.end(), pEvent->hfp_ag_event.str);
              fprintf(stdout, "\n %s - number added ", pEvent->hfp_ag_event.str);
              ALOGD(LOGTAG "%s - number added ", pEvent->hfp_ag_event.str);
            } else {
              fprintf(stdout, "\n Can not add more than 2 numbers ");
              ALOGD(LOGTAG "Can not add more than 2 numbers ");
            }
            break;
        case HFP_AG_DELETE_NUMBER:
            if (number_vec.size() > 0) {
              number_vec.pop_back();
              fprintf(stdout, "\n number deleted ");
              ALOGD(LOGTAG "number deleted ");
            } else {
              fprintf(stdout, "\n all numbers deleted/no Number added to delete ");
              ALOGD(LOGTAG " all numbers deleted no Number added to delete ");
            }
            break;
        case HFP_AG_SEND_DEVICE_STAT_NOTFY:
            bdaddr_to_string(&pEvent->hfp_ag_event.bd_addr, str, 18);
            fprintf(stdout, "VHFP_AG_SEND_DEVICE_STAT_NOTFY %s", str);
            ALOGD(LOGTAG "HFP_AG_SEND_DEVICE_STAT_NOTFY %s", str);
            if(sBtHfpAgInterface != NULL) {
              if (pEvent->hfp_ag_event.arg1 == 0){
                fprintf(stdout, " network not avaialble \n ");
                ALOGD(LOGTAG "HFP_AG_SEND_DEVICE_STAT_NOTFY ");
                sBtHfpAgInterface->device_status_notification(BTHF_NETWORK_STATE_NOT_AVAILABLE,
                                BTHF_SERVICE_TYPE_HOME, pEvent->hfp_ag_event.arg2,
                                pEvent->hfp_ag_event.arg3, &pEvent->hfp_ag_event.bd_addr);
              } else if (pEvent->hfp_ag_event.arg1 == 1){
                fprintf(stdout, " network avaialble \n ");
                ALOGD(LOGTAG "HFP_AG_SEND_DEVICE_STAT_NOTFY ");
                sBtHfpAgInterface->device_status_notification(BTHF_NETWORK_STATE_AVAILABLE,
                                BTHF_SERVICE_TYPE_HOME, pEvent->hfp_ag_event.arg2,
                                pEvent->hfp_ag_event.arg3, &pEvent->hfp_ag_event.bd_addr);
              } else
                fprintf(stdout, " Invalid input \n ");
            }
            break;
        case HFP_AG_VR_CB:
            bdaddr_to_string(&pEvent->hfp_ag_event.bd_addr, str, 18);
            fprintf(stdout, "VR start/stop req from device %s", str);
            ALOGD(LOGTAG "VR start/stop req from device %s", str);

            if (sBtHfpAgInterface != NULL) {
                sBtHfpAgInterface->at_response(BTHF_AT_RESPONSE_ERROR, 0,
                                               &pEvent->hfp_ag_event.bd_addr);
            }
            break;
        case HFP_AG_WBS_CB:
            mWbsState = (bthf_wbs_config_t)pEvent->hfp_ag_event.arg1;
            break;
        case HFP_AG_NREC_CB:
            mNrec = (bthf_nrec_t)pEvent->hfp_ag_event.arg1;
            break;
        case HFP_AG_ANSWER_CALL_CB:
            // answer call using RIL APIs.
            // OK will be sent from stack itself.
#if defined(BT_MODEM_INTEGRATION)
            send_voice_cmd(MCM_VOICE_CALL_ANSWER_V01);
#else
            if(sBtHfpAgInterface != NULL) {
              sBtHfpAgInterface->phone_state_change(1,0,BTHF_CALL_STATE_IDLE,"",
                    BTHF_CALL_ADDRTYPE_INTERNATIONAL, &pEvent->hfp_ag_event.bd_addr);
            }
#endif
            break;
        case HFP_AG_HANGUP_CALL_CB:
#if defined(BT_MODEM_INTEGRATION)
            end_call(BTHF_CALL_STATE_ACTIVE);
#else
            EndVoipCall(&pEvent->hfp_ag_event.bd_addr);
#endif
            break;
        case HFP_AG_VOL_CONTROL_CB:
            // TODO: change the speaker volume using mm audio shell script
            // OK will be sent from stack itself.
            break;
        case HFP_AG_DIAL_CALL_CB:
#if defined(BT_MODEM_INTEGRATION)
            dial_call(pEvent->hfp_ag_event.str, &pEvent->hfp_ag_event.bd_addr);
#else
            if((number_vec.size() == 0) && ((pEvent->hfp_ag_event.str[0] == '>')
                || (pEvent->hfp_ag_event.str[0] == '\0'))) {
              // if we don't add any number , send error
              // if it is redial request and we don't have last dialed number, send error
              // if memory dialing is requested, send error
              if (sBtHfpAgInterface != NULL)
                sBtHfpAgInterface->at_response(BTHF_AT_RESPONSE_ERROR, 0,
                                               &pEvent->hfp_ag_event.bd_addr);
            }else {
              if (sBtHfpAgInterface != NULL)
                sBtHfpAgInterface->at_response(BTHF_AT_RESPONSE_OK, 0,
                                               &pEvent->hfp_ag_event.bd_addr);
                sBtHfpAgInterface->phone_state_change(0,0,BTHF_CALL_STATE_DIALING,"",
                        BTHF_CALL_ADDRTYPE_INTERNATIONAL, &pEvent->hfp_ag_event.bd_addr);
                usleep(20000);
                sBtHfpAgInterface->phone_state_change(0,0,BTHF_CALL_STATE_ALERTING,"",
                        BTHF_CALL_ADDRTYPE_INTERNATIONAL, &pEvent->hfp_ag_event.bd_addr);
            }
#endif
            break;
        case HFP_AG_CIND_CB:
            bdaddr_to_string(&pEvent->hfp_ag_event.bd_addr, str, 18);
            fprintf(stdout, "Sending CIND resp to device %s", str);
            ALOGD(LOGTAG "Sending CIND resp to device %s", str);

            if (sBtHfpAgInterface != NULL) {
#if defined(BT_MODEM_INTEGRATION)
                // we already have active/held/ringing call, call setup info. send it to stack
                sBtHfpAgInterface->cind_response(1, mNumActiveCalls, mNumHeldCalls,
                                    mCallSetupState, 5, 0, 5, &pEvent->hfp_ag_event.bd_addr);
#else
                sBtHfpAgInterface->cind_response(1, mActiveCallsNum, mHeldCallsNum,
                                    BTHF_CALL_STATE_IDLE, 5, 0, 5, &pEvent->hfp_ag_event.bd_addr);
#endif
            }
            break;
        case HFP_AG_CHLD_CB:
#if defined(BT_MODEM_INTEGRATION)
            {
               uint32 ret_val = 0;
               ret_val = process_chld(pEvent->hfp_ag_event.arg1);
               if (ret_val != MCM_SUCCESS_V01) {
                   ALOGE(LOGTAG, "error processing chld %d", pEvent->hfp_ag_event.arg1);
                   fprintf(stdout, "error processing chld %d", pEvent->hfp_ag_event.arg1);
               }

               if (sBtHfpAgInterface != NULL) {
                   sBtHfpAgInterface->at_response( (ret_val == MCM_SUCCESS_V01)
                         ? BTHF_AT_RESPONSE_OK : BTHF_AT_RESPONSE_ERROR,
                         0, &pEvent->hfp_ag_event.bd_addr);
               }
            }
#else
            if (sBtHfpAgInterface != NULL) {
                sBtHfpAgInterface->at_response(BTHF_AT_RESPONSE_ERROR, 0,
                                               &pEvent->hfp_ag_event.bd_addr);
            }
#endif
            break;
        case HFP_AG_COPS_CB:
            bdaddr_to_string(&pEvent->hfp_ag_event.bd_addr, str, 18);
            fprintf(stdout, "Sending COPS resp to device %s", str);
            ALOGD(LOGTAG "Sending COPS resp to device %s", str);

#if defined(BT_MODEM_INTEGRATION)
            //  info needs to be fetched from RIL for MDM
            get_and_send_operator_name(&pEvent->hfp_ag_event.bd_addr);
#else
            if (sBtHfpAgInterface != NULL) {
                sBtHfpAgInterface->cops_response("", &pEvent->hfp_ag_event.bd_addr);
            }
#endif
            break;
        case HFP_AG_SUBSCRIBER_INFO_CB:
            bdaddr_to_string(&pEvent->hfp_ag_event.bd_addr, str, 18);
            fprintf(stdout, "Sending CNUM resp to device %s", str);
            ALOGD(LOGTAG "Sending CNUM resp to device %s", str);

#if defined(BT_MODEM_INTEGRATION)
            //  info needs to be fetched from RIL for MDM
            get_and_send_subscriber_number(&pEvent->hfp_ag_event.bd_addr);
#else
            if (sBtHfpAgInterface != NULL) {
                sBtHfpAgInterface->at_response(BTHF_AT_RESPONSE_OK, 0,
                                               &pEvent->hfp_ag_event.bd_addr);
            }
#endif
            break;
        case HFP_AG_CLCC_CB:
            bdaddr_to_string(&pEvent->hfp_ag_event.bd_addr, str, 18);
            fprintf(stdout, "Sending CLCC resp to device %s", str);
            ALOGD(LOGTAG "Sending CLCC resp to device %s", str);

            // TODO: cross check if we need to call get_current_calls here.

            if (sBtHfpAgInterface != NULL) {
#if defined(BT_MODEM_INTEGRATION)
                for (int i = 0; i < MCM_MAX_VOICE_CALLS_V01; i++) {
                    if (mCalls[i].call_id != 0xFFFFFFFF)
                        sBtHfpAgInterface->clcc_response(mCalls[i].idx,
                                                         mCalls[i].dir,
                                                         mCalls[i].stat,
                                                         mCalls[i].mode,
                                                         mCalls[i].mpty,
                                                         mCalls[i].number,
                                                         mCalls[i].numType,
                                                         &pEvent->hfp_ag_event.bd_addr);
                }
#else
                if (number_vec.size() > 0) {
                  int call_idx = 1;
                  //FOR PTS - just adding code to send CLCC
                  for (int i = 0; i < mActiveCallsNum; i++) {
                    sBtHfpAgInterface->clcc_response(call_idx,
                                                     BTHF_CALL_DIRECTION_INCOMING,
                                                     BTHF_CALL_STATE_HELD,
                                                     BTHF_CALL_TYPE_VOICE,
                                                     BTHF_CALL_MPTY_TYPE_SINGLE,
                                                     number_vec[call_idx-1],
                                                     BTHF_CALL_ADDRTYPE_INTERNATIONAL,
                                                     &pEvent->hfp_ag_event.bd_addr);
                    call_idx++;
                  }
                  for (int i = 0; i < mHeldCallsNum; i++) {
                    sBtHfpAgInterface->clcc_response(call_idx,
                                                     BTHF_CALL_DIRECTION_INCOMING,
                                                     BTHF_CALL_STATE_ACTIVE,
                                                     BTHF_CALL_TYPE_VOICE,
                                                     BTHF_CALL_MPTY_TYPE_SINGLE,
                                                     number_vec[call_idx-1],
                                                     BTHF_CALL_ADDRTYPE_INTERNATIONAL,
                                                     &pEvent->hfp_ag_event.bd_addr);
                    call_idx++;
                  }
                }
#endif
                // just send OK for now
                sBtHfpAgInterface->at_response(BTHF_AT_RESPONSE_OK, 0, &pEvent->hfp_ag_event.bd_addr);
            }
            break;
         case HFP_AG_UNKNOWN_AT_CMD_CB:
            if (sBtHfpAgInterface != NULL) {
                sBtHfpAgInterface->at_response(BTHF_AT_RESPONSE_ERROR, 0, &pEvent->hfp_ag_event.bd_addr);
            }
         break;
#if defined(BT_MODEM_INTEGRATION)
         case HFP_AG_RIL_IND_CB:
             process_ril_ind(pEvent);
             break;
         case HFP_AG_RIL_RESP_CB:
             process_ril_resp(pEvent);
             break;
#endif
        case HFP_AG_KEY_PRESSED_CB:
            bdaddr_to_string(&pEvent->hfp_ag_event.bd_addr, str, 18);
            fprintf(stdout, "key press cb- AcceptVoipCall %s", str);
            ALOGD(LOGTAG "key press cb- AcceptVoipCall %s", str);

            AcceptVoipCall(&pEvent->hfp_ag_event.bd_addr);
            break;
        case HFP_AG_API_CONNECT_AUDIO_REQ:
            bdaddr_to_string(&pEvent->hfp_ag_event.bd_addr, str, 18);
            fprintf(stdout, "Connecting SCO/eSCO with device %s", str);
            ALOGD(LOGTAG "Connecting SCO/eSCO with device %s", str);

            if (sBtHfpAgInterface != NULL) {
               bt_status_t status = sBtHfpAgInterface->set_sco_allowed(true);
               if (status != BT_STATUS_SUCCESS)
                 ALOGD("Failed HF set sco allowed, status: %d", status);
               else
                sBtHfpAgInterface->connect_audio(&pEvent->hfp_ag_event.bd_addr);
            }
            break;
        case HFP_AG_AUDIO_STATE_CONNECTED_CB:
            bdaddr_to_string(&pEvent->hfp_ag_event.bd_addr, str, 18);
            fprintf(stdout, "SCO/eSCO connected with device %s, codec %s", str,
                ((mWbsState == BTHF_WBS_YES)? "WBS": "NBS"));
            ALOGD(LOGTAG "SCO/eSCO connected with device %s, codec %s", str,
                ((mWbsState == BTHF_WBS_YES)? "WBS": "NBS"));
            setup_sco_path();
            change_state(HFP_AG_STATE_AUDIO_ON);
            configurescoaudio(true);
            break;
        case HFP_AG_BIND_CB:
            process_at_bind(pEvent);
            break;
        case HFP_AG_BIEV_CB:
            process_at_biev(pEvent);
            break;
        case HFP_AG_API_ACCEPT_CALL_REQ:
            if (sBtHfpAgInterface != NULL) {
            }
            break;
        case HFP_AG_API_RELEASE_HELD_CALL_REQ:
            if (sBtHfpAgInterface != NULL) {
            }
            break;
        case HFP_AG_API_REJECT_CALL_REQ:
            break;
        case HFP_AG_API_END_CALL_REQ:
            if (sBtHfpAgInterface != NULL) {
            }
            break;
        case HFP_AG_API_HOLD_CALL_REQ:
            break;
        case HFP_AG_API_SWAP_CALLS_REQ:
            if (sBtHfpAgInterface != NULL) {
            }
            break;
        case HFP_AG_API_DIAL_REQ:
            if (sBtHfpAgInterface != NULL) {
            }
            break;
        case HFP_AG_API_REDIAL_REQ:
            if (sBtHfpAgInterface != NULL) {
            }
            break;
        case HFP_AG_API_START_VR_REQ:
            if (sBtHfpAgInterface != NULL) {
                sBtHfpAgInterface->start_voice_recognition(&mConnectedDevice);
            }
            break;
        case HFP_AG_API_STOP_VR_REQ:
            if (sBtHfpAgInterface != NULL) {
                sBtHfpAgInterface->stop_voice_recognition(&mConnectedDevice);
            }
            break;
        case HFP_AG_API_QUERY_CURRENT_CALLS_REQ:
            if (sBtHfpAgInterface != NULL) {
            }
            break;
        case HFP_AG_API_QUERY_OPERATOR_NAME_REQ:
            if (sBtHfpAgInterface != NULL) {
            }
            break;
        case HFP_AG_API_QUERY_SUBSCRIBER_INFO_REQ:
            if (sBtHfpAgInterface != NULL) {
            }
            break;
        case HFP_AG_API_SPK_VOL_CTRL_REQ:
            if (sBtHfpAgInterface != NULL) {
                sBtHfpAgInterface->volume_control(BTHF_VOLUME_TYPE_SPK,
                                          pEvent->hfp_ag_event.arg1, &mConnectedDevice);
            }
            break;
        case HFP_AG_API_MIC_VOL_CTRL_REQ:
            if (sBtHfpAgInterface != NULL) {
                sBtHfpAgInterface->volume_control(BTHF_VOLUME_TYPE_MIC,
                                          pEvent->hfp_ag_event.arg1, &mConnectedDevice);
            }
            break;
        default:
            ALOGD(LOGTAG," event not handled %d ", pEvent->event_id);
            break;
    }
}

void Hfp_Ag::state_audio_on_handler(BtEvent* pEvent) {
    char str[18];
    BtEvent *pControlRequest, *pReleaseControlReq;
    ALOGD(LOGTAG "state_audio_on_handler Processing event %d", pEvent->event_id);
    switch(pEvent->event_id) {
        case HFP_AG_API_DISCONNECT_REQ:
            bdaddr_to_string(&pEvent->hfp_ag_event.bd_addr, str, 18);

            // disconnect SCO, clean up SCO
            teardown_sco_path();
            if (sBtHfpAgInterface != NULL) {
                bt_status_t ret_val;
                // no need to check if disconnection of SCO is success here.
                sBtHfpAgInterface->disconnect_audio(&pEvent->hfp_ag_event.bd_addr);

                BtEvent tmpEvent;
                memcpy(&tmpEvent, pEvent, sizeof(BtEvent));
                memorized_evt.push_back(tmpEvent);
            }

            fprintf(stdout, "Disconnecting with device %s", str);
            ALOGD(LOGTAG "Disconnecting with device %s", str);
            change_state(HFP_AG_STATE_PENDING);
            break;
        case HFP_AG_API_DISCONNECT_AUDIO_REQ:
            if (sBtHfpAgInterface != NULL) {
                sBtHfpAgInterface->disconnect_audio(&pEvent->hfp_ag_event.bd_addr);
            }

            bdaddr_to_string(&pEvent->hfp_ag_event.bd_addr, str, 18);
            fprintf(stdout, "Disconnecting SCO/eSCO with device %s", str);
            ALOGD(LOGTAG "Disconnecting SCO/eSCO with device %s", str);
            break;
        case HFP_AG_AUDIO_STATE_DISCONNECTED_CB:

            bdaddr_to_string(&pEvent->hfp_ag_event.bd_addr, str, 18);
            fprintf(stdout, "Disconnected SCO connection with device %s", str);
            ALOGD(LOGTAG "Disconnected SCO connection with device %s", str);

            teardown_sco_path();
            configurescoaudio(false);
            change_state(HFP_AG_STATE_CONNECTED);
            break;
        case HFP_AG_VOIP_CALL_INDICATION:
            VoipCallInd(&pEvent->hfp_ag_event.bd_addr);
            break;
        case HFP_AG_VOIP_CALL_TERMINATION:
            EndVoipCall(&pEvent->hfp_ag_event.bd_addr);
            break;
        case HFP_AG_VOIP_CALL_INCOMING_INDICATION:
            VoipCallIncomingInd(&pEvent->hfp_ag_event.bd_addr,pEvent->hfp_ag_event.str,
                                pEvent->hfp_ag_event.arg1);
            break;
        case HFP_AG_VOIP_CALL_ACCEPT:
            AcceptVoipCall(&pEvent->hfp_ag_event.bd_addr);
            break;
        case HFP_AG_VOIP_CALL_SWAP:
            SwapVoipCall(&pEvent->hfp_ag_event.bd_addr);
            break;
        case HFP_AG_UPDATE_ACTIVE_CALL_NUM:
            update_activecall_num(pEvent->hfp_ag_event.arg1);
            break;
        case HFP_AG_UPDATE_HELD_CALL_NUM:
            update_heldcall_num(pEvent->hfp_ag_event.arg1);
            break;
        case HFP_AG_ADD_NUMBER:
            if (number_vec.size() < 2) {
              number_vec.insert(number_vec.end(), pEvent->hfp_ag_event.str);
              fprintf(stdout, "\n %s - number added ", pEvent->hfp_ag_event.str);
              ALOGD(LOGTAG "%s - number added ", pEvent->hfp_ag_event.str);
            } else {
              fprintf(stdout, "\n Can not add more than 2 numbers ");
              ALOGD(LOGTAG "Can not add more than 2 numbers ");
            }
            break;
        case HFP_AG_DELETE_NUMBER:
            if (number_vec.size() > 0) {
              number_vec.pop_back();
              fprintf(stdout, "\n number deleted ");
              ALOGD(LOGTAG " number deleted ");
            } else {
              fprintf(stdout, "\n all numbers deleted/no Number added to delete ");
              ALOGD(LOGTAG " all numbers deleted no Number added to delete ");
            }
            break;
        case HFP_AG_SEND_DEVICE_STAT_NOTFY:
            bdaddr_to_string(&pEvent->hfp_ag_event.bd_addr, str, 18);
            fprintf(stdout, "VHFP_AG_SEND_DEVICE_STAT_NOTFY %s", str);
            ALOGD(LOGTAG "HFP_AG_SEND_DEVICE_STAT_NOTFY %s", str);
            if(sBtHfpAgInterface != NULL) {
              if (pEvent->hfp_ag_event.arg1 == 0){
                fprintf(stdout, " network not avaialble \n ");
                ALOGD(LOGTAG "HFP_AG_SEND_DEVICE_STAT_NOTFY ");
                sBtHfpAgInterface->device_status_notification(BTHF_NETWORK_STATE_NOT_AVAILABLE,
                                        BTHF_SERVICE_TYPE_HOME, pEvent->hfp_ag_event.arg2,
                                        pEvent->hfp_ag_event.arg3, &pEvent->hfp_ag_event.bd_addr);
              } else if (pEvent->hfp_ag_event.arg1 == 1) {
                fprintf(stdout, " network avaialble \n ");
                ALOGD(LOGTAG "HFP_AG_SEND_DEVICE_STAT_NOTFY ");
                sBtHfpAgInterface->device_status_notification(BTHF_NETWORK_STATE_AVAILABLE,
                                        BTHF_SERVICE_TYPE_HOME, pEvent->hfp_ag_event.arg2,
                                        pEvent->hfp_ag_event.arg3, &pEvent->hfp_ag_event.bd_addr);
              } else
                fprintf(stdout, " Invalid input \n ");
            }
            break;
        case HFP_AG_VR_CB:
            bdaddr_to_string(&pEvent->hfp_ag_event.bd_addr, str, 18);
            fprintf(stdout, "VR start/stop req from device %s", str);
            ALOGD(LOGTAG "VR start/stop req from device %s", str);

            // send error for VR start/stop request

            if (sBtHfpAgInterface != NULL) {
                sBtHfpAgInterface->at_response(BTHF_AT_RESPONSE_ERROR, 0, &pEvent->hfp_ag_event.bd_addr);
            }
            break;
        case HFP_AG_WBS_CB:
            mWbsState = (bthf_wbs_config_t)pEvent->hfp_ag_event.arg1;
            break;
        case HFP_AG_NREC_CB:
            mNrec = (bthf_nrec_t)pEvent->hfp_ag_event.arg1;
            break;
        case HFP_AG_ANSWER_CALL_CB:
            // answer call using RIL APIs.
            // OK will be sent from stack itself.
#if defined(BT_MODEM_INTEGRATION)
            send_voice_cmd(MCM_VOICE_CALL_ANSWER_V01);
#else
            if(sBtHfpAgInterface != NULL) {
              sBtHfpAgInterface->phone_state_change(1,0,BTHF_CALL_STATE_IDLE,"",
                                BTHF_CALL_ADDRTYPE_INTERNATIONAL, &pEvent->hfp_ag_event.bd_addr);
            }
#endif
            break;
        case HFP_AG_HANGUP_CALL_CB:
#if defined(BT_MODEM_INTEGRATION)
            end_call(BTHF_CALL_STATE_ACTIVE);
#else
            EndVoipCall(&pEvent->hfp_ag_event.bd_addr);
#endif
            break;
        case HFP_AG_VOL_CONTROL_CB:
            // TODO: change the speaker volume using mm audio shell script
            // OK will be sent from stack itself.
            break;
        case HFP_AG_DIAL_CALL_CB:
#if defined(BT_MODEM_INTEGRATION)
            dial_call(pEvent->hfp_ag_event.str, &pEvent->hfp_ag_event.bd_addr);
#else
            if((number_vec.size() == 0) && ((pEvent->hfp_ag_event.str[0] == '>')
                || (pEvent->hfp_ag_event.str[0] == '\0'))) {
              // if we don't add any number , send error
              // if it is redial request and we don't have last dialed number, send error
              // if memory dialing is requested, send error
              if (sBtHfpAgInterface != NULL)
                sBtHfpAgInterface->at_response(BTHF_AT_RESPONSE_ERROR, 0,
                                          &pEvent->hfp_ag_event.bd_addr);
            }else {
              if (sBtHfpAgInterface != NULL)
                sBtHfpAgInterface->at_response(BTHF_AT_RESPONSE_OK, 0,
                                       &pEvent->hfp_ag_event.bd_addr);
                sBtHfpAgInterface->phone_state_change(0,0,BTHF_CALL_STATE_DIALING,"",
                        BTHF_CALL_ADDRTYPE_INTERNATIONAL, &pEvent->hfp_ag_event.bd_addr);
                usleep(20000);
                sBtHfpAgInterface->phone_state_change(0,0,BTHF_CALL_STATE_ALERTING,"",
                        BTHF_CALL_ADDRTYPE_INTERNATIONAL, &pEvent->hfp_ag_event.bd_addr);
            }
#endif
            break;
        case HFP_AG_CIND_CB:
            bdaddr_to_string(&pEvent->hfp_ag_event.bd_addr, str, 18);
            fprintf(stdout, "Sending CIND resp to device %s", str);
            ALOGD(LOGTAG "Sending CIND resp to device %s", str);

            if (sBtHfpAgInterface != NULL) {
#if defined(BT_MODEM_INTEGRATION)
                // we already have active/held/ringing call, call setup info. send it to stack
                sBtHfpAgInterface->cind_response(1, mNumActiveCalls, mNumHeldCalls,
                                                mCallSetupState, 5, 0, 5, &pEvent->hfp_ag_event.bd_addr);
#else
                sBtHfpAgInterface->cind_response(1, mActiveCallsNum, mHeldCallsNum,
                            BTHF_CALL_STATE_IDLE, 5, 0, 5, &pEvent->hfp_ag_event.bd_addr);
#endif

            }
            break;
        case HFP_AG_CHLD_CB:
#if defined(BT_MODEM_INTEGRATION)
            {
               uint32 ret_val = 0;
               ret_val = process_chld(pEvent->hfp_ag_event.arg1);
               if (ret_val != MCM_SUCCESS_V01) {
                   ALOGE(LOGTAG, "error processing chld %d", pEvent->hfp_ag_event.arg1);
                   fprintf(stdout, "error processing chld %d", pEvent->hfp_ag_event.arg1);
               }

               if (sBtHfpAgInterface != NULL) {
                   sBtHfpAgInterface->at_response( (ret_val == MCM_SUCCESS_V01)
                         ? BTHF_AT_RESPONSE_OK : BTHF_AT_RESPONSE_ERROR,
                         0, &pEvent->hfp_ag_event.bd_addr);
               }
            }
#else
            if (sBtHfpAgInterface != NULL) {
                sBtHfpAgInterface->at_response(BTHF_AT_RESPONSE_ERROR, 0,
                                               &pEvent->hfp_ag_event.bd_addr);
            }
#endif
            break;
        case HFP_AG_COPS_CB:
            bdaddr_to_string(&pEvent->hfp_ag_event.bd_addr, str, 18);
            fprintf(stdout, "Sending COPS resp to device %s", str);
            ALOGD(LOGTAG "Sending COPS resp to device %s", str);

#if defined(BT_MODEM_INTEGRATION)
            //  info needs to be fetched from RIL for MDM
            get_and_send_operator_name(&pEvent->hfp_ag_event.bd_addr);
#else
            if (sBtHfpAgInterface != NULL) {
                sBtHfpAgInterface->cops_response("", &pEvent->hfp_ag_event.bd_addr);
            }
#endif
            break;
        case HFP_AG_SUBSCRIBER_INFO_CB:
            bdaddr_to_string(&pEvent->hfp_ag_event.bd_addr, str, 18);
            fprintf(stdout, "Sending CNUM resp to device %s", str);
            ALOGD(LOGTAG "Sending CNUM resp to device %s", str);

#if defined(BT_MODEM_INTEGRATION)
            //  info needs to be fetched from RIL for MDM
            get_and_send_subscriber_number(&pEvent->hfp_ag_event.bd_addr);
#else
            if (sBtHfpAgInterface != NULL) {
                sBtHfpAgInterface->at_response(BTHF_AT_RESPONSE_ERROR, 0,
                                               &pEvent->hfp_ag_event.bd_addr);
            }
#endif
            break;
        case HFP_AG_CLCC_CB:
            bdaddr_to_string(&pEvent->hfp_ag_event.bd_addr, str, 18);
            fprintf(stdout, "Sending CLCC resp to device %s", str);
            ALOGD(LOGTAG "Sending CLCC resp to device %s", str);

            // TODO: cross check if we need to call get_current_calls here.

            if (sBtHfpAgInterface != NULL) {
#if defined(BT_MODEM_INTEGRATION)
                for (int i = 0; i < MCM_MAX_VOICE_CALLS_V01; i++) {
                    if (mCalls[i].call_id != 0xFFFFFFFF)
                        sBtHfpAgInterface->clcc_response(mCalls[i].idx,
                                                         mCalls[i].dir,
                                                         mCalls[i].stat,
                                                         mCalls[i].mode,
                                                         mCalls[i].mpty,
                                                         mCalls[i].number,
                                                         mCalls[i].numType,
                                                         &pEvent->hfp_ag_event.bd_addr);
                }
#else
                if (number_vec.size() > 0) {
                  int call_idx = 1;
                  //FOR PTS - just adding code to send CLCC
                  for (int i = 0; i < mActiveCallsNum; i++) {
                    sBtHfpAgInterface->clcc_response(call_idx,
                                                     BTHF_CALL_DIRECTION_INCOMING,
                                                     BTHF_CALL_STATE_HELD,
                                                     BTHF_CALL_TYPE_VOICE,
                                                     BTHF_CALL_MPTY_TYPE_SINGLE,
                                                     number_vec[call_idx-1],
                                                     BTHF_CALL_ADDRTYPE_INTERNATIONAL,
                                                     &pEvent->hfp_ag_event.bd_addr);
                    call_idx++;
                  }
                  for (int i = 0; i < mHeldCallsNum; i++) {
                    sBtHfpAgInterface->clcc_response(call_idx,
                                                     BTHF_CALL_DIRECTION_INCOMING,
                                                     BTHF_CALL_STATE_ACTIVE,
                                                     BTHF_CALL_TYPE_VOICE,
                                                     BTHF_CALL_MPTY_TYPE_SINGLE,
                                                     number_vec[call_idx-1],
                                                     BTHF_CALL_ADDRTYPE_INTERNATIONAL,
                                                     &pEvent->hfp_ag_event.bd_addr);
                    call_idx++;
                  }
                }
#endif
                // just send OK for now
                sBtHfpAgInterface->at_response(BTHF_AT_RESPONSE_OK, 0, &pEvent->hfp_ag_event.bd_addr);
            }
            break;
         case HFP_AG_UNKNOWN_AT_CMD_CB:
            if (sBtHfpAgInterface != NULL) {
                sBtHfpAgInterface->at_response(BTHF_AT_RESPONSE_ERROR, 0, &pEvent->hfp_ag_event.bd_addr);
            }
         break;
#if defined(BT_MODEM_INTEGRATION)
         case HFP_AG_RIL_IND_CB:
             process_ril_ind(pEvent);
             break;
         case HFP_AG_RIL_RESP_CB:
             process_ril_resp(pEvent);
             break;
#endif
        case HFP_AG_KEY_PRESSED_CB:
            bdaddr_to_string(&pEvent->hfp_ag_event.bd_addr, str, 18);
            fprintf(stdout, "key press cb- end the voip call %s", str);
            ALOGD(LOGTAG "key press cb- end the voip call %s", str);

            EndVoipCall(&pEvent->hfp_ag_event.bd_addr);
            break;
        case HFP_AG_BIND_CB:
            process_at_bind(pEvent);
            break;
        case HFP_AG_BIEV_CB:
            process_at_biev(pEvent);
            break;
        case HFP_AG_API_ACCEPT_CALL_REQ:
            break;
        case HFP_AG_API_RELEASE_HELD_CALL_REQ:
            break;
        case HFP_AG_API_REJECT_CALL_REQ:
            break;
        case HFP_AG_API_END_CALL_REQ:
            break;
        case HFP_AG_API_HOLD_CALL_REQ:
            break;
        case HFP_AG_API_SWAP_CALLS_REQ:
            break;
        case HFP_AG_API_DIAL_REQ:
            break;
        case HFP_AG_API_REDIAL_REQ:
            break;
        case HFP_AG_API_START_VR_REQ:
            if (sBtHfpAgInterface != NULL) {
                sBtHfpAgInterface->start_voice_recognition(&mConnectedDevice);
            }
            break;
        case HFP_AG_API_STOP_VR_REQ:
            if (sBtHfpAgInterface != NULL) {
                sBtHfpAgInterface->stop_voice_recognition(&mConnectedDevice);
            }
            break;
        case HFP_AG_API_QUERY_CURRENT_CALLS_REQ:
            if (sBtHfpAgInterface != NULL) {
            }
            break;
        case HFP_AG_API_QUERY_OPERATOR_NAME_REQ:
            if (sBtHfpAgInterface != NULL) {
            }
            break;
        case HFP_AG_API_QUERY_SUBSCRIBER_INFO_REQ:
            if (sBtHfpAgInterface != NULL) {
            }
            break;
        case HFP_AG_API_SPK_VOL_CTRL_REQ:
            if (sBtHfpAgInterface != NULL) {
                sBtHfpAgInterface->volume_control(BTHF_VOLUME_TYPE_SPK,
                                          pEvent->hfp_ag_event.arg1, &mConnectedDevice);
            }
            break;
        case HFP_AG_API_MIC_VOL_CTRL_REQ:
            if (sBtHfpAgInterface != NULL) {
                sBtHfpAgInterface->volume_control(BTHF_VOLUME_TYPE_MIC,
                                          pEvent->hfp_ag_event.arg1, &mConnectedDevice);
            }
            break;
        default:
            ALOGD(LOGTAG," event not handled %d ", pEvent->event_id);
            break;
    }

}

void Hfp_Ag::ConfigureAudio(bool enable) {

}

bool Hfp_Ag::VoipCallInd(bt_bdaddr_t *bd_addr) {
    char str[18];
    ALOGD(LOGTAG, "%s", __func__);
    if(memcmp(bd_addr,&mConnectedDevice,sizeof(bt_bdaddr_t))) {
        bdaddr_to_string(bd_addr, str, 18);
        ALOGE(LOGTAG, "%s, Device not connected: %s", __func__,str);
        fprintf(stdout, "Device not connected: %s\n", str);
        return false;
    }
    if(sBtHfpAgInterface != NULL) {
        sBtHfpAgInterface->phone_state_change(0,0,BTHF_CALL_STATE_DIALING,"",
                                              BTHF_CALL_ADDRTYPE_INTERNATIONAL, bd_addr);
        usleep(20000);
        sBtHfpAgInterface->phone_state_change(0,0,BTHF_CALL_STATE_ALERTING,"",
                                              BTHF_CALL_ADDRTYPE_INTERNATIONAL, bd_addr);
        usleep(20000);
        sBtHfpAgInterface->phone_state_change(1,0,BTHF_CALL_STATE_IDLE,"",
                                              BTHF_CALL_ADDRTYPE_INTERNATIONAL, bd_addr);
        return true;
    }
    return false;
}

bool Hfp_Ag::EndVoipCall(bt_bdaddr_t *bd_addr) {
    char str[18];
    ALOGD(LOGTAG, "%s", __func__);
    if(memcmp(bd_addr,&mConnectedDevice,sizeof(bt_bdaddr_t))) {
        bdaddr_to_string(bd_addr, str, 18);
        ALOGE(LOGTAG, "%s, Device not connected: %s", __func__,str);
        fprintf(stdout, "Device not connected: %s\n", str);
        return false;
    }
    if(sBtHfpAgInterface != NULL) {
        sBtHfpAgInterface->phone_state_change(0,0,BTHF_CALL_STATE_IDLE,"",
                                              BTHF_CALL_ADDRTYPE_INTERNATIONAL, bd_addr);
        return true;
    }
    return false;
}

bool Hfp_Ag::VoipCallIncomingInd(bt_bdaddr_t *bd_addr,char* number, int call_active) {
    char str[18];
    ALOGD(LOGTAG, "%s", __func__);
    if(memcmp(bd_addr,&mConnectedDevice,sizeof(bt_bdaddr_t))) {
        bdaddr_to_string(bd_addr, str, 18);
        ALOGE(LOGTAG, "%s, Device not connected: %s", __func__,str);
        fprintf(stdout, "Device not connected: %s\n", str);
        return false;
    }
    if(sBtHfpAgInterface != NULL) {
        sBtHfpAgInterface->phone_state_change(call_active,0,BTHF_CALL_STATE_INCOMING,number,
                                              BTHF_CALL_ADDRTYPE_INTERNATIONAL, bd_addr);
        return true;
    }
    return false;
}

bool Hfp_Ag::AcceptVoipCall(bt_bdaddr_t *bd_addr) {
    char str[18];
    ALOGD(LOGTAG, "%s", __func__);
    if(memcmp(bd_addr,&mConnectedDevice,sizeof(bt_bdaddr_t))) {
        bdaddr_to_string(bd_addr, str, 18);
        ALOGE(LOGTAG, "%s, Device not connected: %s", __func__,str);
        fprintf(stdout, "Device not connected: %s\n", str);
        return false;
    }
    if(sBtHfpAgInterface != NULL) {
        sBtHfpAgInterface->phone_state_change(1,0,BTHF_CALL_STATE_IDLE,"",
                                              BTHF_CALL_ADDRTYPE_INTERNATIONAL, bd_addr);
        return true;
    }
    return false;
}

bool Hfp_Ag::SwapVoipCall(bt_bdaddr_t *bd_addr) {
    char str[18];
    ALOGD(LOGTAG, "%s", __func__);
    if(memcmp(bd_addr,&mConnectedDevice,sizeof(bt_bdaddr_t))) {
        bdaddr_to_string(bd_addr, str, 18);
        ALOGE(LOGTAG, "%s, Device not connected: %s", __func__,str);
        fprintf(stdout, "Device not connected: %s\n", str);
        return false;
    }
    if(sBtHfpAgInterface != NULL) {
        sBtHfpAgInterface->phone_state_change(0,1,BTHF_CALL_STATE_INCOMING,"",
                                              BTHF_CALL_ADDRTYPE_INTERNATIONAL, bd_addr);
        usleep(20000);
        sBtHfpAgInterface->phone_state_change(1,1,BTHF_CALL_STATE_IDLE,"",
                                              BTHF_CALL_ADDRTYPE_INTERNATIONAL, bd_addr);
        return true;
    }
    return false;
}

void Hfp_Ag::update_activecall_num(int active) {
    if (active) {
      if (mActiveCallsNum < 1) {
        mActiveCallsNum++;
        fprintf(stdout, "\n Active call number updated,ActiveCallsNum: %d",
                mActiveCallsNum);
        ALOGD(LOGTAG "Active call number updated, ActiveCallsNum: %d", mActiveCallsNum);
      } else {
        fprintf(stdout, "\n Can not make more than one active call, ActiveCallsNum: %d",
                mActiveCallsNum);
        ALOGD(LOGTAG "Can not make more than one active call,ActiveCallsNum: %d",
              mActiveCallsNum);
      }
    } else if (mActiveCallsNum != 0) {
      mActiveCallsNum--;
      fprintf(stdout, "\n Active call number updated,ActiveCallsNum: %d",
              mActiveCallsNum);
      ALOGD(LOGTAG "Active call number updated, ActiveCallsNum: %d", mActiveCallsNum);
    } else {
      fprintf(stdout, "\n No active calls, ActiveCallsNum:%d", mActiveCallsNum);
      ALOGD(LOGTAG " No active calls, ActiveCallsNum:%d", mActiveCallsNum);
    }
}

void Hfp_Ag::update_heldcall_num(int held) {
    if (held) {
      if (mHeldCallsNum < 1) {
        mHeldCallsNum++;
        fprintf(stdout, "\n held call number updated,HeldCallsNum: %d",
                mHeldCallsNum);
        ALOGD(LOGTAG "held call number updated, HeldCallsNum: %d", mHeldCallsNum);
      } else {
        fprintf(stdout, "\n Can not make more than one held call, HeldCallsNum: %d",
                mHeldCallsNum);
        ALOGD(LOGTAG "Can not make more than one held call,HeldCallsNum: %d",
                mHeldCallsNum);
      }
    } else if (mHeldCallsNum != 0) {
      mHeldCallsNum--;
      fprintf(stdout, "\n held call number updated,HeldCallsNum: %d",
              mHeldCallsNum);
      ALOGD(LOGTAG "held call number updated, HeldCallsNum: %d", mHeldCallsNum);
    } else {
      fprintf(stdout, "\n No held calls, HeldCallsNum:%d", mHeldCallsNum);
      ALOGD(LOGTAG " No held calls, HeldCallsNum:%d", mHeldCallsNum);
    }
}

#if defined(BT_MODEM_INTEGRATION)

void Hfp_Ag::processSlcConnected() {
}

void Hfp_Ag::get_and_send_operator_name(bt_bdaddr_t *bd_addr) {
}

void Hfp_Ag::get_and_send_subscriber_number(bt_bdaddr_t *bd_addr) {
}

bthf_call_state_t Hfp_Ag::get_call_state(mcm_voice_call_state_t_v01 state){
}

void Hfp_Ag::process_call_list(mcm_voice_call_record_t_v01 *calls, uint32_t num_calls) {
}

int Hfp_Ag::get_current_calls() {
}

void Hfp_Ag::dial_call(char *number, bt_bdaddr_t *bd_addr) {
}

uint32 Hfp_Ag::send_voice_cmd(mcm_voice_call_operation_t_v01 op) {

}

uint32 Hfp_Ag::get_call_id(bthf_call_state_t state){
}

uint32 Hfp_Ag::end_call(bthf_call_state_t state) {
}

uint32 Hfp_Ag::process_chld(int chld) {
}

void Hfp_Ag::process_ril_ind(BtEvent* pEvent){
}

void Hfp_Ag::process_ril_resp(BtEvent* pEvent){
}

void Hfp_Ag::init_modem() {
}

void Hfp_Ag::release_modem() {
   if (mcm_client_hdl)
       mcm_client_release_ptr(mcm_client_hdl);

   if (lib_handle)
       dlclose(lib_handle);
}
#endif


static std::string g_encoding = "pcm";
static uint32_t g_rate = 8000;
static std::string g_format = "s16le";
static std::string g_map = "front-left,front-right";

int RemovePABtScoDevice(bool isSource);
int CreatePABtScoDevice(bool isSource);
int SetParamPABtScoDevice(bool isSource, const char* param);

void Hfp_Ag::init_audio() {
  mWbsState = BTHF_WBS_NO;
  g_rate = 8000;
  mNrec = BTHF_NREC_STOP;
  
  ALOGD(LOGTAG, "%s: nothing to init for PA", __func__);
}

void Hfp_Ag::set_audio_params() {
  
  if (mWbsState == BTHF_WBS_YES) {
    g_rate = 16000;
  }
  else {
    g_rate = 8000;
  }
  ALOGD(LOGTAG, "%s: set params for PA rate:%d", __func__, g_rate);
}

void Hfp_Ag::setup_sco_path() {
  ALOGD(LOGTAG, "%s: open device for PA\n", __func__);
  // set sample rate before starting sco
  set_audio_params();
  //
  //
  //
  CreatePABtScoDevice(false);
  CreatePABtScoDevice(true);
  if (mWbsState == BTHF_WBS_YES)
  {
    ALOGD(LOGTAG, "%s: codec-configure - bt_wbs=on", __func__);
    SetParamPABtScoDevice(false, "bt_wbs=on");
    SetParamPABtScoDevice(true, "bt_wbs=on");
  }
  else
  {
    ALOGD(LOGTAG, "%s: codec-configure - bt_wbs=off", __func__);
    SetParamPABtScoDevice(false, "bt_wbs=off");
    SetParamPABtScoDevice(true, "bt_wbs=off");
  }
}

void Hfp_Ag::teardown_sco_path() {
  ALOGD(LOGTAG, "%s: close device for PA\n", __func__);
  RemovePABtScoDevice(true);
  RemovePABtScoDevice(false);
}

void Hfp_Ag::release_audio() {
  ALOGD(LOGTAG, "%s: nothing to release for PA", __func__);
}


void Hfp_Ag::process_at_bind(BtEvent* pEvent) {
   char *at_string;
   int type = pEvent->hfp_ag_event.arg1;

   at_string = pEvent->hfp_ag_event.str;
   ALOGD(LOGTAG " %s: at_string is %s, type is %d", __func__, at_string, type);
}

void Hfp_Ag::process_at_biev(BtEvent* pEvent) {
    // TODO: just send OK for now
    if (sBtHfpAgInterface != NULL) {
        sBtHfpAgInterface->at_response(BTHF_AT_RESPONSE_OK, 0,
                    &pEvent->hfp_ag_event.bd_addr);
    }
}

void Hfp_Ag::change_state(HfpAgState mState) {
   ALOGD(LOGTAG " current State = %d, new state = %d", mAgState, mState);
   pthread_mutex_lock(&lock);
   mAgState = mState;
   ALOGD(LOGTAG " state changes to %d ", mAgState);
   pthread_mutex_unlock(&lock);

   // reset variables when we enter into disconnected state
   if (mState == HFP_AG_STATE_DISCONNECTED) {
       mWbsState = BTHF_WBS_NO;
       mNrec = BTHF_NREC_STOP;
   }
}

Hfp_Ag :: Hfp_Ag(const bt_interface_t *bt_interface, config_t *config) {
    this->bluetooth_interface = bt_interface;
    sBtHfpAgInterface = NULL;
    mAgState = HFP_AG_STATE_NOT_STARTED;
    mcontrolStatus = STATUS_LOSS_TRANSIENT;
    pthread_mutex_init(&this->lock, NULL);

    memset(mHfIndHfList, 0, sizeof(mHfIndHfList));
    memset(mHfIndAgList, 0, sizeof(mHfIndAgList));
}

Hfp_Ag :: ~Hfp_Ag() {
    mcontrolStatus = STATUS_LOSS_TRANSIENT;
    pthread_mutex_destroy(&lock);
}

int SetParamPABtScoDevice(bool isSource, const char* param)  {
  // Call Start stream config on the qahw Dbus interface and path
  ALOGE(LOGTAG "%s", __func__);
  int r;
  sd_bus_error error = SD_BUS_ERROR_NULL;
  sd_bus_message *m = NULL;
  sd_bus *pa_add_bus;
  std::string dev_path("/org/pulseaudio/ext/qahw/port/");
  
  if (isSource) {
      dev_path += "btsco_in";
  } else {
      dev_path += "btsco_out";
  }
  
  // Create an Address bus to communitate with PA dbus interface directly
  do {
    r = sd_bus_new(&pa_add_bus);
    if (r < 0) {
        ALOGE(LOGTAG "PA Address bus Creation Failed with %d - %s", r,
            strerror(-r));
        break;
    }
    r = sd_bus_set_address(pa_add_bus,
        "unix:path=/var/run/pulse/dbus-socket");
    if (r < 0) {
        ALOGE(LOGTAG "Set PA bus address Failed %d - %s", r, strerror(-r));
        break;
    }
    r = sd_bus_set_anonymous(pa_add_bus, 1);
    if (r < 0) {
        ALOGE(LOGTAG "PA bus set anonymous Failed: %d - %s", r,
            strerror(-r));
        break;
    }
    if (sd_bus_start(pa_add_bus) < 0) {
        ALOGE(LOGTAG "Couldn't Start the PA Address Bus\n");
        break;
    }
  
    r = sd_bus_message_new_method_call(pa_add_bus, &m,
        "org.pulseaudio.Server",
        dev_path.c_str(),
        "org.PulseAudio.Ext.Qahw.Module", "SetParam");
  
    if (r < 0) {
        ALOGE(LOGTAG
            "Failed to create method call message object for StartStream "
            "error %d - %s\n",
            r, strerror(-r));
        break;
    }
  
    ALOGD(LOGTAG "Path : %s", dev_path.c_str());
    ALOGD(LOGTAG "Param : %s", param);
  
    r = sd_bus_message_append(m, "s", param);
    if (r < 0) {
        ALOGE(LOGTAG
            "%s failed to append args to StartStream call, error %d - %s\n",
            __func__, r, strerror(-r));
        break;
    }
  
    r = sd_bus_call(pa_add_bus, m, 0, &error, NULL);
  
    if (r < 0) {
        ALOGE(LOGTAG
            "A2dpSplit::%s sd_bus_call_method failed error %d - %s:%s\n",
            __func__, r, strerror(-r), error.message);
        break;
    }
  } while (0);
  
  if (NULL != m)
      sd_bus_message_unref(m);
  sd_bus_error_free(&error);
  sd_bus_flush_close_unref(pa_add_bus);
  return r;
}

int CreatePABtScoDevice(bool isSource)  {
  // Call Start stream config on the qahw Dbus interface and path
  ALOGD(LOGTAG "%s", __func__);
  int r;
  sd_bus_error error = SD_BUS_ERROR_NULL;
  sd_bus_message *m = NULL;
  sd_bus *pa_add_bus;
  std::string dev_path("/org/pulseaudio/ext/qahw/port/");
  
  if (isSource) {
      dev_path += "btsco_in";
  } else {
      dev_path += "btsco_out";
  }
  
  // Create an Address bus to communitate with PA dbus interface directly
  do {
    r = sd_bus_new(&pa_add_bus);
    if (r < 0) {
        ALOGE(LOGTAG "PA Address bus Creation Failed with %d - %s", r,
            strerror(-r));
        break;
    }
    r = sd_bus_set_address(pa_add_bus,
        "unix:path=/var/run/pulse/dbus-socket");
    if (r < 0) {
        ALOGE(LOGTAG "Set PA bus address Failed %d - %s", r, strerror(-r));
        break;
    }
    r = sd_bus_set_anonymous(pa_add_bus, 1);
    if (r < 0) {
        ALOGE(LOGTAG "PA bus set anonymous Failed: %d - %s", r,
            strerror(-r));
        break;
    }
    if (sd_bus_start(pa_add_bus) < 0) {
        ALOGE(LOGTAG "Couldn't Start the PA Address Bus\n");
        break;
    }
  
    r = sd_bus_message_new_method_call(pa_add_bus, &m,
        "org.pulseaudio.Server",
        dev_path.c_str(),
        "org.PulseAudio.Ext.Qahw.Module", "StartStream");
  
    if (r < 0) {
        ALOGE(LOGTAG
            "Failed to create method call message object for StartStream "
            "error %d - %s\n",
            r, strerror(-r));
        break;
    }
  
    ALOGD(LOGTAG "Path : %s", dev_path.c_str());
    ALOGD(LOGTAG "Encoding : %s , Rate : %d , Format :%s , Mapping : %s", g_encoding.c_str(), g_rate, g_format.c_str(), g_map.c_str());
  
    r = sd_bus_message_append(m, "(suss)", g_encoding.c_str(), g_rate, g_format.c_str(), g_map.c_str());
    if (r < 0) {
        ALOGE(LOGTAG
            "%s failed to append args to StartStream call, error %d - %s\n",
            __func__, r, strerror(-r));
        break;
    }
  
    r = sd_bus_call(pa_add_bus, m, 0, &error, NULL);
  
    if (r < 0) {
        ALOGE(LOGTAG
            "A2dpSplit::%s sd_bus_call_method failed error %d - %s:%s\n",
            __func__, r, strerror(-r), error.message);
        break;
    }
  } while (0);
  
  if (NULL != m)
      sd_bus_message_unref(m);
  sd_bus_error_free(&error);
  sd_bus_flush_close_unref(pa_add_bus);
  return r;
}

int RemovePABtScoDevice(bool isSource)
{
  // Call Stop stream config on the qahw Dbus interface and path
  ALOGD(LOGTAG "%s", __func__);

  int r;
  sd_bus_error error = SD_BUS_ERROR_NULL;
  sd_bus_message *m = NULL;
  sd_bus *pa_add_bus;
  std::string dev_path("/org/pulseaudio/ext/qahw/port/");

  if (isSource)
  {
    dev_path += "btsco_in";
  }
  else
  {
    dev_path += "btsco_out";
  }

  // Create an Address bus to communitate with PA dbus interface directly

  do
  {
    r = sd_bus_new(&pa_add_bus);
    if (r < 0)
    {
      ALOGE(LOGTAG "PA Address bus Creation Failed with %d - %s", r,
                    strerror(-r));
      break;
    }
    r = sd_bus_set_address(pa_add_bus,
                           "unix:path=/var/run/pulse/dbus-socket");
    if (r < 0)
    {
      ALOGE(LOGTAG "Set PA bus address Failed %d - %s", r,
                    strerror(-r));
      break;
    }
    r = sd_bus_set_anonymous(pa_add_bus, 1);
    if (r < 0)
    {
      ALOGE(LOGTAG "PA bus set anonymous Failed: %d - %s", r,
                    strerror(-r));
      break;
    }
    if (sd_bus_start(pa_add_bus) < 0)
    {
      ALOGE(LOGTAG "Couldn't Start the PA Address Bus\n");
      break;
    }

    r = sd_bus_call_method(pa_add_bus, NULL,
                           dev_path.c_str(),
                           "org.PulseAudio.Ext.Qahw.Module", "StopStream", &error, &m,
                           NULL);
    if (r < 0)
    {
      ALOGE(LOGTAG
                    " A2dpSplit::%s sd_bus_call_method failed error %d - %s:%s\n",
                    __func__, r, strerror(-r), error.message);
    }
  } while (0);

  sd_bus_error_free(&error);
  if (NULL != m)
    sd_bus_message_unref(m);
  sd_bus_flush_close_unref(pa_add_bus);
  return r;
}

