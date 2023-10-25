/*
 * Copyright (c) 2018, The Linux Foundation. All rights reserved.
 * Not a Contribution.
 * Copyright (C) 2017 The Android Open Source Project
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

#include "GattNativeInterfaceV2.hpp"
#include "GattLibService.hpp"
#include <algorithm>
#include <unordered_map>
#include <list>
#include <mutex>
#include <thread>
#include <chrono>

#define LOGTAG "GattLibService"

using namespace std;

void BtGattMsgHandler (void *context)
{
  BtEvent* event = NULL;
  if (!context) {
    ALOGE(LOGTAG " (BtGattMsgHandler): Msg is null, return");
    return;
  }

  event = ( BtEvent *) context;
  gatt::GattLibService *sGattLibService = gatt::GattLibService::getGatt();

  switch (event->event_id) {
    case PROFILE_API_START:
      if (sGattLibService) {
        BtEvent *start_event = new BtEvent;
        CHECK_PARAM_VOID (start_event)
        ALOGD(LOGTAG "(%s) posting profile start done\n",__FUNCTION__);
        start_event->profile_start_event.event_id = PROFILE_EVENT_START_DONE;
        start_event->profile_start_event.profile_id = PROFILE_ID_GATT;
        start_event->profile_start_event.status = true;
        PostMessage(THREAD_ID_GAP, start_event);
      }
      break;
    case PROFILE_API_STOP:
      if (sGattLibService) {
        BtEvent *stop_event = new BtEvent;
        CHECK_PARAM_VOID (stop_event)
        stop_event->profile_start_event.event_id = PROFILE_EVENT_STOP_DONE;
        stop_event->profile_start_event.profile_id = PROFILE_ID_GATT;
        stop_event->profile_start_event.status = true;
        PostMessage(THREAD_ID_GAP, stop_event);
      }
      break;
    default:
      if (sGattLibService) {
        sGattLibService->ProcessEvent(( BtEvent *) context);
      }
      delete event;
      break;
  }
}

namespace gatt {

GattLibService* volatile GattLibService::sGattService = NULL;
static std::mutex singletonLock;


/* Bluetooth Adapter and Remote Device property types */
enum {
  /* Properties common to both adapter and remote device */
  /**
  * Description - Bluetooth Device Name
  * Access mode - Adapter name can be GET/SET. Remote device can be GET
  * Data type   - bt_bdname_t
  */
  BT_PROPERTY_BDNAME = 0x1,
  /**
  * Description - Bluetooth Device Address
  * Access mode - Only GET.
  * Data type   - RawAddress
  */
  BT_PROPERTY_BDADDR,
  /**
  * Description - Bluetooth Service 128-bit UUIDs
  * Access mode - Only GET.
  * Data type   - Array of bluetooth::Uuid (Array size inferred from property
  *               length).
  */
  BT_PROPERTY_UUIDS,
  /**
  * Description - Bluetooth Class of Device as found in Assigned Numbers
  * Access mode - Only GET.
  * Data type   - uint32_t.
  */
  BT_PROPERTY_CLASS_OF_DEVICE,
  /**
  * Description - Device Type - BREDR, BLE or DUAL Mode
  * Access mode - Only GET.
  * Data type   - bt_device_type_t
  */
  BT_PROPERTY_TYPE_OF_DEVICE,
  /**
  * Description - Bluetooth Service Record
  * Access mode - Only GET.
  * Data type   - bt_service_record_t
  */
  BT_PROPERTY_SERVICE_RECORD,

  /* Properties unique to adapter */
  /**
  * Description - Bluetooth Adapter scan mode
  * Access mode - GET and SET
  * Data type   - bt_scan_mode_t.
  */
  BT_PROPERTY_ADAPTER_SCAN_MODE,
  /**
  * Description - List of bonded devices
  * Access mode - Only GET.
  * Data type   - Array of RawAddress of the bonded remote devices
  *               (Array size inferred from property length).
  */
  BT_PROPERTY_ADAPTER_BONDED_DEVICES,
  /**
  * Description - Bluetooth Adapter Discovery timeout (in seconds)
  * Access mode - GET and SET
  * Data type   - uint32_t
  */
  BT_PROPERTY_ADAPTER_DISCOVERY_TIMEOUT,

  /* Properties unique to remote device */
  /**
  * Description - User defined friendly name of the remote device
  * Access mode - GET and SET
  * Data type   - bt_bdname_t.
  */
  BT_PROPERTY_REMOTE_FRIENDLY_NAME,
  /**
  * Description - RSSI value of the inquired remote device
  * Access mode - Only GET.
  * Data type   - int8_t.
  */
  BT_PROPERTY_REMOTE_RSSI,
  /**
  * Description - Remote version info
  * Access mode - SET/GET.
  * Data type   - bt_remote_version_t.
  */

  BT_PROPERTY_REMOTE_VERSION_INFO,

  /**
  * Description - Local LE features
  * Access mode - GET.
  * Data type   - bt_local_le_features_t.
  */
  BT_PROPERTY_LOCAL_LE_FEATURES,

  BT_PROPERTY_REMOTE_DEVICE_TIMESTAMP = 0xFF,
} bt_property_type_t;


static std::string intToHexString(int data)
{
  std::stringstream stream;
  stream << std::hex;
  stream << std::setw(2) << std::setfill('0') << (int)data;
  return stream.str();
}

static const Uuid HID_UUIDS[4] = {
  Uuid::FromString("00002A4A-0000-1000-8000-00805F9B34FB"),
  Uuid::FromString("00002A4B-0000-1000-8000-00805F9B34FB"),
  Uuid::FromString("00002A4C-0000-1000-8000-00805F9B34FB"),
  Uuid::FromString("00002A4D-0000-1000-8000-00805F9B34FB")
};

static const Uuid FIDO_UUIDS[1] = {
  Uuid::FromString("0000FFFD-0000-1000-8000-00805F9B34FB") // U2F
};

// Helper method to extract bytes from byte array.
std::vector<uint8_t> GattLibService::extractBytes(std::vector<uint8_t> scanRecord,
                                                                int start, int length)
{
  std::vector<uint8_t> bytes(length);
  if ((length > 0) && (length+start < scanRecord.size())) {
    bytes.assign(scanRecord.begin()+start, scanRecord.begin()+start+length);
  }
  return bytes;
}

/**
* CALLBACKS
*/

void GattLibService::onScanResult(int eventType, int addressType,
                                      string address, int primaryPhy,
                                      int secondaryPhy, int advertisingSid,
                                      int txPower, int rssi, int periodicAdvInt,
                                      std::vector<uint8_t> advData)
{
  if (VDBG) {
    ALOGD(LOGTAG " onScanResult() - eventType= %s , addressType %d \
           , address= %s, primaryPhy=%d, secondaryPhy=%d, advertisingSid=%s \
           , txPower= %d , rssi=%d, periodicAdvInt=%s ",
            intToHexString(eventType).c_str(),
            addressType, address.c_str(), primaryPhy, secondaryPhy,
            intToHexString(advertisingSid).c_str(), txPower, rssi,
            intToHexString(periodicAdvInt).c_str());
  }

  if (advData.empty()) {
    return;
  }

  if (!mScanManager) return;

  std::vector<Uuid> remoteUuids = parseUuids(advData);

  std::vector<uint8_t> legacyAdvData(62);
  if(advData.size() > 62)
    legacyAdvData.assign(advData.begin(), advData.begin()+62);
  else
    legacyAdvData.assign(advData.begin(), advData.end());

  std::string tem (legacyAdvData.begin(), legacyAdvData.end());
  for (ScanClient *client : mScanManager->getRegularScanQueue()) {
    if (client->uuids.size() > 0) {
      int matches = 0;
      for (Uuid search : client->uuids) {
        for (Uuid remote : remoteUuids) {
          if (remote == search) {
            ++matches;
            break; // Only count 1st match in case of duplicates
          }
        }
      }

      if (matches < client->uuids.size()) {
        continue;
      }
    }

    ScannerMap::App *app = mScannerMap->getById(client->scannerId);
    CHECK_PARAM_VOID(app);

    ScanSettings *settings = client->settings;
    std::vector<uint8_t> scanRecordData;
    // This is for compability with applications that assume fixed size scan data.
    if (settings->getLegacy()) {
      if ((eventType & ET_LEGACY_MASK) == 0) {
        // If this is legacy scan, but nonlegacy result - skip.
        continue;
      } else {
        // Some apps are used to fixed-size advertise data.
        scanRecordData.insert(scanRecordData.begin(), legacyAdvData.begin(), legacyAdvData.end());
        legacyAdvData.clear();
      }
    } else {
      scanRecordData.insert(scanRecordData.begin(), advData.begin(), advData.end());
      advData.clear();
    }
    std::string t(scanRecordData.begin(), scanRecordData.end());

    ScanResult *result =
            new ScanResult(address, eventType, primaryPhy, secondaryPhy, advertisingSid,
                          txPower, rssi, periodicAdvInt,
                          ScanRecord::parseFromBytes(scanRecordData));

    if (!matchesFilters(client, result)) {
      continue;
    }

    if ((settings->getCallbackType() & ScanSettings::CALLBACK_TYPE_ALL_MATCHES) == 0) {
       continue;
    }

    try {
      if (app->callback != NULL) {
        app->callback->onScanResult(result);
      } else {
       ALOGE(LOGTAG "onScanResult() - No Callback");
       return;
      }
    } catch (std::exception& e) {
      ALOGE(LOGTAG " Exception: %s", e.what());
      mScannerMap->remove(client->scannerId);
      if (!mScanManager) return;
      mScanManager->stopScan(client);
    }
  }
}

void GattLibService::onScannerRegisteredCbApp(ScannerMap::App *cbApp, int status, int scannerId)
{
  if (DBG) {
    ALOGD(LOGTAG " onScannerRegisteredCbApp()-> entered");
  }

  if (cbApp->callback != NULL)
    cbApp->callback->onScannerRegistered(status, scannerId);

  if (DBG) {
    ALOGD(LOGTAG " onScannerRegisteredCbApp()-> exited");
  }

}
void GattLibService::onScannerRegistered(int status, int scannerId, Uuid app_uuid)
{
  Uuid uuid = app_uuid;
  ALOGE(LOGTAG "onScannerRegistered scanner id %d", scannerId);

  if (DBG) {
    ALOGD(LOGTAG " onScannerRegistered() - UUID=%s, scannerId=%d \
              , status=%d", uuid.ToString().c_str(),
              scannerId, status);
  }

  // First check the callback map
  ScannerMap::App *cbApp = mScannerMap->getByUuid(uuid);
  if (cbApp != NULL) {
    if (status == 0) {
      cbApp->id = scannerId;
      // If app is callback based, setup a death recipient. App will initiate the start.
      // Otherwise, if PendingIntent based, start the scan directly.
      if (cbApp->callback == NULL) {
        ALOGE(LOGTAG "onScannerRegistered() - No Callback");
        return;
      }
    } else {
        mScannerMap->remove(scannerId);
    }
    if (cbApp->callback != NULL) {
        /* onScannerRegistered app callback should be given in different thread context.
         * So it will not block gatt thread while starting the scan
         * from onScannerRegistered app callback. */
        std::thread (onScannerRegisteredCbApp, cbApp, status, scannerId).detach();
    }
  }
}

void GattLibService::onClientRegistered(int status, int clientIf, Uuid app_uuid)
{
  Uuid uuid = app_uuid;
  if (DBG) {
    ALOGD(LOGTAG " onClientRegistered() - Uuid=%s, clientIf=%d",
                                                      uuid.ToString().c_str(), clientIf);
  }
  ClientMap::App *app = mClientMap->getByUuid(uuid);
  if (app != NULL) {
    if (status == 0) {
      app->id = clientIf;
    } else {
        mClientMap->remove(uuid);
    }
    app->callback->onClientRegistered(status, clientIf);
  }
}

void GattLibService::onConnected(int clientIf, int connId, int status, string address)
{
  if (DBG) {
    ALOGD(LOGTAG " onConnected() - clientIf=%d, connId=%d, address=%s",
             clientIf, connId, address.c_str());
  }

  if (status == 0) {
    mClientMap->addConnection(clientIf, connId, address);
  }
  ClientMap::App *app = mClientMap->getById(clientIf);
  if (app != NULL) {
    app->callback->onConnectionState(status, clientIf, (status == 0), address);
  }
}

void GattLibService::onDisconnected(int clientIf, int connId, int status,
                                                                      string address)
{
  if (DBG) {
    ALOGD(LOGTAG " onDisconnected() - clientIf=%d, connId=%d, address=%s",
              clientIf, connId, address.c_str());
  }

  mClientMap->removeConnection(clientIf, connId);
  ClientMap::App *app = mClientMap->getById(clientIf);
  if (app != NULL) {
    app->callback->onConnectionState(status, clientIf, false, address);
  }
}

void GattLibService::onClientPhyUpdate(int connId, int txPhy, int rxPhy, int status)
{
  if (DBG) {
    ALOGD(LOGTAG " onClientPhyUpdate() - connId=%d, status=%d",
                  connId, status);
  }

  string address = mClientMap->addressByConnId(connId);
  if (address.empty()) {
    return;
  }

  ClientMap::App *app = mClientMap->getByConnId(connId);
  CHECK_PARAM_VOID(app)

  app->callback->onPhyUpdate(address, txPhy, rxPhy, status);
}

void GattLibService::onClientPhyRead(int clientIf, string address, int txPhy,
                            int rxPhy, int status)
{
  if (DBG) {
    ALOGD(LOGTAG " onClientPhyRead() - address=%s, status=%d, clientIf=%d",
               address.c_str(), status, clientIf);
  }

  int connId = mClientMap->connIdByAddress(clientIf, address);
  if (connId < 0) {
    ALOGD(LOGTAG " onClientPhyRead() - no connection to %s", address.c_str());
    return;
  }

  ClientMap::App *app = mClientMap->getByConnId(connId);
  CHECK_PARAM_VOID(app)

  app->callback->onPhyRead(address, txPhy, rxPhy, status);
}

void GattLibService::onClientConnUpdate(int connId, int interval, int latency,
                                                    int timeout, int status)
{
  if (DBG) {
    ALOGD(LOGTAG " onClientConnUpdate() - connId=%d, status=%d",
               connId, status);
  }

  string address = mClientMap->addressByConnId(connId);
  if (address.empty()) {
    return;
  }

  ClientMap::App *app = mClientMap->getByConnId(connId);
  CHECK_PARAM_VOID(app);

  app->callback->onConnectionUpdated(address, interval, latency, timeout, status);
}

