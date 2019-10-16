/*
 * Copyright (c) 2017-18, The Linux Foundation. All rights reserved.
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

#ifndef GATTSTEST_APP_H
#define GATTSTEST_APP_H

#pragma once

#include <vector>
#include <string>
#include <map>

#include "osi/include/log.h"
#include "osi/include/thread.h"
#include "osi/include/config.h"
#include "ipc.hpp"
#include "GattServer.hpp"
#include "GattServerCallback.hpp"
#include "uuid.h"
#include "GattDescriptor.hpp"


#define LOGTAG "GATTSTEST "

using namespace std;
using namespace gatt;

bool split (const string &s, char c,vector<string> &v);
class GattsTest {
  private:
    GattLibService *mlibservice;
    list <GattService> mServiceList;

    struct Service {
      string s_uuid;
      string c_uuid;
      int c_property;
      int c_permissions;
      string d_uuid;
      int d_permissions;
    };

    struct AdvertiseSet {
      int tx_power;
      int legacyflag;
      int periodicflag;
      int connectableflag;
      int scannableflag;
      int anonymousflag;
      int includeTxPowerflag;
      int primary_phy;
      int secondary_phy;
      int interval;
      int timeout_legacy;
      int advertise_mode;
    };

    vector <AdvertiseSet*> AdvSet_list;
    AdvertiseSet *set_temp= NULL;
    vector <Service*> service_list[5];
    vector <int> manufacturerId_list;
    vector <string> manufacturerData_list;
    GattCharacteristic *mgattCharacteristic = NULL;
    GattDescriptor *mgattDescriptor = NULL;
    string manufacturer_ID;
    string manufacturer_data;
    int manufacturerID;

  public:
    GattsTest(GattLibService* );
    ~GattsTest();
    void ReadServerConfigurationFile();
    bool ParseServiceDetails(string,int);
    void ParseServiceElement(int);
    void AddServer();
    bool ReadAdvertiserConfigFile();
    void ParseAdvertiserDetails(string);
    bool DisableGATTSTEST();
    bool StartAdvertisement(string);
    bool BuildAdvertisingParameters(int);
    bool BuildAdvertisingData(int);
    void StopAdvertisement(string);
    bool UnregisterServer(string);
    bool AddService(string,string);
    void AddCharacteristics(Uuid,int,int,string);
    void AddDescriptors(Uuid,int,string);
    bool SetPreferredPhy(string,string,string,string,int);
    bool ReadPhy(string,string);
    bool EnablePeriodicAdvertising(bool);
    bool SetPeriodicAdvertisingData(int);
    bool SetPeriodicAdvertisingParameters(int);
    bool SetScanResponseData(int);
    void CancelConnection(string);
};


class gattstestServerCallback :public GattServerCallback
{

  public:
  void onConnectionStateChange(string deviceAddress, int status, int newState);
  void onServiceAdded(int status,GattService *service);
  void onCharacteristicReadRequest(string deviceAddress, int requestId, int offset,
                                      GattCharacteristic *characteristic);
  void onCharacteristicWriteRequest(string deviceAddress,int requestId,
                                      GattCharacteristic *characteristic,bool preparedWrite,
                                      bool responseNeeded,int offset,uint8_t* value);
  void onDescriptorReadRequest(string deviceAddress, int requestId, int offset,
                                      GattDescriptor *descriptor);
  void onDescriptorWriteRequest(string deviceAddress, int requestId,
                                      GattDescriptor *descriptor,bool preparedWrite,
                                      bool responseNeeded, int offset, uint8_t * value);
  void onExecuteWrite(string deviceAddress, int requestId, bool execute);
  void onNotificationSent(string deviceAddress, int status);
  void onMtuChanged(string deviceAddress, int mtu);
  void onPhyUpdate(string deviceAddress,int txPhy, int rxPhy, int status);
  void onPhyRead(string deviceAddress,int txPhy,int rxPhy,int status);
  void onConnectionUpdated(string deviceAddress,int interval,int latency,
                                    int timeout,int status);
};

#endif

