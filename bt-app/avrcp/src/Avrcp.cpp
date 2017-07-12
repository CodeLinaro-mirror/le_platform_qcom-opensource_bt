 /*
  * Copyright (c) 2016-2017, The Linux Foundation. All rights reserved.
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
#include <hardware/bt_rc.h>

#include "Avrcp.hpp"
#include "A2dp_Sink_Streaming.hpp"
#include "Gap.hpp"
#include "hardware/bt_rc_vendor.h"
#include "A2dp_Sink.hpp"
#include <math.h>
#include <algorithm>

#define LOGTAG "AVRCP"
#define LOGTAG_CTRL "AVRCP_CTRL"

using namespace std;
using std::list;
using std::string;

Avrcp *pAvrcp = NULL;
extern A2dp_Sink_Streaming *pA2dpSinkStream;
extern A2dp_Sink *pA2dpSink;

static const bt_bdaddr_t bd_addr_null= {0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
#define ABS_VOL_BASE 127
#define AUDIO_MAX_VOL_LEVEL 15
int curr_audio_index = 1;
#ifdef __cplusplus
extern "C" {
#endif

void BtAvrcpMsgHandler(void *msg) {
    BtEvent* pEvent = NULL;
    BtEvent* pCleanupEvent = NULL;
    if(!msg) {
        printf("Msg is NULL, return.\n");
        return;
    }

    pEvent = ( BtEvent *) msg;
    switch(pEvent->event_id) {
        case PROFILE_API_START:
            ALOGD(LOGTAG " enable avrcp");
            if (pAvrcp) {
                pAvrcp->HandleEnableAvrcp();
            }
            break;
        case PROFILE_API_STOP:
            ALOGD(LOGTAG " disable avrcp");
            if (pAvrcp) {
                pAvrcp->HandleDisableAvrcp();
            }
            break;
        case AVRCP_CLEANUP_REQ:
            ALOGD(LOGTAG " cleanup a2dp avrcp");
            pCleanupEvent = new BtEvent;
            pCleanupEvent->event_id = AVRCP_CLEANUP_DONE;
            PostMessage(THREAD_ID_GAP, pCleanupEvent);
            break;
        case AVRCP_CTRL_CONNECTED_CB:
        case AVRCP_CTRL_DISCONNECTED_CB:
        case AVRCP_CTRL_PASS_THRU_CMD_REQ:
        case AVRCP_CTRL_REG_NOTI_ABS_VOL_CB:
        case AVRCP_CTRL_VOL_CHANGED_NOTI_REQ:
        case AVRCP_CTRL_SET_ABS_VOL_CMD_CB:
            ALOGD( LOGTAG_CTRL " handle avrcp events ");
            if (pAvrcp) {
                pAvrcp->HandleAvrcpEvents(( BtEvent *) msg);
            }
            break;
        default:
            break;
    }
    delete pEvent;
}

#ifdef __cplusplus
}
#endif


static void btavrcpctrl_passthru_rsp_vendor_callback(int id, int key_state, bt_bdaddr_t *bd_addr) {
    ALOGD(LOGTAG_CTRL " btavrcpctrl_passthru_rsp_vendor_callback id = %d key_state = %d",
            id, key_state);
    if (id == CMD_ID_PAUSE && key_state == 1 &&
            !memcmp(&pA2dpSinkStream->mStreamingDevice, bd_addr, sizeof(bt_bdaddr_t)))
    {
        ALOGD(LOGTAG_CTRL " need to flush both stack queue and audio queue ");
        BtEvent *pFlushAudioPackets = new BtEvent;
        pFlushAudioPackets->a2dpSinkStreamingEvent.event_id = A2DP_SINK_STREAMING_FLUSH_AUDIO;
        memcpy(&pFlushAudioPackets->a2dpSinkStreamingEvent.bd_addr, bd_addr, sizeof(bt_bdaddr_t));
        if (pA2dpSinkStream) {
            thread_post(pA2dpSinkStream->threadInfo.thread_id,
            pA2dpSinkStream->threadInfo.thread_handler, (void*)pFlushAudioPackets);
        }
    }
}

static void btavrcpctrl_passthru_rsp_callback(int id, int key_state) {
    ALOGD(LOGTAG_CTRL " btavrcpctrl_passthru_rsp_callback id = %d key_state = %d", id, key_state);
}

static void btavrcpctrl_groupnavigation_rsp_callback(int id, int key_state) {
    ALOGD(LOGTAG_CTRL " btavrcpctrl_groupnavigation_rsp_callback id = %d key_state = %d", id, key_state);
}

static void btavrcpctrl_setplayerapplicationsetting_rsp_callback(bt_bdaddr_t *bd_addr, uint8_t accepted) {
    ALOGD(LOGTAG_CTRL " btavrcctrl_setplayerapplicationsetting_rsp_callback accepted = %d", accepted);
}

static void btavrcpctrl_playerapplicationsetting_callback(bt_bdaddr_t *bd_addr, uint8_t num_attr,
                                                          btrc_player_app_attr_t *app_attrs,
                                                          uint8_t num_ext_attr, btrc_player_app_ext_attr_t *ext_attrs) {
     ALOGD(LOGTAG_CTRL " btavrcpctrl_playerapplicationsetting_callback");
}
 
static void btavrcpctrl_playerapplicationsetting_changed_callback(bt_bdaddr_t *bd_addr, btrc_player_settings_t *p_vals) {
     ALOGD(LOGTAG_CTRL " btrc_ctrl_playerapplicationsetting_changed_callback");
}

static void btavrcpctrl_track_changed_callback(bt_bdaddr_t *bd_addr, uint8_t num_attr,
                                                     btrc_element_attr_val_t *p_attrs) {
    ALOGD(LOGTAG_CTRL "btrc_ctrl_track_changed_callback");
}

static void btavrcpctrl_play_position_changed_callback(bt_bdaddr_t *bd_addr,
                                                          uint32_t song_len, uint32_t song_pos, btrc_play_status_t play_status) {
    ALOGD(LOGTAG_CTRL "btrc_ctrl_play_position_changed_callback");
}

static void btavrcpctrl_play_status_changed_callback(bt_bdaddr_t *bd_addr, btrc_play_status_t play_status) {
    ALOGD(LOGTAG_CTRL "btrc_ctrl_play_status_changed_callback");
}

static void btavrcpctrl_connection_state_callback(bool state, bt_bdaddr_t* bd_addr) {
    ALOGD(LOGTAG_CTRL " btavrcpctrl_connection_state_callback state = %d", state);
    BtEvent *pEvent = new BtEvent;
    memcpy(&pEvent->avrcpCtrlEvent.bd_addr, bd_addr, sizeof(bt_bdaddr_t));
    if (state == true)
        pEvent->avrcpCtrlEvent.event_id = AVRCP_CTRL_CONNECTED_CB;
    else
        pEvent->avrcpCtrlEvent.event_id = AVRCP_CTRL_DISCONNECTED_CB;
    PostMessage(THREAD_ID_AVRCP, pEvent);
}

static void btavrcpctrl_getrcfeatures_callback( bt_bdaddr_t* bd_addr, int features) {
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

static void btavrcpctrl_setabsvol_cmd_callback(bt_bdaddr_t *bd_addr, uint8_t abs_vol, uint8_t label) {
    ALOGD(LOGTAG_CTRL " btavrcpctrl_setabsvol_cmd_vendor_callback");
    BtEvent *pEvent = new BtEvent;
    pEvent->avrcpCtrlEvent.event_id = AVRCP_CTRL_SET_ABS_VOL_CMD_CB;
    pEvent->avrcpCtrlEvent.arg1 = label;
    pEvent->avrcpCtrlEvent.arg2 = abs_vol;
    memcpy(&pEvent->avrcpCtrlEvent.bd_addr, bd_addr, sizeof(bt_bdaddr_t));
    PostMessage(THREAD_ID_AVRCP, pEvent);
}

static void btavrcpctrl_registernotification_absvol_callback(bt_bdaddr_t *bd_addr, uint8_t label) {
    ALOGD(LOGTAG_CTRL " btavrcpctrl_registernotification_absvol_vendor_callback");
    BtEvent *pEvent = new BtEvent;
    pEvent->avrcpCtrlEvent.event_id = AVRCP_CTRL_REG_NOTI_ABS_VOL_CB;
    pEvent->avrcpCtrlEvent.arg1 = label;
    memcpy(&pEvent->avrcpCtrlEvent.bd_addr, bd_addr, sizeof(bt_bdaddr_t));
    PostMessage(THREAD_ID_AVRCP, pEvent);
}

static btrc_ctrl_callbacks_t sBluetoothAvrcpCtrlCallbacks = {
   sizeof(sBluetoothAvrcpCtrlCallbacks),
   btavrcpctrl_passthru_rsp_callback,
   btavrcpctrl_groupnavigation_rsp_callback,
   btavrcpctrl_connection_state_callback,
   btavrcpctrl_getrcfeatures_callback,
   btavrcpctrl_setplayerapplicationsetting_rsp_callback,
   btavrcpctrl_playerapplicationsetting_callback,
   btavrcpctrl_playerapplicationsetting_changed_callback,
   btavrcpctrl_setabsvol_cmd_callback,
   btavrcpctrl_registernotification_absvol_callback,
   btavrcpctrl_track_changed_callback,
   btavrcpctrl_play_position_changed_callback,
   btavrcpctrl_play_status_changed_callback,
};

static btrc_ctrl_vendor_callbacks_t sBluetoothAvrcpCtrlVendorCallbacks = {
   sizeof(sBluetoothAvrcpCtrlVendorCallbacks),
   btavrcpctrl_getcap_rsp_vendor_callback,
   btavrcpctrl_listplayerappsettingattrib_rsp_vendor_callback,
   btavrcpctrl_listplayerappsettingvalue_rsp_vendor_callback,
   btavrcpctrl_currentplayerappsetting_rsp_vendor_callback,
   btavrcpctrl_notification_rsp_vendor_callback,
   btavrcpctrl_getelementattrib_rsp_vendor_callback,
   btavrcpctrl_getplaystatus_rsp_vendor_callback,
   btavrcpctrl_passthru_rsp_vendor_callback,
};

void Avrcp::SendPassThruCommandNative(uint8_t key_id, bt_bdaddr_t* addr, uint8_t direct) {
    ALOGD(LOGTAG_CTRL " SendPassThruCommandNative ");
    if (memcmp(&pA2dpSinkStream->mStreamingDevice, &bd_addr_null, sizeof(bt_bdaddr_t)) &&
            memcmp(&pA2dpSinkStream->mStreamingDevice, addr, sizeof(bt_bdaddr_t)) &&
            (key_id == CMD_ID_PLAY))
        direct = 1;

    if (!direct && pA2dpSinkStream && pA2dpSinkStream->use_bt_a2dp_hal &&
            ((key_id == CMD_ID_PAUSE) || (key_id == CMD_ID_PLAY))) {
        ALOGD(LOGTAG_CTRL " SendPassThruCommandNative: using bt_a2dp_hal ");
        if (CMD_ID_PAUSE == key_id)
        {
            ALOGD( LOGTAG_CTRL " sending stopped event ");

            if (!memcmp(&pA2dpSinkStream->mStreamingDevice, addr, sizeof(bt_bdaddr_t)))
            {
                pA2dpSinkStream->StopDataFetchTimer();
                pA2dpSinkStream->SuspendInputStream();
            }

        }
        if (CMD_ID_PLAY == key_id)
        {
            ALOGD( LOGTAG_CTRL " sending started event ");
            BtEvent *pEvent = new BtEvent;
            pEvent->a2dpSinkEvent.event_id = A2DP_SINK_AUDIO_STARTED;
            memcpy(&pEvent->a2dpSinkEvent.bd_addr, addr, sizeof(bt_bdaddr_t));
            PostMessage(THREAD_ID_A2DP_SINK, pEvent);
        }
    }
    else if (sBtAvrcpCtrlInterface != NULL) {
        ALOGD(LOGTAG_CTRL " SendPassThruCommandNative send_pass_through_cmd");

        sBtAvrcpCtrlInterface->send_pass_through_cmd(addr, key_id, 0);
        sBtAvrcpCtrlInterface->send_pass_through_cmd(addr, key_id, 1);
    }
}

list<A2dp_Device>::iterator FindAvDeviceByAddr(list<A2dp_Device>& pA2dpDev, bt_bdaddr_t dev) {
    list<A2dp_Device>::iterator p = pA2dpDev.begin();
    while(p != pA2dpDev.end()) {
        if (memcmp(&dev, &p->mDevice, sizeof(bt_bdaddr_t)) == 0) {
            break;
        }
        p++;
    }
    return p;
}

int getVolumePercentage() {
    int maxVolume = AUDIO_MAX_VOL_LEVEL;
                  //mAudioManager.getStreamMaxVolume(AudioManager.STREAM_MUSIC);
    int currIndex = curr_audio_index;
                  //mAudioManager.getStreamVolume(AudioManager.STREAM_MUSIC);
    int percentageVol = ((currIndex * ABS_VOL_BASE) / maxVolume);
    return percentageVol;
}

void Avrcp::setAbsVolume(bt_bdaddr_t* dev, int absVol, int label) {
    int maxVolume = AUDIO_MAX_VOL_LEVEL;
                  //mAudioManager.getStreamMaxVolume(AudioManager.STREAM_MUSIC);
    int currIndex = curr_audio_index;
                  //mAudioManager.getStreamVolume(AudioManager.STREAM_MUSIC);

    // Ignore first volume command since phone may not know difference between stream volume
    // and amplifier volume.
    if (mFirstAbsVolCmdRecvd) {
        int newIndex =(int) round((double) absVol * maxVolume / ABS_VOL_BASE);
        ALOGD(LOGTAG_CTRL " setAbsVol = %d maxVol = %d cur = %d new = %d", absVol,
                                                  maxVolume, currIndex, newIndex);
        /*
              * In some cases change in percentage is not sufficient enough to warrant
              * change in index values which are in range of 0-15. For such cases
              * no action is required
              */
        if (newIndex != currIndex) {
            curr_audio_index = newIndex;
            pA2dpSinkStream->SetStreamVol(curr_audio_index);
        }
    } else {
        mFirstAbsVolCmdRecvd = true;
        absVol = (currIndex * ABS_VOL_BASE) / maxVolume;
        ALOGD(LOGTAG_CTRL " SetAbsVol recvd for first time, respond with absVol %d", absVol);
    }
    sBtAvrcpCtrlInterface->set_volume_rsp(dev, absVol, label);
}

