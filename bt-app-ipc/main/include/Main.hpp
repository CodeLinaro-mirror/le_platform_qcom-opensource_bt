/******************************************************************************
 *
 *  Copyright (c) 2016-2017, The Linux Foundation. All rights reserved.
 *  Not a Contribution.
 *  Copyright (C) 2014 Google, Inc.
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at:
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 *
 ******************************************************************************/


#ifndef BT_APP_HPP
#define BT_APP_HPP

#include "osi/include/thread.h"
#include "osi/include/reactor.h"
#include "osi/include/alarm.h"
#include "osi/include/config.h"
#include <hardware/bluetooth.h>
#include "include/ipc.hpp"
#include "utils.h"

#ifdef USE_GEN_GATT
#include "GattcTest.hpp"
#include "GattsTest.hpp"
#include "Rsp.hpp"
#endif
#include <cutils/sockets.h>
#include <sys/un.h>
#include <sys/poll.h>
#include <errno.h>
#include <netinet/in.h>
#include <pthread.h>
#include <stdbool.h>
#include <string.h>
#include <sys/prctl.h>
#include <sys/socket.h>
#include <sys/types.h>

#ifdef USE_GLIB
#include <glib.h>
#define strlcpy g_strlcpy
#endif

/**
 * @file Main.hpp
 * @brief Main header file for the BT application
*/

/**
 * Maximum argument length
 */
#define COMMAND_ARG_SIZE     200

/**
 * Maximum command length
 */
#define COMMAND_SIZE        200

/**
 * Maximum arguments count
 */
#define MAX_ARGUMENTS        20 //TODO

/**
 * Maximum sub-arguments count
 */
#define MAX_SUB_ARGUMENTS    10


#define BTM_MAX_LOC_BD_NAME_LEN     248

/**
 * Macro used to find the total commands number
 */
#define  NO_OF_COMMANDS(x)  (sizeof(x) / sizeof((x)[0]))

/**
* The size in bytes of the inquiry database.
*/
#define INQ_DB_SIZE    40

#define USEC_PER_SEC 1000000L

/**
 * The Configuration options
 */
const char *BT_SOCKET_ENABLED      = "BtSockInputEnabled";
const char *BT_ENABLE_DEFAULT      = "BtEnableByDefault";
const char *BT_ENABLE_AUTOTEST     = "BtEnableAutoTest";
const char *BT_USER_INPUT          = "UserInteractionNeeded";
const char *BT_A2DP_SINK_ENABLED   = "BtA2dpSinkEnable";
const char *BT_A2DP_SINK_SPLIT_ENABLED   = "BtA2dpSinkSplitEnable";
const char *BT_A2DP_SOURCE_ENABLED = "BtA2dpSourceEnable";
const char *BT_HFP_CLIENT_ENABLED  = "BtHfClientEnable";
const char *BT_HFP_AG_ENABLED      = "BtHfpAGEnable";
const char *BT_AVRCP_ENABLED       = "BtAvrcpEnable";
const char *BT_ENABLE_EXT_POWER    = "BtEnableExtPower";
const char *BT_ENABLE_FW_SNOOP     = "BtEnableFWSnoop";
const char *BT_ENABLE_SOC_LOG      = "BtEnableSocLog";
const char *BT_HID_ENABLED         = "BtHidEnable";
/**
 * The Configuration file path
 */
const char *CONFIG_FILE_PATH       = "/etc/bluetooth/bt_app.conf";

/**
 * To track user command status
 */
typedef enum  {
    COMMAND_NONE = 0,
    COMMAND_INPROGRESS,
    COMMAND_COMPLETE,
} CommandStatus;

/**
 * To track user command status
 */
typedef struct {
    CommandStatus enable_cmd;
    CommandStatus enquiry_cmd;
    CommandStatus stop_enquiry_cmd;
    CommandStatus disable_cmd;
    CommandStatus pairing_cmd;
} UiCommandStatus;

/**
 * list of supported commands
 */
