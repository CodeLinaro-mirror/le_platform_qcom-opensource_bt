/******************************************************************************
 *
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

#pragma once
#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <unistd.h>

#ifdef __cplusplus
extern "C" {
#endif

#define TAG "bt_app"

#define PRI_INFO " I"
#define PRI_WARN " W"
#define PRI_ERROR " E"
#define PRI_DEBUG " D"
#define PRI_VERB " V"

//#define ALOG(pri, tag, fmt, arg...) fprintf(stderr, tag " : %s " fmt  " : %s : %d "  pri ": \n", __TIME__ , __func__, __LINE__, ##arg)
#define ALOG(pri, tag, fmt, arg...) fprintf(stderr, tag ": %s " ": %s "  ": %d " pri": " fmt"\n", __TIME__ , __func__, __LINE__, ##arg)
#define ALOGV(fmt, arg...) ALOG(PRI_VERB, TAG, fmt, ##arg)
#define ALOGD(fmt, arg...) ALOG(PRI_DEBUG, TAG, fmt, ##arg)
#define ALOGI(fmt, arg...) ALOG(PRI_INFO, TAG, fmt, ##arg)
#define ALOGW(fmt, arg...) ALOG(PRI_WARN, TAG, fmt, ##arg)
#define ALOGE(fmt, arg...) ALOG(PRI_ERROR, TAG, fmt, ##arg)

#define LOG_VERBOSE(fmt, arg...) ALOG(PRI_VERB, TAG, fmt, ##arg)
#define LOG_DEBUG(fmt, arg...) ALOG(PRI_DEBUG, TAG, fmt, ##arg)
#define LOG_INFO(fmt, arg...) ALOG(PRI_INFO, TAG, fmt, ##arg)
#define LOG_WARN(fmt, arg...) ALOG(PRI_WARN, TAG, fmt, ##arg)
#define LOG_ERROR(fmt, arg...) ALOG(PRI_ERROR, TAG, fmt, ##arg)

#ifdef __cplusplus
}
#endif
