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

#include <algorithm>
#include <fstream>
#include <string>
#include <sstream>
#include <vector>
#include <map>
#include <unordered_map>
#include <list>
#include <iterator>
#include <regex>


#include "GattsTest.hpp"
#include "AdvertiseSettings.hpp"
#include "AdvertiseData.hpp"
#include "AdvertisingSetCallback.hpp"
#include "AdvertisingSet.hpp"
#include "AdvertisingSetParameters.hpp"
#include "PeriodicAdvertiseParameters.hpp"
#include "GattLeAdvertiser.hpp"


using namespace std;
using namespace gatt;

#define LOGTAG "GATTSTEST "
#define UNUSED

#define SERVER_CFG_FILE_PATH "/data/misc/bluetooth/ServerConfigFile.txt"
#define ADV_CFG_FILE_PATH "/data/misc/bluetooth/AdvertiserConfigFile.txt"
#define GATT_SUCCESS 0
#define AUTO_CONNECT 0
#define TRANSPORT 0
#define MAX_SERVER_INSTANCE 20
#define MAX_SERVICE_INSTANCE 5

GattsTest *gattstest = NULL;
extern GattLibService *g_gatt;
int num_of_server;
int num_of_devices;
int num_of_advertiser = 0;
GattServer *mgattServer = NULL;
gattstestServerCallback *gattstestServerCb = NULL;



map<int, AdvertisingSet*> advSetMap;
vector <string> connectedDevices;
unordered_map < gattstestServerCallback*, GattServer*> servCBInstanceMap;

map<gattstestServerCallback*,string> connectedDeviceMap;
map<string,GattServer*> DeviceMap;


vector <string> service_field;
list <GattCharacteristic> mCharList;

AdvertiseSettings *mAdvertiseSettings = NULL;
AdvertiseData *mAdvertiseData = NULL;
AdvertiseData *mScanResponseData = NULL;
AdvertiseData *mPeriodicData = NULL;


AdvertisingSetParameters *mAdvertisingParameters;
PeriodicAdvertiseParameters *mPeriodicParams;
AdvertisingSet *mAdvertisingSet;

bool split (const string &s, char c,vector<string> &v)
{
  string::size_type i = 0;
  string::size_type j = s.find(c);
  //if comma separator found at the first index then the entry is invalid
  if ( j == 0 ) {
    return false;
  }
  //if comma separator not found in entire string
  if( j == string::npos) {
    v.push_back(s);
    return true;
  }
  while(j != string::npos) {
    v.push_back(s.substr(i,j-i));
    i = ++j;
    j = s.find(c,j);
    if(j == string::npos) {
      v.push_back(s.substr(i,s.length()));
      return true;
    }
  }
}

void gattstestServerCallback::onConnectionStateChange(string deviceAddress, int status,
                                                               int newState)
{
  bool connected= false;
  string address;
  GattServer *mServer;
  unordered_map <gattstestServerCallback*,GattServer*> ::iterator ptr;
  map <string,GattServer*> ::iterator dtr = DeviceMap.find(deviceAddress);
  map<gattstestServerCallback*,string> ::iterator iter;
  iter =connectedDeviceMap.find(gattstestServerCb);
  vector <string> ::iterator it;
  it = find(connectedDevices.begin(),connectedDevices.end(),deviceAddress);
  ALOGD(LOGTAG"%s status = %d newState = %d", __FUNCTION__ , status , newState);
  ALOGD(LOGTAG"%s device address: %s",__FUNCTION__, deviceAddress.c_str());
  if (newState == GattDevice::STATE_CONNECTED && status == GATT_SUCCESS) {
    fprintf(stdout,"The device %s got connected \n", deviceAddress.c_str());
    gattstestServerCb = this;
    if(it != connectedDevices.end()) {
      //Device already exists do not insert
    } else {
        connectedDevices.push_back(deviceAddress);
    }
    for(ptr = servCBInstanceMap.begin(); ptr != servCBInstanceMap.end() ; ++ptr ) {
      if(ptr->first == gattstestServerCb) {
        ALOGD(LOGTAG"ptr->first == gattstestServerCb");
        connected = true;
        break;
      }
    }
    if(connected) {
      mServer= ptr->second;
      if(dtr != DeviceMap.end()) {
      //Device Already exists do not add
    } else {
      DeviceMap.insert(pair <string,GattServer*> (deviceAddress,mServer));
    }
    mServer->connect(deviceAddress,AUTO_CONNECT);
    }
  } else if(newState == GattDevice::STATE_DISCONNECTED) {
    fprintf(stdout,"The device %s got disconnected \n", deviceAddress.c_str());
    if(dtr != DeviceMap.end()) {
      DeviceMap.erase(dtr);
    }
    for(it = connectedDevices.begin(); it != connectedDevices.end() ; ++it ) {
      if(*it == deviceAddress) {
        ALOGD(LOGTAG"*itr == deviceAddress  %s == %s", (*it).c_str(),deviceAddress.c_str());
        connectedDevices.erase(it);
        break;
      }
    }
  }
  ALOGD(LOGTAG"Connected Device list :");
  for(it = connectedDevices.begin(); it != connectedDevices.end() ; ++it ) {
    ALOGD(LOGTAG"deviceAddress: %s", (*it).c_str());
  }
}

void gattstestServerCallback::onServiceAdded(int status,GattService *service)
{
  ALOGD(LOGTAG"%s",__FUNCTION__);
  if (status == GATT_SUCCESS) {
    ALOGD(LOGTAG"%s Service Added successfully",__FUNCTION__);
    ALOGD(LOGTAG"The Service has a UUID: %s IsAdvertisePreferred: %d instanceid: %d", service->getUuid().ToString().c_str(),
          service->isAdvertisePreferred(), service->getInstanceId());
    int type = service->getType();
    if(type) {
      ALOGD(LOGTAG"Service is of Type - Secondary");
    } else {
      ALOGD(LOGTAG"Service is of Type - Primary");
    }
  } else {
    ALOGD(LOGTAG "%s Failed to Add Service instance id: %d ", __FUNCTION__, service->getInstanceId());
  }
}

