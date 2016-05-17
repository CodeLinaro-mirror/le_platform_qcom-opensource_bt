/*
 * Copyright (c) 2016, The Linux Foundation. All rights reserved.
 * Not a Contribution.
 * Copyright (C) 2012 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <list>
#include <map>
#include <string.h>
#include <hardware/bluetooth.h>
#include <hardware/hardware.h>

#include "osi/include/log.h"
#include "Gap.hpp"

const char *BT_LOCAL_DEV_NAME = "BtLocalDeviceName";

#define LOGTAG "GAP"

using namespace std;
using std::list;
using std::string;

Gap *g_gap = NULL;


#ifdef __cplusplus
extern "C" {
#endif

static bool SetWakeAlarm(uint64_t delay_millis, bool should_wake, alarm_cb cb,
                                                                    void *data) {
    return BT_STATUS_SUCCESS;
}

static int AcquireWakeLock(const char *lock_name) {
    return BT_STATUS_SUCCESS;
}

static int ReleaseWakeLock(const char *lock_name) {
    return BT_STATUS_SUCCESS;
}

static bt_os_callouts_t callouts = {
    sizeof(bt_os_callouts_t),
    SetWakeAlarm,
    AcquireWakeLock,
    ReleaseWakeLock,
};

static void AdapterStateChangeCallback(bt_state_t state) {
    BtEvent *event = new BtEvent;

    ALOGV (LOGTAG " AdapterStateChangeCallback: state %d",state);

    event->event_id = GAP_EVENT_ADAPTER_STATE;
    event->state_event.status = state;
    PostMessage(THREAD_ID_GAP, event);
}

static void AdapterPropertiesCb(bt_status_t status, int num_properties,
                                            bt_property_t *properties) {
    bt_property_t *props;
    unsigned short index;
    BtEvent *event = new BtEvent;

    ALOGV (LOGTAG " adapter_properties_callback:");

    props = new bt_property_t[num_properties];
    memcpy(props, properties, num_properties * sizeof(bt_property_t));
    for (index = 0; index < num_properties; index++) {
        props[index].val = new char[properties[index].len];
        memcpy(props[index].val, properties[index].val, properties[index].len);
    }
    event->adapater_properties_event.num_properties = num_properties;
    event->adapater_properties_event.properties = props;

    event->event_id = GAP_EVENT_ADAPTER_PROPERTIES;
    PostMessage(THREAD_ID_GAP, event);
}

static void RemoteDevicePropertiesCb(bt_status_t status, bt_bdaddr_t *bd_addr,
                                int num_properties, bt_property_t *properties) {
    bt_property_t *props;
    unsigned short index;
    BtEvent *event = new BtEvent;

    ALOGV (LOGTAG " RemoteDevicePropertiesCb:");
    props = new bt_property_t[num_properties];
    memcpy(props, properties, num_properties * sizeof(bt_property_t));
    for (index = 0; index < num_properties; index++) {
        props[index].val = new char[properties[index].len];
        memcpy(props[index].val, properties[index].val, properties[index].len);
    }
    memcpy(&event->remote_properties_event.bd_addr, bd_addr, sizeof(bt_bdaddr_t));
    event->remote_properties_event.num_properties = num_properties;
    event->remote_properties_event.properties = props;

    event->event_id = GAP_EVENT_REMOTE_DEVICE_PROPERTIES;
    PostMessage(THREAD_ID_GAP, event);
}


static void DeviceFoundCb(int num_properties, bt_property_t *properties) {

    bt_property_t *props;
    unsigned short index;
    BtEvent *event = new BtEvent;

    ALOGV (LOGTAG " DeviceFoundCb:");
    props = new bt_property_t[num_properties];
    memcpy(props, properties, num_properties * sizeof(bt_property_t));
    for (index = 0; index < num_properties; index++) {
        props[index].val = new char[properties[index].len];
        memcpy(props[index].val, properties[index].val, properties[index].len);
    }
    event->device_found_event_int.num_properties = num_properties;
    event->device_found_event_int.properties = props;
    event->event_id = GAP_EVENT_DEVICE_FOUND_INT;
    PostMessage(THREAD_ID_GAP, event);
}


static void BondStateChangedCb(bt_status_t status, bt_bdaddr_t *bd_addr,
                                                bt_bond_state_t state) {

    BtEvent *event = new BtEvent;

    ALOGV (LOGTAG " BondStateChangedCb: %d", state);
    event->event_id = GAP_EVENT_BOND_STATE_INT;
    memcpy(&event->bond_state_event_int.bd_addr, bd_addr, sizeof(bt_bdaddr_t));
    event->bond_state_event_int.state = state;
    PostMessage(THREAD_ID_GAP, event);
}

static void AclStateChangedCb(bt_status_t status, bt_bdaddr_t *bd_addr,
                                                    bt_acl_state_t state) {

    BtEvent *event = new BtEvent;
    event->event_id = GAP_EVENT_ACL_STATE_CHANGED;
    memcpy(&event->acl_state_event.bd_addr, bd_addr, sizeof(bt_bdaddr_t));
    event->acl_state_event.status = status;
    event->acl_state_event.state = state;
    PostMessage(THREAD_ID_GAP, event);
    ALOGV (LOGTAG " AclStateChangedCb:");
}

static void DiscoveryStateChangedCb(bt_discovery_state_t state) {

    ALOGV (LOGTAG " DiscoveryStateChangedCb:");

    BtEvent *event = new BtEvent;
    event->event_id = GAP_EVENT_DISCOVERY_STATE_CHANGED;
    event->discovery_state_event.state = state;
    PostMessage(THREAD_ID_GAP, event);
}

static void PinRequestCb(bt_bdaddr_t *bd_addr, bt_bdname_t *bdname,
                                uint32_t cod, bool min_16_digit) {
    BtEvent *event = new BtEvent;

    ALOGV (LOGTAG " PinRequestCb:");
    event->event_id = GAP_EVENT_PIN_REQUEST;
    memcpy(&event->pin_request_event.bd_addr, bd_addr, sizeof(bt_bdaddr_t));
    memset(&event->pin_request_event.bd_name, 0, sizeof(bt_bdname_t));

    event->pin_request_event.cod = cod;
    event->pin_request_event.secure = min_16_digit;
    PostMessage(THREAD_ID_GAP, event);
}

static void SspRequestCb(bt_bdaddr_t *bd_addr, bt_bdname_t *bdname, uint32_t cod,
        bt_ssp_variant_t pairing_variant, uint32_t pass_key) {

    BtEvent *event = new BtEvent;

    ALOGV (LOGTAG " SspRequestCb:");
    event->event_id = GAP_EVENT_SSP_REQUEST;
    memcpy(&event->ssp_request_event.bd_addr, bd_addr, sizeof(bt_bdaddr_t));

    memset(&event->ssp_request_event.bd_name, 0, sizeof(bt_bdname_t));

    event->ssp_request_event.cod = cod;
    event->ssp_request_event.pairing_variant = pairing_variant;
    event->ssp_request_event.pass_key = pass_key;
    PostMessage(THREAD_ID_GAP, event);
}

static void CbThreadEvent(bt_cb_thread_evt event) {
    if (event  == ASSOCIATE_JVM) {
        ALOGV (LOGTAG " Callback thread attached: ");
    } else if (event == DISASSOCIATE_JVM) {
        ALOGE (LOGTAG " Callback: CbThreadEvent is not called on correct thread");
    }
}

static void DutModeRecvCb (uint16_t opcode, uint8_t *buf, uint8_t len) {
    ALOGV (LOGTAG " DutModeRecvCb ");
}

static void LeTestModeRecvCb (bt_status_t status, uint16_t packet_count) {

    ALOGV (LOGTAG " LeTestModeRecvCb: status:%d packet_count:%d ", status,
                                                            packet_count);
}

static void EnergyInfoRecvCb(bt_activity_energy_info *p_energy_info) {
    ALOGV (LOGTAG " EnergyInfoRecvCb: ");
}

static bt_callbacks_t sBluetoothCallbacks = {
    sizeof(sBluetoothCallbacks),
    AdapterStateChangeCallback,
    AdapterPropertiesCb,
    RemoteDevicePropertiesCb,
    DeviceFoundCb,
    DiscoveryStateChangedCb,
    PinRequestCb,
    SspRequestCb,
    BondStateChangedCb,
    AclStateChangedCb,
    CbThreadEvent,
    DutModeRecvCb,
    LeTestModeRecvCb,
    EnergyInfoRecvCb,
    NULL,
};

void BtGapMsgHandler(void *msg) {
    BtEvent* event = NULL;
    if (!msg) {
        printf("Msg is null, return.\n");
        return;
    }

    event = ( BtEvent *) msg;

    switch (event->event_id) {
        default:
            if (g_gap) {
                g_gap->ProcessEvent(( BtEvent *) msg);
            }
            delete event;
            break;
    }
}

#ifdef __cplusplus
}
#endif

void Gap::HandlePinRequestEvent(PINRequestEvent *event) {

    bt_pin_code_t pincode;
    DeviceProperties *remote_dev_prop;
    BtEvent *bt_event;
    bdstr_t bd_str;

    memset(&pincode, 0, sizeof(pincode));
    /* For now auto accept with most common pincode "0000" */
    pincode.pin[0] = '0';
    pincode.pin[1] = '0';
    pincode.pin[2] = '0';
    pincode.pin[3] = '0';

    bluetooth_interface_->pin_reply(&event->bd_addr, 1, 4, &pincode);
    BdAddr2Str(&event->bd_addr, &bd_str[0]);
    string deviceAddress(bd_str);

    std::map<std::string, DeviceProperties*>::iterator it;
    it = remote_devices_obj_->remote_device_prop.find(deviceAddress);
    if (it != remote_devices_obj_->remote_device_prop.end()) {
        ALOGD(LOGTAG "Device in the list");
    } else {
        remote_dev_prop = new DeviceProperties;
        remote_devices_obj_->remote_device_prop[deviceAddress] = remote_dev_prop;
        bt_event = new BtEvent;
        bt_event->device_found_event.event_id = GAP_EVENT_DEVICE_FOUND;
        memcpy(&bt_event->device_found_event.remoteDevice, remote_dev_prop,
                                            sizeof(DeviceProperties));
    }
}

