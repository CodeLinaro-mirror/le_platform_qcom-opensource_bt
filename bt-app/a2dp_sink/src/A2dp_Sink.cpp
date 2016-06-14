 /*
  * Copyright (c) 2016, The Linux Foundation. All rights reserved.
  *
  * Redistribution and use in source and binary forms, with or without
  * modification, are permitted provided that the following conditions are
  * met:
  *  * Redistributions of source code must retain the above copyright
  *    notice, this list of conditions and the following disclaimer.
  *  * Redistributions in binary form must reproduce the above
  *    copyright notice, this list of conditions and the following
  *    disclaimer in the documentation and/or other materials provided
  *    with the distribution.
  *  * Neither the name of The Linux Foundation nor the names of its
  *    contributors may be used to endorse or promote products derived
  *    from this software without specific prior written permission.
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

#include <list>
#include <map>
#include <string.h>
#include <hardware/bluetooth.h>
#include <hardware/hardware.h>
#include <hardware/bt_av.h>

#include "../include/A2dp_Sink.hpp"

#define LOGTAG "A2DP_SINK"
#define LOGTAG_CTRL "AVRCP_CTRL"

using namespace std;
using std::list;
using std::string;

A2dp_Sink *pA2dpSink = NULL;

#ifdef __cplusplus
extern "C" {
#endif

void BtA2dpSinkMsgHandler(void *msg) {
    BtEvent* pEvent = NULL;
    if(!msg) {
        printf("Msg is NULL, return.\n");
        return;
    }

    pEvent = ( BtEvent *) msg;
    ALOGD(LOGTAG " bt_a2dp_sink_msg_handler event = %d", pEvent->event_id);
    switch(pEvent->event_id) {
        case PROFILE_API_START:
            if (pA2dpSink) {
                pA2dpSink->HandleEnableSink();
            }
            break;
        case PROFILE_API_STOP:
            if (pA2dpSink) {
                pA2dpSink->HandleDisableSink();
            }
            break;
        case AVRCP_CTRL_CONNECTED_CB:
        case AVRCP_CTRL_DISCONNECTED_CB:
        case AVRCP_CTRL_PASS_THRU_CMD_REQ:
            if (pA2dpSink) {
                pA2dpSink->HandleAvrcpEvents(( BtEvent *) msg);
            }
            break;
        default:
            if(pA2dpSink) {
               pA2dpSink->ProcessEvent(( BtEvent *) msg);
            }
            break;
    }
    delete pEvent;
}

#ifdef __cplusplus
}
#endif




static void bta2dp_connection_state_callback(btav_connection_state_t state, bt_bdaddr_t* bd_addr) {
    ALOGD(LOGTAG " Connection State CB");
    BtEvent *pEvent = new BtEvent;
    memcpy(&pEvent->a2dpSinkEvent.bd_addr, bd_addr, sizeof(bt_bdaddr_t));
    switch( state ) {
        case BTAV_CONNECTION_STATE_DISCONNECTED:
            pEvent->a2dpSinkEvent.event_id = A2DP_SINK_DISCONNECTED_CB;
        break;
        case BTAV_CONNECTION_STATE_CONNECTING:
            pEvent->a2dpSinkEvent.event_id = A2DP_SINK_CONNECTING_CB;
        break;
        case BTAV_CONNECTION_STATE_CONNECTED:
            pEvent->a2dpSinkEvent.event_id = A2DP_SINK_CONNECTED_CB;
        break;
        case BTAV_CONNECTION_STATE_DISCONNECTING:
            pEvent->a2dpSinkEvent.event_id = A2DP_SINK_DISCONNECTING_CB;
        break;
    }
    PostMessage(THREAD_ID_A2DP_SINK, pEvent);
}

static void bta2dp_audio_state_callback(btav_audio_state_t state, bt_bdaddr_t* bd_addr) {
    ALOGD(LOGTAG " Audio State CB");
    BtEvent *pEvent = new BtEvent;
    memcpy(&pEvent->a2dpSinkEvent.bd_addr, bd_addr, sizeof(bt_bdaddr_t));
    switch( state ) {
        case BTAV_AUDIO_STATE_REMOTE_SUSPEND:
            pEvent->a2dpSinkEvent.event_id = A2DP_SINK_AUDIO_SUSPENDED;
        break;
        case BTAV_AUDIO_STATE_STOPPED:
            pEvent->a2dpSinkEvent.event_id = A2DP_SINK_AUDIO_STOPPED;
        break;
        case BTAV_AUDIO_STATE_STARTED:
            pEvent->a2dpSinkEvent.event_id = A2DP_SINK_AUDIO_STARTED;
        break;
    }
    PostMessage(THREAD_ID_A2DP_SINK, pEvent);
}

static void bta2dp_audio_config_callback(bt_bdaddr_t *bd_addr, uint32_t sample_rate,
        uint8_t channel_count) {
    ALOGD(LOGTAG " Audio Config CB");
}
static void bta2dp_audio_focus_request_callback(bt_bdaddr_t *bd_addr) {
    ALOGD(LOGTAG " bta2dp_audio_focus_request_callback ");
    BtEvent *pEvent = new BtEvent;
    pEvent->a2dpSinkEvent.event_id = A2DP_SINK_FOCUS_REQUEST_CB;
    memcpy(&pEvent->a2dpSinkEvent.bd_addr, bd_addr, sizeof(bt_bdaddr_t));
    PostMessage(THREAD_ID_A2DP_SINK, pEvent);
}

static btav_callbacks_t sBluetoothA2dpSinkCallbacks = {
    sizeof(sBluetoothA2dpSinkCallbacks),
    bta2dp_connection_state_callback,
    bta2dp_audio_state_callback,
    bta2dp_audio_config_callback,
    NULL,
    NULL,
    bta2dp_audio_focus_request_callback,
};

static void btavrcpctrl_passthru_rsp_callback(int id, int key_state) {
    ALOGD(LOGTAG_CTRL " btavrcpctrl_passthru_rsp_callback id = %d key_state = %d", id, key_state);
}

static void btavrcpctrl_connection_state_callback(bool state, bt_bdaddr_t* bd_addr) {
    ALOGD(LOGTAG_CTRL " btavrcpctrl_connection_state_callback state = %d", state);
    BtEvent *pEvent = new BtEvent;
    memcpy(&pEvent->avrcpCtrlEvent.bd_addr, bd_addr, sizeof(bt_bdaddr_t));
    if (state == true)
        pEvent->avrcpCtrlEvent.event_id = AVRCP_CTRL_CONNECTED_CB;
    else
        pEvent->avrcpCtrlEvent.event_id = AVRCP_CTRL_DISCONNECTED_CB;
    PostMessage(THREAD_ID_A2DP_SINK, pEvent);
}

static void btavrcpctrl_rcfeatures_callback( bt_bdaddr_t* bd_addr, int features) {
    ALOGD(LOGTAG_CTRL " btavrcpctrl_rcfeatures_callback features = %d", features);
}

static void btavrcpctrl_getcap_rsp_callback( bt_bdaddr_t *bd_addr, int cap_id,
                uint32_t* supported_values, int num_supported, uint8_t rsp_type) {
    ALOGD(LOGTAG_CTRL " btavrcpctrl_getcap_rsp_callback");
}

static void btavrcpctrl_listplayerappsettingattrib_rsp_callback( bt_bdaddr_t *bd_addr,
                          uint8_t* supported_attribs, int num_attrib, uint8_t rsp_type) {
    ALOGD(LOGTAG_CTRL " btavrcpctrl_listplayerappsettingattrib_rsp_callback");
}

static void btavrcpctrl_listplayerappsettingvalue_rsp_callback( bt_bdaddr_t *bd_addr,
                       uint8_t* supported_val, uint8_t num_supported, uint8_t rsp_type) {
    ALOGD(LOGTAG_CTRL " btavrcpctrl_listplayerappsettingvalue_rsp_callback");
}

static void btavrcpctrl_currentplayerappsetting_rsp_callback( bt_bdaddr_t *bd_addr,
        uint8_t* supported_ids, uint8_t* supported_val, uint8_t num_attrib, uint8_t rsp_type) {
    ALOGD(LOGTAG_CTRL " btavrcpctrl_currentplayerappsetting_rsp_callback");
}

static void btavrcpctrl_setplayerappsetting_rsp_callback( bt_bdaddr_t *bd_addr,uint8_t rsp_type) {
    ALOGD(LOGTAG_CTRL " btavrcpctrl_setplayerappsetting_rsp_callback");
}

static void btavrcpctrl_notification_rsp_callback( bt_bdaddr_t *bd_addr, uint8_t rsp_type,
        int rsp_len, uint8_t* notification_rsp) {
    ALOGD(LOGTAG_CTRL " btavrcpctrl_notification_rsp_callback");
}

static void btavrcpctrl_getelementattrib_rsp_callback(bt_bdaddr_t *bd_addr, uint8_t num_attributes,
       int rsp_len, uint8_t* attrib_rsp, uint8_t rsp_type) {
    ALOGD(LOGTAG_CTRL " btavrcpctrl_getelementattrib_rsp_callback");
}

static void btavrcpctrl_getplaystatus_rsp_callback(bt_bdaddr_t *bd_addr, int param_len,
        uint8_t* play_status_rsp, uint8_t rsp_type) {
    ALOGD(LOGTAG_CTRL " btavrcpctrl_getplaystatus_rsp_callback");
}

static void btavrcpctrl_setabsvol_cmd_callback(bt_bdaddr_t *bd_addr, uint8_t abs_vol) {
    ALOGD(LOGTAG_CTRL " btavrcpctrl_setabsvol_cmd_callback");
}

static void btavrcpctrl_registernotification_absvol_callback(bt_bdaddr_t *bd_addr) {
    ALOGD(LOGTAG_CTRL " btavrcpctrl_registernotification_absvol_callback");
}

static btrc_ctrl_callbacks_t sBluetoothAvrcpCtrlCallbacks = {
   sizeof(sBluetoothAvrcpCtrlCallbacks),
   btavrcpctrl_passthru_rsp_callback,
   btavrcpctrl_connection_state_callback,
   btavrcpctrl_rcfeatures_callback,
   btavrcpctrl_getcap_rsp_callback,
   btavrcpctrl_listplayerappsettingattrib_rsp_callback,
   btavrcpctrl_listplayerappsettingvalue_rsp_callback,
   btavrcpctrl_currentplayerappsetting_rsp_callback,
   btavrcpctrl_setplayerappsetting_rsp_callback,
   btavrcpctrl_notification_rsp_callback,
   btavrcpctrl_getelementattrib_rsp_callback,
   btavrcpctrl_getplaystatus_rsp_callback,
   btavrcpctrl_setabsvol_cmd_callback,
   btavrcpctrl_registernotification_absvol_callback,
};

void A2dp_Sink::HandleAvrcpEvents(BtEvent* pEvent) {
    ALOGD(LOGTAG_CTRL " HandleAvrcpEvents event = %d", pEvent->avrcpCtrlEvent.event_id);
    switch(pEvent->avrcpCtrlEvent.event_id) {
    case AVRCP_CTRL_CONNECTED_CB:
        mAvrcpConnected = true;
        memcpy(&mConnectedAvrcpDevice, &pEvent->avrcpCtrlEvent.bd_addr,
                sizeof(bt_bdaddr_t));
        break;
    case AVRCP_CTRL_DISCONNECTED_CB:
        mAvrcpConnected = false;
        memset(&mConnectedAvrcpDevice, 0, sizeof(bt_bdaddr_t));
        break;
    case AVRCP_CTRL_PASS_THRU_CMD_REQ:
        if (!mAvrcpConnected || (memcmp(&mConnectedAvrcpDevice, &mConnectedDevice,
                                                          sizeof(bt_bdaddr_t)) != 0)) {
            ALOGD(LOGTAG_CTRL " Avrcp Not connected/ Not to A2DP Sink ");
            break;
        }
        if (sBtAvrcpCtrlInterface != NULL) {
            sBtAvrcpCtrlInterface->send_pass_through_cmd(&mConnectedAvrcpDevice,
                                                 pEvent->avrcpCtrlEvent.key_id, 0);
            sBtAvrcpCtrlInterface->send_pass_through_cmd(&mConnectedAvrcpDevice,
                                                 pEvent->avrcpCtrlEvent.key_id, 1);
        }
        break;
    }
}

void A2dp_Sink::HandleEnableSink(void) {
    BtEvent *pEvent = new BtEvent;
    if (bluetooth_interface != NULL)
    {
        sBtA2dpSinkInterface = (btav_interface_t *)bluetooth_interface->
                get_profile_interface(BT_PROFILE_ADVANCED_AUDIO_SINK_ID);
        if (sBtA2dpSinkInterface == NULL)
        {
             pEvent->profile_start_event.event_id = PROFILE_EVENT_START_DONE;
             pEvent->profile_start_event.profile_id = PROFILE_ID_A2DP_SINK;
             pEvent->profile_start_event.status = false;
             PostMessage(THREAD_ID_GAP, pEvent);
             return;
        }
        change_state(STATE_DISCONNECTED);
        sBtA2dpSinkInterface->init(&sBluetoothA2dpSinkCallbacks, 1, 0);
        pEvent->profile_start_event.event_id = PROFILE_EVENT_START_DONE;
        pEvent->profile_start_event.profile_id = PROFILE_ID_A2DP_SINK;
        pEvent->profile_start_event.status = true;
        // AVRCP Initialization
        sBtAvrcpCtrlInterface = (btrc_ctrl_interface_t *)bluetooth_interface->
                get_profile_interface(BT_PROFILE_AV_RC_CTRL_ID);
        if (sBtAvrcpCtrlInterface != NULL) {
            sBtAvrcpCtrlInterface->init(&sBluetoothAvrcpCtrlCallbacks);
        }
        PostMessage(THREAD_ID_GAP, pEvent);
    }
}

void A2dp_Sink::HandleDisableSink(void) {
   change_state(STATE_NOT_STARTED);
   if(sBtA2dpSinkInterface != NULL) {
       sBtA2dpSinkInterface->cleanup();
       sBtA2dpSinkInterface = NULL;
   }
   if (sBtAvrcpCtrlInterface != NULL) {
       sBtAvrcpCtrlInterface->cleanup();
       sBtAvrcpCtrlInterface = NULL;
   }
   BtEvent *pEvent = new BtEvent;
    pEvent->profile_stop_event.event_id = PROFILE_EVENT_STOP_DONE;
        pEvent->profile_stop_event.profile_id = PROFILE_ID_A2DP_SINK;
        pEvent->profile_stop_event.status = true;
        PostMessage(THREAD_ID_GAP, pEvent);
}

void A2dp_Sink::ProcessEvent(BtEvent* pEvent) {
    ALOGD(LOGTAG " Processing event %d", pEvent->event_id);
    switch(mSinkState) {
        case STATE_DISCONNECTED:
            state_disconnected_handler(pEvent);
            break;
        case STATE_PENDING:
            state_pending_handler(pEvent);
            break;
        case STATE_CONNECTED:
            state_connected_handler(pEvent);
            break;
        case STATE_NOT_STARTED:
            ALOGE(LOGTAG " STATE UNINITIALIZED, return");
            break;
    }
}

void A2dp_Sink::state_disconnected_handler(BtEvent* pEvent) {
    ALOGD(LOGTAG "state_disconnected_handler Processing event %d", pEvent->event_id);
    switch(pEvent->event_id) {
        case A2DP_SINK_API_CONNECT_REQ:
            memcpy(&mConnectingDevice, &pEvent->a2dpSinkEvent.bd_addr, sizeof(bt_bdaddr_t));
            if (sBtA2dpSinkInterface != NULL) {
                sBtA2dpSinkInterface->connect(&pEvent->a2dpSinkEvent.bd_addr);
            }
            change_state(STATE_PENDING);
            break;
        case A2DP_SINK_CONNECTING_CB:
            memcpy(&mConnectingDevice, &pEvent->a2dpSinkEvent.bd_addr, sizeof(bt_bdaddr_t));
            change_state(STATE_PENDING);
            break;
        case A2DP_SINK_CONNECTED_CB:
            memset(&mConnectingDevice, 0, sizeof(bt_bdaddr_t));
            memcpy(&mConnectedDevice, &pEvent->a2dpSinkEvent.bd_addr, sizeof(bt_bdaddr_t));
            change_state(STATE_CONNECTED);
            break;
        default:
            ALOGD(LOGTAG " event not handled %d ", pEvent->event_id);
            break;
    }
}
void A2dp_Sink::state_pending_handler(BtEvent* pEvent) {
    ALOGD(LOGTAG "state_pending_handler Processing event %d", pEvent->event_id);
    switch(pEvent->event_id) {
        case A2DP_SINK_CONNECTING_CB:
            break;
        case A2DP_SINK_CONNECTED_CB:
            memcpy(&mConnectedDevice, &pEvent->a2dpSinkEvent.bd_addr, sizeof(bt_bdaddr_t));
            memset(&mConnectingDevice, 0, sizeof(bt_bdaddr_t));
            change_state(STATE_CONNECTED);
            break;
        case A2DP_SINK_DISCONNECTED_CB:
            memset(&mConnectedDevice, 0, sizeof(bt_bdaddr_t));
            memset(&mConnectingDevice, 0, sizeof(bt_bdaddr_t));
            change_state(STATE_DISCONNECTED);
            break;
        default:
            ALOGD(LOGTAG " event not handled %d ", pEvent->event_id);
            break;
    }
}

void A2dp_Sink::state_connected_handler(BtEvent* pEvent) {
    ALOGD(LOGTAG "state_connected_handler Processing event %d", pEvent->event_id);
    switch(pEvent->event_id) {
        case A2DP_SINK_API_DISCONNECT_REQ:
            memset(&mConnectedDevice, 0, sizeof(bt_bdaddr_t));
            memset(&mConnectingDevice, 0, sizeof(bt_bdaddr_t));
            if (sBtA2dpSinkInterface != NULL) {
                sBtA2dpSinkInterface->disconnect(&pEvent->a2dpSinkEvent.bd_addr);
            }
            change_state(STATE_PENDING);
            break;
        case A2DP_SINK_DISCONNECTED_CB:
            memset(&mConnectedDevice, 0, sizeof(bt_bdaddr_t));
            memset(&mConnectingDevice, 0, sizeof(bt_bdaddr_t));
            change_state(STATE_DISCONNECTED);
            break;
        case A2DP_SINK_DISCONNECTING_CB:
            change_state(STATE_PENDING);
            break;
        case A2DP_SINK_FOCUS_REQUEST_CB:
            if (sBtA2dpSinkInterface != NULL) {
                sBtA2dpSinkInterface->audio_focus_state(3);
            }
            break;
        default:
            ALOGD(LOGTAG " event not handled %d ", pEvent->event_id);
            break;
    }
}

void A2dp_Sink::change_state(A2dpSinkState mState) {
   ALOGD(LOGTAG " current State = %d, new state = %d", mSinkState, mState);
   pthread_mutex_lock(&lock);
   mSinkState = mState;
   ALOGD(LOGTAG " state changes to %d ", mState);
   pthread_mutex_unlock(&lock);
}
A2dp_Sink :: A2dp_Sink(const bt_interface_t *bt_interface, config_t *config) {

    this->bluetooth_interface = bt_interface;
    this->config = config;
    sBtA2dpSinkInterface = NULL;
    sBtAvrcpCtrlInterface = NULL;
    mSinkState = STATE_NOT_STARTED;
    mAvrcpConnected = false;
    memset(&mConnectedDevice, 0, sizeof(bt_bdaddr_t));
    memset(&mConnectingDevice, 0, sizeof(bt_bdaddr_t));
    memset(&mConnectedAvrcpDevice, 0, sizeof(bt_bdaddr_t));
    pthread_mutex_init(&this->lock, NULL);
}

A2dp_Sink :: ~A2dp_Sink() {
    pthread_mutex_destroy(&lock);
    mAvrcpConnected = false;
}