void gattstestServerCallback::onCharacteristicReadRequest(string deviceAddress, int requestId,
                                          int offset, GattCharacteristic *characteristic)
{
  ALOGD(LOGTAG"%s address = %s requestId = %d offset = %d",__FUNCTION__,deviceAddress.c_str(),requestId,offset);
  uint8_t *value = NULL;
  value = characteristic->getValue();
  GattService *mService = characteristic->getService();
  Uuid s_uuid = mService->getUuid();
  Uuid c_uuid = characteristic->getUuid();
  ALOGD(LOGTAG"%s value = %s", __FUNCTION__, value);
  ALOGD(LOGTAG"%s characteristic = %p", __FUNCTION__, *characteristic);
  ALOGD(LOGTAG"%s service Uuid = %s", __FUNCTION__, s_uuid.ToString().c_str());
  ALOGD(LOGTAG"%s characteristic uuid = %s", __FUNCTION__, c_uuid.ToString().c_str());
  GattServer *mServer = NULL;
  unordered_map <gattstestServerCallback*,GattServer*> ::iterator str;
  gattstestServerCb = this;
  for(str = servCBInstanceMap.begin(); str != servCBInstanceMap.end() ; ++str ) {
    if(str->first == gattstestServerCb) {
      ALOGD(LOGTAG"str->first == gattstestServerCb");
      break;
    }
  }
  mServer= str->second;
  ALOGD(LOGTAG"str->first %p  str->second %p", str->first, str->second);
  bool status = mServer->sendResponse(deviceAddress,requestId,0,offset,value);
  if(status) {
    ALOGD(LOGTAG"%s response sent ", __FUNCTION__);
  }
}

void gattstestServerCallback::onCharacteristicWriteRequest(string deviceAddress,int requestId,
                                      GattCharacteristic *characteristic,bool preparedWrite,bool responseNeeded,
                                      int offset,uint8_t* value)
{
  ALOGD(LOGTAG"%s address:%s requestId: %d offset: %d preparedWrite = %d, responseNeeded = %d", __FUNCTION__, deviceAddress.c_str(),
       requestId,offset,preparedWrite,responseNeeded);
  ALOGD(LOGTAG"%s uuid: %s ", __FUNCTION__, characteristic->getUuid().ToString().c_str());
  ALOGD(LOGTAG"%s value: %s ", __FUNCTION__,value);
  characteristic->setValue(value);
  GattServer *mServer = NULL;
  unordered_map <gattstestServerCallback*,GattServer*> ::iterator str;
  bool confirm  = false;
  gattstestServerCb = this;
  for(str = servCBInstanceMap.begin(); str != servCBInstanceMap.end() ; ++str ) {
    if(str->first == gattstestServerCb) {
      ALOGD(LOGTAG"str->first == gattstestServerCb");
      break;
    }
  }
  mServer= str->second;
  if (responseNeeded) {
    mServer->sendResponse(deviceAddress,requestId,0,offset,value);
  }
  int d = characteristic->getProperties() & GattCharacteristic::PROPERTY_NOTIFY;
  if((characteristic->getProperties() & GattCharacteristic::PROPERTY_NOTIFY) != 0) {
    confirm = false;
    mServer->notifyCharacteristicChanged(deviceAddress,*characteristic,confirm);
  } else if((characteristic->getProperties() & GattCharacteristic::PROPERTY_INDICATE) != 0) {
    confirm = true;
    mServer->notifyCharacteristicChanged(deviceAddress,*characteristic,confirm);
  }
}

void gattstestServerCallback::onDescriptorReadRequest(string deviceAddress, int requestId, int offset, GattDescriptor *descriptor)
{
  ALOGD(LOGTAG"%s address = %s requestId = %d offset = %d", __FUNCTION__, deviceAddress.c_str(), requestId, offset);
  uint8_t *value = NULL;
  Uuid desc_uuid = descriptor->getUuid();
  value = descriptor->getValue();
  ALOGD(LOGTAG"%s Descriptor UUID: %s  value = %s", __FUNCTION__, desc_uuid.ToString().c_str(),descriptor->getValue());
  GattServer *mServer = NULL;
  unordered_map <gattstestServerCallback*,GattServer*> ::iterator str;
    gattstestServerCb = this;
    for(str = servCBInstanceMap.begin(); str != servCBInstanceMap.end() ; ++str ) {
    if(str->first == gattstestServerCb)
        ALOGD(LOGTAG"str->first == gattstestServerCb");
        break;
    }
    mServer= str->second;
    bool status = mServer->sendResponse(deviceAddress,requestId,0,offset,value);
    if(status) {
        ALOGD(LOGTAG"%s response sent ", __FUNCTION__);
    }
}

void gattstestServerCallback::onDescriptorWriteRequest(string deviceAddress, int requestId,
                                                    GattDescriptor *descriptor,bool preparedWrite,
                                                    bool responseNeeded, int offset, uint8_t * value)
{
  ALOGD(LOGTAG"%s address: %s requestID: %d preparedWrite: %d  responseNeeded: \
              %d offset: %d", __FUNCTION__, deviceAddress.c_str(), requestId,
              preparedWrite, responseNeeded, offset);
  GattCharacteristic *characteristic = descriptor->getCharacteristic();
  Uuid d_uid = descriptor->getUuid();
  Uuid c_uid = characteristic->getUuid();
  value = descriptor->getValue();
  ALOGD(LOGTAG"%s  descriptor_uuid: %s value = %s", __FUNCTION__, d_uid.ToString().c_str(),
                                                    descriptor->getValue());
  GattServer *mServer = NULL;
  unordered_map <gattstestServerCallback*,GattServer*> ::iterator str;
  gattstestServerCb = this;
  for(str = servCBInstanceMap.begin(); str != servCBInstanceMap.end() ; ++str ) {
    if(str->first == gattstestServerCb) {
      ALOGD(LOGTAG"str->first == gattstestServerCb");
      break;
    }
  }
  mServer= str->second;
  if (responseNeeded) {
    bool status = mServer->sendResponse(deviceAddress,requestId,0,offset,value);
    if (status) {
      ALOGD(LOGTAG"%s response sent ", __FUNCTION__);
    }
  }
}

void gattstestServerCallback::onExecuteWrite(string deviceAddress, int requestId, bool execute)
{
  ALOGD(LOGTAG"%s deviceAddress: %s, requestID: %d execute %d", __FUNCTION__, deviceAddress,
                                                              requestId, execute);
}

void gattstestServerCallback::onNotificationSent(string deviceAddress, int status)
{
  ALOGD(LOGTAG"%s deviceAddress %s status %d", __FUNCTION__, deviceAddress.c_str(), status);
  if (status == GATT_SUCCESS)
    ALOGD(LOGTAG"Notification sent Successfully");
}

void gattstestServerCallback::onMtuChanged(string deviceAddress, int mtu)
{
  ALOGD(LOGTAG"%s deviceAddress: %s mtu %d", deviceAddress.c_str(), mtu);
}

