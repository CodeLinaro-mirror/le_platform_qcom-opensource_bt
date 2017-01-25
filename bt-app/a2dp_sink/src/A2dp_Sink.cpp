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
#include <iostream>
#include <string.h>
#include <hardware/bluetooth.h>
#include <hardware/hardware.h>
#include <hardware/bt_av.h>
#include "Audio_Manager.hpp"

#include "A2dp_Sink.hpp"
#include "Gap.hpp"
#include "hardware/bt_av_vendor.h"
#include "hardware/bt_rc_vendor.h"

#define LOGTAG "A2DP_SINK"
#define LOGTAG_CTRL "AVRCP_CTRL"

using namespace std;
using std::list;
using std::string;

A2dp_Sink *pA2dpSink = NULL;
extern BT_Audio_Manager *pBTAM;

#if (!defined(BT_AUDIO_HAL_INTEGRATION))
#define DUMP_PCM_DATA TRUE
#endif

//#define DUMP_COMPRESSED_DATA TRUE

#if (defined(DUMP_PCM_DATA) && (DUMP_PCM_DATA == TRUE))
FILE *outputPcmSampleFile;
char outputFilename [50] = "/data/misc/bluetooth/output_sample.pcm";
#endif

#if (defined(DUMP_COMPRESSED_DATA) && (DUMP_COMPRESSED_DATA == TRUE))
FILE *outputPcmSampleFile;
char outputFilename [50] = "/etc/bluetooth/output_sample.pcm";
#endif