void Gap::HandleSspRequestEvent(SSPRequestEvent *event) {
    DeviceProperties *remote_dev_prop;
    BtEvent *bt_event;
    bdstr_t bd_str;

    /* For now auto accept */
    bluetooth_interface_->ssp_reply(&event->bd_addr, event->pairing_variant,
            1, event->pass_key);
    BdAddr2Str(&event->bd_addr, &bd_str[0]);

    string deviceAddress(bd_str);

    std::map<std::string, DeviceProperties*>::iterator it;
    it = remote_devices_obj_->remote_device_prop.find(deviceAddress);
    if (it != remote_devices_obj_->remote_device_prop.end()) {
        ALOGD(LOGTAG "Device in the list");
    } else {
        remote_dev_prop = new DeviceProperties;
        remote_devices_obj_->remote_device_prop[deviceAddress] = remote_dev_prop;
        bt_event = new BtEvent;
        bt_event->device_found_event.event_id = GAP_EVENT_DEVICE_FOUND;
        memcpy(&bt_event->device_found_event.remoteDevice, remote_dev_prop,
                                    sizeof(DeviceProperties));
    }
}

void Gap::HandleBondStateEvent(DeviceBondStateEventInt *event) {
    DeviceProperties *pRemoteDevice;

    pRemoteDevice = remote_devices_obj_->GetDeviceProperties(event->bd_addr);

    if (pRemoteDevice == NULL ) {
        pRemoteDevice = remote_devices_obj_->AddDeviceProperties(event->bd_addr);
    }

    if (pRemoteDevice->mBondState == event->state)
        return;

    adapter_properties_obj_->OnbondStateChanged(event->bd_addr, event->state);
}

