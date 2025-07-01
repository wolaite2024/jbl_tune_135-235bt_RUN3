/*
 * Copyright (c) 2018, Realsil Semiconductor Corporation. All rights reserved.
 */

#if F_APP_ERWS_SUPPORT
#include <string.h>
#include "wdg.h"
#include "os_mem.h"
#include "os_timer.h"
#include "os_sync.h"
#include "remote.h"
#include "bt_avrcp.h"
#include "ringtone.h"
#include "trace.h"
#include "gap_timer.h"
#include "remote.h"
#include "sysm.h"
#include "btm.h"
#include "bt_hfp.h"
#include "audio_volume.h"
#include "eq_utils.h"
#include "app_main.h"
#include "app_device.h"
#include "app_cfg.h"
#include "app_relay.h"
#include "app_bond.h"
#include "app_bt_sniffing.h"
#include "app_ble_device.h"
#include "app_roleswap.h"
#include "app_roleswap_control.h"
#include "app_multilink.h"
#include "app_mmi.h"
#include "app_hfp.h"
#include "app_bt_policy_api.h"
#include "app_bt_policy_int.h"
#include "app_audio_policy.h"
#include "app_avrcp.h"
#include "app_sensor.h"
#include "app_ota.h"
#include "app_eq.h"
#include "section.h"
#include "app_key_process.h"
#include "app_bud_loc.h"
#if F_APP_ANC_SUPPORT
#include "app_anc.h"
#include "anc.h"
#include "anc_tuning.h"
#endif
#if F_APP_LISTENING_MODE_SUPPORT
#include "app_listening_mode.h"
#endif
#if GFPS_FEATURE_SUPPORT
#include "app_gfps.h"
#endif
#include "app_report.h"
#if F_APP_IPHONE_ABS_VOL_HANDLE
#include "app_iphone_abs_vol_handle.h"
#include "app_cfg.h"
#endif
#include "eq.h"

#if F_APP_TEAMS_FEATURE_SUPPORT
#include "app_teams_cfu.h"
#endif

#if (F_APP_AVP_INIT_SUPPORT == 1)
#include "app_avp.h"
#endif

#define ROLESWAP_MIC_PROTECT_TIMER_MS       6000
#define ROLESWAP_TERMINATED_TIMER_MS        3500
#define ROLESWAP_PROTECT_TIMER_MS           6000
#define ROLESWAP_RESET_TIMER_MS             1000
#define ROLESWAP_BAT_TRIG_DELAY_MS          6000
#define BUD_SYNC_MS                         50
#define EAR_ROLESWAP_MS                     200
#define SNIFFING_DELAY_MS                   1000
#define ROLESWAP_RETRY_TIMER_MS             300

static uint8_t app_roleswap_timer_queue_id = 0;

#if (F_APP_AVP_INIT_SUPPORT == 1)
void (*app_roleswap_ear_changed_hook)(uint8_t *, uint8_t, bool, uint8_t) = NULL;
void (*app_roleswap_call_changed_hook)(void) = NULL;
#endif