void gattstestServerCallback::onPhyUpdate(string deviceAddress,int txPhy, int rxPhy, int status)
{
  ALOGD(LOGTAG"%s deviceAddress: %s txphy: %d rxPhy: %d   status: %d", __FUNCTION__,
                                          deviceAddress.c_str(), txPhy, rxPhy, status);
  if (status == GATT_SUCCESS)
    ALOGD(LOGTAG"Phy Update Sucessful");
}

void gattstestServerCallback::onPhyRead(string deviceAddress,int txPhy,int rxPhy,int status)
{
  fprintf(stdout,"%s deviceAddress: %s, txPhy: %d rxPhy: %d status: %d \n", __FUNCTION__,
                                                    deviceAddress.c_str(), txPhy, rxPhy, status);
  ALOGD(LOGTAG"%s deviceAddress: %s, txPhy: %d rxPhy: %d status: %d",__FUNCTION__,
                                                    deviceAddress.c_str(), txPhy, rxPhy, status);
  if (status == GATT_SUCCESS)
    ALOGD(LOGTAG"Phy Read Sucessful");
}

void gattstestServerCallback::onConnectionUpdated(string deviceAddress,int interval,int latency,int timeout,int status)
{
  ALOGD(LOGTAG"%s deviceAddress: %s,interval: %d,latency %d,timeout %d, status:%d", __FUNCTION__,
                                        deviceAddress.c_str(), interval, latency, timeout, status);
  if (status == GATT_SUCCESS)
    ALOGD(LOGTAG"Connection updated Successfully");
}


class gattstestAdvertiserCallback  :public AdvertisingSetCallback
{
  public:
  void onAdvertisingSetStarted (AdvertisingSet *advertisingSet, int txPower, int status) {
    ALOGD(LOGTAG"%s status: %d  txpower: %d", __FUNCTION__, status, txPower);
    switch (status) {
      case AdvertisingSetCallback::ADVERTISE_SUCCESS:
        num_of_advertiser++;
        ALOGD(LOGTAG"Advertising Set Success");
        fprintf(stdout,"onAdvertisingSetStarted - Success \n");
        ALOGD(LOGTAG"AdvertiserID: %d", advertisingSet->getAdvertiserId());
        advSetMap.insert(pair <int,AdvertisingSet*> (num_of_advertiser,advertisingSet));
      break;
      case AdvertisingSetCallback::ADVERTISE_FAILED_ALREADY_STARTED:
        ALOGD(LOGTAG"Advertising Already started");
      break;
      case AdvertisingSetCallback::ADVERTISE_FAILED_DATA_TOO_LARGE:
        ALOGD(LOGTAG"Advertising Failed: Data too Large");
      break;
      case AdvertisingSetCallback::ADVERTISE_FAILED_FEATURE_UNSUPPORTED:
        ALOGD(LOGTAG"Advertising Failed: Feature Unsupported");
      break;
      case AdvertisingSetCallback::ADVERTISE_FAILED_INTERNAL_ERROR:
        ALOGD(LOGTAG"Advertising Failed: Internal Error");
      break;
      case AdvertisingSetCallback::ADVERTISE_FAILED_TOO_MANY_ADVERTISERS:
        ALOGD(LOGTAG"Advertising Failed: Too Many Advertisers");
      break;
      default:
        ALOGD(LOGTAG"Failure case unknown");
     }
  }

  void  onAdvertisingDataSet(AdvertisingSet *advertisingset,int status)
  {
    ALOGD(LOGTAG"%s status: %d", __FUNCTION__, status);
    if (status == AdvertisingSetCallback::ADVERTISE_SUCCESS) {
      ALOGD(LOGTAG"Advertising Data is set successfully");
      ALOGD(LOGTAG"Advertiser ID  %d", advertisingset->getAdvertiserId());
    }
  }

  void onAdvertisingSetStopped (AdvertisingSet *advertisingSet)
  {
    ALOGD(LOGTAG"%s",__FUNCTION__);
    ALOGD(LOGTAG"Advertiser ID  %d", advertisingSet->getAdvertiserId());
  }

  void onAdvertisingEnabled (AdvertisingSet *advertisingSet, bool enable, int status)
  {
    ALOGD(LOGTAG"%s  enable: %d status %d",__FUNCTION__,enable,status);
    if (status == AdvertisingSetCallback::ADVERTISE_SUCCESS) {
      ALOGD(LOGTAG"AdvertiserID: %d Advertising Enabled Succesfully", advertisingSet->getAdvertiserId());
    }
  }

  void onScanResponseDataSet (AdvertisingSet *advertisingSet, int status)
  {
    ALOGD(LOGTAG"onScanResponseDataSet status: %d", status);
    if (status == AdvertisingSetCallback::ADVERTISE_SUCCESS) {
      ALOGD(LOGTAG"Advertiser id: %d Scan response Data set successfully", advertisingSet->getAdvertiserId());
    }
  }

  void onAdvertisingParametersUpdated (AdvertisingSet *advertisingSet, int txPower, int status)
  {
    ALOGD(LOGTAG"onAdvertisingParametersUpdated txpower: %d status %d", txPower, status);
    if (status == 0) {
      ALOGD(LOGTAG"Advertiser Id: %d Advertising Parameters Updated Succesfully", advertisingSet->getAdvertiserId());
     }
  }

  void onPeriodicAdvertisingParametersUpdated (AdvertisingSet *advertisingSet, int status)
  {
    ALOGD(LOGTAG"onPeriodicParametersUpdated  status: %d", status);
    if (status == 0) {
      ALOGD(LOGTAG"Advertiser id: %d Periodic Parameters Updated Succesfully", advertisingSet->getAdvertiserId());
    }
  }

  void onPeriodicAdvertisingDataSet (AdvertisingSet *advertisingSet, int status)
  {
    ALOGD(LOGTAG"onPeriodicAAdvertisingDataSet status: %d", status);
    if (status == 0) {
      ALOGD(LOGTAG"Advertiser Id: %d Periodic advertising data Updated Succesfully", advertisingSet->getAdvertiserId());
    }
  }

  void onPeriodicAdvertisingEnabled (AdvertisingSet *advertisingSet, bool enable, int status)
  {
    ALOGD(LOGTAG"onPeriodicAdvertisingEnabled enable : %d status: %d Advertiser id: %d", enable, status, advertisingSet->getAdvertiserId());
  }