void GattLibService::onServerPhyUpdate(int connId, int txPhy, int rxPhy, int status)
{
  if (DBG) {
    ALOGD(LOGTAG "  onServerPhyUpdate() - connId=%d, status=%d",
                             connId, status);
  }

  string address = mServerMap->addressByConnId(connId);
  if (address.empty()) {
    return;
  }

  ServerMap::App *app = mServerMap->getByConnId(connId);
  CHECK_PARAM_VOID(app);

  app->callback->onPhyUpdate(address, txPhy, rxPhy, status);
}

void GattLibService::onServerPhyRead(int serverIf, string address, int txPhy,
                        int rxPhy, int status)
{
  if (DBG) {
    ALOGD(LOGTAG " onServerPhyRead() - address=%s, status=%d", address.c_str(), status);
  }

  int connId = mServerMap->connIdByAddress(serverIf, address);
  if (connId < 0) {
    ALOGD(LOGTAG "onServerPhyRead() - no connection to %s", address.c_str());
    return;
  }

  ServerMap::App *app = mServerMap->getByConnId(connId);
  CHECK_PARAM_VOID(app);

  app->callback->onPhyRead(address, txPhy, rxPhy, status);
}

void GattLibService::onServerConnUpdate(int connId, int interval, int latency,
                  int timeout, int status)
{
  if (DBG) {
    ALOGD(LOGTAG " onServerConnUpdate() - connId=%d, status=%d", connId, status);
  }

  string address = mServerMap->addressByConnId(connId);
  if (address.empty()) {
    return;
  }

  ServerMap::App *app = mServerMap->getByConnId(connId);
  CHECK_PARAM_VOID(app);

  app->callback->onConnectionUpdated(address, interval, latency, timeout, status);
}

void GattLibService::onSearchCompleted(int connId, int status)
{
  if (DBG) {
    ALOGD(LOGTAG " onSearchCompleted() - connId=%d, status=%d",
                 connId, status);
  }
  mNative->gattClientGetGattDbNative(connId);
}

void GattLibService::onGetGattDb(int connId, std::vector<GattDbElement*> db)
{
  string address = mClientMap->addressByConnId(connId);

  if (DBG) {
    ALOGD(LOGTAG " onGetGattDb() - address=%s", address.c_str());
  }

  ClientMap::App *app = mClientMap->getByConnId(connId);
  if (app == NULL || app->callback == NULL) {
    ALOGE(LOGTAG " app or callback is null");
    return;
  }

  std::vector<GattService*> dbOut;

  GattService *currSrvc = NULL;
  GattCharacteristic *currChar = NULL;
  for(GattDbElement *el : db ) {
  switch (el->type) {
    case GattDbElement::TYPE_PRIMARY_SERVICE:
    case GattDbElement::TYPE_SECONDARY_SERVICE:
      if (DBG) {
          ALOGD(LOGTAG " got service with UUID=%s, id=%d",
                       el->uuid.ToString().c_str(), el->id);
      }

      currSrvc = new GattService(el->uuid, el->id, el->type);
      dbOut.push_back(currSrvc);
      break;
    case GattDbElement::TYPE_CHARACTERISTIC:
      if (DBG) {
          ALOGD(LOGTAG " got characteristic with UUID=%s, id=%d",
                       el->uuid.ToString().c_str(), el->id);
      }

      currChar = new GattCharacteristic(el->uuid, el->id, el->properties, 0);
      currSrvc->addCharacteristic(currChar);
      break;
    case GattDbElement::TYPE_DESCRIPTOR:
      if (DBG) {
          ALOGD(LOGTAG " got descriptor with UUID=%s, id=%d",
                       el->uuid.ToString().c_str(), el->id);
      }

      currChar->addDescriptor(new GattDescriptor(el->uuid, el->id, 0));
      break;
    case GattDbElement::TYPE_INCLUDED_SERVICE:
      if (DBG) {
          ALOGD(LOGTAG " got included service with UUID=%s, id=%d, startHandle=%d",
                     el->uuid.ToString().c_str(), el->id, el->startHandle);
      }

      currSrvc->addIncludedService(
              new GattService(el->uuid, el->startHandle, el->type));
      break;
    default:
        ALOGD(LOGTAG " got unknown element with type=%d and Uuid=%s, id=%d",
                            el->type, el->uuid.ToString().c_str(), el->id);
  }
}

  // Search is complete when there was error, or nothing more to process
  mGattClientDatabases.insert({{connId, dbOut}});
  app->callback->onSearchComplete(address, dbOut, 0 /* status */);
}

void GattLibService::onRegisterForNotifications(int connId, int status,
                          int registered, int handle)
{
  string address = mClientMap->addressByConnId(connId);

  if (DBG) {
    ALOGD(LOGTAG " onRegisterForNotifications() - address=%s, status=%d, registered=%d, handle=%d",
                 address.c_str(), status, registered, handle);
  }
}

void GattLibService::onNotify(int connId, string address, int handle, bool isNotify,
                                uint8_t *data, int length)
{
  if (DBG) {
    if (data != NULL) {
      ALOGD(LOGTAG " onNotify() - address=%s, handle=%d, length=%d",
                address.c_str(), handle, length);
    }
  }

  if (!permissionCheck(connId, handle)) {
    ALOGW(LOGTAG " onNotify() - permission check failed!");
    return;
  }

  ClientMap::App *app = mClientMap->getByConnId(connId);
  if (app != NULL) {
    app->callback->onNotify(address, handle, data, length);
  }
}

void GattLibService::onReadCharacteristic(int connId, int status, int handle,
                                                  uint8_t *data, int length)
{
  string address = mClientMap->addressByConnId(connId);

  if (DBG) {
    if (data != NULL) {
      ALOGD(LOGTAG " onReadCharacteristic() - address=%s, status=%d, length=%d",
                    address.c_str(), status, length);
    }
  }

  ClientMap::App *app = mClientMap->getByConnId(connId);
  if (app != NULL) {
    app->callback->onCharacteristicRead(address, status, handle, data, length);
  }
}

void GattLibService::onWriteCharacteristic(int connId, int status, int handle)
{
  string address = mClientMap->addressByConnId(connId);

  if (VDBG) {
    ALOGD(LOGTAG " onWriteCharacteristic() - address=%s, status=%d",
                              address.c_str(), status);
  }

  ClientMap::App *app = mClientMap->getByConnId(connId);
  CHECK_PARAM_VOID(app);

  if (!app->isCongested) {
    app->callback->onCharacteristicWrite(address, status, handle);
  } else {
    if (status == GATT_CONNECTION_CONGESTED) {
      status = GATT_SUCCESS;
    }
    CallbackInfo *callbackInfo = new CallbackInfo(address, status, handle);
    app->queueCallback(callbackInfo);
  }
}

void GattLibService::onExecuteCompleted(int connId, int status)
{
  string address = mClientMap->addressByConnId(connId);
  if (VDBG) {
    ALOGD(LOGTAG " onExecuteCompleted() - address=%s, status=%d",
              address.c_str(), status);
  }

  ClientMap::App *app = mClientMap->getByConnId(connId);
  if (app != NULL) {
    app->callback->onExecuteWrite(address, status);
  }
}

void GattLibService::onReadDescriptor(int connId, int status, int handle, uint8_t *data, int length)
{
  string address = mClientMap->addressByConnId(connId);

  if (DBG) {
    if (data != NULL) {
      ALOGD(LOGTAG " onReadDescriptor() - address=%s, status=%d, length=%d",
                            address.c_str(), status, length);
    }
  }

  ClientMap::App *app = mClientMap->getByConnId(connId);
  if (app != NULL) {
    app->callback->onDescriptorRead(address, status, handle, data, length);
  }
}

void GattLibService::onWriteDescriptor(int connId, int status, int handle)
{
  string address = mClientMap->addressByConnId(connId);

  if (DBG) {
    ALOGD(LOGTAG " onWriteDescriptor() - address=%s, status=%d", address.c_str(), status);
  }

  ClientMap::App *app = mClientMap->getByConnId(connId);
  if (app != NULL) {
    app->callback->onDescriptorWrite(address, status, handle);
  }
}

void GattLibService::onReadRemoteRssi(int clientIf, string address, int rssi, int status)
{
  if (DBG) {
    ALOGD(LOGTAG " onReadRemoteRssi() - clientIf=%d, address=%s, rssi=%d, status=%d",
                  clientIf, address.c_str(), rssi, status);
  }

  ClientMap::App *app = mClientMap->getById(clientIf);
  if (app != NULL) {
    app->callback->onReadRemoteRssi(address, rssi, status);
  }
}

void GattLibService::onScanFilterEnableDisabled(int action, int status, int clientIf)
{
  if (DBG) {
    ALOGD(LOGTAG " onScanFilterEnableDisabled() - clientIf=%d, status=%d, action=%d",
              clientIf, status, action);
  }

  if (!mScanManager) return;
  mScanManager->callbackDone(clientIf, status);
}

void GattLibService::onScanFilterParamsConfigured(int action, int status,
                                                            int clientIf, int availableSpace)
{
  if (DBG) {
    ALOGD(LOGTAG " onScanFilterParamsConfigured() - clientIf=%d, status=%d, action=%d,\
                        availableSpace=%d", clientIf, status, action, availableSpace);
  }

  if (!mScanManager) return;
  mScanManager->callbackDone(clientIf, status);
}

void GattLibService::onScanFilterConfig(int action, int status, int clientIf, int filterType,
                      int availableSpace)
{
  if (DBG) {
    ALOGD(LOGTAG " onScanFilterConfig() - clientIf=%d, action=%d, status=%d, filterType=%d,\
              availableSpace=%d", clientIf, action, status, filterType, availableSpace);
  }

  if (!mScanManager) return;
  mScanManager->callbackDone(clientIf, status);
}

void GattLibService::onBatchScanStorageConfigured(int status, int clientIf)
{
  if (DBG) {
    ALOGD(LOGTAG " onBatchScanStorageConfigured() - clientIf=%d, status=%d",
                   clientIf, status);
  }

  if (!mScanManager) return;
  mScanManager->callbackDone(clientIf, status);
}

void GattLibService::onBatchScanStarted(int status, int clientIf)
{
  if (DBG) {
    ALOGD(LOGTAG " onBatchScanStartStopped() - clientIf=%d, status=%d",
                clientIf, status);
  }

  if (!mScanManager) return;
  mScanManager->callbackDone(clientIf, status);
}

void GattLibService::onBatchScanStopped(int status, int clientIf)
{
  if (DBG) {
    ALOGD(LOGTAG " onBatchScanStartStopped() - clientIf=%d, status=%d",
               clientIf, status);
  }

  if (!mScanManager) return;
  mScanManager->callbackDone(clientIf, status);
}

void GattLibService::onBatchScanReports(int status, int scannerId, int reportType,
                                             int numRecords, std::vector<uint8_t> recordData)
{
  if (DBG) {
    ALOGD(LOGTAG " onBatchScanReports() - scannerId=%d, status=%d, reportType=%d, numRecords=%d",
                scannerId, status, reportType, numRecords);
  }

  if (!mScanManager) return;
  mScanManager->callbackDone(scannerId, status);
  std::unordered_set<ScanResult*> results;
  results = parseBatchScanResults(numRecords, reportType, recordData);
  if (reportType == ScanManager::SCAN_RESULT_TYPE_TRUNCATED) {
    // We only support single client for truncated mode.
    ScannerMap::App *app = mScannerMap->getById(scannerId);
    CHECK_PARAM_VOID(app);

    if (app->callback != NULL) {
      std::vector<ScanResult*>results_vec(results.begin(), results.end());
      app->callback->onBatchScanResults(results_vec);
    } else {
      ALOGE(LOGTAG "onBatchScanReports() - No Callback");
      return;
    }
  } else {
    for (ScanClient *client : mScanManager->getFullBatchScanQueue()) {
      // Deliver results for each client.
      deliverBatchScan(client, results);
    }
  }
}

bool GattLibService::matchesFilters(ScanClient *client, ScanResult *scanResult)
{
  if (client->filters.empty()) {
    return true;
  }
  for (ScanFilter *filter : client->filters) {
    if (filter->matches(scanResult)) {
      return true;
    }
  }
  return false;
}

void GattLibService::sendBatchScanResults(ScannerMap::App *app, ScanClient *client,
                      std::vector<ScanResult*> results)
{
  try{
    if (app->callback != NULL) {
      app->callback->onBatchScanResults(results);
    } else {
        ALOGE(LOGTAG "sendBatchScanResults() - No Callback");
    }
  } catch (std::exception &e) {
    ALOGE(LOGTAG " Exception: %s", e.what());
    mScannerMap->remove(client->scannerId);
    if (!mScanManager) return;
    mScanManager->stopScan(client);
    }
}

void GattLibService::deliverBatchScan(ScanClient *client,
                        std::unordered_set<ScanResult*> allResults)
{
  ScannerMap::App *app = mScannerMap->getById(client->scannerId);
  CHECK_PARAM_VOID(app);

  if (client->filters.empty()) {
    std::vector<ScanResult*> allResults_vec;
    std::copy(allResults.begin(), allResults.end(),
                  inserter(allResults_vec, allResults_vec.begin()));
    sendBatchScanResults(app, client, allResults_vec);
    // TODO: Question to reviewer: Shouldn't there be a return here?
  }
  // Reconstruct the scan results.
  std::vector<ScanResult*> results;
  for (ScanResult *scanResult : allResults) {
    if (matchesFilters(client, scanResult)) {
      results.push_back(scanResult);
    }
  }
  sendBatchScanResults(app, client, results);
}

std::unordered_set<ScanResult*> GattLibService::parseBatchScanResults
                      (int numRecords, int reportType, std::vector<uint8_t> batchRecord)
{
  if (numRecords == 0) {
    return std::unordered_set<ScanResult*>();
  }

  if (reportType == ScanManager::SCAN_RESULT_TYPE_TRUNCATED) {
    return parseTruncatedResults(numRecords, batchRecord);
  } else {
    return parseFullResults(numRecords, batchRecord);
  }
}

std::unordered_set<ScanResult*> GattLibService::parseTruncatedResults(int numRecords,
                                    std::vector<uint8_t> batchRecord)
{

  std::string s( batchRecord.begin(), batchRecord.end());
  if (DBG) {
    ALOGD(LOGTAG " batch record %s ", s.c_str());
  }
  std::unordered_set<ScanResult*> results(numRecords);

  for (int i = 0; i < numRecords; ++i) {
    std::vector<uint8_t> record = extractBytes(batchRecord,
                                                i * TRUNCATED_RESULT_SIZE, TRUNCATED_RESULT_SIZE);
    std::vector<uint8_t> address = extractBytes(record, 0, 6);
    reverse(address);
    std::string str_address( address.begin(), address.end() );
    int rssi = record[8];

    results.insert(new ScanResult(str_address,
                               ScanRecord::parseFromBytes(std::vector<uint8_t>()), rssi));
  }
  return results;
}

