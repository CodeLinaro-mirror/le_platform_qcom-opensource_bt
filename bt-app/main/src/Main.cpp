/******************************************************************************
 *
 *  Copyright (c) 2016, The Linux Foundation. All rights reserved.
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

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <algorithm>
#include <sys/syslog.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <dlfcn.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <hardware/hardware.h>
#include <iostream>
#include <iomanip>
#include "Main.hpp"

#include "utils.h"

#define LOGTAG  "MAIN"
#define LOCAL_SOCKET_NAME "btappsocket"

extern Gap *g_gap;
static BluetoothApp *g_bt_app = NULL;

#ifdef __cplusplus
extern "C"
{
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

    // initialize signal handler
    signal(SIGINT, SignalHandler);

    g_main_thread = thread_new ("Main_Thread");
    if (g_main_thread) {
        BtEvent *event = new BtEvent;
        event->event_id = MAIN_API_INIT;
        ALOGV (LOGTAG " Posting init to Main thread\n");
        PostMessage (THREAD_ID_MAIN, event);

        // wait for Main thread to exit
        thread_join (g_main_thread);
        thread_free (g_main_thread);
    }
    return 0;
}

static bool HandleUserInput (int *cmd_id, char input_args[][COMMAND_ARG_SIZE],
                                                          MenuType menu_type) {
    char user_input[COMMAND_SIZE] = {'\0'};
    int index = 0 , found_index = -1, num_cmds;
    char *temp_arg = NULL;
    char delim[] = " ";
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
        case TEST_MENU:
            menu = &TestMenu[0];
            num_cmds  = NO_OF_COMMANDS(TestMenu);
            break;
        case MAIN_MENU:
        // fallback to default main menu
        default:
            menu = &MainMenu[0];
            num_cmds  = NO_OF_COMMANDS(MainMenu);
            break;
    }

    if ( (temp_arg = strtok(user_input, delim)) != NULL ) {
        // find out the command name
        for (index = 0; index < num_cmds; index++) {
            if(!strcasecmp (menu[index].cmd_name, temp_arg)) {
                *cmd_id = menu[index].cmd_id;
                found_index = index;
                strncpy(input_args[param_count], temp_arg, COMMAND_ARG_SIZE - 1);
                input_args[param_count++][COMMAND_ARG_SIZE - 1] = '\0';
                break;
            }
        }

        // validate the command parameters
        if (found_index != -1 ) {
            max_param = menu[found_index].max_param;
            while ((temp_arg = strtok(NULL, delim)) &&
                    (param_count < max_param + 1)) {
                strncpy(input_args[param_count], temp_arg, COMMAND_ARG_SIZE - 1);
                input_args[param_count++][COMMAND_ARG_SIZE - 1] = '\0';
            }

            // consider command as other param
            if(param_count == max_param + 1) {
                if(temp_arg != NULL) {
                    ALOGV (LOGTAG " Maximum params reached");
                    ALOGV (LOGTAG " Refer help: %s", menu[found_index].cmd_help);
                } else {
                    status = true;
                }
            } else if(param_count < max_param + 1) {
                ALOGV (LOGTAG " Missing required parameters ");
                ALOGV (LOGTAG " Refer help: %s", menu[found_index].cmd_help);
            }
        } else {
            // to handle the paring inputs
            if(temp_arg != NULL) {
                strncpy(input_args[param_count], temp_arg, COMMAND_ARG_SIZE - 1);
                input_args[param_count++][COMMAND_ARG_SIZE - 1] = '\0';
            }
        }
    }
    return status;
}

static void DisplayMenu(MenuType menu_type) {

    UserMenuList *menu = NULL;
    int index = 0, num_cmds = 0;

    switch(menu_type) {
        case GAP_MENU:
            menu = &GapMenu[0];
            num_cmds  = NO_OF_COMMANDS(GapMenu);
            break;
        case TEST_MENU:
            menu = &TestMenu[0];
            num_cmds  = NO_OF_COMMANDS(TestMenu);
            break;
        case MAIN_MENU:
            menu = &MainMenu[0];
            num_cmds  = NO_OF_COMMANDS(MainMenu);
            break;
    }
    fprintf (stdout, " \n***************** Menu *******************\n");
    for (index = 0; index < num_cmds; index++)
        fprintf (stdout, "\t %s \n",  menu[index].cmd_help);
    fprintf (stdout, " ******************************************\n");
}

static void SignalHandler(int sig) {
    ExitHandler();
}

static void ExitHandler(void) {

    // post the disable message to GAP incase BT is on
    if (g_bt_app->bt_state == BT_STATE_ON) {
        BtEvent *event = new BtEvent;
        event->event_id = GAP_API_DISABLE;
        PostMessage (THREAD_ID_GAP, event);
        sleep(1);
    }

    // TODO to wait for complete turn off before proceeding

    if (g_bt_app) {
        BtEvent *event = new BtEvent;
        event->event_id = MAIN_API_DEINIT;
        g_bt_app->ProcessEvent (event);
        delete event;
        delete g_bt_app;
        g_bt_app = NULL;
    }

    // stop the reactor for self exit of main thread
    reactor_stop (thread_get_reactor (g_main_thread));
}

static void HandleMainCommand(int cmd_id, char user_cmd[][COMMAND_ARG_SIZE]) {

    switch (cmd_id) {
        case GAP_OPTION:
            menu_type = GAP_MENU;
            DisplayMenu(menu_type);
            break;

        case TEST_MODE:
            menu_type = TEST_MENU;
            DisplayMenu(menu_type);
            break;

        case MAIN_EXIT:
            ALOGV (LOGTAG " Self exit of Main thread");
            ExitHandler();
            break;

         default:
            ALOGV (LOGTAG " Command not handled");
            break;
    }
}

static void HandleTestCommand(int cmd_id, char user_cmd[][COMMAND_ARG_SIZE]) {

    long num;
    char *end;
    int index = 0;
    switch (cmd_id) {
        case TEST_ON_OFF:
            if ( user_cmd[ONE_PARAM][0] != '\0') {
                errno = 0;
                num = strtol(user_cmd[ONE_PARAM], &end, 0);
                if (*end != '\0' || errno != 0 || num < INT_MIN || num > INT_MAX){
                    ALOGV (LOGTAG " Enter numeric Value");
                    break;
                }
                for( index = 0; index < (int)num; index++){
                    BtEvent *event_on = new BtEvent;
                    event_on->event_id = GAP_API_ENABLE;
                    PostMessage (THREAD_ID_GAP, event_on);
                    sleep(2);
                    BtEvent *event_off = new BtEvent;
                    event_off->event_id = GAP_API_DISABLE;
                    PostMessage (THREAD_ID_GAP, event_off);
                    sleep(2);
                }
           }
           ALOGV (LOGTAG " Currently not Handled %ld ", num);
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

static void HandleGapCommand(int cmd_id, char user_cmd[][COMMAND_ARG_SIZE]) {
    BtEvent *event = NULL;

    switch (cmd_id) {
        case BACK_TO_MAIN:
            menu_type = MAIN_MENU;
            DisplayMenu(menu_type);
            break;

        case BT_ENABLE:
            if ((g_bt_app->status.enable_cmd != COMMAND_INPROGRESS) &&
                (g_bt_app->bt_state == BT_STATE_OFF)) {

                g_bt_app->status.enable_cmd = COMMAND_INPROGRESS;
                BtEvent *event = new BtEvent;

                event->event_id = GAP_API_ENABLE;
                ALOGV (LOGTAG " Posting BT enable to GAP thread");
                PostMessage (THREAD_ID_GAP, event);
            } else if ( g_bt_app->status.enable_cmd == COMMAND_INPROGRESS ) {
                ALOGI (LOGTAG "BT enable is already in process");
            } else {
                 ALOGI (LOGTAG "Currently BT is already in ON state");
            }
            break;

        case BT_DISABLE:

            if ((g_bt_app->status.disable_cmd != COMMAND_INPROGRESS) &&
                                (g_bt_app->bt_state == BT_STATE_ON)) {

                g_bt_app->status.disable_cmd = COMMAND_INPROGRESS;
                event = new BtEvent;
                event->event_id = GAP_API_DISABLE;
                ALOGV (LOGTAG " Posting disable to GAP thread");
                PostMessage (THREAD_ID_GAP, event);
            } else if (g_bt_app->status.disable_cmd == COMMAND_INPROGRESS) {
                ALOGI (LOGTAG " disable command is already in process");
            } else {
                ALOGI (LOGTAG "Currently BT is already in OFF state");
            }
            break;

        case START_ENQUIRY:

            if ((g_bt_app->status.enquiry_cmd != COMMAND_INPROGRESS) &&
                                (g_bt_app->bt_state == BT_STATE_ON)) {

                g_bt_app->status.enquiry_cmd = COMMAND_INPROGRESS;
                event = new BtEvent;
                event->event_id = GAP_API_START_INQUIRY;
                ALOGV (LOGTAG " Posting inquiry to GAP thread");
                PostMessage (THREAD_ID_GAP, event);

            } else if (g_bt_app->status.enquiry_cmd == COMMAND_INPROGRESS) {
                ALOGI (LOGTAG " The inquiry is already in process");

            } else {
                ALOGI (LOGTAG "currently BT is in OFF state");
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

            } else if (g_bt_app->status.stop_enquiry_cmd == COMMAND_INPROGRESS) {
                ALOGI (LOGTAG " The stop inquiry is already in process");

            } else if (g_bt_app->bt_state == BT_STATE_OFF) {
                ALOGI (LOGTAG "currently BT is in OFF state");

            } else if (g_bt_app->bt_discovery_state != BT_DISCOVERY_STARTED) {
                ALOGI (LOGTAG "Inquiry is not started, ignoring the stop inquiry");
            }
            break;

        case START_PAIR:
            if ((g_bt_app->status.pairing_cmd != COMMAND_INPROGRESS) &&
                (g_bt_app->bt_state == BT_STATE_ON)) {
                if (string_is_bdaddr(user_cmd[ONE_PARAM])) {
                    g_bt_app->status.pairing_cmd = COMMAND_INPROGRESS;
                    event = new BtEvent;
                    event->event_id = GAP_API_CREATE_BOND;
                    string_to_bdaddr(user_cmd[ONE_PARAM], &event->bond_device.bd_addr);
                    PostMessage (THREAD_ID_GAP, event);
                } else {
                 ALOGV (LOGTAG " BD address is NULL/Invalid ");
                }
            } else if (g_bt_app->status.pairing_cmd == COMMAND_INPROGRESS) {
                ALOGI (LOGTAG " The Pairing is already in process");
            } else {
                ALOGI (LOGTAG " Currently BT is in OFF state");
            }
            break;

        case UNPAIR:
            if ( g_bt_app->bt_state == BT_STATE_ON ) {
                if (string_is_bdaddr(user_cmd[ONE_PARAM])) {
                    bt_bdaddr_t bd_addr;
                    string_to_bdaddr(user_cmd[ONE_PARAM], &bd_addr);
                    g_bt_app->HandleUnPair(bd_addr);
                } else {
                    ALOGV (LOGTAG " BD address is NULL/Invalid ");
                }
            } else {
                ALOGI (LOGTAG " Currently BT is in OFF state");
            }
            break;

        case INQUIRY_LIST:
            if (!g_bt_app->inquiry_list.empty()) {
                g_bt_app->PrintInquiryList();
            } else {
                ALOGI (LOGTAG " Inquiry list is empty");
            }
            break;

        case BONDED_LIST:
            if (!g_bt_app->bonded_devices.empty()) {
                g_bt_app->PrintBondedDeviceList();
            } else {
                ALOGI (LOGTAG " No bonded devices");
            }
            break;

        case GET_BT_STATE:
           if ( g_bt_app->GetState() == BT_STATE_ON )
               std::cout << " Currently BT is ON" << std::endl;
            else if ( g_bt_app->GetState() == BT_STATE_OFF)
                std::cout <<" Currently BT is OFF "<< std::endl;
            break;

        default:
            ALOGV (LOGTAG " Command not handled");
            break;
    }
}


void BtSocketDataHandler (void *context) {
    //TODO keep the data handler
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
                (thread_get_reactor (g_main_thread),
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
        switch(menu_type) {
            case GAP_MENU:
                HandleGapCommand(cmd_id,user_cmd);
                break;
            case TEST_MENU:
                HandleTestCommand(cmd_id, user_cmd);
                break;
            case MAIN_MENU:
                HandleMainCommand(cmd_id,user_cmd );
                break;
        }
    } else if (g_bt_app->ssp_notification && user_cmd[0][0] &&
                        g_bt_app->HandleSspInput(user_cmd)) {
        // validate the user input for SSP
        g_bt_app->ssp_notification = false;
    } else if (g_bt_app->pin_notification && user_cmd[0][0] &&
                        g_bt_app->HandlePinInput(user_cmd)) {
        // validate the user input for PIN
        g_bt_app->pin_notification = false;
    } else {
        ALOGI (LOGTAG " Wrong option selected");
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
    } else if(strlen(user_cmd[ZERO_PARAM]) >16 ) {
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
    if (!strcasecmp (user_cmd[ZERO_PARAM], "yes")) {
        ssp_data.accept = true;
    }
    else if (!strcasecmp (user_cmd[ZERO_PARAM], "no")) {
        ssp_data.accept = false;
    } else {
        ALOGV (LOGTAG " Wrong option selected");
        return false;
    }

    memcpy(&bt_event->ssp_reply_event.bd_addr, &ssp_data.bd_addr,
                                            sizeof(bt_bdaddr_t));
    memcpy(&bt_event->ssp_reply_event.bd_name, &ssp_data.bd_name, sizeof(bt_bdname_t));
    bt_event->ssp_reply_event.cod = ssp_data.cod;
    bt_event->ssp_reply_event.pairing_variant = ssp_data.pairing_variant;
    bt_event->ssp_reply_event.pass_key = ssp_data.pass_key;
    bt_event->ssp_reply_event.accept = ssp_data.accept;
    bt_event->event_id = GAP_API_SSP_REPLY;
    PostMessage (THREAD_ID_GAP, bt_event);
    return true;
}

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

        case MAIN_EVENT_ENABLED:
            bt_state = event->state_event.status;
            if (event->state_event.status == BT_STATE_OFF) {
                ALOGE (LOGTAG " Error in Enabling BT");
            } else {
               ALOGD (LOGTAG " BT State is ON");
            }
            status.enable_cmd = COMMAND_COMPLETE;
            break;

        case MAIN_EVENT_DISABLED:
            bt_state = event->state_event.status;
            if (event->state_event.status == BT_STATE_ON) {
                ALOGE (LOGTAG " Error in disabling BT");
            } else {
                // clear the inquiry related cmds
                status.enquiry_cmd = COMMAND_COMPLETE;
                status.stop_enquiry_cmd = COMMAND_COMPLETE;
                bt_discovery_state = BT_DISCOVERY_STOPPED;
                ALOGD (LOGTAG " BT State is OFF");
            }
            status.disable_cmd = COMMAND_COMPLETE;
            break;

        case MAIN_EVENT_ACL_CONNECTED:
            ALOGD (LOGTAG " MAIN_EVENT_ACL_CONNECTED");
            break;

        case MAIN_EVENT_ACL_DISCONNECTED:
            ALOGD (LOGTAG " MAIN_EVENT_ACL_DISCONNECTED");
            break;

        case MAIN_EVENT_INQUIRY_STATUS:
            if (event->discovery_state_event.state == BT_DISCOVERY_STARTED) {
                ALOGI (LOGTAG " Inquiry Started");
            } else if (event->discovery_state_event.state == BT_DISCOVERY_STOPPED) {
                if ( status.enquiry_cmd == COMMAND_INPROGRESS) {
                    if ((bt_discovery_state == BT_DISCOVERY_STARTED) &&
                       (status.stop_enquiry_cmd != COMMAND_INPROGRESS))
                        ALOGI (LOGTAG " Inquiry Stopped automatically");
                    else if (bt_discovery_state == BT_DISCOVERY_STOPPED)
                        ALOGI (LOGTAG " Unable to start Inquiry");
                    status.enquiry_cmd = COMMAND_COMPLETE;
                }
                if (status.stop_enquiry_cmd == COMMAND_INPROGRESS) {
                    status.stop_enquiry_cmd = COMMAND_COMPLETE;
                    ALOGI (LOGTAG " Inquiry Stopped due to user input");
                }
            }
            bt_discovery_state = event->discovery_state_event.state;
            break;

        case MAIN_EVENT_DEVICE_FOUND:
            ALOGI (LOGTAG "Device Found details : \n:");
            AddFoundedDevice(event->device_found_event.remoteDevice.name,
                                    event->device_found_event.remoteDevice.address);
            ptr = (event->device_found_event.remoteDevice.address.address);
            ALOGI (LOGTAG "Found device Addr: %02x:%02x:%02x:%02x:%02x:%02x\n ",
                                ptr[0], ptr[1], ptr[2], ptr[3], ptr[4], ptr[5]);
            ALOGI (LOGTAG "Found device Name : %s\n", event->device_found_event.
                                                    remoteDevice.name);
            ALOGI (LOGTAG "Device calss is : %d\n", event->device_found_event.
                                        remoteDevice.bluetooth_class);
            break;

        case MAIN_EVENT_BOND_STATE: {
            std::string bd_name((const char*)event->bond_state_event.bd_name.name);
            HandleBondState(event->bond_state_event.state,
                                    event->bond_state_event.bd_addr, bd_name);
            }
            break;
        case MAIN_EVENT_SSP_REQUEST:
            memcpy(&ssp_data.bd_addr, &event->ssp_request_event.bd_addr, sizeof(bt_bdaddr_t));
            memcpy(&ssp_data.bd_name, &event->ssp_request_event.bd_name, sizeof(bt_bdname_t));
            ssp_data.cod = event->ssp_request_event.cod;
            ssp_data.pairing_variant = event->ssp_request_event.pairing_variant;
            ssp_data.pass_key = event->ssp_request_event.pass_key;
            // instruct the cmd handler to treat the next inputs for SSP
            fprintf(stdout, "\n************************************************************");
            fprintf(stdout, "\n Bluetooth pairing requset :: Device %s :: Pairing Code :: %d",
            ssp_data.bd_name.name, ssp_data.pass_key);
            fprintf(stdout, "\n************************************************************\n");

            fprintf(stdout, " ******* Please enter yes / no for incomming paring ******\n");
            ssp_notification = true;
            break;
        case MAIN_EVENT_PIN_REQUEST:

            memcpy(&pin_reply.bd_addr, &event->pin_request_event.bd_addr, sizeof(bt_bdaddr_t));
            memcpy(&pin_reply.bd_name, &event->pin_request_event.bd_name, sizeof(bt_bdname_t));
            pin_reply.secure = event->pin_request_event.secure;
            // instruct the cmd handler to treat the next inputs for PIN
            pin_notification = true;
            break;
        default:
            ALOGD (LOGTAG " Default Case");
            break;
    }
}

void BluetoothApp:: HandleBondState(bt_bond_state_t new_state, const bt_bdaddr_t bd_addr,
                                                    std::string bd_name ) {
    std::map<std::string, std::string>::iterator it;
    bdstr_t bd_str;
    bdaddr_to_string(&bd_addr, &bd_str[0], sizeof(bd_str));
    std::string deviceAddress(bd_str);
    it = bonded_devices.find(deviceAddress);

    if(new_state == BT_BOND_STATE_BONDED) {
        if (it == bonded_devices.end()) {
            bonded_devices[deviceAddress] = bd_name;
        }
        g_bt_app->status.pairing_cmd = COMMAND_COMPLETE;
    } else if (new_state == BT_BOND_STATE_NONE) {
        if (it != bonded_devices.end()) {
            bonded_devices.erase(it);
        }
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
}

bt_bdaddr_t BluetoothApp:: AddFoundedDevice(std::string bd_name, bt_bdaddr_t bd_addr ) {

    ALOGI(LOGTAG " Adding Device to inquiry list");
    std::map<std::string, std::string>::iterator it;
    bdstr_t bd_str;
    bdaddr_to_string(&bd_addr, &bd_str[0], sizeof(bd_str));
    std::string deviceAddress(bd_str);

    it = inquiry_list.find(deviceAddress);
    if (it != inquiry_list.end()) {
        return (bd_addr);
    } else {
        inquiry_list[deviceAddress] = bd_name;
        return bd_addr;
    }
}

bt_state_t BluetoothApp:: GetState() {
    return bt_state;
}

void BluetoothApp:: PrintInquiryList() {

    std::cout << "\n**************************** Inquiry List \
*********************************\n";
    std::map<std::string, std::string>::iterator it;
    for (it = inquiry_list.begin(); it != inquiry_list.end(); ++it) {
        std::cout << std::left << std::setw(50) << it->second << std::left <<
        std::setw(50) << it->first << std::endl;
    }
    std::cout << "**************************** End of List \
*********************************\n";
}


void BluetoothApp:: PrintBondedDeviceList() {

    std::cout <<"\n**************************** Bonded Device List \
**************************** \n";
    std::map<std::string, std::string>::iterator it;
    for (it = bonded_devices.begin(); it != bonded_devices.end(); ++it) {
            std::cout << std::left << std::setw(50) << it->second
             << std::left << std::setw(50) << it->first << std::endl;
    }
    std::cout<< "****************************  End of List \
*********************************\n";
}

bool BluetoothApp :: LoadBtStack (void) {
    hw_module_t *module;

    if (hw_get_module (BT_STACK_MODULE_ID, (hw_module_t const **) &module)) {
        return false;
    }

    if (module->methods->open (module, BT_STACK_MODULE_ID, &device_)) {
        return false;
    }

    bt_device_ = (bluetooth_device_t *) device_;
    bt_interface = bt_device_->get_bluetooth_interface ();
    if (!bt_interface) {
        bt_device_->common.close ((hw_device_t *) & bt_device_->common);
        bt_device_ = NULL;
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


void BluetoothApp :: InitHandler (void) {

    if (!LoadBtStack())
        return;

    // Starting GAP Thread
    g_gap_thread = thread_new ("Gap_Thread");
    if (g_gap_thread) {
        g_gap = new Gap (bt_interface, config);
    }
    //TODO error handler

    // registers reactors for socket
    if (is_socket_input_enabled_) {
        if(LocalSocketCreate() != -1) {
            listen_reactor_ = reactor_register (thread_get_reactor (g_main_thread),
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

    // Enable Command line input
    if (is_user_input_enabled_) {
        cmd_reactor_ = reactor_register (thread_get_reactor (g_main_thread),
                        STDIN_FILENO, NULL, BtCmdHandler, NULL);
    }
}


void BluetoothApp :: DeInitHandler (void) {
    UnLoadBtStack ();

     // de-register reactors for socket
    if (is_socket_input_enabled_) {
        if(listen_reactor_)
            reactor_unregister ( listen_reactor_);
        if(accept_reactor_)
            reactor_unregister ( accept_reactor_);
    }

    // Stop GAP Thread
    if (g_gap_thread != NULL) {
        thread_free (g_gap_thread);
        if ( g_gap != NULL)
            delete g_gap;
    }

    // Stop Command Handler
    if (is_user_input_enabled_) {
        reactor_unregister (cmd_reactor_);
    }
}


BluetoothApp :: BluetoothApp () {

    // Initial values
    is_bt_enable_default_ = false;
    is_user_input_enabled_ = false;
    ssp_notification = false;
    pin_notification =false;
    listen_socket_local_ = -1;
    client_socket_ = -1;
    cmd_reactor_ = NULL;
    listen_reactor_ = NULL;
    accept_reactor_ = NULL;

    bt_state = BT_STATE_OFF;
    bt_discovery_state = BT_DISCOVERY_STOPPED;

    memset (&status, '\0', sizeof (UiCommandStatus));
    config = NULL;

    if (!LoadConfigParameters (CONFIG_FILE_PATH))
        ALOGE (LOGTAG " Error in Loading config file");
}


BluetoothApp :: ~BluetoothApp () {
    if (config)
        config_free(config);
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
  strncpy(addr.sun_path, LOCAL_SOCKET_NAME, sizeof(addr.sun_path)-1);
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

    config = config_new (configpath);
    if (!config) {
        ALOGE (LOGTAG " Unable to open config file");
        return false;
    }

    // checking for the BT Enable option in config file
    is_bt_enable_default_ = config_get_bool (config, CONFIG_DEFAULT_SECTION,
                                    BT_ENABLE_DEFAULT, false);

    //checking for user input
    is_user_input_enabled_ = config_get_bool (config, CONFIG_DEFAULT_SECTION,
                                    BT_USER_INPUT, false);
    //checking for socket handler
    is_socket_input_enabled_ = config_get_bool (config, CONFIG_DEFAULT_SECTION,
                                    BT_SOCKET_ENABLED, false);
    return true;
}
