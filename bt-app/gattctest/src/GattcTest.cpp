/*
 * Copyright (c) 2017-2018, The Linux Foundation. All rights reserved.
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

#include "GattcTest.hpp"
#include "GattClient.hpp"
#include "GattClientCallback.hpp"
#include "uuid.h"
#include "ScanSettings.hpp"
#include "ScanCallback.hpp"
#include "GattLeScanner.hpp"
#include <GattDescriptor.hpp>

#include "utils.h"
#include <stdlib.h>
#include <ctime>
#include <thread>
#include <chrono>
#include <algorithm>


using namespace gatt;
using namespace btapp;

#define LOGTAG "GATTCTEST "
#define UNUSED

#define COPYMAXLEN 200

#define BR_EDR_TRANSPORT 1
#define BLE_TRANSPORT 2

#define MAX_BD_NAME 20

#define NO_AUTO_CONNECTION 0
#define AUTO_CONNECTION 1

#define UUID_FOR_CHARACTERISTIC_WRITE "ffffeeee-0000-1000-8000-00805f9b34fb"

#define PREPARE_WRITE_DATA 0xAA
#define PREPARE_WRITE_NEXT_DATA 0xBB

string WRITE_VALUE_BYTES_FOR_RELIABLE_WRITE = "Testing";
string WRITE_VALUE_BAD_RESP = "BAD_RESP_TEST";

#define BATCH_SCAN_REPORT_DELAY_MILLIS 10000
#define SCAN_DURATION_MILLIS    10000

#define CHARACTERISTIC_NEED_ENCRYPTED_READ_UUID 0
#define dbg 1
#define INDICATE_CHARACTERISTIC_UUID 1
#define START_HANDLE 1
#define END_HANDLE 0xFFFF

GattcTest *gattctest = NULL;
extern GattLibService *g_gatt;

mRemoteDev mDeviceMap("", NULL);

GattLeScanner* mScanner = NULL;
ScanSettings *setting = NULL;

enum ReliableWriteState
{
  RELIABLE_WRITE_NONE,
  RELIABLE_WRITE_WRITE_1ST_DATA,
  RELIABLE_WRITE_WRITE_2ND_DATA,
  RELIABLE_WRITE_EXECUTE,
  RELIABLE_WRITE_BAD_RESP
};
ReliableWriteState mExecReliableWrite;

class gattctestClientCallback:public GattClientCallback
{
  public:
    void onConnectionStateChange(GattClient *gatt, int status, int newState)
    {
      ALOGD(LOGTAG "onConnectionStateChange: status= (%d) Connected:%d",
          status, newState);
      if (status == GattClient::GATT_SUCCESS) {
        if (newState == GattDevice::STATE_CONNECTED) {
          ALOGE("OnConnectionStateChange device Connected to Rem Dev:%s",
              gatt->getDeviceAddress().c_str());
          fprintf(stdout, "OnConnectionStateChange device Connected to Rem Dev:%s\n",
              gatt->getDeviceAddress().c_str());
          mDeviceMap.add(gatt->getDeviceAddress(), gatt);
          gatt->discoverServices();
        }
        if (newState == GattDevice::STATE_DISCONNECTED) {
          mDeviceMap.remove(gatt->getDeviceAddress());
          ALOGE("OnConnectionStateChange device disconnected "
              "From Rem Dev:%s", gatt->getDeviceAddress().c_str());
        fprintf(stdout, "OnConnectionStateChange device disconnected to Rem Dev:%s\n",
              gatt->getDeviceAddress().c_str());
          gatt->close();
        }
      } else {
        mDeviceMap.remove(gatt->getDeviceAddress());
        gatt->close();
      }
    }

    void onPhyUpdate (GattClient *gatt, int txPhy, int rxPhy, int status)
    {
      ALOGD(LOGTAG "onPhyUpdate status %d", status);

      if (status == GattClient::GATT_SUCCESS) {
        ALOGD(LOGTAG "Txphy is %d and RxPhy is %d", txPhy, rxPhy);
        fprintf(stdout, "Txphy is %d and RxPhy is %d\n", txPhy, rxPhy);
      } else {
        ALOGE(LOGTAG "onPhyUpdate Failed %d", status);
        fprintf(stdout, "onPhyUpdate Failed %d\n", status);
      }
    }

    void onPhyRead (GattClient *gatt, int txPhy, int rxPhy, int status)
    {
      ALOGD(LOGTAG "onPhyRead status (%d)", status);
      if (status == GattClient::GATT_SUCCESS) {
        ALOGD(LOGTAG "Txphy is %d and RxPhy is %d", txPhy, rxPhy);
        fprintf(stdout, "Txphy is %d and RxPhy is %d\n", txPhy, rxPhy);
      } else {
        ALOGE(LOGTAG "onPhyRead Failed %d", status);
        fprintf(stdout, "onPhyRead Failed %d\n", status);
      }
    }

    void onServicesDiscovered (GattClient *gatt, int status)
    {
      ALOGD(LOGTAG " onServiceDiscovered status : %d", status);

      if ((status == GattClient::GATT_SUCCESS)) {
        std::list<GattService*> list_services = gatt->getServices();
        if (list_services.size() > 0) {
          ALOGD(LOGTAG "The no of services: %d", list_services.size());
          for (auto it = list_services.begin();
              it != list_services.end(); it++) {
            fprintf(stdout, "===============================\n");
            fprintf(stdout, "===The service type is %d InstaceId %d\n",
                (*it)->getType(), (*it)->getInstanceId());
            Uuid tmp_uuid = (*it)->getUuid();
            fprintf(stdout, "== SERVICE uuid is %s\n",
                tmp_uuid.ToString().c_str());

            std::vector<GattCharacteristic*> tmp_char
              = (*it)->getCharacteristics();
            ALOGD(LOGTAG "The no of Characteristics for this service:"
                "%d", tmp_char.size());
            fprintf(stdout, "The no of Characteristics for this"
                "service: %ld\n", tmp_char.size());
            std::vector<GattCharacteristic*>::const_iterator tmp_it;
            for(tmp_it = tmp_char.begin(); tmp_it != tmp_char.end();
                tmp_it++) {
              Uuid char_uuid = (*tmp_it)->getUuid();
              fprintf(stdout, "== ==CHAR uuid is %s   "
                  "InstanceID %d\n", char_uuid.ToString().c_str(),
                  (*tmp_it)->getInstanceId());
              fprintf(stdout, "== ==Properties %d ; "
                  "permissions %d; writeType %d\n",
                  (*tmp_it)->getProperties(),
                  (*tmp_it)->getPermissions(),
                  (*tmp_it)->getWriteType());
              std::vector<GattDescriptor*> tmp_desc
                = (*tmp_it)->getDescriptors();
              ALOGD(LOGTAG "The no of descriptors for this"
                  "char: %d", tmp_desc.size());
              fprintf(stdout, "The no of descriptors for this"
                  "char %ld\n", tmp_desc.size());
            }
          }
        } else {
          ALOGE(LOGTAG "No Services Found");
          fprintf(stdout, "No Services Found\n");
        }
      } else {
        ALOGE(LOGTAG "onServiceDiscovered failed status %d", status);
        fprintf(stdout, "onServiceDiscovered failed status %d\n", status);
      }
    }

    void onCharacteristicRead (GattClient *gattc,
        GattCharacteristic *characteristic, int status)
    {
      ALOGD(LOGTAG "onCharacteristicRead");
      Uuid uid = characteristic->getUuid();

      if (status == GattClient::GATT_SUCCESS) {
        uint8_t *value = characteristic->getValue();
        ALOGD(LOGTAG "onCharacteristicRead UUID %s, value is %s",
            characteristic->getUuid().ToString().c_str(), value);
        fprintf(stdout,"onCharacteristicRead UUID %s, value is %s\n",
            characteristic->getUuid().ToString().c_str(), value);
      } else if (status == GattClient::GATT_READ_NOT_PERMITTED) {
        ALOGE(LOGTAG "onCharacteristicRead error");
        fprintf(stdout, "onCharacteristicRead"
            "GATT_READ_NOT_PERMITTED\n");
      } else if(status == GattClient::GATT_INSUFFICIENT_AUTHENTICATION) {
        ALOGE(LOGTAG "Not Authentication Read");
        fprintf(stdout, "onCharacteristicRead "
            "GATT_INSUFFICIENT_AUTHENTICATION\n");
      } else {
        ALOGE(LOGTAG "Failed to read characteristic: ");
        fprintf(stdout, "Failed to read characteristic\n");
      }
    }

    void onCharacteristicWrite (GattClient *gattc,
        GattCharacteristic *characteristic, int status)
    {
      ALOGD(LOGTAG "onCharacteristicWrite: characteristic.val %d", status);

      uint8_t *value = characteristic->getValue();
      Uuid uid = characteristic->getUuid();

      /* MTU change notification */
      if (status == GattClient::GATT_SUCCESS) {
        ALOGE(LOGTAG "write characteristic uid %s, value:%s success",
            uid.ToString().c_str(), value);
        fprintf(stdout, "write characteristic uid %s, value:%s"
            "==success\n", uid.ToString().c_str(), value);
      } else {
        ALOGE(LOGTAG "Failed to write characteristic: %d", status);
        fprintf(stdout,"Failed to write characteristic: %d\n", status);
      }
      switch (mExecReliableWrite) {
        case ReliableWriteState::RELIABLE_WRITE_NONE:
        {
          if (status == GattClient::GATT_SUCCESS) {
            ALOGE(LOGTAG "write characteristic: %d success", status);
            fprintf(stdout, "write characteristic: %d success\n",
                  status);
          } else if (status == GattClient::GATT_WRITE_NOT_PERMITTED) {
            ALOGE(LOGTAG "Not Permission Write: %d", status);
            fprintf(stdout, "write characteristic: %d"
                  "GATT_WRITE_NOT_PERMITTED\n", status);
          } else if (status ==
              GattClient::GATT_INSUFFICIENT_AUTHENTICATION) {
            fprintf(stdout, "write characteristic: %d "
                  "GATT_INSUFFICIENT_AUTHENTICATION\n", status);
            ALOGE(LOGTAG "Not Authentication Write: %d", status);
          } else {
            ALOGE(LOGTAG "Failed to write characteristic: %d", status);
            fprintf(stdout, "Failed to write characteristic: %d\n",
                  status);
          }
          break;
        }
        case ReliableWriteState::RELIABLE_WRITE_WRITE_1ST_DATA:
        {
          mExecReliableWrite =
            ReliableWriteState::RELIABLE_WRITE_WRITE_2ND_DATA;
          string str = UUID_FOR_CHARACTERISTIC_WRITE;
          if (uid.ToString().compare(str) == 0) {
            ALOGD(LOGTAG "Sending prepare write after 1st prepare "
              "write successfully finished");
            fprintf(stdout, "Sending prepare write after 1st prepare "
              "write successfully finished\n");
            uint8_t tmp_ch[11] = {0};
            int i;
            for (i = 0; i < 10; i++)
              tmp_ch[i] = PREPARE_WRITE_NEXT_DATA;
            std::string s;
            s.assign(tmp_ch, tmp_ch + sizeof(tmp_ch));
            fprintf(stdout, "string write is %s\n", s.c_str());
            characteristic->setValue(tmp_ch, sizeof(tmp_ch));
            int status = gattc->writeCharacteristic(*characteristic);
            if (status) {
              fprintf(stdout, "write success\n");
            } else {
              fprintf(stdout, "write failed \n");
            }
          }
          break;
        }
        case ReliableWriteState::RELIABLE_WRITE_WRITE_2ND_DATA:
        {
          mExecReliableWrite =
            ReliableWriteState::RELIABLE_WRITE_EXECUTE;
          if (!gattc->executeReliableWrite()) {
            ALOGE(LOGTAG "reliable write failed");
            fprintf(stdout, "executeReliableWrite failed \n");
          } else {
            ALOGD(LOGTAG "Execute write succeeded %d\n", status);
            fprintf(stdout, "Execute write succeeded %d \n", status);
          }
          break;
        }
        case ReliableWriteState::RELIABLE_WRITE_EXECUTE:
        {
          mExecReliableWrite =
            ReliableWriteState::RELIABLE_WRITE_NONE;
          ALOGD(LOGTAG "After Executed write succeeded %d\n", status);
          fprintf(stdout, "After Execute write succeeded %d \n", status);
          break;
        }
        case ReliableWriteState::RELIABLE_WRITE_BAD_RESP:
        {
          mExecReliableWrite =
            ReliableWriteState::RELIABLE_WRITE_NONE;
          /* verify response
           * Server sends empty response for this test.
           * Response must be empty.
           * finish reliable write */
          gattc->abortReliableWrite();
          fprintf(stdout, "Notified the user. Reliable write aborted\n");
          break;
        }
        default:
          break;
      }
    }

    void onCharacteristicChanged(GattClient *gattc,
        GattCharacteristic *characteristic)
    {
      ALOGD(LOGTAG "onCharacteristicChanged: uid");
      Uuid uid = characteristic->getUuid();

      if (!uid.IsEmpty()) {
        ALOGD(LOGTAG "onCharacteristicChanged Equal");
        ALOGD(LOGTAG "onCharacteristicChanged intimation");
      }
    }

    void onDescriptorRead(GattClient *gatt, GattDescriptor *descriptor,
        int status)
    {
      ALOGD(LOGTAG "(%s)", __FUNCTION__);
      Uuid uid = descriptor->getUuid();
      if (status == GattClient::GATT_SUCCESS) {
        if (uid.IsEmpty()) {
          ALOGE(LOGTAG "(%s) UUID is EMPTY\n", __FUNCTION__);
          fprintf(stdout, " descriptor UUID is EMPTY\n");
        }
        uint8_t *des = descriptor->getValue();
        if(des != NULL) {
          ALOGD(LOGTAG "(%s) DESCRIPTOR VALUE is %s", __FUNCTION__, des);
          fprintf(stdout, " DESCRIPTOR VALUE is %s\n", des);
        }
      } else if (status == GattClient::GATT_READ_NOT_PERMITTED) {
        ALOGE(LOGTAG "(%s) UUID READ NOT PERMITTED\n", __FUNCTION__);
        fprintf(stdout, " DESCRIPTOR VALUE is GATT_READ_NOT_PERMITTED\n");
      } else {
        ALOGE(LOGTAG "(%s)  Failed to read descriptor", __FUNCTION__);
        fprintf(stdout, " Failed to read descriptor\n");
      }
    }

    void onDescriptorWrite (GattClient *gatt, GattDescriptor *descriptor,
        int status)
    {
      ALOGD(LOGTAG "onDescriptorWrite Completed %d", status);
      fprintf(stdout, "onDescriptorWrite Completed %d\n", status);
      Uuid uid = descriptor->getUuid();

      if ((status == GattClient::GATT_SUCCESS)) {
        ALOGD(LOGTAG "onDescriptorWrite Success Value : %s",
            descriptor->getValue());
        fprintf(stdout, "onDescriptorWrite Success Value : %s\n",
            descriptor->getValue());
      } else if (status == GattClient::GATT_WRITE_NOT_PERMITTED) {
        ALOGE(LOGTAG, "Write NOT PERMITTED for the descriptor");
        fprintf(stdout, "Write NOT PERMITTED for the descriptor\n");
      } else {
        ALOGE(LOGTAG "onDescriptorWrite FAILED %d", status);
        fprintf(stdout, "onDescriptorWrite FAILED %d\n", status);
      }
    }

    void onReliableWriteCompleted (GattClient *gatt, int status)
    {
      ALOGD(LOGTAG "onReliableWrite %d", status);
      if (mExecReliableWrite !=
          ReliableWriteState::RELIABLE_WRITE_NONE) {
        if (status == GattClient::GATT_SUCCESS) {
          ALOGD(LOGTAG "Reliable write completed");
        } else {
          ALOGE(LOGTAG "Reliable write complete fail %d", status);
          fprintf(stdout, "Reliable write complete fail %d\n",
              status);
        }
        mExecReliableWrite =
          ReliableWriteState::RELIABLE_WRITE_NONE;
      }
    }

    void onReadRemoteRssi (GattClient *gatt, int rssi, int status)
    {
      ALOGD(LOGTAG "onReadRemoteRssi");

      if (status == GattClient::GATT_SUCCESS) {
        ALOGD(LOGTAG "onReadRemoteRssi RSSI: %d", rssi);
        fprintf(stdout, "onReadRemoteRssi RSSI: %d\n", rssi);
      } else {
        ALOGE(LOGTAG "Failed to read remote rssi");
        fprintf(stdout, "Failed to read remote rssi\n");
      }
    }

    void onMtuChanged (GattClient *gatt, int mtu, int status)
    {
      ALOGD(LOGTAG "onMtuChanged");

      if (status == GattClient::GATT_SUCCESS) {
        if (mtu >= 23 && mtu <= 512) {
          ALOGE(LOGTAG "onMtuChanged to (%d)", mtu);
          fprintf(stdout, "MTU changed %d\n", mtu);
        } else {
          ALOGE(LOGTAG "Invalid Mtu value (%d)", mtu);
          fprintf(stdout, "Invalid Mtu value %d\n", mtu);
        }
      } else {
        ALOGE(LOGTAG "Failed to request mtu: (%d)", status);
        fprintf(stdout, "Failed to request mtu: (%d)", status);
      }
    }

    void onConnectionUpdated (GattClient *gatt, int interval, int latency,
        int timeout, int status)
    {
      ALOGD(LOGTAG "onConnectionUpdated");

      if ((status == GattClient::GATT_SUCCESS)) {
        ALOGD(LOGTAG "onConnectionUpdated interval (%d), latency (%d),"
            "timeout (%d), status (%d)\n", interval, latency, timeout,
            status);
        fprintf(stdout, "onConnectionUpdated interval (%d), latency (%d),"
            "timeout (%d), status (%d)\n", interval, latency, timeout,
            status);
      } else {
        ALOGE(LOGTAG "Connection Update failed status %d\n", status);
        fprintf(stdout, "Connection Update failed status %d\n", status);
      }
    }
};