std::unordered_set<ScanResult*> GattLibService::parseFullResults(int numRecords,
                                                        std::vector<uint8_t> batchRecord)
{
  if (DBG) {
    ALOGD(LOGTAG " Batch record : %d", batchRecord.size());
  }
  std::unordered_set<ScanResult*> results(numRecords);
  int position = 0;

  int count = 0;
  while (count < numRecords) {
    count ++;
    std::vector<uint8_t> address = extractBytes(batchRecord, position, 6);
    // TODO: remove temp hack.
    reverse(address);
    std::string str_address( address.begin(), address.end() );
    position += 6;
    // Skip address type.
    position++;
    // Skip tx power level.
    position++;
    int rssi = batchRecord[position++];
    position += 2;

    // Combine advertise packet and scan response packet.
    int advertisePacketLen = batchRecord[position++];
    std::vector<uint8_t> advertiseBytes = extractBytes(batchRecord, position,
                                                                        advertisePacketLen);
    position += advertisePacketLen;
    int scanResponsePacketLen = batchRecord[position++];
    std::vector<uint8_t> scanResponseBytes = extractBytes(batchRecord, position,
                                                                        scanResponsePacketLen);
    position += scanResponsePacketLen;
    std::vector<uint8_t> scanRecord(advertisePacketLen + scanResponsePacketLen);
    scanRecord.insert(scanRecord.begin(),advertiseBytes.begin(),
                                                    advertiseBytes.begin()+ advertisePacketLen);
    scanRecord.insert(scanRecord.begin()+advertisePacketLen, scanResponseBytes.begin(),
                            scanResponseBytes.begin() +scanResponsePacketLen);
    if (DBG) {
      std::string scanRecordString(scanRecord.begin(), scanRecord.end());
      ALOGD(LOGTAG "parseFullResults ScanRecord : %s " , scanRecordString.c_str());
    }
    results.insert(new ScanResult(str_address, ScanRecord::parseFromBytes(scanRecord), rssi));
  }
  return results;
}

// Reverse byte array.
void GattLibService::reverse(std::vector<uint8_t> &address)
{
  int len = static_cast<int>(address.size());
  for (int i = 0; i < len / 2; ++i) {
    uint8_t b = address[i];
    address[i] = address[len - 1 - i];
    address[len - 1 - i] = b;
  }
}

void GattLibService::onBatchScanThresholdCrossed(int clientIf)
{
  if (DBG) {
    ALOGD(LOGTAG " onBatchScanThresholdCrossed() - clientIf=%d", clientIf);
  }
  flushPendingBatchResults(clientIf);
}

AdvtFilterOnFoundOnLostInfo* GattLibService::createOnTrackAdvFoundLostObject(
                                                                  int clientIf, int advPktLen,
                                                                  std::vector<uint8_t> advPkt,
                                                                  int scanRspLen,
                                                                  std::vector<uint8_t> scanRsp,
                                                                  int filtIndex, int advState,
                                                                  int advInfoPresent,
                                                                  string address,
                                                                  int addrType, int txPower,
                                                                  int rssiValue, int timeStamp)
{

  return new AdvtFilterOnFoundOnLostInfo(clientIf, advPktLen, advPkt, scanRspLen, scanRsp,
         filtIndex, advState, advInfoPresent, address, addrType, txPower, rssiValue,
         timeStamp);
}

void GattLibService::onTrackAdvFoundLost(AdvtFilterOnFoundOnLostInfo *trackingInfo)
{
  if (DBG) {
    ALOGD(LOGTAG " onTrackAdvFoundLost() - scannerId=%d, address=%s, adv_state=%d ",
              trackingInfo->getClientIf(), trackingInfo->getAddress().c_str(),
              trackingInfo->getAdvState());
  }

  ScannerMap::App *app = mScannerMap->getById(trackingInfo->getClientIf());
  if (app == NULL || app->callback == NULL) {
    ALOGE(LOGTAG " app or callback is null");
    return;
  }
  if (mScanManager == NULL) {
    ALOGE(LOGTAG "scanManager not instantiated");
    return;
  }
  int advertiserState = trackingInfo->getAdvState();

  ScanResult *result =
          new ScanResult(trackingInfo->getAddress(),
                                  ScanRecord::parseFromBytes(trackingInfo->getResult()),
                  trackingInfo->getRSSIValue());
  for (ScanClient *client : mScanManager->getRegularScanQueue()) {
    if (client->scannerId == trackingInfo->getClientIf()) {
        ScanSettings *settings = client->settings;
      if ((advertiserState == ADVT_STATE_ONFOUND) && (
              (settings->getCallbackType() & ScanSettings::CALLBACK_TYPE_FIRST_MATCH)
                      != 0)) {
        if (app->callback != NULL) {
          app->callback->onFoundOrLost(true, result);
        } else {
          ALOGE(LOGTAG "onTrackAdvFoundLost() - No callback");
        }
      } else if ((advertiserState == ADVT_STATE_ONLOST) && (
              (settings->getCallbackType() & ScanSettings::CALLBACK_TYPE_MATCH_LOST)
                      != 0)) {
        if (app->callback != NULL) {
          app->callback->onFoundOrLost(false, result);
        } else {
          ALOGE(LOGTAG "onTrackAdvFoundLost() - No Callback");
          return;
        }
      } else {
        if (DBG) {
          ALOGD(LOGTAG " Not reporting onlost/onfound :%d, scannerId=%d, callbackType=%d ",
                    advertiserState, client->scannerId, settings->getCallbackType());
        }
      }
    }
  }
}

void GattLibService::onScanParamSetupCompleted(int status, int scannerId)
{
  ScannerMap::App *app = mScannerMap->getById(scannerId);
  if (app == NULL || app->callback == NULL) {
    ALOGE(LOGTAG " Advertise app or callback is null");
    return;
  }
  if (DBG) {
    ALOGD(LOGTAG " onScanParamSetupCompleted : %d", status);
  }
}

// callback from ScanManager for dispatch of errors apps.
void GattLibService::onScanManagerErrorCallback(int scannerId, int errorCode)
{
  ScannerMap::App *app = mScannerMap->getById(scannerId);
  if (app == NULL || app->callback == NULL) {
    ALOGE(LOGTAG " App or callback is null");
    return;
  }
  if (app->callback != NULL) {
    app->callback->onScanManagerErrorCallback(errorCode);
  } else {
    ALOGE(LOGTAG " onScanManagerErrorCallback() -  No Callback");
    return;
    }
}

void GattLibService::onConfigureMTU(int connId, int status, int mtu)
{
  string address = mClientMap->addressByConnId(connId);

  if (DBG) {
    ALOGD(LOGTAG " onConfigureMTU() address=%s, status=%d, mtu=%d",
                  address.c_str(), status, mtu);
  }

  ClientMap::App *app = mClientMap->getByConnId(connId);
  if (app != NULL) {
    app->callback->onConfigureMTU(address, mtu, status);
  }
}

void GattLibService::onClientCongestion(int connId, bool congested)
{
  if (VDBG) {
    ALOGD(LOGTAG " onClientCongestion() - connId=%d , congested=%d", connId, congested);
  }

  ClientMap::App *app = mClientMap->getByConnId(connId);

  if (app != NULL) {
    app->isCongested = congested;
    while (!app->isCongested) {
      CallbackInfo *callbackInfo = app->popQueuedCallback();
      if (callbackInfo == NULL) {
        return;
      }
      app->callback->onCharacteristicWrite(callbackInfo->address, callbackInfo->status,
                callbackInfo->handle);
    }
  }
}

/**************************************************************************
 * Callback functions - SERVER
 *************************************************************************/

void GattLibService::onServerRegistered(int status, int serverIf, Uuid app_uuid)
{
  Uuid uuid = app_uuid;
  if (DBG) {
    ALOGD(LOGTAG "onServerRegistered() - UUID=%s, serverIf=%d",
                  uuid.ToString().c_str(), serverIf);
  }

  ServerMap::App *app = mServerMap->getByUuid(uuid);
  if (app != NULL) {
    app->id = serverIf;
    app->callback->onServerRegistered(status, serverIf);
  }
}

void GattLibService::onServiceAdded(int status, int serverIf,
          std::vector<GattDbElement*> service)
{
  if (DBG) {
    ALOGD(LOGTAG " onServiceAdded(), status=%d", status);
  }

  if (status != 0) {
    return;
  }

  GattDbElement *svcEl = service.at(0);
  int srvcHandle = svcEl->attributeHandle;

  GattService *svc = NULL;

  for (GattDbElement *el : service) {
    if (el->type == GattDbElement::TYPE_PRIMARY_SERVICE) {
      mHandleMap->addService(serverIf, el->attributeHandle, el->uuid,
             GattService::SERVICE_TYPE_PRIMARY, 0, false);
      svc = new GattService(svcEl->uuid, svcEl->attributeHandle,
             GattService::SERVICE_TYPE_PRIMARY);
    } else if (el->type == GattDbElement::TYPE_SECONDARY_SERVICE) {
       mHandleMap->addService(serverIf, el->attributeHandle, el->uuid,
               GattService::SERVICE_TYPE_SECONDARY, 0, false);
       svc = new GattService(svcEl->uuid, svcEl->attributeHandle,
               GattService::SERVICE_TYPE_SECONDARY);
    } else if (el->type == GattDbElement::TYPE_CHARACTERISTIC) {
       mHandleMap->addCharacteristic(serverIf, el->attributeHandle,
                        el->uuid, srvcHandle);
       svc->addCharacteristic(
               new GattCharacteristic(el->uuid, el->attributeHandle, el->properties,
                       el->permissions));
    } else if (el->type == GattDbElement::TYPE_DESCRIPTOR) {
       mHandleMap->addDescriptor(serverIf, el->attributeHandle, el->uuid, srvcHandle);
       std::vector<GattCharacteristic*> chars = svc->getCharacteristics();
       chars.at(chars.size() - 1)
               ->addDescriptor(new GattDescriptor(el->uuid, el->attributeHandle,
                       el->permissions));
    }
  }
  mHandleMap->setStarted(serverIf, srvcHandle, true);

  ServerMap::App *app = mServerMap->getById(serverIf);
  if (app != NULL) {
    for (GattDbElement *el : service)
      delete el;
    app->callback->onServiceAdded(status, svc);
  }
}

void GattLibService::onServiceStopped(int status, int serverIf, int srvcHandle)
{
  if (DBG) {
    ALOGD(LOGTAG " onServiceStopped() srvcHandle=%d, status=%d", srvcHandle, status);
  }
  if (status == 0) {
    mHandleMap->setStarted(serverIf, srvcHandle, false);
  }
  stopNextService(serverIf, status);
}

void GattLibService::onServiceDeleted(int status, int serverIf, int srvcHandle)
{
  if (DBG) {
    ALOGD(LOGTAG " onServiceDeleted() srvcHandle=%d, status=%d", srvcHandle, status);
  }
  mHandleMap->deleteService(serverIf, srvcHandle);
}

void GattLibService::onClientConnected(string address, bool connected,
                                            int connId, int serverIf)
{
  if (DBG) {
    ALOGD(LOGTAG " onClientConnected() connId=%d, address=%s, connected=%d",
                  connId, address.c_str(), connected);
  }

  ServerMap::App *app = mServerMap->getById(serverIf);
  CHECK_PARAM_VOID(app);

  if (connected) {
    mServerMap->addConnection(serverIf, connId, address);
  } else {
      mServerMap->removeConnection(serverIf, connId);
  }

  app->callback->onConnectionState((uint8_t) 0, serverIf, connected, address);
}

void GattLibService::onServerReadCharacteristic(string address, int connId,
        int transId, int handle, int offset, bool isLong)
{
  if (VDBG) {
    ALOGD(LOGTAG " onServerReadCharacteristic() connId=%d, address=%s, handle=%d,\
                transId=%d, offset=%d", connId, address.c_str(), handle, transId, offset);
  }

  HandleMap::Entry *entry = mHandleMap->getByHandle(handle);
  CHECK_PARAM_VOID(entry);

  mHandleMap->addRequest(transId, handle);

  ServerMap::App *app = mServerMap->getById(entry->serverIf);
  CHECK_PARAM_VOID(app);

  app->callback->onCharacteristicReadRequest(address, transId, offset, isLong, handle);
}

void GattLibService::onServerReadDescriptor(string address, int connId, int transId,
                  int handle, int offset, bool isLong)
{
  if (VDBG) {
    ALOGD(LOGTAG " onServerReadDescriptor() connId=%d, address=%s, handle=%d,\
                  transId=%d, offset=%d",
                 connId, address.c_str(), handle, transId, offset);
  }

  HandleMap::Entry *entry = mHandleMap->getByHandle(handle);
  CHECK_PARAM_VOID(entry);

  mHandleMap->addRequest(transId, handle);

  ServerMap::App *app = mServerMap->getById(entry->serverIf);
  CHECK_PARAM_VOID(app);

  app->callback->onDescriptorReadRequest(address, transId, offset, isLong, handle);
}

void GattLibService::onServerWriteCharacteristic(string address, int connId,
                                                          int transId, int handle,
                                                          int offset, int length,
                                                          bool needRsp, bool isPrep,
                                                          uint8_t *data)
{
  if (VDBG) {
    ALOGD(LOGTAG " onServerWriteCharacteristic() connId=%d, address=%s, handle=%d,\
                    requestId=%d, isPrep=%d, offset=%d",
                 connId, address.c_str(), handle, transId, isPrep, offset);
  }

  HandleMap::Entry *entry = mHandleMap->getByHandle(handle);
  CHECK_PARAM_VOID(entry);

  mHandleMap->addRequest(transId, handle);

  ServerMap::App *app = mServerMap->getById(entry->serverIf);
  CHECK_PARAM_VOID(app);

  app->callback->onCharacteristicWriteRequest(address, transId, offset, length, isPrep, needRsp,
          handle, data);
}

void GattLibService::onServerWriteDescriptor(string address, int connId, int transId,
                                                    int handle, int offset, int length,
                                                    bool needRsp, bool isPrep, uint8_t *data)
{
  if (VDBG) {
    ALOGD(LOGTAG "onAttributeWrite() connId=%d, address=%s, handle=%d, requestId=%d,\
                          isPrep=%d, offset=%d",
          connId, address.c_str(), handle, transId, isPrep, offset);
  }

  HandleMap::Entry *entry = mHandleMap->getByHandle(handle);
  CHECK_PARAM_VOID(entry);

  mHandleMap->addRequest(transId, handle);

  ServerMap::App *app = mServerMap->getById(entry->serverIf);
  CHECK_PARAM_VOID(app);

  app->callback->onDescriptorWriteRequest(address, transId, offset, length, isPrep, needRsp,
          handle, data);
}

