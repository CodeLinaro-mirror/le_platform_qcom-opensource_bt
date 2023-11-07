/*
 * Copyright (c) 2016, The Linux Foundation. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 *       copyright notice, this list of conditions and the following
 *       disclaimer in the documentation and/or other materials provided
 *       with the distribution.
 *     * Neither the name of The Linux Foundation nor the names of its
 *       contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
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

#include "Rsp.hpp"


#define LOGTAG "RSP "
#define UNUSED

#define CONNECTABLE 1
#define SCANNABLE 0
#define LEGACYMODE 0
#define ANONYMOUS 0
#define INCLUDETXPOWER 0

Rsp *rsp = NULL;
int serverif, clientif;

using namespace gatt;

#define TRANSPORT 0


extern GattLibService *g_gatt;
GattServer *mServer = NULL;
GattService *mService = NULL;
GattCharacteristic *mgattCharacteristic = NULL;
GattDescriptor *mgattDescriptor = NULL;
AdvertisingSetParameters *mParameters;
AdvertiseData *mAdvData = NULL;

const Uuid SERVICE_UUID = Uuid::FromString("0000AA01-0000-1000-8000-00805f9b34fb");
const Uuid CHAR_UUID = Uuid::FromString("0000BB02-0000-1000-8000-00805f9b34fb");
const Uuid DESC_UUID = Uuid::FromString("0000CC03-0000-1000-8000-00805f9b34fb");
string adv_data = "Remote Start Profile";

class RspServerCallback  :public GattServerCallback{
  public:
  void onConnectionStateChange(string deviceAddress, int status,
                                                              int newState) {
    ALOGD(LOGTAG"%s status: %d",__FUNCTION__,status);
    rsp->StopAdvertisement();
  }

  void onServiceAdded(int status,GattService *service) {
    ALOGD(LOGTAG"%s status: %d",__FUNCTION__,status);
  }

  void onCharacteristicReadRequest(string deviceAddress, int requestId,
                              int offset, GattCharacteristic *characteristic) {
    ALOGD(LOGTAG"%s",__FUNCTION__);
  }

  void onCharacteristicWriteRequest(string deviceAddress,int requestId,
        GattCharacteristic *characteristic,bool preparedWrite,bool responseNeeded,
        int offset, uint8_t* value, int length) {
	ALOGD(LOGTAG"%s: preparedWrite(%d) responseNeeded(%d) offset(%d)", __FUNCTION__, preparedWrite, responseNeeded, offset);
        {
          int i;
          for (i = 0; i < length; i++)
            ALOGD(LOGTAG"%s: value[%d] = %02x(%c)", __func__, i, value[i], value[i]);
        }
        rsp->SendResponse(deviceAddress,requestId,0,offset, value, length);
  }

  void onDescriptorReadRequest(string deviceAddress, int requestId,
                                       int offset, GattDescriptor *descriptor) {
    ALOGD(LOGTAG"%s",__FUNCTION__);
  }

  void onDescriptorWriteRequest(string deviceAddress, int requestId,
                           GattDescriptor *descriptor,bool preparedWrite,
                           bool responseNeeded, int offset, uint8_t * value, int length) {
    ALOGD(LOGTAG"%s",__FUNCTION__);
    rsp->SendResponse(deviceAddress,requestId,0,offset, value, length);
  }

  void onExecuteWrite(string deviceAddress, int requestId, bool execute) {
    ALOGD(LOGTAG"%s",__FUNCTION__);
  }

  void onNotificationSent(string deviceAddress, int status) {
    ALOGD(LOGTAG"%s",__FUNCTION__);
  }

  void onMtuChanged(string deviceAddress, int mtu) {
    ALOGD(LOGTAG"%s",__FUNCTION__);
  }

  void onPhyUpdate(string deviceAddress,int txPhy, int rxPhy, int status) {
    ALOGD(LOGTAG"%s",__FUNCTION__);
  }

  void onPhyRead(string deviceAddress,int txPhy,int rxPhy,int status) {
    ALOGD(LOGTAG"%s",__FUNCTION__);
  }

  void onConnectionUpdated(string deviceAddress,int interval,
                                  int latency,int timeout,int status) {
    ALOGD(LOGTAG"%s",__FUNCTION__);
  }
};


class RSPAdvertiserCallback  :public AdvertisingSetCallback
{
  public:
  void onAdvertisingSetStarted (AdvertisingSet *advertisingSet, int txPower, int status) {
    ALOGD(LOGTAG"(%s) ", __FUNCTION__);
  }

  void  onAdvertisingDataSet(AdvertisingSet *advertisingset,int status) {
    ALOGD(LOGTAG"(%s)", __FUNCTION__);
  }

  void onAdvertisingSetStopped (AdvertisingSet *advertisingSet) {
    ALOGD(LOGTAG"(%s)",__FUNCTION__);
  }

  void onAdvertisingEnabled (AdvertisingSet *advertisingSet, bool enable, int status) {
    ALOGD(LOGTAG"(%s)",__FUNCTION__);
  }

  void onScanResponseDataSet (AdvertisingSet *advertisingSet, int status) {
    ALOGD(LOGTAG"(%s)",__FUNCTION__);
  }

  void onAdvertisingParametersUpdated (AdvertisingSet *advertisingSet, int txPower, int status) {
    ALOGD(LOGTAG"(%s)",__FUNCTION__);
  }

  void onPeriodicAdvertisingParametersUpdated (AdvertisingSet *advertisingSet, int status) {
    ALOGD(LOGTAG"(%s)",__FUNCTION__);
  }

  void onPeriodicAdvertisingDataSet (AdvertisingSet *advertisingSet, int status) {
    ALOGD(LOGTAG"(%s)",__FUNCTION__);
  }

  void onPeriodicAdvertisingEnabled (AdvertisingSet *advertisingSet, bool enable, int status) {
    ALOGD(LOGTAG"(%s)",__FUNCTION__);
  }

  void onOwnAddressRead (AdvertisingSet *advertisingSet, int addressType, string address) {
    ALOGD(LOGTAG"(%s)",__FUNCTION__);
  }

  void onStartSuccess(AdvertiseSettings *settingsInEffect) {
    ALOGD(LOGTAG "onStartSuccess()");
  }

  void onStartFailure(int errorCode) {
    ALOGE(LOGTAG "onStartFailure()");
  }

};


RspServerCallback *mRspServerCb = NULL;
RSPAdvertiserCallback *mRSPAdvCb = NULL;
GattLeAdvertiser *mAdvertiser = NULL;


Rsp::Rsp(GattLibService* g_gatt) {
  ALOGE(LOGTAG "rsp instantiated");
  fprintf(stdout,"rsp instantiated ");
  wlan_state = WLAN_INACTIVE;
  mlibservice = g_gatt->getGatt();
}


Rsp::~Rsp() {
  fprintf(stdout, "(%s) RSP DeInitialized",__FUNCTION__);
  SetDeviceState(WLAN_INACTIVE);
  delete(mServer);
  delete(mRSPAdvCb);
  delete(mRspServerCb);
}

bool Rsp::EnableRSP() {
  fprintf(stdout, "(%s) Enable RSP Initiated \n",__FUNCTION__);
  ALOGE(LOGTAG "%s",__FUNCTION__);
  mServer = new GattServer(g_gatt,TRANSPORT);
  mRspServerCb = new RspServerCallback();
  mServer->registerCallback(*mRspServerCb);
  mRSPAdvCb    = new RSPAdvertiserCallback();
  mAdvertiser = GattLeAdvertiser::getGattLeAdvertiser();
  AddService();
  return true;
}

bool Rsp::StartAdvertisement() {
  bool ext_adv_supp = false;
  std::vector<uint8_t> vec(adv_data.begin(), adv_data.end());
  ext_adv_supp = mlibservice->isLeExtendedAdvertisingSupported();
  ALOGE(LOGTAG "%s: ext_adv_supp = %d", __FUNCTION__, (int)ext_adv_supp);
  fprintf(stdout,"Rsp::StartAdvertisement \n");
  SetDeviceState(WLAN_INACTIVE);
  mParameters = AdvertisingSetParameters::Builder()
                            .setConnectable(CONNECTABLE)
                            .setScannable((ext_adv_supp ? SCANNABLE : 1))
                            .setLegacyMode((ext_adv_supp ? LEGACYMODE : 1))
                            .setAnonymous(ANONYMOUS)
                            .setIncludeTxPower(INCLUDETXPOWER)
                            .setInterval(AdvertisingSetParameters::INTERVAL_MEDIUM)
                            .setTxPowerLevel(AdvertisingSetParameters::TX_POWER_MEDIUM)
                            .build();
  AdvertiseData::Builder builder = AdvertiseData::Builder().setIncludeDeviceName(true)
                                  .setIncludeTxPowerLevel(false);
  builder.addServiceUuid(SERVICE_UUID);
  if (ext_adv_supp) {
    builder.addServiceData(SERVICE_UUID,vec);
  }
  mAdvData = builder.build();
  try {
      mAdvertiser->startAdvertisingSet(mParameters,
                         mAdvData,NULL,NULL,NULL,mRSPAdvCb);
  } catch(const std::exception &ex) {
    ALOGE(LOGTAG"%s start Advertising exception  %s", __FUNCTION__, ex.what());
    return false;
  }
  return true;
}

void Rsp::SendResponse(string deviceAddress, int requestId, int status,
                                              int offset, uint8_t * value, int length) {
  ALOGE(LOGTAG "%s",__FUNCTION__);
  bool is_valid_value = false;

  if (value && (length == 2) && !strncasecmp((const char *)(value), "on", 2))
	is_valid_value = true;

  if (is_valid_value) {
      status = 0;
	  if (GetDeviceState() == WLAN_INACTIVE) {
		fprintf(stdout, "(%s) Turn ON WLAN\n", __FUNCTION__);
		HandleWlanOn();
		SetDeviceState(WLAN_TRANSACTION_PENDING);
	  }
	  mServer->sendResponse(deviceAddress,requestId,status,offset,value,length);
  } else {
	  status = -1;
      if (value && (length == 2))
		fprintf(stdout, "(%s) INvalid input value %c%c\n", __func__, value[0], value[1]);
	  else
		fprintf(stdout, "(%s) INvalid length %d\n", __func__, length);
      mServer->sendResponse(deviceAddress,requestId,status,offset,value,length);
  }
}

bool Rsp::HandleWlanOn() {
  BtEvent *event = new BtEvent;
  CHECK_PARAM(event);
  event->event_id = SKT_API_IPC_MSG_WRITE;
  event->bt_ipc_msg_event.ipc_msg.type = BT_IPC_REMOTE_START_WLAN;
  event->bt_ipc_msg_event.ipc_msg.status = INITIATED;
  StopAdvertisement();
  fprintf(stdout, "(%s) Posting wlan start to main thread \n",__FUNCTION__);
  PostMessage (THREAD_ID_MAIN, event);
  return true;
}

void Rsp::StopAdvertisement(){
  ALOGE(LOGTAG "%s",__FUNCTION__);
  mAdvertiser->stopAdvertising(mRSPAdvCb);
}

bool Rsp::AddService() {
  ALOGE(LOGTAG "%s",__FUNCTION__);
  mService  = new GattService(SERVICE_UUID,GattService::SERVICE_TYPE_PRIMARY);
  AddCharacteristics();
  AddDescriptor();
  mService->addCharacteristic(mgattCharacteristic);
  mServer->addService(*mService);
  return true;
}


bool Rsp::AddCharacteristics() {
  ALOGE(LOGTAG "%s",__FUNCTION__);
  int property = GATT_PROP_WRITE;
  int permissions = GATT_PERM_WRITE;
  mgattCharacteristic = new GattCharacteristic(CHAR_UUID,property,permissions);
  return true;
}

bool Rsp::AddDescriptor(void) {
  ALOGE(LOGTAG "%s",__FUNCTION__);
  int permissions = GATT_PERM_WRITE;
  mgattDescriptor = new GattDescriptor(DESC_UUID,permissions);
  mgattCharacteristic->addDescriptor(mgattDescriptor);
  return true;
}

