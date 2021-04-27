/*
* Copyright (c) 2021, The Linux Foundation. All rights reserved.
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

#include "BleWlanDppBootstrap.hpp"
#include "BleSocketManager.hpp"
#include "uuid.h"
#include "AdvertiseSettings.hpp"
#include "AdvertiseData.hpp"
#include "AdvertisingSetCallback.hpp"
#include "AdvertisingSet.hpp"
#include "AdvertisingSetParameters.hpp"
#include "GattLeAdvertiser.hpp"

#define LOGTAG "BleWlanDppBootstrap: "
using namespace gatt;

extern BleWlanDppBootstrap *g_ble_wlan_dpp_bootstrap;
extern BleSocketManager *g_ble_socket_manager;

class DppBootstrapAdvertiserCallback : public AdvertisingSetCallback {
  public:
    void onAdvertisingSetStarted (AdvertisingSet *advertisingSet, int txPower, int status) {
      ALOGD(LOGTAG "%s : status=%d ", __FUNCTION__, status);
      if (status == AdvertisingSetCallback::ADVERTISE_SUCCESS) {
        g_ble_wlan_dpp_bootstrap->WlanDppBootstrapModeEnableRsp(BLE_IPC_STATUS_SUCCESS);
      } else {
        g_ble_wlan_dpp_bootstrap->WlanDppBootstrapModeEnableRsp(BLE_IPC_STATUS_FAILED);
      }
    }

    void  onAdvertisingDataSet(AdvertisingSet *advertisingset,int status) {
      ALOGD(LOGTAG "%s ", __FUNCTION__);
    }

    void onAdvertisingSetStopped (AdvertisingSet *advertisingSet) {
      ALOGD(LOGTAG "%s :. isBootstrapModeEnabled: %d", __FUNCTION__,
          g_ble_wlan_dpp_bootstrap->isBootstrapModeEnabled());

      if (g_ble_wlan_dpp_bootstrap->isBootstrapModeEnabled()) {
        g_ble_wlan_dpp_bootstrap->WlanDppBootstrapModeDisableRsp(BLE_IPC_STATUS_SUCCESS);
      }
    }

    void onOwnAddressRead (AdvertisingSet *advertisingSet, int addressType, string address) {
      ALOGD(LOGTAG "%s ", __FUNCTION__);
    }

    void onStartSuccess(AdvertiseSettings *settingsInEffect) {
      ALOGD(LOGTAG "%s ", __FUNCTION__);
    }

    void onStartFailure(int errorCode) {
      ALOGE(LOGTAG "%s ", __FUNCTION__);
    }
};


static DppBootstrapAdvertiserCallback mDppBootstrapAdvCb;
static GattLeAdvertiser *mAdvInstance = NULL;

BleWlanDppBootstrap :: BleWlanDppBootstrap()
{
  mAdvInstance = GattLeAdvertiser::getGattLeAdvertiser();
  is_bootstrap_mode_enabled_ = false;
}

BleWlanDppBootstrap :: ~BleWlanDppBootstrap()
{
  mAdvInstance = NULL;
}

bool BleWlanDppBootstrap :: isBootstrapModeEnabled()
{
  return is_bootstrap_mode_enabled_;
}
void BleWlanDppBootstrap :: WlanDppBootstrapModeEnableReq(WlanDppBootstrapInfo *info)
{
  ALOGD(LOGTAG "%s : isBootstrapModeEnabled: %d", __FUNCTION__, isBootstrapModeEnabled());

  if (isBootstrapModeEnabled()) {
    WlanDppBootstrapModeEnableRsp(BLE_IPC_STATUS_WLAN_DPP_BOOTSTRAP_MODE_ALREADY_ENABLED);
    return;
  } else if(info->adv_data_len > MAX_BOOTSTRAP_ADV_DATA_LEN) {
    WlanDppBootstrapModeEnableRsp(BLE_IPC_STATUS_WLAN_DPP_BOOTSTRAP_MODE_ADV_DATA_TOO_BIG);
    return;
  }

  std::vector<uint8_t> service_data(info->adv_data, (info->adv_data) + (info->adv_data_len));
  const Uuid service_data_uuid = Uuid::From16Bit((info->service_data_uuid));
  AdvertiseData *advData = NULL;
  AdvertisingSetParameters *parameters;

  parameters = AdvertisingSetParameters::Builder()
                            .setConnectable(false)
                            .setScannable(false)
                            .setLegacyMode(false)
                            .setAnonymous(false)
                            .setIncludeTxPower(true)
                            .setInterval(AdvertisingSetParameters::INTERVAL_LOW)
                            .setTxPowerLevel(AdvertisingSetParameters::TX_POWER_HIGH)
                            .build();
  AdvertiseData::Builder builder = AdvertiseData::Builder().setIncludeDeviceName(true)
                                  .setIncludeTxPowerLevel(false);
  builder.addServiceData(service_data_uuid, service_data);
  advData = builder.build();

  try {
    mAdvInstance->startAdvertisingSet(parameters,
                       advData, NULL, NULL ,NULL, &mDppBootstrapAdvCb);
    is_bootstrap_mode_enabled_ = true;
  } catch(const std::exception &ex) {
    ALOGE(LOGTAG "%s : start Advertising exception  %s", __FUNCTION__, ex.what());
    WlanDppBootstrapModeEnableRsp(BLE_IPC_STATUS_FAILED);
  }
}

void BleWlanDppBootstrap :: WlanDppBootstrapModeEnableRsp(BleIpcStatus status)
{
  ble_ipc_msg_t ipc_msg;

  ALOGD(LOGTAG "%s : status: %d", __FUNCTION__, status);
  if (status == BLE_IPC_STATUS_SUCCESS)
    is_bootstrap_mode_enabled_ = true;
  else
    is_bootstrap_mode_enabled_ = false;

  ipc_msg.eventId = BLE_IPC_MSG_WLAN_DPP_BOOTSTRAP_MODE_ENABLE_RSP;
  ipc_msg.wlanDppBootstrapModeEnableRspEvent.status = status;
  g_ble_socket_manager->WBDSSocketWriteHandler(&ipc_msg);
}

void BleWlanDppBootstrap :: WlanDppBootstrapModeDisableReq()
{
  ALOGD(LOGTAG "%s : isBootstrapModeEnabled: %d", __FUNCTION__, isBootstrapModeEnabled());
  if (!isBootstrapModeEnabled()) {
    WlanDppBootstrapModeDisableRsp(BLE_IPC_STATUS_WLAN_DPP_BOOTSTRAP_MODE_ALREADY_DISABLED);
    return;
  }

  mAdvInstance->stopAdvertising(&mDppBootstrapAdvCb);
}

void BleWlanDppBootstrap :: WlanDppBootstrapModeDisableRsp(BleIpcStatus status)
{
  ble_ipc_msg_t ipc_msg;

  ALOGD(LOGTAG "%s : status = %d", __FUNCTION__, status);

  is_bootstrap_mode_enabled_ = false;

  ipc_msg.eventId = BLE_IPC_MSG_WLAN_DPP_BOOTSTRAP_MODE_DISABLE_RSP;
  ipc_msg.wlanDppBootstrapModeDisableRspEvent.status = status;
  g_ble_socket_manager->WBDSSocketWriteHandler(&ipc_msg);
}