void GattLibService::onExecuteWrite(string address, int connId, int transId, int execWrite)
{
  if (DBG) {
    ALOGD(LOGTAG " onExecuteWrite() connId=%d, address=%s, transId=%d",
                  connId, address.c_str(), transId);
  }

  ServerMap::App *app = mServerMap->getByConnId(connId);
  CHECK_PARAM_VOID(app);

  app->callback->onExecuteWrite(address, transId, execWrite == 1);
}

void GattLibService::onResponseSendCompleted(int status, int attrHandle)
{
  if (DBG) {
    ALOGD(LOGTAG " onResponseSendCompleted() handle=%d", attrHandle);
  }
}

void GattLibService::onNotificationSent(int connId, int status)
{
  if (DBG) {
     ALOGD(LOGTAG " onNotificationSent() connId=%d, status=%d", connId, status);
  }

  string address = mServerMap->addressByConnId(connId);
  if (address.empty()) {
    return;
  }

  ServerMap::App *app = mServerMap->getByConnId(connId);
  CHECK_PARAM_VOID(app);

  if (!app->isCongested) {
    app->callback->onNotificationSent(address, status);
  } else {
    if (status == GATT_CONNECTION_CONGESTED) {
      status = GATT_SUCCESS;
    }
    app->queueCallback(new CallbackInfo(address, status));
  }
}

void GattLibService::onServerCongestion(int connId, bool congested)
{
  if (DBG) {
    ALOGD(LOGTAG " onServerCongestion() - connId=%d, congested=%d",
                       connId, congested);
  }

  ServerMap::App *app = mServerMap->getByConnId(connId);
  CHECK_PARAM_VOID(app);

  app->isCongested = congested;
  while (!app->isCongested) {
    CallbackInfo *callbackInfo = app->popQueuedCallback();
    if (callbackInfo == NULL) {
      return;
    }
    app->callback->onNotificationSent(callbackInfo->address, callbackInfo->status);
  }
}

void GattLibService::onMtuChanged(int connId, int mtu)
{
  if (DBG) {
    ALOGD(LOGTAG " onMtuChanged() - connId=%d, mtu=%d", connId, mtu);
  }

  string address = mServerMap->addressByConnId(connId);
  if (address.empty()) {
    return;
  }

  ServerMap::App *app = mServerMap->getByConnId(connId);
  CHECK_PARAM_VOID(app);

  app->callback->onMtuChanged(address, mtu);
}

GattLibService* GattLibService::getGatt()
{
  std::lock_guard<std::mutex> myLock(singletonLock);
  if (sGattService == NULL) {
    ALOGE(LOGTAG " getGatt() - gatt is NULL");
    return NULL;
  }
  return sGattService;
}

GattLibService* GattLibService::getInstance(const bt_interface_t *bt_interface)
{
  if(sGattService == NULL) {
    std::lock_guard<std::mutex> myLock(singletonLock);
    if(sGattService == NULL) {
      sGattService = new GattLibService(bt_interface);
    }
  }
  ALOGE(LOGTAG " getInstance() ");
  return sGattService;
}

GattLibService::~GattLibService()
{
  if(sGattService != NULL)
    cleanup();

  ALOGE(LOGTAG " Deinit Done");
}

GattLibService::GattLibService(const bt_interface_t *bt_interface)
{
  if (DBG) {
    ALOGD(LOGTAG " GattLibService()");
  }

  mNative = new GattNativeInterfaceV2(bt_interface);
  if (mNative == NULL) {
    ALOGE(LOGTAG "Gatt Native Interface init failed");
    return;
  }
  mGattDevice = new GattDevice();
  mAdvertiserManager = new AdvertiserManager(mNative);
  mScanManager = new ScanManager(mNative);
  mPeriodicScanManager = new PeriodicScanManager(mNative);
  start();
}

void GattLibService::start()
{
  mAdvertiserManager->start();

  if (!mScanManager) return;
  mScanManager->start();

  mPeriodicScanManager->start();
}

void GattLibService::cleanup()
{
  ALOGE(LOGTAG " cleanup() ");

  if(mNative != NULL) {
    delete(mNative);
  }
  mNative = NULL;

  if(mGattDevice != NULL) {
    delete(mGattDevice);
  }
  mGattDevice = NULL;

  if(mAdvertiserManager != NULL) {
    delete(mAdvertiserManager);
  }
  mAdvertiserManager = NULL;

  if(mScanManager != NULL) {
    delete(mScanManager);
  }
  mScanManager = NULL;

  if(mPeriodicScanManager != NULL) {
    delete(mPeriodicScanManager);
  }
  mPeriodicScanManager =NULL;

  std::unordered_map<int, std::vector<GattService*>>::iterator it = mGattClientDatabases.begin();
  for(; it!=mGattClientDatabases.end();++it) {
    for( GattService *svc : it->second)
      delete(svc);
  }
  mGattClientDatabases.clear();

  if(mScannerMap != NULL) {
    mScannerMap->clear();
  }
  mScannerMap = NULL;

  if(mClientMap != NULL) {
    mClientMap->clear();
  }
  mClientMap = NULL;

  if(mServerMap != NULL) {
    mServerMap->clear();
  }
  mServerMap = NULL;

  if(sGattService != NULL) {
    sGattService = NULL;
  }
}

bool GattLibService::isRestrictedCharUuid(const Uuid charUuid)
{
  return isHidUuid(charUuid);
}

bool GattLibService::isRestrictedSrvcUuid(const Uuid srvcUuid)
{
  return isFidoUUID(srvcUuid);
}

bool GattLibService::isHidUuid(const Uuid uuid)
{
  for (Uuid hidUuid : HID_UUIDS) {
    if (hidUuid == uuid) {
      return true;
    }
  }
  return false;
}

bool GattLibService::isFidoUUID(const Uuid uuid)
{
  for (Uuid fidoUuid : FIDO_UUIDS) {
    if (fidoUuid == uuid) {
      return true;
    }
  }
  return false;
}

bool GattLibService::permissionCheck(Uuid uuid)
{
  return !(isRestrictedCharUuid(uuid));
}

bool GattLibService::permissionCheck(int connId, int handle)
{
  auto map_ptr = mGattClientDatabases.find(connId);
  if(map_ptr == mGattClientDatabases.end()) {
     return false;
  }

  const std::vector<GattService*> db =  map_ptr->second;

  for (GattService *service : db) {
    for (GattCharacteristic *characteristic : service->getCharacteristics()) {
      if (handle == characteristic->getInstanceId()) {
        return !((isRestrictedCharUuid(characteristic->getUuid())
                || isRestrictedSrvcUuid(service->getUuid())));
      }

      for (GattDescriptor *descriptor : characteristic->getDescriptors()) {
        if (handle == descriptor->getInstanceId()) {
          return !((isRestrictedCharUuid(characteristic->getUuid())
                  || isRestrictedSrvcUuid(service->getUuid())));
        }
      }
    }
  }

   return true;
}

void GattLibService::stopNextService(int serverIf, int status)
{
  if (DBG) {
    ALOGD(LOGTAG " stopNextService() - serverIf= %d, status= %d", serverIf, status);
  }

  if (status == 0) {
    std::vector<HandleMap::Entry*> entries = mHandleMap->getEntries();
    for (HandleMap::Entry *entry : entries) {
      if (entry->type != HandleMap::TYPE_SERVICE || entry->serverIf != serverIf) {
        continue;
      }
      mNative->gattServerStopServiceNative(serverIf, entry->handle);
      return;
    }
  }
}

void GattLibService::deleteServices(int serverIf)
{
  if (DBG) {
    ALOGD(LOGTAG "deleteServices() - serverIf %d", serverIf);
  }

  /*
   * Figure out which handles to delete.
   * The handles are copied into a new list to avoid race conditions.
   */
  std::vector<int> handleList;
  std::vector<HandleMap::Entry*> entries = mHandleMap->getEntries();
  ALOGD(LOGTAG "deleteServices() size %d", entries.size());
  for (HandleMap::Entry *entry : entries) {
    if (entry->type != HandleMap::TYPE_SERVICE || entry->serverIf != serverIf) {
      continue;
    }
    ALOGE(LOGTAG "deleteServices() %d",entry->serverIf);
    handleList.push_back(entry->handle);
  }

  /* Now actually delete the services.... */
  for (int handle : handleList) {
    ALOGE(LOGTAG "native call deleteServices() ");
      mNative->gattServerDeleteServiceNative(serverIf, handle);
  }
}

std::vector<Uuid> GattLibService::parseUuids(std::vector<uint8_t> advData)
{
  std::vector<Uuid> uuids;
  if (advData.empty())
    return uuids;

  int offset = 0;
  while (offset < advData.size() - 2) {
    int len = static_cast<unsigned int>(advData[offset++]);
    if (len == 0) {
      break;
    }

    int type = advData[offset++];
    switch (type) {
      case 0x02: // Partial list of 16-bit UUIDs
      case 0x03: // Complete list of 16-bit UUIDs
        while (len > 1) {
          int uuid16 = advData[offset++];
          uuid16 += (advData[offset++] << 8);
          len -= 2;
          std::string str = "%08x-0000-1000-8000-00805f9b34fb" + std::to_string(uuid16);
          uuids.push_back(Uuid::FromString(str));
        }
        break;
      default:
        offset += (len - 1);
        break;
    }
  }

  return uuids;
}

/**************************************************************************
* GATT Service functions - CLIENT
*************************************************************************/

void GattLibService::registerClient(Uuid uuid, IClientCallback& callback)
{
  if (DBG) {
    ALOGD(LOGTAG " registerClient() - UUID %s", uuid.ToString().c_str());
  }
  mClientMap->add(uuid, &callback, this);
  mNative->gattClientRegisterAppNative(uuid);
}

void GattLibService::unregisterClient(int clientIf)
{
  if (DBG) {
    ALOGD(LOGTAG " unregisterClient() - clientIf=%d", clientIf);
  }
  mClientMap->remove(clientIf);
  mNative->gattClientUnregisterAppNative(clientIf);
}

void GattLibService::clientConnect(int clientIf, string address, bool isDirect, int transport,
       bool opportunistic, int phy)
{
  if (DBG) {
    ALOGD(LOGTAG " clientConnect() - address %s isDirect %d opportunistic %d phy %d",
                              address.c_str(), isDirect, opportunistic, phy);
  }
  mNative->gattClientConnectNative(clientIf, address, isDirect, transport, opportunistic, phy);
}

void GattLibService::clientDisconnect(int clientIf, string address)
{
  int connId = mClientMap->connIdByAddress(clientIf, address);
  if (connId < 0)
    connId = 0;
  if (DBG) {
    ALOGD(LOGTAG " clientDisconnect() - address %s connId %d",
                address.c_str(), connId);
  }
  mNative->gattClientDisconnectNative(clientIf, address, connId);
}

void GattLibService::clientSetPreferredPhy(int clientIf, string address, int txPhy, int rxPhy,
                                                  int phyOptions)
{
  int connId = mClientMap->connIdByAddress(clientIf, address);
  if (connId < 0) {
    if (DBG) {
      ALOGD(LOGTAG "clientSetPreferredPhy() - no connection to %s", address.c_str());
    }
    return;
  }

  if (DBG) {
    ALOGD(LOGTAG" clientSetPreferredPhy() - address %s connId %d", address.c_str(), connId);
  }
  mNative->gattClientSetPreferredPhyNative(clientIf, address, txPhy, rxPhy, phyOptions);
}

void GattLibService::clientReadPhy(int clientIf, string address)
{
  int connId = mClientMap->connIdByAddress(clientIf, address);
  if (connId < 0) {
    if (DBG) {
      ALOGD(LOGTAG " clientReadPhy() - no connection to %s ", address.c_str());
    }
    return;
  }

  if (DBG) {
    ALOGD(LOGTAG " clientReadPhy() - address %s connId %d", address.c_str(), connId);
  }
  mNative->gattClientReadPhyNative(clientIf, address);
}

void GattLibService::refreshDevice(int clientIf, string address)
{
  if (DBG) {
    ALOGD(LOGTAG " refreshDevice() - address %s", address.c_str());
  }
  mNative->gattClientRefreshNative(clientIf, address);
}

void GattLibService::discoverServices(int clientIf, string address)
{
  int connId = mClientMap->connIdByAddress(clientIf, address);
  if (DBG) {
    ALOGD(LOGTAG " discoverServices() - address %s connId %d", address.c_str(), connId);
  }

  if (connId >= 0) {
    mNative->gattClientSearchServiceNative(connId, true, Uuid::kEmpty);
  } else {
    ALOGE(LOGTAG " discoverServices() - No connection for %s ...", address.c_str() );
  }
}

void GattLibService::discoverServiceByUuid(int clientIf, string address, Uuid uuid)
{
  int connId = mClientMap->connIdByAddress(clientIf, address);
  if (DBG) {
    ALOGD(LOGTAG " discoverServiceByUuid() - address %s connId %d", address.c_str(), connId);
  }

  if (connId > 0) {
    mNative->gattClientDiscoverServiceByUuidNative(connId, uuid);
  } else {
    ALOGE(LOGTAG " discoverServiceByUuid() - No connection for %s ...", address.c_str());
  }
}

void GattLibService::readCharacteristic(int clientIf, string address, int handle, int authReq)
{
  if (VDBG) {
    ALOGD(LOGTAG " readCharacteristic() - address %s ", address.c_str());
  }

  int connId = mClientMap->connIdByAddress(clientIf, address);
  if (connId < 0) {
    ALOGE(LOGTAG " readCharacteristic() -  No connection for %s ...", address.c_str());
    return;
  }

 if (!permissionCheck(connId, handle)) {
    ALOGW(LOGTAG "readCharacteristic() - permission check failed!");
    return;
  }
  mNative->gattClientReadCharacteristicNative(connId, handle, authReq);
}

void GattLibService::readUsingCharacteristicUuid(int clientIf, string address, Uuid uuid,
       int startHandle, int endHandle, int authReq)
{

  if (VDBG) {
    ALOGD(LOGTAG " readUsingCharacteristicUuid() - address %s", address.c_str());
  }

  int connId = mClientMap->connIdByAddress(clientIf, address);
  if (connId < 0 /*== NULL*/) {
    ALOGE(LOGTAG "readUsingCharacteristicUuid() - No connection for %s ...", address.c_str());
    return;
  }

  if (!permissionCheck(uuid)) {
    ALOGW(LOGTAG "readUsingCharacteristicUuid() - permission check failed!");
    return;
  }

  mNative->gattClientReadUsingCharacteristicUuidNative(connId, uuid, startHandle,
                                        endHandle, authReq);
}

