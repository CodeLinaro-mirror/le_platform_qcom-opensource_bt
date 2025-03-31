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
 * Changes from Qualcomm Innovation Center, Inc. are provided under the
 * following license:
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 ******************************************************************************/

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <bsd/string.h>
#include <algorithm>
#include <sstream>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <unistd.h>
#include <dlfcn.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <hardware/hardware.h>
#include <iostream>
#include <iomanip>
#include "Main.hpp"
#include "SdpClient.hpp"
#include "A2dp_Sink.hpp"
#include "A2dp_Sink_Split.hpp"
#include "Hid.hpp"
#include "HfpClient.hpp"
#include "Pan.hpp"
#ifdef USE_GEN_GATT
#include "GattLibService.hpp"
#include "GattcTest.hpp"
#include "GattsTest.hpp"
#include "Rsp.hpp"
#endif
#include "HfpAG.hpp"
#include "Audio_Manager.hpp"


#include "A2dp_Src.hpp"
#include "Avrcp.hpp"

#ifdef USE_BT_OBEX
#include "PbapClient.hpp"
#include "Opp.hpp"
#endif
#include "osi/include/compat.h"
#include "osi/include/properties.h"

#include "utils.h"
#ifdef SUPPORT_ESL_AP
#include <hardware/vendor_ap.h>
#endif

#ifdef USE_GEN_GATT
using namespace gatt;
using namespace btapp;
#endif
#define LOGTAG  "MAIN "
#define LOCAL_SOCKET_NAME "/etc/bluetooth/btappsocket"
int server_num;
bool file_read = 0;
bool init_advertiser_file = 0;
long onoff_count = 0;
long onoff_index = 0;
bool exithandler_waitbtoff = FALSE;
bool isBT_ON = true;

bool is_pts_test_enabled_ = false; // used to help test certficatoin cases

extern Gap *g_gap;
extern A2dp_Sink *pA2dpSink;
extern A2dp_Sink_Split *pA2dpSinkSplit;
extern HidH *pHid;
extern A2dp_Source *pA2dpSource;
extern Pan *g_pan;
extern BT_Audio_Manager *pBTAM;
extern Hfp_Client *pHfpClient;
extern Hfp_Ag *pHfpAG;
extern Avrcp *pAvrcp;
bool gattsEnabled = false;

extern const char *BT_PAN_ENABLED;
extern SdpClient *g_sdpClient;
#ifdef USE_BT_OBEX
extern PbapClient *g_pbapClient;
extern Opp *g_opp;
extern const char *BT_OBEX_ENABLED;
#endif
extern Spp_Server *pSppServer;
extern Spp_Client *pSppClient;

static BluetoothApp *g_bt_app = NULL;
extern ThreadInfo threadInfo[THREAD_ID_MAX];

#ifdef USE_BT_OBEX
static alarm_t *opp_incoming_file_accept_timer = NULL;
#define USER_ACCEPTANCE_TIMEOUT 25000
#endif

#ifdef USE_GEN_GATT
GattLibService *g_gatt;
extern const char *BT_GATT_ENABLED;
extern GattcTest *gattctest;
extern GattsTest *gattstest;
extern Rsp *rsp;
#endif

extern const char *BT_SPP_SERVER_ENABLED;
extern const char *BT_SPP_CLIENT_ENABLED;