  void onOwnAddressRead (AdvertisingSet *advertisingSet, int addressType, string address)
  {
    ALOGD(LOGTAG"onOwnAddressRead  addressType: %d  address: %s advertiser id: %d", addressType, address, advertisingSet->getAdvertiserId());
  }

  void onStartSuccess(AdvertiseSettings *settingsInEffect)
  {
    ALOGD(LOGTAG "onStartSuccess()");
  }

  void onStartFailure(int errorCode)
  {
    ALOGE(LOGTAG "onStartFailure() %d", errorCode);
    fprintf(stdout," onStartFailure errorCode = %d", errorCode);
  }

};

map <int, GattServer*> servInstanceMap;
map <int,gattstestAdvertiserCallback*> advCBInstanceMap;
gattstestAdvertiserCallback *gattstestAdvCb = NULL;
GattLeAdvertiser *madvertiser = NULL;

GattsTest::GattsTest(GattLibService* g_gatt)
{
  ALOGD(LOGTAG"gattstest instantiated ");
  mlibservice = g_gatt->getGatt();
}

GattsTest::~GattsTest()
{
  ALOGD(LOGTAG "(%s) GATTSTEST DeInitialized",__FUNCTION__);
  mlibservice = NULL;
}


void GattsTest::ReadServerConfigurationFile()
{
  ALOGD(LOGTAG"%s",__FUNCTION__);
  string ch;
  int line_num = 0;
  int desired_line = 7;
  bool status = false;
  std::ifstream infile(SERVER_CFG_FILE_PATH);
  //check whether file exists
  if(!infile) {
    ALOGD(LOGTAG"Error opening file");
    return ;
  }

  while(!infile.eof()) {
    getline(infile,ch,'\n');
    if(std::regex_search(ch,std::regex("\\bServer[1-9]|Server[1-9][0-9]\\b"))) {
      while(line_num < desired_line) {
        getline(infile,ch,'\n');
        status = ParseServiceDetails(ch);
        if(!status) {
          fprintf(stdout,"Service Records are not consistent \n");
          break;
        }
        line_num++;
      }
      } else {
        fprintf(stdout,"Server Config File is incorrect \n");
      }
      line_num = 0;
    }
  fprintf(stdout,"File reading Done \n");
  //closing the file after reading
  infile.close();
}

bool GattsTest::ParseServiceDetails(string temp)
{
  ALOGD(LOGTAG"%s",__FUNCTION__);
  int pos=0;
  int i =0;
  int manuID;
  string manuData;
  bool status = false;
  if(regex_search(temp,regex("\\bService1\\b"))) {
    i = 1;
  }
  if(regex_search(temp,regex("\\bService2\\b"))){
    i = 2;
  }
  if(regex_search(temp,regex("\\bService3\\b"))) {
    i = 3;
  }
  if(regex_search(temp,regex("\\bService4\\b"))) {
    i = 4;
  }
  if(regex_search(temp,regex("\\bService5\\b"))) {
    i = 5;
  }
  if(regex_search(temp,regex("\\bManufacturerId\\b"))) {
    i = 6;
  }
  if(regex_search(temp,regex("\\bManufacturerData\\b"))) {
    i = 7;
  }
  pos = temp.find(":");
  temp = temp.substr(pos + 1);
  stringstream ss(temp);
  ss >> temp;
  if(i == 6) {
    istringstream(temp) >> manuID;
    manufacturerId_list.push_back(manuID);
    return true;
  } else if (i == 7) {
    manufacturerData_list.push_back(temp);
    return true;
  } else if (i <= 5 && i >= 1) {
    status = split(temp,',',service_field);
    if(status) {
      ALOGD(LOGTAG"%s  status: %d", __FUNCTION__, status);
      ParseServiceElement(i);
      service_field.clear();
      return true;
    } else {
      if (temp.empty()) {
        ALOGD(LOGTAG" No Service UUID is present for service%d record",i);
        return true;
      } else {
        ALOGD(LOGTAG"Invalid service entry");
        return false;
      }
    }
  } else if (i == 0) {
    fprintf(stdout,"No Service record");
    return false;
  }
}

void GattsTest::ParseServiceElement(int instance)
{
  ALOGD(LOGTAG"%s instance: %d",__FUNCTION__,instance);
  string parameter;
  int property;
  int permissions;
  Service *service_temp = new Service;
  service_temp->s_uuid = "";
  service_temp->c_uuid = "";
  service_temp->d_uuid = "";
  service_temp->c_property = -1;
  service_temp->c_permissions = -1;
  service_temp->d_permissions = -1;
  int len = service_field.size();
  if(instance >= 1 && instance <= 5) {
    if(len >= 1) {
      service_temp->s_uuid = service_field[0];
    }
    if(len >= 2) {
      service_temp->c_uuid = service_field[1];
    }
    if(len >= 3) {
      istringstream(service_field[2]) >> property;
      service_temp->c_property = property;
    }
    if(len >= 4) {
      istringstream(service_field[3]) >> permissions;
      service_temp->c_permissions = permissions;
    }
    if(len >= 5) {
      service_temp->d_uuid = service_field[4];
    }
    if(len >= 5) {
      istringstream(service_field[5]) >> permissions;
      service_temp->d_permissions = permissions;
    }
  }
  if(instance == 1) {
    service1_list.push_back(service_temp);
  }
  if(instance == 2) {
    service2_list.push_back(service_temp);
  }
  if(instance == 3) {
    service3_list.push_back(service_temp);
  }
  if(instance == 4) {
    service4_list.push_back(service_temp);
  }
  if(instance == 5) {
    service5_list.push_back(service_temp);
  }
}

void GattsTest::AddServer()
{
  ALOGD(LOGTAG"%s",__FUNCTION__);
  if(num_of_server <= MAX_SERVER_INSTANCE) {
    num_of_server++;
    ALOGD(LOGTAG"Adding Server Instance : %d", num_of_server);
    mgattServer = new GattServer(g_gatt,TRANSPORT);
    servInstanceMap.insert(pair <int,GattServer*> (num_of_server,mgattServer));
    ALOGD(LOGTAG"Adding Server CallBack : %d ", num_of_server);
    gattstestServerCb = new gattstestServerCallback();
    servCBInstanceMap.insert(pair <gattstestServerCallback*,GattServer*> (gattstestServerCb,mgattServer));
    mgattServer->registerCallback(*gattstestServerCb);
    ALOGD(LOGTAG"Adding Advertiser Callback: %d ", num_of_server);
    gattstestAdvCb    = new gattstestAdvertiserCallback();
    advCBInstanceMap.insert(pair <int,gattstestAdvertiserCallback*> (num_of_server,gattstestAdvCb));
  } else {
    fprintf(stdout,"The number of servers that can be created has reached it's limit of 20 \n");
    ALOGD(LOGTAG"Server Not created");
    num_of_server = 20;
  }
}

