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

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/syslog.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <dlfcn.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <hardware/bluetooth.h>
#include <hardware/hardware.h>

#include "Main.hpp"


#define LOGTAG  "MAIN"

extern Gap *g_gap;
static BluetoothApp *g_bt_app = NULL;

#ifdef __cplusplus
extern "C"
{
#endif


int main (int argc, char *argv[]) {

    g_main_thread = thread_new ("Main_Thread");
    if (g_main_thread) {
        BtEvent *event = new BtEvent;
        event->event_id = MAIN_API_INIT;
        ALOGV (LOGTAG " Posting init to UI thread\n");
        PostMessage (THREAD_ID_MAIN, event);

        // wait for UI thread to exit
        thread_join (g_main_thread);
        thread_free (g_main_thread);
    }
    return SUCCESS;
}


static int ProcessUserInput (int *number) {

    char uiCommand[COMMAND_SIZE] = { '\0' };
    int index = 0;

    if ((fgets (uiCommand, sizeof (uiCommand), stdin)) != 0) {
        uiCommand[strlen (uiCommand) - 1] = '\0';

        for (index = 0; index < (END - 1); index++) {
            if (strcasecmp (InputOptions[index].ui_option, uiCommand) == 0) {
                *number = InputOptions[index].number;
                return SUCCESS;
            }
        }
    }
    return FAILURE;
}


static void BtCmdHandler (void *context) {

    int user_option = -1;
    BtEvent *event = NULL;

    if (ProcessUserInput (&user_option) == FAILURE) {
        ALOGI (LOGTAG " Wrong option selected");
        return;
    }

    switch (user_option) {

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

        case MAIN_EXIT:

            ALOGV (LOGTAG " Self exit of UI thread");
            reactor_stop (thread_get_reactor (g_main_thread));

            if (g_bt_app) {
                BtEvent *event = new BtEvent;
                event->event_id = MAIN_API_DEINIT;
                g_bt_app->ProcessEvent (event);
                delete event;
                delete g_bt_app;
                g_bt_app = NULL;
            }
            break;

        case NO_INPUT:
            ALOGV (LOGTAG " Nothing to read");
            break;

        default:
            ALOGV (LOGTAG " Command not handled");
            break;
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
            ptr = (event->device_found_event.remoteDevice.address.address);
            ALOGI (LOGTAG "Found device Addr: %02x:%02x:%02x:%02x:%02x:%02x\n ",
                                ptr[0], ptr[1], ptr[2], ptr[3], ptr[4], ptr[5]);
            ALOGI (LOGTAG "Found device Name : %s\n", event->device_found_event.
                                                    remoteDevice.mName);
            ALOGI (LOGTAG "Device calss is : %d\n", event->device_found_event.
                                        remoteDevice.mBluetoothClass);
            break;

        default:
            ALOGD (LOGTAG " DEFAULT CASE");
            break;
    }
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

    // Stop GAP Thread
    if (g_gap_thread != NULL) {
        thread_free (g_gap_thread);
        if ( g_gap != NULL)
            delete g_gap;
    }


    if (is_user_input_enabled_) {
        reactor_unregister (cmd_reactor_);
    }
}


BluetoothApp :: BluetoothApp () {

    // Intiall values
    is_bt_enable_default_ = false;
    is_user_input_enabled_ = false;

    bt_state = BT_STATE_OFF;
    bt_discovery_state = BT_DISCOVERY_STOPPED;

    memset (&status, '\0', sizeof (UiCommandStatus));
    config = NULL;

    if (SUCCESS != LoadConfigParameters (CONFIG_FILE_PATH))
        ALOGE (LOGTAG " Error in Loading config file");
}


BluetoothApp :: ~BluetoothApp () {
    if (config)
        config_free(config);
}


int BluetoothApp::LoadConfigParameters (const char *configpath) {

    config = config_new (configpath);
    if (!config) {
        ALOGE (LOGTAG " Unable to open config file");
        return FAILURE;
    }

    // checking for the BT Enable option in config file
    is_bt_enable_default_ = config_get_bool (config, CONFIG_DEFAULT_SECTION,
                                    BT_ENABLE_DEFAULT, false);

    //checking for user input
    is_user_input_enabled_ = config_get_bool (config, CONFIG_DEFAULT_SECTION,
                                    BT_USER_INPUT, false);
    return SUCCESS;
}
