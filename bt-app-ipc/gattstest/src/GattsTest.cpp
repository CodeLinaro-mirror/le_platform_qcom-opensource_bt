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
#define TRANSPORT 2
#define MAX_SERVER_INSTANCE 20
#define MAX_SERVICE_INSTANCE 5
#define INVALID_VALUE -1
#define VALID_VALUE 0
#define PERIODIC_INTERVAL 200
#define PHY_LE_1M 1
#define PHY_LE_2M 2
#define PHY_LE_CODED 4
#define PROPERTY_READ 2
#define SERVICE_LINE_MIN 0
#define SERVICE_LINE_MAX 4
#define MANUFACTURER_ID_LINE 5
#define MANUFACTURER_DATA_LINE 6
#define SERVICE_1 1
#define SERVICE_2 2
#define SERVICE_3 3
#define SERVICE_4 4
#define SERVICE_5 5


GattsTest *gattstest = NULL;
extern GattLibService *g_gatt;
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
string receivedData;
string receivedDescValue;
GattCharacteristic *executeWriteChar;
GattDescriptor *executeWriteDesc;



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
        ALOGD(LOGTAG"deviceAddress:  %s", (*it).c_str());
        connectedDevices.erase(it);
        break;
      }
    }
  }
  if(connectedDevices.size() != 0) {
    ALOGD(LOGTAG"Connected Device list :");
    for(it = connectedDevices.begin(); it != connectedDevices.end() ; ++it ) {
      ALOGD(LOGTAG"deviceAddress: %s", (*it).c_str());
    }
  }
}

void gattstestServerCallback::onServiceAdded(int status,GattService *service)
{
  ALOGD(LOGTAG"%s status: %d",__FUNCTION__,status);
}

void gattstestServerCallback::onCharacteristicReadRequest(string deviceAddress, int requestId,
                                          int offset, GattCharacteristic *characteristic)
{
  ALOGD(LOGTAG"%s ",__FUNCTION__);
  uint8_t *value = NULL;
  value = characteristic->getValue();
  GattService *mService = characteristic->getService();
  Uuid s_uuid = mService->getUuid();
  Uuid c_uuid = characteristic->getUuid();
  ALOGD(LOGTAG"%s value = %s", __FUNCTION__, value);
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
  bool status = mServer->sendResponse(deviceAddress,requestId,0,offset,value + offset);
  if(status) {
    ALOGD(LOGTAG"%s response sent ", __FUNCTION__);
  }
}

void gattstestServerCallback::onCharacteristicWriteRequest(string deviceAddress,int requestId,
                            GattCharacteristic *characteristic,bool preparedWrite,bool responseNeeded,
                            int offset,uint8_t* value, int length)
{
  ALOGD(LOGTAG"%s ",__FUNCTION__);
  string temp((char *)value);
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
  if(preparedWrite) {
     executeWriteChar = characteristic;
     receivedData += temp;
  } else {
    characteristic->setValue(value, length);
  }
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

void gattstestServerCallback::onDescriptorReadRequest(string deviceAddress, int requestId,
                                                              int offset, GattDescriptor *descriptor)
{
  ALOGD(LOGTAG"%s ",__FUNCTION__);
  uint8_t *value = NULL;
  Uuid desc_uuid = descriptor->getUuid();
  value = descriptor->getValue();
  ALOGD(LOGTAG"%s Descriptor UUID: %s  value = %s", __FUNCTION__, desc_uuid.ToString().c_str(),descriptor->getValue());
  GattServer *mServer = NULL;
  unordered_map <gattstestServerCallback*,GattServer*> ::iterator str;
    gattstestServerCb = this;
    for(str = servCBInstanceMap.begin(); str != servCBInstanceMap.end() ; ++str ) {
    if(str->first == gattstestServerCb)
        break;
    }
    mServer= str->second;
    bool status = mServer->sendResponse(deviceAddress,requestId,0,offset,value+offset);
    if(status) {
        ALOGD(LOGTAG"%s response sent ", __FUNCTION__);
    }
}

void gattstestServerCallback::onDescriptorWriteRequest(string deviceAddress, int requestId,
                                                    GattDescriptor *descriptor,bool preparedWrite,
                                                    bool responseNeeded, int offset, uint8_t * value,
                                                    int length)
{
  ALOGD(LOGTAG"%s ",__FUNCTION__);
  string temp((char *)value);
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
      break;
    }
  }
  mServer= str->second;
  if(preparedWrite) {
     executeWriteDesc = descriptor;
     receivedDescValue += temp;
  } else {
    descriptor->setValue(value, length);
  }
  if (responseNeeded) {
    bool status = mServer->sendResponse(deviceAddress,requestId,0,offset,value+offset);
    if (status) {
      ALOGD(LOGTAG"%s response sent ", __FUNCTION__);
    }
  }
}