class mscancallback : public ScanCallback
{
  public:
    void onScanResult(int callbackType, ScanResult *result)
    {
      ScanRecord *sr = result->getScanRecord();
      std::vector <Uuid> uuids = sr->getServiceUuids();
      ALOGD(LOGTAG "The scanned device is %s",
          result->getDevice().c_str());
      fprintf(stdout, "The scanned device is %s\n",
          result->getDevice().c_str());
      fprintf(stdout, "The scanned device is %s\n",
          sr->getDeviceName().c_str());
    }

    void onBatchScanResults(std::vector<ScanResult*> batchResult)
    {
      ALOGD(LOGTAG "BatchScan results size %ld", batchResult.size());
      fprintf(stdout, "onBatchScanResults %ld\n", batchResult.size());
      // In case onBatchScanResults are called due to buffer full,
      //we want to collect all scan results.
      if (!batchResult.empty()) {
        std::vector <ScanResult*>::iterator it;
        for (it = batchResult.begin(); it != batchResult.end(); it++) {
          fprintf(stdout, "The scanned device is %s\n",
              (*it)->getDevice().c_str());
        }
        fprintf(stdout, " ******************* \n");
      }
    }

    void onScanFailed (int errorCode)
    {
      ALOGE(LOGTAG "Scan Failed due to error %d", errorCode);
      fprintf(stdout, "Scan Failed due to error %d\n", errorCode);
    }
};

