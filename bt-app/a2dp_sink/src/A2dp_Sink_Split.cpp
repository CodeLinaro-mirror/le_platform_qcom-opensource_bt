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
#include "A2dp_Sink_Streaming.hpp"
#include "A2dp_Sink_Split.hpp"
#include "Avrcp.hpp"
#include "Gap.hpp"
#include "A2dp_Src.hpp"
#include "hardware/bt_av_vendor.h"
#include <algorithm>

#define LOGTAG "A2DP_SINK_SPLIT"

using namespace std;
using std::list;
using std::string;

A2dp_Sink_Split *pA2dpSinkSplit = NULL;
extern Avrcp *pAvrcp;
extern void flush_relay_data(void);

#ifdef __cplusplus
extern "C" {
#endif

#ifndef _ARRAYSIZE
#define _ARRAYSIZE(a) (sizeof(a)/sizeof(a[0]))
#endif
/**
 * Maximum argument length
 */
#define COMMAND_ARG_SIZE     200
#define SBC_PARAM_LEN 1
#define APTX_PARAM_LEN 1
#define APTX_HD_PARAM_LEN 1
#define APTX_AD_PARAM_LEN 1
#define MP3_PARAM_LEN 2
#define AAC_PARAM_LEN 2

static btav_codec_configuration_t a2dpSnkCodecList[MAX_NUM_CODEC_CONFIGS];

static const char * valid_codecs[] = {
    "aac",
    "mp3",
    "sbc",
    "aptx",
    "aptx_hd",
    "aptx_ad"
};

static uint8_t valid_codec_values[] = {
    A2DP_SINK_AUDIO_CODEC_AAC,
    A2DP_SINK_AUDIO_CODEC_MP3,
    A2DP_SINK_AUDIO_CODEC_SBC,
    A2DP_SINK_AUDIO_CODEC_APTX,
    A2DP_SINK_AUDIO_CODEC_APTX_HD,
    A2DP_SINK_AUDIO_CODEC_APTX_AD
};

static const char * valid_sbc_freq[] = {
    "16",
    "32",
    "44.1",
    "48",
};

static uint8_t valid_sbc_freq_values[] = {
    SBC_SAMP_FREQ_16,
    SBC_SAMP_FREQ_32,
    SBC_SAMP_FREQ_44,
    SBC_SAMP_FREQ_48,
};

static const char * valid_aac_freq[] = {
    "8",
    "11.025",
    "12",
    "16",
    "22.05",
    "24",
    "32",
    "44.1",
    "48",
    "64",
    "88.2",
    "96",
};

static uint16_t valid_aac_freq_values[] = {
    AAC_SAMP_FREQ_8000,
    AAC_SAMP_FREQ_11025,
    AAC_SAMP_FREQ_12000,
    AAC_SAMP_FREQ_16000,
    AAC_SAMP_FREQ_22050,
    AAC_SAMP_FREQ_24000,
    AAC_SAMP_FREQ_32000,
    AAC_SAMP_FREQ_44100,
    AAC_SAMP_FREQ_48000,
    AAC_SAMP_FREQ_64000,
    AAC_SAMP_FREQ_88200,
    AAC_SAMP_FREQ_96000,
};

static const char * valid_aac_obj_type[] = {
    "MPEG-2-LC",
    "MPEG-4-LC",
    "MPEG-4-LTP",
    "MPEG-4-SC"
};

static uint8_t valid_aac_obj_type_values[] = {
    AAC_OBJ_TYPE_MPEG_2_AAC_LC,
    AAC_OBJ_TYPE_MPEG_4_AAC_LC,
    AAC_OBJ_TYPE_MPEG_4_AAC_LTP,
    AAC_OBJ_TYPE_MPEG_4_AAC_SCA,
};

static const char * valid_mp3_freq[] = {
    "16",
    "22.05",
    "24",
    "32",
    "44.1",
    "48",
};

static uint8_t valid_mp3_freq_values[] = {
    MP3_SAMP_FREQ_16000,
    MP3_SAMP_FREQ_22050,
    MP3_SAMP_FREQ_24000,
    MP3_SAMP_FREQ_32000,
    MP3_SAMP_FREQ_44100,
    MP3_SAMP_FREQ_48000,
};

static const char * valid_mp3_layer[] = {
    "LAYER1",
    "LAYER2",
    "LAYER3",
};

static uint8_t valid_mp3_layer_values[] = {
    MP3_LAYER_1,
    MP3_LAYER_2,
    MP3_LAYER_3,
};

static const char * valid_aptx_freq[] = {
    "44.1",
    "48",
};

static const char * valid_aptx_hd_freq[] = {
    "44.1",
    "48",
};

static const char * valid_aptx_ad_freq[] = {
    "44.1",
    "48",
};

static uint8_t valid_aptx_freq_values[] = {
    APTX_SAMPLERATE_44100,
    APTX_SAMPLERATE_48000,
};

static uint8_t valid_aptx_hd_freq_values[] = {
    APTX_HD_SAMPLERATE_44100,
    APTX_HD_SAMPLERATE_48000,
};

static uint8_t valid_aptx_ad_freq_values[] = {
    APTX_AD_SAMPLERATE_44100,
    APTX_AD_SAMPLERATE_48000,
};

/******************************************************************************
 * This structure defines the A2DP Sink variable.
 */
typedef struct {
    const char *name;            /**< Run-time variable name */
    const char *description;     /**< Run-time variable description */
    const char **valid_options;  /**< List of valid variable values */
    int valid_option_cnt;        /**< Size of valid value list */
} A2DP_SINK_VARIABLE;

/******************************************************************************
 * List of A2DP Sink variables.
 */
const A2DP_SINK_VARIABLE variable_list[] = {
    { "codec type", "Valid Codec Type to Use",
      valid_codecs, _ARRAYSIZE(valid_codecs) },
    { "sbc freq", "Valid SBC Freq to Use",
      valid_sbc_freq, _ARRAYSIZE(valid_sbc_freq) },
    { "aac freq", "Valid AAC Freq to Use",
      valid_aac_freq, _ARRAYSIZE(valid_aac_freq) },
    { "mp3 freq", "Valid MP3 Freq to Use",
      valid_mp3_freq, _ARRAYSIZE(valid_mp3_freq) },
    { "aptx freq", "Valid APTX Freq to Use",
      valid_aptx_freq, _ARRAYSIZE(valid_aptx_freq) },
    { "aptxhd freq", "Valid APTX HD Freq to Use",
      valid_aptx_hd_freq, _ARRAYSIZE(valid_aptx_hd_freq) },
    { "aptxad freq", "Valid APTX AD Freq to Use",
      valid_aptx_ad_freq, _ARRAYSIZE(valid_aptx_ad_freq) },
    { "aac object type", "Valid AAC Object Type to Use",
      valid_aac_obj_type, _ARRAYSIZE(valid_aac_obj_type) },
    { "mp3 layer", "Valid MP3 Layer to Use",
      valid_mp3_layer, _ARRAYSIZE(valid_mp3_layer) },
};

/******************************************************************************
 *
 * Basic utilities.
 *
 */

#ifndef isdelimiter
#define isdelimiter(c) ((c) == ' ' || (c) == ',' || (c) == '\f' || (c) == '\n' || \
        (c) == '\r' || (c) == '\t' || (c) == '\v')
#endif

#ifndef ishyphon
#define ishyphon(c) (c == '-')
#endif

extern bool GetCodecInfoByAddr(bt_bdaddr_t* bd_addr, uint16_t *dev_codec_type, btav_codec_config_t* codec_config);
extern list<A2dp_Device>::iterator FindDeviceByAddr(list<A2dp_Device>& pA2dpDev, bt_bdaddr_t dev);
extern int StrcompareInsensitiv(char const *p1,  char const *p2);

