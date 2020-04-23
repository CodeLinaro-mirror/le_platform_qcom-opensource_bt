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

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
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
#ifdef USE_GEN_GATT
#include "GattLibService.hpp"
#include "GattcTest.hpp"
#include "GattsTest.hpp"
#include "Rsp.hpp"
#endif
#include "HfpAG.hpp"

#include "osi/include/compat.h"
#include <cutils/properties.h>

#include "utils.h"
#include "GattNativeInterfaceV2_b.hpp"
#include "sdbus_ipc.h"

#ifdef USE_GEN_GATT
using namespace gatt;
using namespace btapp;
#endif
#define LOGTAG  "MAIN "
#define LOCAL_SOCKET_NAME "/data/misc/bluetooth/btappsocket"
int server_num;
bool file_read = 0;

extern Hfp_Ag *pHfpAG;
bool gattsEnabled = false;

static BluetoothApp *g_bt_app = NULL;
extern QThreadInfo threadInfo[THREAD_ID_MAX];

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

#ifdef __cplusplus
extern "C"
{
#endif

thread_t *test_thread_id = NULL;
ThreadIdType thread_id = THREAD_ID_MAX; //thread id to handle sink non-split,split
#define BTIPC_MODULE_ID "bluetoothipc"

const gatt_native_interface_v2b_t *g_gatt_native_interface_v2b = NULL;

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
#define GATTSERVER_LOOP_TEST
#define TEST_DIRECT_UNREGISTER_GATTSERVER
#ifdef GATTSERVER_LOOP_TEST

static int g_serverIf = 0;

class ServerCallbackTest :public GattServerCallback
{

  public:
  virtual void onServerRegistered (int status, int serverIf){
    printf("serverIf:%d is registered with status:%d]n", serverIf, status);
    g_serverIf = serverIf;
  }

  void onConnectionStateChange(string deviceAddress, int status, int newState){}
  void onServiceAdded(int status,GattService *service){}
  void onCharacteristicReadRequest(string deviceAddress, int requestId, int offset,
                                      GattCharacteristic *characteristic){}
  void onCharacteristicWriteRequest(string deviceAddress,int requestId,
                                      GattCharacteristic *characteristic,bool preparedWrite,
                                      bool responseNeeded,int offset,uint8_t* value, int length){}
  void onDescriptorReadRequest(string deviceAddress, int requestId, int offset,
                                      GattDescriptor *descriptor){}
  void onDescriptorWriteRequest(string deviceAddress, int requestId,
                                      GattDescriptor *descriptor,bool preparedWrite,
                                      bool responseNeeded, int offset, uint8_t * value, int length){}
  void onExecuteWrite(string deviceAddress, int requestId, bool execute){}
  void onNotificationSent(string deviceAddress, int status){}
  void onMtuChanged(string deviceAddress, int mtu){}
  void onPhyUpdate(string deviceAddress,int txPhy, int rxPhy, int status){}
  void onPhyRead(string deviceAddress,int txPhy,int rxPhy,int status){}
  void onConnectionUpdated(string deviceAddress,int interval,int latency,
                                    int timeout,int status){}
};


class AdvertiserCallbackTest  :public AdvertisingSetCallback
{
  public:
  void onAdvertisingSetStarted (AdvertisingSet *advertisingSet, int txPower, int status) {
    ALOGD(LOGTAG"%s status: %d  txpower: %d", __FUNCTION__, status, txPower);
  }

  void  onAdvertisingDataSet(AdvertisingSet *advertisingset,int status)
  {
    ALOGD(LOGTAG"%s status: %d", __FUNCTION__, status);
  }

  void onAdvertisingSetStopped (AdvertisingSet *advertisingSet)
  {
    ALOGD(LOGTAG"%s Advertiser ID  %d",__FUNCTION__,advertisingSet->getAdvertiserId());
  }

  void onAdvertisingEnabled (AdvertisingSet *advertisingSet, bool enable, int status)
  {
    ALOGD(LOGTAG"%s  enable: %d status %d",__FUNCTION__,enable,status);
  }

  void onScanResponseDataSet (AdvertisingSet *advertisingSet, int status)
  {
    ALOGD(LOGTAG"onScanResponseDataSet status: %d", status);
  }

  void onAdvertisingParametersUpdated (AdvertisingSet *advertisingSet, int txPower, int status)
  {
    ALOGD(LOGTAG"onAdvertisingParametersUpdated txpower: %d status %d", txPower, status);
  }

  void onPeriodicAdvertisingParametersUpdated (AdvertisingSet *advertisingSet, int status)
  {
    ALOGD(LOGTAG"onPeriodicParametersUpdated  status: %d", status);
  }

  void onPeriodicAdvertisingDataSet (AdvertisingSet *advertisingSet, int status)
  {
    ALOGD(LOGTAG"onPeriodicAAdvertisingDataSet status: %d", status);
  }

  void onPeriodicAdvertisingEnabled (AdvertisingSet *advertisingSet, bool enable, int status)
  {
    ALOGD(LOGTAG"onPeriodicAdvertisingEnabled enable : %d status: %d Advertiser id: %d", enable,
                                                      status, advertisingSet->getAdvertiserId());
  }

  void onOwnAddressRead (AdvertisingSet *advertisingSet, int addressType, string address)
  {
    ALOGD(LOGTAG"onOwnAddressRead  addressType: %d  address: %s advertiser id: %d", addressType, 
                                                address.c_str(), advertisingSet->getAdvertiserId());
  }

  void onStartSuccess(AdvertiseSettings *settingsInEffect)
  {
    ALOGD(LOGTAG "onStartSuccess()");
  }

  void onStartFailure(int errorCode)
  {
    ALOGE(LOGTAG "onStartFailure() %d", errorCode);
  }

};


const Uuid TEST_SERVICE_UUID = Uuid::FromString("0000AA01-0000-1000-8000-00805f9b34fb");
const Uuid TEST_CHAR_UUID = Uuid::FromString("0000BB02-0000-1000-8000-00805f9b34fb");

static GattLibService *gattLibService_;
static ServerCallbackTest *peripheralCb_;
static GattServer * gattServer_;
static GattService *mService;
static GattCharacteristic *g_char;
static AdvertiseSettings *advertiseSettings_;
static AdvertiseData *advertiseData_;
static AdvertiserCallbackTest *advertiserCb_;
static GattLeAdvertiser *gattLeAdvertiser_;

void gattserver_load (void)
{
  g_gatt_native_interface_v2b = &GattNativeInterfaceV2bImplInst;
  threadInfo[THREAD_ID_GATT].thread_id = thread_new(threadInfo[THREAD_ID_GATT].thread_name);
  gattLibService_ = GattLibService::getInstance(NULL); //
#if 1  // seg-fault - 136 times
  peripheralCb_ = new ServerCallbackTest;
  gattServer_ = new GattServer(gattLibService_, 0);
  gattServer_->registerCallback(*peripheralCb_);
#endif  
#if 1  
  mService = new GattService(TEST_SERVICE_UUID,GattService::SERVICE_TYPE_PRIMARY);
  int property = 2;
  int permissions = 1;
  g_char = new GattCharacteristic(TEST_CHAR_UUID,property,permissions);
  g_char->setValue(reinterpret_cast<uint8_t*>(const_cast<char*>("hello")), 1);
  mService->addCharacteristic(g_char);
  gattServer_->addService(*mService);
  gattLeAdvertiser_ = GattLeAdvertiser::getGattLeAdvertiser();
#endif
#if 1 //-- seg fault  
  advertiseSettings_ = AdvertiseSettings::Builder()
  .setAdvertiseMode(AdvertiseSettings::ADVERTISE_MODE_BALANCED)
  .setConnectable(true)
  .setTimeout(0)
  .setTxPowerLevel(AdvertiseSettings::ADVERTISE_TX_POWER_MEDIUM)
  .build();
  
  AdvertiseData::Builder builder = AdvertiseData::Builder()
  .setIncludeDeviceName(true)
  .addServiceUuid(TEST_SERVICE_UUID);
  advertiseData_ = builder.build();
  
  advertiserCb_ = new AdvertiserCallbackTest();
  gattLeAdvertiser_->startAdvertising(advertiseSettings_, advertiseData_, NULL, advertiserCb_);
#endif

}

void gattserver_unload (void)
{
#if 1
  //stopping Gatt Service
  if (gattLeAdvertiser_) gattLeAdvertiser_->stopAdvertising(advertiserCb_);

  if (advertiserCb_) delete advertiserCb_;
  advertiserCb_ = NULL;
  if (advertiseData_) delete advertiseData_;
  advertiseData_ = NULL;
  if (advertiseSettings_) delete advertiseSettings_;
  advertiseSettings_ = NULL;
#endif

#if 1
  if (gattLeAdvertiser_) delete gattLeAdvertiser_; // I added this
  gattLeAdvertiser_ = NULL;
  
#endif


#if 1
  if (gattServer_) {
    gattServer_->clearServices();
    gattServer_->close();
    //it is deleted in Service desructor - delete gchar;
    //it is deleted in addService call  - delete mService;
    delete gattServer_;
  }
  gattServer_ = NULL;
  if (peripheralCb_) delete peripheralCb_;
  peripheralCb_ = NULL;
  
#endif  
  if (gattLibService_) delete gattLibService_; // I added this
  gattLibService_ = NULL;

  if (threadInfo[THREAD_ID_GATT].thread_id != NULL)
    thread_free(threadInfo[THREAD_ID_GATT].thread_id);
  threadInfo[THREAD_ID_GATT].thread_id = NULL;
}



void gattserver_loop_test (void) {

  printf("\niniailizing and pending for 10 seconds\n");

  open_sdbus_ipc();
  gattserver_load();

  for (int i=0; i<20; i++) {
    sleep(1); //
    printf("*\n");
  }
  printf("\nclosing and pending for 10 seconds\n");

  gattserver_unload();
  close_sdbus_ipc();

  for (int i=0; i<10; i++) {
    sleep(1); //
    printf(".\n");
  }
}


int loopback_test = 0;
int unregister_test = 0;

static void sigint_handler(int sig) {
  printf("\nclosing by SIGINT...\n");
  signal(SIGINT, SIG_IGN);
  if (unregister_test) {
    printf("try to unregister %d\n", g_serverIf);
    if (g_serverIf != 0) g_gatt_native_interface_v2b->gattServerUnregisterAppNative(g_serverIf);
  } else {
    gattserver_unload();
  }
  close_sdbus_ipc();
  exit(0);
}
#endif


int main (int argc, char *argv[]) {

#ifdef GATTSERVER_LOOP_TEST
  if (2 <= argc) {
    for (int i; i<argc; i++){
      if (argv[i][0] == '-') {
        if (argv[i][1] == 't') {
          loopback_test = 1;
          printf("\nloopback test\n");
        } else if (argv[i][1] == 'u') {
          unregister_test = 1;
          printf("\nunregister server, client test\n");
        }
      }
    }
  }
#endif

  if (loopback_test)
  {
    int count = 0;
    // initialize signal handler
    signal(SIGINT, sigint_handler);
    while (1) 
    {
      printf("test #%d\n", count++);
      gattserver_loop_test();
    }
    return 0;
  }

    // initialize signal handler
    signal(SIGINT, SignalHandler);

    QThreadInfo *main_thread = &threadInfo[THREAD_ID_MAIN];
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
#ifdef USE_GEN_GATT
        case GATTC_TEST_MENU:
            menu = &GattcTestMenu[0];
            num_cmds  = NO_OF_COMMANDS(GattcTestMenu);
            break;
        case GATTSTEST_MENU:
            menu = &GattsTestMenu[0];
            num_cmds  = NO_OF_COMMANDS(GattsTestMenu);
            break;
#endif
        case HFP_AG_MENU:
            menu = &HfpAGMenu[0];
            num_cmds  = NO_OF_COMMANDS(HfpAGMenu);
            break;
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

    switch(menu_type) {
		
#ifdef USE_GEN_GATT
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

        case HFP_AG_MENU:
            menu = &HfpAGMenu[0];
            num_cmds  = NO_OF_COMMANDS(HfpAGMenu);
            break;
    }
    fprintf (stdout, " \n***************** Menu *******************\n");
    for (index = 0; index < num_cmds; index++)
        fprintf (stdout, "\t %s \n",  menu[index].cmd_help);
    fprintf (stdout, " ******************************************\n");
}

static void SignalHandler(int sig) {
    signal(SIGINT, SIG_IGN);
    ExitHandler();
}

static void ExitHandler(void) {

    if (g_bt_app) {
        BtEvent *event = new BtEvent;
        event->event_id = MAIN_API_DEINIT;
        g_bt_app->ProcessEvent (event);
        delete event;
        delete g_bt_app;
        g_bt_app = NULL;
    }

    // stop the reactor for self exit of main thread
    reactor_stop (thread_get_reactor (threadInfo[THREAD_ID_MAIN].thread_id));
}

static void HandleHfpAGCommand(int cmd_id, char user_cmd[][COMMAND_ARG_SIZE]) {
    ALOGD(LOGTAG, "HandleHfpAGCommand cmd_id = %d", cmd_id);
    fprintf(stdout, "HandleHfpAGCommand cmd_id = %d\n" , cmd_id);
    BtEvent *event = NULL;
    switch (cmd_id) {
        case CONNECT:
            event = new BtEvent;
            event->hfp_ag_event.event_id = HFP_AG_API_CONNECT_REQ;
            string_to_bdaddr(user_cmd[ONE_PARAM], &event->hfp_ag_event.bd_addr);
            PostMessage (THREAD_ID_HFP_AG, event);
            break;
        case DISCONNECT:
            event = new BtEvent;
            event->hfp_ag_event.event_id = HFP_AG_API_DISCONNECT_REQ;
            string_to_bdaddr(user_cmd[ONE_PARAM], &event->hfp_ag_event.bd_addr);
            PostMessage (THREAD_ID_HFP_AG, event);
            break;
        case CREATE_SCO_CONN:
            event = new BtEvent;
            event->hfp_ag_event.event_id = HFP_AG_API_CONNECT_AUDIO_REQ;
            string_to_bdaddr(user_cmd[ONE_PARAM], &event->hfp_ag_event.bd_addr);
            PostMessage (THREAD_ID_HFP_AG, event);
            break;
        case DESTROY_SCO_CONN:
            event = new BtEvent;
            event->hfp_ag_event.event_id = HFP_AG_API_DISCONNECT_AUDIO_REQ;
            string_to_bdaddr(user_cmd[ONE_PARAM], &event->hfp_ag_event.bd_addr);
            PostMessage (THREAD_ID_HFP_AG, event);
            break;
        case VOIP_CALL_IND:
            event = new BtEvent;
            event->hfp_ag_event.event_id = HFP_AG_VOIP_CALL_INDICATION;
            string_to_bdaddr(user_cmd[ONE_PARAM], &event->hfp_ag_event.bd_addr);
            PostMessage (THREAD_ID_HFP_AG, event);
            break;
        case END_VOIP_CALL:
            event = new BtEvent;
            event->hfp_ag_event.event_id = HFP_AG_VOIP_CALL_TERMINATION;
            string_to_bdaddr(user_cmd[ONE_PARAM], &event->hfp_ag_event.bd_addr);
            PostMessage (THREAD_ID_HFP_AG, event);
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
            strlcpy(event->hfp_ag_event.str, user_cmd[TWO_PARAM], 20+1);
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
            strlcpy(event->hfp_ag_event.str, user_cmd[ONE_PARAM], 20+1);
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
            strlcpy(event->hfp_ag_event.str, user_cmd[ONE_PARAM], 20+1);
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
    }
}

static void HandleMainCommand(int cmd_id, char user_cmd[][COMMAND_ARG_SIZE]) {

    switch (cmd_id) {
        case GAP_OPTION:
            menu_type = GAP_MENU;
            DisplayMenu(menu_type);
            break;
        case RSP_OPTION:
            menu_type = RSP_MENU;
            DisplayMenu(menu_type);
            break;
        case GATTCTEST_OPTION:
            menu_type = GATTC_TEST_MENU;
            DisplayMenu(menu_type);
            break;
        case GATTSTEST_OPTION:
            menu_type = GATTSTEST_MENU;
            DisplayMenu(menu_type);
            break;

        case HFP_AG:
            menu_type = HFP_AG_MENU;
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
            if (string_is_bdaddr(user_cmd[THREE_PARAM])) {
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
                    gattctest->setPreferredPhy(atoi(user_cmd[ONE_PARAM]),
                        atoi(user_cmd[TWO_PARAM]), 1, user_cmd[THREE_PARAM]);
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
    static bool init_advertiser_file = 0;
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
                    int phyOption = AdvertisingSetParameters::PHY_OPTION_NO_PREFERRED;
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

static void BtCmdHandler (void *context) {

    int cmd_id = -1;
    char user_cmd[MAX_ARGUMENTS][COMMAND_ARG_SIZE];
    int index = 0;
    memset( (void *) user_cmd, '\0', sizeof(user_cmd));

    if (HandleUserInput (&cmd_id, user_cmd, menu_type)) {
        ALOGI (LOGTAG "BtCmdHandler menu_type:%d cmd_id:%d", menu_type, cmd_id);
        switch(menu_type) {
#ifdef USE_GEN_GATT
            case GATTC_TEST_MENU:
                fprintf(stdout, "BtCmdHandler GATTC_TEST_MENU");
                HandleGattcTestCommand(cmd_id, user_cmd);
                break;
            case GATTSTEST_MENU:
                HandleGattsTestCommand(cmd_id, user_cmd);
                break;
#endif
            case MAIN_MENU:
                HandleMainCommand(cmd_id,user_cmd );
                break;
            case HFP_AG_MENU:
                HandleHfpAGCommand(cmd_id, user_cmd );
                break;
        }
   } 
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

        default:
            ALOGD (LOGTAG " Event ID: %x - not handled", event->event_id);
            break;
    }
}


const gatt_native_interface_v2b_t *get_gatt_native_interface_v2b_inst (void)
{
	return g_gatt_native_interface_v2b;
}

bool BluetoothApp :: LoadBtStack (void) {
	if (!open_sdbus_ipc())
	{
	  ALOGE(LOGTAG "(%s) Failed to open sd-bus", __func__);
	  return false;
	}

    g_gatt_native_interface_v2b = &GattNativeInterfaceV2bImplInst;
    fprintf(stdout, "%s : set gatt native interface V2 directly\n", __FUNCTION__);

	bt_state = BT_STATE_ON;
	fprintf(stdout, "%s : set bt_state %d\n", __FUNCTION__, bt_state);

	return true;
}

void BluetoothApp :: UnLoadBtStack (void)
{
    // close ipc 
    close_sdbus_ipc();
}

void BluetoothApp :: InitHandler (void) {

	BtEvent *bt_event;
    LoadBtStack();

    if (is_gatt_enable_default_) {
        ALOGV (LOGTAG "  Starting GATT thread");
        threadInfo[THREAD_ID_GATT].thread_id = thread_new (
            threadInfo[THREAD_ID_GATT].thread_name);
  
        if (threadInfo[THREAD_ID_GATT].thread_id)
            g_gatt = GattLibService::getInstance(bt_interface);
    }

    if(is_hfp_ag_enabled_) {
        threadInfo[THREAD_ID_HFP_AG].thread_id = thread_new (
                threadInfo[THREAD_ID_HFP_AG].thread_name);

        if (threadInfo[THREAD_ID_HFP_AG].thread_id) {
            pHfpAG = new Hfp_Ag(bt_interface, config);

			bt_event = new BtEvent;
			bt_event->event_id = PROFILE_API_START;
			PostMessage(THREAD_ID_HFP_AG, bt_event);

        }
    }

    // Enable Command line input
    if (is_user_input_enabled_) {
        cmd_reactor_ = reactor_register (thread_get_reactor
                        (threadInfo[THREAD_ID_MAIN].thread_id),
                        STDIN_FILENO, NULL, BtCmdHandler, NULL);
    }

}

void BluetoothApp :: DeInitHandler (void) {

	ALOGV (LOGTAG "  %s:",__func__);

	if(is_hfp_ag_enabled_) {
	  //STOP HFP AG thread
	  if (threadInfo[THREAD_ID_HFP_AG].thread_id != NULL) {
	      thread_free (threadInfo[THREAD_ID_HFP_AG].thread_id);
	      if ( pHfpAG != NULL)
	          delete pHfpAG;
	  }
	}

	if (is_gatt_enable_default_) {
	  if (threadInfo[THREAD_ID_GATT].thread_id != NULL){
	      thread_free(threadInfo[THREAD_ID_GATT].thread_id);
	      if (g_gatt != NULL)
	          delete g_gatt;
	  }
	}

	// Stop Command Handler
	if (is_user_input_enabled_) {
	  reactor_unregister (cmd_reactor_);
	}

	UnLoadBtStack ();
}

BluetoothApp :: BluetoothApp () {

    // Initial values
    is_bt_enable_default_ = false;
    is_bt_enable_autotest = false;
    is_user_input_enabled_ = true;
	
    ssp_notification = false;
    pin_notification =false;
    listen_socket_local_ = -1;
    client_socket_ = -1;
    cmd_reactor_ = NULL;
    listen_reactor_ = NULL;
    accept_reactor_ = NULL;

    bt_state = BT_STATE_ON;
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

bool BluetoothApp::LoadConfigParameters (const char *configpath) {

    bool is_bt_ext_ldo, fw_snoop_enable,soc_log_enable;

    // checking for the BT auto test Enable option in config file
    is_bt_enable_autotest = false;
    //checking for user input // console 
    is_user_input_enabled_ = true;
    //checking for hfp ag
    is_hfp_ag_enabled_ = true; //config_get_bool (config, CONFIG_DEFAULT_SECTION, BT_HFP_AG_ENABLED, false);

#ifdef USE_GEN_GATT
    //checking for Gatt handler
    is_gatt_enable_default_= true; //config_get_bool (config, CONFIG_DEFAULT_SECTION, BT_GATT_ENABLED, false);
#endif

    is_bt_ext_ldo = true;
    fw_snoop_enable = false;
    soc_log_enable = false;
    
    config = config_new (configpath);
    if (!config) {
        ALOGE (LOGTAG " Unable to open config file");
        return false;
    }
    is_bt_ext_ldo = config_get_bool (config, CONFIG_DEFAULT_SECTION,
                                    BT_ENABLE_EXT_POWER, false);
    if(is_bt_ext_ldo){
        property_set("wc_transport.extldo", "enabled");
    }else{
        property_set("wc_transport.extldo", "disabled");
    }

    fw_snoop_enable = config_get_bool (config, CONFIG_DEFAULT_SECTION,
                                    BT_ENABLE_FW_SNOOP, false);
    if(fw_snoop_enable){
        property_set("persist.service.bdroid.fwsnoop", "true");
    }else{
        property_set("persist.service.bdroid.fwsnoop", "false");
    }

    soc_log_enable = config_get_bool (config, CONFIG_DEFAULT_SECTION,
                                    BT_ENABLE_SOC_LOG, false);
    if(soc_log_enable){
        property_set("persist.service.bdroid.soclog", "true");
    }else{
        property_set("persist.service.bdroid.soclog", "false");
    }

    return true;
}