#ifdef __cplusplus
extern "C"
{
#endif

ThreadIdType thread_id = THREAD_ID_MAX; //thread id to handle sink non-split,split
static void SendDisableCmdToGap();
#ifdef SUPPORT_ESL_AP
void HandleAPDeinitCmd(void);
static uint8_t cert_cmd_parameter_count = 0;
#endif
/**
 * @brief main function
 *
 *
 *  This is main function of BT Application
 *
 * @param  argc
 * @param  *argv[]
 *
 */
int main (int argc, char *argv[]) {
#define MAX_LINE_LEN 256
    int count = 0;
    FILE *fp = popen("pgrep -f btapp", "r");
    if (fp) {
        char buffer[MAX_LINE_LEN];
        while (fgets(buffer, sizeof(buffer), fp) != NULL) {
            count++;
        }
        pclose(fp);
        if (count > 1) {
            fprintf(stdout, " Another instance of btapp is already running\n");
            return 0;
        }
    }
    // initialize signal handler
    signal(SIGINT, SignalHandler);
    signal(SIGTERM, SignalHandler);
    signal(SIGKILL, SignalHandler);

    ThreadInfo *main_thread = &threadInfo[THREAD_ID_MAIN];
#ifndef USE_ANDROID_LOGGING
    openlog ("bt-app", LOG_CONS | LOG_PID | LOG_NDELAY, LOG_LOCAL1);
#endif
    main_thread->thread_id = thread_new (main_thread->thread_name);
    if (main_thread->thread_id) {
        BtEvent *event = new BtEvent;
        event->event_id = MAIN_API_INIT;
        ALOGV (LOGTAG " Posting init to Main thread\n");
        PostMessage (THREAD_ID_MAIN, event);

        // wait for Main thread to exit
        thread_join (main_thread->thread_id);
        thread_free (main_thread->thread_id);
    }
#ifndef USE_ANDROID_LOGGING
    closelog ();
#endif
    return 0;
}

static bool HandleUserInput (int *cmd_id, char input_args[][COMMAND_ARG_SIZE],
                                                          MenuType menu_type) {
    char user_input[COMMAND_SIZE] = {'\0'};
    int index = 0 , found_index = -1, num_cmds;
    char *temp_arg = NULL;
    char delim[] = " ";
    char *ptr1;
    int param_count = 0;
    bool status = false;
    int max_param = 0;
    UserMenuList *menu = NULL;

    // validate the input string
    if(((fgets (user_input, sizeof (user_input), stdin)) == NULL) ||
       (user_input[0] == '\n')) {
        return status;
    }
    // remove trialing \n character
    user_input[strlen(user_input) - 1] = '\0';

    // According to the current menu assign Command menu
    switch(menu_type) {
        case GAP_MENU:
            menu = &GapMenu[0];
            num_cmds  = NO_OF_COMMANDS(GapMenu);
            break;
        case PAN_MENU:
            menu = &PanMenu[0];
            num_cmds  = NO_OF_COMMANDS(PanMenu);
            break;
        case TEST_MENU:
            menu = &TestMenu[0];
            num_cmds  = NO_OF_COMMANDS(TestMenu);
            break;
#ifdef USE_GEN_GATT
         case RSP_MENU:
            menu = &RspMenu[0];
            num_cmds  = NO_OF_COMMANDS(RspMenu);
           break;
        case GATTC_TEST_MENU:
            menu = &GattcTestMenu[0];
            num_cmds  = NO_OF_COMMANDS(GattcTestMenu);
            break;
        case GATTSTEST_MENU:
            menu = &GattsTestMenu[0];
            num_cmds  = NO_OF_COMMANDS(GattsTestMenu);
            break;
#endif
        case A2DP_SINK_MENU:
            menu = &A2dpSinkMenu[0];
            num_cmds  = NO_OF_COMMANDS(A2dpSinkMenu);
            break;
        case A2DP_SOURCE_MENU:
            menu = &A2dpSourceMenu[0];
            num_cmds  = NO_OF_COMMANDS(A2dpSourceMenu);
            break;
        case HFP_CLIENT_MENU:
            menu = &HfpClientMenu[0];
            num_cmds  = NO_OF_COMMANDS(HfpClientMenu);
            break;
#ifdef USE_BT_OBEX
        case PBAP_CLIENT_MENU:
            menu = &PbapClientMenu[0];
            num_cmds  = NO_OF_COMMANDS(PbapClientMenu);
            break;
        case OPP_MENU:
            menu = &OppMenu[0];
            num_cmds  = NO_OF_COMMANDS(OppMenu);
            break;
#endif
        case SPP_SERVER_MENU:
            menu = &SppServerMenu[0];
            num_cmds = NO_OF_COMMANDS(SppServerMenu);
            break;
        case SPP_CLIENT_MENU:
            menu = &SppClientMenu[0];
            num_cmds = NO_OF_COMMANDS(SppClientMenu);
            break;
        case HFP_AG_MENU:
            menu = &HfpAGMenu[0];
            num_cmds  = NO_OF_COMMANDS(HfpAGMenu);
            break;
        case HIDH_MENU:
            menu = &HidMenu[0];
            num_cmds  = NO_OF_COMMANDS(HidMenu);
            break;
#ifdef SUPPORT_ESL_AP
        case ESLAP_MENU:
            menu = &EslapMenu[0];
            num_cmds  = NO_OF_COMMANDS(EslapMenu);
            break;
#endif
        case MAIN_MENU:
        // fallback to default main menu
        default:
            menu = &MainMenu[0];
            num_cmds  = NO_OF_COMMANDS(MainMenu);
            break;
    }

    if ( (temp_arg = strtok_r(user_input, delim, &ptr1)) != NULL ) {
        // find out the command name
        for (index = 0; index < num_cmds; index++) {
            if(!strcasecmp (menu[index].cmd_name, temp_arg)) {
                *cmd_id = menu[index].cmd_id;
                found_index = index;
                strlcpy(input_args[param_count], temp_arg, COMMAND_ARG_SIZE);
                input_args[param_count++][COMMAND_ARG_SIZE - 1] = '\0';
                break;
            }
        }

        // validate the command parameters
        if (found_index != -1 ) {
            max_param = menu[found_index].max_param;
            while ((temp_arg = strtok_r(NULL, delim, &ptr1)) &&
                    (param_count < max_param + 1)) {
                strlcpy(input_args[param_count], temp_arg, COMMAND_ARG_SIZE);
                input_args[param_count++][COMMAND_ARG_SIZE - 1] = '\0';
            }

#ifdef SUPPORT_ESL_AP
            if ((menu_type == ESLAP_MENU) && (menu[found_index].cmd_id == AP_CERT)) {
                cert_cmd_parameter_count = param_count;
                status = true;
            } else {
#endif
                // consider command as other param
                if(param_count == max_param + 1) {
                    if(temp_arg != NULL) {
                        fprintf( stdout, " Maximum params reached\n");
                        fprintf( stdout, " Refer help: %s\n", menu[found_index].cmd_help);
                    } else {
                        status = true;
                    }
                } else if(param_count < max_param + 1) {
                    fprintf( stdout, " Missing required parameters\n");
                    fprintf( stdout, " Refer help: %s\n", menu[found_index].cmd_help);
                }
#ifdef SUPPORT_ESL_AP
            }
#endif
        } else {
            // to handle the paring inputs
            if(temp_arg != NULL) {
                strlcpy(input_args[param_count], temp_arg, COMMAND_ARG_SIZE);
                input_args[param_count++][COMMAND_ARG_SIZE - 1] = '\0';
            }
        }
    }
    return status;
}

static void DisplayMenu(MenuType menu_type) {

    UserMenuList *menu = NULL;
    int index = 0, num_cmds = 0;
    char pts_value[6];
    switch(menu_type) {
        case GAP_MENU:
            menu = &GapMenu[0];
            num_cmds  = NO_OF_COMMANDS(GapMenu);
            break;
        case PAN_MENU:
            menu = &PanMenu[0];
            num_cmds  = NO_OF_COMMANDS(PanMenu);
            break;
        case TEST_MENU:
            menu = &TestMenu[0];
            num_cmds  = NO_OF_COMMANDS(TestMenu);
            break;
#ifdef USE_GEN_GATT
        case RSP_MENU:
            menu = &RspMenu[0];
            num_cmds  = NO_OF_COMMANDS(RspMenu);
            break;
        case GATTC_TEST_MENU:
            menu = &GattcTestMenu[0];
            num_cmds = NO_OF_COMMANDS(GattcTestMenu);
            break;
        case GATTSTEST_MENU:
            menu = &GattsTestMenu[0];
            num_cmds  = NO_OF_COMMANDS(GattsTestMenu);
            break;
#endif
        case MAIN_MENU:
            menu = &MainMenu[0];
            num_cmds  = NO_OF_COMMANDS(MainMenu);
            break;
        case A2DP_SINK_MENU:
            menu = &A2dpSinkMenu[0];
            osi_property_get("vendor.bt.pts.certification", pts_value, "false");
            if (!(strcmp(pts_value,"true"))) {
                num_cmds  = NO_OF_COMMANDS(A2dpSinkMenu);
            } else {
                num_cmds  = 32;
            }
            break;
        case A2DP_SOURCE_MENU:
            menu = &A2dpSourceMenu[0];
            num_cmds  = NO_OF_COMMANDS(A2dpSourceMenu);
            break;
        case HFP_CLIENT_MENU:
            menu = &HfpClientMenu[0];
            num_cmds  = NO_OF_COMMANDS(HfpClientMenu);
            break;
#ifdef USE_BT_OBEX
        case PBAP_CLIENT_MENU:
            menu = &PbapClientMenu[0];
            num_cmds  = NO_OF_COMMANDS(PbapClientMenu);
            break;
        case OPP_MENU:
            menu = &OppMenu[0];
            num_cmds  = NO_OF_COMMANDS(OppMenu);
            break;
#endif
        case SPP_SERVER_MENU:
            menu = &SppServerMenu[0];
            num_cmds = NO_OF_COMMANDS(SppServerMenu);
            break;
        case SPP_CLIENT_MENU:
            menu = &SppClientMenu[0];
            num_cmds = NO_OF_COMMANDS(SppClientMenu);
            break;
        case HFP_AG_MENU:
            osi_property_get("vendor.bt.pts.certification", pts_value, "false");
            if (!(strcmp(pts_value,"true"))) {
                num_cmds  = NO_OF_COMMANDS(HfpAGMenu);
           } else {
                num_cmds  = 7;
           }
            menu = &HfpAGMenu[0];
            break;
        case HIDH_MENU:
            menu = &HidMenu[0];
            num_cmds  = NO_OF_COMMANDS(HidMenu);
            break;
#ifdef SUPPORT_ESL_AP
        case ESLAP_MENU:
            menu = &EslapMenu[0];
            num_cmds  = NO_OF_COMMANDS(EslapMenu);
            break;
#endif
    }
    fprintf (stdout, " \n***************** Menu *******************\n");
    for (index = 0; index < num_cmds; index++)
        fprintf (stdout, "\t %s \n",  menu[index].cmd_help);
    fprintf (stdout, " ******************************************\n");
}

static void SignalHandler(int sig) {
    fprintf (stdout, " btapp will exit for signal %d\n", sig);
    printBtappState();
    signal(sig, SIG_IGN);
    ExitHandler();
}

static void ExitHandler(void) {
    if ( g_bt_app ) {
        // post the disable message to GAP incase BT is on
        if(g_bt_app->bt_state == BT_STATE_ON) {
#ifdef SUPPORT_ESL_AP
            if(g_bt_app->ap_state == AP_STATE_ON) {
                HandleAPDeinitCmd();
            } else if(g_bt_app->ap_state == AP_STATE_OFF){
                fprintf (stdout, " \n AP is Already OFF\n");
            }
#endif
            SendDisableCmdToGap();
            // No need to wait here, wait for BT turn off(BT Disable event)
            // before proceeding to close the BT APP
            exithandler_waitbtoff = TRUE;
        } else if(g_bt_app->bt_state == BT_STATE_OFF){
            // If BT enable command is still in process, wait for BT to turn on.
            if (g_bt_app->status.enable_cmd == COMMAND_INPROGRESS){
                isBT_ON = false;
                fprintf( stdout, " Previous enable command is still in process\n");
            }else{
                fprintf (stdout, " \n BT is Already OFF, Just exiting APP\n");
                kill(getpid(), SIGKILL);
            }
        }
    }
}

static int GetArgsFromString(char cmdString[COMMAND_ARG_SIZE], uint8_t* nArgs){
    int     i;
    char    *p;
    char    *p_s;
    bool     cont= false;

    p_s = cmdString; i = 0;
    while(p_s)
    {
        /* skip to comma delimiter */
        for(p = p_s; *p != ',' && *p != 0; p++);

        /* get integre value */
        if (*p != 0)
        {
            *p = 0;
            cont = true;
        }
        else
            cont = false;

        nArgs[i] = atoi(p_s);

        if (cont && i < MAX_SUB_ARGUMENTS-1)
        {
            p_s = p + 1;
            i++;
        }
        else
            break;
    }
    ++i;

    for(int n=0; n < i; n++)
        ALOGV (LOGTAG " GetArgsFromString Arg%d: %d\n",n, nArgs[n]);
    ALOGV (LOGTAG " GetArgsFromString return %d\n", i);
    return i;
}

static int Get32ArgsFromString(char cmdString[COMMAND_ARG_SIZE], uint32_t* nArgs){
    int     i;
    char    *p;
    char    *p_s;
    bool     cont= false;

    p_s = cmdString; i = 0;
    while(p_s)
    {
        /* skip to comma delimiter */
        for(p = p_s; *p != ',' && *p != 0; p++);

        /* get integre value */
        if (*p != 0)
        {
            *p = 0;
            cont = true;
        }
        else
            cont = false;

        nArgs[i] = atoi(p_s);

        if (cont && i < MAX_SUB_ARGUMENTS-1)
        {
            p_s = p + 1;
            i++;
        }
        else
            break;
    }
    ++i;

    for(int n=0; n < i; n++)
        ALOGV (LOGTAG " GetArgsFromString Arg%d: %d\n",n, nArgs[n]);
    ALOGV (LOGTAG " GetArgsFromString return %d\n", i);
    return i;
}

static void HandleA2dpSinkCommand(int cmd_id, char user_cmd[][COMMAND_ARG_SIZE]) {
    ALOGD(LOGTAG "HandleA2DPSinkCommand cmd_id = %d", cmd_id);
    BtEvent *event = NULL;
    uint8_t* pAttr = NULL;
    uint32_t* pAttr32 = NULL;
    uint8_t  num_Attr = 0;
    uint64_t* pData = NULL;
    switch (cmd_id) {
        case CONNECT:
        {
            if (string_is_bdaddr(user_cmd[ONE_PARAM])) {
                bt_bdaddr_t address;
                string_to_bdaddr(user_cmd[ONE_PARAM], &address);
                if (!g_gap->IsDeviceBonded(address)) {
                   fprintf( stdout, " Please pair with the device before A2DPSink connection\n");
                   break;
                }
                event = new BtEvent;
                memset(event, 0, sizeof(BtEvent));
                event->a2dpSinkEvent.event_id = A2DP_SINK_API_CONNECT_REQ;
                string_to_bdaddr(user_cmd[ONE_PARAM], &event->a2dpSinkEvent.bd_addr);
                PostMessage (thread_id, event);
            }
            else {
                fprintf(stdout, "Invalid BT Address\n");
            }
            break;
        }
        case DISCONNECT:
            if (string_is_bdaddr(user_cmd[ONE_PARAM])) {
                event = new BtEvent;
                memset(event, 0, sizeof(BtEvent));
                event->a2dpSinkEvent.event_id = A2DP_SINK_API_DISCONNECT_REQ;
                string_to_bdaddr(user_cmd[ONE_PARAM], &event->a2dpSinkEvent.bd_addr);
                PostMessage (thread_id, event);
            }
            else {
                fprintf(stdout, "Invalid BT Address\n");
            }
            break;
        case PLAY:
            if (string_is_bdaddr(user_cmd[ONE_PARAM])) {
                event = new BtEvent;
                memset(event, 0, sizeof(BtEvent));
                event->avrcpCtrlPassThruEvent.event_id = AVRCP_CTRL_PASS_THRU_CMD_REQ;
                event->avrcpCtrlPassThruEvent.key_id = CMD_ID_PLAY;
                string_to_bdaddr(user_cmd[ONE_PARAM], &event->avrcpCtrlPassThruEvent.bd_addr);
                PostMessage (THREAD_ID_AVRCP, event);
            }
            else {
                fprintf(stdout, "Invalid BT Address\n");
            }
            break;
        case PAUSE:
            if (string_is_bdaddr(user_cmd[ONE_PARAM])) {
                event = new BtEvent;
                memset(event, 0, sizeof(BtEvent));
                event->avrcpCtrlPassThruEvent.event_id = AVRCP_CTRL_PASS_THRU_CMD_REQ;
                event->avrcpCtrlPassThruEvent.key_id = CMD_ID_PAUSE;
                string_to_bdaddr(user_cmd[ONE_PARAM], &event->avrcpCtrlPassThruEvent.bd_addr);
                PostMessage (THREAD_ID_AVRCP, event);
            }
            else {
                fprintf(stdout, "Invalid BT Address\n");
            }
            break;
        case STOP:
            if (string_is_bdaddr(user_cmd[ONE_PARAM])) {
                event = new BtEvent;
                memset(event, 0, sizeof(BtEvent));
                event->avrcpCtrlPassThruEvent.event_id = AVRCP_CTRL_PASS_THRU_CMD_REQ;
                event->avrcpCtrlPassThruEvent.key_id = CMD_ID_STOP;
                string_to_bdaddr(user_cmd[ONE_PARAM], &event->avrcpCtrlPassThruEvent.bd_addr);
                PostMessage (THREAD_ID_AVRCP, event);
            }
            else {
                fprintf(stdout, "Invalid BT Address\n");
            }
            break;
        case AVDT_START:
            if (string_is_bdaddr(user_cmd[ONE_PARAM])) {
                event = new BtEvent;
                memset(event, 0, sizeof(BtEvent));
                event->a2dpSinkEvent.event_id = A2DP_SINK_AUDIO_START_REQ;
                string_to_bdaddr(user_cmd[ONE_PARAM], &event->a2dpSinkEvent.bd_addr);
                PostMessage (thread_id, event);
            }
            else {
                fprintf(stdout, "Invalid BT Address\n");
            }
            break;
        case AVDT_SUSPEND:
            if (string_is_bdaddr(user_cmd[ONE_PARAM])) {
                event = new BtEvent;
                memset(event, 0, sizeof(BtEvent));
                event->a2dpSinkEvent.event_id = A2DP_SINK_AUDIO_SUSPEND_REQ;
                string_to_bdaddr(user_cmd[ONE_PARAM], &event->a2dpSinkEvent.bd_addr);
                PostMessage (thread_id, event);
            }
            else {
                fprintf(stdout, "Invalid BT Address\n");
            }
            break;
        case ACCEPT:
            event = new BtEvent;
            memset(event, 0, sizeof(BtEvent));
            event->a2dpSinkEvent.event_id = A2DP_SINK_ACCEPT_PENDING_COMMAND;
            PostMessage (thread_id, event);
            break;
        case REJECT:
            event = new BtEvent;
            memset(event, 0, sizeof(BtEvent));
            event->a2dpSinkEvent.event_id = A2DP_SINK_REJECT_PENDING_COMMAND;
            PostMessage (thread_id, event);
            break;
        case FASTFORWARD:
            event = new BtEvent;
            memset(event, 0, sizeof(BtEvent));
            event->avrcpCtrlPassThruEvent.event_id = AVRCP_CTRL_PASS_THRU_CMD_REQ;
            event->avrcpCtrlPassThruEvent.key_id = CMD_ID_FF;
            string_to_bdaddr(user_cmd[ONE_PARAM], &event->avrcpCtrlPassThruEvent.bd_addr);
            PostMessage (THREAD_ID_AVRCP, event);
            break;
        case REWIND:
            event = new BtEvent;
            memset(event, 0, sizeof(BtEvent));
            event->avrcpCtrlPassThruEvent.event_id = AVRCP_CTRL_PASS_THRU_CMD_REQ;
            event->avrcpCtrlPassThruEvent.key_id = CMD_ID_REWIND;
            string_to_bdaddr(user_cmd[ONE_PARAM], &event->avrcpCtrlPassThruEvent.bd_addr);
            PostMessage (THREAD_ID_AVRCP, event);
            break;
        case FORWARD:
            event = new BtEvent;
            memset(event, 0, sizeof(BtEvent));
            event->avrcpCtrlPassThruEvent.event_id = AVRCP_CTRL_PASS_THRU_CMD_REQ;
            event->avrcpCtrlPassThruEvent.key_id = CMD_ID_FORWARD;
            string_to_bdaddr(user_cmd[ONE_PARAM], &event->avrcpCtrlPassThruEvent.bd_addr);
            PostMessage (THREAD_ID_AVRCP, event);
            break;
        case BACKWARD:
            event = new BtEvent;
            memset(event, 0, sizeof(BtEvent));
            event->avrcpCtrlPassThruEvent.event_id = AVRCP_CTRL_PASS_THRU_CMD_REQ;
            event->avrcpCtrlPassThruEvent.key_id = CMD_ID_BACKWARD;
            string_to_bdaddr(user_cmd[ONE_PARAM], &event->avrcpCtrlPassThruEvent.bd_addr);
            PostMessage (THREAD_ID_AVRCP, event);
            break;
        case POWER:
            event = new BtEvent;
            memset(event, 0, sizeof(BtEvent));
            event->avrcpCtrlPassThruEvent.event_id = AVRCP_CTRL_PASS_THRU_CMD_REQ;
            event->avrcpCtrlPassThruEvent.key_id = CMD_ID_POWER;
            string_to_bdaddr(user_cmd[ONE_PARAM], &event->avrcpCtrlPassThruEvent.bd_addr);
            PostMessage (THREAD_ID_AVRCP, event);
            break;
        case VOL_UP:
            event = new BtEvent;
            memset(event, 0, sizeof(BtEvent));
            event->avrcpCtrlPassThruEvent.event_id = AVRCP_CTRL_PASS_THRU_CMD_REQ;
            event->avrcpCtrlPassThruEvent.key_id = CMD_ID_VOL_UP;
            string_to_bdaddr(user_cmd[ONE_PARAM], &event->avrcpCtrlPassThruEvent.bd_addr);
            PostMessage (THREAD_ID_AVRCP, event);
            break;
        case VOL_DOWN:
            event = new BtEvent;
            memset(event, 0, sizeof(BtEvent));
            event->avrcpCtrlPassThruEvent.event_id = AVRCP_CTRL_PASS_THRU_CMD_REQ;
            event->avrcpCtrlPassThruEvent.key_id = CMD_ID_VOL_DOWN;
            string_to_bdaddr(user_cmd[ONE_PARAM], &event->avrcpCtrlPassThruEvent.bd_addr);
            PostMessage (THREAD_ID_AVRCP, event);
            break;
        case MUTE:
            event = new BtEvent;
            memset(event, 0, sizeof(BtEvent));
            event->avrcpCtrlPassThruEvent.event_id = AVRCP_CTRL_PASS_THRU_CMD_REQ;
            event->avrcpCtrlPassThruEvent.key_id = CMD_ID_MUTE;
            string_to_bdaddr(user_cmd[ONE_PARAM], &event->avrcpCtrlPassThruEvent.bd_addr);
            PostMessage (THREAD_ID_AVRCP, event);
            break;
        case VOL_CHANGED_NOTI:
            event = new BtEvent;
            memset(event, 0, sizeof(BtEvent));
            event->avrcpCtrlPassThruEvent.event_id = AVRCP_CTRL_VOL_CHANGED_NOTI_REQ;
            event->avrcpCtrlPassThruEvent.arg1 = atoi(user_cmd[ONE_PARAM]);
            if(event->avrcpCtrlPassThruEvent.arg1 < 0 || event->avrcpCtrlPassThruEvent.arg1 > 15) {
                fprintf( stdout, " Volume out of Range 0-15\n");
                delete event;
                break;
            }
            PostMessage (THREAD_ID_AVRCP, event);
            break;
        case GET_CAP:
            event = new BtEvent;
            memset(event, 0, sizeof(BtEvent));
            event->avrcpCtrlEvent.event_id = AVRCP_CTRL_GET_CAP_REQ;
            string_to_bdaddr(user_cmd[ONE_PARAM], &event->avrcpCtrlEvent.bd_addr);
            event->avrcpCtrlEvent.arg1 = atoi(user_cmd[TWO_PARAM]);
            PostMessage (THREAD_ID_AVRCP, event);
            break;
        case LIST_PLAYER_SETTING_ATTR:
            event = new BtEvent;
            memset(event, 0, sizeof(BtEvent));
            event->avrcpCtrlEvent.event_id = AVRCP_CTRL_LIST_PALYER_SETTING_ATTR_REQ;
            string_to_bdaddr(user_cmd[ONE_PARAM], &event->avrcpCtrlEvent.bd_addr);
            PostMessage (THREAD_ID_AVRCP, event);
            break;
        case LIST_PALYER_SETTING_VALUE:
            event = new BtEvent;
            memset(event, 0, sizeof(BtEvent));
            event->avrcpCtrlEvent.event_id = AVRCP_CTRL_LIST_PALYER_SETTING_VALUE_REQ;
            string_to_bdaddr(user_cmd[ONE_PARAM], &event->avrcpCtrlEvent.bd_addr);
            event->avrcpCtrlEvent.arg1 = atoi(user_cmd[TWO_PARAM]);
            PostMessage (THREAD_ID_AVRCP, event);
            break;
        case GET_PALYER_APP_SETTING:
            event = new BtEvent;
            memset(event, 0, sizeof(BtEvent));
            pAttr = new uint8_t[MAX_SUB_ARGUMENTS];
            event->avrcpCtrlEvent.event_id = AVRCP_CTRL_GET_PALYER_APP_SETTING_REQ;
            string_to_bdaddr(user_cmd[ONE_PARAM], &event->avrcpCtrlEvent.bd_addr);
            num_Attr = GetArgsFromString(user_cmd[TWO_PARAM],pAttr);
            if(num_Attr)
            {
                event->avrcpCtrlEvent.num_attrb = num_Attr;
                event->avrcpCtrlEvent.buf_ptr = pAttr;
                PostMessage (THREAD_ID_AVRCP, event);
            }
            break;
        case SET_PALYER_APP_SETTING:
            {
                event = new BtEvent;
                memset(event, 0, sizeof(BtEvent));
                pAttr = new uint8_t[MAX_SUB_ARGUMENTS];
                uint8_t* pValue = new uint8_t[MAX_SUB_ARGUMENTS];
                event->avrcpCtrlEvent.event_id = AVRCP_CTRL_SET_PALYER_APP_SETTING_VALUE_REQ;
                string_to_bdaddr(user_cmd[ONE_PARAM], &event->avrcpCtrlEvent.bd_addr);
                num_Attr = GetArgsFromString(user_cmd[TWO_PARAM],pAttr);
                GetArgsFromString(user_cmd[THREE_PARAM],pValue);
                if(num_Attr)
                {
                    event->avrcpCtrlEvent.num_attrb = num_Attr;
                    event->avrcpCtrlEvent.buf_ptr = pAttr;
                    event->avrcpCtrlEvent.arg6 = (uint64_t)pValue;
                    PostMessage (THREAD_ID_AVRCP, event);
                }
                break;
            }

        case GET_ELEMENT_ATTR:{
                event = new BtEvent;
                memset(event, 0, sizeof(BtEvent));
                pAttr32 = new uint32_t[MAX_SUB_ARGUMENTS];
                event->avrcpCtrlEvent.event_id = AVRCP_CTRL_GET_ELEMENT_ATTR_REQ;
                string_to_bdaddr(user_cmd[ONE_PARAM], &event->avrcpCtrlEvent.bd_addr);
                int nCount = atoi(user_cmd[TWO_PARAM]);
                if(nCount == 0)
                    event->avrcpCtrlEvent.num_attrb = nCount;
                else
                {
                    num_Attr = Get32ArgsFromString(user_cmd[THREE_PARAM],pAttr32);
                    if(num_Attr)
                    {
                        event->avrcpCtrlEvent.num_attrb = num_Attr;
                        event->avrcpCtrlEvent.buf_ptr32 = pAttr32;
                    }
                }
                PostMessage (THREAD_ID_AVRCP, event);
                break;
            }
        case GET_PLAY_STATUS:
            event = new BtEvent;
            memset(event, 0, sizeof(BtEvent));
            event->avrcpCtrlEvent.event_id = AVRCP_CTRL_GET_PLAY_STATUS_REQ;
            string_to_bdaddr(user_cmd[ONE_PARAM], &event->avrcpCtrlEvent.bd_addr);
            PostMessage (THREAD_ID_AVRCP, event);
            break;
        case SET_ADDRESSED_PLAYER:
            event = new BtEvent;
            memset(event, 0, sizeof(BtEvent));
            event->avrcpCtrlEvent.event_id = AVRCP_CTRL_SET_ADDRESSED_PLAYER_REQ;
            string_to_bdaddr(user_cmd[ONE_PARAM], &event->avrcpCtrlEvent.bd_addr);
            event->avrcpCtrlEvent.arg3 = atoi(user_cmd[TWO_PARAM]);
            PostMessage (THREAD_ID_AVRCP, event);
            break;
        case SET_BROWSED_PLAYER:
            event = new BtEvent;
            memset(event, 0, sizeof(BtEvent));
            event->avrcpCtrlEvent.event_id = AVRCP_CTRL_SET_BROWSED_PLAYER_REQ;
            string_to_bdaddr(user_cmd[ONE_PARAM], &event->avrcpCtrlEvent.bd_addr);
            event->avrcpCtrlEvent.arg3 = atoi(user_cmd[TWO_PARAM]);
            PostMessage (THREAD_ID_AVRCP, event);
            break;
        case CHANGE_PATH:{
            event = new BtEvent;
            memset(event, 0, sizeof(BtEvent));
            event->avrcpCtrlEvent.event_id = AVRCP_CTRL_CHANGE_PATH_REQ;
            string_to_bdaddr(user_cmd[ONE_PARAM], &event->avrcpCtrlEvent.bd_addr);
            event->avrcpCtrlEvent.arg1 = atoi(user_cmd[TWO_PARAM]);
            event->avrcpCtrlEvent.arg6 = strtoull(user_cmd[THREE_PARAM],NULL,10);

            PostMessage (THREAD_ID_AVRCP, event);
            }
            break;
        case GETFOLDERITEMS:{
                event = new BtEvent;
                memset(event, 0, sizeof(BtEvent));
                event->avrcpCtrlEvent.event_id = AVRCP_CTRL_GET_FOLDER_ITEMS_REQ;
                string_to_bdaddr(user_cmd[ONE_PARAM], &event->avrcpCtrlEvent.bd_addr);
                event->avrcpCtrlEvent.arg1 = atoi(user_cmd[TWO_PARAM]);
                event->avrcpCtrlEvent.arg4 = atoi(user_cmd[THREE_PARAM]);
                event->avrcpCtrlEvent.arg5 = atoi(user_cmd[FOUR_PARAM]);
                int nCount = atoi(user_cmd[FIVE_PARAM]);
                if(nCount == 255 || nCount == 0)
                    event->avrcpCtrlEvent.arg2 = nCount;
                else
                {
                    event->avrcpCtrlEvent.buf_ptr32 = new uint32_t[MAX_SUB_ARGUMENTS];
                    num_Attr = Get32ArgsFromString(user_cmd[SIX_PARAM],event->avrcpCtrlEvent.buf_ptr32);
                    event->avrcpCtrlEvent.arg2 = num_Attr;
                }
                PostMessage (THREAD_ID_AVRCP, event);
                break;
            }
        case GETITEMATTRIBUTES:{
                event = new BtEvent;
                memset(event, 0, sizeof(BtEvent));
                event->avrcpCtrlEvent.event_id = AVRCP_CTRL_GET_ITEM_ATTRIBUTES_REQ;
                string_to_bdaddr(user_cmd[ONE_PARAM], &event->avrcpCtrlEvent.bd_addr);
                event->avrcpCtrlEvent.arg1 = atoi(user_cmd[TWO_PARAM]);
                event->avrcpCtrlEvent.arg6 = strtoull(user_cmd[THREE_PARAM],NULL,10);
                event->avrcpCtrlEvent.arg3 = atoi(user_cmd[FOUR_PARAM]);
                int nCount = atoi(user_cmd[FIVE_PARAM]);
                if(nCount == 255 || nCount == 0)
                    event->avrcpCtrlEvent.arg2 = nCount;
                else
                {
                    event->avrcpCtrlEvent.buf_ptr32 = new uint32_t[MAX_SUB_ARGUMENTS];
                    num_Attr = Get32ArgsFromString(user_cmd[SIX_PARAM],event->avrcpCtrlEvent.buf_ptr32);
                    event->avrcpCtrlEvent.arg2 = num_Attr;
                }
                PostMessage (THREAD_ID_AVRCP, event);
                break;
            }
        case PLAYITEM:
            event = new BtEvent;
            memset(event, 0, sizeof(BtEvent));
            event->avrcpCtrlEvent.event_id = AVRCP_CTRL_PLAY_ITEMS_REQ;
            string_to_bdaddr(user_cmd[ONE_PARAM], &event->avrcpCtrlEvent.bd_addr);
            event->avrcpCtrlEvent.arg1 = atoi(user_cmd[TWO_PARAM]);
            event->avrcpCtrlEvent.arg6 = strtoull(user_cmd[THREE_PARAM],NULL,10);
            event->avrcpCtrlEvent.arg3 = atoi(user_cmd[FOUR_PARAM]);
            PostMessage (THREAD_ID_AVRCP, event);
            break;
        case ADDTONOWPLAYING:
            event = new BtEvent;
            memset(event, 0, sizeof(BtEvent));
            event->avrcpCtrlEvent.event_id = AVRCP_CTRL_ADDTO_NOW_PLAYING_REQ;
            string_to_bdaddr(user_cmd[ONE_PARAM], &event->avrcpCtrlEvent.bd_addr);
            event->avrcpCtrlEvent.arg1 = atoi(user_cmd[TWO_PARAM]);
            event->avrcpCtrlEvent.arg6 = strtoull(user_cmd[THREE_PARAM],NULL,10);
            event->avrcpCtrlEvent.arg3 = atoi(user_cmd[FOUR_PARAM]);
            PostMessage (THREAD_ID_AVRCP, event);
            break;
        case SEARCH:{
            event = new BtEvent;
            memset(event, 0, sizeof(BtEvent));
            event->avrcpCtrlEvent.event_id = AVRCP_CTRL_SEARCH_REQ;
            string_to_bdaddr(user_cmd[ONE_PARAM], &event->avrcpCtrlEvent.bd_addr);
            int length = atoi(user_cmd[TWO_PARAM]);
            event->avrcpCtrlEvent.arg3 = length;
            event->avrcpCtrlEvent.buf_ptr = new uint8_t[length+1];
            memset(event->avrcpCtrlEvent.buf_ptr, 0, length+1);
            strlcpy((char*)event->avrcpCtrlEvent.buf_ptr,user_cmd[THREE_PARAM],length+1);
            PostMessage (THREAD_ID_AVRCP, event);
            break;
            }
        case REG_NOTIFICATION:
            event = new BtEvent;
            memset(event, 0, sizeof(BtEvent));
            event->avrcpCtrlEvent.event_id = AVRCP_CTRL_REG_NOTIFICATION_REQ;
            string_to_bdaddr(user_cmd[ONE_PARAM], &event->avrcpCtrlEvent.bd_addr);
            event->avrcpCtrlEvent.arg1 = atoi(user_cmd[TWO_PARAM]);
            PostMessage (THREAD_ID_AVRCP, event);
            break;
        case CODEC_LIST:
            event = new BtEvent;
            event->a2dpCodecListEvent.event_id = A2DP_SINK_CODEC_LIST;
            memset( (void *) event->a2dpCodecListEvent.codec_list, '\0',
                sizeof(event->a2dpCodecListEvent.codec_list));
            strlcpy(event->a2dpCodecListEvent.codec_list, user_cmd[ONE_PARAM],
                COMMAND_SIZE);
            PostMessage (thread_id, event);
            break;
        case BACK_TO_MAIN:
            menu_type = MAIN_MENU;
            DisplayMenu(menu_type);
            break;
    }
}

static void HandleA2dpSourceCommand(int cmd_id, char user_cmd[][COMMAND_ARG_SIZE]) {
    ALOGV(LOGTAG, "HandleA2DPSourceCommand cmd_id = %d", cmd_id);
    BtEvent *event = NULL;
    switch (cmd_id) {
        case CONNECT:
        {
            if (string_is_bdaddr(user_cmd[ONE_PARAM])) {
                bt_bdaddr_t address;
                string_to_bdaddr(user_cmd[ONE_PARAM], &address);
                if (!g_gap->IsDeviceBonded(address)){
                    fprintf( stdout, " Please pair with the device before A2DPSource connection\n");
                    break;
                }
                event = new BtEvent;
                event->a2dpSourceEvent.event_id = A2DP_SOURCE_API_CONNECT_REQ;
                string_to_bdaddr(user_cmd[ONE_PARAM], &event->a2dpSourceEvent.bd_addr);
                PostMessage (THREAD_ID_A2DP_SOURCE, event);
            }
            else {
                fprintf(stdout, "Invalid BT Address\n");
            }
            break;
        }
        case DISCONNECT:
            if (string_is_bdaddr(user_cmd[ONE_PARAM])) {
                event = new BtEvent;
                event->a2dpSourceEvent.event_id = A2DP_SOURCE_API_DISCONNECT_REQ;
                string_to_bdaddr(user_cmd[ONE_PARAM], &event->a2dpSourceEvent.bd_addr);
                PostMessage (THREAD_ID_A2DP_SOURCE, event);
            }
            else {
                fprintf(stdout, "Invalid BT Address\n");
            }
            break;
        case PLAY:
            event = new BtEvent;
            event->avrcpTargetEvent.event_id = A2DP_SOURCE_AUDIO_CMD_REQ;
            event->avrcpTargetEvent.key_id = CMD_ID_PLAY;
            PostMessage (THREAD_ID_A2DP_SOURCE, event);
            break;
        case PAUSE:
            event = new BtEvent;
            event->avrcpTargetEvent.event_id = A2DP_SOURCE_AUDIO_CMD_REQ;
            event->avrcpTargetEvent.key_id = CMD_ID_PAUSE;
            PostMessage (THREAD_ID_A2DP_SOURCE, event);
            break;
        case STOP:
            event = new BtEvent;
            event->avrcpTargetEvent.event_id = A2DP_SOURCE_AUDIO_CMD_REQ;
            event->avrcpTargetEvent.key_id = CMD_ID_STOP;
            PostMessage (THREAD_ID_A2DP_SOURCE, event);
            break;
        case AVDT_START:
            event = new BtEvent;
            event->avrcpTargetEvent.event_id = A2DP_SOURCE_AUDIO_AVDT_CMD_REQ;
            event->avrcpTargetEvent.key_id = CMD_ID_PLAY;
            PostMessage (THREAD_ID_A2DP_SOURCE, event);
            break;
        case AVDT_SUSPEND:
            event = new BtEvent;
            event->avrcpTargetEvent.event_id = A2DP_SOURCE_AUDIO_AVDT_CMD_REQ;
            event->avrcpTargetEvent.key_id = CMD_ID_PAUSE;
            PostMessage (THREAD_ID_A2DP_SOURCE, event);
            break;
        case TRACK_CHANGE:
            event = new BtEvent;
            event->avrcpTargetEvent.event_id = AVRCP_TARGET_TRACK_CHANGED;
            PostMessage (THREAD_ID_A2DP_SOURCE, event);
            break;
        case NOW_PLAYING_CONTENT_CHANGED:
            event = new BtEvent;
            event->avrcpTargetEvent.event_id = AVRCP_TARGET_NOW_PLAYING_CONTENT_CHANGED;
            PostMessage (THREAD_ID_A2DP_SOURCE, event);
            break;
        case SET_ABS_VOL:
            event = new BtEvent;
            event->avrcpTargetEvent.event_id = AVRCP_TARGET_SET_ABS_VOL;
            event->avrcpTargetEvent.arg3 = atoi(user_cmd[ONE_PARAM]);
            PostMessage (THREAD_ID_A2DP_SOURCE, event);
            break;
        case SEND_VOL_UP_DOWN:
            event = new BtEvent;
            event->avrcpTargetEvent.event_id = AVRCP_TARGET_SEND_VOL_UP_DOWN;
            event->avrcpTargetEvent.arg3 = atoi(user_cmd[ONE_PARAM]);
            PostMessage (THREAD_ID_A2DP_SOURCE, event);
            break;
        case ADDR_PLAYER_CHANGE:
            event = new BtEvent;
            event->avrcpTargetEvent.event_id = AVRCP_TARGET_ADDR_PLAYER_CHANGED;
            event->avrcpTargetEvent.arg3 = atoi(user_cmd[ONE_PARAM]);
            PostMessage (THREAD_ID_A2DP_SOURCE, event);
            break;
        case AVAIL_PLAYER_CHANGE:
            event = new BtEvent;
            event->avrcpTargetEvent.event_id = AVRCP_TARGET_AVAIL_PLAYER_CHANGED;
            PostMessage (THREAD_ID_A2DP_SOURCE, event);
            break;
        case BIGGER_METADATA:
            event = new BtEvent;
            event->avrcpTargetEvent.event_id = AVRCP_TARGET_USE_BIGGER_METADATA;
            PostMessage (THREAD_ID_A2DP_SOURCE, event);
            break;
        case CODEC_LIST:
            event = new BtEvent;
            event->a2dpCodecListEvent.event_id = A2DP_SOURCE_CODEC_LIST;
            memset( (void *) event->a2dpCodecListEvent.codec_list, '\0',
                sizeof(event->a2dpCodecListEvent.codec_list));
            strlcpy(event->a2dpCodecListEvent.codec_list, user_cmd[ONE_PARAM],
                COMMAND_SIZE);
            PostMessage (THREAD_ID_A2DP_SOURCE, event);
            break;
        case MODE_CHANGE:
            event = new BtEvent;
            string_to_bdaddr(user_cmd[ONE_PARAM],&event->a2dpCodecListEvent.bd_addr);
            event->a2dpCodecListEvent.event_id = A2DP_SOURCE_CODEC_MODE_CHANGE;
            memset( (void *) event->a2dpCodecListEvent.codec_list, '\0',
                sizeof(event->a2dpCodecListEvent.codec_list));
            strlcpy(event->a2dpCodecListEvent.codec_list, user_cmd[TWO_PARAM],
                COMMAND_SIZE);
            PostMessage (THREAD_ID_A2DP_SOURCE, event);
        case SET_EQUALIZER_VAL:
            event = new BtEvent;
            event->avrcpTargetEvent.event_id = AVRCP_SET_EQUALIZER_VAL;
            event->avrcpTargetEvent.arg3 = atoi(user_cmd[ONE_PARAM]);
            PostMessage (THREAD_ID_A2DP_SOURCE, event);
            break;
        case SET_REPEAT_VAL:
            event = new BtEvent;
            event->avrcpTargetEvent.event_id = AVRCP_SET_REPEAT_VAL;
            event->avrcpTargetEvent.arg3 = atoi(user_cmd[ONE_PARAM]);
            PostMessage (THREAD_ID_A2DP_SOURCE, event);
            break;
        case SET_SHUFFLE_VAL:
            event = new BtEvent;
            event->avrcpTargetEvent.event_id = AVRCP_SET_SHUFFLE_VAL;
            event->avrcpTargetEvent.arg3 = atoi(user_cmd[ONE_PARAM]);
            PostMessage (THREAD_ID_A2DP_SOURCE, event);
            break;
        case SET_SCAN_VAL:
            event = new BtEvent;
            event->avrcpTargetEvent.event_id = AVRCP_SET_SCAN_VAL;
            event->avrcpTargetEvent.arg3 = atoi(user_cmd[ONE_PARAM]);
            PostMessage (THREAD_ID_A2DP_SOURCE, event);
            break;
        case SET_SCMST_CP_FLAG:
            event = new BtEvent;
            event->a2dpSourceEvent.event_id = A2DP_SOURCE_SET_SCMST_CP_FLAG;
            string_to_bdaddr(user_cmd[ONE_PARAM], &event->a2dpSourceEvent.bd_addr);
            event->a2dpSourceEvent.arg1 = atoi(user_cmd[TWO_PARAM]);
            PostMessage (THREAD_ID_A2DP_SOURCE, event);
            break;
        case BACK_TO_MAIN:
            menu_type = MAIN_MENU;
            DisplayMenu(menu_type);
            break;
    }
}

static void HandleHfpClientCommand(int cmd_id, char user_cmd[][COMMAND_ARG_SIZE]) {
    ALOGD(LOGTAG, "HandleHfpClientCommand cmd_id = %d", cmd_id);
    BtEvent *event = NULL;
    switch (cmd_id) {
        case CONNECT:
        {
            if (string_is_bdaddr(user_cmd[ONE_PARAM])) {
                bt_bdaddr_t address;
                string_to_bdaddr(user_cmd[ONE_PARAM], &address);
                if (!g_gap->IsDeviceBonded(address)) {
                    fprintf(stdout," Please pair with the device before HfpClient connection\n");
                    break;
                }
                event = new BtEvent;
                event->hfp_client_event.event_id = HFP_CLIENT_API_CONNECT_REQ;
                string_to_bdaddr(user_cmd[ONE_PARAM], &event->hfp_client_event.bd_addr);
                PostMessage (THREAD_ID_HFP_CLIENT, event);
            }
            else {
                fprintf(stdout, "Invalid BT Address\n");
            }
            break;
        }
        case DISCONNECT:
            if (string_is_bdaddr(user_cmd[ONE_PARAM])) {
                event = new BtEvent;
                event->hfp_client_event.event_id = HFP_CLIENT_API_DISCONNECT_REQ;
                string_to_bdaddr(user_cmd[ONE_PARAM], &event->hfp_client_event.bd_addr);
                PostMessage (THREAD_ID_HFP_CLIENT, event);
            }
            else {
                fprintf(stdout, "Invalid BT Address\n");
            }
            break;
        case CREATE_SCO_CONN:
            if (string_is_bdaddr(user_cmd[ONE_PARAM])) {
                event = new BtEvent;
                event->hfp_client_event.event_id = HFP_CLIENT_API_CONNECT_AUDIO_REQ;
                string_to_bdaddr(user_cmd[ONE_PARAM], &event->hfp_client_event.bd_addr);
                PostMessage (THREAD_ID_HFP_CLIENT, event);
            }
            else {
                 fprintf(stdout, "Invalid BT Address\n");
            }
            break;
        case DESTROY_SCO_CONN:
            if (string_is_bdaddr(user_cmd[ONE_PARAM])) {
                event = new BtEvent;
                event->hfp_client_event.event_id = HFP_CLIENT_API_DISCONNECT_AUDIO_REQ;
                string_to_bdaddr(user_cmd[ONE_PARAM], &event->hfp_client_event.bd_addr);
                PostMessage (THREAD_ID_HFP_CLIENT, event);
            }
            else {
                fprintf(stdout, "Invalid BT Address\n");
            }
            break;
        case ACCEPT_CALL:
            event = new BtEvent;
            event->hfp_client_event.event_id = HFP_CLIENT_API_ACCEPT_CALL_REQ;
            PostMessage (THREAD_ID_HFP_CLIENT, event);
            break;
        case REJECT_CALL:
            event = new BtEvent;
            event->hfp_client_event.event_id = HFP_CLIENT_API_REJECT_CALL_REQ;
            PostMessage (THREAD_ID_HFP_CLIENT, event);
            break;
        case END_CALL:
            event = new BtEvent;
            event->hfp_client_event.event_id = HFP_CLIENT_API_END_CALL_REQ;
            PostMessage (THREAD_ID_HFP_CLIENT, event);
            break;
        case HOLD_CALL:
            event = new BtEvent;
            event->hfp_client_event.event_id = HFP_CLIENT_API_HOLD_CALL_REQ;
            PostMessage (THREAD_ID_HFP_CLIENT, event);
            break;
        case RELEASE_HELD_CALL:
            event = new BtEvent;
            event->hfp_client_event.event_id = HFP_CLIENT_API_RELEASE_HELD_CALL_REQ;
            PostMessage (THREAD_ID_HFP_CLIENT, event);
            break;
        case RELEASE_ACTIVE_ACCEPT_WAITING_OR_HELD_CALL:
            event = new BtEvent;
            event->hfp_client_event.event_id = HFP_CLIENT_API_RELEASE_ACTIVE_ACCEPT_WAITING_OR_HELD_CALL_REQ;
            PostMessage (THREAD_ID_HFP_CLIENT, event);
            break;
        case SWAP_CALLS:
            event = new BtEvent;
            event->hfp_client_event.event_id = HFP_CLIENT_API_SWAP_CALLS_REQ;
            PostMessage (THREAD_ID_HFP_CLIENT, event);
            break;
        case ADD_HELD_CALL_TO_CONF:
            event = new BtEvent;
            event->hfp_client_event.event_id = HFP_CLIENT_API_ADD_HELD_CALL_TO_CONF_REQ;
            PostMessage (THREAD_ID_HFP_CLIENT, event);
            break;
        case RELEASE_SPECIFIED_ACTIVE_CALL:
            event = new BtEvent;
            event->hfp_client_event.event_id = HFP_CLIENT_API_RELEASE_SPECIFIED_ACTIVE_CALL_REQ;
            event->hfp_client_event.arg1 = atoi(user_cmd[ONE_PARAM]);
            PostMessage (THREAD_ID_HFP_CLIENT, event);
            break;
        case PRIVATE_CONSULTATION_MODE:
            event = new BtEvent;
            event->hfp_client_event.event_id = HFP_CLIENT_API_PRIVATE_CONSULTATION_MODE_REQ;
            event->hfp_client_event.arg1 = atoi(user_cmd[ONE_PARAM]);
            PostMessage (THREAD_ID_HFP_CLIENT, event);
            break;
        case PUT_INCOMING_CALL_ON_HOLD:
            event = new BtEvent;
            event->hfp_client_event.event_id = HFP_CLIENT_API_PUT_INCOMING_CALL_ON_HOLD_REQ;
            PostMessage (THREAD_ID_HFP_CLIENT, event);
            break;
        case ACCEPT_HELD_INCOMING_CALL:
            event = new BtEvent;
            event->hfp_client_event.event_id = HFP_CLIENT_API_ACCEPT_HELD_INCOMING_CALL_REQ;
            PostMessage (THREAD_ID_HFP_CLIENT, event);
            break;
        case REJECT_HELD_INCOMING_CALL:
            event = new BtEvent;
            event->hfp_client_event.event_id = HFP_CLIENT_API_REJECT_HELD_INCOMING_CALL_REQ;
            PostMessage (THREAD_ID_HFP_CLIENT, event);
            break;
        case DIAL:
            event = new BtEvent;
            event->hfp_client_event.event_id = HFP_CLIENT_API_DIAL_REQ;
            strlcpy(event->hfp_client_event.str, user_cmd[ONE_PARAM], 20);
            PostMessage (THREAD_ID_HFP_CLIENT, event);
            break;
        case REDIAL:
            event = new BtEvent;
            event->hfp_client_event.event_id = HFP_CLIENT_API_REDIAL_REQ;
            PostMessage (THREAD_ID_HFP_CLIENT, event);
            break;
        case DIAL_MEMORY:
            event = new BtEvent;
            event->hfp_client_event.event_id = HFP_CLIENT_API_DIAL_MEMORY_REQ;
            event->hfp_client_event.arg1 = atoi(user_cmd[ONE_PARAM]);
            PostMessage (THREAD_ID_HFP_CLIENT, event);
            break;
        case START_VR:
            event = new BtEvent;
            event->hfp_client_event.event_id = HFP_CLIENT_API_START_VR_REQ;
            PostMessage (THREAD_ID_HFP_CLIENT, event);
            break;
        case STOP_VR:
            event = new BtEvent;
            event->hfp_client_event.event_id = HFP_CLIENT_API_STOP_VR_REQ;
            PostMessage (THREAD_ID_HFP_CLIENT, event);
            break;
        case CALL_ACTION:
            event = new BtEvent;
            event->hfp_client_event.event_id = HFP_CLIENT_API_CALL_ACTION_REQ;
            event->hfp_client_event.arg1 = atoi(user_cmd[ONE_PARAM]);
            event->hfp_client_event.arg2 = atoi(user_cmd[TWO_PARAM]);
            PostMessage (THREAD_ID_HFP_CLIENT, event);
            break;
        case QUERY_CURRENT_CALLS:
            event = new BtEvent;
            event->hfp_client_event.event_id = HFP_CLIENT_API_QUERY_CURRENT_CALLS_REQ;
            PostMessage (THREAD_ID_HFP_CLIENT, event);
            break;
        case QUERY_OPERATOR_NAME:
            event = new BtEvent;
            event->hfp_client_event.event_id = HFP_CLIENT_API_QUERY_OPERATOR_NAME_REQ;
            PostMessage (THREAD_ID_HFP_CLIENT, event);
            break;
        case QUERY_SUBSCRIBER_INFO:
            event = new BtEvent;
            event->hfp_client_event.event_id = HFP_CLIENT_API_QUERY_SUBSCRIBER_INFO_REQ;
            PostMessage (THREAD_ID_HFP_CLIENT, event);
            break;
        case SCO_VOL_CTRL:
            event = new BtEvent;
            event->hfp_client_event.event_id = HFP_CLIENT_API_SCO_VOL_CTRL_REQ;
            event->hfp_client_event.arg1 = atoi(user_cmd[ONE_PARAM]);
            event->hfp_client_event.arg2 = atoi(user_cmd[TWO_PARAM]);
            PostMessage (THREAD_ID_HFP_CLIENT, event);
            break;
        case MIC_VOL_CTRL:
            event = new BtEvent;
            event->hfp_client_event.event_id = HFP_CLIENT_API_MIC_VOL_CTRL_REQ;
            event->hfp_client_event.arg1 = atoi(user_cmd[ONE_PARAM]);
            if(event->hfp_client_event.arg1 < 0 || event->hfp_client_event.arg1 > 15) {
                fprintf( stdout, " Volume out of Range 0-15\n");
                delete event;
                break;
            }
            PostMessage (THREAD_ID_HFP_CLIENT, event);
            break;
        case SPK_VOL_CTRL:
            event = new BtEvent;
            event->hfp_client_event.event_id = HFP_CLIENT_API_SPK_VOL_CTRL_REQ;
            event->hfp_client_event.arg1 = atoi(user_cmd[ONE_PARAM]);
            /* Trigger is from Application side need to send AT+VGS,
             * so second argument set to true */
            event->hfp_client_event.arg2 = true;
            if(event->hfp_client_event.arg1 < 0 || event->hfp_client_event.arg1 > 15) {
                    fprintf( stdout, " Volume out of Range 0-15\n");
                    delete event;
                    break;
            }
            PostMessage (THREAD_ID_HFP_CLIENT, event);
            break;
        case SEND_DTMF:
            event = new BtEvent;
            event->hfp_client_event.event_id = HFP_CLIENT_API_SEND_DTMF_REQ;
            strlcpy(event->hfp_client_event.str, user_cmd[ONE_PARAM], 20);
            PostMessage (THREAD_ID_HFP_CLIENT, event);
            break;
        case DISABLE_NREC_ON_AG:
            event = new BtEvent;
            event->hfp_client_event.event_id = HFP_CLIENT_API_DISABLE_NREC_ON_AG_REQ;
            PostMessage (THREAD_ID_HFP_CLIENT, event);
            break;
        case BACK_TO_MAIN:
            menu_type = MAIN_MENU;
            DisplayMenu(menu_type);
            break;
    }
}

static void HandleHfpAGCommand(int cmd_id, char user_cmd[][COMMAND_ARG_SIZE]) {
    ALOGD(LOGTAG, "HandleHfpAGCommand cmd_id = %d", cmd_id);
    fprintf(stdout, "HandleHfpAGCommand cmd_id = %d\n" , cmd_id);
    BtEvent *event = NULL;
    switch (cmd_id) {
        case CONNECT:
        {
            if (string_is_bdaddr(user_cmd[ONE_PARAM])) {
                bt_bdaddr_t address;
                string_to_bdaddr(user_cmd[ONE_PARAM], &address);
                if (!g_gap->IsDeviceBonded(address)) {
                    fprintf( stdout, " Please pair with the device before HfpAG connection\n");
                    break;
                }
                event = new BtEvent;
                event->hfp_ag_event.event_id = HFP_AG_API_CONNECT_REQ;
                string_to_bdaddr(user_cmd[ONE_PARAM], &event->hfp_ag_event.bd_addr);
                PostMessage (THREAD_ID_HFP_AG, event);
            }
            else {
                fprintf(stdout, "Invalid BT Address\n");
            }
            break;
        }
        case DISCONNECT:
            if (string_is_bdaddr(user_cmd[ONE_PARAM])) {
                event = new BtEvent;
                event->hfp_ag_event.event_id = HFP_AG_API_DISCONNECT_REQ;
                string_to_bdaddr(user_cmd[ONE_PARAM], &event->hfp_ag_event.bd_addr);
                PostMessage (THREAD_ID_HFP_AG, event);
            }
            else {
                fprintf(stdout, "Invalid BT Address\n");
            }
            break;
        case CREATE_SCO_CONN:
            if (string_is_bdaddr(user_cmd[ONE_PARAM])) {
                event = new BtEvent;
                event->hfp_ag_event.event_id = HFP_AG_API_CONNECT_AUDIO_REQ;
                string_to_bdaddr(user_cmd[ONE_PARAM], &event->hfp_ag_event.bd_addr);
                PostMessage (THREAD_ID_HFP_AG, event);
            }
            else {
                fprintf(stdout, "Invalid BT Address\n");
            }
            break;
        case DESTROY_SCO_CONN:
            if (string_is_bdaddr(user_cmd[ONE_PARAM])) {
                event = new BtEvent;
                event->hfp_ag_event.event_id = HFP_AG_API_DISCONNECT_AUDIO_REQ;
                string_to_bdaddr(user_cmd[ONE_PARAM], &event->hfp_ag_event.bd_addr);
                PostMessage (THREAD_ID_HFP_AG, event);
            }
            else {
                fprintf(stdout, "Invalid BT Address\n");
            }
            break;
        case VOIP_CALL_IND:
            if (string_is_bdaddr(user_cmd[ONE_PARAM])) {
                event = new BtEvent;
                event->hfp_ag_event.event_id = HFP_AG_VOIP_CALL_INDICATION;
                string_to_bdaddr(user_cmd[ONE_PARAM], &event->hfp_ag_event.bd_addr);
                PostMessage (THREAD_ID_HFP_AG, event);
            }
            else {
                fprintf(stdout, "Invalid BT Address\n");
            }
            break;
        case END_VOIP_CALL:
            if (string_is_bdaddr(user_cmd[ONE_PARAM])) {
                event = new BtEvent;
                event->hfp_ag_event.event_id = HFP_AG_VOIP_CALL_TERMINATION;
                string_to_bdaddr(user_cmd[ONE_PARAM], &event->hfp_ag_event.bd_addr);
                PostMessage (THREAD_ID_HFP_AG, event);
            }
            else {
               fprintf(stdout, "Invalid BT Address\n");
            }
            break;
        case ACCEPT_VOIP_CALL:
            event = new BtEvent;
            event->hfp_ag_event.event_id = HFP_AG_VOIP_CALL_ACCEPT;
            string_to_bdaddr(user_cmd[ONE_PARAM], &event->hfp_ag_event.bd_addr);
            PostMessage (THREAD_ID_HFP_AG, event);
            break;
        case INCOM_VOIP_CALL_IND:
            event = new BtEvent;
            event->hfp_ag_event.event_id = HFP_AG_VOIP_CALL_INCOMING_INDICATION;
            string_to_bdaddr(user_cmd[ONE_PARAM], &event->hfp_ag_event.bd_addr);
            strlcpy(event->hfp_ag_event.str, user_cmd[TWO_PARAM], 20);
            event->hfp_ag_event.arg1 = atoi(user_cmd[THREE_PARAM]);
            PostMessage (THREAD_ID_HFP_AG, event);
            break;
        case SWAP_VOIP_CALLS:
            event = new BtEvent;
            event->hfp_ag_event.event_id = HFP_AG_VOIP_CALL_SWAP;
            string_to_bdaddr(user_cmd[ONE_PARAM], &event->hfp_ag_event.bd_addr);
            PostMessage (THREAD_ID_HFP_AG, event);
            break;
        case UPDATE_ACTIVE_CALLS_NUM:
            event = new BtEvent;
            event->hfp_ag_event.event_id = HFP_AG_UPDATE_ACTIVE_CALL_NUM;
            event->hfp_ag_event.arg1 = atoi(user_cmd[ONE_PARAM]);
            PostMessage (THREAD_ID_HFP_AG, event);
            break;
        case UPDATE_HELD_CALLS_NUM:
            event = new BtEvent;
            event->hfp_ag_event.event_id = HFP_AG_UPDATE_HELD_CALL_NUM;
            event->hfp_ag_event.arg1 = atoi(user_cmd[ONE_PARAM]);
            PostMessage (THREAD_ID_HFP_AG, event);
            break;
        case ADD_NUMBER:
            event = new BtEvent;
            event->hfp_ag_event.event_id = HFP_AG_ADD_NUMBER;
            strlcpy(event->hfp_ag_event.str, user_cmd[ONE_PARAM], 20);
            PostMessage (THREAD_ID_HFP_AG, event);
            break;
        case DELETE_NUMBER:
            event = new BtEvent;
            event->hfp_ag_event.event_id = HFP_AG_DELETE_NUMBER;
            PostMessage (THREAD_ID_HFP_AG, event);
            break;
        case SEND_DEVICE_STAT_NOTFY:
            event = new BtEvent;
            event->hfp_ag_event.event_id = HFP_AG_SEND_DEVICE_STAT_NOTFY;
            string_to_bdaddr(user_cmd[ONE_PARAM], &event->hfp_ag_event.bd_addr);
            event->hfp_ag_event.arg1 = atoi(user_cmd[TWO_PARAM]);
            event->hfp_ag_event.arg2 = atoi(user_cmd[THREE_PARAM]);
            event->hfp_ag_event.arg3 = atoi(user_cmd[FOUR_PARAM]);
            PostMessage (THREAD_ID_HFP_AG, event);
            break;
        case ACCEPT_CALL:
            event = new BtEvent;
            event->hfp_ag_event.event_id = HFP_AG_API_ACCEPT_CALL_REQ;
            PostMessage (THREAD_ID_HFP_AG, event);
            break;
        case REJECT_CALL:
            event = new BtEvent;
            event->hfp_ag_event.event_id = HFP_AG_API_REJECT_CALL_REQ;
            PostMessage (THREAD_ID_HFP_AG, event);
            break;
        case END_CALL:
            event = new BtEvent;
            event->hfp_ag_event.event_id = HFP_AG_API_END_CALL_REQ;
            PostMessage (THREAD_ID_HFP_AG, event);
            break;
        case HOLD_CALL:
            event = new BtEvent;
            event->hfp_ag_event.event_id = HFP_AG_API_HOLD_CALL_REQ;
            PostMessage (THREAD_ID_HFP_AG, event);
            break;
        case SWAP_CALLS:
            event = new BtEvent;
            event->hfp_ag_event.event_id = HFP_AG_API_SWAP_CALLS_REQ;
            PostMessage (THREAD_ID_HFP_AG, event);
            break;
        case ADD_HELD_CALL_TO_CONF:
            event = new BtEvent;
            event->hfp_ag_event.event_id = HFP_AG_API_ADD_HELD_CALL_TO_CONF_REQ;
            PostMessage (THREAD_ID_HFP_AG, event);
            break;
        case DIAL:
            event = new BtEvent;
            event->hfp_ag_event.event_id = HFP_AG_API_DIAL_REQ;
            strlcpy(event->hfp_ag_event.str, user_cmd[ONE_PARAM], 20);
            PostMessage (THREAD_ID_HFP_AG, event);
            break;
        case START_VR:
            event = new BtEvent;
            event->hfp_ag_event.event_id = HFP_AG_API_START_VR_REQ;
            PostMessage (THREAD_ID_HFP_AG, event);
            break;
        case STOP_VR:
            event = new BtEvent;
            event->hfp_ag_event.event_id = HFP_AG_API_STOP_VR_REQ;
            PostMessage (THREAD_ID_HFP_AG, event);
            break;
        case QUERY_CURRENT_CALLS:
            event = new BtEvent;
            event->hfp_ag_event.event_id = HFP_AG_API_QUERY_CURRENT_CALLS_REQ;
            PostMessage (THREAD_ID_HFP_AG, event);
            break;
        case MIC_VOL_CTRL:
            event = new BtEvent;
            event->hfp_ag_event.event_id = HFP_AG_API_MIC_VOL_CTRL_REQ;
            event->hfp_ag_event.arg1 = atoi(user_cmd[ONE_PARAM]);
            PostMessage (THREAD_ID_HFP_AG, event);
            break;
        case SPK_VOL_CTRL:
            event = new BtEvent;
            event->hfp_ag_event.event_id = HFP_AG_API_SPK_VOL_CTRL_REQ;
            event->hfp_ag_event.arg1 = atoi(user_cmd[ONE_PARAM]);
            PostMessage (THREAD_ID_HFP_AG, event);
            break;
        case BACK_TO_MAIN:
            menu_type = MAIN_MENU;
            DisplayMenu(menu_type);
            break;
        case CONFIGURE_WBS:
            event = new BtEvent;
            event->hfp_ag_event.event_id = HFP_AG_CONFIGURE_WBS;
            string_to_bdaddr(user_cmd[ONE_PARAM], &event->hfp_ag_event.bd_addr);
            event->hfp_ag_event.arg1 = atoi(user_cmd[TWO_PARAM]);
            PostMessage (THREAD_ID_HFP_AG, event);
            break;
    }
}

static void HandleMainCommand(int cmd_id, char user_cmd[][COMMAND_ARG_SIZE]) {

    switch (cmd_id) {
        case GAP_OPTION:
            menu_type = GAP_MENU;
            DisplayMenu(menu_type);
            break;
        case PAN_OPTION:
            menu_type = PAN_MENU;
            DisplayMenu(menu_type);
            break;
        case TEST_MODE:
            menu_type = TEST_MENU;
            DisplayMenu(menu_type);
            break;
#ifdef USE_GEN_GATT
        case RSP_OPTION:
            menu_type = RSP_MENU;
            DisplayMenu(menu_type);
            break;
#endif
        case A2DP_SINK:
            menu_type = A2DP_SINK_MENU;
            DisplayMenu(menu_type);
            break;
        case A2DP_SOURCE:
            menu_type = A2DP_SOURCE_MENU;
            DisplayMenu(menu_type);
            break;
        case HFP_CLIENT:
            menu_type = HFP_CLIENT_MENU;
            DisplayMenu(menu_type);
            break;
#ifdef USE_GEN_GATT
        case GATTCTEST_OPTION:
            menu_type = GATTC_TEST_MENU;
            DisplayMenu(menu_type);
            break;
        case GATTSTEST_OPTION:
            menu_type = GATTSTEST_MENU;
            DisplayMenu(menu_type);
            break;
#endif
#ifdef USE_BT_OBEX
        case PBAP_CLIENT_OPTION:
            menu_type = PBAP_CLIENT_MENU;
            DisplayMenu(menu_type);
            break;
        case OPP_OPTION:
            menu_type = OPP_MENU;
            DisplayMenu(menu_type);
            break;
#endif
        case SPP_SERVER_OPTION:
            menu_type = SPP_SERVER_MENU;
            DisplayMenu(menu_type);
            break;
        case SPP_CLIENT_OPTION:
            menu_type = SPP_CLIENT_MENU;
            DisplayMenu(menu_type);
            break;
        case HFP_AG:
            menu_type = HFP_AG_MENU;
            DisplayMenu(menu_type);
            break;
        case HID_HOST:
            if(! (g_bt_app->is_hid_enabled)){
                menu_type = MAIN_MENU;
                ALOGE(LOGTAG "HID not supported. Enable it in bt_app.conf.");
                fprintf(stdout,"HID not supported. Enable it in bt_app.conf.\n");
            }
            else
                menu_type = HIDH_MENU;
            DisplayMenu(menu_type);
            break;
        case MAIN_EXIT:
            ALOGV (LOGTAG " Self exit of Main thread");
            ExitHandler();
            break;
#ifdef SUPPORT_ESL_AP
        case ESLAP_OPTION:
            menu_type = ESLAP_MENU;
            DisplayMenu(menu_type);
            break;
#endif
         default:
            ALOGV (LOGTAG " Command not handled");
            break;
    }
}

static void HandleTestCommand(int cmd_id, char user_cmd[][COMMAND_ARG_SIZE]) {

    long num = 0;
    char *end;
    int index = 0;
    switch (cmd_id) {
        case TEST_ON_OFF:
            if ((user_cmd[ONE_PARAM][0] != '\0')  && (onoff_count == 0)) {
                errno = 0;
                num = strtol(user_cmd[ONE_PARAM], &end, 0);
                if (*end != '\0' || errno != 0 || num < INT_MIN || num > INT_MAX){
                    fprintf( stdout, " Enter numeric Value\n");
                    break;
                }
                onoff_index = 1;
                onoff_count = (long) num;
                if (g_bt_app->bt_state == BT_STATE_OFF) {
                  BtEvent *event_on = new BtEvent;
                  event_on->event_id = MAIN_API_ENABLE;
                  fprintf( stdout, "Iteration: %d : Posting enable\n", onoff_index);
                  ALOGD (LOGTAG "Iteration: %d Posting enable\n",onoff_index);
                  PostMessage (THREAD_ID_MAIN, event_on);
                } else if (g_bt_app->bt_state == BT_STATE_ON) {
                  fprintf( stdout, "BT is already Enabled\n", onoff_index);
                  BtEvent *event_on = new BtEvent;
                  event_on->event_id = MAIN_API_DISABLE;
                  fprintf( stdout, "Iteration: %d : Posting Disable\n", onoff_index);
                  ALOGD (LOGTAG "Iteration: %d Posting Disable\n",onoff_index);
                  PostMessage (THREAD_ID_MAIN, event_on);
                }
                g_bt_app->is_bt_enable_test_menu_ = true;
            } else  {
                fprintf( stdout, "Test is ongoing, please wait until it finishes\n");
            }
            break;

        case BACK_TO_MAIN:
            menu_type = MAIN_MENU;
            DisplayMenu(menu_type);
            break;
        default:
            ALOGV (LOGTAG " Command not handled");
            break;
    }
}

static int send_hci_cmd_parse_args(char *args, unsigned char **cmd)
{
    int i;
    int nr_cmd;
    uint8_t *cmd_buff;
    char *p;
    unsigned long c;

    nr_cmd = strlen(args) + 1;
    if ((nr_cmd % 3))
        return -1;
    nr_cmd /= 3;
    cmd_buff = (uint8_t *)malloc(nr_cmd);
    if (NULL == cmd_buff)
        return -2;

    p = args;
    for (i = 0; i <  nr_cmd; i++) {
        if ('\0' == *args)
            break;
        c = strtol(args, &p, 16);
        if (p == args)
            break;
        if ((*p != ',') && (*p != '\0'))
            break;
        cmd_buff[i] = (uint8_t)c;
        if (*p == '\0') {
            i++;
            break;
        }
        p ++;
        args = p;
    }

    if ((i == nr_cmd) && ('\0' == *p)) {
        *cmd = cmd_buff;
        return nr_cmd;
    }

    free(cmd_buff);
    return -3;
}

#ifdef USE_GEN_GATT
static void HandleRspCommand(int cmd_id, char user_cmd[][COMMAND_ARG_SIZE]) {

    long num;
    char *end;
    int index = 0;
    switch (cmd_id) {
        case RSP_INIT:
            if ((g_bt_app->bt_state == BT_STATE_ON)) {
                fprintf( stdout, "ENABLE RSP\n");
                if (rsp) {
                   fprintf(stdout,"rsp already initialized \n");
                   return;
                } else {
                  if (g_gatt) {
                     rsp = new Rsp(g_gatt);
                     if (rsp) {
                        rsp->EnableRSP();
                        fprintf(stdout, " EnableRSP done \n");
                     }
                     else {
                        fprintf(stdout, " RSP Alloc failed return failure \n");
                     }
                  } else {
                     fprintf(stdout," gatt interface us null \n");
                  }
                }
             }
             else {
                fprintf( stdout, "BT is in OFF State now \n");
             }
            break;

        case RSP_START:
            if ((g_bt_app->bt_state == BT_STATE_ON)) {
                if (rsp) {
                    fprintf( stdout, "(Re)start Advertisement \n");
                    rsp->StartAdvertisement();
                } else {
                    fprintf(stdout , "Do Init first\n");
                }
            } else {
                fprintf( stdout, "BT is in OFF State now \n");
            }
            break;

        case BACK_TO_MAIN:
            menu_type = MAIN_MENU;
            DisplayMenu(menu_type);
            break;

        default:
            fprintf(stdout, " Command not handled\n");
            break;
    }
}
#endif
static void HandleHIDCommand(int cmd_id, char user_cmd[][COMMAND_ARG_SIZE]) {
    ALOGD(LOGTAG "HandleHIDCommand cmd_id = %d", cmd_id);
    BtEvent *event = NULL;
    switch (cmd_id) {
        case CONNECT:
            event = new BtEvent;
            event->hid_profile_event.event_id = HID_API_CONNECT_REQ;
            string_to_bdaddr(user_cmd[ONE_PARAM], &event->hid_profile_event.bd_addr);
            PostMessage (THREAD_ID_HID, event);
            break;

        case DISCONNECT:
            event = new BtEvent;
            event->hid_profile_event.event_id = HID_API_DISCONNECT_REQ;
            string_to_bdaddr(user_cmd[ONE_PARAM], &event->hid_profile_event.bd_addr);
            PostMessage (THREAD_ID_HID, event);
            break;

        case VIRTUAL_UNPLUG:
            event = new BtEvent;
            event->hid_profile_event.event_id = HID_API_VIRTUAL_UNPLUG_REQ;
            string_to_bdaddr(user_cmd[ONE_PARAM], &event->hid_profile_event.bd_addr);
            PostMessage (THREAD_ID_HID, event);
            break;

        case GET_PROTOCOL:
            event = new BtEvent;
            event->hid_profile_event.event_id = HID_API_GET_PROTOCOL_REQ;
            string_to_bdaddr(user_cmd[ONE_PARAM], &event->hid_profile_event.bd_addr);
            event->hid_profile_event.protocolMode = atoi(user_cmd[TWO_PARAM]);
            PostMessage (THREAD_ID_HID, event);
            break;

        case SET_PROTOCOL:
            event = new BtEvent;
            event->hid_profile_event.event_id = HID_API_SET_PROTOCOL_REQ;
            string_to_bdaddr(user_cmd[ONE_PARAM], &event->hid_profile_event.bd_addr);
            event->hid_profile_event.protocolMode = atoi(user_cmd[TWO_PARAM]);
            PostMessage (THREAD_ID_HID, event);
            break;

        case GET_REPORT:
            event = new BtEvent;
            event->hid_profile_event.event_id = HID_API_GET_REPORT_REQ;
            string_to_bdaddr(user_cmd[ONE_PARAM], &event->hid_profile_event.bd_addr);
            event->hid_profile_event.reportType= atoi(user_cmd[TWO_PARAM]);
            event->hid_profile_event.reportID  = atoi(user_cmd[THREE_PARAM]);
            event->hid_profile_event.bufSize   = atoi(user_cmd[FOUR_PARAM]);
            PostMessage (THREAD_ID_HID, event);
            break;

        case SET_REPORT:
            event = new BtEvent;
            event->hid_profile_event.event_id = HID_API_SET_REPORT_REQ;
            string_to_bdaddr(user_cmd[ONE_PARAM], &event->hid_profile_event.bd_addr);
            event->hid_profile_event.reportType= atoi(user_cmd[TWO_PARAM]);
            event->hid_profile_event.bufSize   = atoi(user_cmd[FOUR_PARAM]);
            strlcpy(event->hid_profile_event.report , user_cmd[THREE_PARAM], 20);
            PostMessage (THREAD_ID_HID, event);
            break;

        case CFG_MTU:
            event = new BtEvent;
            event->hogp_cfg_mtu_event.event_id = HID_API_CONFIGURE_MTU_EVENT;
            string_to_bdaddr(user_cmd[ONE_PARAM], &event->hogp_cfg_mtu_event.bd_addr);
            event->hogp_cfg_mtu_event.mtu= atoi(user_cmd[TWO_PARAM]);
            PostMessage (THREAD_ID_HID, event);
            break;

        case CONN_PARAMS:
            event = new BtEvent;
            event->hogp_conn_params_event.event_id = HID_API_CONN_UPDATED_EVENT;
            string_to_bdaddr(user_cmd[ONE_PARAM], &event->hogp_conn_params_event.bd_addr);
            event->hogp_conn_params_event.min_int = atoi(user_cmd[TWO_PARAM]);
            event->hogp_conn_params_event.max_int = atoi(user_cmd[THREE_PARAM]);
            event->hogp_conn_params_event.latency = atoi(user_cmd[FOUR_PARAM]);
            event->hogp_conn_params_event.timeout = atoi(user_cmd[FIVE_PARAM]);
            PostMessage (THREAD_ID_HID, event);
            break;

        case HID_BONDED_LIST:
            event = new BtEvent;
            event->hid_profile_event.event_id = HID_API_BONDED_LIST_REQ;
            PostMessage (THREAD_ID_HID, event);
            break;

        case BACK_TO_MAIN:
            menu_type = MAIN_MENU;
            DisplayMenu(menu_type);
            break;
    }
}

#ifdef USE_GEN_GATT
static void HandleGattcTestCommand(int cmd_id, char user_cmd[][COMMAND_ARG_SIZE]) {

    long num;
    char *end;
    int index = 0;
    switch (cmd_id) {
        case GATTCTEST_INIT:
            fprintf( stdout, "ENABLE GATTCTEST \n");
            if ((g_bt_app->bt_state == BT_STATE_ON)) {
            if (gattctest) {
                   fprintf(stdout,"gattctest already initialized \n");
                   return;
                } else {
                    fprintf(stdout,"gattctest not initialized \n");
                  if (g_gatt) {

                     gattctest = new GattcTest(g_gatt);

                     if (gattctest) {
                        gattctest->enableGattctest();
                        fprintf(stdout, " EnableGATTCTEST done \n");
                     }
                     else {
                        fprintf(stdout, " GATTCTEST Alloc failed return failure \n");
                     }
                  } else {
                     fprintf(stdout," gatt interface us null \n");
                  }
                }
             }
             else {
                fprintf( stdout, "BT is in OFF State now \n");
             }
            break;
         case GATTCTEST_SCAN_FILTER:
            fprintf(stdout,"Scan filter \n");
            if (gattctest) {
                bool status = gattctest->validateInput(user_cmd[ONE_PARAM]);
                if (!status) {
                    fprintf(stdout, "Enter proper ScanFilter Type\n");
                    break;
                }
                fprintf(stdout,"Do scan filtering\n");
                gattctest->scanFilter(atoi(user_cmd[ONE_PARAM]),
                  user_cmd[TWO_PARAM]);
            } else {
                fprintf(stdout,"Do the GATTCINIT first\n");
            }
            break;
         case GATTCTEST_SCANFILTER_MAN_DATA:
            fprintf(stdout,"Scan filter Manu Data\n");
            if (gattctest) {
                bool status = gattctest->validateInput(user_cmd[ONE_PARAM]);
                if (!status) {
                    fprintf(stdout, "Enter proper Filter Type\n");
                    break;
                }
                fprintf(stdout,"Do scan filtering\n");
                gattctest->scanFilterManuData(atoi(user_cmd[ONE_PARAM]),
                    user_cmd[TWO_PARAM], user_cmd[THREE_PARAM]);
            } else {
                fprintf(stdout,"Do the GATTCINIT first\n");
            }
            break;
         case GATTCTEST_SCAN_SETTINGS:
            fprintf(stdout,"Scan settings \n");
            if (gattctest) {
                bool status = gattctest->validateInput(user_cmd[ONE_PARAM]);
                if (!status) {
                    fprintf(stdout, "Enter proper scanSetting Type\n");
                    break;
                }
                status = gattctest->validateInput(user_cmd[TWO_PARAM]);
                if (!status) {
                    fprintf(stdout, "Enter proper setting value \n");
                    break;
                }
                fprintf(stdout,"Do scan settings\n");
                gattctest->scanSettings(atoi(user_cmd[ONE_PARAM]),
                    atoi(user_cmd[TWO_PARAM]));
            } else {
                fprintf(stdout,"Do the GATTCINIT first\n");
            }
            break;
         case GATTCTEST_CONN_PARAMS:
            fprintf(stdout,"Connection parameters \n");
            if (gattctest) {
                bool status = gattctest->validateInput(user_cmd[ONE_PARAM]);
                if (!status) {
                fprintf(stdout, "Enter proper auto value\n");
                break;
                }
                status = gattctest->validateInput(user_cmd[TWO_PARAM]);
                if (!status) {
                    fprintf(stdout, "Enter proper phy value\n");
                    break;
                }
                status = gattctest->validateInput(user_cmd[THREE_PARAM]);
                if (!status) {
                    fprintf(stdout, "Enter proper oppurtunistic value\n");
                    break;
                }
                if ((atoi(user_cmd[ONE_PARAM]) != 0) &&
                    (atoi(user_cmd[ONE_PARAM]) != 1)) {
                    ALOGW(LOGTAG "Enter correct auto value (0/1)");
                    fprintf(stdout, "Enter correct auto value (0/1)\n");
                } else if ((atoi(user_cmd[THREE_PARAM]) != 0 ) &&
                    (atoi(user_cmd[THREE_PARAM]) != 1)) {
                    ALOGW(LOGTAG "Enter correct oppurtunistic value (0/1)");
                    fprintf(stdout, "Enter correct oppurtunistic value (0/1)\n");
                } else {
                gattctest->gattConnParams((bool)(atoi(user_cmd[ONE_PARAM])),
                  atoi(user_cmd[TWO_PARAM]),
                  (bool)atoi(user_cmd[THREE_PARAM]));
                }
            } else {
              fprintf(stdout,"Do the GATTCINIT first\n");
            }
            break;
         case GATTCTEST_START_SCAN:
            fprintf(stdout,"trying to start scan \n");
            if (gattctest) {
                fprintf(stdout,"starting scan \n");
                gattctest->startScan();
            } else {
                fprintf(stdout,"Do the GATTCINIT first\n");
            }
            break;
         case GATTCTEST_BATCH_SCAN:
            fprintf(stdout,"trying to start batch scan \n");
            if (gattctest) {
                bool valid = gattctest->validateInput(user_cmd[ONE_PARAM]);
                if (!valid) {
                   fprintf(stdout, "Enter proper auto value\n");
                   break;
                }
                switch (atoi(user_cmd[ONE_PARAM])){
                    case 0:
                    case 1:
                        fprintf(stdout,"starting batch scan \n");
                        gattctest->testBatchscan(atoi(user_cmd[ONE_PARAM]));
                        break;
                    default:
                        fprintf( stdout, "Enter proper parameter \n");
                        fprintf( stdout, "0-FULL MODE 1- TRUNCATED MODE \n");
                        break;
                }
            } else {
                fprintf(stdout,"Do the GATTCINIT first\n");
            }
            break;
        case GATTCTEST_STOP_SCAN:
           if (gattctest) {
               fprintf(stdout,"stopping scan \n");
               gattctest->stopScan();
           } else {
               fprintf(stdout,"Do the GATTCINIT first\n");
           }
           break;

        case GATTCTEST_CONNECT:
           if (string_is_bdaddr(user_cmd[ONE_PARAM])) {
               bool status = gattctest->validateInput(user_cmd[TWO_PARAM]);
               if (!status) {
                   fprintf(stdout, "Enter proper Transport Value\n");
                   break;
              }
              if (gattctest) {
                  fprintf(stdout,"connecting \n");
                  gattctest->gattConnect(user_cmd[ONE_PARAM],
                    atoi(user_cmd[TWO_PARAM]));
              } else {
                  fprintf(stdout,"Do the GATTCINIT first\n");
              }
           } else {
                fprintf( stdout, " BD address is NULL/Invalid \n");
           }
            break;

        case GATTCTEST_DISCONNECT:
            if (string_is_bdaddr(user_cmd[ONE_PARAM])) {
               if (gattctest) {
                   fprintf(stdout,"disconnecting \n");
                   gattctest->gattDisconnect(user_cmd[ONE_PARAM]);
               } else {
                    fprintf(stdout,"Do the GATTCINIT first\n");
               }
            } else {
                fprintf( stdout, " BD address is NULL/Invalid \n");
            }
            break;
        case GATTCTEST_DISCSRVC:
            if (string_is_bdaddr(user_cmd[ONE_PARAM])) {
               if (gattctest) {
                   fprintf(stdout,"Discovering services \n");
                   gattctest->gattDiscoverServices(user_cmd[ONE_PARAM]);
               } else {
                    fprintf(stdout,"Do the GATTCINIT first\n");
               }
            } else {
                fprintf( stdout, " BD address is NULL/Invalid \n");
            }
            break;
        case GATTCTEST_DISCSRVC_UUID:
            if (string_is_bdaddr(user_cmd[ONE_PARAM])) {
               if (gattctest) {
                   fprintf(stdout,"Discovering services by uuid\n");
                   Uuid uuid = uuid.FromString((user_cmd[TWO_PARAM]), NULL);
                   gattctest->gattDiscoverServicesByUuid
                       (uuid, user_cmd[ONE_PARAM]);
               } else {
                    fprintf(stdout,"Do the GATTCINIT first\n");
               }
            } else {
                fprintf( stdout, " BD address is NULL/Invalid \n");
            }
            break;
         case GATTCTEST_RDCHAR_UUID:
            if (string_is_bdaddr(user_cmd[ONE_PARAM])) {
               if (gattctest) {
                   fprintf(stdout,"reading char by uuid\n");
                   Uuid uuid = uuid.FromString((user_cmd[TWO_PARAM]), NULL);
                   gattctest->readCharacteristicUUID(user_cmd[ONE_PARAM], uuid);
               } else {
                    fprintf(stdout,"Do the GATTCINIT first\n");
               }
            } else {
                fprintf( stdout, " BD address is NULL/Invalid \n");
            }
            break;
        case GATTCTEST_READPHY:
            if (string_is_bdaddr(user_cmd[ONE_PARAM])) {
               if (gattctest) {
                   fprintf(stdout,"Reading PHY \n");
                   gattctest->gattClientReadPhy(user_cmd[ONE_PARAM]);
               } else {
                    fprintf(stdout,"Do the GATTCINIT first\n");
               }
            } else {
                fprintf( stdout, " BD address is NULL/Invalid \n");
            }
            break;
        case GATTCTEST_READRSSI:
            if (string_is_bdaddr(user_cmd[ONE_PARAM])) {
               if (gattctest) {
                   fprintf(stdout,"Reading RSSI \n");
                   gattctest->gattReadRemoteRssi(user_cmd[ONE_PARAM]);
               } else {
                    fprintf(stdout,"Do the GATTCINIT first\n");
               }
            } else {
                fprintf( stdout, " BD address is NULL/Invalid \n");
            }
            break;
        case GATTCTEST_REFRESH:
            if (string_is_bdaddr(user_cmd[ONE_PARAM])) {
               if (gattctest) {
                   fprintf(stdout,"REFRESHING \n");
                   gattctest->gattRefresh(user_cmd[ONE_PARAM]);
               } else {
                    fprintf(stdout,"Do the GATTCINIT first\n");
               }
            } else {
                fprintf( stdout, " BD address is NULL/Invalid \n");
            }
            break;
        case GATTCTEST_REQMTU:
            if (string_is_bdaddr(user_cmd[ONE_PARAM])) {
                if (gattctest) {
                    bool status = gattctest->validateInput
                        (user_cmd[TWO_PARAM]);
                    if (!status) {
                        fprintf(stdout, "Enter proper MTU Value\n");
                        break;
                    }
                    fprintf(stdout,"REQUESTING MTU \n");
                    gattctest->gattrequestMtu(user_cmd[ONE_PARAM],
                    atoi(user_cmd[TWO_PARAM]));
              } else {
                   fprintf(stdout,"Do the GATTCINIT first\n");
              }
            } else {
                fprintf( stdout, " BD address is NULL/Invalid \n");
            }
            break;
        case GATTCTEST_SETPHY:
            if (string_is_bdaddr(user_cmd[FOUR_PARAM])) {
                if (gattctest) {
                    fprintf(stdout,"Setting PHY \n");
                    bool status = gattctest->validateInput(user_cmd[ONE_PARAM]);
                    if (!status) {
                        fprintf(stdout, "Enter proper TX Value\n");
                        break;
                    }
                    status = gattctest->validateInput(user_cmd[TWO_PARAM]);
                    if (!status) {
                        fprintf(stdout, "Enter proper RX Value\n");
                        break;
                    }
                    status = gattctest->validateInput(user_cmd[THREE_PARAM]);
                    if (!status) {
                        fprintf(stdout, "Enter proper Phy Options\n");
                        break;
                    }
                    gattctest->setPreferredPhy(atoi(user_cmd[ONE_PARAM]),
                        atoi(user_cmd[TWO_PARAM]), atoi(user_cmd[THREE_PARAM]), user_cmd[FOUR_PARAM]);
               } else {
                   fprintf(stdout,"Do the GATTCINIT first\n");
               }
            } else {
                fprintf( stdout, " BD address is NULL/Invalid \n");
            }
            break;
        case GATTCTEST_GETSERVICES:
            if (string_is_bdaddr(user_cmd[ONE_PARAM])) {
               if (gattctest) {
                   fprintf(stdout,"getting services \n");
                   gattctest->getServices(user_cmd[ONE_PARAM]);
               } else {
                    fprintf(stdout,"Do the GATTCINIT first\n");
               }
            } else {
                fprintf( stdout, " BD address is NULL/Invalid \n");
            }
            break;
        case GATTCTEST_GETSRVC:
        {
            if (string_is_bdaddr(user_cmd[ONE_PARAM])) {
                Uuid uuid = uuid.FromString((user_cmd[TWO_PARAM]), NULL);
                bool status = gattctest->validateInput(user_cmd[THREE_PARAM]);
                if (!status) {
                    fprintf(stdout, "Enter proper instanceID\n");
                    break;
                }
                if (gattctest) {
                    fprintf(stdout,"getting service  \n");
                    gattctest->getService(user_cmd[ONE_PARAM], uuid,
                        atoi(user_cmd[THREE_PARAM]));
                } else {
                    fprintf(stdout,"Do the GATTCINIT first\n");
               }
            } else {
                fprintf( stdout, " BD address is NULL/Invalid \n");
            }
            break;
        }
        case GATTCTEST_RDWRDESC:
        {
            if (string_is_bdaddr(user_cmd[ONE_PARAM])) {
               if (gattctest) {
                   fprintf(stdout,"Reading writing DESC\n");
                   bool status = gattctest->validateInput
                        (user_cmd[TWO_PARAM]);
                   if (!status) {
                       fprintf(stdout, "Enter proper Read/Write Type\n");
                       break;
                   }
                   status = gattctest->validateInput(user_cmd[FOUR_PARAM]);
                   if (!status) {
                       fprintf(stdout, "Enter instanceId\n");
                       break;
                   }
                   int i = atoi(user_cmd[TWO_PARAM]);
                   int j = atoi(user_cmd[FOUR_PARAM]);
                   fprintf(stdout, "instanceid %d\n", j);
                   if (i == 1) {
                       gattctest->writeDescriptor(user_cmd[ONE_PARAM],
                         (uint8_t *)&(user_cmd[THREE_PARAM]),
                         atoi(user_cmd[FIVE_PARAM]), j);
                   } else if (i == 2) {
                       gattctest->readDescriptor(user_cmd[ONE_PARAM],
                         atoi(user_cmd[FOUR_PARAM]));
                   } else {
                       fprintf(stdout, "Enter the correct 2nd parameter.."
                         "1 -write , 2 -read\n");
                   }
               } else {
                    fprintf(stdout,"Do the GATTCINIT first\n");
               }
            } else {
                fprintf( stdout, " BD address is NULL/Invalid \n");
            }
            break;
        }
        case GATTCTEST_REGISTER_NOTIFICATIONS:
        {
            if (string_is_bdaddr(user_cmd[ONE_PARAM])) {
                if (gattctest) {
                    fprintf(stdout,"Enable disable notifications\n");
                    gattctest->registerNotifications(user_cmd[ONE_PARAM],
                        atoi(user_cmd[TWO_PARAM]),
                        atoi(user_cmd[THREE_PARAM]),atoi(user_cmd[FOUR_PARAM]));
                } else {
                    fprintf(stdout,"Do the GATTCINIT first\n");
                }
            } else {
                fprintf( stdout, " BD address is NULL/Invalid \n");
            }
            break;
        }
        case GATTCTEST_RDWRCHAR:
        {
            if (string_is_bdaddr(user_cmd[ONE_PARAM])) {
               if (gattctest) {
                   fprintf(stdout,"Reading writing char\n");
                bool status = gattctest->validateInput(user_cmd[TWO_PARAM]);
                if (!status) {
                  fprintf(stdout, "Enter proper Read/Write Type\n");
                  break;
                }
                status = gattctest->validateInput(user_cmd[FOUR_PARAM]);
                if (!status) {
                  fprintf(stdout, "Enter instanceId\n");
                  break;
                }
                   int i = atoi(user_cmd[TWO_PARAM]);
                   int j = atoi(user_cmd[FOUR_PARAM]);
                   fprintf(stdout, "instanceid %d\n", j);
                   if (i == 1) {
                       gattctest->writeCharacteristic(user_cmd[ONE_PARAM],
                         (uint8_t *)&(user_cmd[THREE_PARAM]),
                           atoi(user_cmd[FIVE_PARAM]), j);
                   } else if (i == 2) {
                       gattctest->readCharacteristic(user_cmd[ONE_PARAM],
                         atoi(user_cmd[FOUR_PARAM]));
                   } else if (i == 3) {
                       gattctest->prepareWriteCharacteristic(user_cmd[ONE_PARAM],
                           (uint8_t *)&(user_cmd[THREE_PARAM]),
                           atoi(user_cmd[FIVE_PARAM]), j);
                   }else {
                       fprintf(stdout, "Enter the correct 2nd parameter.."
                         "1 -write , 2 -read\n");
                   }
               } else {
                    fprintf(stdout,"Do the GATTCINIT first\n");
               }
            } else {
                fprintf( stdout, " BD address is NULL/Invalid \n");
            }
            break;
        }
        case GATTCTEST_GETCHARID:
            if (string_is_bdaddr(user_cmd[ONE_PARAM])) {
                if (gattctest) {
                    fprintf(stdout,"getting Characteristic \n");
                    bool status = gattctest->validateInput
                        (user_cmd[TWO_PARAM]);
                if (!status) {
                    fprintf(stdout, "Enter proper InstanceID\n");
                    break;
                }
                gattctest->getCharacteristicById(user_cmd[ONE_PARAM],
                    atoi(user_cmd[TWO_PARAM]));
               } else {
                    fprintf(stdout,"Do the GATTCINIT first\n");
               }
            } else {
                fprintf( stdout, "BD address is NULL/Invalid \n");
            }
            break;
        case GATTCTEST_RELIABLEWRITE:
            if (string_is_bdaddr(user_cmd[ONE_PARAM])) {
                if (gattctest) {
                    bool status = gattctest->validateInput
                        (user_cmd[TWO_PARAM]);
                    if (!status) {
                        fprintf(stdout, "Enter proper InstanceID\n");
                        break;
                    }
                    fprintf(stdout,"Reliablewrite Characteristic \n");
                    gattctest->reliableWrite(user_cmd[ONE_PARAM],
                        atoi(user_cmd[TWO_PARAM]));
               } else {
                    fprintf(stdout,"Do the GATTCINIT first\n");
               }
            } else {
                fprintf( stdout, "BD address is NULL/Invalid \n");
            }
            break;
        case GATTCTEST_GETDESCID:
            if (string_is_bdaddr(user_cmd[ONE_PARAM])) {
                if (gattctest) {
                    fprintf(stdout,"getting Descriptor \n");
                    bool status = gattctest->validateInput
                        (user_cmd[TWO_PARAM]);
                    if (!status) {
                        fprintf(stdout, "Enter proper InstanceID\n");
                        break;
                    }
                    gattctest->getDescriptorById(user_cmd[ONE_PARAM],
                    atoi(user_cmd[TWO_PARAM]));
                } else {
                    fprintf(stdout,"Do the GATTCINIT first\n");
                }
            } else {
                fprintf( stdout, "BD address is NULL/Invalid \n");
            }
            break;
        case GATTCTEST_REQCONN_PRI:
            if (string_is_bdaddr(user_cmd[ONE_PARAM])) {
                if (gattctest) {
                    fprintf(stdout,"Requesting Connection Priority \n");
                    bool status = gattctest->validateInput
                        (user_cmd[TWO_PARAM]);
                    if (!status) {
                        fprintf(stdout, "Enter Connection Priority\n");
                        break;
                    }
                    int i = atoi(user_cmd[TWO_PARAM]);
                    if (i >= 0 && i <= 2) {
                        gattctest->reqConnPri(user_cmd[ONE_PARAM],
                            atoi(user_cmd[TWO_PARAM]));
                    } else {
                        fprintf(stdout, "Enter 0/1/2 as priority\n");
                    }
                } else {
                    fprintf(stdout,"Do the GATTCINIT first\n");
                }
            } else {
                fprintf( stdout, "BD address is NULL/Invalid \n");
            }
            break;
        case GATTCTEST_CONN_DEVICES:
            if (gattctest) {
                fprintf(stdout,"Listing Connected devices \n");
                gattctest->list_conn_devices();
           } else {
                fprintf(stdout,"Do the GATTCINIT first\n");
           }
           break;
        case BACK_TO_MAIN:
            menu_type = MAIN_MENU;
            DisplayMenu(menu_type);
            break;

        default:
            fprintf(stdout, " Command not handled");
            break;
    }
}


static void HandleGattsTestCommand(int cmd_id, char user_cmd[][COMMAND_ARG_SIZE])
{
    bool advEnable = true;
    int  advDuration = 20000;
    int  advMaxEvents = 0;
    bool isConnected=0;
    static bool init_server_file=0;
    int  server_inst = 0;
    int  service_inst = 0;
    switch (cmd_id) {
        case GATTSTEST_INIT_SERVER:
            if ((g_bt_app->bt_state == BT_STATE_ON)) {
                fprintf( stdout, "ENABLE GATTSTEST\n");
                if (gattstest) {
                    fprintf(stdout,"Gattstest already initialized \n");
                    return;
                } else {
                    if (g_gatt) {
                        fprintf(stdout,"Initializing Gattstest \n");
                        gattstest = new GattsTest(g_gatt);
                        if (gattstest) {
                            fprintf(stdout,"Reading Server Configuration File .... \n");
                            gattstest->ReadServerConfigurationFile();
                            init_server_file = true;
                        } else {
                            fprintf(stdout, " GATTSTEST Alloc failed return failure \n");
                        }
                    } else {
                        fprintf(stdout," gatt interface is null \n");
                    }
                }
             } else {
                fprintf( stdout, "BT is in OFF State now \n");
             }
             break;
        case GATTSTEST_ADDSERVER:
            if ((g_bt_app->bt_state == BT_STATE_ON)) {
                if(g_gatt) {
                    if (gattstest) {
                        if(init_server_file)  {
                            server_num++;
                            gattstest->AddServer();
                        } else {
                            fprintf(stdout,"Do gattstest_init_server first \n");
                        }
                    } else {
                            fprintf(stdout , "Do Init first\n");
                    }
                } else {
                    fprintf(stdout,"gatt interface is null \n");
                }
            } else {
                fprintf(stdout, "BT is in OFF State now \n");
            }
            break;
        case GATTSTEST_ADDSERVICES:
            if ((g_bt_app->bt_state == BT_STATE_ON)) {
                if (gattstest) {
                    fprintf(stdout,"AddServices \n");
                    string server_instance = user_cmd[ONE_PARAM];
                    string service_instance = user_cmd[TWO_PARAM];
                    bool result = gattstest->AddService(server_instance,service_instance);
                    if(!result) {
                        fprintf(stdout,"Service could not be added\n");
                    }
                } else {
                    fprintf(stdout , "Do Init first\n");
                }
            } else {
                fprintf( stdout, "BT is in OFF State now \n");
            }
            break;
        case GATTSTEST_INIT_ADVERTISER:
            if ((g_bt_app->bt_state == BT_STATE_ON)) {
                if (gattstest) {
                    fprintf(stdout,"Initialize Advertiser \n");
                    file_read = gattstest->ReadAdvertiserConfigFile();
                    init_advertiser_file = true;
                    if(file_read)
                        fprintf(stdout,"File read Succcessfully \n");
                    else
                        fprintf(stdout,"File not read \n");
                } else {
                    fprintf(stdout , "Do Init first\n");
                }
            } else {
                fprintf( stdout, "BT is in OFF State now \n");
            }
            break;
        case GATTSTEST_START_ADVERTISER:
            if ((g_bt_app->bt_state == BT_STATE_ON)) {
                if (gattstest) {
                    fprintf(stdout,"StartAdvertisement \n");
                    string server_instance = user_cmd[ONE_PARAM];
                    if(file_read && (init_advertiser_file == true)) {
                        bool result =gattstest->StartAdvertisement(server_instance);
                        if(!result){
                            fprintf(stdout,"Advertisement has not started\n");
                        }
                    } else {
                        fprintf(stdout,"Do init Advertiser first \n");
                    }
                } else {
                    fprintf(stdout , "Do Init first\n");
                }
             } else {
                fprintf( stdout, "BT is in OFF State now \n");
             }
             break;
        case GATTSTEST_READPHY:
            fprintf(stdout,"Read Phy \n");
            if(string_is_bdaddr(user_cmd[ONE_PARAM])) {
                if (gattstest) {
                    ALOGE(LOGTAG"gattstest->ReadPhy");
                    fprintf(stdout,"User input is %s \n",user_cmd[ONE_PARAM]);
                    string server_instance = user_cmd[TWO_PARAM];
                    bool status =gattstest->ReadPhy(server_instance,user_cmd[ONE_PARAM]);
                    if(!status)
                    {
                        fprintf(stdout,"tx/rx phy could not be read \n");
                    }
                } else {
                    fprintf( stdout, "Do Init first \n ");
                }
            } else {
                fprintf(stdout,"BD address is NULL/Invalid \n");
            }
            break;
        case GATTSTEST_SET_PREFERRED_PHY:
            fprintf(stdout,"Set Preferred Phy \n");
            if (string_is_bdaddr(user_cmd[ONE_PARAM])) {
                if (gattstest) {
                    string deviceAddress = user_cmd[ONE_PARAM];
                    string server_instance = user_cmd[TWO_PARAM];
                    string txOption = user_cmd[THREE_PARAM];
                    string rxOption = user_cmd[FOUR_PARAM];
                    int phyOption =  atoi(user_cmd[FIVE_PARAM]);
                    fprintf(stdout,"the user options are address: %s server_instance: %s txoption: %s rxoption: %s phyoption: %d \n",deviceAddress.c_str(),server_instance.c_str(),txOption.c_str(),rxOption.c_str(),phyOption);
                    bool status =gattstest->SetPreferredPhy(deviceAddress,server_instance,txOption,rxOption,phyOption);
                    if(!status) {
                        fprintf(stdout,"Phy preferences were not set \n");
                    }
                } else {
                    fprintf( stdout, "Do Init first \n ");
                }
            } else {
                fprintf(stdout,"BD address is NULL/Invalid \n");
            }
            break;
        case GATTSTEST_STOP:
            if(gattstest) {
                fprintf(stdout, "Stop Advertisement \n");
                string server_instance = user_cmd[ONE_PARAM];
                gattstest->StopAdvertisement(server_instance);
            } else {
                fprintf( stdout, "Do Init first \n ");
            }
            break;
        case GATTSTEST_UNREGISTER_SERVER:
            if((g_bt_app->bt_state == BT_STATE_ON)){
                fprintf( stdout, "Unregister Server \n");
                if (gattstest) {
                    bool status = gattstest->UnregisterServer(user_cmd[ONE_PARAM]);
                    server_num --;
                    if(status)
                    {
                        fprintf(stdout,"Server unregistered succesfully \n");
                    } else {
                         fprintf(stdout,"Server not unregistered\n");
                    }
                } else {
                    fprintf( stdout, "Do Init first \n ");
                }
             } else {
                fprintf( stdout, "BT is in OFF State now \n");
             }
             break;
        case GATTSTEST_DISABLE:
            if((g_bt_app->bt_state == BT_STATE_ON)){
                fprintf( stdout, "Disable Gattstest \n");
                if (gattstest) {
                    gattstest->DisableGATTSTEST();
                    delete gattstest;
                    gattstest = NULL;
                    server_num = 0;
                    file_read = 0;
                    init_advertiser_file = false;
                } else {
                    fprintf( stdout, "Do Init first \n ");
                }
             } else {
                fprintf( stdout, "BT is in OFF State now \n");
             }
             break;
        case GATTSTEST_CANCEL_CONNECTION:
            if((g_bt_app->bt_state == BT_STATE_ON)) {
                fprintf( stdout, "Cancel Connection \n");
                if (string_is_bdaddr(user_cmd[ONE_PARAM])) {
                    if (gattstest) {
                        string deviceAddress = user_cmd[ONE_PARAM];
                        gattstest->CancelConnection(deviceAddress);
                    } else {
                        fprintf( stdout, "Do Init first \n ");
                    }
                } else {
                    fprintf(stdout,"BD address is NULL/Invalid \n");
                }
            } else {
                fprintf( stdout, "BT is in OFF State now \n");
            }
            break;
        case BACK_TO_MAIN:
            menu_type = MAIN_MENU;
            DisplayMenu(menu_type);
            break;

        default:
            fprintf(stdout, " Command not handled\n");
            break;
    }
}

#endif

static void SendEnableCmdToGap() {

    if ((g_bt_app->status.enable_cmd != COMMAND_INPROGRESS) &&
        (g_bt_app->status.disable_cmd != COMMAND_INPROGRESS) &&
        (g_bt_app->bt_state == BT_STATE_OFF)) {

        g_bt_app->status.enable_cmd = COMMAND_INPROGRESS;
        // Killing previous iteration filter if they still exists
        //system("killall -KILL wcnssfilter");
        //system("killall -KILL btsnoop");
        //system("killall -KILL qcbtdaemon");
        usleep(200);

        BtEvent *event = new BtEvent;
        event->event_id = GAP_API_ENABLE;
        ALOGV (LOGTAG " Posting BT enable to GAP thread");
        PostMessage (THREAD_ID_GAP, event);
    } else if ( g_bt_app->status.enable_cmd == COMMAND_INPROGRESS ) {
        fprintf( stdout, "BT enable is already in process\n");
    } else if ( g_bt_app->status.disable_cmd == COMMAND_INPROGRESS ) {
        fprintf( stdout, "Previous BT disable is still in progress\n");
    } else {
        fprintf( stdout, "Currently BT is already ON\n");
    }
}

static void SendDisableCmdToGap() {

    if ((g_bt_app->status.disable_cmd != COMMAND_INPROGRESS) &&
        (g_bt_app->status.enable_cmd != COMMAND_INPROGRESS) &&
        (g_bt_app->bt_state == BT_STATE_ON)) {

        g_bt_app->status.disable_cmd = COMMAND_INPROGRESS;

        BtEvent *event = new BtEvent;
        event->event_id = GAP_API_DISABLE;
        ALOGV (LOGTAG " Posting disable to GAP thread");
        PostMessage (THREAD_ID_GAP, event);
    } else if (g_bt_app->status.disable_cmd == COMMAND_INPROGRESS) {
        fprintf( stdout, " disable command is already in process\n");
    } else if (g_bt_app->status.enable_cmd == COMMAND_INPROGRESS) {
        fprintf( stdout, " Previous enable command is still in process\n");
    } else {
        fprintf( stdout, "Currently BT is already OFF\n");
    }
}

#ifdef SUPPORT_ESL_AP
//for AP post "AP exception to BT main thread", then deinit AP
void PostMessageToBtMainThread(void *msg) {
    PostMessage(THREAD_ID_MAIN, msg);
}

void HandleAPInitCmd(void) {

    if ((g_bt_app->status.eslap_init_cmd != COMMAND_INPROGRESS) &&
        (g_bt_app->status.eslap_deinit_cmd != COMMAND_INPROGRESS) &&
        (g_bt_app->bt_state == BT_STATE_ON) &&
        (g_bt_app->ap_state == AP_STATE_OFF)) {

        g_bt_app->status.eslap_init_cmd = COMMAND_INPROGRESS;
        if (g_bt_app->ap_interface) {
            if (g_bt_app->ap_interface->init(g_bt_app->bt_interface, PostMessageToBtMainThread) == 0) {
                g_bt_app->status.eslap_init_cmd = COMMAND_COMPLETE;
                g_bt_app->ap_state = AP_STATE_ON;
            } else {
                g_bt_app->status.eslap_init_cmd = COMMAND_COMPLETE;
                g_bt_app->ap_state = AP_STATE_OFF;
                fprintf( stdout, "AP init failed\n");
            }
        }

    } else if ( g_bt_app->status.eslap_init_cmd == COMMAND_INPROGRESS ) {
        fprintf( stdout, "AP init is already in process\n");
    } else if ( g_bt_app->status.eslap_deinit_cmd == COMMAND_INPROGRESS ) {
        fprintf( stdout, "Previous ap init is still in progress\n");
    } else if ( g_bt_app->bt_state != BT_STATE_ON ) {
        fprintf( stdout, "Currently BT is not ON, enable BT first\n");
    } else {
        fprintf( stdout, "Currently AP is already ON\n");
    }
}

void HandleAPDeinitCmd(void) {

    if ((g_bt_app->status.eslap_deinit_cmd != COMMAND_INPROGRESS) &&
        (g_bt_app->status.eslap_init_cmd != COMMAND_INPROGRESS) &&
        (g_bt_app->ap_state == AP_STATE_ON)) {

        g_bt_app->status.eslap_deinit_cmd = COMMAND_INPROGRESS;
        if (g_bt_app->ap_interface) {
            if (g_bt_app->ap_interface->deInit() == 0) {
                g_bt_app->status.eslap_deinit_cmd = COMMAND_COMPLETE;
                g_bt_app->ap_state = AP_STATE_OFF;
            } else {
                g_bt_app->status.eslap_deinit_cmd = COMMAND_COMPLETE;
                g_bt_app->ap_state = AP_STATE_ON; //state????
                fprintf( stdout, "AP deinit failed\n");
            }
        }
    } else if (g_bt_app->status.eslap_deinit_cmd == COMMAND_INPROGRESS) {
        fprintf( stdout, " deinit AP command is already in process\n");
    } else if (g_bt_app->status.eslap_init_cmd == COMMAND_INPROGRESS) {
        fprintf( stdout, " Previous AP init command is still in process\n");
    } else {
        fprintf( stdout, "Currently AP is already OFF\n");
    }
}

void HandleAPCertCmd(char user_cmd[][COMMAND_ARG_SIZE]) {
    if (g_bt_app->ap_state != AP_STATE_ON) {
        fprintf( stdout, "please init AP first.\n");
        return;
    }
    g_bt_app->ap_interface->cert(cert_cmd_parameter_count - CERT_CMD_PARAMETER_COUNT_MIN, &user_cmd[ONE_PARAM]);
}

static void HandleEslapCommand(int cmd_id, char user_cmd[][COMMAND_ARG_SIZE]) {
    BtEvent *event = NULL;

    switch (cmd_id) {
        case BACK_TO_MAIN:
            menu_type = MAIN_MENU;
            DisplayMenu(menu_type);
            break;
        case AP_INIT:
            HandleAPInitCmd();
            break;
        case AP_DEINIT:
            HandleAPDeinitCmd();
            break;
        case AP_CERT:
            HandleAPCertCmd(user_cmd);
            break;
        default:
            ALOGV (LOGTAG " Command not handled");
            break;
    }
}
#endif

static void HandleGapCommand(int cmd_id, char user_cmd[][COMMAND_ARG_SIZE]) {
    BtEvent *event = NULL;

    switch (cmd_id) {
        case BACK_TO_MAIN:
            menu_type = MAIN_MENU;
            DisplayMenu(menu_type);
            break;

        case BT_ENABLE:
            SendEnableCmdToGap();
            break;

        case BT_DISABLE:
#ifdef SUPPORT_ESL_AP
            HandleAPDeinitCmd();
#endif
            SendDisableCmdToGap();

            break;

        case START_ENQUIRY:

            if ((g_bt_app->status.enquiry_cmd != COMMAND_INPROGRESS) &&
                                (g_bt_app->bt_state == BT_STATE_ON)) {

                g_bt_app->inquiry_list.clear();
                g_bt_app->inq_db_count = 0;
                g_bt_app->status.enquiry_cmd = COMMAND_INPROGRESS;
                event = new BtEvent;
                event->event_id = GAP_API_START_INQUIRY;
                ALOGV (LOGTAG " Posting inquiry to GAP thread");
                PostMessage (THREAD_ID_GAP, event);

            } else if (g_bt_app->status.enquiry_cmd == COMMAND_INPROGRESS) {
                fprintf( stdout, " The inquiry is already in process\n");

            } else {
                fprintf( stdout, "currently BT is OFF\n");
            }
            break;

        case CANCEL_ENQUIRY:

            if ((g_bt_app->status.stop_enquiry_cmd != COMMAND_INPROGRESS) &&
                (g_bt_app->bt_discovery_state == BT_DISCOVERY_STARTED) &&
                        (g_bt_app->bt_state == BT_STATE_ON)) {
                g_bt_app->status.stop_enquiry_cmd = COMMAND_INPROGRESS;
                event = new BtEvent;
                event->event_id = GAP_API_STOP_INQUIRY;
                ALOGV (LOGTAG " Posting stop inquiry to GAP thread");
                PostMessage (THREAD_ID_GAP, event);
                if (!g_bt_app->inquiry_list.empty()) {
                    g_bt_app->PrintInquiryList();
                } else {
                    fprintf( stdout, " Empty Inquiry list\n");
                }

            } else if (g_bt_app->status.stop_enquiry_cmd == COMMAND_INPROGRESS) {
                fprintf( stdout, " The stop inquiry is already in process\n");

            } else if (g_bt_app->bt_state == BT_STATE_OFF) {
                fprintf( stdout, "currently BT is OFF\n");

            } else if (g_bt_app->bt_discovery_state != BT_DISCOVERY_STARTED) {
                fprintf( stdout,"Inquiry is not started, ignoring the stop inquiry\n");
            }
            break;

        case GET_ROLE:
            if (g_bt_app->bt_state == BT_STATE_ON) {
                if (string_is_bdaddr(user_cmd[ONE_PARAM])) {
                    event = new BtEvent;
                    event->event_id = GAP_API_GET_ROLE_REQ;
                    string_to_bdaddr(user_cmd[ONE_PARAM], &event->role_request.bd_addr);
                    PostMessage (THREAD_ID_GAP, event);
                } else {
                 fprintf( stdout, " BD address is NULL/Invalid \n");
                }
            } else {
                fprintf( stdout, " Currently BT is OFF\n");
            }
            break;

         case SWITCH_ROLE:
            if (g_bt_app->bt_state == BT_STATE_ON) {
                if (string_is_bdaddr(user_cmd[ONE_PARAM])) {
                            event = new BtEvent;
                            event->event_id = GAP_API_SWITCH_ROLE_REQ;
                            string_to_bdaddr(user_cmd[ONE_PARAM], &event->role_switch.bd_addr);
                            uint8_t new_role = atoi(user_cmd[TWO_PARAM]);
                            event->role_switch.new_role = new_role;
                            PostMessage (THREAD_ID_GAP, event);
                } else {
                 fprintf( stdout, " BD address is NULL/Invalid \n");
                }
            } else {
                fprintf( stdout, " Currently BT is OFF\n");
            }
            break;

        case START_PAIR:
            if ((g_bt_app->status.pairing_cmd != COMMAND_INPROGRESS) &&
                (g_bt_app->bt_state == BT_STATE_ON)) {
                if (string_is_bdaddr(user_cmd[ONE_PARAM])) {
                    bt_bdaddr_t address;
                    string_to_bdaddr(user_cmd[ONE_PARAM], &address);
                    /* Check if device is already bonded */
                    if(g_gap->IsDeviceBonded(address))
                    {
                       fprintf( stdout, "Device is already paired!\n");
                       return;
                    }
                    g_bt_app->status.pairing_cmd = COMMAND_INPROGRESS;
                    event = new BtEvent;
                    event->event_id = GAP_API_CREATE_BOND;
                    memcpy(&event->bond_device.bd_addr, &address, sizeof(bt_bdaddr_t));
                    switch (atoi(user_cmd[TWO_PARAM])){
                    case 0:
                        event->bond_device.transport = 0;
                        fprintf( stdout, " Auto select in the stack \n");
                        break;
                    case 1:
                        event->bond_device.transport = 1;
                        fprintf( stdout, " BR/EDR Bonding\n");
                        break;
                    case 2:
                        event->bond_device.transport = 2;
                        fprintf( stdout, " BLE Bonding \n");
                        break;
                    default:
                        event->bond_device.transport = 0;
                        fprintf( stdout, " Invalid transport parameter, auto selecting \n");
                        break;
                    }
                    PostMessage (THREAD_ID_GAP, event);
                } else {
                 fprintf( stdout, " BD address is NULL/Invalid \n");
                }
            } else if (g_bt_app->status.pairing_cmd == COMMAND_INPROGRESS) {
                fprintf( stdout, " Pairing is already in process\n");
            } else {
                fprintf( stdout, " Currently BT is OFF\n");
            }
            break;

        case UNPAIR:
            if ( g_bt_app->bt_state == BT_STATE_ON ) {
                if (string_is_bdaddr(user_cmd[ONE_PARAM])) {
                    bt_bdaddr_t bd_addr;
                    string_to_bdaddr(user_cmd[ONE_PARAM], &bd_addr);
                    g_bt_app->HandleUnPair(bd_addr);
                } else {
                    fprintf( stdout, " BD address is NULL/Invalid \n");
                }
            } else {
                fprintf( stdout, " Currently BT is OFF\n");
            }
            break;

        case INQUIRY_LIST:
            if (!g_bt_app->inquiry_list.empty()) {
                g_bt_app->PrintInquiryList();
            } else {
                fprintf( stdout, " Empty Inquiry list\n");
            }
            break;

        case BONDED_LIST:
            if (!g_bt_app->bonded_devices.empty()) {
                g_bt_app->PrintBondedDeviceList();
            } else {
                fprintf( stdout, " Empty bonded list\n");
            }
            break;

        case GET_BT_STATE:
           if ( g_bt_app->GetState() == BT_STATE_ON )
               fprintf( stdout, "ON\n" );
            else if ( g_bt_app->GetState() == BT_STATE_OFF)
                fprintf( stdout, "OFF\n");
            break;

        case GET_BT_NAME:
            if ( g_bt_app->GetState() == BT_STATE_ON ) {
                fprintf(stdout, "BT Name : %s\n", g_gap->GetBtName());
            } else {
                fprintf( stdout, "No Name due to BT is OFF\n");
            }
            break;

        case GET_BT_ADDR:
            if ( g_bt_app->GetState() == BT_STATE_ON ) {
                bdstr_t bd_str;
                bt_bdaddr_t *bd_addr = g_gap->GetBtAddress();
                bdaddr_to_string(bd_addr, &bd_str[0], sizeof(bd_str));
                fprintf(stdout, " BT Address : %s\n", bd_str);
            } else {
                fprintf( stdout, "No Addr due to BT is OFF\n");
            }
            break;

        case SET_BT_NAME:
            if ( g_bt_app->GetState() == BT_STATE_ON ) {
                if (strlen(user_cmd[ONE_PARAM]) < BTM_MAX_LOC_BD_NAME_LEN &&
                    (user_cmd[ONE_PARAM] != NULL) ) {
                    bt_bdname_t bd_name;
                    event = new BtEvent;
                    event->event_id = GAP_API_SET_BDNAME;
                    event->set_device_name_event.prop.type = BT_PROPERTY_BDNAME;
                    strlcpy((char*)&bd_name.name[0],user_cmd[ONE_PARAM],COMMAND_SIZE);
                    event->set_device_name_event.prop.val = &bd_name;
                    event->set_device_name_event.prop.len = strlen((char*)bd_name.name);
                    PostMessage (THREAD_ID_GAP, event);
                } else {
                 fprintf( stdout, " BD Name is NULL/more than required legnth\n");
                }
            } else {
                fprintf( stdout, " Currently BT is OFF\n");
            }
            break;

        case SET_SCAN_MODE:
            if ( g_bt_app->GetState() == BT_STATE_ON ) {
                if (user_cmd[ONE_PARAM] !=NULL)  {
                    bt_scan_mode_t scan_mode;
                    event = new BtEvent;
                    switch (atoi(user_cmd[ONE_PARAM])){
                    case 0:
                        scan_mode = BT_SCAN_MODE_NONE;
                        fprintf( stdout, " Disabled Inquiry Scan and Page Scan \n");
                        break;
                    case 1:
                        scan_mode = BT_SCAN_MODE_CONNECTABLE;
                        fprintf( stdout, " Disabled Inquiry Scan and Page Scan Enabled \n");
                        break;
                    case 2:
                        scan_mode = BT_SCAN_MODE_CONNECTABLE_DISCOVERABLE;
                        fprintf( stdout, " Enabled Inquiry Scan and Page Scan \n");
                        break;
                    default:
                        fprintf( stdout, " Invalid SCAN MODE parameter \n");
                        return;
                    }
                event->event_id = GAP_API_SET_SCAN_MODE;
                event->set_scan_mode_event.prop.type = BT_PROPERTY_ADAPTER_SCAN_MODE;
                event->set_scan_mode_event.prop.val = &scan_mode;
                event->set_scan_mode_event.prop.len = sizeof(bt_scan_mode_t);
                PostMessage (THREAD_ID_GAP, event);
                } else {
                    fprintf( stdout, " Invalid SCAN MODE parameter\n");
                }
            } else {
                fprintf( stdout, " Currently BT is OFF\n");
            }
            break;
        case SET_AFH_CHANNELS:
            unsigned char output[10];
            if ( g_bt_app->GetState() == BT_STATE_ON ) {
                if ( user_cmd[ONE_PARAM] != NULL ) {
                    event = new BtEvent;
                    event->event_id = GAP_API_SET_AFH_CHANNELS;
                    bool ret = sscanf(user_cmd[ONE_PARAM],
                                      "%2hhx%02hhx%02hhx%02hhx%02hhx%02hhx%02hhx%02hhx%02hhx%02hhx",
                                      &output[0],&output[1],&output[2],&output[3],&output[4],
                                      &output[5],&output[6],&output[7],&output[8],&output[9])==10;
                    if (ret)
                        memcpy(event->set_afh_channels_event.map, output, 10);
                    else
                        fprintf( stdout, " AFH Host Channel Classification should be 10 Bytes \n");

                    ALOGD(LOGTAG " setAFHChannels map:0x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x",
                          event->set_afh_channels_event.map[0],
                          event->set_afh_channels_event.map[1],
                          event->set_afh_channels_event.map[2],
                          event->set_afh_channels_event.map[3],
                          event->set_afh_channels_event.map[4],
                          event->set_afh_channels_event.map[5],
                          event->set_afh_channels_event.map[6],
                          event->set_afh_channels_event.map[7],
                          event->set_afh_channels_event.map[8],
                          event->set_afh_channels_event.map[9]);
                    PostMessage (THREAD_ID_GAP, event);
                } else {
                    fprintf( stdout, " AFH Host Channel Classification should be 10 Bytes \n");
                }
            } else {
                fprintf( stdout, " Currently BT is OFF\n");
            }
            break;

        case SEND_HCI_COMMAND:
            if ( g_bt_app->GetState() == BT_STATE_ON ) {
                if ( user_cmd[ONE_PARAM] != NULL ) {
                    int cmd_size;
                    uint8_t *cmd_ptr;
                    event = new BtEvent;
                    event->event_id = GAP_API_SEND_HCI_COMMAND;
                    cmd_size = send_hci_cmd_parse_args(user_cmd[ONE_PARAM], &cmd_ptr);

                    if (cmd_size <= 0) {
                        fprintf( stdout, "hci cmd format error!\n");
                        break;
                    }

                    if (cmd_size != (cmd_ptr[2] + 3)) {
                        free(cmd_ptr);
                        fprintf( stdout, "hci cmd length error!\n");
                        break;
                    }
                    fprintf( stdout, "hci cmd cmd_size:%d\n",cmd_size);
                    event->send_hci_command_event.cmd = cmd_ptr;

                    PostMessage (THREAD_ID_GAP, event);
                } else {
                    fprintf( stdout, " Invalid param for HCI command \n");
                }
            } else {
                fprintf( stdout, " Currently BT is OFF\n");
            }
            break;

        case READ_CLOCK:
            if ( g_bt_app->GetState() == BT_STATE_ON ) {
                if (string_is_bdaddr(user_cmd[TWO_PARAM])) {
                    event = new BtEvent;
                    event->event_id = GAP_API_READ_CLOCK;
                    event->read_clock_event.which_clock= atoi(user_cmd[ONE_PARAM]);
                    string_to_bdaddr(user_cmd[TWO_PARAM], &event->read_clock_event.bd_addr);
                    PostMessage (THREAD_ID_GAP, event);
                } else {
                    fprintf( stdout, " Incorrect paramters\n");
                }
            } else {
                fprintf( stdout, " Currently BT is OFF\n");
            }
            break;

        default:
            ALOGV (LOGTAG " Command not handled");
            break;
    }
}

static void HandlePanCommand(int cmd_id, char user_cmd[][COMMAND_ARG_SIZE]) {

    long num;
    char *end;
    int index = 0;

    switch (cmd_id) {
        case CONNECT:
            if ((g_bt_app->bt_state == BT_STATE_ON)) {
                if (string_is_bdaddr(user_cmd[ONE_PARAM])) {
                    BtEvent *event = new BtEvent;
                    event->event_id = PAN_EVENT_DEVICE_CONNECT_REQ;
                    string_to_bdaddr(user_cmd[ONE_PARAM],
                            &event->pan_device_connect_event.bd_addr);
                    PostMessage (THREAD_ID_PAN, event);
                } else {
                    fprintf(stdout, " BD address is NULL/Invalid \n");
                }
            } else {
                fprintf(stdout, " Currently BT is in OFF state\n");
            }
            break;

        case DISCONNECT:
            if ((g_bt_app->bt_state == BT_STATE_ON)) {
                if (string_is_bdaddr(user_cmd[ONE_PARAM])) {
                    BtEvent *event = new BtEvent;
                    event->event_id = PAN_EVENT_DEVICE_DISCONNECT_REQ;
                    string_to_bdaddr(user_cmd[ONE_PARAM],
                            &event->pan_device_disconnect_event.bd_addr);
                    PostMessage (THREAD_ID_PAN, event);
                } else {
                    fprintf(stdout, " BD address is NULL/Invalid\n");
                }
            } else {
                fprintf(stdout, " Currently BT is in OFF state\n");
            }
            break;

        case CONNECTED_LIST:
            if ((g_bt_app->bt_state == BT_STATE_ON)) {
                BtEvent *event = new BtEvent;
                event->event_id = PAN_EVENT_DEVICE_CONNECTED_LIST_REQ;
                PostMessage (THREAD_ID_PAN, event);
            } else {
                fprintf(stdout," Currently BT is in OFF state\n");
            }
            break;

        case SET_TETHERING:

            if ((g_bt_app->bt_state == BT_STATE_ON)) {
                bool is_tethering_enable;

                if (!strcasecmp (user_cmd[ONE_PARAM], "true")) {
                    is_tethering_enable = true;
                } else if (!strcasecmp (user_cmd[ONE_PARAM], "false")) {
                    is_tethering_enable = false;
                } else {
                    fprintf(stdout, " Wrong option selected\n");
                    return;
                }

                BtEvent *event = new BtEvent;
                event->event_id = PAN_EVENT_SET_TETHERING_REQ;
                event->pan_set_tethering_event.is_tethering_on = is_tethering_enable;
                PostMessage (THREAD_ID_PAN, event);
            } else {
                fprintf(stdout, " Currently BT is in OFF state\n");
            }
            break;

        case GET_PAN_MODE:
            if ((g_bt_app->bt_state == BT_STATE_ON)) {
                BtEvent *event = new BtEvent;
                event->event_id = PAN_EVENT_GET_MODE_REQ;
                PostMessage (THREAD_ID_PAN, event);
            } else {
                fprintf(stdout, " Currently BT is in OFF state\n");
            }
            break;

        case BACK_TO_MAIN:
            menu_type = MAIN_MENU;
            DisplayMenu(menu_type);
            break;

        default:
            ALOGV (LOGTAG " Command not handled: %d", cmd_id);
            break;
    }
}

#ifdef USE_BT_OBEX
static void HandlePbapClientCommand(int cmd_id, char user_cmd[][COMMAND_ARG_SIZE]) {

    long num;
    char *end;
    int index = 0;
    BtEvent *event = NULL;

    if (g_bt_app && g_bt_app->bt_state != BT_STATE_ON) {
        ALOGE(LOGTAG "BT not switched on, can't handle PBAP Client commands");
        return;
    }

    switch (cmd_id) {

        case PBAP_REGISTER:
            event = new BtEvent;
            event->pbap_client_event.event_id = PBAP_CLIENT_REGISTER;
            PostMessage (THREAD_ID_PBAP_CLIENT, event);
            break;
        case CONNECT:
            event = new BtEvent;
            event->pbap_client_event.event_id = PBAP_CLIENT_CONNECT;
            string_to_bdaddr(user_cmd[ONE_PARAM], &event->pbap_client_event.bd_addr);
            PostMessage (THREAD_ID_PBAP_CLIENT, event);
            break;
        case DISCONNECT:
            event = new BtEvent;
            event->pbap_client_event.event_id = PBAP_CLIENT_DISCONNECT;
            string_to_bdaddr(user_cmd[ONE_PARAM], &event->pbap_client_event.bd_addr);
            PostMessage (THREAD_ID_PBAP_CLIENT, event);
            break;
        case PBAP_ABORT:
            event = new BtEvent;
            event->pbap_client_event.event_id = PBAP_CLIENT_ABORT;
            PostMessage (THREAD_ID_PBAP_CLIENT, event);
            break;
        case PBAP_GET_PHONEBOOK_SIZE:
            event = new BtEvent;
            event->pbap_client_event.event_id = PBAP_CLIENT_GET_PHONEBOOK_SIZE;
            PostMessage (THREAD_ID_PBAP_CLIENT, event);
            break;
        case PBAP_GET_PHONEBOOK:
            event = new BtEvent;
            event->pbap_client_event.event_id = PBAP_CLIENT_GET_PHONEBOOK;
            PostMessage (THREAD_ID_PBAP_CLIENT, event);
            break;
        case PBAP_GET_VCARD:
            event = new BtEvent;
            event->pbap_client_event.event_id = PBAP_CLIENT_GET_VCARD;
            memset( (void *) event->pbap_client_event.value, '\0',
                sizeof(event->pbap_client_event.value));
            strlcpy(event->pbap_client_event.value, user_cmd[ONE_PARAM],
                COMMAND_SIZE);
            PostMessage (THREAD_ID_PBAP_CLIENT, event);
            break;
        case PBAP_GET_VCARD_LISTING:
            event = new BtEvent;
            event->pbap_client_event.event_id = PBAP_CLIENT_GET_VCARD_LISTING;
            PostMessage (THREAD_ID_PBAP_CLIENT, event);
            break;
        case PBAP_SET_PATH:
            event = new BtEvent;
            event->pbap_client_event.event_id = PBAP_CLIENT_SET_PATH;
            memset( (void *) event->pbap_client_event.value, '\0',
                sizeof(event->pbap_client_event.value));
            strlcpy(event->pbap_client_event.value, user_cmd[ONE_PARAM],
                COMMAND_SIZE);
            PostMessage (THREAD_ID_PBAP_CLIENT, event);
            break;
        case PBAP_SET_FILTER:
            event = new BtEvent;
            event->pbap_client_event.event_id = PBAP_CLIENT_SET_FILTER;
            memset( (void *) event->pbap_client_event.value, '\0',
                sizeof(event->pbap_client_event.value));
            strlcpy(event->pbap_client_event.value, user_cmd[ONE_PARAM],
                COMMAND_SIZE);
            PostMessage (THREAD_ID_PBAP_CLIENT, event);
            break;
        case PBAP_GET_FILTER:
            event = new BtEvent;
            event->pbap_client_event.event_id = PBAP_CLIENT_GET_FILTER;
            PostMessage (THREAD_ID_PBAP_CLIENT, event);
            break;
        case PBAP_SET_ORDER:
            event = new BtEvent;
            event->pbap_client_event.event_id = PBAP_CLIENT_SET_ORDER;
            memset( (void *) event->pbap_client_event.value, '\0',
                sizeof(event->pbap_client_event.value));
            strlcpy(event->pbap_client_event.value, user_cmd[ONE_PARAM],
                COMMAND_SIZE);
            PostMessage (THREAD_ID_PBAP_CLIENT, event);
            break;
        case PBAP_GET_ORDER:
            event = new BtEvent;
            event->pbap_client_event.event_id = PBAP_CLIENT_GET_ORDER;
            PostMessage (THREAD_ID_PBAP_CLIENT, event);
            break;
        case PBAP_SET_SEARCH_ATTRIBUTE:
            event = new BtEvent;
            event->pbap_client_event.event_id = PBAP_CLIENT_SET_SEARCH_ATTRIBUTE;
            memset( (void *) event->pbap_client_event.value, '\0',
                sizeof(event->pbap_client_event.value));
            strlcpy(event->pbap_client_event.value, user_cmd[ONE_PARAM],
                COMMAND_SIZE);
            PostMessage (THREAD_ID_PBAP_CLIENT, event);
            break;
        case PBAP_GET_SEARCH_ATTRIBUTE:
            event = new BtEvent;
            event->pbap_client_event.event_id = PBAP_CLIENT_GET_SEARCH_ATTRIBUTE;
            PostMessage (THREAD_ID_PBAP_CLIENT, event);
            break;
        case PBAP_SET_SEARCH_VALUE:
            event = new BtEvent;
            event->pbap_client_event.event_id = PBAP_CLIENT_SET_SEARCH_VALUE;
            memset( (void *) event->pbap_client_event.value, '\0',
                sizeof(event->pbap_client_event.value));
            strlcpy(event->pbap_client_event.value, user_cmd[ONE_PARAM],
                COMMAND_SIZE);
            PostMessage (THREAD_ID_PBAP_CLIENT, event);
            break;
        case PBAP_SET_PHONE_BOOK:
            event = new BtEvent;
            event->pbap_client_event.event_id = PBAP_CLIENT_SET_PHONE_BOOK;
            memset( (void *) event->pbap_client_event.value, '\0',
                sizeof(event->pbap_client_event.value));
            strlcpy(event->pbap_client_event.value, user_cmd[ONE_PARAM],
                COMMAND_SIZE);
            PostMessage (THREAD_ID_PBAP_CLIENT, event);
            break;
        case PBAP_GET_PHONE_BOOK:
            event = new BtEvent;
            event->pbap_client_event.event_id = PBAP_CLIENT_GET_PHONE_BOOK;
            PostMessage (THREAD_ID_PBAP_CLIENT, event);
            break;
        case PBAP_SET_REPOSITORY:
            event = new BtEvent;
            event->pbap_client_event.event_id = PBAP_CLIENT_SET_REPOSITORY;
            memset( (void *) event->pbap_client_event.value, '\0',
                sizeof(event->pbap_client_event.value));
            strlcpy(event->pbap_client_event.value, user_cmd[ONE_PARAM],
                COMMAND_SIZE);
            PostMessage (THREAD_ID_PBAP_CLIENT, event);
            break;
        case PBAP_GET_REPOSITORY:
            event = new BtEvent;
            event->pbap_client_event.event_id = PBAP_CLIENT_GET_REPOSITORY;
            PostMessage (THREAD_ID_PBAP_CLIENT, event);
            break;
        case PBAP_SET_VCARD_FORMAT:
            event = new BtEvent;
            event->pbap_client_event.event_id = PBAP_CLIENT_SET_VCARD_FORMAT;
            memset( (void *) event->pbap_client_event.value, '\0',
                sizeof(event->pbap_client_event.value));
            strlcpy(event->pbap_client_event.value, user_cmd[ONE_PARAM],
                COMMAND_SIZE);
            PostMessage (THREAD_ID_PBAP_CLIENT, event);
            break;
        case PBAP_GET_VCARD_FORMAT:
            event = new BtEvent;
            event->pbap_client_event.event_id = PBAP_CLIENT_GET_VCARD_FORMAT;
            PostMessage (THREAD_ID_PBAP_CLIENT, event);
            break;
        case PBAP_SET_LIST_COUNT:
            event = new BtEvent;
            event->pbap_client_event.event_id = PBAP_CLIENT_SET_LIST_COUNT;
            event->pbap_client_event.max_list_count = atoi(user_cmd[ONE_PARAM]);
            PostMessage (THREAD_ID_PBAP_CLIENT, event);
            break;
        case PBAP_GET_LIST_COUNT:
            event = new BtEvent;
            event->pbap_client_event.event_id = PBAP_CLIENT_GET_LIST_COUNT;
            PostMessage (THREAD_ID_PBAP_CLIENT, event);
            break;
        case PBAP_SET_START_OFFSET:
            event = new BtEvent;
            event->pbap_client_event.event_id = PBAP_CLIENT_SET_START_OFFSET;
            event->pbap_client_event.list_start_offset = atoi(user_cmd[ONE_PARAM]);
            PostMessage (THREAD_ID_PBAP_CLIENT, event);
            break;
        case PBAP_GET_START_OFFSET:
            event = new BtEvent;
            event->pbap_client_event.event_id = PBAP_CLIENT_GET_START_OFFSET;
            PostMessage (THREAD_ID_PBAP_CLIENT, event);
            break;
        case BACK_TO_MAIN:
            menu_type = MAIN_MENU;
            DisplayMenu(menu_type);
            break;

        default:
        ALOGV (LOGTAG " Command not handled: %d", cmd_id);
        break;
    }
}

static void HandleOppCommand(int cmd_id, char user_cmd[][COMMAND_ARG_SIZE]) {

    long num;
    char *end;
    int index = 0;
    BtEvent *event = NULL;

    if (g_bt_app && g_bt_app->bt_state != BT_STATE_ON) {
        ALOGE(LOGTAG "BT not switched on, can't handle OPP commands");
        return;
    }

    switch (cmd_id) {

        case OPP_REGISTER:
            event = new BtEvent;
            event->opp_event.event_id = OPP_SRV_REGISTER;
            PostMessage (THREAD_ID_OPP, event);
            break;
        case OPP_SEND:
            event = new BtEvent;
            event->opp_event.event_id = OPP_SEND_DATA;
            if(string_to_bdaddr(user_cmd[ONE_PARAM],
                &event->opp_event.bd_addr)) {
                memset( (void *) event->opp_event.value, 0,
                    sizeof(event->opp_event.value));
                strlcpy(event->opp_event.value, user_cmd[TWO_PARAM],
                    COMMAND_SIZE);
                PostMessage (THREAD_ID_OPP, event);
            } else {
                ALOGV (LOGTAG " Please enter valid BD Address %s",
                    user_cmd[ONE_PARAM]);
            }
            break;
        case OPP_ABORT:
            event = new BtEvent;
            event->opp_event.event_id = OPP_ABORT_TRANSFER;
            PostMessage (THREAD_ID_OPP, event);
            break;
        case BACK_TO_MAIN:
            menu_type = MAIN_MENU;
            DisplayMenu(menu_type);
            break;

        default:
        ALOGV (LOGTAG " Command not handled: %d", cmd_id);
        break;
    }
}
#endif
static void HandleSppClientCommand(int cmd_id, char user_cmd[][COMMAND_ARG_SIZE]) {

    BtEvent *event = NULL;

    if ((cmd_id != BACK_TO_MAIN) && g_bt_app && g_bt_app->bt_state != BT_STATE_ON) {
        ALOGE(LOGTAG "BT not switched on, can't handle SPP commands");
        return;
    }

    switch(cmd_id) {
        case SPPCLIENT_CONNECT:
            event = new BtEvent;
            event->spp_cli_event.event_id = SPP_CLI_CONNECT;
            if(string_to_bdaddr(user_cmd[ONE_PARAM],
                &event->spp_cli_event.bd_addr)) {
                PostMessage (THREAD_ID_SPP_CLIENT, event);
            } else {
                ALOGV (LOGTAG " Please enter valid BD Address %s",
                    user_cmd[ONE_PARAM]);
            }
            break;

        case SPPCLIENT_DISCONNECT:
            event = new BtEvent;
            event->spp_cli_event.event_id = SPP_CLI_DISCONNECT;
            PostMessage(THREAD_ID_SPP_CLIENT, event);
            break;

        case SPPCLIENT_SEND_FILE:
            event = new BtEvent;
            event->spp_cli_event.event_id = SPP_CLI_SEND_FILE;
            memset( (void *) event->spp_cli_event.value, 0,
                sizeof(event->spp_cli_event.value));
            strlcpy(event->spp_cli_event.value, user_cmd[ONE_PARAM],
                COMMAND_SIZE);
            PostMessage (THREAD_ID_SPP_CLIENT, event);
            break;

        case SPPCLIENT_SEND_DATA:
            event = new BtEvent;
            event->spp_cli_event.event_id = SPP_CLI_SEND_DATA;
            memset( (void *) event->spp_cli_event.value, 0,
                sizeof(event->spp_cli_event.value));
            strlcpy(event->spp_cli_event.value, user_cmd[ONE_PARAM],
                COMMAND_SIZE);
            PostMessage (THREAD_ID_SPP_CLIENT, event);
            break;

        case SPPCLIENT_RECV_FILE:
            event = new BtEvent;
            event->spp_cli_event.event_id = SPP_CLI_RECV_FILE;
            memset( (void *) event->spp_cli_event.value, 0,
                sizeof(event->spp_cli_event.value));
            strlcpy(event->spp_cli_event.value, user_cmd[ONE_PARAM],
                COMMAND_SIZE);
            PostMessage (THREAD_ID_SPP_CLIENT, event);
            break;

        case SPPCLIENT_RECV_DATA:
            event = new BtEvent;
            event->spp_cli_event.event_id = SPP_CLI_RECV_DATA;
            PostMessage (THREAD_ID_SPP_CLIENT, event);
            break;

        case BACK_TO_MAIN:
            menu_type = MAIN_MENU;
            DisplayMenu(menu_type);
            break;

        default:
            ALOGV(LOGTAG " %s Command not handled: %d", __func__, cmd_id);
            break;

    }
}


static void HandleSppServerCommand(int cmd_id, char user_cmd[][COMMAND_ARG_SIZE]) {
    BtEvent *event = NULL;

    if ((cmd_id != BACK_TO_MAIN) && g_bt_app && g_bt_app->bt_state != BT_STATE_ON) {
        ALOGE(LOGTAG "BT not switched on, can't handle SPP Server commands");
        return;
    }

    switch(cmd_id) {
        case SPPSERVER_START:
            event = new BtEvent;
            event->spp_srv_event.event_id = SPP_SRV_START;
            PostMessage(THREAD_ID_SPP_SERVER, event);
            break;
        case SPPSERVER_DISCONNECT:
            event = new BtEvent;
            event->spp_srv_event.event_id = SPP_SRV_DISCONNECT;
            PostMessage(THREAD_ID_SPP_SERVER, event);
            break;
        case SPPSERVER_SEND_FILE:
            event = new BtEvent;
            event->spp_srv_event.event_id = SPP_SRV_SEND_FILE;
            memset( (void *) event->spp_srv_event.value, 0,
                sizeof(event->spp_srv_event.value));
            strlcpy(event->spp_srv_event.value, user_cmd[ONE_PARAM],
                COMMAND_SIZE);
            PostMessage (THREAD_ID_SPP_SERVER, event);
            break;
        case SPPSERVER_SEND_DATA:
            event = new BtEvent;
            event->spp_srv_event.event_id = SPP_SRV_SEND_DATA;
            memset( (void *) event->spp_srv_event.value, 0,
                sizeof(event->spp_srv_event.value));
            strlcpy(event->spp_srv_event.value, user_cmd[ONE_PARAM],
                COMMAND_SIZE);
            PostMessage (THREAD_ID_SPP_SERVER, event);
            break;
        case SPPSERVER_RECV_FILE:
            event = new BtEvent;
            event->spp_srv_event.event_id = SPP_SRV_RECV_FILE;
            memset( (void *) event->spp_srv_event.value, 0,
                sizeof(event->spp_srv_event.value));
            strlcpy(event->spp_srv_event.value, user_cmd[ONE_PARAM],
                COMMAND_SIZE);
            PostMessage (THREAD_ID_SPP_SERVER, event);
            break;
        case SPPSERVER_RECV_DATA:
            event = new BtEvent;
            event->spp_cli_event.event_id = SPP_SRV_RECV_DATA;
            PostMessage (THREAD_ID_SPP_SERVER, event);
            break;
        case BACK_TO_MAIN:
            menu_type = MAIN_MENU;
            DisplayMenu(menu_type);
            break;
        default:
            ALOGV(LOGTAG " %s Command not handled: %d", __func__, cmd_id);
            break;
    }


}
void BtSocketDataHandler (void *context) {
    char ipc_msg[BT_IPC_MSG_LEN]  = {0};
    int len;
    if(g_bt_app->client_socket_ != -1) {
        len = recv(g_bt_app->client_socket_, ipc_msg, BT_IPC_MSG_LEN, 0);

        if (len <= 0) {
            ALOGE("Not able to receive msg to remote dev: %s", strerror(errno));
            reactor_unregister (g_bt_app->accept_reactor_);
            g_bt_app->accept_reactor_ = NULL;
            close(g_bt_app->client_socket_);
            g_bt_app->client_socket_ = -1;
        } else if(len == BT_IPC_MSG_LEN) {
            BtEvent *event = new BtEvent;
            event->event_id = SKT_API_IPC_MSG_READ;
            event->bt_ipc_msg_event.ipc_msg.type = ipc_msg[0];
            event->bt_ipc_msg_event.ipc_msg.status = ipc_msg[1];

            switch (event->bt_ipc_msg_event.ipc_msg.type) {
                /*fall through for PAN IPC message*/
                case BT_IPC_ENABLE_TETHERING:
                case BT_IPC_DISABLE_TETHERING:
                case BT_IPC_ENABLE_REVERSE_TETHERING:
                case BT_IPC_DISABLE_REVERSE_TETHERING:
                    ALOGV (LOGTAG "  Posting IPC_MSG to PAN thread");
                    PostMessage (THREAD_ID_PAN, event);
                    break;
                case BT_IPC_REMOTE_START_WLAN:
                    ALOGV (LOGTAG "  Posting IPC_MSG to GATT thread");
                    PostMessage (THREAD_ID_GATT, event);
                    break;
                default:
                    delete event;
                break;
            }
        }
    }
}

void BtSocketListenHandler (void *context) {
    struct sockaddr_un cliaddr;
    int length;

    if(g_bt_app->client_socket_ == -1) {
        g_bt_app->client_socket_ = accept(g_bt_app->listen_socket_local_,
            (struct sockaddr*) &cliaddr, ( socklen_t *) &length);
        if (g_bt_app->client_socket_ == -1) {
            ALOGE (LOGTAG "%s error accepting LOCAL socket: %s",
                        __func__, strerror(errno));
        } else {
            g_bt_app->accept_reactor_ = reactor_register
                (thread_get_reactor (threadInfo[THREAD_ID_MAIN].thread_id),
                g_bt_app->client_socket_, NULL, BtSocketDataHandler, NULL);
        }
    } else {
        ALOGI (LOGTAG " Accepting and closing the next connection .\n");
        int accept_socket = accept(g_bt_app->listen_socket_local_,
            (struct sockaddr*) &cliaddr, ( socklen_t *) &length);
        if(accept_socket)
            close(accept_socket);
    }
}

static void BtCmdHandler (void *context) {

    int cmd_id = -1;
    char user_cmd[MAX_ARGUMENTS][COMMAND_ARG_SIZE];
    int index = 0;
    memset( (void *) user_cmd, '\0', sizeof(user_cmd));

    if (HandleUserInput (&cmd_id, user_cmd, menu_type)) {
        ALOGI (LOGTAG "BtCmdHandler menu_type:%d cmd_id:%d", menu_type, cmd_id);
        switch(menu_type) {
            case GAP_MENU:
                HandleGapCommand(cmd_id,user_cmd);
                break;
            case PAN_MENU:
                HandlePanCommand(cmd_id, user_cmd);
                break;
            case TEST_MENU:
                HandleTestCommand(cmd_id, user_cmd);
                break;
#ifdef USE_GEN_GATT
            case GATTC_TEST_MENU:
                fprintf(stdout, "BtCmdHandler GATTC_TEST_MENU");
                HandleGattcTestCommand(cmd_id, user_cmd);
                break;
            case GATTSTEST_MENU:
                HandleGattsTestCommand(cmd_id, user_cmd);
                break;
           case RSP_MENU:
                HandleRspCommand(cmd_id, user_cmd);
                break;
#endif
            case MAIN_MENU:
                HandleMainCommand(cmd_id,user_cmd );
                break;
            case A2DP_SINK_MENU:
                HandleA2dpSinkCommand(cmd_id,user_cmd );
                break;
            case A2DP_SOURCE_MENU:
                HandleA2dpSourceCommand(cmd_id, user_cmd );
                break;
            case HFP_CLIENT_MENU:
                HandleHfpClientCommand(cmd_id,user_cmd );
                break;
#ifdef USE_BT_OBEX
            case PBAP_CLIENT_MENU:
                HandlePbapClientCommand(cmd_id,user_cmd );
                break;
            case OPP_MENU:
                HandleOppCommand(cmd_id,user_cmd );
                break;
#endif
            case SPP_SERVER_MENU:
                HandleSppServerCommand(cmd_id, user_cmd);
                break;
            case SPP_CLIENT_MENU:
                HandleSppClientCommand(cmd_id, user_cmd);
                break;
            case HFP_AG_MENU:
                HandleHfpAGCommand(cmd_id, user_cmd );
                break;
            case HIDH_MENU:
                HandleHIDCommand(cmd_id,user_cmd );
                break;
#ifdef SUPPORT_ESL_AP
            case ESLAP_MENU:
                HandleEslapCommand(cmd_id,user_cmd);
                break;
#endif
        }
   } else if (g_bt_app->ssp_notification && user_cmd[0][0] &&
                        (!strcasecmp (user_cmd[ZERO_PARAM], "yes") ||
                        !strcasecmp (user_cmd[ZERO_PARAM], "no") ||
                        !strcasecmp (user_cmd[ZERO_PARAM], "cancel") )
                        && g_bt_app->HandleSspInput(user_cmd)) {
        // validate the user input for SSP
        g_bt_app->ssp_notification = false;
    } else if (g_bt_app->pin_notification && user_cmd[0][0] &&
                        (strcasecmp (user_cmd[ZERO_PARAM], "yes") &&
                        strcasecmp (user_cmd[ZERO_PARAM], "no") &&
                        strcasecmp (user_cmd[ZERO_PARAM], "accept") &&
                        strcasecmp (user_cmd[ZERO_PARAM], "reject"))
                        && g_bt_app->HandlePinInput(user_cmd)) {
        // validate the user input for PIN
        g_bt_app->pin_notification = false;
    } else if (g_bt_app->ssp_notification && user_cmd[0][0] &&
                        (!strcasecmp (user_cmd[ZERO_PARAM], "ok") ||
                        !strcasecmp (user_cmd[ZERO_PARAM], "cancel"))
                        && g_bt_app->HandleSspInput(user_cmd)) {
        // validate the user input for SSP
        g_bt_app->ssp_notification = false;
    }
#ifdef USE_BT_OBEX
    else if (g_bt_app->incoming_file_notification && user_cmd[0][0] &&
                        (!strcasecmp (user_cmd[ZERO_PARAM], "accept") ||
                        !strcasecmp (user_cmd[ZERO_PARAM], "reject"))
                        && g_bt_app->HandleIncomingFile(user_cmd)) {
        // validate the user input for OPP Incoming File
        g_bt_app->incoming_file_notification = false;
    }
#endif
    else {
        fprintf( stdout, " Wrong option selected\n");
        DisplayMenu(menu_type);
        // TODO print the given input string
        return;
    }
}

void BtMainMsgHandler (void *context) {

    BtEvent *event = NULL;
    if (!context) {
        ALOGI (LOGTAG " Msg is null, return.\n");
        return;
    }
    event = (BtEvent *) context;

    switch (event->event_id) {
        case SKT_API_IPC_MSG_WRITE:
            ALOGV (LOGTAG "client_socket: %d", g_bt_app->client_socket_);
            if(g_bt_app->client_socket_ != -1) {
                int len;
                if((len = send(g_bt_app->client_socket_, &(event->bt_ipc_msg_event.ipc_msg),
                    BT_IPC_MSG_LEN, 0)) < 0) {
                    reactor_unregister (g_bt_app->accept_reactor_);
                    close(g_bt_app->client_socket_);
                    g_bt_app->client_socket_ = -1;
                    ALOGE (LOGTAG "Local socket send fail %s", strerror(errno));
                }
                ALOGV (LOGTAG "sent %d bytes", len);
            }
            delete event;
            break;

        case MAIN_API_INIT:
            if (!g_bt_app)
                g_bt_app = new BluetoothApp();
        // fallback to default handler
        default:
            if (g_bt_app)
                g_bt_app->ProcessEvent ((BtEvent *) context);
            delete event;
            break;
    }
}

#ifdef __cplusplus
}
#endif