void gattstestServerCallback::onExecuteWrite(string deviceAddress, int requestId, bool execute)
{
  ALOGD(LOGTAG"%s deviceAddress: %s, requestID: %d execute %d", __FUNCTION__, deviceAddress.c_str(),
                                                              requestId, execute);
  GattServer *mServer = NULL;
  unordered_map <gattstestServerCallback*,GattServer*> ::iterator str;
  gattstestServerCb = this;
  for(str = servCBInstanceMap.begin(); str != servCBInstanceMap.end() ; ++str ) {
    if(str->first == gattstestServerCb) {
      break;
    }
  }
  mServer= str->second;
  if(execute) {
    executeWriteChar->setValue(receivedData);
    receivedData.clear();
  } else {
     receivedData.clear();
  }
  bool status = mServer->sendResponse(deviceAddress,requestId,GATT_SUCCESS,0,NULL);
  if (status) {
    ALOGD(LOGTAG"%s response sent ", __FUNCTION__);
  }
}

void gattstestServerCallback::onNotificationSent(string deviceAddress, int status)
{
  ALOGD(LOGTAG"%s deviceAddress %s status %d", __FUNCTION__, deviceAddress.c_str(), status);
}

void gattstestServerCallback::onMtuChanged(string deviceAddress, int mtu)
{
  ALOGD(LOGTAG"%s deviceAddress: %s mtu %d",__FUNCTION__,deviceAddress.c_str(), mtu);
}

void gattstestServerCallback::onPhyUpdate(string deviceAddress,int txPhy, int rxPhy, int status)
{
  ALOGD(LOGTAG"%s deviceAddress: %s txphy: %d rxPhy: %d   status: %d",__FUNCTION__,
                                                    deviceAddress.c_str(),txPhy,rxPhy,status);
}

void gattstestServerCallback::onPhyRead(string deviceAddress,int txPhy,int rxPhy,int status)
{
  fprintf(stdout,"%s deviceAddress: %s, txPhy: %d rxPhy: %d status: %d", __FUNCTION__,
                                                    deviceAddress.c_str(), txPhy, rxPhy, status);
  ALOGD(LOGTAG"%s deviceAddress: %s, txPhy: %d rxPhy: %d status: %d",__FUNCTION__,
                                                    deviceAddress.c_str(), txPhy, rxPhy, status);
}