typedef enum {
    BT_ENABLE,
    BT_DISABLE,
    START_ENQUIRY,
    CANCEL_ENQUIRY,
    MAIN_EXIT,
    START_PAIR,
    INQUIRY_LIST,
    BONDED_LIST,
    GET_BT_NAME,
    GET_BT_ADDR,
    SET_BT_NAME,
    SET_SCAN_MODE,
    UNPAIR,
    GET_BT_STATE,
    TEST_MODE,
    GAP_OPTION,
    TEST_ON_OFF,
    A2DP_SINK,
    A2DP_SOURCE,
    CONNECT,
    DISCONNECT,
    PLAY,
    PAUSE,
    STOP,
    MODE_CHANGE,
    AVDT_START,
    AVDT_SUSPEND,
    ACCEPT,
    REJECT,
    FASTFORWARD,
    REWIND,
    FORWARD,
    BACKWARD,
    POWER,
    VOL_UP,
    VOL_DOWN,
    MUTE,
    CODEC_LIST,
    TRACK_CHANGE,
    NOW_PLAYING_CONTENT_CHANGED,
    SET_ABS_VOL,
    SEND_VOL_UP_DOWN,
    VOL_CHANGED_NOTI,
    GET_CAP,
    LIST_PLAYER_SETTING_ATTR,
    LIST_PALYER_SETTING_VALUE,
    GET_PALYER_APP_SETTING,
    SET_PALYER_APP_SETTING,
    GET_ELEMENT_ATTR,
    GET_PLAY_STATUS,
    SET_ADDRESSED_PLAYER,
    SET_BROWSED_PLAYER,
    CHANGE_PATH,
    GETFOLDERITEMS,
    GETITEMATTRIBUTES,
    PLAYITEM,
    ADDTONOWPLAYING,
    SEARCH,
    REG_NOTIFICATION,
    ADDR_PLAYER_CHANGE,
    AVAIL_PLAYER_CHANGE,
    SET_EQUALIZER_VAL,
    SET_REPEAT_VAL,
    SET_SHUFFLE_VAL,
    SET_SCAN_VAL,
    BIGGER_METADATA,
    PAN_OPTION,
    CONNECTED_LIST,
    SET_TETHERING,
    GET_PAN_MODE,
    RSP_OPTION,
    RSP_INIT,
    RSP_START,
#ifdef USE_BT_OBEX
    PBAP_CLIENT_OPTION,
    PBAP_REGISTER,
    PBAP_GET_PHONEBOOK_SIZE,
    PBAP_GET_PHONEBOOK,
    PBAP_GET_VCARD,
    PBAP_GET_VCARD_LISTING,
    PBAP_SET_PATH,
    PBAP_ABORT,
    PBAP_SET_FILTER,
    PBAP_SET_ORDER,
    PBAP_SET_SEARCH_ATTRIBUTE,
    PBAP_SET_SEARCH_VALUE,
    PBAP_SET_PHONE_BOOK,
    PBAP_SET_REPOSITORY,
    PBAP_SET_VCARD_FORMAT,
    PBAP_SET_LIST_COUNT,
    PBAP_SET_START_OFFSET,
    PBAP_GET_FILTER,
    PBAP_GET_ORDER,
    PBAP_GET_SEARCH_ATTRIBUTE,
    PBAP_GET_PHONE_BOOK,
    PBAP_GET_REPOSITORY,
    PBAP_GET_VCARD_FORMAT,
    PBAP_GET_LIST_COUNT,
    PBAP_GET_START_OFFSET,
    OPP_OPTION,
    OPP_REGISTER,
    OPP_SEND,
    OPP_ABORT,
#endif
#ifdef USE_GEN_GATT
    GATTCTEST_OPTION,
    GATTCTEST_INIT,
    GATTCTEST_SCAN_FILTER,
    GATTCTEST_SCANFILTER_MAN_DATA,
    GATTCTEST_SCAN_SETTINGS,
    GATTCTEST_START_SCAN,
    GATTCTEST_STOP_SCAN,
    GATTCTEST_BATCH_SCAN,
    GATTCTEST_CONN_PARAMS,
    GATTCTEST_CONNECT,
    GATTCTEST_DISCONNECT,
    GATTCTEST_DISCSRVC,
    GATTCTEST_DISCSRVC_UUID,
    GATTCTEST_ALERT,
    GATTCTEST_READPHY,
    GATTCTEST_READRSSI,
    GATTCTEST_REQMTU,
    GATTCTEST_REFRESH,
    GATTCTEST_SETPHY,
    GATTCTEST_GETSERVICES,
    GATTCTEST_REQCONN_PRI,
    GATTCTEST_GETSRVC,
    GATTCTEST_RDCHAR_UUID,
    GATTCTEST_RDWRCHAR,
    GATTCTEST_RDWRDESC,
    GATTCTEST_GETCHARID,
    GATTCTEST_GETDESCID,
    GATTCTEST_CONN_DEVICES,
    GATTCTEST_RELIABLEWRITE,
    GATTSTEST_OPTION,
    GATTSTEST_INIT_SERVER,
    GATTSTEST_ADDSERVER,
    GATTSTEST_ADDSERVICES,
    GATTSTEST_INIT_ADVERTISER,
    GATTSTEST_START_ADVERTISER,
    GATTSTEST_READPHY,
    GATTSTEST_SET_PREFERRED_PHY,
    GATTSTEST_STOP,
    GATTSTEST_UNREGISTER_SERVER,
    GATTSTEST_DISABLE,
    GATTSTEST_CANCEL_CONNECTION,
#endif
    HFP_CLIENT,
    CREATE_SCO_CONN,
    DESTROY_SCO_CONN,
    VOIP_CALL_IND,
    END_VOIP_CALL,
    ACCEPT_VOIP_CALL,
    INCOM_VOIP_CALL_IND,
    SWAP_VOIP_CALLS,
    UPDATE_ACTIVE_CALLS_NUM,
    UPDATE_HELD_CALLS_NUM,
    ADD_NUMBER,
    DELETE_NUMBER,
    SEND_DEVICE_STAT_NOTFY,
    ACCEPT_CALL,
    REJECT_CALL,
    END_CALL,
    HOLD_CALL,
    RELEASE_HELD_CALL,
    RELEASE_ACTIVE_ACCEPT_WAITING_OR_HELD_CALL,
    SWAP_CALLS,
    ADD_HELD_CALL_TO_CONF,
    RELEASE_SPECIFIED_ACTIVE_CALL,
    PRIVATE_CONSULTATION_MODE,
    PUT_INCOMING_CALL_ON_HOLD,
    ACCEPT_HELD_INCOMING_CALL,
    REJECT_HELD_INCOMING_CALL,
    DIAL,
    REDIAL,
    DIAL_MEMORY,
    START_VR,
    STOP_VR,
    CALL_ACTION,
    QUERY_CURRENT_CALLS,
    QUERY_OPERATOR_NAME,
    QUERY_SUBSCRIBER_INFO,
    SCO_VOL_CTRL,
    MIC_VOL_CTRL,
    SPK_VOL_CTRL,
    SEND_DTMF,
    DISABLE_NREC_ON_AG,
    SEND_AT_CMD,
    HFP_AG,
    HID_HOST,
    SET_PROTOCOL,
    GET_PROTOCOL,
    SET_REPORT,
    GET_REPORT,
    VIRTUAL_UNPLUG,
    HID_BONDED_LIST,
    CFG_MTU,
    CONN_PARAMS,
    SET_AFH_CHANNELS,
    SEND_HCI_COMMAND,
    BACK_TO_MAIN,
    END,
} CommandList;