void Avrcp::HandleAvrcpEvents(BtEvent* pEvent) {
    list<A2dp_Device>::iterator iter;
    int perVol = 0;
    bdstr_t bd_str;
    std::list<std::string>::iterator bdstring;
    ALOGD(LOGTAG_CTRL " HandleAvrcpEvents event = %s",
            dump_message(pEvent->avrcpCtrlEvent.event_id));
    switch(pEvent->avrcpCtrlEvent.event_id) {
    case AVRCP_CTRL_CONNECTED_CB:
        iter = FindAvDeviceByAddr(pA2dpSink->pA2dpDeviceList, pEvent->avrcpCtrlEvent.bd_addr);
        if (iter != pA2dpSink->pA2dpDeviceList.end())
        {
            ALOGD(LOGTAG_CTRL " Rc connection for AV connected dev, mark avrcp connected");
            iter->mAvrcpConnected = true;
        }
        else
        {
            ALOGE(LOGTAG_CTRL " Rc connection from device without AV connection");
            bdaddr_to_string(&pEvent->avrcpCtrlEvent.bd_addr, &bd_str[0], sizeof(bd_str));
            std::string deviceAddress(bd_str);
            bdstring = std::find(rc_only_devices.begin(), rc_only_devices.end(), deviceAddress);
            if (bdstring == rc_only_devices.end())
            {
                ALOGE(LOGTAG_CTRL " RC connected for this dev w/o AV, cache this device in list");
                rc_only_devices.push_back(deviceAddress);
            }
            else
            {
                ALOGE(LOGTAG_CTRL " this RC device already in list, should never hit here, ERROR!!!");
            }
        }
        break;
    case AVRCP_CTRL_DISCONNECTED_CB:
        iter = FindAvDeviceByAddr(pA2dpSink->pA2dpDeviceList, pEvent->avrcpCtrlEvent.bd_addr);
        if (iter != pA2dpSink->pA2dpDeviceList.end())
        {
            ALOGD(LOGTAG_CTRL " Rc disconnection for AV connected dev, mark avrcp disconnected");
            iter->mAvrcpConnected = false;
        }
        else
        {
            ALOGE(LOGTAG_CTRL " Rc disconnection from device without AV connection");
            bdaddr_to_string(&pEvent->avrcpCtrlEvent.bd_addr, &bd_str[0], sizeof(bd_str));
            std::string deviceAddress(bd_str);
            bdstring = std::find(rc_only_devices.begin(), rc_only_devices.end(), deviceAddress);
            if (bdstring != rc_only_devices.end())
            {
                ALOGD (LOGTAG " found match for RC only disconnection, remove from list");
                rc_only_devices.remove(deviceAddress);
            }
            else
            {
                ALOGD (LOGTAG " found no match for RC only disconnection, entry was removed during AV connection");
            }
        }
        break;
    case AVRCP_CTRL_PASS_THRU_CMD_REQ:
        iter = FindAvDeviceByAddr(pA2dpSink->pA2dpDeviceList, pEvent->avrcpCtrlEvent.bd_addr);
        if (iter != pA2dpSink->pA2dpDeviceList.end() && (iter->mAvrcpConnected == true))
        {
            ALOGD(LOGTAG_CTRL " passthrough cmd for AV & RC connected device, send to stack");
            SendPassThruCommandNative(pEvent->avrcpCtrlEvent.key_id,
            &pEvent->avrcpCtrlEvent.bd_addr, 0);
        }
        else
        {
            ALOGD(LOGTAG_CTRL " Avrcp not connected or AV not connected");
        }
        break;
    case AVRCP_CTRL_SET_ABS_VOL_CMD_CB:
        iter = FindAvDeviceByAddr(pA2dpSink->pA2dpDeviceList, pEvent->avrcpCtrlEvent.bd_addr);
        if (iter != pA2dpSink->pA2dpDeviceList.end() && (iter->mAvrcpConnected == true))
        {
            ALOGD(LOGTAG_CTRL " setabsvol cmd cb for AV & RC connected device, send to stack");
            iter->mAbsoluteVolumeChangeInProgress = true;
            setAbsVolume(&iter->mDevice, (int)pEvent->avrcpCtrlEvent.arg2,
                                         (int)pEvent->avrcpCtrlEvent.arg1);
        }
        else
        {
            ALOGD(LOGTAG_CTRL " Avrcp not connected or AV not connected");
        }
        break;
    case AVRCP_CTRL_REG_NOTI_ABS_VOL_CB:
        iter = FindAvDeviceByAddr(pA2dpSink->pA2dpDeviceList, pEvent->avrcpCtrlEvent.bd_addr);
        if (iter != pA2dpSink->pA2dpDeviceList.end() && (iter->mAvrcpConnected == true))
        {
            ALOGD(LOGTAG_CTRL " NOTI_ABS_VOL_CB for AV & RC connected device, send to stack");
            iter->mNotificationLabel = (int)pEvent->avrcpCtrlEvent.arg1;
            iter->mAbsVolNotificationRequested = true;
            perVol = getVolumePercentage();
            ALOGD(LOGTAG_CTRL " Sending Interim Response = %d label %d", perVol,
                                                      iter->mNotificationLabel);
            sBtAvrcpCtrlInterface->register_abs_vol_rsp(&pEvent->avrcpCtrlEvent.bd_addr,
                    BTRC_NOTIFICATION_TYPE_INTERIM, perVol, iter->mNotificationLabel);
        }
        else
        {
            ALOGD(LOGTAG_CTRL " Avrcp not connected or AV not connected");
        }
        break;
    case AVRCP_CTRL_VOL_CHANGED_NOTI_REQ:
        ALOGD(LOGTAG_CTRL " AVRCP_CTRL_VOL_CHANGED_NOTI_REQ, vol level = %d",
                                                 pEvent->avrcpCtrlEvent.arg1);
        iter = pA2dpSink->pA2dpDeviceList.begin();
        while(iter != pA2dpSink->pA2dpDeviceList.end()) {
            if (iter->mAbsoluteVolumeChangeInProgress)
            {
                iter->mAbsoluteVolumeChangeInProgress = false;
            }
            else
            {
                ALOGD(LOGTAG_CTRL " iter->mAvrcpConnected %d ", iter->mAvrcpConnected);
                ALOGD(LOGTAG_CTRL " iter->mAbsVolNotificationRequested %d",
                                    iter->mAbsVolNotificationRequested);
                if (iter->mAvrcpConnected && iter->mAbsVolNotificationRequested)
                {
                    perVol = (((int)pEvent->avrcpCtrlEvent.arg1*ABS_VOL_BASE)/AUDIO_MAX_VOL_LEVEL);
                    curr_audio_index = (int)pEvent->avrcpCtrlEvent.arg1;
                    ALOGD(LOGTAG_CTRL " perVol %d & mPreviousPercentageVol %d", perVol,
                                                    mPreviousPercentageVol);
                    if (perVol != mPreviousPercentageVol)
                    {
                        sBtAvrcpCtrlInterface->register_abs_vol_rsp(&iter->mDevice,
                                BTRC_NOTIFICATION_TYPE_CHANGED, perVol, iter->mNotificationLabel);
                        iter->mAbsVolNotificationRequested = false;
                    }
                }
                else
                    ALOGD(LOGTAG_CTRL " iter %x !conn to RC or !reg for Abs vol change noti", iter);
            }
            iter++;
        }
        mPreviousPercentageVol = perVol;
        pA2dpSinkStream->SetStreamVol(curr_audio_index);
        break;
    }
}