gattctestClientCallback *gattCliCallback = NULL;
mscancallback *mscan_callback = NULL;

GattcTest::GattcTest(GattLibService* gatt)
{
  ALOGD(LOGTAG "gattctest instantiated ");
  libservice = gatt;
  mScanner = GattLeScanner::getGattLeScanner();
  mscan_callback = new mscancallback;
  gattCliCallback = new gattctestClientCallback;
  mExecReliableWrite = ReliableWriteState::RELIABLE_WRITE_NONE;
}

void GattcTest::enableGattctest()
{
  ALOGD(LOGTAG "Enabling Gattctest");

  gattctest->setting = NULL;
  //Setting default PHY to 1Mbps
  gattctest->phy = 1;

  //Setting isOppurtunistic disable by default
  gattctest->isOpportunistic = 0;

  //Setting NO_AUTO conection by default
  gattctest->isAuto = 0;

  gattctest->gattcli = NULL;
}

GattcTest::~GattcTest()
{
  if (gattctest != NULL) {
    gattctest->setting = NULL;
    settingMask = 0;
	if (gattctest->filters.size() > 0)
	  gattctest->filters.clear();
    mScanner->stopScan(mscan_callback);
    if (mscan_callback != NULL) {
      delete(mscan_callback);
    }
    if (gattCliCallback != NULL) {
      delete(gattCliCallback);
      gattCliCallback = NULL;
    }
    if (gattctest->gattcli != NULL) {
      gattctest->gattcli->close();
      delete(gattctest->gattcli);
      gattctest->gattcli = NULL;
    }
  }
  libservice = NULL;

  ALOGD(LOGTAG "(%s) GATTCTEST DeInitialized\n", __FUNCTION__);
}

bool GattcTest:: validateInput(string str)
{
  return find_if(str.begin(), str.end(),
    [](char ch){ return !isdigit(ch); })
    == str.end();
}

void GattcTest :: gattConnParams(bool automatic, int phy, bool isOpportunistic)
{
  ALOGD(LOGTAG "Setting gattConnParams");

  if ((automatic == 0) || (automatic == 1)) {
    gattctest->isAuto = automatic;
  } else {
    ALOGW(LOGTAG "Enter correct auto value (0/1)");
    fprintf(stdout, "Enter correct auto value (0/1)\n");
  }
  if ((phy > 0) && (phy <= 255)) {
    gattctest->phy = phy;
  } else {
    ALOGW(LOGTAG "Enter correct phy value (0-255)");
    fprintf(stdout, "Enter correct phy value (0-255)\n");
  }
  if ((isOpportunistic == 0) || (isOpportunistic == 1)) {
    gattctest->isOpportunistic = isOpportunistic;
  } else {
    ALOGW(LOGTAG "Enter correct isOppurtunistic value (0/1)");
    fprintf(stdout, "Enter correct isOppurtunistic value (0/1)\n");
  }
}

bool GattcTest :: gattConnect(string bdaddr, int transport)
{
  ALOGD(LOGTAG "GattConnect transport %d", transport);
  if (transport < 0 || transport > 2) {
    ALOGE(LOGTAG "Invalid transport");
    fprintf(stdout, "Invalid transport\n");
    return false;
  }
  gattctest->gattcli = new GattClient(g_gatt, bdaddr,
      transport, gattctest->isOpportunistic, gattctest->phy);
  bool status = gattctest->gattcli->connect(gattctest->isAuto,
      *gattCliCallback);

  if (status) {
    return true;
  } else {
    return false;
  }
}

void GattcTest :: gattDisconnect(string bdaddr)
{
  ALOGD(LOGTAG "Gatt Disconnect");

  if (!mDeviceMap.containsDevice(bdaddr)) {
    ALOGE(LOGTAG "Device not found on Map");
    fprintf(stdout, "Device not found on Map");
    return;
  }

  GattClient *CliDevice;
  CliDevice = mDeviceMap.getGatt(bdaddr);

  CliDevice->disconnect();
}

void GattcTest :: gattDiscoverServices(string bdaddr)
{
  ALOGD(LOGTAG "gattDiscoverServices");

  if (!mDeviceMap.containsDevice(bdaddr)) {
    ALOGE(LOGTAG "Device not found on Map");
    fprintf(stdout, "Device not found on Map");
    return;
  }
  GattClient *CliDevice = mDeviceMap.getGatt(bdaddr);

  CliDevice->discoverServices();
}

bool GattcTest :: gattDiscoverServicesByUuid(Uuid uuid,string bdaddr)
{
  ALOGD(LOGTAG "gattDiscoverServicesByUuid");

  if (!mDeviceMap.containsDevice(bdaddr)) {
    ALOGE(LOGTAG "Device not found on Map");
    fprintf(stdout, "Device not found on Map");
    return false;
  }
  GattClient *CliDevice = mDeviceMap.getGatt(bdaddr);

  bool status = CliDevice->discoverServiceByUuid(uuid);
  if (status) {
    ALOGD(LOGTAG "gattDiscoverServicesByUuid Initiated");
    fprintf(stdout, "gattDiscoverServicesByUuid Initiated\n");
    return true;
  } else {
    ALOGE(LOGTAG "gattDiscoverServicesByUuid Failed");
    fprintf(stdout, "gattDiscoverServicesByUuid Initiation failed\n");
    return false;
  }
}

