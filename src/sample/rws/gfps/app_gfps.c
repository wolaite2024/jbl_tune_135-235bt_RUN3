/*
 * Copyright (c) 2018, Realsil Semiconductor Corporation. All rights reserved.
 */

#include "os_mem.h"
#include "trace.h"
#include "gap_bond_legacy.h"
#include "gap_bond_le.h"
#include "bt_gfps.h"
#include "gfps.h"
#include "app_gfps_timer.h"
#include "ble_ext_adv.h"
#include "ble_conn.h"
#include "app_gfps.h"
#include "app_gfps_account_key.h"
#include "app_gfps_personalized_name.h"
#include "app_ble_gap.h"
#include "app_cfg.h"
#include "app_main.h"
#include "app_bt_policy_api.h"
#include "app_hfp.h"
#if GFPS_FEATURE_SUPPORT
#include "app_gfps_battery.h"
#include "app_gfps_rfc.h"
#include "app_multilink.h"
#include "app_sass_policy.h"
#include "app_bond.h"
T_GFPS_DB gfps_db;
#if GFPS_FINDER_SUPPORT
#include "gfps_find_my_device.h"
#include "app_gfps_finder.h"
#endif
typedef struct
{
    bool                    is_gfps_pairing;
    bool                    gfps_raw_passkey_received;
    bool                    le_bond_confirm_pending;
    bool                    edr_bond_confirm_pending;
    bool                    auth_param_change;
    uint8_t                 io_cap;
    uint16_t                auth_flags;
    uint32_t                gfps_raw_passkey;
    uint32_t                le_bond_passkey;
    uint32_t                edr_bond_passkey;
    uint8_t                 edr_bond_bd_addr[6];
} T_GFPS_LINK;

T_GFPS_LINK gfps_link;

uint8_t gfps_adv_len;
uint8_t gfps_adv_data[GAP_MAX_LEGACY_ADV_LEN];

extern T_APP_RESULT app_gfps_cb(T_SERVER_ID service_id, void *p_data);
extern void app_gfps_le_disconnect_cb(uint8_t conn_id, uint8_t local_disc_cause,
                                      uint16_t disc_cause);
extern void app_gfps_handle_ble_link_disconnected(void);

static uint8_t gfps_scan_rsp_data[] =
{
    0x09,/* length */
    GAP_ADTYPE_LOCAL_NAME_COMPLETE,/* type="device name" */
    'G', 'F', 'P', 'S', '_', 'A', 'D', 'V',
};

/*google Fast pair initialize*/
void app_gfps_init(void)
{
    uint8_t sec_req_enable = false;
    if (app_gfps_account_key_init(extend_app_cfg_const.gfps_account_key_num) == false)
    {
        goto error;
    }
#if GFPS_SASS_SUPPORT
    if (app_sass_policy_support_easy_connection_switch())
    {
        gfps_sass_conn_status_init();
    }
#endif

#if GFPS_PERSONALIZED_NAME_SUPPORT
    app_gfps_personalized_name_init();
#endif
    le_bond_set_param(GAP_PARAM_BOND_SEC_REQ_ENABLE, sizeof(uint8_t), &sec_req_enable);
    if (gfps_init(extend_app_cfg_const.gfps_model_id, extend_app_cfg_const.gfps_public_key,
                  extend_app_cfg_const.gfps_private_key) == false)
    {
        goto error;
    }

    gfps_set_finder_enable(extend_app_cfg_const.gfps_finder_support);
    gfps_set_sass_enable(extend_app_cfg_const.gfps_sass_support);
    gfps_set_tx_power(extend_app_cfg_const.gfps_enable_tx_power, extend_app_cfg_const.gfps_tx_power);
    app_gfps_battery_info_init();
    gfps_db.gfps_conn_id = 0xFF;
    gfps_db.gfps_id = gfps_add_service(app_gfps_cb);
    app_gfps_relay_init();
    app_gfps_timer_init();
    return;

error:
    APP_PRINT_ERROR0("app_gfps_init: failed");
}

void app_gfps_get_random_addr(uint8_t *random_bd)
{
    memcpy(random_bd, gfps_db.random_address, GAP_BD_ADDR_LEN);
}

bool app_gfps_adv_start(T_GFPS_ADV_MODE mode, bool show_ui)
{
    bool update_addr = true;
    uint8_t random_address[6] = {0};

    app_gfps_timer_stop_update_rpa();
    app_gfps_timer_start_update_rpa();

#if GFPS_FINDER_SUPPORT
    if (extend_app_cfg_const.gfps_finder_support)
    {
        //if (app_gfps_finder_provisoned() != true)
        //{
        //    update_addr = false;
        //}
        update_addr = false;
    }
#endif

    if (gfps_gen_adv_data(mode, gfps_adv_data, &gfps_adv_len, show_ui))
    {
        uint16_t interval = extend_app_cfg_const.gfps_discov_adv_interval;

        if (mode == NOT_DISCOVERABLE_MODE)
        {
            interval = extend_app_cfg_const.gfps_not_discov_adv_interval;
        }

        if (update_addr)
        {
            le_gen_rand_addr(GAP_RAND_ADDR_RESOLVABLE, random_address);
            memcpy(gfps_db.random_address, random_address, GAP_BD_ADDR_LEN);
            /*should sent ble address to the Seeker when RFCOMM connects and whenever the address is rotated*/
            app_gfps_rfc_update_ble_addr();
            ble_ext_adv_mgr_set_multi_param(gfps_db.gfps_adv_handle, random_address,
                                            interval, gfps_adv_len, gfps_adv_data, 0, NULL);
        }
        else
        {
            ble_ext_adv_mgr_set_multi_param(gfps_db.gfps_adv_handle, NULL,
                                            interval, gfps_adv_len, gfps_adv_data, 0, NULL);
        }
    }
    else
    {
        APP_PRINT_ERROR0("app_gfps_adv_start: gfps_gen_adv_data failed");
        return false;
    }

    if (gfps_db.gfps_adv_state == BLE_EXT_ADV_MGR_ADV_DISABLED)
    {
        if (ble_ext_adv_mgr_enable(gfps_db.gfps_adv_handle, 0) == GAP_CAUSE_SUCCESS)
        {
            return true;
        }
    }
    return true;
}