void Avrcp::HandleEnableAvrcp(void) {
    BtEvent *pEvent = new BtEvent;
    ALOGD(LOGTAG_CTRL " HandleEnableAvrcp ");

    max_avrcp_conn = config_get_int (config,
            CONFIG_DEFAULT_SECTION, "BtMaxA2dpConn", 1);

    if (bluetooth_interface != NULL)
    {
        // AVRCP CT Initialization
        sBtAvrcpCtrlInterface = (btrc_ctrl_interface_t *)bluetooth_interface->
                get_profile_interface(BT_PROFILE_AV_RC_CTRL_ID);

        // AVRCP CT Vendor Initialization
        sBtAvrcpCtrlVendorInterface = (btrc_ctrl_vendor_interface_t *)bluetooth_interface->
                get_profile_interface(BT_PROFILE_AV_RC_CTRL_VENDOR_ID);

        if (sBtAvrcpCtrlInterface == NULL || sBtAvrcpCtrlVendorInterface == NULL)
        {
             pEvent->profile_start_event.event_id = PROFILE_EVENT_START_DONE;
             pEvent->profile_start_event.profile_id = PROFILE_ID_AVRCP;
             pEvent->profile_start_event.status = false;
             PostMessage(THREAD_ID_GAP, pEvent);
             return;
        }

        if (sBtAvrcpCtrlInterface != NULL) {
            sBtAvrcpCtrlInterface->init(&sBluetoothAvrcpCtrlCallbacks);
        }

        if (sBtAvrcpCtrlVendorInterface != NULL) {
            sBtAvrcpCtrlVendorInterface->
                    init_vendor(&sBluetoothAvrcpCtrlVendorCallbacks, max_avrcp_conn);
        }

        pEvent->profile_start_event.event_id = PROFILE_EVENT_START_DONE;
        pEvent->profile_start_event.profile_id = PROFILE_ID_AVRCP;
        pEvent->profile_start_event.status = true;

        PostMessage(THREAD_ID_GAP, pEvent);
    }
}

