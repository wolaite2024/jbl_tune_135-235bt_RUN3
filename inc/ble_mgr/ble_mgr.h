#ifndef __BLE_MGR_H
#define __BLE_MGR_H


#include <stdint.h>
#include <stdbool.h>
#include "gap_callback_le.h"
#include "gap_msg.h"

typedef struct
{
    struct
    {
        bool enable;
        uint32_t adv_num;
    } ble_ext_adv;

    struct
    {
        bool enable;
        uint32_t link_num;
    } ble_conn;

    struct
    {
        bool                    enable;
        bool                    update_scan_data;
        T_GAP_LOCAL_ADDR_TYPE   own_address_type;
        T_GAP_REMOTE_ADDR_TYPE  peer_address_type;
        uint32_t                adv_interval;
        uint32_t                scan_rsp_len;
        uint8_t                 *scan_rsp_data;
    } ble_adv_data;

} BLE_MGR_PARAMS;

typedef void (*BLE_MGR_MSG_CB)(uint8_t subtype, T_LE_GAP_MSG *gap_msg);


void ble_mgr_init(BLE_MGR_PARAMS *p_params);

void ble_mgr_handle_gap_cb(uint8_t cb_type, T_LE_CB_DATA *cb_data);

void ble_mgr_handle_gap_msg(uint8_t subtype, T_LE_GAP_MSG *gap_msg);

bool ble_mgr_msg_cback_register(BLE_MGR_MSG_CB cb);
#endif
