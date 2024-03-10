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
  *
  * Changes from Qualcomm Innovation Center are provided under the following license:
  *
  * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
  * SPDX-License-Identifier: BSD-3-Clause-Clear
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
#include "osi/include/fixed_queue.h"
#include "osi/include/list.h"
#include <mutex>

#include "A2dp_Sink_Streaming.hpp"
#include "Gap.hpp"
#include "hardware/bt_av_vendor.h"
#include "Avrcp.hpp"
#if defined(BT_AUDIO_PAL_INTEGRATION)
#include "pa_routing_interface.h"
#include "A2dp_Sink_Split.hpp"
#endif

#if (defined USE_GST)
#ifdef __cplusplus
extern "C" {
#endif
#include <gst/gstbthelper.h>
#ifdef __cplusplus
}
#endif
#endif

#if (defined USE_GST)

gstbt gstbtobj;

#endif

#define LOGTAG "A2DP_SINK_STREAMING"

using namespace std;
using std::list;
using std::string;

static pthread_mutex_t data_q_lock;


extern A2dp_Sink_Streaming *pA2dpSinkStream;
#if defined(BT_AUDIO_PAL_INTEGRATION)
extern A2dp_Sink_Split *pA2dpSinkSplit;
#endif
extern BT_Audio_Manager *pBTAM;
extern Avrcp *pAvrcp;
extern Gap *g_gap;

typedef struct
{
    uint16_t len;
    uint16_t offset;
} tBT_SINK_DQ_DATA_HDR;

tBT_SINK_DQ_DATA_HDR* audioFragment = NULL;
fixed_queue_t *CompressDataQ = NULL;
uint64_t buffered_length = 0;
#define MAX_COMPRESS_BUF_TSHLD  20480
#define START_COMPRESS_BUF_TSHLD  15360
bool wait_for_mm_callback = false; // are we waiting for callback from mm

//#define DUMP_COMPRESSED_DATA TRUE
#if (defined(DUMP_PCM_DATA) && (DUMP_PCM_DATA == TRUE))
FILE *outputPcmSampleFile;
char outputFilename [50] = "/etc/bluetooth/output_sample.pcm";
#endif

#if (defined(DUMP_COMPRESSED_DATA) && (DUMP_COMPRESSED_DATA == TRUE))
FILE *outputPcmSampleFile;
char outputFilename [50] = "/etc/bluetooth/output_sample.pcm";
#endif

