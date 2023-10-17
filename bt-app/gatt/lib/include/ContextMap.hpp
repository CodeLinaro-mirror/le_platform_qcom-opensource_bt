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

#ifndef CONTEXT_MAP_HPP_
#define CONTEXT_MAP_HPP_
#pragma once

#include <unordered_set>
#include <vector>
#include <unordered_map>
#include <string>
#include <mutex>
#include "CallbackInfo.hpp"
#include "log.h"
#include <iostream>
#include <iterator>
#include <cmath>

#define LOG__TAG "ContextMap"

using namespace std;
using std::string;

namespace gatt {
class GattLibService;
/**
 * Helper class that keeps track of registered GATT applications.
 * This class manages application callbacks and keeps track of GATT connections.
 */
/*package*/
template<class C>
class ContextMap {

  public:
  /**
   * Connection class helps map connection IDs to device addresses.
   */
  class Connection {
    public:
      int connId;
      string address;
      int appId;
      long startTime;

      Connection(int connId, string address, int appId);
  };

  /**
   * Application entry mapping UUIDs to appIDs and callbacks.
   */
 class App {
    private:
        std::vector<CallbackInfo*> mCongestionQueue;

    public:
      /** The UUID of the application */
      Uuid uuid;

      /** The id of the application */
      int id;

      /** Application callbacks */
      C callback;

      /** Flag to signal that transport is congested */
      bool isCongested = false;

      /** Internal callback info queue, waiting to be send on congestion clear */

      /**
       * Creates a new app context.
       */
      App(Uuid uuid, C callback);

      void queueCallback(CallbackInfo *callbackInfo);

      CallbackInfo* popQueuedCallback();

      ~App();
  };

  private:

    /** Our internal application list */
    std::vector<App*> mApps;

    std::mutex mtx;
    std::mutex mtx1;

    static const int INVALID_INT = -1;

  public:

    /** Internal list of connected devices **/
    std::unordered_set<Connection*> mConnections;// HashSet;

    /**
     * Add an entry to the application context list.
     */
    App* add(Uuid uuid, C callback,  GattLibService *service);

    /**
     * Remove the context for a given UUID
     */
    void remove(Uuid uuid);

    /**
     * Remove the context for a given application ID.
     */
    void remove(int id);

    std::vector<int> getAllAppsIds();

    /**
     * Add a new connection for a given application ID.
     */
    void addConnection(int id, int connId, string address);

    /**
     *  Remove a connection with the given ID.
     */
    void removeConnection(int id, int connId);

    /**
     * Remove all connections for a given application ID.
     */
    void removeConnectionsByAppId(int appId);

    /**
     * Get an application context by ID.
     */
    App* getById(int id);

    /**
     * Get an application context by UUID.
     */
    App* getByUuid(Uuid uuid);

    /**
     * Get the device addresses for all connected devices
     */
    std::unordered_set<string> getConnectedDevices();

    /**
     * Get an application context by a connection ID.
     */
    App* getByConnId(int connId);

    /**
     * Returns a connection ID for a given device address.
     */
    int connIdByAddress(int id, string address);

    /**
     *Returns the device address for a given connection ID.
     */
    string addressByConnId(int connId);

    std::vector<Connection*> getConnectionByApp(int appId);

    /**
     * Erases all application context entries.
     */
    void clear();

    /**
     * Returns connect device map with addr and appid
     */
    std::unordered_map<int, string> getConnectedMap();
};
#include "ContextMap_Impl.hpp"
}
#endif
