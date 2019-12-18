/*
 Copyright (c) 2019, The Linux Foundation. All rights reserved.
 
 Redistribution and use in source and binary forms, with or without
 modification, are permitted provided that the following conditions are
 met:
	 * Redistributions of source code must retain the above copyright
	   notice, this list of conditions and the following disclaimer.
	 * Redistributions in binary form must reproduce the above
	   copyright notice, this list of conditions and the following
	   disclaimer in the documentation and/or other materials provided
	   with the distribution.
	 * Neither the name of The Linux Foundation nor the names of its
	   contributors may be used to endorse or promote products derived
	   from this software without specific prior written permission.
 
 THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESS OR IMPLIED
 WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT
 ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS
 BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
 BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

 */

#include <map>
#include <string>
#include <vector>
#include <array>
#include <memory>
#include <poll.h>
#include <iostream>
#include <fstream>
#include <stdio.h>
#include <pthread.h>
#include <systemdq/sd-bus.h>
#include <sys/eventfd.h>
#include <hardware/bluetooth.h>
#include <hardware/hardware.h>
#include <hardware/bt_hf.h>
#include "hardware/bt_hf_vendor.h"
#include "utils/Log.h"

using std::string;

#define LOGTAG "hfpag_b" // Log prefix

#define DBUS_B_SVC_NAME   "com.qualcomm.qti.adk.btipc.app"
#define DBUS_B_IF_NAME    "com.qualcomm.qti.adk.btipc.app.hfpag"
#define DBUS_B_OBJ_PATH   "/com/qualcomm/qti/adk/btipc/app/hfpag"
#define DBUS_B_SVC_NAME_SENDER   "com.qualcomm.qti.adk.btipc.app.hfpag.sender"

#define DBUS_A_SVC_NAME   "com.qualcomm.qti.adk.btipc.daemon"
#define DBUS_A_IF_NAME    "com.qualcomm.qti.adk.btipc.daemon.hfpag"
#define DBUS_A_OBJ_PATH   "/com/qualcomm/qti/adk/btipc/daemon/hfpag"
#define DBUS_A_SVC_NAME_SENDER   "com.qualcomm.qti.adk.btipc.daemon.hfpag.sender"

#define CHECK_DBUS_CALL_RESULT(rtn, m)  {                                                     \
  ALOGD(LOGTAG "::%s call finished!!", __func__);                                             \
  if( rtn < 0 ) {                                                                             \
    ALOGE(LOGTAG "::%s Failed to call method : %d - %s", __func__, -rtn, strerror(-rtn));     \
  }                                                                                           \
  if( m != NULL ) sd_bus_message_unref(m);                                                  \
}

#define DBUS_SESSION_FILE "/tmp/dbus-session"

/*
bthf_interface_t
sBtHfpAgInterface = (bthf_interface_t *)bluetooth_interface->
		get_profile_interface(BT_PROFILE_HANDSFREE_ID);
*/
static bt_status_t bthf_init (bthf_callbacks_t* callbacks, int max_hf_clients, bool inband_ringing_supported);
static bt_status_t bthf_connect (RawAddress* bd_addr);
static bt_status_t bthf_disconnect (RawAddress* bd_addr);
static bt_status_t bthf_connect_audio (RawAddress* bd_addr);
static bt_status_t bthf_disconnect_audio (RawAddress* bd_addr);
static bt_status_t bthf_start_voice_recognition (RawAddress* bd_addr);
static bt_status_t bthf_stop_voice_recognition (RawAddress* bd_addr);
static bt_status_t bthf_volume_control (bthf_volume_type_t type, int volume,
                            RawAddress* bd_addr);
static bt_status_t bthf_device_status_notification (bthf_network_state_t ntk_state,
                                        bthf_service_type_t svc_type,
                                        int signal, int batt_chg, RawAddress* bd_addr);
static bt_status_t bthf_cops_response (const char* cops, RawAddress* bd_addr);
static bt_status_t bthf_cind_response (int svc, int num_active, int num_held,
                           bthf_call_state_t call_setup_state, int signal,
                           int roam, int batt_chg, RawAddress* bd_addr);
static bt_status_t bthf_formatted_at_response (const char* rsp, RawAddress* bd_addr);
static bt_status_t bthf_at_response (bthf_at_response_t response_code, int error_code,
                         RawAddress* bd_addr);
static bt_status_t bthf_clcc_response (int index, bthf_call_direction_t dir,
                           bthf_call_state_t state, bthf_call_mode_t mode,
                           bthf_call_mpty_type_t mpty, const char* number,
                           bthf_call_addrtype_t type, RawAddress* bd_addr);
static bt_status_t bthf_phone_state_change (int num_active, int num_held,
                                bthf_call_state_t call_setup_state,
                                const char* number,
                                bthf_call_addrtype_t type, RawAddress* bd_addr);
static void bthf_cleanup (void);
static bt_status_t bthf_set_sco_allowed (bool value);
static bt_status_t bthf_send_bsir(bool value, RawAddress* bd_addr);
static bt_status_t bthf_set_active_device(RawAddress* active_device_addr);

bthf_interface_t bthf_interface_b =
{
	sizeof(bthf_interface_b),
    bthf_init,
    bthf_connect,
    bthf_disconnect,
    bthf_connect_audio,
    bthf_disconnect_audio,
    bthf_start_voice_recognition,
    bthf_stop_voice_recognition,
    bthf_volume_control,
    bthf_device_status_notification,
    bthf_cops_response,
    bthf_cind_response,
    bthf_formatted_at_response,
    bthf_at_response,
    bthf_clcc_response,
    bthf_phone_state_change,
    bthf_cleanup,
    bthf_set_sco_allowed,
    bthf_send_bsir,
    bthf_set_active_device
};

static bthf_callbacks_t *bthf_callbacks;

/*sdbus*/
extern sd_bus *g_sdbus;
static sd_bus *g_sdbus_call = nullptr;

/**/
static RawAddress str2addr(string address) {
  RawAddress bd_addr;
  RawAddress::FromString(std::string(address), bd_addr);
  return bd_addr;
}