bool BluetoothApp :: HandlePinInput(char user_cmd[][COMMAND_ARG_SIZE]) {
    BtEvent *bt_event = new BtEvent;

    if (pin_reply.secure  == true ) {
        if ( strlen(user_cmd[ZERO_PARAM]) != 16){
            fprintf(stdout, " Minimum 16 digit pin required\n");
            return false;
        }
    } else if(strlen(user_cmd[ZERO_PARAM]) > 16 ) {
        return false;
    }

    memset(&pin_reply.pincode, 0, sizeof(bt_pin_code_t));
    memcpy(&pin_reply.pincode.pin, user_cmd[ZERO_PARAM],
                        strlen(user_cmd[ZERO_PARAM]));
    memcpy(&bt_event->pin_reply_event.bd_addr, &pin_reply.bd_addr,
                                            sizeof(bt_bdaddr_t));
    bt_event->pin_reply_event.pin_len = strlen(user_cmd[ZERO_PARAM]);
    memcpy(&bt_event->pin_reply_event.bd_name, &pin_reply.bd_name,
                                        sizeof(bt_bdname_t));
    memcpy(&bt_event->pin_reply_event.pincode.pin, &pin_reply.pincode.pin,
                                        sizeof(bt_pin_code_t));
    bt_event->event_id = GAP_API_PIN_REPLY;
    PostMessage (THREAD_ID_GAP, bt_event);
    return true;
}


