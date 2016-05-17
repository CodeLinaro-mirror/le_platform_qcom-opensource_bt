/******************************************************************************
 *
 *  Copyright (c) 2016, The Linux Foundation. All rights reserved.
 *  Not a Contribution.
 *  Copyright (C) 2014 Google, Inc.
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at:
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 *
 ******************************************************************************/

#include "ipc.h"
#include "osi/include/thread.h"

thread_t *g_gap_thread = NULL;
thread_t *g_main_thread = NULL;

void PostMessage(ThreadIdType thread_id, void *msg) {
    if(thread_id == THREAD_ID_GAP) {
        if(g_gap_thread)
            thread_post(g_gap_thread, BtGapMsgHandler, msg);
    } else if(thread_id == THREAD_ID_MAIN) {
        if(g_main_thread)
            thread_post(g_main_thread, BtMainMsgHandler, msg);
    }
}

const char *BdAddr2Str(const bt_bdaddr_t *bd_addr, char *bd_str) {

    if((bd_addr == NULL) || (bd_str == NULL))
        return NULL;

    const char *bd_ptr = bd_addr->address;
    snprintf(bd_str, MAX_BD_STR_LEN, "%02x:%02x:%02x:%02x:%02x:%02x",
            bd_ptr[0], bd_ptr[1], bd_ptr[2],
            bd_ptr[3], bd_ptr[4], bd_ptr[5]);
    return bd_str;
}