void gattstestServerCallback::onConnectionUpdated(string deviceAddress,int interval,int latency,int timeout,int status)
{
  ALOGD(LOGTAG"%s deviceAddress: %s,interval: %d,latency %d,timeout %d, status:%d", __FUNCTION__,
                                        deviceAddress.c_str(), interval, latency, timeout, status);
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
    ALOGD(LOGTAG"%s Advertiser ID  %d",__FUNCTION__,advertisingSet->getAdvertiserId());
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
  }

  void onAdvertisingParametersUpdated (AdvertisingSet *advertisingSet, int txPower, int status)
  {
    ALOGD(LOGTAG"onAdvertisingParametersUpdated txpower: %d status %d", txPower, status);
  }

  void onPeriodicAdvertisingParametersUpdated (AdvertisingSet *advertisingSet, int status)
  {
    ALOGD(LOGTAG"onPeriodicParametersUpdated  status: %d", status);
  }

  void onPeriodicAdvertisingDataSet (AdvertisingSet *advertisingSet, int status)
  {
    ALOGD(LOGTAG"onPeriodicAAdvertisingDataSet status: %d", status);
  }

  void onPeriodicAdvertisingEnabled (AdvertisingSet *advertisingSet, bool enable, int status)
  {
    ALOGD(LOGTAG"onPeriodicAdvertisingEnabled enable : %d status: %d Advertiser id: %d", enable,
                                                      status, advertisingSet->getAdvertiserId());
  }

  void onOwnAddressRead (AdvertisingSet *advertisingSet, int addressType, string address)
  {
    ALOGD(LOGTAG"onOwnAddressRead  addressType: %d  address: %s advertiser id: %d", addressType, 
                                                address.c_str(), advertisingSet->getAdvertiserId());
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
  std::ifstream infile(SERVER_CFG_FILE_PATH,std::ios::binary);
  //check whether file exists
  if(!infile) {
    ALOGD(LOGTAG"Error opening file");
    return ;
  }

  while(!infile.eof()) {
    getline(infile,ch,'\r');
    if(std::regex_search(ch,std::regex("\\bServer[1-9]|Server[1-9][0-9]\\b"))) {
      while(line_num < desired_line) {
        getline(infile,ch,'\r');
        status = ParseServiceDetails(ch,line_num);
        if(!status) {
          fprintf(stdout,"Service Records are not consistent \n");
          break;
        }
        line_num++;
      }
    } else {
        fprintf(stdout,"Server Config File is incorrect \n");
        break;
      }
      line_num = 0;
    }
  fprintf(stdout,"File reading Done \n");
  //closing the file after reading
  infile.close();
}