void GattLibService::writeCharacteristic(int clientIf, string address, int handle,
                              int writeType, int authReq, uint8_t *value, int length)
{
  if (VDBG) {
    ALOGD(LOGTAG " writeCharacteristic() - address %s", address.c_str());
  }

  if (mReliableQueue.find(address) != mReliableQueue.end()) {
    writeType = 3; // Prepared write
  }

  int connId = mClientMap->connIdByAddress(clientIf, address);
  if (connId < 0 /*== NULL*/) {
    ALOGE(LOGTAG "writeCharacteristic() - No connection for %s ...", address.c_str());
    return;
  }

  if (!permissionCheck(connId, handle)) {
    ALOGW(LOGTAG "writeCharacteristic() - permission check failed!");
    return;
  }

  std::vector<uint8_t> val_vec;
  if (value != NULL) {
    val_vec.assign(&value[0], &value[length]);
  }

  mNative->gattClientWriteCharacteristicNative(connId, handle, writeType, authReq, val_vec);
}

void GattLibService::readDescriptor(int clientIf, string address, int handle, int authReq)
{
  if (VDBG) {
    ALOGD(LOGTAG " readDescriptor() - address %s", address.c_str());
  }

  int connId = mClientMap->connIdByAddress(clientIf, address);
  if (connId < 0) {
    ALOGE(LOGTAG "readDescriptor() - No connection for %s ...", address.c_str());
    return;
  }

  if (!permissionCheck(connId, handle)) {
    ALOGW(LOGTAG " readDescriptor() - permission check failed!");
    return;
  }

  mNative->gattClientReadDescriptorNative(connId, handle, authReq);
}

void GattLibService::writeDescriptor(int clientIf, string address, int handle, int authReq,
       uint8_t *value, int length)
{

  if (VDBG) {
    ALOGD(LOGTAG "writeDescriptor() - address %s", address.c_str());
  }

  int connId = mClientMap->connIdByAddress(clientIf, address);
  if (connId < 0) {
    ALOGE(LOGTAG "writeDescriptor() - No connection for %s ...", address.c_str());
    return;
  }

  if (!permissionCheck(connId, handle)) {
    ALOGW(LOGTAG " writeDescriptor() - permission check failed!");
    return;
  }

  std::vector<uint8_t> val_vec;
  if (value != NULL) {
    val_vec.assign(&value[0], &value[length]);
  }

  mNative->gattClientWriteDescriptorNative(connId, handle, authReq, val_vec);
}

void GattLibService::beginReliableWrite(int clientIf, string address)
{
  if (DBG) {
    ALOGD(LOGTAG  "beginReliableWrite() - address=%s", address.c_str());
  }
  mReliableQueue.insert(address);
}

void GattLibService::endReliableWrite(int clientIf, string address, bool execute)
{
  if (DBG) {
    ALOGD(LOGTAG " endReliableWrite() - address %s execute %d", address.c_str(), execute);
  }
  mReliableQueue.erase(address);

  int connId = mClientMap->connIdByAddress(clientIf, address);
  if (connId >= 0) {
    mNative->gattClientExecuteWriteNative(connId, execute);
  }
}

void GattLibService::registerForNotification(int clientIf, string address, int handle,
                                             bool enable)
{
  if (DBG) {
    ALOGD(LOGTAG " registerForNotification() - address %s enable %d", address.c_str(), enable);
  }

  int connId = mClientMap->connIdByAddress(clientIf, address);
  if (connId < 0) {
    ALOGE(LOGTAG " registerForNotification() - No connection for %s ...", address.c_str());
    return;
  }

  if (!permissionCheck(connId, handle)) {
    ALOGW(LOGTAG " registerForNotification() - permission check failed!");
    return;
  }

  mNative->gattClientRegisterForNotificationsNative(clientIf, address, handle, enable);
}

void GattLibService::readRemoteRssi(int clientIf, string address)
{
  if (DBG) {
    ALOGD(LOGTAG " readRemoteRssi() - address %s", address.c_str());
  }
  mNative->gattClientReadRemoteRssiNative(clientIf, address);
}

void GattLibService::configureMTU(int clientIf, string address, int mtu)
{
  if (DBG) {
    ALOGD(LOGTAG " configureMTU() - address %s mtu %d", address.c_str(), mtu);
  }
  int connId = mClientMap->connIdByAddress(clientIf, address);

  if (connId >= 0) {
    mNative->gattClientConfigureMTUNative(connId, mtu);
  } else {
    ALOGE(LOGTAG " configureMTU() -  No connection for %s ...", address.c_str());
  }
}

void GattLibService::connectionParameterUpdate(int clientIf, string address,
                                                              int connectionPriority)
{
  int minInterval;
  int maxInterval;

  // Slave latency
  int latency;

  // Link supervision timeout is measured in N * 10ms
  int timeout = 2000; // 20s

  switch (connectionPriority) {
    case CONNECTION_PRIORITY_HIGH:
      minInterval = GATT_HIGH_PRIORITY_MIN_INTERVAL;
      maxInterval = GATT_HIGH_PRIORITY_MAX_INTERVAL;
      latency = GATT_HIGH_PRIORITY_LATENCY;
      break;
    case CONNECTION_PRIORITY_LOW_POWER:
      minInterval = GATT_LOW_POWER_MIN_INTERVAL;
      maxInterval = GATT_LOW_POWER_MAX_INTERVAL;
      latency = GATT_LOW_POWER_LATENCY;
      break;
    default:
      // Using the values for CONNECTION_PRIORITY_BALANCED.
      minInterval =GATT_BALANCED_PRIORITY_MIN_INTERVAL;
      maxInterval =GATT_BALANCED_PRIORITY_MAX_INTERVAL;
      latency = GATT_BALANCED_PRIORITY_LATENCY;
      break;
  }

  if (DBG) {
    ALOGD(LOGTAG
      " connectionParameterUpdate() - address %s params %d minInterval %d maxInterval %d",
      address.c_str(), connectionPriority, minInterval, maxInterval);
  }
  mNative->gattConnectionParameterUpdateNative(clientIf, address, minInterval, maxInterval, latency,
          timeout, 0, 0);
}

void GattLibService::leConnectionUpdate(int clientIf, string address,
                                       int minConnectionInterval, int maxConnectionInterval,
                                       int slaveLatency, int supervisionTimeout,
                                       int minConnectionEventLen, int maxConnectionEventLen)
{
  if (DBG) {
    ALOGD(LOGTAG "leConnectionUpdate() - address %s intervals %d/%d latency %d timeout %d\
                  msec min_ce %d max_ce %d", address.c_str(), minConnectionInterval,
                  maxConnectionInterval, slaveLatency, supervisionTimeout,
                  minConnectionEventLen, maxConnectionEventLen);
  }
  mNative->gattConnectionParameterUpdateNative(clientIf, address, minConnectionInterval,
                                      maxConnectionInterval, slaveLatency, supervisionTimeout,
                                      minConnectionEventLen, maxConnectionEventLen);
}

/**************************************************************************
* GATT Service functions -SERVER
*************************************************************************/

void GattLibService::registerServer(Uuid uuid, IServerCallback& callback)
{
  if (DBG) {
    ALOGD(LOGTAG " registerServer()  - UUID %s", uuid.ToString().c_str());
  }
  mServerMap->add(uuid, &callback, this);
  ALOGE(LOGTAG "callback %p",(void *)&callback);
  mNative->gattServerRegisterAppNative(uuid);
}

void GattLibService::unregisterServer(int serverIf)
{
  if (DBG) {
    ALOGD(LOGTAG " unregisterServer()  - serverIf %d", serverIf);
  }

  deleteServices(serverIf);
  mServerMap->remove(serverIf);
  mNative->gattServerUnregisterAppNative(serverIf);
}

void GattLibService::serverConnect(int serverIf, string address, bool isDirect, int transport)
{
  if (DBG) {
    ALOGD(LOGTAG " serverConnect() - address %s", address.c_str());
  }
  mNative->gattServerConnectNative(serverIf, address, isDirect, transport);
}

void GattLibService::serverDisconnect(int serverIf, string address)
{
  int connId = mServerMap->connIdByAddress(serverIf, address);
  if (connId < 0)
    connId = 0;
  if (DBG) {
    ALOGD(LOGTAG " serverDisconnect() - address %s , connId %d", address.c_str(), connId);
  }

  mNative->gattServerDisconnectNative(serverIf, address, connId);
}

void GattLibService::serverSetPreferredPhy(int serverIf, string address, int txPhy,
                                                  int rxPhy, int phyOptions)
{
  int connId = mServerMap->connIdByAddress(serverIf, address);
  if (connId < 0) {
    if (DBG) {
      ALOGD(LOGTAG " serverSetPreferredPhy() - no connection to %s ", address.c_str());
    }
    return;
  }

  if (DBG) {
    ALOGD(LOGTAG " serverSetPreferredPhy() - address %s connId %d", address.c_str(), connId);
  }
  mNative->gattServerSetPreferredPhyNative(serverIf, address, txPhy, rxPhy, phyOptions);
}

void GattLibService::serverReadPhy(int clientIf, string address)
{
  int connId = mServerMap->connIdByAddress(clientIf, address);
  if (connId < 0) {
    if (DBG) {
      ALOGD(LOGTAG " serverReadPhy() - no connection to %s", address.c_str());
    }
    return;
  }

  if (DBG) {
    ALOGD(LOGTAG " serverReadPhy() - address %s connId %d", address.c_str(), connId);
  }
  mNative->gattServerReadPhyNative(clientIf, address);
}

std::vector<gatt_db_element_t> GattLibService::fillGattDbStructureArray(
                                                                std::vector<GattDbElement*> db,
                                                                int count)
{
  std::vector<gatt_db_element_t> db_vec;
  for (int i = 0;i < count; i++) {
    gatt_db_element_t *db_struct = new gatt_db_element_t;
    db_struct->id = db[i]->id;
    db_struct->attribute_handle = db[i]->attributeHandle;
    db_struct->type = static_cast<gatt_db_attribute_type_t>(db[i]->type);
    db_struct->start_handle = db[i]->startHandle;
    db_struct->end_handle = db[i]->endHandle;
    db_struct->properties = db[i]->properties;
    db_struct->permissions = db[i]->permissions;
    db_struct->uuid = db[i]->uuid;

    db_vec.push_back(*db_struct);
  }
  for(GattDbElement *temp_db : db)
    delete temp_db;
  db.clear();
  return db_vec;
}

void GattLibService::addService(int serverIf, GattService *svc)
{
  if (DBG) {
    ALOGD(LOGTAG " addService() - uuid %s", svc->getUuid().ToString().c_str());
  }

  std::vector<GattDbElement*> db;

  if (svc->getType() == GattService::SERVICE_TYPE_PRIMARY) {
    db.push_back(GattDbElement::createPrimaryService(svc->getUuid()));
  } else {
    db.push_back(GattDbElement::createSecondaryService(svc->getUuid()));
  }

  for (GattService *includedService : svc->getIncludedServices()) {
    int inclSrvcHandle = includedService->getInstanceId();

    if (mHandleMap->checkServiceExists(includedService->getUuid(), inclSrvcHandle)) {
      db.push_back(GattDbElement::createIncludedService(inclSrvcHandle));
    } else {
      ALOGE(LOGTAG " included service with UUID %s not found!",
                              includedService->getUuid().ToString().c_str() );
    }
  }

  for (GattCharacteristic *characteristic : svc->getCharacteristics()) {
    int permission =
            ((characteristic->getKeySize() - 7) << 12) + characteristic->getPermissions();
    db.push_back(GattDbElement::createCharacteristic(characteristic->getUuid(),
                            characteristic->getProperties(), permission));

    for (GattDescriptor *descriptor : characteristic->getDescriptors()) {
      permission =
              ((characteristic->getKeySize() - 7) << 12) + descriptor->getPermissions();
      db.push_back(GattDbElement::createDescriptor(descriptor->getUuid(), permission));
    }
  }

  std::vector<gatt_db_element_t> service = fillGattDbStructureArray(db,db.size());
  mNative->gattServerAddServiceNative(serverIf, service);

}

void GattLibService::removeService(int serverIf, int handle)
{
  if (DBG) {
    ALOGD(LOGTAG " removeService() - handle %d", handle);
  }

  mNative->gattServerDeleteServiceNative(serverIf, handle);
}

void GattLibService::clearServices(int serverIf)
{
  if (DBG) {
    ALOGD(LOGTAG " clearServices()");
  }
  deleteServices(serverIf);
}

void GattLibService::sendResponse(int serverIf, string address, int requestId, int status,
        int offset, uint8_t *value, int length)
{
  if (VDBG) {
    ALOGD(LOGTAG " sendResponse() - address %s", address.c_str());
  }

  int handle = 0;
  HandleMap::Entry *entry = mHandleMap->getByRequestId(requestId);
  if (entry != NULL) {
    handle = entry->handle;
  }
  int connId = mServerMap->connIdByAddress(serverIf, address);
  if (connId < 0)
    connId = 0;
  std::vector<uint8_t> val_vec;
  if (value != NULL) {
    val_vec.assign(&value[0], &value[length]);
  }
  mNative->gattServerSendResponseNative(serverIf, connId , requestId,
          (uint8_t) status, handle, offset, val_vec, (uint8_t) 0);
  mHandleMap->deleteRequest(requestId);
}

void GattLibService::sendNotification(int serverIf, string address, int handle, bool confirm,
        uint8_t *value, int length)
{
  if (VDBG) {
    ALOGD(LOGTAG " sendNotification() - address %s  handle %d", address.c_str(), handle);
  }

  int connId = mServerMap->connIdByAddress(serverIf, address);
  if (connId < 0 || connId == 0) {
    return;
  }

  std::vector<uint8_t> val_vec;
  if (value != NULL) {
    val_vec.assign(&value[0], &value[length]);
  }

  if (confirm) {
    mNative->gattServerSendIndicationNative(serverIf, handle, connId, val_vec);
  } else {
      mNative->gattServerSendNotificationNative(serverIf, handle, connId, val_vec);
  }
}

/**************************************************************************
* ADVERTISING SET
*************************************************************************/
void GattLibService::startAdvertisingSet(AdvertisingSetParameters *parameters,
        AdvertiseData *advertiseData, AdvertiseData *scanResponse,
        PeriodicAdvertiseParameters *periodicParameters, AdvertiseData *periodicData,
        int duration, int maxExtAdvEvents, IAdvertisingSetCallback *callback)
{
  if (DBG) {
    ALOGD(LOGTAG " startAdvertisingSet()");
  }

  if (!mAdvertiserManager) return;
  mAdvertiserManager->startAdvertisingSet(*parameters, *advertiseData, *scanResponse,
                  *periodicParameters, *periodicData, duration, maxExtAdvEvents, *callback);
}

void GattLibService::stopAdvertisingSet(IAdvertisingSetCallback *callback)
{
  if (DBG) {
    ALOGD(LOGTAG " stopAdvertisingSet()");
  }

  if (!mAdvertiserManager) return;
  mAdvertiserManager->stopAdvertisingSet(*callback);
}

