/*
 * Copyright (c) 2018, Realsil Semiconductor Corporation. All rights reserved.
 */

#include <string.h>
#include "trace.h"
#include "gap_timer.h"
#include "gap_le.h"
#include "btm.h"

#include "engage.h"
#include "app_cfg.h"
#include "app_main.h"
#include "app_bt_policy_api.h"
#include "app_bt_policy_int.h"
#include "app_linkback.h"
#include "app_relay.h"
#include "app_hfp.h"
#include "app_multilink.h"
#if F_APP_TEAMS_BT_POLICY
#include "app_teams_hid.h"
#include "app_teams_cfu.h"
#endif

extern T_LINKBACK_ACTIVE_NODE linkback_active_node;

extern T_BP_STATE bp_state;
extern T_EVENT cur_event;

extern T_BT_DEVICE_MODE radio_mode;

extern bool b2b_connected;
extern T_B2S_CONNECTED b2s_connected;
extern bool first_connect_sync_default_volume_to_src;
static uint8_t enlarge_tpoll_state = false;

const T_BP_TPOLL_MAPPING tpoll_table[BP_TPOLL_MAX] =
{
    {BP_TPOLL_INIT, 0},
#if F_APP_TEAMS_FEATURE_SUPPORT
    {BP_TPOLL_IDLE, 16},
#else
    {BP_TPOLL_IDLE, 40},
#endif
    {BP_TPOLL_GAMING_A2DP, 22},
    {BP_TPOLL_GAMING_8753BAU_RWS, 6},
    {BP_TPOLL_GAMING_8753BAU_RWS_SINGLE, 12},
    {BP_TPOLL_GAMING_8753BAU_STEREO, 6},
#if F_APP_TEAMS_FEATURE_SUPPORT
    {BP_TPOLL_A2DP, 10},
#else
    {BP_TPOLL_A2DP, 16},
#endif
    {BP_TPOLL_TEAMS_UPDATE, 16},
    {BP_TPOLL_IDLE_SINGLE_LINKBACK, 120},
};

void app_bt_policy_startup(T_BP_IND_FUN fun, bool at_once_trigger)
{
    T_STARTUP_PARAM param;

    param.ind_fun = fun;
    param.at_once_trigger = at_once_trigger;
    state_machine(EVENT_STARTUP, &param);
}

void app_bt_policy_shutdown(void)
{
    state_machine(EVENT_SHUTDOWN, NULL);
}

void app_bt_policy_stop(void)
{
    state_machine(EVENT_STOP, NULL);
}

void app_bt_policy_restore(void)
{
    state_machine(EVENT_RESTORE, NULL);
}

void app_bt_policy_prepare_for_roleswap(void)
{
    state_machine(EVENT_PREPARE_FOR_ROLESWAP, NULL);
}

void app_bt_policy_msg_prof_conn(uint8_t *bd_addr, uint32_t prof)
{
    T_BT_PARAM bt_param;

    memset(&bt_param, 0, sizeof(T_BT_PARAM));

    bt_param.bd_addr = bd_addr;
    bt_param.prof = prof;

    state_machine(EVENT_PROFILE_CONN_SUC, &bt_param);
}

void app_bt_policy_msg_prof_disconn(uint8_t *bd_addr, uint32_t prof, uint16_t cause)
{
    T_BT_PARAM bt_param;

    memset(&bt_param, 0, sizeof(T_BT_PARAM));

    bt_param.bd_addr = bd_addr;
    bt_param.prof = prof;
    bt_param.cause = cause;

    state_machine(EVENT_PROFILE_DISCONN, &bt_param);
}

void app_bt_policy_enter_pairing_mode(bool force, bool visiable)
{
    T_BT_PARAM bt_param;

    memset(&bt_param, 0, sizeof(T_BT_PARAM));

    bt_param.is_force = force;
    bt_param.is_visiable = visiable;

    state_machine(EVENT_DEDICATED_ENTER_PAIRING_MODE, &bt_param);
}

void app_bt_policy_exit_pairing_mode(void)
{
    state_machine(EVENT_DEDICATED_EXIT_PAIRING_MODE, NULL);
}

void app_bt_policy_enter_dut_test_mode(void)
{
    state_machine(EVENT_ENTER_DUT_TEST_MODE, NULL);
}

#if (F_APP_OTA_TOOLING_SUPPORT == 1)
void app_bt_policy_start_ota_shaking(void)
{
    state_machine(EVENT_START_OTA_SHAKING, NULL);
}