static void app_roleswap_bud_sync(void)
{
    T_APP_BR_LINK *p_link = NULL;

    if ((app_cfg_const.charger_support && (app_cfg_const.box_detect_method == ADAPTOR_DETECT))
#if F_APP_GPIO_ONOFF_SUPPORT
        || (app_cfg_const.box_detect_method == GPIO_DETECT)
#endif
       )
    {
        if ((app_cfg_const.enable_rtk_charging_box == 0) || !app_db.power_on_by_cmd) //need to read directly
        {
            app_db.local_in_case = app_device_is_in_the_box();
        }
    }
    app_db.local_loc = app_sensor_bud_loc_detect();

    app_sensor_bud_loc_sync();

    APP_PRINT_TRACE2("app_roleswap_timeout_cb: local_batt_level %d local_charger_state %d",
                     app_db.local_batt_level, app_db.local_charger_state);

    T_APP_RELAY_MSG_LIST_HANDLE relay_msg_handle = app_relay_async_build();
    app_relay_async_add(relay_msg_handle, APP_MODULE_TYPE_CHARGER, APP_CHARGER_EVENT_BATT_CHANGE);
    app_relay_async_add(relay_msg_handle, APP_MODULE_TYPE_CHARGER, APP_CHARGER_EVENT_BATT_STATE_CHANGE);

    if (app_cfg_const.enable_rtk_charging_box && (app_cfg_nv.bud_role != REMOTE_SESSION_ROLE_SECONDARY))
    {
        app_relay_async_add(relay_msg_handle, APP_MODULE_TYPE_CHARGER, APP_CHARGER_EVENT_BATT_CASE_LEVEL);
    }

#if (F_APP_AVP_INIT_SUPPORT == 1)
//rsv for avp
#else
    if (app_cfg_const.sensor_support && (app_cfg_const.enable_rtk_charging_box == 0) &&
        (app_cfg_nv.bud_role !=
         REMOTE_SESSION_ROLE_SECONDARY)) //if enable rtk,light sensor is enable from power on
    {
        if (app_cfg_nv.light_sensor_enable != 0)
        {
            app_relay_async_add(relay_msg_handle, APP_MODULE_TYPE_BUD_LOC, APP_BUD_LOC_EVENT_SENSOR_LD_CONFIG);
        }
    }
#endif

    app_relay_async_send(relay_msg_handle);

    if (app_cfg_const.enable_rtk_charging_box)
    {
        app_bud_loc_evt_handle(APP_BUD_LOC_EVENT_CASE_OPEN_CASE, 0, 0);
    }

    if (app_cfg_nv.bud_role == REMOTE_SESSION_ROLE_PRIMARY)
    {
        if (app_find_b2s_link_num() > 0)
        {
            if (app_cfg_const.enable_low_bat_role_swap)
            {
                app_roleswap_req_battery_level();
            }
        }
    }

    if (app_cfg_const.enable_multi_link && app_cfg_nv.bud_role == REMOTE_SESSION_ROLE_PRIMARY)
    {
        app_relay_async_single(APP_MODULE_TYPE_MULTI_LINK, APP_REMOTE_MSG_ACTIVE_BD_ADDR_SYNC);
    }

#if F_APP_IPHONE_ABS_VOL_HANDLE
    uint8_t active_idx;

    active_idx = app_get_active_a2dp_idx();
    p_link = &app_db.br_link[active_idx];

    if (p_link != NULL)
    {
        if ((p_link->remote_device_vendor_id == APP_REMOTE_DEVICE_IOS) &&
            (app_iphone_abs_vol_check_a2dp_total_gain_num_16() == true))
        {
            uint8_t pair_idx_mapping;

            app_bond_get_pair_idx_mapping(app_db.br_link[active_idx].bd_addr, &pair_idx_mapping);

            app_iphone_abs_vol_sync_abs_vol(app_db.br_link[active_idx].bd_addr,
                                            app_cfg_nv.abs_vol[pair_idx_mapping]);
        }
    }
#endif

    //abs vol state sync
    for (uint8_t i = 0; i < MAX_BR_LINK_NUM; i++)
    {
        if (app_check_b2s_link_by_id(i))
        {
            p_link = &app_db.br_link[i];
            if (p_link != NULL)
            {
                app_avrcp_sync_abs_vol_state(p_link->bd_addr, p_link->abs_vol_state);
            }
        }
    }
}

static void app_roleswap_sub_chargerbox_audio(uint8_t event, bool from_remote, uint8_t para)
{
}

static void app_roleswap_dm_cback(T_SYS_EVENT event_type, void *event_buf, uint16_t buf_len)
{
    bool handle = true;

    switch (event_type)
    {
    case SYS_EVENT_POWER_ON:
        {
            app_db.batt_roleswap = 0;
        }
        break;

    case SYS_EVENT_POWER_OFF:
        {
            app_db.batt_roleswap = 0;
        }
        break;

    default:
        handle = false;
        break;
    }

    if (handle == true)
    {
        APP_PRINT_INFO1("app_roleswap_dm_cback: event_type 0x%04x", event_type);
    }
}