bool GattcTest :: getService(string bdaddr, Uuid serviceUid,int instanceid)
{
  ALOGD(LOGTAG "getService");
  GattService *service;

  if (!mDeviceMap.containsDevice(bdaddr)) {
    ALOGE(LOGTAG "Device not found on Map");
    fprintf(stdout, "Device not found on Map");
    return false;
  }
  GattClient *CliDevice = mDeviceMap.getGatt(bdaddr);

  service = CliDevice->getService(bdaddr, serviceUid, instanceid);
  if (service == NULL) {
    ALOGE("Service not found");
    return false;
  }
  ALOGD(LOGTAG "===The service type is %d  Instance ID is %x\n",
      service->getType(), service->getInstanceId());
  fprintf(stdout, "===The service type is %d  Instance ID is %x\n",
      service->getType(), service->getInstanceId());
  Uuid tmp_uuid = service->getUuid();
  ALOGD(LOGTAG "== SERVICE uuid is %s\n", tmp_uuid.ToString().c_str());
  fprintf(stdout, "== SERVICE uuid is %s\n", tmp_uuid.ToString().c_str());

  return true;
}

void GattcTest :: getServices(string bdaddr)
{
  ALOGD(LOGTAG "getServices list");

  if (!mDeviceMap.containsDevice(bdaddr)) {
    ALOGE(LOGTAG "Device not found on Map");
    fprintf(stdout, "Device not found on Map");
    return;
  }

  GattClient *CliDevice = mDeviceMap.getGatt(bdaddr);

  std::list<GattService*> list_services;
  list_services = CliDevice->getServices();
  if (list_services.size() > 0) {
    ALOGD(LOGTAG "The no of services: %d", list_services.size());
    for (auto it = list_services.begin(); it != list_services.end();
        it++) {
      fprintf(stdout, "==================================="
          "==========\n");
      fprintf(stdout, "===The service type is %d  Instance ID is"
          "%d\n", (*it)->getType(), (*it)->getInstanceId());
      Uuid tmp_uuid = (*it)->getUuid();
      fprintf(stdout, "== SERVICE uuid is %s\n",
          tmp_uuid.ToString().c_str());
      std::vector<GattCharacteristic*> tmp_char
        = (*it)->getCharacteristics();
      ALOGD(LOGTAG "The no of Characteristics for this service: %d",
          tmp_char.size());
      fprintf(stdout, "The no of Characteristics for this service:"
          "%ld\n", tmp_char.size());
      std::vector<GattCharacteristic*>::const_iterator tmp_it;
      for(tmp_it = tmp_char.begin(); tmp_it != tmp_char.end();
          tmp_it++) {
        Uuid char_uuid = (*tmp_it)->getUuid();
        fprintf(stdout, "== == == CHAR uuid is %s InstanceId "
            ": %d\n", char_uuid.ToString().c_str(),
            (*tmp_it)->getInstanceId());
        fprintf(stdout, "== == == Properties %d ; permissions %d;"
            "writeType %d\n", (*tmp_it)->getProperties(),
            (*tmp_it)->getPermissions(),
            (*tmp_it)->getWriteType());
      }
    }
  }
}

bool GattcTest :: getCharacteristicById (string bdaddr, int instanceId)
{
  ALOGD(LOGTAG "getCharacteristicById");

  GattCharacteristic* characteristic = NULL;

  if (!mDeviceMap.containsDevice(bdaddr)) {
    ALOGE(LOGTAG "Device not found on Map");
    fprintf(stdout, "Device not found on Map");
    return false;
  }
  GattClient *CliDevice = mDeviceMap.getGatt(bdaddr);

  characteristic = CliDevice->getCharacteristicById(bdaddr, instanceId);

  if (characteristic == NULL) {
    ALOGE(LOGTAG "No characteristic with this %d instance id",
        instanceId);
    fprintf(stdout, "No characteristic with this %d instance id \n",
        instanceId);
    return false;
  }
  Uuid char_uuid = characteristic->getUuid();
  ALOGD(LOGTAG "== == == CHAR uuid is %s InstanceId : %d",
      char_uuid.ToString().c_str(), characteristic->getInstanceId());
  fprintf(stdout, "== == == CHAR uuid is %s InstanceId : %d\n",
      char_uuid.ToString().c_str(), characteristic->getInstanceId());
  ALOGD(LOGTAG "== == == Properties %d ; permissions %d ; writeType %d\n",
      characteristic->getProperties(), characteristic->getPermissions(),
      characteristic->getWriteType());
  fprintf(stdout, "== == == Properties %d ; permissions %d ; writeType %d\n",
      characteristic->getProperties(), characteristic->getPermissions(),
      characteristic->getWriteType());

  std::vector<GattDescriptor*> tmp_desc
    = characteristic->getDescriptors();
  ALOGD(LOGTAG "The no of descriptors for this char: %d",
      tmp_desc.size());
  fprintf(stdout, "The no of descriptors for this char: %ld\n",
      tmp_desc.size());
  std::vector<GattDescriptor*>::iterator desc_it;
  for (desc_it = tmp_desc.begin(); desc_it != tmp_desc.end(); desc_it++) {
    Uuid desc_uuid = (*desc_it)->getUuid();
    fprintf(stdout, "@@@@@@ desc uuid is %s InstanceId : %d\n",
        desc_uuid.ToString().c_str(), (*desc_it)->getInstanceId());
    fprintf(stdout, "******permissions %d ; value %s\n\n",
        (*desc_it)->getPermissions(), (*desc_it)->getValue());
  }

  return true;
}

bool GattcTest :: getDescriptorById(string bdaddr, int instanceId)
{
  ALOGD(LOGTAG "getDescriptorById");
  GattDescriptor* descriptor = NULL;

  if (!mDeviceMap.containsDevice(bdaddr)) {
    ALOGE(LOGTAG "Device not found on Map");
    fprintf(stdout, "Device not found on Map");
    return false;
  }
  GattClient *CliDevice = mDeviceMap.getGatt(bdaddr);

  descriptor = CliDevice->getDescriptorById(bdaddr, instanceId);
  if (descriptor == NULL) {
    ALOGE(LOGTAG "Descriptor not found with %d instanceid\n", instanceId);
    return false;
  }

  ALOGD(LOGTAG "@@@@@@ desc uuid is %s InstanceId : %d\n",
      descriptor->getUuid().ToString().c_str(),
      descriptor->getInstanceId());

  fprintf(stdout, "@@@@@@ desc uuid is %s InstanceId : %d\n",
      descriptor->getUuid().ToString().c_str(),
      descriptor->getInstanceId());

  return true;
}

GattCharacteristic* GattcTest :: getCharacteristic(Uuid uid, string bdaddr)
{
  ALOGD(LOGTAG "getCharacteristic");
  GattCharacteristic* characteristic = NULL;

  if (!mDeviceMap.containsDevice(bdaddr)) {
    ALOGE(LOGTAG "Device not found on Map");
    fprintf(stdout, "Device not found on Map");
    return NULL;
  }
  GattClient *CliDevice = mDeviceMap.getGatt(bdaddr);

  GattService *service = CliDevice->getService(uid);
  if (service != NULL) {
    characteristic = service->getCharacteristic(uid);
    if (characteristic == NULL) {
      ALOGE(LOGTAG "Characteristic not found");
    }
  }
  return characteristic;
}