#ifdef __cplusplus
extern "C" {
#endif

#define BE_STREAM_TO_UINT16(u16, p) {u16 = (uint16_t)(((uint16_t)(*(p)) << 8) + (uint16_t)(*((p) + 1))); (p) += 2;}
#define BE_STREAM_TO_UINT32(u32, p) {u32 = ((uint32_t)(*((p) + 3)) + ((uint32_t)(*((p) + 2)) << 8) + ((uint32_t)(*((p) + 1)) << 16) + ((uint32_t)(*(p)) << 24)); (p) += 4;}

void BtA2dpSinkMsgHandler(void *msg) {
    BtEvent* pEvent = NULL;
    BtEvent* pCleanupEvent = NULL;
    if(!msg) {
        printf("Msg is NULL, return.\n");
        return;
    }

    pEvent = ( BtEvent *) msg;
    switch(pEvent->event_id) {
        case PROFILE_API_START:
            ALOGD(LOGTAG " enable a2dp sink");
            if (pA2dpSink) {
                pA2dpSink->HandleEnableSink();
            }
            break;
        case PROFILE_API_STOP:
            ALOGD(LOGTAG " disable a2dp sink");
            if (pA2dpSink) {
                pA2dpSink->HandleDisableSink();
            }
            break;
        case A2DP_SINK_CLEANUP_REQ:
            ALOGD(LOGTAG " cleanup a2dp sink");
            if (pA2dpSink) {
                if (pA2dpSink->use_bt_a2dp_hal) {
                    pA2dpSink->CloseInputStream();
                }
                pA2dpSink->CloseAudioStream();
                pA2dpSink->StopDataFetchTimer();
            }
            pCleanupEvent = new BtEvent;
            pCleanupEvent->event_id = A2DP_SINK_CLEANUP_DONE;
            PostMessage(THREAD_ID_GAP, pCleanupEvent);
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

void compress_audio_feed_handler(void *context) {
    ALOGV(LOGTAG " compress_audio_feed_handler ");

    pA2dpSink->compress_offload_timer = false;
    BtEvent *pEvent = new BtEvent;
    pEvent->a2dpSinkEvent.event_id = A2DP_SINK_FILL_COMPRESS_BUFFER;
    PostMessage(THREAD_ID_A2DP_SINK, pEvent);
}

void A2dp_Sink::StartCompressAudioFeedTimer() {
    if(compress_offload_timer) {
        ALOGV(LOGTAG " compressed timer already running, return ");
        return;
    }
    compress_offload_timer = true;
    alarm_set(compress_audio_feed_timer, A2DP_SINK_COMPRESS_FEED_TIMER_DURATION,
            compress_audio_feed_handler, NULL);
}

void A2dp_Sink::StopCompressAudioFeedTimer() {
    if((compress_audio_feed_timer != NULL) && (compress_offload_timer)) {
        alarm_cancel(compress_audio_feed_timer);
        compress_offload_timer = false;
    }
}

void pcm_fetch_timer_handler(void *context) {
    ALOGV(LOGTAG " pcm_fetch_timer_handler ");

    BtEvent *pEvent = new BtEvent;
    pEvent->a2dpSinkEvent.event_id = A2DP_SINK_FETCH_PCM_DATA;
    PostMessage(THREAD_ID_A2DP_SINK, pEvent);
}

void A2dp_Sink::StartPcmTimer() {
    if(pcm_timer) {
        ALOGD(LOGTAG " PCM Timer still running + ");
        return;
    }
    alarm_set(pcm_data_fetch_timer, A2DP_SINK_PCM_FETCH_TIMER_DURATION,
           pcm_fetch_timer_handler, NULL);
    pcm_timer = true;
}
void A2dp_Sink::StopDataFetchTimer() {

    if((codec_type == A2DP_SINK_AUDIO_CODEC_SBC)&&(pcm_data_fetch_timer != NULL) && (pcm_timer)) {
        alarm_cancel(pcm_data_fetch_timer);
        pcm_timer = false;
    }
    else {
        StopCompressAudioFeedTimer();
    }
}
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
    ALOGD(LOGTAG " Audio State CB state = %d", state);
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
    ALOGD(LOGTAG " Audio Config CB sample_rate %d, channel_count %d", sample_rate, channel_count);
    if(pA2dpSink)
    {
        pA2dpSink->sample_rate = sample_rate;
        pA2dpSink->channel_count = channel_count;
    }
}

static void bta2dp_audio_focus_request_vendor_callback(bt_bdaddr_t *bd_addr) {
    ALOGD(LOGTAG " bta2dp_audio_focus_request_vendor_callback ");
    BtEvent *pEvent = new BtEvent;
    pEvent->a2dpSinkEvent.event_id = A2DP_SINK_FOCUS_REQUEST_CB;
    memcpy(&pEvent->a2dpSinkEvent.bd_addr, bd_addr, sizeof(bt_bdaddr_t));
    PostMessage(THREAD_ID_A2DP_SINK, pEvent);
}

static void bta2dp_audio_codec_config_vendor_callback(bt_bdaddr_t *bd_addr, uint16_t codec_type,
        btav_codec_config_t codec_config) {
    ALOGD(LOGTAG " bta2dp_audio_codec_config_vendor_callback ");
    BtEvent *pEvent = new BtEvent;
    pEvent->a2dpSinkEvent.event_id = A2DP_SINK_CODEC_CONFIG;
    memcpy(&pEvent->a2dpSinkEvent.bd_addr, bd_addr, sizeof(bt_bdaddr_t));
    pEvent->a2dpSinkEvent.buf_size = sizeof(btav_codec_config_t);
    pEvent->a2dpSinkEvent.buf_ptr = (uint8_t*)osi_malloc(pEvent->a2dpSinkEvent.buf_size);
    memcpy(pEvent->a2dpSinkEvent.buf_ptr, &codec_config, pEvent->a2dpSinkEvent.buf_size);
    pEvent->a2dpSinkEvent.arg1 = codec_type;
    PostMessage(THREAD_ID_A2DP_SINK, pEvent);
}

static btav_callbacks_t sBluetoothA2dpSinkCallbacks = {
    sizeof(sBluetoothA2dpSinkCallbacks),
    bta2dp_connection_state_callback,
    bta2dp_audio_state_callback,
    bta2dp_audio_config_callback,
};

static btav_sink_vendor_callbacks_t sBluetoothA2dpSinkVendorCallbacks = {
    sizeof(sBluetoothA2dpSinkVendorCallbacks),
    bta2dp_audio_focus_request_vendor_callback,
    bta2dp_audio_codec_config_vendor_callback,
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

static void btavrcpctrl_rcfeatures_vendor_callback( bt_bdaddr_t* bd_addr, int features) {
    ALOGD(LOGTAG_CTRL " btavrcpctrl_rcfeatures_vendor_callback features = %d", features);
}

static void btavrcpctrl_getcap_rsp_vendor_callback( bt_bdaddr_t *bd_addr, int cap_id,
                uint32_t* supported_values, int num_supported, uint8_t rsp_type) {
    ALOGD(LOGTAG_CTRL " btavrcpctrl_getcap_rsp_vendor_callback");
}

static void btavrcpctrl_listplayerappsettingattrib_rsp_vendor_callback( bt_bdaddr_t *bd_addr,
                          uint8_t* supported_attribs, int num_attrib, uint8_t rsp_type) {
    ALOGD(LOGTAG_CTRL " btavrcpctrl_listplayerappsettingattrib_rsp_vendor_callback");
}

static void btavrcpctrl_listplayerappsettingvalue_rsp_vendor_callback( bt_bdaddr_t *bd_addr,
                       uint8_t* supported_val, uint8_t num_supported, uint8_t rsp_type) {
    ALOGD(LOGTAG_CTRL " btavrcpctrl_listplayerappsettingvalue_rsp_vendor_callback");
}

static void btavrcpctrl_currentplayerappsetting_rsp_vendor_callback( bt_bdaddr_t *bd_addr,
        uint8_t* supported_ids, uint8_t* supported_val, uint8_t num_attrib, uint8_t rsp_type) {
    ALOGD(LOGTAG_CTRL " btavrcpctrl_currentplayerappsetting_rsp_vendor_callback");
}

static void btavrcpctrl_setplayerappsetting_rsp_vendor_callback( bt_bdaddr_t *bd_addr,uint8_t rsp_type) {
    ALOGD(LOGTAG_CTRL " btavrcpctrl_setplayerappsetting_rsp_vendor_callback");
}

static void btavrcpctrl_notification_rsp_vendor_callback( bt_bdaddr_t *bd_addr, uint8_t rsp_type,
        int rsp_len, uint8_t* notification_rsp) {
    ALOGD(LOGTAG_CTRL " btavrcpctrl_notification_rsp_vendor_callback");
}

static void btavrcpctrl_getelementattrib_rsp_vendor_callback(bt_bdaddr_t *bd_addr, uint8_t num_attributes,
       int rsp_len, uint8_t* attrib_rsp, uint8_t rsp_type) {
    ALOGD(LOGTAG_CTRL " btavrcpctrl_getelementattrib_rsp_vendor_callback");
}

static void btavrcpctrl_getplaystatus_rsp_vendor_callback(bt_bdaddr_t *bd_addr, int param_len,
        uint8_t* play_status_rsp, uint8_t rsp_type) {
    ALOGD(LOGTAG_CTRL " btavrcpctrl_getplaystatus_rsp_vendor_callback");
}

static void btavrcpctrl_setabsvol_cmd_vendor_callback(bt_bdaddr_t *bd_addr, uint8_t abs_vol) {
    ALOGD(LOGTAG_CTRL " btavrcpctrl_setabsvol_cmd_vendor_callback");
}

static void btavrcpctrl_registernotification_absvol_vendor_callback(bt_bdaddr_t *bd_addr) {
    ALOGD(LOGTAG_CTRL " btavrcpctrl_registernotification_absvol_vendor_callback");
}

static btrc_ctrl_callbacks_t sBluetoothAvrcpCtrlCallbacks = {
   sizeof(sBluetoothAvrcpCtrlCallbacks),
   btavrcpctrl_passthru_rsp_callback,
   btavrcpctrl_connection_state_callback,
};

static btrc_ctrl_vendor_callbacks_t sBluetoothAvrcpCtrlVendorCallbacks = {
   sizeof(sBluetoothAvrcpCtrlVendorCallbacks),
   btavrcpctrl_rcfeatures_vendor_callback,
   btavrcpctrl_getcap_rsp_vendor_callback,
   btavrcpctrl_listplayerappsettingattrib_rsp_vendor_callback,
   btavrcpctrl_listplayerappsettingvalue_rsp_vendor_callback,
   btavrcpctrl_currentplayerappsetting_rsp_vendor_callback,
   btavrcpctrl_setplayerappsetting_rsp_vendor_callback,
   btavrcpctrl_notification_rsp_vendor_callback,
   btavrcpctrl_getelementattrib_rsp_vendor_callback,
   btavrcpctrl_getplaystatus_rsp_vendor_callback,
   btavrcpctrl_setabsvol_cmd_vendor_callback,
   btavrcpctrl_registernotification_absvol_vendor_callback,
};

void A2dp_Sink::SendPassThruCommandNative(uint8_t key_id) {
    if ((use_bt_a2dp_hal) && ((key_id == CMD_ID_PAUSE) || (key_id == CMD_ID_PLAY))) {
        if (CMD_ID_PAUSE == key_id)
        {
            StopDataFetchTimer();
            SuspendInputStream();
        }
        if (CMD_ID_PLAY == key_id)
        {
            ALOGD( LOGTAG_CTRL " sending started event ");
            BtEvent *pEvent = new BtEvent;
            pEvent->a2dpSinkEvent.event_id = A2DP_SINK_AUDIO_STARTED;
            PostMessage(THREAD_ID_A2DP_SINK, pEvent);
        }
    }
    else if (sBtAvrcpCtrlInterface != NULL) {
        sBtAvrcpCtrlInterface->send_pass_through_cmd(&mConnectedAvrcpDevice, key_id, 0);
        sBtAvrcpCtrlInterface->send_pass_through_cmd(&mConnectedAvrcpDevice, key_id, 1);
    }
}
void A2dp_Sink::HandleAvrcpEvents(BtEvent* pEvent) {
    ALOGD(LOGTAG_CTRL " HandleAvrcpEvents event = %s", dump_message(pEvent->avrcpCtrlEvent.event_id));
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
        SendPassThruCommandNative(pEvent->avrcpCtrlEvent.key_id);
        break;
    }
}

void A2dp_Sink::HandleEnableSink(void) {
    BtEvent *pEvent = new BtEvent;
    if (bluetooth_interface != NULL)
    {
        sBtA2dpSinkInterface = (btav_interface_t *)bluetooth_interface->
                get_profile_interface(BT_PROFILE_ADVANCED_AUDIO_SINK_ID);
        sBtA2dpSinkVendorInterface = (btav_sink_vendor_interface_t *)bluetooth_interface->
                get_profile_interface(BT_PROFILE_ADVANCED_AUDIO_SINK_VENDOR_ID);

        if (sBtA2dpSinkInterface == NULL)
        {
             pEvent->profile_start_event.event_id = PROFILE_EVENT_START_DONE;
             pEvent->profile_start_event.profile_id = PROFILE_ID_A2DP_SINK;
             pEvent->profile_start_event.status = false;
             PostMessage(THREAD_ID_GAP, pEvent);
             return;
        }
        change_state(STATE_DISCONNECTED);
        fetch_rtp_info = config_get_bool (config,
                CONFIG_DEFAULT_SECTION, "BtFetchRTPForSink", false);
        ALOGD(LOGTAG " Fetch RTP Info %d", fetch_rtp_info);
#ifdef USE_LIBHW_AOSP
        sBtA2dpSinkInterface->init(&sBluetoothA2dpSinkCallbacks);
#else
        sBtA2dpSinkInterface->init(&sBluetoothA2dpSinkCallbacks, 1, 0);
#endif
        if (fetch_rtp_info) {
            sBtA2dpSinkVendorInterface->init_vendor(&sBluetoothA2dpSinkVendorCallbacks, 1, 0,
             A2DP_SINK_ENABLE_SBC_DECODING|A2DP_SINK_RETREIVE_RTP_HEADER);
        } else {
            sBtA2dpSinkVendorInterface->init_vendor(&sBluetoothA2dpSinkVendorCallbacks, 1, 0,
                 A2DP_SINK_ENABLE_SBC_DECODING);
        }

        pEvent->profile_start_event.event_id = PROFILE_EVENT_START_DONE;
        pEvent->profile_start_event.profile_id = PROFILE_ID_A2DP_SINK;
        pEvent->profile_start_event.status = true;
        // AVRCP CT Initialization
        sBtAvrcpCtrlInterface = (btrc_ctrl_interface_t *)bluetooth_interface->
                get_profile_interface(BT_PROFILE_AV_RC_CTRL_ID);
        if (sBtAvrcpCtrlInterface != NULL) {
            sBtAvrcpCtrlInterface->init(&sBluetoothAvrcpCtrlCallbacks);
        }
        // AVRCP CT Vendor Initialization
        sBtAvrcpCtrlVendorInterface = (btrc_ctrl_vendor_interface_t *)bluetooth_interface->
                get_profile_interface(BT_PROFILE_AV_RC_CTRL_VENDOR_ID);
        if (sBtAvrcpCtrlVendorInterface != NULL) {
            sBtAvrcpCtrlVendorInterface->init_vendor(&sBluetoothAvrcpCtrlVendorCallbacks);
        }

        PostMessage(THREAD_ID_GAP, pEvent);
    }
    use_bt_a2dp_hal = config_get_bool (config,
            CONFIG_DEFAULT_SECTION, "BtUseA2dpHalForSink", false);
    ALOGD(LOGTAG " Use BT A2DP HAL ENabled %d", use_bt_a2dp_hal);
    if(use_bt_a2dp_hal) {
        LoadBtA2dpHAL();
    }
}

void A2dp_Sink::HandleDisableSink(void) {
   change_state(STATE_NOT_STARTED);
   CloseAudioStream();
   StopDataFetchTimer();
   if(use_bt_a2dp_hal) {
       UnLoadBtA2dpHAL();
   }
   if(sBtA2dpSinkInterface != NULL) {
       sBtA2dpSinkInterface->cleanup();
       sBtA2dpSinkInterface = NULL;
   }
   if (sBtAvrcpCtrlInterface != NULL) {
       sBtAvrcpCtrlInterface->cleanup();
       sBtAvrcpCtrlInterface = NULL;
   }
   if(sBtA2dpSinkVendorInterface != NULL) {
       sBtA2dpSinkVendorInterface->cleanup_vendor();
       sBtA2dpSinkVendorInterface = NULL;
   }
   if (sBtAvrcpCtrlVendorInterface != NULL) {
       sBtAvrcpCtrlVendorInterface->cleanup_vendor();
       sBtAvrcpCtrlVendorInterface = NULL;
   }
   BtEvent *pEvent = new BtEvent;
    pEvent->profile_stop_event.event_id = PROFILE_EVENT_STOP_DONE;
        pEvent->profile_stop_event.profile_id = PROFILE_ID_A2DP_SINK;
        pEvent->profile_stop_event.status = true;
        PostMessage(THREAD_ID_GAP, pEvent);
}

void A2dp_Sink::ProcessEvent(BtEvent* pEvent) {
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

char* A2dp_Sink::dump_message(BluetoothEventId event_id) {
    switch(event_id) {
    case A2DP_SINK_API_CONNECT_REQ:
        return"API_CONNECT_REQ";
    case A2DP_SINK_API_DISCONNECT_REQ:
        return "API_DISCONNECT_REQ";
    case A2DP_SINK_DISCONNECTED_CB:
        return "DISCONNECTED_CB";
    case A2DP_SINK_CONNECTING_CB:
        return "CONNECING_CB";
    case A2DP_SINK_CONNECTED_CB:
        return "CONNECTED_CB";
    case A2DP_SINK_DISCONNECTING_CB:
        return "DISCONNECTING_CB";
    case A2DP_SINK_FOCUS_REQUEST_CB:
        return "FOCUS_REQUEST_CB";
    case A2DP_SINK_AUDIO_SUSPENDED:
        return "AUDIO_SUSPENDED_CB";
    case A2DP_SINK_AUDIO_STOPPED:
        return "AUDIO_STOPPED_CB";
    case A2DP_SINK_AUDIO_STARTED:
        return "AUDIO_STARTED_CB";
    case AVRCP_CTRL_CONNECTED_CB:
        return "AVRCP_CTRL_CONNECTED_CB";
    case AVRCP_CTRL_DISCONNECTED_CB:
        return "AVRCP_CTRL_DISCONNECTED_CB";
    case AVRCP_CTRL_PASS_THRU_CMD_REQ:
        return "PASS_THRU_CMD_REQ";
    case BT_AM_CONTROL_STATUS:
        return "AM_CONTROL_STATUS";
    case A2DP_SINK_FETCH_PCM_DATA:
        return "A2DP_SINK_FETCH_PCM_DATA";
    case A2DP_SINK_CODEC_CONFIG:
        return "A2DP_SINK_CODEC_CONFIG";
    case A2DP_SINK_FILL_COMPRESS_BUFFER:
        return "FILL_COMPRESS_BUFFER";
    }
    return "UNKNOWN";
}
uint32_t A2dp_Sink::get_a2dp_sbc_sampling_rate(uint8_t frequency) {
    uint32_t freq = 48000;
    switch (frequency) {
        case SBC_SAMP_FREQ_16:
            freq = 16000;
            break;
        case SBC_SAMP_FREQ_32:
            freq = 32000;
            break;
        case SBC_SAMP_FREQ_44:
            freq = 44100;
            break;
        case SBC_SAMP_FREQ_48:
            freq = 48000;
            break;
    }
    return freq;
}

uint8_t A2dp_Sink::get_a2dp_sbc_channel_mode(uint8_t channeltype) {
    uint8_t count = 1;
    switch (channeltype) {
        case SBC_CH_MONO:
            count = 1;
            break;
        case SBC_CH_DUAL:
        case SBC_CH_STEREO:
        case SBC_CH_JOINT:
            count = 2;
            break;
    }
    return count;
}

uint32_t A2dp_Sink::get_a2dp_aac_sampling_rate(uint16_t frequency) {
    uint32_t freq = 0;
    switch (frequency) {
        case AAC_SAMP_FREQ_44100:
            freq = 44100;
            break;
        case AAC_SAMP_FREQ_48000:
            freq = 48000;
            break;
    }
    return freq;
}

uint8_t A2dp_Sink::get_a2dp_aac_channel_mode(uint8_t channel_count) {
    uint8_t count = 1;
    switch (channel_count) {
        case AAC_CHANNELS_1:
            count = 1;
            break;
        case AAC_CHANNELS_2:
            count = 2;
            break;
    }
    return count;
}

uint32_t A2dp_Sink::get_a2dp_mp3_sampling_rate(uint16_t frequency) {
    uint32_t freq = 0;
    switch (frequency) {
        case MP3_SAMP_FREQ_44100:
            freq = 44100;
            break;
        case MP3_SAMP_FREQ_48000:
            freq = 48000;
            break;
    }
    return freq;
}

uint8_t A2dp_Sink::get_a2dp_mp3_channel_mode(uint8_t channel_count) {
    uint8_t count = 1;
    switch (channel_count) {
        case MP3_CHANNEL_MONO:
            count = 1;
            break;
        case MP3_CHANNEL_DUAL:
        case MP3_CHANNEL_STEREO:
        case MP3_CHANNEL_JOINT_STEREO:
            count = 2;
            break;
    }
    return count;
}

uint32_t A2dp_Sink::get_a2dp_aptx_sampling_rate(uint8_t frequency) {
    uint32_t freq = 0;
    switch (frequency) {
        case APTX_SAMPLERATE_44100:
            freq = 44100;
            break;
        case APTX_SAMPLERATE_48000:
            freq = 48000;
            break;
    }
    return freq;
}

uint8_t A2dp_Sink::get_a2dp_aptx_channel_mode(uint8_t channel_count) {
    uint8_t count = 1;
    switch (channel_count) {
        case APTX_CHANNELS_MONO:
            count = 1;
            break;
        case APTX_CHANNELS_STEREO:
            count = 2;
            break;
    }
    return count;
}

void A2dp_Sink::state_disconnected_handler(BtEvent* pEvent) {
    char str[18];
    ALOGD(LOGTAG "state_disconnected_handler Processing event %s", dump_message(pEvent->event_id));
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
            bdaddr_to_string(&mConnectingDevice, str, 18);
            cout << "A2DP Sink Connecting to " << str << endl;
            change_state(STATE_PENDING);
            break;
        case A2DP_SINK_CONNECTED_CB:
            memset(&mConnectingDevice, 0, sizeof(bt_bdaddr_t));
            memcpy(&mConnectedDevice, &pEvent->a2dpSinkEvent.bd_addr, sizeof(bt_bdaddr_t));
            bdaddr_to_string(&mConnectedDevice, str, 18);
            cout << "A2DP Sink Connected to " << str << endl;
            change_state(STATE_CONNECTED);
            if(use_bt_a2dp_hal)
                OpenInputStream();
            break;
        default:
            ALOGD(LOGTAG " event not handled %d ", pEvent->event_id);
            break;
    }
}
void A2dp_Sink::state_pending_handler(BtEvent* pEvent) {
    char str[18];
    ALOGD(LOGTAG "state_pending_handler Processing event %s", dump_message(pEvent->event_id));
    switch(pEvent->event_id) {
        case A2DP_SINK_CONNECTING_CB:
            break;
        case A2DP_SINK_CONNECTED_CB:
            memcpy(&mConnectedDevice, &pEvent->a2dpSinkEvent.bd_addr, sizeof(bt_bdaddr_t));
            memset(&mConnectingDevice, 0, sizeof(bt_bdaddr_t));
            bdaddr_to_string(&mConnectedDevice, str, 18);
            cout << "A2DP Sink Connected to " << str << endl;
            change_state(STATE_CONNECTED);
            if(use_bt_a2dp_hal)
                OpenInputStream();
            break;
        case A2DP_SINK_DISCONNECTED_CB:
            cout << "A2DP Sink DisConnected "<< endl;
            memset(&mConnectedDevice, 0, sizeof(bt_bdaddr_t));
            memset(&mConnectingDevice, 0, sizeof(bt_bdaddr_t));

            change_state(STATE_DISCONNECTED);
            break;
        case A2DP_SINK_API_CONNECT_REQ:
            bdaddr_to_string(&mConnectingDevice, str, 18);
            cout << "A2DP Sink Connecting to " << str << endl;
            break;
        case A2DP_SINK_CODEC_CONFIG:
            codec_type = pEvent->a2dpSinkEvent.arg1;
            if (pEvent->a2dpSinkEvent.buf_ptr == NULL) {
                break;
            }
            memcpy(&codec_config, pEvent->a2dpSinkEvent.buf_ptr, pEvent->a2dpSinkEvent.buf_size);
            osi_free(pEvent->a2dpSinkEvent.buf_ptr);
            break;
        default:
            ALOGD(LOGTAG " event not handled %d ", pEvent->event_id);
            break;
    }
}

void A2dp_Sink::state_connected_handler(BtEvent* pEvent) {
    char str[18];
    uint32_t pcm_data_read = 0;
    BtEvent *pControlRequest, *pReleaseControlReq;
#if (defined(BT_AUDIO_HAL_INTEGRATION))
    qahw_out_buffer_t out_buf;
#endif
    ALOGD(LOGTAG " state_connected_handler Processing event %s", dump_message(pEvent->event_id));
    switch(pEvent->event_id) {
        case A2DP_SINK_API_CONNECT_REQ:
            bdaddr_to_string(&mConnectedDevice, str, 18);
            cout << "A2DP Sink Connected to " << str << endl;
            break;
        case A2DP_SINK_API_DISCONNECT_REQ:
            CloseAudioStream();
            // release control
            pReleaseControlReq = new BtEvent;
            pReleaseControlReq->btamControlRelease.event_id = BT_AM_RELEASE_CONTROL;
            pReleaseControlReq->btamControlRelease.profile_id = PROFILE_ID_A2DP_SINK;
            PostMessage(THREAD_ID_BT_AM, pReleaseControlReq);

            bdaddr_to_string(&mConnectedDevice, str, 18);
            cout << "A2DP Sink DisConnecting from " << str << endl;
            memset(&mConnectedDevice, 0, sizeof(bt_bdaddr_t));
            memset(&mConnectingDevice, 0, sizeof(bt_bdaddr_t));
            if (sBtA2dpSinkInterface != NULL) {
                sBtA2dpSinkInterface->disconnect(&pEvent->a2dpSinkEvent.bd_addr);
            }
            change_state(STATE_PENDING);
            break;
        case A2DP_SINK_DISCONNECTED_CB:
            CloseAudioStream();
            // release control
            pReleaseControlReq = new BtEvent;
            pReleaseControlReq->btamControlRelease.event_id = BT_AM_RELEASE_CONTROL;
            pReleaseControlReq->btamControlRelease.profile_id = PROFILE_ID_A2DP_SINK;
            PostMessage(THREAD_ID_BT_AM, pReleaseControlReq);

            memset(&mConnectedDevice, 0, sizeof(bt_bdaddr_t));
            memset(&mConnectingDevice, 0, sizeof(bt_bdaddr_t));
            cout << "A2DP Sink DisConnected " << endl;
            change_state(STATE_DISCONNECTED);
            break;
        case A2DP_SINK_DISCONNECTING_CB:
            CloseAudioStream();
            // release control
            pReleaseControlReq = new BtEvent;
            pReleaseControlReq->btamControlRelease.event_id = BT_AM_RELEASE_CONTROL;
            pReleaseControlReq->btamControlRelease.profile_id = PROFILE_ID_A2DP_SINK;
            PostMessage(THREAD_ID_BT_AM, pReleaseControlReq);

            cout << "A2DP Sink DisConnecting " << endl;
            change_state(STATE_PENDING);
            break;
        case A2DP_SINK_CODEC_CONFIG:
            codec_type = pEvent->a2dpSinkEvent.arg1;
            if (pEvent->a2dpSinkEvent.buf_ptr == NULL) {
                break;
            }
            memcpy(&codec_config, pEvent->a2dpSinkEvent.buf_ptr, pEvent->a2dpSinkEvent.buf_size);
            osi_free(pEvent->a2dpSinkEvent.buf_ptr);
            if (codec_type == A2DP_SINK_AUDIO_CODEC_SBC) {
                sample_rate = get_a2dp_sbc_sampling_rate(codec_config.sbc_config.samp_freq);
                channel_count = get_a2dp_sbc_channel_mode(codec_config.sbc_config.ch_mode);
            }
            break;
        case A2DP_SINK_AUDIO_STARTED:
        case A2DP_SINK_FOCUS_REQUEST_CB:
            pControlRequest = new BtEvent;
            pControlRequest->btamControlReq.event_id = BT_AM_REQUEST_CONTROL;
            pControlRequest->btamControlReq.profile_id = PROFILE_ID_A2DP_SINK;
            pControlRequest->btamControlReq.request_type = REQUEST_TYPE_PERMANENT;
            PostMessage(THREAD_ID_BT_AM, pControlRequest);
            break;
        case A2DP_SINK_FETCH_PCM_DATA:
           pcm_timer = false;
           if (pcm_buf == NULL) {
              // pcm buffer is null, closeStream have been called earlier
              break;
           }
           // first start next timer
           StartPcmTimer();

            if ((sBtA2dpSinkVendorInterface != NULL) && ( pcm_buf != NULL)) {
                if(use_bt_a2dp_hal) {
                    // read data from BT A2DP HAL
                    pcm_data_read =  ReadInputStream(pcm_buf, pcm_buf_size);
                }
                else {
                    // fetch PCM data from fluoride
                    pcm_data_read =  sBtA2dpSinkVendorInterface->get_a2dp_sink_streaming_data_vendor(
                            A2DP_SINK_AUDIO_CODEC_PCM, pcm_buf, pcm_buf_size);
                }
                ALOGD(LOGTAG " pcm_data_read = %d", pcm_data_read);
            }
#if (defined(BT_AUDIO_HAL_INTEGRATION))
            if ((pBTAM->GetAudioDevice() != NULL) && (out_stream != NULL) && (pcm_data_read)) {
                out_buf.buffer = pcm_buf;
                out_buf.bytes = pcm_data_read;
                qahw_out_write(out_stream, &out_buf);
            }
#endif
#if (defined(DUMP_PCM_DATA) && (DUMP_PCM_DATA == TRUE))
           if ((outputPcmSampleFile) && (pcm_buf != NULL))
           {
              fwrite ((void*)pcm_buf, 1, (size_t)(pcm_data_read), outputPcmSampleFile);
           }
#endif
            break;
        case BT_AM_CONTROL_STATUS:
            ALOGD(LOGTAG " earlier status = %d  new status = %d", controlStatus,
                                                      pEvent->btamControlStatus.status_type);
            controlStatus = pEvent->btamControlStatus.status_type;
            switch(controlStatus) {
                case STATUS_LOSS:
                    // inform bluedroid
                    if (sBtA2dpSinkVendorInterface != NULL) {
                        sBtA2dpSinkVendorInterface->audio_focus_state_vendor(0);
                    }
                    // send pause to remote
                    SendPassThruCommandNative(CMD_ID_PAUSE);
                    // release control
                    pReleaseControlReq = new BtEvent;
                    pReleaseControlReq->btamControlRelease.event_id = BT_AM_RELEASE_CONTROL;
                    pReleaseControlReq->btamControlRelease.profile_id = PROFILE_ID_A2DP_SINK;
                    PostMessage(THREAD_ID_BT_AM, pReleaseControlReq);
                    CloseAudioStream();
                    StopDataFetchTimer();
                    break;
                case STATUS_LOSS_TRANSIENT:
                    // inform bluedroid
                    if (sBtA2dpSinkVendorInterface != NULL) {
                        sBtA2dpSinkVendorInterface->audio_focus_state_vendor(0);
                    }
                    // send pause to remote
                    SendPassThruCommandNative(CMD_ID_PAUSE);
                    CloseAudioStream();
                    StopDataFetchTimer();
                    break;
                case STATUS_GAIN:
                    // inform bluedroid
                    if (sBtA2dpSinkVendorInterface != NULL) {
                        sBtA2dpSinkVendorInterface->audio_focus_state_vendor(3);
                    }
                    ConfigureAudioHal();
                    if (codec_type == A2DP_SINK_AUDIO_CODEC_SBC)
                        StartPcmTimer();
                    else {
                        BtEvent *pEvent = new BtEvent;
                        pEvent->a2dpSinkEvent.event_id = A2DP_SINK_FILL_COMPRESS_BUFFER;
                        PostMessage(THREAD_ID_A2DP_SINK, pEvent);
                    }
                    break;
                case STATUS_REGAINED:
                    // inform bluedroid
                    if (sBtA2dpSinkVendorInterface != NULL) {
                        sBtA2dpSinkVendorInterface->audio_focus_state_vendor(3);
                    }
                    ConfigureAudioHal();
                    if (codec_type == A2DP_SINK_AUDIO_CODEC_SBC)
                        StartPcmTimer();
                    else {
                        BtEvent *pEvent = new BtEvent;
                        pEvent->a2dpSinkEvent.event_id = A2DP_SINK_FILL_COMPRESS_BUFFER;
                        PostMessage(THREAD_ID_A2DP_SINK, pEvent);
                    }
                    // send play to remote
                    SendPassThruCommandNative(CMD_ID_PLAY);
                    break;
            }
            break;
        case A2DP_SINK_FILL_COMPRESS_BUFFER:
            FillCompressBuffertoAudioOutHal();
            break;
        case A2DP_SINK_AUDIO_SUSPENDED:
        case A2DP_SINK_AUDIO_STOPPED:
            // release focus in this case.
            CloseAudioStream();
            StopDataFetchTimer();
            if (use_bt_a2dp_hal) {
                SuspendInputStream();
            }
            if (controlStatus != STATUS_LOSS_TRANSIENT) {
                pReleaseControlReq = new BtEvent;
                pReleaseControlReq->btamControlRelease.event_id = BT_AM_RELEASE_CONTROL;
                pReleaseControlReq->btamControlRelease.profile_id = PROFILE_ID_A2DP_SINK;
                PostMessage(THREAD_ID_BT_AM, pReleaseControlReq);
            }
            break;
        default:
            ALOGD(LOGTAG " event not handled %d ", pEvent->event_id);
            break;
    }
}
#if (defined BT_AUDIO_HAL_INTEGRATION)
int compressed_callback(qahw_stream_callback_event_t event, void *param,
                  void *cookie) {
    BtEvent *pEvent = new BtEvent;
    switch (event) {
    case QAHW_STREAM_CBK_EVENT_WRITE_READY:
        ALOGD(LOGTAG " EVENT_WRITE_READY");
        pEvent->a2dpSinkEvent.event_id = A2DP_SINK_FILL_COMPRESS_BUFFER;
        PostMessage(THREAD_ID_A2DP_SINK, pEvent);
        break;
    case QAHW_STREAM_CBK_EVENT_DRAIN_READY:
        ALOGD(LOGTAG " EVENT_DRAIN_READY");
        break;
    default:
        break;
    }
    return 0;
}
#endif
uint8_t get_rtp_offset(uint8_t* p_start, uint16_t codec_type)
{
    uint8_t   rtp_version, padding, extension, csrc_count, extension_len;
    uint8_t offset = 0;
    uint8_t* ptr = p_start;
    uint16_t seq_num; uint32_t t_stamp;
    // NO RTP Header for APTX classic
    if (codec_type == A2DP_SINK_AUDIO_CODEC_APTX)
        return 0;
    rtp_version = *(p_start) >> 6;
    padding = (*(p_start) >> 5) & 0x01;
    extension = (*(p_start) >> 4) & 0x01;
    csrc_count = *(p_start) & 0x0F;
    p_start ++; p_start ++; // increment 2 byte
    BE_STREAM_TO_UINT16(seq_num, p_start);
    BE_STREAM_TO_UINT32(t_stamp, p_start);

    ALOGD(LOGTAG " rtp_v = %d, padding = %d, xtn = %d, csrc_count = %d, seq = %d, t_stamp = %d",
             rtp_version, padding, extension, csrc_count, seq_num, t_stamp);
    offset =  12 + csrc_count *4;
    if(extension)
    {
        ptr = ptr + offset + 2;
        BE_STREAM_TO_UINT16(extension_len, ptr);
        offset = offset + 4 + extension_len * 4;
    }
    ALOGD(LOGTAG " codec_type = %d offset = %d",codec_type, offset);
    return offset;
}

void A2dp_Sink::FillCompressBuffertoAudioOutHal() {
#if (defined(BT_AUDIO_HAL_INTEGRATION))
    qahw_out_buffer_t out_buf;
    uint32_t data_read_from_bt = 0;
    uint32_t data_sent_to_audio = 0;
    uint8_t rtp_offset = 0;
    if (pcm_buf == NULL) {
       // pcm buffer is null, closeStream have been called earlier
       ALOGE(LOGTAG " FillCOmpressBUffer, pcm buf null, bail out");
       return;
    }
    do {
        if ((sBtA2dpSinkInterface != NULL) && ( pcm_buf != NULL)) {
             // fetch PCM data from fluoride
            if( residual_compress_data == 0) {
            data_read_from_bt =  sBtA2dpSinkVendorInterface->get_a2dp_sink_streaming_data_vendor(
                 codec_type, pcm_buf, pcm_buf_size);
            if (fetch_rtp_info && (data_read_from_bt > 12)) {
                rtp_offset = get_rtp_offset(pcm_buf, codec_type);
                data_read_from_bt = data_read_from_bt - rtp_offset;
                }
            }
            else {
                data_read_from_bt =  residual_compress_data;
            }
        }
        if (data_read_from_bt <= 0) {
           // in this case, we don't have data from bt, but we try after some time
           ALOGD(LOGTAG " NO Data from BT , try after %d ms", A2DP_SINK_PCM_FETCH_TIMER_DURATION);
           StartCompressAudioFeedTimer();
           break;
        }
        if ((pBTAM->GetAudioDevice() != NULL) && (out_stream != NULL)) {
             if (fetch_rtp_info) {
                 out_buf.buffer = pcm_buf + rtp_offset;
             } else {
                 out_buf.buffer = pcm_buf;
             }
             out_buf.bytes = data_read_from_bt;
#if (defined(DUMP_COMPRESSED_DATA) && (DUMP_COMPRESSED_DATA == TRUE))
             if ((outputPcmSampleFile) && (pcm_buf != NULL))
             {
                fwrite ((void*)pcm_buf, 1, (size_t)(data_read_from_bt), outputPcmSampleFile);
                data_sent_to_audio = data_read_from_bt;
             }
#else
             data_sent_to_audio = qahw_out_write(out_stream, &out_buf);
#endif
             cuml_data_written_to_audio = cuml_data_written_to_audio + data_sent_to_audio;
        }
        ALOGD(LOGTAG " data_read_from_bt = %d data_sent_to_audio = %d cuml_data = %d", data_read_from_bt,
                 data_sent_to_audio, cuml_data_written_to_audio);
        residual_compress_data = data_read_from_bt - data_sent_to_audio;
        if (residual_compress_data > 0) {
           /* This is the case for AUDIO buffers completely filled
            * We should wait for EVENT_WRITE_READY from AUDIO */
           ALOGE( LOGTAG " Residual Data, Wait for EVENT_WRITE_READY ");
           memcpy(pcm_buf, pcm_buf + data_sent_to_audio, data_read_from_bt - data_sent_to_audio);
           break;
        }
    }while(1);
    ALOGD(LOGTAG " FillCompressBuffertoAudioOutHal - cum_data = %d", cuml_data_written_to_audio);
    if(cuml_data_written_to_audio >= pcm_buf_size)//reset for next iteration.
        cuml_data_written_to_audio = 0;
#endif
}
void A2dp_Sink::ConfigureAudioHal() {
#if (defined BT_AUDIO_HAL_INTEGRATION)
    qahw_module_handle_t* audio_device;
    audio_config_t config;
    audio_io_handle_t handle = 0x07;
    //audio_output_flags_t flags = AUDIO_OUTPUT_FLAG_NONE;
    int flags = AUDIO_OUTPUT_FLAG_NONE;

    memset(&config, 0, sizeof(audio_config_t));
    config.offload_info.size = sizeof(audio_offload_info_t);
    ALOGD(LOGTAG " ConfigureAudioHal codec_type = %d", codec_type);
    switch(codec_type) {
    case A2DP_SINK_AUDIO_CODEC_SBC:
        sample_rate = get_a2dp_sbc_sampling_rate(codec_config.sbc_config.samp_freq);
        channel_count = get_a2dp_sbc_channel_mode(codec_config.sbc_config.ch_mode);
        config.offload_info.format = AUDIO_FORMAT_PCM_16_BIT;
        flags |= AUDIO_OUTPUT_FLAG_DIRECT_PCM;
        break;
    case A2DP_SINK_AUDIO_CODEC_AAC:
        sample_rate = get_a2dp_aac_sampling_rate(codec_config.aac_config.sampling_freq);
        channel_count = get_a2dp_aac_channel_mode(codec_config.aac_config.channel_count);
        config.offload_info.format = AUDIO_FORMAT_AAC_LATM_LC;
        flags |= AUDIO_OUTPUT_FLAG_NON_BLOCKING;
        flags |= AUDIO_OUTPUT_FLAG_COMPRESS_OFFLOAD;
        break;
    case A2DP_SINK_AUDIO_CODEC_MP3:
        sample_rate = get_a2dp_mp3_sampling_rate(codec_config.mp3_config.sampling_freq);
        channel_count = get_a2dp_mp3_channel_mode(codec_config.mp3_config.channel_count);
        config.offload_info.format = AUDIO_FORMAT_MP3;
        flags |= AUDIO_OUTPUT_FLAG_NON_BLOCKING;
        flags |= AUDIO_OUTPUT_FLAG_COMPRESS_OFFLOAD;
        break;
    case A2DP_SINK_AUDIO_CODEC_APTX:
        sample_rate = get_a2dp_aptx_sampling_rate(codec_config.aptx_config.sampling_freq);
        channel_count = get_a2dp_aptx_channel_mode(codec_config.aptx_config.channel_count);
        //config.offload_info.format = AUDIO_FORMAT_APTX;// TODO:ADD for APTX_FR
        flags |= AUDIO_OUTPUT_FLAG_NON_BLOCKING;
        flags |= AUDIO_OUTPUT_FLAG_COMPRESS_OFFLOAD;
        break;
    }
    ALOGD(LOGTAG " sample_rate = %d, channel_count = %d", sample_rate, channel_count);

    if (out_stream != NULL) {
        ALOGD(LOGTAG " HAL already configured ");
        return;
    }
    // HAL is not yet configured, configure it now.
   config.offload_info.version = AUDIO_OFFLOAD_INFO_VERSION_CURRENT;
   config.sample_rate = sample_rate;
   config.offload_info.sample_rate = sample_rate;
   config.channel_mask = audio_channel_out_mask_from_count(channel_count);
   config.offload_info.channel_mask = audio_channel_out_mask_from_count(channel_count);
    if (pBTAM != NULL) {
        audio_device = pBTAM->GetAudioDevice();
        if(audio_device != NULL) {
            // 2 refers to speaker
            ALOGD(LOGTAG " opening output stream ");
            qahw_open_output_stream(audio_device, handle, 2, (audio_output_flags_t)flags,
                   &config, &out_stream, "bt_a2dp_sink");
        }
        if (out_stream != NULL) {
            pcm_buf_size = qahw_out_get_buffer_size(out_stream);
            ALOGD(LOGTAG " pcm buf size %d", pcm_buf_size);
            pcm_buf = (uint8_t*)osi_malloc(pcm_buf_size);
        }
        if (codec_type != A2DP_SINK_AUDIO_CODEC_SBC) {
            qahw_out_set_callback(out_stream, compressed_callback, NULL);
        }
    }
#endif
#if (defined(DUMP_PCM_DATA) && (DUMP_PCM_DATA == TRUE))
    if (!sample_rate || !channel_count) {
        return;
    }
    switch(sample_rate) {
    case 44100:
        pcm_buf_size = 7065;
        break;
    case 48000:
        pcm_buf_size = 7680;
        break;
    }
    pcm_buf = (uint8_t*)osi_malloc(pcm_buf_size);
    if (outputPcmSampleFile == NULL)
        outputPcmSampleFile = fopen(outputFilename, "ab");
#endif
#if (defined(DUMP_COMPRESSED_DATA) && (DUMP_COMPRESSED_DATA == TRUE))
    if (outputPcmSampleFile == NULL)
        outputPcmSampleFile = fopen(outputFilename, "ab");
#endif
}
void A2dp_Sink::CloseAudioStream() {
#if (defined BT_AUDIO_HAL_INTEGRATION)
    qahw_module_handle_t* audio_device;
    if (pBTAM != NULL) {
        audio_device = pBTAM->GetAudioDevice();
        if((audio_device != NULL) && (out_stream != NULL)) {
            // 2 refers to speaker
            ALOGD(LOGTAG " closing output stream ");
            qahw_close_output_stream(out_stream);
            cuml_data_written_to_audio = 0;
            residual_compress_data = 0;
            out_stream = NULL;
        }
        if (pcm_buf != NULL) {
            osi_free(pcm_buf);
            pcm_buf = NULL;
        }
    }
#endif
#if (defined(DUMP_PCM_DATA) && (DUMP_PCM_DATA == TRUE))
    if (outputPcmSampleFile)
    {
        fclose(outputPcmSampleFile);
    }
    outputPcmSampleFile = NULL;
    if (pcm_buf != NULL) {
        osi_free(pcm_buf);
        pcm_buf = NULL;
    }
#endif
#if (defined(DUMP_COMPRESSED_DATA) && (DUMP_COMPRESSED_DATA == TRUE))
    if (outputPcmSampleFile)
    {
        fclose(outputPcmSampleFile);
    }
    outputPcmSampleFile = NULL;
#endif
}
void A2dp_Sink::LoadBtA2dpHAL() {
#if (defined(BT_AUDIO_HAL_INTEGRATION))
    ALOGD(LOGTAG " Load A2dp HAL ");
    a2dp_input_device = qahw_load_module(QAHW_MODULE_ID_A2DP);
    if (a2dp_input_device == NULL) {
        ALOGE(LOGTAG " A2dp Hal can not be opened ");
        return;
    }
    ALOGD(LOGTAG " A2dp HAL successfully loaded ");
#endif
}

void A2dp_Sink::UnLoadBtA2dpHAL() {
#if (defined(BT_AUDIO_HAL_INTEGRATION))
    int ret  =  0;
    ALOGD(LOGTAG " Unload A2dp HAL");
    if(!a2dp_input_device)
    {
        ALOGD(LOGTAG " A2dp_input_device not valid ");
        return;
    }
    ret = qahw_unload_module(a2dp_input_device);
    if (ret < 0) {
        ALOGE(LOGTAG " A2dp HAL could not be closed gracefully");
        a2dp_input_device = NULL;
        return;
    }
    a2dp_input_device = NULL;
    ALOGD(LOGTAG " A2dp HAL successfully Unloaded ");
#endif
}

void A2dp_Sink::OpenInputStream()
{
#if (defined(BT_AUDIO_HAL_INTEGRATION))
    int ret = -1;
    ALOGD(LOGTAG " Open A2dp Input Stream ");
    if (!a2dp_input_device) {
        ALOGE(LOGTAG " Invalid A2dp HAL device. Bail out! ");
        return;
    }
    ret = qahw_open_input_stream(a2dp_input_device, 0, AUDIO_DEVICE_OUT_ALL_A2DP,
            NULL, &input_stream, AUDIO_INPUT_FLAG_NONE, "bt_a2dp_input_stream" , AUDIO_SOURCE_DEFAULT);
    if (ret < 0) {
        input_stream = NULL;
        ALOGE(LOGTAG " open input stream returned %d\n ", ret);
    }
    ALOGD(LOGTAG " A2dp Input Stream successfully opened ");
#endif
}

void A2dp_Sink::CloseInputStream()
{
#if (defined(BT_AUDIO_HAL_INTEGRATION))
    ALOGD(LOGTAG " Close A2dp Input Stream ");
    if ((a2dp_input_device == NULL) || (input_stream == NULL)) {
        ALOGE(LOGTAG " Invalid A2dp HAL device. Bail out! ");
        return;
    }
    qahw_close_input_stream(input_stream);
    input_stream = NULL;
    ALOGD(LOGTAG " A2dp Input Stream successfully closed ");
#endif
}

void A2dp_Sink::SuspendInputStream()
{
#if (defined(BT_AUDIO_HAL_INTEGRATION))
    ALOGD(LOGTAG " Suspend Input Stream ");
    if(!input_stream)
    {
        ALOGE(LOGTAG " Invalid Input Stream. Bail out! ");
        return;
    }
    qahw_in_standby(input_stream);
    ALOGD(LOGTAG " A2dp Stream suspended successfully");
#endif
}

uint32_t A2dp_Sink::ReadInputStream(uint8_t* data, uint32_t size)
{
#if (defined(BT_AUDIO_HAL_INTEGRATION))
    uint32_t data_read;
    qahw_in_buffer_t in_buf;
    ALOGD(LOGTAG " Read Input Stream");
    if(!input_stream)
    {
        ALOGE(LOGTAG " Invalid Input Stream. Bail out! ");
        return 0 ;
    }
    in_buf.buffer = data;
    in_buf.bytes = size;
    data_read = qahw_in_read(input_stream, &in_buf);;
    ALOGD(LOGTAG " A2dp Input Stream bytes read = %d", data_read);
    return data_read;
#endif
}

uint32_t A2dp_Sink::GetInputStreamBufferSize()
{
#if (defined(BT_AUDIO_HAL_INTEGRATION))
    ALOGD(LOGTAG " GetInputStreamBufferSize + ");
    if(!input_stream)
    {
        ALOGE(LOGTAG " Invalid Input Stream. Bail out! ");
        return 0 ;
    }
    return qahw_in_get_buffer_size(input_stream);
    ALOGD(LOGTAG " GetInputStreamBufferSize %d ");
#endif
}

void A2dp_Sink::OnDisconnected() {
    ALOGD(LOGTAG " onDisconnected ");
    StopDataFetchTimer();
    if (use_bt_a2dp_hal) {
        CloseInputStream();
    }
    CloseAudioStream();
    pcm_buf_size = 0;
    cuml_data_written_to_audio = 0;
    residual_compress_data = 0;
    codec_type = A2DP_SINK_AUDIO_CODEC_SBC;
    memset(&codec_config, 0, sizeof(btav_codec_config_t));

}
void A2dp_Sink::change_state(A2dpSinkState mState) {
   ALOGD(LOGTAG " current State = %d, new state = %d", mSinkState, mState);
   pthread_mutex_lock(&lock);
   mSinkState = mState;
   if (mSinkState == STATE_DISCONNECTED)
   {
        OnDisconnected();
   }
   ALOGD(LOGTAG " state changes to %d ", mState);
   pthread_mutex_unlock(&lock);
}
A2dp_Sink :: A2dp_Sink(const bt_interface_t *bt_interface, config_t *config) {

    this->bluetooth_interface = bt_interface;
    this->config = config;
    sBtA2dpSinkInterface = NULL;
    sBtAvrcpCtrlInterface = NULL;
    mSinkState = STATE_NOT_STARTED;
    controlStatus = STATUS_LOSS;
    mAvrcpConnected = false;
    use_bt_a2dp_hal = false;
    channel_count = 0;
    sample_rate = 0;
    memset(&mConnectedDevice, 0, sizeof(bt_bdaddr_t));
    memset(&mConnectingDevice, 0, sizeof(bt_bdaddr_t));
    memset(&mConnectedAvrcpDevice, 0, sizeof(bt_bdaddr_t));
    pthread_mutex_init(&this->lock, NULL);
    pcm_data_fetch_timer = alarm_new();
    compress_audio_feed_timer = alarm_new();
    pcm_buf = NULL;
    pcm_timer = false;
    compress_offload_timer = false;
    residual_compress_data = 0;
    codec_type = A2DP_SINK_AUDIO_CODEC_SBC;//by default make it SBC
    memset(&codec_config, 0, sizeof(btav_codec_config_t));
#if (defined BT_AUDIO_HAL_INTEGRATION)
    out_stream =  NULL;
    input_stream = NULL;
    a2dp_input_device = NULL;
#endif
#if (defined(DUMP_PCM_DATA) && (DUMP_PCM_DATA == TRUE))
    outputPcmSampleFile =  NULL;
#endif
#if (defined(DUMP_COMPRESSED_DATA) && (DUMP_COMPRESSED_DATA == TRUE))
    outputPcmSampleFile =  NULL;
#endif
}

A2dp_Sink :: ~A2dp_Sink() {
    pthread_mutex_destroy(&lock);
    mAvrcpConnected = false;
    use_bt_a2dp_hal = false;
    controlStatus = STATUS_LOSS;
    alarm_free(pcm_data_fetch_timer);
    alarm_free(compress_audio_feed_timer);
    pcm_data_fetch_timer = NULL;
    compress_audio_feed_timer = NULL;
#if (defined BT_AUDIO_HAL_INTEGRATION)
    out_stream = NULL;
    input_stream = NULL;
    a2dp_input_device = NULL;
#endif
    if (pcm_buf != NULL) {
        osi_free(pcm_buf);
        pcm_buf = NULL;
    }
    codec_type = A2DP_SINK_AUDIO_CODEC_SBC;//by default make it SBC
    memset(&codec_config, 0, sizeof(btav_codec_config_t));
}