void GattLibService::getOwnAddress(int advertiserId)
{
  if (!mAdvertiserManager) return;
  mAdvertiserManager->getOwnAddress(advertiserId);
}

void GattLibService::enableAdvertisingSet(int advertiserId, bool enable, int duration,
                                                  int maxExtAdvEvents)
{
  if (!mAdvertiserManager) return;
  mAdvertiserManager->enableAdvertisingSet(advertiserId, enable, duration, maxExtAdvEvents);
}

void GattLibService::setAdvertisingData(int advertiserId, AdvertiseData *data)
{
  if (!mAdvertiserManager) return;
  mAdvertiserManager->setAdvertisingData(advertiserId, *data);
}

void GattLibService::setScanResponseData(int advertiserId, AdvertiseData *data)
{
  if (!mAdvertiserManager) return;
  mAdvertiserManager->setScanResponseData(advertiserId, *data);
}

void GattLibService::setAdvertisingParameters(int advertiserId,
                                                   AdvertisingSetParameters *parameters)
{
  if (!mAdvertiserManager) return;
  mAdvertiserManager->setAdvertisingParameters(advertiserId, *parameters);
}

void GattLibService::setPeriodicAdvertisingParameters(int advertiserId,
                                                     PeriodicAdvertiseParameters *parameters)
{
  if (!mAdvertiserManager) return;
  mAdvertiserManager->setPeriodicAdvertisingParameters(advertiserId, *parameters);
}

void GattLibService::setPeriodicAdvertisingData(int advertiserId, AdvertiseData *data)
{
  if (!mAdvertiserManager) return;
  mAdvertiserManager->setPeriodicAdvertisingData(advertiserId, *data);
}

void GattLibService::setPeriodicAdvertisingEnable(int advertiserId, bool enable)
{
  if (!mAdvertiserManager) return;
  mAdvertiserManager->setPeriodicAdvertisingEnable(advertiserId, enable);
}

/**************************************************************************
* PERIODIC SCANNING
*************************************************************************/


void GattLibService::registerSync(ScanResult *scanResult, int skip, int timeout,
        IPeriodicAdvertisingCallback *callback)
{
  if (!mPeriodicScanManager) return;
  mPeriodicScanManager->startSync(scanResult, skip, timeout, callback);
}

void GattLibService::unregisterSync(IPeriodicAdvertisingCallback *callback)
{
  if (!mPeriodicScanManager) return;
  mPeriodicScanManager->stopSync(callback);
}


/**********************************************************************
* GATT Service functions - Shared CLIENT/SERVER
**********************************************************************/
void GattLibService::registerScanner(IScannerCallback *callback)
{
  Uuid uuid = Uuid::GetRandom();
  if (DBG) {
    ALOGD(LOGTAG "registerScanner() - UUID %s", uuid.ToString().c_str());
  }

  mScannerMap->add(uuid, callback, this);
  if (!mScanManager) return;
  mScanManager->registerScanner(uuid);
}

void GattLibService::unregisterScanner(int scannerId)
{
  if (DBG) {
    ALOGD(LOGTAG "unregisterScanner() - scannerId %d", scannerId);
  }
  mScannerMap->remove(scannerId);

  if (!mScanManager) return;
  mScanManager->unregisterScanner(scannerId);
}

void GattLibService::startScan(int scannerId, ScanSettings *settings,
                    std::vector<ScanFilter*> filters,
                    std::vector<std::vector<ResultStorageDescriptor*>> storages)
{
  if (DBG) {
    ALOGD( LOGTAG "startScan() - start scan with filters");
  }

  ScanClient *scanClient = new ScanClient(scannerId, settings, filters, storages);

  if (!mScanManager) return;
  mScanManager->startScan(scanClient);
}

void GattLibService::flushPendingBatchResults(int scannerId)
{
  if (DBG) {
    ALOGD(LOGTAG "flushPendingBatchResults() - scannerId %d", scannerId);
  }

  if (!mScanManager) return;
  mScanManager->flushBatchScanResults(new ScanClient(scannerId));
}

void GattLibService::stopScan(int scannerId)
{
  stopScan(new ScanClient(scannerId));
}

void GattLibService::stopScan(ScanClient *client)
{
  if (!mScanManager) return;
  int scanQueueSize =
          mScanManager->getBatchScanQueue().size() + mScanManager->getRegularScanQueue().size();
  if (DBG) {
    ALOGD( LOGTAG " stopScan() - queue size %d", scanQueueSize);
  }

  mScanManager->stopScan(client);

}

void GattLibService::disconnectAll()
{
  if (DBG) {
    ALOGD(LOGTAG " disconnectAll()");
  }
  std::unordered_map<int, string> connMap = mClientMap->getConnectedMap();
  for (std::unordered_map<int, string>::iterator entry = connMap.begin();
              entry != connMap.end(); ++entry) {
   if (DBG) {
     ALOGD(LOGTAG "disconnecting addr: %s",entry->second.c_str());
   }
   clientDisconnect(entry->first, entry->second);
   }
}

bool GattLibService::isScanClient(int clientIf)
{
  if (!mScanManager) return false;

  for (ScanClient *client : mScanManager->getRegularScanQueue()) {
    if (client->scannerId == clientIf) {
      return true;
    }
  }
  for (ScanClient *client : mScanManager->getBatchScanQueue()) {
    if (client->scannerId == clientIf) {
      return true;
    }
  }
  return false;
}

void GattLibService::unregAll()
{
  for (int appId : mClientMap->getAllAppsIds()) {
    if (DBG) {
      ALOGD(LOGTAG " unreg: %d", appId);
    }
    unregisterClient(appId);
  }
  for (int appId : mServerMap->getAllAppsIds()) {
    if (DBG) ALOGD(LOGTAG " unreg:%d", appId);
    unregisterServer(appId);
  }
  for (int appId : mScannerMap->getAllAppsIds()) {
    if (DBG) ALOGD(LOGTAG " unreg:%d", appId);
    if (isScanClient(appId)) {
      ScanClient *client = new ScanClient(appId);
      stopScan(client);
      unregisterScanner(appId);
    }
  }
  if (mAdvertiserManager != NULL) {
    mAdvertiserManager->stopAdvertisingSets();
  }
}

int GattLibService::numHwTrackFiltersAvailable()
{
  if (mScanManager != NULL)
    return (getTotalNumOfTrackableAdvertisements()
                 - mScanManager->getCurrentUsedTrackingAdvertisement());
  else
    return getTotalNumOfTrackableAdvertisements();
}

string GattLibService::getAddress()
{
  return mAddress;
}

string GattLibService::getDeviceName()
{
  return mDeviceName;
}

int GattLibService::getNumOfAdvertisementInstancesSupported()
{
  return mNumOfAdvertisementInstancesSupported;
}

bool GattLibService::isRpaOffloadSupported()
{
  return mRpaOffloadSupported;
}

int GattLibService::getNumOfOffloadedIrkSupported()
{
  return mNumOfOffloadedIrkSupported;
}

int GattLibService::getNumOfOffloadedScanFilterSupported()
{
  return mNumOfOffloadedScanFilterSupported;
}

int GattLibService::getOffloadedScanResultStorage()
{
  return mOffloadedScanResultStorageBytes;
}

bool GattLibService::isActivityAndEnergyReportingSupported()
{
  return mIsActivityAndEnergyReporting;
}

bool GattLibService::isLe2MPhySupported()
{
  return mIsLe2MPhySupported;
}

bool GattLibService::isLeCodedPhySupported()
{
  return mIsLeCodedPhySupported;
}

bool GattLibService::isLeExtendedAdvertisingSupported()
{
  return mIsLeExtendedAdvertisingSupported;
}

bool GattLibService::isLePeriodicAdvertisingSupported()
{
  return mIsLePeriodicAdvertisingSupported;
}

int GattLibService::getLeMaximumAdvertisingDataLength()
{
  return mLeMaximumAdvertisingDataLength;
}

int GattLibService::getTotalNumOfTrackableAdvertisements()
{
  return mTotNumOfTrackableAdv;
}

bool GattLibService::isOffloadedFilteringSupported()
{
  int val = getNumOfOffloadedScanFilterSupported();
  return (val >= MIN_OFFLOADED_FILTERS);
}

bool GattLibService::isMultipleAdvertisementSupported()
{
  return getNumOfAdvertisementInstancesSupported() >= MIN_ADVT_INSTANCES_FOR_MA;
}

bool GattLibService::isOffloadedScanBatchingSupported()
{
  int val = getOffloadedScanResultStorage();
  return (val >= MIN_OFFLOADED_SCAN_STORAGE_BYTES);
}

void GattLibService::setAddress(void *val, int len)
{
  std::string sp(static_cast<const char*>(val), len);
  mAddress = sp;
}

void GattLibService::setDeviceName(void *val, int len)
{
  std::string sp(static_cast<const char*>(val), len);
  mDeviceName = sp;
}

void GattLibService::updateFeatureSupport(void *value, int len)
{
  uint8_t *val = new uint8_t[len];
  if (!val)
    return;

  std::memcpy(val, static_cast<uint8_t*>(value), len);
  mVersSupported = ((0xFF & ((int) val[1])) << 8) + (0xFF & ((int) val[0]));
  mNumOfAdvertisementInstancesSupported = (0xFF & ((int) val[3]));
  mRpaOffloadSupported = ((0xFF & ((int) val[4])) != 0);
  mNumOfOffloadedIrkSupported = (0xFF & ((int) val[5]));
  mNumOfOffloadedScanFilterSupported = (0xFF & ((int) val[6]));
  mIsActivityAndEnergyReporting = ((0xFF & ((int) val[7])) != 0);
  mOffloadedScanResultStorageBytes = ((0xFF & ((int) val[9])) << 8) + (0xFF & ((int) val[8]));
  mTotNumOfTrackableAdv = ((0xFF & ((int) val[11])) << 8) + (0xFF & ((int) val[10]));
  mIsExtendedScanSupported = ((0xFF & ((int) val[12])) != 0);
  mIsDebugLogSupported = ((0xFF & ((int) val[13])) != 0);
  mIsLe2MPhySupported = ((0xFF & ((int) val[14])) != 0);
  mIsLeCodedPhySupported = ((0xFF & ((int) val[15])) != 0);
  mIsLeExtendedAdvertisingSupported = ((0xFF & ((int) val[16])) != 0);
  mIsLePeriodicAdvertisingSupported = ((0xFF & ((int) val[17])) != 0);
  mLeMaximumAdvertisingDataLength =
          (0xFF & ((int) val[18])) + ((0xFF & ((int) val[19])) << 8);

  ALOGD(LOGTAG "BT_PROPERTY_LOCAL_LE_FEATURES: update from BT controller \
                 mNumOfAdvertisementInstancesSupported = %d \
                 mRpaOffloadSupported = %d \
                 mNumOfOffloadedIrkSupported = %d \
                 mNumOfOffloadedScanFilterSupported = %d \
                 mOffloadedScanResultStorageBytes = %d \
                 mIsActivityAndEnergyReporting = %d \
                 mVersSupported = %d \
                 mTotNumOfTrackableAdv = %d \
                 mIsExtendedScanSupported = %d \
                 mIsDebugLogSupported = %d \
                 mIsLe2MPhySupported = %d \
                 mIsLeCodedPhySupported = %d \
                 mIsLeExtendedAdvertisingSupported = %d \
                 mIsLePeriodicAdvertisingSupported = %d \
                 mLeMaximumAdvertisingDataLength = %d",
                 mNumOfAdvertisementInstancesSupported,
                 mRpaOffloadSupported,
                 mNumOfOffloadedIrkSupported,
                 mNumOfOffloadedScanFilterSupported,
                 mOffloadedScanResultStorageBytes,
                 mIsActivityAndEnergyReporting,
                 mVersSupported,
                 mTotNumOfTrackableAdv,
                 mIsExtendedScanSupported,
                 mIsDebugLogSupported,
                 mIsLe2MPhySupported,
                 mIsLeCodedPhySupported,
                 mIsLeExtendedAdvertisingSupported,
                 mIsLePeriodicAdvertisingSupported,
                 mLeMaximumAdvertisingDataLength);
  delete[] val;
}



/**********************************************************************
 * HANDLE CALLBACK FROM JNI
 ***********************************************************************/

void GattLibService::HandleGattcRegisterAppEvent(GattcRegisterAppEvent *event)
{
  if (!sGattService) return;
  sGattService->onClientRegistered(event->status, event->clientIf, event->app_uuid);
}

void GattLibService::HandleGattcOpenEvent(GattcOpenEvent *event)
{
  if (!sGattService) return;
  sGattService->onConnected(event->clientIf, event->conn_id, event->status, *(event->bda));
}

void GattLibService::HandleGattcCloseEvent(GattcCloseEvent *event)
{
  if (!sGattService) return;
  sGattService->onDisconnected(event->clientIf, event->conn_id, event->status, *(event->bda));
}

void GattLibService::HandleGattcSearchCompleteEvent(GattcSearchCompleteEvent *event)
{
  if (!sGattService) return;
  sGattService->onSearchCompleted(event->conn_id, event->status);
}

void GattLibService::HandleGattcRegisterForNotificationEvent(
                                      GattcRegisterForNotificationEvent *event)
{
  if (!sGattService) return;
  sGattService->onRegisterForNotifications(event->conn_id, event->status, event->registered,
                            event->handle);
}

void GattLibService::HandleGattcNotifyEvent(GattcNotifyEvent *event)
{
  if (!sGattService) return;
  uint8_t *value = new uint8_t[event->p_data.len];
  if (!value)
    return;
  std::memcpy(value, &event->p_data.value, event->p_data.len);
  sGattService->onNotify(event->conn_id, *(event->p_data.bda), event->p_data.handle,
                                event->p_data.is_notify, value, event->p_data.len);
}

void GattLibService::HandleGattcReadCharacteristicEvent(
                                                  GattcReadCharacteristicEvent *event)
{
  if (!sGattService) return;
  uint8_t *value = NULL;
  if (event->status == 0) {
    value = new uint8_t[event->p_data.value.len];
    if (!value)
      return;
    std::memcpy(value, &event->p_data.value.value, event->p_data.value.len);
  }
  sGattService->onReadCharacteristic(event->conn_id, event->status, event->p_data.handle,
                                                value, event->p_data.value.len);
}

void GattLibService::HandleGattcWriteCharacterisitcEvent(
                                                GattcWriteCharacteristicEvent *event)
{
  if (!sGattService) return;
  sGattService->onWriteCharacteristic(event->conn_id, event->status, event->handle);
}

void GattLibService::HandleGattcExecuteWriteEvent(GattcExecuteWriteEvent *event)
{
  if (!sGattService) return;
  sGattService->onExecuteCompleted(event->conn_id, event->status);
}