bool BluetoothApp :: HandleSspInput(char user_cmd[][COMMAND_ARG_SIZE]) {


    BtEvent *bt_event = new BtEvent;
    if ((!strcasecmp (user_cmd[ZERO_PARAM], "yes")) || (!strcasecmp (user_cmd[ZERO_PARAM], "ok"))) {
        ssp_data.accept = true;
    }
    else if (!strcasecmp (user_cmd[ZERO_PARAM], "no")) {
        ssp_data.accept = false;
    }
    else if (!strcasecmp (user_cmd[ZERO_PARAM], "cancel")) {
        fprintf( stdout, " cancel selected\n");
        bt_interface->cancel_bond(&ssp_data.bd_addr);
        return true;
    } else {
        fprintf( stdout, " Wrong option selected\n");
        return false;
    }

    memcpy(&bt_event->ssp_reply_event.bd_addr, &ssp_data.bd_addr,
                                            sizeof(bt_bdaddr_t));
    memcpy(&bt_event->ssp_reply_event.bd_name, &ssp_data.bd_name,
                                        sizeof(bt_bdname_t));
    bt_event->ssp_reply_event.cod = ssp_data.cod;
    bt_event->ssp_reply_event.pairing_variant = ssp_data.pairing_variant;
    bt_event->ssp_reply_event.pass_key = ssp_data.pass_key;
    bt_event->ssp_reply_event.accept = ssp_data.accept;
    bt_event->event_id = GAP_API_SSP_REPLY;
    PostMessage (THREAD_ID_GAP, bt_event);
    return true;
}

