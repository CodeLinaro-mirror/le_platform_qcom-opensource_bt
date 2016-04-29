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

#ifndef BT_APP_HPP
#define BT_APP_HPP

#include "osi/include/thread.h"
#include "osi/include/reactor.h"
#include "osi/include/alarm.h"
#include "osi/include/config.h"
#include "gap/include/Gap.hpp"
#include <hardware/bluetooth.h>
#include "include/ipc.h"


#define SUCCESS              1
#define FAILURE             -1
#define COMMAND_ARG_SIZE    100
#define COMMAND_SIZE        200
#define MAX_ARGUMENTS        20 //TODO

const char *BT_APP_GAP_ENABLED     = "BtAppGapEnabled";
const char *BT_ENABLE_DEFAULT        = "BtEnableByDefault";
const char *BT_USER_INPUT            = "UserInteractionNeeded";
const char *CONFIG_FILE_PATH         = "/etc/bluetooth/bt_app.conf";


typedef enum  {
    COMMAND_NONE = 0,
    COMMAND_INPROGRESS,
    COMMAND_COMPLETE,
} CommandStatus;

typedef struct {
    CommandStatus enable_cmd;
    CommandStatus enquiry_cmd;
    CommandStatus stop_enquiry_cmd;
    CommandStatus disable_cmd;
} UiCommandStatus;


typedef enum {
    BT_ENABLE = 1,
    BT_DISABLE,
    START_ENQUIRY,
    CANCEL_ENQUIRY,
    MAIN_EXIT,
    START_PAIR,
    NO_INPUT,
    END,
} CommandList;

typedef struct {
    CommandList number;
    const char ui_option[COMMAND_SIZE];
} UIuseroption;

//TODO cancel inquiry
UIuseroption InputOptions[] = {
    {BT_ENABLE,                 "enable"},
    {BT_DISABLE,                "disable"},
    {START_ENQUIRY,             "inquiry"},
    {CANCEL_ENQUIRY,            "cancel_inquiry"},
    {MAIN_EXIT,                 "exit"},
    {START_PAIR,                "pair"},
    {NO_INPUT,                  NULL},
};

class BluetoothApp {
  private:
    config_t *config;
    bool is_bt_enable_default_;
    bool is_user_input_enabled_;
    reactor_object_t *cmd_reactor_;
    struct hw_device_t *device_;
    bluetooth_device_t *bt_device_;
    int LoadConfigParameters(const char *configpath);
    void InitHandler();
    void DeInitHandler();
    bool LoadBtStack();
    void UnLoadBtStack();


  public:
    UiCommandStatus status;
    const bt_interface_t *bt_interface;
    bt_state_t bt_state;
    bt_discovery_state_t bt_discovery_state;
    BluetoothApp();
    ~BluetoothApp();
    void ProcessEvent(BtEvent * pEvent);
};

#endif
