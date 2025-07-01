/*
 * Copyright (c) 2018, Realsil Semiconductor Corporation. All rights reserved.
 */

#include "trace.h"
#include "app_gfps_device.h"
#include "app_gfps.h"
#include "app_main.h"
#include "app_gfps_account_key.h"
#include "app_gfps_personalized_name.h"
#include "app_cfg.h"
#include "app_bt_policy_api.h"
#include "bt_bond.h"
#include "app_bond.h"
#include "app_sass_policy.h"

#if GFPS_FEATURE_SUPPORT
#include "app_gfps_battery.h"

#if GFPS_FINDER_SUPPORT
#include "app_gfps_finder.h"
#include "app_gfps_finder_adv.h"
#include "app_adv_stop_cause.h"
#endif

void app_gfps_handle_power_off(void)
{
    APP_PRINT_TRACE0("app_gfps_handle_power_off");
    app_gfps_battery_show_ui(true);

    /*disable gfps adv and disconnect gfps ble link*/
    app_gfps_next_action(GFPS_ACTION_IDLE);

#if GFPS_SASS_SUPPORT
    if (app_sass_policy_support_easy_connection_switch())
    {
        if (gfps_get_inuse_account_key_index() != 0xff)
        {
            app_cfg_nv.sass_recently_used_account_key_index = gfps_get_inuse_account_key_index();
        }
        else if (gfps_get_recently_inuse_account_key_index() != 0xff)
        {
            app_cfg_nv.sass_recently_used_account_key_index = gfps_get_recently_inuse_account_key_index();
        }
        app_cfg_store(&app_cfg_nv.sass_recently_used_account_key_index, 1);
    }
#endif

#if GFPS_FINDER_SUPPORT
    if (extend_app_cfg_const.gfps_finder_support)
    {
        app_gfps_finder_handle_power_off();
    }
#endif
}

void app_gfps_handle_power_on(bool is_pairing)
{
    APP_PRINT_TRACE0("app_gfps_handle_power_on");

#if GFPS_SASS_SUPPORT
    if (app_sass_policy_support_easy_connection_switch())
    {
        gfps_set_most_recently_inuse_account_key_index(app_cfg_nv.sass_recently_used_account_key_index);
    }
#endif

#if GFPS_FINDER_SUPPORT
    if (extend_app_cfg_const.gfps_finder_support)
    {
        app_gfps_finder_set_ring_param();
    }
#endif

    if (app_cfg_nv.bud_role == REMOTE_SESSION_ROLE_SINGLE)
    {
        uint8_t bond_num = app_b2s_bond_num_get();
        if ((!is_pairing) && (bond_num != 0))
        {
            app_gfps_next_action(GFPS_ACTION_ADV_NOT_DISCOVERABLE_MODE);
        }
#if GFPS_FINDER_SUPPORT
        if (extend_app_cfg_const.gfps_finder_support)
        {
            T_GFPS_FINDER_BEACON_STATE beacon_state = GFPS_FINDER_BEACON_STATE_ON;
            uint32_t adv_interval = extend_app_cfg_const.gfps_power_on_finder_adv_interval;

            gfps_finder_adv_update_adv_interval(adv_interval);
            app_gfps_finder_enter_beacon_state(beacon_state);
        }
#endif
    }
    else
    {
        //rws mode, wait role discieded info
    }
}

void app_gfps_handle_radio_mode(T_BT_DEVICE_MODE radio_mode)
{
    APP_PRINT_TRACE1("app_gfps_handle_radio_mode: radio_mode %d", radio_mode);
    app_gfps_check_state();
}

void app_gfps_handle_engage_role_decided(T_BT_DEVICE_MODE radio_mode)
{
    APP_PRINT_TRACE1("app_gfps_handle_engage_role_decided: bud_role %d", app_cfg_nv.bud_role);
#if GFPS_FINDER_SUPPORT
    if (extend_app_cfg_const.gfps_finder_support)
    {
        app_gfps_finder_set_ring_param();
    }
#endif

    if (app_cfg_nv.bud_role != REMOTE_SESSION_ROLE_SECONDARY)
    {
        app_gfps_check_state();
#if GFPS_FINDER_SUPPORT
        if (extend_app_cfg_const.gfps_finder_support)
        {
            T_GFPS_FINDER_BEACON_STATE beacon_state = GFPS_FINDER_BEACON_STATE_ON;
            app_gfps_finder_enter_beacon_state(beacon_state);
        }
#endif
    }
    else
    {
        app_gfps_next_action(GFPS_ACTION_IDLE);
#if GFPS_FINDER_SUPPORT
        if (extend_app_cfg_const.gfps_finder_support)
        {
            T_GFPS_FINDER_BEACON_STATE beacon_state = GFPS_FINDER_BEACON_STATE_OFF;
            app_gfps_finder_enter_beacon_state(beacon_state);
        }
#endif
    }

    if (app_cfg_const.bud_role == REMOTE_SESSION_ROLE_PRIMARY)
    {
        if (app_db.remote_session_state == REMOTE_SESSION_STATE_CONNECTED)
        {
            app_gfps_battery_info_report(GFPS_BATTERY_REPORT_ENGAGE_SUCCESS);
        }
        else
        {
            app_gfps_battery_info_report(GFPS_BATTERY_REPORT_ENGAGE_FAIL);
        }
    }
}