void GattLibService::HandleGattcReadDescriptorEvent(GattcReadDescriptorEvent *event)
{
  if (!sGattService) return;
  uint8_t *value = NULL;
  if (event->p_data.value.len != 0) {
    value = new uint8_t[event->p_data.value.len];
    if (!value)
      return;
    std::memcpy(value, &event->p_data.value.value, event->p_data.value.len);
  }

  sGattService->onReadDescriptor(event->conn_id, event->status, event->p_data.handle,
                                  value, event->p_data.value.len);
}

void GattLibService::HandleGattcWriteDescriptorEvent(GattcWriteDescriptorEvent *event)
{
  if (!sGattService) return;
  sGattService->onWriteDescriptor(event->conn_id, event->status, event->handle);
}

void GattLibService::HandleGattcRemoteRssiEvent(GattcRemoteRssiEvent *event)
{
  if (!sGattService) return;
   sGattService->onReadRemoteRssi(event->client_if, *(event->bda), event->rssi,
                              event->status);
}

void GattLibService::HandleGattcConfigureMtuEvent(GattcConfigureMtuEvent *event)
{
  if (!sGattService) return;
  sGattService->onConfigureMTU(event->conn_id, event->status, event->mtu);
}

void GattLibService::HandleGattcCongestionEvent(GattcCongestionEvent *event)
{
  if (!sGattService) return;
  sGattService->onClientCongestion(event->conn_id, event->congested);
}

void GattLibService::HandleGattcGetGattDbEvent(GattcGetGattDbEvent *event)
{
  if (!sGattService) return;
  std::vector<GattDbElement*> service =
    fillGattDbElementArray(event->db, event->count);

  sGattService->onGetGattDb(event->conn_id, service);
}

void GattLibService::HandleGattcPhyUpdatedEvent(GattcPhyUpdatedEvent *event)
{
  if (!sGattService) return;
  sGattService->onClientPhyUpdate(event->conn_id, event->tx_phy, event->rx_phy, event->status);
}

void GattLibService::HandleGattcConnUpdatedEvent(GattcConnUpdatedEvent *event)
{
  if (!sGattService) return;
  sGattService->onClientConnUpdate(event->conn_id, event->interval, event->latency,
          event->timeout, event->status);
}

void GattLibService::HandleGattcReadPhyEvent(GattcReadPhyEvent *event)
{
  if (!sGattService) return;
  sGattService->onClientPhyRead(event->clientIf, *(event->bda), event->tx_phy,
                                        event->rx_phy, event->status);
}

void GattLibService::HandleGattsRegisterAppEvent(GattsRegisterAppEvent *event)
{
  if (!sGattService) return;
  sGattService->onServerRegistered(event->status, event->server_if, event->uuid);
}

void GattLibService::HandleGattsConnectionEvent(GattsConnectionEvent *event)
{
  if (!sGattService) return;
  sGattService->onClientConnected(*(event->bda), event->connected, event->conn_id,
                                                event->server_if);
}

GattDbElement* GattLibService::getSampleGattDbElement()
{
  return new GattDbElement();
}

std::vector<GattDbElement*> GattLibService::fillGattDbElementArray
                                              (gatt_db_element_t *db, int count)
{

  std::vector<GattDbElement*> gdb_vec;
  for (int i = 0; i < count; i++) {
    GattDbElement *gdb = getSampleGattDbElement();

    gdb->id = db->id;
    gdb->attributeHandle = db->attribute_handle;
    gdb->startHandle = db->start_handle;
    gdb->endHandle = db->end_handle;
    gdb->properties = db->properties;
    gdb->type = db->type;
    gdb->permissions = db->permissions;
    gdb->uuid = db->uuid;
    db++;

    gdb_vec.push_back(gdb);
  }
  return gdb_vec;
}

void GattLibService::HandleGattsServiceAddedEvent(GattsServiceAddedEvent *event)
{
  if (!sGattService) return;
  std::vector<GattDbElement*> service =
                      fillGattDbElementArray(event->service->data(),
                                             static_cast<int>(event->service->size()));
  sGattService->onServiceAdded(event->status, event->server_if, service);
}

void GattLibService::HandleGattsServiceStoppedEvent(GattsServiceStoppedEvent *event)
{
  if (!sGattService) return;
  sGattService->onServiceStopped(event->status, event->server_if, event->srvc_handle);
}

void GattLibService::HandleGattsServiceDeletedEvent(GattsServiceDeletedEvent *event)
{
  if (!sGattService) return;
  sGattService->onServiceDeleted(event->status, event->server_if, event->srvc_handle);
}

void GattLibService::HandleGattsRequestReadCharacteristicEvent(
                                                      GattsRequestReadCharacteristicEvent *event)
{
  if (!sGattService) return;
  sGattService->onServerReadCharacteristic(*(event->bda), event->conn_id, event->trans_id,
                                event->attr_handle, event->offset, event->is_long);
}

void GattLibService::HandleGattsRequestReadDescriptorEvent(GattsRequestReadDescriptorEvent *event)
{
  if (!sGattService) return;
  sGattService->onServerReadDescriptor(*(event->bda), event->conn_id, event->trans_id,
                                  event->attr_handle,event->offset, event->is_long);
}

void GattLibService::HandleGattsRequestWriteCharacteristicEvent(
                                                       GattsRequestWriteCharacteristicEvent *event)
{
  if (!sGattService) return;
  uint8_t len = event->value->size();
  uint8_t *p_value = new uint8_t[len];
  if (!p_value)
    return;
  if (len == 0) {
    ALOGE(LOGTAG "HandleGattsRequestWriteCharacteristicEvent () - Data is NULL");
  }
  else {
    std::memcpy(p_value, event->value->data(), len);
  }
  sGattService->onServerWriteCharacteristic(*(event->bda), event->conn_id, event->trans_id,
                                        event->attr_handle, event->offset, event->value->size(),
                                        event->need_rsp, event->is_prep, p_value);
}

void GattLibService::HandleGattsRequestWriteDescriptorEvent(
                                                      GattsRequestWriteDescriptorEvent *event)
{
  if (!sGattService) return;
  uint8_t len = event->value->size();
  uint8_t *p_value = new uint8_t[len];
  if (!p_value)
    return;
  if (len == 0) {
    ALOGE(LOGTAG "HandleGattsRequestWriteDescriptorEvent () - Data is NULL");
  }
  else {
    std::memcpy(p_value, event->value->data(), len);
  }
  sGattService->onServerWriteDescriptor(*(event->bda), event->conn_id, event->trans_id,
                                           event->attr_handle, event->offset, event->value->size(),
                                           event->need_rsp, event->is_prep, p_value);
}

void GattLibService::HandleGattsRequestExecWriteEvent(GattsRequestExecWriteEvent *event)
{
  if (!sGattService) return;
  sGattService->onExecuteWrite(*(event->bda), event->conn_id, event->trans_id, event->exec_write);
}

void GattLibService::HandleGattsResponseConfirmationEvent(
                                                              GattsResponseConfirmationEvent *event)
{
  if (!sGattService) return;
  sGattService->onResponseSendCompleted(event->status, event->handle);
}

void GattLibService::HandleGattsIndicationSentEvent(GattsIndicationSentEvent *event)
{
  if (!sGattService) return;
  sGattService->onNotificationSent(event->conn_id , event->status);
}

void GattLibService::HandleGattsCongestionEvent(GattsCongestionEvent *event)
{
  if (!sGattService) return;
  sGattService->onServerCongestion(event->conn_id, event->congested);
}

void GattLibService::HandleGattsMtuChangedEvent(GattsMTUchangedEvent *event)
{
  if (!sGattService) return;
  sGattService->onMtuChanged(event->conn_id, event->mtu);
}

void GattLibService::HandleGattsPhyUpdatedEvent(GattsPhyUpdatedEvent *event)
{
  if (!sGattService) return;
  sGattService->onServerPhyUpdate(event->conn_id, event->tx_phy, event->rx_phy,
                                    event->status);
}

void GattLibService::HandleGattsConnUpdatedEvent(GattsConnUpdatedEvent *event)
{
  if (!sGattService) return;
  sGattService->onServerConnUpdate(event->conn_id, event->interval,
                  event->latency, event->timeout, event->status);
}

void GattLibService::HandleGattsReadPhyEvent(GattsReadPhyEvent *event) {
  if (!sGattService) return;
  sGattService->onServerPhyRead(event->serverIf, *(event->bda),
                                        event->tx_phy, event->rx_phy, event->status);
}

void GattLibService::HandleBleScannerRegisterScannerEvent(
                                                       BleScannerRegisterScannerEvent *event)
{
  if (!sGattService) return;
  sGattService->onScannerRegistered(event->status, event->scannerId, event->app_uuid);
}

void GattLibService::HandleBleScannerScanParamsCompleteEvent(
                                                       BleScannerScanParamCompleteEvent *event)
{
  if (!sGattService) return;
  sGattService->onScanParamSetupCompleted(event->status, event->client_if);
}

void GattLibService::HandleBleScannerScanResultEvent(BleScannerScanResultEvent *event)
{
  if (!sGattService) return;
  int len = event->adv_data->size();
  std::vector<uint8_t>p_value(len);
  if (len == 0) {
    ALOGE(LOGTAG "HandleBleScannerScanResultEvent () - Data is NULL");
  }
  else {
    p_value.insert(p_value.begin(), event->adv_data->begin(), event->adv_data->end());
  }
  ALOGE(LOGTAG "HandleBleScannerScanResultEvent () ");
  sGattService->onScanResult(event->event_type, event->addr_type, *(event->bda),
                    event->primary_phy, event->secondary_phy, event->advertising_sid,
                    event->tx_power, event->rssi, event->periodic_adv_int, p_value);
}

void GattLibService::HandleBleScannerBatchScanReportsEvent(
                                                       BleScannerBatchscanReportsEvent *event)
{
  if (!sGattService) return;

  int len = event->data->size();
  std::vector<uint8_t>p_value(len);
  if (len == 0) {
    ALOGE(LOGTAG "HandleBleScannerBatchScanReportsEvent () - Data is NULL");
  }
  else {
    p_value.insert(p_value.begin(), event->data->begin(), event->data->end());
  }
  sGattService->onBatchScanReports(event->status, event->client_if, event->report_format,
                                      event->num_records, p_value);
  delete(event->data);
}

void GattLibService::HandleBleScannerBatchScanThresholdEvent(
                                                       BleScannerBatchscanThresholdEvent *event)
{
  if (!sGattService) return;
  sGattService->onBatchScanThresholdCrossed(event->client_if);
}

void GattLibService::HandleBleScannerTrackAdvEvent(BleScannerTrackAdvEvent *event)
{
  if (!sGattService) return;
  std::vector<uint8_t> advPkt(&event->p_adv_track_info.p_adv_pkt_data[0],
                   &event->p_adv_track_info.p_adv_pkt_data[event->p_adv_track_info.adv_pkt_len]);
  std::vector<uint8_t> scanRspPkt(&event->p_adv_track_info.p_scan_rsp_data[0],
            &event->p_adv_track_info.p_scan_rsp_data[event->p_adv_track_info.scan_rsp_len]);
  AdvtFilterOnFoundOnLostInfo *adv_info = sGattService->createOnTrackAdvFoundLostObject(
                                        event->p_adv_track_info.client_if,
                                        event->p_adv_track_info.adv_pkt_len,
                                        advPkt,
                                        event->p_adv_track_info.scan_rsp_len,
                                        scanRspPkt,
                                        event->p_adv_track_info.filt_index,
                                        event->p_adv_track_info.advertiser_state,
                                        event->p_adv_track_info.advertiser_info_present,
                                        *(event->p_adv_track_info.bd_addr),
                                        event->p_adv_track_info.addr_type,
                                        event->p_adv_track_info.tx_power,
                                        event->p_adv_track_info.rssi_value,
                                        event->p_adv_track_info.time_stamp);

  sGattService->onTrackAdvFoundLost(adv_info);

}

void GattLibService::HandleBleScannerScanFilterCfgEvent(BleScannerScanFilterCfgEvent *event)
{
  if (!sGattService) return;
  sGattService->onScanFilterConfig(event->action, event->status, event->client_if,
                                      event->filt_type, event->avbl_space);
}

void GattLibService::HandleBleScannerScanFilterParamEvent(BleScannerScanFilterParamEvent *event)
{
  if (!sGattService) return;
  sGattService->onScanFilterParamsConfigured(event->action, event->status, event->client_if,
                                                event->avbl_space);
}

void GattLibService::HandleBleScannerScanFilterStatusEvent(
                                                               BleScannerScanFilterStatusEvent *event)
{
  if (!sGattService) return;
  sGattService->onScanFilterEnableDisabled(event->action, event->status, event->client_if);
}

void GattLibService::HandleBleScannerBatchScanCfgStorageEvent(
                                                               BleScannerBatchscanCfgStorageEvent *event)
{
  if (!sGattService) return;
  sGattService->onBatchScanStorageConfigured(event->status, event->client_if);
}

void GattLibService::HandleBleScannerBatchscanStartEvent(BleScannerBatchscanStartEvent *event)
{
  if (!sGattService) return;
  sGattService->onBatchScanStarted(event->status, event->client_if);
}

void GattLibService::HandleBleScannerBatchscanStopEvent(BleScannerBatchscanStopEvent *event)
{
  if (!sGattService) return;
  sGattService->onBatchScanStopped(event->status, event->client_if);
}

void GattLibService::HandleBleAdvertiserSetAdvertisingDataEvent(
                                                               BleAdvertiserSetAdvDataEvent *event)
{
  if (!mAdvertiserManager) return;
  mAdvertiserManager->onAdvertisingDataSet(event->advertiser_id,event->status);
}

void GattLibService::HandleBleAdvertiserSetScanResponseDataEvent(
                                                               BleAdvertiserSetScanRespEvent *event)
{
  if (!mAdvertiserManager) return;
  mAdvertiserManager->onScanResponseDataSet(event->advertiser_id, event->status);
}

void GattLibService::HandleBleAdvertiserSetPeriodicAdvertisingParameterEvent(
                                                      BleAdvertiserSetPeriodicAdvParamEvent *event)
{
  if (!mAdvertiserManager) return;
  mAdvertiserManager->onPeriodicAdvertisingParametersUpdated(event->advertiser_id, event->status);
}

void GattLibService::HandleBleAdvertiserSetPeriodicAdvertisingDataEvent(
                                                      BleAdvertiserSetPeriodicAdvDataEvent *event)
{
  if (!mAdvertiserManager) return;
  mAdvertiserManager->onPeriodicAdvertisingDataSet(event->advertiser_id, event->status);
}

void GattLibService::HandleBleAdvertiserGetOwnAddressEvent(
                                                      BleAdvertiserGetOwnAddressEvent *event)
{
  if (!mAdvertiserManager) return;
  mAdvertiserManager->onOwnAddressRead(event->advertiser_id, event->address_type, event->bda);
}

