/*
 * Copyright (c) 2018, Realsil Semiconductor Corporation. All rights reserved.
 */

#include <string.h>
#include "trace.h"
#include "gap.h"
#include "gap_bond_legacy.h"
#if F_APP_CLI_BINARY_MP_SUPPORT
#include "mp_test.h"
#endif
#include "app_gap.h"
#include "app_main.h"
#include "app_cmd.h"
#include "app_cfg.h"
#include "remote.h"
#include "btm.h"

#if GFPS_FEATURE_SUPPORT
#include "app_gfps.h"
#endif

static void app_gap_common_callback(uint8_t cb_type, void *p_cb_data)
{
    T_GAP_CB_DATA cb_data;
    memcpy(&cb_data, p_cb_data, sizeof(T_GAP_CB_DATA));
    APP_PRINT_INFO1("app_gap_common_callback: cb_type = %d", cb_type);
    switch (cb_type)
    {
    case GAP_MSG_WRITE_AIRPLAN_MODE:
        APP_PRINT_INFO1("app_gap_common_callback: GAP_MSG_WRITE_AIRPLAN_MODE cause 0x%04x",
                        cb_data.p_gap_write_airplan_mode_rsp->cause);
        break;
    case GAP_MSG_READ_AIRPLAN_MODE:
        APP_PRINT_INFO2("app_gap_common_callback: GAP_MSG_READ_AIRPLAN_MODE cause 0x%04x, mode %d",
                        cb_data.p_gap_read_airplan_mode_rsp->cause,
                        cb_data.p_gap_read_airplan_mode_rsp->mode);
        break;
    case GAP_MSG_VENDOR_CMD_CMPL_EVENT:
        {
#if F_APP_CLI_BINARY_MP_SUPPORT
            mp_test_send_vnd_cmd_cmpl_evt(cb_data.p_gap_vnd_cmd_cmpl_evt_rsp);
#endif
            if (cb_data.p_gap_vnd_cmd_cmpl_evt_rsp->command == MP_CMD_HCI_OPCODE)
            {
                app_cmd_handle_mp_cmd_hci_evt(p_cb_data);
            }
        }
        break;

    case GAP_MSG_SET_LOCAL_BD_ADDR:
        {
            APP_PRINT_INFO1("app_gap_common_callback: GAP_MSG_SET_LOCAL_BD_ADDR: cause 0x%04x",
                            cb_data.p_gap_set_bd_addr_rsp->cause);
        }
        break;

    default:
        break;
    }
    return;
}

static void app_gap_bt_cback(T_BT_EVENT event_type, void *event_buf, uint16_t buf_len)
{
    T_BT_EVENT_PARAM *param = event_buf;

    switch (event_type)
    {
    case BT_EVENT_USER_CONFIRMATION_REQ:
        {
#if GFPS_FEATURE_SUPPORT
            if (extend_app_cfg_const.gfps_support)
            {
                app_gfps_handle_bt_user_confirm(param->user_confirmation_req);
            }
            else
#endif
            {
                legacy_bond_user_cfm(param->user_confirmation_req.bd_addr, GAP_CFM_CAUSE_ACCEPT);
            }
        }
        break;

    default:
        break;
    }
}

void app_gap_init(void)
{
    gap_register_app_cb(app_gap_common_callback);

    bt_mgr_cback_register(app_gap_bt_cback);
}