static void app_roleswap_bt_cback(T_BT_EVENT event_type, void *event_buf, uint16_t buf_len)
{
    T_BT_EVENT_PARAM *param = event_buf;
    bool handle = true;

    switch (event_type)
    {
    case BT_EVENT_REMOTE_ROLESWAP_STATUS:
        {
            T_BT_ROLESWAP_STATUS status;

            status = param->remote_roleswap_status.status;

            if (status == BT_ROLESWAP_STATUS_ACL_INFO)
            {
                uint8_t bd_addr[6];
                T_APP_BR_LINK *p_link;

                memcpy(bd_addr, param->remote_roleswap_status.u.acl.bd_addr, 6);
                p_link = app_find_br_link(bd_addr);
                if (p_link == NULL)
                {
                    p_link = app_alloc_br_link(bd_addr);
                }

                if (p_link == NULL)
                {
                    APP_PRINT_TRACE1("app_roleswap_bt_cback: CB_EVENT_ROLESWAP_STATUS: ROLESWAP_STATUS_ACL_INFO bd_addr %s failed",
                                     TRACE_BDADDR(bd_addr));
                }
                else
                {
                    p_link->acl_link_role = param->remote_roleswap_status.u.acl.role;
                    p_link->auth_flag = param->remote_roleswap_status.u.acl.authenticated;
                }

                if (!memcmp(bd_addr, app_cfg_nv.bud_peer_addr, 6) && (p_link != NULL))
                {
                    if (p_link->acl_link_role == BT_LINK_ROLE_MASTER)
                    {
                        app_roleswap_ctrl_check(APP_ROLESWAP_CTRL_EVENT_ACL_ROLE);
                    }
                }
            }
            else if (status == BT_ROLESWAP_STATUS_SPP_INFO)
            {
                uint8_t bd_addr[6];
                T_APP_BR_LINK *p_link;

                memcpy(bd_addr, param->remote_roleswap_status.u.spp.bd_addr, 6);
                p_link = app_find_br_link(bd_addr);
                if (p_link)
                {
                    p_link->connected_profile |= SPP_PROFILE_MASK;

                    p_link->rfc_credit = param->remote_roleswap_status.u.spp.remote_credit;
                    p_link->rfc_frame_size = param->remote_roleswap_status.u.spp.frame_size;
                }
            }
            else if (status == BT_ROLESWAP_STATUS_A2DP_INFO)
            {
                uint8_t bd_addr[6];
                T_APP_BR_LINK *p_link;

                memcpy(bd_addr, param->remote_roleswap_status.u.a2dp.bd_addr, 6);
                p_link = app_find_br_link(bd_addr);
                if (p_link)
                {
                    p_link->connected_profile |= A2DP_PROFILE_MASK;
                    p_link->streaming_fg = param->remote_roleswap_status.u.a2dp.streaming_fg;
                    app_judge_active_a2dp_idx_and_qos(p_link->id, JUDGE_EVENT_A2DP_CONNECTED);
                }
            }
            else if (status == BT_ROLESWAP_STATUS_AVRCP_INFO)
            {
                uint8_t bd_addr[6];
                T_APP_BR_LINK *p_link;

                memcpy(bd_addr, param->remote_roleswap_status.u.avrcp.bd_addr, 6);
                p_link = app_find_br_link(bd_addr);
                if (p_link)
                {
                    p_link->connected_profile |= AVRCP_PROFILE_MASK;
                    p_link->avrcp_play_status = param->remote_roleswap_status.u.avrcp.play_status;
                    if (app_get_active_a2dp_idx() == p_link->id)
                    {
                        app_audio_set_avrcp_status(p_link->avrcp_play_status);
                        app_avrcp_sync_status();
                    }
                }
            }
            else if (status == BT_ROLESWAP_STATUS_HFP_INFO)
            {
                uint8_t bd_addr[6];
                T_APP_BR_LINK *p_link;

                memcpy(bd_addr, param->remote_roleswap_status.u.hfp.bd_addr, 6);
                p_link = app_find_br_link(bd_addr);
                if (p_link)
                {
                    if (param->remote_roleswap_status.u.hfp.is_hfp)
                    {
                        p_link->connected_profile |= HFP_PROFILE_MASK;
                    }
                    else
                    {
                        p_link->connected_profile |= HSP_PROFILE_MASK;
                    }
                    p_link->call_status = param->remote_roleswap_status.u.hfp.call_status;
                    p_link->service_status = param->remote_roleswap_status.u.hfp.service_status;

                    app_hfp_set_active_hf_index(bd_addr);
                    p_link->app_hf_state = APP_HF_STATE_CONNECTED;
                }
            }
            else if (status == BT_ROLESWAP_STATUS_PBAP_INFO)
            {
                uint8_t bd_addr[6];
                T_APP_BR_LINK *p_link;

                memcpy(bd_addr, param->remote_roleswap_status.u.pbap.bd_addr, 6);
                p_link = app_find_br_link(bd_addr);
                if (p_link)
                {
                    p_link->connected_profile |= PBAP_PROFILE_MASK;
                }
            }
            else if (status == BT_ROLESWAP_STATUS_IAP_INFO)
            {
                uint8_t bd_addr[6];
                T_APP_BR_LINK *p_link;

                memcpy(bd_addr, param->remote_roleswap_status.u.iap.bd_addr, 6);
                p_link = app_find_br_link(bd_addr);
                if (p_link)
                {
                    p_link->connected_profile |= IAP_PROFILE_MASK;

                    p_link->iap.credit = param->remote_roleswap_status.u.iap.remote_credit;
                    p_link->iap.frame_size = param->remote_roleswap_status.u.iap.frame_size;
                }
            }
            else if (status == BT_ROLESWAP_STATUS_AVP_INFO)
            {
                uint8_t bd_addr[6];
                T_APP_BR_LINK *p_link;

                memcpy(bd_addr, param->remote_roleswap_status.u.avp.bd_addr, 6);
                p_link = app_find_br_link(bd_addr);
                if (p_link)
                {
                    if (param->remote_roleswap_status.u.avp.control_chann_connected)
                    {
                        p_link->connected_profile |= AVP_PROFILE_MASK;
                    }

                    if (param->remote_roleswap_status.u.avp.audio_chann_connected)
                    {
                        p_link->connected_profile |= GATT_PROFILE_MASK;
                    }
                }
            }
            else if (status == BT_ROLESWAP_STATUS_SCO_INFO)
            {
                uint8_t bd_addr[6];
                T_APP_BR_LINK *p_link;

                memcpy(bd_addr, param->remote_roleswap_status.u.sco.bd_addr, 6);
                p_link = app_find_br_link(bd_addr);
                if (p_link)
                {
                    p_link->sco_handle = param->remote_roleswap_status.u.sco.handle;
                }
            }
            else if ((status == BT_ROLESWAP_STATUS_SUCCESS) || (status == BT_ROLESWAP_STATUS_FAIL))
            {
                if (app_db.pending_mmi_action_in_roleswap)
                {
                    T_MMI_ACTION action = (T_MMI_ACTION)(app_db.pending_mmi_action_in_roleswap);
                    if (app_cfg_nv.bud_role == REMOTE_SESSION_ROLE_PRIMARY)
                    {
                        app_mmi_handle_action(action);
                    }
                    else
                    {
                        app_relay_async_single(APP_MODULE_TYPE_MMI, action);
                    }
                    app_db.pending_mmi_action_in_roleswap = 0;
                }

                if (status == BT_ROLESWAP_STATUS_SUCCESS)
                {
                    uint8_t app_idx = MAX_BR_LINK_NUM;

                    for (int i = 0; i < MAX_BR_LINK_NUM; i++)
                    {
                        if (app_check_b2s_link_by_id(i))
                        {
                            app_idx = i;
                            break;
                        }
                    }

                    if (app_cfg_nv.bud_role != REMOTE_SESSION_ROLE_SECONDARY)
                    {
                        if (app_idx < MAX_BR_LINK_NUM)
                        {
                            //Battery level update to phone when roleswap success
                            if (app_db.br_link[app_idx].connected_profile & HFP_PROFILE_MASK)
                            {
                                {
                                    app_hfp_batt_level_report(app_db.br_link[app_idx].bd_addr);
                                }
                            }
                        }

                        app_report_rws_bud_info();
                    }
                }
            }
        }
        break;

    case BT_EVENT_REMOTE_CONN_CMPL:
        {
            T_APP_BR_LINK *p_link;

            p_link = app_find_br_link(param->remote_conn_cmpl.bd_addr);
            if (p_link != NULL)
            {
                if (app_db.device_state != APP_DEVICE_STATE_OFF)
                {
                    app_roleswap_bud_sync();
                }
            }

            app_roleswap_ctrl_check(APP_ROLESWAP_CTRL_EVENT_B2B_CONNECTED);
        }
        break;

    case BT_EVENT_REMOTE_DISCONN_CMPL:
        {
        }
        break;

    case BT_EVENT_HFP_CALL_STATUS:
        {
        }
        break;

    case BT_EVENT_SCO_CONN_CMPL:
        {
            app_roleswap_ctrl_check(APP_ROLESWAP_CTRL_EVENT_SCO_CONN_CMPL);
        }
        break;

    case BT_EVENT_DEVICE_MODE_RSP:
        {
        }
        break;

    default:
        handle = false;
        break;
    }

    if (handle == true)
    {
        APP_PRINT_INFO1("app_roleswap_bt_cback: event_type 0x%04x", event_type);
    }
}