bool GattsTest::ParseServiceDetails(string temp,int line_num)
{
  ALOGD(LOGTAG"%s",__FUNCTION__);
  int pos=0;
  int manuID;
  string manuData;
  bool status = false;
  static int rec = 0;
  if (!regex_search(temp,regex("\\bService[1-5]|ManufacturerId|ManufacturerData\\b"))) {
      fprintf(stdout,"record incorrect \n");
      return false;
  } else {
    pos = temp.find(":");
    temp = temp.substr(pos + 1);
    stringstream ss(temp);
    ss >> temp;
    if(line_num == MANUFACTURER_ID_LINE) {
        istringstream(temp) >> manuID;
        manufacturerId_list.push_back(manuID);
        return true;
    } else if(line_num == MANUFACTURER_DATA_LINE) {
      manufacturerData_list.push_back(temp);
      return true;
    } else if(line_num >= SERVICE_LINE_MIN && line_num <= SERVICE_LINE_MAX) {
        status = split(temp,',',service_field);
        if(status) {
          ParseServiceElement(line_num);
          service_field.clear();
          return true;
        } else {
          ALOGD(LOGTAG"Invalid service entry");
          return false;
        }
    }
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
  service_temp->c_property = INVALID_VALUE;
  service_temp->c_permissions = INVALID_VALUE;
  service_temp->d_permissions = INVALID_VALUE;
  int len = service_field.size();
  if(instance >= SERVICE_LINE_MIN && instance <= SERVICE_LINE_MAX) {
      if(service_field[0].empty()) {
        ALOGD(LOGTAG"Service details are empty");
        service_temp = NULL;
      } else {
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
        if(len >= 6) {
          istringstream(service_field[5]) >> permissions;
          service_temp->d_permissions = permissions;
        }
      }
      service_list[instance].push_back(service_temp);
  }
}

void GattsTest::AddServer()
{
  ALOGD(LOGTAG"%s",__FUNCTION__);
  int num_of_server;
  if(servInstanceMap.size() <= MAX_SERVER_INSTANCE) {
//  Add new server at first missing key on consecutive order
    for(num_of_server = 1; num_of_server <= servInstanceMap.size(); num_of_server++) {
      if(!servInstanceMap.count(num_of_server)) {
         break;
      }
    }
    fprintf(stdout,"Adding Server %d \n", num_of_server);
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
  if(!servInstanceMap.count(server_inst)) {
    fprintf(stdout,"Please create the server instance first \n");
    return false;
  } else if((service_inst <= 0) || (service_inst > MAX_SERVICE_INSTANCE) ) {
    fprintf(stdout,"Incorrect service instance values  \n" );
    return false;
  } else if (service_inst < 6) {
    mServer = servInstanceMap[server_inst];
    service_temp = service_list[service_inst - 1][server_inst -1];
    if(service_temp == NULL) {
      fprintf(stdout,"Service details are not available, service cannot be added \n");
      ALOGE(LOGTAG"Service details are not available, service cannot be added ");
      return false;
    } else {
      uid = service_temp->s_uuid;
      temp_UUID = Uuid::FromString(uid);
      mService  = new GattService(temp_UUID,GattService::SERVICE_TYPE_PRIMARY);
      uid = service_temp->c_uuid;
      if(!uid.empty()) {
        temp_UUID = Uuid::FromString(uid);
        property = service_temp->c_property;
        if(property == INVALID_VALUE) {
          property = 2;
        }
        permissions = service_temp->c_permissions;
        if(permissions == INVALID_VALUE) {
          permissions = 1;
        }
        AddCharacteristics(temp_UUID,property,permissions,char_val);
        uid = service_temp->d_uuid;
        if(!uid.empty()) {
          temp_UUID = Uuid::FromString(uid);
          permissions = service_temp->d_permissions;
          AddDescriptors(temp_UUID,permissions,desc_val);
          mgattCharacteristic->addDescriptor(mgattDescriptor);
        } else {
          ALOGD(LOGTAG"No descriptor added");
        }
        mService->addCharacteristic(mgattCharacteristic);
        } else {
        ALOGD(LOGTAG"No Characteristics added");
      }
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
      set_temp->tx_power = INVALID_VALUE;
      set_temp->legacyflag = INVALID_VALUE;
      set_temp->periodicflag = INVALID_VALUE;
      set_temp->connectableflag = INVALID_VALUE;
      set_temp->scannableflag = INVALID_VALUE;
      set_temp->anonymousflag = INVALID_VALUE;
      set_temp->includeTxPowerflag= INVALID_VALUE;
      set_temp->primary_phy= INVALID_VALUE;
      set_temp->secondary_phy= INVALID_VALUE;
      set_temp->interval = INVALID_VALUE;
      set_temp->timeout_legacy= INVALID_VALUE;
      set_temp->advertise_mode= INVALID_VALUE;
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
  ALOGD(LOGTAG"File reading done mAdvertiser %p ", madvertiser);
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
    set_temp->tx_power = parameter;
  } else if(regex_search(temp,regex("\\bLegacyFlag\\b"))) {
    set_temp->legacyflag = parameter;
  } else if(regex_search(temp,regex("\\bPeriodicFlag\\b"))) {
    set_temp->periodicflag = parameter;
  } else if(regex_search(temp,regex("\\bConnectableFlag\\b"))) {
    set_temp->connectableflag = parameter;
  } else if(regex_search(temp,regex("\\bScannableFlag\\b"))) {
    set_temp->scannableflag = parameter;
  } else if(regex_search(temp,regex("\\bAnonymousFlag\\b"))) {
    set_temp->anonymousflag = parameter;
  } else if(regex_search(temp,regex("\\bIncludePower\\b"))) {
    set_temp->includeTxPowerflag= parameter;
  } else if(regex_search(temp,regex("\\bPrimaryPhy\\b"))) {
    set_temp->primary_phy= parameter;
  } else if(regex_search(temp,regex("\\bSecondaryPhy\\b"))) {
    set_temp->secondary_phy= parameter;
  } else if(regex_search(temp,regex("\\bInterval\\b"))) {
    set_temp->interval = parameter;
  } else if(regex_search(temp,regex("\\bTimeOutLegacy\\b"))) {
    set_temp->timeout_legacy= parameter;
  } else if(regex_search(temp,regex("\\bAdvertiseMode\\b"))) {
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
  ALOGD(LOGTAG"%s",__FUNCTION__);
  AdvertiseSet *setParams;
  setParams = AdvSet_list[instance - 1];
  if(setParams == NULL) {
    ALOGE(LOGTAG"%s Advertising Configuration not found", __FUNCTION__);
    return false;
  }
  if (setParams->legacyflag < VALID_VALUE && setParams->connectableflag < VALID_VALUE &&
      setParams->scannableflag < VALID_VALUE && setParams->periodicflag < VALID_VALUE &&
      setParams->anonymousflag < VALID_VALUE && setParams->includeTxPowerflag < VALID_VALUE &&
      setParams->primary_phy < VALID_VALUE && setParams->secondary_phy < VALID_VALUE &&
      setParams->interval < VALID_VALUE && setParams->timeout_legacy < VALID_VALUE &&
      setParams->advertise_mode < VALID_VALUE) {
    fprintf(stdout,"Incorrect flag value \n");
    return false;
  }
  if (setParams->legacyflag > 1 && setParams->connectableflag > 1 &&
      setParams->scannableflag > 1 && setParams->periodicflag > 1 &&
      setParams->anonymousflag > 1 && setParams->includeTxPowerflag > 1) {
    fprintf(stdout,"Flags values set incorrectly, Please check 'legacyflag' 'connectableflag'\
    'scannableflag' periodicflag' 'anonymousflag' includetxpowerflag' \n");
    return false;
  }

  try {
    if(setParams->legacyflag) {
      ALOGD(LOGTAG" Legacy Advertising will be used \n");
      mAdvertiseSettings = AdvertiseSettings::Builder()
                           .setAdvertiseMode(setParams->advertise_mode)
                           .setTxPowerLevel(setParams->tx_power)
                           .setConnectable(setParams->connectableflag)
                           .setTimeout(setParams->timeout_legacy)
                           .build();

      ALOGD(LOGTAG"Advertising Settings connectable = %d \
            TxPowerLevel = %d AdvertiseMode =%d TimeOut = %d",
            mAdvertiseSettings->isConnectable(),
            mAdvertiseSettings->getTxPowerLevel(), mAdvertiseSettings->getMode(),
            mAdvertiseSettings->getTimeout());
    } else {
      mAdvertisingParameters = AdvertisingSetParameters::Builder()
                              .setConnectable(setParams->connectableflag)
                              .setScannable(setParams->scannableflag)
                              .setLegacyMode(setParams->legacyflag)
                              .setAnonymous(setParams->anonymousflag)
                              .setIncludeTxPower(setParams->includeTxPowerflag)
                              .setPrimaryPhy(setParams->primary_phy)
                              .setSecondaryPhy(setParams->secondary_phy)
                              .setInterval(setParams->interval)
                              .setTxPowerLevel(setParams->tx_power)
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
    temp= service_list[SERVICE_1][instance -1];
    if(!temp->s_uuid.empty()) {
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
  int periodic_interval = PERIODIC_INTERVAL;
  include_txpower = temp->includeTxPowerflag;

  if(periodic_flag) {
    mPeriodicParams = PeriodicAdvertiseParameters::Builder()
                      .setIncludeTxPower(include_txpower)
                      .setInterval(periodic_interval)
                      .build();

    ALOGD(LOGTAG"SetPeriodicAdvertisingParameters:: IncludeTxPower: %d interval %d",
              mPeriodicParams->getIncludeTxPower() ,mPeriodicParams->getInterval());
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
  if(!servInstanceMap.count(instanceId)) {
    fprintf(stdout,"Server instance value invalid, Please type a valid instance\n");
    return false;
  }
  if(servInstanceMap.size() <= MAX_SERVER_INSTANCE) {
    mServer = servInstanceMap[instanceId];
    mServer->close();
    if(!AdvSet_list.empty()){
      AdvertisingSetCallback *mAdvSetCB;
      mAdvSetCB = advCBInstanceMap[instanceId];
      madvertiser->stopAdvertising(mAdvSetCB);
    }
    unordered_map <gattstestServerCallback*,GattServer*> ::iterator itr;
    for(itr = servCBInstanceMap.begin(); itr!= servCBInstanceMap.end(); ++itr) {
      if(itr->second == mServer ){
         servCBInstanceMap.erase(itr->first);
         break;
       }
    }
    servInstanceMap.erase(instanceId);
    advCBInstanceMap.erase(instanceId);
    return true;
  } else {
    fprintf(stdout,"There are no more servers to unregister \n");
    return false;
  }
}

void GattsTest::StopAdvertisement(string instance)
{
  ALOGD(LOGTAG"StopAdvertisement \n");
  int instanceId;
  istringstream(instance) >> instanceId;
  if(!advCBInstanceMap.count(instanceId)) {
    fprintf(stdout,"Server instance value invalid, Please type a valid instance\n");
  } else {
    AdvertisingSetCallback *mAdvSetCB;
    mAdvSetCB = advCBInstanceMap[instanceId];
    madvertiser->stopAdvertising(mAdvSetCB);
  }
}


void GattsTest::AddCharacteristics(Uuid uid,int property, int permissions, string val)
{
  ALOGD(LOGTAG"%s",__FUNCTION__);
  mgattCharacteristic = new GattCharacteristic(uid,property,permissions);
  uint8_t char_val[val.length()+1];
  std::copy(val.begin(),val.end(),char_val);
  char_val[val.length()] = '\0';
  mgattCharacteristic->setValue((uint8_t*)char_val, (int)val.length()+1);
  ALOGD(LOGTAG"CharacteristicUUID: %s  ", uid.ToString().c_str());
  ALOGD(LOGTAG"Characteristic Property: %d ", mgattCharacteristic->getProperties());
  ALOGD(LOGTAG"characteristic Permissions: %d ", mgattCharacteristic->getPermissions());
  ALOGD(LOGTAG"characteristic value: %s", mgattCharacteristic->getValue());
}

void GattsTest::AddDescriptors(Uuid uid,int permissions,string value)
{
  ALOGD(LOGTAG"%s",__FUNCTION__);
  ALOGD(LOGTAG"string value =  %s", value.c_str());
  mgattDescriptor = new GattDescriptor(uid,permissions);
  uint8_t dsc_val[value.length()+1];
  std::copy(value.begin(),value.end(),dsc_val);
  dsc_val[value.length()] = '\0';
  mgattDescriptor->setValue(dsc_val, value.length()+1);
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

  if(!servInstanceMap.count(instanceId)) {
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
  if(!servInstanceMap.count(instanceId)) {
    fprintf(stdout,"Server instance value invalid, Please type a valid instance\n");
    return false;
  } else {
    if((tx_phy != PHY_LE_1M) && (tx_phy != PHY_LE_2M) && (tx_phy != PHY_LE_CODED)) {
      fprintf(stdout,"Enter a valid tx phy option \n");
      return false;
  }
  if((rx_phy != PHY_LE_1M) && (rx_phy != PHY_LE_2M) && (rx_phy != PHY_LE_CODED)) {
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
  g_gatt->unregAll();
  servInstanceMap.clear();
  unordered_map  <gattstestServerCallback*,GattServer*> ::iterator it;
  for(it = servCBInstanceMap.begin(); it != servCBInstanceMap.end(); ++it) {
    mServercallback = it->first;
    delete(mServercallback);
    mServer = it->second;
    mServer->close();
    delete(mServer);
  }
  servCBInstanceMap.clear();
  for(at = advCBInstanceMap.begin(); at != advCBInstanceMap.end(); ++at) {
    mAdvertisercallback = at->second;
    delete(mAdvertisercallback);
  }
  advCBInstanceMap.clear();
  return true;
}