void Gap::HandleEnable(void) {
    BtEvent  *bt_event  = NULL;
    if ((adapter_properties_obj_->GetState() == BT_ADAPTER_STATE_OFF) &&
       (bluetooth_interface_->enable() == BT_STATUS_SUCCESS)) {
        adapter_properties_obj_->SetState(BT_ADAPTER_STATE_TURNING_ON);
    } else {
        //Sending update to the UI thread
        bt_event = new BtEvent;
        bt_event->event_id = MAIN_EVENT_ENABLED;
        bt_event->state_event.status = BT_STATE_OFF;
        PostMessage(THREAD_ID_MAIN, bt_event);
    }
}

void Gap::HandleDisable(void) {
    BtEvent  *bt_event  = NULL;
    if ((adapter_properties_obj_->GetState() == BT_ADAPTER_STATE_ON) &&
       (bluetooth_interface_->disable() == BT_STATUS_SUCCESS)) {
        adapter_properties_obj_->SetState(BT_ADAPTER_STATE_TURNING_OFF);
    } else {
        //Sending update to the UI thread
        bt_event = new BtEvent;
        bt_event->event_id = MAIN_EVENT_DISABLED;
        bt_event->state_event.status = BT_STATE_ON;
        PostMessage(THREAD_ID_MAIN, bt_event);
    }
}