static void app_roleswap_timeout_cb(uint8_t timer_id, uint16_t timer_chann)
{
    APP_PRINT_TRACE2("app_roleswap_timeout_cb: timer_id 0x%02x, timer_chann %d", timer_id,
                     timer_chann);

    switch (timer_id)
    {

    default:
        break;
    }
}

void app_roleswap_state_machine(uint8_t event, bool from_remote, uint8_t para)
{
    uint8_t group = event & 0xf0;
    APP_PRINT_TRACE4("app_roleswap_state_machine: event 0x%x, from_remote 0x%x, para 0x%x, group 0x%x",
                     event, from_remote, para, group);

    switch (group)
    {
    case ROLESWAP_EVENT_OTHER_GROUP:
    case ROLESWAP_EVENT_OTHER_SYNC_GROUP:
        {
        }
        break;

    case ROLESWAP_EVENT_CHARGERBOX_GROUP:
        {
            app_roleswap_sub_chargerbox_audio(event, from_remote, para);
        }
        break;

    default:
        {
            APP_PRINT_WARN1("app_roleswap_state_machine: Invalid event group 0x%x", group);
        }
        break;
    }

    if (!from_remote)
    {
        app_roleswap_event_info_remote(event, &para, 1);
    }
}

void app_roleswap_event_info_remote(uint8_t event, uint8_t *param,
                                    uint8_t para_len)
{
    uint8_t len = para_len + 1;
    uint8_t params[33] = {0};

    if (app_db.remote_loc == BUD_LOC_UNKNOWN)
    {
        return;
    }

    if (para_len > 32)
    {
        APP_PRINT_WARN0("app_roleswap_event_info_remote: len exceed limit ");
        return;// invalid para_len
    }

    params[0] = event;
    if (param != NULL)
    {
        APP_PRINT_TRACE3("app_roleswap_event_info_remote: event 0x%02x param %d para_len %d", event,
                         param[0], para_len);
        for (int i = 0; i < para_len; i++)
        {
            params[i + 1] = param[i];
        }
    }

    remote_async_msg_relay(app_db.relay_handle, APP_REMOTE_MSG_ROLESWAP_STATE_SYNC, params, len, false);
}