bool GattcTest :: writeCharacteristic(string bdaddr, uint8_t *writeValue, int length,
    int instanceId)
{
  ALOGD(LOGTAG "writeCharacteristic value is %s", writeValue);
  fprintf(stdout, "writeCharacteristic value %s\n", writeValue);

  GattCharacteristic* characteristic = NULL;

  if (!mDeviceMap.containsDevice(bdaddr)) {
    ALOGE(LOGTAG "Device not found on Map");
    fprintf(stdout, "Device not found on Map");
    return NULL;
  }
  GattClient *CliDevice = mDeviceMap.getGatt(bdaddr);

  characteristic = CliDevice->getCharacteristicById(bdaddr, instanceId);

  if (characteristic != NULL) {
    characteristic->setValue(writeValue, length);
    ALOGD(LOGTAG "Instance ID   %d", characteristic->getInstanceId());

    bool status = CliDevice->writeCharacteristic(*characteristic);
    if (status) {
      ALOGD(LOGTAG "writeCharacteristic success");
      fprintf(stdout, "writeCharacteristic success\n");
    } else {
      ALOGE(LOGTAG "writecharacteristic failed");
      fprintf(stdout, "writeCharacteristic Failed\n");
    }
  } else {
    ALOGE(LOGTAG "No Characteristic. Please refresh services");
    fprintf(stdout, "No Characteristic. Please refresh services");
  }
  return true;
}

bool GattcTest:: reqConnPri(string bdaddr, int conn_priority)
{
  ALOGD(LOGTAG "requestConnPri");
  if (!mDeviceMap.containsDevice(bdaddr)) {
    ALOGE(LOGTAG "Device not found on Map");
    fprintf(stdout, "Device not found on Map");
    return false;
  }
  GattClient *CliDevice = mDeviceMap.getGatt(bdaddr);

  if (CliDevice->requestConnectionPriority(conn_priority)) {
    ALOGD(LOGTAG "Requested Connection priority");
    fprintf(stdout, "Requested Connection priority\n");
  } else {
    ALOGE(LOGTAG "Connection priority request failed");
    fprintf(stdout, "Connection priority request failed \n");
  }

  return true;
}

void GattcTest :: writeDescriptor(string bdaddr,
    uint8_t *writeValue,  int length, int instanceid)
{
  ALOGD(LOGTAG "writeDescriptor");

  if (!mDeviceMap.containsDevice(bdaddr)) {
    ALOGE(LOGTAG "Device not found on Map");
    fprintf(stdout, "Device not found on Map");
    return;
  }
  GattClient *CliDevice = mDeviceMap.getGatt(bdaddr);

  GattDescriptor* descriptor =
    CliDevice->getDescriptorById(bdaddr, instanceid);
  if (descriptor != NULL) {
    descriptor->setValue(writeValue, length);

    if (!CliDevice->writeDescriptor(*descriptor)) {
      ALOGE(LOGTAG "WriteDescriptor Failed");
      fprintf(stdout, "Write Descriptor Failed\n");
    }
  } else {
    ALOGE(LOGTAG "No descriptor found with that instanceId");
    fprintf(stdout, "No descriptor found with that instanceId\n");
  }
}

void GattcTest :: readCharacteristic(string bdaddr, int instanceid)
{
  ALOGD(LOGTAG "readCharacteristic");
  if (!mDeviceMap.containsDevice(bdaddr)) {
    ALOGE(LOGTAG "Device not found on Map");
    fprintf(stdout, "Device not found on Map");
    return;
  }
  GattClient *CliDevice = mDeviceMap.getGatt(bdaddr);

  GattCharacteristic* characteristic =
    CliDevice->getCharacteristicById(bdaddr, instanceid);
  if (characteristic != NULL) {
    bool status = CliDevice->readCharacteristic((*characteristic));
    if (status) {
      ALOGD(LOGTAG "readcharacteristic Initiated");
      fprintf(stdout, "readcharacteristic Initiated\n");
    } else {
      ALOGE(LOGTAG "readcharacteristic Failed");
      fprintf(stdout, "readcharacteristic Failed\n");
    }
  } else {
    ALOGE(LOGTAG "InstanceID Not found");
    fprintf(stdout, "InstanceID Not found\n");
  }
}

bool GattcTest :: readCharacteristicUUID(string bdaddr, Uuid uuid)
{
  ALOGD(LOGTAG "Reading characteristic using UUID");

  if (!mDeviceMap.containsDevice(bdaddr)) {
    ALOGE(LOGTAG "Device not found on Map");
    fprintf(stdout, "Device not found on Map");
    return false;
  }
  GattClient *CliDevice = mDeviceMap.getGatt(bdaddr);

  bool status = CliDevice->readUsingCharacteristicUuid(uuid, START_HANDLE,
      END_HANDLE);
  if (status) {
    ALOGD(LOGTAG "readcharacteristic Initiated using uuid");
    fprintf(stdout, "readcharacteristic Initiated using uuid\n");
  } else {
    ALOGE(LOGTAG "readcharacteristic failed using uuid");
    fprintf(stdout, "readcharacteristic failed using uuid \n");
  }
  return true;
}


void GattcTest :: readDescriptor(string bdaddr, int instanceid)
{
  ALOGD(LOGTAG "readDescriptor using instanceID");

  if (!mDeviceMap.containsDevice(bdaddr)) {
    ALOGE(LOGTAG "Device not found on Map");
    fprintf(stdout, "Device not found on Map");
    return;
  }
  GattClient *CliDevice = mDeviceMap.getGatt(bdaddr);

  GattDescriptor* descriptor =
    CliDevice->getDescriptorById(bdaddr,instanceid);
  if (descriptor != NULL) {
    bool status = CliDevice->readDescriptor((*descriptor));
    if (status) {
      ALOGD(LOGTAG "readDescriptor Initiated");
      fprintf(stdout, "readDescriptor Initiated\n");
    } else {
      ALOGE(LOGTAG "readDescriptor initiated failed");
      fprintf(stdout, "readDescriptor initiated failed\n");
    }
  } else {
    ALOGE(LOGTAG "descriptor not found using instanceId");
    fprintf(stdout, "descriptor not found using instanceId \n");
  }
}

void GattcTest :: gattClientReadPhy(string bdaddr)
{
  ALOGD(LOGTAG "gattClientReadPhy Setting");

  if (!mDeviceMap.containsDevice(bdaddr)) {
    ALOGE(LOGTAG "Device not found on Map");
    fprintf(stdout, "Device not found on Map");
    return;
  }
  GattClient *CliDevice = mDeviceMap.getGatt(bdaddr);

  CliDevice->readPhy();
  ALOGD(LOGTAG "ReadPhy Setting initiated");
  fprintf(stdout, "ReadPhy Setting initiated\n");
}

void GattcTest :: gattReadRemoteRssi(string bdaddr)
{
  ALOGD(LOGTAG "gattReadRemoteRssi Setting");

  if (!mDeviceMap.containsDevice(bdaddr)) {
    ALOGE(LOGTAG "Device not found on Map");
    fprintf(stdout, "Device not found on Map");
    return;
  }
  GattClient *CliDevice = mDeviceMap.getGatt(bdaddr);

  bool status = CliDevice->readRemoteRssi();
  if (status) {
    ALOGD(LOGTAG "gattReadRemoteRssi Initiated Success");
    fprintf(stdout, "gattReadRemoteRssi Initiated Success\n");
  } else {
    ALOGE(LOGTAG "gattReadRemoteRssi Failed");
    fprintf(stdout, "gattReadRemoteRssi initiation Failed\n");
  }
}

void GattcTest :: gattRefresh(string bdaddr)
{
  ALOGD(LOGTAG "gattRefresh Setting");

  if (!mDeviceMap.containsDevice(bdaddr)) {
    ALOGE(LOGTAG "Device not found on Map");
    fprintf(stdout, "Device not found on Map\n");
    return;
  }
  GattClient *CliDevice = mDeviceMap.getGatt(bdaddr);

  bool status = CliDevice->refresh();
  if (status) {
    ALOGD(LOGTAG "gattRefresh Success");
    fprintf(stdout, "gattRefresh Success\n");
  } else {
    ALOGE(LOGTAG "gattRefresh Failed");
    fprintf(stdout, "gattRefresh Failed\n");
  }
}

