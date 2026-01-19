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

#include <iostream>
#include <string.h>
#include <hardware/bluetooth.h>
#include <hardware/hardware.h>
#include <hardware/bt_av.h>
#include <hardware/bt_rc.h>
#include <list>
#include <map>
#include <dlfcn.h>
#include "Avrcp.hpp"
#include "A2dp_Src.hpp"
#include "Gap.hpp"
#include "hardware/bt_av_vendor.h"
#include "hardware/bt_rc_vendor.h"
#include <math.h>
#include <algorithm>
#include "osi/include/properties.h"
#include "osi/include/list.h"
#include "osi/include/allocator.h"
#include "A2dp_Sink_Streaming.hpp"
#include "audio_a2dp_hw/include/audio_a2dp_hw.h"
#include <mutex>

#define LOGTAG_A2DP "A2DP_SRC "
#define LOGTAG_AVRCP "AVRCP_TG "
#define LOGTAG "A2DP_SRC_PA "
using namespace std;
using std::list;
using std::string;

extern Avrcp *pAvrcp;
extern A2dp_Sink_Streaming *pA2dpSinkStream;
A2dp_Source *pA2dpSource = NULL;
static pthread_t playback_thread = NULL;
AttrType mAttrType;
bool media_playing = false;
bool use_bigger_metadata = false;
btrc_play_status_t playStatus = BTRC_PLAYSTATE_STOPPED;
btrc_notification_type_t mPlayStatusNotiType = BTRC_NOTIFICATION_TYPE_CHANGED;
btrc_notification_type_t mTrackChangeNotiType = BTRC_NOTIFICATION_TYPE_CHANGED;
btrc_notification_type_t mAddrPlayerChangedNotiType = BTRC_NOTIFICATION_TYPE_CHANGED;
btrc_notification_type_t mAvailPlayerChangedNotiType = BTRC_NOTIFICATION_TYPE_CHANGED;
btrc_notification_type_t mUidChangedNotiType = BTRC_NOTIFICATION_TYPE_CHANGED;
btrc_notification_type_t mNowPlayingContentChangedNotiType = BTRC_NOTIFICATION_TYPE_CHANGED;
btrc_notification_type_t mPlayPosChangedNotiType = BTRC_NOTIFICATION_TYPE_CHANGED;
btrc_notification_type_t mAppSettingChangedNotiType = BTRC_NOTIFICATION_TYPE_CHANGED;


uint8_t mfolder_depth = 0;
bool is_empty_folder = 0;
bool is_search_req_recieved = false;
btrc_br_folder_name_t* mp_folders = nullptr;
uint8_t rootUid[BTRC_UID_SIZE] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x04};
uint8_t folderUid1[BTRC_UID_SIZE] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03};
uint8_t folderUid2[BTRC_UID_SIZE] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02};
uint8_t mediaUid1[BTRC_UID_SIZE] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01};
uint8_t mediaUid2[BTRC_UID_SIZE] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02};
uint8_t mediaUid3[BTRC_UID_SIZE] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x05};


char* rootString = "root";
int rootStringLength = 4;
static int ATTRIBUTE_NOTSUPPORTED = -1;

static int ATTRIBUTE_EQUALIZER = 1;
static int ATTRIBUTE_REPEATMODE = 2;
static int ATTRIBUTE_SHUFFLEMODE = 3;
static int ATTRIBUTE_SCANMODE = 4;
static int NUMPLAYER_ATTRIBUTE = 4;

uint8_t default_eq_value = BTRC_PLAYER_VAL_OFF_EQUALIZER;
uint8_t default_repeat_value = BTRC_PLAYER_VAL_OFF_REPEAT;
uint8_t default_shuffle_value = BTRC_PLAYER_VAL_OFF_SHUFFLE;
uint8_t default_scan_value = BTRC_PLAYER_VAL_OFF_SCAN;

uint32_t a2dp_play_position = 10;
static uint32_t a2dp_playstatus = A2DP_SOURCE_AUDIO_STOPPED;
long NO_TRACK_SELECTED = -1L;
long TRACK_IS_SELECTED = 0L;
long mCurrentTrackID = NO_TRACK_SELECTED;

int DEFAULT_PLAYER_ADDRESSED = 1;//Always addressed player is 1(musicplayer1 player id)
int mCurrentAddressedPlayer = DEFAULT_PLAYER_ADDRESSED;

static uint16_t MTU_src;
static uint16_t sequence_number;
static uint32_t timestamp;

int mCurrentEqualizer = default_eq_value;
int mCurrentRepeat = default_repeat_value;
int mCurrentShuffle = default_shuffle_value;
int mCurrentScan = default_scan_value;

static btav_codec_config_t src_codec_cfg;
static uint16_t src_codec_type;

#define AVRCP_MAX_VOL 127
int mAudioStreamMax = 15;
bool is_sink_relay_enabled = false;
bool bt_a2dp_split_enabled = false;

struct a2dp_stream_common {
  std::recursive_mutex* mutex;  // See note below on mutex acquisition order.
  int ctrl_fd;
  int audio_fd;
  size_t buffer_sz;
  struct a2dp_config cfg;
  a2dp_state_t state;
  tA2DP_LATENCY sink_latency;
  uint8_t codec_cfg[MAX_CODEC_CFG_SIZE];
};


typedef enum
{
    SRC_STREAMING,
    SRC_NO_STREAMING,
}SrcStreamStatus;

#define AUDIO_STREAM_OUTPUT_BUFFER_SZ      (20*512)
#define INVALID_CODEC    -1
#define NON_A2DP_MEDIA_CT    0xFF
#define DEBUGPRINTBIT
#ifdef DEBUGPRINTBIT
#define PRINTBIT(s,num)   do{ ALOGD("IN Function %s The content of %s:",__func__,#s);\
                              for(int i=0;i<num;i++) ALOGD(" %hhu",*((uint8_t*)(s)+i));}while(0)
#else
#define PRINTBIT(s,num)
#endif

typedef struct
{
    uint16_t codec_type;
    uint16_t len;
    uint16_t offset;
} t_SINK_RELAY_DATA;
list_t *a2dp_sink_relay_data_list;
static pthread_mutex_t a2dp_sink_relay_mutex = PTHREAD_MUTEX_INITIALIZER;
extern bool GetCodecInfoByAddr(bt_bdaddr_t* bd_addr, uint16_t *dev_codec_type, btav_codec_config_t* codec_config);
#ifdef __cplusplus
extern "C" {
#endif

typedef struct {

    uint8_t nrof_blocks;   /**< The block size used to encode the stream. Input parameter. */
    uint8_t blocks;


    uint8_t nrof_subbands; /**< The number of subbands of the encoded stream. Input parameter. */
    uint8_t subbands;

    uint8_t mode;          /**< The mode of the encoded channel. Input parameter. */
    uint8_t nrof_channels; /**< The number of channels of the encoded stream. */

    uint8_t bitpool;       /**< Size of the bit allocation pool used to encode the stream. Input parameter. */

} A2DP_SBC_FRAME;

  #define SBC_HEADER_LEN 4
  #define SBC_SYNCWORD 0x9c

#define SBC_MONO 0         /**< The mode of the A2DP channel is mono*/
#define SBC_DUAL_CHANNEL 1 /**< The mode of the A2DP channel is dual-channel*/
#define SBC_STEREO 2       /**< The mode of the A2DP channel is stereo*/
#define SBC_JOINT_STEREO 3 /**< The mode of the A2DP channel is joint stereo*/

#ifndef _ARRAYSIZE
#define _ARRAYSIZE(a) (sizeof(a)/sizeof(a[0]))
#endif

/**
 * Maximum argument length
 */
#define COMMAND_ARG_SIZE     200

static std::vector<btav_a2dp_codec_config_t> a2dpSrcCodecList;

#define SBC_MIN_BITPOOL      2
#define SBC_MAX_BITPOOL      250
#define SBC_PARAM_LEN 8
#define NON_SBC_PARAM_LEN 3

#define A2DP_AAC_MIN_BITRATE 64000       // 64 kbps

static const char * valid_codecs[] = {
    "sbc",
    "aac",
    "aptx",
    "aptx_hd",
    "ldac"
};

static uint8_t valid_codec_values[] = {
  BTAV_A2DP_CODEC_INDEX_SOURCE_SBC,
  BTAV_A2DP_CODEC_INDEX_SOURCE_AAC,
  BTAV_A2DP_CODEC_INDEX_SOURCE_APTX,
  BTAV_A2DP_CODEC_INDEX_SOURCE_APTX_HD,
  BTAV_A2DP_CODEC_INDEX_SOURCE_APTX_ADAPTIVE,
  BTAV_A2DP_CODEC_INDEX_SOURCE_LDAC,
};

static const char * valid_freq[] = {
    "16",
    "32",
    "44.1",
    "48",
    "88.2",
    "96",
    "176.4",
    "192",
};

static uint16_t valid_freq_values[] = {
  BTAV_A2DP_CODEC_SAMPLE_RATE_16000,
  BTAV_A2DP_CODEC_SAMPLE_RATE_32000,
  BTAV_A2DP_CODEC_SAMPLE_RATE_44100,
  BTAV_A2DP_CODEC_SAMPLE_RATE_48000,
  BTAV_A2DP_CODEC_SAMPLE_RATE_88200,
  BTAV_A2DP_CODEC_SAMPLE_RATE_96000,
  BTAV_A2DP_CODEC_SAMPLE_RATE_176400,
  BTAV_A2DP_CODEC_SAMPLE_RATE_192000
};

static const char * valid_bits_per_sample[] = {
    "16",
    "24",
    "32",
};

static uint8_t valid_bits_per_sample_values[] = {
   BTAV_A2DP_CODEC_BITS_PER_SAMPLE_16,
   BTAV_A2DP_CODEC_BITS_PER_SAMPLE_24,
   BTAV_A2DP_CODEC_BITS_PER_SAMPLE_32,
};

static const char * valid_channel[] = {
    "mono",
    "dual",
    "stereo",
    "joint",
};

static uint8_t valid_channel_values[] = {
  BTAV_A2DP_CODEC_CHANNEL_MODE_MONO,
  BTAV_A2DP_CODEC_CHANNEL_MODE_DUAL,
  BTAV_A2DP_CODEC_CHANNEL_MODE_STEREO,
  BTAV_A2DP_CODEC_CHANNEL_MODE_JOINT,
};

static const char * valid_sbc_blocks[] = {
    "4",
    "8",
    "12",
    "16",
};

static uint8_t valid_sbc_blocks_values[] = {
    SBC_BLOCKS_4,
    SBC_BLOCKS_8,
    SBC_BLOCKS_12,
    SBC_BLOCKS_16,
};

static const char * valid_sbc_subbands[] = {
    "4",
    "8",
};

static uint8_t valid_sbc_subbands_values[] = {
    SBC_SUBBAND_4,
    SBC_SUBBAND_8,
};

static const char * valid_sbc_allocation[] = {
    "snr",
    "loud",
};

static uint8_t valid_sbc_allocation_values[] = {
    SBC_ALLOC_SNR,
    SBC_ALLOC_LOUDNESS,
};

static const char * valid_sbc_bitpool[] = {
    "2 - 250",
};


static const char * valid_vbrSupported[] = {
    "0",
    "128",
};

/******************************************************************************
 * This structure defines the A2DP Sink variable.
 */
typedef struct {
    const char *name;            /**< Run-time variable name */
    const char *description;     /**< Run-time variable description */
    const char **valid_options;  /**< List of valid variable values */
    int valid_option_cnt;        /**< Size of valid value list */
} A2DP_SRC_VARIABLE;

/******************************************************************************
 * List of A2DP Sink variables.
 */
const A2DP_SRC_VARIABLE variable_list[] = {
    { "codec type", "Valid Codec Type to Use",
      valid_codecs, _ARRAYSIZE(valid_codecs) },
    { "freq", "Valid Freq to Use",
      valid_freq, _ARRAYSIZE(valid_freq) },
    { "bitspersample", "Valid bitspersample to Use",
      valid_bits_per_sample, _ARRAYSIZE(valid_bits_per_sample) },
    { "channels", "Valid Channels to Use",
      valid_channel, _ARRAYSIZE(valid_channel) },
    { "sbc blocks", "Valid SBC Blocks to Use",
      valid_sbc_blocks, _ARRAYSIZE(valid_sbc_blocks) },
    { "sbc subbands", "Valid SBC Subbands to Use",
      valid_sbc_subbands, _ARRAYSIZE(valid_sbc_subbands) },
    { "sbc allocation", "Valid SBC Allocation to Use",
      valid_sbc_allocation, _ARRAYSIZE(valid_sbc_allocation) },
    { "sbc bitpool", "Valid SBC Bitpool to Use",
      valid_sbc_bitpool, _ARRAYSIZE(valid_sbc_bitpool) },
    { "aac vbr support", "Valid vbr support to Use",
      valid_vbrSupported, _ARRAYSIZE(valid_vbrSupported) },
};

/******************************************************************************
 *
 * Basic utilities.
 *
 */

 bool uid_cmp(uint8_t* a, uint8_t*b){
    return memcmp(a, b, BTRC_UID_SIZE);
}

#ifndef isdelimiter
#define isdelimiter(c) ((c) == ' ' || (c) == ',' || (c) == '\f' || (c) == '\n' || \
        (c) == '\r' || (c) == '\t' || (c) == '\v')
#endif

int StrcompareInsensitive(char const *p1,  char const *p2){
    if((p1 != NULL) && (p2 != NULL)){
      for (;;) {
        char uc1 = std::toupper(*p1);
        char uc2 = std::toupper(*p2);
        if (uc1 < uc2) return -1;
        if (uc1 > uc2) return 1;
        if (uc1 == '\0') return 0;
        p1++;
        p2++;
      }
    }
}