static int find_str_in_list(const char *str, const char * const *list,
                                   int list_size)
{
    int i;
    int item = list_size;
    int match_cnt = 0;

    if (str == NULL || list == NULL || list_size <= 0)
        return -1;

    for (i = 0; i < list_size; i++) {
        if (!StrcompareInsensitiv(list[i], str)) {
            item = i;
            match_cnt++;
        }
    }

    if (match_cnt == 1) {
        return item;
    } else {
        return list_size;
    }
}

static void print_help(const A2DP_SINK_VARIABLE *var)
{
    int i;
    if (var) {
        printf("\n=====HELP=====\n%s:\t%s\n(valid options:", var->name, var->description);
        for (i = 0; i < (uint8_t)var->valid_option_cnt; i++) {
            printf(" %s", var->valid_options[i]);
        }
        printf(")\n");
    }
}

static const char * skip_delimiter(const char *data)
{
    if (data == NULL)
        return NULL;

    while (*data && !ishyphon(*data)) {
        data++;
    }
    return data;
}

static int ParseUserInput (char *input, char output[][COMMAND_ARG_SIZE]) {
    char *temp_arg = NULL;
    char delim[] = ",";
    char *ptr1;
    int param_count = 0;
    bool status = false;

    if (input == NULL || output == NULL)
        return 0;

    if ((temp_arg = strtok_r(input, delim, &ptr1)) != NULL ) {
        strlcpy(output[param_count], temp_arg, COMMAND_ARG_SIZE);
        output[param_count ++][COMMAND_ARG_SIZE - 1] = '\0';
        ALOGE(LOGTAG " %s ", output[param_count -1]);
    }

    while ((temp_arg = strtok_r(NULL, delim, &ptr1))) {
        strlcpy(output[param_count], temp_arg, COMMAND_ARG_SIZE);
        output[param_count ++][COMMAND_ARG_SIZE - 1] = '\0';
        ALOGE(LOGTAG " %s ", output[param_count -1]);
    }

    ALOGE(LOGTAG " %s: returning %d \n", __func__, param_count);
    return param_count;
}