bool GattsTest::AddService(string server_instance,string service_instance)
{
  ALOGD(LOGTAG"%s  ",__FUNCTION__);
  int server_inst;
  int service_inst;
  istringstream(server_instance) >> server_inst;
  istringstream(service_instance) >> service_inst;
  ALOGD(LOGTAG"server_inst: %d service_inst %d", server_inst, service_inst);
  Uuid  temp_UUID;
  string uid;
  GattServer *mServer = NULL;
  GattService *mService = NULL;
  Service *service_temp;
  int property = 0;
  int permissions = 0;
  string char_val = "QTI_LE";
  string desc_val = "QTI_DESC";
  if(server_inst > num_of_server) {
    fprintf(stdout,"Please create the server instance first \n");
    return false;
  } else if((server_inst <= 0) || (server_inst > MAX_SERVER_INSTANCE) || (service_inst <=0) || (service_inst > MAX_SERVICE_INSTANCE) ) {
    fprintf(stdout,"Incorrect instance values  \n" );
    return false;
  } else if (service_inst < 6) {
    mServer = servInstanceMap[server_inst];
    if(service_inst == 1) {
      service_temp = service1_list[server_inst - 1];
    }
    if(service_inst == 2) {
      service_temp = service2_list[server_inst - 1];
    }
    if(service_inst == 3) {
      service_temp = service3_list[server_inst - 1];
    }
    if(service_inst == 4) {
      service_temp = service4_list[server_inst - 1];
    }
    if(service_inst == 5) {
      service_temp = service5_list[server_inst - 1];
    }
    uid = service_temp->s_uuid;
    temp_UUID = Uuid::FromString(uid);
    mService  = new GattService(temp_UUID,GattService::SERVICE_TYPE_PRIMARY);
    uid = service_temp->c_uuid;
    if(uid != "") {
      temp_UUID = Uuid::FromString(uid);
      property = service_temp->c_property;
      if(property == -1) {
        property = 2;
      }
      permissions = service_temp->c_permissions;
      if(permissions == -1) {
        permissions = 1;
      }
      AddCharacteristics(temp_UUID,property,permissions,char_val);
      uid = service_temp->d_uuid;
      if(uid != "") {
        temp_UUID = Uuid::FromString(uid);
        permissions = service_temp->d_permissions;
        AddDescriptors(temp_UUID,permissions,desc_val);
        mgattCharacteristic->addDescriptor(mgattDescriptor);
      } else {
        ALOGD(LOGTAG,"No descriptor added\n");
      }
      mService->addCharacteristic(mgattCharacteristic);
    } else {
      ALOGD(LOGTAG,"No Characteristics added \n");
    }
  }
  mServer->addService(*mService);
  return true;
}

bool GattsTest::ReadAdvertiserConfigFile()
{
  ALOGD(LOGTAG"%s",__FUNCTION__);
  string ch;
  int pos=0;
  int line_num = 0;
  int desired_line = 12;
  std::ifstream infile(ADV_CFG_FILE_PATH,std::ios::binary);
  if(!infile) {
    ALOGD(LOGTAG"File doesn't exist \n");
    return false;
  }
  while(!infile.eof()) {
    getline(infile,ch,'\n');
    if(regex_search(ch,regex("\\bAdvertisingSet[1-9]|AdvertisingSet[1-9][0-9]\\b"))) {
      set_temp = new AdvertiseSet;
      set_temp->tx_power = -1;
      set_temp->legacyflag = -1;
      set_temp->periodicflag = -1;
      set_temp->connectableflag = -1;
      set_temp->scannableflag = -1;
      set_temp->anonymousflag = -1;
      set_temp->includeTxPowerflag= -1;
      set_temp->primary_phy= -1;
      set_temp->secondary_phy= -1;
      set_temp->interval = -1;
      set_temp->timeout_legacy= -1;
      set_temp->advertise_mode= -1;
      while(line_num < desired_line) {
        ALOGD(LOGTAG"line_num < desired_line  %d < %d", line_num, desired_line);
        getline(infile,ch,'\n');
        ParseAdvertiserDetails(ch);
        line_num++;
      }
      AdvSet_list.push_back(set_temp);
    }else {
    fprintf(stdout,"There are no Advertising Set records in the file \n");
    break;
    }
    line_num = 0;
  }
  madvertiser = GattLeAdvertiser::getGattLeAdvertiser();
  ALOGD(LOGTAG"File reading done \n");
  infile.close();
  return true;
}

void GattsTest::ParseAdvertiserDetails(string temp)
{
  ALOGD(LOGTAG"%s",__FUNCTION__);
  int pos = 0;
  int parameter = 0;
  string data;
  pos = temp.find(":");
  data = temp.substr(pos + 1);
  istringstream(data) >> parameter;
  if(regex_search(temp,regex("\\bTxPower\\b"))){
    ALOGD(LOGTAG"%s Found Tx Power",__FUNCTION__);
    set_temp->tx_power = parameter;
  }
  if(regex_search(temp,regex("\\bLegacyFlag\\b"))) {
    ALOGD(LOGTAG"%s Found LegacyFlag",__FUNCTION__);
    set_temp->legacyflag = parameter;
  }
  if(regex_search(temp,regex("\\bPeriodicFlag\\b"))) {
    ALOGD(LOGTAG"%s Found PeriodicFlag",__FUNCTION__);
    set_temp->periodicflag = parameter;
  }
  if(regex_search(temp,regex("\\bConnectableFlag\\b"))) {
    ALOGD(LOGTAG"%s Found ConnectableFlag",__FUNCTION__);
    set_temp->connectableflag = parameter;
  }
  if(regex_search(temp,regex("\\bScannableFlag\\b"))) {
    ALOGD(LOGTAG"%s Found ScannableFlag",__FUNCTION__);
    set_temp->scannableflag = parameter;
  }
  if(regex_search(temp,regex("\\bAnonymousFlag\\b"))) {
    ALOGD(LOGTAG"%s Found AnonymousFlag",__FUNCTION__);
    set_temp->anonymousflag = parameter;
  }
  if(regex_search(temp,regex("\\bIncludePower\\b"))) {
    ALOGD(LOGTAG"%s Found IncludePower",__FUNCTION__);
    set_temp->includeTxPowerflag= parameter;
  }
  if(regex_search(temp,regex("\\bPrimaryPhy\\b"))) {
    ALOGD(LOGTAG"%s Found PrimaryPhy",__FUNCTION__);
    set_temp->primary_phy= parameter;
  }
  if(regex_search(temp,regex("\\bSecondaryPhy\\b"))) {
    ALOGD(LOGTAG"%s Found SecondaryPhy",__FUNCTION__);
    set_temp->secondary_phy= parameter;
  }
  if(regex_search(temp,regex("\\bInterval\\b"))) {
    ALOGD(LOGTAG"%s Found Interval",__FUNCTION__);
    set_temp->interval = parameter;
  }
  if(regex_search(temp,regex("\\bTimeOutLegacy\\b"))) {
    ALOGD(LOGTAG"%s Found TimeOutLegacy",__FUNCTION__);
    set_temp->timeout_legacy= parameter;
  }
  if(regex_search(temp,regex("\\bAdvertiseMode\\b"))) {
    ALOGD(LOGTAG"%s Found AdvertiseMode",__FUNCTION__);
    set_temp->advertise_mode= parameter;
  }
}