T_GAP_CAUSE app_gfps_adv_update_adv_interval(uint32_t adv_interval)
{
    T_GAP_CAUSE result = GAP_CAUSE_SUCCESS;

    result = ble_ext_adv_mgr_change_adv_interval(gfps_db.gfps_adv_handle, adv_interval);
    APP_PRINT_TRACE2("app_gfps_adv_update_adv_interval: result %d, adv_interval %d", result,
                     adv_interval);
    return result;
}

void app_gfps_update_rpa(void)
{
    uint8_t random_address[6] = {0};
    le_gen_rand_addr(GAP_RAND_ADDR_RESOLVABLE, random_address);
    memcpy(gfps_db.random_address, random_address, GAP_BD_ADDR_LEN);
    app_gfps_rfc_update_ble_addr();
    ble_ext_adv_mgr_set_random(gfps_db.gfps_adv_handle, random_address);
    APP_PRINT_INFO1("app_gfps_update_rpa: RPA %b", TRACE_BDADDR(gfps_db.random_address));
}

bool app_gfps_next_action(T_GFPS_ACTION gfps_next_action)
{
    APP_PRINT_TRACE4("app_gfps_next_action: gfps_curr_action %d, gfps_next_action %d, gfps_adv_state %d, gfps_conn_state %d",
                     gfps_db.gfps_curr_action, gfps_next_action,
                     gfps_db.gfps_adv_state, gfps_db.gfps_conn_state);

    {
        if ((gfps_next_action != GFPS_ACTION_IDLE) &&
            (gfps_db.gfps_conn_state != GAP_CONN_STATE_DISCONNECTED))
        {
            return false;
        }

        gfps_db.gfps_curr_action = gfps_next_action;
        switch (gfps_db.gfps_curr_action)
        {
        case GFPS_ACTION_IDLE:
            {
                if (gfps_db.gfps_adv_state == BLE_EXT_ADV_MGR_ADV_ENABLED)
                {
                    ble_ext_adv_mgr_disable(gfps_db.gfps_adv_handle, APP_STOP_ADV_CAUSE_GFPS_ACTION_IDLE);
                }
                else if (gfps_db.gfps_conn_state == GAP_CONN_STATE_CONNECTED)
                {
                    T_APP_LE_LINK *p_link;
                    p_link = app_find_le_link_by_conn_id(gfps_db.gfps_conn_id);
                    app_ble_disconnect(p_link, LE_LOCAL_DISC_CAUSE_GFPS_STOP);
                }
            }
            break;

        case GFPS_ACTION_ADV_DISCOVERABLE_MODE_WITH_MODEL_ID:
            {
                app_gfps_adv_start(DISCOVERABLE_MODE_WITH_MODEL_ID, true);
            }
            break;

        case GFPS_ACTION_ADV_NOT_DISCOVERABLE_MODE:
            {
                app_gfps_battery_info_report(GFPS_BATTERY_REPORT_ADV_START);
                app_gfps_adv_start(NOT_DISCOVERABLE_MODE, true);
            }
            break;

        case GFPS_ACTION_ADV_NOT_DISCOVERABLE_MODE_HIDE_UI:
            {
                app_gfps_adv_start(NOT_DISCOVERABLE_MODE, false);
            }
            break;

        default:
            break;
        }
    }

    return true;
}

static void app_gfps_adv_callback(uint8_t cb_type, void *p_cb_data)
{
    T_BLE_EXT_ADV_CB_DATA cb_data;
    memcpy(&cb_data, p_cb_data, sizeof(T_BLE_EXT_ADV_CB_DATA));

    switch (cb_type)
    {
    case BLE_EXT_ADV_STATE_CHANGE:
        {
            gfps_db.gfps_adv_state = cb_data.p_ble_state_change->state;
            APP_PRINT_TRACE4("app_gfps_adv_callback: adv_handle %d, adv_state %d, stack stop cause 0x%x, app stop cause 0x%x",
                             cb_data.p_ble_state_change->adv_handle, gfps_db.gfps_adv_state,
                             cb_data.p_ble_state_change->stop_cause, cb_data.p_ble_state_change->app_cause);
            if (cb_data.p_ble_state_change->state == BLE_EXT_ADV_MGR_ADV_DISABLED)
            {
                gfps_db.gfps_curr_action = GFPS_ACTION_IDLE;
            }
        }
        break;

    case BLE_EXT_ADV_SET_CONN_INFO:
        {
            APP_PRINT_TRACE1("app_gfps_adv_callback: BLE_EXT_ADV_SET_CONN_INFO conn_id 0x%x",
                             cb_data.p_ble_conn_info->conn_id);
            app_reg_le_link_disc_cb(cb_data.p_ble_conn_info->conn_id, app_gfps_le_disconnect_cb);
            gfps_db.gfps_conn_state = GAP_CONN_STATE_CONNECTED;
            gfps_db.gfps_conn_id = cb_data.p_ble_conn_info->conn_id;
            gfps_db.gfps_curr_action = GFPS_ACTION_IDLE;
        }
        break;

    default:
        break;
    }
    return;
}