void app_bt_policy_enter_ota_mode(bool connectable)
{
    T_BT_PARAM bt_param;

    memset(&bt_param, 0, sizeof(T_BT_PARAM));

    bt_param.is_connectable = connectable;

    state_machine(EVENT_ENTER_OTA_MODE, &bt_param);
}
#endif

void app_bt_policy_default_connect(uint8_t *bd_addr, uint32_t plan_profs, bool check_bond_flag)
{
    T_BT_PARAM bt_param;

    memset(&bt_param, 0, sizeof(T_BT_PARAM));

    bt_param.bd_addr = bd_addr;
    bt_param.prof = plan_profs;
    bt_param.is_special = false;
    bt_param.check_bond_flag = check_bond_flag;
    state_machine(EVENT_DEDICATED_CONNECT, &bt_param);
}

void app_bt_policy_special_connect(uint8_t *bd_addr, uint32_t plan_prof,
                                   T_LINKBACK_SEARCH_PARAM *search_param)
{
    T_BT_PARAM bt_param;

    memset(&bt_param, 0, sizeof(T_BT_PARAM));

    bt_param.bd_addr = bd_addr;
    bt_param.prof = plan_prof;
    bt_param.is_special = true;
    bt_param.search_param = search_param;
    bt_param.check_bond_flag = false;
    state_machine(EVENT_DEDICATED_CONNECT, &bt_param);
}

void app_bt_policy_disconnect(uint8_t *bd_addr, uint32_t plan_profs)
{
    T_BT_PARAM bt_param;

    ENGAGE_PRINT_TRACE2("app_bt_policy_disconnect: bd_addr %s, prof 0x%08x", TRACE_BDADDR(bd_addr),
                        plan_profs);

    memset(&bt_param, 0, sizeof(T_BT_PARAM));

    bt_param.bd_addr = bd_addr;
    bt_param.prof = plan_profs;
    state_machine(EVENT_DEDICATED_DISCONNECT, &bt_param);
}

void app_bt_policy_disconnect_all_link(void)
{
    state_machine(EVENT_DISCONNECT_ALL, NULL);
}

T_BP_STATE app_bt_policy_get_state(void)
{
    return bp_state;
}

T_BT_DEVICE_MODE app_bt_policy_get_radio_mode(void)
{
    return radio_mode;
}

bool app_bt_policy_get_b2b_connected(void)
{
    return b2b_connected;
}

uint8_t app_bt_policy_get_b2s_connected_num(void)
{
    return b2s_connected.num;
}

uint8_t app_bt_policy_get_b2s_connected_num_with_profile(void)
{
    uint8_t phone_link_num = 0;

    for (uint8_t i = 0; i < MAX_BR_LINK_NUM; i++)
    {
        if (app_db.br_link[i].connected_profile & (~RDTP_PROFILE_MASK))
        {
            phone_link_num++;
        }
    }

    return phone_link_num;
}

void app_bt_policy_set_b2s_connected_num_max(uint8_t num_max)
{
    b2s_connected.num_max = num_max;
}

void app_bt_policy_sync_b2s_connected(void)
{
    app_db.b2s_connected_num = app_bt_policy_get_b2s_connected_num();

    if ((app_db.remote_session_state == REMOTE_SESSION_STATE_CONNECTED) &&
        (app_cfg_nv.bud_role == REMOTE_SESSION_ROLE_PRIMARY))
    {
        app_relay_async_single(APP_MODULE_TYPE_MULTI_LINK, APP_REMOTE_MSG_PHONE_CONNECTED);
    }
}

bool app_bt_policy_get_first_connect_sync_default_vol_flag(void)
{
    return first_connect_sync_default_volume_to_src;
}

void app_bt_policy_set_first_connect_sync_default_vol_flag(bool flag)
{
    first_connect_sync_default_volume_to_src = flag;
}

uint8_t app_bt_policy_find_a2dp_inacitve(void)
{
    uint8_t find_another_app_idx = MAX_BR_LINK_NUM;

    for (uint8_t i = 0; i < MAX_BR_LINK_NUM; i++)
    {
        if (app_check_b2s_link_by_id(i))
        {
            if (i != app_get_active_a2dp_idx())
            {
                find_another_app_idx = i;
            }
        }
    }
    return find_another_app_idx;
}