/**
 * Argument Count
 */
typedef enum {
    ZERO_PARAM,
    ONE_PARAM,
    TWO_PARAM,
    THREE_PARAM,
    FOUR_PARAM,
    FIVE_PARAM,
    SIX_PARAM,
} MaxParamCount;

typedef enum {
    MAIN_MENU,
    GAP_MENU,
    TEST_MENU,
    GATTC_TEST_MENU,
    GATTSTEST_MENU,
    RSP_MENU,
    HFP_AG_MENU,
} MenuType;

/**
 * Default menu_type is Main Menu
 */
MenuType menu_type = MAIN_MENU;

/**
 * UserMenuList
 */
typedef struct {
    CommandList cmd_id;
    const char cmd_name[COMMAND_SIZE];
    MaxParamCount max_param;
    const char cmd_help[COMMAND_SIZE];
} UserMenuList;

/**
 * list of supported commands for Main Menu
 */
UserMenuList MainMenu[] = {
    {GATTCTEST_OPTION,      "gattctest_menu",   ZERO_PARAM,   "gattctest_menu"},
    {GATTSTEST_OPTION,      "gattstest_menu",   ZERO_PARAM,   "gattstest_menu"},
    {HFP_AG,                "hfp_ag_menu",      ZERO_PARAM,   "hfp_ag_menu"},
    {MAIN_EXIT,             "exit",             ZERO_PARAM,   "exit"},
};