void app_roleswap_req_battery_level(void)
{
    app_relay_async_single(APP_MODULE_TYPE_CHARGER, APP_CHARGER_EVENT_BATT_REQ_LEVEL);
}

void app_roleswap_sync_poweroff(void)
{
    uint8_t action = MMI_DEV_POWER_OFF;

    if (app_cfg_nv.bud_role == REMOTE_SESSION_ROLE_PRIMARY)
    {
        app_relay_sync_single(APP_MODULE_TYPE_MMI, action, REMOTE_TIMER_HIGH_PRECISION,
                              0, false);
    }
    else
    {
        app_relay_sync_single(APP_MODULE_TYPE_MMI, action, REMOTE_TIMER_HIGH_PRECISION,
                              0, true);
    }
}

/* handle roleswap then power off if it needs for unsync power off*/
void app_roleswap_poweroff_handle(bool is_bat_0_to_power_off)
{

    bool disallow_sync_power_off = false;

    APP_PRINT_TRACE1("app_roleswap_poweroff_handle: is_batt %d", is_bat_0_to_power_off);

    if (app_db.remote_session_state == REMOTE_SESSION_STATE_DISCONNECTED)
    {
        app_mmi_handle_action(MMI_DEV_POWER_OFF);

        return;
    }

    if (is_bat_0_to_power_off)
    {
        if (app_cfg_const.enable_disallow_sync_power_off_under_voltage_0_percent)
        {
            disallow_sync_power_off = true;
        }
    }
    else
    {
        if (app_cfg_const.rws_disallow_sync_power_off)
        {
            disallow_sync_power_off = true;
        }
    }

    if (disallow_sync_power_off)
    {
        if (app_roleswap_ctrl_check(APP_ROLESWAP_CTRL_EVENT_SINGLE_BUD_TO_POWER_OFF))
        {
            /* power off will be triggered after role swap */
        }
        else
        {
            app_mmi_handle_action(MMI_DEV_POWER_OFF);
        }
    }
    else
    {
        app_roleswap_sync_poweroff();
    }
}