void GattLibService::HandleBleAdvertiserAdvSetStartEvent(BleAdvertiserAdvSetStartEvent *event)
{
  if (!mAdvertiserManager) return;
  mAdvertiserManager->onAdvertisingSetStarted(event->reg_id, event->advertiser_id,
                                                                    event->tx_power, event->status);
}

void GattLibService::HandleBleAdvertiserAdvSetEnableEvent(BleAdvertiserAdvSetEnableEvent *event)
{
  if (!mAdvertiserManager) return;
  mAdvertiserManager->onAdvertisingEnabled(event->advertiser_id, event->isEnabled, event->status);
}

void GattLibService::HandleBleAdvertiserAdvParameterUpdatedEvent(
                                                      BleAdvertiserAdvSetParamUpdateEvent *event)
{
  if (!mAdvertiserManager) return;
  mAdvertiserManager->onAdvertisingParametersUpdated(event->advertiser_id, event->tx_power,
                                                                              event->status);
}

void GattLibService::HandleBleScannerPeriodicAdvSyncStartEvent(
                                                       BleScannerPeriodicAdvSyncStartEvent *event)
{
  if (!mPeriodicScanManager) return;
  mPeriodicScanManager->onSyncStarted(event->reg_id, event->sync_handle, event->sid,
                          event->address_type, *(event->bda), event->phy, event->interval,
                          event->status);
}
void GattLibService::HandleBleScannerPeriodicAdvSyncLostEvent(
                                                    BleScannerPeriodicAdvSyncLostEvent *event)
{
  if (!mPeriodicScanManager) return;
  mPeriodicScanManager->onSyncLost(event->sync_handle);
}

void GattLibService::HandleBleScannerPeriodicAdvSyncReportEvent(
                                                   BleScannerPeriodicAdvSyncReportEvent *event)
{
  if (!mPeriodicScanManager) return;
  mPeriodicScanManager->onSyncReport(event->sync_handle, event->tx_power, event->rssi,
                                      event->data_status, *(event->data));
}

void GattLibService::HandleBleAdvertiserPeriodicAdvSetEnableEvent(
                                                  BleAdvertiserPeriodicAdvSetEnableEvent *event)
{
  if (!mAdvertiserManager) return;
  mAdvertiserManager->onPeriodicAdvertisingEnabled(event->advertiser_id,
                                                  event->isEnabled, event->status);
}

void GattLibService::HandleGattAdapterPropertyEvent(GattAdapterPropertyEvent *event)
{
  int type = event->type;
  int len = event->len;
  void *val = event->val;

  switch (type)
    {
    case BT_PROPERTY_BDADDR:
      setAddress(event->val,
                          event->len);
      break;
    case BT_PROPERTY_BDNAME:
      setDeviceName(event->val,
                    event->len);
      break;
    case BT_PROPERTY_LOCAL_LE_FEATURES:
      updateFeatureSupport(event->val,
                          event->len);
      break;
    }
}

void GattLibService::HandleBleBatchScanTimeoutEvent(BleScannerBatchscantimeoutEvent *event)
{
  ScanManager *sM = (ScanManager*)event->scanmanager;

  for(std::unordered_set<ScanClient*>::iterator it = sM->getBatchScanQueue().begin();
            it != sM->getBatchScanQueue().end(); ++it){
    sM->flushBatchScanResults(*it);
  }
}

void GattLibService::ProcessEvent(BtEvent* event)
{
   ALOGD(LOGTAG " Processing event %d", event->event_id);
   switch(event->event_id) {
     case BTGATTC_REGISTER_APP_EVENT:
       HandleGattcRegisterAppEvent((GattcRegisterAppEvent *)event);
       break;
     case BTGATTC_OPEN_EVENT:
       HandleGattcOpenEvent((GattcOpenEvent *)event);
       break;
     case BTGATTC_CLOSE_EVENT:
       HandleGattcCloseEvent((GattcCloseEvent *)event);
       break;
     case BTGATTC_SEARCH_COMPLETE_EVENT:
       HandleGattcSearchCompleteEvent((GattcSearchCompleteEvent *)event);
       break;
     case BTGATTC_REGISTER_FOR_NOTIFICATION_EVENT:
       HandleGattcRegisterForNotificationEvent((GattcRegisterForNotificationEvent *)event);
       break;
     case BTGATTC_NOTIFY_EVENT:
       HandleGattcNotifyEvent((GattcNotifyEvent *)event);
       break;
     case BTGATTC_READ_CHARACTERISTIC_EVENT:
       HandleGattcReadCharacteristicEvent((GattcReadCharacteristicEvent *)event);
       break;
     case BTGATTC_WRITE_CHARACTERISTIC_EVENT:
       HandleGattcWriteCharacterisitcEvent((GattcWriteCharacteristicEvent *)event);
       break;
     case BTGATTC_EXECUTE_WRITE_EVENT:
       HandleGattcExecuteWriteEvent((GattcExecuteWriteEvent *)event);
       break;
     case BTGATTC_READ_DESCRIPTOR_EVENT:
       HandleGattcReadDescriptorEvent((GattcReadDescriptorEvent *)event);
       break;
     case BTGATTC_WRITE_DESCRIPTOR_EVENT:
       HandleGattcWriteDescriptorEvent((GattcWriteDescriptorEvent *)event);
       break;
     case BTGATTC_REMOTE_RSSI_EVENT:
       HandleGattcRemoteRssiEvent((GattcRemoteRssiEvent *)event);
       break;
     case BTGATTC_CONFIGURE_MTU_EVENT:
       HandleGattcConfigureMtuEvent((GattcConfigureMtuEvent *)event);
       break;
     case BTGATTC_CONGESTION_EVENT:
       HandleGattcCongestionEvent((GattcCongestionEvent *)event);
       break;
     case BTGATTC_GET_GATT_DB_EVENT:
       HandleGattcGetGattDbEvent((GattcGetGattDbEvent *)event);
       break;
     case BTGATTC_PHY_UPDATED_EVENT:
       HandleGattcPhyUpdatedEvent((GattcPhyUpdatedEvent *)event);
       break;
     case BTGATTC_CONN_UPDATED_EVENT:
       HandleGattcConnUpdatedEvent((GattcConnUpdatedEvent *)event);
       break;
     case BTGATTC_READ_PHY_EVENT:
       HandleGattcReadPhyEvent((GattcReadPhyEvent *)event);
       break;
     case BTGATTS_REGISTER_APP_EVENT:
       HandleGattsRegisterAppEvent((GattsRegisterAppEvent *)event);
       break;
     case BTGATTS_CONNECTION_EVENT:
       HandleGattsConnectionEvent((GattsConnectionEvent *)event);
       break;
     case BTGATTS_SERVICE_ADDED_EVENT:
       HandleGattsServiceAddedEvent((GattsServiceAddedEvent *)event);
       break;
     case BTGATTS_SERVICE_STOPPED_EVENT:
       HandleGattsServiceStoppedEvent((GattsServiceStoppedEvent *)event);
       break;
     case BTGATTS_SERVICE_DELETED_EVENT:
       HandleGattsServiceDeletedEvent((GattsServiceDeletedEvent *)event);
       break;
     case BTGATTS_REQUEST_READ_CHARACTERISTIC_EVENT:
       HandleGattsRequestReadCharacteristicEvent((GattsRequestReadCharacteristicEvent *)event);
       break;
     case BTGATTS_REQUEST_READ_DESCRIPTOR_EVENT:
       HandleGattsRequestReadDescriptorEvent((GattsRequestReadDescriptorEvent *)event);
       break;
     case BTGATTS_REQUEST_WRITE_CHARACTERISTIC_EVENT:
       HandleGattsRequestWriteCharacteristicEvent((GattsRequestWriteCharacteristicEvent *)event);
       break;
     case BTGATTS_REQUEST_WRITE_DESCRIPTOR_EVENT:
       HandleGattsRequestWriteDescriptorEvent((GattsRequestWriteDescriptorEvent *)event);
       break;
     case BTGATTS_REQUEST_EXEC_WRITE_EVENT:
       HandleGattsRequestExecWriteEvent((GattsRequestExecWriteEvent *)event);
       break;
     case BTGATTS_RESPONSE_CONFIRMATION_EVENT:
       HandleGattsResponseConfirmationEvent((GattsResponseConfirmationEvent *)event);
       break;
     case BTGATTS_INDICATION_SENT_EVENT:
       HandleGattsIndicationSentEvent((GattsIndicationSentEvent *)event);
       break;
     case BTGATTS_CONGESTION_EVENT:
       HandleGattsCongestionEvent((GattsCongestionEvent *)event);
       break;
     case BTGATTS_MTU_CHANGED_EVENT:
       HandleGattsMtuChangedEvent((GattsMTUchangedEvent *)event);
       break;
     case BTGATTS_PHY_UPDATED_EVENT:
       HandleGattsPhyUpdatedEvent((GattsPhyUpdatedEvent *)event);
       break;
     case BTGATTS_CONN_UPDATED_EVENT:
       HandleGattsConnUpdatedEvent((GattsConnUpdatedEvent *)event);
       break;
     case BTGATTS_READ_PHY_EVENT:
       HandleGattsReadPhyEvent((GattsReadPhyEvent *)event);
       break;
     case BLESCANNER_REGISTER_SCANNER_EVENT:
       HandleBleScannerRegisterScannerEvent((BleScannerRegisterScannerEvent *)event);
       break;
     case BLESCANNER_SCAN_PARAMS_COMPLETE_EVENT:
       HandleBleScannerScanParamsCompleteEvent((BleScannerScanParamCompleteEvent *)event);
       break;
     case BLESCANNER_SCAN_RESULT_EVENT:
       HandleBleScannerScanResultEvent((BleScannerScanResultEvent *)event);
       break;
     case BLESCANNER_BATCHSCAN_REPORTS_EVENT:
       HandleBleScannerBatchScanReportsEvent((BleScannerBatchscanReportsEvent *)event);
       break;
     case BLESCANNER_BATCHSCAN_THRESHOLD_EVENT:
       HandleBleScannerBatchScanThresholdEvent((BleScannerBatchscanThresholdEvent *)event);
       break;
     case BLESCANNER_TRACK_ADV_EVENT_EVENT:
       HandleBleScannerTrackAdvEvent((BleScannerTrackAdvEvent *)event);
       break;
     case BLESCANNER_SCAN_FILTER_CFG_EVENT:
       HandleBleScannerScanFilterCfgEvent((BleScannerScanFilterCfgEvent *)event);
       break;
     case BLESCANNER_SCAN_FILTER_PARAM_EVENT:
       HandleBleScannerScanFilterParamEvent((BleScannerScanFilterParamEvent *)event);
       break;
     case BLESCANNER_SCAN_FILTER_STATUS_EVENT:
       HandleBleScannerScanFilterStatusEvent((BleScannerScanFilterStatusEvent *)event);
       break;
     case BLESCANNER_BATCHSCAN_CFG_STORAGE_EVENT:
       HandleBleScannerBatchScanCfgStorageEvent((BleScannerBatchscanCfgStorageEvent *)event);
       break;
     case BLESCANNER_BATCHSCAN_START_EVENT:
       HandleBleScannerBatchscanStartEvent((BleScannerBatchscanStartEvent *)event);
       break;
     case BLESCANNER_BATCHSCAN_STOP_EVENT:
       HandleBleScannerBatchscanStopEvent((BleScannerBatchscanStopEvent *)event);
       break;
     case BLEADVERTISER_SET_ADVERTISING_DATA_EVENT:
       HandleBleAdvertiserSetAdvertisingDataEvent((BleAdvertiserSetAdvDataEvent *)event);
       break;
     case BLEADVERTISER_SET_SCAN_RESPONSE_DATA_EVENT:
       HandleBleAdvertiserSetScanResponseDataEvent((BleAdvertiserSetScanRespEvent *)event);
       break;
     case BLEADVERTISER_SET_PERIODIC_ADVERTISING_PARAMETER_EVENT:
       HandleBleAdvertiserSetPeriodicAdvertisingParameterEvent(
                                        (BleAdvertiserSetPeriodicAdvParamEvent *)event);
       break;
     case BLEADVERTISER_SET_PERIODIC_ADVERTISING_DATA_EVENT:
       HandleBleAdvertiserSetPeriodicAdvertisingDataEvent(
                                        (BleAdvertiserSetPeriodicAdvDataEvent *)event);
       break;
     case BLEDAVERTISER_GET_OWN_ADDRESS_EVENT:
       HandleBleAdvertiserGetOwnAddressEvent((BleAdvertiserGetOwnAddressEvent *)event);
       break;
     case BLEDAVERTISER_ADVERTISING_SET_START_EVENT:
       HandleBleAdvertiserAdvSetStartEvent((BleAdvertiserAdvSetStartEvent *)event);
       break;
     case BLEDAVERTISER_ADVERTISING_SET_ENABLE_EVENT:
       HandleBleAdvertiserAdvSetEnableEvent((BleAdvertiserAdvSetEnableEvent *)event);
       break;
     case BLEADVERTISER_ADVERTISING_PARAMETER_UPDATED_EVENT:
       HandleBleAdvertiserAdvParameterUpdatedEvent((BleAdvertiserAdvSetParamUpdateEvent *)event);
       break;
     case BLEDAVERTISER_PERIODIC_ADVERTISING_SET_ENABLE_EVENT:
       HandleBleAdvertiserPeriodicAdvSetEnableEvent
                                      ((BleAdvertiserPeriodicAdvSetEnableEvent *)event);
       break;
     case BLESCANNER_PERIODIC_ADVERTISING_SYNC_START_EVENT:
       HandleBleScannerPeriodicAdvSyncStartEvent((BleScannerPeriodicAdvSyncStartEvent *)event);
       break;
     case BLESCANNER_PERIODIC_ADVERTISING_SYNC_LOST_EVENT:
       HandleBleScannerPeriodicAdvSyncLostEvent((BleScannerPeriodicAdvSyncLostEvent *)event);
       break;
     case BLESCANNER_PERIODIC_ADVERTISING_SYNC_REPORT_EVENT:
       HandleBleScannerPeriodicAdvSyncReportEvent((BleScannerPeriodicAdvSyncReportEvent *)event);
       break;
     case GATT_EVENT_ADAPTER_PROPERTIES:
       HandleGattAdapterPropertyEvent((GattAdapterPropertyEvent *)event);
       break;
     case BLESCANNER_BATCHSCAN_TIMEOUT_EVENT:
       HandleBleBatchScanTimeoutEvent((BleScannerBatchscantimeoutEvent *)event);
       break;
     default: //All fall-through, enable as needed
       ALOGD(LOGTAG  "(BtMsgHandler) Unhandled Event(%d)", event->event_id);
   }
}
}