void GattcTest :: gattrequestMtu(string bdaddr, int mtu_value)
{
  ALOGD(LOGTAG "Requesting MTU");

  if (!mDeviceMap.containsDevice(bdaddr)) {
    ALOGE(LOGTAG "Device not found on Map");
    fprintf(stdout, "Device not found on Map\n");
    return;
  }
  GattClient *CliDevice = mDeviceMap.getGatt(bdaddr);

  if ((mtu_value >= 23) && (mtu_value <= 512)) {
    bool status = CliDevice->requestMtu(mtu_value);
    if (status) {
      ALOGD(LOGTAG "gattrequestMtu Success");
      fprintf(stdout, "gattrequestMtu Success\n");
    } else {
      ALOGE(LOGTAG "gattrequestMtu FAILED to request");
      fprintf(stdout, "gattrequestMtu FAILED to request\n");
    }
  } else {
    ALOGE(LOGTAG "Enter correct MTU value");
    fprintf(stdout, "Enter correct MTU value\n");
  }
}

  bool GattcTest :: readUsingCharacteristicUuid
(string bdaddr, Uuid uuid, int startHandle, int endHandle)
{
  ALOGD(LOGTAG "readUsingCharacteristicUuid");

  if (!mDeviceMap.containsDevice(bdaddr)) {
    ALOGE(LOGTAG "Device not found on Map");
    fprintf(stdout, "Device not found on Map\n");
    return false;
  }
  GattClient *CliDevice = mDeviceMap.getGatt(bdaddr);

  bool status = CliDevice->readUsingCharacteristicUuid
    (uuid, startHandle, endHandle);
  if (status) {
    ALOGD(LOGTAG "readcharacteristic Initiated using uuid");
    fprintf(stdout, "readcharacteristic Initiated using uuid\n");
  } else {
    ALOGE(LOGTAG "readcharacteristic failed using uuid");
    fprintf(stdout, "readcharacteristic failed using uuid \n");
  }
  return status;
}

bool GattcTest::prepareWriteCharacteristic(string bdaddr,
    uint8_t * writeValue, int length, int instanceId)
{
  ALOGD(LOGTAG "prepareWriteCharacteristic");
  fprintf(stdout, "prepareWriteCharacteristic\n");

  if (!mDeviceMap.containsDevice(bdaddr)) {
    ALOGE(LOGTAG "Device not found on Map");
    fprintf(stdout, "Device not found on Map\n");
    return false;
  }
  GattClient *CliDevice = mDeviceMap.getGatt(bdaddr);
  bool status;

  // Begin Reliable Write
  status = CliDevice->beginReliableWrite();
  if (status) {
    ALOGD(LOGTAG "beginReliableWrite initiated");
  } else {
    ALOGE(LOGTAG "beginReliableWrite Failed");
    fprintf(stdout, "beginReliableWrite Failed\n");
    return false;
  }

  GattCharacteristic* characteristic = NULL;
  characteristic = CliDevice->getCharacteristicById(bdaddr, instanceId);

  if (characteristic != NULL) {
    characteristic->setValue(writeValue, length);
    bool status = CliDevice->writeCharacteristic(*characteristic);

    if (status) {
      ALOGD(LOGTAG "preparewriteCharacteristic success");
      fprintf(stdout, "preparewriteCharacteristic success\n");
    } else {
      ALOGE(LOGTAG "preparewritecharacteristic failed");
      fprintf(stdout, "preparewriteCharacteristic Failed\n");
    }
  } else {
    ALOGE(LOGTAG "characteristic not found with that InstanceID");
    fprintf(stdout, "characteristic not found with that InstanceID\n");
  }
  return true;
}

/*
Function:reliableWrite
Purpose: It tests, prepare write, abort reliablewrites,
executewrites framework API's.
*/
bool GattcTest ::reliableWrite(string bdaddr, int instanceid)
{
  ALOGD(LOGTAG "Reliable Write");
  fprintf(stdout, "Reliable write\n");

  if (!mDeviceMap.containsDevice(bdaddr)) {
    ALOGE(LOGTAG "Device not found on Map");
    fprintf(stdout, "Device not found on Map\n");
    return false;
  }
  GattClient *CliDevice = mDeviceMap.getGatt(bdaddr);

  bool status;
  // Begin Reliable Write
  status = CliDevice->beginReliableWrite();
  if (status) {
    ALOGD(LOGTAG "beginReliableWrite initiated");
  } else {
    ALOGE(LOGTAG "beginReliableWrite Failed");
    fprintf(stdout, "beginReliableWrite Failed\n");
    return false;
  }
  //Abort after 2 seconds
  std::this_thread::sleep_for (std::chrono::seconds(2));
  CliDevice->abortReliableWrite();
  ALOGD(LOGTAG "Reliable write Aborted");
  fprintf(stdout, "Reliable write Aborted\n");

  // Again Writing the Preparewrites
  std::this_thread::sleep_for (std::chrono::seconds(1));
  status = CliDevice->beginReliableWrite();
  if (status) {
    ALOGD(LOGTAG "beginReliableWrite initiated");
  } else {
    ALOGE(LOGTAG "beginReliableWrite Failed");
    fprintf(stdout, "beginReliableWrite Failed\n");
    return false;
  }
  std::this_thread::sleep_for (std::chrono::seconds(1));

  GattCharacteristic *characteristic = CliDevice->getCharacteristicById
    (bdaddr, instanceid);
  if(characteristic == NULL) {
    mExecReliableWrite = ReliableWriteState::RELIABLE_WRITE_NONE;
    return false;
  }
  string str = UUID_FOR_CHARACTERISTIC_WRITE;
  if (characteristic->getUuid().ToString().compare(str) == 0) {
    fprintf(stdout, "checking reliable writes\n");
    /*
       Writing some default value to tmp buffer to test the
       prepare write and execute write scenario.
       */
    uint8_t tmp_ch[10];
    int i;
    for (i = 0; i < 10; i++)
      tmp_ch[i] = PREPARE_WRITE_DATA;

    characteristic->setValue(tmp_ch, sizeof(tmp_ch)/sizeof(tmp_ch[0]));

    if (mExecReliableWrite == ReliableWriteState::RELIABLE_WRITE_NONE) {
      mExecReliableWrite =
        ReliableWriteState::RELIABLE_WRITE_WRITE_1ST_DATA;
    } else {
      mExecReliableWrite =
        ReliableWriteState::RELIABLE_WRITE_BAD_RESP;
    }
    status = CliDevice->writeCharacteristic(*characteristic);
    fprintf(stdout, "write characteristic executedexecuted\n");
  } else {
    ALOGE(LOGTAG "Reliable write failed due to mismatch in UUID");
    fprintf(stdout, "Reliable write failed due to mismatch in UUID\n");
    return false;
  }
  return true;
}

void GattcTest :: setPreferredPhy(int txPhy, int rxPhy, int phyOptions,
    string bdaddr)
{
  ALOGD(LOGTAG "Setting preferredphy");

  if (!mDeviceMap.containsDevice(bdaddr)) {
    ALOGE(LOGTAG "Device not found on Map");
    fprintf(stdout, "Device not found on Map\n");
    return ;
  }
  if ((txPhy >= 0 && txPhy <= 255) &&
    (rxPhy >= 0 && rxPhy <= 255)) {
    GattClient *CliDevice = mDeviceMap.getGatt(bdaddr);
    CliDevice->setPreferredPhy(txPhy, rxPhy, phyOptions);
  } else {
    ALOGE(LOGTAG "Enter proper PHY values (0-255)");
    fprintf(stdout, "Enter proper PHY values (0-255)\n");
  }

  return;
}

void GattcTest :: list_conn_devices()
{
  list<string> conn_list;

  conn_list = mDeviceMap.getConnectedDevices();
  ALOGE(LOGTAG "No of Devices connected %ld", conn_list.size());
  fprintf(stdout,"No of Devices connected %ld\n", conn_list.size());
  for (auto i = conn_list.begin(); i != conn_list.end(); i++) {
    ALOGD(LOGTAG " %s ", (*i).c_str());
    fprintf(stdout, " %s \n", (*i).c_str());
  }
  fprintf(stdout,"=====================================\n");
}

mRemoteDev::mRemoteDev(string x, GattClient *gattconn)
{
  mapClient[x] = gattconn;
  mapClient.clear();
}

void mRemoteDev :: add(string dev, GattClient *gattConn)
{
  ALOGD(LOGTAG "Adding device to map");
  mDeviceMap.mapClient.insert(std::pair<string,
      class GattClient *>(dev, gattConn));
}