bool app_roleswap_call_transfer_check(uint8_t event)
{
    bool need_call_transfer = false;
    T_BT_HFP_CALL_STATUS call_status = app_hfp_get_call_status();
    bool ret = false;
    bool sco_connected = app_hfp_sco_is_connected();

    if (app_cfg_nv.bud_role != REMOTE_SESSION_ROLE_SECONDARY
        && (call_status == BT_HFP_CALL_ACTIVE || call_status == BT_HFP_VOICE_ACTIVATION_ONGOING))
    {
        if (app_cfg_const.enable_rtk_charging_box == true && LIGHT_SENSOR_ENABLED)
        {
            if (sco_connected)
            {
                if (event == APP_BUD_LOC_EVENT_SENSOR_OUT_EAR &&
                    app_cfg_const.disallow_auto_tansfer_to_phone_when_out_of_ear == false)
                {
                    if (app_db.remote_session_state == REMOTE_SESSION_STATE_CONNECTED)
                    {
                        if (app_db.local_loc != BUD_LOC_IN_EAR && app_db.remote_loc != BUD_LOC_IN_EAR)
                        {
                            need_call_transfer = true;
                        }
                    }
                    else
                    {
                        need_call_transfer = true;
                    }
                }
                else if (event == APP_BUD_LOC_EVENT_CASE_IN_CASE)
                {
                    if (app_db.remote_session_state == REMOTE_SESSION_STATE_CONNECTED)
                    {
                        if (app_db.local_loc == BUD_LOC_IN_CASE && app_db.remote_loc == BUD_LOC_IN_CASE)
                        {
                            need_call_transfer = true;
                        }
                    }
                    else
                    {
                        need_call_transfer = true;
                    }
                }
            }
            else
            {
                if (event == APP_BUD_LOC_EVENT_SENSOR_IN_EAR)
                {
                    need_call_transfer = true;
                }
            }
        }
        else if (app_cfg_const.enable_rtk_charging_box == true && app_cfg_const.sensor_support == false)
        {
            if (sco_connected)
            {
                if (event == APP_BUD_LOC_EVENT_CASE_IN_CASE)
                {
                    if (app_db.remote_session_state == REMOTE_SESSION_STATE_CONNECTED)
                    {
                        if (app_db.local_loc == BUD_LOC_IN_CASE && app_db.remote_loc == BUD_LOC_IN_CASE)
                        {
                            need_call_transfer = true;
                        }
                    }
                    else
                    {
                        need_call_transfer = true;
                    }
                }
            }
            else
            {
                if (event == APP_BUD_LOC_EVENT_CASE_OUT_CASE)
                {
                    need_call_transfer = true;
                }
            }
        }
        else if (app_cfg_const.enable_rtk_charging_box == false && LIGHT_SENSOR_ENABLED)
        {
            if (sco_connected)
            {
                if (event == APP_BUD_LOC_EVENT_SENSOR_OUT_EAR &&
                    app_cfg_const.disallow_auto_tansfer_to_phone_when_out_of_ear == false)
                {
                    if (app_db.remote_session_state == REMOTE_SESSION_STATE_CONNECTED)
                    {
                        if (app_db.local_loc != BUD_LOC_IN_EAR && app_db.remote_loc != BUD_LOC_IN_EAR)
                        {
                            need_call_transfer = true;
                        }
                    }
                    else
                    {
                        need_call_transfer = true;
                    }
                }
            }
            else
            {
                if (event == APP_BUD_LOC_EVENT_SENSOR_IN_EAR)
                {
                    need_call_transfer = true;
                }
            }
        }

        if (need_call_transfer)
        {
            if (app_roleswap_ctrl_get_status() == APP_ROLESWAP_STATUS_IDLE)
            {
                ret = true;
            }
        }

        APP_PRINT_INFO4("app_roleswap_call_transfer_check: need_call_transfer %d ret %d sco %d call_status %d",
                        need_call_transfer, ret, sco_connected, call_status);
    }

    return ret;
}