UserMenuList GattsTestMenu[] = {
    {GATTSTEST_INIT_SERVER,        "gattstest_init_server",        ZERO_PARAM,    "gattstest_init_server (only for Init time)"},
    {GATTSTEST_ADDSERVER,          "gattstest_addservers",         ZERO_PARAM,    "gattstest_addservers"},
    {GATTSTEST_ADDSERVICES,        "gattstest_addservices",        TWO_PARAM,     "gattstest_addservices<space><server instance><space><service instance>"},
    {GATTSTEST_INIT_ADVERTISER,    "gattstest_init_advertiser",    ZERO_PARAM,    "gattstest_init_advertiser initialzes advertiser"},
    {GATTSTEST_START_ADVERTISER,   "gattstest_start_advertiser",   ONE_PARAM,     "gattstest_start_advertiser<space><server instance>"},
    {GATTSTEST_READPHY,            "gattstest_readphy",            TWO_PARAM,     "gattstest_readphy<space><remote address><server instance>"},
    {GATTSTEST_SET_PREFERRED_PHY,  "gattstest_set_preferred_phy",  FOUR_PARAM,    "gattstest_set_preferred_phy<space><remote address><space><server instance><space><tx phy><space><rx phy>"},
    {GATTSTEST_STOP,               "gattstest_stop",               ONE_PARAM,     "gattstest_stop<space><server_instance>"},
    {GATTSTEST_DISABLE,             "gattstest_disable",         ZERO_PARAM,     "gattstest_disable"},
    {GATTSTEST_CANCEL_CONNECTION,   "gattstest_cancel_connection",  ONE_PARAM,   "gattstest_cancel_connection<space><remote address>"},
    {GATTSTEST_UNREGISTER_SERVER, "gattstest_unregister_Server",  ONE_PARAM,    "gattstest_unregister_server<space><server instance>"},
    {BACK_TO_MAIN,          "main_menu",       ZERO_PARAM,    "main_menu"},
};