#ifdef USE_BT_OBEX
bool BluetoothApp :: HandleIncomingFile(char user_cmd[][COMMAND_ARG_SIZE]) {
    BtEvent *bt_event = new BtEvent;
    if (!strcasecmp (user_cmd[ZERO_PARAM], "accept")) {
        bt_event->opp_event.accept = true;
    } else if (!strcasecmp (user_cmd[ZERO_PARAM], "reject")) {
        bt_event->opp_event.accept = false;
    } else {
        fprintf( stdout, "Wrong option entered\n");
        return false;
    }
    /* Cancel the timer */
    alarm_cancel(opp_incoming_file_accept_timer);
    bt_event->event_id = OPP_INCOMING_FILE_RESPONSE;
    PostMessage (THREAD_ID_OPP, bt_event);
    return true;
}

void user_acceptance_timer_expired(void *context) {
    BtEvent *bt_event = new BtEvent;
    ALOGD(LOGTAG " user_acceptance_timer_expired, rejecting incoming file");
    fprintf(stdout,"No user input for %d seconds, rejecting the file\n",
        USER_ACCEPTANCE_TIMEOUT/1000);

    bt_event->opp_event.accept = false;
    g_bt_app->incoming_file_notification = false;
    bt_event->event_id = OPP_INCOMING_FILE_RESPONSE;
    PostMessage (THREAD_ID_OPP, bt_event);
}
#endif