uint16_t app_roleswap_relay_cback(uint8_t *buf, uint8_t msg_type, bool total)
{
    APP_PRINT_INFO1("app_roleswap_relay_cback: msg_type %d", msg_type);

    // uint16_t payload_len = 0;
    // uint8_t *msg_ptr = NULL;
    // bool skip = true;

    return app_relay_msg_pack(buf, msg_type, APP_MODULE_TYPE_ROLESWAP, 0, NULL, true, total);
}

static void app_roleswap_parse_cback(uint8_t msg_type, uint8_t *buf, uint16_t len,
                                     T_REMOTE_RELAY_STATUS status)
{
    if (status == REMOTE_RELAY_STATUS_ASYNC_RCVD)
    {
        uint8_t *p_info = buf;
        uint8_t event;
        uint8_t para;

        event = msg_type;
        if (buf != NULL)
        {
            para = p_info[0];
        }
        else
        {
            para = 0;
        }

        app_roleswap_state_machine(event, 1, para);
    }
}

void app_roleswap_init(void)
{
    app_db.local_batt_level = 50;
    app_db.remote_batt_level = 50;
    app_db.batt_roleswap = 0;

    sys_mgr_cback_register(app_roleswap_dm_cback);
    bt_mgr_cback_register(app_roleswap_bt_cback);
    gap_reg_timer_cb(app_roleswap_timeout_cb, &app_roleswap_timer_queue_id);
    app_relay_cback_register(app_roleswap_relay_cback, app_roleswap_parse_cback,
                             APP_MODULE_TYPE_ROLESWAP, ROLESWAP_EVENT_MAX);
}

#endif
