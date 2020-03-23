/*
 * Copyright (c) 2018, The Linux Foundation. All rights reserved.
 * Not a Contribution.
 * Copyright (C) 2013 The Android Open Source Project
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

#include "GattServer.hpp"
#include "GattService.hpp"

#include <iostream>
#include <vector>
#include <string>
#include <mutex>
#include <exception>
#include <chrono>
#include <thread>

#define LOGTAG "GattServer"

using namespace std;
namespace gatt{

void GattServer::onServerRegistered(int status, int serverIf)
{
  if (DBG) {
    ALOGD(LOGTAG " onServerRegistered() - status %d serverIf %d", status, serverIf);
  }

  if (mCallback != NULL) {
    mServerIf = serverIf;
    try {
      mCallback->onServerRegistered(status, mServerIf);
    } catch (std::exception& e) {
        ALOGE(LOGTAG " Unhandled exception in callback: %s", e.what());
    }
  } else {
    // registration timeout
    ALOGE(LOGTAG " onServerRegistered() : mCallback is null");
  }
}

void GattServer::onConnectionState(int status, int serverIf,
                                 bool connected, string address)
{
  if (DBG) {
    ALOGD(LOGTAG " onServerConnectionState() - status %d serverIf %d device %s",
                    status, serverIf, address.c_str());
  }

  try {
    mCallback->onConnectionStateChange(address, status,
            connected ? GattDevice::STATE_CONNECTED :
                    GattDevice::STATE_DISCONNECTED);
  } catch (std::exception& e) {
    ALOGE(LOGTAG " Unhandled exception in callback: %s", e.what());
  }
}

void GattServer::onServiceAdded(int status, GattService *service)
{
  if (DBG) {
      ALOGD(LOGTAG " onServiceAdded() - handle %d uuid %s status %d",
                service->getInstanceId(), service->getUuid().ToString().c_str(), status);
  }

  if (mPendingService == NULL) {
      return;
  }

  GattService *tmp = mPendingService;
  mPendingService = NULL;

  // Rewrite newly assigned handles to existing service.
  tmp->setInstanceId(service->getInstanceId());
  std::vector<GattCharacteristic*> svc_chars = service->getCharacteristics();
  std::vector<GattCharacteristic*> temp_chars = tmp->getCharacteristics();

  for (int i = 0; i < svc_chars.size(); i++) {
    GattCharacteristic *temp_char = temp_chars.at(i);
    GattCharacteristic *svc_char = svc_chars.at(i);

    temp_char->setInstanceId(svc_char->getInstanceId());

    std::vector<GattDescriptor*> temp_descs = temp_char->getDescriptors();
    std::vector<GattDescriptor*> svc_descs = svc_char->getDescriptors();

    for (int j = 0; j < svc_descs.size(); j++) {
       temp_descs.at(j)->setInstanceId(svc_descs.at(j)->getInstanceId());
    }
  }

  mServices.push_back(tmp);
  delete service;
  try {
    mCallback->onServiceAdded(status, tmp);
  } catch (std::exception& e) {
      ALOGE(LOGTAG " Unhandled exception in callback: %s", e.what());
  }
}

void GattServer::onCharacteristicReadRequest(string address, int transId, int offset,
                                     bool isLong, int handle)
{
  if (VDBG) ALOGD(LOGTAG " onCharacteristicReadRequest() - handle %d", handle);

  GattCharacteristic *characteristic = getCharacteristicByHandle(handle);
  if (characteristic == NULL) {
    ALOGW(LOGTAG " onCharacteristicReadRequest() - no char for handle %d", handle);
    return;
  }

  try {
    mCallback->onCharacteristicReadRequest(address, transId, offset,
            characteristic);
  } catch (std::exception& e) {
    ALOGE(LOGTAG " Unhandled exception in callback: %s", e.what());
  }
}

void GattServer::onDescriptorReadRequest(string address, int transId,
                                    int offset, bool isLong,
                                    int handle)
{
  if (VDBG) ALOGD(LOGTAG " onDescriptorReadRequest() - handle %d", handle);

  GattDescriptor *descriptor = getDescriptorByHandle(handle);
  if (descriptor == NULL) {
    ALOGE(LOGTAG " onDescriptorReadRequest() - no desc for handle %d", handle);
    return;
  }

  try {
    mCallback->onDescriptorReadRequest(address, transId, offset, descriptor);
  } catch (std::exception& e) {
    ALOGE(LOGTAG " Unhandled exception in callback: %s", e.what());
  }
}

void GattServer::onCharacteristicWriteRequest(string address, int transId, int offset,
                                  int length, bool isPrep, bool needRsp,
                                  int handle, uint8_t *value)
{
  if (VDBG) ALOGD(LOGTAG " onCharacteristicWriteRequest() - handle %d", handle);

  GattCharacteristic *characteristic = getCharacteristicByHandle(handle);
  if (characteristic == NULL) {
    ALOGE(LOGTAG " onCharacteristicWriteRequest() - no char for handle %d", handle);
    return;
  }

  try {
    mCallback->onCharacteristicWriteRequest(address, transId, characteristic,
            isPrep, needRsp, offset, value, length);
  } catch (std::exception& e) {
    ALOGE(LOGTAG " Unhandled exception in callback: %s", e.what());
  }

}

void GattServer::onDescriptorWriteRequest(string address, int transId, int offset,
                                   int length, bool isPrep, bool needRsp,
                                    int handle, uint8_t *value)
{
  if (VDBG) ALOGD(LOGTAG " onDescriptorWriteRequest() - handle %d", handle);

  GattDescriptor *descriptor = getDescriptorByHandle(handle);
  if (descriptor == NULL) {
    ALOGE(LOGTAG " onDescriptorWriteRequest() - no char for handle %d", handle);
    return;
  }

  try {
    mCallback->onDescriptorWriteRequest(address, transId, descriptor,
            isPrep, needRsp, offset, value, length);
  } catch (std::exception& e) {
    ALOGE(LOGTAG " Unhandled exception in callback: %s", e.what());
  }
}

void GattServer::onExecuteWrite(string address, int transId, bool execWrite)
{
  if (DBG) {
    ALOGD(LOGTAG " onServerExecuteWrite() - device %s transId %d execWrite %d ",
                            address.c_str(), transId, execWrite);
  }

  try {
    mCallback->onExecuteWrite(address, transId, execWrite);
  } catch (std::exception& e) {
    ALOGE(LOGTAG " Unhandled exception in callback: %s", e.what());
  }
}

void GattServer::onNotificationSent(string address, int status)
{
  if (VDBG) {
    ALOGD(LOGTAG " onServerNotificationSent() - device %s status %d",
                  address.c_str(), status);
  }

  try {
    mCallback->onNotificationSent(address, status);
  } catch (std::exception& e) {
    ALOGE(LOGTAG " Unhandled exception in callback: %s", e.what());
  }
}

void GattServer::onMtuChanged(string address, int mtu)
{
  if (DBG) {
    ALOGD(LOGTAG " onServerMtuChanged() - device %s mtu %d ",
              address.c_str(), mtu);
  }

  try {
    mCallback->onMtuChanged(address, mtu);
  } catch (std::exception& e) {
      ALOGE(LOGTAG " Unhandled exception in callback: %s", e.what());
  }
}

void GattServer::onPhyUpdate(string address, int txPhy, int rxPhy, int status)
{
  if (DBG) {
    ALOGD(LOGTAG " onServerPhyUpdate() - device %s txPhy %d rxPhy %d ",
                 address.c_str(), txPhy, rxPhy);
  }

  try {
    mCallback->onPhyUpdate(address, txPhy, rxPhy, status);
  } catch (std::exception& e) {
      ALOGE(LOGTAG " Unhandled exception in callback: %s", e.what());
  }
}

void GattServer::onPhyRead(string address, int txPhy, int rxPhy, int status)
{
  if (DBG) {
    ALOGD(LOGTAG " onServerPhyRead() - device %s txPhy %d rxPhy %d ",
                address.c_str(), txPhy, rxPhy);
  }

  try {
    mCallback->onPhyRead(address, txPhy, rxPhy, status);
  } catch (std::exception& e) {
      ALOGE(LOGTAG " Unhandled exception in callback: %s", e.what());
  }
}

void GattServer::onConnectionUpdated(string address, int interval, int latency,
                            int timeout, int status)
{
  if (DBG) {
    ALOGD(LOGTAG
      " onServerConnectionUpdated() - Device %s interval %d latency %d timeout %d status %d",
      address.c_str(), interval, latency, timeout, status);
  }

  try {
    mCallback->onConnectionUpdated(address, interval, latency,
             timeout, status);
  } catch (std::exception& e) {
      ALOGE(LOGTAG " Unhandled exception in callback: %s", e.what());
  }
}

GattServer::GattServer(GattLibService *gatt,int transport)
{
  mService = gatt;
  mCallback = NULL;
  mServerIf = 0;
  mTransport = transport;
}

GattServer::~GattServer()
{
  if (mService != NULL)
      mService = NULL;

  if (mCallback != NULL)
      mCallback = NULL;

  if (mServerCallback != NULL)
    mServerCallback = NULL;

  if (mPendingService != NULL)
    mPendingService = NULL;

  for (GattService *svc : mServices)
    delete svc;
  mServices.clear();
}

GattCharacteristic* GattServer::getCharacteristicByHandle(int handle)
{
  for (GattService *svc : mServices) {
    for (GattCharacteristic *charac : svc->getCharacteristics()) {
      if (charac->getInstanceId() == handle) {
        return charac;
      }
    }
  }
  return nullptr;
}

GattDescriptor* GattServer::getDescriptorByHandle(int handle)
{
  for (GattService *svc : mServices) {
    for (GattCharacteristic *charac : svc->getCharacteristics()) {
      for (GattDescriptor *desc : charac->getDescriptors()) {
        if (desc->getInstanceId() == handle) {
          return desc;
        }
      }
    }
 }
 return NULL;
}

void GattServer::close()
{
  if (DBG) ALOGD(LOGTAG " close()");
  unregisterCallback();
}

bool GattServer::registerCallback(GattServerCallback& callback)
{
  if (DBG) ALOGD(LOGTAG " registerCallback()");
  if (mService == nullptr) {
    ALOGE(LOGTAG " GATT service not available");
    return false;
  }
  Uuid uuid = Uuid::GetRandom();
  if (DBG) ALOGD(LOGTAG " registerCallback() - UUID= %s", uuid.ToString().c_str());

  if (mCallback != NULL) {
    ALOGE(LOGTAG " App can register callback only once");
    return false;
  }

  mCallback = &callback;
  mServerCallback = this;
  try {
    mService->registerServer(uuid, *mServerCallback);
  } catch (std::exception& e) {
      ALOGE(LOGTAG " %s", e.what());
      mCallback = NULL;
      return false;
  }

  try {
      std::this_thread::sleep_for(std::chrono::milliseconds(CALLBACK_REG_TIMEOUT));
  } catch (std::exception &e) {
      ALOGE(LOGTAG " %s", e.what());
      mCallback = NULL;
  }

  if (mServerIf == 0) {
    mCallback = NULL;
    return false;
  } else {
    return true;
  }
}

GattService* GattServer::getService(Uuid uuid, int instanceId, int type)
{
  for (GattService *svc : mServices) {
    if ( (svc->getType() == type)
           && (svc->getInstanceId() == instanceId)
           && (svc->getUuid() == uuid)) {
      return svc;
    }
 }
 return NULL;
}

void GattServer::unregisterCallback()
{
  if (DBG) ALOGD(LOGTAG " unregisterCallback() - mServerIf= %d", mServerIf);
  if (mService == NULL || mServerIf == 0) return;

  try {
    mCallback = NULL;
    mService->unregisterServer(mServerIf);
    mServerIf = 0;
  } catch (std::exception& e) {
      ALOGE(LOGTAG " %s", e.what());
  }
}

bool GattServer::connect(string deviceAddress, bool autoConnect)
{
  if (DBG) {
    ALOGD(LOGTAG " connect() - device: %s , auto: %d",
                    deviceAddress.c_str() , autoConnect);
  }
  if (mService == NULL || mServerIf == 0) return false;

  try {
    // autoConnect is inverse of "isDirect"
    mService->serverConnect(mServerIf, deviceAddress, !autoConnect, mTransport);
  } catch (std::exception& e) {
      ALOGE(LOGTAG " %s", e.what());
      return false;
  }
  return true;
}

void GattServer::cancelConnection(string deviceAddress)
{
  if (DBG) ALOGD(LOGTAG " cancelConnection() - device: %s ", deviceAddress.c_str());
  if (mService == NULL || mServerIf == 0) return;

  try {
    mService->serverDisconnect(mServerIf, deviceAddress);
  } catch (std::exception& e) {
      ALOGE(LOGTAG " %s", e.what());
  }
}

void GattServer::setPreferredPhy(string deviceAddress, int txPhy, int rxPhy, int phyOptions)
{
  try {
    mService->serverSetPreferredPhy(mServerIf, deviceAddress, txPhy, rxPhy,
          phyOptions);
  } catch (std::exception& e) {
      ALOGE(LOGTAG " %s", e.what());
  }
}

void GattServer::readPhy(string deviceAddress)
{
  try {
    mService->serverReadPhy(mServerIf, deviceAddress);
  } catch (std::exception& e) {
      ALOGE(LOGTAG " %s", e.what());
  }

}

bool GattServer::sendResponse(string deviceAddress, int requestId,
          int status, int offset, uint8_t *value)
{
  if (VDBG) ALOGD(LOGTAG " sendResponse() - device: %s", deviceAddress.c_str());
  if (mService == NULL || mServerIf == 0) return false;

  try {
    mService->sendResponse(mServerIf, deviceAddress, requestId,
            status, offset, value);
  } catch (std::exception& e) {
      ALOGE(LOGTAG " %s", e.what());
      return false;
  }
  return true;
}

bool GattServer::notifyCharacteristicChanged(string deviceAddress,
          GattCharacteristic &characteristic, bool confirm)
{
  if (VDBG) ALOGD(LOGTAG " notifyCharacteristicChanged() - device: %s ",
                  deviceAddress.c_str());
  if (mService == NULL || mServerIf == 0) return false;

  GattService *service = characteristic.getService();
  if (service == NULL) return false;

  if (characteristic.getValue() == NULL) {
    throw std::invalid_argument
      ("Chracteristic value is empty. Use BluetoothGattCharacteristic#setvalue to update");
  }

  try {
    mService->sendNotification(mServerIf, deviceAddress,
            characteristic.getInstanceId(), confirm,
            characteristic.getValue());
  } catch (std::exception& e) {
      ALOGE(LOGTAG " %s", e.what());
      return false;
  }
  return true;
}

bool GattServer::addService(GattService &service)
{
  if (DBG) ALOGD(LOGTAG " addService() - service: %s",
          service.getUuid().ToString().c_str());
  if (mService == NULL || mServerIf == 0) return false;

  mPendingService = &service;

  try {
    mService->addService(mServerIf, &service);
  } catch (std::exception& e) {
      ALOGE(" %s", e.what());
      return false;
  }
  return true;
}

bool GattServer::removeService(GattService &service)
{
  if (DBG) ALOGD(LOGTAG " removeService() - service: %s",
                  service.getUuid().ToString().c_str());
  if (mService == NULL || mServerIf == 0) return false;

  GattService *intService = getService(service.getUuid(),
          service.getInstanceId(), service.getType());
  if (intService == NULL) return false;

  try {
    mService->removeService(mServerIf, service.getInstanceId());
    auto it = std::find(mServices.begin(), mServices.end(), intService);
    // check that there actually is a 3 in our vector
    if (it != mServices.end()) {
      mServices.erase(it);
    }
  } catch (std::exception& e) {
      ALOGE(LOGTAG " %s", e.what());
      return false;
  }

  return true;
}

void GattServer::clearServices()
{
  if (DBG) ALOGD(LOGTAG " clearServices()");
  if (mService == NULL || mServerIf == 0) return;

  try {
    mService->clearServices(mServerIf);
    mServices.clear();
  } catch (std::exception& e) {
      ALOGE(LOGTAG " %s", e.what());
  }
}

std::vector<GattService*> GattServer::getServices()
{
  return mServices;
}

GattService* GattServer::getService(Uuid uuid)
{
  for (GattService *service : mServices) {
     if (service->getUuid() == uuid) {
         return service;
     }
   }

   return NULL;
}
}