bool GattsTest::StartAdvertisement(string        instanceID)
{
  ALOGD(LOGTAG"%s",__FUNCTION__);
  int instance = 0;
  istringstream(instanceID) >> instance;
  if (instance <= 0 || instance > MAX_SERVER_INSTANCE) {
    ALOGD("%s invalid input argument");
    fprintf(stdout,"Invalid input argument \n");
    return false;
  }
  int legacyflag = 0;
  AdvertiseSet *temp = NULL;
  bool status = false;
  status = BuildAdvertisingParameters(instance);
  if(!status) {
    fprintf(stdout,"Advertising Parameters not set \n");
    return false;
  }
  status = BuildAdvertisingData(instance);
  if(!status) {
    fprintf(stdout,"Advertising Data not set \n");
    return false;
  }
  status = SetPeriodicAdvertisingParameters(instance);
  if(!status) {
    fprintf(stdout,"Periodic Advertising parameters not set \n");
    return false;
  }
  status = SetPeriodicAdvertisingData(instance);
  if(!status) {
    fprintf(stdout,"Periodic Advertising Data not set \n");
    return false;
  }
  status = SetScanResponseData(instance);
  if(!status) {
    fprintf(stdout,"Scan Response Data not set \n");
    return false;
  }
  //fetching advertiser Callback instance for the server/advertiser instance key
  gattstestAdvCb = advCBInstanceMap[instance];
  //Finding corresponding Legacy flag details for the corresponding advertiser
  temp = AdvSet_list[instance -1];
  legacyflag = temp->legacyflag;
  try {
    if(legacyflag) {
      ALOGD(LOGTAG"Legacy StartAdvertisement");
      madvertiser->startAdvertising(mAdvertiseSettings,mAdvertiseData,mScanResponseData,gattstestAdvCb);
    } else {
      madvertiser->startAdvertisingSet(mAdvertisingParameters,
                         mAdvertiseData,mScanResponseData,mPeriodicParams,mPeriodicData,gattstestAdvCb);
    }
  } catch(const std::exception &ex) {
    ALOGD(LOGTAG"%s start Advertising exception  %s", __FUNCTION__, ex.what());
    return false;
  }
  return true;
}

bool GattsTest::BuildAdvertisingParameters(int instance)
{
  int connectableflag;
  int scannableflag;
  int legacyflag;
  int anonymousflag;
  int periodicflag;
  int includeTxPowerflag;
  int primary_phy;
  int secondary_phy;
  int interval;
  int tx_power;
  int power_mode;
  int timeout_legacy;
  int advertise_mode;
  ALOGD(LOGTAG"%s",__FUNCTION__);
  AdvertiseSet *temp;
  temp = AdvSet_list[instance - 1];
  if(temp == NULL) {
    ALOGE(LOGTAG"%s Advertising Configuration not found", __FUNCTION__);
    return false;
  }
  legacyflag = temp->legacyflag;
  //input validation
  if (legacyflag < 0 && connectableflag < 0 && scannableflag < 0 && periodicflag < 0 && anonymousflag < 0 && includeTxPowerflag < 0 &&
        primary_phy < 0 && secondary_phy < 0 && interval < 0 && tx_power < 0 && power_mode < 0 && timeout_legacy < 0 && advertise_mode < 0) {
    fprintf(stdout,"Incorrect value of legacy flag\n");
    return false;
  }
  if (legacyflag > 1 && connectableflag > 1 && scannableflag > 1 && periodicflag > 1 && anonymousflag > 1 && includeTxPowerflag > 1) {
    fprintf(stdout,"Flags values set incorrectly, Please check 'legacyflag' 'connectableflag' 'scannableflag' periodicflag' 'anonymousflag' includetxpowerflag' \n");
    return false;
  }
  try {
    if(legacyflag) {
      ALOGD(LOGTAG" Legacy Advertising will be used \n");
      mAdvertiseSettings = AdvertiseSettings::Builder()
                           .setAdvertiseMode(temp->advertise_mode)
                           .setTxPowerLevel(temp->tx_power)
                           .setConnectable(temp->connectableflag)
                           .setTimeout(temp->timeout_legacy)
                           .build();

      ALOGD(LOGTAG"Advertising Settings connectable = %d \
            TxPowerLevel = %d AdvertiseMode =%d TimeOut = %d",
            mAdvertiseSettings->isConnectable(),
            mAdvertiseSettings->getTxPowerLevel(), mAdvertiseSettings->getMode(),
            mAdvertiseSettings->getTimeout());
    } else {
      mAdvertisingParameters = AdvertisingSetParameters::Builder()
                              .setConnectable(temp->connectableflag)
                              .setScannable(temp->scannableflag)
                              .setLegacyMode(temp->legacyflag)
                              .setAnonymous(temp->anonymousflag)
                              .setIncludeTxPower(temp->includeTxPowerflag)
                              .setPrimaryPhy(temp->primary_phy)
                              .setSecondaryPhy(temp->secondary_phy)
                              .setInterval(temp->interval)
                              .setTxPowerLevel(temp->tx_power)
                              .build();

    ALOGD(LOGTAG"Advertising parameters connectable = %d \
            scannable= %d LegacyMode =%d anonymous = %d \
            includeTxPower = %d primaryphy = %d secondaryphy = %d \
            interval = %d Txpower = %d \n", mAdvertisingParameters->isConnectable(),
            mAdvertisingParameters->isScannable(), mAdvertisingParameters->isLegacy(),
            mAdvertisingParameters->isAnonymous(), mAdvertisingParameters->includeTxPower(),
            mAdvertisingParameters->getPrimaryPhy(), mAdvertisingParameters->getSecondaryPhy(),
            mAdvertisingParameters->getInterval(), mAdvertisingParameters->getTxPowerLevel());
    }
  }
  catch(const std::exception &ex) {
    ALOGD(LOGTAG"%s exception caught: %s", __FUNCTION__, ex.what());
    fprintf(stdout,"exception: %s", ex.what());
    return false;
  }
  return true;
}