/**/
static bt_status_t bthf_init (bthf_callbacks_t* callbacks, int max_hf_clients, bool inband_ringing_supported)
{
	bthf_callbacks = callbacks;
	//
	ALOGD(LOGTAG "::%s",__func__);
	int rtn;
	sd_bus_message *m = NULL;
	rtn = sd_bus_call_method(g_sdbus_call,
		   DBUS_A_SVC_NAME,			  /* service to contact */
		   DBUS_A_OBJ_PATH, 		  /* object path */
		   DBUS_A_IF_NAME,		      /* interface name */
		   __func__,					  /* method name */
		   NULL,						  /* object to return error in */
		   &m,							  /* return message on success */
		   "ii",						  /* input signature */
		   max_hf_clients,						  /* parameters */
		   inband_ringing_supported?1:0);			  
	CHECK_DBUS_CALL_RESULT( rtn, m );
	return BT_STATUS_SUCCESS;
}
static bt_status_t bthf_connect (RawAddress* bd_addr)
{
	//
	ALOGD(LOGTAG "::%s",__func__);
	int rtn;
	sd_bus_message *m = NULL;
	rtn = sd_bus_call_method(g_sdbus_call,
		   DBUS_A_SVC_NAME,			  /* service to contact */
		   DBUS_A_OBJ_PATH, 		  /* object path */
		   DBUS_A_IF_NAME,		      /* interface name */
		   __func__,					  /* method name */
		   NULL,						  /* object to return error in */
		   &m,							  /* return message on success */
		   "s",						  /* input signature */
		   bd_addr->ToString().c_str());			  
	CHECK_DBUS_CALL_RESULT( rtn, m );
	return BT_STATUS_SUCCESS;
}
static bt_status_t bthf_disconnect (RawAddress* bd_addr)
{
	//
	ALOGD(LOGTAG "::%s",__func__);
	int rtn;
	sd_bus_message *m = NULL;
	rtn = sd_bus_call_method(g_sdbus_call,
		   DBUS_A_SVC_NAME,			  /* service to contact */
		   DBUS_A_OBJ_PATH, 		  /* object path */
		   DBUS_A_IF_NAME,		      /* interface name */
		   __func__,					  /* method name */
		   NULL,						  /* object to return error in */
		   &m,							  /* return message on success */
		   "s",						  /* input signature */
		   bd_addr->ToString().c_str());			  
	CHECK_DBUS_CALL_RESULT( rtn, m );
	return BT_STATUS_SUCCESS;
}
static bt_status_t bthf_connect_audio (RawAddress* bd_addr)
{
	//
	ALOGD(LOGTAG "::%s",__func__);
	int rtn;
	sd_bus_message *m = NULL;
	rtn = sd_bus_call_method(g_sdbus_call,
		   DBUS_A_SVC_NAME,			  /* service to contact */
		   DBUS_A_OBJ_PATH, 		  /* object path */
		   DBUS_A_IF_NAME,		      /* interface name */
		   __func__,					  /* method name */
		   NULL,						  /* object to return error in */
		   &m,							  /* return message on success */
		   "s",						  /* input signature */
		   bd_addr->ToString().c_str());			  
	CHECK_DBUS_CALL_RESULT( rtn, m );
	return BT_STATUS_SUCCESS;
}
static bt_status_t bthf_disconnect_audio (RawAddress* bd_addr)
{
	//
	ALOGD(LOGTAG "::%s",__func__);
	int rtn;
	sd_bus_message *m = NULL;
	rtn = sd_bus_call_method(g_sdbus_call,
		   DBUS_A_SVC_NAME,			  /* service to contact */
		   DBUS_A_OBJ_PATH, 		  /* object path */
		   DBUS_A_IF_NAME,		      /* interface name */
		   __func__,					  /* method name */
		   NULL,						  /* object to return error in */
		   &m,							  /* return message on success */
		   "s",						  /* input signature */
		   bd_addr->ToString().c_str());			  
	CHECK_DBUS_CALL_RESULT( rtn, m );
	return BT_STATUS_SUCCESS;
}
static bt_status_t bthf_start_voice_recognition (RawAddress* bd_addr)
{
	//
	ALOGD(LOGTAG "::%s",__func__);
	int rtn;
	sd_bus_message *m = NULL;
	rtn = sd_bus_call_method(g_sdbus_call,
		   DBUS_A_SVC_NAME,			  /* service to contact */
		   DBUS_A_OBJ_PATH, 		  /* object path */
		   DBUS_A_IF_NAME,		      /* interface name */
		   __func__,					  /* method name */
		   NULL,						  /* object to return error in */
		   &m,							  /* return message on success */
		   "s",						  /* input signature */
		   bd_addr->ToString().c_str());			  
	CHECK_DBUS_CALL_RESULT( rtn, m );
	return BT_STATUS_SUCCESS;
}
static bt_status_t bthf_stop_voice_recognition (RawAddress* bd_addr)
{
	//
	ALOGD(LOGTAG "::%s",__func__);
	int rtn;
	sd_bus_message *m = NULL;
	rtn = sd_bus_call_method(g_sdbus_call,
		   DBUS_A_SVC_NAME,			  /* service to contact */
		   DBUS_A_OBJ_PATH, 		  /* object path */
		   DBUS_A_IF_NAME,		      /* interface name */
		   __func__,					  /* method name */
		   NULL,						  /* object to return error in */
		   &m,							  /* return message on success */
		   "s",						  /* input signature */
		   bd_addr->ToString().c_str());			  
	CHECK_DBUS_CALL_RESULT( rtn, m );
	return BT_STATUS_SUCCESS;
}
static bt_status_t bthf_volume_control (bthf_volume_type_t type, int volume,
                            RawAddress* bd_addr)
{
	//
	ALOGD(LOGTAG "::%s",__func__);
	int rtn;
	sd_bus_message *m = NULL;
	rtn = sd_bus_call_method(g_sdbus_call,
		   DBUS_A_SVC_NAME,			  /* service to contact */
		   DBUS_A_OBJ_PATH, 		  /* object path */
		   DBUS_A_IF_NAME,		      /* interface name */
		   __func__,					  /* method name */
		   NULL,						  /* object to return error in */
		   &m,							  /* return message on success */
		   "iis",						  /* input signature */
		   type, 
		   volume,
		   bd_addr->ToString().c_str());			  
	CHECK_DBUS_CALL_RESULT( rtn, m );
	return BT_STATUS_SUCCESS;

}
static bt_status_t bthf_device_status_notification (bthf_network_state_t ntk_state,
                                        bthf_service_type_t svc_type,
                                        int signal, int batt_chg, RawAddress* bd_addr)
{
	//
	ALOGD(LOGTAG "::%s",__func__);
	int rtn;
	sd_bus_message *m = NULL;
	rtn = sd_bus_call_method(g_sdbus_call,
		   DBUS_A_SVC_NAME,			  /* service to contact */
		   DBUS_A_OBJ_PATH, 		  /* object path */
		   DBUS_A_IF_NAME,		      /* interface name */
		   __func__,					  /* method name */
		   NULL,						  /* object to return error in */
		   &m,							  /* return message on success */
		   "iiiis",						  /* input signature */
		   ntk_state,
		   svc_type,
		   signal,
		   batt_chg,
		   bd_addr->ToString().c_str());			  
	CHECK_DBUS_CALL_RESULT( rtn, m );
	return BT_STATUS_SUCCESS;

}
static bt_status_t bthf_cops_response (const char* cops, RawAddress* bd_addr)
{
	//
	ALOGD(LOGTAG "::%s",__func__);
	int rtn;
	sd_bus_message *m = NULL;
	rtn = sd_bus_call_method(g_sdbus_call,
		   DBUS_A_SVC_NAME,			  /* service to contact */
		   DBUS_A_OBJ_PATH, 		  /* object path */
		   DBUS_A_IF_NAME,		      /* interface name */
		   __func__,					  /* method name */
		   NULL,						  /* object to return error in */
		   &m,							  /* return message on success */
		   "ss",						  /* input signature */
		   cops,
		   bd_addr->ToString().c_str());			  
	CHECK_DBUS_CALL_RESULT( rtn, m );
	return BT_STATUS_SUCCESS;
}
static bt_status_t bthf_cind_response (int svc, int num_active, int num_held,
                           bthf_call_state_t call_setup_state, int signal,
                           int roam, int batt_chg, RawAddress* bd_addr)
{
	//
	ALOGD(LOGTAG "::%s",__func__);
	int rtn;
	sd_bus_message *m = NULL;
	rtn = sd_bus_call_method(g_sdbus_call,
		   DBUS_A_SVC_NAME,			  /* service to contact */
		   DBUS_A_OBJ_PATH, 		  /* object path */
		   DBUS_A_IF_NAME,		      /* interface name */
		   __func__,					  /* method name */
		   NULL,						  /* object to return error in */
		   &m,							  /* return message on success */
		   "iiiiiiis",						  /* input signature */
		   svc,
		   num_active,
		   num_held,
		   call_setup_state,
		   signal,
		   roam,
		   batt_chg,
		   bd_addr->ToString().c_str());			  
	CHECK_DBUS_CALL_RESULT( rtn, m );
	return BT_STATUS_SUCCESS;

}
static bt_status_t bthf_formatted_at_response (const char* rsp, RawAddress* bd_addr)
{
	//
	ALOGD(LOGTAG "::%s",__func__);
	int rtn;
	sd_bus_message *m = NULL;
	rtn = sd_bus_call_method(g_sdbus_call,
		   DBUS_A_SVC_NAME,			  /* service to contact */
		   DBUS_A_OBJ_PATH, 		  /* object path */
		   DBUS_A_IF_NAME,		      /* interface name */
		   __func__,					  /* method name */
		   NULL,						  /* object to return error in */
		   &m,							  /* return message on success */
		   "ss",						  /* input signature */
		   rsp,
		   bd_addr->ToString().c_str());			  
	CHECK_DBUS_CALL_RESULT( rtn, m );
	return BT_STATUS_SUCCESS;
}
static bt_status_t bthf_at_response (bthf_at_response_t response_code, int error_code,
                         RawAddress* bd_addr)
{
	ALOGD(LOGTAG "::%s",__func__);
	int rtn;
	sd_bus_message *m = NULL;
	rtn = sd_bus_call_method(g_sdbus_call,
		   DBUS_A_SVC_NAME,			  /* service to contact */
		   DBUS_A_OBJ_PATH, 		  /* object path */
		   DBUS_A_IF_NAME,		      /* interface name */
		   __func__,					  /* method name */
		   NULL,						  /* object to return error in */
		   &m,							  /* return message on success */
		   "iis",						  /* input signature */
		   response_code,
		   error_code,
		   bd_addr->ToString().c_str());			  
	CHECK_DBUS_CALL_RESULT( rtn, m );
	return BT_STATUS_SUCCESS;
}
static bt_status_t bthf_clcc_response (int index, bthf_call_direction_t dir,
                           bthf_call_state_t state, bthf_call_mode_t mode,
                           bthf_call_mpty_type_t mpty, const char* number,
                           bthf_call_addrtype_t type, RawAddress* bd_addr)
{
	//
	ALOGD(LOGTAG "::%s",__func__);
	int rtn;
	sd_bus_message *m = NULL;
	rtn = sd_bus_call_method(g_sdbus_call,
		   DBUS_A_SVC_NAME,			  /* service to contact */
		   DBUS_A_OBJ_PATH, 		  /* object path */
		   DBUS_A_IF_NAME,		      /* interface name */
		   __func__,					  /* method name */
		   NULL,						  /* object to return error in */
		   &m,							  /* return message on success */
		   "iiiiisis",						  /* input signature */
		   index,
		   dir,
		   state,
		   mode,
		   mpty,
		   number,
		   type,
		   bd_addr->ToString().c_str());			  
	CHECK_DBUS_CALL_RESULT( rtn, m );
	return BT_STATUS_SUCCESS;
}
static bt_status_t bthf_phone_state_change (int num_active, int num_held,
                                bthf_call_state_t call_setup_state,
                                const char* number,
                                bthf_call_addrtype_t type, RawAddress* bd_addr)
{
	//
	ALOGD(LOGTAG "::%s",__func__);
	int rtn;
	sd_bus_message *m = NULL;
	rtn = sd_bus_call_method(g_sdbus_call,
		   DBUS_A_SVC_NAME,			  /* service to contact */
		   DBUS_A_OBJ_PATH, 		  /* object path */
		   DBUS_A_IF_NAME,		      /* interface name */
		   __func__,					  /* method name */
		   NULL,						  /* object to return error in */
		   &m,							  /* return message on success */
		   "iiisis",						  /* input signature */
		   num_active,
		   num_held,
		   call_setup_state,
		   number,
		   type,
		   bd_addr->ToString().c_str());			  
	CHECK_DBUS_CALL_RESULT( rtn, m );
	return BT_STATUS_SUCCESS;

}
static void bthf_cleanup (void)
{
	//
	ALOGD(LOGTAG "::%s",__func__);
	int rtn;
	sd_bus_message *m = NULL;
	rtn = sd_bus_call_method(g_sdbus_call,
		   DBUS_A_SVC_NAME,			  /* service to contact */
		   DBUS_A_OBJ_PATH, 		  /* object path */
		   DBUS_A_IF_NAME,		      /* interface name */
		   __func__,					  /* method name */
		   NULL,						  /* object to return error in */
		   &m,							  /* return message on success */
		   "");			  
	CHECK_DBUS_CALL_RESULT( rtn, m );
}
static bt_status_t bthf_set_sco_allowed (bool value)
{
	//
	ALOGD(LOGTAG "::%s",__func__);
	int rtn;
	sd_bus_message *m = NULL;
	rtn = sd_bus_call_method(g_sdbus_call,
		   DBUS_A_SVC_NAME,			  /* service to contact */
		   DBUS_A_OBJ_PATH, 		  /* object path */
		   DBUS_A_IF_NAME,		      /* interface name */
		   __func__,					  /* method name */
		   NULL,						  /* object to return error in */
		   &m,							  /* return message on success */
		   "i",						  /* input signature */
		   value?1:0);			  
	CHECK_DBUS_CALL_RESULT( rtn, m );
	return BT_STATUS_SUCCESS;

}
static bt_status_t bthf_send_bsir(bool value, RawAddress* bd_addr)
{
	//
	ALOGD(LOGTAG "::%s",__func__);
	int rtn;
	sd_bus_message *m = NULL;
	rtn = sd_bus_call_method(g_sdbus_call,
		   DBUS_A_SVC_NAME,			  /* service to contact */
		   DBUS_A_OBJ_PATH, 		  /* object path */
		   DBUS_A_IF_NAME,		      /* interface name */
		   __func__,					  /* method name */
		   NULL,						  /* object to return error in */
		   &m,							  /* return message on success */
		   "is",						  /* input signature */
		   value?1:0,
		   bd_addr->ToString().c_str());			  
	CHECK_DBUS_CALL_RESULT( rtn, m );
	return BT_STATUS_SUCCESS;

}
static bt_status_t bthf_set_active_device(RawAddress* active_device_addr)
{
	//
	ALOGD(LOGTAG "::%s",__func__);
	int rtn;
	sd_bus_message *m = NULL;
	rtn = sd_bus_call_method(g_sdbus_call,
		   DBUS_A_SVC_NAME,			  /* service to contact */
		   DBUS_A_OBJ_PATH, 		  /* object path */
		   DBUS_A_IF_NAME,		      /* interface name */
		   __func__,					  /* method name */
		   NULL,						  /* object to return error in */
		   &m,							  /* return message on success */
		   "s",
		   active_device_addr->ToString().c_str());			  
	CHECK_DBUS_CALL_RESULT( rtn, m );
	return BT_STATUS_SUCCESS;
}