static int find_str_in_list(const char *str, const char * const *list,
                                   int list_size)
{
    int i;
    int item = list_size;
    int match_cnt = 0;
    int len;

    if (str == NULL || list == NULL || list_size <= 0)
        return -1;


    for (i = 0; i < list_size; i++) {
        if (!StrcompareInsensitive(list[i], str)) {
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

static void print_help(const A2DP_SRC_VARIABLE *var)
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

    while (*data && isdelimiter(*data)) {
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
        ALOGE(LOGTAG_A2DP " %s ", output[param_count -1]);
    }

    while ((temp_arg = strtok_r(NULL, delim, &ptr1))) {
        strlcpy(output[param_count], temp_arg, COMMAND_ARG_SIZE);
        output[param_count ++][COMMAND_ARG_SIZE - 1] = '\0';
        ALOGE(LOGTAG_A2DP " %s ", output[param_count -1]);
    }

    ALOGE(LOGTAG_A2DP " %s: returning %d \n", __func__, param_count);
    return param_count;
}

bool compareByPriority(const btav_a2dp_codec_config_t &a, const btav_a2dp_codec_config_t &b){
    return a.codec_priority > b.codec_priority;
}


void assignCodecConfigPriorities(int *priority_values, int numConfigs){
for (int i = 0; i < numConfigs; i++) {

btav_a2dp_codec_config_t codec_config = {
        .codec_type = static_cast<btav_a2dp_codec_index_t>(i),
        .codec_priority = static_cast<btav_a2dp_codec_priority_t>(priority_values[i]),
        .sample_rate = BTAV_A2DP_CODEC_SAMPLE_RATE_NONE,
        .bits_per_sample = BTAV_A2DP_CODEC_BITS_PER_SAMPLE_NONE,
        .channel_mode = BTAV_A2DP_CODEC_CHANNEL_MODE_NONE,
        .codec_specific_1 = 0,
        .codec_specific_2 = 0,
        .codec_specific_3 = 0,
        .codec_specific_4 = 0,
        .codec_specific_5 = 0,
        };

    a2dpSrcCodecList.push_back(codec_config);
}
std::sort(a2dpSrcCodecList.begin(), a2dpSrcCodecList.end(), compareByPriority);
}

static const char* pass_through_cmd_id_to_str(uint8_t cmd_id) {
    switch (cmd_id) {
        case CMD_ID_PLAY:
                return "CMD_ID_PLAY";
        case CMD_ID_PAUSE:
                return "CMD_ID_PAUSE";
        case CMD_ID_STOP:
                return "CMD_ID_STOP";
        case CMD_ID_REWIND:
                return "CMD_ID_REWIND";
        case CMD_ID_FORWARD:
                return "CMD_ID_FORWARD";
        case CMD_ID_BACKWARD:
                return "CMD_ID_BACKWARD";
        case CMD_ID_FF:
                return "CMD_ID_FASTFORWARD";
        case CMD_ID_VOL_DOWN:
                return "CMD_ID_VOL_DOWN";
        case CMD_ID_VOL_UP:
                return "CMD_ID_VOL_UP";
        default:
                return "unknown";
    }
}

/* This function is used for testing purpose. Parses string which represents codec list*/
static bool A2dpCodecList(char *codec_param_list, int *num_codec_configs){
    int i = 0, j = 0, k = 0;
    char output_list[COMMAND_ARG_SIZE][COMMAND_ARG_SIZE];
    int codec_params_list_size;
    int codec_prio = BTAV_A2DP_CODEC_PRIORITY_HIGHEST;
    a2dpSrcCodecList.clear();

    if (*codec_param_list == '\0') {
        ALOGE(LOGTAG_A2DP " codec list cannot be set to nothing \n");
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
        btav_a2dp_codec_config_t codec_config;
        codec_config.codec_type = static_cast<btav_a2dp_codec_index_t>(valid_codec_values[i]);
        switch (codec_config.codec_type) {
            case BTAV_A2DP_CODEC_INDEX_SOURCE_SBC:
                /* check number of parameters passed are ok or not */
                if (j + SBC_PARAM_LEN > codec_params_list_size + 1) {
                    fprintf(stdout, "Invalid SBC Parameters passed\n");
                    return false;
                }
                i = find_str_in_list(output_list[j], valid_freq,
                    _ARRAYSIZE(valid_freq));
                if (i >= _ARRAYSIZE(valid_freq)) {
                    fprintf(stdout, "Invalid SBC Sampling Freq: %s\n", output_list[j]);
                    print_help(&variable_list[1]);
                    return false;
                }
                codec_config.sample_rate = static_cast<btav_a2dp_codec_sample_rate_t>(valid_freq_values[i]);
                j++;
                i = find_str_in_list(output_list[j], valid_bits_per_sample,
                    _ARRAYSIZE(valid_bits_per_sample));
                if (i >= _ARRAYSIZE(valid_bits_per_sample)) {
                    fprintf(stdout, "Invalid SBC Bits per sample: %s\n", output_list[j]);
                    print_help(&variable_list[2]);
                    return false;
                }
                codec_config.bits_per_sample = static_cast<btav_a2dp_codec_bits_per_sample_t>(valid_bits_per_sample_values[i]);
                j++;
                i = find_str_in_list(output_list[j], valid_channel,
                    _ARRAYSIZE(valid_channel));
                if (i >= _ARRAYSIZE(valid_channel)) {
                    fprintf(stdout, "Invalid SBC Channels: %s\n", output_list[j]);
                    print_help(&variable_list[3]);
                    return false;
                }
                codec_config.channel_mode = static_cast<btav_a2dp_codec_channel_mode_t>(valid_channel_values[i]);
                j++;
                i = find_str_in_list(output_list[j], valid_sbc_blocks,
                    _ARRAYSIZE(valid_sbc_blocks));
                if (i >= _ARRAYSIZE(valid_sbc_blocks)) {
                    fprintf(stdout, "Invalid SBC Blocks: %s\n", output_list[j]);
                    print_help(&variable_list[4]);
                    return false;
                }
                codec_config.codec_specific_1 = valid_sbc_blocks_values[i];
                j++;
                i = find_str_in_list(output_list[j], valid_sbc_subbands,
                    _ARRAYSIZE(valid_sbc_subbands));
                if (i >= _ARRAYSIZE(valid_sbc_subbands)) {
                    fprintf(stdout, "Invalid SBC Sub bands: %s\n", output_list[j]);
                    print_help(&variable_list[5]);
                    return false;
                }
                codec_config.codec_specific_2 = valid_sbc_subbands_values[i];
                j++;
                i = find_str_in_list(output_list[j], valid_sbc_allocation,
                    _ARRAYSIZE(valid_sbc_allocation));
                if (i >= _ARRAYSIZE(valid_sbc_allocation)) {
                    fprintf(stdout, "Invalid SBC Allocation Mode: %s\n", output_list[j]);
                    print_help(&variable_list[6]);
                    return false;
                }
                codec_config.codec_specific_3 = valid_sbc_allocation_values[i];
                j++;
                codec_config.codec_specific_4 = atoi (output_list[j++]);
                ALOGD(LOGTAG_A2DP "Min Bitool %d", codec_config.codec_specific_4);
                if (codec_config.codec_specific_4 < SBC_MIN_BITPOOL ||
                    codec_config.codec_specific_4 > SBC_MAX_BITPOOL) {
                    fprintf(stdout, "Invalid SBC Max bitpool %s\n",
                        output_list[j - 1]);
                    print_help(&variable_list[7]);
                    return false;
                }
                codec_config.codec_specific_5 = atoi (output_list[j++]);
                ALOGD(LOGTAG_A2DP "Max Bitool %d", codec_config.codec_specific_5);
                if (codec_config.codec_specific_5 < SBC_MIN_BITPOOL ||
                    codec_config.codec_specific_5 > SBC_MAX_BITPOOL) {
                    fprintf(stdout, "Invalid SBC Min bitpool %s\n", output_list[j - 1]);
                    print_help(&variable_list[7]);
                    return false;
                }
                break;
            case BTAV_A2DP_CODEC_INDEX_SOURCE_AAC:
                /* check number of parameters passed are ok or not */
                if (j + NON_SBC_PARAM_LEN > codec_params_list_size + 1) {
                    fprintf(stdout, "Invalid Codec Parameters passed\n");
                    return false;
                }
                i = find_str_in_list(output_list[j], valid_freq,
                    _ARRAYSIZE(valid_freq));
                if (i >= _ARRAYSIZE(valid_freq)) {
                    fprintf(stdout, "Invalid %s codec Sampling Freq: %s\n", valid_codecs[codec_config.codec_type], output_list[j]);
                    print_help(&variable_list[1]);
                    return false;
                }
                codec_config.sample_rate = static_cast<btav_a2dp_codec_sample_rate_t>(valid_freq_values[i]);
                j++;
                i = find_str_in_list(output_list[j], valid_bits_per_sample,
                    _ARRAYSIZE(valid_bits_per_sample));
                if (i >= _ARRAYSIZE(valid_bits_per_sample)) {
                    fprintf(stdout, "Invalid %s Bits per sample: %s\n", valid_codecs[codec_config.codec_type], output_list[j]);
                    print_help(&variable_list[2]);
                    return false;
                }
                codec_config.bits_per_sample = static_cast<btav_a2dp_codec_bits_per_sample_t>(valid_bits_per_sample_values[i]);
                j++;
                i = find_str_in_list(output_list[j], valid_channel,
                    _ARRAYSIZE(valid_channel));
                if (i >= _ARRAYSIZE(valid_channel)) {
                    fprintf(stdout, "Invalid %s codec Channel Mode: %s\n", valid_codecs[codec_config.codec_type], output_list[j]);
                    print_help(&variable_list[3]);
                    return false;
                }
                codec_config.channel_mode = static_cast<btav_a2dp_codec_channel_mode_t>(valid_channel_values[i]);
                j++;
                i = find_str_in_list(output_list[j], valid_vbrSupported,
                    _ARRAYSIZE(valid_vbrSupported));
                if (i >= _ARRAYSIZE(valid_vbrSupported)) {
                    fprintf(stdout, "Invalid %s vbrSupported Mode: %s\n", valid_codecs[codec_config.codec_type], output_list[j]);
                    print_help(&variable_list[8]);
                    return false;
                }
                codec_config.variableBitRateSupport = atoi(output_list[j]);
                j++;
                i = atoi(output_list[j]);
                if (i < A2DP_AAC_MIN_BITRATE) {
                    fprintf(stdout, "Invalid %s bit rate : %s try above 64 kbps \n", valid_codecs[codec_config.codec_type], output_list[j]);
                    return false;
                }
                codec_config.bitRate = i;
                j++;
                break;
            case BTAV_A2DP_CODEC_INDEX_SOURCE_APTX:
            case BTAV_A2DP_CODEC_INDEX_SOURCE_APTX_HD:
            case BTAV_A2DP_CODEC_INDEX_SOURCE_APTX_ADAPTIVE:
            case BTAV_A2DP_CODEC_INDEX_SOURCE_LDAC:
                /* check number of parameters passed are ok or not */
                if (j + NON_SBC_PARAM_LEN > codec_params_list_size + 1) {
                    fprintf(stdout, "Invalid Codec Parameters passed\n");
                    return false;
                }
                i = find_str_in_list(output_list[j], valid_freq,
                    _ARRAYSIZE(valid_freq));
                if (i >= _ARRAYSIZE(valid_freq)) {
                    fprintf(stdout, "Invalid %s codec Sampling Freq: %s\n", valid_codecs[codec_config.codec_type], output_list[j]);
                    print_help(&variable_list[1]);
                    return false;
                }
                codec_config.sample_rate = static_cast<btav_a2dp_codec_sample_rate_t>(valid_freq_values[i]);
                j++;
                i = find_str_in_list(output_list[j], valid_bits_per_sample,
                    _ARRAYSIZE(valid_bits_per_sample));
                if (i >= _ARRAYSIZE(valid_bits_per_sample)) {
                    fprintf(stdout, "Invalid %s Bits per sample: %s\n", valid_codecs[codec_config.codec_type], output_list[j]);
                    print_help(&variable_list[2]);
                    return false;
                }
                codec_config.bits_per_sample = static_cast<btav_a2dp_codec_bits_per_sample_t>(valid_bits_per_sample_values[i]);
                j++;
                i = find_str_in_list(output_list[j], valid_channel,
                    _ARRAYSIZE(valid_channel));
                if (i >= _ARRAYSIZE(valid_channel)) {
                    fprintf(stdout, "Invalid %s codec Channel Mode: %s\n", valid_codecs[codec_config.codec_type], output_list[j]);
                    print_help(&variable_list[3]);
                    return false;
                }
                codec_config.channel_mode = static_cast<btav_a2dp_codec_channel_mode_t>(valid_channel_values[i]);
                j++;
                break;
        }
        codec_config.codec_priority = static_cast<btav_a2dp_codec_priority_t>(codec_prio--);
        a2dpSrcCodecList.push_back(codec_config);
        k++;
        if (k >= MAX_NUM_CODEC_CONFIGS) {
            fprintf(stdout, "num_codec_configs  exceeds max number(%d) = %d\n",
                k, MAX_NUM_CODEC_CONFIGS);
            return false;
        }
    }
    *num_codec_configs = k;
    fprintf(stdout, "num_codec_configs in the codec list  = %d\n", *num_codec_configs);
    return true;
}

static bool aptxad_mode_change(char *codec_param_list, int *num_codec_configs)
{
    int aptx_mode = atoi(codec_param_list);
    char output_list[COMMAND_ARG_SIZE][COMMAND_ARG_SIZE];
    int codec_params_list_size;
    std::vector<btav_a2dp_codec_config_t>::iterator cp;

    if (*codec_param_list == '\0') {
        ALOGE(LOGTAG_A2DP " APTX AD mode not specified \n");
        fprintf(stdout, "APTX AD mode not specified \n");
        print_help(&variable_list[0]);
        return false;
    }

    codec_params_list_size = ParseUserInput(codec_param_list, output_list);

    if (aptx_mode != 0 && aptx_mode != 1 || codec_params_list_size > 1) {
        ALOGE(LOGTAG_A2DP " APTX AD mode can be only HQ or LL, aptx_mode = %d \n", aptx_mode);
        fprintf(stdout, "APTX AD mode can be only HQ or LL \n");
        return false;
    }

    *num_codec_configs = a2dpSrcCodecList.size();
    for (cp = a2dpSrcCodecList.begin(); cp < a2dpSrcCodecList.end(); cp++) {
        if (cp->codec_type == BTAV_A2DP_CODEC_INDEX_SOURCE_APTX_ADAPTIVE) {
            switch (aptx_mode) {
                case 0:
                     cp->codec_specific_4 = 0x1000;
                     cp->codec_specific_5 = 1;
                     ALOGD(LOGTAG_A2DP " APTX AD mode HQ\n");
                     break;
                case 1:
                     cp->codec_specific_4 = 0x2000;
                     cp->codec_specific_5 = 2;
                     ALOGD(LOGTAG_A2DP " APTX AD mode LL\n");
                     break;
            }
        }
    }
    return true;
}


void registerMediaPlayers () {
    ALOGD(LOGTAG_AVRCP "registerMediaPlayers");

    char* playerName1 = "MusicPlayer1";
    char* Folder1 = "Songs";
    char* Folder2 = "EmptyFolder";
    char* Media1 = "abc1";
    char* Media2 = "abc2";

    char featureMasks[16] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

    featureMasks[5] = featureMasks[5] | 0x01; /*Play*/
    featureMasks[5] = featureMasks[5] | 0x04; /*Pause*/
    featureMasks[5] = featureMasks[5] | 0x02; /*Stop*/
    featureMasks[7] = featureMasks[7] | 0x04; /*Advanced Control Player*/
    featureMasks[7] = featureMasks[7] | 0x08; /*Browsing*/
    featureMasks[7] = featureMasks[7] | 0x10; /*search operation supported*/
    featureMasks[7] = featureMasks[7] | 0x80; /*Only Browsable when addressed*/
    featureMasks[8] = featureMasks[8] | 0x02; /*Now playing*/

    pA2dpSource->pMediaPlayerList.push_back(MediaPlayerInfo (1, 1, 0, 2, 0x006A, 12,
            playerName1, "com.default.music", false, false, BTRC_ITEM_PLAYER, true, featureMasks));

    pA2dpSource->pFolderList.push_back(FolderInfo (folderUid1,
            BTRC_ITEM_FOLDER, 0x00, 0x006A, 5, Folder1));
    pA2dpSource->pFolderList.push_back(FolderInfo (folderUid2,
            BTRC_ITEM_FOLDER, 0x00, 0x006A, 11, Folder2));

    pA2dpSource->pMediaList.push_back(MediaInfo (mediaUid1,
            BTRC_ITEM_MEDIA, 0x006A, 6, Media2, 0));
    pA2dpSource->pMediaList.push_back(MediaInfo (mediaUid2,
            BTRC_ITEM_MEDIA, 0x006A, 6, Media1, 0));

    fprintf(stdout, "register player:%s \n" , playerName1);
    ALOGD(LOGTAG_AVRCP "Exit registerMediaPlayers()");
}

void showMediaItem () {
    list<MediaInfo>::iterator p = pA2dpSource->pMediaList.begin();
    list<MediaInfo>::iterator p_end = pA2dpSource->pMediaList.end();
    while (p != p_end) {
        fprintf(stdout,"%s ",p->mDisplayableName);
        p++;
    }
    fprintf(stdout,"\n");
}

void A2dp_Source::unregisterMediaPlayers () {
    ALOGD(LOGTAG_AVRCP "unregisterMediaPlayers()");
    pMediaPlayerList.clear();
    pFolderList.clear();
    pMediaList.clear();
    ALOGD(LOGTAG_AVRCP "Exit unregisterMediaPlayers()");
}

uint16_t A2DP_SBC_Calculate_FrameLength(A2DP_SBC_FRAME *frame, const uint8_t *data)
{
    uint8_t d1;
    uint16_t nbits,nrof_subbands,result;

    uint16_t freq_values[] =    { 16000, 32000, 44100, 48000 };
    uint8_t block_values[] =    { 4, 8, 12, 16 };
    uint8_t channel_values[] =  { 1, 2, 2, 2 };
    uint8_t band_values[] =     { 4, 8 };

    uint8_t bit_0 = 0x01;
    uint8_t bit_1 = 0x02;
    uint8_t bit_2 = 0x04;
    uint8_t bit_3 = 0x08;
    uint8_t bit_4 = 0x10;
    uint8_t bit_5 = 0x20;
    uint8_t bit_6 = 0x40;
    uint8_t bit_7 = 0x80;

    if(data[0] != SBC_SYNCWORD)
    {
        ALOGE(LOGTAG_A2DP "A2dp src :Error SBC sync word doesn't match");
        return 0;
    }

    d1 = data[1];

    frame->blocks = (d1 & (bit_5 | bit_4)) >> 4;
    frame->nrof_blocks = block_values[frame->blocks];

    frame->mode = (d1 & (bit_3 | bit_2)) >> 2;
    frame->nrof_channels = channel_values[frame->mode];

    frame->subbands = (d1 & bit_0);
    frame->nrof_subbands = band_values[frame->subbands];

    frame->bitpool = data[2];

    nbits = frame->nrof_blocks * frame->bitpool;
    nrof_subbands = frame->nrof_subbands;
    result = nbits;
    if (frame->mode == SBC_JOINT_STEREO) {
        result += nrof_subbands + (8 * nrof_subbands);
    } else {
        if (frame->mode == SBC_DUAL_CHANNEL) { result += nbits; }
        if (frame->mode == SBC_MONO) { result += 4*nrof_subbands; } else { result += 8*nrof_subbands; }
    }
    return SBC_HEADER_LEN + (result + 7) / 8;
}

extern uint8_t get_rtp_offset(uint8_t* p_start, uint16_t codec_type);

void A2dp_Source:: updateResetNotification(btrc_event_id_t noti) {
    ALOGD(LOGTAG_AVRCP "updateResetNotification for %d", noti);
    btrc_register_notification_t param;
    long TrackNumberRsp = -1L;
    switch(noti)
    {
        case BTRC_EVT_PLAY_STATUS_CHANGED:
            if (mPlayStatusNotiType == BTRC_NOTIFICATION_TYPE_INTERIM) {
                mPlayStatusNotiType = BTRC_NOTIFICATION_TYPE_REJECT;
                param.play_status = BTRC_PLAYSTATE_PAUSED;
                sBtAvrcpTargetInterface->register_notification_rsp(BTRC_EVT_PLAY_STATUS_CHANGED,
                        mPlayStatusNotiType, &param);
            }
            break;
        case BTRC_EVT_TRACK_CHANGE:
            if (mTrackChangeNotiType == BTRC_NOTIFICATION_TYPE_INTERIM) {
                mTrackChangeNotiType = BTRC_NOTIFICATION_TYPE_REJECT;
                mCurrentTrackID = TRACK_IS_SELECTED;
                TrackNumberRsp = mCurrentTrackID;
                ALOGD(LOGTAG_AVRCP " TrackNumberRsp = %l", TrackNumberRsp);
                for (int i = 0; i < 8; ++i) {
                    param.track[i] = (uint8_t) (TrackNumberRsp >> (56 - 8 * i));
                }
                sBtAvrcpTargetInterface->register_notification_rsp(BTRC_EVT_TRACK_CHANGE,
                        mTrackChangeNotiType, &param);
            }
            break;
        case BTRC_EVT_PLAY_POS_CHANGED:
            if (mPlayPosChangedNotiType == BTRC_NOTIFICATION_TYPE_INTERIM) {
                mPlayPosChangedNotiType = BTRC_NOTIFICATION_TYPE_REJECT;
                param.song_pos = -1;
                sBtAvrcpTargetInterface->register_notification_rsp(BTRC_EVT_PLAY_POS_CHANGED,
                                mPlayPosChangedNotiType, &param);
            }
            break;
        case BTRC_EVT_APP_SETTINGS_CHANGED:
            if (mAppSettingChangedNotiType == BTRC_NOTIFICATION_TYPE_INTERIM) {
                mAppSettingChangedNotiType = BTRC_NOTIFICATION_TYPE_REJECT;
                param.player_setting.num_attr = NUMPLAYER_ATTRIBUTE;
                param.player_setting.attr_ids[0] = ATTRIBUTE_EQUALIZER;
                param.player_setting.attr_values[0]= BTRC_PLAYER_VAL_OFF_EQUALIZER;
                param.player_setting.attr_ids[1] = ATTRIBUTE_REPEATMODE;
                param.player_setting.attr_values[1] = BTRC_PLAYER_VAL_OFF_REPEAT;
                param.player_setting.attr_ids[2] = ATTRIBUTE_SHUFFLEMODE;
                param.player_setting.attr_values[2] = BTRC_PLAYER_VAL_OFF_SHUFFLE;
                param.player_setting.attr_ids[3] = ATTRIBUTE_SCANMODE;
                param.player_setting.attr_values[3] = BTRC_PLAYER_VAL_OFF_SCAN;
                sBtAvrcpTargetInterface->register_notification_rsp(BTRC_EVT_APP_SETTINGS_CHANGED,
                                           mAppSettingChangedNotiType, &param);
            }
            break;
        case BTRC_EVT_NOW_PLAYING_CONTENT_CHANGED:
            if (mNowPlayingContentChangedNotiType == BTRC_NOTIFICATION_TYPE_INTERIM) {
                mNowPlayingContentChangedNotiType = BTRC_NOTIFICATION_TYPE_REJECT;
                sBtAvrcpTargetInterface->register_notification_rsp(BTRC_EVT_NOW_PLAYING_CONTENT_CHANGED,
                                mNowPlayingContentChangedNotiType, &param);
            }
            break;
        default:
            ALOGD(LOGTAG_AVRCP "Invalid Noti");
            break;
    }
}

void resetAndSendPlayerStatusReject() {
    ALOGD(LOGTAG_A2DP "resetAndSendPlayerStatusReject");
    pA2dpSource->updateResetNotification(BTRC_EVT_PLAY_STATUS_CHANGED);
    pA2dpSource->updateResetNotification(BTRC_EVT_TRACK_CHANGE);
    pA2dpSource->updateResetNotification(BTRC_EVT_PLAY_POS_CHANGED);
    pA2dpSource->updateResetNotification(BTRC_EVT_APP_SETTINGS_CHANGED);
    pA2dpSource->updateResetNotification(BTRC_EVT_NOW_PLAYING_CONTENT_CHANGED);
}

void BtA2dpSourceMsgHandler(void *msg) {
    BtEvent* pEvent = NULL;
    int num_codec_cfgs = 0;
    if(!msg) {
        ALOGE("Msg is NULL, bail out!!");
        return;
    }

    pEvent = ( BtEvent *) msg;
    switch(pEvent->event_id) {
        case PROFILE_API_START:
            ALOGD(LOGTAG_A2DP "enable a2dp source");
            if (pA2dpSource) {
                pA2dpSource->HandleEnableSource();
            }
            break;
        case PROFILE_API_STOP:
            ALOGD(LOGTAG_A2DP "disable a2dp source");
            if (pA2dpSource) {
                pA2dpSource->HandleDisableSource();
            }
            break;
        case AVRCP_TARGET_CONNECTED_CB:
        case AVRCP_TARGET_DISCONNECTED_CB:
        case A2DP_SOURCE_AUDIO_CMD_REQ:
        case A2DP_SOURCE_AUDIO_AVDT_CMD_REQ:
        case AVRCP_TARGET_GET_ELE_ATTR:
        case AVRCP_TARGET_GET_PLAY_STATUS:
        case AVRCP_TARGET_REG_NOTI:
        case AVRCP_TARGET_TRACK_CHANGED:
        case AVRCP_TARGET_NOW_PLAYING_CONTENT_CHANGED:
        case AVRCP_TARGET_VOLUME_CHANGED:
        case AVRCP_TARGET_ADDR_PLAYER_CHANGED:
        case AVRCP_TARGET_AVAIL_PLAYER_CHANGED:
        case AVRCP_TARGET_SET_ABS_VOL:
        case AVRCP_TARGET_ABS_VOL_TIMEOUT:
        case AVRCP_TARGET_SEND_VOL_UP_DOWN:
        case AVRCP_TARGET_GET_FOLDER_ITEMS_CB:
        case AVRCP_TARGET_SET_ADDR_PLAYER_CB:
        case AVRCP_TARGET_USE_BIGGER_METADATA:
        case AVRCP_TARGET_LIST_PLAYER_APP_ATTR:
        case AVRCP_TARGET_LIST_PLAYER_APP_VALUES:
        case AVRCP_TARGET_GET_PLAYER_APP_VALUE:
        case AVRCP_TARGET_SET_PLAYER_APP_VALUE:
        case AVRCP_SET_EQUALIZER_VAL:
        case AVRCP_SET_REPEAT_VAL:
        case AVRCP_SET_SHUFFLE_VAL:
        case AVRCP_SET_SCAN_VAL:
        case AVRCP_TARGET_PLAY_POSITION_TIMEOUT:
        case AVRCP_TARGET_SET_BROWSED_PLAYER_REQ:
        case AVRCP_TARGET_CHANGE_PATH_REQ:
        case AVRCP_TARGET_GET_TOTAL_NUM_OF_ITEMS_CB:
        case AVRCP_TARGET_SEARCH_CB:
        case AVRCP_TARGET_GET_ITEM_ATTRIBUTES_REQ:
        case AVRCP_TARGET_PLAY_ITEMS_REQ:
        case AVRCP_TARGET_ADDTO_NOW_PLAYING_REQ:
            if (pA2dpSource) {
                if(pA2dpSource->get_state() == STATE_A2DP_SOURCE_NOT_STARTED) {
                    fprintf(stdout, "Ignore!! Make sure BT is turned on!!\n");
                    ALOGE(LOGTAG_A2DP " STATE UNINITIALIZED, return");
                    break;
                }
                pA2dpSource->HandleAvrcpEvents(( BtEvent *) msg);
            }
            break;
        case A2DP_SOURCE_CODEC_LIST:
            if(pA2dpSource && pA2dpSource->get_state() == STATE_A2DP_SOURCE_NOT_STARTED) {
                fprintf(stdout, "Ignore!! Make sure BT is turned on!!\n");
                ALOGE(LOGTAG_A2DP " STATE UNINITIALIZED, return");
                break;
            }
            A2dpCodecList(pEvent->a2dpCodecListEvent.codec_list, &num_codec_cfgs);
            if (pA2dpSource && num_codec_cfgs)
                pA2dpSource->UpdateSupportedCodecs(pEvent->a2dpSourceEvent.bd_addr, num_codec_cfgs);
            break;
        case A2DP_SOURCE_CODEC_MODE_CHANGE:
            if(pA2dpSource && pA2dpSource->get_state() == STATE_A2DP_SOURCE_NOT_STARTED) {
                fprintf(stdout, "Ignore!! Make sure BT is turned on!!\n");
                ALOGE(LOGTAG_A2DP " STATE UNINITIALIZED, return");
                break;
            }
            aptxad_mode_change(pEvent->a2dpCodecListEvent.codec_list, &num_codec_cfgs);
            if (pA2dpSource)
                pA2dpSource->UpdateSupportedCodecs(pEvent->a2dpCodecListEvent.bd_addr, num_codec_cfgs);
            break;
        default:
            if(pA2dpSource) {
               pA2dpSource->ProcessEvent(( BtEvent *) msg);
            }
            break;
    }
    delete pEvent;
}

#ifdef __cplusplus
}
#endif


static void BtA2dpLoadA2dpHal() {

}

static void BtA2dpStopStreaming()
{

}

static void BtA2dpCloseOutputStream()
{

}

static void BtA2dpUnloadA2dpHal() {


}

static void BtA2dpOpenOutputStream()
{

}

static void BtA2dpSuspendStreaming()
{

}

static void BtA2dpResumeStreaming()
{

}

int get_codec_relay_data(void)
{
    pthread_mutex_lock(&a2dp_sink_relay_mutex);
    if(list_is_empty(a2dp_sink_relay_data_list)) {
        pthread_mutex_unlock(&a2dp_sink_relay_mutex);
        return INVALID_CODEC;
    }
    t_SINK_RELAY_DATA* ptr = (t_SINK_RELAY_DATA*)list_front(a2dp_sink_relay_data_list);
    pthread_mutex_unlock(&a2dp_sink_relay_mutex);
    return ptr->codec_type;
}

#define RELAY_QUEUE_SIZE        24
#define MIN_NR_RELAY_FRAME        8

static bool is_relay_sink2src(void)
{
    bool ret = false;
    uint16_t codec_type;
    t_SINK_RELAY_DATA* ptr;

    if (NULL == pA2dpSinkStream)
        return false;

    if (pA2dpSinkStream->sbc_decoding)
        codec_type = A2DP_SINK_AUDIO_CODEC_PCM;
    else
        codec_type = A2DP_SINK_AUDIO_CODEC_SBC;

    pthread_mutex_lock(&a2dp_sink_relay_mutex);
    if (list_is_empty(a2dp_sink_relay_data_list))
        goto out;

    ptr = (t_SINK_RELAY_DATA*)list_front(a2dp_sink_relay_data_list);
    if (ptr->codec_type != codec_type) {
        ALOGD("in %s : Discard a frame : ptr->codec_type = %d, codec_type = %d",
                __func__, ptr->codec_type, codec_type);
        list_remove(a2dp_sink_relay_data_list, ptr);
        osi_free(ptr);
        goto out;
    }

    if(list_length(a2dp_sink_relay_data_list) < MIN_NR_RELAY_FRAME)
        goto out;

    ret = true;

out:
    pthread_mutex_unlock(&a2dp_sink_relay_mutex);
    return ret;
}

void flush_relay_data(void)
{
    if(!a2dp_sink_relay_data_list)
        return;
    ALOGD("flush relay data, list_length = %d",list_length(a2dp_sink_relay_data_list));
    t_SINK_RELAY_DATA* ptr;
    pthread_mutex_lock(&a2dp_sink_relay_mutex);
    while (!list_is_empty(a2dp_sink_relay_data_list))
    {
        ptr = (t_SINK_RELAY_DATA*)list_front(a2dp_sink_relay_data_list);
        list_remove(a2dp_sink_relay_data_list, ptr);
        osi_free(ptr);
    }
    pthread_mutex_unlock(&a2dp_sink_relay_mutex);
    return;
}

void enque_relay_data(uint8_t* buffer, size_t size, uint8_t codec_type)
{
    static int prompt_cnt = 8;
    if (NULL == a2dp_sink_relay_data_list) {
        if (prompt_cnt > 0) {
            printf("Please enable A2dp source function firstly\n");
            prompt_cnt--;
        }
        return;
    }

    ALOGD(" enque_relay_data size %d list_len = %d codec=%d", size, list_length(a2dp_sink_relay_data_list),codec_type);
    pthread_mutex_lock(&a2dp_sink_relay_mutex);
    if (list_length(a2dp_sink_relay_data_list) > RELAY_QUEUE_SIZE) {
        ALOGE(LOGTAG_A2DP "%s:a2dp sink relay queue is full",__func__);
        pthread_mutex_unlock(&a2dp_sink_relay_mutex);
        return;
    }

    /* allocate memory, first 4 bytes will have size, next 4 bytes will have offset */
    t_SINK_RELAY_DATA* ptr = (t_SINK_RELAY_DATA*)osi_malloc(size + sizeof(t_SINK_RELAY_DATA));
    uint8_t* data_ptr;
    if(ptr != NULL)
    {
        data_ptr  = (uint8_t*)(ptr+1);
        memcpy(data_ptr, (uint8_t*)buffer, size);
        ptr->codec_type = codec_type;// 0 is for SBC
        ptr->offset = 0;
        ptr->len = size;
        ALOGD(" enque data codec = %d, size=%d",codec_type,size);
    }
    else
    {
        ALOGE(LOGTAG_A2DP "%s:can not alloc t_SINK_RELAY_DATA",__func__);
        pthread_mutex_unlock(&a2dp_sink_relay_mutex);
        return;
    }
    list_append(a2dp_sink_relay_data_list, ptr);
    pthread_mutex_unlock(&a2dp_sink_relay_mutex);
}

size_t get_sbc_data(uint8_t* buffer, size_t size)
{
    ALOGD("revise1 get SBC Data size %d list_len = %d", size, list_length(a2dp_sink_relay_data_list));
    uint8_t* start_buf_ptr = buffer;
    uint8_t* end_buf_ptr = buffer + size;
    uint8_t* data_ptr;
    size_t data_len=0;
    pthread_mutex_lock(&a2dp_sink_relay_mutex);
    if(list_is_empty(a2dp_sink_relay_data_list)) {
            pthread_mutex_unlock(&a2dp_sink_relay_mutex);
            return 0;
    }
    t_SINK_RELAY_DATA* ptr = (t_SINK_RELAY_DATA*)list_front(a2dp_sink_relay_data_list);
    //ALOGD("len=%d,offset=%d, end-stat=%d",ptr->len,ptr->offset,end_buf_ptr - start_buf_ptr);
    while((start_buf_ptr < end_buf_ptr) && (!list_is_empty(a2dp_sink_relay_data_list)))
    {
        data_ptr = (uint8_t*)(ptr + 1);
        /* packets in topmost element are more than what is to be written */
        if((ptr->len - ptr->offset) > (end_buf_ptr - start_buf_ptr))
        {
            ALOGD("packet is more than left buffer len=%d, gap=%d",ptr->len,end_buf_ptr - start_buf_ptr);
            break;
            memcpy(start_buf_ptr, data_ptr + ptr->offset, (end_buf_ptr - start_buf_ptr));
            ptr->offset += (end_buf_ptr -  start_buf_ptr);
            start_buf_ptr += (end_buf_ptr -  start_buf_ptr);
        }
        else /* packets in topmost element is lesser than what is required */
        {
            memcpy(start_buf_ptr, data_ptr + ptr->offset, (ptr->len - ptr->offset));
            data_len+=(ptr->len - ptr->offset);
            PRINTBIT(start_buf_ptr,4);
            start_buf_ptr += (ptr->len - ptr->offset);
            ptr->offset += (ptr->len - ptr->offset);
           //ALOGD("ptr->len=%d,ptr->offset=%d, end-stat=%d,data_len=%d",ptr->len,ptr->offset,end_buf_ptr - start_buf_ptr,data_len);
            list_remove(a2dp_sink_relay_data_list, ptr);
            osi_free(ptr);
            if (!list_is_empty(a2dp_sink_relay_data_list)) {
                ptr = (t_SINK_RELAY_DATA*)list_front(a2dp_sink_relay_data_list);
            }
        }
    }
    pthread_mutex_unlock(&a2dp_sink_relay_mutex);
    if(start_buf_ptr == end_buf_ptr)
        return size;
    else
        return data_len;

}

size_t get_pcm_data(uint8_t* buffer, size_t size)
{
    ALOGD(" get PCM Data size %d list_len = %d", size, list_length(a2dp_sink_relay_data_list));
    uint8_t* start_buf_ptr = buffer;
    uint8_t* end_buf_ptr = buffer + size;
    uint8_t* data_ptr;
    pthread_mutex_lock(&a2dp_sink_relay_mutex);
    if(list_is_empty(a2dp_sink_relay_data_list)) {
        pthread_mutex_unlock(&a2dp_sink_relay_mutex);
        return 0;
    }
    t_SINK_RELAY_DATA* ptr = (t_SINK_RELAY_DATA*)list_front(a2dp_sink_relay_data_list);
    if(ptr->codec_type != A2DP_SINK_AUDIO_CODEC_PCM)
    {
        list_remove(a2dp_sink_relay_data_list, ptr);
        osi_free(ptr);
        pthread_mutex_unlock(&a2dp_sink_relay_mutex);
        return 0;
    }

    while((start_buf_ptr < end_buf_ptr) && (!list_is_empty(a2dp_sink_relay_data_list)))
    {
        data_ptr = (uint8_t*)(ptr + 1);
        /* packets in topmost element are more than what is to be written */
        if((ptr->len - ptr->offset) > (end_buf_ptr - start_buf_ptr))
        {
            memcpy(start_buf_ptr, data_ptr + ptr->offset, (end_buf_ptr - start_buf_ptr));
            ptr->offset += (end_buf_ptr -  start_buf_ptr);
            start_buf_ptr += (end_buf_ptr -  start_buf_ptr);
        }
        else /* packets in topmost element is lesser than what is required */
        {
            memcpy(start_buf_ptr, data_ptr + ptr->offset, (ptr->len - ptr->offset));
            start_buf_ptr += (ptr->len - ptr->offset);
            ptr->offset += (ptr->len - ptr->offset);
            list_remove(a2dp_sink_relay_data_list, ptr);
            osi_free(ptr);
            if (!list_is_empty(a2dp_sink_relay_data_list)) {
                ptr = (t_SINK_RELAY_DATA*)list_front(a2dp_sink_relay_data_list);
            }
        }
    }
    pthread_mutex_unlock(&a2dp_sink_relay_mutex);
    if(start_buf_ptr == end_buf_ptr)
        return size;
    else
        return(end_buf_ptr - start_buf_ptr);
}

#define PCM_HEADER_SIZE        44
static void skip_pcm_header(FILE *pcm_file)
{
    char hdr_buff[PCM_HEADER_SIZE];

    memset(hdr_buff, 0x00, PCM_HEADER_SIZE);
    if (fread(hdr_buff, 1, PCM_HEADER_SIZE, pcm_file) == PCM_HEADER_SIZE) {
        if ((strncmp(hdr_buff, "RIFF", 4) == 0) && (strncmp(&hdr_buff[8], "WAVEfmt", 7) == 0)) {
            ALOGD("in %s : skip %d bytes\n", __func__, PCM_HEADER_SIZE);
            return;
        }
    }

    fseek(pcm_file, 0, SEEK_SET);
    return;
}

void update_src_codec_type(uint16_t *src_codec_tp, btav_a2dp_codec_index_t codec_type){
    switch(codec_type){
      case BTAV_A2DP_CODEC_INDEX_SOURCE_SBC:
      *src_codec_tp = A2DP_SINK_AUDIO_CODEC_SBC;
      break;
      default:
      *src_codec_tp = NON_A2DP_MEDIA_CT;
      break;
  }
}

void update_src_codec_config(btav_codec_config_t *src_codec_cnfg, btav_a2dp_codec_config_t codec_cfg){
    switch(codec_cfg.sample_rate){
      case BTAV_A2DP_CODEC_SAMPLE_RATE_16000:
      src_codec_cnfg->sbc_config.samp_freq = SBC_SAMP_FREQ_16;
      break;
      case BTAV_A2DP_CODEC_SAMPLE_RATE_32000:
      src_codec_cnfg->sbc_config.samp_freq = SBC_SAMP_FREQ_32;
      break;
      case BTAV_A2DP_CODEC_SAMPLE_RATE_44100:
      src_codec_cnfg->sbc_config.samp_freq = SBC_SAMP_FREQ_44;
      break;
      case BTAV_A2DP_CODEC_SAMPLE_RATE_48000:
      src_codec_cnfg->sbc_config.samp_freq = SBC_SAMP_FREQ_48;
      break;
      default:
      src_codec_cnfg->sbc_config.samp_freq = SBC_SAMP_FREQ_NONE;
      break;
    }
    ALOGD(LOGTAG_A2DP "channel_mode : ",codec_cfg.channel_mode);

    switch(codec_cfg.channel_mode){
      case BTAV_A2DP_CODEC_CHANNEL_MODE_MONO:
      src_codec_cnfg->sbc_config.ch_mode = SBC_CH_MONO;
      break;
      case BTAV_A2DP_CODEC_CHANNEL_MODE_STEREO:
      src_codec_cnfg->sbc_config.ch_mode = SBC_CH_STEREO;
      break;
      case BTAV_A2DP_CODEC_CHANNEL_MODE_JOINT:
      src_codec_cnfg->sbc_config.ch_mode = SBC_CH_JOINT;
      break;
      case BTAV_A2DP_CODEC_CHANNEL_MODE_DUAL:
      src_codec_cnfg->sbc_config.ch_mode = SBC_CH_DUAL;
      break;
      default:
      src_codec_cnfg->sbc_config.ch_mode = SBC_CH_NONE;
      break;
    }
    src_codec_cnfg->sbc_config.block_len = codec_cfg.codec_specific_1;
    src_codec_cnfg->sbc_config.num_subbands = codec_cfg.codec_specific_2;
    src_codec_cnfg->sbc_config.alloc_mthd = codec_cfg.codec_specific_3;
    src_codec_cnfg->sbc_config.max_bitpool = codec_cfg.codec_specific_4;
    src_codec_cnfg->sbc_config.min_bitpool = codec_cfg.codec_specific_5;
}



static void BtA2dpStartStreaming()
{

}

static void bta2dp_connection_state_callback(const RawAddress& bd_addr, btav_connection_state_t state) {
    ALOGD(LOGTAG_A2DP " Connection State CB");
    BtEvent *pEvent = new BtEvent;
    pEvent->a2dpSourceEvent.bd_addr = bd_addr;
    switch( state ) {
        case BTAV_CONNECTION_STATE_DISCONNECTED:
            pEvent->a2dpSourceEvent.event_id = A2DP_SOURCE_DISCONNECTED_CB;
        break;
        case BTAV_CONNECTION_STATE_CONNECTING:
            pEvent->a2dpSourceEvent.event_id = A2DP_SOURCE_CONNECTING_CB;
        break;
        case BTAV_CONNECTION_STATE_CONNECTED:
            pEvent->a2dpSourceEvent.event_id = A2DP_SOURCE_CONNECTED_CB;
        break;
        case BTAV_CONNECTION_STATE_DISCONNECTING:
            pEvent->a2dpSourceEvent.event_id = A2DP_SOURCE_DISCONNECTING_CB;
        break;
    }
    PostMessage(THREAD_ID_A2DP_SOURCE, pEvent);
}

static void bta2dp_audio_state_callback(const RawAddress& bd_addr, btav_audio_state_t state) {
    ALOGD(LOGTAG_A2DP " Audio State CB");
    BtEvent *pEvent = new BtEvent;
    pEvent->a2dpSourceEvent.bd_addr = bd_addr;
    switch( state ) {
        case BTAV_AUDIO_STATE_REMOTE_SUSPEND:
            pEvent->a2dpSourceEvent.event_id = A2DP_SOURCE_AUDIO_SUSPENDED;
            a2dp_playstatus = A2DP_SOURCE_AUDIO_SUSPENDED;
        break;
        case BTAV_AUDIO_STATE_STOPPED:
            pEvent->a2dpSourceEvent.event_id = A2DP_SOURCE_AUDIO_STOPPED;
            a2dp_playstatus = A2DP_SOURCE_AUDIO_STOPPED;
        break;
        case BTAV_AUDIO_STATE_STARTED:
            pEvent->a2dpSourceEvent.event_id = A2DP_SOURCE_AUDIO_STARTED;
            a2dp_playstatus = A2DP_SOURCE_AUDIO_STARTED;
        break;
    }
    ALOGD(LOGTAG_A2DP " Audio State = %d",a2dp_playstatus);
    PostMessage(THREAD_ID_A2DP_SOURCE, pEvent);
}



static void bta2dp_delay_report_vendor_callback(bt_bdaddr_t *bd_addr, uint16_t report_delay) {
    char str[18];
    bdaddr_to_string(bd_addr,str, 18);
    ALOGD(LOGTAG_A2DP "Received delay report! [%s] the delay is [%d]", str, report_delay);
    fprintf(stdout, "Received delay report! the delay is %d ms\n", report_delay);
}

static void mtu_packettype_vendor_callback(uint16_t mtu,uint8_t packettype, bt_bdaddr_t *bd_addr) {
    ALOGD(LOGTAG_A2DP "mtu_packettype_vendor_callback: L2cap mtu = %d , packet type = %d", mtu,packettype);
    MTU_src = mtu;
    if(bd_addr!=NULL)
        ALOGD(LOGTAG_A2DP "mtu_packettype_vendor_callback: bd_addr = %02x:%02x:%02x:%02x:%02x:%02x" ,
            bd_addr->address[0],bd_addr->address[1],bd_addr->address[2],bd_addr->address[3],bd_addr->address[4],bd_addr->address[5]);
    else
        ALOGD(LOGTAG_A2DP "bd_addr is NULL");
}

static void bta2dp_audio_config_callback( const RawAddress& bd_addr, btav_a2dp_codec_config_t codec_config,  std::vector<btav_a2dp_codec_config_t> codecs_local_capabilities ) {
    ALOGD(LOGTAG_A2DP " bta2dp_audio_config_callback codec_type");
    BtEvent *pEvent = new BtEvent;
    pEvent->a2dpSourceEvent.event_id = A2DP_SOURCE_CODEC_CONFIG_CB;
    pEvent->a2dpSourceEvent.bd_addr = bd_addr;
    pEvent->a2dpSourceEvent.buf_size = sizeof(btav_a2dp_codec_config_t);
    pEvent->a2dpSourceEvent.buf_ptr = (uint8_t*)osi_malloc(pEvent->a2dpSourceEvent.buf_size);
    memcpy(pEvent->a2dpSourceEvent.buf_ptr, &codec_config, pEvent->a2dpSourceEvent.buf_size);
    PostMessage(THREAD_ID_A2DP_SOURCE, pEvent);
}

static void bta2dp_audio_registration_callback(bool state) {
    ALOGD(LOGTAG_A2DP " Audio Registration Callback: state = %d", state);
}

static void scmst_capabalities_vendor_callback(bt_bdaddr_t *bd_addr, bool scmst_enabled) {
    ALOGD(LOGTAG_A2DP " %s , bd_addr : %s, scmst_enabled: %d",__func__,
          bd_addr->ToString().c_str(),scmst_enabled);
    if(scmst_enabled) {
        fprintf(stdout, "Connection established with SCMS-T content protection \n");
    }
}

static void bta2dp_avdt_cap_callback(const RawAddress& bd_addr, uint8_t seid, uint16_t psc_mask) {
    ALOGD(LOGTAG_A2DP " bta2dp_avdt_cap_callback seid:%d", seid);
}

static btav_source_callbacks_t sBluetoothA2dpSourceCallbacks = {
    sizeof(sBluetoothA2dpSourceCallbacks),
    bta2dp_connection_state_callback,
    bta2dp_audio_state_callback,
    bta2dp_audio_config_callback,
    bta2dp_avdt_cap_callback,
};

static btav_vendor_callbacks_t sBluetoothA2dpSourceVendorCallbacks = {
    sizeof(sBluetoothA2dpSourceVendorCallbacks),
    NULL,
    NULL,
    NULL,
    NULL,
    bta2dp_delay_report_vendor_callback,
    NULL,
    mtu_packettype_vendor_callback,
    bta2dp_audio_registration_callback,
    scmst_capabalities_vendor_callback,
};

static void btavrcp_target_passthrough_cmd_callback(int id, int key_state, bt_bdaddr_t* bd_addr) {
    ALOGD(LOGTAG_AVRCP " btavrcp_target_passthrough_cmd_callback id = %d key_state = %d", id, key_state);
    if (key_state == KEY_PRESSED) {
        BtEvent *event = new BtEvent;
        event->avrcpTargetEvent.event_id = A2DP_SOURCE_AUDIO_CMD_REQ;
        /*As there is no player impl available at this point hence STOP/PAUSE has got same functionality*/
        /*Note: There is no need to convert the command type when testing certification tests.*/
        if(id == CMD_ID_PAUSE && !is_pts_test_enabled_)
            id = CMD_ID_STOP;
        event->avrcpTargetEvent.key_id = id;
        PostMessage (THREAD_ID_A2DP_SOURCE, event);
    }
}

static void btavrcp_target_setaddrplayer_cmd_callback(uint16_t player_id, bt_bdaddr_t *bd_addr) {
    ALOGD(LOGTAG_AVRCP " btavrc_target_setaddrplayer_cmd_vendor_callback ");
    BtEvent *pEvent = new BtEvent;
    pEvent->avrcpTargetEvent.event_id = AVRCP_TARGET_SET_ADDR_PLAYER_CB;
    pEvent->avrcpTargetEvent.arg1 = player_id;
    memcpy(&pEvent->avrcpTargetEvent.bd_addr, bd_addr, sizeof(bt_bdaddr_t));
    PostMessage (THREAD_ID_A2DP_SOURCE, pEvent);
}

static void btavrcp_target_getfolderitems_cmd_callback(uint8_t scope, uint32_t start_item,
              uint32_t end_item, uint8_t num_attr, uint32_t *p_attr_ids, uint16_t size, RawAddress *bd_addr) {
    ALOGD(LOGTAG_AVRCP " btavrcp_target_getfolderitems_cmd_callback ");
    BtEvent *pEvent = new BtEvent;
    pEvent->avrcpTargetEvent.event_id = AVRCP_TARGET_GET_FOLDER_ITEMS_CB;

    FolderListEntries* folderItem = (FolderListEntries*)osi_malloc(sizeof(FolderListEntries));
    memset(folderItem, 0, sizeof(FolderListEntries));
    if( num_attr == 0 )
        num_attr = BTRC_MAX_ELEM_ATTR_SIZE;
    else if ( num_attr == 255 )
        num_attr = 0;

    if(num_attr <= BTRC_MAX_ELEM_ATTR_SIZE) {
        memcpy(&folderItem->p_attr, p_attr_ids, num_attr * sizeof(uint32_t));
    }

    folderItem->mStart = start_item;
    folderItem->mEnd = end_item;
    folderItem->mSize = size;
    folderItem->mNumAttr = num_attr;

    pEvent->avrcpTargetEvent.buf_size = sizeof(folderItem);
    pEvent->avrcpTargetEvent.buf_ptr = (uint8_t*)osi_malloc(pEvent->avrcpTargetEvent.buf_size);
    memcpy(pEvent->avrcpTargetEvent.buf_ptr, &folderItem, pEvent->avrcpTargetEvent.buf_size);
    pEvent->avrcpTargetEvent.arg1 = (uint16_t)scope;
    memcpy(&pEvent->avrcpTargetEvent.bd_addr, bd_addr, sizeof(bt_bdaddr_t));
    PostMessage (THREAD_ID_A2DP_SOURCE, pEvent);
}

static void btavrcp_target_connection_state_callback(bool state, bt_bdaddr_t* bd_addr) {
    ALOGD(LOGTAG_AVRCP " btavrcp_target_connection_state_callback rc state = %d", state);
    BtEvent *pEvent = new BtEvent;
    memcpy(&pEvent->avrcpTargetEvent.bd_addr, bd_addr, sizeof(bt_bdaddr_t));
    if (state == true)
        pEvent->avrcpTargetEvent.event_id = AVRCP_TARGET_CONNECTED_CB;
    else
        pEvent->avrcpTargetEvent.event_id = AVRCP_TARGET_DISCONNECTED_CB;
    PostMessage(THREAD_ID_A2DP_SOURCE, pEvent);
}

static void btavrcp_target_rcfeatures_callback( bt_bdaddr_t* bd_addr, btrc_remote_features_t features) {
    ALOGD(LOGTAG_AVRCP " btavrcp_target_rcfeatures_callback features = %d", features);
    if ((features & BTRC_FEAT_ABSOLUTE_VOLUME) != 0) {
        ALOGD(LOGTAG_AVRCP " Abs vol supported for dev ");
        pA2dpSource->mAbsVolRemoteSupported = true;
    } else {
        ALOGD(LOGTAG_AVRCP " Abs vol NOT supported for dev ");
    }

    if((features & BTRC_FEAT_BROWSE) != 0) {
        TRACK_IS_SELECTED = 1L;
    }

    pA2dpSource->mVolCmdSetInProgress = false;
    pA2dpSource->mVolCmdAdjustInProgress = false;
    pA2dpSource->mInitialRemoteVolume = -1;
    pA2dpSource->mLastRemoteVolume = -1;
    pA2dpSource->mRemoteVolume = -1;
    pA2dpSource->mLastLocalVolume = -1;
    pA2dpSource->mLocalVolume = -1;
}

static void btavrcp_target_getelemattr_callback(uint8_t num_attr,
        btrc_media_attr_t *p_attrs, bt_bdaddr_t *bd_addr) {
    ALOGD(LOGTAG_AVRCP " btavrcp_target_getelemattr_callback ");
    int i;
    for (i = 0; i < num_attr; ++i) {
        ALOGD(LOGTAG_AVRCP " btavrcp_target_getelemattr_callback features = %d", p_attrs[i]);
    }
    BtEvent *pEvent = new BtEvent;
    pEvent->avrcpTargetEvent.event_id = AVRCP_TARGET_GET_ELE_ATTR;

    ItemAttr* itemAttr = (ItemAttr*)osi_malloc(sizeof(ItemAttr));
    itemAttr->p_attr = (btrc_media_attr_t*)osi_malloc(num_attr * sizeof(btrc_media_attr_t));
    memcpy(itemAttr->p_attr, p_attrs, num_attr * sizeof(btrc_media_attr_t));

    pEvent->avrcpTargetEvent.buf_size = sizeof(ItemAttr);
    pEvent->avrcpTargetEvent.buf_ptr = (uint8_t*)osi_malloc(pEvent->avrcpTargetEvent.buf_size);
    memcpy(pEvent->avrcpTargetEvent.buf_ptr, itemAttr, pEvent->avrcpTargetEvent.buf_size);
    ItemAttr* pAttr = (ItemAttr*)pEvent->avrcpTargetEvent.buf_ptr;
    pAttr->p_attr = (btrc_media_attr_t*)osi_malloc(num_attr * sizeof(btrc_media_attr_t));
    memcpy(pAttr->p_attr, itemAttr->p_attr, num_attr * sizeof(btrc_media_attr_t));
    pEvent->avrcpTargetEvent.arg1 = (uint16_t)num_attr;
    memcpy(&pEvent->avrcpTargetEvent.bd_addr, bd_addr, sizeof(bt_bdaddr_t));
    PostMessage(THREAD_ID_A2DP_SOURCE, pEvent);
    osi_free(itemAttr->p_attr);
    osi_free(itemAttr);
}

static void btavrcp_target_getplaystatus_callback(bt_bdaddr_t *bd_addr) {
    ALOGD(LOGTAG_AVRCP " btavrcp_target_getplaystatus_callback ");
    BtEvent *pEvent = new BtEvent;
    pEvent->avrcpTargetEvent.event_id = AVRCP_TARGET_GET_PLAY_STATUS;
    memcpy(&pEvent->avrcpTargetEvent.bd_addr, bd_addr, sizeof(bt_bdaddr_t));
    PostMessage(THREAD_ID_A2DP_SOURCE, pEvent);
}

static void btavrcp_target_listplayerapp_attr_callback(bt_bdaddr_t *bd_addr) {
    ALOGD(LOGTAG_AVRCP " btavrcp_target_listplayerapp_attr_callback ");
    BtEvent *pEvent = new BtEvent;
    pEvent->avrcpTargetEvent.event_id = AVRCP_TARGET_LIST_PLAYER_APP_ATTR;
    memcpy(&pEvent->avrcpTargetEvent.bd_addr, bd_addr, sizeof(bt_bdaddr_t));
    PostMessage(THREAD_ID_A2DP_SOURCE, pEvent);
}

static void btavrcp_target_listplayerapp_values_callback(btrc_player_attr_t attr_id, bt_bdaddr_t *bd_addr) {
    ALOGD(LOGTAG_AVRCP "btavrcp_target_listplayerapp_values_callback");
    BtEvent *pEvent = new BtEvent;
    pEvent->avrcpTargetEvent.event_id = AVRCP_TARGET_LIST_PLAYER_APP_VALUES;
    memcpy(&pEvent->avrcpTargetEvent.bd_addr, bd_addr, sizeof(bt_bdaddr_t));
    pEvent->avrcpTargetEvent.attr_id = attr_id;
    ALOGD(LOGTAG_AVRCP "attr_id:%d", attr_id);
    PostMessage(THREAD_ID_A2DP_SOURCE, pEvent);
}

static void  btavrcp_target_getplayerapp_value_callback(uint8_t num_attr,
                                                    btrc_player_attr_t *p_attrs, bt_bdaddr_t *bd_addr) {
    ALOGD(LOGTAG_AVRCP "btavrcp_target_getplayerapp_value_callback");
    BtEvent *pEvent = new BtEvent;
    int i;
    pEvent->avrcpTargetEvent.event_id = AVRCP_TARGET_GET_PLAYER_APP_VALUE;
    memcpy(&pEvent->avrcpTargetEvent.bd_addr, bd_addr, sizeof(bt_bdaddr_t));
    pEvent->avrcpTargetEvent.arg3 = num_attr;
    ALOGD(LOGTAG_AVRCP "num_attr:%d",num_attr);
    for (i = 0; i < num_attr; i++)
        pEvent->avrcpTargetEvent.attr_ids[i] = p_attrs[i];
    PostMessage(THREAD_ID_A2DP_SOURCE, pEvent);
}

static void btavrcp_target_setplayerapp_value_callback(btrc_player_settings_t *p_vals, bt_bdaddr_t *bd_addr) {
    ALOGD(LOGTAG_AVRCP "btavrcp_target_setplayerapp_value_callback");
    BtEvent *pEvent = new BtEvent;
    uint8_t i;
    pEvent->avrcpTargetEvent.event_id = AVRCP_TARGET_SET_PLAYER_APP_VALUE;
    memcpy(&pEvent->avrcpTargetEvent.bd_addr, bd_addr, sizeof(bt_bdaddr_t));
    pEvent->avrcpTargetEvent.arg3 = p_vals->num_attr;
    ALOGD(LOGTAG_AVRCP "num_attr:%d", p_vals->num_attr);
    for (i = 0; i < p_vals->num_attr; i++) {
        pEvent->avrcpTargetEvent.attr_ids[i] = p_vals->attr_ids[i];
        pEvent->avrcpTargetEvent.attr_values[i] = p_vals->attr_values[i];
        ALOGD(LOGTAG_AVRCP "attr_ids:%d", p_vals->attr_ids[i]);
        ALOGD(LOGTAG_AVRCP "attr_values:%d", p_vals->attr_values[i]);
    }
    PostMessage(THREAD_ID_A2DP_SOURCE, pEvent);
}

static void btavrcp_target_regnoti_callback(btrc_event_id_t event_id, uint32_t param,
        bt_bdaddr_t *bd_addr) {
    ALOGD(LOGTAG_AVRCP " btavrcp_target_regnoti_callback ");
    BtEvent *pEvent = new BtEvent;
    pEvent->avrcpTargetEvent.event_id = AVRCP_TARGET_REG_NOTI;
    memcpy(&pEvent->avrcpTargetEvent.bd_addr, bd_addr, sizeof(bt_bdaddr_t));
    pEvent->avrcpTargetEvent.arg1 = (uint16_t)event_id;
    pEvent->avrcpTargetEvent.arg2 = param;
    PostMessage(THREAD_ID_A2DP_SOURCE, pEvent);
}

static void btavrcp_target_volchanged_callback(uint8_t volume, uint8_t ctype,
        bt_bdaddr_t *bd_addr) {
    ALOGD(LOGTAG_AVRCP " btavrcp_target_volchanged_callback ");
    BtEvent *pEvent = new BtEvent;
    pEvent->avrcpTargetEvent.event_id = AVRCP_TARGET_VOLUME_CHANGED;
    memcpy(&pEvent->avrcpTargetEvent.bd_addr, bd_addr, sizeof(bt_bdaddr_t));
    pEvent->avrcpTargetEvent.arg3 = volume;
    pEvent->avrcpTargetEvent.arg4 = (AvrcRspType)ctype;
    PostMessage(THREAD_ID_A2DP_SOURCE, pEvent);
}

static void btavrcp_target_setbrowsedplayer_cmd_callback(uint16_t player_id, bt_bdaddr_t *bd_addr) {
    ALOGD(LOGTAG_AVRCP " btavrc_target_setbrowsedplayer_cmd_callback ");
    BtEvent *pEvent = new BtEvent;
    pEvent->avrcpTargetEvent.event_id = AVRCP_TARGET_SET_BROWSED_PLAYER_REQ;
    pEvent->avrcpTargetEvent.arg1 = player_id;
    memcpy(&pEvent->avrcpTargetEvent.bd_addr, bd_addr, sizeof(bt_bdaddr_t));
    PostMessage (THREAD_ID_A2DP_SOURCE, pEvent);
}

static void btavrcp_target_change_path_callback(uint8_t direction, uint8_t* folder_uid,
                                          RawAddress* bd_addr){
    ALOGD(LOGTAG_AVRCP " btrc_change_path_callback ");
    BtEvent *pEvent = new BtEvent;
    pEvent->avrcpTargetEvent.event_id = AVRCP_TARGET_CHANGE_PATH_REQ;
    memcpy(&pEvent->avrcpTargetEvent.bd_addr, bd_addr, sizeof(bt_bdaddr_t));
    pEvent->avrcpTargetEvent.buf_size = BTRC_UID_SIZE;
    pEvent->avrcpTargetEvent.buf_ptr = (uint8_t*)osi_malloc(pEvent->avrcpTargetEvent.buf_size);
    memcpy(pEvent->avrcpTargetEvent.buf_ptr, folder_uid, pEvent->avrcpTargetEvent.buf_size);
    pEvent->avrcpTargetEvent.arg3 = direction;
    PostMessage(THREAD_ID_A2DP_SOURCE, pEvent);
}

static void btavrcp_target_get_item_attr_callback(uint8_t scope, uint8_t* uid,
                                            uint16_t uid_counter,
                                            uint8_t num_attr,
                                            btrc_media_attr_t* p_attrs,
                                            RawAddress* bd_addr){
    ALOGD(LOGTAG_AVRCP " btrc_get_item_attr_callback ");
    BtEvent *pEvent = new BtEvent;
    int i;
    pEvent->avrcpTargetEvent.event_id = AVRCP_TARGET_GET_ITEM_ATTRIBUTES_REQ;
    memcpy(&pEvent->avrcpTargetEvent.bd_addr, bd_addr, sizeof(bt_bdaddr_t));
    pEvent->avrcpTargetEvent.arg3 = scope;
    pEvent->avrcpTargetEvent.arg1 = uid_counter;
    ItemAttr* itemAttr = (ItemAttr*)osi_malloc(sizeof(ItemAttr));
    itemAttr->p_attr = (btrc_media_attr_t*)osi_malloc(num_attr * sizeof(btrc_media_attr_t));
    memcpy(itemAttr->p_attr, p_attrs, num_attr * sizeof(btrc_media_attr_t));
    memcpy(itemAttr->mUid, uid, BTRC_UID_SIZE);
    itemAttr->mSize = BTRC_UID_SIZE;

    pEvent->avrcpTargetEvent.buf_size = sizeof(ItemAttr);
    pEvent->avrcpTargetEvent.buf_ptr = (uint8_t*)osi_malloc(pEvent->avrcpTargetEvent.buf_size);
    memcpy(pEvent->avrcpTargetEvent.buf_ptr, itemAttr, pEvent->avrcpTargetEvent.buf_size);
    ItemAttr* pAttr = (ItemAttr*)pEvent->avrcpTargetEvent.buf_ptr;
    pAttr->p_attr = (btrc_media_attr_t*)osi_malloc(num_attr * sizeof(btrc_media_attr_t));
    memcpy(pAttr->p_attr, itemAttr->p_attr, num_attr * sizeof(btrc_media_attr_t));
    pEvent->avrcpTargetEvent.arg4 = (uint16_t)num_attr;
    ALOGD(LOGTAG_AVRCP "num_attr:%d",num_attr);

    memcpy(&pEvent->avrcpTargetEvent.bd_addr, bd_addr, sizeof(bt_bdaddr_t));
    PostMessage(THREAD_ID_A2DP_SOURCE, pEvent);
    osi_free(itemAttr->p_attr);
    osi_free(itemAttr);
}

static void btavrcp_target_play_item_callback(uint8_t scope, uint16_t uid_counter,
                                        uint8_t* uid, RawAddress* bd_addr){
    ALOGD(LOGTAG_AVRCP " btavrcp_target_play_item_callback ");
    BtEvent *pEvent = new BtEvent;
    pEvent->avrcpTargetEvent.event_id = AVRCP_TARGET_PLAY_ITEMS_REQ;
    memcpy(&pEvent->avrcpTargetEvent.bd_addr, bd_addr, sizeof(bt_bdaddr_t));
    pEvent->avrcpTargetEvent.buf_size = sizeof(uid);
    pEvent->avrcpTargetEvent.buf_ptr = (uint8_t*)osi_malloc(pEvent->avrcpTargetEvent.buf_size);
    memcpy(pEvent->avrcpTargetEvent.buf_ptr, uid, pEvent->avrcpTargetEvent.buf_size);
    pEvent->avrcpTargetEvent.arg1 = uid_counter;
    pEvent->avrcpTargetEvent.arg3 = scope;
    PostMessage(THREAD_ID_A2DP_SOURCE, pEvent);
}

static void btavrcp_target_get_total_num_of_items_callback(uint8_t scope,
                                                           RawAddress* bd_addr)
{
    ALOGD(LOGTAG_AVRCP "%s",__func__);
    BtEvent *pEvent = new BtEvent;
    memset(pEvent,0,sizeof(BtEvent));
    pEvent->avrcpTargetEvent.event_id = AVRCP_TARGET_GET_TOTAL_NUM_OF_ITEMS_CB;
    memcpy(&pEvent->avrcpTargetEvent.bd_addr, bd_addr, sizeof(bt_bdaddr_t));
    pEvent->avrcpTargetEvent.arg1 = (uint16_t) scope;
    PostMessage(THREAD_ID_A2DP_SOURCE, pEvent);
}

static void btavrcp_target_search_callback(uint16_t charset_id, uint16_t str_len,
                                           uint8_t* p_str, RawAddress* bd_addr)
{
    ALOGD(LOGTAG_AVRCP "%s",__func__);
    is_search_req_recieved = true;
    BtEvent *pEvent = new BtEvent;
    memset(pEvent,0,sizeof(BtEvent));
    pEvent->avrcpTargetEvent.event_id = AVRCP_TARGET_SEARCH_CB;
    memcpy(&pEvent->avrcpTargetEvent.bd_addr, bd_addr, sizeof(bt_bdaddr_t));
    pEvent->avrcpTargetEvent.arg1 = charset_id;
    pEvent->avrcpTargetEvent.buf_size = str_len;
    pEvent->avrcpTargetEvent.buf_ptr = (uint8_t*)osi_malloc(pEvent->avrcpTargetEvent.buf_size);
    memcpy(pEvent->avrcpTargetEvent.buf_ptr, p_str, pEvent->avrcpTargetEvent.buf_size);
    PostMessage(THREAD_ID_A2DP_SOURCE, pEvent);
}

static void btavrcp_target_add_to_now_playing_callback(uint8_t scope, uint8_t* uid,
                                                 uint16_t uid_counter,
                                                 RawAddress* bd_addr){
    ALOGD(LOGTAG_AVRCP " btavrcp_target_add_to_now_playing_callback ");
    BtEvent *pEvent = new BtEvent;
    pEvent->avrcpTargetEvent.event_id = AVRCP_TARGET_ADDTO_NOW_PLAYING_REQ;
    memcpy(&pEvent->avrcpTargetEvent.bd_addr, bd_addr, sizeof(bt_bdaddr_t));
    pEvent->avrcpTargetEvent.arg1 = uid_counter;
    pEvent->avrcpTargetEvent.arg3 = scope;
    pEvent->avrcpTargetEvent.buf_size = sizeof(uid);
    pEvent->avrcpTargetEvent.buf_ptr = (uint8_t*)osi_malloc(pEvent->avrcpTargetEvent.buf_size);
    memcpy(pEvent->avrcpTargetEvent.buf_ptr, uid, pEvent->avrcpTargetEvent.buf_size);
    PostMessage(THREAD_ID_A2DP_SOURCE, pEvent);
}

static btrc_callbacks_t sBluetoothAvrcpTargetCallbacks = {
   sizeof(sBluetoothAvrcpTargetCallbacks),
   btavrcp_target_rcfeatures_callback,
   btavrcp_target_getplaystatus_callback,
   btavrcp_target_listplayerapp_attr_callback,
   btavrcp_target_listplayerapp_values_callback,
   btavrcp_target_getplayerapp_value_callback,
   NULL,
   NULL,
   btavrcp_target_setplayerapp_value_callback,
   btavrcp_target_getelemattr_callback,
   btavrcp_target_regnoti_callback,
   btavrcp_target_volchanged_callback,
   btavrcp_target_passthrough_cmd_callback,
   btavrcp_target_setaddrplayer_cmd_callback,
   btavrcp_target_setbrowsedplayer_cmd_callback,
   btavrcp_target_getfolderitems_cmd_callback,
   btavrcp_target_change_path_callback,
   btavrcp_target_get_item_attr_callback,
   btavrcp_target_play_item_callback,
   btavrcp_target_get_total_num_of_items_callback,
   btavrcp_target_search_callback,
   btavrcp_target_add_to_now_playing_callback,
   btavrcp_target_connection_state_callback,
};

const char* getString(int mAttrType,uint8_t* Uid) {
    const char* new_str = "";
    const char* title1 = "Here, on the other hand, I've gone crazy \
        and really let the literal span several lines, \
        without bothering with quoting each line's \
        and really let the literal span several lines \
        Here, on the other hand, I've gone crazy \
        and really let the literal span several lines, \
        without bothering with quoting each line's \
        and really let the literal span several lines";
    const char* artistName1 = "Here, on the other hand, I've gone crazy \
        and really let the literal span several lines, \
        without bothering with quoting each line's \
        and really let the literal span several lines \
        Here, on the other hand, I've gone crazy \
        and really let the literal span several lines, \
        without bothering with quoting each line's \
        and really let the literal span several lines";
    const char* title = "abc1";
    const char* artistName = "abc2";
    const char* albumName = "abc3";
    const char* mediaNumber = "abc4";
    const char* mediaTotalNumber = "abc5";
    const char* genre = "abc6";
    const char* playingTimeMs = "abc7";
    const char* tracknum = "abc8";

    ALOGD(LOGTAG_AVRCP "%s mAttrType : %d ", __func__, mAttrType);
    switch (mAttrType) {
        case ATTR_TRACK_NUM:
            return tracknum;
        case ATTR_TITLE:
            if (use_bigger_metadata)
                return title1;
            else if(Uid == NULL)
                return title;

            if (pA2dpSource->pMediaList.size() > 0) {
                list<MediaInfo>::iterator p = pA2dpSource->pMediaList.begin();
                list<MediaInfo>::iterator p_end = pA2dpSource->pMediaList.end();
                while (p != p_end) {
                    if(!memcmp(p->mUid , Uid, BTRC_UID_SIZE))
                    {
                        ALOGD(LOGTAG_AVRCP " Uid mactched, return title");
                        return p->mDisplayableName;
                    }
                    p++;
                }
                ALOGD(LOGTAG_AVRCP "UID didn't match ");
            }
            else
            {
                ALOGD(LOGTAG_AVRCP "media list is empty");
            }
            return new_str;
        case ATTR_ARTIST_NAME:
            if (use_bigger_metadata)
                return artistName1;
            else
                return artistName;
        case ATTR_ALBUM_NAME:
            return albumName;
        case ATTR_MEDIA_NUMBER:
            return mediaNumber;
        case ATTR_MEDIA_TOTAL_NUMBER:
            return mediaTotalNumber;
        case ATTR_GENRE:
            return genre;
        case ATTR_PLAYING_TIME_MS:
            return playingTimeMs;
        default:
            return new_str;
    }
}

void set_abs_volume_timer_handler(void *context) {
    ALOGD(LOGTAG_AVRCP " set_abs_volume_timer_handler ");
    BtEvent *pEvent = new BtEvent;
    pEvent->avrcpTargetEvent.event_id = AVRCP_TARGET_ABS_VOL_TIMEOUT;
    PostMessage(THREAD_ID_A2DP_SOURCE, pEvent);
}

void A2dp_Source::StartSetAbsVolTimer() {
    if(abs_vol_timer) {
        ALOGD(LOGTAG_AVRCP " Abs Vol Timer still running + ");
        return;
    }
    alarm_set(set_abs_volume_timer, A2DP_SOURCE_SET_ABS_VOL_TIMER_DURATION,
           set_abs_volume_timer_handler, NULL);
    abs_vol_timer = true;
}

void A2dp_Source::StopSetAbsVolTimer() {
    ALOGD(LOGTAG_AVRCP " StopSetAbsVolTimer ");
    if((set_abs_volume_timer != NULL) && (abs_vol_timer)) {
        alarm_cancel(set_abs_volume_timer);
        ALOGD(LOGTAG_AVRCP " StopSetAbsVolTimer -1");
        abs_vol_timer = false;
    }
}

int convertToAudioStreamVolume(int volume) {
    // Rescale volume to match AudioSystem's volume
    return (int) round((double) volume*mAudioStreamMax/AVRCP_MAX_VOL);
}

int convertToAvrcpVolume(int volume) {
    return (int) ceil((double) volume*AVRCP_MAX_VOL/mAudioStreamMax);
}

bool isAbsoluteVolumeSupported() {
    ALOGD(LOGTAG_AVRCP " isAbsoluteVolumeSupported() %d ", pA2dpSource->mAbsVolRemoteSupported);
    return pA2dpSource->mAbsVolRemoteSupported;
}

void PlayPosTimehandler(void *context) {
    ALOGD(LOGTAG_AVRCP "PlayPosTimehandler ");
    BtEvent *pEvent = new BtEvent;
    pEvent->avrcpTargetEvent.event_id = AVRCP_TARGET_PLAY_POSITION_TIMEOUT;
    PostMessage(THREAD_ID_A2DP_SOURCE, pEvent);
}

void A2dp_Source::StartPlayPostionTimer() {
    ALOGD(LOGTAG_AVRCP "%s:Entered",__func__);
    if(play_pos_timer) {
        ALOGD(LOGTAG_AVRCP "Play postion Timer still running + ");
        return;
    }
    ALOGD(LOGTAG_AVRCP "play_position_interval:%d",play_position_interval);
    alarm_set(set_play_postion_timer, play_position_interval * 1000,
                                    PlayPosTimehandler, NULL);
    play_pos_timer = true;
}

void A2dp_Source::StopPlayPostionTimer() {
    ALOGD(LOGTAG_AVRCP " StopPlayPostionTimer ");
    if((set_play_postion_timer != NULL) && (play_pos_timer)) {
        alarm_cancel(set_play_postion_timer);
        ALOGD(LOGTAG_AVRCP " StopPlayPostionTimer -1");
        play_pos_timer = false;
    }
}

void A2dp_Source::HandleAvrcpEvents(BtEvent* pEvent) {
    ALOGD(LOGTAG_AVRCP " HandleAvrcpEvents event = %s",
            dump_message(pEvent->avrcpTargetEvent.event_id));
    uint8_t absvol, avrcpVolume;
    long TrackNumberRsp = -1L, pecentVolChanged;
    char *folderItems, *playerEntry, *folderEntry, *mediaEntry;
    uint16_t num_attr, num_val, scope, set_addr_player_id = 0, set_br_player_id = 0;
    bool isSetVol, volAdj = false, player_found = false;
    int i, pos = 0, song_len = 0, volIndex, start = 0,end = 0, count = 0, countElementLength = 0;
    int countTotalBytes = 0, countTemp = 0, checkLength = 0, folderItemLengths[32];
    int availableMediaPlayers = 0, positionItemStart = 0, availableFolders = 0, availableMedias = 0;
    AvrcRspType ctype;
    AvrcKeyDir dir;
    ItemAttr *item = NULL;
    FolderListEntries *folderitem = NULL;
    btrc_element_attr_val_t *pAttrs = NULL;
    btrc_register_notification_t param;
    btrc_vendor_folder_list_entries_t *p_param;
    btrc_player_attr_t p_attr[BTRC_MAX_APP_SETTINGS];
    uint8_t *attr_values = NULL;
    uint8_t key_id;
    static list<MediaInfo>::iterator pCurMedia = pMediaList.begin();

    switch(pEvent->avrcpTargetEvent.event_id) {
        case AVRCP_TARGET_USE_BIGGER_METADATA:
            ALOGD(LOGTAG_AVRCP " AVRCP_TARGET_USE_BIGGER_METADATA ");
            use_bigger_metadata = true;
            break;
        case AVRCP_TARGET_SET_ADDR_PLAYER_CB:
            set_addr_player_id = pEvent->avrcpTargetEvent.arg1;
            if (pMediaPlayerList.size() > 0) {
                list<MediaPlayerInfo>::iterator p = pMediaPlayerList.begin();
                while (p != pMediaPlayerList.end()) {
                    if (p->mPlayerId == set_addr_player_id)
                    {
                        player_found = true;
                        ALOGD(LOGTAG_AVRCP " valid player found for set addr player ");
                        mCurrentAddressedPlayer = set_addr_player_id;
                        break;
                    }
                    p++;
                }
            }
            else {
                ALOGE(LOGTAG_AVRCP "  No media players");
            }

            if (!player_found)
            {
                ALOGE(LOGTAG_AVRCP " Since not a valid player %d send error", set_addr_player_id);
                sBtAvrcpTargetInterface->set_addressed_player_rsp(&(pEvent->avrcpTargetEvent.bd_addr), BTRC_STS_INV_PLAYER);
                break;
            }

            ALOGD(LOGTAG_AVRCP " Send response for set addressed player %d", set_addr_player_id);
            sBtAvrcpTargetInterface->set_addressed_player_rsp(&(pEvent->avrcpTargetEvent.bd_addr), BTRC_STS_NO_ERROR);

            if (mCurrentAddrPlayerId == set_addr_player_id)
            {
                ALOGD(LOGTAG_AVRCP " Player already addresssed %d", set_addr_player_id);
            }
            else
            {
                mPreviousAddrPlayerId = mCurrentAddrPlayerId;
                mCurrentAddrPlayerId = set_addr_player_id;
                ALOGD(LOGTAG_AVRCP " mPreviousAddrPlayerId %d mCurrentAddrPlayerId = %d",
                                     mPreviousAddrPlayerId, mCurrentAddrPlayerId);
                if (mAddrPlayerChangedNotiType == BTRC_NOTIFICATION_TYPE_INTERIM) {
                    mAddrPlayerChangedNotiType = BTRC_NOTIFICATION_TYPE_CHANGED;
                    param.player_id = mCurrentAddrPlayerId;
                    sBtAvrcpTargetInterface->register_notification_rsp(
                            BTRC_EVT_ADDR_PLAYER_CHANGE,
                            mAddrPlayerChangedNotiType, &param);
                    if (mPreviousAddrPlayerId != -1)
                        resetAndSendPlayerStatusReject();
                }
            }
            break;
        case AVRCP_TARGET_GET_FOLDER_ITEMS_CB:
            scope = pEvent->avrcpTargetEvent.arg1;
            ALOGD(LOGTAG_AVRCP " Send response for get folder items for scope %d", scope);
            if (pEvent->avrcpTargetEvent.buf_ptr == NULL) {
                break;
            }
            folderitem = (FolderListEntries*)osi_malloc(sizeof(FolderListEntries));
            memcpy(&folderitem, pEvent->avrcpTargetEvent.buf_ptr, pEvent->avrcpTargetEvent.buf_size);
            ALOGD(LOGTAG_AVRCP "  %d %d %d %d", folderitem->mStart, folderitem->mEnd,
                                                folderitem->mSize, folderitem->mNumAttr);
            start = folderitem->mStart;
            end = folderitem->mEnd;
            folderItems = (char*) osi_malloc(folderitem->mSize * sizeof(char));
            switch(scope){
            case BTRC_SCOPE_PLAYER_LIST :
                if (pMediaPlayerList.size() > 0 && start == 0) {
                    list<MediaPlayerInfo>::iterator p = pMediaPlayerList.begin();
                    while (p != pMediaPlayerList.end()) {
                        //if (start == 0) {
                            playerEntry = (char*)osi_malloc(p->RetrievePlayerEntryLength()*
                                                               sizeof(char));
                            playerEntry = p->RetrievePlayerItemEntry();
                            int length = p->RetrievePlayerEntryLength();
                            folderItemLengths[availableMediaPlayers ++] = length;
                            for (count = 0; count < length; count ++) {
                                folderItems[positionItemStart + count] = playerEntry[count];
                            }
                            positionItemStart += length; // move start to next item star
                            osi_free(playerEntry);
                        /*}
                        else if (start > 0) {
                            --start;
                        }*/
                        p++;
                    }
                }
                else {
                    ALOGD(LOGTAG_AVRCP "  No media players");
                }

                ALOGD("availableMediaPlayers %d",availableMediaPlayers);
                p_param = (btrc_vendor_folder_list_entries_t*)osi_malloc(
                                    sizeof(btrc_vendor_folder_list_entries_t));
                p_param->status = BTRC_STS_NO_ERROR;
                p_param->uid_counter = 0;
                p_param->item_count = availableMediaPlayers;
                p_param->p_item_list =
               (btrc_folder_items_t*) osi_malloc (p_param->item_count*
                                      sizeof(btrc_folder_items_t));
                for (count = 0; count < p_param->item_count; count++) {
                    p_param->p_item_list[count].item_type =
                    folderItems[countTotalBytes]; countTotalBytes++;
                    p_param->p_item_list[count].player.player_id =
                    (uint16_t)(folderItems[countTotalBytes] & 0x00ff); countTotalBytes++;
                    p_param->p_item_list[count].player.player_id +=
                    (uint16_t)((folderItems[countTotalBytes] << 8) & 0xff00); countTotalBytes++;
                    p_param->p_item_list[count].player.major_type =
                    folderItems[countTotalBytes]; countTotalBytes++;
                    p_param->p_item_list[count].player.sub_type =
                    (uint32_t)(folderItems[countTotalBytes] & 0x000000ff); countTotalBytes++;
                    p_param->p_item_list[count].player.sub_type +=
                    (uint32_t)((folderItems[countTotalBytes] << 8) & 0x0000ff00); countTotalBytes++;
                    p_param->p_item_list[count].player.sub_type +=
                    (uint32_t)((folderItems[countTotalBytes] << 16) & 0x00ff0000); countTotalBytes++;
                    p_param->p_item_list[count].player.sub_type +=
                    (uint32_t)((folderItems[countTotalBytes] << 24) & 0xff000000); countTotalBytes++;
                    p_param->p_item_list[count].player.play_status =
                    folderItems[countTotalBytes]; countTotalBytes++;
                    for (countTemp = 0; countTemp < 16; countTemp ++) {
                        p_param->p_item_list[count].player.features[countTemp] =
                            folderItems[countTotalBytes];
                        ALOGD(LOGTAG_A2DP "player feat sending in resp %d",
                        p_param->p_item_list[count].player.features[countTemp]);
                            countTotalBytes++;
                    }
                    p_param->p_item_list[count].player.charset_id =
                    (uint16_t)(folderItems[countTotalBytes] & 0x00ff); countTotalBytes++;
                    p_param->p_item_list[count].player.charset_id +=
                    (uint16_t)((folderItems[countTotalBytes] << 8) & 0xff00); countTotalBytes++;
                    uint16_t str_len = (uint16_t)(folderItems[countTotalBytes] & 0x00ff); countTotalBytes++;
                    str_len += (uint16_t)((folderItems[countTotalBytes] << 8) & 0xff00); countTotalBytes++;
                    for (countTemp = 0; countTemp < str_len; countTemp ++) {
                        p_param->p_item_list[count].player.name[countTemp] =
                        folderItems[countTotalBytes]; countTotalBytes++;
                    }
                    /*To check if byte feeding went well*/
                    checkLength += folderItemLengths[count];
                    ALOGD(LOGTAG_AVRCP "checkLength = %u countTotalBytes = %u", checkLength,
                        countTotalBytes);
                    if (checkLength != countTotalBytes) {
                        ALOGE(LOGTAG_AVRCP "Error Populating Intermediate Folder Entry");
                    }
                }
                /* failed to copy list of media players */
                if (p_param->item_count == 0) {
                    p_param->status = BTRC_STS_INV_RANGE;
                    ALOGE("%s: No media players", __func__);
                }
                sBtAvrcpTargetInterface->get_folder_items_list_rsp(&(pEvent->avrcpTargetEvent.bd_addr), (btrc_status_t)p_param->status, p_param->uid_counter,
                      p_param->item_count, p_param->p_item_list);
                osi_free(pEvent->avrcpTargetEvent.buf_ptr);
                osi_free(folderitem);
                osi_free(folderItems);
                osi_free(p_param->p_item_list);
                osi_free(p_param);
            break;
            case BTRC_SCOPE_FILE_SYSTEM :
            case BTRC_SCOPE_NOW_PLAYING :
            ALOGD("mfolder_depth %d", mfolder_depth);
                if((mfolder_depth == 0) && (scope != BTRC_SCOPE_NOW_PLAYING)){
                    if (pFolderList.size() > 0 && start <= 1) {
                        list<FolderInfo>::iterator p = pFolderList.begin();
                        list<FolderInfo>::iterator p_end = pFolderList.begin();
                        if(end >= pFolderList.size())
                          p_end = pFolderList.end();
                        else
                          advance(p_end,end+1);
                        advance(p,start);

                        while (p != p_end) {
                            //if (start == 0) {
                                folderEntry = (char*)osi_malloc(p->RetrieveFolderEntryLength()*
                                                               sizeof(char));
                                folderEntry = p->RetrieveFolderItemEntry();
                                int length = p->RetrieveFolderEntryLength();
                                ALOGD("p->RetrieveFolderEntryLength %d",length);
                                folderItemLengths[availableFolders ++] = length;
                                for (count = 0; count < length; count ++)
                                    folderItems[positionItemStart + count] = folderEntry[count];
                                positionItemStart += length; // move start to next item star
                                osi_free(folderEntry);
                            /*}
                            else if (start > 0) {
                                --start;
                            }*/
                            p++;
                        }
                    }
                    else
                        ALOGD(LOGTAG_AVRCP "  No folders");
                    ALOGD("availableFolders %d",availableFolders);
                    p_param = (btrc_vendor_folder_list_entries_t*)osi_malloc(
                                    sizeof(btrc_vendor_folder_list_entries_t));
                    p_param->status = BTRC_STS_NO_ERROR ;
                    p_param->uid_counter = 0;
                    p_param->item_count = availableFolders;
                    p_param->p_item_list =
                   (btrc_folder_items_t*) osi_malloc (p_param->item_count*
                                      sizeof(btrc_folder_items_t));
                    for (count = 0; count < p_param->item_count; count++) {
                        for (countTemp = 0; countTemp < BTRC_UID_SIZE; countTemp ++) {
                            p_param->p_item_list[count].folder.uid[countTemp] =
                            folderItems[countTotalBytes]; countTotalBytes++;

                        }
                        p_param->p_item_list[count].folder.type =
                        folderItems[countTotalBytes];
                        p_param->p_item_list[count].item_type =
                        folderItems[countTotalBytes]; countTotalBytes++;

                        p_param->p_item_list[count].folder.playable =
                        folderItems[countTotalBytes]; countTotalBytes++;

                        p_param->p_item_list[count].folder.charset_id =
                        (uint16_t)(folderItems[countTotalBytes] & 0x00ff); countTotalBytes++;
                        p_param->p_item_list[count].folder.charset_id +=
                        (uint16_t)((folderItems[countTotalBytes] << 8) & 0xff00); countTotalBytes++;
                        uint16_t str_len = (uint16_t)(folderItems[countTotalBytes] & 0x00ff); countTotalBytes++;
                        str_len += (uint16_t)((folderItems[countTotalBytes] << 8) & 0xff00); countTotalBytes++;
                        for (countTemp = 0; countTemp < str_len; countTemp ++) {
                            p_param->p_item_list[count].folder.name[countTemp] =
                            folderItems[countTotalBytes]; countTotalBytes++;
                        }
                        p_param->p_item_list[count].folder.name[countTemp] = '\0';
                        /*To check if byte feeding went well*/
                        checkLength += folderItemLengths[count];
                        ALOGD(LOGTAG_AVRCP "checkLength = %u countTotalBytes = %u", checkLength,countTotalBytes);
                        if (checkLength != countTotalBytes)
                            ALOGE(LOGTAG_AVRCP "Error Populating Intermediate Folder Entry");
                    }
                    if (p_param->item_count == 0) {
                        p_param->status = BTRC_STS_INV_RANGE;
                        ALOGE("%s: No folders", __func__);
                    }
                    sBtAvrcpTargetInterface->get_folder_items_list_rsp(&(pEvent->avrcpTargetEvent.bd_addr), (btrc_status_t)p_param->status, p_param->uid_counter,
                      p_param->item_count, p_param->p_item_list);
                    osi_free(pEvent->avrcpTargetEvent.buf_ptr);
                    osi_free(folderitem);
                    osi_free(folderItems);
                    osi_free(p_param->p_item_list);
                    osi_free(p_param);
                }
                else if((is_empty_folder == 1) && (scope == BTRC_SCOPE_FILE_SYSTEM) && (mfolder_depth == 1)){
                    ALOGE(LOGTAG_AVRCP "Empty folder");
                    sBtAvrcpTargetInterface->get_folder_items_list_rsp(&(pEvent->avrcpTargetEvent.bd_addr), BTRC_STS_INV_RANGE, 0, 0, nullptr);
                }
                else if((mfolder_depth == 1) || (scope == BTRC_SCOPE_NOW_PLAYING)){
                    ALOGD("pMediaList.size() %d",pMediaList.size());
                    if (pMediaList.size() > 0 && start <= 1) {
                        list<MediaInfo>::iterator p = pMediaList.begin();
                        list<MediaInfo>::iterator p_end = pMediaList.begin();
                        if(end >= pMediaList.size())
                          p_end = pMediaList.end();
                        else
                          advance(p_end,end+1);
                        advance(p,start);
                        while (p != p_end) {
                            //if (start == 0) {
                                mediaEntry = (char*)osi_malloc(p->RetrieveMediaEntryLength()*
                                                               sizeof(char));
                                mediaEntry = p->RetrieveMediaItemEntry();
                                int length = p->RetrieveMediaEntryLength();
                                folderItemLengths[availableMedias ++] = length;
                                for (count = 0; count < length; count ++)
                                    folderItems[positionItemStart + count] = mediaEntry[count];
                                positionItemStart += length; // move start to next item star
                                osi_free(mediaEntry);
                            /*}
                            else if (start > 0) {
                                --start;
                            }*/
                            p++;
                        }
                    }
                    else
                        ALOGD(LOGTAG_AVRCP "  No Media");
                    ALOGD("availableMedias %d",availableMedias);
                    p_param = (btrc_vendor_folder_list_entries_t*)osi_malloc(
                                    sizeof(btrc_vendor_folder_list_entries_t));
                    p_param->status = BTRC_STS_NO_ERROR ;
                    p_param->uid_counter = 0;
                    p_param->item_count = availableMedias;
                    p_param->p_item_list =
                   (btrc_folder_items_t*) osi_malloc (p_param->item_count*
                                      sizeof(btrc_folder_items_t));
                    for (count = 0; count < p_param->item_count; count++) {
                        for (countTemp = 0; countTemp < BTRC_UID_SIZE; countTemp ++) {
                            p_param->p_item_list[count].media.uid[countTemp] =
                            folderItems[countTotalBytes]; countTotalBytes++;
                        }
                        p_param->p_item_list[count].item_type =
                        folderItems[countTotalBytes];
                        p_param->p_item_list[count].media.type =
                        BTRC_MEDIA_TYPE_AUDIO ;
                        countTotalBytes++;
                        p_param->p_item_list[count].media.charset_id =
                        (uint16_t)(folderItems[countTotalBytes] & 0x00ff); countTotalBytes++;
                        p_param->p_item_list[count].media.charset_id +=
                        (uint16_t)((folderItems[countTotalBytes] << 8) & 0xff00); countTotalBytes++;
                        uint16_t str_len = (uint16_t)(folderItems[countTotalBytes] & 0x00ff); countTotalBytes++;
                        str_len += (uint16_t)((folderItems[countTotalBytes] << 8) & 0xff00); countTotalBytes++;
                        for (countTemp = 0; countTemp < str_len; countTemp ++) {
                            p_param->p_item_list[count].media.name[countTemp] =
                            folderItems[countTotalBytes]; countTotalBytes++;
                        }
                        p_param->p_item_list[count].media.name[countTemp] = '\0';
                        p_param->p_item_list[count].media.num_attrs = folderitem -> mNumAttr;
                        countTotalBytes++;
                        p_param->p_item_list[count].media.p_attrs = ( btrc_element_attr_val_t*)osi_malloc( folderitem->mNumAttr*sizeof(btrc_element_attr_val_t));
                        for(i = 0 ;i < folderitem->mNumAttr; ++i) {
                            if( folderitem->mNumAttr == BTRC_MAX_ELEM_ATTR_SIZE )
                                p_param->p_item_list[count].media.p_attrs[i].attr_id = i;
                            else
                                p_param->p_item_list[count].media.p_attrs[i].attr_id = folderitem->p_attr[i];
                            memcpy(p_param->p_item_list[count].media.p_attrs[i].text,
                                   getString(p_param->p_item_list[count].media.p_attrs[i].attr_id, NULL),
                                   strlen(getString(p_param->p_item_list[count].media.p_attrs[i].attr_id, NULL)) + 1);
                            ALOGD(LOGTAG_AVRCP " %s attrid : %d value :%s ", __func__, p_param->p_item_list[count].media.p_attrs[i].attr_id,
                            p_param->p_item_list[count].media.p_attrs[i].text);
                        }
                        /*To check if byte feeding went well*/
                        checkLength += folderItemLengths[count];
                        ALOGD(LOGTAG_AVRCP "checkLength = %u countTotalBytes = %u strlen = %d ", checkLength,countTotalBytes,str_len);
                        if (checkLength != countTotalBytes)
                            ALOGE(LOGTAG_AVRCP "Error Populating Intermediate Media Entry");
                    }
                    if (p_param->item_count == 0) {
                        p_param->status = BTRC_STS_INV_RANGE;
                        ALOGE("%s: No folders", __func__);
                    }
                    sBtAvrcpTargetInterface->get_folder_items_list_rsp(&(pEvent->avrcpTargetEvent.bd_addr), (btrc_status_t)p_param->status, p_param->uid_counter,
                      p_param->item_count, p_param->p_item_list);
                    for(count = 0; count < p_param->item_count; count++) {
                       osi_free(p_param->p_item_list[count].media.p_attrs);
                    }
                    osi_free(pEvent->avrcpTargetEvent.buf_ptr);
                    osi_free(folderitem);
                    osi_free(folderItems);
                    osi_free(p_param->p_item_list);
                    osi_free(p_param);
                }
                break;
            case BTRC_SCOPE_SEARCH:
                //always returns first media elemrnt in the list during search rsp
                p_param = (btrc_vendor_folder_list_entries_t*)osi_malloc(
                                   sizeof(btrc_vendor_folder_list_entries_t));
                p_param->status = BTRC_STS_NO_ERROR ;
                p_param->uid_counter = 0;
                p_param->item_count =  0;

                if(!is_search_req_recieved) {
                    //send error response since search req is not recieved
                    p_param->status = BTRC_STS_BAD_CMD;
                    sBtAvrcpTargetInterface->get_folder_items_list_rsp(
                                             &(pEvent->avrcpTargetEvent.bd_addr),
                                             (btrc_status_t)p_param->status, p_param->uid_counter,
                                             p_param->item_count, p_param->p_item_list);
                    break;
                }

                if (pMediaList.size() > 0 && pCurMedia != pMediaList.end()) {
                    list<MediaInfo>::iterator p = pCurMedia;
                    mediaEntry = (char*)osi_malloc(p->RetrieveMediaEntryLength()*sizeof(char));
                    mediaEntry = p->RetrieveMediaItemEntry();
                    int length = p->RetrieveMediaEntryLength();
                    folderItemLengths[0] = length;
                    for (count = 0; count < length; count ++)
                        folderItems[positionItemStart + count] = mediaEntry[count];
                    osi_free(mediaEntry);
                    p_param->item_count =  1;
                    p_param->p_item_list = (btrc_folder_items_t*)
                                           osi_malloc(sizeof(btrc_folder_items_t));
                    for (countTemp = 0; countTemp < BTRC_UID_SIZE; countTemp ++) {
                        p_param->p_item_list->media.uid[countTemp] =
                        folderItems[countTotalBytes]; countTotalBytes++;
                    }
                    p_param->p_item_list->item_type =
                        folderItems[countTotalBytes];
                    p_param->p_item_list->media.type =
                        BTRC_MEDIA_TYPE_AUDIO ;
                    countTotalBytes++;
                    p_param->p_item_list->media.charset_id =
                        (uint16_t)(folderItems[countTotalBytes] & 0x00ff);
                    countTotalBytes++;
                    p_param->p_item_list->media.charset_id +=
                        (uint16_t)((folderItems[countTotalBytes] << 8) & 0xff00);
                    countTotalBytes++;
                    uint16_t str_len = (uint16_t)(folderItems[countTotalBytes] & 0x00ff);
                    countTotalBytes++;
                    str_len += (uint16_t)((folderItems[countTotalBytes] << 8) & 0xff00);
                    countTotalBytes++;
                    for (countTemp = 0; countTemp < str_len; countTemp ++) {
                        p_param->p_item_list->media.name[countTemp] =
                        folderItems[countTotalBytes]; countTotalBytes++;
                    }
                    p_param->p_item_list->media.name[countTemp] = '\0';
                    p_param->p_item_list->media.num_attrs =
                        folderItems[countTotalBytes]; countTotalBytes++;
                    /*To check if byte feeding went well*/
                    checkLength += folderItemLengths[count];
                    ALOGD(LOGTAG_AVRCP "checkLength = %u countTotalBytes = %u strlen = %d ",
                                      checkLength,countTotalBytes,str_len);
                    ++pCurMedia;
                    sBtAvrcpTargetInterface->get_folder_items_list_rsp(
                        &(pEvent->avrcpTargetEvent.bd_addr),
                        (btrc_status_t)p_param->status, p_param->uid_counter,
                        p_param->item_count, p_param->p_item_list);
                } else {
                    p_param->status = BTRC_STS_INV_RANGE;
                    sBtAvrcpTargetInterface->get_folder_items_list_rsp(
                                             &(pEvent->avrcpTargetEvent.bd_addr),
                                             (btrc_status_t)p_param->status, p_param->uid_counter,
                                             p_param->item_count, p_param->p_item_list);
                    pCurMedia = pMediaList.begin();

                    if(is_pts_test_enabled_) {
                        printf("Media items in media list:");
                        showMediaItem ();
                    }
                }

                osi_free(pEvent->avrcpTargetEvent.buf_ptr);
                osi_free(folderitem);
                osi_free(folderItems);
                osi_free(p_param->p_item_list);
                osi_free(p_param);
            }
            break;
        case AVRCP_TARGET_GET_TOTAL_NUM_OF_ITEMS_CB: {
            uint32_t num_items = 0;
            scope = pEvent->avrcpTargetEvent.arg1;
            switch(scope){
            case BTRC_SCOPE_PLAYER_LIST:
                num_items = 1;
                break;
            case BTRC_SCOPE_FILE_SYSTEM:
                num_items = 2;
                break;
            case BTRC_SCOPE_SEARCH:
                num_items = 1;
                break;
            case BTRC_SCOPE_NOW_PLAYING:
                num_items = 2;
                break;
            default:
                ALOGE(LOGTAG_AVRCP "Invalid scope id,scope id: %d", scope);
                break;
            }
            sBtAvrcpTargetInterface->
                    get_total_num_of_items_rsp(&pEvent->avrcpTargetEvent.bd_addr,
                                               BTRC_STS_NO_ERROR,0,num_items);
            } break;
        case AVRCP_TARGET_SEARCH_CB:
            //Always responds with one media item in search rsp
            if (pMediaList.size() > 0) {
                sBtAvrcpTargetInterface->search_rsp(&pEvent->avrcpTargetEvent.bd_addr,
                                                    BTRC_STS_NO_ERROR, 0, 1);
            } else {
                sBtAvrcpTargetInterface->search_rsp(&pEvent->avrcpTargetEvent.bd_addr,
                                                    BTRC_STS_NO_ERROR, 0, 0);
            }
            break;
        case AVRCP_TARGET_ABS_VOL_TIMEOUT:
            ALOGD(LOGTAG_AVRCP " MESSAGE_ABS_VOL_TIMEOUT: Volume change cmd timed out");
            mVolCmdSetInProgress = false;
            mVolCmdAdjustInProgress = false;
            break;
        case AVRCP_TARGET_SEND_VOL_UP_DOWN:
            ALOGD(LOGTAG_AVRCP " AVRCP_TARGET_SEND_VOL_UP_DOWN, event not handled ");
            dir = (AvrcKeyDir)pEvent->avrcpTargetEvent.arg3;
            ALOGD(LOGTAG_AVRCP " AVRCP_TARGET_SEND_VOL_UP_DOWN, dir = %d ", dir);
            if (dir == AVRC_KEY_UP)
            {
                sBtAvrcpTargetInterface->send_pass_through_cmd(&mConnectedDevice, CMD_ID_VOL_UP, 0);
                sBtAvrcpTargetInterface->send_pass_through_cmd(&mConnectedDevice, CMD_ID_VOL_UP, 1);
            }
            else if (dir == AVRC_KEY_DOWN)
            {
                sBtAvrcpTargetInterface->send_pass_through_cmd(&mConnectedDevice,
                                                                        CMD_ID_VOL_DOWN, 0);
                sBtAvrcpTargetInterface->send_pass_through_cmd(&mConnectedDevice,
                                                                        CMD_ID_VOL_DOWN, 1);
            }
            else
            {
                ALOGD(LOGTAG_AVRCP " AVRCP_TARGET_SEND_VOL_UP_DOWN: Invalid value");
            }
            break;
        case AVRCP_TARGET_VOLUME_CHANGED:
            if (!isAbsoluteVolumeSupported()) {
                ALOGD(LOGTAG_AVRCP "ignore AVRCP_TARGET_VOLUME_CHANGED");
                break;
            }
            absvol = pEvent->avrcpTargetEvent.arg3 & 0x7f; // discard MSB as it is RFD
            ctype = (AvrcRspType)pEvent->avrcpTargetEvent.arg4;
            ALOGD(LOGTAG_AVRCP " AVRCP_TARGET_VOLUME_CHANGED, vol = %d absvol = %d ctype = %x",
                    pEvent->avrcpTargetEvent.arg3, absvol, ctype);

            if (ctype == AVRC_RSP_ACCEPT || ctype == AVRC_RSP_REJ) {
                if ((mVolCmdSetInProgress == false) && (mVolCmdAdjustInProgress == false)) {
                    ALOGD(LOGTAG_AVRCP "Unsolicited response, ignored");
                    break;
                }
                pA2dpSource->StopSetAbsVolTimer();
                volAdj = mVolCmdAdjustInProgress;
                mVolCmdSetInProgress = false;
                mVolCmdAdjustInProgress = false;
            }

            volIndex = convertToAudioStreamVolume(absvol);
            ALOGD(LOGTAG_AVRCP " Volume Index = %d", volIndex);

            if (mInitialRemoteVolume == -1) {
                mInitialRemoteVolume = absvol;
            }

            if (mLocalVolume != volIndex && (ctype == AVRC_RSP_ACCEPT ||
                    ctype == AVRC_RSP_CHANGED || ctype == AVRC_RSP_INTERIM)) {
                /* If the volume has successfully changed */
                mLocalVolume = volIndex;
                if (mLastLocalVolume != -1 && ctype == AVRC_RSP_ACCEPT) {
                    if (mLastLocalVolume != volIndex) {
                        /* remote volume changed more than requested due to
                                      * local and remote has different volume steps */
                        ALOGD(LOGTAG_AVRCP "Remote returned vol does not match desired volume %d",
                        mLastLocalVolume, " vs %d", volIndex);
                        mLastLocalVolume = mLocalVolume;
                    }
                }

                // remember the remote volume value, as it's the one supported by remote
                if (volAdj) {
                    ALOGD(LOGTAG_AVRCP "TODO : remember the remote volume value,"
                                             "as it's the one supported by remote");
                }

                mRemoteVolume = absvol;
                pecentVolChanged = ((long)absvol * 100) / 0x7f;
                ALOGD(LOGTAG_AVRCP " percent volume changed: %d", pecentVolChanged, "%");
                fprintf(stdout, "percent volume changed: %d\n", pecentVolChanged);
            }
            else if (ctype == AVRC_RSP_REJ) {
                ALOGD(LOGTAG_AVRCP "setAbsoluteVolume call rejected");
            }
            break;
        case AVRCP_TARGET_SET_ABS_VOL:
            ALOGD(LOGTAG_AVRCP "AVRCP_TARGET_SET_ABS_VOL, vol step = %d",
                                               pEvent->avrcpTargetEvent.arg3);
            if (!isAbsoluteVolumeSupported()) {
                ALOGD(LOGTAG_AVRCP "ignore MESSAGE_SET_ABSOLUTE_VOLUME");
                break;
            }
            if (pEvent->avrcpTargetEvent.arg3 < 0 ||
                           pEvent->avrcpTargetEvent.arg3 > mAudioStreamMax) {
                ALOGD(LOGTAG_AVRCP "wrong vol step input");
                break;
            }
            if (mVolCmdSetInProgress || mVolCmdAdjustInProgress){
                ALOGD(LOGTAG_AVRCP "There is already a volume command in progress.");
                break;
            }
            if (mInitialRemoteVolume == -1) {
                ALOGD(LOGTAG_AVRCP "remote never tell us initial volume, black list it.");
                break;
            }
            avrcpVolume = std::min(AVRCP_MAX_VOL,
                          std::max(0, convertToAvrcpVolume(pEvent->avrcpTargetEvent.arg3)));
            isSetVol = sBtAvrcpTargetInterface->set_volume(avrcpVolume);
            if (isSetVol == BT_STATUS_SUCCESS) {
                pA2dpSource->StartSetAbsVolTimer();
                mVolCmdSetInProgress = true;
                mLastRemoteVolume = avrcpVolume;
                mLastLocalVolume = pEvent->avrcpTargetEvent.arg3;
            } else {
                ALOGE(LOGTAG_AVRCP "setVolumeNative failed");
            }
            break;
        case AVRCP_TARGET_TRACK_CHANGED:
            ALOGD(LOGTAG_AVRCP " AVRCP_TARGET_TRACK_CHANGED");
            mCurrentTrackID = TRACK_IS_SELECTED;
            if (mTrackChangeNotiType == BTRC_NOTIFICATION_TYPE_INTERIM) {
                TrackNumberRsp = mCurrentTrackID;
                mTrackChangeNotiType = BTRC_NOTIFICATION_TYPE_CHANGED;
                ALOGD(LOGTAG_AVRCP " TrackNumberRsp = %ld", TrackNumberRsp);
                for (int i = 0; i < 8; ++i) {
                    param.track[i] = (uint8_t) (TrackNumberRsp >> (56 - 8 * i));
                }
                sBtAvrcpTargetInterface->register_notification_rsp(BTRC_EVT_TRACK_CHANGE,
                        mTrackChangeNotiType,
                        &param);
            }
            break;
        case AVRCP_TARGET_ADDR_PLAYER_CHANGED:
            ALOGD(LOGTAG_AVRCP " AVRCP_TARGET_ADDR_PLAYER_CHANGED %d",
                                         pEvent->avrcpTargetEvent.arg3);
            if (mCurrentAddrPlayerId != pEvent->avrcpTargetEvent.arg3) {
                mPreviousAddrPlayerId = mCurrentAddrPlayerId;
                mCurrentAddrPlayerId = pEvent->avrcpTargetEvent.arg3;
                ALOGD(LOGTAG_AVRCP " mPreviousAddrPlayerId = %d mCurrentAddrPlayerId = %d",
                                     mPreviousAddrPlayerId, mCurrentAddrPlayerId);
                if (mAddrPlayerChangedNotiType == BTRC_NOTIFICATION_TYPE_INTERIM) {
                    mAddrPlayerChangedNotiType = BTRC_NOTIFICATION_TYPE_CHANGED;
                    param.player_id = mCurrentAddrPlayerId;
                    sBtAvrcpTargetInterface->register_notification_rsp(
                            BTRC_EVT_ADDR_PLAYER_CHANGE,
                            mAddrPlayerChangedNotiType, &param);
                    if (mPreviousAddrPlayerId != -1)
                        resetAndSendPlayerStatusReject();
                }
            }
            break;
        case AVRCP_TARGET_AVAIL_PLAYER_CHANGED:
            ALOGD(LOGTAG_AVRCP " AVRCP_TARGET_AVAIL_PLAYER_CHANGED");
            if (mAvailPlayerChangedNotiType == BTRC_NOTIFICATION_TYPE_INTERIM) {
                mAvailPlayerChangedNotiType = BTRC_NOTIFICATION_TYPE_CHANGED;
                sBtAvrcpTargetInterface->register_notification_rsp(
                        BTRC_EVT_AVAL_PLAYER_CHANGE,
                        mAvailPlayerChangedNotiType, &param);
            }
        case AVRCP_TARGET_UID_CHANGED:
            ALOGD(LOGTAG_AVRCP " AVRCP_TARGET_UID_CHANGED");
            if (mUidChangedNotiType == BTRC_NOTIFICATION_TYPE_INTERIM) {
                mUidChangedNotiType = BTRC_NOTIFICATION_TYPE_CHANGED;
                param.uids_changed.uid_counter =  0;
                ALOGD("param.uids_changed.uid_counter %d (database unaware player)",param.uids_changed.uid_counter);
                sBtAvrcpTargetInterface->register_notification_rsp(
                         BTRC_EVT_UIDS_CHANGED,
                        mUidChangedNotiType, &param);
            }
            break;
        case AVRCP_TARGET_NOW_PLAYING_CONTENT_CHANGED:{
            ALOGD(LOGTAG_AVRCP " AVRCP_TARGET_NOW_PLAYING_CONTENT_CHANGED");
            if(is_pts_test_enabled_) {
                pA2dpSource->pMediaList.push_back(MediaInfo (mediaUid3,BTRC_ITEM_MEDIA, 0x006A, 6, "abcNew", 0));
                fprintf(stdout,"add the new media(abcNew) to now playing list, the new now playing list is:\n");
                showMediaItem ();
            }
            if (mNowPlayingContentChangedNotiType == BTRC_NOTIFICATION_TYPE_INTERIM) {
                mNowPlayingContentChangedNotiType = BTRC_NOTIFICATION_TYPE_CHANGED;
                sBtAvrcpTargetInterface->register_notification_rsp(
                        BTRC_EVT_NOW_PLAYING_CONTENT_CHANGED,
                        mNowPlayingContentChangedNotiType, &param);
            }
            } break;
        case AVRCP_TARGET_GET_ELE_ATTR:
            num_attr = pEvent->avrcpTargetEvent.arg1;
            if (pEvent->avrcpTargetEvent.buf_ptr == NULL) {
                break;
            }
            ALOGD(LOGTAG_AVRCP " Send response for Get element attribute, num_attr %d", num_attr);
            item = (ItemAttr*)osi_malloc(sizeof(ItemAttr));
            memcpy(item, pEvent->avrcpTargetEvent.buf_ptr, pEvent->avrcpTargetEvent.buf_size);
            for (i = 0; i < num_attr; ++i) {
                ALOGD(LOGTAG_AVRCP " attr[%d] %d", i, item->p_attr[i]);
            }
            pAttrs = (btrc_element_attr_val_t*)osi_malloc(num_attr*sizeof(btrc_element_attr_val_t));
            for (int i = 0; i < num_attr; ++i) {
                pAttrs[i].attr_id = item->p_attr[i];
                memcpy(pAttrs[i].text, getString(pAttrs[i].attr_id, NULL),
                                strlen(getString(pAttrs[i].attr_id, NULL))+1);
                ALOGD(LOGTAG_AVRCP " %d %s", pAttrs[i].attr_id, pAttrs[i].text);
            }
            sBtAvrcpTargetInterface->get_element_attr_rsp(&pEvent->avrcpTargetEvent.bd_addr, (uint8_t)num_attr, pAttrs);
            osi_free(pEvent->avrcpTargetEvent.buf_ptr);
            osi_free(item->p_attr);
            osi_free(item);
            osi_free(pAttrs);
            use_bigger_metadata = false;
            break;
        case AVRCP_TARGET_GET_ITEM_ATTRIBUTES_REQ:
                num_attr = pEvent->avrcpTargetEvent.arg4;
                if (pEvent->avrcpTargetEvent.buf_ptr == NULL) {
                    break;
                }
                ALOGD(LOGTAG_AVRCP " Send response for Get item attribute, num_attr %d", num_attr);
                item = (ItemAttr*)osi_malloc(sizeof(ItemAttr));
                memcpy(item, pEvent->avrcpTargetEvent.buf_ptr, pEvent->avrcpTargetEvent.buf_size);
            for (i = 0; i < num_attr; ++i) {
                ALOGD(LOGTAG_AVRCP " attr[%d] %d", i, item->p_attr[i]);
            }
            pAttrs = (btrc_element_attr_val_t*)osi_malloc(num_attr*sizeof(btrc_element_attr_val_t));
            for (int i = 0; i < num_attr; ++i) {
                pAttrs[i].attr_id = item->p_attr[i];
                memcpy(pAttrs[i].text, getString(pAttrs[i].attr_id,item->mUid),
                                strlen(getString(pAttrs[i].attr_id, item->mUid))+1);
                ALOGD(LOGTAG_AVRCP " %d %s", pAttrs[i].attr_id, pAttrs[i].text);
            }
            if(pEvent->avrcpTargetEvent.arg1 != 0)
                sBtAvrcpTargetInterface->get_item_attr_rsp(&pEvent->avrcpTargetEvent.bd_addr, BTRC_STS_UID_CHANGED , 0, nullptr);
            else
                sBtAvrcpTargetInterface->get_item_attr_rsp(&pEvent->avrcpTargetEvent.bd_addr, BTRC_STS_NO_ERROR, (uint8_t)num_attr, pAttrs);
            osi_free(pEvent->avrcpTargetEvent.buf_ptr);
            osi_free(item->p_attr);
            osi_free(item);
            osi_free(pAttrs);
            use_bigger_metadata = false;
            break;
        case AVRCP_SET_EQUALIZER_VAL:
            mCurrentEqualizer = (AvrcKeyDir)pEvent->avrcpTargetEvent.arg3;
            if (mAppSettingChangedNotiType == BTRC_NOTIFICATION_TYPE_INTERIM) {
                mAppSettingChangedNotiType = BTRC_NOTIFICATION_TYPE_CHANGED;
                param.player_setting.num_attr = NUMPLAYER_ATTRIBUTE;
                param.player_setting.attr_ids[0] = ATTRIBUTE_EQUALIZER;
                param.player_setting.attr_values[0]= mCurrentEqualizer;
                param.player_setting.attr_ids[1] = ATTRIBUTE_REPEATMODE;
                param.player_setting.attr_values[1] = mCurrentRepeat;
                param.player_setting.attr_ids[2] = ATTRIBUTE_SHUFFLEMODE;
                param.player_setting.attr_values[2] = mCurrentShuffle;
                param.player_setting.attr_ids[3] = ATTRIBUTE_SCANMODE;
                param.player_setting.attr_values[3] = mCurrentScan;
                sBtAvrcpTargetInterface->register_notification_rsp(BTRC_EVT_APP_SETTINGS_CHANGED,
                            mAppSettingChangedNotiType, &param);
            }
            break;
        case AVRCP_SET_REPEAT_VAL:
            mCurrentRepeat = (AvrcKeyDir)pEvent->avrcpTargetEvent.arg3;
            if (mAppSettingChangedNotiType == BTRC_NOTIFICATION_TYPE_INTERIM) {
                mAppSettingChangedNotiType = BTRC_NOTIFICATION_TYPE_CHANGED;
                param.player_setting.num_attr = NUMPLAYER_ATTRIBUTE;
                param.player_setting.attr_ids[0] = ATTRIBUTE_EQUALIZER;
                param.player_setting.attr_values[0]= mCurrentEqualizer;
                param.player_setting.attr_ids[1] = ATTRIBUTE_REPEATMODE;
                param.player_setting.attr_values[1] = mCurrentRepeat;
                param.player_setting.attr_ids[2] = ATTRIBUTE_SHUFFLEMODE;
                param.player_setting.attr_values[2] = mCurrentShuffle;
                param.player_setting.attr_ids[3] = ATTRIBUTE_SCANMODE;
                param.player_setting.attr_values[3] = mCurrentScan;
                sBtAvrcpTargetInterface->register_notification_rsp(BTRC_EVT_APP_SETTINGS_CHANGED,
                     mAppSettingChangedNotiType, &param);
            }
            break;
        case AVRCP_SET_SHUFFLE_VAL:
            mCurrentShuffle = (AvrcKeyDir)pEvent->avrcpTargetEvent.arg3;
            if (mAppSettingChangedNotiType == BTRC_NOTIFICATION_TYPE_INTERIM) {
                mAppSettingChangedNotiType = BTRC_NOTIFICATION_TYPE_CHANGED;
                param.player_setting.num_attr = NUMPLAYER_ATTRIBUTE;
                param.player_setting.attr_ids[0] = ATTRIBUTE_EQUALIZER;
                param.player_setting.attr_values[0]= mCurrentEqualizer;
                param.player_setting.attr_ids[1] = ATTRIBUTE_REPEATMODE;
                param.player_setting.attr_values[1] = mCurrentRepeat;
                param.player_setting.attr_ids[2] = ATTRIBUTE_SHUFFLEMODE;
                param.player_setting.attr_values[2] = mCurrentShuffle;
                param.player_setting.attr_ids[3] = ATTRIBUTE_SCANMODE;
                param.player_setting.attr_values[3] = mCurrentScan;
                sBtAvrcpTargetInterface->register_notification_rsp(BTRC_EVT_APP_SETTINGS_CHANGED,
                       mAppSettingChangedNotiType, &param);
            }
            break;
        case AVRCP_SET_SCAN_VAL:
                mCurrentScan = (AvrcKeyDir)pEvent->avrcpTargetEvent.arg3;
            if (mAppSettingChangedNotiType == BTRC_NOTIFICATION_TYPE_INTERIM) {
                mAppSettingChangedNotiType = BTRC_NOTIFICATION_TYPE_CHANGED;
                param.player_setting.num_attr = NUMPLAYER_ATTRIBUTE;
                param.player_setting.attr_ids[0] = ATTRIBUTE_EQUALIZER;
                param.player_setting.attr_values[0]= mCurrentEqualizer;
                param.player_setting.attr_ids[1] = ATTRIBUTE_REPEATMODE;
                param.player_setting.attr_values[1] = mCurrentRepeat;
                param.player_setting.attr_ids[2] = ATTRIBUTE_SHUFFLEMODE;
                param.player_setting.attr_values[2] = mCurrentShuffle;
                param.player_setting.attr_ids[3] = ATTRIBUTE_SCANMODE;
                param.player_setting.attr_values[3] = mCurrentScan;
                sBtAvrcpTargetInterface->register_notification_rsp(BTRC_EVT_APP_SETTINGS_CHANGED,
                        mAppSettingChangedNotiType, &param);
            }
            break;
        case AVRCP_TARGET_PLAY_POSITION_TIMEOUT:
            param.song_pos = a2dp_play_position;
            pA2dpSource->StopPlayPostionTimer();
            if(sBtAvrcpTargetInterface != NULL)
            {
                mPlayPosChangedNotiType = BTRC_NOTIFICATION_TYPE_CHANGED;
                sBtAvrcpTargetInterface->register_notification_rsp(BTRC_EVT_PLAY_POS_CHANGED,
                                  mPlayPosChangedNotiType, &param);
            }
            else
                ALOGD(LOGTAG_AVRCP " sBtAvrcpTargetInterface == NULL, ignore calling the register_notification_rsp!");
            break;
        case AVRCP_TARGET_GET_PLAY_STATUS:
            ALOGD(LOGTAG_AVRCP " Send response for Get play status = %d",playStatus);
            pos = 10L;
            song_len = 100L;
            if(playStatus == BTRC_PLAYSTATE_ERROR)
            {
                playStatus = BTRC_PLAYSTATE_STOPPED;
                ALOGD(LOGTAG_AVRCP " set  play status as stopped = %d",playStatus);
            }
            sBtAvrcpTargetInterface->get_play_status_rsp(&pEvent->avrcpTargetEvent.bd_addr, playStatus,
                    song_len, pos);
            break;
        case AVRCP_TARGET_LIST_PLAYER_APP_ATTR:
            ALOGD(LOGTAG_AVRCP " Send response for list player app attr");
            num_attr =  4;
            p_attr[0] = BTRC_PLAYER_ATTR_EQUALIZER;
            p_attr[1] = BTRC_PLAYER_ATTR_REPEAT;
            p_attr[2] = BTRC_PLAYER_ATTR_SHUFFLE;
            p_attr[3] = BTRC_PLAYER_ATTR_SCAN;
            sBtAvrcpTargetInterface->list_player_app_attr_rsp(&pEvent->avrcpTargetEvent.bd_addr, num_attr, p_attr );
            break;
        case AVRCP_TARGET_LIST_PLAYER_APP_VALUES:
            ALOGD(LOGTAG_AVRCP "attr_id:%d", pEvent->avrcpTargetEvent.attr_id);
            switch(pEvent->avrcpTargetEvent.attr_id) {
                case BTRC_PLAYER_ATTR_EQUALIZER:
                    num_val = 2;
                    attr_values = (uint8_t *)osi_malloc(sizeof(uint8_t) * num_val);
                    attr_values[0] = BTRC_PLAYER_VAL_OFF_EQUALIZER;
                    attr_values[1] = BTRC_PLAYER_VAL_ON_EQUALIZER;
                    break;
                case BTRC_PLAYER_ATTR_REPEAT:
                    num_val = 4;
                    attr_values = (uint8_t *)osi_malloc(sizeof(uint8_t) * num_val);
                    attr_values[0] = BTRC_PLAYER_VAL_OFF_REPEAT;
                    attr_values[1] = BTRC_PLAYER_VAL_SINGLE_REPEAT;
                    attr_values[2] = BTRC_PLAYER_VAL_ALL_REPEAT;
                    attr_values[3] = BTRC_PLAYER_VAL_GROUP_REPEAT;
                    break;
                case BTRC_PLAYER_ATTR_SHUFFLE:
                    num_val = 3;
                    attr_values = (uint8_t *)osi_malloc(sizeof(uint8_t) * num_val);
                    attr_values[0] = BTRC_PLAYER_VAL_OFF_SHUFFLE;
                    attr_values[1] = BTRC_PLAYER_VAL_ALL_SHUFFLE;
                    attr_values[2] = BTRC_PLAYER_VAL_GROUP_SHUFFLE;
                    break;
                case BTRC_PLAYER_ATTR_SCAN:
                    num_val = 3;
                    attr_values = (uint8_t *)osi_malloc(sizeof(uint8_t) * num_val);
                    attr_values[0] = BTRC_PLAYER_VAL_OFF_SCAN;
                    attr_values[1] = BTRC_PLAYER_VAL_ON_SCAN;
                    attr_values[2] = BTRC_PLAYER_VAL_GRP_SCAN;
                    break;
            }
            sBtAvrcpTargetInterface->list_player_app_value_rsp(&pEvent->avrcpTargetEvent.bd_addr, num_val, attr_values );
            if (attr_values)
                osi_free(attr_values);
            break;
        case AVRCP_TARGET_GET_PLAYER_APP_VALUE:
            ALOGD(LOGTAG_AVRCP "No of attr:%d", pEvent->avrcpTargetEvent.arg3);
            btrc_player_settings_t get_app_rsp;
            memset(&get_app_rsp, 0, sizeof(btrc_player_settings_t));
            get_app_rsp.num_attr = pEvent->avrcpTargetEvent.arg3;
            for(i = 0; i < pEvent->avrcpTargetEvent.arg3; i++) {
                get_app_rsp.attr_ids[i] = pEvent->avrcpTargetEvent.attr_ids[i];
                if (pEvent->avrcpTargetEvent.attr_ids[i] == BTRC_PLAYER_ATTR_EQUALIZER) {
                    get_app_rsp.attr_values[i] = mCurrentEqualizer;
                } else if (pEvent->avrcpTargetEvent.attr_ids[i] == BTRC_PLAYER_ATTR_REPEAT) {
                    get_app_rsp.attr_values[i] = mCurrentRepeat;
                } else if (pEvent->avrcpTargetEvent.attr_ids[i] == BTRC_PLAYER_ATTR_SHUFFLE) {
                    get_app_rsp.attr_values[i] = mCurrentShuffle;
                } else if (pEvent->avrcpTargetEvent.attr_ids[i] == BTRC_PLAYER_ATTR_SCAN) {
                    get_app_rsp.attr_values[i] = mCurrentScan;
                }
            }
            sBtAvrcpTargetInterface->get_player_app_value_rsp(&pEvent->avrcpTargetEvent.bd_addr, &get_app_rsp );
            break;
        case AVRCP_TARGET_SET_PLAYER_APP_VALUE:
            for (i = 0; i < pEvent->avrcpTargetEvent.arg3; i++) {
                ALOGD(LOGTAG_AVRCP "attr_ids:%d", pEvent->avrcpTargetEvent.attr_ids[i]);
                ALOGD(LOGTAG_AVRCP "attr_values:%d", pEvent->avrcpTargetEvent.attr_values[i]);
                if (pEvent->avrcpTargetEvent.attr_ids[i] == BTRC_PLAYER_ATTR_EQUALIZER)
                    mCurrentEqualizer = pEvent->avrcpTargetEvent.attr_values[i];
                else if (pEvent->avrcpTargetEvent.attr_ids[i] == BTRC_PLAYER_ATTR_REPEAT)
                    mCurrentRepeat = pEvent->avrcpTargetEvent.attr_values[i];
                else if (pEvent->avrcpTargetEvent.attr_ids[i] == BTRC_PLAYER_ATTR_SHUFFLE)
                    mCurrentShuffle = pEvent->avrcpTargetEvent.attr_values[i];
                else if (pEvent->avrcpTargetEvent.attr_ids[i] == BTRC_PLAYER_ATTR_SCAN)
                    mCurrentScan = pEvent->avrcpTargetEvent.attr_values[i];
            }
            sBtAvrcpTargetInterface->set_player_app_value_rsp(&pEvent->avrcpTargetEvent.bd_addr, BTRC_STS_NO_ERROR);
            break;
        case AVRCP_TARGET_REG_NOTI:
            switch(pEvent->avrcpTargetEvent.arg1) {
                case BTRC_EVT_PLAY_STATUS_CHANGED :
                    ALOGD(LOGTAG_AVRCP " AVRCP_TARGET_REG_NOTI: BTRC_EVT_PLAY_STATUS_CHANGED %d",playStatus);
                    fprintf(stdout, "AVRCP_TARGET_REG_NOTI: BTRC_EVT_PLAY_STATUS_CHANGED %d\n", playStatus);
                    mPlayStatusNotiType = BTRC_NOTIFICATION_TYPE_INTERIM;
                    param.play_status = playStatus;
                    sBtAvrcpTargetInterface->register_notification_rsp(BTRC_EVT_PLAY_STATUS_CHANGED,
                            mPlayStatusNotiType, &param);
                    break;
                case BTRC_EVT_TRACK_CHANGE:
                    ALOGD(LOGTAG_AVRCP " AVRCP_TARGET_REG_NOTI: BTRC_EVT_TRACK_CHANGE");
                    mTrackChangeNotiType = BTRC_NOTIFICATION_TYPE_INTERIM;
                    TrackNumberRsp = mCurrentTrackID;
                    ALOGD(LOGTAG_AVRCP " TrackNumberRsp = %l", TrackNumberRsp);
                    for (int i = 0; i < 8; ++i) {
                        param.track[i] = (uint8_t) (TrackNumberRsp >> (56 - 8 * i));
                    }
                    sBtAvrcpTargetInterface->register_notification_rsp(BTRC_EVT_TRACK_CHANGE,
                            mTrackChangeNotiType, &param);
                    break;
                case BTRC_EVT_PLAY_POS_CHANGED:
                    ALOGD(LOGTAG_AVRCP "AVRCP_TARGET_REG_NOTI: BTRC_EVT_PLAY_POS_CHANGED");
                    ALOGD(LOGTAG_AVRCP "play_position_interval:%d", pEvent->avrcpTargetEvent.arg2);
                    param.song_pos = a2dp_play_position;
                    mPlayPosChangedNotiType = BTRC_NOTIFICATION_TYPE_INTERIM;
                    play_position_interval = pEvent->avrcpTargetEvent.arg2;  //Interval sec
                    sBtAvrcpTargetInterface->register_notification_rsp(BTRC_EVT_PLAY_POS_CHANGED,
                                    mPlayPosChangedNotiType, &param);
                    if (playStatus == BTRC_PLAYSTATE_PLAYING)
                        pA2dpSource->StartPlayPostionTimer();
                    break;
                case BTRC_EVT_APP_SETTINGS_CHANGED:
                    ALOGD(LOGTAG_AVRCP " AVRCP_TARGET_REG_NOTI: BTRC_EVT_APP_SETTINGS_CHANGED");
                    mAppSettingChangedNotiType = BTRC_NOTIFICATION_TYPE_INTERIM;
                    param.player_setting.num_attr = NUMPLAYER_ATTRIBUTE;
                    param.player_setting.attr_ids[0] = ATTRIBUTE_EQUALIZER;
                    param.player_setting.attr_values[0]= mCurrentEqualizer;
                    param.player_setting.attr_ids[1] = ATTRIBUTE_REPEATMODE;
                    param.player_setting.attr_values[1] = mCurrentRepeat;
                    param.player_setting.attr_ids[2] = ATTRIBUTE_SHUFFLEMODE;
                    param.player_setting.attr_values[2] = mCurrentShuffle;
                    param.player_setting.attr_ids[3] = ATTRIBUTE_SCANMODE;
                    param.player_setting.attr_values[3] = mCurrentScan;
                    sBtAvrcpTargetInterface->register_notification_rsp(BTRC_EVT_APP_SETTINGS_CHANGED,
                                           mAppSettingChangedNotiType, &param);
                    break;
                case BTRC_EVT_ADDR_PLAYER_CHANGE:
                    ALOGD(LOGTAG_AVRCP "AVRCP_TARGET_REG_NOTI: BTRC_EVT_ADDR_PLAYER_CHANGE ");
                    mAddrPlayerChangedNotiType = BTRC_NOTIFICATION_TYPE_INTERIM;
                    param.player_id = (uint16_t)mCurrentAddrPlayerId;
                    sBtAvrcpTargetInterface->register_notification_rsp(
                            BTRC_EVT_ADDR_PLAYER_CHANGE,
                            mAddrPlayerChangedNotiType, &param);
                    break;
                case BTRC_EVT_AVAL_PLAYER_CHANGE:
                    ALOGD(LOGTAG_AVRCP "AVRCP_TARGET_REG_NOTI: BTRC_EVT_AVAL_PLAYER_CHANGE ");
                    mAvailPlayerChangedNotiType = BTRC_NOTIFICATION_TYPE_INTERIM;
                    sBtAvrcpTargetInterface->register_notification_rsp(
                            BTRC_EVT_AVAL_PLAYER_CHANGE,
                            mAvailPlayerChangedNotiType, &param);
                    break;
                case BTRC_EVT_UIDS_CHANGED :
                    ALOGD(LOGTAG_AVRCP "AVRCP_TARGET_REG_NOTI: BTRC_EVT_UIDS_CHANGED ");
                    mUidChangedNotiType = BTRC_NOTIFICATION_TYPE_INTERIM;
                    param.uids_changed.uid_counter =  0;
                    ALOGD("param.uids_changed.uid_counter %d (database unaware player)",param.uids_changed.uid_counter);
                    sBtAvrcpTargetInterface->register_notification_rsp(
                            BTRC_EVT_UIDS_CHANGED,
                            mUidChangedNotiType, &param);
                    break;
                case BTRC_EVT_NOW_PLAYING_CONTENT_CHANGED :
                    ALOGD(LOGTAG_AVRCP "AVRCP_TARGET_REG_NOTI: BTRC_EVT_NOW_PLAYING_CONTENT_CHANGED ");
                    mNowPlayingContentChangedNotiType = BTRC_NOTIFICATION_TYPE_INTERIM;
                    sBtAvrcpTargetInterface->register_notification_rsp(
                            BTRC_EVT_NOW_PLAYING_CONTENT_CHANGED,
                            mNowPlayingContentChangedNotiType, &param);
                    break;
                default:
                    ALOGE(LOGTAG_AVRCP "AVRCP_TARGET_REG_NOTI: unhandled event ");
                    break;
            }
            break;
        case AVRCP_TARGET_CONNECTED_CB:
            mAvrcpConnected = true;
            memcpy(&mConnectedAvrcpDevice, &pEvent->avrcpTargetEvent.bd_addr, sizeof(bt_bdaddr_t));
            break;
        case AVRCP_TARGET_DISCONNECTED_CB:
            mAvrcpConnected = false;
            is_search_req_recieved = false;
            mfolder_depth = 0;
            memset(&mConnectedAvrcpDevice, 0, sizeof(bt_bdaddr_t));
            break;
        case A2DP_SOURCE_AUDIO_CMD_REQ:{
            key_id = pEvent->avrcpTargetEvent.key_id;
            if(is_pts_test_enabled_)
                fprintf(stdout, "receive the %s(Note: This only represents the receipt of the command, not the actual effect.)\n", pass_through_cmd_id_to_str(key_id));
            if (!mAvrcpConnected || (memcmp(&mConnectedAvrcpDevice, &mConnectedDevice,
                           sizeof(bt_bdaddr_t)) != 0)) {
                ALOGD(LOGTAG_AVRCP " No Active connection. Bail out!! ");
                break;
            }
            switch(key_id) {
                case CMD_ID_PLAY:
                    if (media_playing)
                        BtA2dpResumeStreaming();
                    else
                        BtA2dpStartStreaming();
                    if (playStatus != BTRC_PLAYSTATE_PLAYING)
                    {
                        playStatus = BTRC_PLAYSTATE_PLAYING;
                        if (mPlayStatusNotiType == BTRC_NOTIFICATION_TYPE_INTERIM) {
                            param.play_status = playStatus;
                            mPlayStatusNotiType = BTRC_NOTIFICATION_TYPE_CHANGED;
                            sBtAvrcpTargetInterface->register_notification_rsp(
                                    BTRC_EVT_PLAY_STATUS_CHANGED,
                                    mPlayStatusNotiType, &param);
                        }
                    }
                    mCurrentTrackID = TRACK_IS_SELECTED;
                    if (mTrackChangeNotiType == BTRC_NOTIFICATION_TYPE_INTERIM)
                    {
                        TrackNumberRsp = mCurrentTrackID;
                        mTrackChangeNotiType = BTRC_NOTIFICATION_TYPE_CHANGED;
                        ALOGD(LOGTAG_AVRCP " TrackNumberRsp = %l", TrackNumberRsp);
                        for (int i = 0; i < 8; ++i) {
                            param.track[i] = (uint8_t) (TrackNumberRsp >> (56 - 8 * i));
                        }
                        sBtAvrcpTargetInterface->register_notification_rsp(
                                BTRC_EVT_TRACK_CHANGE,
                                mTrackChangeNotiType, &param);
                    }
                    if (mPlayPosChangedNotiType == BTRC_NOTIFICATION_TYPE_INTERIM)
                        pA2dpSource->StartPlayPostionTimer();

                    break;
                case CMD_ID_PAUSE:
                    /*Pause key id is mapped to A2dp suspend*/
                    BtA2dpSuspendStreaming();
                    pA2dpSource->StopPlayPostionTimer();
                    if (playStatus != BTRC_PLAYSTATE_PAUSED)
                    {
                        playStatus = BTRC_PLAYSTATE_PAUSED;
                        if (mPlayStatusNotiType == BTRC_NOTIFICATION_TYPE_INTERIM) {
                            param.play_status = playStatus;
                            mPlayStatusNotiType = BTRC_NOTIFICATION_TYPE_CHANGED;
                            sBtAvrcpTargetInterface->register_notification_rsp(
                                    BTRC_EVT_PLAY_STATUS_CHANGED,
                                    mPlayStatusNotiType, &param);
                        }
                    }
                    break;
                case CMD_ID_STOP:
                    /*Pause and Stop passthrough commands are handled here*/
                    BtA2dpSuspendStreaming();
                    pA2dpSource->StopPlayPostionTimer();
                    if (playStatus != BTRC_PLAYSTATE_STOPPED)
                    {
                        playStatus = BTRC_PLAYSTATE_STOPPED;
                        if (mPlayStatusNotiType == BTRC_NOTIFICATION_TYPE_INTERIM) {
                            param.play_status = playStatus;
                            mPlayStatusNotiType = BTRC_NOTIFICATION_TYPE_CHANGED;
                            sBtAvrcpTargetInterface->register_notification_rsp(
                                    BTRC_EVT_PLAY_STATUS_CHANGED,
                                    mPlayStatusNotiType, &param);
                        }
                    }
                    break;
                default:
                   ALOGE(LOGTAG_AVRCP " Command not supported ");
                   break;
            }
            break;
        case A2DP_SOURCE_AUDIO_AVDT_CMD_REQ:
            key_id = pEvent->avrcpTargetEvent.key_id;
            switch(key_id) {
                case CMD_ID_PLAY:
                    if (media_playing)
                        BtA2dpResumeStreaming();
                    else
                        BtA2dpStartStreaming();
                    break;
                case CMD_ID_PAUSE:
                    /*Pause key id is mapped to A2dp suspend*/
                    BtA2dpSuspendStreaming();
                    break;
                default:
                   ALOGE(LOGTAG_AVRCP " Command not supported ");
                   break;
            }
            break;
            }
        case AVRCP_TARGET_SET_BROWSED_PLAYER_REQ:
            set_br_player_id = pEvent->avrcpTargetEvent.arg1;
            if (pMediaPlayerList.size() > 0) {
                list<MediaPlayerInfo>::iterator p = pMediaPlayerList.begin();
                while (p != pMediaPlayerList.end()) {
                    if (p->mPlayerId == set_br_player_id)
                    {
                        player_found = true;
                        ALOGD(LOGTAG_AVRCP " valid player found for set br player ");
                        break;
                    }
                    p++;
                }
            }
            else {
                ALOGE(LOGTAG_AVRCP "  No media players");
            }
            if (!player_found)
            {
                ALOGE(LOGTAG_AVRCP " Not valid player %d, send error", set_br_player_id);
                sBtAvrcpTargetInterface->set_browsed_player_rsp(&(pEvent->avrcpTargetEvent.bd_addr), BTRC_STS_INV_PLAYER , 2, 0x6A, 0, nullptr);
                break;
            }
            if (set_br_player_id != mCurrentAddressedPlayer)
            {
                ALOGE(LOGTAG_AVRCP " Player %d is not the addressed player, send error", set_br_player_id);
                sBtAvrcpTargetInterface->set_browsed_player_rsp(&(pEvent->avrcpTargetEvent.bd_addr), BTRC_STS_PLAY_NOT_ADDR, 2, 0x6A, 0, nullptr);
                break;
            }
            ALOGD(LOGTAG_AVRCP " Send response for set browsed player %d", set_br_player_id);
            sBtAvrcpTargetInterface->set_browsed_player_rsp(&(pEvent->avrcpTargetEvent.bd_addr), (btrc_status_t)BTRC_STS_NO_ERROR, 2, 0x6A, mfolder_depth, mp_folders);
            break;
        case AVRCP_TARGET_CHANGE_PATH_REQ:
            if(mfolder_depth ==0){
                if(pEvent->avrcpTargetEvent.arg3 ==0)
                    sBtAvrcpTargetInterface->change_path_rsp(&(pEvent->avrcpTargetEvent.bd_addr), (btrc_status_t)BTRC_STS_INV_DIRN, 0);
                else if(uid_cmp(pEvent->avrcpTargetEvent.buf_ptr, folderUid1) && (uid_cmp(pEvent->avrcpTargetEvent.buf_ptr, folderUid2)))
                    /*AVRCP/TG/MCN/CB/BI-04-C requires TG should sent "Does Not Exist" error code to PTS when an invalid  folder uid is requested*/
                    if(is_pts_test_enabled_)
                        sBtAvrcpTargetInterface->change_path_rsp(&(pEvent->avrcpTargetEvent.bd_addr), (btrc_status_t)BTRC_STS_INV_ITEM, 0);
                    else
                        sBtAvrcpTargetInterface->change_path_rsp(&(pEvent->avrcpTargetEvent.bd_addr), (btrc_status_t)BTRC_STS_INV_DIRECTORY, 0);
                else if((!uid_cmp(pEvent->avrcpTargetEvent.buf_ptr, folderUid1)
                    ||(!uid_cmp(pEvent->avrcpTargetEvent.buf_ptr, folderUid2)))&&(pEvent->avrcpTargetEvent.arg3 ==1)){
                    is_empty_folder = (uid_cmp(pEvent->avrcpTargetEvent.buf_ptr, folderUid2))? 0:1;
                    int no_items = (is_empty_folder)? 0:2;
                    mfolder_depth = 1;
                    mp_folders = (btrc_br_folder_name_t*)osi_malloc(sizeof(btrc_br_folder_name_t));
                    mp_folders->str_len = rootStringLength;
                    for (int count = 0; count < mp_folders->str_len; count++)
                        mp_folders->p_str[count] = (char)rootString[count];
                    sBtAvrcpTargetInterface->change_path_rsp(&(pEvent->avrcpTargetEvent.bd_addr), (btrc_status_t)BTRC_STS_NO_ERROR, no_items);
                }

            }
            else if(mfolder_depth ==1){
                if(pEvent->avrcpTargetEvent.arg3 ==0){
                    mfolder_depth = 0;
                    mp_folders = nullptr;
                    sBtAvrcpTargetInterface->change_path_rsp(&(pEvent->avrcpTargetEvent.bd_addr), (btrc_status_t)BTRC_STS_NO_ERROR, 2);
                }
                else if(pEvent->avrcpTargetEvent.arg3 ==1)
                    sBtAvrcpTargetInterface->change_path_rsp(&(pEvent->avrcpTargetEvent.bd_addr), (btrc_status_t)BTRC_STS_INV_DIRN, 0);

            }
            osi_free(pEvent->avrcpTargetEvent.buf_ptr);
            break;
        case AVRCP_TARGET_PLAY_ITEMS_REQ:
            if((pEvent->avrcpTargetEvent.arg3 == BTRC_SCOPE_FILE_SYSTEM)&&(mfolder_depth != 1))
                sBtAvrcpTargetInterface->play_item_rsp(&pEvent->avrcpTargetEvent.bd_addr, BTRC_STS_INV_ITEM);
            else if((pEvent->avrcpTargetEvent.arg3 == BTRC_SCOPE_FILE_SYSTEM)&&((!uid_cmp(pEvent->avrcpTargetEvent.buf_ptr, rootUid))
                ||(!uid_cmp(pEvent->avrcpTargetEvent.buf_ptr, folderUid1))||(!uid_cmp(pEvent->avrcpTargetEvent.buf_ptr, folderUid2))))
                    sBtAvrcpTargetInterface->play_item_rsp(&pEvent->avrcpTargetEvent.bd_addr, BTRC_STS_DIRECTORY);
            else if((!uid_cmp(pEvent->avrcpTargetEvent.buf_ptr, mediaUid1)) || (!uid_cmp(pEvent->avrcpTargetEvent.buf_ptr, mediaUid2))) {
                pA2dpSource->pMediaList.push_back(MediaInfo (mediaUid3,BTRC_ITEM_MEDIA, 0x006A, 6, "abcNew", 0));
                sBtAvrcpTargetInterface->play_item_rsp(&pEvent->avrcpTargetEvent.bd_addr, BTRC_STS_NO_ERROR);
            }
            else
                    sBtAvrcpTargetInterface->play_item_rsp(&pEvent->avrcpTargetEvent.bd_addr, BTRC_STS_INV_ITEM);
            osi_free(pEvent->avrcpTargetEvent.buf_ptr);
            break;
        case AVRCP_TARGET_ADDTO_NOW_PLAYING_REQ:
        ALOGD("AVRCP_TARGET_ADDTO_NOW_PLAYING_REQ");
            if((pEvent->avrcpTargetEvent.arg3 == BTRC_SCOPE_FILE_SYSTEM)&&(mfolder_depth != 1))
            sBtAvrcpTargetInterface->add_to_now_playing_rsp(&pEvent->avrcpTargetEvent.bd_addr, BTRC_STS_INV_ITEM);
            else if((pEvent->avrcpTargetEvent.arg3 == BTRC_SCOPE_FILE_SYSTEM)&&((!uid_cmp(pEvent->avrcpTargetEvent.buf_ptr, rootUid))
                ||(!uid_cmp(pEvent->avrcpTargetEvent.buf_ptr, folderUid1))||(!uid_cmp(pEvent->avrcpTargetEvent.buf_ptr, folderUid2))))
                    sBtAvrcpTargetInterface->add_to_now_playing_rsp(&pEvent->avrcpTargetEvent.bd_addr, BTRC_STS_DIRECTORY);
            else if((!uid_cmp(pEvent->avrcpTargetEvent.buf_ptr, mediaUid1)) || (!uid_cmp(pEvent->avrcpTargetEvent.buf_ptr, mediaUid2)))
            sBtAvrcpTargetInterface->add_to_now_playing_rsp(&pEvent->avrcpTargetEvent.bd_addr, BTRC_STS_NO_ERROR);
            else
                    sBtAvrcpTargetInterface->add_to_now_playing_rsp(&pEvent->avrcpTargetEvent.bd_addr, BTRC_STS_INV_ITEM);
            osi_free(pEvent->avrcpTargetEvent.buf_ptr);
            break;
    }
}

void A2dp_Source::HandleEnableSource(void) {
    BtEvent *pEvent = new BtEvent;
    uint8_t streaming_param = 0;
    char value[PROPERTY_VALUE_MAX] = {'\0'};
    set_abs_volume_timer = alarm_new();
    set_play_postion_timer = alarm_new();
    if (bluetooth_interface != NULL)
    {
        sBtA2dpSourceInterface = (btav_source_interface_t *)bluetooth_interface->
                get_profile_interface(BT_PROFILE_ADVANCED_AUDIO_ID);
        sBtA2dpSourceVendorInterface = (btav_vendor_interface_t *)bluetooth_interface->
                get_profile_interface(BT_PROFILE_ADVANCED_AUDIO_VENDOR_ID);
        if (sBtA2dpSourceInterface == NULL)
        {
             pEvent->profile_start_event.event_id = PROFILE_EVENT_START_DONE;
             pEvent->profile_start_event.profile_id = PROFILE_ID_A2DP_SOURCE;
             pEvent->profile_start_event.status = false;
             PostMessage(THREAD_ID_GAP, pEvent);
             return;
        }

        pump_encoded_data = config_get_bool (config, CONFIG_DEFAULT_SECTION,
                                              "BtA2dpPumpEncodedData", false);
        if(pump_encoded_data)
            streaming_param |= A2DP_SRC_PUMP_ENCODED_DATA;
        ALOGD(LOGTAG_A2DP " Try to get config , pump_encoded_data %d", pump_encoded_data);
        int numConfigs = BTAV_A2DP_CODEC_INDEX_SOURCE_MAX - BTAV_A2DP_CODEC_INDEX_SOURCE_MIN;
        int priority_values[numConfigs];
        priority_values[0]   = config_get_int (config, CONFIG_DEFAULT_SECTION,
                                            "a2dp_source_codec_priority_sbc", 1001);
        priority_values[1]   = config_get_int (config, CONFIG_DEFAULT_SECTION,
                                            "a2dp_source_codec_priority_aac", 2001);
        priority_values[2]   = config_get_int (config, CONFIG_DEFAULT_SECTION,
                                            "a2dp_source_codec_priority_aptx",3001);
        priority_values[3]   = config_get_int (config, CONFIG_DEFAULT_SECTION,
                                            "a2dp_source_codec_priority_aptx_hd",4001);
        priority_values[4]   = config_get_int (config, CONFIG_DEFAULT_SECTION,
                                            "a2dp_source_codec_priority_aptx_ad",6001);
        priority_values[5]   = config_get_int (config, CONFIG_DEFAULT_SECTION,
                                            "a2dp_source_codec_priority_ldac",5001);

        ALOGD(LOGTAG_A2DP "assignCodecConfigPriorities");
        assignCodecConfigPriorities(priority_values, numConfigs);
        sBtA2dpSourceInterface->init(&sBluetoothA2dpSourceCallbacks, 1, a2dpSrcCodecList);
        osi_property_get("persist.bt.a2dp_offload_cap", value, "false");
        ALOGD(LOGTAG_A2DP "offload_cap:%s", value);
        if (strcmp(value, "false") != 0)
            bt_a2dp_split_enabled = true;

        sBtA2dpSourceVendorInterface->init_vendor(
            &sBluetoothA2dpSourceVendorCallbacks, 1, 0, streaming_param);
        pEvent->profile_start_event.event_id = PROFILE_EVENT_START_DONE;
        pEvent->profile_start_event.profile_id = PROFILE_ID_A2DP_SOURCE;
        pEvent->profile_start_event.status = true;
        // AVRCP TG Initialization
        sBtAvrcpTargetInterface = (btrc_interface_t *)bluetooth_interface->
                get_profile_interface(BT_PROFILE_AV_RC_ID);
        if (sBtAvrcpTargetInterface != NULL) {
        //TODO: check and update
            sBtAvrcpTargetInterface->init(&sBluetoothAvrcpTargetCallbacks);
        }
        change_state(STATE_A2DP_SOURCE_DISCONNECTED);
        PostMessage(THREAD_ID_GAP, pEvent);
        ALOGD(LOGTAG_A2DP "Calling BtA2dpLoadA2dpHal");
        BtA2dpLoadA2dpHal();
        media_playing = false;
        playStatus = BTRC_PLAYSTATE_STOPPED;
        mCurrentTrackID = NO_TRACK_SELECTED;
        mCurrentAddressedPlayer = DEFAULT_PLAYER_ADDRESSED;
        mfolder_depth = 0;
        mp_folders = nullptr;
        registerMediaPlayers();
    }
    a2dp_sink_relay_data_list = list_new(NULL);
#if defined(BT_AUDIO_PAL_INTEGRATION)
    pa_routing_intf = NULL;
#endif
}

void A2dp_Source::HandleDisableSource(void) {
   change_state(STATE_A2DP_SOURCE_NOT_STARTED);
   BtA2dpUnloadA2dpHal();
   alarm_free(set_abs_volume_timer);
   alarm_free(set_play_postion_timer);
   set_abs_volume_timer = NULL;
   set_play_postion_timer = NULL;
   if(sBtA2dpSourceInterface != NULL) {
       sBtA2dpSourceInterface->cleanup();
       sBtA2dpSourceInterface = NULL;
   }
   if (sBtAvrcpTargetInterface != NULL) {
       sBtAvrcpTargetInterface->cleanup();
       sBtAvrcpTargetInterface = NULL;
   }
   BtEvent *pEvent = new BtEvent;
   pEvent->profile_stop_event.event_id = PROFILE_EVENT_STOP_DONE;
   pEvent->profile_stop_event.profile_id = PROFILE_ID_A2DP_SOURCE;
   pEvent->profile_stop_event.status = true;
   PostMessage(THREAD_ID_GAP, pEvent);
   media_playing = false;
   is_search_req_recieved = false;
   playStatus = BTRC_PLAYSTATE_STOPPED;
   mCurrentTrackID = NO_TRACK_SELECTED;
   mCurrentAddressedPlayer = DEFAULT_PLAYER_ADDRESSED;
   if (a2dp_sink_relay_data_list != NULL) {
       list_free(a2dp_sink_relay_data_list);
       a2dp_sink_relay_data_list = NULL;
   }
   unregisterMediaPlayers();
   a2dpSrcCodecList.clear();
#if defined(BT_AUDIO_PAL_INTEGRATION)
    if (pa_routing_intf) {
        int ret = pa_routing_intf->pa_bt_connect_fn(PA_BT_A2DP_SOURCE, false);
        if (!ret) {
            ALOGD(LOGTAG " BT a2dp source disconnect success");
            fprintf(stdout, "BT a2dp source disconnect success\n");
        }
        else {
            ALOGD(LOGTAG " BT a2dp source disconnect failed");
            fprintf(stdout, "BT a2dp source disconnect failed\n");
        }
       pa_routing_intf_close(pa_routing_intf);
       pa_routing_intf = NULL;
   }
#endif
}

void A2dp_Source::ProcessEvent(BtEvent* pEvent) {
    switch(mSourceState) {
        case STATE_A2DP_SOURCE_DISCONNECTED:
            state_disconnected_handler(pEvent);
            break;
        case STATE_A2DP_SOURCE_PENDING:
            state_pending_handler(pEvent);
            break;
        case STATE_A2DP_SOURCE_CONNECTED:
            state_connected_handler(pEvent);
            break;
        case STATE_A2DP_SOURCE_NOT_STARTED:
            fprintf(stdout, "Ignore!! Make sure BT is turned on!!\n");
            ALOGE(LOGTAG_A2DP " STATE UNINITIALIZED, return");
            break;
    }
}

char* A2dp_Source::dump_message(BluetoothEventId event_id) {
    switch(event_id) {
    case A2DP_SOURCE_API_CONNECT_REQ:
        return"API_CONNECT_REQ";
    case A2DP_SOURCE_API_DISCONNECT_REQ:
        return "API_DISCONNECT_REQ";
    case A2DP_SOURCE_DISCONNECTED_CB:
        return "DISCONNECTED_CB";
    case A2DP_SOURCE_CONNECTING_CB:
        return "CONNECING_CB";
    case A2DP_SOURCE_CONNECTED_CB:
        return "CONNECTED_CB";
    case A2DP_SOURCE_DISCONNECTING_CB:
        return "DISCONNECTING_CB";
    case A2DP_SOURCE_AUDIO_SUSPENDED:
        return "AUDIO_SUSPENDED_CB";
    case A2DP_SOURCE_AUDIO_STOPPED:
        return "AUDIO_STOPPED_CB";
    case A2DP_SOURCE_AUDIO_STARTED:
        return "AUDIO_STARTED_CB";
    case AVRCP_TARGET_CONNECTED_CB:
        return "AVRCP_TARGET_CONNECTED_CB";
    case AVRCP_TARGET_DISCONNECTED_CB:
        return "AVRCP_TARGET_DISCONNECTED_CB";
    case A2DP_SOURCE_AUDIO_CMD_REQ:
        return "AUDIO_CMD_REQ";
    case A2DP_SOURCE_AUDIO_AVDT_CMD_REQ:
        return "AUDIO_AVDT_CMD_REQ";
    case AVRCP_TARGET_GET_ELE_ATTR:
        return "AVRCP_TARGET_GET_ELE_ATTR";
    case AVRCP_TARGET_GET_PLAY_STATUS:
        return "AVRCP_TARGET_GET_PLAY_STATUS";
    case AVRCP_TARGET_REG_NOTI:
        return "AVRCP_TARGET_REG_NOTI";
    case AVRCP_TARGET_TRACK_CHANGED:
        return "AVRCP_TARGET_TRACK_CHANGED";
    case AVRCP_TARGET_NOW_PLAYING_CONTENT_CHANGED:
        return "AVRCP_TARGET_NOW_PLAYING_CONTENT_CHANGED";
    case AVRCP_TARGET_VOLUME_CHANGED:
        return "AVRCP_TARGET_VOLUME_CHANGED";
    case AVRCP_TARGET_SET_ABS_VOL:
        return "AVRCP_TARGET_SET_ABS_VOL";
    case AVRCP_TARGET_ABS_VOL_TIMEOUT:
        return "AVRCP_TARGET_ABS_VOL_TIMEOUT";
    case AVRCP_TARGET_SEND_VOL_UP_DOWN:
        return "AVRCP_TARGET_SEND_VOL_UP_DOWN";
    case AVRCP_TARGET_GET_FOLDER_ITEMS_CB:
        return "AVRCP_TARGET_GET_FOLDER_ITEMS_CB";
    case AVRCP_TARGET_SET_ADDR_PLAYER_CB:
        return "AVRCP_TARGET_SET_ADDR_PLAYER_CB";
    case AVRCP_TARGET_USE_BIGGER_METADATA:
        return "AVRCP_TARGET_USE_BIGGER_METADATA";
    case A2DP_SOURCE_CODEC_CONFIG_CB:
        return "CODEC_CONFIG_CB";
    case AVRCP_TARGET_LIST_PLAYER_APP_ATTR:
        return "AVRCP_TARGET_LIST_PLAYER_APP_ATTR";
    case AVRCP_TARGET_LIST_PLAYER_APP_VALUES:
        return " AVRCP_TARGET_LIST_PLAYER_APP_VALUES";
    case AVRCP_TARGET_GET_PLAYER_APP_VALUE:
        return "AVRCP_TARGET_GET_PLAYER_APP_VALUE";
    case AVRCP_TARGET_SET_PLAYER_APP_VALUE:
        return "AVRCP_TARGET_SET_PLAYER_APP_VALUE";
    case AVRCP_TARGET_PLAY_POSITION_TIMEOUT:
        return "AVRCP_TARGET_PLAY_POSITION_TIMEOUT";
    case AVRCP_TARGET_SET_BROWSED_PLAYER_REQ:
        return "AVRCP_TARGET_SET_BROWSED_PLAYER_REQ";
    case AVRCP_TARGET_CHANGE_PATH_REQ:
        return "AVRCP_TARGET_CHANGE_PATH_REQ";
    case AVRCP_TARGET_GET_TOTAL_NUM_OF_ITEMS_CB:
        return "AVRCP_TARGET_GET_TOTAL_NUM_OF_ITEMS_CB";
    case AVRCP_TARGET_SEARCH_CB:
        return "AVRCP_TARGET_SEARCH_CB";
    case AVRCP_TARGET_GET_ITEM_ATTRIBUTES_REQ:
        return "AVRCP_TARGET_GET_ITEM_ATTRIBUTES_REQ";
    case AVRCP_TARGET_PLAY_ITEMS_REQ:
        return "AVRCP_TARGET_PLAY_ITEMS_REQ";
    case AVRCP_TARGET_ADDTO_NOW_PLAYING_REQ:
        return "AVRCP_TARGET_ADDTO_NOW_PLAYING_REQ";
    case A2DP_SOURCE_SET_SCMST_CP_FLAG:
        return "A2DP_SOURCE_SET_SCMST_CP_FLAG";
    }
    return "UNKNOWN";
}

void A2dp_Source::state_disconnected_handler(BtEvent* pEvent) {
    char str[18];
    ALOGD(LOGTAG_A2DP "state_disconnected_handler Processing event %s", dump_message(pEvent->event_id));
    switch(pEvent->event_id) {
        case A2DP_SOURCE_API_CONNECT_REQ:
            memcpy(&mConnectingDevice, &pEvent->a2dpSourceEvent.bd_addr, sizeof(bt_bdaddr_t));
            if (sBtA2dpSourceInterface != NULL) {
                sBtA2dpSourceInterface->connect(pEvent->a2dpSourceEvent.bd_addr);
            }
            bdaddr_to_string(&mConnectingDevice, str, 18);
            fprintf(stdout, "A2DP Source Connecting to %s\n", str);
            change_state(STATE_A2DP_SOURCE_PENDING);
            break;
        case A2DP_SOURCE_API_DISCONNECT_REQ:
            fprintf(stdout, "A2DP Source Disconnect can not be processed\n");
            break;
        case A2DP_SOURCE_CONNECTING_CB:
            memcpy(&mConnectingDevice, &pEvent->a2dpSourceEvent.bd_addr, sizeof(bt_bdaddr_t));
            bdaddr_to_string(&mConnectingDevice, str, 18);
            fprintf(stdout, "A2DP Source Connecting to %s\n", str);
            change_state(STATE_A2DP_SOURCE_PENDING);
            break;
        case A2DP_SOURCE_CONNECTED_CB:
            memset(&mConnectingDevice, 0, sizeof(bt_bdaddr_t));
            memcpy(&mConnectedDevice, &pEvent->a2dpSourceEvent.bd_addr, sizeof(bt_bdaddr_t));
            bdaddr_to_string(&mConnectedDevice, str, 18);
            fprintf(stdout, "A2DP Source Connected to %s\n", str);
            change_state(STATE_A2DP_SOURCE_CONNECTED);
            bt_status_t ret_val;
            ret_val = sBtA2dpSourceInterface->set_active_device(pEvent->a2dpSourceEvent.bd_addr);
            if (ret_val != BT_STATUS_SUCCESS) {
                fprintf(stdout, "Failure setting active device %s", str);
                ALOGD(LOGTAG_A2DP "Failure setting active device %s", str);
                break;
            }
            BtA2dpOpenOutputStream();
#if defined(BT_AUDIO_PAL_INTEGRATION)
            if (!pa_routing_intf) {
                pa_routing_intf = pa_routing_intf_open();
                if (!pa_routing_intf) {
                   ALOGE(LOGTAG " pa_routing_intf_open failed!!!");
                }
                else {
                   ALOGD(LOGTAG " pa_routing_intf_open success!!!");
                   int ret = pa_routing_intf->pa_bt_connect_fn(PA_BT_A2DP_SOURCE, true);
                   if (!ret) {
                      ALOGD(LOGTAG " BT a2dp source connect success");
                      fprintf(stdout, "BT a2dp source connect success\n");
                   }
                   else {
                      ALOGD(LOGTAG " BT a2dp source connect failed");
                      fprintf(stdout, "BT a2dp source connect failed\n");
                   }
                }
            }
#endif
            break;
        default:
            fprintf(stdout, "Event not processed in disconnected state %d \n", pEvent->event_id);
            ALOGE(LOGTAG_A2DP " event not handled %d ", pEvent->event_id);
            break;
    }
}
void A2dp_Source::state_pending_handler(BtEvent* pEvent) {
    char str[18];
    bt_bdaddr_t mDevice;
    btav_a2dp_codec_config_t cur_codec_cfg;
    btav_a2dp_codec_index_t cur_codec_type;
    bool is_valid_codec = true;
    ALOGD(LOGTAG_A2DP "state_pending_handler Processing event %s", dump_message(pEvent->event_id));
    switch(pEvent->event_id) {
        case A2DP_SOURCE_CONNECTED_CB:
            memcpy(&mConnectedDevice, &pEvent->a2dpSourceEvent.bd_addr, sizeof(bt_bdaddr_t));
            memset(&mConnectingDevice, 0, sizeof(bt_bdaddr_t));
            bdaddr_to_string(&mConnectedDevice, str, 18);
            fprintf(stdout, "A2DP Source Connected to %s\n", str);
            change_state(STATE_A2DP_SOURCE_CONNECTED);
            bt_status_t ret_val;
            ret_val = sBtA2dpSourceInterface->set_active_device(pEvent->a2dpSourceEvent.bd_addr);
            if (ret_val != BT_STATUS_SUCCESS) {
                fprintf(stdout, "Failure setting active device %s\n", str);
                ALOGD(LOGTAG_A2DP "Failure setting active device %s", str);
                break;
            }
            BtA2dpOpenOutputStream();
#if defined(BT_AUDIO_PAL_INTEGRATION)
            if (!pa_routing_intf) {
                pa_routing_intf = pa_routing_intf_open();
                if (!pa_routing_intf) {
                   ALOGE(LOGTAG " pa_routing_intf_open failed!!!");
                }
                else {
                   ALOGD(LOGTAG " pa_routing_intf_open success!!!");
                   int ret = pa_routing_intf->pa_bt_connect_fn(PA_BT_A2DP_SOURCE, true);
                   if (!ret) {
                      ALOGD(LOGTAG " BT a2dp source connect success");
                      fprintf(stdout, "BT a2dp source connect success\n");
                   }
                   else {
                      ALOGD(LOGTAG " BT a2dp source connect failed");
                      fprintf(stdout, "BT a2dp source connect failed\n");
                   }
                }
            }
#endif
            break;
        case A2DP_SOURCE_DISCONNECTED_CB:
            fprintf(stdout, "A2DP Source DisConnected \n");
            media_playing = false;
            playStatus = BTRC_PLAYSTATE_STOPPED;
            a2dp_playstatus = A2DP_SOURCE_AUDIO_STOPPED;
            mCurrentTrackID = NO_TRACK_SELECTED;
            mCurrentAddressedPlayer = DEFAULT_PLAYER_ADDRESSED;
            pA2dpSource->mAbsVolRemoteSupported = false;
            TRACK_IS_SELECTED = 0L;
            BtA2dpCloseOutputStream();
            memset(&mConnectedDevice, 0, sizeof(bt_bdaddr_t));
            memset(&mConnectingDevice, 0, sizeof(bt_bdaddr_t));
            change_state(STATE_A2DP_SOURCE_DISCONNECTED);
#if defined(BT_AUDIO_PAL_INTEGRATION)
            if (pa_routing_intf) {
               int ret = pa_routing_intf->pa_bt_connect_fn(PA_BT_A2DP_SOURCE, false);
               if (!ret) {
                  ALOGD(LOGTAG " BT a2dp source disconnect success");
                  fprintf(stdout, "BT a2dp source disconnect success\n");
               }
               else {
                  ALOGD(LOGTAG " BT a2dp source disconnect failed");
                  fprintf(stdout, "BT a2dp source disconnect failed\n");
               }
               pa_routing_intf_close(pa_routing_intf);
               pa_routing_intf = NULL;
            }
#endif
            break;
        case A2DP_SOURCE_API_CONNECT_REQ:
            bdaddr_to_string(&mConnectingDevice, str, 18);
            fprintf(stdout, "A2DP Source already Connecting to %s\n", str);
            break;
        case A2DP_SOURCE_API_DISCONNECT_REQ:
            fprintf(stdout, "A2DP Source Disconnect can not be processed\n");
            break;
        case A2DP_SOURCE_CODEC_CONFIG_CB:
            memcpy(&mDevice, &pEvent->a2dpSourceEvent.bd_addr, sizeof(bt_bdaddr_t));
            bdaddr_to_string(&mDevice, str, 18);
            fprintf(stdout, "Codec Configuration for device %s\n", str);
            if (pEvent->a2dpSourceEvent.buf_ptr == NULL) {
                 break;
            }
            memcpy(&cur_codec_cfg, (btav_a2dp_codec_config_t *)pEvent->a2dpSourceEvent.buf_ptr, sizeof(btav_a2dp_codec_config_t));
            cur_codec_type = cur_codec_cfg.codec_type;
            fprintf(stdout, "Codec type = %s \n", get_a2dp_codec_type(cur_codec_type));
            fprintf(stdout, "Sample Rate = %d\n", get_a2dp_sampling_rate(cur_codec_cfg.sample_rate));
            fprintf(stdout, "Bits per sample = %d\n", get_a2dp_bits_per_sample(cur_codec_cfg.bits_per_sample));
            fprintf(stdout, "Channel Mode = %s\n", get_a2dp_channel_mode(cur_codec_cfg.channel_mode));
            if (cur_codec_type == BTAV_A2DP_CODEC_INDEX_SOURCE_SBC) {
                fprintf(stdout, "Block Len = %d\n",get_a2dp_sbc_block_len(cur_codec_cfg.codec_specific_1));
                fprintf(stdout, "Num of Subbands = %d\n",get_a2dp_sbc_sub_band(cur_codec_cfg.codec_specific_2));
                fprintf(stdout, "Allocation Method = %s\n",get_a2dp_sbc_allocation_mth(cur_codec_cfg.codec_specific_3));
                fprintf(stdout, "Max Bitpool = %d\n", cur_codec_cfg.codec_specific_4);
                fprintf(stdout, "Min Bitpool = %d\n", cur_codec_cfg.codec_specific_5);
            }
            update_src_codec_type(&src_codec_type, cur_codec_type);
            memset(&src_codec_cfg, 0, sizeof(btav_codec_config_t));
            if(src_codec_type ==  A2DP_SINK_AUDIO_CODEC_SBC)
               update_src_codec_config(&src_codec_cfg, cur_codec_cfg);
            osi_free(pEvent->a2dpSourceEvent.buf_ptr);
             break;
        default:
            ALOGE(LOGTAG_A2DP " event not handled %d ", pEvent->event_id);
            break;
    }
}

void A2dp_Source::SendStartStreamReq(){
    sBtA2dpSourceVendorInterface->start_stream(&mConnectedDevice);
}

void A2dp_Source::SendSuspendStreamReq(){
    sBtA2dpSourceVendorInterface->suspend_stream(&mConnectedDevice);
}

void A2dp_Source::SendEncodedData(){
    uint8_t no_of_frames=0;
    uint8_t sbc_frame_size=0;
    uint16_t data_pushed = 0;
    uint8_t rtp_header =0;
    A2DP_SBC_FRAME sbc_frame;
    uint8_t* p_buf = (uint8_t*)osi_malloc(MTU_src);
    if(p_buf == NULL)
    {
        ALOGD(LOGTAG_A2DP "memory allocation failed");
        return;
    }
    ALOGD(LOGTAG_A2DP "pump_encoded_data is true , reading from sink");
    pthread_mutex_lock(&a2dp_sink_relay_mutex);
    if(list_is_empty(a2dp_sink_relay_data_list))
    {
        pthread_mutex_unlock(&a2dp_sink_relay_mutex);
        free(p_buf);
        return;
    }
    t_SINK_RELAY_DATA* ptr = (t_SINK_RELAY_DATA*)list_front(a2dp_sink_relay_data_list);
    uint8_t* data_ptr;
    data_ptr = (uint8_t*)(ptr + 1);
    rtp_header = get_rtp_offset(data_ptr,A2DP_SINK_AUDIO_CODEC_SBC);
    memcpy(p_buf,data_ptr,rtp_header+1);// copying rtp header and Media packet header
    data_pushed += rtp_header+1;
    if(ptr->offset < rtp_header+1)
        ptr->offset = rtp_header+1;
    while((data_pushed < MTU_src) && (no_of_frames < 7))
    {
        while(*(data_ptr+ptr->offset)!= SBC_SYNCWORD)
        {
            ALOGD(LOGTAG_A2DP "syncword doesn't match");
            data_ptr++;
            ptr->offset++;
        }
        sbc_frame_size = A2DP_SBC_Calculate_FrameLength(&sbc_frame, data_ptr+ptr->offset);
        ALOGD(LOGTAG_A2DP "SBC calculated frame length : %d", sbc_frame_size);
        if((data_pushed+sbc_frame_size) > MTU_src)
            break;
        memcpy(p_buf+data_pushed,data_ptr+ptr->offset,sbc_frame_size);
        data_pushed += sbc_frame_size;
        ptr->offset += sbc_frame_size;
        no_of_frames++;

        if(ptr->offset >= ptr->len)
        {
            list_remove(a2dp_sink_relay_data_list, ptr);
            osi_free(ptr);
            if(list_is_empty(a2dp_sink_relay_data_list))
                break;
            ptr = (t_SINK_RELAY_DATA*)list_front(a2dp_sink_relay_data_list);
            data_ptr = (uint8_t*)(ptr + 1);
            uint8_t new_rtp_header =get_rtp_offset(data_ptr,A2DP_SINK_AUDIO_CODEC_SBC);;
            ptr->offset = new_rtp_header+1;
        }
    }

    sequence_number++;

    *(p_buf+3) = (uint8_t) sequence_number;
    *(p_buf+2) = (uint8_t)(sequence_number>>8);

    uint8_t *temp_timestamp;
    temp_timestamp = (uint8_t*) &timestamp;
    *(p_buf+7) = (uint8_t)(timestamp);
    *(p_buf+6) = (uint8_t)(timestamp>>8);
    *(p_buf+5) = (uint8_t)(timestamp>>16);
    *(p_buf+4) = (uint8_t)(timestamp>>24);

    *(p_buf + rtp_header) = no_of_frames;
    timestamp += no_of_frames*sbc_frame.nrof_subbands*sbc_frame.nrof_blocks;
    pthread_mutex_unlock(&a2dp_sink_relay_mutex);
    sBtA2dpSourceVendorInterface->btav_send_encoded_data_vendor(&mConnectedDevice,p_buf,data_pushed,A2DP_SINK_AUDIO_CODEC_SBC);
    ALOGD(LOGTAG_A2DP "sent one packet of encoded data contaning %d sbc frames and data pushed = %d", no_of_frames, data_pushed);
    osi_free(p_buf);
    return;
}

void A2dp_Source::state_connected_handler(BtEvent* pEvent) {
    char str[18];
    bt_bdaddr_t mDevice;
    btav_a2dp_codec_config_t cur_codec_cfg;
    btav_a2dp_codec_index_t cur_codec_type;
    uint32_t freq;
    char *mode;
    bool is_valid_codec = true;
    BtEvent *pControlRequest, *pReleaseControlReq;
    btrc_register_notification_t param;
    ALOGD(LOGTAG_A2DP "state_connected_handler Processing event %s", dump_message(pEvent->event_id));
    switch(pEvent->event_id) {
        case A2DP_SOURCE_API_CONNECT_REQ:
            bdaddr_to_string(&mConnectedDevice, str, 18);
            fprintf(stdout, "A2DP Source Already Connected to %s\n", str);
            break;
        case A2DP_SOURCE_API_DISCONNECT_REQ:
            if (memcmp(&mConnectedDevice, &pEvent->a2dpSourceEvent.bd_addr, sizeof(bt_bdaddr_t)))
            {
                bdaddr_to_string(&pEvent->a2dpSourceEvent.bd_addr, str, 18);
                fprintf(stdout, "Device not connected: %s\n", str);
                break;
            }
            bdaddr_to_string(&mConnectedDevice, str, 18);
            fprintf(stdout, "A2DP Source DisConnecting: %s\n", str);
            media_playing = false;
            playStatus = BTRC_PLAYSTATE_STOPPED;
            mCurrentTrackID = NO_TRACK_SELECTED;
            pA2dpSource->mAbsVolRemoteSupported = false;
            TRACK_IS_SELECTED = 0L;
            if (sBtA2dpSourceInterface != NULL) {
                sBtA2dpSourceInterface->disconnect(pEvent->a2dpSourceEvent.bd_addr);
            }
            change_state(STATE_A2DP_SOURCE_PENDING);
            break;
        case A2DP_SOURCE_DISCONNECTED_CB:
            media_playing = false;
            playStatus = BTRC_PLAYSTATE_STOPPED;
            a2dp_playstatus = A2DP_SOURCE_AUDIO_STOPPED;
            mCurrentTrackID = NO_TRACK_SELECTED;
            mCurrentAddressedPlayer = DEFAULT_PLAYER_ADDRESSED;
            pA2dpSource->mAbsVolRemoteSupported = false;
            TRACK_IS_SELECTED = 0L;
            BtA2dpCloseOutputStream();
            memset(&mConnectedDevice, 0, sizeof(bt_bdaddr_t));
            memset(&mConnectingDevice, 0, sizeof(bt_bdaddr_t));
            fprintf(stdout, "A2DP Source DisConnected \n");
            change_state(STATE_A2DP_SOURCE_DISCONNECTED);
#if defined(BT_AUDIO_PAL_INTEGRATION)
            if (pa_routing_intf) {
               int ret = pa_routing_intf->pa_bt_connect_fn(PA_BT_A2DP_SOURCE, false);
               if (!ret) {
                  ALOGD(LOGTAG " BT a2dp source disconnect success");
                  fprintf(stdout, "BT a2dp source disconnect success\n");
               }
               else {
                  ALOGD(LOGTAG " BT a2dp source disconnect failed");
                  fprintf(stdout, "BT a2dp source disconnect failed\n");
               }
               pa_routing_intf_close(pa_routing_intf);
               pa_routing_intf = NULL;
            }
#endif
            break;
        case A2DP_SOURCE_DISCONNECTING_CB:
            fprintf(stdout, "A2DP Source DisConnecting \n");
            change_state(STATE_A2DP_SOURCE_PENDING);
            break;
        case A2DP_SOURCE_AUDIO_STARTED:
            fprintf(stdout, "A2DP Source Audio state changes to: %d  \n",pEvent->event_id);
            if (playStatus != BTRC_PLAYSTATE_PLAYING)
            {
               playStatus = BTRC_PLAYSTATE_PLAYING;
               if (mPlayStatusNotiType == BTRC_NOTIFICATION_TYPE_INTERIM) {
                   param.play_status = playStatus;
                   mPlayStatusNotiType = BTRC_NOTIFICATION_TYPE_CHANGED;
                   sBtAvrcpTargetInterface->register_notification_rsp(
                                    BTRC_EVT_PLAY_STATUS_CHANGED,
                                    mPlayStatusNotiType, &param);
               }
            }
            break;
        case A2DP_SOURCE_AUDIO_SUSPENDED:
            fprintf(stdout, "A2DP Source Audio state changes to: %d  \n",pEvent->event_id);
            if (playStatus != BTRC_PLAYSTATE_PAUSED)
            {
               playStatus = BTRC_PLAYSTATE_PAUSED;
               if (mPlayStatusNotiType == BTRC_NOTIFICATION_TYPE_INTERIM) {
                   param.play_status = playStatus;
                   mPlayStatusNotiType = BTRC_NOTIFICATION_TYPE_CHANGED;
                   sBtAvrcpTargetInterface->register_notification_rsp(
                                    BTRC_EVT_PLAY_STATUS_CHANGED,
                                    mPlayStatusNotiType, &param);
               }
            }
            break;
        case A2DP_SOURCE_AUDIO_STOPPED:
            fprintf(stdout, "A2DP Source Audio state changes to: %d \n", pEvent->event_id);
            if (playStatus != BTRC_PLAYSTATE_STOPPED)
            {
               playStatus = BTRC_PLAYSTATE_STOPPED;
               if (mPlayStatusNotiType == BTRC_NOTIFICATION_TYPE_INTERIM) {
                   param.play_status = playStatus;
                   mPlayStatusNotiType = BTRC_NOTIFICATION_TYPE_CHANGED;
                   sBtAvrcpTargetInterface->register_notification_rsp(
                                    BTRC_EVT_PLAY_STATUS_CHANGED,
                                    mPlayStatusNotiType, &param);
               }
            }
            break;
        case A2DP_SOURCE_CODEC_CONFIG_CB:
            memcpy(&mDevice, &pEvent->a2dpSourceEvent.bd_addr, sizeof(bt_bdaddr_t));
            bdaddr_to_string(&mDevice, str, 18);
            fprintf(stdout, "Codec Configuration for device %s\n", str);
            if (pEvent->a2dpSourceEvent.buf_ptr == NULL) {
                 break;
            }
            memcpy(&cur_codec_cfg, (btav_a2dp_codec_config_t *)pEvent->a2dpSourceEvent.buf_ptr, sizeof(btav_a2dp_codec_config_t));
            cur_codec_type = cur_codec_cfg.codec_type;
            fprintf(stdout, "Codec type = %s \n", get_a2dp_codec_type(cur_codec_type));
            fprintf(stdout, "Sample Rate = %d\n", get_a2dp_sampling_rate(cur_codec_cfg.sample_rate));
            fprintf(stdout, "Bits per sample = %d\n", get_a2dp_bits_per_sample(cur_codec_cfg.bits_per_sample));
            fprintf(stdout, "Channel Mode = %s\n", get_a2dp_channel_mode(cur_codec_cfg.channel_mode));
            if (cur_codec_type == BTAV_A2DP_CODEC_INDEX_SOURCE_SBC) {
                fprintf(stdout, "Block Len = %d\n",get_a2dp_sbc_block_len(cur_codec_cfg.codec_specific_1));
                fprintf(stdout, "Num of Subbands = %d\n",get_a2dp_sbc_sub_band(cur_codec_cfg.codec_specific_2));
                fprintf(stdout, "Allocation Method = %s\n",get_a2dp_sbc_allocation_mth(cur_codec_cfg.codec_specific_3));
                fprintf(stdout, "Max Bitpool = %d\n", cur_codec_cfg.codec_specific_4);
                fprintf(stdout, "Min Bitpool = %d\n", cur_codec_cfg.codec_specific_5);
            }
            if (cur_codec_type == BTAV_A2DP_CODEC_INDEX_SOURCE_APTX_ADAPTIVE) {
                fprintf(stdout, "codec_specific info 4 = %lx\n", cur_codec_cfg.codec_specific_4);
            }
            update_src_codec_type(&src_codec_type, cur_codec_type);
            memset(&src_codec_cfg, 0, sizeof(btav_codec_config_t));
            if(src_codec_type ==  A2DP_SINK_AUDIO_CODEC_SBC)
               update_src_codec_config(&src_codec_cfg, cur_codec_cfg);
            osi_free(pEvent->a2dpSourceEvent.buf_ptr);
             break;
        case A2DP_SOURCE_SET_SCMST_CP_FLAG:
            {
            uint8_t p = (uint8_t)pEvent->a2dpSourceEvent.arg1;
            ALOGD(LOGTAG_A2DP "%s SCMS-T Cp flag: %x ",__func__, p);
            fprintf(stdout, "SCMS-T Cp flag : %x\n", p);
            if(sBtA2dpSourceVendorInterface) {
                if(sBtA2dpSourceVendorInterface->update_cp_header(
                        &pEvent->a2dpSourceEvent.bd_addr, p) != BT_STATUS_SUCCESS ) {
                  ALOGD(LOGTAG_A2DP "%s Invalid parameters for SCMS-T",__func__, p);
                  fprintf(stdout, "Invalid parameters for SCMS-T \n", p);
                }
            } else {
                ALOGE(LOGTAG_A2DP " sBtA2dpSourceVendorInterface is NULL ");
            }
            break;
            }
        default:
            fprintf(stdout, "Event not processed in connected state %d \n", pEvent->event_id);
            ALOGE(LOGTAG_A2DP " event not handled %d ", pEvent->event_id);
            break;
    }
}

A2dpSourceState A2dp_Source::get_state() {
   ALOGD(LOGTAG_A2DP "current state changed to %d ", mSourceState);
   return mSourceState;
}


void A2dp_Source::change_state(A2dpSourceState mState) {
   ALOGD(LOGTAG_A2DP " current State = %d, new state = %d", mSourceState, mState);
   pthread_mutex_lock(&lock);
   mSourceState = mState;
   pthread_mutex_unlock(&lock);
   ALOGD(LOGTAG_A2DP " state changed to %d ", mState);
}


void A2dp_Source::UpdateSupportedCodecs(const RawAddress& bd_addr, uint8_t num_codec_cfgs) {
    bt_status_t status;
    if (sBtA2dpSourceInterface != NULL) {
        status = sBtA2dpSourceInterface->config_codec(bd_addr, a2dpSrcCodecList);
        if (BT_STATUS_SUCCESS != status) {
            ALOGE(LOGTAG_A2DP " UpdateSupportedCodecs: failed, status = %d", status);
            fprintf(stdout, "UpdateSupportedCodecs: failed, status = %d\n", status);
        }
    }
}

char * A2dp_Source::get_a2dp_codec_type(uint8_t codectype) {
    switch (codectype) {
        case BTAV_A2DP_CODEC_INDEX_SOURCE_SBC:
            return "sbc";
        case BTAV_A2DP_CODEC_INDEX_SOURCE_AAC:
            return "aac";
        case BTAV_A2DP_CODEC_INDEX_SOURCE_APTX:
            return "aptx";
        case BTAV_A2DP_CODEC_INDEX_SOURCE_APTX_HD:
            return "aptx_hd";
        case BTAV_A2DP_CODEC_INDEX_SOURCE_LDAC:
            return "ldac";
        case BTAV_A2DP_CODEC_INDEX_SOURCE_APTX_ADAPTIVE:
            return "aptx_ad";
    }
    return "NULL";
}

uint32_t A2dp_Source::get_a2dp_sampling_rate(uint8_t frequency) {
    uint32_t freq = 999;
    switch (frequency) {
        case BTAV_A2DP_CODEC_SAMPLE_RATE_16000:
            freq = 16000;
            break;
        case BTAV_A2DP_CODEC_SAMPLE_RATE_32000:
            freq = 32000;
            break;
        case BTAV_A2DP_CODEC_SAMPLE_RATE_44100:
            freq = 44100;
            break;
        case BTAV_A2DP_CODEC_SAMPLE_RATE_48000:
            freq = 48000;
            break;
        case BTAV_A2DP_CODEC_SAMPLE_RATE_88200:
            freq = 88200;
            break;
        case BTAV_A2DP_CODEC_SAMPLE_RATE_96000:
            freq = 96000;
            break;
        case BTAV_A2DP_CODEC_SAMPLE_RATE_176400:
            freq = 176400;
            break;
        case BTAV_A2DP_CODEC_SAMPLE_RATE_192000:
            freq = 192000;
            break;
    }
    return freq;
}

uint32_t A2dp_Source::get_a2dp_bits_per_sample(uint8_t bits_per_sample) {
    uint32_t bps = 999;
    switch (bits_per_sample) {
        case BTAV_A2DP_CODEC_BITS_PER_SAMPLE_16:
            bps = 16;
            break;
        case BTAV_A2DP_CODEC_BITS_PER_SAMPLE_24:
            bps = 24;
            break;
        case BTAV_A2DP_CODEC_BITS_PER_SAMPLE_32:
            bps = 32;
            break;
    }
    return bps;
}

char * A2dp_Source::get_a2dp_channel_mode(uint8_t channeltype) {
    switch (channeltype) {
        case BTAV_A2DP_CODEC_CHANNEL_MODE_MONO:
            return "mono";
        case BTAV_A2DP_CODEC_CHANNEL_MODE_STEREO:
            return "stereo";
        case BTAV_A2DP_CODEC_CHANNEL_MODE_JOINT:
            return "joint";
        case BTAV_A2DP_CODEC_CHANNEL_MODE_DUAL:
            return "dual";
    }
    return "NULL";
}

uint8_t A2dp_Source::get_a2dp_sbc_block_len(uint8_t blocklen) {
    uint8_t bl= 99;
    switch (blocklen) {
        case SBC_BLOCKS_4:
            bl = 4;
            break;
        case SBC_BLOCKS_8:
            bl = 8;
            break;
        case SBC_BLOCKS_12:
            bl = 12;
            break;
        case SBC_BLOCKS_16:
            bl = 16;
            break;
    }
    return bl;
}

uint8_t A2dp_Source::get_a2dp_sbc_sub_band(uint8_t subband) {
    uint8_t sb= 99;
    switch (subband) {
        case SBC_SUBBAND_4:
            sb = 4;
            break;
        case SBC_SUBBAND_8:
            sb = 8;
            break;
    }
    return sb;
}

char * A2dp_Source::get_a2dp_sbc_allocation_mth(uint8_t allocation) {
    switch (allocation) {
        case SBC_ALLOC_SNR:
            return "snr";
        case SBC_ALLOC_LOUDNESS:
            return "loudness";
    }
    return "NULL";
}


A2dp_Source :: A2dp_Source(const bt_interface_t *bt_interface, config_t *config) {
    this->bluetooth_interface = bt_interface;
    this->config = config;
    sBtA2dpSourceInterface = NULL;
    sBtAvrcpTargetInterface = NULL;
    mSourceState = STATE_A2DP_SOURCE_NOT_STARTED;
    mAvrcpConnected = false;
    abs_vol_timer = false;
    mVolCmdSetInProgress = false;
    mVolCmdAdjustInProgress = false;
    mInitialRemoteVolume = -1;
    mLastRemoteVolume = -1;
    mRemoteVolume = -1;
    mLastLocalVolume = -1;
    mLocalVolume = -1;
    mPreviousAddrPlayerId = 0;
    mCurrentAddrPlayerId = 0;
    mAbsVolRemoteSupported = false;
    TRACK_IS_SELECTED = 0L;
    sequence_number = 0;
    timestamp = 0;
    memset(&mConnectedDevice, 0, sizeof(bt_bdaddr_t));
    memset(&mConnectingDevice, 0, sizeof(bt_bdaddr_t));
    memset(&mConnectedAvrcpDevice, 0, sizeof(bt_bdaddr_t));
    pthread_mutex_init(&this->lock, NULL);
    is_sink_relay_enabled = config_get_bool (config,
            CONFIG_DEFAULT_SECTION, "BtRelaySinkDatatoSrc", false);
    ALOGD(LOGTAG_A2DP " Sink Relay Enabled %d ", is_sink_relay_enabled);
}

A2dp_Source :: ~A2dp_Source() {
    mAvrcpConnected = false;
    mVolCmdSetInProgress = false;
    mVolCmdAdjustInProgress = false;
    mInitialRemoteVolume = -1;
    mLastRemoteVolume = -1;
    mRemoteVolume = -1;
    mLastLocalVolume = -1;
    mLocalVolume = -1;
    mPreviousAddrPlayerId = 0;
    mCurrentAddrPlayerId = 0;
    mAbsVolRemoteSupported = false;
    TRACK_IS_SELECTED = 0L;
    pthread_mutex_destroy(&lock);
#if defined(BT_AUDIO_PAL_INTEGRATION)
   if (pa_routing_intf) {
       pa_routing_intf_close(pa_routing_intf);
       pa_routing_intf = NULL;
   }
#endif
}

MediaPlayerInfo :: MediaPlayerInfo(short playerId, char majorPlayerType, int playerSubType,
                                      char playState, short charsetId, short displayableNameLength,
                                      char* displayableName, char* playerPackageName,
                                      bool isAvailable, bool isFocussed, char itemType,
                                      bool isRemoteAddressable,
                                      char featureMask[]) {
    int i;
    mPlayerId = playerId;
    mMajorPlayerType = majorPlayerType;
    mPlayerSubType = playerSubType;
    mPlayState = playState;
    mCharsetId = charsetId;
    mDisplayableNameLength = displayableNameLength;
    memcpy(&mDisplayableName, &displayableName, strlen(displayableName)+1);
    memcpy(&mPlayerPackageName, &playerPackageName, strlen(playerPackageName)+1);
    ALOGD(LOGTAG_AVRCP "  %s %s", mDisplayableName, mPlayerPackageName);

    mIsAvailable = isAvailable;
    mIsFocussed = isFocussed;
    mItemType = itemType;
    mIsRemoteAddressable = isRemoteAddressable;
    mItemLength = (short)(mDisplayableNameLength + 2 + 1 + 4 + 1 + 2 + 2 + 16);
    mEntryLength = (short)(mItemLength + /* ITEM_LENGTH_LENGTH +*/ 1);
    memcpy(mFeatureMask, featureMask, 16);

    for (i = 0; i < 16; i++)
        ALOGD(LOGTAG_AVRCP " %d", mFeatureMask[i]);
}

int MediaPlayerInfo :: RetrievePlayerEntryLength() {
    return mEntryLength;
}

char* MediaPlayerInfo :: RetrievePlayerItemEntry() {
    int position = 0;
    int count;
    char* playerEntry1 = (char*)osi_malloc(mEntryLength * sizeof(char));

    playerEntry1[position] = (char)mItemType;
    ALOGD(LOGTAG_AVRCP "RetrievePlayerItemEntry type %d", playerEntry1[position]);
    position++;

    playerEntry1[position] = (char)(mPlayerId & 0xff);
    ALOGD(LOGTAG_AVRCP "RetrievePlayerItemEntry playerid %d", playerEntry1[position]);
    position++;

    playerEntry1[position] = (char)((mPlayerId >> 8) & 0xff);
    ALOGD(LOGTAG_AVRCP "RetrievePlayerItemEntry playerid %d", playerEntry1[position]);
    position++;

    playerEntry1[position] = (char)mMajorPlayerType;
    ALOGD(LOGTAG_AVRCP "RetrievePlayerItemEntry MajorPlayerType %d", playerEntry1[position]);
    position++;

    for (count = 0; count < 4; count++) {
        playerEntry1[position] = (char)((mPlayerSubType >> (8 * count)) & 0xff); position++;
    }

    playerEntry1[position] = (char)mPlayState; position++;
    for (count = 0; count < 16; count++) {
        playerEntry1[position] = (char)mFeatureMask[count];
        ALOGD(LOGTAG_AVRCP "RetrievePlayerItemEntry playerEntry1[%d] position %d,  %d", count,
                            position, playerEntry1[position]);
        position++;
    }
    playerEntry1[position] = (char)(mCharsetId & 0xff); position++;
    playerEntry1[position] = (char)((mCharsetId >> 8) & 0xff); position++;
    playerEntry1[position] = (char)(mDisplayableNameLength & 0xff); position++;
    playerEntry1[position] = (char)((mDisplayableNameLength >> 8) & 0xff); position++;

    for (count = 0; count < mDisplayableNameLength; count++) {
        playerEntry1[position] = (char)mDisplayableName[count]; position++;
    }
    if (position != mEntryLength) {
        ALOGE(LOGTAG_AVRCP "ERROR populating PlayerItemEntry: position: %d mEntryLength: %d",
                            position, mEntryLength);
    }
    return playerEntry1;
}

MediaPlayerInfo :: ~MediaPlayerInfo() {
}

FolderInfo :: FolderInfo(uint8_t   uid[],    uint8_t   type, uint8_t   playable, uint16_t  charsetId, short displayableNameLength,
                                char* displayableName){
    for(int i = 0; i < BTRC_UID_SIZE; i++)
        mUid[i] = uid[i];
    mType = type;
    mPlayable = playable;
    mCharsetId = charsetId;
    mDisplayableNameLength = displayableNameLength;
    mDisplayableName = (char*)osi_malloc(mDisplayableNameLength);
    memcpy(&mDisplayableName, &displayableName, mDisplayableNameLength);
    mItemLength = (short)(mDisplayableNameLength + BTRC_UID_SIZE + 1 + 2 + 2);
    mEntryLength = (short)(mItemLength + /* ITEM_LENGTH_LENGTH +*/ 1);
}

int FolderInfo :: RetrieveFolderEntryLength() {
    return mEntryLength;
}

char* FolderInfo :: RetrieveFolderItemEntry() {
    int position = 0;
    int count;
    char* folderEntry1 = (char*)osi_malloc(mEntryLength * sizeof(char));
    for(position = 0; position < BTRC_UID_SIZE; position++)
        folderEntry1[position] = (char)(mUid[position]);
    folderEntry1[position] = (char)mType;
    ALOGD(LOGTAG_AVRCP "RetrieveFolder type %d", folderEntry1[position]);
    position++;
    folderEntry1[position] = (char)mPlayable; position++;
    folderEntry1[position] = (char)(mCharsetId & 0xff); position++;
    folderEntry1[position] = (char)((mCharsetId >> 8) & 0xff); position++;
    folderEntry1[position] = (char)(mDisplayableNameLength & 0xff); position++;
    folderEntry1[position] = (char)((mDisplayableNameLength >> 8) & 0xff); position++;
    ALOGD("mDisplayableNameLength=%d",mDisplayableNameLength);

    for (count = 0; count < mDisplayableNameLength; count++){
        folderEntry1[position] = (char)mDisplayableName[count];
        position++;
    }
    if (position != mEntryLength) {
        ALOGE(LOGTAG_AVRCP "ERROR populating FolderItemEntry: position: %d mEntryLength: %d",
                            position, mEntryLength);
    }
    return folderEntry1;
}

FolderInfo :: ~FolderInfo() {
}


MediaInfo :: MediaInfo(uint8_t   uid[],    uint8_t   type,  uint16_t  charsetId, short displayableNameLength,
                                char* displayableName, uint8_t   num_attrs){
    for(int i = 0; i < BTRC_UID_SIZE; i++)
        mUid[i] = uid[i];
    mType = type;
    mCharsetId = charsetId;
    mDisplayableNameLength = displayableNameLength;
    mDisplayableName = (char*)osi_malloc(mDisplayableNameLength);
    memcpy(&mDisplayableName, &displayableName, mDisplayableNameLength);
    ALOGD(LOGTAG_AVRCP "  %s ", mDisplayableName);
    mNum_attrs = num_attrs;
    mItemLength = (short)(mDisplayableNameLength + BTRC_UID_SIZE + 1 + 2 + 2);
    mEntryLength = (short)(mItemLength + /* ITEM_LENGTH_LENGTH +*/ 1);
}

int MediaInfo :: RetrieveMediaEntryLength() {
    return mEntryLength;
}

char* MediaInfo :: RetrieveMediaItemEntry() {
    int position = 0;
    int count;
    char* mediaEntry1 = (char*)osi_malloc(mEntryLength * sizeof(char));
    for(position = 0; position < BTRC_UID_SIZE; position++)
        mediaEntry1[position] = (char)(mUid[position]);
    mediaEntry1[position] = (char)mType;
    ALOGD(LOGTAG_AVRCP "RetrieveFolder type %d", mediaEntry1[position]);
    position++;
    mediaEntry1[position] = (char)(mCharsetId & 0xff); position++;
    mediaEntry1[position] = (char)((mCharsetId >> 8) & 0xff); position++;
    mediaEntry1[position] = (char)(mDisplayableNameLength & 0xff); position++;
    mediaEntry1[position] = (char)((mDisplayableNameLength >> 8) & 0xff); position++;
    for (count = 0; count < mDisplayableNameLength; count++){
        mediaEntry1[position] = (char)mDisplayableName[count]; position++;
    }
    mediaEntry1[position] = (char)mNum_attrs; position++;
    if (position != mEntryLength) {
        ALOGE(LOGTAG_AVRCP "ERROR populating MediaItemEntry: position: %d mEntryLength: %d",
                            position, mEntryLength);
    }
    return mediaEntry1;
}

MediaInfo :: ~MediaInfo() {
}

