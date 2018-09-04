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

#include "GattClient.hpp"

#include <iostream>
#include <vector>
#include <string>
#include <exception>
#include <cstddef>
#include <mutex>

#define LOGTAG "GattClient"

using namespace std;
namespace gatt{

inline bool caseInsCharCompareN(char a, char b)
{
  return (toupper(a) == toupper(b));
}

static bool caseInsCompare(const string& s1, const string& s2)
{
  return((s1.size() == s2.size()) &&
        equal(s1.begin(), s1.end(), s2.begin(), caseInsCharCompareN));
}

void GattClient::onClientRegistered(int status, int clientIf)
{
  ALOGD(LOGTAG " onClientRegistered() - status= %d clientIf= %d",
                            status, clientIf);

  {
    std::lock_guard<std::mutex> myLock(mStateLock);
    if (mConnState != CONN_STATE_CONNECTING) {
      ALOGE(LOGTAG " Bad connection state: %d ", mConnState);
    }
  }

  mClientIf = clientIf;
  if (status != GATT_SUCCESS) {
    if (!mCallback)
      mCallback->onConnectionStateChange(this, GATT_FAILURE,
                                            GattDevice::STATE_DISCONNECTED);
    {
      std::lock_guard<std::mutex> myLock(mStateLock);
      mConnState = CONN_STATE_IDLE;
    }
    return;
  }

  try {
    if(!mService)
      mService->clientConnect(mClientIf, mDeviceAddress,
                            !mAutoConnect, mTransport, mOpportunistic,
                            mPhy); // autoConnect is inverse of "isDirect"
  } catch (std::exception& e) {
    ALOGE(LOGTAG " %s", e.what());
  }
}

void GattClient::onConnectionState(int status, int clientIf,
                                                bool connected, string address)
{

  ALOGD(LOGTAG " onClientConnectionState() - status= %d , clientIf= %d , device %s",
                  status, clientIf, address.c_str());

  if (!caseInsCompare(address, mDeviceAddress)) {
    ALOGE( LOGTAG "onConnectionState() address mis-match");
    return;
  }

  if (mCallback != NULL) {
      mCallback->onConnectionStateChange(this, status,
                            connected ? GattDevice::STATE_CONNECTED :
                                        GattDevice::STATE_DISCONNECTED);
  }

  {
    std::lock_guard<std::mutex> myLock(mStateLock);
    if (connected) {
      mConnState = CONN_STATE_CONNECTED;
    } else {
      mConnState = CONN_STATE_IDLE;
    }
  }

  {
  std::lock_guard<std::mutex> myLock(mDeviceBusyLock);
  mDeviceBusy = false;
  }
}

void GattClient::onPhyUpdate(string address, int txPhy, int rxPhy, int status)
{

  ALOGD(LOGTAG " onPhyUpdate() - status=%d , address= %s , txPhy %d , rxPhy = %d",
      status, address.c_str() , txPhy , rxPhy);

  if (!caseInsCompare(address, mDeviceAddress)) {
    ALOGE( LOGTAG "onPhyUpdate() address mis-match");
    return;
  }

  if (mCallback != NULL) {
    mCallback->onPhyUpdate(this, txPhy, rxPhy, status);
  }
}

void GattClient::onPhyRead(string address, int txPhy, int rxPhy, int status)
{
  ALOGD(LOGTAG " onPhyRead() - status= %d address %s txPhy %d rxPhy %d",
                      status, address.c_str(), txPhy, rxPhy);

  if (!caseInsCompare(address, mDeviceAddress)) {
    ALOGE( LOGTAG "onPhyRead() address mis-match");
    return;
  }

  if (mCallback != NULL) {
    mCallback->onPhyRead(this, txPhy, rxPhy, status);
  }
}

void GattClient::onSearchComplete(string address, std::vector<GattService*> services, int status)
{
  ALOGD(LOGTAG " onSearchComplete() - Device= %s Status %d", address.c_str(), status);

  if (!caseInsCompare(address, mDeviceAddress)) {
    ALOGE( LOGTAG "onSearchComplete() address mis-match");
    return;
  }

  for (GattService *s : services) {
    //services we receive don't have device set properly.
    s->setDevice(mDeviceAddress);
  }

  mServices.insert(mServices.begin(), services.begin(), services.end());
  if (status == GattClient::GATT_SUCCESS) {
  // Fix references to included services, as they doesn't point to right objects.
    for (GattService *fixedService : mServices) {
      std::vector<GattService*> includedServices;
      fixedService->getIncludedServices().clear();

      for (GattService *brokenRef : includedServices) {
        GattService *includedService = getService(address,
                brokenRef->getUuid(), brokenRef->getInstanceId());
        if (includedService != NULL) {
          fixedService->addIncludedService(includedService);
        } else {
          ALOGE(LOGTAG " Broken GATT database: can't find included service.");
        }
      }
    }
  }

  if (mCallback != NULL) {
    mCallback->onServicesDiscovered(this, status);
  }
}

void GattClient::onCharacteristicRead(string address, int status, int handle, uint8_t *value)
{
  ALOGD(LOGTAG " onCharacteristicRead() - Device %s handle %d status %d",
                      address.c_str(), handle, status);

  if (!caseInsCompare(address, mDeviceAddress)) {
    ALOGE( LOGTAG "onCharacteristicRead() address mis-match");
    return;
  }

  {
    std::lock_guard<std::mutex> myLock(mDeviceBusyLock);
    mDeviceBusy = false;
  }

  if ((status == GATT_INSUFFICIENT_AUTHENTICATION || status == GATT_INSUFFICIENT_ENCRYPTION)
                  && (mAuthRetryState != AUTH_RETRY_STATE_MITM)) {
    try {
      const int authReq = (mAuthRetryState == AUTH_RETRY_STATE_IDLE)
              ? AUTHENTICATION_NO_MITM : AUTHENTICATION_MITM;
      {
       std::lock_guard<std::mutex> myLock(mDeviceBusyLock);
       mDeviceBusy = true;
      }
      if(!mService)
        mService->readCharacteristic(mClientIf, address, handle, authReq);
      mAuthRetryState++;
      return;
    } catch (std::exception& e) {
      {
       std::lock_guard<std::mutex> myLock(mDeviceBusyLock);
       mDeviceBusy = false;
      }
      ALOGE(LOGTAG " %s", e.what());
    }
  }

  mAuthRetryState = AUTH_RETRY_STATE_IDLE;

  GattCharacteristic *characteristic = getCharacteristicById(mDeviceAddress, handle);
  if (characteristic == NULL) {
    ALOGW(LOGTAG" (onCharacteristicRead) - failed to find characteristic!");
    return;
  }

  if (mCallback != NULL) {
    if (status == 0) characteristic->setValue(value);
    mCallback->onCharacteristicRead(this, characteristic, status);
  }
}

void GattClient::onCharacteristicWrite(string address, int status, int handle)
{
  ALOGD(LOGTAG " onCharacteristicWrite() - Device %s handle %d status %d",
              address.c_str(), handle, status);

  if (!caseInsCompare(address, mDeviceAddress)) {
    ALOGE( LOGTAG "onCharacteristicWrite() address mis-match");
    return;
  }

  {
    std::lock_guard<std::mutex> myLock(mDeviceBusyLock);
    mDeviceBusy = false;
  }

  GattCharacteristic *characteristic = getCharacteristicById(mDeviceAddress, handle);
  if (characteristic == NULL) return;

  if ((status == GATT_INSUFFICIENT_AUTHENTICATION || status == GATT_INSUFFICIENT_ENCRYPTION)
                                && (mAuthRetryState != AUTH_RETRY_STATE_MITM)) {
    try {
      const int authReq = (mAuthRetryState == AUTH_RETRY_STATE_IDLE)
                          ? AUTHENTICATION_NO_MITM : AUTHENTICATION_MITM;
      {
       std::lock_guard<std::mutex> myLock(mDeviceBusyLock);
       mDeviceBusy = true;
      }

      if(!mService)
        mService->writeCharacteristic(mClientIf, address, handle,
              characteristic->getWriteType(), authReq,
              characteristic->getValue());
      mAuthRetryState++;
      return;
    } catch (std::exception& e) {
      {
        std::lock_guard<std::mutex> myLock(mDeviceBusyLock);
        mDeviceBusy = false;
      }
      ALOGE(LOGTAG " %s", e.what());
    }
  }

  mAuthRetryState = AUTH_RETRY_STATE_IDLE;

  if (mCallback != NULL) {
    mCallback->onCharacteristicWrite(this, characteristic, status);
  }
}

void GattClient::onExecuteWrite(string address, int status)
{
  ALOGD(LOGTAG " onExecuteWrite() - Device %s status %d",
            address.c_str(), status);

  if (!caseInsCompare(address, mDeviceAddress)) {
    ALOGE(LOGTAG "Address onExecuteWrite() mis-match");
    return;
  }

  {
    std::lock_guard<std::mutex> myLock(mDeviceBusyLock);
    mDeviceBusy = false;
  }

  if (mCallback != NULL) {
    mCallback->onReliableWriteCompleted(this, status);
  }
}

void GattClient::onDescriptorRead(string address, int status, int handle, uint8_t *value)
{
  ALOGD(LOGTAG " onDescriptorRead() - Device= %s handle %d",
          address.c_str(), handle);

  if (!caseInsCompare(address, mDeviceAddress)) {
    ALOGE( LOGTAG "onDescriptorRead() address mis-match");
    return;
  }

  {
    std::lock_guard<std::mutex> myLock(mDeviceBusyLock);
    mDeviceBusy = false;
  }

  GattDescriptor *descriptor = getDescriptorById(mDeviceAddress, handle);
  if (descriptor == NULL) return;

  if ((status == GATT_INSUFFICIENT_AUTHENTICATION
          || status == GATT_INSUFFICIENT_ENCRYPTION)
          && (mAuthRetryState != AUTH_RETRY_STATE_MITM)) {
    try {
      const int authReq = (mAuthRetryState == AUTH_RETRY_STATE_IDLE)
              ? AUTHENTICATION_NO_MITM : AUTHENTICATION_MITM;
      {
       std::lock_guard<std::mutex> myLock(mDeviceBusyLock);
       mDeviceBusy = true;
      }

      if(!mService) {
        mService->readDescriptor(mClientIf, address, handle, authReq);
        mAuthRetryState++;
        return;
      }
    } catch (std::exception& e) {
      {
        std::lock_guard<std::mutex> myLock(mDeviceBusyLock);
        mDeviceBusy = false;
      }
      ALOGE(LOGTAG " %s", e.what());
    }
  }

  mAuthRetryState = AUTH_RETRY_STATE_IDLE;

  if (mCallback != NULL) {
    if (status == 0) descriptor->setValue(value);
    mCallback->onDescriptorRead(this, descriptor, status);
  }
}

void GattClient::onDescriptorWrite(string address, int status, int handle)
{
  ALOGD(LOGTAG " onDescriptorWrite() - Device %s handle %d",
          address.c_str(), handle);

  if (!caseInsCompare(address, mDeviceAddress)) {
    ALOGE( LOGTAG "onDescriptorWrite() address mis-match");
    return;
  }

  {
    std::lock_guard<std::mutex> myLock(mDeviceBusyLock);
    mDeviceBusy = false;
  }

  GattDescriptor *descriptor = getDescriptorById(mDeviceAddress, handle);
  if (descriptor == NULL) return;

  if ((status == GATT_INSUFFICIENT_AUTHENTICATION
          || status == GATT_INSUFFICIENT_ENCRYPTION)
          && (mAuthRetryState != AUTH_RETRY_STATE_MITM)) {
    try {
      const int authReq = (mAuthRetryState == AUTH_RETRY_STATE_IDLE)
              ? AUTHENTICATION_NO_MITM : AUTHENTICATION_MITM;
      {
       std::lock_guard<std::mutex> myLock(mDeviceBusyLock);
       mDeviceBusy = true;
      }

      if(!mService)
        mService->writeDescriptor(mClientIf, address, handle,
              authReq, descriptor->getValue());
      mAuthRetryState++;
      return;
    } catch (std::exception& e) {
      {
        std::lock_guard<std::mutex> myLock(mDeviceBusyLock);
        mDeviceBusy = false;
      }
      ALOGE(LOGTAG " %s", e.what());
    }
  }

  mAuthRetryState = AUTH_RETRY_STATE_IDLE;

  if (mCallback != NULL) {
    mCallback->onDescriptorWrite(this, descriptor, status);
  }
}

void GattClient::onNotify(string address, int handle, uint8_t *value)
{
  ALOGD(LOGTAG " onNotify() - Device %s handle %d", address.c_str(), handle);

  if (!caseInsCompare(address, mDeviceAddress)) {
    ALOGE( LOGTAG "onNotify() address mis-match");
    return;
  }

  GattCharacteristic *characteristic = getCharacteristicById(mDeviceAddress, handle);
  if (characteristic == NULL) return;

  if (mCallback != NULL) {
    characteristic->setValue(value);
    mCallback->onCharacteristicChanged(this, characteristic);
  }
}

void GattClient::onReadRemoteRssi(string address, int rssi, int status)
{
  ALOGD(LOGTAG " onReadRemoteRssi() - Device %s rssi %d status %d",
                address.c_str(), rssi, status);

  if (!caseInsCompare(address, mDeviceAddress)) {
    ALOGE( LOGTAG "onReadRemoteRssi() address mis-match");
    return;
  }

  if (mCallback != NULL) {
    mCallback->onReadRemoteRssi(this, rssi, status);
  }
}

void GattClient::onConfigureMTU(string address, int mtu, int status)
{
  ALOGD(LOGTAG " onConfigureMTU() - Device %s mtu %d status %d",
                address.c_str(), mtu, status);
  if (!caseInsCompare(address, mDeviceAddress)) {
    ALOGE( LOGTAG "onConfigureMTU() address mis-match");
    return;
  }

  if (mCallback != NULL) {
    mCallback->onMtuChanged(this, mtu, status);
  }
}

void GattClient::onConnectionUpdated(string address, int interval, int latency,
                                    int timeout, int status)
{

  ALOGD(LOGTAG " onConnectionUpdated() - Device %s interval %d latency %d timeout %d status %d",
                address.c_str(),
                interval, latency, timeout, status);

  if (!caseInsCompare(address, mDeviceAddress)) {
    ALOGE( LOGTAG "onConnectionUpdated() address mis-match");
    return;
  }

  if (mCallback != NULL) {
      mCallback->onConnectionUpdated(this, interval, latency,
              timeout, status);
  }
}

GattClient::~GattClient()
{
  if ( mService != NULL) {
    mService = NULL;
  }

  if (mGattCallback != NULL) {
      mGattCallback = NULL;
  }

  if (mCallback != NULL) {
    mCallback = NULL;
  }

  for (GattService *svc : mServices)
    delete svc;

  mServices.clear();
}

GattClient::GattClient(GattLibService *gatt,string deviceAddress, int transport, bool opportunistic,
                              int phy)
{
  mService = gatt;
  mDeviceAddress = deviceAddress;
  mPhy = phy;
  mTransport = transport;
  mOpportunistic = opportunistic;
  mConnState = CONN_STATE_IDLE;
  mAuthRetryState = AUTH_RETRY_STATE_IDLE;
}

void GattClient::abortReliableWrite()
{
  ALOGD(LOGTAG " abortReliableWrite() device: %s ", mDeviceAddress.c_str());
  if (mService == nullptr || mClientIf == 0) return;

  try {
    mService->endReliableWrite(mClientIf, mDeviceAddress, false);
  } catch (std::exception& e) {
    ALOGE(LOGTAG " %s", e.what());
  }
}

bool GattClient::beginReliableWrite()
{
  ALOGD(LOGTAG " beginReliableWrite() ");
  if (mService == nullptr || mClientIf == 0) return false;
  try {
    mService->beginReliableWrite(mClientIf, mDeviceAddress);
  } catch (std::exception& e) {
    ALOGE(LOGTAG " %s", e.what());
    return false;
  }

  return true;
}

void GattClient::close()
{
  ALOGD(LOGTAG " close() ");

  unregisterApp();
  mConnState = CONN_STATE_CLOSED;
  mAuthRetryState = AUTH_RETRY_STATE_IDLE;
}

bool GattClient::connect(bool autoConnect, GattClientCallback& callback)
{
  ALOGD(LOGTAG " connect() - device: %s , auto: %d ", mDeviceAddress.c_str() , autoConnect);
  {
    std::lock_guard<std::mutex> myLock(mStateLock);

    if (mConnState != CONN_STATE_IDLE) {
      throw std::runtime_error(std::string("Connection State Not idle"));
    }
    mConnState = CONN_STATE_CONNECTING;
  }

  mAutoConnect = autoConnect;

  if (!registerApp(callback)) {
    {
      std::lock_guard<std::mutex> myLock(mStateLock);
      mConnState = CONN_STATE_IDLE;
    }
    ALOGE(LOGTAG " connect() - Failed to register callback");
    return false;
  }

  // The connection will continue in the onClientRegistered callback
  return true;
}

void GattClient::disconnect()
{
  ALOGD(LOGTAG " disconnect() - device: %s ", mDeviceAddress.c_str());
  if (mService == nullptr || mClientIf == 0) return;

  try {
    mService->clientDisconnect(mClientIf, mDeviceAddress);
  } catch (std::exception& e) {
    ALOGE(LOGTAG " %s", e.what());
  }
}

bool GattClient::discoverServiceByUuid(Uuid uuid)
{
  ALOGD(LOGTAG " (discoverServiceByUuid) - device: %s", mDeviceAddress.c_str());
  if (mService == nullptr || mClientIf == 0) return false;

  mServices.clear();

  try {
    mService->discoverServiceByUuid(mClientIf, mDeviceAddress,uuid);
  } catch (std::exception& e) {
    ALOGE(LOGTAG " %s", e.what());
    return false;
  }
  return true;
}

bool GattClient::discoverServices()
{
  ALOGD(LOGTAG " discoverServices() - device: %s", mDeviceAddress.c_str());
  if (mService == nullptr || mClientIf == 0) return false;

  mServices.clear();

  try {
    mService->discoverServices(mClientIf, mDeviceAddress);
  } catch (std::exception& e) {
    ALOGE(LOGTAG " %s", e.what());
    return false;
  }

  return true;
}

bool GattClient::executeReliableWrite()
{
  ALOGD(LOGTAG " executeReliableWrite() - device: %s", mDeviceAddress.c_str());
  if (mService == nullptr || mClientIf == 0) return false;

  {
    std::lock_guard<std::mutex> myLock(mDeviceBusyLock);
    if (mDeviceBusy) return false;
    mDeviceBusy = true;
  }


  try {
    mService->endReliableWrite(mClientIf, mDeviceAddress, true);
  } catch (std::exception& e) {
    ALOGE(LOGTAG " %s", e.what());
    {
      std::lock_guard<std::mutex> myLock(mDeviceBusyLock);
      mDeviceBusy = false;
    }
    return false;
  }

  return true;
}

GattCharacteristic* GattClient::getCharacteristicById(string deviceAddress, int instanceId)
{
  for (GattService *svc : mServices) {
    for (GattCharacteristic *charac : svc->getCharacteristics()) {
      if (charac->getInstanceId() == instanceId) {
        return charac;
      }
    }
  }
  return NULL;
}

GattDescriptor* GattClient::getDescriptorById(string deviceAddress, int instanceId)
{
  for (GattService *svc : mServices) {
    for (GattCharacteristic *charac : svc->getCharacteristics()) {
      for (GattDescriptor *desc : charac->getDescriptors()) {
        if (desc->getInstanceId() == instanceId) {
            return desc;
        }
      }
    }
  }
  return NULL;
}

string GattClient::getDeviceAddress(){
  return mDeviceAddress;
}

GattService* GattClient::getService(string deviceAddress, Uuid uuid, int instanceId)
{
  for (GattService *service : mServices) {
    if (service->getDevice() ==  deviceAddress && service->getUuid()== uuid) {
        return service;
    }
  }

  return NULL;
}

std::list<GattService*> GattClient::getServices()
{
  std::list<GattService*> result;

  for (GattService *service : mServices) {
    if (service->getDevice()== mDeviceAddress)
    {
      result.push_back(service);
    }
  }

  return result;
}

GattService* GattClient::getService(Uuid uuid)
{
  for (GattService *service : mServices) {
    if (service->getDevice() == mDeviceAddress && service->getUuid() == uuid) {
      return service;
    }
  }

  return NULL;
}

bool GattClient::readCharacteristic(GattCharacteristic &characteristic)
{
  if ((characteristic.getProperties() & GattCharacteristic::PROPERTY_READ) == 0) {
    return false;
  }

  ALOGD(LOGTAG " readCharacteristic() - uuid: %s ", characteristic.getUuid().ToString().c_str());
  if (mService == NULL || mClientIf == 0) return false;

  GattService *service = characteristic.getService();
  if (service == NULL) return false;

  string mDevice = service->getDevice();
  if (mDevice.empty()) return false;
  service = NULL;

  {
    std::lock_guard<std::mutex> myLock(mDeviceBusyLock);
    if (mDeviceBusy) return false;
    mDeviceBusy = true;
  }

  try {
    mService->readCharacteristic(mClientIf, mDevice,
              characteristic.getInstanceId(), AUTHENTICATION_NONE);
  } catch (std::exception& e) {
    ALOGE(LOGTAG " %s", e.what());
    {
      std::lock_guard<std::mutex> myLock(mDeviceBusyLock);
      mDeviceBusy = false;
    }
    return false;
  }

  return true;
}

bool GattClient::readDescriptor(GattDescriptor &descriptor)
{
  ALOGD(LOGTAG " readDescriptor() - uuid: %s", descriptor.getUuid().ToString().c_str());
  if (mService == NULL || mClientIf == 0) return false;

  GattCharacteristic *characteristic = descriptor.getCharacteristic();
  if (characteristic == NULL) return false;

  GattService *service = characteristic->getService();
  if (service == NULL) return false;

  string mDevice = service->getDevice();
  if(mDevice.empty())  return false;
  service = NULL;

  {
    std::lock_guard<std::mutex> myLock(mDeviceBusyLock);
    if (mDeviceBusy) return false;
    mDeviceBusy = true;
  }
  try {
    mService->readDescriptor(mClientIf, mDevice,
            descriptor.getInstanceId(), AUTHENTICATION_NONE);
  } catch (std::exception& e) {
    ALOGE(LOGTAG " %s", e.what());
    {
      std::lock_guard<std::mutex> myLock(mDeviceBusyLock);
      mDeviceBusy = false;
    }
    return false;
  }

  return true;
}

void GattClient::readPhy()
{
  if (mService == NULL || mClientIf == 0) return;
  try {
    mService->clientReadPhy(mClientIf, mDeviceAddress);
  } catch (std::exception& e) {
    ALOGE(LOGTAG " %s", e.what());
  }
}

bool GattClient::readRemoteRssi()
{
  ALOGD(LOGTAG " readRemoteRssi() - device: %s", mDeviceAddress.c_str());
  if (mService == nullptr || mClientIf == 0) return false;

  try {
    mService->readRemoteRssi(mClientIf, mDeviceAddress);
  } catch (std::exception& e) {
    ALOGE(LOGTAG " %s", e.what());
    return false;
  }

  return true;
}

bool GattClient::readUsingCharacteristicUuid(Uuid uuid, int startHandle, int endHandle)
{
  ALOGD(LOGTAG " readUsingCharacteristicUuid() - uuid: %s ", uuid.ToString().c_str());
  if (mService == nullptr || mClientIf == 0) return false;

  {
    std::lock_guard<std::mutex> myLock(mDeviceBusyLock);
    if (mDeviceBusy) return false;
    mDeviceBusy = true;
  }

  try {
    mService->readUsingCharacteristicUuid(mClientIf, mDeviceAddress,
              uuid, startHandle, endHandle, AUTHENTICATION_NONE);
  } catch (std::exception& e) {
    ALOGE(LOGTAG " %s", e.what());
  {
    std::lock_guard<std::mutex> myLock(mDeviceBusyLock);
    mDeviceBusy = false;
  }
    return false;
  }

  return true;
}

bool GattClient::refresh()
{
  ALOGD(LOGTAG " refresh() - device: %s", mDeviceAddress.c_str());
  if (mService == nullptr || mClientIf == 0) return false;

  try {
    mService->refreshDevice(mClientIf, mDeviceAddress);
  } catch (std::exception& e) {
    ALOGE(LOGTAG " %s", e.what());
    return false;
  }

  return true;
}

bool GattClient::registerApp(GattClientCallback& callback)
{
  ALOGD(LOGTAG " registerApp()");
  if (mService == NULL) return false;

  mCallback = &callback;
  mGattCallback = this;
  Uuid uuid = Uuid::GetRandom();
  ALOGD(LOGTAG " registerApp() - UUID= %s", uuid.ToString().c_str());

  try {
    mService->registerClient(uuid, *mGattCallback);
  } catch (std::exception& e) {
    ALOGE(LOGTAG " %s", e.what());
    return false;
  }

  return true;
}

bool GattClient::requestConnectionPriority(int connectionPriority)
{
  if (connectionPriority < CONNECTION_PRIORITY_BALANCED
          || connectionPriority > CONNECTION_PRIORITY_LOW_POWER) {
      throw std::invalid_argument("connectionPriority not within valid range");
  }

  ALOGD(LOGTAG " requestConnectionPriority() - params: %d", connectionPriority);
  if (mService == nullptr || mClientIf == 0) return false;

  try {
    mService->connectionParameterUpdate(mClientIf, mDeviceAddress, connectionPriority);
  } catch (std::exception& e) {
    ALOGE(LOGTAG " %s", e.what());
    return false;
  }

  return true;
}

bool GattClient::requestMtu(int mtu)
{
  ALOGD(LOGTAG " requestMtu() - device: %s mtu: %d", mDeviceAddress.c_str(), mtu);

  if (mService == nullptr || mClientIf == 0) return false;

  try {
    mService->configureMTU(mClientIf, mDeviceAddress, mtu);
  } catch (std::exception& e) {
    ALOGE(LOGTAG " %s", e.what());
    return false;
  }

  return true;
}

bool GattClient::setCharacteristicNotification(GattCharacteristic &characteristic,
                                    bool enable)
{
  ALOGD(LOGTAG " setCharacteristicNotification() - uuid: %s enable: %d ",
            characteristic.getUuid().ToString().c_str(), enable);

  if (mService == nullptr || mClientIf == 0) return false;

  GattService *service = characteristic.getService();
  if (service == nullptr) return false;

  string mDevice = service->getDevice();
  if (mDevice.empty()) return false;
  service = NULL;

  try {
      mService->registerForNotification(mClientIf, mDeviceAddress,
              characteristic.getInstanceId(), enable);
  } catch (std::exception& e) {
    ALOGE(LOGTAG " %s", e.what());
    return false;
  }

  return true;
}

void GattClient::setPreferredPhy(int txPhy, int rxPhy, int phyOptions)
{
  if (mService == nullptr || mClientIf == 0) return;
  try {
    mService->clientSetPreferredPhy(mClientIf, mDeviceAddress, txPhy, rxPhy, phyOptions);
  } catch (std::exception& e) {
    ALOGE(LOGTAG " %s", e.what());
  }
}

void GattClient::unregisterApp()
{
  ALOGD(LOGTAG " unregisterApp() - mClientIf= %d", mClientIf);
  if (mService == nullptr || mClientIf == 0) return;

  try {
    mCallback = NULL;
    mService->unregisterClient(mClientIf);
    mClientIf = 0;
  } catch (std::exception& e) {
    ALOGE(LOGTAG " %s", e.what());
  }
}

bool GattClient::writeCharacteristic(GattCharacteristic &characteristic)
{
  if ((characteristic.getProperties() & GattCharacteristic::PROPERTY_WRITE) == 0
       && (characteristic.getProperties() & GattCharacteristic::PROPERTY_WRITE_NO_RESPONSE) == 0) {
    return false;
  }

  ALOGD(LOGTAG " writeCharacteristic() - uuid: %s ",
                               characteristic.getUuid().ToString().c_str());
  if (mService == nullptr || mClientIf == 0 || characteristic.getValue() == nullptr) return false;

  GattService *service = characteristic.getService();
  if (service == nullptr) return false;

  string mDevice = service->getDevice();
  if (mDevice.empty()) return false;
  service = NULL;

  {
    std::lock_guard<std::mutex> myLock(mDeviceBusyLock);
    if (mDeviceBusy) return false;
    mDeviceBusy = true;
  }

  try {
    mService->writeCharacteristic(mClientIf, mDevice,
            characteristic.getInstanceId(), characteristic.getWriteType(),
            AUTHENTICATION_NONE, characteristic.getValue());
  } catch (std::exception& e) {
    ALOGE(LOGTAG " %s", e.what());
  {
    std::lock_guard<std::mutex> myLock(mDeviceBusyLock);
    mDeviceBusy = false;
  }
    return false;
  }

  return true;
}

bool GattClient::writeDescriptor(GattDescriptor &descriptor)
{
  ALOGD(LOGTAG " writeDescriptor() - uuid: %s ", descriptor.getUuid().ToString().c_str());
  if (mService == nullptr || mClientIf == 0 || descriptor.getValue() == nullptr) {
    return false;
  }

  GattCharacteristic *characteristic = descriptor.getCharacteristic();
  if (characteristic == nullptr) {
    return false;
  }

  GattService *service = characteristic->getService();
  if (service == nullptr) {
    return false;
  }

  string mDevice = service->getDevice();
  if (mDevice.empty()) {
    return false;
  }
  service = NULL;

  {
    std::lock_guard<std::mutex> myLock(mDeviceBusyLock);
    if (mDeviceBusy) {
      return false;
    }
  }

  try {
    mService->writeDescriptor(mClientIf, mDevice, descriptor.getInstanceId(),
            AUTHENTICATION_NONE, descriptor.getValue());
  } catch (std::exception& e) {
    ALOGE(LOGTAG " %s", e.what());
  {
    std::lock_guard<std::mutex> myLock(mDeviceBusyLock);
    mDeviceBusy = false;
  }
    return false;
  }

  return true;
}
}//namespace gatt