/*
callbacks for         sBtHfpAgInterface->init(&sBluetoothHfpAgCallbacks, 1, true);
*/

static int _bthf_connection_state_callback (sd_bus_message *m, void *userdata, sd_bus_error *ret_error)
{
  bthf_connection_state_t state; 
  RawAddress bd_addr;
  //
  const char *str_get;
  int rtn, res;
  res = sd_bus_reply_method_return(m, nullptr);
  rtn = sd_bus_message_read(m, "is", &state, &str_get);
  
  if (rtn < 0){
    ALOGE(LOGTAG "::%s Failed to read parameters: %d - %s\n", __func__, -rtn, strerror(-rtn));
  } else {
    bd_addr = str2addr(str_get); 
    ALOGD(LOGTAG "::%s state:%d,addr:%s", __func__, state, str_get);
	if (bthf_callbacks) bthf_callbacks->connection_state_cb(state, &bd_addr);
  }
  return res;
}

static int _bthf_audio_state_callback (sd_bus_message *m, void *userdata, sd_bus_error *ret_error)
{
  bthf_audio_state_t state;
  RawAddress bd_addr;
  //
  const char *str_get;
  int rtn, res;
  res = sd_bus_reply_method_return(m, nullptr);
  rtn = sd_bus_message_read(m, "is", &state, &str_get);
  
  if (rtn < 0){
    ALOGE(LOGTAG "::%s Failed to read parameters: %d - %s\n", __func__, -rtn, strerror(-rtn));
  } else {
    bd_addr = str2addr(str_get); 
    ALOGD(LOGTAG "::%s state:%d,addr:%s", __func__, state, str_get);
	if (bthf_callbacks) bthf_callbacks->audio_state_cb(state, &bd_addr);
  }
  return res;
}