void mRemoteDev :: remove(string dev)
{
  ALOGD(LOGTAG "Remove device to map");
  if (!mDeviceMap.containsDevice(dev)) {
    ALOGD(LOGTAG "Device Not Found");
  } else {
    mDeviceMap.mapClient.erase(dev);
  }
}

list<string> mRemoteDev :: getConnectedDevices()
{
  ALOGD(LOGTAG "Remove device to map");
  list<string> dev_list;
  for (auto i = mDeviceMap.mapClient.begin();
      i != mDeviceMap.mapClient.end(); i++) {
    dev_list.push_back((*i).first);
  }
  return dev_list;
}

GattClient* mRemoteDev :: getGatt(string dev)
{
  ALOGD(LOGTAG "getting particular remote device");
  map<string, class GattClient *>::const_iterator it =
    mDeviceMap.mapClient.find(dev);
  return it->second;
}

list<GattClient *> mRemoteDev :: getGattList()
{
  ALOGD(LOGTAG "getting particular remote device");
  list<GattClient*> gattlist;
  return gattlist;
}

bool mRemoteDev :: containsDevice(string dev)
{
  if (mDeviceMap.mapClient.find(dev) ==
      mDeviceMap.mapClient.end()) {
    return false;
  }
  return true;
}

void mRemoteDev ::clear()
{
  mDeviceMap.mapClient.clear();
}

enum filterTypes
{
  NO_FILTER_SET=0,
  FILTER_BD_ADDR,
  FILTER_DEVICE_NAME,
  FILTER_SRVC_UUID,
  FILTER_SRVC_DATA
};

enum settingType
{
  NO_SCAN_SETTING,
  SCAN_MODE,
  CALLBACK_TYPE,
  SCANRESULT_TYPE,
  PHY_TYPE,
  SET_LEGACY,
  REPORT_DELAY_MILLS,
  MATCH_ADVS
};

enum settingCbValue
{
  ALL_TYPES,
  ON_LOST_ON_FOUND,
  SENSOR_TYPE
};

enum settingTypeMask {
  NO_SCAN_SETTING_MASK,
  SCAN_MODE_MASK,
  CALLBACK_TYPE_MASK,
  SCANRESULT_TYPE_MASK,
  PHY_TYPE_MASK,
  SET_LEGACY_MASK,
  REPORT_DELAY_MILLS_MASK,
  MATCH_ADVS_MASK,
  END_MASK
};

enum filterTypes mFilterTypes = NO_FILTER_SET;
enum settingType mscanSettings = NO_SCAN_SETTING;

int GattcTest::settingMask = 0;
int GattcTest::mScanMode = 0; //SCAN_MODE_LOW_POWER
int GattcTest::mCallbackType = 1; //CALLBACK_TYPE_ALL_MATCHES
int GattcTest::mScanResultType = 0; //SCAN_RESULT_TYPE_FULL
long GattcTest::mReportDelayMillis = 0;
int GattcTest::mMatchMode = 1; //MATCH_MODE_AGGRESSIVE
int GattcTest::mNumOfMatchesPerFilter = 3; //MATCH_NUM_MAX_ADVERTISEMENT
bool GattcTest::mLegacy = true;
int GattcTest::mPhy = 255; //PHY_LE_ALL_SUPPORTED

bool GattcTest :: scanFilter(int filterType, string value)
{
  fprintf(stdout, "FilterType %d\n", filterType);

  if (filterType != filterTypes::NO_FILTER_SET) {
    switch (filterType) {
      case filterTypes::FILTER_BD_ADDR:
      {
        fprintf(stdout, "FILTER_BD_ADDR\n");
        const char *tmpAddr = value.c_str();
        if (string_is_bdaddr(tmpAddr)) {
          gattctest->filter =
            ScanFilter::Builder().setDeviceAddress(value).build();
          gattctest->filters.push_back(gattctest->filter);
          } else {
            fprintf(stdout, "Enter the correct bdaddr\n");
            return false;
          }
          mFilterTypes = filterTypes::FILTER_BD_ADDR;
          break;
      }
      case filterTypes::FILTER_DEVICE_NAME:
      {
        fprintf(stdout, "FILTER_DEVICE_NAME\n");
        if ((value.length() > 0) && (value.length() < MAX_BD_NAME)) {
          gattctest->filter =
            ScanFilter::Builder().setDeviceName(value).build();
          gattctest->filters.push_back(gattctest->filter);
          fprintf(stdout, "Dev_Name is %s\n",
                      gattctest->filter->getDeviceName().c_str());
          } else {
            fprintf(stdout, "Enter valid name\n");
            return false;
          }
          mFilterTypes = filterTypes::FILTER_DEVICE_NAME;
          break;
      }
      case filterTypes::FILTER_SRVC_UUID:
      {
        fprintf(stdout, "FILTER_SRVC_UUID\n");
        Uuid uuid = uuid.FromString(value, NULL);
        std::vector<uint8_t> vUuid = Uuid::uuidToByte(uuid);

        int uuidLen = vUuid.size();
        if ((uuidLen == Uuid::kNumBytes16) ||
          (uuidLen == Uuid::kNumBytes32) ||
          (uuidLen == Uuid::kNumBytes128)) {
          gattctest->filter =
            ScanFilter::Builder().setServiceUuid(uuid).build();
          gattctest->filters.push_back(gattctest->filter);
          } else {
            ALOGE(LOGTAG "Enter the Valid UUID (16/32/128) bytes");
            fprintf(stdout, "Enter the Valid UUID (16/32/128) bytes");
          }
          break;
      }
      default:
        fprintf(stdout, "Enter correct filter type\n");
        return false;
    }
  } else {
    fprintf(stdout, "No filter set\n");
    mFilterTypes = filterTypes::NO_FILTER_SET;
    return true;
  }
  return true;
}

void GattcTest :: scanFilterManuData(int manuId, string manuData,
    string manuMask)
{
  if (manuId < 0) {
    ALOGE(LOGTAG "Enter valid Manufacturer ID");
    fprintf(stdout, "Enter valid Manufacturer ID\n");
    return;
  }
  if (manuData.empty()) {
    ALOGE(LOGTAG "Enter valid Manufacturer DATA");
    fprintf(stdout, "Enter valid Manufacturer DATA\n");
    return;
  }
  fprintf(stdout, "Manu Data %s\n", manuData.c_str());
  std::vector<uint8_t> vManuData(manuData.begin(), manuData.end());
  for (auto i = vManuData.begin(); i != vManuData.end(); ++i) {
    cout<<(*i);
  }
  cout<<endl;

  if (manuMask.length() <= 0) {
    gattctest->filter = ScanFilter::Builder()
      .setManufacturerData(manuId, vManuData).build();
    gattctest->filters.push_back(gattctest->filter);
    ALOGE(LOGTAG "Manufacture id %d",
        gattctest->filter->getManufacturerId());
    fprintf(stdout, "manuId is %d\n",
        gattctest->filter->getManufacturerId());
    vector<uint8_t> tmp = gattctest->filter->getManufacturerData();
    for (auto j = tmp.begin();
        j != tmp.end(); ++j) {
      std::cout<< (*j);
    }
    std::cout<<endl;
  } else {
    std::vector<uint8_t> vManuMask(manuMask.begin(), manuMask.end());
    gattctest->filter = ScanFilter::Builder()
      .setManufacturerData(manuId, vManuData,
          vManuMask)
      .build();
    gattctest->filters.push_back(gattctest->filter);
    ALOGE(LOGTAG "Manufacture id %d",
        gattctest->filter->getManufacturerId());
    fprintf(stdout, "manuId is %d\n",
        gattctest->filter->getManufacturerId());
  }
  return;
}

