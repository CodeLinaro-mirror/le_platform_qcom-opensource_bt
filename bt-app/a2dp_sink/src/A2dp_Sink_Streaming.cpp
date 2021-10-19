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
char outputFilename [50] = "/data/misc/bluetooth/output_sample.pcm";
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
#if (defined(BT_AUDIO_HAL_INTEGRATION))
    qahw_out_buffer_t out_buf;
#endif
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
#if (defined(BT_AUDIO_HAL_INTEGRATION))
            if ((pBTAM->GetAudioDevice() != NULL) && (pA2dpSinkStream->out_stream != NULL) &&
                    (pcm_data_read)) {
                out_buf.buffer = pA2dpSinkStream->pcm_buf + timestamp_len;
                out_buf.bytes = pcm_data_read;
                if (pA2dpSinkStream->relay_sink_data && !pA2dpSinkStream->enable_notification_cb) {
                    if(!pA2dpSinkStream->sbc_decoding)
                    {
                        ALOGD(LOGTAG " total frames = %d", *(pA2dpSinkStream->pcm_buf));
                        if(!pA2dpSinkStream->fetch_rtp_info)
                            enque_relay_data(pA2dpSinkStream->pcm_buf+1,
                                             pcm_data_read-1,
                                             A2DP_SINK_AUDIO_CODEC_SBC);//using sbc
                        else
                            enque_relay_data(pA2dpSinkStream->pcm_buf,
                                             pcm_data_read,
                                             A2DP_SINK_AUDIO_CODEC_SBC);//using sbc
                    }
                    else
                    {
                        enque_relay_data(pA2dpSinkStream->pcm_buf, pcm_data_read, A2DP_SINK_AUDIO_CODEC_PCM);
                    }
                }
                if(pA2dpSinkStream->sbc_decoding)
                    qahw_out_write(pA2dpSinkStream->out_stream, &out_buf);
            }
#endif
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
#if (defined(BT_AUDIO_HAL_INTEGRATION))
                qahw_out_flush(pA2dpSinkStream->out_stream);
                qahw_out_pause(pA2dpSinkStream->out_stream);
#endif
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

#if (defined BT_AUDIO_HAL_INTEGRATION)
void parse_aptx_dec_bd_addr(char *value, struct qahw_aptx_dec_param *aptx_cfg)
{
    int ba[6];
    char *str, *tok;
    uint32_t addr[3];
    int i = 0;

    tok = strtok_r(value, ":", &str);
    while (tok != NULL) {
        ba[i] = strtol(tok, NULL, 16);
        i++;
        tok = strtok_r(NULL, ":", &str);
    }
    addr[0] = (ba[0] << 8) | ba[1];
    addr[1] = ba[2];
    addr[2] = (ba[3] << 16) | (ba[4] << 8) | ba[5];

    aptx_cfg->bt_addr.nap = addr[0];
    aptx_cfg->bt_addr.uap = addr[1];
    aptx_cfg->bt_addr.lap = addr[2];
}


