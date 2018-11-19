 /*
  * Copyright (c) 2016-2017, The Linux Foundation. All rights reserved.
  *
  * Redistribution and use in source and binary forms, with or without
  * modification, are permitted provided that the following conditions are
  * met:
  *  * Redistributions of source code must retain the above copyright
  *    notice, this list of conditions and the following disclaimer.
  *  * Redistributions in binary form must reproduce the above
  *    copyright notice, this list of conditions and the following
  *    disclaimer in the documentation and/or other materials provided
  *    with the distribution.
  *  * Neither the name of The Linux Foundation nor the names of its
  *    contributors may be used to endorse or promote products derived
  *    from this software without specific prior written permission.
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

#ifndef A2DP_SINK_SPLIT_APP_H//to do
#define A2DP_SINK_SPLIT_APP_H

#include <map>
#include <list>
#include <string>
#include <hardware/bluetooth.h>
#include <hardware/bt_av.h>
#include <pthread.h>

#include "osi/include/log.h"
#include "osi/include/thread.h"
#include "osi/include/config.h"
#include "osi/include/allocator.h"
#include "osi/include/alarm.h"
#include "ipc.hpp"
#include "utils.h"
#include "hardware/bt_av_vendor.h"
#include "A2dp_Sink.hpp"
#include "A2dp_Sink_Streaming.hpp"

using namespace std;
using std::list;
using std::string;

class A2dp_Sink_Split {

  private:
    config_t *config;
    const bt_interface_t * bluetooth_interface;
    A2dpSinkState mSinkState;

  public:
    A2dp_Sink_Split(const bt_interface_t *bt_interface, config_t *config);
    ~A2dp_Sink_Split();
    const btav_sink_vendor_interface_t *sBtA2dpSinkVendorInterface;
    const btav_sink_interface_t *sBtA2dpSinkInterface;
    void ProcessEvent(BtEvent* pEvent, list<A2dp_Device>::iterator iter);
    void state_disconnected_handler(BtEvent* pEvent, list<A2dp_Device>::iterator iter);
    void state_pending_handler(BtEvent* pEvent, list<A2dp_Device>::iterator iter);
    void state_connected_handler(BtEvent* pEvent, list<A2dp_Device>::iterator iter);
    void change_state(list<A2dp_Device>::iterator iter, A2dpSinkDeviceState mState);
    char* dump_message(BluetoothEventId event_id);
    pthread_mutex_t lock;
    uint32_t max_a2dp_conn;
    bt_bdaddr_t mStreamingDevice;
    bt_bdaddr_t mResumingDevice;
    bool start_pending;
    bool suspend_pending;
    bt_bdaddr_t mPendingDevice;
    void HandleEnableSink();
    void HandleDisableSink();
    void HandleSinkStreamingDisableDone();
    void ConnectionManager(BtEvent* pEvent, bt_bdaddr_t dev);
    void EventManager(BtEvent* pEvent, bt_bdaddr_t dev);
    bool isConnectionEvent(BluetoothEventId event_id);
    void UpdateSupportedCodecs(uint8_t num_codecs);
    list<A2dp_Device> pA2dpDeviceList;
    ControlStatusType controlStatus;
};

#endif
