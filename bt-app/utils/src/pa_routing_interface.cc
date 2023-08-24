/*
* Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
* SPDX-License-Identifier: BSD-3-Clause-Clear
*/

#include <stdio.h>
#include <stdlib.h>
#include <dlfcn.h>
#include "osi/include/log.h"
#include "pa_routing_interface.h"

#define LOGTAG "PA_ROUTING_INTF "

void *dl_handle = NULL;

pa_routing_interface_t* pa_routing_intf_open(void)
{
    int ret = 0;
    pa_routing_interface_t *pa_routing_intf_p = NULL;

    dl_handle = dlopen(PA_BT_CLIENT_WRAPPER_LIB, RTLD_NOW);
    if (dl_handle) {
        ALOGD(LOGTAG "PA BT client libarary open sucessful");
    } else {
        ALOGE(LOGTAG "Unable to open PA BT client libarary");
        fprintf(stdout, "Unable to open PA BT client libarary\n");
        return NULL;
    }

    pa_routing_intf_p = (pa_routing_interface_t*) calloc(1, sizeof(pa_routing_interface_t));
    if (!pa_routing_intf_p) {
        ALOGE(LOGTAG "Insufficient memory");
        ret = -1;
        goto quit;
    }
    pa_routing_intf_p->pa_bt_connect_fn = (pa_bt_connect_fn_t) dlsym(dl_handle, "pa_bt_connect");
    if (!pa_routing_intf_p->pa_bt_connect_fn) {
        ALOGE(LOGTAG "dlsym failed for pa_bt_connect %s", dlerror());
        ret = -1;
        goto quit;
    }

    pa_routing_intf_p->pa_bt_set_param_fn = (pa_bt_set_param_fn_t) dlsym(dl_handle, "pa_bt_set_param");
    if (!pa_routing_intf_p->pa_bt_set_param_fn) {
        ALOGE(LOGTAG "dlsym failed for pa_bt_set_param %s", dlerror());
        ret = -1;
        goto quit;
    }

    pa_routing_intf_p->pa_bt_get_param_fn = (pa_bt_get_param_fn_t) dlsym(dl_handle, "pa_bt_get_param");
    if (!pa_routing_intf_p->pa_bt_get_param_fn) {
        ALOGE(LOGTAG "dlsym failed for pa_bt_get_param %s", dlerror());
        ret = -1;
        goto quit;
    }

    pa_routing_intf_p->pa_sink_init_fn = (pa_sink_init_fn_t) dlsym(dl_handle, "pa_sink_init");
    if (!pa_routing_intf_p->pa_sink_init_fn) {
        ALOGE(LOGTAG "dlsym failed for pa_sink_init %s", dlerror());
        ret = -1;
        goto quit;
    }

    pa_routing_intf_p->pa_sink_play_fn = (pa_sink_play_fn_t) dlsym(dl_handle, "pa_sink_play");
    if (!pa_routing_intf_p->pa_sink_play_fn) {
        ALOGE(LOGTAG "dlsym failed for pa_sink_play %s", dlerror());
        ret = -1;
        goto quit;
    }

    pa_routing_intf_p->pa_sink_deinit_fn = (pa_sink_deinit_fn_t) dlsym(dl_handle, "pa_sink_deinit");
    if (!pa_routing_intf_p->pa_sink_deinit_fn) {
        ALOGE(LOGTAG "dlsym failed for pa_sink_deinit %s", dlerror());
        ret = -1;
        goto quit;
    }

    return pa_routing_intf_p;

quit:
    if (pa_routing_intf_p)
        free(pa_routing_intf_p);

    dlclose(dl_handle);

    return NULL;
}

void pa_routing_intf_close(pa_routing_interface_t *pa_routing_intf_p)
{
    if (pa_routing_intf_p)
        free(pa_routing_intf_p);

    if (dl_handle)
        dlclose(dl_handle);
}