void Gap::HandleStartDiscovery(void) {
    if ((adapter_properties_obj_->GetState() == BT_ADAPTER_STATE_ON) &&
       (bluetooth_interface_->start_discovery() == BT_STATUS_SUCCESS)) {
    } else {
        //Sending update to the UI thread
        BtEvent *event = new BtEvent;
        event->event_id = GAP_EVENT_DISCOVERY_STATE_CHANGED;
        event->discovery_state_event.state = BT_DISCOVERY_STOPPED;
        PostMessage(THREAD_ID_GAP, event);
    }
}

void Gap::HandleStopDiscovery(void) {
    if ((adapter_properties_obj_->GetState() == BT_ADAPTER_STATE_ON) &&
       (bluetooth_interface_->cancel_discovery() == BT_STATUS_SUCCESS)) {
    } else {
        //Sending update to the UI thread
        BtEvent *event = new BtEvent;
        event->event_id = GAP_EVENT_DISCOVERY_STATE_CHANGED;
        event->discovery_state_event.state = BT_DISCOVERY_STARTED;
        PostMessage(THREAD_ID_GAP, event);
    }
}

void Gap::ProcessEvent(BtEvent* event) {
    bt_property_t prop;
    bt_scan_mode_t scan_mode = BT_SCAN_MODE_CONNECTABLE_DISCOVERABLE;
    bt_bdname_t bd_name;
    BtEvent  *bt_event  = NULL;

    ALOGD(LOGTAG " Processing event %d", event->event_id);

    switch (event->event_id) {
        case GAP_EVENT_ADAPTER_STATE:
            adapter_properties_obj_->SetState((AdapterState)event->state_event.status);
            if ( event->state_event.status == BT_STATE_ON ) {

                prop.type = BT_PROPERTY_ADAPTER_SCAN_MODE;
                prop.val = &scan_mode;
                prop.len = sizeof(bt_scan_mode_t);
                bluetooth_interface_->set_adapter_property(&prop);
                prop.type = BT_PROPERTY_BDNAME;

                strcpy((char*)&bd_name.name[0], config_get_string (config_,
                   CONFIG_DEFAULT_SECTION, BT_LOCAL_DEV_NAME, "MDM_Fluoride"));
                prop.val = &bd_name;
                prop.len = strlen((char*)bd_name.name);
                bluetooth_interface_->set_adapter_property(&prop);

                //Sending update to the UI thread
                bt_event = new BtEvent;
                bt_event->event_id = MAIN_EVENT_ENABLED;
                bt_event->state_event.status = event->state_event.status;
                PostMessage(THREAD_ID_MAIN, bt_event);
            } else if ( event->state_event.status == BT_STATE_OFF) {
                //Sending update to the UI thread
                //TODO to check the right place for this
                scan_mode = BT_SCAN_MODE_NONE;
                prop.type = BT_PROPERTY_ADAPTER_SCAN_MODE;
                prop.val = &scan_mode;
                prop.len = sizeof(bt_scan_mode_t);
                bluetooth_interface_->set_adapter_property(&prop);

                bt_event = new BtEvent;
                bt_event->event_id = MAIN_EVENT_DISABLED;
                bt_event->state_event.status = event->state_event.status;
                PostMessage(THREAD_ID_MAIN, bt_event);

                adapter_properties_obj_->FlushBondedDeviceList();
                remote_devices_obj_->FlushDiscoveredDeviceList();
            }
            break;

        case GAP_API_ENABLE:
            HandleEnable();
            break;

        case GAP_API_DISABLE:
            HandleDisable();
            break;

        case GAP_EVENT_DEVICE_FOUND_INT:
            remote_devices_obj_->DeviceFound(&event->device_found_event_int);
            break;

        case GAP_EVENT_REMOTE_DEVICE_PROPERTIES:
            remote_devices_obj_->RemoteDeviceProperties(
                                            &event->remote_properties_event);
            break;

        case GAP_EVENT_ADAPTER_PROPERTIES:
            adapter_properties_obj_->AdapterPropertiesUpdate(
                                            &event->adapater_properties_event);
            break;

        case GAP_EVENT_PIN_REQUEST:
            HandlePinRequestEvent(&event->pin_request_event);
            break;

        case GAP_EVENT_SSP_REQUEST:
            HandleSspRequestEvent(&event->ssp_request_event);
            break;

        case GAP_EVENT_BOND_STATE_INT:
            HandleBondStateEvent(&event->bond_state_event_int);
            break;
        case GAP_EVENT_ACL_STATE_CHANGED:
            remote_devices_obj_->HandleAclStateChange(event->acl_state_event.status,
                event->acl_state_event.bd_addr, event->acl_state_event.state);
            break;
        case GAP_EVENT_DISCOVERY_STATE_CHANGED:
            adapter_properties_obj_->HandleDiscoveryStateChange(
                                            event->discovery_state_event.state);
            break;
        case GAP_API_START_INQUIRY:
            HandleStartDiscovery();
            break;
        case GAP_API_STOP_INQUIRY:
            HandleStopDiscovery();
            break;

        case GAP_API_CREATE_BOND:
            bluetooth_interface_->create_bond(&event->bond_device.bd_addr, 1);
            break;
    }
}

Gap :: Gap(const bt_interface_t *bt_interface, config_t *config) {

    this->bluetooth_interface_ = bt_interface;
    this->config_ = config;

    if ((bluetooth_interface_->init(&sBluetoothCallbacks) == BT_STATUS_SUCCESS))
        bt_interface->set_os_callouts(&callouts);

    this->remote_devices_obj_ = new RemoteDevices(bluetooth_interface_);
    this->adapter_properties_obj_ = new AdapterProperties(bluetooth_interface_,
                                                        remote_devices_obj_);
}

Gap :: ~Gap() {
    delete adapter_properties_obj_;
    delete remote_devices_obj_;
}

int Gap:: GetState() {
    if (adapter_properties_obj_ != NULL)
        return adapter_properties_obj_->GetState();
    return  BT_STATE_OFF;
}

bool Gap:: IsDiscovering() {
    return adapter_properties_obj_->IsDiscovering();
}

bool Gap:: IsEnabled() {
    return (adapter_properties_obj_->GetState() == BT_STATE_ON);
}