int compressed_callback(qahw_stream_callback_event_t event, void *param,
                  void *cookie) {
    BtEvent *pEvent = new BtEvent;
    switch (event) {
    case QAHW_STREAM_CBK_EVENT_WRITE_READY:
        ALOGD(LOGTAG " EVENT_WRITE_READY %d ", (pA2dpSinkStream->get_cur_time() - pA2dpSinkStream->out_write_ts));
        pA2dpSinkStream->out_write_ts = pA2dpSinkStream->get_cur_time(); // update tstamp.
        pEvent->a2dpSinkStreamingEvent.event_id = A2DP_SINK_SEND_TO_OUT_WRITE;
        if (pA2dpSinkStream) {
            thread_post(pA2dpSinkStream->threadInfo.thread_id,
            pA2dpSinkStream->threadInfo.thread_handler, (void*)pEvent);
        }
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
#if (defined(BT_AUDIO_HAL_INTEGRATION))
    tBT_SINK_DQ_DATA_HDR *p_data_q_buf;
    uint16_t q_bytes_left = 0; // data left in topmost element in Q
    uint32_t audio_frag_bytes_left = 0;// bytes left to compete audioFragment
    qahw_out_buffer_t out_buf;
    uint32_t data_sent_to_audio = 0;
    uint32_t timestamp_len = ((pA2dpSinkStream->enable_notification_cb &&
                          pA2dpSinkStream->enable_timestamp) ? sizeof(uint64_t) : 0);
    uint8_t* p_src; uint8_t* p_dest;

    if ((pBTAM->GetAudioDevice() == NULL) || (out_stream == NULL)) {
        ALOGI(LOGTAG " %s Audio HAL not ready", __FUNCTION__);
        return;
    }
    ALOGI(LOGTAG " %s  Initial  audio_frag_len = %d pcm_buf_size = %d offset = %d Q Len = %d, buffered_len = %d",
            __FUNCTION__, audioFragment->len, pcm_buf_size, audioFragment->offset,
            fixed_queue_length(CompressDataQ), buffered_length);
        pthread_mutex_lock(&data_q_lock);
    do {
        /* first loop is to write to audio hal, until we hit buffer overflow condition */
        ALOGI(LOGTAG " %s audio_frag_len = %d pcm_buf_size = %d offset = %d Q Len = %d, buffered_len = %d",
               __FUNCTION__, audioFragment->len, pcm_buf_size, audioFragment->offset,
               fixed_queue_length(CompressDataQ), buffered_length);
        audio_frag_bytes_left = pcm_buf_size - audioFragment->len;
        while ((audio_frag_bytes_left > 0) && (!fixed_queue_is_empty(CompressDataQ))) {
            /* audioFragment buffer is not full yet, read data from Q */
            /* lets peek into first packet, it has packet from front of Q */
            p_data_q_buf = (tBT_SINK_DQ_DATA_HDR *)fixed_queue_try_peek(CompressDataQ);
            if(p_data_q_buf == NULL) {
                break;
            }
            q_bytes_left = p_data_q_buf->len ;
            ALOGI(LOGTAG " %s q_bytes_left = %d audio_frag_bytes_left = %d", __FUNCTION__,
                                                    q_bytes_left, audio_frag_bytes_left);
            if (q_bytes_left <= audio_frag_bytes_left) {
                /* deque whole packet and write to audioFragment buffer */
                p_data_q_buf = (tBT_SINK_DQ_DATA_HDR *)fixed_queue_try_dequeue(CompressDataQ);
                if(p_data_q_buf == NULL) {
                    break;
                }
                p_src = (uint8_t*)(p_data_q_buf + 1) + p_data_q_buf->offset;
                p_dest = (uint8_t*)(audioFragment + 1) + audioFragment->len;
                memcpy(p_dest, p_src, q_bytes_left);
                audioFragment->len += q_bytes_left;
                osi_free(p_data_q_buf);
                buffered_length -= q_bytes_left;
          }
          else {
                /* read only audio_frag_bytes_left from Q and don't dequeu packet */
                p_src = (uint8_t*)(p_data_q_buf + 1) + p_data_q_buf->offset;
                p_dest = (uint8_t*)(audioFragment + 1) + audioFragment->len;
                memcpy(p_dest, p_src, audio_frag_bytes_left);
                audioFragment->len += audio_frag_bytes_left;
                p_data_q_buf->offset += audio_frag_bytes_left;
                p_data_q_buf->len -= audio_frag_bytes_left;
                buffered_length -= audio_frag_bytes_left;
            }
            audio_frag_bytes_left = pcm_buf_size - audioFragment->len;
        }
        /* copied data from Q */
        ALOGI(LOGTAG " %s audio_frag_len = %d offset = %d  buffered_length = %d", __FUNCTION__,
                                    audioFragment->len, audioFragment->offset, buffered_length);
        if (audio_frag_bytes_left > 0) {
            ALOGE(LOGTAG " %s not enough data in Q", __FUNCTION__);
            /*lets call this flow, once data is received in Q */
            wait_for_mm_callback = false; // make this variable flase and bail out
            break;
         }
        else {
            /* fragment is full, lets write to audio hal */
            out_buf.buffer = (audioFragment + 1) + audioFragment->offset;
            out_buf.bytes = audioFragment->len;
            data_sent_to_audio = qahw_out_write(out_stream, &out_buf);
            ALOGI(LOGTAG " %s data_sent_to_audio = %d frag_left = %d time_delta = %d", __FUNCTION__,
                              data_sent_to_audio, (audioFragment->len - data_sent_to_audio),
                              (get_cur_time() -  out_write_ts));

#if (defined(DUMP_COMPRESSED_DATA) && (DUMP_COMPRESSED_DATA == TRUE))
            if((outputPcmSampleFile) && (data_sent_to_audio > 0)) {
                fwrite ((void*)out_buf.buffer, 1, (size_t)(out_buf.bytes), outputPcmSampleFile);
            }
#endif
            out_write_ts = get_cur_time(); // update tstamp every write.
            if (data_sent_to_audio == audioFragment->len) { // MM consumed whole packet
                /* Audio HAL consumed all data
                 * reset audioFragment len an offset and try next write */
                audioFragment->len = 0;
                audioFragment->offset = 0;
                wait_for_mm_callback = false;
            }
            else {
                if (data_sent_to_audio == 0) {
                    /* MM did not consume any data, audioFragment is intact
                     * No need to left shift data */
                    wait_for_mm_callback = true;
                }
                if (data_sent_to_audio > audioFragment->len) {
                    /* this is an erroneous case, MM was not able to consume even single packet
                     * Sometimes we are seeing an error -14 from MM
                     * Lets retry in this case.*/
                    wait_for_mm_callback = false;
                }
                /* in this case we should copy remaining data */
                if(data_sent_to_audio < audioFragment->len) {
                    /* hit audio full condition, lets adjust offset and wait for next callback */
                    audioFragment->len = audioFragment->len - data_sent_to_audio;
                    audioFragment->offset += data_sent_to_audio;
                    p_src  = (uint8_t*)(audioFragment + 1) + audioFragment->offset;
                    p_dest = (uint8_t*)(audioFragment + 1);
                    // lets left shift data
                    memcpy(p_dest, p_src, audioFragment->len);
                    // update offset after left shifting
                    audioFragment->offset = 0;
                    wait_for_mm_callback = true;
                }
                break;
            }
        }
    }while (1);
    pthread_mutex_unlock(&data_q_lock);
#endif
}

void A2dp_Sink_Streaming::FillCompressBuffertoAudioOutHal() {
#if (defined(BT_AUDIO_HAL_INTEGRATION))
    uint32_t data_read_from_bt = 0;
    uint8_t rtp_offset = 0;
    uint64_t timestamp;
    tBT_SINK_DQ_DATA_HDR *p_msg;
    uint8_t *p_dest, *p_src;
    uint32_t timestamp_len = ((pA2dpSinkStream->enable_notification_cb &&
                              pA2dpSinkStream->enable_timestamp) ? sizeof(uint64_t) : 0);
#if (!defined (USE_GST))
    if (pcm_buf == NULL) {
       // pcm buffer is null, closeStream have been called earlier
       ALOGE(LOGTAG " FillCOmpressBUffer, pcm buf null, bail out");
       return;
	}
#endif
    do {
#if (defined USE_GST)
        if (mBtA2dpSinkStreamingVendorInterface != NULL) {

            if( residual_compress_data == 0) {
            int size = 0;
            uint8_t * tempbuf;
            /* TODO: When callback mechnism is enabled, handle size before invoking
                     following api to fetch data from data queue. */
            data_read_from_bt =  mBtA2dpSinkStreamingVendorInterface->
                 get_a2dp_sink_streaming_data_vendor(codec_type, gbuff, A2DP_SINK_GBUF_MAX_SIZE);
            gstbtobj.blocksize = data_read_from_bt;
            allocate_gst_buffer(&gstbtobj, &tempbuf);
            memcpy(tempbuf,gbuff,data_read_from_bt);

            if (fetch_rtp_info && (data_read_from_bt > 12)) {
                rtp_offset = get_rtp_offset(tempbuf, codec_type);
                data_read_from_bt = data_read_from_bt - rtp_offset;
                }
            }
            else {
                data_read_from_bt =  residual_compress_data;
            }
        }
#else
        if ((mBtA2dpSinkStreamingVendorInterface != NULL) && ( pcm_buf != NULL)) {
             // fetch compressed data from fluoride, if we have not reached THSLD value.
            if( buffered_length < MAX_COMPRESS_BUF_TSHLD) {
                data_read_from_bt =  mBtA2dpSinkStreamingVendorInterface->
                 get_a2dp_sink_streaming_data_vendor(codec_type, pcm_buf, pcm_buf_size);
                // if callback mechanism is used, remove timestamp before sending data to Audio Hal
                if (pA2dpSinkStream->enable_notification_cb && pA2dpSinkStream->enable_timestamp) {
                    if (data_read_from_bt <= 0) {
                        ALOGD(LOGTAG " %s No Data available in Data queue, break", __FUNCTION__);
                        StartCompressAudioFeedTimer();
                        break;
                    }
                    uint64_t tStamp = *((uint64_t *)pcm_buf);
                    data_read_from_bt -= sizeof(uint64_t); // timestamp data read
                    // fetch current timestamp and check latency
                    uint64_t cur_time = get_cur_time();
                    ALOGD(LOGTAG " media packet timestamp = %llu, latency to receive data = %llu"
                            " micro sec", tStamp, (cur_time - tStamp));
                }
                if (fetch_rtp_info && (data_read_from_bt > 12)) {
                    rtp_offset = get_rtp_offset((pcm_buf + timestamp_len), codec_type);
                    data_read_from_bt = data_read_from_bt - rtp_offset;
                }
            }
           else {
                /* Threshold value reached, start timer again and bail out */
                ALOGD(LOGTAG " THSLD value reached  buffered_length  = %d wait_for_mm_callback %d ",
                                                    buffered_length, wait_for_mm_callback);
                if (!wait_for_mm_callback) {
                    /* compress_offload_timer is active, thsld reached, but callback will not come
                     *  nobody is going to deque from Q, trigger out write from here */
                     start_compress_offload();
                }
                StartCompressAudioFeedTimer();
                break;
            }
        }
#endif
        /* we haven't reached threshold, but there is no data in stack */
        if (data_read_from_bt <= 0) {
           /* in this case, we don't have data from bt,
            *and we don't have enough data in Q.  We try after some time */
           ALOGD(LOGTAG " %s NO Data from BT , try after data is queued in Stack", __FUNCTION__);
           StartCompressAudioFeedTimer();
           break;
        }
#if (defined USE_GST)
        if (pBTAM->GetAudioDevice() != NULL) {
             if (fetch_rtp_info)
                 send_gst_data(&gstbtobj, data_read_from_bt, rtp_offset);
             else
                 send_gst_data(&gstbtobj, data_read_from_bt, 0);
             data_sent_to_audio = data_read_from_bt;
             cuml_data_written_to_audio = cuml_data_written_to_audio + data_sent_to_audio;

        }
#else
        // if callback mechanism is enabled, relay mechanism will be disabled
        if (pA2dpSinkStream->relay_sink_data && !pA2dpSinkStream->enable_notification_cb) {
            ALOGD(LOGTAG " %s Enquee the data codec type = %d size = %d ",__FUNCTION__, codec_type ,data_read_from_bt);
            enque_relay_data(pcm_buf,data_read_from_bt, codec_type);
        }

        if ((p_msg = (tBT_SINK_DQ_DATA_HDR *) osi_malloc(sizeof(tBT_SINK_DQ_DATA_HDR) + pcm_buf_size)) != NULL) {
            pthread_mutex_lock(&data_q_lock);
            p_dest = (uint8_t*)(p_msg + 1);
            p_src = pcm_buf + rtp_offset + timestamp_len;
            memcpy(p_dest, p_src, data_read_from_bt);
            p_msg->len = data_read_from_bt;
            p_msg->offset = 0;
            fixed_queue_enqueue(CompressDataQ, p_msg);
            buffered_length += data_read_from_bt;
            pthread_mutex_unlock(&data_q_lock);
            ALOGD(LOGTAG " %s data_read_from_bt = %d data copied to Q = %d buffered_length %d Qlen %d",__FUNCTION__,
            data_read_from_bt,data_read_from_bt,buffered_length,fixed_queue_length(CompressDataQ) );
            /* we should have bufferred enough data
             * we should not start sending data from here, if we are waiting for callback */
            if((buffered_length > START_COMPRESS_BUF_TSHLD) && (!wait_for_mm_callback)) {
                start_compress_offload(); // start trigger to pump to audio hal
                /* start timer and break from here */
                StartCompressAudioFeedTimer();
                break;
            }
        }
        else {
            ALOGD(LOGTAG " Cannot allocate packet for Q ");
            /* not have memory now, might have later */
            StartCompressAudioFeedTimer();
           break;
        }
#endif

        if (enable_notification_cb) {
            /* if callback mechanism is enabled, BTAPP fetches 1 packet at a time. So wait for
             * next callback, break */
            break;
        }
    } while(1);
 #endif
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
#if (defined BT_AUDIO_HAL_INTEGRATION)
        qahw_out_resume(pA2dpSinkStream->out_stream);
#endif
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
#if (defined BT_AUDIO_HAL_INTEGRATION)
    qahw_module_handle_t* audio_device;
    audio_config_t config;
    audio_io_handle_t handle = 0x07;
    //audio_output_flags_t flags = AUDIO_OUTPUT_FLAG_NONE;
    int flags = AUDIO_OUTPUT_FLAG_NONE;
    struct qahw_aptx_dec_param aptx_params;
    qahw_param_payload payload;
    bdstr_t bd_str;
    int rc = 0;

    memset(&config, 0, sizeof(audio_config_t));
    config.offload_info.size = sizeof(audio_offload_info_t);
    ALOGD(LOGTAG " ConfigureAudioHal codec_type = %d", codec_type);
    switch(codec_type) {
    case A2DP_SINK_AUDIO_CODEC_SBC:
        sample_rate = get_a2dp_sbc_sampling_rate(codec_config.sbc_config.samp_freq);
        channel_count = get_a2dp_sbc_channel_mode(codec_config.sbc_config.ch_mode);
        config.offload_info.format = AUDIO_FORMAT_PCM_16_BIT;
        flags |= AUDIO_OUTPUT_FLAG_DIRECT_PCM;
        qahw_delay = DEFAULT_SBC_SYSTEM_DELAY;
        break;
    case A2DP_SINK_AUDIO_CODEC_AAC:
        sample_rate = get_a2dp_aac_sampling_rate(codec_config.aac_config.sampling_freq);
        channel_count = get_a2dp_aac_channel_mode(codec_config.aac_config.channel_count);
        config.offload_info.format = AUDIO_FORMAT_AAC_LATM_LC;
        flags |= AUDIO_OUTPUT_FLAG_NON_BLOCKING;
        flags |= AUDIO_OUTPUT_FLAG_COMPRESS_OFFLOAD;
        qahw_delay = DEFAULT_AAC_SYSTEM_DELAY;
        break;
    case A2DP_SINK_AUDIO_CODEC_MP3:
        sample_rate = get_a2dp_mp3_sampling_rate(codec_config.mp3_config.sampling_freq);
        channel_count = get_a2dp_mp3_channel_mode(codec_config.mp3_config.channel_count);
        config.offload_info.format = AUDIO_FORMAT_MP3;
        flags |= AUDIO_OUTPUT_FLAG_NON_BLOCKING;
        flags |= AUDIO_OUTPUT_FLAG_COMPRESS_OFFLOAD;
        qahw_delay = DEFAULT_MP3_SYSTEM_DELAY;
        break;
    case A2DP_SINK_AUDIO_CODEC_APTX:
        #if (defined USE_GST)
             gstbtobj.is_compressed = true;
        #endif
        sample_rate = get_a2dp_aptx_sampling_rate(codec_config.aptx_config.sampling_freq);
        channel_count = get_a2dp_aptx_channel_mode(codec_config.aptx_config.channel_count);
        config.offload_info.format = AUDIO_FORMAT_APTX;
        flags |= AUDIO_OUTPUT_FLAG_NON_BLOCKING;
        flags |= AUDIO_OUTPUT_FLAG_COMPRESS_OFFLOAD;
        qahw_delay = DEFAULT_APTX_SYSTEM_DELAY;
        break;
    }
    ALOGD(LOGTAG " sample_rate = %d, channel_count = %d", sample_rate, channel_count);
#if (defined USE_GST)
    if (gstbtobj.is_compressed == true){
        bt_bdaddr_t *bd_addr = g_gap->GetBtAddress();
        bdaddr_to_string(bd_addr, &bd_str[0], sizeof(bd_str));
        ALOGD (LOGTAG " Local bdaddr %s", bd_str);
        memcpy(gstbtobj.bt_addr,&bd_str[0], sizeof(bd_str));
        gstbtobj.bt_addr[sizeof(bd_str)+1]='\0';
        ALOGD (LOGTAG " gstobj bt_addr %s \n", gstbtobj.bt_addr);
    }
    init_gst_pipeline(&gstbtobj, config.offload_info.format, sample_rate, channel_count, flags, "bt_a2dp_sink");
#else
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
        if (audio_device != NULL) {
            if (codec_type == A2DP_SINK_AUDIO_CODEC_APTX) {
                // send local bd_addr to audio
                bt_bdaddr_t *bd_addr = g_gap->GetBtAddress();
                bdaddr_to_string(bd_addr, &bd_str[0], sizeof(bd_str));
                ALOGD (LOGTAG " Local bdaddr %s", bd_str);
                parse_aptx_dec_bd_addr(&bd_str[0], &aptx_params);
                payload.aptx_params = aptx_params;
                rc = qahw_set_param_data(audio_device, QAHW_PARAM_APTX_DEC, &payload);
                if (rc != 0)
                    ALOGE(LOGTAG "Error. Failed to set Local bluetooth address to audio hal");
            }
            // 2 refers to speaker
            ALOGD(LOGTAG " opening output stream ");
            qahw_open_output_stream(audio_device, handle, audio_out_device,
                   (audio_output_flags_t)flags,
                   &config, &out_stream, "bt_a2dp_sink");
        }
        if (out_stream != NULL) {
            pcm_buf_size = qahw_out_get_buffer_size(out_stream);
            ALOGD(LOGTAG " pcm buf size %d", pcm_buf_size);
            /* pcm_buf is buffer that we read frm stack */
            if (pcm_buf == NULL) {
                pcm_buf = (uint8_t*)osi_malloc(pcm_buf_size);
            }
            /* audioFragment is buffer that we write to audio-hal in out_write */
            if(audioFragment == NULL) {
                ALOGD(LOGTAG " %s allocating audioFragment ", __FUNCTION__);
                audioFragment = (tBT_SINK_DQ_DATA_HDR*)osi_malloc(sizeof(tBT_SINK_DQ_DATA_HDR) + pcm_buf_size);
                audioFragment->len = 0;
                audioFragment->offset = 0;
            }
            /* CompressDataQ is Queue in btapp, where we buffere data from stack */
            if (CompressDataQ == NULL) {
                CompressDataQ = fixed_queue_new(SIZE_MAX);
            }
            if (codec_type == A2DP_SINK_AUDIO_CODEC_SBC)
            {
                pcm_timer_duration = (pcm_buf_size*1000)/(sample_rate*channel_count*2);
                /* we should calculate pcm_timer_duration from buffer size
                 * sample_rate * channel_count * 2 = Number of bytes for 1 second.
                 * 2 for 16-bit-pcm, 3 for 24-bit-pcm etc 
                 * We have to start pcm timer for this duration */
                pcm_timer_duration = pcm_timer_duration - 5;
                /* reeucing timer duration ensures that timer fires quicker and
                 * effectlvely we pump slightly more data. This approach is taken to handle
                 * timer skew */
                ALOGD(LOGTAG " pcm timer duration %d", pcm_timer_duration);
            }

            if(pAvrcp != NULL) {
                if(pAvrcp->is_abs_vol_supported(pA2dpSinkStream->mStreamingDevice)) {
                    SetStreamVol(pAvrcp->get_current_audio_index());
                }
            }
        }
        if (codec_type != A2DP_SINK_AUDIO_CODEC_SBC) {
            qahw_out_set_callback(out_stream, compressed_callback, NULL);
        }
        mBtA2dpSinkStreamingVendorInterface->update_qahw_delay_vendor(qahw_delay);
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
    pcm_timer_duration = A2DP_SINK_PCM_FETCH_TIMER_DURATION;
    if (outputPcmSampleFile == NULL)
        outputPcmSampleFile = fopen(outputFilename, "ab");
#endif
#if (defined(DUMP_COMPRESSED_DATA) && (DUMP_COMPRESSED_DATA == TRUE))
    if (outputPcmSampleFile == NULL)
        outputPcmSampleFile = fopen(outputFilename, "ab");
#endif
#endif

}

void A2dp_Sink_Streaming::CloseAudioStream() {
#if (defined BT_AUDIO_HAL_INTEGRATION)
    qahw_module_handle_t* audio_device;
    if (pBTAM != NULL) {
        audio_device = pBTAM->GetAudioDevice();
        if((audio_device != NULL) && (out_stream != NULL)) {
            // 2 refers to speaker
            ALOGD(LOGTAG " closing output stream ");
            wait_for_mm_callback = false;
            qahw_close_output_stream(out_stream);
            cuml_data_written_to_audio = 0;
            out_write_ts = 0;
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

void A2dp_Sink_Streaming::UnLoadBtA2dpHAL() {
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

void A2dp_Sink_Streaming::OpenInputStream()
{
#if (defined(BT_AUDIO_HAL_INTEGRATION))
    int ret = -1;
    ALOGD(LOGTAG " Open A2dp Input Stream ");
    if (!a2dp_input_device) {
        ALOGE(LOGTAG " Invalid A2dp HAL device. Bail out! ");
        return;
    }
    ret = qahw_open_input_stream(a2dp_input_device, 0, AUDIO_DEVICE_OUT_ALL_A2DP,
          NULL, &input_stream, AUDIO_INPUT_FLAG_NONE, "bt_a2dp_input_stream", AUDIO_SOURCE_DEFAULT);
    if (ret < 0) {
        input_stream = NULL;
        ALOGE(LOGTAG " open input stream returned %d\n ", ret);
    }
    ALOGD(LOGTAG " A2dp Input Stream successfully opened ");
#endif
}

void A2dp_Sink_Streaming::CloseInputStream()
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

void A2dp_Sink_Streaming::SuspendInputStream()
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

void A2dp_Sink_Streaming::SetStreamVol(int curr_audio_index)
{
#if (defined(BT_AUDIO_HAL_INTEGRATION))
    ALOGD(LOGTAG " SetStreamVol curr_audio_index %d ", curr_audio_index);
    if(!out_stream)
    {
        ALOGE(LOGTAG " Invalid output Stream. Bail out! ");
        return;
    }
    qahw_out_set_volume(out_stream, (float)curr_audio_index/15, (float)curr_audio_index/15);
    ALOGD(LOGTAG " SetStreamVol = %d successfully", curr_audio_index);
#endif
}

uint32_t A2dp_Sink_Streaming::ReadInputStream(uint8_t* data, uint32_t size)
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

uint32_t A2dp_Sink_Streaming::GetInputStreamBufferSize()
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
#if (defined BT_AUDIO_HAL_INTEGRATION)
    out_stream =  NULL;
    input_stream = NULL;
    a2dp_input_device = NULL;
#if (defined USE_GST)
    gbuff = (uint8_t *)malloc(A2DP_SINK_GBUF_MAX_SIZE);
#endif
#endif
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
#if (defined BT_AUDIO_HAL_INTEGRATION)
#if (defined USE_GST)
    free(gbuff);
#else
    out_stream = NULL;
#endif
    input_stream = NULL;
    a2dp_input_device = NULL;
#endif
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