static int _bthf_vr_cmd_callback (sd_bus_message *m, void *userdata, sd_bus_error *ret_error)
{
  bthf_vr_state_t state;
  RawAddress bd_addr;
  //
  const char *str_get;
  int rtn, res;
  res = sd_bus_reply_method_return(m, nullptr);
  rtn = sd_bus_message_read(m, "is", &state, &str_get);
  
  if (rtn < 0){
    ALOGE(LOGTAG "::%s Failed to read parameters: %d - %s\n", __func__, -rtn, strerror(-rtn));
  } else {
    bd_addr = str2addr(str_get); 
    ALOGD(LOGTAG "::%s state:%d,addr:%s", __func__, state, str_get);
    if (bthf_callbacks) bthf_callbacks->vr_cmd_cb(state, &bd_addr);
  }
  return res;
}

static int _bthf_answer_call_cmd_callback (sd_bus_message *m, void *userdata, sd_bus_error *ret_error)
{
  RawAddress bd_addr;
  //
  const char *str_get;
  int rtn, res;
  res = sd_bus_reply_method_return(m, nullptr);
  rtn = sd_bus_message_read(m, "s", &str_get);
  
  if (rtn < 0){
    ALOGE(LOGTAG "::%s Failed to read parameters: %d - %s\n", __func__, -rtn, strerror(-rtn));
  } else {
    bd_addr = str2addr(str_get); 
    ALOGD(LOGTAG "::%s addr:%s", __func__, str_get);
    if (bthf_callbacks) bthf_callbacks->answer_call_cmd_cb(&bd_addr);
  }
  return res;	
}

/** Callback for disconnect call (AT+CHUP)
 */
