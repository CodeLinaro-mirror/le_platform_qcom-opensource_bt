/*
 * Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include "BleGapService.hpp"
#include "BleSocketManager.hpp"
#include "uuid.h"
#include "ScanSettings.hpp"
#include "ScanCallback.hpp"
#include "GattLeScanner.hpp"
#include "AdvertiseSettings.hpp"
#include "AdvertiseData.hpp"
#include "AdvertisingSetCallback.hpp"
#include "AdvertisingSet.hpp"
#include "AdvertisingSetParameters.hpp"
#include "GattLeAdvertiser.hpp"

#define LOGTAG "BleGapService: "
using namespace gatt;

extern BleGapService *g_ble_gap_service;
extern BleSocketManager *g_ble_socket_manager;

static std::condition_variable mAdvCV;
static std::mutex mAdvLock;

class BleGapServiceScannerCallback : public ScanCallback
{
  public:
    void onScanResult(int callbackType, ScanResult *result)
    {
      ScanRecord *sr = result->getScanRecord();
      std::vector<uint8_t> sData =
          sr->getServiceData(g_ble_gap_service->service_data_uuid_);
      int advertise_flags = sr->getAdvertiseFlags();
      string bd_addr = result->getDevice();
      ALOGD(LOGTAG "The scanned device is %s", bd_addr.c_str());
      uint16_t service_data_uuid16 = g_ble_gap_service->service_data_uuid_.As16Bit();
      uint8_t service_data[MAX_SERVICE_DATA_LEN];
      uint8_t service_data_len = sData.size();
      if ((service_data_len > 0) &&
          (service_data_len <= MAX_SERVICE_DATA_LEN)) {
        std::copy(sData.begin(), sData.end(), service_data);
      }
      vector<uint8_t> raw_adv_data = sr->getBytes();
      vector<uint8_t> dev_name = {};

      int i=0;
      while (i<raw_adv_data.size()) {
        // LTV format
        if ((raw_adv_data[i+1] == 0x08) || (raw_adv_data[i+1] == 0x09)) {
          dev_name.assign(raw_adv_data.begin() + i+2, raw_adv_data.begin() + i+2+raw_adv_data[i]-1);
          break;
        }
        i = i + raw_adv_data[i]+1;
      }
      g_ble_gap_service->ScanResult(advertise_flags,service_data_uuid16,
            service_data_len, service_data, bd_addr, raw_adv_data, dev_name,
            result->isConnectable(), result->isLegacy(), result->getRssi(),
            result->getTxPower(), result->getAdvertisingSid(),
            result->getPeriodicAdvertisingInterval(), result->getPrimaryPhy(),
            result->getSecondaryPhy());
    }

    void onScanFailed (int errorCode)
    {
      ALOGE(LOGTAG "%s:  Scan Failed due to error %d", __FUNCTION__, errorCode);
      g_ble_gap_service->ScanEnableRsp(BLE_IPC_STATUS_FAILED);
    }
};

class BleGapServiceAdvertiserCallback : public AdvertisingSetCallback {
  public:
    void onAdvertisingSetStarted (AdvertisingSet *advertisingSet, int txPower, int status) {
      ALOGD(LOGTAG "%s : status=%d ", __FUNCTION__, status);
      if (status == AdvertisingSetCallback::ADVERTISE_SUCCESS) {
        g_ble_gap_service->AdvertiseEnableRsp(BLE_IPC_STATUS_SUCCESS);
      } else {
        g_ble_gap_service->adv_in_progress_ = false;
        g_ble_gap_service->AdvertiseEnableRsp(BLE_IPC_STATUS_FAILED);
      }
      std::unique_lock<std::mutex> lck(mAdvLock);
      mAdvCV.notify_all();
    }

   void onAdvertisingEnabled (AdvertisingSet *advertisingSet, bool enable, int status) {
      ALOGD(LOGTAG "%s, enable=%d, status=%d", __FUNCTION__, enable, status);
      if (!enable) {
        // There are two callbacks when using non-zero duration adv
        // one due to host timer, another due to controller timer
        g_ble_gap_service->AdvertiseDisableReq();
      }
    }

    void  onAdvertisingDataSet(AdvertisingSet *advertisingset, int status) {
      ALOGD(LOGTAG "%s ", __FUNCTION__);
    }

    void onAdvertisingSetStopped (AdvertisingSet *advertisingSet) {
      ALOGD(LOGTAG "%s ", __FUNCTION__);

      g_ble_gap_service->AdvertiseDisableRsp(BLE_IPC_STATUS_SUCCESS);
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


static BleGapServiceScannerCallback mBleGapScanCb;
static BleGapServiceAdvertiserCallback mBleGapAdvCb;
static GattLeAdvertiser * mAdvInstance = NULL;
static GattLeScanner * mScanner = NULL;

BleGapService :: BleGapService()
{
  ALOGD(LOGTAG "%s ", __FUNCTION__);
  mAdvInstance = GattLeAdvertiser::getGattLeAdvertiser();
  mScanner = GattLeScanner::getGattLeScanner();
  scan_in_progress_ = false;
  adv_in_progress_ = false;
}

BleGapService :: ~BleGapService()
{
  ALOGD(LOGTAG "%s ", __FUNCTION__);
  CleanupClientRequests();
  mAdvInstance = NULL;
  mScanner = NULL;
}

void BleGapService :: CleanupClientRequests() {
  if (scan_in_progress_) {
    scan_in_progress_ = false;
    mScanner->stopScan(&mBleGapScanCb);
  }
  if (adv_in_progress_) {
    adv_in_progress_ = false;
    mAdvInstance->stopAdvertisingSet(&mBleGapAdvCb);
  }
}

void BleGapService :: ScanResult(int advertise_flags,
                            uint16_t service_data_uuid16,
                            uint8_t service_data_len,
                            const uint8_t *service_data,
                            string bdaddr,
                            const vector<uint8_t> & raw_adv_data,
                            const vector<uint8_t> & dev_name,
                            bool is_connectable,
                            bool is_legacy,
                            int16_t rssi,
                            int16_t tx_power,
                            uint8_t sid,
                            uint16_t periodic_adv_int,
                            uint8_t primary_phy,
                            uint8_t secondary_phy)
{
  ALOGD(LOGTAG "%s ", __FUNCTION__);

  ble_ipc_msg_t ipc_msg;

  ALOGD(LOGTAG "%s: bdaddr: %s", __FUNCTION__, bdaddr.c_str());
  ipc_msg.eventId = BLE_IPC_MSG_GAP_SCAN_RESULT;
  ipc_msg.bleGapScanResultEvent.advertise_flags = advertise_flags;
  ipc_msg.bleGapScanResultEvent.service_data_uuid16 = service_data_uuid16;
  ipc_msg.bleGapScanResultEvent.service_data_len = service_data_len;
  memcpy(ipc_msg.bleGapScanResultEvent.service_data, service_data, service_data_len);
  strlcpy(ipc_msg.bleGapScanResultEvent.bd_addr, bdaddr.c_str(), BD_ADDR_STR_LEN);
  ipc_msg.bleGapScanResultEvent.raw_adv_data_len = raw_adv_data.size();
  memset(ipc_msg.bleGapScanResultEvent.raw_adv_data, 0, MAX_EXT_ADV_DATA_LEN);
  memcpy(ipc_msg.bleGapScanResultEvent.raw_adv_data, &raw_adv_data[0], raw_adv_data.size());
  ipc_msg.bleGapScanResultEvent.device_name_len = dev_name.size();
  memset(ipc_msg.bleGapScanResultEvent.device_name, 0, MAX_ADV_DATA_TYPE_VAL_LEN);
  memcpy(ipc_msg.bleGapScanResultEvent.device_name, &dev_name[0], dev_name.size());
  ipc_msg.bleGapScanResultEvent.is_connectable = is_connectable;
  ipc_msg.bleGapScanResultEvent.is_legacy = is_legacy;
  ipc_msg.bleGapScanResultEvent.rssi = rssi;
  ipc_msg.bleGapScanResultEvent.tx_power = tx_power;
  ipc_msg.bleGapScanResultEvent.adv_sid = sid;
  ipc_msg.bleGapScanResultEvent.periodic_adv_int = periodic_adv_int;
  ipc_msg.bleGapScanResultEvent.primary_phy = primary_phy;
  ipc_msg.bleGapScanResultEvent.secondary_phy = secondary_phy;
  g_ble_socket_manager->BleGapSocketWriteHandler(&ipc_msg);

}

void BleGapService :: ScanEnableReq(BleGapScanEnableReqEvent * evt)
{
  ALOGD(LOGTAG "%s ", __FUNCTION__);
  if (scan_in_progress_) return ScanEnableRsp(BLE_IPC_STATUS_GAP_SCAN_ALREADY_ENABLED);
  std::vector<uint8_t> service_data(evt->service_data,
                                    (evt->service_data) + (evt->service_data_len));
  std::vector<uint8_t> service_data_mask(evt->service_data_mask,
      (evt->service_data_mask) + (evt->service_data_mask_len));
  service_data_uuid_ = Uuid::From16Bit(evt->service_data_uuid16);
  ALOGD(LOGTAG "%s service_data_uuid = %d, service_data.size()=%d, service_data_mask.size()=%d", __FUNCTION__, service_data_uuid_, service_data.size(), service_data_mask.size());

  ScanSettings *settings = ScanSettings::Builder()
                            .setScanMode(evt->scan_mode)
                            .setLegacy(evt->legacy).build();
  ScanFilter *filter =
      ScanFilter::Builder().setServiceData(service_data_uuid_,
                                           service_data, service_data_mask).build();
  vector < ScanFilter*> filters;

  filters.push_back(filter);
  try {
    mScanner->startScan(filters, settings, &mBleGapScanCb);
  } catch(const std::exception &ex) {
    ALOGE(LOGTAG "%s : start scanning exception  %s", __FUNCTION__, ex.what());
    ScanEnableRsp(BLE_IPC_STATUS_FAILED);
    return;
  }
  scan_in_progress_ = true;
  ScanEnableRsp(BLE_IPC_STATUS_SUCCESS);
}

void BleGapService :: ScanEnableRsp(BleIpcStatus status) {
  ble_ipc_msg_t ipc_msg;

  ALOGD(LOGTAG "%s : status: %d", __FUNCTION__, status);

  ipc_msg.eventId = BLE_IPC_MSG_GAP_SCAN_ENABLE_RSP;
  ipc_msg.bleGapScanEnableRspEvent.status = status;
  g_ble_socket_manager->BleGapSocketWriteHandler(&ipc_msg);
}

void BleGapService :: ScanDisableReq()
{
  ALOGD(LOGTAG "%s ", __FUNCTION__);
  if (scan_in_progress_) {
    scan_in_progress_ = false;
    mScanner->stopScan(&mBleGapScanCb);
  }
  ScanDisableRsp(BLE_IPC_STATUS_SUCCESS);
}

void BleGapService :: ScanDisableRsp(BleIpcStatus status) {
  ble_ipc_msg_t ipc_msg;

  ALOGD(LOGTAG "%s : status: %d", __FUNCTION__, status);

  ipc_msg.eventId = BLE_IPC_MSG_GAP_SCAN_DISABLE_RSP;
  ipc_msg.bleGapScanDisableRspEvent.status = status;
  g_ble_socket_manager->BleGapSocketWriteHandler(&ipc_msg);
}

void BleGapService :: AdvertiseEnableReq(BleGapAdvertiseInfo *info)
{
  ALOGD(LOGTAG "%s ", __FUNCTION__);

  if (adv_in_progress_) return AdvertiseEnableRsp(BLE_IPC_STATUS_GAP_ADV_ALREADY_ENABLED);
  AdvertiseData *advData = NULL;
  std::vector<uint8_t> service_data(info->adv_data.service_data, (info->adv_data.service_data) + (info->adv_data.service_data_len));
  const Uuid service_data_uuid = Uuid::From16Bit((info->adv_data.service_data_uuid16));

  AdvertiseData::Builder builder = AdvertiseData::Builder().setIncludeDeviceName(info->adv_data.include_device_name)
                                  .setIncludeTxPowerLevel(info->adv_data.include_tx_power_level);
  builder.addServiceData(service_data_uuid, service_data);
  advData = builder.build();

  AdvertisingSetParameters *parameters;
  parameters = AdvertisingSetParameters::Builder()
                            .setConnectable(info->connectable)
                            .setScannable(info->scannable)
                            .setLegacyMode(info->legacy)
                            .setAnonymous(info->anonymous)
                            .setIncludeTxPower(info->include_tx_power)
                            .setInterval(info->interval)
                            .setTxPowerLevel(info->tx_power_level)
                            .build();

  adv_in_progress_ = true;
  try {
    mAdvInstance->startAdvertisingSet(parameters,
                       advData, NULL, NULL ,NULL, info->duration_msec/10, 0, &mBleGapAdvCb);
  } catch(const std::exception &ex) {
    ALOGE(LOGTAG "%s : start Advertising exception  %s", __FUNCTION__, ex.what());
    adv_in_progress_ = false;
    AdvertiseEnableRsp(BLE_IPC_STATUS_FAILED);
    return;
  }

  if (adv_in_progress_) {
    std::unique_lock<std::mutex> lck(mAdvLock);
    if (mAdvCV.wait_for(lck,
      std::chrono::milliseconds(500)) == std::cv_status::timeout)
    {
      ALOGE(LOGTAG "%s : start Advertising timeout", __FUNCTION__);
    }
  } else {
    ALOGE(LOGTAG "%s : start Advertising failed", __FUNCTION__);
  }
}

void BleGapService :: AdvertiseEnableRsp(BleIpcStatus status)
{
  ble_ipc_msg_t ipc_msg;

  ALOGD(LOGTAG "%s : status: %d", __FUNCTION__, status);

  ipc_msg.eventId = BLE_IPC_MSG_GAP_ADVERTISE_ENABLE_RSP;
  ipc_msg.bleGapAdvertiseEnableRspEvent.status = status;
  g_ble_socket_manager->BleGapSocketWriteHandler(&ipc_msg);
}

void BleGapService :: AdvertiseDisableReq()
{
  ALOGD(LOGTAG "%s ", __FUNCTION__);
  if (adv_in_progress_) {
    adv_in_progress_ = false;
    mAdvInstance->stopAdvertisingSet(&mBleGapAdvCb);
  } else {
    AdvertiseDisableRsp(BLE_IPC_STATUS_SUCCESS);
  }
}

void BleGapService :: AdvertiseDisableRsp(BleIpcStatus status)
{
  ble_ipc_msg_t ipc_msg;

  ALOGD(LOGTAG "%s : status = %d", __FUNCTION__, status);

  ipc_msg.eventId = BLE_IPC_MSG_GAP_ADVERTISE_DISABLE_RSP;
  ipc_msg.bleGapAdvertiseDisableRspEvent.status = status;
  g_ble_socket_manager->BleGapSocketWriteHandler(&ipc_msg);
}
