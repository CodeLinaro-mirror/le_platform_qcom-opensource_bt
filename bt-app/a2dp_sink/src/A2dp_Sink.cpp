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

//#include "gap/include/Gap.hpp"
#include "../include/A2dp_Sink.hpp"

#define LOGTAG "A2DP_SINK"

using namespace std;
using std::list;
//using std::map;
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
        case PROFILE_API_START: // TODO_SINK: from where to send this msg.
            if (pA2dpSink) {
                pA2dpSink->HandleEnableSink();
            }
            break;
        case PROFILE_API_STOP: // TODO_SINK: from where to send this msg.
            if (pA2dpSink) {
                pA2dpSink->HandleDisableSink();
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
    ALOGD(LOGTAG," bta2dp_audio_focus_request_callback ");
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

void A2dp_Sink::HandleEnableSink(void) {
    if (bluetooth_interface != NULL)
    {
        sBtA2dpSinkInterface = (btav_interface_t *)bluetooth_interface->
                get_profile_interface(BT_PROFILE_ADVANCED_AUDIO_SINK_ID);
        if (sBtA2dpSinkInterface == NULL)
        {
            // TODO_SINK: sent message to indicate failure for sink profile init
            return;
        }
        change_state(STATE_DISCONNECTED);
        sBtA2dpSinkInterface->init(&sBluetoothA2dpSinkCallbacks, 1, 0);
        BtEvent *pEvent = new BtEvent;
        pEvent->profile_start_event.event_id = PROFILE_EVENT_START_DONE;
        pEvent->profile_start_event.profile_id = PROFILE_ID_A2DP_SINK;
        pEvent->profile_start_event.status = true;
        PostMessage(THREAD_ID_GAP, pEvent);
    }
}

void A2dp_Sink::HandleDisableSink(void) {
   change_state(STATE_NOT_STARTED);
   if(sBtA2dpSinkInterface != NULL) {
       sBtA2dpSinkInterface->cleanup();
       sBtA2dpSinkInterface = NULL;
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
            ALOGE(LOGTAG," STATE UNINITIALIZED, return");
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
            ALOGD(LOGTAG," event not handled %d ", pEvent->event_id);
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
            ALOGD(LOGTAG," event not handled %d ", pEvent->event_id);
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
            ALOGD(LOGTAG," event not handled %d ", pEvent->event_id);
            break;
    }
}

void A2dp_Sink::change_state(A2dpSinkState mState) {
   ALOGD(LOGTAG," current State = %d, new state = %d", mSinkState, mState);
   pthread_mutex_lock(&lock);
   mSinkState = mState;
   ALOGD(LOGTAG," state changes to %d ", mState);
   pthread_mutex_unlock(&lock);
}
A2dp_Sink :: A2dp_Sink(const bt_interface_t *bt_interface, config_t *config) {

    this->bluetooth_interface = bt_interface;
    this->config = config;
    sBtA2dpSinkInterface = NULL;
    mSinkState = STATE_NOT_STARTED;
    pthread_mutex_init(&this->lock, NULL);
}

A2dp_Sink :: ~A2dp_Sink() {
    pthread_mutex_destroy(&lock);
}
