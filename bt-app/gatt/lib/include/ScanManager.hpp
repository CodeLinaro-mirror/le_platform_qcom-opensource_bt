/*
 * Copyright (c) 2018, The Linux Foundation. All rights reserved.
 * Not a Contribution.
 * Copyright (C) 2014 The Android Open Source Project
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

#ifndef SCAN_MANAGER_HPP_
#define SCAN_MANAGER_HPP_

#pragma once
#include "GattNativeInterfaceV2.hpp"
#include "FilterParams.hpp"
#include "ScanClient.hpp"
#include "ScanCallback.hpp"
#include "ScanSettings.hpp"
#include "ScanFilterQueue.hpp"
#include "GattDevice.hpp"
#include "GattLibService.hpp"
#include <deque>
#include <climits>
#include <cmath>
#include <mutex>
#include <condition_variable>
#include <unordered_set>
#include <unordered_map>
#include <vector>
#include <utility>
#include "utils/Log.h"
#include "hardware/bt_common_types.h"
#include <hardware/bluetooth.h>
#include "utils/include/uuid.h"
#include "osi/include/alarm.h"

using namespace std;
using std::string;
using btapp::Uuid;
namespace gatt{
class GattLibService;

/**
 * Class that handles Bluetooth LE scan related operations.
 *
 * @hide
 */

class ScanManager {
  public:
    std::mutex lock;
    std::condition_variable cv;
    bool countDown = false;
    /**
     * Parameters for batch scans.
     */
    class BatchScanParams {
      public:
        int scanMode;
        int fullScanscannerId;
        int truncatedScanscannerId;

        BatchScanParams();
        bool operator!=(const BatchScanParams& rhs) const;
        bool operator==(const BatchScanParams& obj) const;
    };

  private:
   class PhyInfo {
      public:
        int scanPhy;
        int scanModeLE1M;
        int scanModeLECoded;

        PhyInfo(int scanPhy, int scanModeLE1M, int scanModeLECoded);
    };
    class ScanNative {
      private:

        // Delivery mode defined in bt stack.
        static const int DELIVERY_MODE_IMMEDIATE = 0;
        static const int DELIVERY_MODE_ON_FOUND_LOST = 1;
        static const int DELIVERY_MODE_BATCH = 2;
        static const int DELIVERY_MODE_ROUTE = 8;

        static const int ONFOUND_SIGHTINGS_AGGRESSIVE = 1;
        static const int ONFOUND_SIGHTINGS_STICKY = 4;

        static const int ALL_PASS_FILTER_INDEX_REGULAR_SCAN = 1;
        static const int ALL_PASS_FILTER_INDEX_BATCH_SCAN = 2;
        static const int ALL_PASS_FILTER_SELECTION = 0;

        static const int DISCARD_OLDEST_WHEN_BUFFER_FULL = 0;

        /**
         * Scan params corresponding to regular scan setting
         */
        static const int SCAN_MODE_LOW_POWER_WINDOW_MS = 140;
        static const int SCAN_MODE_LOW_POWER_INTERVAL_MS = 1400;
        static const int SCAN_MODE_BALANCED_WINDOW_MS = 183;
        static const int SCAN_MODE_BALANCED_INTERVAL_MS = 730;
        static const int SCAN_MODE_LOW_LATENCY_WINDOW_MS = 100;
        static const int SCAN_MODE_LOW_LATENCY_INTERVAL_MS = 100;

        /**
         * Onfound/onlost for scan settings
         */
        static const int MATCH_MODE_AGGRESSIVE_TIMEOUT_FACTOR = (1);
        static const int MATCH_MODE_STICKY_TIMEOUT_FACTOR = (3);
        static const int ONLOST_FACTOR = 2;
        static const int ONLOST_ONFOUND_BASE_TIMEOUT_MS = 500;

        /**
         * Scan params corresponding to batch scan setting
         */
        static const int SCAN_MODE_BATCH_LOW_POWER_WINDOW_MS = 1500;
        static const int SCAN_MODE_BATCH_LOW_POWER_INTERVAL_MS = 150000;
        static const int SCAN_MODE_BATCH_BALANCED_WINDOW_MS = 1500;
        static const int SCAN_MODE_BATCH_BALANCED_INTERVAL_MS = 15000;
        static const int SCAN_MODE_BATCH_LOW_LATENCY_WINDOW_MS = 1500;
        static const int SCAN_MODE_BATCH_LOW_LATENCY_INTERVAL_MS = 5000;

        // The logic is AND for each filter field.
        static const int LIST_LOGIC_TYPE = 0x1111111;
        static const int FILTER_LOGIC_TYPE = 1;
        // Filter indices that are available to user. It's sad we need to maintain filter index.
        std::deque<int> mFilterIndexStack;
        // Map of scannerId and Filter indices used by client.
        unordered_map<int, std::deque<int>> mClientFilterIndexMap;
        // Keep track of the clients that uses ALL_PASS filters.
        std::unordered_set<int> mAllPassRegularClients;
        std::unordered_set<int> mAllPassBatchClients;

        std::mutex trackAdvLock;

        ScanManager *sManager;
        GattDevice *mGattDevice;

        int getScanPhyMask(int phy);