void app_gfps_adv_init(void)
{
    T_LE_EXT_ADV_LEGACY_ADV_PROPERTY adv_event_prop = LE_EXT_ADV_LEGACY_ADV_CONN_SCAN_UNDIRECTED;
    uint16_t adv_interval_min = extend_app_cfg_const.gfps_discov_adv_interval;
    uint16_t adv_interval_max = extend_app_cfg_const.gfps_discov_adv_interval;
    T_GAP_LOCAL_ADDR_TYPE own_address_type = GAP_LOCAL_ADDR_LE_RANDOM;
    T_GAP_REMOTE_ADDR_TYPE peer_address_type = GAP_REMOTE_ADDR_LE_PUBLIC;
    uint8_t  peer_address[6] = {0, 0, 0, 0, 0, 0};
    T_GAP_ADV_FILTER_POLICY filter_policy = GAP_ADV_FILTER_ANY;

    app_ble_gen_scan_rsp_data(&scan_rsp_data_len, scan_rsp_data);

    uint8_t random_address[6] = {0};
    T_GAP_CAUSE cause = le_gen_rand_addr(GAP_RAND_ADDR_RESOLVABLE, random_address);
    memcpy(gfps_db.random_address, random_address, GAP_BD_ADDR_LEN);

    ble_ext_adv_mgr_init_adv_params(&gfps_db.gfps_adv_handle, adv_event_prop, adv_interval_min,
                                    adv_interval_max, own_address_type, peer_address_type, peer_address,
                                    filter_policy, 0, NULL,
                                    scan_rsp_data_len, scan_rsp_data, random_address);

    if (ble_ext_adv_mgr_register_callback(app_gfps_adv_callback,
                                          gfps_db.gfps_adv_handle) == GAP_CAUSE_SUCCESS)
    {
        APP_PRINT_INFO3("app_gfps_adv_init: gfps_adv_handle %d, cause %d, gfps version %b",
                        gfps_db.gfps_adv_handle, cause, TRACE_STRING(GFPS_FIRMWARE_VERSION));
    }
}

/**
 * @brief app_gfps_le_disconnect_cb
 * gfps ble link disconnect callback
 * @param conn_id
 * @param local_disc_cause
 * @param disc_cause
 */
void app_gfps_le_disconnect_cb(uint8_t conn_id, uint8_t local_disc_cause, uint16_t disc_cause)
{
    APP_PRINT_TRACE4("app_gfps_le_disconnect_cb: conn_id %d, gfps_conn_id %d,local_disc_cause %d, disc_cause 0x%x",
                     conn_id, gfps_db.gfps_conn_id, local_disc_cause, disc_cause);

    if (conn_id != gfps_db.gfps_conn_id)
    {
        return;
    }

    if (gfps_link.auth_param_change)
    {
        //provider recover to original io capability
        gap_set_param(GAP_PARAM_BOND_IO_CAPABILITIES, sizeof(uint8_t), &(gfps_link.io_cap));
        gap_set_param(GAP_PARAM_BOND_AUTHEN_REQUIREMENTS_FLAGS, sizeof(uint16_t), &(gfps_link.auth_flags));
        gap_set_pairable_mode();
    }

    gfps_db.gfps_conn_state = GAP_CONN_STATE_DISCONNECTED;
    gfps_db.gfps_conn_id = 0xff;
    memset(&gfps_link, 0, sizeof(T_GFPS_LINK));
    app_gfps_handle_ble_link_disconnected();

#if GFPS_FINDER_SUPPORT
    /*gfps finder maybe connected by gfps ble link to do provising or provisioned actions,
    so when gfps ble link disconencted, we shall reset gfps finder link here*/
    if (extend_app_cfg_const.gfps_finder_support)
    {
        app_gfps_finder_reset_conn();
    }
#endif
}

void app_gfps_send_le_bond_confirm(uint8_t conn_id)
{
    if (gfps_link.le_bond_passkey == gfps_link.gfps_raw_passkey)
    {
        le_bond_user_confirm(conn_id, GAP_CFM_CAUSE_ACCEPT);
        gfps_send_passkey(conn_id, gfps_db.gfps_id, gfps_link.le_bond_passkey);
    }
    else
    {
        le_bond_user_confirm(conn_id, GAP_CFM_CAUSE_REJECT);
    }
    gfps_link.le_bond_confirm_pending = false;
}

void app_gfps_send_legacy_bond_confirm(uint8_t conn_id)
{
    if (gfps_link.edr_bond_passkey == gfps_link.gfps_raw_passkey)
    {
        legacy_bond_user_cfm(gfps_link.edr_bond_bd_addr, GAP_CFM_CAUSE_ACCEPT);
        gfps_send_passkey(conn_id, gfps_db.gfps_id, gfps_link.edr_bond_passkey);
    }
    else
    {
        legacy_bond_user_cfm(gfps_link.edr_bond_bd_addr, GAP_CFM_CAUSE_REJECT);
    }
    gfps_link.edr_bond_confirm_pending = false;
}

