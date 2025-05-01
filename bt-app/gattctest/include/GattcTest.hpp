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

#ifndef GATTCTEST_APP_H
#define GATTCTEST_APP_H

#pragma once


#include "osi/include/log.h"
#include "osi/include/thread.h"
#include "osi/include/config.h"
#include "ipc.hpp"
#include "GattClient.hpp"
#include "GattClientCallback.hpp"
#include <GattDescriptor.hpp>
#include "uuid.h"

#ifdef USE_GLIB
#include <glib.h>
#define strlcpy g_strlcpy
#endif

#define LOGTAG "GATTCTEST "


using namespace gatt;
using namespace btapp;

typedef map<string, class GattClient *> RmDev;
typedef map<string, DeviceProperties> PaDev;
typedef map<int, string> PaSyncedDev;

class GattcTest {
    private:
        GattLibService *libservice;
        GattClient *gattcli;
        ScanFilter *filter;
        vector < ScanFilter *> filters;
        ScanSettings *setting;
        bool isOpportunistic;
        int phy;
        bool isAuto;
        static int settingMask;
        static int mScanMode;
        static int mCallbackType;
        static int mScanResultType;
        static long mReportDelayMillis;
        static int mMatchMode;
        static int mNumOfMatchesPerFilter;
        static bool mLegacy;
        static int mPhy;
/***************************/
    public:
        GattcTest(GattLibService *);
        ~GattcTest();
        void enableGattctest();
        bool validateInput(string str);
        void gattConnParams(bool automatic, int phy, bool isOpportunistic);
        bool gattConnect(string bdaddr, int transport);
        void gattDisconnect(string bdaddr);
        void gattDiscoverServices(string bdaddr);
        GattCharacteristic* getCharacteristic(Uuid uuid, string bdaddr);
        bool getCharacteristicById(string bdaddr, int instanceId);
        bool getDescriptorById(string bdaddr, int instanceId);
        GattDescriptor* getDescriptor(Uuid uid, string bdaddr);
        bool getService(string bdaddr, Uuid serviceUid,int instanceid);
        GattService getService(Uuid uuid);
        bool readUsingCharacteristicUuid(string bdaddr, Uuid uuid, int startHandle, int endHandle);
        bool writeCharacteristic(string bdaddr, uint8_t *writeValue, int length,
            int instanceId);
        bool prepareWriteCharacteristic(string bdaddr, uint8_t *writeValue, int length,
            int instanceId);
        bool readCharacteristicUUID(string bdaddr, Uuid uuid);
        void writeDescriptor(string bdaddr, uint8_t *writeValue, int length, int instanceid);
        void readCharacteristic(string bdaddr, int instanceid);
        //bool readCharacteristicUUID(string bdaddr, Uuid uuid);
        void readDescriptor(string bdaddr, int instanceid);
        void gattClientReadPhy(string bdaddr);
        void gattReadRemoteRssi(string bdaddr);
        void gattRefresh(string bdaddr);
        void gattrequestMtu(string bdaddr, int mtu_value);
        void getServices(string bdaddr);
        bool requestConnectionPriority(string bdaddr, int connectionPriority);
        void setPreferredPhy(int txPhy, int rxPhy, int phyOptions, string bdaddr);
        bool gattDiscoverServicesByUuid(Uuid uuid,string bdaddr);
        void testBatchscan(int value);
        void stopScan();
        void startScan();
        void list_conn_devices();
        bool reqConnPri(string bdaddr, int conn_priority);
        bool reliableWrite(string bdaddr, int instanceId);
        bool scanFilter(int filterType, string value);
        bool isValidScanPhy(int value);
        bool scanSettings(int scanType, int value);
        void scanFilterManuData(int manuId, string manuData, string manuMask);
        bool createPeriodicSync(string bdaddr);
        void stopPeriodicSync(string bdaddr);
        void list_pa_devices();
        void list_pa_synced_devices();
};

class mRemoteDev {
    private:
        RmDev mapClient;
        PaDev mapPaDev;
        PaSyncedDev mapPaSyncedDev;
    public:
        mRemoteDev(string x, class GattClient *);
        void add(string dev, class GattClient *);
        void remove(string dev);
        list<string> getConnectedDevices();
        GattClient* getGatt(string dev);
        list<GattClient*> getGattList();
        bool containsDevice(string device);
        void addUpdatePaDev(string dev, DeviceProperties dev_property);
        void removePaDev(string dev);
        int getPaSid(string dev);
        bool containsPaDevice(string dev);
        void printPaDevices();
        void addPaSyncedDev(int sync_handle, string dev);
        void removePaSyncedDev(int sync_handle);
        int containsPaSyncedDevice(string dev);
        bool containsPaSyncedDevice(int sync_handle);
        void printPaSyncedDevices();
        void clear();
};

#endif