extern void enque_relay_data(uint8_t* buffer, size_t size, uint8_t codec_type);
#ifdef __cplusplus
extern "C" {
#endif

#define BE_STREAM_TO_UINT16(u16, p) {u16 = (uint16_t)(((uint16_t)(*(p)) << 8) + (uint16_t)(*((p) + 1))); (p) += 2;}
#define BE_STREAM_TO_UINT32(u32, p) {u32 = ((uint32_t)(*((p) + 3)) + ((uint32_t)(*((p) + 2)) << 8) +((uint32_t)(*((p) + 1)) << 16) + ((uint32_t)(*(p)) << 24)); (p) += 4;}

#define DEFAULT_SBC_SYSTEM_DELAY    60
#define DEFAULT_MP3_SYSTEM_DELAY    100
#define DEFAULT_APTX_SYSTEM_DELAY   1330
#define DEFAULT_AAC_SYSTEM_DELAY    1530

uint8_t get_rtp_offset(uint8_t* p_start, uint16_t codec_type);
void BtA2dpSinkStreamingMsgHandler(void *msg) {
    BtEvent* pEvent = NULL;
    BtEvent* pCleanupEvent = NULL, *pControlRequest = NULL, *pReleaseControlReq = NULL;
    uint32_t pcm_data_read = 0;
    uint8_t rtp_offset = 0;
    uint32_t timestamp_len = 0;
    if (pA2dpSinkStream) {
        timestamp_len = ((pA2dpSinkStream->enable_notification_cb &&
                         pA2dpSinkStream->enable_timestamp) ? sizeof(uint64_t) : 0);
    }

    if(!msg) {
        printf("Msg is NULL, return.\n");
        return;
    }

    pEvent = ( BtEvent *) msg;
    switch(pEvent->a2dpSinkStreamingEvent.event_id) {
        case A2DP_SINK_STREAMING_API_START:
            ALOGD(LOGTAG " enable a2dp sink streaming");
            if (pA2dpSinkStream) {
                pA2dpSinkStream->HandleEnableSinkStreaming();
            }
            break;
        case A2DP_SINK_STREAMING_API_STOP:
            ALOGD(LOGTAG " disable a2dp sink streaming");
            if (pA2dpSinkStream) {
                pA2dpSinkStream->HandleDisableSinkStreaming();
            }
            break;
        case A2DP_SINK_STREAMING_CLEANUP_REQ:
            ALOGD(LOGTAG " cleanup a2dp sink streaming");
            if (pA2dpSinkStream) {
                if (pA2dpSinkStream->use_bt_a2dp_hal) {
                    pA2dpSinkStream->CloseInputStream();
                }
                pA2dpSinkStream->CloseAudioStream();
                pA2dpSinkStream->StopDataFetchTimer();
            }
            break;
        case A2DP_SINK_STREAMING_OPEN_INPUT_STREAM:
            ALOGD(LOGTAG " A2DP_SINK_STREAMING_OPEN_INPUT_STREAM");
            if(pA2dpSinkStream && pA2dpSinkStream->use_bt_a2dp_hal)
                pA2dpSinkStream->OpenInputStream();
            break;
        case A2DP_SINK_STREAMING_CLOSE_AUDIO_STREAM:
            ALOGD(LOGTAG " A2DP_SINK_STREAMING_CLOSE_AUDIO_STREAM");
            if (pA2dpSinkStream) {
                pA2dpSinkStream->CloseAudioStream();
                pA2dpSinkStream->StopDataFetchTimer();
            }
            // release control
            pReleaseControlReq = new BtEvent;
            pReleaseControlReq->btamControlRelease.event_id = BT_AM_RELEASE_CONTROL;
            pReleaseControlReq->btamControlRelease.profile_id = PROFILE_ID_A2DP_SINK;
            PostMessage(THREAD_ID_BT_AM, pReleaseControlReq);
            break;
        case A2DP_SINK_STREAMING_AM_REQUEST_CONTROL:
            ALOGD(LOGTAG " A2DP_SINK_STREAMING_AM_REQUEST_CONTROL");
            pControlRequest = new BtEvent;
            pControlRequest->btamControlReq.event_id = BT_AM_REQUEST_CONTROL;
            pControlRequest->btamControlReq.profile_id = PROFILE_ID_A2DP_SINK;
            pControlRequest->btamControlReq.request_type = REQUEST_TYPE_PERMANENT;
            PostMessage(THREAD_ID_BT_AM, pControlRequest);
            break;
        case A2DP_SINK_STREAMING_FETCH_PCM_DATA:
            ALOGD(LOGTAG " A2DP_SINK_STREAMING_FETCH_PCM_DATA");
            if (pA2dpSinkStream) {
                if (!pA2dpSinkStream->enable_notification_cb) {
                    if (!pA2dpSinkStream->pcm_timer) {
                        ALOGD(LOGTAG " pcm_timer already false, don't fetch data");
                        break;
                    }
                    pA2dpSinkStream->pcm_timer = false;
                }
            }
#if (defined USE_GST)
            uint8_t * data;
            int size;
            pA2dpSinkStream->StartPcmTimer();
            size = allocate_gst_buffer(&gstbtobj, &data);
            if ((pA2dpSinkStream->mBtA2dpSinkStreamingVendorInterface != NULL) &&
                    (data != NULL)) {
                if(pA2dpSinkStream->use_bt_a2dp_hal) {
                    // read data from BT A2DP HAL
                    pcm_data_read =  pA2dpSinkStream->ReadInputStream(data, size);
                }
                else
                {
                    /* TODO: When callback mechnism is enabled, handle size before invoking
                             following api to fetch data from data queue in stack*/
                    // fetch PCM data from fluoride
                    pcm_data_read =  pA2dpSinkStream->mBtA2dpSinkStreamingVendorInterface->
                    get_a2dp_sink_streaming_data_vendor(A2DP_SINK_AUDIO_CODEC_PCM,data, size);
                }
                ALOGD(LOGTAG " pcm_data_read = %d", pcm_data_read);
            }
            send_gst_data(&gstbtobj, pcm_data_read, 0);
#else
            if ((pA2dpSinkStream->pcm_buf == NULL) || bdaddr_is_empty(&pA2dpSinkStream->mStreamingDevice)) {
                // pcm buffer is null, closeStream or streaming device null have been called earlier
                break;
            }
            // first start next timer
            pA2dpSinkStream->StartPcmTimer();

            if ((pA2dpSinkStream->mBtA2dpSinkStreamingVendorInterface != NULL) &&
                    (pA2dpSinkStream->pcm_buf != NULL)) {
                if(pA2dpSinkStream->use_bt_a2dp_hal) {
                    // read data from BT A2DP HAL
                    pcm_data_read =  pA2dpSinkStream->ReadInputStream(pA2dpSinkStream->pcm_buf,
                            pA2dpSinkStream->pcm_buf_size);
                }
                else
                {
                    // fetch PCM data from fluoride
                    if(pA2dpSinkStream->sbc_decoding)
                    {
                        ALOGD(LOGTAG"sbc_decdoing is true, capture the pcm data");
                        pcm_data_read =  pA2dpSinkStream->mBtA2dpSinkStreamingVendorInterface->
                        get_a2dp_sink_streaming_data_vendor(A2DP_SINK_AUDIO_CODEC_PCM,
                        pA2dpSinkStream->pcm_buf, pA2dpSinkStream->pcm_buf_size);
                    }
                    else
                    {
                        uint32_t pcm_buf_size = 0;
                        if(pA2dpSinkStream->peer_mtu > (pA2dpSinkStream->pcm_buf_size/4)){
                            pcm_buf_size = pA2dpSinkStream->peer_mtu + 50;
                            //50 is to make sure that buf_size is more than pack size in stack
                        } else {
                            pcm_buf_size = pA2dpSinkStream->pcm_buf_size/4;
                        }
                        pcm_data_read =  pA2dpSinkStream->mBtA2dpSinkStreamingVendorInterface->
                        get_a2dp_sink_streaming_data_vendor(A2DP_SINK_AUDIO_CODEC_SBC,
                        pA2dpSinkStream->pcm_buf,
                        (pA2dpSinkStream->enable_notification_cb ? pA2dpSinkStream->pcm_buf_size :
                        pcm_buf_size));
                    }
                    /* when callback mechanism is used, remove timestamp before sending data
                     * to Audio Hal */
                    if (pA2dpSinkStream->enable_notification_cb &&
                            pA2dpSinkStream->enable_timestamp) {
                        if (pcm_data_read <= 0) {
                            ALOGD(LOGTAG" No Data available in Data queue, break");
                            break;
                        }
                        uint64_t tStamp = *((uint64_t *)pA2dpSinkStream->pcm_buf);
                        pcm_data_read -= sizeof(uint64_t); // decrement timestamp data read size
                        // fetch current timestamp and check latency
                        uint64_t cur_time = pA2dpSinkStream->get_cur_time();
                        ALOGD(LOGTAG" media packet timestamp = %llu, latency to receive data = %llu"
                                " micro sec", tStamp, (cur_time - tStamp));
                    }
                }

                if (pA2dpSinkStream->fetch_rtp_info && ( pcm_data_read > 12)) {
                     if(!pA2dpSinkStream->sbc_decoding)
                     {
                         rtp_offset = get_rtp_offset( pA2dpSinkStream->pcm_buf, A2DP_SINK_AUDIO_CODEC_SBC);
                     }
                 }
                ALOGD(LOGTAG " fluoried stored_data_read = %d", pcm_data_read);
             }

#if (defined(DUMP_PCM_DATA) && (DUMP_PCM_DATA == TRUE))
            if ((outputPcmSampleFile) && (pA2dpSinkStream->pcm_buf != NULL))
            {
                fwrite ((void*)pA2dpSinkStream->pcm_buf, 1, (size_t)(pcm_data_read), outputPcmSampleFile);
            }
#endif
#endif
            break;
        case A2DP_SINK_STREAMING_AM_RELEASE_CONTROL:
            ALOGD(LOGTAG " A2DP_SINK_STREAMING_AM_RELEASE_CONTROL");
            // release focus in this case.
            if (pA2dpSinkStream) {
                pA2dpSinkStream->CloseAudioStream();
                pA2dpSinkStream->StopDataFetchTimer();
                if (pA2dpSinkStream->use_bt_a2dp_hal) {
                    pA2dpSinkStream->SuspendInputStream();
                }
                if (pA2dpSinkStream->controlStatus != STATUS_LOSS_TRANSIENT) {
                    ALOGD("A2DP_SINK_STREAMING_AM_RELEASE_CONTROL pA2dpSinkStream->controlStatus %d", pA2dpSinkStream->controlStatus);
                    pReleaseControlReq = new BtEvent;
                    pReleaseControlReq->btamControlRelease.event_id = BT_AM_RELEASE_CONTROL;
                    pReleaseControlReq->btamControlRelease.profile_id = PROFILE_ID_A2DP_SINK;
                    PostMessage(THREAD_ID_BT_AM, pReleaseControlReq);
                }
            }
            break;
        case BT_AM_CONTROL_STATUS:
            ALOGD(LOGTAG " BT_AM_CONTROL_STATUS");
            if (pA2dpSinkStream) {
                ALOGD(LOGTAG " earlier status = %d  new status = %d", pA2dpSinkStream->controlStatus,
                        pEvent->btamControlStatus.status_type);
                pA2dpSinkStream->controlStatus = pEvent->btamControlStatus.status_type;
                switch(pA2dpSinkStream->controlStatus) {
                    case STATUS_LOSS:
                    ALOGD(LOGTAG " BT_AM_CONTROL_STATUS, STATUS_LOSS");
                         // inform bluedroid
                        if (pA2dpSinkStream->mBtA2dpSinkStreamingVendorInterface != NULL) {
                            pA2dpSinkStream->mBtA2dpSinkStreamingVendorInterface->
                            audio_focus_state_vendor(0, &pA2dpSinkStream->mStreamingDevice);
                        }
                        // send pause to remote
                        if (pAvrcp != NULL)
                            pAvrcp->SendPassThruCommandNative(CMD_ID_PAUSE,
                            &pA2dpSinkStream->mStreamingDevice, 0);
                        // release control
                        pReleaseControlReq = new BtEvent;
                        pReleaseControlReq->btamControlRelease.event_id = BT_AM_RELEASE_CONTROL;
                        pReleaseControlReq->btamControlRelease.profile_id = PROFILE_ID_A2DP_SINK;
                        PostMessage(THREAD_ID_BT_AM, pReleaseControlReq);
                        pA2dpSinkStream->CloseAudioStream();
                        pA2dpSinkStream->StopDataFetchTimer();
                        break;
                    case STATUS_LOSS_TRANSIENT:
                    ALOGD(LOGTAG " BT_AM_CONTROL_STATUS, STATUS_LOSS_TRANSIENT");
                        // inform bluedroid
                        if (pA2dpSinkStream->mBtA2dpSinkStreamingVendorInterface != NULL) {
                            pA2dpSinkStream->mBtA2dpSinkStreamingVendorInterface->
                            audio_focus_state_vendor(0, &pA2dpSinkStream->mStreamingDevice);
                        }
                        // send pause to remote
                        if (pAvrcp != NULL) {
                            ALOGD(LOGTAG " copy resuming device");
                            memcpy(&pA2dpSinkStream->mResumingDevice,
                                &pA2dpSinkStream->mStreamingDevice, sizeof(bt_bdaddr_t));
                            ALOGD(LOGTAG " sending pause copy resuming device");
                            pAvrcp->SendPassThruCommandNative(CMD_ID_PAUSE,
                                &pA2dpSinkStream->mStreamingDevice, 0);
                        }
                        pA2dpSinkStream->CloseAudioStream();
                        pA2dpSinkStream->StopDataFetchTimer();
                        // Notify Audio Stream close
                        pReleaseControlReq = new BtEvent;
                        pReleaseControlReq->btamControlRelease.event_id = BT_AM_OUT_CLOSE;
                        pReleaseControlReq->btamControlRelease.profile_id = PROFILE_ID_A2DP_SINK;
                        PostMessage(THREAD_ID_BT_AM, pReleaseControlReq);
                        break;
                    case STATUS_GAIN:
                    ALOGD(LOGTAG " BT_AM_CONTROL_STATUS, STATUS_GAIN");
                        // inform bluedroid
                        if (pA2dpSinkStream->mBtA2dpSinkStreamingVendorInterface != NULL) {
                            pA2dpSinkStream->mBtA2dpSinkStreamingVendorInterface->
                            audio_focus_state_vendor(3, &pA2dpSinkStream->mStreamingDevice);
                        }
                        pA2dpSinkStream->ConfigureAudioHal();
                        if (!pA2dpSinkStream->enable_notification_cb) {
                            if (pA2dpSinkStream->codec_type == A2DP_SINK_AUDIO_CODEC_SBC)
                                pA2dpSinkStream->StartPcmTimer();
                            else {
                                BtEvent *pEvent = new BtEvent;
                                pEvent->a2dpSinkStreamingEvent.event_id =
                                        A2DP_SINK_FILL_COMPRESS_BUFFER;
                                thread_post(pA2dpSinkStream->threadInfo.thread_id,
                                pA2dpSinkStream->threadInfo.thread_handler, (void*)pEvent);
                            }
                        }
                        break;
                    case STATUS_REGAINED:
                        // inform bluedroid
                        ALOGD(LOGTAG " BT_AM_CONTROL_STATUS, STATUS_REGAINED");
                        if (pA2dpSinkStream->mBtA2dpSinkStreamingVendorInterface != NULL) {
                            pA2dpSinkStream->mBtA2dpSinkStreamingVendorInterface->
                            audio_focus_state_vendor(3, &pA2dpSinkStream->mStreamingDevice);
                        }
                        pA2dpSinkStream->ConfigureAudioHal();
                        if (!pA2dpSinkStream->enable_notification_cb) {
                            if (pA2dpSinkStream->codec_type == A2DP_SINK_AUDIO_CODEC_SBC)
                                pA2dpSinkStream->StartPcmTimer();
                            else {
                                BtEvent *pEvent = new BtEvent;
                                pEvent->a2dpSinkStreamingEvent.event_id =
                                        A2DP_SINK_FILL_COMPRESS_BUFFER;
                                    thread_post(pA2dpSinkStream->threadInfo.thread_id,
                                    pA2dpSinkStream->threadInfo.thread_handler, (void*)pEvent);
                            }
                        }
                        // send play to remote
                        if (pAvrcp != NULL && !bdaddr_is_empty(&pA2dpSinkStream->mResumingDevice)) {
                            ALOGD(LOGTAG " STATUS_REGAINED, sending play");
                            pAvrcp->SendPassThruCommandNative(CMD_ID_PLAY,
                                    &pA2dpSinkStream->mResumingDevice, 1);
                            memset(&pA2dpSinkStream->mResumingDevice, 0, sizeof(bt_bdaddr_t));
                        }
                        break;
                }
            }
            break;
        case A2DP_SINK_FILL_COMPRESS_BUFFER:
            ALOGD(LOGTAG " A2DP_SINK_FILL_COMPRESS_BUFFER");
            if (pA2dpSinkStream) {
                pA2dpSinkStream->FillCompressBuffertoAudioOutHal();
            }
            break;
        case A2DP_SINK_STREAMING_DISCONNECTED:
            ALOGD(LOGTAG " A2DP_SINK_STREAMING_DISCONNECTED");
            if (pA2dpSinkStream) {
                pA2dpSinkStream->StopDataFetchTimer();
                if (pA2dpSinkStream->use_bt_a2dp_hal) {
                    pA2dpSinkStream->CloseInputStream();
                }
                pA2dpSinkStream->CloseAudioStream();
            }
            break;
        case A2DP_SINK_STREAMING_FLUSH_AUDIO:
            ALOGD(LOGTAG " A2DP_SINK_STREAMING_FLUSH_AUDIO");
            if (pA2dpSinkStream) {
                if (pA2dpSinkStream->mBtA2dpSinkStreamingVendorInterface != NULL)
                {
                    pA2dpSinkStream->mBtA2dpSinkStreamingVendorInterface->
                        update_flushing_device_vendor(&pEvent->a2dpSinkStreamingEvent.bd_addr);
                }
            }
            break;
       case A2DP_SINK_SEND_TO_OUT_WRITE:
            ALOGD(LOGTAG " A2DP_SINK_SEND_TO_OUT_WRITE %d compress_timer_stoped %d ",
                wait_for_mm_callback, pA2dpSinkStream->compress_timer_stoped);
            /* wait_for_mm_callback == false, this must have been triggerd from packet que logic
             * wait_for_mm_callback == true, this must have been triggered from mm-callback
             * wait_for_mm_callback : this variable should be changed here only*/
            if (pA2dpSinkStream == NULL) {
                wait_for_mm_callback = false;
                break;
            }
            /* Verify compress_timer_stoped state, when callback mechanism is disabled */
            if(pA2dpSinkStream->compress_timer_stoped && !pA2dpSinkStream->enable_notification_cb) {
                /* if timer is not scheduled, then pause/suspend might have been triggered
                 *  we are no longer waiting or callback, and lets bail out */
                wait_for_mm_callback = false;
                break;
            }
            /* lets write to audio hal and wait for callback */
            if (pA2dpSinkStream)
                pA2dpSinkStream->send_to_out_write();

            break;
        default:
            break;
    }
    delete pEvent;
}

#ifdef __cplusplus
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

uint64_t A2dp_Sink_Streaming::get_cur_time() {
    struct timespec ts_now;
    memset(&ts_now, 0, sizeof(ts_now));
    clock_gettime(CLOCK_REALTIME, &ts_now);
    // convert current time in micro second
    uint64_t cur_ts = (uint64_t)ts_now.tv_sec * 1000000 + ts_now.tv_nsec/1000;
    return cur_ts;
}

void start_compress_offload() {
    ALOGV(LOGTAG "%s  wait_for_mm_callback %d ", __FUNCTION__, wait_for_mm_callback);

    BtEvent *pEvent = new BtEvent;
    pEvent->a2dpSinkStreamingEvent.event_id = A2DP_SINK_SEND_TO_OUT_WRITE;
    if (pA2dpSinkStream) {
        pA2dpSinkStream->out_write_ts = pA2dpSinkStream->get_cur_time(); // update during first time trigger
        thread_post(pA2dpSinkStream->threadInfo.thread_id,
        pA2dpSinkStream->threadInfo.thread_handler, (void*)pEvent);
    }
}


void A2dp_Sink_Streaming::send_to_out_write() {

}

void A2dp_Sink_Streaming::FillCompressBuffertoAudioOutHal() {

}


void compress_audio_feed_handler(void *context) {
    ALOGV(LOGTAG " compress_audio_feed_handler ");

    pA2dpSinkStream->compress_offload_timer = false;
    BtEvent *pEvent = new BtEvent;
    pEvent->a2dpSinkStreamingEvent.event_id = A2DP_SINK_FILL_COMPRESS_BUFFER;
    if (pA2dpSinkStream) {
        thread_post(pA2dpSinkStream->threadInfo.thread_id,
        pA2dpSinkStream->threadInfo.thread_handler, (void*)pEvent);
    }
}

void A2dp_Sink_Streaming::StartCompressAudioFeedTimer() {
    if (pA2dpSinkStream->enable_notification_cb) {
        ALOGD(LOGTAG " Compress Audio feed timer is disabled in Streaming with Callback"
                " Mechanism, return");
        return;
    }
    if(compress_offload_timer) {
        ALOGV(LOGTAG " compressed timer already running, return ");
        return;
    }
    compress_offload_timer = true;
    compress_timer_stoped = false;
    alarm_set(compress_audio_feed_timer, A2DP_SINK_COMPRESS_FEED_TIMER_DURATION,
            compress_audio_feed_handler, NULL);
}

void A2dp_Sink_Streaming::StopCompressAudioFeedTimer() {
    if (pA2dpSinkStream->enable_notification_cb) {
        ALOGD(LOGTAG " Compress Audio feed timer is disabled in Streaming with"
                " callback mechanism, return");
        return;
    }
    if((compress_audio_feed_timer != NULL) && (compress_offload_timer)) {
        alarm_cancel(compress_audio_feed_timer);
        compress_offload_timer = false;
        compress_timer_stoped = true;
    }
}

void pcm_fetch_timer_handler(void *context) {
    ALOGV(LOGTAG " pcm_fetch_timer_handler ");

    BtEvent *pEvent = new BtEvent;
    pEvent->a2dpSinkStreamingEvent.event_id = A2DP_SINK_STREAMING_FETCH_PCM_DATA;
    if (pA2dpSinkStream) {
        thread_post(pA2dpSinkStream->threadInfo.thread_id,
        pA2dpSinkStream->threadInfo.thread_handler, (void*)pEvent);
    }
}

void A2dp_Sink_Streaming::StartPcmTimer() {
    if (pA2dpSinkStream->enable_notification_cb) {
        ALOGD(LOGTAG " Pcm Timer is disabled in Streaming with Callback Mechanism, return");
        return;
    }

    if(pcm_timer) {
        ALOGD(LOGTAG " PCM Timer still running + ");
        return;
    }
    ALOGD(LOGTAG " StartingTimer for %d ",pcm_timer_duration);
    alarm_set(pcm_data_fetch_timer, pcm_timer_duration,
           pcm_fetch_timer_handler, NULL);
    pcm_timer = true;
}

void A2dp_Sink_Streaming::StopDataFetchTimer() {
    ALOGD(LOGTAG " StopDataFetchTimer ");
    if (pA2dpSinkStream->enable_notification_cb) {
        ALOGD(LOGTAG " Pcm Timer is disabled in Streaming with Callback Mechanism, return");
        return;
    }
    if((codec_type == A2DP_SINK_AUDIO_CODEC_SBC) && (pcm_data_fetch_timer != NULL) && (pcm_timer)) {
        alarm_cancel(pcm_data_fetch_timer);
        pcm_timer = false;
    } else {
        StopCompressAudioFeedTimer();
    }
}

void remote_suspend_wait_timer_handler(void *context) {
    ALOGD(LOGTAG " remote_suspend_wait_timer_handler ");
    bt_bdaddr_t bd_addr;
    pA2dpSinkStream->suspend_wait_timer = false;
    memcpy(&bd_addr, (bt_bdaddr_t *)context, sizeof(bt_bdaddr_t));
    if (!memcmp(&pA2dpSinkStream->mStreamingDevice, &bd_addr, sizeof(bt_bdaddr_t))) {
        ALOGD(LOGTAG " remote_suspend_wait_timer_handler pA2dpSinkStream->StartPcmTimer()");
        if (pA2dpSinkStream->codec_type == A2DP_SINK_AUDIO_CODEC_SBC) {
            pA2dpSinkStream->StartPcmTimer();
        }
        else {
            pA2dpSinkStream->FillCompressBuffertoAudioOutHal();
        }
    }
}

void A2dp_Sink_Streaming::StartRemoteSuspendWaitTimer() {
    char str[18];
    bdaddr_to_string(&pA2dpSinkStream->mStreamingDevice, str, 18);
    ALOGD("StartRemoteSuspendWaitTimer %s ", str);
    if(suspend_wait_timer) {
        ALOGD(LOGTAG " Remote Suspend Wait Timer still running + ");
        return;
    }
    alarm_set(remote_suspend_wait_timer, A2DP_SINK_REMOTE_SUSPEND_WAIT_TIMER_DURATION,
           remote_suspend_wait_timer_handler, &mStreamingDevice);
    suspend_wait_timer = true;
}

void A2dp_Sink_Streaming::StopRemoteSuspendWaitTimer() {
    ALOGD(LOGTAG " StopRemoteSuspendWaitTimer ");
    if((remote_suspend_wait_timer != NULL) && (suspend_wait_timer)) {
        ALOGD(LOGTAG " Cancelling remote_suspend_wait_timer");
        alarm_cancel(remote_suspend_wait_timer);
        suspend_wait_timer = false;
    }
}

void A2dp_Sink_Streaming::HandleEnableSinkStreaming(void) {
    ALOGD(LOGTAG " HandleEnableSinkStreaming");

    pcm_data_fetch_timer = alarm_new();
    remote_suspend_wait_timer = alarm_new();
    compress_audio_feed_timer = alarm_new();

    use_bt_a2dp_hal = config_get_bool (config,
            CONFIG_DEFAULT_SECTION, "BtUseA2dpHalForSink", false);
    ALOGD(LOGTAG " Use BT A2DP HAL ENabled %d", use_bt_a2dp_hal);
    if(use_bt_a2dp_hal) {
        LoadBtA2dpHAL();
    }
    relay_sink_data = config_get_bool (config,
            CONFIG_DEFAULT_SECTION, "BtRelaySinkDatatoSrc", false);
    ALOGD(LOGTAG " Sink Relay ENabled %d", relay_sink_data);

    audio_out_device = (uint32_t)config_get_int(config,
            CONFIG_DEFAULT_SECTION, "AudioOutDevice", 131072);
    ALOGD(LOGTAG "Audio Out Device %d", audio_out_device);
}

void A2dp_Sink_Streaming::HandleDisableSinkStreaming(void) {
   BtEvent *pEvent = new BtEvent;
   ALOGD(LOGTAG " HandleDisableSinkStreaming");

   CloseAudioStream();
   StopDataFetchTimer();
   if(use_bt_a2dp_hal) {
       UnLoadBtA2dpHAL();
   }

   alarm_free(pcm_data_fetch_timer);
   alarm_free(remote_suspend_wait_timer);
   alarm_free(compress_audio_feed_timer);
   pcm_data_fetch_timer = NULL;
   remote_suspend_wait_timer = NULL;
   compress_audio_feed_timer = NULL;

   ALOGD(LOGTAG " set the mStreamingDevice to zero");
   memset(&mStreamingDevice, 0, sizeof(bt_bdaddr_t));
   pEvent->a2dpSinkEvent.event_id = A2DP_SINK_STREAMING_DISABLE_DONE;
   PostMessage(THREAD_ID_A2DP_SINK, pEvent);
}

char* A2dp_Sink_Streaming::dump_message(BluetoothEventId event_id) {
    switch(event_id) {
    case A2DP_SINK_FOCUS_REQUEST_CB:
        return "FOCUS_REQUEST_CB";
    case BT_AM_CONTROL_STATUS:
        return "AM_CONTROL_STATUS";
    case A2DP_SINK_FETCH_PCM_DATA:
        return "A2DP_SINK_FETCH_PCM_DATA";
    case A2DP_SINK_FILL_COMPRESS_BUFFER:
        return "FILL_COMPRESS_BUFFER";
    }
    return "UNKNOWN";
}

uint32_t A2dp_Sink_Streaming::get_a2dp_sbc_sampling_rate(uint8_t frequency) {
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

uint8_t A2dp_Sink_Streaming::get_a2dp_sbc_channel_mode(uint8_t channeltype) {
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

uint32_t A2dp_Sink_Streaming::get_a2dp_aac_sampling_rate(uint16_t frequency) {
    uint32_t freq = 0;
    switch (frequency) {
        case AAC_SAMP_FREQ_8000:
            freq = 8000;
            break;
        case AAC_SAMP_FREQ_11025:
            freq = 11025;
            break;
        case AAC_SAMP_FREQ_12000:
            freq = 12000;
            break;
        case AAC_SAMP_FREQ_16000:
            freq = 16000;
            break;
        case AAC_SAMP_FREQ_22050:
            freq = 22050;
            break;
        case AAC_SAMP_FREQ_24000:
            freq = 24000;
            break;
        case AAC_SAMP_FREQ_32000:
            freq = 32000;
            break;
        case AAC_SAMP_FREQ_44100:
            freq = 44100;
            break;
        case AAC_SAMP_FREQ_48000:
            freq = 48000;
            break;
        case AAC_SAMP_FREQ_64000:
            freq = 64000;
            break;
        case AAC_SAMP_FREQ_88200:
            freq = 88200;
            break;
        case AAC_SAMP_FREQ_96000:
            freq = 96000;
            break;
    }
    return freq;
}

uint8_t A2dp_Sink_Streaming::get_a2dp_aac_channel_mode(uint8_t channel_count) {
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

uint32_t A2dp_Sink_Streaming::get_a2dp_mp3_sampling_rate(uint16_t frequency) {
    uint32_t freq = 0;
    switch (frequency) {
        case MP3_SAMP_FREQ_16000:
            freq = 16000;
            break;
        case MP3_SAMP_FREQ_22050:
            freq = 22050;
            break;
        case MP3_SAMP_FREQ_24000:
            freq = 24000;
            break;
        case MP3_SAMP_FREQ_32000:
            freq = 32000;
            break;
        case MP3_SAMP_FREQ_44100:
            freq = 44100;
            break;
        case MP3_SAMP_FREQ_48000:
            freq = 48000;
            break;
    }
    return freq;
}

uint8_t A2dp_Sink_Streaming::get_a2dp_mp3_channel_mode(uint8_t channel_count) {
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

uint32_t A2dp_Sink_Streaming::get_a2dp_aptx_sampling_rate(uint8_t frequency) {
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

uint8_t A2dp_Sink_Streaming::get_a2dp_aptx_channel_mode(uint8_t channel_count) {
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

void A2dp_Sink_Streaming::ConfigureAudioHal() {

}

void A2dp_Sink_Streaming::CloseAudioStream() {

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
    /* free streaming buffers */
    if (pcm_buf != NULL) {
        osi_free(pcm_buf);
        pcm_buf = NULL;
    }
    if (audioFragment != NULL) {
        osi_free(audioFragment);
        audioFragment = NULL;
    }
    if (CompressDataQ != NULL) {
        tBT_SINK_DQ_DATA_HDR *p_data_q_buf;
        pthread_mutex_lock(&data_q_lock);
        while(!fixed_queue_is_empty(CompressDataQ)) {
            p_data_q_buf = (tBT_SINK_DQ_DATA_HDR *)fixed_queue_try_dequeue(CompressDataQ);
            osi_free(p_data_q_buf);
        }
        fixed_queue_free(CompressDataQ,NULL);
        CompressDataQ = NULL;
        buffered_length = 0;
        pthread_mutex_unlock(&data_q_lock);
    }
}
void A2dp_Sink_Streaming::LoadBtA2dpHAL() {

}

void A2dp_Sink_Streaming::UnLoadBtA2dpHAL() {

}

void A2dp_Sink_Streaming::OpenInputStream()
{

}

void A2dp_Sink_Streaming::CloseInputStream()
{

}

void A2dp_Sink_Streaming::SuspendInputStream()
{

}

void A2dp_Sink_Streaming::SetStreamVol(int curr_audio_index)
{
#if (defined(BT_AUDIO_PAL_INTEGRATION))
    int ret = 0;
    char vol_cmd[25];
    char *vol_str = NULL;

    if (pA2dpSinkSplit->pa_routing_intf) {
        vol_str = "btsink_volume=";
        snprintf(vol_cmd, strlen(vol_str) + 3, "%s%d", vol_str, curr_audio_index);
        ret = pA2dpSinkSplit->pa_routing_intf->pa_bt_set_param_fn(PA_BT_A2DP_SINK, vol_cmd);
        if (ret) {
            fprintf(stdout, "%s set failed\n", vol_cmd);
            ALOGD(LOGTAG "%s set failed", vol_cmd);
        } else {
            fprintf(stdout, "%s set successfully\n", vol_cmd);
            ALOGD(LOGTAG "%s set successfully", vol_cmd);
        }
    }
    else {
        ALOGD(LOGTAG "pa_routing_intf not ready for set_volume command");
    }
#endif
}

uint32_t A2dp_Sink_Streaming::ReadInputStream(uint8_t* data, uint32_t size)
{

}

uint32_t A2dp_Sink_Streaming::GetInputStreamBufferSize()
{

}

void A2dp_Sink_Streaming::OnDisconnected() {
    ALOGD(LOGTAG " onDisconnected ");
    StopDataFetchTimer();
    if (use_bt_a2dp_hal) {
        CloseInputStream();
    }
    CloseAudioStream();
    pcm_buf_size = 0;
    pcm_timer_duration = 0;
    cuml_data_written_to_audio = 0;
    residual_compress_data = 0;
    codec_type = A2DP_SINK_AUDIO_CODEC_SBC;
    memset(&codec_config, 0, sizeof(btav_codec_config_t));
}

void A2dp_Sink_Streaming::GetLibInterface(const btav_sink_vendor_interface_t *sBtA2dpSinkStrVendorInterface) {
    ALOGD(LOGTAG " GetLibInterface ");
    mBtA2dpSinkStreamingVendorInterface = sBtA2dpSinkStrVendorInterface;
}

A2dp_Sink_Streaming :: A2dp_Sink_Streaming( config_t *config) {
    this->config = config;
    controlStatus = STATUS_LOSS;
    use_bt_a2dp_hal = false;
    //sbc_decoding = true;
    channel_count = 0;
    sample_rate = 0;
    threadInfo.thread_handler = &BtA2dpSinkStreamingMsgHandler;
    threadInfo.thread_name = "A2dp_Sink_Streaming_Thread";
    mBtA2dpSinkStreamingVendorInterface = NULL;
    memset(&mStreamingDevice, 0, sizeof(bt_bdaddr_t));
    memset(&mResumingDevice, 0, sizeof(bt_bdaddr_t));
    pthread_mutex_init(&this->lock, NULL);
    pcm_buf = NULL;
    pcm_timer = false;
    compress_offload_timer = false;
    compress_timer_stoped = true;
    residual_compress_data = 0;
    codec_type = A2DP_SINK_AUDIO_CODEC_SBC;//by default make it SBC
    memset(&codec_config, 0, sizeof(btav_codec_config_t));
    pthread_mutex_init(&data_q_lock, NULL);
#if (defined(DUMP_PCM_DATA) && (DUMP_PCM_DATA == TRUE))
    outputPcmSampleFile =  NULL;
#endif
#if (defined(DUMP_COMPRESSED_DATA) && (DUMP_COMPRESSED_DATA == TRUE))
    outputPcmSampleFile =  NULL;
#endif


}

A2dp_Sink_Streaming :: ~A2dp_Sink_Streaming() {
    pthread_mutex_destroy(&lock);
    use_bt_a2dp_hal = false;
    controlStatus = STATUS_LOSS;
    out_write_ts = 0;
    threadInfo.thread_handler = &BtA2dpSinkStreamingMsgHandler;
    threadInfo.thread_name = "A2dp_Sink_Streaming_Thread";
    memset(&mStreamingDevice, 0, sizeof(bt_bdaddr_t));
    memset(&mResumingDevice, 0, sizeof(bt_bdaddr_t));
    mBtA2dpSinkStreamingVendorInterface = NULL;
    if (pcm_buf != NULL) {
        osi_free(pcm_buf);
        pcm_buf = NULL;
    }
    if (audioFragment != NULL) {
        osi_free(audioFragment);
        audioFragment = NULL;
    }

    if (CompressDataQ != NULL) {
        fixed_queue_free(CompressDataQ,NULL);
        CompressDataQ = NULL;
    }
    pthread_mutex_destroy(&data_q_lock);
    codec_type = A2DP_SINK_AUDIO_CODEC_SBC;//by default make it SBC
    memset(&codec_config, 0, sizeof(btav_codec_config_t));
}