void app_gfps_handle_bt_user_confirm(T_BT_EVENT_PARAM_USER_CONFIRMATION_REQ confirm_req)
{
    APP_PRINT_INFO3("app_gfps_handle_bt_user_confirm: is_gfps_pairing %d, passkey %d, gfps_raw_passkey %d",
                    gfps_link.is_gfps_pairing, confirm_req.display_value, gfps_link.gfps_raw_passkey);
    if (gfps_link.is_gfps_pairing)
    {
        memcpy(gfps_link.edr_bond_bd_addr, confirm_req.bd_addr, 6);
        gfps_link.edr_bond_passkey = confirm_req.display_value;

        if (gfps_link.gfps_raw_passkey_received)
        {
            app_gfps_send_legacy_bond_confirm(gfps_db.gfps_conn_id);
            gfps_link.gfps_raw_passkey_received = false;
        }
        else
        {
            gfps_link.edr_bond_confirm_pending = true;
            //TODO: start 10s timer
        }
    }
    else
    {
        legacy_bond_user_cfm(confirm_req.bd_addr, GAP_CFM_CAUSE_ACCEPT);
    }
}

void app_gfps_handle_ble_user_confirm(uint8_t conn_id)
{
    uint32_t passkey = 0;
    le_bond_get_display_key(conn_id, &passkey);
    gfps_link.le_bond_passkey = passkey;

    APP_PRINT_INFO3("app_gfps_handle_ble_user_confirm: passkey %d, gfps_raw_passkey %d, is_gfps_pairing %d",
                    passkey,
                    gfps_link.gfps_raw_passkey, gfps_link.is_gfps_pairing);

    if (gfps_link.is_gfps_pairing)
    {
        if (gfps_link.gfps_raw_passkey_received)
        {
            app_gfps_send_le_bond_confirm(conn_id);
            gfps_link.gfps_raw_passkey_received = false;
        }
        else
        {
            gfps_link.le_bond_confirm_pending = true;
            //TODO: start 10s timer
        }
    }
    else
    {
        le_bond_user_confirm(conn_id, GAP_CFM_CAUSE_ACCEPT);
    }
}

void app_gfps_set_ble_conn_param(uint8_t conn_id)
{
    uint16_t conn_interval_min = 0x06;
    uint16_t conn_interval_max = 0x06;
    uint16_t conn_latency = 0;
    uint16_t conn_supervision_timeout = 500;

    ble_set_prefer_conn_param(conn_id, conn_interval_min, conn_interval_max, conn_latency,
                              conn_supervision_timeout);
}