#ifdef USE_GEN_GATT
/**
* list of supported commands for GATTCTEST Menu
*/
UserMenuList GattcTestMenu[] = {
    {GATTCTEST_INIT,              "gattctest_init",       ZERO_PARAM,    "gattctest_init (only for Init time)"},
    {GATTCTEST_SCAN_SETTINGS,       "gattctest_scanset",    TWO_PARAM,    "gattctest_scanset<space><scan_type><space><value> \
        eg: scanType: 0-NO_SET,1-SCAN_MODE,2-CB_Type,3-RESULT_TYPE,4-PHY,5-LEGACY,6-REPORT_DELAY,7-NUM_RESPONSE"},
    {GATTCTEST_SCAN_FILTER,       "gattctest_scanFilter",    TWO_PARAM,    "gattctest_scanFilter<space><filter_type><space><filter_Value> \
        eg: filterType: 0-NO_FILT,1-FILT_BD_ADDR,2-FILT_DEV_NAME,3-FILT_SRVC_UUID"},
    {GATTCTEST_SCANFILTER_MAN_DATA,       "gattctest_scanFilter_manData",    THREE_PARAM,    "gattctest_scanFilter_manData<space><manuId><space><ManuData><space><ManuMask>"},
    {GATTCTEST_START_SCAN,        "gattctest_start_scan", ZERO_PARAM,    "gattctest_start_scan"},
    {GATTCTEST_STOP_SCAN,         "gattctest_stop_scan",  ZERO_PARAM,    "gattctest_stop_scan"},
    {GATTCTEST_BATCH_SCAN,        "gattctest_batch_scan", ONE_PARAM,    "gattctest_batch_scan  0-FULL MODE 1- TRUNCATED MODE"},
    {BACK_TO_MAIN,          "main_menu",      ZERO_PARAM,    "main_menu"},
    {GATTCTEST_CONN_PARAMS,       "gattctest_conn_params",    THREE_PARAM,    "gattctest_conn_params<space><isAuto><space><phy><space><isOppur> \
        eg: isAuto(0/1);phy (0-255 (0 bit:1M(1); 1bit:2M(2); 2bit:Coded(4); or any combination); isOppur(0/1))"},
    {GATTCTEST_CONNECT,           "gattctest_connect",    TWO_PARAM,     "gattctest_connect<space><bt_address><space><transport>\
         eg. gattctest_connect 00:11:22:33:44:55 0(Auto)/1(BREDR)/2(LE)"},
    {GATTCTEST_DISCONNECT,           "gattctest_disconnect", ONE_PARAM,     "gattctest_disconnect<space><bt_address> \
          eg.gattctest_connect 00:11:22:33:44:55 "},
    {GATTCTEST_DISCSRVC,              "gattctest_discsrvc",       ONE_PARAM,    "gattctest_discsrvc<space><bdaddr>  discovering services"},
    {GATTCTEST_RDCHAR_UUID,              "gattctest_rdchar_uuid",       TWO_PARAM,    "gattctest_rdchar_uuid<space><bdaddr><space><uuid> \
        eg: reading char by uuid"},
    {GATTCTEST_READPHY,           "gattctest_readPhy",    ONE_PARAM,    "gattctest_readPhy<space><bt_address>"},
    {GATTCTEST_READRSSI,           "gattctest_readrssi",    ONE_PARAM,    "gattctest_readrssi<space><bt_address>"},
    {GATTCTEST_REQMTU,           "gattctest_reqMtu",    TWO_PARAM,    "gattctest_reqMtu<space><bt_address><space><value>"},
    {GATTCTEST_REFRESH,           "gattctest_refresh",    ONE_PARAM,    "gattctest_refresh<space><bt_address>"},
    {GATTCTEST_SETPHY,           "gattctest_setphy",    THREE_PARAM,    "gattctest_setphy<space>\
          <TxValue(0-255)><space><RxValue(0-255)><space><bt_address> (0-255 (0 bit:1M(1); 1bit:2M(2); 2bit:Coded(4); or any combination)"},
    {GATTCTEST_GETSERVICES,           "gattctest_getservices",    ONE_PARAM,    "gattctest_getservices<space><bt_address>"},
    {GATTCTEST_REQCONN_PRI,           "gattctest_reqconn_pri",    TWO_PARAM,    "gattctest_reqconn_pri<space><bt_address><space><priority 0/1/2>"},
    {GATTCTEST_GETCHARID,           "gattctest_getcharid",    TWO_PARAM,    "gattctest_getcharid<space><bt_address><space><instanceid>"},
    {GATTCTEST_RELIABLEWRITE,       "gattctest_reliablewrite",    TWO_PARAM,    "gattctest_reliablewrite<space><bt_address><space><instanceid>"},
    {GATTCTEST_GETDESCID,           "gattctest_getdescid",    TWO_PARAM,    "gattctest_getdescid<space><bt_address><space><instanceid>"},
    {GATTCTEST_GETSRVC,           "gattctest_getsrvc",    THREE_PARAM,    "gattctest_getsrvc<space><bt_address><space><UUID><space><INSTANCEID>"},
    {GATTCTEST_RDWRDESC,       "gattctest_RdWrDesc",    FIVE_PARAM,    "gattctest_RdWrDesc<space><bt_address><space><R-2/W-1><space><value><space><INSTANCEID><space><value length>"},
    {GATTCTEST_RDWRCHAR,           "gattctest_RdWrchar",    FIVE_PARAM,    "gattctest_RdWrchar<space><bt_address><space><R-2/W-1><space><value><space><INSTANCEID><space><value length>"},
    {GATTCTEST_CONN_DEVICES,     "gattctest_conn_dev",  ZERO_PARAM,    "gattctest_conn_dev"},
};
#endif