void Avrcp::HandleDisableAvrcp(void) {
    ALOGD(LOGTAG_CTRL " HandleDisableAvrcp ");

   if (sBtAvrcpCtrlInterface != NULL) {
       sBtAvrcpCtrlInterface->cleanup();
       sBtAvrcpCtrlInterface = NULL;
   }
   if (sBtAvrcpCtrlVendorInterface != NULL) {
       sBtAvrcpCtrlVendorInterface->cleanup_vendor();
       sBtAvrcpCtrlVendorInterface = NULL;
   }
   BtEvent *pEvent = new BtEvent;
   pEvent->profile_stop_event.event_id = PROFILE_EVENT_STOP_DONE;
       pEvent->profile_stop_event.profile_id = PROFILE_ID_AVRCP;
       pEvent->profile_stop_event.status = true;
       PostMessage(THREAD_ID_GAP, pEvent);
}

char* Avrcp::dump_message(BluetoothEventId event_id) {
    switch(event_id) {
    case AVRCP_CTRL_CONNECTED_CB:
        return "AVRCP_CTRL_CONNECTED_CB";
    case AVRCP_CTRL_DISCONNECTED_CB:
        return "AVRCP_CTRL_DISCONNECTED_CB";
    case AVRCP_CTRL_PASS_THRU_CMD_REQ:
        return "PASS_THRU_CMD_REQ";
    }
    return "UNKNOWN";
}

Avrcp :: Avrcp(const bt_interface_t *bt_interface, config_t *config) {
    this->bluetooth_interface = bt_interface;
    this->config = config;
    sBtAvrcpCtrlInterface = NULL;
    max_avrcp_conn = 0;
    memset(&mConnectedAvrcpDevice, 0, sizeof(bt_bdaddr_t));
    pthread_mutex_init(&this->lock, NULL);
    mPreviousPercentageVol = -1;
    mFirstAbsVolCmdRecvd = false;
}

Avrcp :: ~Avrcp() {
    pthread_mutex_destroy(&lock);
    rc_only_devices.clear();
}
