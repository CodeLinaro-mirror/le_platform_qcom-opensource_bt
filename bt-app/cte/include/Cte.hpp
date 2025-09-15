/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef CTE_APP_H
#define CTE_APP_H

#include <hardware/vendor.h>
#include "osi/include/config.h"
#include "ipc.hpp"

#define HCI_BLE_SET_ADV_CTE_TX_PARA   (0x0051 | 0x2000)
#define HCI_BLE_SET_ADV_CTE_TX_ENABLE (0x0052 | 0x2000)
#define HCI_BLE_SET_CONN_CTE_TX_PARA  (0x0055 | 0x2000)
#define HCI_BLE_CONN_CTE_RSP_ENABLE   (0x0057 | 0x2000)
#define HCI_BLE_READ_ANTENNA          (0x0058 | 0x2000)
#define SWITCHING_PATTERN_LENGTH 0x02
#define ANTENNA_ID1 0x00
#define ANTENNA_ID2 0x01
#define INVALID_CONN_HANDLE 0xFFFF

#define UINT8_TO_STREAM(p, u8)   \
  do {                         \
    *(p) = (uint8_t)(u8);    \
    (p) += 1;                \
  } while (0)

#define UINT16_TO_STREAM(p, u16)      \
  do {                              \
    *(p) = (uint8_t)(u16);      /* low byte */ \
    *((p)+1) = (uint8_t)((u16) >> 8); /* high byte */ \
    (p) += 2;                     \
  } while (0)

#define UINT8_FROM_STREAM(u8, p)      \
  do {                            \
    (u8) = (uint8_t)(*(p));     \
    (p) += 1;                   \
  } while (0)

#define UINT16_FROM_STREAM(u16, p)                \
  do {                                           \
    (u16) = (uint16_t)(*(p));            /* low byte */ \
    (u16) |= (uint16_t)(*((p) + 1)) << 8;   /* high byte */ \
    (p) += 2;                                  \
  } while (0)

class Cte {

  private:
    config_t *config;
    /**
     *  structure object for standard Bluetooth DM interface
     */
    const bt_interface_t *bluetooth_interface;

    /**
     *  structure object for Vendor interface
     */
    const btvendor_interface_t *sBtVendorInterface;

  public:

    Cte(const bt_interface_t *bt_interface, config_t *config);
    ~Cte();
    void ProcessEvent(BtEvent* pEvent);
    /**
     * @brief Read_antenna_info
     *
     * It will get the antenna information, and print on screen
     *
     * @param none
     * @return none
     */
    void ReadAntenna(void);
    /**
     * @brief SetAdvTxPara
     *
     * It will set the connectionless CTE TX parameters
     * default:
     * CTE type: AoA
     * Switching pattern is ignored
     *
     * @param adv_id Advertising_Handle of PA
     * @param cte_length CTE_Length
     * @param cte_count CTE_Count
     * @return none
     */
    void SetAdvTxPara(int adv_id, int cte_length, int cte_count);
    /**
     * @brief EnableAdvTx
     *
     * It will enable/disable the connectionless CTE TX
     *
     * @param adv_id Advertising_Handle of PA
     * @param enable Enable/Disable
     * @return none
     */
    void EnableAdvTx(int adv_id, bool enable);
    /**
     * @brief SetConnTxPara
     *
     * It will set the connection CTE TX parameters
     * default:
     * CTE type: AoA
     * Switching pattern is ignored
     *
     * @param addr Peer address
     * @return none
     */
    void SetConnTxPara(bt_bdaddr_t *addr);
    /**
     * @brief EnableConnTx
     *
     * It will enable/disable the connection CTE response
     *
     * @param addr Peer address
     * @param enable Enable/Disable
     * @return none
     */
    void EnableConnTx(bt_bdaddr_t *addr, bool enable);
};

#endif /* CTE_APP_H */