/*Fast pair service callback*/
T_APP_RESULT app_gfps_cb(T_SERVER_ID service_id, void *p_data)
{
    uint8_t ret = 0;
    T_APP_RESULT app_result = APP_RESULT_SUCCESS;
    T_GFPS_CALLBACK_DATA *p_callback = (T_GFPS_CALLBACK_DATA *)p_data;

    switch (p_callback->msg_type)
    {
    case GFPS_CALLBACK_TYPE_NOTIFICATION_ENABLE:
        {
            APP_PRINT_INFO1("app_gfps_cb:notify_type %d", p_callback->msg_data.notify_type);
        }
        break;

    case GFPS_CALLBACK_TYPE_KBP_WRITE_REQ:
        {
            if (p_callback->msg_data.kbp.result == GFPS_WRITE_RESULT_SUCCESS)
            {
                if (gfps_db.gfps_conn_id != p_callback->conn_id)
                {
                    //Invalid connection, disconnect
                    T_APP_LE_LINK *p_link;
                    p_link = app_find_le_link_by_conn_id(p_callback->conn_id);
                    app_ble_disconnect(p_link, LE_LOCAL_DISC_CAUSE_GFPS_FAILED);
                    break;
                }

                APP_PRINT_INFO6("app_gfps_cb: GFPS_CALLBACK_TYPE_KBP_WRITE_REQ, provider_init_bond %d, notify_existing_name %d, "
                                "retroactively_account_key %d, pk_field_exist %d, account_key_idx %d, provider_addr %s",
                                p_callback->msg_data.kbp.provider_init_bond,
                                p_callback->msg_data.kbp.notify_existing_name,
                                p_callback->msg_data.kbp.retroactively_account_key,
                                p_callback->msg_data.kbp.pk_field_exist,
                                p_callback->msg_data.kbp.account_key_idx,
                                TRACE_BDADDR(p_callback->msg_data.kbp.provider_addr)
                               );

                if (p_callback->msg_data.kbp.retroactively_account_key == 1
                    || p_callback->msg_data.kbp.provider_init_bond == 1)
                {
                    APP_PRINT_INFO1("app_gfps_cb: seek_br_edr_addr %s",
                                    TRACE_BDADDR(p_callback->msg_data.kbp.seek_br_edr_addr));
                }

                if (p_callback->msg_data.kbp.retroactively_account_key == 1)
                {
                    T_APP_BR_LINK *p_link;
                    p_link = app_find_br_link(p_callback->msg_data.kbp.seek_br_edr_addr);
                    if (p_link == NULL)
                    {
                        ret = 1;
//                      APP_PRINT_ERROR0("app_gfps_cb: retroactively_account_key not find edr link");
                        app_result = APP_RESULT_REJECT;
                    }
                    else
                    {
                        memcpy(gfps_link.edr_bond_bd_addr, p_callback->msg_data.kbp.seek_br_edr_addr, 6);
                    }

                }
                else
                {
                    uint8_t io_cap = GAP_IO_CAP_DISPLAY_YES_NO;
                    uint16_t auth_flags;

                    gfps_link.is_gfps_pairing = true;
                    //provider shall set io capability to DISPLAY YES/NO for triggering numeric comparison pairing method
                    //set Authentication Requirements to MITM Protection Required
                    gap_get_param(GAP_PARAM_BOND_IO_CAPABILITIES, &(gfps_link.io_cap));
                    gap_get_param(GAP_PARAM_BOND_AUTHEN_REQUIREMENTS_FLAGS, &(gfps_link.auth_flags));
                    gfps_link.auth_param_change = true;
                    auth_flags = (gfps_link.auth_flags | GAP_AUTHEN_BIT_MITM_FLAG);

                    gap_set_param(GAP_PARAM_BOND_IO_CAPABILITIES, sizeof(uint8_t), &io_cap);
                    gap_set_param(GAP_PARAM_BOND_AUTHEN_REQUIREMENTS_FLAGS, sizeof(uint16_t), &auth_flags);
                    gap_set_pairable_mode();

                    if (p_callback->msg_data.kbp.provider_init_bond)
                    {
                        legacy_bond_pair(p_callback->msg_data.kbp.seek_br_edr_addr);
                    }
                }
#if GFPS_PERSONALIZED_NAME_SUPPORT
                //If the Request's Flags byte has bit 2 set to 1, notify the personalized name characteristic(p_callback->msg_data.kbp.notify_existing_name)
                if (p_callback->msg_data.kbp.notify_existing_name)
                {
                    /*need get personalized name from ftl and send the name into gfps*/
                    app_gfps_personalized_name_send(p_callback->conn_id, service_id);
                }
#endif

                if (p_callback->msg_data.kbp.pk_field_exist == 0)
                {
                    if (app_cfg_const.enable_multi_link)
                    {
                        if (app_bt_policy_get_b2s_connected_num() == app_cfg_const.max_legacy_multilink_devices)
                        {
                            if (app_hfp_get_call_status() == BT_HFP_CALL_IDLE)
                            {
                                ret = 2;
//                              APP_PRINT_INFO0("app_gfps_cb: disconnect b2s link for subsequent pairing, multilink enable");
                                app_bt_policy_enter_pairing_mode(true, false);
                            }
                        }
                    }
                    else
                    {
                        if (app_bt_policy_get_b2s_connected_num() > 0)
                        {
                            if (app_hfp_get_call_status() == BT_HFP_CALL_IDLE)
                            {
                                ret = 3;
//                              APP_PRINT_INFO0("app_gfps_cb: disconnect b2s link for subsequent pairing, multilink disable");
                                app_bt_policy_enter_pairing_mode(true, false);
                            }
                        }
                    }
                }
            }
            else
            {
                ret = 4;
//              APP_PRINT_ERROR1("app_gfps_cb: GFPS_CALLBACK_TYPE_KBP_WRITE_REQ failed, result %d",
//                              p_callback->msg_data.kbp.result);
            }
        }
        break;

    case GFPS_CALLBACK_TYPE_ACTION_REQ:
        {
            if (p_callback->msg_data.action_req.result == GFPS_WRITE_RESULT_SUCCESS)
            {
                APP_PRINT_INFO5("app_gfps_cb: GFPS_CALLBACK_TYPE_ACTION_REQ, pk_field_exist %d, account_key_idx %d, "
                                "device_action %d, additional_data %d, provider_addr %s",
                                p_callback->msg_data.action_req.pk_field_exist,
                                p_callback->msg_data.action_req.account_key_idx,
                                p_callback->msg_data.action_req.device_action,
                                p_callback->msg_data.action_req.additional_data,
                                TRACE_BDADDR(p_callback->msg_data.action_req.provider_addr)
                               );
                if (p_callback->msg_data.action_req.device_action)
                {
                    APP_PRINT_INFO4("app_gfps_cb: device_action, message_group %d, message_code %d, additional_data[%d] %b",
                                    p_callback->msg_data.action_req.message_group,
                                    p_callback->msg_data.action_req.message_code,
                                    p_callback->msg_data.action_req.additional_data_length,
                                    TRACE_BINARY(p_callback->msg_data.action_req.additional_data_length,
                                                 p_callback->msg_data.action_req.add_data));
                }
                if (p_callback->msg_data.action_req.additional_data)
                {
//                  APP_PRINT_INFO1("app_gfps_cb: additional_data, data_id 0x%x",
//                                  p_callback->msg_data.action_req.data_id);
                }
            }
            else
            {
                ret = 5;
//              APP_PRINT_ERROR1("app_gfps_cb: GFPS_CALLBACK_TYPE_ACTION_REQ failed, result %d",
//                               p_callback->msg_data.action_req.result);
            }
        }
        break;

    case GFPS_CALLBACK_TYPE_PASSKEY:
        {
            gfps_link.gfps_raw_passkey = p_callback->msg_data.passkey;
            gfps_link.gfps_raw_passkey_received = true;

//          APP_PRINT_INFO3("app_gfps_cb: GFPS_CALLBACK_TYPE_PASSKEY, gfps_raw_passkey %d ,le_bond_confirm_pending %d, edr_bond_confirm_pending %d",
//                          gfps_link.gfps_raw_passkey,
//                          gfps_link.le_bond_confirm_pending, gfps_link.edr_bond_confirm_pending);

            if (gfps_link.le_bond_confirm_pending)
            {
                app_gfps_send_le_bond_confirm(p_callback->conn_id);
                gfps_link.gfps_raw_passkey_received = false;
            }
            else if (gfps_link.edr_bond_confirm_pending)
            {
                app_gfps_send_legacy_bond_confirm(p_callback->conn_id);
                gfps_link.gfps_raw_passkey_received = false;
            }
        }
        break;
    case GFPS_CALLBACK_TYPE_ACCOUNT_KEY:
        {
//          APP_PRINT_INFO1("app_gfps_cb: account_key %b",TRACE_BINARY(16, p_callback->msg_data.account_key));
            /*for initial pairing or retroactive write account key receive account key from seeker*/
            if (app_gfps_account_key_store(p_callback->msg_data.account_key, gfps_link.edr_bond_bd_addr))
            {
                app_gfps_remote_account_key_add(p_callback->msg_data.account_key, gfps_link.edr_bond_bd_addr);
            }
        }
        break;
#if GFPS_ADDTIONAL_DATA_SUPPORT
    case GFPS_CALLBACK_TYPE_ADDITIONAL_DATA:
        {
//          APP_PRINT_INFO2("app_gfps_cb: GFPS_CALLBACK_TYPE_ADDITIONAL_DATA, data_id 0x%x, data_len %d",
//                          p_callback->msg_data.additional_data.data_id,
//                          p_callback->msg_data.additional_data.data_len);
            //personalized name need to be stored in ftl
#if GFPS_PERSONALIZED_NAME_SUPPORT
            if (p_callback->msg_data.additional_data.data_id == GFPS_DATA_ID_PERSONALIZED_NAME)
            {
                T_APP_GFPS_PERSONALIZED_NAME_RESULT result = app_gfps_personalized_name_store(
                                                                 p_callback->msg_data.additional_data.p_data,
                                                                 p_callback->msg_data.additional_data.data_len);
                if (result == APP_GFPS_PERSONALIZED_NAME_SUCCESS)
                {
                    app_gfps_remote_personalized_name();
                }

                APP_PRINT_INFO2("GFPS_DATA_ID_PERSONALIZED_NAME: personalized name %s, result %d",
                                TRACE_STRING(p_callback->msg_data.additional_data.p_data), result);
            }
#endif
        }
        break;
#endif
    default:
        break;
    }

    if (ret != 0)
    {
        APP_PRINT_ERROR1("app_gfps_cb: error ret %d", ret);
    }
    return app_result;
}
/**
 * @brief non discoverable mode has two submode:show pop up ui or hide pop up ui
 * hide pop up ui: we want to stop showing the subsequent pairing notification since that pairing could be rejected
 *
 */