void app_gfps_handle_role_swap(T_REMOTE_SESSION_ROLE device_role)
{
    APP_PRINT_TRACE1("app_gfps_handle_role_swap: device_role %d", device_role);
    if (device_role == REMOTE_SESSION_ROLE_PRIMARY)
    {
        app_gfps_check_state();
        app_gfps_battery_info_report(GFPS_BATTERY_REPORT_ROLESWAP_SUCCESS);
#if GFPS_FINDER_SUPPORT
        if (extend_app_cfg_const.gfps_finder_support)
        {
            T_GFPS_FINDER_BEACON_STATE beacon_state = GFPS_FINDER_BEACON_STATE_ON;
            app_gfps_finder_enter_beacon_state(beacon_state);
        }
#endif
    }
    else
    {
        app_gfps_next_action(GFPS_ACTION_IDLE);
#if GFPS_FINDER_SUPPORT
        if (extend_app_cfg_const.gfps_finder_support)
        {
            T_GFPS_FINDER_BEACON_STATE beacon_state = GFPS_FINDER_BEACON_STATE_OFF;
            app_gfps_finder_enter_beacon_state(beacon_state);
        }
#endif
    }
}
void app_gfps_handle_b2s_link_disconnected(void)
{
    APP_PRINT_TRACE0("app_gfps_handle_b2s_link_disconnected");
    app_gfps_check_state();
}

void app_gfps_handle_ble_link_disconnected(void)
{
    APP_PRINT_TRACE0("app_gfps_handle_ble_link_disconnected");
    gfps_reset_aeskey_and_pairing_mode();
    app_gfps_check_state();
}

void app_gfps_handle_remote_conn_cmpl(void)
{
#if GFPS_FINDER_SUPPORT
    if (extend_app_cfg_const.gfps_finder_support)
    {
        app_gfps_finder_set_ring_param();
    }
#endif

    if (app_cfg_nv.bud_role == REMOTE_SESSION_ROLE_PRIMARY)
    {
        APP_PRINT_INFO0("app_gfps_handle_remote_conn_cmpl:pri sync account key and name");
#if GFPS_FINDER_SUPPORT
        if (extend_app_cfg_const.gfps_finder_support)
        {
            app_gfps_remote_owner_address_sync();
        }
#endif
        app_gfps_remote_account_key_sync();
#if GFPS_PERSONALIZED_NAME_SUPPORT
        app_gfps_remote_personalized_name();
#endif
        app_gfps_battery_info_report(GFPS_BATTERY_REPORT_b2b_CONNECT_SUCCESS);
#if GFPS_FINDER_SUPPORT
        if (extend_app_cfg_const.gfps_finder_support)
        {
            T_GFPS_FINDER_BEACON_STATE beacon_state = GFPS_FINDER_BEACON_STATE_ON;
            app_gfps_finder_enter_beacon_state(beacon_state);
        }
#endif
    }
}

void app_gfps_handle_remote_disconnect_cmpl(void)
{
#if GFPS_FINDER_SUPPORT
    if (extend_app_cfg_const.gfps_finder_support)
    {
        app_gfps_finder_set_ring_param();
    }
#endif

    if (app_cfg_nv.bud_role == REMOTE_SESSION_ROLE_PRIMARY)
    {
        app_gfps_battery_info_report(GFPS_BATTERY_REPORT_b2b_DISCONNECT);
    }
}

void app_gfps_handle_factory_reset(void)
{
    app_gfps_account_key_clear();

#if GFPS_PERSONALIZED_NAME_SUPPORT
    app_gfps_personalized_name_clear();
#endif

#if GFPS_FINDER_SUPPORT
    if (extend_app_cfg_const.gfps_finder_support)
    {
        app_gfps_finder_handle_factory_reset();
    }
#endif
}

#endif