uint32_t get_a2dp_sbc_sampling_rate(uint8_t frequency) {
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

uint8_t get_a2dp_sbc_channel_mode(uint8_t channeltype) {
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

uint32_t get_a2dp_aac_sampling_rate(uint16_t frequency) {
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

uint8_t get_a2dp_aac_channel_mode(uint8_t channel_count) {
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

uint32_t get_a2dp_mp3_sampling_rate(uint16_t frequency) {
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

uint8_t get_a2dp_mp3_channel_mode(uint8_t channel_count) {
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

uint32_t get_a2dp_aptx_sampling_rate(uint8_t frequency) {
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

uint32_t get_a2dp_aptx_hd_sampling_rate(uint8_t frequency) {
    uint32_t freq = 0;
    switch (frequency) {
        case APTX_HD_SAMPLERATE_44100:
            freq = 44100;
            break;
        case APTX_HD_SAMPLERATE_48000:
            freq = 48000;
            break;
    }
    return freq;
}

uint32_t get_a2dp_aptx_ad_sampling_rate(uint8_t frequency) {
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

uint8_t get_a2dp_aptx_channel_mode(uint8_t channel_count) {
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

uint8_t get_a2dp_aptx_hd_channel_mode(uint8_t channel_count) {
    uint8_t count = 1;
    switch (channel_count) {
        case APTX_HD_CHANNELS_MONO:
            count = 1;
            break;
        case APTX_HD_CHANNELS_STEREO:
            count = 2;
            break;
    }
    return count;
}

uint8_t get_a2dp_aptx_ad_channel_mode(uint8_t channel_count) {
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

/* This function is used for testing purpose. Parses string which represents codec list*/

static bool A2dpCodecList(char *codec_param_list, int *num_codec_configs)
{
    int i = 0, j = 0, k = 0;
    char output_list[COMMAND_ARG_SIZE][COMMAND_ARG_SIZE];
    int codec_params_list_size;

    if (*codec_param_list == '\0') {
        ALOGE(LOGTAG " codec list cannot be set to nothing \n");
        fprintf(stdout, "codec list cannot be set to nothing \n");
        print_help(&variable_list[0]);
        return false;
    }
    fprintf(stdout, "Codec List: %s\n", codec_param_list);

    codec_params_list_size = ParseUserInput(codec_param_list, output_list);

    while (j < codec_params_list_size) {
        i = find_str_in_list(output_list[j], valid_codecs, _ARRAYSIZE(valid_codecs));
        if (i >= _ARRAYSIZE(valid_codecs)) {
            fprintf(stdout, "Invalid codec type values: %s\n", output_list[j]);
            print_help(&variable_list[0]);
            return false;
        }
        j++;
        a2dpSnkCodecList[k].codec_type = valid_codec_values[i];
        switch (a2dpSnkCodecList[k].codec_type) {
            case A2DP_SINK_AUDIO_CODEC_AAC:
                /* check number of parameters passed are ok or not */
                if (j + AAC_PARAM_LEN > codec_params_list_size + 1) {
                    fprintf(stdout, "Invalid AAC Parameters passed\n");
                    return false;
                }
                i = find_str_in_list(output_list[j], valid_aac_freq,
                    _ARRAYSIZE(valid_aac_freq));
                if (i >= _ARRAYSIZE(valid_aac_freq)) {
                    fprintf(stdout, "Invalid AAC Sampling Freq: %s\n", output_list[j]);
                    print_help(&variable_list[2]);
                    return false;
                }
                a2dpSnkCodecList[k].codec_config.aac_config.sampling_freq =
                    valid_aac_freq_values[i];
                j ++;
                i = find_str_in_list(output_list[j], valid_aac_obj_type,
                    _ARRAYSIZE(valid_aac_obj_type));
                if (i >= _ARRAYSIZE(valid_aac_obj_type)) {
                    fprintf(stdout, "Invalid AAC Object Type: %s\n", output_list[j]);
                    print_help(&variable_list[2]);
                    return false;
                }
                a2dpSnkCodecList[k].codec_config.aac_config.obj_type =
                    valid_aac_obj_type_values[i];
                j ++;
                break;
            case A2DP_SINK_AUDIO_CODEC_MP3:
                /* check number of parameters passed are ok or not */
                if (j + MP3_PARAM_LEN > codec_params_list_size + 1) {
                    fprintf(stdout, "Invalid MP3 Parameters passed\n");
                    return false;
                }
                i = find_str_in_list(output_list[j], valid_mp3_freq,
                    _ARRAYSIZE(valid_mp3_freq));
                if (i >= _ARRAYSIZE(valid_mp3_freq)) {
                    fprintf(stdout, "Invalid MP3 Sampling Freq: %s\n",
                        output_list[j]);
                    print_help(&variable_list[3]);
                    return false;
                }
                a2dpSnkCodecList[k].codec_config.mp3_config.sampling_freq =
                    valid_mp3_freq_values[i];
                j ++;
                i = find_str_in_list(output_list[j], valid_mp3_layer,
                    _ARRAYSIZE(valid_mp3_layer));
                if (i >= _ARRAYSIZE(valid_mp3_layer)) {
                    fprintf(stdout, "Invalid MP3 Layer: %s\n", output_list[j]);
                    print_help(&variable_list[6]);
                    return false;
                }
                a2dpSnkCodecList[k].codec_config.mp3_config.layer =
                    valid_mp3_layer_values[i];
                j ++;
                break;
            case A2DP_SINK_AUDIO_CODEC_SBC:
                /* check number of parameters passed are ok or not */
                if (j + SBC_PARAM_LEN > codec_params_list_size + 1) {
                    fprintf(stdout, "Invalid SBC Parameters passed\n");
                    return false;
                }
                i = find_str_in_list(output_list[j], valid_sbc_freq,
                    _ARRAYSIZE(valid_sbc_freq));
                if (i >= _ARRAYSIZE(valid_sbc_freq)) {
                    fprintf(stdout, "Invalid SBC Sampling Freq: %s\n",
                        output_list[j]);
                    print_help(&variable_list[1]);
                    return false;
                }
                a2dpSnkCodecList[k].codec_config.sbc_config.samp_freq =
                    valid_sbc_freq_values[i];
                j ++;
                break;
            case A2DP_SINK_AUDIO_CODEC_APTX:
                /* check number of parameters passed are ok or not */
                if (j + APTX_PARAM_LEN > codec_params_list_size + 1) {
                    fprintf(stdout, "Invalid APTX Parameters passed\n");
                    return false;
                }
                i = find_str_in_list(output_list[j], valid_aptx_freq,
                    _ARRAYSIZE(valid_aptx_freq));
                if (i >= _ARRAYSIZE(valid_aptx_freq)) {
                    fprintf(stdout, "Invalid APTX Sampling Freq: %s\n",
                        output_list[j]);
                    print_help(&variable_list[4]);
                    return false;
                }
                a2dpSnkCodecList[k].codec_config.aptx_config.sampling_freq =
                    valid_aptx_freq_values[i];
                j ++;
                break;
            case A2DP_SINK_AUDIO_CODEC_APTX_HD:
                /* check number of parameters passed are ok or not */
                if (j + APTX_HD_PARAM_LEN > codec_params_list_size + 1) {
                    fprintf(stdout, "Invalid APTX HD Parameters passed\n");
                    return false;
                }
                i = find_str_in_list(output_list[j], valid_aptx_hd_freq,
                    _ARRAYSIZE(valid_aptx_hd_freq));
                if (i >= _ARRAYSIZE(valid_aptx_hd_freq)) {
                    fprintf(stdout, "Invalid APTX HD Sampling Freq: %s\n",
                        output_list[j]);
                    print_help(&variable_list[5]);
                    return false;
                }
                a2dpSnkCodecList[k].codec_config.aptx_hd_config.sampling_freq =
                    valid_aptx_hd_freq_values[i];
                j ++;
                break;
           case A2DP_SINK_AUDIO_CODEC_APTX_AD:
                /* check number of parameters passed are ok or not */
                if (j + APTX_AD_PARAM_LEN > codec_params_list_size + 1) {
                fprintf(stdout, "Invalid APTX Parameters passed\n");
                return false;
                }
                i = find_str_in_list(output_list[j], valid_aptx_ad_freq,
                _ARRAYSIZE(valid_aptx_freq));
                if (i >= _ARRAYSIZE(valid_aptx_ad_freq)) {
                fprintf(stdout, "Invalid APTX Sampling Freq: %s\n",
                    output_list[j]);
                print_help(&variable_list[6]);
                return false;
                }
                a2dpSnkCodecList[k].codec_config.aptx_ad_config.sampling_freq =
                valid_aptx_ad_freq_values[i];
                j ++;
                break;
        }
        k++;
        if (k >= MAX_NUM_CODEC_CONFIGS) {
            fprintf(stdout, "num_codec_configs  exceeds max number(%d) = %d\n",
                k, MAX_NUM_CODEC_CONFIGS);
            return false;
        }
    }
    *num_codec_configs = k;
    fprintf(stdout, "num_codec_configs  = %d\n", *num_codec_configs);
    return true;
}

void BtA2dpSinkSplitMsgHandler(void *msg) {
    BtEvent* pEvent = NULL;
    BtEvent* pCleanupEvent = NULL;
    BtEvent *pCleanupSinkStreaming = NULL;
    BtEvent *pReleaseControlReq = NULL;
    int num_codec_configs = 0;
    if(!msg) {
        printf("Msg is NULL, return.\n");
        return;
    }

    pEvent = ( BtEvent *) msg;
    switch(pEvent->event_id) {
        case PROFILE_API_START:
            ALOGD(LOGTAG " enable a2dp sink");
            if (pA2dpSinkSplit) {
                pA2dpSinkSplit->HandleEnableSink();
            }
            break;
        case PROFILE_API_STOP:
            ALOGD(LOGTAG " disable a2dp sink");
            if (pA2dpSinkSplit) {
                pA2dpSinkSplit->HandleDisableSink();
            }
            break;
        case A2DP_SINK_CLEANUP_REQ://to do :check if any handling is needed here
            ALOGD(LOGTAG " cleanup a2dp sink");
            pCleanupEvent = new BtEvent;
            pCleanupEvent->event_id = A2DP_SINK_CLEANUP_DONE;
            PostMessage(THREAD_ID_GAP, pCleanupEvent);
            break;
        case A2DP_SINK_CODEC_LIST:
            A2dpCodecList(pEvent->a2dpCodecListEvent.codec_list,
                &num_codec_configs);
            if (num_codec_configs)
                pA2dpSinkSplit->UpdateSupportedCodecs(num_codec_configs);
            break;
        case A2DP_SINK_ACCEPT_PENDING_COMMAND:
            if(pA2dpSinkSplit){
                ALOGD(LOGTAG " Accept pending command, start: %d, suspend:%d ",
                    pA2dpSinkSplit->start_pending,pA2dpSinkSplit->suspend_pending);
                if(pA2dpSinkSplit->start_pending)
                    pA2dpSinkSplit->sBtA2dpSinkVendorInterface->start_ind_rsp(pA2dpSinkSplit->
                            mPendingDevice,CMD_ACCEPTED);
                else if(pA2dpSinkSplit->suspend_pending)
                    pA2dpSinkSplit->sBtA2dpSinkVendorInterface->suspend_ind_rsp(pA2dpSinkSplit->
                            mPendingDevice,CMD_ACCEPTED);
            pA2dpSinkSplit->start_pending = false;
            pA2dpSinkSplit->suspend_pending = false;
            }
            break;
        case A2DP_SINK_REJECT_PENDING_COMMAND:
            if(pA2dpSinkSplit){
                ALOGD(LOGTAG " Accept pending command, start: %d, suspend:%d ",
                    pA2dpSinkSplit->start_pending,pA2dpSinkSplit->suspend_pending);
                if(pA2dpSinkSplit->start_pending)
                    pA2dpSinkSplit->sBtA2dpSinkVendorInterface->start_ind_rsp(pA2dpSinkSplit->
                            mPendingDevice,CMD_REJECTED);
                else if(pA2dpSinkSplit->suspend_pending)
                    pA2dpSinkSplit->sBtA2dpSinkVendorInterface->suspend_ind_rsp(pA2dpSinkSplit->
                            mPendingDevice,CMD_REJECTED);
            pA2dpSinkSplit->start_pending = false;
            pA2dpSinkSplit->suspend_pending = false;
            }
            break;
        case BT_AM_CONTROL_STATUS:
            ALOGD(LOGTAG " BT_AM_CONTROL_STATUS");
            if (pA2dpSinkSplit) {
                ALOGD(LOGTAG " earlier status = %d  new status = %d", pA2dpSinkSplit->controlStatus,
                        pEvent->btamControlStatus.status_type);
                pA2dpSinkSplit->controlStatus = pEvent->btamControlStatus.status_type;
                switch(pA2dpSinkSplit->controlStatus) {
                    case STATUS_LOSS:
                        // send pause to remote
                        if (pAvrcp != NULL)
                            pAvrcp->SendPassThruCommandNative(CMD_ID_PAUSE,
                            &pA2dpSinkSplit->mStreamingDevice, 0);
                        // release control
                        pReleaseControlReq = new BtEvent;
                        pReleaseControlReq->btamControlRelease.event_id = BT_AM_RELEASE_CONTROL;
                        pReleaseControlReq->btamControlRelease.profile_id = PROFILE_ID_A2DP_SINK;
                        PostMessage(THREAD_ID_BT_AM, pReleaseControlReq);
                        break;
                    case STATUS_LOSS_TRANSIENT:
                        // send pause to remote
                        if (pAvrcp != NULL) {
                            ALOGD(LOGTAG " copy resuming device");
                            memcpy(&pA2dpSinkSplit->mResumingDevice,
                                &pA2dpSinkSplit->mStreamingDevice, sizeof(bt_bdaddr_t));
                            ALOGD(LOGTAG " sending pause copy resuming device");
                            pAvrcp->SendPassThruCommandNative(CMD_ID_PAUSE,
                                &pA2dpSinkSplit->mStreamingDevice, 0);
                        }
                        break;
                    case STATUS_GAIN:
                        ALOGD(LOGTAG " STATUS_GAIN");
                        break;
                    case STATUS_REGAINED:
                        ALOGD(LOGTAG " STATUS_REGAINED");
                        // send play to remote
                        if (pAvrcp != NULL && !bdaddr_is_empty(&pA2dpSinkSplit->mResumingDevice)) {
                            ALOGD(LOGTAG " STATUS_REGAINED, sending play");
                            pAvrcp->SendPassThruCommandNative(CMD_ID_PLAY,
                                    &pA2dpSinkSplit->mResumingDevice, 1);
                            memset(&pA2dpSinkSplit->mResumingDevice, 0, sizeof(bt_bdaddr_t));
                        }
                        break;
                }
            }
            break;
        case A2DP_SINK_SPLIT_SET_TTP_RANGE:
            if(pA2dpSinkSplit) {
                if (pA2dpSinkSplit->sBtA2dpSinkVendorInterface != NULL) {
                    ALOGD(LOGTAG " set ttp range for aptx ad in stack");
                    ALOGD(LOGTAG " min %d max %d",pEvent->a2dpSinkEvent.arg1,pEvent->a2dpSinkEvent.arg2);
                    pA2dpSinkSplit->sBtA2dpSinkVendorInterface->set_ttp_range_for_aptx_ad(pEvent->
                            a2dpSinkEvent.arg1, pEvent->a2dpSinkEvent.arg2);
                }
            }
            break;
        default:
            if(pA2dpSinkSplit) {
               pA2dpSinkSplit->EventManager(( BtEvent *) msg, pEvent->a2dpSinkEvent.bd_addr);
            }
            break;
    }
    delete pEvent;
}

#ifdef __cplusplus
}
#endif

static void bta2dp_connection_state_callback(const RawAddress& bd_addr, btav_connection_state_t state) {
    ALOGD(LOGTAG " Connection State CB state = %d", state);
    BtEvent *pEvent = new BtEvent;
    pEvent->a2dpSinkEvent.bd_addr = bd_addr;
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
    PostMessage(THREAD_ID_A2DP_SINK_SPLIT, pEvent);
}

static void bta2dp_audio_state_callback(const RawAddress& bd_addr, btav_audio_state_t state) {
    ALOGD(LOGTAG " Audio State CB state = %d", state);
    BtEvent *pEvent = new BtEvent;
    pEvent->a2dpSinkEvent.bd_addr = bd_addr;
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
    PostMessage(THREAD_ID_A2DP_SINK_SPLIT, pEvent);
}

static void bta2dp_audio_config_callback(const RawAddress& bd_addr, uint32_t sample_rate,
        uint8_t channel_count) {
    ALOGD(LOGTAG " Audio Config CB sample_rate %d, channel_count %d", sample_rate, channel_count);
    list<A2dp_Device>::iterator iter = FindDeviceByAddr(pA2dpSinkSplit->pA2dpDeviceList, bd_addr);
    if(iter != pA2dpSinkSplit->pA2dpDeviceList.end())
    {
        ALOGD(LOGTAG " Audio Config CB: found matching device");
        iter->av_config.sample_rate = sample_rate;
        iter->av_config.channel_count = channel_count;
    }
    else
    {
        ALOGE(LOGTAG " ERROR: Audio Config CB: No matching device");
    }
}
// no changes before this
static void bta2dp_audio_data_read_callback(bt_bdaddr_t *bd_addr) {
    ALOGD(LOGTAG " Audio Data Read Callback");
    //to do : check btif part for any changes here
}
// no chnages after this
static void bta2dp_audio_focus_request_vendor_callback(bt_bdaddr_t *bd_addr) {
    ALOGD(LOGTAG " bta2dp_audio_focus_request_vendor_callback ");
    BtEvent *pEvent = new BtEvent;
    pEvent->a2dpSinkEvent.event_id = A2DP_SINK_FOCUS_REQUEST_CB;
    memcpy(&pEvent->a2dpSinkEvent.bd_addr, bd_addr, sizeof(bt_bdaddr_t));
    PostMessage(THREAD_ID_A2DP_SINK_SPLIT, pEvent);
}

static void bta2dp_audio_codec_config_vendor_callback(bt_bdaddr_t *bd_addr, uint16_t codec_type,
        btav_codec_config_t codec_config) {
    ALOGD(LOGTAG " bta2dp_audio_codec_config_vendor_callback codec_type=%d",codec_type);

    BtEvent *pEvent = new BtEvent;
    pEvent->a2dpSinkEvent.event_id = A2DP_SINK_CODEC_CONFIG;
    memcpy(&pEvent->a2dpSinkEvent.bd_addr, bd_addr, sizeof(bt_bdaddr_t));
    pEvent->a2dpSinkEvent.buf_size = sizeof(btav_codec_config_t);
    pEvent->a2dpSinkEvent.buf_ptr = (uint8_t*)osi_malloc(pEvent->a2dpSinkEvent.buf_size);
    memcpy(pEvent->a2dpSinkEvent.buf_ptr, &codec_config, pEvent->a2dpSinkEvent.buf_size);
    pEvent->a2dpSinkEvent.arg1 = codec_type;
    PostMessage(THREAD_ID_A2DP_SINK_SPLIT, pEvent);
}

static void bta2dp_audio_registration_callback(bool state) {
    ALOGD(LOGTAG " Audio Registration Callback: state = %d", state);
}

static void bta2dp_audio_split_sink_start_ind_callback(const bt_bdaddr_t& bd_addr) {
    int accepted = 1;
    int length = 200;
    char user_input[length] = {'\0'};
    ALOGD(LOGTAG " bta2dp_audio_split_sink_start_ind_callback ");
    fprintf(stdout, "\n*************************************************");
    fprintf(stdout, "\n Recieved Start from Src device");
    fprintf(stdout, "\n*************************************************\n");
    fprintf(stdout, " ** Please enter accept / reject in a2dp_sink_menu **\n");
    memcpy(&pA2dpSinkSplit->mPendingDevice, &bd_addr, sizeof(bt_bdaddr_t));
    pA2dpSinkSplit->start_pending = true;
    pA2dpSinkSplit->suspend_pending = false;
}

static void bta2dp_audio_split_sink_suspend_ind_callback(const bt_bdaddr_t& bd_addr) {
    int accepted = 1;
    int length = 200;
    char user_input[length] = {'\0'};
    ALOGD(LOGTAG " bta2dp_audio_split_sink_suspend_ind_callback ");
    fprintf(stdout, "\n*************************************************");
    fprintf(stdout, "\n Recieved Suspend from Src device");
    fprintf(stdout, "\n*************************************************\n");
    fprintf(stdout, " ** Please enter accept / reject in a2dp_sink_menu **\n");
    memcpy(&pA2dpSinkSplit->mPendingDevice, &bd_addr, sizeof(bt_bdaddr_t));
    pA2dpSinkSplit->suspend_pending = true;
    pA2dpSinkSplit->start_pending = false;
}

static void bta2dp_audio_mtu_config_callback(uint16_t mtu, const RawAddress& bd_addr) {
    ALOGD(LOGTAG " %s, mtu = %d, bdaddr = %s",__func__,mtu,
          bd_addr.ToString().c_str());
}

static void bta2dp_audio_scmst_capabilities_callback(bt_bdaddr_t *bd_addr, bool scmst_enabled) {
    ALOGD(LOGTAG " %s, scmst_enabled = %d, bdaddr = %s",__func__,scmst_enabled,
          bd_addr->ToString().c_str());
    if(scmst_enabled) {
        fprintf(stdout, "Connection established with SCMS-T content protection \n");
    }
}

static void bta2dp_audio_update_cp_callback(const RawAddress& bd_addr, uint8_t cp_header) {
    ALOGD(LOGTAG " %s, cp_header = %d, bdaddr = %s",__func__,cp_header,
          bd_addr.ToString().c_str());
}

static btav_sink_callbacks_t sBluetoothA2dpSinkCallbacks = {
    sizeof(sBluetoothA2dpSinkCallbacks),
    bta2dp_connection_state_callback,
    bta2dp_audio_state_callback,
    bta2dp_audio_config_callback,
};

static btav_sink_vendor_callbacks_t sBluetoothA2dpSinkVendorCallbacks = {
    sizeof(sBluetoothA2dpSinkVendorCallbacks),
    bta2dp_audio_focus_request_vendor_callback,
    bta2dp_audio_codec_config_vendor_callback,
    bta2dp_audio_data_read_callback,
    bta2dp_audio_registration_callback,
    bta2dp_audio_split_sink_start_ind_callback,
    bta2dp_audio_split_sink_suspend_ind_callback,
    bta2dp_audio_mtu_config_callback,
    bta2dp_audio_scmst_capabilities_callback,
    bta2dp_audio_update_cp_callback,
};

void A2dp_Sink_Split::HandleEnableSink(void) {
    ALOGD(LOGTAG " HandleEnableSink ");

    uint8_t streaming_param = 0;
    BtEvent *pEvent = new BtEvent;
    max_a2dp_conn = config_get_int (config,
            CONFIG_DEFAULT_SECTION, "BtMaxA2dpConn", 1);

    if (bluetooth_interface != NULL)
    {
        sBtA2dpSinkInterface = (btav_sink_interface_t *)bluetooth_interface->
                get_profile_interface(BT_PROFILE_ADVANCED_AUDIO_SINK_SPLIT_ID);
        sBtA2dpSinkVendorInterface = (btav_sink_vendor_interface_t *)bluetooth_interface->
                get_profile_interface(BT_PROFILE_ADVANCED_AUDIO_SINK_SPLIT_VENDOR_ID);

        if (sBtA2dpSinkVendorInterface == NULL || sBtA2dpSinkInterface == NULL)
        {
             pEvent->profile_start_event.event_id = PROFILE_EVENT_START_DONE;
             pEvent->profile_start_event.profile_id = PROFILE_ID_A2DP_SINK;
             pEvent->profile_start_event.status = false;
             PostMessage(THREAD_ID_GAP, pEvent);
             return;
        }
        pA2dpSinkSplit->mSinkState = SINK_STATE_STARTED;

#ifdef USE_LIBHW_AOSP
        sBtA2dpSinkInterface->init(&sBluetoothA2dpSinkCallbacks);
#else
        sBtA2dpSinkInterface->init(&sBluetoothA2dpSinkCallbacks, max_a2dp_conn, 0);
#endif

        sBtA2dpSinkVendorInterface->init_vendor(&sBluetoothA2dpSinkVendorCallbacks,
                    max_a2dp_conn, 0,streaming_param);

        pEvent->profile_start_event.event_id = PROFILE_EVENT_START_DONE;
        pEvent->profile_start_event.profile_id = PROFILE_ID_A2DP_SINK;
        pEvent->profile_start_event.status = true;

        PostMessage(THREAD_ID_GAP, pEvent);
    }
}

void A2dp_Sink_Split::HandleDisableSink(void) {
    ALOGD(LOGTAG " HandleDisableSink ");
    pA2dpSinkSplit->mSinkState = SINK_STATE_NOT_STARTED;

    if (pA2dpSinkSplit->pA2dpDeviceList.size() != 0)
        pA2dpSinkSplit->pA2dpDeviceList.clear();

    if (sBtA2dpSinkInterface != NULL) {
        sBtA2dpSinkInterface->cleanup();
        sBtA2dpSinkInterface = NULL;
    }
    if (sBtA2dpSinkVendorInterface != NULL) {
        sBtA2dpSinkVendorInterface->cleanup_vendor();
        sBtA2dpSinkVendorInterface = NULL;
    }

    BtEvent *pEvent = new BtEvent;
    pEvent->profile_stop_event.event_id = PROFILE_EVENT_STOP_DONE;
    pEvent->profile_stop_event.profile_id = PROFILE_ID_A2DP_SINK;
    pEvent->profile_stop_event.status = true;
    PostMessage(THREAD_ID_GAP, pEvent);
}

void A2dp_Sink_Split::ProcessEvent(BtEvent* pEvent, list<A2dp_Device>::iterator iter) {
    switch(iter->mSinkDeviceState) {
        case DEVICE_STATE_DISCONNECTED:
            state_disconnected_handler(pEvent, iter);
            break;
        case DEVICE_STATE_PENDING:
            state_pending_handler(pEvent, iter);
            break;
        case DEVICE_STATE_CONNECTED:
            state_connected_handler(pEvent, iter);
            break;
    }
}
// no change before this
void A2dp_Sink_Split::ConnectionManager(BtEvent* pEvent, bt_bdaddr_t dev) {
    ALOGD(LOGTAG " ConnectionManager ");
    A2dp_Device *newNode = NULL;
    list<A2dp_Device>::iterator iter;

    switch(pEvent->event_id) {
        case A2DP_SINK_API_CONNECT_REQ:
            if (pA2dpDeviceList.size() == max_a2dp_conn) {
                ALOGE(LOGTAG " already max devices connected");
                fprintf(stdout, "Already %d device connected\n", max_a2dp_conn);
                return;
            }
            if (pA2dpDeviceList.size() != 0) {
                ALOGD(LOGTAG " Atleast 1 remote device connected/connecting ");
                iter = FindDeviceByAddr(pA2dpDeviceList, dev);
                if (iter != pA2dpDeviceList.end())
                {
                    ALOGE(LOGTAG " Connect req for already connected/connecting device");
                    fprintf(stdout, "Connect req for already connected/connecting device\n");
                    return;
                }
            }
            if (pA2dpDeviceList.size() < max_a2dp_conn) {
                ALOGD(LOGTAG " pA2dpDeviceList.size() < max_a2dp_conn ");
                pA2dpDeviceList.push_back(A2dp_Device(config, dev));
                iter = pA2dpDeviceList.end();
                --iter;
            }
            break;
        case A2DP_SINK_CONNECTING_CB:
        case A2DP_SINK_CONNECTED_CB:
            iter = FindDeviceByAddr(pA2dpDeviceList, dev);
            bdstr_t bd_str;
            if (iter != pA2dpDeviceList.end())
            {
                ALOGD(LOGTAG " found a match, donot alloc new");
            }
            else if (pA2dpDeviceList.size() < max_a2dp_conn)
            {
                ALOGD(LOGTAG " reached end of list without a match, alloc new");
                pA2dpDeviceList.push_back(A2dp_Device(config, dev));
                iter = pA2dpDeviceList.end();
                --iter;
            }
            else
            {
                ALOGE(LOGTAG " already max devices connected");
                fprintf(stdout, "Already %d device connected\n", max_a2dp_conn);
                return;
            }
            if (!pAvrcp->rc_only_devices.empty())
            {
                bdaddr_to_string(&iter->mDevice, &bd_str[0], sizeof(bd_str));
                std::string deviceAddress(bd_str);
                std::list<std::string>::iterator bdstring;
                bdstring = std::find(pAvrcp->rc_only_devices.begin(), pAvrcp->rc_only_devices.end(), deviceAddress);
                if (bdstring != pAvrcp->rc_only_devices.end())
                {
                    ALOGE(LOGTAG "RC already connected earlier for this AV connected device, set RC connected");
                    iter->mAvrcpConnected = true;
                    pAvrcp->rc_only_devices.remove(deviceAddress);
                }
                else
                {
                    ALOGE(LOGTAG "RC not already connected with this device ");
                }
            }
            break;
        case A2DP_SINK_API_DISCONNECT_REQ:
        case A2DP_SINK_DISCONNECTING_CB:
        case A2DP_SINK_DISCONNECTED_CB:
            if (pA2dpDeviceList.size() == 0) {
                ALOGE(LOGTAG " no device to disconnect");
                fprintf(stdout, "No device connected\n");
                return;
            }
            else
            {
                iter = FindDeviceByAddr(pA2dpDeviceList, dev);
                if (iter == pA2dpDeviceList.end())
                {
                    ALOGE(LOGTAG " reached end of list without a match, cannot disconnect");
                    return;
                }
                else
                {
                    ALOGE(LOGTAG " found a match, disconnect this device iter = %x", iter);
                }
            }
            break;
    }
    ProcessEvent(pEvent, iter);
}

void A2dp_Sink_Split::EventManager(BtEvent* pEvent, bt_bdaddr_t dev) {
    ALOGD(LOGTAG " EventManager ");

    if (pA2dpSinkSplit->mSinkState == SINK_STATE_NOT_STARTED)
    {
       ALOGE(LOGTAG " SINK STATE UNINITIALIZED, return");
       return;
    }

    if(isConnectionEvent(pEvent->event_id))
    {
        ConnectionManager(pEvent, dev);
    }
    else
    {
        list<A2dp_Device>::iterator iter = FindDeviceByAddr(pA2dpDeviceList, dev);
        if (iter != pA2dpDeviceList.end())
        {
            ProcessEvent(pEvent, iter);
        }
        else
        {
            ALOGE(LOGTAG " no matching device ignore process event");
        }
    }
}

void A2dp_Sink_Split::UpdateSupportedCodecs(uint8_t num_codec_configs) {
    bt_status_t status;
    int i;
    if (sBtA2dpSinkVendorInterface != NULL) {
        status = sBtA2dpSinkVendorInterface->update_supported_codecs_param_vendor
            (a2dpSnkCodecList, num_codec_configs);
        if (BT_STATUS_SUCCESS != status) {
            ALOGE(LOGTAG " UpdateSupportedCodecs: failed, status = %d", status);
            fprintf(stdout, "UpdateSupportedCodecs: failed, status = %d\n", status);
        }
    }
}

bool A2dp_Sink_Split::isConnectionEvent(BluetoothEventId event_id) {
    bool ret = false;
    if (event_id >= A2DP_SINK_API_CONNECT_REQ && event_id <= A2DP_SINK_DISCONNECTING_CB)
        ret = true;
    ALOGD(LOGTAG " isConnectionEvent: %d", ret);
    return ret;
}
char* A2dp_Sink_Split::dump_message(BluetoothEventId event_id) {
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
    case A2DP_SINK_CODEC_CONFIG:
        return "A2DP_SINK_CODEC_CONFIG";
    }
    return "UNKNOWN";
}
// no change before this
void A2dp_Sink_Split::state_disconnected_handler(BtEvent* pEvent, list<A2dp_Device>::iterator iter) {
    char str[18];
    BtEvent *pOpenInputStream = NULL;
    ALOGD(LOGTAG "state_disconnected_handler Processing event %s", dump_message(pEvent->event_id));
    switch(pEvent->event_id) {
        case A2DP_SINK_API_CONNECT_REQ:
            memcpy(&iter->mConnectingDevice, &iter->mDevice, sizeof(bt_bdaddr_t));
            if (sBtA2dpSinkInterface != NULL) {
                sBtA2dpSinkInterface->connect(iter->mDevice);
            }
            change_state(iter, DEVICE_STATE_PENDING);
            break;
        case A2DP_SINK_CONNECTING_CB:
            memcpy(&iter->mConnectingDevice, &iter->mDevice, sizeof(bt_bdaddr_t));
            bdaddr_to_string(&iter->mConnectingDevice, str, 18);
            fprintf(stdout, "A2DP Sink Connecting to %s\n", str);
            change_state(iter, DEVICE_STATE_PENDING);
            break;
        case A2DP_SINK_CONNECTED_CB:
            memset(&iter->mConnectingDevice, 0, sizeof(bt_bdaddr_t));
            memcpy(&iter->mConnectedDevice, &iter->mDevice, sizeof(bt_bdaddr_t));
            bdaddr_to_string(&iter->mConnectedDevice, str, 18);
            fprintf(stdout, "A2DP Sink Connected to %s\n", str);
            change_state(iter, DEVICE_STATE_CONNECTED);
            break;
        default:
            ALOGD(LOGTAG " event not handled %d ", pEvent->event_id);
            break;
    }
}
void A2dp_Sink_Split::state_pending_handler(BtEvent* pEvent, list<A2dp_Device>::iterator iter) {
    char str[18];
    bool is_valid_codec = true;
    BtEvent *pOpenInputStream = NULL;
    ALOGD(LOGTAG " state_pending_handler Processing event %s", dump_message(pEvent->event_id));
    switch(pEvent->event_id) {
        case A2DP_SINK_CONNECTING_CB:
            ALOGD(LOGTAG " dummy event A2DP_SINK_CONNECTING_CB");
            break;
        case A2DP_SINK_CONNECTED_CB:
            memcpy(&iter->mConnectedDevice, &iter->mDevice, sizeof(bt_bdaddr_t));
            memset(&iter->mConnectingDevice, 0, sizeof(bt_bdaddr_t));
            bdaddr_to_string(&iter->mConnectedDevice, str, 18);
            fprintf(stdout,  "A2DP Sink Connected to %s\n", str);
            change_state(iter, DEVICE_STATE_CONNECTED);
            break;
        case A2DP_SINK_DISCONNECTED_CB:
            fprintf(stdout, "A2DP Sink DisConnected\n");
            memset(&iter->mConnectedDevice, 0, sizeof(bt_bdaddr_t));
            memset(&iter->mConnectingDevice, 0, sizeof(bt_bdaddr_t));
            change_state(iter, DEVICE_STATE_DISCONNECTED);
            break;
        case A2DP_SINK_API_CONNECT_REQ:
            bdaddr_to_string(&iter->mConnectingDevice, str, 18);
            fprintf(stdout, "A2DP Sink Connecting to %s\n", str);
            break;
        case A2DP_SINK_DISCONNECTING_CB:
            ALOGD(LOGTAG " dummy event A2DP_SINK_DISCONNECTING_CB");
            break;
        case A2DP_SINK_CODEC_CONFIG:
            iter->dev_codec_type = pEvent->a2dpSinkEvent.arg1;
            if (pEvent->a2dpSinkEvent.buf_ptr == NULL) {
                break;
            }
            memcpy(&iter->dev_codec_config, pEvent->a2dpSinkEvent.buf_ptr,
                    pEvent->a2dpSinkEvent.buf_size);
            osi_free(pEvent->a2dpSinkEvent.buf_ptr);
            memcpy(&iter->mDevice, &pEvent->a2dpSinkEvent.bd_addr, sizeof(bt_bdaddr_t));
            bdaddr_to_string(&iter->mDevice, str, 18);
            fprintf(stdout, "Codec Configuration for device %s\n", str);
            switch (iter->dev_codec_type) {
                case A2DP_SINK_AUDIO_CODEC_SBC:
                    fprintf(stdout, "Codec type = SBC\n");
                    iter->av_config.sample_rate = get_a2dp_sbc_sampling_rate(iter->dev_codec_config
                        .sbc_config.samp_freq);
                    iter->av_config.channel_count = get_a2dp_sbc_channel_mode(iter->dev_codec_config
                        .sbc_config.ch_mode);
                    break;
                case A2DP_SINK_AUDIO_CODEC_MP3:
                    fprintf(stdout, "Codec type = MP3\n");
                    iter->av_config.sample_rate = get_a2dp_mp3_sampling_rate(iter->dev_codec_config
                        .mp3_config.sampling_freq);
                    iter->av_config.channel_count = get_a2dp_mp3_channel_mode(iter->dev_codec_config
                        .mp3_config.channel_count);
                    break;
                case A2DP_SINK_AUDIO_CODEC_AAC:
                    fprintf(stdout, "Codec type = AAC\n");
                    iter->av_config.sample_rate = get_a2dp_aac_sampling_rate(iter->dev_codec_config
                        .aac_config.sampling_freq);
                    iter->av_config.channel_count = get_a2dp_aac_channel_mode(iter->dev_codec_config
                        .aac_config.channel_count);
                    break;
                case A2DP_SINK_AUDIO_CODEC_APTX:
                    fprintf(stdout, "Codec type = APTX\n");
                    iter->av_config.sample_rate = get_a2dp_aptx_sampling_rate(iter->dev_codec_config
                        .aptx_config.sampling_freq);
                    iter->av_config.channel_count = get_a2dp_aptx_channel_mode(iter->dev_codec_config
                        .aptx_config.channel_count);
                    break;
                case A2DP_SINK_AUDIO_CODEC_APTX_HD:
                    fprintf(stdout, "Codec type = APTX HD\n");
                    iter->av_config.sample_rate = get_a2dp_aptx_hd_sampling_rate(iter->dev_codec_config
                        .aptx_hd_config.sampling_freq);
                    iter->av_config.channel_count = get_a2dp_aptx_hd_channel_mode(iter->dev_codec_config
                        .aptx_hd_config.channel_count);
                    break;
                case A2DP_SINK_AUDIO_CODEC_APTX_AD:
                    fprintf(stdout, "Codec type = APTX AD\n");
                    iter->av_config.sample_rate = get_a2dp_aptx_ad_sampling_rate(iter->dev_codec_config
                         .aptx_config.sampling_freq);
                    iter->av_config.channel_count = get_a2dp_aptx_ad_channel_mode(iter->dev_codec_config
                         .aptx_config.channel_count);
                    break;
                default:
                    is_valid_codec = false;
                    ALOGE(LOGTAG " Invalid codec type %d ", iter->dev_codec_type);
                    break;
            }
            if (is_valid_codec) {
                fprintf(stdout, "Sample Rate = %d\n", iter->av_config.sample_rate);
                fprintf(stdout, "Channel Mode = %d\n", iter->av_config.channel_count);
            }
            break;
        default:
            ALOGD(LOGTAG " event not handled %d ", pEvent->event_id);
            break;
    }
}

void A2dp_Sink_Split::state_connected_handler(BtEvent* pEvent, list<A2dp_Device>::iterator iter) {
    char str[18];
    bool is_valid_codec = true;
    uint32_t pcm_data_read = 0;
    BtEvent *pAMReleaseControl = NULL, *pCloseAudioStream = NULL, *pAMRequestControl = NULL;
    BtEvent *pControlRequest = NULL, *pReleaseControlReq = NULL;
    ALOGD(LOGTAG " state_connected_handler Processing event %s", dump_message(pEvent->event_id));
    switch(pEvent->event_id) {
        case A2DP_SINK_API_CONNECT_REQ:
            bdaddr_to_string(&iter->mConnectedDevice, str, 18);
            fprintf(stdout, "A2DP Sink Connected to %s\n", str);
            break;
        case A2DP_SINK_API_DISCONNECT_REQ:
            if (!memcmp(&pA2dpSinkSplit->mStreamingDevice, &iter->mDevice, sizeof(bt_bdaddr_t)))
            {
                //release control
                pReleaseControlReq = new BtEvent;
                pReleaseControlReq->btamControlRelease.event_id = BT_AM_RELEASE_CONTROL;
                pReleaseControlReq->btamControlRelease.profile_id = PROFILE_ID_A2DP_SINK;
                PostMessage(THREAD_ID_BT_AM, pReleaseControlReq);
            }
            bdaddr_to_string(&iter->mConnectedDevice, str, 18);
            fprintf(stdout, "A2DP Sink DisConnecting from %s\n", str);
            memset(&iter->mConnectedDevice, 0, sizeof(bt_bdaddr_t));
            memset(&iter->mConnectingDevice, 0, sizeof(bt_bdaddr_t));

            if (sBtA2dpSinkInterface != NULL) {
                sBtA2dpSinkInterface->disconnect(iter->mDevice);
            }
            change_state(iter, DEVICE_STATE_PENDING);
            break;
        case A2DP_SINK_AUDIO_START_REQ:
            ALOGD(LOGTAG "A2DP_SINK_AUDIO_START_REQ");
            sBtA2dpSinkVendorInterface->start_req(iter->mDevice);
            break;
        case A2DP_SINK_AUDIO_SUSPEND_REQ:
        ALOGD(LOGTAG "A2DP_SINK_AUDIO_SUSPEND_REQ");
            sBtA2dpSinkVendorInterface->suspend_req(iter->mDevice);
            break;
        case A2DP_SINK_DISCONNECTED_CB:
            if (!memcmp(&pA2dpSinkSplit->mStreamingDevice, &iter->mDevice, sizeof(bt_bdaddr_t)))
            {
                //release control
                pReleaseControlReq = new BtEvent;
                pReleaseControlReq->btamControlRelease.event_id = BT_AM_RELEASE_CONTROL;
                pReleaseControlReq->btamControlRelease.profile_id = PROFILE_ID_A2DP_SINK;
                PostMessage(THREAD_ID_BT_AM, pReleaseControlReq);
            }
            memset(&iter->mConnectedDevice, 0, sizeof(bt_bdaddr_t));
            memset(&iter->mConnectingDevice, 0, sizeof(bt_bdaddr_t));
            fprintf(stdout, "A2DP Sink DisConnected \n");
            change_state(iter, DEVICE_STATE_DISCONNECTED);
            break;
        case A2DP_SINK_DISCONNECTING_CB:
            if (!memcmp(&pA2dpSinkSplit->mStreamingDevice, &iter->mDevice, sizeof(bt_bdaddr_t)))
            {
                //release control
                pReleaseControlReq = new BtEvent;
                pReleaseControlReq->btamControlRelease.event_id = BT_AM_RELEASE_CONTROL;
                pReleaseControlReq->btamControlRelease.profile_id = PROFILE_ID_A2DP_SINK;
                PostMessage(THREAD_ID_BT_AM, pReleaseControlReq);
            }
            fprintf(stdout, "A2DP Sink DisConnecting\n");
            change_state(iter, DEVICE_STATE_PENDING);
            break;
        case A2DP_SINK_CODEC_CONFIG:
            iter->dev_codec_type = pEvent->a2dpSinkEvent.arg1;
            if (pEvent->a2dpSinkEvent.buf_ptr == NULL) {
                break;
            }
            memcpy(&iter->dev_codec_config, pEvent->a2dpSinkEvent.buf_ptr,
                    pEvent->a2dpSinkEvent.buf_size);
            osi_free(pEvent->a2dpSinkEvent.buf_ptr);
            memcpy(&iter->mDevice, &pEvent->a2dpSinkEvent.bd_addr, sizeof(bt_bdaddr_t));
            bdaddr_to_string(&iter->mDevice, str, 18);
            fprintf(stdout, "Codec Configuration for device %s\n", str);
            switch (iter->dev_codec_type) {
                case A2DP_SINK_AUDIO_CODEC_SBC:
                    fprintf(stdout, "Codec type = SBC\n");
                    iter->av_config.sample_rate = get_a2dp_sbc_sampling_rate(iter->dev_codec_config
                        .sbc_config.samp_freq);
                    iter->av_config.channel_count = get_a2dp_sbc_channel_mode(iter->dev_codec_config
                        .sbc_config.ch_mode);
                    break;
                case A2DP_SINK_AUDIO_CODEC_MP3:
                    fprintf(stdout, "Codec type = MP3\n");
                    iter->av_config.sample_rate = get_a2dp_mp3_sampling_rate(iter->dev_codec_config
                        .mp3_config.sampling_freq);
                    iter->av_config.channel_count = get_a2dp_mp3_channel_mode(iter->dev_codec_config
                        .mp3_config.channel_count);
                    break;
                case A2DP_SINK_AUDIO_CODEC_AAC:
                    fprintf(stdout, "Codec type = AAC\n");
                    iter->av_config.sample_rate = get_a2dp_aac_sampling_rate(iter->dev_codec_config
                        .aac_config.sampling_freq);
                    iter->av_config.channel_count = get_a2dp_aac_channel_mode(iter->dev_codec_config
                        .aac_config.channel_count);
                    break;
                case A2DP_SINK_AUDIO_CODEC_APTX:
                    fprintf(stdout, "Codec type = APTX\n");
                    iter->av_config.sample_rate = get_a2dp_aptx_sampling_rate(iter->dev_codec_config
                        .aptx_config.sampling_freq);
                    iter->av_config.channel_count = get_a2dp_aptx_channel_mode(iter->dev_codec_config
                        .aptx_config.channel_count);
                    break;
                case A2DP_SINK_AUDIO_CODEC_APTX_HD:
                    fprintf(stdout, "Codec type = APTX HD\n");
                    iter->av_config.sample_rate = get_a2dp_aptx_hd_sampling_rate(iter->dev_codec_config
                        .aptx_hd_config.sampling_freq);
                    iter->av_config.channel_count = get_a2dp_aptx_hd_channel_mode(iter->dev_codec_config
                        .aptx_hd_config.channel_count);
                    break;
                case A2DP_SINK_AUDIO_CODEC_APTX_AD:
                    fprintf(stdout, "Codec type = APTX AD\n");
                    iter->av_config.sample_rate = get_a2dp_aptx_ad_sampling_rate(iter->dev_codec_config
                         .aptx_config.sampling_freq);
                    iter->av_config.channel_count = get_a2dp_aptx_ad_channel_mode(iter->dev_codec_config
                         .aptx_config.channel_count);
                    break;
                default:
                    is_valid_codec = false;
                    ALOGE(LOGTAG " Invalid codec type %d ", iter->dev_codec_type);
                    break;
             }
            if (is_valid_codec) {
                fprintf(stdout, "Sample Rate = %d\n", iter->av_config.sample_rate);
                fprintf(stdout, "Channel Mode = %d\n", iter->av_config.channel_count);
            }
            break;
        case A2DP_SINK_AUDIO_STARTED:
        case A2DP_SINK_FOCUS_REQUEST_CB:
            bdaddr_to_string(&pA2dpSinkSplit->mStreamingDevice, str, 18);
            ALOGD(LOGTAG " current streaming device %s", str);

            if (!(bdaddr_is_empty(&pA2dpSinkSplit->mStreamingDevice)) &&
                    memcmp(&pA2dpSinkSplit->mStreamingDevice, &iter->mDevice, sizeof(bt_bdaddr_t)))
            {
                ALOGD(LOGTAG " another dev started streaming, pause previous one");
                if (pAvrcp != NULL)
                    pAvrcp->SendPassThruCommandNative(CMD_ID_PAUSE,
                            &pA2dpSinkSplit->mStreamingDevice, 1);
                    // release control
                    pReleaseControlReq = new BtEvent;
                    pReleaseControlReq->btamControlRelease.event_id = BT_AM_RELEASE_CONTROL;
                    pReleaseControlReq->btamControlRelease.profile_id = PROFILE_ID_A2DP_SINK;
                    PostMessage(THREAD_ID_BT_AM, pReleaseControlReq);
            }
            ALOGE(LOGTAG " updating avconfig parameters for this device");

            memcpy(&pA2dpSinkSplit->mStreamingDevice, &pEvent->a2dpSinkEvent.bd_addr,
                    sizeof(bt_bdaddr_t));
            bdaddr_to_string(&pA2dpSinkSplit->mStreamingDevice, str, 18);
            ALOGD(LOGTAG " A2DP_SINK_AUDIO_STARTED - set current streaming device as %s", str);

            sBtA2dpSinkVendorInterface->
                    update_streaming_device_vendor(&pA2dpSinkSplit->mStreamingDevice);

            ALOGD(LOGTAG " BT_AM_REQUEST_CONTROL");
            pControlRequest = new BtEvent;
            pControlRequest->btamControlReq.event_id = BT_AM_REQUEST_CONTROL;
            pControlRequest->btamControlReq.profile_id = PROFILE_ID_A2DP_SINK;
            pControlRequest->btamControlReq.request_type = REQUEST_TYPE_PERMANENT;
            //TODO: check why this is causing crash
            //PostMessage(THREAD_ID_BT_AM, pControlRequest);
            break;
        case A2DP_SINK_AUDIO_SUSPENDED:
        case A2DP_SINK_AUDIO_STOPPED:
            if(memcmp(&pA2dpSinkSplit->mStreamingDevice, &pEvent->a2dpSinkEvent.bd_addr,
                    sizeof(bt_bdaddr_t)))
            {
                ALOGD(LOGTAG " A2DP_SINK_AUDIO_SUSPENDED/STOPPED for non streaming device, ignore");
                break;
            }
            memset(&pA2dpSinkSplit->mStreamingDevice, 0, sizeof(bt_bdaddr_t));

            if (pA2dpSinkSplit && pA2dpSinkSplit->controlStatus != STATUS_LOSS_TRANSIENT) {
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

void A2dp_Sink_Split::change_state(list<A2dp_Device>::iterator iter, A2dpSinkDeviceState mState) {
   BtEvent *pA2dpSinkSplitDisconnected = NULL;
   ALOGD(LOGTAG " current State = %d, new state = %d", iter->mSinkDeviceState, mState);
   pthread_mutex_lock(&lock);
   iter->mSinkDeviceState = mState;
   if (iter->mSinkDeviceState == DEVICE_STATE_DISCONNECTED)
   {
       if (!memcmp(&pA2dpSinkSplit->mStreamingDevice, &iter->mDevice, sizeof(bt_bdaddr_t)))
       {
           memset(&pA2dpSinkSplit->mStreamingDevice, 0, sizeof(bt_bdaddr_t));
       }
       pA2dpDeviceList.erase(iter);
       ALOGD(LOGTAG " iter %x deleted from list", iter);
   }
   ALOGD(LOGTAG " iter %x state changes to %d ", iter, mState);
   pthread_mutex_unlock(&lock);
}

A2dp_Sink_Split :: A2dp_Sink_Split(const bt_interface_t *bt_interface, config_t *config) {
    controlStatus = STATUS_LOSS;
    this->bluetooth_interface = bt_interface;
    this->config = config;
    sBtA2dpSinkInterface = NULL;
    mSinkState = SINK_STATE_NOT_STARTED;
    pthread_mutex_init(&this->lock, NULL);
    max_a2dp_conn = 0;
    start_pending = false;
    suspend_pending = false;
    memset(&mStreamingDevice, 0, sizeof(bt_bdaddr_t));
    memset(&mResumingDevice, 0, sizeof(bt_bdaddr_t));
    memset(&mPendingDevice, 0, sizeof(bt_bdaddr_t));
}

A2dp_Sink_Split :: ~A2dp_Sink_Split() {
    pthread_mutex_destroy(&lock);
}