void app_gfps_enter_nondiscoverable_mode(void)
{
    APP_PRINT_INFO0("app_gfps_enter_nondiscoverable_mode");
#if 0
    if (app_cfg_const.enable_multi_link && app_cfg_const.max_legacy_multilink_devices > 1)
    {
        if (app_find_b2s_link_num() == app_cfg_const.max_legacy_multilink_devices)
        {
            APP_PRINT_INFO0("app_gfps_enter_nondiscoverable_mode: pop up hide ui");
            app_gfps_next_action(GFPS_ACTION_ADV_NOT_DISCOVERABLE_MODE_HIDE_UI);
        }
        else
        {
            app_gfps_next_action(GFPS_ACTION_ADV_NOT_DISCOVERABLE_MODE);
        }
    }
    else
#endif
    {
        app_gfps_next_action(GFPS_ACTION_ADV_NOT_DISCOVERABLE_MODE);
    }
}


/**
 * @brief Include the battery information in the advertisement with hide battery notification (0b0100) for at least 5 seconds
 * when case is closed and both buds are docked.
 * Include the battery information in the advertisement with show battery notification(0b0011) when the case has opened.
 * @param[in] open  if open = true means case open, if open = false means case close.
 */
void app_gfps_handle_case_status(bool open)
{
    APP_PRINT_INFO1("app_gfps_handle_case_status: open %d", open);
    T_BT_DEVICE_MODE radio_mode = app_bt_policy_get_radio_mode();

    if (radio_mode != BT_DEVICE_MODE_DISCOVERABLE_CONNECTABLE &&
        app_cfg_nv.bud_role != REMOTE_SESSION_ROLE_SECONDARY)
    {
        if (open)
        {
            /*show pair popup*/
            app_gfps_next_action(GFPS_ACTION_ADV_NOT_DISCOVERABLE_MODE);
            /*show battery popup*/
            app_gfps_battery_show_ui(true);
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
            /*hid pair popup*/
            app_gfps_next_action(GFPS_ACTION_ADV_NOT_DISCOVERABLE_MODE_HIDE_UI);
            /*hid battery popup*/
            app_gfps_battery_show_ui(false);
#if GFPS_FINDER_SUPPORT
            if (extend_app_cfg_const.gfps_finder_support)
            {
                T_GFPS_FINDER_BEACON_STATE beacon_state = GFPS_FINDER_BEACON_STATE_OFF;
                app_gfps_finder_enter_beacon_state(beacon_state);
            }
#endif
        }

        //app_gfps_enter_nondiscoverable_mode();
    }
}

