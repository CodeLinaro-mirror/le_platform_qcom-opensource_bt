/*
* Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
* SPDX-License-Identifier: BSD-3-Clause-Clear 
*/

#ifndef _PA_ROUTING_INTERFACE_
#define _PA_ROUTING_INTERFACE_

#include "pa_bt_audio_client_wrapper.h"

#ifdef __cplusplus
extern "C"
{
#endif

/* Lib function handles */
typedef struct {
    pa_bt_connect_fn_t pa_bt_connect_fn;
    pa_bt_set_param_fn_t pa_bt_set_param_fn;
    pa_bt_get_param_fn_t pa_bt_get_param_fn;
    pa_sink_init_fn_t pa_sink_init_fn;
    pa_sink_play_fn_t pa_sink_play_fn;
    pa_sink_deinit_fn_t pa_sink_deinit_fn;
} pa_routing_interface_t;

/* Interface lib open/close functions */
pa_routing_interface_t* pa_routing_intf_open(void);
void pa_routing_intf_close(pa_routing_interface_t *pa_routing_intf);

#ifdef __cplusplus
}
#endif
#endif //_PA_ROUTING_INTERFACE_