int BluetoothApp::inq_db_count = 0;
void BluetoothApp :: ProcessEvent (BtEvent * event) {

    ALOGD (LOGTAG " Processing event %d", event->event_id);
    const uint8_t *ptr = NULL;

    switch (event->event_id) {
        case MAIN_API_INIT:
            InitHandler();
            break;

        case MAIN_API_DEINIT:
            DeInitHandler();
            break;

        case MAIN_API_ENABLE:
            SendEnableCmdToGap();
            break;

        case MAIN_API_DISABLE:
#ifdef SUPPORT_ESL_AP
            HandleAPDeinitCmd();
#endif
            SendDisableCmdToGap();
            break;

        case MAIN_EVENT_ENABLED:
            bt_state = event->state_event.status;
            if (event->state_event.status == BT_STATE_OFF) {
                fprintf(stdout," Error in Enabling BT\n");
            } else {
              ALOGD (LOGTAG " BT State is ON : %d",event->state_event.status);
              fprintf(stdout," BT State is ON\n");

              if (is_bt_enable_autotest){
                if ((g_bt_app->status.enquiry_cmd != COMMAND_INPROGRESS) &&
                     (g_bt_app->bt_state == BT_STATE_ON)) {
                  g_bt_app->inquiry_list.clear();
                  g_bt_app->status.enquiry_cmd = COMMAND_INPROGRESS;
                  event = new BtEvent;
                  event->event_id = GAP_API_START_INQUIRY;
                  ALOGV (LOGTAG " Posting inquiry to GAP thread");
                  PostMessage (THREAD_ID_GAP, event);
                } else if (g_bt_app->status.enquiry_cmd == COMMAND_INPROGRESS) {
                  fprintf( stdout, " The inquiry is already in process\n");
                } else {
                  fprintf( stdout, "currently BT is OFF\n");
                }
              }
            }
            status.enable_cmd = COMMAND_COMPLETE;
            if(!isBT_ON){
                isBT_ON=true;
                ExitHandler();
            } else if (is_bt_enable_test_menu_) {
                event = new BtEvent;
                event->event_id = MAIN_EVENT_TESTMENU_BT_ENABLED;
                fprintf (stdout, " Posting testmenu_BT enabled event to main thread\n");
                PostMessage (THREAD_ID_MAIN, event);
            }
            break;

        case MAIN_EVENT_DISABLED:
            bt_state = event->state_event.status;
            if (event->state_event.status == BT_STATE_ON) {
                fprintf(stdout, " Error in disabling BT\n");
            } else {
                if (gattctest!= NULL) {
                  fprintf(stdout, " delete Gattctest\n");
                  delete gattctest;
                  gattctest = NULL;
                }
                if (gattstest!= NULL) {
                  fprintf(stdout, " delete Gattstest\n");
                  gattstest->DisableGATTSTEST();
                  delete gattstest;
                  gattstest = NULL;
                  server_num = 0;
                  file_read = 0;
                  init_advertiser_file = false;
                }
                // clear the inquiry related cmds
                status.enquiry_cmd = COMMAND_COMPLETE;
                status.stop_enquiry_cmd = COMMAND_COMPLETE;
                bt_discovery_state = BT_DISCOVERY_STOPPED;
                // clearing bond_devices list and inquiry_list
                bonded_devices.clear();
                inquiry_list.clear();
                inq_db_count = 0;
                //system("killall -KILL wcnssfilter");
                usleep(200);
                ALOGD (LOGTAG " BT State is OFF : %d",bt_state);
                fprintf(stdout, " BT State is OFF\n");
                if(exithandler_waitbtoff){
                    // in exit scenario wait for BT disable once BT is disabled
                    // kill the process the to close the app
                    kill(getpid(), SIGKILL);
                }
            }
            if (is_bt_enable_test_menu_) {
              event = new BtEvent;
              event->event_id = MAIN_EVENT_TESTMENU_BT_DISABLED;
              fprintf (stdout, " Posting testmenu_BT disabled event to main thread\n");
              PostMessage (THREAD_ID_MAIN, event);
            }
            status.disable_cmd = COMMAND_COMPLETE;
            break;

        case MAIN_EVENT_ACL_CONNECTED:
            ALOGD (LOGTAG " MAIN_EVENT_ACL_CONNECTED\n");
            break;

        case MAIN_EVENT_ACL_DISCONNECTED:
            ALOGD (LOGTAG " MAIN_EVENT_ACL_DISCONNECTED\n");
            break;

        case MAIN_EVENT_INQUIRY_STATUS:
            if (event->discovery_state_event.state == BT_DISCOVERY_STARTED) {
                fprintf(stdout, " Inquiry Started\n");
            } else if (event->discovery_state_event.state == BT_DISCOVERY_STOPPED) {
                if ( status.enquiry_cmd == COMMAND_INPROGRESS) {
                    if ((bt_discovery_state == BT_DISCOVERY_STARTED) &&
                       (status.stop_enquiry_cmd != COMMAND_INPROGRESS))
                        fprintf(stdout, " Inquiry Stopped automatically\n");
                    else if (bt_discovery_state == BT_DISCOVERY_STOPPED)
                        fprintf(stdout, " Unable to start Inquiry\n");
                    status.enquiry_cmd = COMMAND_COMPLETE;
                }
                if (status.stop_enquiry_cmd == COMMAND_INPROGRESS) {
                    status.stop_enquiry_cmd = COMMAND_COMPLETE;
                    fprintf(stdout," Inquiry Stopped due to user input\n");
                }
            }
            if (event->discovery_state_event.state == BT_DISCOVERY_INQ_COMPLETE)
                bt_discovery_state = BT_DISCOVERY_STARTED;
            else
                bt_discovery_state = event->discovery_state_event.state;
            break;

        case MAIN_EVENT_DEVICE_FOUND:
           {
            bdstr_t bd_str;
            std::map<std::string, std::string>::iterator it;
            bdaddr_to_string(&event->device_found_event.remoteDevice.address, &bd_str[0], sizeof(bd_str));
            std::string deviceAddress(bd_str);

            it = bonded_devices.find(deviceAddress);
            ptr = (event->device_found_event.remoteDevice.address.address);
            if (it != bonded_devices.end())
            {
                fprintf(stdout, "Bonded Device Found details: \n");
                fprintf(stdout,"Found device Addr: %02x:%02x:%02x:%02x:%02x:%02x\n",
                                ptr[0], ptr[1], ptr[2], ptr[3], ptr[4], ptr[5]);
                fprintf(stdout, "Found device Name: %s\n", event->device_found_event.
                                                    remoteDevice.name);
                break;
            }
            fprintf(stdout, "Device Found details: \n");
            AddFoundedDevice(&event->device_found_event.remoteDevice);
            fprintf(stdout,"Found device Addr: %02x:%02x:%02x:%02x:%02x:%02x\n",
                                ptr[0], ptr[1], ptr[2], ptr[3], ptr[4], ptr[5]);
            fprintf(stdout, "Found device Name: %s\n", event->device_found_event.
                                                    remoteDevice.name);
            fprintf(stdout, "Device Type is: %d\n", event->device_found_event.
                                        remoteDevice.device_type);
            break;
        }
        case MAIN_EVENT_BOND_STATE:{
            char str[18];
            std::string bd_name((const char*)event->bond_state_event.bd_name.name);
            HandleBondState(event->bond_state_event.state,
                                    event->bond_state_event.bd_addr, bd_name);
            }
            break;

        case MAIN_EVENT_SSP_REQUEST:
            memcpy(&ssp_data.bd_addr, &event->ssp_request_event.bd_addr,
                                            sizeof(bt_bdaddr_t));
            memcpy(&ssp_data.bd_name, &event->ssp_request_event.bd_name,
                                                    sizeof(bt_bdname_t));
            ssp_data.cod = event->ssp_request_event.cod;
            ssp_data.pairing_variant = event->ssp_request_event.pairing_variant;
            ssp_data.pass_key = event->ssp_request_event.pass_key;
            // if pairing variant is passkey notification just show passkey
            // and cancel option only
            fprintf(stdout, "\n BT pairing_variant %d",
                                      ssp_data.pairing_variant);
            switch(ssp_data.pairing_variant)
            {
                case BT_SSP_VARIANT_PASSKEY_CONFIRMATION:
                    // instruct the cmd handler to treat the next inputs for SSP
                    fprintf(stdout, "\n*************************************************");
                    fprintf(stdout, "\n BT pairing request::Device %s::Pairing Code:: %06d",
                                                    ssp_data.bd_name.name, ssp_data.pass_key);
                    fprintf(stdout, "\n*************************************************\n");
                    fprintf(stdout, " ** Please enter yes / no **\n");
                    ssp_notification = true;
                break;
                case BT_SSP_VARIANT_PASSKEY_NOTIFICATION:
                    // instruct the cmd handler to treat the next inputs for SSP
                    fprintf(stdout, "\n*************************************************");
                    fprintf(stdout, "\n Pair with Device :: %s", ssp_data.bd_name.name);
                    fprintf(stdout, "\n Bluetooth Pairing code::%06d", ssp_data.pass_key);
                    fprintf(stdout, "\n Type the pairing code then press Return or Enter");
                    fprintf(stdout, "\n*************************************************\n");
                    fprintf(stdout, "** Please enter cancel **\n");
                    ssp_notification = true;
                break;
                case BT_SSP_VARIANT_CONSENT:
                    // instruct the cmd handler to treat the next inputs for SSP
                    fprintf(stdout, "\n*************************************************");
                    fprintf(stdout, "\n BT pairing request::Device %s",ssp_data.bd_name.name);
                    fprintf(stdout, "\n*************************************************\n");
                    fprintf(stdout, "** Please enter ok / cancel **\n");
                    ssp_notification = true;
                break;
                default:
                    fprintf(stdout, "Device Type::%d is not supported \n", ssp_data.pairing_variant);
                break;
            }
            break;

        case MAIN_EVENT_PIN_REQUEST:

            memcpy(&pin_reply.bd_addr, &event->pin_request_event.bd_addr,
                                            sizeof(bt_bdaddr_t));
            memcpy(&pin_reply.bd_name, &event->pin_request_event.bd_name,
                                            sizeof(bt_bdname_t));
            fprintf(stdout, "\n*************************************************");
            fprintf(stdout, "\n BT Legacy pairing request::Device %s::",
                                    pin_reply.bd_name.name);
            fprintf(stdout, "\n*************************************************\n");
            fprintf(stdout, " ** Please enter valid PIN key **\n");
            pin_reply.secure = event->pin_request_event.secure;
            // instruct the cmd handler to treat the next inputs for PIN
            pin_notification = true;
            break;

        case MAIN_EVENT_TESTMENU_BT_ENABLED:

           if (bt_state == BT_STATE_ON)
           {
             //fprintf(stdout, "Test_menu BT enabled for Iteration: %d\n",onoff_count);
             if(onoff_count > 0) {
               BtEvent *event_off = new BtEvent;
               event_off->event_id = MAIN_API_DISABLE;
               fprintf( stdout, "Iteration: %d Posting disable\n",onoff_index);
               ALOGD (LOGTAG "Iteration: %d Posting disable\n",onoff_index);
               PostMessage (THREAD_ID_MAIN, event_off);
             } else
               is_bt_enable_test_menu_ = false;
           } else {
             fprintf(stdout, "Test_menu BT enable failed in Iteration: %d\n",onoff_count);
             ALOGD (LOGTAG "Test_menu BT enable failed in Iteration: %d\n",onoff_count);
             onoff_count = 0;
           }
           break;

        case MAIN_EVENT_TESTMENU_BT_DISABLED:

            if(onoff_count > 0) {
              onoff_count--;
              onoff_index++;
            }
            if (bt_state == BT_STATE_OFF)
            {
              //fprintf(stdout, "Test_menu BT disabled for Iteration: %d\n",onoff_count);
              if(onoff_count > 0) {
                BtEvent *event_off = new BtEvent;
                event_off->event_id = MAIN_API_ENABLE;
                fprintf( stdout, "Iteration: %d Posting enable\n",onoff_index);
                ALOGD (LOGTAG "Iteration: %d Posting enable\n",onoff_index);
                PostMessage (THREAD_ID_MAIN, event_off);
              } else
                is_bt_enable_test_menu_ = false;
              } else {
                fprintf(stdout, "Test_menu BT disable failed in Iteration: %d\n",onoff_count);
                ALOGD (LOGTAG "Test_menu BT disable failed in Iteration: %d\n",onoff_count);
                onoff_count = 0;
            }
            break;

#ifdef USE_BT_OBEX
        case MAIN_EVENT_INCOMING_FILE_REQUEST:

            fprintf(stdout, "\n*************************************************");
            fprintf(stdout, "\n Incoming File Request");
            fprintf(stdout, "\n*************************************************\n");
            fprintf(stdout, " ** Please enter \"accept\" / \"reject\" **\n");
            // instruct the cmd handler to treat the next inputs for OPP
            incoming_file_notification = true;
            if (opp_incoming_file_accept_timer) {
                 // start the user acceptance/rejection rimer
                alarm_set(opp_incoming_file_accept_timer, USER_ACCEPTANCE_TIMEOUT,
                                    user_acceptance_timer_expired, NULL);
            } else {
                fprintf(stdout, "\n Already pending user acceptance request\n");
            }
            break;
#endif
#ifdef SUPPORT_ESL_AP
        case ESL_AP_EXCEPTION:
            if ((g_bt_app->ap_state == AP_STATE_ON) && (g_bt_app->status.eslap_deinit_cmd != COMMAND_INPROGRESS)) {
                fprintf(stdout, "\n ESL_AP_EXCEPTION\n");
                HandleAPDeinitCmd();
            }
            break;
#endif
        default:
            ALOGD (LOGTAG " Default Case");
            break;
    }
}