static int _bthf_hangup_call_cmd_callback (sd_bus_message *m, void *userdata, sd_bus_error *ret_error)
{
	RawAddress bd_addr;
	//
	const char *str_get;
	int rtn, res;
	res = sd_bus_reply_method_return(m, nullptr);
	rtn = sd_bus_message_read(m, "s", &str_get);
	
	if (rtn < 0){
	  ALOGE(LOGTAG "::%s Failed to read parameters: %d - %s\n", __func__, -rtn, strerror(-rtn));
	} else {
	  bd_addr = str2addr(str_get); 
	  ALOGD(LOGTAG "::%s addr:%s", __func__, str_get);
	  if (bthf_callbacks) bthf_callbacks->hangup_call_cmd_cb(&bd_addr);
	}
	return res;   
}

/** Callback for disconnect call (AT+CHUP)
 *  type will denote Speaker/Mic gain (BtHfVolumeControl).
 */
static int _bthf_volume_cmd_callback (sd_bus_message *m, void *userdata, sd_bus_error *ret_error)
{
	bthf_volume_type_t type;
	int volume; 
	RawAddress bd_addr;
	//
	const char *str_get;
	int rtn, res;
	res = sd_bus_reply_method_return(m, nullptr);
	rtn = sd_bus_message_read(m, "iis", &type, &volume, &str_get);
	
	if (rtn < 0){
	  ALOGE(LOGTAG "::%s Failed to read parameters: %d - %s\n", __func__, -rtn, strerror(-rtn));
	} else {
	  bd_addr = str2addr(str_get); 
	  ALOGD(LOGTAG "::%s addr:%s", __func__, str_get);
	  if (bthf_callbacks) bthf_callbacks->volume_cmd_cb(type, volume, &bd_addr);
	}
	return res;  
}

/** Callback for dialing an outgoing call
 *  If number is NULL, redial
 */
static int _bthf_dial_call_cmd_callback (sd_bus_message *m, void *userdata, sd_bus_error *ret_error)
{
  char *number; 
  RawAddress bd_addr;
  //
  const char *str_get;
  int rtn, res;
  res = sd_bus_reply_method_return(m, nullptr);
  rtn = sd_bus_message_read(m, "ss", &number, &str_get);
  
  if (rtn < 0){
    ALOGE(LOGTAG "::%s Failed to read parameters: %d - %s\n", __func__, -rtn, strerror(-rtn));
  } else {
    bd_addr = str2addr(str_get); 
    ALOGD(LOGTAG "::%s addr:%s", __func__, str_get);
    if (bthf_callbacks) bthf_callbacks->dial_call_cmd_cb(number, &bd_addr);
  }
  return res;  
}

/** Callback for sending DTMF tones
 *  tone contains the dtmf character to be sent
 */
static int _bthf_dtmf_cmd_callback (sd_bus_message *m, void *userdata, sd_bus_error *ret_error)
{
  char tone; 
  RawAddress bd_addr;
  //
  const char *str_get;
  int rtn, res;
  res = sd_bus_reply_method_return(m, nullptr);
  rtn = sd_bus_message_read(m, "ys", &tone, &str_get);
  
  if (rtn < 0){
    ALOGE(LOGTAG "::%s Failed to read parameters: %d - %s\n", __func__, -rtn, strerror(-rtn));
  } else {
    bd_addr = str2addr(str_get); 
    ALOGD(LOGTAG "::%s addr:%s", __func__, str_get);
    if (bthf_callbacks) bthf_callbacks->dtmf_cmd_cb(tone, &bd_addr);
  }
  return res; 
}

/** Callback for enabling/disabling noise reduction/echo cancellation
 *  value will be 1 to enable, 0 to disable
 */
static int _bthf_nrec_cmd_callback (sd_bus_message *m, void *userdata, sd_bus_error *ret_error)
{
  bthf_nrec_t nrec;
  RawAddress bd_addr;
  //
  const char *str_get;
  int rtn, res;
  res = sd_bus_reply_method_return(m, nullptr);
  rtn = sd_bus_message_read(m, "is", &nrec, &str_get);
  
  if (rtn < 0){
    ALOGE(LOGTAG "::%s Failed to read parameters: %d - %s\n", __func__, -rtn, strerror(-rtn));
  } else {
    bd_addr = str2addr(str_get); 
    ALOGD(LOGTAG "::%s addr:%s", __func__, str_get);
    if (bthf_callbacks) bthf_callbacks->nrec_cmd_cb(nrec, &bd_addr);
  }
  return res; 
}

/** Callback for AT+BCS and event from BAC
 *  WBS enable, WBS disable
 */
static int _bthf_wbs_callback (sd_bus_message *m, void *userdata, sd_bus_error *ret_error)
{
  bthf_wbs_config_t wbs;
  RawAddress bd_addr;
  //
  const char *str_get;
  int rtn, res;
  res = sd_bus_reply_method_return(m, nullptr);
  rtn = sd_bus_message_read(m, "is", &wbs, &str_get);
  
  if (rtn < 0){
    ALOGE(LOGTAG "::%s Failed to read parameters: %d - %s\n", __func__, -rtn, strerror(-rtn));
  } else {
    bd_addr = str2addr(str_get); 
    ALOGD(LOGTAG "::%s addr:%s", __func__, str_get);
    if (bthf_callbacks) bthf_callbacks->wbs_cb(wbs, &bd_addr);
  }
  return res;   
}

/** Callback for call hold handling (AT+CHLD)
 *  value will contain the call hold command (0, 1, 2, 3)
 */
static int _bthf_chld_cmd_callback (sd_bus_message *m, void *userdata, sd_bus_error *ret_error)
{
  bthf_chld_type_t chld; 
  RawAddress bd_addr;
  //
  const char *str_get;
  int rtn, res;
  res = sd_bus_reply_method_return(m, nullptr);
  rtn = sd_bus_message_read(m, "is", &chld, &str_get);
  
  if (rtn < 0){
    ALOGE(LOGTAG "::%s Failed to read parameters: %d - %s\n", __func__, -rtn, strerror(-rtn));
  } else {
    bd_addr = str2addr(str_get); 
    ALOGD(LOGTAG "::%s addr:%s", __func__, str_get);
	if (bthf_callbacks) bthf_callbacks->chld_cmd_cb(chld, &bd_addr);
  }
  return res;    
}

/** Callback for CNUM (subscriber number)
 */