void app_gfps_handle_b2s_connected(uint8_t *bd_addr)
{
    uint8_t account_key[16] = {0};
    /*(memcmp(bd_addr, gfps_link.edr_bond_bd_addr, 6) == 0): b2s is connected through gfps pair.
    gfps_get_subsequent_pairing_account_key(account_key) == true: is gfps subsequent pairing.
    */
    if ((memcmp(bd_addr, gfps_link.edr_bond_bd_addr, 6) == 0) &&
        (gfps_get_subsequent_pairing_account_key(account_key) == true))
    {
        /*store subsequent pairing account key*/
        if (app_gfps_account_key_store(account_key, gfps_link.edr_bond_bd_addr))
        {
            app_gfps_remote_account_key_add(account_key, gfps_link.edr_bond_bd_addr);
        };
    }
    APP_PRINT_INFO2("app_gfps_handle_b2s_connected:bd_addr %b,gfps_link->edr_bond_bd_addr %b",
                    TRACE_BDADDR(bd_addr), TRACE_BDADDR(gfps_link.edr_bond_bd_addr));
}

void app_gfps_check_state(void)
{
    if (app_db.device_state == APP_DEVICE_STATE_ON)
    {
        T_BT_DEVICE_MODE radio_mode = app_bt_policy_get_radio_mode();
#if GFPS_SASS_SUPPORT
        if (app_sass_policy_support_easy_connection_switch())
        {
            app_gfps_update_adv();
        }
#endif

        APP_PRINT_INFO2("app_gfps_check_state: radio_mode %d, bud_role %d", radio_mode,
                        app_cfg_nv.bud_role);
        if (app_cfg_nv.bud_role != REMOTE_SESSION_ROLE_SECONDARY)
        {
            if (radio_mode == BT_DEVICE_MODE_DISCOVERABLE_CONNECTABLE)
            {
                /*initial pairing*/
                if ((app_cfg_const.enable_multi_link) &&
                    (app_cfg_const.enter_pairing_while_only_one_device_connected) &&
                    (app_bt_policy_get_b2s_connected_num() != 0))
                {
                    /* workaround for fast pair validator */
                    app_gfps_enter_nondiscoverable_mode();
                }
                else
                {
                    app_gfps_next_action(GFPS_ACTION_ADV_DISCOVERABLE_MODE_WITH_MODEL_ID);
                }
            }/*subsequent pairing*/
            else
            {
                app_gfps_enter_nondiscoverable_mode();
            }
        }
    }
}

#if GFPS_SASS_SUPPORT
void app_gfps_update_adv(void)
{
    T_SASS_CONN_STATUS_FIELD cur_conn_status;
    gfps_sass_get_conn_status(&cur_conn_status);
    app_gfps_gen_conn_status(&cur_conn_status);
    gfps_sass_set_conn_status(&cur_conn_status);

    if (gfps_db.gfps_curr_action == GFPS_ACTION_ADV_NOT_DISCOVERABLE_MODE)
    {
        if (gfps_gen_adv_data(NOT_DISCOVERABLE_MODE, gfps_adv_data, &gfps_adv_len, true))
        {
            ble_ext_adv_mgr_set_adv_data(gfps_db.gfps_adv_handle, gfps_adv_len, gfps_adv_data);
        }
    }
    else if (gfps_db.gfps_curr_action == GFPS_ACTION_ADV_NOT_DISCOVERABLE_MODE_HIDE_UI)
    {
        if (gfps_gen_adv_data(NOT_DISCOVERABLE_MODE, gfps_adv_data, &gfps_adv_len, false))
        {
            ble_ext_adv_mgr_set_adv_data(gfps_db.gfps_adv_handle, gfps_adv_len, gfps_adv_data);
        }
    }
}

