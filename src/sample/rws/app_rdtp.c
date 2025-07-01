/*
 * Copyright (c) 2018, Realsil Semiconductor Corporation. All rights reserved.
 */

#if F_APP_ERWS_SUPPORT
#include "trace.h"
#include "btm.h"
#include "app_rdtp.h"
#include "app_link_util.h"
#include "app_report.h"
#include "remote.h"
#include "app_main.h"
#include "app_cfg.h"
#include "bt_rdtp.h"
#include "app_bond.h"
#include "app_report.h"
#include "app_bt_policy_api.h"

static void app_rdtp_bt_cback(T_BT_EVENT event_type, void *event_buf, uint16_t buf_len)
{
    T_BT_EVENT_PARAM *param = event_buf;
    bool handle = true;

    switch (event_type)
    {
    case BT_EVENT_REMOTE_CONN_CMPL:
        {
            T_APP_BR_LINK *p_link;

            p_link = app_find_br_link(param->remote_conn_cmpl.bd_addr);
            if (p_link != NULL)
            {
                app_db.remote_session_state = REMOTE_SESSION_STATE_CONNECTED;
                app_report_rws_state();
                app_report_rws_bud_info();
                app_sync_b2s_link_record();
            }
            app_bt_policy_qos_param_update(param->remote_conn_cmpl.bd_addr, BP_TPOLL_B2B_CONN_EVENT);
        }
        break;

    case BT_EVENT_REMOTE_DISCONN_CMPL:
        {
            T_APP_BR_LINK *p_link;

            p_link = app_find_br_link(param->remote_disconn_cmpl.bd_addr);
            if (p_link != NULL)
            {
                app_db.remote_session_state = REMOTE_SESSION_STATE_DISCONNECTED;
                app_report_rws_state();
                app_report_rws_bud_info();
            }
            app_bt_policy_qos_param_update(param->remote_disconn_cmpl.bd_addr, BP_TPOLL_B2B_DISC_EVENT);
        }
        break;

    default:
        handle = false;
        break;
    }

    if (handle == true)
    {
        APP_PRINT_INFO2("app_rdtp_bt_cback: event_type 0x%04x, remote_session_state %d", event_type,
                        app_db.remote_session_state);
    }
}

void app_rdtp_init(void)
{
    if (app_cfg_const.supported_profile_mask & RDTP_PROFILE_MASK)
    {
        bt_rdtp_init();
        bt_mgr_cback_register(app_rdtp_bt_cback);
    }
}

#endif