static int _bthf_cnum_cmd_callback (sd_bus_message *m, void *userdata, sd_bus_error *ret_error)
{
  RawAddress bd_addr;
  //
  const char *str_get;
  int rtn, res;
  res = sd_bus_reply_method_return(m, nullptr);
  rtn = sd_bus_message_read(m, "s", &str_get);
  
  if (rtn < 0){
    ALOGE(LOGTAG "::%s Failed to read parameters: %d - %s\n", __func__, -rtn, strerror(-rtn));
  } else {
    bd_addr = str2addr(str_get); 
    ALOGD(LOGTAG "::%s addr:%s", __func__, str_get);
	if (bthf_callbacks) bthf_callbacks->cnum_cmd_cb(&bd_addr);
  }
  return res;   
}
/** Callback for indicators (CIND)
 */
static int _bthf_cind_cmd_callback (sd_bus_message *m, void *userdata, sd_bus_error *ret_error)
{
  RawAddress bd_addr;
  //
  const char *str_get;
  int rtn, res;
  res = sd_bus_reply_method_return(m, nullptr);
  rtn = sd_bus_message_read(m, "s", &str_get);
  
  if (rtn < 0){
    ALOGE(LOGTAG "::%s Failed to read parameters: %d - %s\n", __func__, -rtn, strerror(-rtn));
  } else {
    bd_addr = str2addr(str_get); 
    ALOGD(LOGTAG "::%s addr:%s", __func__, str_get);
    if (bthf_callbacks) bthf_callbacks->cind_cmd_cb(&bd_addr);
  }
  return res;  
}

/** Callback for operator selection (COPS)
 */
static int _bthf_cops_cmd_callback (sd_bus_message *m, void *userdata, sd_bus_error *ret_error)
{
  RawAddress bd_addr;
  //
  const char *str_get;
  int rtn, res;
  res = sd_bus_reply_method_return(m, nullptr);
  rtn = sd_bus_message_read(m, "s", &str_get);
  
  if (rtn < 0){
    ALOGE(LOGTAG "::%s Failed to read parameters: %d - %s\n", __func__, -rtn, strerror(-rtn));
  } else {
    bd_addr = str2addr(str_get); 
    ALOGD(LOGTAG "::%s addr:%s", __func__, str_get);
    if (bthf_callbacks) bthf_callbacks->cops_cmd_cb(&bd_addr);
  }
  return res;  
}
/** Callback for call list (AT+CLCC)
 */
static int _bthf_clcc_cmd_callback (sd_bus_message *m, void *userdata, sd_bus_error *ret_error)
{
  RawAddress bd_addr;
  //
  const char *str_get;
  int rtn, res;
  res = sd_bus_reply_method_return(m, nullptr);
  rtn = sd_bus_message_read(m, "s", &str_get);
  
  if (rtn < 0){
    ALOGE(LOGTAG "::%s Failed to read parameters: %d - %s\n", __func__, -rtn, strerror(-rtn));
  } else {
    bd_addr = str2addr(str_get); 
    ALOGD(LOGTAG "::%s addr:%s", __func__, str_get);
    if (bthf_callbacks) bthf_callbacks->clcc_cmd_cb(&bd_addr);
  }
  return res;  
}

/** Callback for unknown AT command recd from HF
 *  at_string will contain the unparsed AT string
 */
static int _bthf_unknown_at_cmd_callback (sd_bus_message *m, void *userdata, sd_bus_error *ret_error)
{
  char *at_string; 
  RawAddress bd_addr;
  //
  const char *str_get;
  int rtn, res;
  res = sd_bus_reply_method_return(m, nullptr);
  rtn = sd_bus_message_read(m, "ss", &at_string, &str_get);
  
  if (rtn < 0){
    ALOGE(LOGTAG "::%s Failed to read parameters: %d - %s\n", __func__, -rtn, strerror(-rtn));
  } else {
    bd_addr = str2addr(str_get); 
    ALOGD(LOGTAG "::%s addr:%s", __func__, str_get);
    if (bthf_callbacks) bthf_callbacks->unknown_at_cmd_cb(at_string, &bd_addr);
  }
  return res;  
}
static int _bthf_bind_cmd_callback (sd_bus_message *m, void *userdata, sd_bus_error *ret_error)
{
  char *at_string; 
  RawAddress bd_addr;
  //
  const char *str_get;
  int rtn, res;
  res = sd_bus_reply_method_return(m, nullptr);
  rtn = sd_bus_message_read(m, "ss", &at_string, &str_get);
  
  if (rtn < 0){
    ALOGE(LOGTAG "::%s Failed to read parameters: %d - %s\n", __func__, -rtn, strerror(-rtn));
  } else {
    bd_addr = str2addr(str_get); 
    ALOGD(LOGTAG "::%s addr:%s", __func__, str_get);
    if (bthf_callbacks) bthf_callbacks->bind_cb(at_string, &bd_addr);
  }
  return res;  
}
static int _bthf_biev_cmd_callback (sd_bus_message *m, void *userdata, sd_bus_error *ret_error)
{
  bthf_hf_ind_type_t ind_id; 
  int ind_value;
  RawAddress bd_addr;
  //
  const char *str_get;
  int rtn, res;
  res = sd_bus_reply_method_return(m, nullptr);
  rtn = sd_bus_message_read(m, "iis", &ind_id, &ind_value, &str_get);
  
  if (rtn < 0){
    ALOGE(LOGTAG "::%s Failed to read parameters: %d - %s\n", __func__, -rtn, strerror(-rtn));
  } else {
    bd_addr = str2addr(str_get); 
    ALOGD(LOGTAG "::%s addr:%s", __func__, str_get);
    if (bthf_callbacks) bthf_callbacks->biev_cb(ind_id, ind_value, &bd_addr);
  }
  return res; 
}
static int _bthf_key_pressed_cmd_callback (sd_bus_message *m, void *userdata, sd_bus_error *ret_error)
{
  RawAddress bd_addr;
  //
  const char *str_get;
  int rtn, res;
  res = sd_bus_reply_method_return(m, nullptr);
  rtn = sd_bus_message_read(m, "s", &str_get);
  
  if (rtn < 0){
    ALOGE(LOGTAG "::%s Failed to read parameters: %d - %s\n", __func__, -rtn, strerror(-rtn));
  } else {
    bd_addr = str2addr(str_get); 
    ALOGD(LOGTAG "::%s addr:%s", __func__, str_get);
    if (bthf_callbacks) bthf_callbacks->key_pressed_cmd_cb(&bd_addr);
  }
  return res;  
}


/*****************************************************************************************************************************
sBtHfpAgVendorInterface = (bthf_vendor_interface_t *)bluetooth_interface->
	get_profile_interface(BT_PROFILE_HANDSFREE_VENDOR_ID);
*/

