/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include "osi/include/log.h"
#include "osi/include/allocator.h"
#include "Cte.hpp"
#include "gap/include/Gap.hpp"

#define LOGTAG "Cte "

using namespace std;

Cte *g_cte = NULL;;
extern Gap *g_gap;


#ifdef __cplusplus
 extern "C"
 {
#endif

 void BtCteMsgHandler(void *msg)
 {
     BtEvent* event = NULL;
     bool status = false;
     if(!msg) {
         ALOGE(LOGTAG "%s: Msg is null, return", __FUNCTION__);
         return;
     }

     event = ( BtEvent *) msg;
     ALOGD(LOGTAG "BtCteMsgHandler event = %d", event->event_id);
     switch(event->event_id) {
         default:
             if(g_cte) {
                g_cte->ProcessEvent(( BtEvent *) msg);
             }
             break;
     }
     delete event;
 }


#ifdef __cplusplus
 }
#endif

 Cte :: Cte(const bt_interface_t *bt_interface, config_t *config)
 {
     this->bluetooth_interface = bt_interface;
     this->config = config;
     sBtVendorInterface = g_gap->GetVendorInterface();
 }

 Cte :: ~Cte()
 {
     g_cte = NULL;
 }

 void Cte::ProcessEvent(BtEvent* pEvent)
 {
     char str[18];
     ALOGD(LOGTAG "%s: Processing event %d", __FUNCTION__, pEvent->event_id);

     switch(pEvent->event_id) {
         case CTE_READ_ANTENNA_REQ:
            ReadAntenna();
            break;

         case CTE_SET_ADV_TX_PARA_REQ:
            SetAdvTxPara(pEvent->cte_event.adv_id, pEvent->cte_event.cte_length, pEvent->cte_event.cte_count);
            break;

         case CTE_ENABLE_ADV_TX_REQ:
            EnableAdvTx(pEvent->cte_event.adv_id, true);
            break;

         case CTE_DISABLE_ADV_TX_REQ:
            EnableAdvTx(pEvent->cte_event.adv_id, false);
            break;

         case CTE_SET_CONN_TX_PARA_REQ:
            SetConnTxPara(&pEvent->cte_event.bd_addr);
            break;

         case CTE_ENABLE_CONN_TX_REQ:
            EnableConnTx(&pEvent->cte_event.bd_addr, true);
            break;

         case CTE_DISABLE_CONN_TX_REQ:
            EnableConnTx(&pEvent->cte_event.bd_addr, false);
            break;

         default:
             ALOGW(LOGTAG "%s: unhandled event: %d", __FUNCTION__, pEvent->event_id);
             break;
     }
 }

 void Cte :: ReadAntenna(void)
 {
    ALOGD(LOGTAG "%s", __FUNCTION__);
    sBtVendorInterface->hci_cmd_send(HCI_BLE_READ_ANTENNA, NULL, 0);
 }

 void Cte :: SetAdvTxPara(int adv_id, int cte_length, int cte_count)
 {
    ALOGD(LOGTAG "%s advertising handle: %d cte_length: %d cte_count: %d",
                    __FUNCTION__, adv_id, cte_length, cte_count);
    uint8_t *cmd_buff = NULL;
    uint8_t *cmd = NULL;
    cmd_buff = (uint8_t *)osi_malloc(7);
    cmd=cmd_buff;
    UINT8_TO_STREAM(cmd, adv_id);
    UINT8_TO_STREAM(cmd, cte_length);
    UINT8_TO_STREAM(cmd, 0);
    UINT8_TO_STREAM(cmd, cte_count);
    UINT8_TO_STREAM(cmd, SWITCHING_PATTERN_LENGTH);
    UINT8_TO_STREAM(cmd, ANTENNA_ID1);
    UINT8_TO_STREAM(cmd, ANTENNA_ID2);
    fprintf(stdout, "\n*****************Set Connectionless TX Parameters*******************\n");
    fprintf(stdout, " Advertising_Handle        :  0x%x\n", adv_id);
    fprintf(stdout, " CTE_Length                :  %d\n", cte_length);
    fprintf(stdout, " CTE_Type                  :  AoA\n");
    fprintf(stdout, " CTE_Count                 :  %d\n", cte_count);
    fprintf(stdout, " Switching_Pattern_Length  :  %d\n", SWITCHING_PATTERN_LENGTH);
    fprintf(stdout, " Antenna_IDs               :  %d %d\n", ANTENNA_ID1, ANTENNA_ID2);
    fprintf(stdout, "*****************FINISH*******************\n");
    sBtVendorInterface->hci_cmd_send(HCI_BLE_SET_ADV_CTE_TX_PARA, &cmd_buff[0], 7);
    free(cmd_buff);
 }

 void Cte :: EnableAdvTx(int adv_id, bool enable)
 {
    ALOGD(LOGTAG "%s advertising handle: %d enable: %d", __FUNCTION__, adv_id, enable);
    uint8_t *cmd_buff = NULL;
    uint8_t *cmd = NULL;
    cmd_buff = (uint8_t *)osi_malloc(2);
    cmd=cmd_buff;
    UINT8_TO_STREAM(cmd, adv_id);
    UINT8_TO_STREAM(cmd, enable);
    sBtVendorInterface->hci_cmd_send(HCI_BLE_SET_ADV_CTE_TX_ENABLE, &cmd_buff[0], 2);
    free(cmd_buff);
 }

 void Cte :: SetConnTxPara(bt_bdaddr_t *addr)
 {
    ALOGD(LOGTAG "%s", __FUNCTION__);
    uint8_t *cmd_buff = NULL;
    uint8_t *cmd = NULL;
    uint16_t conn_handle = sBtVendorInterface->get_conn_handle(addr);
    if (conn_handle == INVALID_CONN_HANDLE) {
        ALOGD(LOGTAG "%s device not connected, can't proceed!", __FUNCTION__);
        fprintf(stdout, "device not connected, can't proceed! \n");
        return;
    }
    ALOGD(LOGTAG "%s conn_handle is 0x%x", __FUNCTION__, conn_handle);
    cmd_buff = (uint8_t *)osi_malloc(6);
    cmd=cmd_buff;
    UINT16_TO_STREAM(cmd, conn_handle);
    UINT8_TO_STREAM(cmd, 1);
    UINT8_TO_STREAM(cmd, SWITCHING_PATTERN_LENGTH);
    UINT8_TO_STREAM(cmd, ANTENNA_ID1);
    UINT8_TO_STREAM(cmd, ANTENNA_ID2);
    fprintf(stdout, "\n*****************Set Connection TX Parameters*******************\n");
    fprintf(stdout, " Connection_Handle         :  0x%x\n", conn_handle);
    fprintf(stdout, " CTE_Type                  :  AoA\n");
    fprintf(stdout, " Switching_Pattern_Length  :  %d\n", SWITCHING_PATTERN_LENGTH);
    fprintf(stdout, " Antenna_IDs               :  %d %d\n", ANTENNA_ID1, ANTENNA_ID2);
    fprintf(stdout, "*****************FINISH*******************\n");
    sBtVendorInterface->hci_cmd_send(HCI_BLE_SET_CONN_CTE_TX_PARA, &cmd_buff[0], 6);
    free(cmd_buff);
 }

 void Cte :: EnableConnTx(bt_bdaddr_t *addr, bool enable)
 {
    ALOGD(LOGTAG "%s enable: %d", __FUNCTION__, enable);
    uint8_t *cmd_buff = NULL;
    uint8_t *cmd = NULL;
    uint16_t conn_handle = sBtVendorInterface->get_conn_handle(addr);
    if (conn_handle == INVALID_CONN_HANDLE) {
        ALOGD(LOGTAG "%s device not connected, can't proceed!", __FUNCTION__);
        fprintf(stdout, "device not connected, can't proceed! \n");
        return;
    }
    ALOGD(LOGTAG "%s conn_handle is 0x%x", __FUNCTION__, conn_handle);
    fprintf(stdout, "conn_handle: %d \n", conn_handle);

    cmd_buff = (uint8_t *)osi_malloc(3);
    cmd=cmd_buff;
    UINT16_TO_STREAM(cmd, conn_handle);
    UINT8_TO_STREAM(cmd, enable);
    sBtVendorInterface->hci_cmd_send(HCI_BLE_CONN_CTE_RSP_ENABLE, &cmd_buff[0], 3);
    free(cmd_buff);
 }