bool GattsTest::BuildAdvertisingData(int instance) {
  int includeTxPowerflag;
  string mManufacturerID;
  string mManufacturerData;
  string service1_uuid;
  Service *temp;
  AdvertiseSet *set = NULL;
  string service_data= "QTI_SERVICE_DATA";
  string service_data_uuid = "0000AAAA-0000-1000-8000-00805F9B34FB";
  Uuid mUuid;
  set = AdvSet_list[instance -1];
  if(set == NULL) {
    ALOGE(LOGTAG"%s Advertising Configuration not found", __FUNCTION__);
    return false;
  }
  ALOGD(LOGTAG"%s",__FUNCTION__);
  AdvertiseData::Builder builder = AdvertiseData::Builder().setIncludeDeviceName(true)
                                  .setIncludeTxPowerLevel(set->includeTxPowerflag);
  mManufacturerID = manufacturerId_list[instance-1];
  mManufacturerData = manufacturerData_list[instance-1];
  int legacyflag =  set->legacyflag;
  //If legacy flag is not set then add manufacturer and Service data
  if(!legacyflag) {
  //If manufacturer Data and ID are not empty then add to Advertising Data
    if(mManufacturerID != "" && mManufacturerData != "" ) {
      int id=0;
      istringstream(mManufacturerID) >> id;
      ALOGD(LOGTAG"ManufacturerID: %d", id);
      ALOGD(LOGTAG"Manufacturer Data: %s", mManufacturerData.c_str());
      std::vector<uint8_t> vec(mManufacturerData.begin(), mManufacturerData.end());
      builder.addManufacturerData(id,vec);
    }
    temp= service1_list[instance -1];
    if(temp->s_uuid.empty()) {
      mUuid = btapp::Uuid::FromString(temp->s_uuid);
      builder.addServiceUuid(mUuid);
      mUuid = btapp::Uuid::FromString(service_data_uuid);
      std::vector<uint8_t> vec(service_data.begin(), service_data.end());
      builder.addServiceData(mUuid,vec);
    }
  }
  mAdvertiseData = builder.build();
  ALOGD(LOGTAG"AdvertiseData IncludeDevicename: %d IncludeTxPowerLevel: %d ",
              mAdvertiseData->getIncludeDeviceName(), mAdvertiseData->getIncludeTxPowerLevel());
  if(!legacyflag) {
    ALOGD(LOGTAG"manufacturer_data size:  %d", mAdvertiseData->getManufacturerSpecificData().size());
    ALOGD(LOGTAG"serviceData size:  %d", mAdvertiseData->getServiceData().size());
  }
  return true;
}

bool GattsTest::SetPeriodicAdvertisingData(int instance)
{
  ALOGD(LOGTAG"%s",__FUNCTION__);
  int periodic_flag;
  AdvertiseSet *temp = NULL;
  temp = AdvSet_list[instance -1];
  if(temp == NULL) {
    ALOGE(LOGTAG"%s Advertising Configuration not found", __FUNCTION__);
    return false;
  }
  periodic_flag = temp->periodicflag;
  if(periodic_flag) {
    mPeriodicData = mAdvertiseData;
    mAdvertisingSet->setPeriodicAdvertisingData(*mPeriodicData);
  } else {
    mPeriodicData = NULL;
  }
  return true;
}


bool GattsTest::SetPeriodicAdvertisingParameters(int instance)
{
  ALOGD(LOGTAG"%s ",__FUNCTION__);
  int periodic_flag;
  AdvertiseSet *temp = NULL;
  temp = AdvSet_list[instance -1];
  if(temp == NULL) {
    ALOGE(LOGTAG"%s Advertising Configuration not found", __FUNCTION__);
    return false;
  }
  periodic_flag = temp->periodicflag;
  int include_txpower;
  int periodic_interval = 200;
  include_txpower = temp->includeTxPowerflag;

  if(periodic_flag) {
    mPeriodicParams = PeriodicAdvertiseParameters::Builder()
                      .setIncludeTxPower(include_txpower)
                      .setInterval(periodic_interval)
                      .build();

    ALOGD(LOGTAG"SetPeriodicAdvertisingParameters:: IncludeTxPower: %d interval %d", mPeriodicParams->getIncludeTxPower() ,mPeriodicParams->getInterval());
      mAdvertisingSet->setPeriodicAdvertisingParameters(*mPeriodicParams);
  } else {
    mPeriodicParams = NULL;
  }
  return true;
}

bool GattsTest::SetScanResponseData(int instance)
{
  ALOGD(LOGTAG"%s ",__FUNCTION__);
  int scannable_flag;
  AdvertiseSet *temp = NULL;
  temp = AdvSet_list[instance -1];
  if(temp == NULL) {
    ALOGE(LOGTAG"%s Advertising Configuration not found", __FUNCTION__);
    return false;
  }
  scannable_flag = temp->scannableflag;
  if(scannable_flag) {
    mScanResponseData = mAdvertiseData;
  } else {
    mScanResponseData = NULL;
  }
  return true;
}

bool GattsTest::UnregisterServer(string instance)
{
  GattServer *mServer;
  ALOGD(LOGTAG"%s ",__FUNCTION__);
  int instanceId;
  istringstream(instance) >> instanceId;
  if(instanceId <=0 || instanceId > num_of_server || instanceId > MAX_SERVER_INSTANCE) {
    fprintf(stdout,"Server instance value invalid, Please type a valid instance\n");
    return false;
  }
  if(num_of_server <= MAX_SERVER_INSTANCE) {
    mServer = servInstanceMap[instanceId];
    mServer->close();
    return true;
  } else {
    fprintf(stdout,"There are no more servers to unregister \n");
    return false;
  }
}