/**
* Register the BtHf Vendor callbacks
*/
static bt_status_t bthf_init_vendor ( bthf_vendor_callbacks_t* callbacks);
/** Response for BIND READ command and activation/deactivation of  HF indicator */
static bt_status_t bthf_bind_response_vendor (int anum, bthf_vendor_hf_indicator_status_t status, bt_bdaddr_t *bd_addr);
/** Response for BIND TEST command */
static bt_status_t bthf_bind_string_response_vendor (const char* result, bt_bdaddr_t *bd_addr);
/** Closes the hf vednor interface. */
static void bthf_cleanup_vendor (void);

bthf_vendor_interface_t bthf_vendor_interface_b = 
{
	sizeof(bthf_vendor_interface_t),
	bthf_init_vendor,
	bthf_bind_response_vendor,
	bthf_bind_string_response_vendor,
	bthf_cleanup_vendor
};

bthf_vendor_callbacks_t *bthf_vendor_callbacks = NULL;

static bt_status_t bthf_init_vendor ( bthf_vendor_callbacks_t* callbacks)
{
	bthf_vendor_callbacks = callbacks;
	//
	ALOGD(LOGTAG "::%s",__func__);
	int rtn;
	sd_bus_message *m = NULL;
	rtn = sd_bus_call_method(g_sdbus_call,
		   DBUS_A_SVC_NAME,			  /* service to contact */
		   DBUS_A_OBJ_PATH, 		  /* object path */
		   DBUS_A_IF_NAME,		      /* interface name */
		   __func__,					  /* method name */
		   NULL,						  /* object to return error in */
		   &m,							  /* return message on success */
		   "");			  
	CHECK_DBUS_CALL_RESULT( rtn, m );
	return BT_STATUS_SUCCESS;	
}
/** Response for BIND READ command and activation/deactivation of  HF indicator */
static bt_status_t bthf_bind_response_vendor (int anum, bthf_vendor_hf_indicator_status_t status, bt_bdaddr_t *bd_addr)
{
  //
  ALOGD(LOGTAG "::%s",__func__);
  int rtn;
  sd_bus_message *m = NULL;
  rtn = sd_bus_call_method(g_sdbus_call,
  	   DBUS_A_SVC_NAME,			  /* service to contact */
  	   DBUS_A_OBJ_PATH, 		  /* object path */
  	   DBUS_A_IF_NAME,		      /* interface name */
  	   __func__,					  /* method name */
  	   NULL,						  /* object to return error in */
  	   &m,							  /* return message on success */
  	   "iis",
  	   anum,
  	   status,
  	   bd_addr->ToString().c_str());			  
  CHECK_DBUS_CALL_RESULT( rtn, m );
}
/** Response for BIND TEST command */
static bt_status_t bthf_bind_string_response_vendor (const char* result, bt_bdaddr_t *bd_addr)
{
  //
  ALOGD(LOGTAG "::%s",__func__);
  int rtn;
  sd_bus_message *m = NULL;
  rtn = sd_bus_call_method(g_sdbus_call,
  	   DBUS_A_SVC_NAME,			  /* service to contact */
  	   DBUS_A_OBJ_PATH, 		  /* object path */
  	   DBUS_A_IF_NAME,		      /* interface name */
  	   __func__,					  /* method name */
  	   NULL,						  /* object to return error in */
  	   &m,							  /* return message on success */
  	   "ss",
  	   result,
  	   bd_addr->ToString().c_str());
  CHECK_DBUS_CALL_RESULT( rtn, m );
}
/** Closes the hf vednor interface. */
static void bthf_cleanup_vendor (void)
{
  //
  ALOGD(LOGTAG "::%s",__func__);
  int rtn;
  sd_bus_message *m = NULL;
  rtn = sd_bus_call_method(g_sdbus_call,
  	   DBUS_A_SVC_NAME,			  /* service to contact */
  	   DBUS_A_OBJ_PATH, 		  /* object path */
  	   DBUS_A_IF_NAME,		      /* interface name */
  	   __func__,					  /* method name */
  	   NULL,						  /* object to return error in */
  	   &m,							  /* return message on success */
  	   "");
  CHECK_DBUS_CALL_RESULT( rtn, m );
}
/** Vendor callback for HF indicators (BIND)
 */
static int _bthf_bind_cmd_vendor_callback (sd_bus_message *m, void *userdata, sd_bus_error *ret_error)
{
	char* hf_ind; 
	bthf_vendor_bind_type_t type; 
	bt_bdaddr_t bd_addr;
	//
	const char *str_get;
	int rtn, res;
	res = sd_bus_reply_method_return(m, nullptr);
	rtn = sd_bus_message_read(m, "sis", &hf_ind, &type, &str_get);
	
	if (rtn < 0){
	  ALOGE(LOGTAG "::%s Failed to read parameters: %d - %s\n", __func__, -rtn, strerror(-rtn));
	} else {
	  bd_addr = str2addr(str_get); 
	  ALOGD(LOGTAG "::%s addr:%s", __func__, str_get);
	  if (bthf_vendor_callbacks) bthf_vendor_callbacks->bind_cmd_vendor_cb(hf_ind, type, &bd_addr);
	}
	return res; 
}
/** Vendor callback for HF indicator value (BIEV)
 */
static int _bthf_biev_cmd_vendor_callback (sd_bus_message *m, void *userdata, sd_bus_error *ret_error)
{
	char* hf_ind_val; 
	bt_bdaddr_t bd_addr;
	//
	const char *str_get;
	int rtn, res;
	res = sd_bus_reply_method_return(m, nullptr);
	rtn = sd_bus_message_read(m, "ss", &hf_ind_val, &str_get);
	
	if (rtn < 0){
	  ALOGE(LOGTAG "::%s Failed to read parameters: %d - %s\n", __func__, -rtn, strerror(-rtn));
	} else {
	  bd_addr = str2addr(str_get); 
	  ALOGD(LOGTAG "::%s addr:%s", __func__, str_get);
	  if (bthf_vendor_callbacks) bthf_vendor_callbacks->biev_cmd_vendor_cb(hf_ind_val, &bd_addr);
	}
	return res; 
}