void app_bt_policy_set_idle_tpoll(T_BP_TPOLL_STATE tpoll_state)
{
    if (tpoll_state == BP_TPOLL_GAMING_A2DP ||
        tpoll_state == BP_TPOLL_A2DP)
    {
        if ((app_bt_policy_find_a2dp_inacitve() != MAX_BR_LINK_NUM) &&
            (app_db.br_link[app_bt_policy_find_a2dp_inacitve()].tpoll_status !=
             tpoll_table[BP_TPOLL_IDLE].state))
        {
            bt_link_qos_set(app_db.br_link[app_bt_policy_find_a2dp_inacitve()].bd_addr, BT_QOS_TYPE_GUARANTEED,
                            (tpoll_table[BP_TPOLL_IDLE].tpoll_value + (app_bt_policy_find_a2dp_inacitve() * 2)));
            app_db.br_link[app_bt_policy_find_a2dp_inacitve()].tpoll_status = tpoll_table[BP_TPOLL_IDLE].state;
        }
    }
    else
    {
        for (uint8_t i = 0; i < MAX_BR_LINK_NUM; i++)
        {
            if (app_check_b2s_link_by_id(i))
            {
                if (app_db.br_link[i].tpoll_status != tpoll_table[tpoll_state].state)
                {
                    bt_link_qos_set(app_db.br_link[i].bd_addr, BT_QOS_TYPE_GUARANTEED,
                                    (tpoll_table[tpoll_state].tpoll_value + (i * 2)));
                    app_db.br_link[i].tpoll_status = tpoll_table[tpoll_state].state;
                }
            }
        }
    }
}

void app_bt_policy_set_active_tpoll_only(uint8_t idx, T_BP_TPOLL_STATE tpoll_state)
{
    if (app_check_b2s_link_by_id(idx))
    {
        if (app_db.br_link[idx].tpoll_status != tpoll_table[tpoll_state].state)
        {
            bt_link_qos_set(app_db.br_link[idx].bd_addr, BT_QOS_TYPE_GUARANTEED,
                            tpoll_table[tpoll_state].tpoll_value);
            app_db.br_link[idx].tpoll_status = tpoll_table[tpoll_state].state;
        }
    }
}

uint8_t app_bt_policy_get_enlarge_tpoll_state(void)
{
    return enlarge_tpoll_state;
}

void app_bt_policy_set_enlarge_tpoll_state(T_BP_TPOLL_EVENT event)
{
    if (((event == BP_TPOLL_ACL_CONN_EVENT) ||
         (event == BP_TPOLL_LINKBACK_START)) &&
        (app_find_b2s_link_num() == 1) &&
        (app_bt_policy_get_state() == BP_STATE_LINKBACK) &&
        (app_db.br_link[app_get_active_a2dp_idx()].streaming_fg == false))
    {
        //page b2s when already conn_1_b2s
        enlarge_tpoll_state = true;
    }
    else if ((event != BP_TPOLL_B2B_CONN_EVENT) &&
             (event != BP_TPOLL_B2B_DISC_EVENT))
    {
        enlarge_tpoll_state = false;
    }

    APP_PRINT_TRACE2("app_bt_policy_get_enlarge_tpoll_state %d,%d", event,
                     enlarge_tpoll_state);
}