void app_gfps_check_inuse_account_key(void)
{
    uint8_t active_idx = app_multilink_get_active_idx() < MAX_BR_LINK_NUM ?
                         app_multilink_get_active_idx() : MAX_BR_LINK_NUM;
    T_APP_BR_LINK *p_link = NULL;
    uint8_t inuse_account_key = gfps_get_inuse_account_key_index();
    uint8_t recently_inuse_account_key = gfps_get_recently_inuse_account_key_index();
    uint8_t b2s_link_num = app_bt_policy_get_b2s_connected_num();
    APP_PRINT_TRACE5("app_gfps_check_inuse_account_key: active_idx: %d, multicfg: %d, b2s: %d, "
                     "inuse_account_key: %d, recently_inuse_account_key: %d",
                     active_idx, app_cfg_nv.sass_multilink_cfg, b2s_link_num,
                     inuse_account_key, recently_inuse_account_key);

    if (b2s_link_num != 0)
    {
        if (active_idx != MAX_BR_LINK_NUM)
        {
            p_link = &app_db.br_link[active_idx];
        }
        if (app_cfg_nv.sass_multilink_cfg)
        {
            if (b2s_link_num == 1)
            {
                if ((active_idx != MAX_BR_LINK_NUM) &&
                    (p_link->used))
                {
                    if (inuse_account_key != p_link->gfps_inuse_account_key)
                    {
                        if (p_link->gfps_inuse_account_key != 0xff)//sass device
                        {
                            gfps_set_most_recently_inuse_account_key_index(0xff);
                        }
                        else //non-sass device
                        {
                            if (inuse_account_key != 0xff)
                            {
                                gfps_set_most_recently_inuse_account_key_index(inuse_account_key);
                                app_cfg_nv.sass_recently_used_account_key_index = inuse_account_key;
                            }
                        }
                        gfps_set_inuse_account_key_index(p_link->gfps_inuse_account_key);
                    }
                }
            }
            else if (b2s_link_num == 2)
            {
                uint8_t inactive_idx = MAX_BR_LINK_NUM;
                for (uint8_t i = 0; i < MAX_BR_LINK_NUM; i++)
                {
                    if (app_check_b2s_link_by_id(i) && (i != active_idx))
                    {
                        inactive_idx = i;
                        break;
                    }
                }
                //sass device
                if ((active_idx != MAX_BR_LINK_NUM) && (inactive_idx != MAX_BR_LINK_NUM))
                {
                    if (p_link->gfps_inuse_account_key != 0xff)
                    {
                        gfps_set_most_recently_inuse_account_key_index(0xff);
                        gfps_set_inuse_account_key_index(p_link->gfps_inuse_account_key);
                    }
                    else//inused non sass device
                    {
                        if (app_db.br_link[inactive_idx].gfps_inuse_account_key != 0xff)
                        {
                            gfps_set_most_recently_inuse_account_key_index(app_db.br_link[inactive_idx].gfps_inuse_account_key);
                            app_cfg_nv.sass_recently_used_account_key_index =
                                app_db.br_link[inactive_idx].gfps_inuse_account_key;
                        }
                        else
                        {
                            gfps_set_most_recently_inuse_account_key_index(app_cfg_nv.sass_recently_used_account_key_index);
                        }
                        gfps_set_inuse_account_key_index(0xff);
                    }
                }
            }
        }
        else
        {
            if ((active_idx != MAX_BR_LINK_NUM) &&
                (p_link->used))
            {
                if (inuse_account_key != p_link->gfps_inuse_account_key)
                {
                    gfps_set_most_recently_inuse_account_key_index(inuse_account_key);
                    if (inuse_account_key != 0xff)
                    {
                        app_cfg_nv.sass_recently_used_account_key_index = inuse_account_key;
                    }
                    gfps_set_inuse_account_key_index(p_link->gfps_inuse_account_key);
                }
            }
        }
    }
    else
    {
        if (inuse_account_key != 0xff)
        {
            gfps_set_most_recently_inuse_account_key_index(inuse_account_key);
            app_cfg_nv.sass_recently_used_account_key_index = inuse_account_key;
            gfps_set_inuse_account_key_index(0xff);
        }
    }
    inuse_account_key = gfps_get_inuse_account_key_index();
    recently_inuse_account_key = gfps_get_recently_inuse_account_key_index();
    if ((inuse_account_key != 0xff) && (recently_inuse_account_key != 0xff))
    {
        APP_PRINT_TRACE3("app_gfps_check_inuse_account_key abnormal key setting 0x%,0x%x,0x%x",
                         inuse_account_key, recently_inuse_account_key, p_link->gfps_inuse_account_key);
        gfps_set_most_recently_inuse_account_key_index(0xff);
    }
}

void app_gfps_notify_conn_status(void)
{
    if (extend_app_cfg_const.gfps_support)
    {
        app_gfps_check_inuse_account_key();
        app_gfps_check_state();
        app_gfps_rfc_notify_connection_status();
    }
}
bool app_gfps_gen_conn_status(T_SASS_CONN_STATUS_FIELD *conn_status)
{
    uint8_t length = 3;
    T_SASS_FOCUS_MODE focus_mode = SASS_NOT_IN_FOCUS;
    T_SASS_ON_HEAD_DETECTION on_head = SASS_NOT_ON_HEAD;
    T_SASS_AUTO_RECONN auto_reconnect = SASS_AUTO_RECONN_DISABLE;
    T_SASS_CONN_STATE conn_state = SASS_DISABLE_CONN_SWITCH;
    T_SASS_CONN_AVAILABLE conn_available = SASS_CONN_NOT_FULL;
    uint8_t active_index = 0;
    if (conn_status == NULL)
    {
        return false;
    }
    active_index = app_multilink_get_active_idx();
#if GFPS_SASS_IN_FOCUS_SUPPORT
    if (app_sass_policy_get_focus_mode())
    {
        focus_mode = SASS_IN_FOCUS;
    }
#endif
#if GFPS_SASS_ON_HEAD_DETECT_SUPPORT
    if ((app_db.local_loc == BUD_LOC_IN_EAR) || (app_db.remote_loc == BUD_LOC_IN_EAR))
    {
        on_head = SASS_ON_HEAD;
    }
#endif
    if (app_db.br_link[active_index].connected_by_linkback)
    {
        auto_reconnect = SASS_AUTO_RECONN_ENABLE;
    }
    conn_state = (T_SASS_CONN_STATE) app_sass_policy_get_connection_state();
    conn_available = app_multilink_get_available_connection_num() <= 0 ? SASS_CONN_FULL :
                     SASS_CONN_NOT_FULL;
    conn_status->conn_status_info.auto_reconn = auto_reconnect;
    conn_status->conn_status_info.conn_availability = conn_available;
    conn_status->conn_status_info.conn_state = (uint8_t)conn_state;
    conn_status->conn_status_info.focus_mode = focus_mode;
    conn_status->conn_status_info.on_head_detection = on_head;
    conn_status->conn_dev_bitmap = app_sass_policy_get_conn_bit_map();
    return true;
}
//void app_gfps_le_update_conn_status(void)
//{
//    app_gfps_update_adv();
//    app_gfps_check_state();

//    APP_PRINT_INFO0("app_gfps_le_update_conn_status: done");
//}
#endif
#endif
