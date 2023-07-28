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

template<class C>
ContextMap<C>::Connection::Connection(int conn_Id, string address, int appId) {
  this->connId = conn_Id;
  this->address = address;
  this->appId = appId;
}

template<class C>
ContextMap<C>::App::App(Uuid uuid, C callback) {
  this->uuid = uuid;
  this->callback = callback;
}

template<class C>
ContextMap<C>::App::~App() {
  for(CallbackInfo *cbinfo : mCongestionQueue) {
    delete(cbinfo);
  }
  mCongestionQueue.clear();
}

template<class C>
void ContextMap<C>::App::queueCallback(CallbackInfo *callbackInfo) {
  mCongestionQueue.push_back(callbackInfo);
}

template<class C>
CallbackInfo* ContextMap<C>::App::popQueuedCallback() {
  if (mCongestionQueue.size() == 0) {
    return NULL;
  }
  CallbackInfo *val = mCongestionQueue.at(0);
  mCongestionQueue.erase(mCongestionQueue.begin());
  return val;
}

template<class C>
typename ContextMap<C>::App* ContextMap<C>::add(Uuid uuid, C callback, GattLibService *service) {
  App *app = new App(uuid, callback);
  mApps.push_back(app);
  return app;
}

template<class C>
void ContextMap<C>::remove(Uuid uuid) {
  for (typename std::vector<App*>::iterator it = mApps.begin() ; it != mApps.end(); ++it) {
    App *entry = *it;
    if (entry->uuid == uuid) {
      mApps.erase(it);
      break;
    }
  }

}

template<class C>
void ContextMap<C>::remove(int id) {
  for (typename std::vector<App*>::iterator it = mApps.begin() ; it != mApps.end(); ++it) {
   App *entry = *it;
   if (entry->id == id) {
     removeConnectionsByAppId(id);
     mApps.erase(it);
     break;
   }
  }
}

template<class C>
std::vector<int> ContextMap<C>::getAllAppsIds() {
  std::vector<int> appIds;
  for (typename std::vector<App*>::iterator it = mApps.begin() ; it != mApps.end(); ++it) {
    App *entry = *it;
    appIds.push_back(entry->id);
  }
  return appIds;
}

template<class C>
void ContextMap<C>::addConnection(int id, int connId, string address) {
  App *entry = getById(id);
  if (entry != NULL) {
    mConnections.insert(new Connection(connId, address, id));
  }
}

template<class C>
void ContextMap<C>::removeConnection(int id, int connId) {
  for (typename std::unordered_set<Connection*>::iterator it = mConnections.begin() ;
                          it != mConnections.end(); ++it) {
    Connection *connection = *it;
    if (connection->connId == connId) {
      mConnections.erase(it);
      delete(connection);
      break;
    }
  }
}

template<class C>
void ContextMap<C>::removeConnectionsByAppId(int appId) {
  for (typename std::unordered_set<Connection*>::iterator it = mConnections.begin();
                                          it != mConnections.end(); ++it) {
    Connection *connection = *it;
    if (connection->appId == appId) {
      mConnections.erase(it);
      delete(connection);
      break;
    }
  }
}

template<class C>
typename ContextMap<C>::App* ContextMap<C>::getById(int id) {
  for (typename std::vector<App*>::iterator it = mApps.begin() ; it != mApps.end(); ++it) {
    App *entry = *it;
    if (entry->id == id) {
      return entry;
    }
  }
  ALOGE(LOG__TAG " Context not found for ID %d", id);
  return NULL;
}

template<class C>
typename ContextMap<C>::App* ContextMap<C>::getByUuid(Uuid uuid) {
 for (typename std::vector<App*>::iterator it = mApps.begin() ; it != mApps.end(); ++it) {
   App *entry = *it;
   if (entry->uuid == uuid) {
       return entry;
   }
 }
 ALOGE(LOG__TAG " Context not found for ID %s", uuid.ToString().c_str());
 return NULL;
}

template<class C>
std::unordered_set<string> ContextMap<C>::getConnectedDevices() {
  std::unordered_set<string> addresses;
  for (typename std::unordered_set<Connection*>::iterator it = mConnections.begin() ;
                                    it != mConnections.end(); ++it) {
    Connection *connection = *it;
    addresses.insert(connection->address);
  }
  return addresses;
}

template<class C>
typename ContextMap<C>::App* ContextMap<C>::getByConnId(int connId) {
  for (typename std::unordered_set<Connection*>::iterator it = mConnections.begin() ;
                          it != mConnections.end(); ++it) {
    Connection *connection = *it;
    if (connection->connId == connId) {
      return getById(connection->appId);
    }
  }
  return NULL;
}

template<class C>
int ContextMap<C>::connIdByAddress(int id, string address) {
  App *entry = getById(id);
  if (entry == NULL) {
    return INVALID_INT;
  }

  for (typename std::unordered_set<Connection*>::iterator it = mConnections.begin() ;
                it != mConnections.end(); ++it) {
    Connection *connection = *it;
    if (connection->address == address && connection->appId == id) {
      return connection->connId;
    }
  }
  return INVALID_INT;
}

template<class C>
string ContextMap<C>::addressByConnId(int connId) {
  for (typename std::unordered_set<Connection*>::iterator it = mConnections.begin() ;
                    it != mConnections.end(); ++it) {
    Connection *connection = *it;
    if (connection->connId == connId) {
      return connection->address;
    }
  }
  return "";
}

template<class C>
typename std::vector<typename ContextMap<C>::Connection*>
                              ContextMap<C>::getConnectionByApp(int appId) {
  typename std::vector<Connection> currentConnections();
  for (typename std::unordered_set<Connection*>::iterator it = mConnections.begin() ;
          it != mConnections.end(); ++it) {
    Connection *connection = *it;
    if (connection->appId == appId) {
      currentConnections.push_back(connection);
    }
  }
  return currentConnections;
}

template<class C>
void ContextMap<C>::clear() {
  for (typename std::vector<App*>::iterator it = mApps.begin() ; it != mApps.end(); ++it) {
    delete(*it);
  }
  mApps.clear();

  for (typename std::unordered_set<Connection*>::iterator it = mConnections.begin() ;
          it != mConnections.end(); ++it)
    delete(*it);
  mConnections.clear();
}

template<class C>
std::unordered_map<int, string> ContextMap<C>::getConnectedMap() {
  std::unordered_map<int, string> connectedmap;
  for (Connection *conn : mConnections) {
    connectedmap.insert({{conn->appId, conn->address}});
  }
  return connectedmap;
}