void app_bt_policy_qos_param_update(uint8_t *bd_addr, T_BP_TPOLL_EVENT event)
{
    uint8_t find_bau_app_idx = MAX_BR_LINK_NUM;
    uint8_t app_idx = app_get_active_a2dp_idx();
#if F_APP_TEAMS_BT_POLICY
    T_APP_BR_LINK *p_teams_link = NULL;
#endif

    app_bt_policy_set_enlarge_tpoll_state(event);

    if ((app_db.gaming_mode) && (app_db.remote_is_8753bau))
    {
        //only one link now, TODO: support multilink tpoll should be modified to other setting.
        for (uint8_t i = 0; i < MAX_BR_LINK_NUM; i++)
        {
            if (app_check_b2s_link_by_id(i))
            {
                find_bau_app_idx = i;
                break;
            }
        }

        if (find_bau_app_idx != MAX_BR_LINK_NUM)
        {
            if (app_db.remote_session_state == REMOTE_SESSION_STATE_CONNECTED) //RWS
            {
                app_bt_policy_set_active_tpoll_only(find_bau_app_idx, BP_TPOLL_GAMING_8753BAU_RWS);
            }
            else
            {
                if (app_cfg_nv.bud_role == REMOTE_SESSION_ROLE_SINGLE) //STEREO
                {
                    app_bt_policy_set_active_tpoll_only(find_bau_app_idx, BP_TPOLL_GAMING_8753BAU_STEREO);
                }
                else//RWS_SINGLE
                {
                    app_bt_policy_set_active_tpoll_only(find_bau_app_idx, BP_TPOLL_GAMING_8753BAU_RWS_SINGLE);
                }
            }
        }
    }
    else if ((app_cfg_const.enable_multi_link) &&
             (app_find_b2s_link_num() == MULTILINK_SRC_CONNECTED))
    {
        if (app_hfp_sco_is_connected()) //sco
        {
            app_bt_policy_set_idle_tpoll(BP_TPOLL_IDLE);
        }
        else if (app_db.br_link[app_idx].streaming_fg == true)//a2dp
        {
            if (app_db.remote_session_state == REMOTE_SESSION_STATE_CONNECTED) //RWS
            {
                app_bt_policy_set_idle_tpoll(BP_TPOLL_IDLE);
            }
            else//STEREO
            {
                if (app_db.gaming_mode)
                {
                    app_bt_policy_set_idle_tpoll(BP_TPOLL_GAMING_A2DP);

                    app_bt_policy_set_active_tpoll_only(app_idx, BP_TPOLL_GAMING_A2DP);
                }
                else
                {
                    app_bt_policy_set_idle_tpoll(BP_TPOLL_A2DP);

                    app_bt_policy_set_active_tpoll_only(app_idx, BP_TPOLL_A2DP);
                }
            }
        }
#if F_APP_TEAMS_BT_POLICY
        /* if there is anyone link is cfu or vp update running*/
        else if (app_hid_vp_update_is_process_check(&p_teams_link) ||
                 app_teams_cfu_is_process_check(&p_teams_link))
        {
            /*set qos of teams update running link to 16 */
            uint8_t index = p_teams_link->id;
            bt_link_qos_set(p_teams_link->bd_addr, BT_QOS_TYPE_GUARANTEED,
                            tpoll_table[BP_TPOLL_TEAMS_UPDATE].tpoll_value);
            app_db.br_link[index].tpoll_status = tpoll_table[BP_TPOLL_TEAMS_UPDATE].state;

            /* set the other link qos to 40*/
            for (uint8_t i = 0; i < MAX_BR_LINK_NUM; i++)
            {
                if (app_check_b2s_link_by_id(i))
                {
                    if ((index != i) && (app_db.br_link[i].tpoll_status != tpoll_table[BP_TPOLL_IDLE].state))
                    {
                        bt_link_qos_set(app_db.br_link[i].bd_addr, BT_QOS_TYPE_GUARANTEED,
                                        (tpoll_table[BP_TPOLL_IDLE].tpoll_value + (i * 2)));
                        app_db.br_link[i].tpoll_status = tpoll_table[BP_TPOLL_IDLE].state;
                    }
                }
            }
        }
#endif
        else //idle
        {
            app_bt_policy_set_idle_tpoll(BP_TPOLL_IDLE);
        }
    }
    else
    {
        if (app_bt_policy_get_enlarge_tpoll_state() == true)
        {
            app_bt_policy_set_active_tpoll_only(app_idx, BP_TPOLL_IDLE_SINGLE_LINKBACK);
        }
        else
        {
            if (app_hfp_sco_is_connected()) //sco
            {
                app_bt_policy_set_idle_tpoll(BP_TPOLL_IDLE);
            }
            else if (app_db.br_link[app_idx].streaming_fg == true)//a2dp
            {
                if (app_db.gaming_mode)
                {
                    app_bt_policy_set_active_tpoll_only(app_idx, BP_TPOLL_GAMING_A2DP);
                }
                else
                {
                    app_bt_policy_set_active_tpoll_only(app_idx, BP_TPOLL_A2DP);
                }
            }
            else
            {
                app_bt_policy_set_idle_tpoll(BP_TPOLL_IDLE);
            }
        }
    }

    APP_PRINT_TRACE8("app_bt_policy_qos_param_update:event %u, sco %d, a2dp_idx %d, avrcp %d, stream %d, active_tpoll %d, inactive_idx %d, bau_idx %d",
                     event,
                     app_hfp_sco_is_connected(),
                     app_idx,
                     app_db.br_link[app_idx].avrcp_play_status,
                     app_db.br_link[app_idx].streaming_fg,
                     app_db.br_link[app_idx].tpoll_status,
                     app_bt_policy_find_a2dp_inacitve(),
                     find_bau_app_idx);
}

uint8_t *app_bt_policy_get_linkback_device(void)
{
    if (linkback_active_node.is_valid)
    {
        return linkback_active_node.linkback_node.bd_addr;
    }
    else
    {
        return NULL;
    }
}