/**
 * list of supported commands for HFP_AG Menu
 */
UserMenuList HfpAGMenu[] = {
    {CONNECT,               "connect",       ONE_PARAM,    "connect<space><bt_address>"},
    {DISCONNECT,            "disconnect",    ONE_PARAM,    "disconnect<space><bt_address>"},
    {CREATE_SCO_CONN,       "create_sco",    ONE_PARAM,    "create_sco<space><bt_address>"},
    {DESTROY_SCO_CONN,      "destroy_sco",   ONE_PARAM,    "destroy_sco<space><bt_address>"},
    {VOIP_CALL_IND,         "voip_call_ind",   ONE_PARAM,    "voip_call_ind<space><bt_address>"},
    {END_VOIP_CALL,         "end_voip_call",   ONE_PARAM,    "end_voip_call<space><bt_address>"},
    {ACCEPT_VOIP_CALL,      "acpt_voip_call",  ONE_PARAM,    "acpt_voip_call<space><bt_address>"},
    {INCOM_VOIP_CALL_IND,   "incom_voip_call_ind", THREE_PARAM, "incom_voip_call_ind<space>"
      "<bt_address><space><number><space><call_active> eg:phone number - phone_number provided in"
      " PTS with out '+', call_active : (0 - no call is active, 1 - onecall is active)"},
    {SWAP_VOIP_CALLS,       "swap_voip_calls",  ONE_PARAM,    "swap_voip_calls<space><bt_address>"},
    {UPDATE_ACTIVE_CALLS_NUM, "update_Active_calls_num", ONE_PARAM, "update_active_calls_num<space>"
      "<0/1>eg:update_active_calls_num 1(0-decrease active call num,1-increase active call num)"},
    {UPDATE_HELD_CALLS_NUM, "update_held_calls_num", ONE_PARAM, "update_held_calls_num<space><0/1>"
      "eg: update_held_calls_num 1 (0 - decrease held calls num, 1 - increase held calls num)"},
    {ADD_NUMBER,            "add_number",     ONE_PARAM,    "add_number<space><number>"},
    {DELETE_NUMBER,         "delete_number",  ZERO_PARAM,    "delete_number"},
    {SEND_DEVICE_STAT_NOTFY, "send_device_stat_notfy", FOUR_PARAM, "send_device_stat_notfy<space>"
      "<bt_address><space><ntk_state><space><signal><space><batt_chg>"
      "eg:send_device_stat_notfy 00:15:83:6b:cf:8e 0(0/1-notavailable/available) 3(0-5) 5(0-5)"},
#if defined(BT_MODEM_INTEGRATION)
    {ACCEPT_CALL,           "accept_call",   ZERO_PARAM,   "accept_call"},
    {REJECT_CALL,           "reject_call",   ZERO_PARAM,   "reject_call"},
    {END_CALL,              "end_call",      ZERO_PARAM,   "end_call"},
    {HOLD_CALL,             "hold_call",     ZERO_PARAM,   "hold_call"},
    {RELEASE_HELD_CALL,     "release_held_call", ZERO_PARAM,   "release_held_call"},
    {SWAP_CALLS,            "swap_calls", ZERO_PARAM,   "swap_calls"},
    {ADD_HELD_CALL_TO_CONF, "add_held_call_to_conference", ZERO_PARAM,   "add_held_call_to_conference"},
    {DIAL,                  "dial",          ONE_PARAM,    "dial<space><phone_number>"},
    {QUERY_CURRENT_CALLS,   "query_current_calls", ZERO_PARAM,   "query_current_calls"},
    {QUERY_OPERATOR_NAME,   "query_operator_name", ZERO_PARAM,   "query_operator_name"},
    {QUERY_SUBSCRIBER_INFO, "query_subscriber_info", ZERO_PARAM, "query_subscriber_info"},
    {MIC_VOL_CTRL,          "mic_volume_control",   ONE_PARAM,   "mic_volume_control<space><value>"},
    {SPK_VOL_CTRL,          "speaker_volume_control",   ONE_PARAM,   "speaker_volume_control<space><value>"},
    {SEND_DTMF,             "send_dtmf",   ONE_PARAM,    "send_dtmf<space><code>"},
#endif
    {BACK_TO_MAIN,          "main_menu",     ZERO_PARAM,   "main_menu"},
};