bool GattcTest :: scanSettings(int scanType, int value)
{
  fprintf(stdout, "scanSettings Type : %d\n", scanType);
  if (scanType != settingType::NO_SCAN_SETTING) {
    switch (scanType) {
      case settingType::SCAN_MODE:
      {
        fprintf(stdout, "SCAN_MODE value : %d\n", value);
        if ((value >= 0) && (value < 3)) {
            mScanMode = value;
            settingMask |= (1 << SCAN_MODE_MASK);
        } else {
          fprintf(stdout, "Enter the correct scan mode value\n");
          return false;
        }
        mscanSettings = settingType::SCAN_MODE;
        break;
      }
      case settingType::CALLBACK_TYPE:
      {
        fprintf(stdout, "CALLBACK_TYPE value : %d\n", value);

        switch (value) {
          case settingCbValue::ALL_TYPES:
          {
            mCallbackType = ScanSettings::CALLBACK_TYPE_ALL_MATCHES;
            settingMask |= (1 << CALLBACK_TYPE_MASK);
          }
          break;
          case settingCbValue::ON_LOST_ON_FOUND:
          {
            mCallbackType = (ScanSettings::CALLBACK_TYPE_FIRST_MATCH|
                ScanSettings::CALLBACK_TYPE_MATCH_LOST);
            settingMask |= (1 << CALLBACK_TYPE_MASK);
          }
          break;
          case settingCbValue::SENSOR_TYPE:
          {
            mCallbackType =ScanSettings::CALLBACK_TYPE_SENSOR_ROUTING;
            settingMask |= (1 << CALLBACK_TYPE_MASK);
          }
          break;
          default:
            ALOGE(LOGTAG "Enter correct callback value"
              "(0-ALL/1-OnLostOnFound/2-SensorType)");
            fprintf(stdout, "Enter correct callback value"
              "(0-ALL/1-OnLostOnFound/2-SensorType)\n");
            return false;
          }
          mscanSettings = settingType::CALLBACK_TYPE;
          break;
      }
      case settingType::MATCH_ADVS:
      {
        fprintf(stdout, "MATCH_ADVS value : %d\n", value);
        if ((value > 0) && (value <= 3)) {
          mNumOfMatchesPerFilter = value;
          settingMask |= (1 << MATCH_ADVS_MASK);
        } else {
          ALOGE(LOGTAG "Enter the correct no of matches value(1/2/3)");
          fprintf(stdout, "Enter the correct no of matches"
              "value(1/2/3)\n");
          return false;
        }
        mscanSettings = settingType::MATCH_ADVS;
        break;
      }
      case settingType::PHY_TYPE:
      {
        fprintf(stdout, "PHY_TYPE value : %d\n", value);
        if (value == 255) {
          mPhy = value;
          settingMask |= (1 << PHY_TYPE_MASK);
        } else {
          ALOGE(LOGTAG "set the correct phy value(255)");
          fprintf(stdout, "set the correct phy value(255)\n");
          return false;
        }
        mscanSettings = settingType::PHY_TYPE;
        break;
      }
      case settingType::SET_LEGACY:
      {
        fprintf(stdout, "SET_LEGACY value : %d\n", value);
        if ((value == 0) || (value == 1)) {
          mLegacy = (bool)value;
          settingMask |= (1 << SET_LEGACY_MASK);
        } else {
          fprintf(stdout, "set or reset the legacy scan type (0/1)\n");
          return false;
        }
        mscanSettings = settingType::SET_LEGACY;
        break;
      }
      case settingType::REPORT_DELAY_MILLS:
      {
        fprintf(stdout, "REPORT_DELAY_MILLS value : %d\n", value);
        if ((value >= 5000) && (value <= BATCH_SCAN_REPORT_DELAY_MILLIS)) {
          mReportDelayMillis = value;
          settingMask |= (1 << REPORT_DELAY_MILLS_MASK);
        } else {
          fprintf(stdout, "set proper values for report delays"
              "(5000 -1000)\n");
          return false;
        }
        mscanSettings = settingType::REPORT_DELAY_MILLS;
        break;
      }
      case settingType::SCANRESULT_TYPE:
      {
        fprintf(stdout, "SCANRESULT_TYPE value : %d\n", value);
        if ((value == 0) || (value == 1)) {
          mScanResultType = value;
          settingMask |= (1 << SCANRESULT_TYPE_MASK);
        } else {
          fprintf(stdout, "set or reset the scan result type (0/1)\n");
          return false;
        }
      }
        mscanSettings = settingType::SCANRESULT_TYPE;
        break;
      default:
        fprintf(stdout, "Enter correct scan setting type\n");
        return false;
    }
   } else {
    settingMask = 0; // NO_SCAN_SETTING
    mScanMode = 0; //SCAN_MODE_LOW_POWER
    mCallbackType = 1; //CALLBACK_TYPE_ALL_MATCHES
    mScanResultType = 0; //SCAN_RESULT_TYPE_FULL
    mReportDelayMillis = 0;
    mMatchMode = 1; //MATCH_MODE_AGGRESSIVE
    mNumOfMatchesPerFilter = 3; //MATCH_NUM_MAX_ADVERTISEMENT
    mLegacy = true;
    mPhy = 255; //PHY_LE_ALL_SUPPORTED
    mscanSettings = settingType::NO_SCAN_SETTING;
  }

  return true;
}

void GattcTest :: startScan()
{
  ALOGD(LOGTAG "startScan");

  if (mscanSettings != settingType::NO_SCAN_SETTING) {
    ALOGD(LOGTAG "SCAN_SETTING present mask (%d) \n", settingMask);
    ScanSettings::Builder builder = ScanSettings::Builder();
    for (auto type = SCAN_MODE_MASK; type < END_MASK; type = settingTypeMask(type+1)) {
      if ((settingMask >> type) & 1) {
        switch(type) {
          case SCAN_MODE_MASK:
            builder.setScanMode(mScanMode);
            break;
          case CALLBACK_TYPE_MASK:
            builder.setCallbackType(mCallbackType);
            break;
          case SCANRESULT_TYPE_MASK:
            builder.setScanResultType(mScanResultType);
            break;
          case PHY_TYPE_MASK:
            builder.setPhy(mPhy);
            break;
          case SET_LEGACY_MASK:
            builder.setLegacy(mLegacy);
            break;
          case REPORT_DELAY_MILLS_MASK:
            builder.setReportDelay(mReportDelayMillis);
            break;
          case MATCH_ADVS_MASK:
            builder.setNumOfMatches(mNumOfMatchesPerFilter);
            break;
          default:
            ALOGE(LOGTAG "Scan Setting Type not valid\n");
            break;
        }
      }
    }
    gattctest->setting = builder.build();
    ALOGD(LOGTAG " ScanSettings matchMode (%d), MatchAdv (%d),\
                  ScanMode (%d) CallbackType (%d) ScanResultType (%d)\
                  legacy (%d), Phy (%d), ReportDelay (%d)",
                  gattctest->setting->getMatchMode(),
                  gattctest->setting->getNumOfMatches(),
                  gattctest->setting->getScanMode(),
                  gattctest->setting->getCallbackType(),
                  gattctest->setting->getScanResultType(),
                  gattctest->setting->getLegacy(),
                  gattctest->setting->getPhy(),
                  gattctest->setting->getReportDelayMillis());
    mScanner->startScan(gattctest->filters, gattctest->setting,
        mscan_callback);
  } else {
    mScanner->startScan(mscan_callback);
  }
}

void GattcTest :: stopScan()
{
  ALOGD(LOGTAG "StopScan");
  fprintf(stdout, "stopping scan results\n");
  gattctest->setting = NULL;
  settingMask = 0;
  gattctest->filters.clear();
  mScanner->stopScan(mscan_callback);
}

void GattcTest :: testBatchscan(int value)
{
  ALOGD(LOGTAG "Test Batch scan Mode");
  fprintf(stdout, "Test Batch scan\n");
  ScanSettings *batchscansettings = ScanSettings::Builder()
    .setScanMode(ScanSettings::SCAN_MODE_BALANCED)
    .setReportDelay(BATCH_SCAN_REPORT_DELAY_MILLIS)
    .setScanResultType(value)
    .build();
  vector < ScanFilter*> filters;
  filters.clear();
  mScanner->startScan(filters, batchscansettings, mscan_callback);
  //Sleep for 5 seconds then flush the results
  std::this_thread::sleep_for (std::chrono::seconds(5));
  //Test Flush Pending scan results API
  fprintf(stdout, "Flush pending scan results\n");
  mScanner->flushPendingScanResults(mscan_callback);
}