/*
   public interface:
*/
static bool hfpag_open_sdbus()
{
  // Set dBus-session
  
  if (g_sdbus == NULL) { ALOGE(LOGTAG "g_sdbus is NULL"); return false;}
  
  if (sd_bus_open_system(&g_sdbus_call) < 0){
      ALOGE(LOGTAG "::%s D-Bus is not Initialised.", __func__);
      return false;
  }
  if (sd_bus_request_name(g_sdbus_call, DBUS_B_SVC_NAME_SENDER, 0) < 0){
      ALOGE(LOGTAG "::%s Failed to acquire name on user bus.", __func__);
      return false;
  }
  
  ALOGD(LOGTAG "::%s Successed to open bus!! : service - %s", __func__, DBUS_B_SVC_NAME);
  
  return true;
}

static void hfpag_close_sdbus()
{
 
  if (g_sdbus_call != nullptr)
  {
    sd_bus_flush_close_unref(g_sdbus_call);
    g_sdbus_call = nullptr;
  }
}

static sd_bus_slot *m_sdbus_slot_hfp;

void hfpag_interface_b_init (void)
{
  // Create DBus interface for Adapter functionalities
  static const sd_bus_vtable dbus_vtable[] = {
    SD_BUS_VTABLE_START(0),
    /*hfp ag callbacks*/
    SD_BUS_METHOD("_bthf_connection_state_callback", "is", nullptr, _bthf_connection_state_callback, SD_BUS_VTABLE_UNPRIVILEGED),
    SD_BUS_METHOD("_bthf_audio_state_callback", "is", nullptr, _bthf_audio_state_callback, SD_BUS_VTABLE_UNPRIVILEGED),
    SD_BUS_METHOD("_bthf_vr_cmd_callback", "is", nullptr, _bthf_vr_cmd_callback, SD_BUS_VTABLE_UNPRIVILEGED),
    SD_BUS_METHOD("_bthf_answer_call_cmd_callback", "s", nullptr, _bthf_answer_call_cmd_callback, SD_BUS_VTABLE_UNPRIVILEGED),
    SD_BUS_METHOD("_bthf_hangup_call_cmd_callback", "s", nullptr, _bthf_hangup_call_cmd_callback, SD_BUS_VTABLE_UNPRIVILEGED),
    SD_BUS_METHOD("_bthf_volume_cmd_callback", "iis", nullptr, _bthf_volume_cmd_callback, SD_BUS_VTABLE_UNPRIVILEGED),
    SD_BUS_METHOD("_bthf_dial_call_cmd_callback", "ss", nullptr, _bthf_dial_call_cmd_callback, SD_BUS_VTABLE_UNPRIVILEGED),
    SD_BUS_METHOD("_bthf_dtmf_cmd_callback", "ys", nullptr, _bthf_dtmf_cmd_callback, SD_BUS_VTABLE_UNPRIVILEGED),
    SD_BUS_METHOD("_bthf_nrec_cmd_callback", "is", nullptr, _bthf_nrec_cmd_callback, SD_BUS_VTABLE_UNPRIVILEGED),
    SD_BUS_METHOD("_bthf_wbs_callback", "is", nullptr, _bthf_wbs_callback, SD_BUS_VTABLE_UNPRIVILEGED),
    SD_BUS_METHOD("_bthf_chld_cmd_callback", "is", nullptr, _bthf_chld_cmd_callback, SD_BUS_VTABLE_UNPRIVILEGED),
    SD_BUS_METHOD("_bthf_cnum_cmd_callback", "s", nullptr, _bthf_cnum_cmd_callback, SD_BUS_VTABLE_UNPRIVILEGED),
    SD_BUS_METHOD("_bthf_cind_cmd_callback", "s", nullptr, _bthf_cind_cmd_callback, SD_BUS_VTABLE_UNPRIVILEGED),
    SD_BUS_METHOD("_bthf_cops_cmd_callback", "s", nullptr, _bthf_cops_cmd_callback, SD_BUS_VTABLE_UNPRIVILEGED),
    SD_BUS_METHOD("_bthf_clcc_cmd_callback", "s", nullptr, _bthf_clcc_cmd_callback, SD_BUS_VTABLE_UNPRIVILEGED),
    SD_BUS_METHOD("_bthf_unknown_at_cmd_callback", "ss", nullptr, _bthf_unknown_at_cmd_callback, SD_BUS_VTABLE_UNPRIVILEGED),
    SD_BUS_METHOD("_bthf_key_pressed_cmd_callback", "s", nullptr, _bthf_key_pressed_cmd_callback, SD_BUS_VTABLE_UNPRIVILEGED),
    SD_BUS_METHOD("_bthf_bind_cmd_callback", "ss", nullptr, _bthf_bind_cmd_callback, SD_BUS_VTABLE_UNPRIVILEGED),
    SD_BUS_METHOD("_bthf_biev_cmd_callback", "iis", nullptr, _bthf_biev_cmd_callback, SD_BUS_VTABLE_UNPRIVILEGED),
    /*hfp ag vendor callbacks*/
    SD_BUS_METHOD("_bthf_bind_cmd_vendor_callback", "sis", nullptr, _bthf_bind_cmd_vendor_callback, SD_BUS_VTABLE_UNPRIVILEGED),
    SD_BUS_METHOD("_bthf_biev_cmd_vendor_callback", "ss", nullptr, _bthf_biev_cmd_vendor_callback, SD_BUS_VTABLE_UNPRIVILEGED),
    SD_BUS_VTABLE_END};

	hfpag_open_sdbus();	
	
	if (g_sdbus)
	{
	  int r = sd_bus_add_object_vtable(g_sdbus,
									   &m_sdbus_slot_hfp,
									   DBUS_B_OBJ_PATH, // object path
									   DBUS_B_IF_NAME,  // interface name
									   dbus_vtable,
									   NULL);
	  if (r < 0) {
		ALOGE(LOGTAG "::%s VTable creation failed (%d) %s", __func__, -r, strerror(-r));
		return ;
	  }

	  ALOGE(LOGTAG "::%s %s is added", __func__, DBUS_B_IF_NAME);
	}
	else {
	  ALOGE(LOGTAG "::%s No dbus connection.", __func__);
	  return ;
	}

}

void hfpag_interface_b_deinit (void)
{
	if( m_sdbus_slot_hfp != nullptr )  {
	  sd_bus_slot_unref(m_sdbus_slot_hfp);
	  m_sdbus_slot_hfp = nullptr;
	}

	hfpag_close_sdbus();
}

bthf_interface_t * get_profile_interface_hfpag(void)
{
	return &bthf_interface_b;
}
bthf_vendor_interface_t * get_profile_interface_hfpag_vendor (void)
{
	return &bthf_vendor_interface_b;
}