void BluetoothApp:: HandleBondState(bt_bond_state_t new_state, const bt_bdaddr_t
                                        bd_addr, std::string bd_name ) {
    std::map<std::string, std::string>::iterator it;
    DeviceProperties *it_inquiry;
    bdstr_t bd_str;
    bdaddr_to_string(&bd_addr, &bd_str[0], sizeof(bd_str));
    std::string deviceAddress(bd_str);
    it = bonded_devices.find(deviceAddress);
    it_inquiry= inq_db_find_bdaddr(deviceAddress);
    if(new_state == BT_BOND_STATE_BONDED) {
        if (it == bonded_devices.end()) {
            bonded_devices[deviceAddress] = bd_name;
        } else {
            bonded_devices.erase(it);
            bonded_devices[deviceAddress] = bd_name;
        }
       if(it_inquiry!=NULL)
        {
            std::vector<InquiryDB>::iterator it;
            for(it = inquiry_list.begin(); it != inquiry_list.end(); ++it) {
              if(deviceAddress.compare(it_inquiry->address.ToString()) == 0)
                break;
            }
            inquiry_list.erase(it);
            --inq_db_count;
        }

        fprintf(stdout, "\n*************************************************");
        fprintf(stdout, "\n Pairing state for %s is BONDED", bd_name.c_str());
        fprintf(stdout, "\n*************************************************\n");
        g_bt_app->status.pairing_cmd = COMMAND_COMPLETE;

    } else if (new_state == BT_BOND_STATE_NONE) {
        if (it != bonded_devices.end()) {
            bonded_devices.erase(it);
        }
        fprintf(stdout, "\n*************************************************");
        fprintf(stdout, "\n Pairing state for %s is BOND NONE", bd_name.c_str());
        fprintf(stdout, "\n*************************************************\n");
        g_bt_app->status.pairing_cmd = COMMAND_COMPLETE;
    }
}

void BluetoothApp:: HandleUnPair(bt_bdaddr_t bd_addr ) {
    bdstr_t bd_str;
    std::map<std::string, std::string>::iterator it;
    bdaddr_to_string(&bd_addr, &bd_str[0], sizeof(bd_str));
    std::string deviceAddress(bd_str);

    it = bonded_devices.find(deviceAddress);
    if (it != bonded_devices.end())
        bt_interface->remove_bond(&bd_addr);
    else
        fprintf( stdout, " Device is not in bonded list\n");
}

unsigned long long BluetoothApp::getTimeInMilliSec() {
  struct timespec now;
  unsigned long long now_us;
  clock_gettime(CLOCK_MONOTONIC, &now);
  now_us = now.tv_sec * USEC_PER_SEC + now.tv_nsec / 1000;

  return now_us;
}

DeviceProperties* BluetoothApp::inq_db_find_bdaddr(std::string bda) {
  ALOGD(LOGTAG "inq_db_find_bdaddr");
  std::vector<InquiryDB>::iterator it;
  int inq_count = 0;
  for (it = inquiry_list.begin(); it != inquiry_list.end() && inq_count < INQ_DB_SIZE; it++,++inq_count) {
    if (it->in_use &&  bda.compare(it->dp.address.ToString()) == 0) {
      it->time_of_resp = getTimeInMilliSec();
      ALOGD(LOGTAG "inq_db_find_bdaddr : Device Found");
      return &it->dp;
    }
  }
  return NULL;
}

DeviceProperties* BluetoothApp:: inq_db_add_new(DeviceProperties& deviceFound) {
  uint16_t xx;
  InquiryDB p_ent;
  InquiryDB p_old;
  uint32_t ot = 0xFFFFFFFF;
  std::vector<InquiryDB>::iterator it;

  if (inq_db_count < INQ_DB_SIZE) {
    memset(&p_ent, 0, sizeof(InquiryDB));
    memcpy(&p_ent.dp, &deviceFound, sizeof(deviceFound));
    p_ent.in_use = true;
    p_ent.time_of_resp = getTimeInMilliSec();
    ALOGE("inq_db_add_new idx %d bda %s",inq_db_count, deviceFound.address.ToString().c_str());
    inquiry_list.push_back(p_ent);
    ++inq_db_count;
    return (&p_ent.dp);
  } else {

    for (int inq_count = 0; inq_count < inq_db_count && inq_db_count < INQ_DB_SIZE; ++inq_count) {
      p_ent = inquiry_list[inq_count];

      if (p_ent.time_of_resp < ot) {
        p_old = p_ent;
        ot = p_ent.time_of_resp;
      }
    }

    /* If here, no free entry found. Return the oldest. */
    ALOGD("inq_db_add_new old %s replaced by new %s",p_old.dp.address.ToString().c_str(),
                                                  deviceFound.address.ToString().c_str());
    memset(&p_old, 0, sizeof(InquiryDB));
    memcpy(&p_old.dp, &deviceFound, sizeof(deviceFound));
    p_old.in_use = true;
    p_ent.time_of_resp = getTimeInMilliSec();
    return (&p_old.dp);
  }
}

bt_bdaddr_t BluetoothApp:: AddFoundedDevice(DeviceProperties *deviceFound) {

    ALOGI(LOGTAG " Adding Device to inquiry list");
    DeviceProperties *it;
    bdstr_t bd_str;
    bdaddr_to_string(&deviceFound->address, &bd_str[0], sizeof(bd_str));
    std::string deviceAddress(bd_str);

    it = inq_db_find_bdaddr(deviceAddress);
    if (it != NULL) {
        ALOGI(LOGTAG " Device exists %s", deviceFound->address.ToString().c_str());
        return (deviceFound->address);
    } else {
        // add new device in db
        it = inq_db_add_new(*deviceFound);
        return deviceFound->address;
    }
}

bt_state_t BluetoothApp:: GetState() {
    return bt_state;
}

#ifdef SUPPORT_ESL_AP
ap_state_t BluetoothApp:: GetAPState() {
    return ap_state;
}
#endif
void BluetoothApp:: PrintInquiryList() {
    ALOGI(LOGTAG " PrintInquiryList");
    fprintf(stdout, "\n**************************** Inquiry List \
*********************************\n");
    std::vector<InquiryDB>::iterator it;
    int cnt = 1;
    for (it = inquiry_list.begin(); it != inquiry_list.end(); ++it,++cnt)
        fprintf(stdout, "%-*d %-*s %s\n",10,cnt, 50, it->dp.name, it->dp.address.ToString().c_str());
    fprintf(stdout, "**************************** End of List \
*********************************\n");
}


void BluetoothApp:: PrintBondedDeviceList() {

    fprintf(stdout, "\n**************************** Bonded Device List \
**************************** \n");
    std::map<std::string, std::string>::iterator it;
    for (it = bonded_devices.begin(); it != bonded_devices.end(); ++it)
        fprintf(stdout, "%-*s %s\n", 50, it->second.data(), it->first.data());

    fprintf(stdout, "****************************  End of List \
*********************************\n");
}