      public:
        PhyInfo* getPhyInfo(std::unordered_set<ScanClient*> cList);
        int numRegularScanClients();
        bool isExemptFromScanDowngrade(ScanClient *client);
        bool isOpportunisticScanClient(ScanClient *client);
        bool isFirstMatchScanClient(ScanClient *client);
        bool isRoutingScanClient(ScanClient *client);
        void resetBatchScan(ScanClient *client);
        int getFullScanStoragePercent(int resultType);
        BatchScanParams* getBatchScanParams();
        int getBatchScanWindowMillis(int scanMode);
        int getBatchScanIntervalMillis(int scanMode);
        static void batchScanTimeoutCb(void *context);
        /** Set the batch alarm to be triggered within a short window after batch interval. This
        * allows system to optimize wake up time while still allows a degree of precise control.
        */
        void setBatchAlarm();
        long getBatchTriggerIntervalMillis();

        /** Add scan filters. The logic is:
        * If no offload filter can/needs to be set, set ALL_PASS filter.
        * Otherwise offload all filters to hardware and enable all filters.
        */
        void configureScanFilters(ScanClient *client);

        /** Check whether the filter should be added to controller.
        * Note only on ALL_PASS filter should be added.
        */
        bool shouldAddAllPassFilterToController(ScanClient *client, int deliveryMode);
        void removeScanFilters(int scannerId);
        void removeFilterIfExisits(std::unordered_set<int>clients, int scannerId, int filterIndex);
        ScanClient* getBatchScanClient(int scannerId);

        /**
         * Return batch scan result type value defined in bt stack.
         */
        int getResultType(BatchScanParams *params);

        // Check if ALL_PASS filter should be used for the client.
        bool shouldUseAllPassFilter(ScanClient *client);

        void initFilterIndexStack();

        /**
        * Configure filter parameters.
        */
        void configureFilterParamter(int scannerId, ScanClient *client, int featureSelection,
                int filterIndex, int numOfTrackingEntries);

        /**
        * Get delivery mode based on scan settings.
        */
        int getDeliveryMode(ScanClient *client);

        int getScanWindowMillis(ScanSettings *settings);
        int getScanIntervalMillis(ScanSettings *settings);
        int getOnFoundOnLostTimeoutMillis(ScanSettings *settings, bool onFound);
        int getOnFoundOnLostSightings(ScanSettings *settings);
        int getNumOfTrackingAdvertisements(ScanSettings *settings);
        bool manageAllocationOfTrackingAdvertisement(int numOfTrackableAdvertisement,
                bool allocate);

        ScanNative(ScanManager *scanManager);
        ~ScanNative();
        void configureRegularScanParams();
        ScanClient* getAggressiveClient(std::unordered_set<ScanClient*> cList);
        bool startRegularScan(ScanClient *client);
        void startBatchScan(ScanClient *client);
        void stopRegularScan(ScanClient *client);
        void regularScanTimeout(ScanClient *client);
        void setOpportunisticScanClient(ScanClient *client);

        /**
        * Find the regular scan client information.
        */
        ScanClient* getRegularScanClient(int scannerId);
        void stopBatchScan(ScanClient *client);
        void flushBatchResults(int scannerId);
        void cleanup();
        std::vector<apcf_command_t> parseScanFilterToApcfCommand
                                      (std::vector<ScanFilterQueue::Entry*> entries);
    };

    static const bool DBG = true;
    static const int MICROS_PER_UNIT = 625;
    static const string ACTION_REFRESH_BATCHED_SCAN;

    // Timeout for each controller operation.
    static const int OPERATION_TIME_OUT_MILLIS = 500;

    int mLastConfiguredScanSettingLE1M = INT_MIN;
    int mLastConfiguredScanSettingLECoded = INT_MIN;
    // Scan parameters for batch scan.
    BatchScanParams *mBatchScanParms;
    int mCurUsedTrackableAdvertisements;
    GattNativeInterfaceV2 *mNative;
    bool mBatchAlarmReceiverRegistered;
    ScanNative *mScanNative;

    std::unordered_set<ScanClient*> mRegularScanClients;
    std::unordered_set<ScanClient*> mBatchClients;

    bool isFilteringSupported();
    bool isBatchClient(ScanClient *client);
    bool isScanSupported(ScanClient *client);
    void resetCountDownLatch();
    void waitForCallback();
    void timeoutCb();
    alarm_t* batch_scan_timer;

  public:
    // Result type defined in bt stack. Need to be accessed by GattService.
    static const int SCAN_RESULT_TYPE_TRUNCATED = 1;
    static const int SCAN_RESULT_TYPE_FULL = 2;
    static const int SCAN_RESULT_TYPE_BOTH = 3;

    ScanManager(GattNativeInterfaceV2 *mGattIf);
    ~ScanManager();
    void start();
    void cleanup();
    void registerScanner(Uuid uuid);
    void unregisterScanner(int scannerId);

    /**
     * Returns the regular scan queue.
     */
    std::unordered_set<ScanClient*> getRegularScanQueue();

    /**
     * Returns batch scan queue.
     */
    std::unordered_set<ScanClient*> getBatchScanQueue();

    /**
     * Returns a set of full batch scan clients.
     */
    std::unordered_set<ScanClient*> getFullBatchScanQueue();

    void startScan(ScanClient *client);
    void stopScan(ScanClient *client);
    void flushBatchScanResults(ScanClient *client);
    void callbackDone(int scannerId, int status);
    static int millsToUnit(int milliseconds);
    int getCurrentUsedTrackingAdvertisement();
    void handleStartScan(ScanClient *client);
    void handleStopScan(ScanClient *client);
    void handleFlushBatchResults(ScanClient *client);
    void handleSuspendScans();
    void handleSuspendScanAll();
    void handleResumeScans();

};
}
#endif