bool GattsTest::StopAdvertisement(string instance)
{
  ALOGD(LOGTAG"StopAdvertisement \n");
  int instanceId;
  istringstream(instance) >> instanceId;
  if(instanceId <=0 || instanceId > num_of_server) {
    fprintf(stdout,"Server instance value invalid, Please type a valid instance\n");
    return false;
  } else {
    AdvertisingSetCallback *mAdvSetCB;
    mAdvSetCB = advCBInstanceMap[instanceId];
    madvertiser->stopAdvertising(mAdvSetCB);
  }
}


bool GattsTest::AddCharacteristics(Uuid uid,int property, int permissions, string val)
{
  ALOGD(LOGTAG"%s",__FUNCTION__);
  mgattCharacteristic = new GattCharacteristic(uid,property,permissions);
  uint8_t c_val1[val.length()+1];
  std::copy(val.begin(),val.end(),c_val1);
  mgattCharacteristic->setValue(c_val1);
  ALOGD(LOGTAG"CharacteristicUUID: %s  ", uid.ToString().c_str());
  ALOGD(LOGTAG"Characteristic Property: %d ", mgattCharacteristic->getProperties());
  ALOGD(LOGTAG"characteristic Permissions: %d ", mgattCharacteristic->getPermissions());
  ALOGD(LOGTAG"characteristic value: %s", mgattCharacteristic->getValue());
}

bool GattsTest::AddDescriptors(Uuid uid,int permissions,string value)
{
  ALOGD(LOGTAG"%s",__FUNCTION__);
  ALOGD(LOGTAG"string value =  %s", value.c_str());
  mgattDescriptor = new GattDescriptor(uid,permissions);
  uint8_t d_val1[value.length()+1];
  std::copy(value.begin(),value.end(),d_val1);
  mgattDescriptor->setValue(d_val1);
  ALOGD(LOGTAG"Descriptor UUID: %s  ", uid.ToString().c_str());
  ALOGD(LOGTAG"Descriptor Permissions: %d ", mgattDescriptor->getPermissions());
}

bool GattsTest::ReadPhy(string instance,string deviceAddress)
{
  ALOGD(LOGTAG"%s Address: %s", __FUNCTION__, deviceAddress.c_str());
  vector <string> ::iterator str;
  bool connected= false;
  int instanceId;
  istringstream(instance) >> instanceId;
  GattServer *mServer;

  if(instanceId <=0 || instanceId > num_of_server || instanceId > MAX_SERVER_INSTANCE) {
    fprintf(stdout,"Server instance value invalid, Please type a valid instance\n");
    return false;
  } else {
    mServer = servInstanceMap[instanceId];
    for(str = connectedDevices.begin(); str != connectedDevices.end(); str++) {
      if(deviceAddress == *str) {
        ALOGD(LOGTAG"Present in connected device list ");
        connected = true;
        break;
      }
    }
  }
  if(connected) {
    mServer->readPhy(deviceAddress);
    return true;
  } else {
    ALOGD(LOGTAG"Device is not present in connected list");
    return false;
  }
}

bool GattsTest::SetPreferredPhy(string deviceAddress,string instance,string txPhy,string rxPhy,int phyOptions)
{
  ALOGD(LOGTAG"%s Address: %s  txPhy: %s rxPhy: %s phyOptions: %d", __FUNCTION__, deviceAddress.c_str(), txPhy.c_str(), rxPhy.c_str(), phyOptions);
  int instanceId = 0;
  int tx_phy = 0;
  int rx_phy = 0;
  istringstream(instance) >> instanceId;
  istringstream(txPhy) >> tx_phy;
  istringstream(rxPhy) >> rx_phy;
  GattServer *mServer;
  vector <string> ::iterator str;
  bool connected= false;
  if (instanceId <=0 || instanceId > num_of_server || instanceId > MAX_SERVER_INSTANCE) {
    fprintf(stdout,"Server instance value invalid, Please type a valid instance\n");
    return false;
  } else {
    if((tx_phy != 1) && (tx_phy != 2) && (tx_phy != 3)) {
      fprintf(stdout,"Enter a valid tx phy option \n");
      return false;
  }
  if((rx_phy != 1) && (rx_phy != 2) && (rx_phy != 3)) {
    fprintf(stdout,"Enter a valid rx phy option \n");
    return false;
  }
  mServer = servInstanceMap[instanceId];
  for(str = connectedDevices.begin(); str != connectedDevices.end(); str++) {
    if(deviceAddress == *str) {
      ALOGD(LOGTAG"Present in connected device list ");
      connected = true;
      break;
    }
  }
  if(connected) {
    mServer->setPreferredPhy(deviceAddress,tx_phy,rx_phy,phyOptions);
    return true;
  } else {
    return false;
  }
  }
}

bool GattsTest::EnablePeriodicAdvertising(bool enable)
{
  ALOGD(LOGTAG"%s ", __FUNCTION__);
  mAdvertisingSet->setPeriodicAdvertisingEnabled(enable);
}

void GattsTest::CancelConnection(string remoteAddress)
{
  ALOGD(LOGTAG"%s", __FUNCTION__);
  GattServer *mServer = NULL;
  bool connected = false;
  map <string,GattServer*> ::iterator dtr = DeviceMap.find(remoteAddress);
  for(dtr = DeviceMap.begin(); dtr != DeviceMap.end() ; ++dtr) {
    if(remoteAddress == dtr->first) {
      mServer = dtr->second;
      connected = true;
      break;
    }
  }
  if(connected) {
    mServer->cancelConnection(remoteAddress);
  } else {
    fprintf(stdout,"Device %s is not connected", remoteAddress.c_str());
    ALOGD(LOGTAG"Device %s is not connected", remoteAddress.c_str());
  }
}

bool GattsTest::DisableGATTSTEST()
{
  ALOGD(LOGTAG"%s",__FUNCTION__);
  GattServer *mServer = NULL;
  gattstestServerCallback *mServercallback = NULL;
  gattstestAdvertiserCallback *mAdvertisercallback = NULL;
  map <int, GattServer*> ::iterator itr;
  map <int,gattstestAdvertiserCallback*> ::iterator at;
  servInstanceMap.clear();
  unordered_map  <gattstestServerCallback*,GattServer*> ::iterator it;
  for(it = servCBInstanceMap.begin(); it != servCBInstanceMap.end(); ++it) {
    mServercallback = it->first;
    delete(mServercallback);
    mServer = it->second;
    delete(mServer);
  }
  servCBInstanceMap.clear();
  for(at = advCBInstanceMap.begin(); at != advCBInstanceMap.end(); ++at) {
    mAdvertisercallback = at->second;
    delete(mAdvertisercallback);
  }
  advCBInstanceMap.clear();
  delete(madvertiser);
  return true;
}