#ifdef __cplusplus
extern "C"
{
#endif
/**
 * @brief DisplayMenu
 *
 *  It will display list of supported commands based an argument @ref MenuType
 *
 * @param[in] menu_type @ref MenuType specify which menu commands need to display
 * @return none
 */
static void DisplayMenu(MenuType menu_type);

/**
 * @brief HandleUserInput
 *
 *  It will parse user input.
 *
 * @param[out]  int :    cmd_id has command id from @ref CommandList
 * @param[out]  char[][] : input_args contains the command and arguments
 * @param[in]  MenuType : It refers to Menu type currently user in
 */
static bool HandleUserInput (int *cmd_id, char input_args[][COMMAND_ARG_SIZE],
                                                          MenuType menu_type);
/**
 * @brief SignalHandler
 *
 *  It will handle SIGINT signal.
 *
 * @param[in]  int signal number
 * @return none
 */
static void SignalHandler(int sig);

/**
 * @brief ExitHandler
 *
 * This function can be called from main thead or signal handler thread to exit from
 * bt-app
 *
 */
static void ExitHandler(void);

/**
 * @brief HandleMainCommand
 *
 * This will handle all the commands in @ref MainMenu
 *
 * @param[in]  int cmd_id has command id from @ref CommandList
 * @param[in]  char[][] user_cmd has parsed command with arguments passed by user
 * @return none
 */
static void HandleMainCommand(int cmd_id, char user_cmd[][COMMAND_ARG_SIZE]);

/**
 * @brief HandleTestCommand
 *
 *  This function will handle all the commands in @ref TestMenu
 *
 * @param[in] cmd_id It has command id from @ref CommandList
 * @param[in] user_cmd It has parsed commands with arguments passed by user
 * @return none
 */
static void HandleTestCommand(int cmd_id, char user_cmd[][COMMAND_ARG_SIZE]);

/**
 * @brief HandleRspCommand
 *
 *  This function will handle all the commands in @ref RspMenu
 *
 * @param[in] cmd_id It has command id from @ref CommandList
 * @param[in] user_cmd It has parsed commands with arguments passed by user
 * @return none
 */
static void HandleRspCommand(int cmd_id, char user_cmd[][COMMAND_ARG_SIZE]);

#ifdef USE_GEN_GATT
/**
* @brief HandleGattcTestCommand
*
*  This function will handle all the commands in @ref RspMenu
*
* @param[in] cmd_id It has command id from @ref CommandList
* @param[in] user_cmd It has parsed commands with arguments passed by user
* @return none
*/
static void HandleGattcTestCommand(int cmd_id, char user_cmd[][COMMAND_ARG_SIZE]);

/**
 * @brief HandleGattsTestCommand
 *
 *  This function will handle all the commands in @ref GattstestMenu
 *
 * @param[in] cmd_id It has command id from @ref CommandList
 * @param[in] user_cmd It has parsed commands with arguments passed by user
 * @return none
 */
 static void HandleGattsTestCommand(int cmd_id, char user_cmd [ ] [ COMMAND_ARG_SIZE ]);

#endif

/**
 * @brief HandleGapCommand
 *
 *  This function will handle all the commands in @ref GapMenu
 *
 * @param[in] cmd_id It has command id from @ref CommandList
 * @param[out] user_cmd It has parsed commands with arguments passed by user
 * @return none
 */
static void HandleGapCommand(int cmd_id, char user_cmd[][COMMAND_ARG_SIZE]);

/**
 * @brief BtCmdHandler
 *
 *  This function will take the command as input from the user. And process as per
 *  command recieved
 *
 * @param[in] context
 * @return none
 */
static void BtCmdHandler (void *context);


/**
 * @brief BtCmdHandler
 *
 *  This function will accept the events from other threads and performs action
 *  based on event
 *
 * @param[in] context
 * @return none
 */
void BtMainMsgHandler (void *context);

#ifdef __cplusplus
}
#endif