bool BluetoothApp :: LoadBtStack (void) {
    hw_module_t *module;

    if (hw_get_module (BT_STACK_MODULE_ID, (hw_module_t const **) &module)) {
        ALOGE(LOGTAG "%s hw_get_module failed", BT_STACK_MODULE_ID);
        fprintf(stdout, "%s hw_get_module failed\n", BT_STACK_MODULE_ID);
        return false;
    }

    if (module->methods->open (module, BT_STACK_MODULE_ID, &device_)) {
        fprintf(stdout, "%s open failed\n", BT_STACK_MODULE_ID);
        return false;
    }

    bt_device_ = (bluetooth_device_t *) device_;
    bt_interface = bt_device_->get_bluetooth_interface ();
    if (!bt_interface) {
        bt_device_->common.close ((hw_device_t *) & bt_device_->common);
        bt_device_ = NULL;
        fprintf(stdout, "%s get_bluetooth_interface failed\n", BT_STACK_MODULE_ID);
        return false;
    }
    return true;
}

void BluetoothApp :: UnLoadBtStack (void)
{
    if (bt_interface) {
        bt_interface->cleanup ();
        bt_interface = NULL;
    }

    if (bt_device_) {
        bt_device_->common.close ((hw_device_t *) & bt_device_->common);
        bt_device_ = NULL;
    }
}

#ifdef SUPPORT_ESL_AP
bool BluetoothApp :: LoadAp (void) {
    hw_module_t *module;

    if (hw_get_module (VENDOR_AP_MODULE_ID, (hw_module_t const **) &module)) {
        ALOGE(LOGTAG "%s hw_get_module failed", VENDOR_AP_MODULE_ID);
        fprintf(stdout, "%s hw_get_module failed\n", VENDOR_AP_MODULE_ID);
        return false;
    }

    if (module->methods->open(module, VENDOR_AP_MODULE_ID, &device_)) {
        fprintf(stdout, "%s open failed\n", VENDOR_AP_MODULE_ID);
        return false;
    }

    ap_device_ = (vendor_ap_device_t *) device_;
    ap_interface = ap_device_->get_ap_interface ();
    if (!ap_interface) {
        ap_device_->common.close ((hw_device_t *) & ap_device_->common);
        ap_device_ = NULL;
        fprintf(stdout, "%s get_ap_interface failed\n", VENDOR_AP_MODULE_ID);
        return false;
    }
    return true;
}

void BluetoothApp :: UnLoadAp (void)
{
    if (ap_interface) {
        ap_interface->deInit ();
        ap_interface = NULL;
    }
}
#endif


void BluetoothApp :: InitHandler (void) {

    if (!LoadBtStack()) {
        fprintf(stdout, "Can't load Bt Stack\n");
        kill(getpid(), SIGKILL);
        return;
    }
#ifdef SUPPORT_ESL_AP
    if (!LoadAp()) {
        ALOGE(LOGTAG "Can't load AP module");
        fprintf(stdout, "Can't load AP module\n");
        kill(getpid(), SIGKILL);
    }
#endif
    // Starting GAP Thread
    threadInfo[THREAD_ID_GAP].thread_id = thread_new (
            threadInfo[THREAD_ID_GAP].thread_name);

    if (threadInfo[THREAD_ID_GAP].thread_id) {
        g_gap = new Gap (bt_interface, config);
    }

    if ((is_hfp_client_enabled_) || (is_hfp_ag_enabled_) || (is_a2dp_sink_enabled_)) {
        // we need to start BT-AM if either of A2DP_SINK or HFP-Client or
        // is_hfp_ag_enabled_ is enabled
        threadInfo[THREAD_ID_BT_AM].thread_id = thread_new (
                        threadInfo[THREAD_ID_BT_AM].thread_name);
        if (threadInfo[THREAD_ID_BT_AM].thread_id) {
             pBTAM = new BT_Audio_Manager (bt_interface, config);
        }
    }

    if(is_a2dp_sink_enabled_) {
        if(is_a2dp_sink_split_enabled_){
            thread_id = THREAD_ID_A2DP_SINK_SPLIT;
            threadInfo[THREAD_ID_A2DP_SINK_SPLIT].thread_id = thread_new (
                    threadInfo[THREAD_ID_A2DP_SINK_SPLIT].thread_name);

            if (threadInfo[THREAD_ID_A2DP_SINK_SPLIT].thread_id) {
                pA2dpSinkSplit = new A2dp_Sink_Split (bt_interface, config);
            }
        }else{
            thread_id = THREAD_ID_A2DP_SINK;
            threadInfo[THREAD_ID_A2DP_SINK].thread_id = thread_new (
                    threadInfo[THREAD_ID_A2DP_SINK].thread_name);

            if (threadInfo[THREAD_ID_A2DP_SINK].thread_id) {
                pA2dpSink = new A2dp_Sink (bt_interface, config);
            }
        }
    }

    if(is_avrcp_enabled_) {
        threadInfo[THREAD_ID_AVRCP].thread_id = thread_new (
                threadInfo[THREAD_ID_AVRCP].thread_name);

        if (threadInfo[THREAD_ID_AVRCP].thread_id) {
            pAvrcp = new Avrcp (bt_interface, config);
        }

    }

    if(is_a2dp_source_enabled_) {
        threadInfo[THREAD_ID_A2DP_SOURCE].thread_id = thread_new (
                threadInfo[THREAD_ID_A2DP_SOURCE].thread_name);

        if (threadInfo[THREAD_ID_A2DP_SOURCE].thread_id) {
            pA2dpSource = new A2dp_Source (bt_interface, config);
        }
    }

    if(is_hfp_client_enabled_) {
        threadInfo[THREAD_ID_HFP_CLIENT].thread_id = thread_new (
                threadInfo[THREAD_ID_HFP_CLIENT].thread_name);

        if (threadInfo[THREAD_ID_HFP_CLIENT].thread_id) {
            pHfpClient = new Hfp_Client(bt_interface, config);
        }
    }

    if(is_hfp_ag_enabled_) {
        threadInfo[THREAD_ID_HFP_AG].thread_id = thread_new (
                threadInfo[THREAD_ID_HFP_AG].thread_name);

        if (threadInfo[THREAD_ID_HFP_AG].thread_id) {
            pHfpAG = new Hfp_Ag(bt_interface, config);
        }
    }

    // registers reactors for socket
    if (is_socket_input_enabled_) {
        if(LocalSocketCreate() != -1) {
            listen_reactor_ = reactor_register (
                thread_get_reactor (threadInfo[THREAD_ID_MAIN].thread_id),
                listen_socket_local_, NULL, BtSocketListenHandler, NULL);
        }
    }

    // Enable Bluetooth
    if (is_bt_enable_default_) {

        BtEvent *event = new BtEvent;
        event->event_id = GAP_API_ENABLE;
        ALOGV (LOGTAG "  Posting enable to GAP thread");
        PostMessage (THREAD_ID_GAP, event);

    }

    if (is_bt_enable_autotest)
    {
        fprintf(stdout, "auto test is enabled!\n");
        SendEnableCmdToGap();
    }

    threadInfo[THREAD_ID_SDP_CLIENT].thread_id = thread_new (
        threadInfo[THREAD_ID_SDP_CLIENT].thread_name);

    if (threadInfo[THREAD_ID_SDP_CLIENT].thread_id)
        g_sdpClient = new SdpClient(bt_interface, config);

    if (is_pan_enable_default_) {
        // Starting PAN Thread
        threadInfo[THREAD_ID_PAN].thread_id = thread_new (
            threadInfo[THREAD_ID_PAN].thread_name);

        if (threadInfo[THREAD_ID_PAN].thread_id)
            g_pan = new Pan (bt_interface, config);
    }
#ifdef USE_GEN_GATT
      if (is_gatt_enable_default_) {
          ALOGV (LOGTAG "  Starting GATT thread");
          threadInfo[THREAD_ID_GATT].thread_id = thread_new (
              threadInfo[THREAD_ID_GATT].thread_name);

          if (threadInfo[THREAD_ID_GATT].thread_id)
              g_gatt = GattLibService::getInstance(bt_interface);
      }
#endif

#ifdef USE_BT_OBEX
    if (is_obex_enabled_ && is_pbap_client_enabled_) {
        threadInfo[THREAD_ID_PBAP_CLIENT].thread_id = thread_new (
            threadInfo[THREAD_ID_PBAP_CLIENT].thread_name);

        if (threadInfo[THREAD_ID_PBAP_CLIENT].thread_id)
            g_pbapClient = new PbapClient(bt_interface, config);
    }
    if (is_obex_enabled_ && is_opp_enabled_) {
        threadInfo[THREAD_ID_OPP].thread_id = thread_new (
            threadInfo[THREAD_ID_OPP].thread_name);

        if (threadInfo[THREAD_ID_OPP].thread_id)
            g_opp = new Opp(bt_interface, config);
    }
    opp_incoming_file_accept_timer = NULL;
    if( !(opp_incoming_file_accept_timer = alarm_new())) {
        ALOGE(LOGTAG " unable to create opp_connect_timer");
        opp_incoming_file_accept_timer = NULL;
    }
#endif

    if(is_spp_client_enabled_) {
        threadInfo[THREAD_ID_SPP_CLIENT].thread_id = thread_new (
            threadInfo[THREAD_ID_SPP_CLIENT].thread_name);
       
        if (threadInfo[THREAD_ID_SPP_CLIENT].thread_id)
            pSppClient = new Spp_Client(bt_interface, config);
    }

    if(is_spp_server_enabled_) {
        threadInfo[THREAD_ID_SPP_SERVER].thread_id = thread_new (
            threadInfo[THREAD_ID_SPP_SERVER].thread_name);
       
        if (threadInfo[THREAD_ID_SPP_SERVER].thread_id)
            pSppServer = new Spp_Server(bt_interface, config);
    }

    // Enable Command line input
    if (is_user_input_enabled_) {
        cmd_reactor_ = reactor_register (thread_get_reactor
                        (threadInfo[THREAD_ID_MAIN].thread_id),
                        STDIN_FILENO, NULL, BtCmdHandler, NULL);
    }

    if (is_hid_enable_default_) {
        ALOGV (LOGTAG "  Starting HID thread");
        threadInfo[THREAD_ID_HID].thread_id = thread_new (
            threadInfo[THREAD_ID_HID].thread_name);

        if (threadInfo[THREAD_ID_HID].thread_id)
            pHid = new HidH(bt_interface, config);
    }
    is_hid_enabled = is_hid_enable_default_ ;
    DisplayMenu(MAIN_MENU);
}


void BluetoothApp :: DeInitHandler (void) {

#ifdef SUPPORT_ESL_AP
    UnLoadAp ();
#endif
    if(g_bt_app->bt_state == BT_STATE_ON) {
        UnLoadBtStack ();
    }

    ALOGV (LOGTAG "  %s:",__func__);
    if (is_hid_enable_default_) {
        if (threadInfo[THREAD_ID_HID].thread_id != NULL){
            thread_free(threadInfo[THREAD_ID_HID].thread_id);
            if (pHid != NULL)
                delete pHid;
        }
    }

     // de-register reactors for socket
    if (is_socket_input_enabled_) {
        if(listen_reactor_)
        {
            reactor_unregister ( listen_reactor_);
            listen_reactor_ = NULL;
        }
        if(accept_reactor_)
        {
            reactor_unregister ( accept_reactor_);
            accept_reactor_ = NULL;
        }
    }

    if ((is_hfp_client_enabled_) || (is_hfp_ag_enabled_) ||(is_a2dp_sink_enabled_)) {
        if (threadInfo[THREAD_ID_BT_AM].thread_id != NULL) {
            thread_free (threadInfo[THREAD_ID_BT_AM].thread_id);
            if ( pBTAM != NULL)
                delete pBTAM;
        }
    }

    if(is_a2dp_sink_enabled_) {
        //STOP A2dp Sink thread
        if (threadInfo[THREAD_ID_A2DP_SINK].thread_id != NULL) {
            thread_free (threadInfo[THREAD_ID_A2DP_SINK].thread_id);
            if ( pA2dpSink != NULL)
                delete pA2dpSink;
        }
        if (threadInfo[THREAD_ID_A2DP_SINK_SPLIT].thread_id != NULL) {
            thread_free (threadInfo[THREAD_ID_A2DP_SINK_SPLIT].thread_id);
            if ( pA2dpSinkSplit != NULL)
                delete pA2dpSinkSplit;
        }
    }

    if(is_avrcp_enabled_) {
        //STOP Avrcp thread
        if (threadInfo[THREAD_ID_AVRCP].thread_id != NULL) {
            thread_free (threadInfo[THREAD_ID_AVRCP].thread_id);
            if ( pAvrcp != NULL)
                delete pAvrcp;
        }
    }

    if(is_a2dp_source_enabled_) {
        //STOP A2dp Source thread
        if (threadInfo[THREAD_ID_A2DP_SOURCE].thread_id != NULL) {
            thread_free (threadInfo[THREAD_ID_A2DP_SOURCE].thread_id);
            if ( pA2dpSource!= NULL)
                delete pA2dpSource;
        }
    }

    if(is_hfp_client_enabled_) {
        //STOP HFP client thread
        if (threadInfo[THREAD_ID_HFP_CLIENT].thread_id != NULL) {
            thread_free (threadInfo[THREAD_ID_HFP_CLIENT].thread_id);
            if ( pHfpClient != NULL)
                delete pHfpClient;
        }
    }

    if(is_hfp_ag_enabled_) {
        //STOP HFP AG thread
        if (threadInfo[THREAD_ID_HFP_AG].thread_id != NULL) {
            thread_free (threadInfo[THREAD_ID_HFP_AG].thread_id);
            if ( pHfpAG != NULL)
                delete pHfpAG;
        }
    }

    // Stop GAP Thread
    if (threadInfo[THREAD_ID_GAP].thread_id != NULL) {
        thread_free (threadInfo[THREAD_ID_GAP].thread_id);
        if ( g_gap != NULL)
            delete g_gap;
    }

    // Stop SDP Client Thread
    if (threadInfo[THREAD_ID_SDP_CLIENT].thread_id != NULL) {
        thread_free (threadInfo[THREAD_ID_SDP_CLIENT].thread_id);
        if ( g_sdpClient != NULL)
            delete g_sdpClient;
    }

    if (is_pan_enable_default_) {
        // Stop PAN Thread
        if (threadInfo[THREAD_ID_PAN].thread_id != NULL) {
            thread_free (threadInfo[THREAD_ID_PAN].thread_id);
            if (g_pan != NULL)
                delete g_pan;
        }
    }

    if(is_spp_client_enabled_) {
        if (threadInfo[THREAD_ID_SPP_CLIENT].thread_id !=NULL) {
                        thread_free (threadInfo[THREAD_ID_SPP_CLIENT].thread_id);
            if (pSppClient != NULL)
                delete pSppClient;
        }
    }

    if(is_spp_server_enabled_) {
        if (threadInfo[THREAD_ID_SPP_SERVER].thread_id !=NULL) {
                        thread_free (threadInfo[THREAD_ID_SPP_SERVER].thread_id);
            if (pSppServer != NULL)
                delete pSppServer;
        }
    }


#ifdef USE_GEN_GATT
      if (is_gatt_enable_default_) {
          if (threadInfo[THREAD_ID_GATT].thread_id != NULL){
              thread_free(threadInfo[THREAD_ID_GATT].thread_id);
              if (g_gatt != NULL)
                  delete g_gatt;
          }
      }
#endif

#ifdef USE_BT_OBEX
    if (opp_incoming_file_accept_timer) {
        alarm_free(opp_incoming_file_accept_timer);
        opp_incoming_file_accept_timer = NULL;
    }
    if (is_obex_enabled_ && is_pbap_client_enabled_) {
        // Stop PBAP Client Thread
        if (threadInfo[THREAD_ID_PBAP_CLIENT].thread_id != NULL) {
            thread_free (threadInfo[THREAD_ID_PBAP_CLIENT].thread_id);
            if (g_pbapClient!= NULL)
                delete g_pbapClient;
        }
    }
    if (is_obex_enabled_ && is_opp_enabled_) {
        // Stop Opp Thread
        if (threadInfo[THREAD_ID_OPP].thread_id != NULL) {
            thread_free (threadInfo[THREAD_ID_OPP].thread_id);
            if (g_opp!= NULL)
                delete g_opp;
        }
    }
#endif

    // Stop Command Handler
    if (is_user_input_enabled_) {
        reactor_unregister (cmd_reactor_);
    }
}

BluetoothApp :: BluetoothApp () {

    // Initial values
    is_bt_enable_default_ = false;
    is_bt_enable_autotest = false;
    is_user_input_enabled_ = false;
    ssp_notification = false;
    pin_notification =false;
    listen_socket_local_ = -1;
    client_socket_ = -1;
    cmd_reactor_ = NULL;
    listen_reactor_ = NULL;
    accept_reactor_ = NULL;
    is_bt_enable_test_menu_ = false;

    bt_state = BT_STATE_OFF;
    bt_discovery_state = BT_DISCOVERY_STOPPED;

    memset (&status, '\0', sizeof (UiCommandStatus));
    memset (&inquiry_list,0,sizeof(inquiry_list));
    inq_db_count = 0;
    config = NULL;

    if (!LoadConfigParameters (CONFIG_FILE_PATH))
        ALOGE (LOGTAG " Error in Loading config file");
}

BluetoothApp :: ~BluetoothApp () {
    if (config)
        config_remove(config);

    bonded_devices.clear();
    inquiry_list.clear();
    inq_db_count = 0;
}

int BluetoothApp:: LocalSocketCreate(void) {
  int conn_sk, length;
  struct sockaddr_un addr;

  listen_socket_local_ = socket(AF_LOCAL, SOCK_STREAM, 0);
  if(listen_socket_local_ < 0) {
    ALOGE (LOGTAG "Failed to create Local Socket 1 (%s)", strerror(errno));
    return -1;
  }

  memset(&addr, 0, sizeof(addr));
  addr.sun_family = AF_LOCAL;
  strlcpy(addr.sun_path, LOCAL_SOCKET_NAME, sizeof(addr.sun_path));
  unlink(LOCAL_SOCKET_NAME);
  if (bind(listen_socket_local_, (struct sockaddr*)&addr, sizeof(addr)) == -1) {
    ALOGE (LOGTAG "Failed to create Local Socket (%s)", strerror(errno));
    return -1;
  }

  if (listen(listen_socket_local_, 1) < 0) {
    ALOGE (LOGTAG "Local socket listen failed (%s)", strerror(errno));
    close(listen_socket_local_);
    return -1;
  }
  return listen_socket_local_;
}

bool BluetoothApp::LoadConfigParameters (const char *configpath) {

    bool is_bt_ext_ldo, fw_snoop_enable,soc_log_enable;
    config = config_new (configpath);
    if (!config) {
        ALOGE (LOGTAG " Unable to open config file");
        fprintf(stdout, " Unable to open config file\n");
        return false;
    }
    is_bt_ext_ldo = config_get_bool (config, CONFIG_DEFAULT_SECTION,
                                    BT_ENABLE_EXT_POWER, false);
    if(is_bt_ext_ldo){
        osi_property_set("wc_transport.extldo", "enabled");
    }else{
        osi_property_set("wc_transport.extldo", "disabled");
    }

    fw_snoop_enable = config_get_bool (config, CONFIG_DEFAULT_SECTION,
                                    BT_ENABLE_FW_SNOOP, false);
    if(fw_snoop_enable){
        osi_property_set("persist.service.bdroid.fwsnoop", "true");
    }else{
        osi_property_set("persist.service.bdroid.fwsnoop", "false");
    }

    soc_log_enable = config_get_bool (config, CONFIG_DEFAULT_SECTION,
                                    BT_ENABLE_SOC_LOG, false);
    if(soc_log_enable){
        osi_property_set("persist.service.bdroid.soclog", "true");
    }else{
        osi_property_set("persist.service.bdroid.soclog", "false");
    }

    // checking for the BT Enable option in config file
    is_bt_enable_default_ = config_get_bool (config, CONFIG_DEFAULT_SECTION,
                                    BT_ENABLE_DEFAULT, false);

    // checking for the BT auto test Enable option in config file
    is_bt_enable_autotest = config_get_bool (config, CONFIG_DEFAULT_SECTION,
                                    BT_ENABLE_AUTOTEST, false);

    //checking for user input
    is_user_input_enabled_ = config_get_bool (config, CONFIG_DEFAULT_SECTION,
                                    BT_USER_INPUT, false);
    //checking for socket handler
    is_socket_input_enabled_ = config_get_bool (config, CONFIG_DEFAULT_SECTION,
                                    BT_SOCKET_ENABLED, false);

    //checking for a2dp sink
    is_a2dp_sink_enabled_ = config_get_bool (config, CONFIG_DEFAULT_SECTION,
                                    BT_A2DP_SINK_ENABLED, false);

    //checking for a2dp sink split
    is_a2dp_sink_split_enabled_ = config_get_bool (config, CONFIG_DEFAULT_SECTION,
                                    BT_A2DP_SINK_SPLIT_ENABLED, false);

    //checking for avrcp
    is_avrcp_enabled_ = config_get_bool (config, CONFIG_DEFAULT_SECTION,
                                    BT_AVRCP_ENABLED, false);

    //checking for a2dp source
    is_a2dp_source_enabled_ = config_get_bool (config, CONFIG_DEFAULT_SECTION,
                                    BT_A2DP_SOURCE_ENABLED, false);

    if (is_a2dp_sink_split_enabled_ == true && is_a2dp_source_enabled_ == true) {
        ALOGE (LOGTAG " Both A2dp Src and A2dp Sink are enabled, disabling A2dp Src. Set \
           BtA2dpSourceEnable to true, BtA2dpSinkSplitEnable to false in bt_app.conf to \
           enable only A2dp Source");
        fprintf(stdout, " Both A2dp Src and A2dp Sink are enabled, disabling A2dp Src. Set \n \
           BtA2dpSourceEnable to true, BtA2dpSinkSplitEnable to false in bt_app.conf to \n \
           enable only A2dp Source\n ");
        is_a2dp_source_enabled_ = false;
    }

    if(is_a2dp_source_enabled_) {
        osi_property_set("persist.bt.a2dp_offload_cap","sbc");
    }
    //checking for hfp client
    is_hfp_client_enabled_ = config_get_bool (config, CONFIG_DEFAULT_SECTION,
                                    BT_HFP_CLIENT_ENABLED, false);
    //checking for hfp ag
    is_hfp_ag_enabled_ = config_get_bool (config, CONFIG_DEFAULT_SECTION,
                                    BT_HFP_AG_ENABLED, false);

    //checking for hid
    is_hid_enable_default_ = config_get_bool (config, CONFIG_DEFAULT_SECTION,
                                    BT_HID_ENABLED, false);
    if (is_hfp_client_enabled_ == true && is_hfp_ag_enabled_ == true) {
        ALOGE (LOGTAG " Both HFP AG and Client are enabled, disabling AG. Set \
           BtHfpAGEnable to true, BtHfClientEnable to false in bt_app.conf to \
           enable only AG");
        fprintf(stdout, " Both HFP AG and Client are enabled, disabling AG. Set \n \
           BtHfpAGEnable to true, BtHfClientEnable to false in bt_app.conf to \n \
           enable only AG\n" );
        is_hfp_ag_enabled_ = false;
    }
    //checking for Pan handler
    is_pan_enable_default_ = config_get_bool (config, CONFIG_DEFAULT_SECTION,
                                    BT_PAN_ENABLED, false);

#ifdef USE_GEN_GATT
    //checking for Gatt handler
    is_gatt_enable_default_= config_get_bool (config, CONFIG_DEFAULT_SECTION,
                                    BT_GATT_ENABLED, false);
#endif

#ifdef USE_BT_OBEX
    //checking for OBEX handler
    is_obex_enabled_ = config_get_bool (config, CONFIG_DEFAULT_SECTION,
                                    BT_OBEX_ENABLED, false);

    //checking for Pbap Client handler
    is_pbap_client_enabled_ = config_get_bool (config, CONFIG_DEFAULT_SECTION,
                                    BT_PBAP_CLIENT_ENABLED, false);

    //checking for OPP handler
    is_opp_enabled_ = config_get_bool (config, CONFIG_DEFAULT_SECTION,
                                    BT_OPP_ENABLED, false);
#endif

    //Check for SPP Server
    is_spp_server_enabled_ = config_get_bool (config, CONFIG_DEFAULT_SECTION,
                                    BT_SPP_SERVER_ENABLED, false);
    //Check for SPP client
    is_spp_client_enabled_ = config_get_bool (config, CONFIG_DEFAULT_SECTION,
                                    BT_SPP_CLIENT_ENABLED, false);

    // checking for PTS test enable
    is_pts_test_enabled_ = config_get_bool (config, CONFIG_DEFAULT_SECTION,
        PTS_TEST_ENABLED, false);

    return true;
}

void printBtappState (void) {
    if (g_bt_app) {
        fprintf(stdout, " BT state:%d\n", g_bt_app->bt_state);
#ifdef SUPPORT_ESL_AP
        fprintf(stdout, " AP state:%d\n", g_bt_app->ap_state);
#endif
/*
        fprintf(stdout, " enable_cmd state:%d\n", g_bt_app->status.enable_cmd);
        fprintf(stdout, " disable_cmd state:%d\n", g_bt_app->status.disable_cmd);
        fprintf(stdout, " enquiry_cmd state:%d\n", g_bt_app->status.enquiry_cmd);
        fprintf(stdout, " stop_enquiry_cmd state:%d\n", g_bt_app->status.stop_enquiry_cmd);
        fprintf(stdout, " pairing_cmd state:%d\n", g_bt_app->status.pairing_cmd);
#ifdef SUPPORT_ESL_AP
        fprintf(stdout, " eslap_init_cmd state:%d\n", g_bt_app->status.eslap_init_cmd);
        fprintf(stdout, " eslap_deinit_cmd state:%d\n", g_bt_app->status.eslap_deinit_cmd);
#endif
*/
    }
}