/**
 * @class BluetoothApp
 *
 * @brief This module will take inputs from command line and also from
 * socket interface. Perform action based on inputs.
 *
 */

class BluetoothApp {
  private:
    config_t *config;
    bool is_bt_enable_default_;
    bool is_bt_enable_autotest;
    bool is_user_input_enabled_;
    bool is_socket_input_enabled_;
    bool is_a2dp_sink_enabled_;
    bool is_a2dp_sink_split_enabled_;
    bool is_avrcp_enabled_;
    bool is_a2dp_source_enabled_;
    bool is_hfp_client_enabled_;
    bool is_hfp_ag_enabled_;
    bool is_pan_enable_default_;
    bool is_gatt_enable_default_;
#ifdef USE_BT_OBEX
    bool is_obex_enabled_;
    bool is_pbap_client_enabled_;
    bool is_opp_enabled_;
#endif
    bool is_hid_enable_default_;
    reactor_object_t *cmd_reactor_;
    struct hw_device_t *device_;
    bluetooth_device_t *bt_device_;
    bool LoadConfigParameters(const char *config_path);
    void InitHandler();
    void DeInitHandler();
    bool LoadBtStack();
    void UnLoadBtStack();

  public:
    int listen_socket_local_;
    int client_socket_;
    bool ssp_notification;
    bool pin_notification;
    bool is_hid_enabled;
#ifdef USE_BT_OBEX
    bool incoming_file_notification;
#endif

    /**
     * structure object for standard Bluetooth DM interface
     */
    const bt_interface_t *bt_interface;

    reactor_object_t *listen_reactor_;
    reactor_object_t *accept_reactor_;
    UiCommandStatus status;
    bt_state_t bt_state;
    bt_discovery_state_t bt_discovery_state;

    //key is bt_bdaddr_t storing as a string in map
    std::map < std::string, std::string> bonded_devices;
    std::vector<InquiryDB> inquiry_list;
    static int inq_db_count;
    SSPReplyEvent   ssp_data;
    PINReplyEvent   pin_reply;


    /**
     * @brief Bluetooth Application Constructor
     *
     * It will initialize class members to default values and calls the
     * @ref LoadConfigParameters, this function reads config file @ref CONFIG_FILE_PATH
     */
    BluetoothApp();

    /**
     * @ref Bluetooth Application Distructor
     *
     * It will free the config parameter
     *
     */

    ~BluetoothApp();
    /**
     * @brief GetState
     *
     *  This function will returns the current BT state
     *
     * @return bt_state_t
     */
    bt_state_t GetState();
    /**
     * @brief HandleSspInput
     *
     *  This function will handles the SSP Input, it shows a message on console
     * for user input
     *
     * @return bool
     */

    /**
     *@brief ProcessEvent
     *
     * It will handle incomming events
     *
     * @param BtEvent
     * @return none
     */
    void ProcessEvent(BtEvent * pEvent);

    /**
    */
    DeviceProperties* inq_db_find_bdaddr(std::string bda);
    DeviceProperties* inq_db_add_new(DeviceProperties& deviceFound);
    unsigned long long getTimeInMilliSec();
};

#endif
