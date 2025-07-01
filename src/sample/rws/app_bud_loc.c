
/*
 * Copyright (c) 2018, Realsil Semiconductor Corporation. All rights reserved.
 */
#if F_APP_ERWS_SUPPORT
#include "trace.h"
#include "bt_avrcp.h"
#include "remote.h"
#include "gap_timer.h"
#include "app_bud_loc.h"
#include "app_roleswap.h"
#include "app_main.h"
#include "app_cfg.h"
#include "app_relay.h"
#include "app_listening_mode.h"
#include "app_adp.h"
#include "app_hfp.h"
#include "app_sensor.h"
#include "app_audio_policy.h"
#include "app_mmi.h"
#include "app_roleswap_control.h"

#if F_APP_RTK_FAST_PAIR_ADV_FEATURE_SUPPORT
#include "app_rtk_fast_pair_adv.h"
#endif

#if GFPS_FEATURE_SUPPORT
#include "app_gfps_battery.h"
#endif

#if F_APP_IO_OUTPUT_SUPPORT
#include "app_io_output.h"
#endif

void *timer_handle_detect_pause_by_out_ear = NULL;
static void *timer_handle_in_ear_restore_a2dp = NULL;
static uint8_t app_bud_loc_timer_queue_id = 0;

#if (F_APP_SENSOR_SUPPORT == 1)
bool ld_sensor_cause_action = false;
#endif

static void app_bud_loc_timeout_cb(uint8_t timer_id, uint16_t timer_chann)
{
    APP_PRINT_TRACE2("app_bud_loc_timeout_cb: timer_id 0x%02x, timer_chann %d", timer_id,
                     timer_chann);

    switch (timer_id)
    {
    case APP_IO_TIMER_DETECT_AVRCP_PAUSE_BY_OUT_EAR:
        {
            if (timer_handle_detect_pause_by_out_ear != NULL)
            {
                app_db.detect_suspend_by_out_ear = false;
                app_bud_loc_stop_pause_by_out_ear_timer();
            }
        }
        break;

    case APP_IO_TIMER_IN_EAR_RESTORE_A2DP:
        {
            if (timer_handle_in_ear_restore_a2dp != NULL)
            {
                app_db.detect_suspend_by_out_ear = false;
                app_bud_loc_update_in_ear_recover_a2dp(false);
            }
        }
        break;

    default:
        break;
    }
}

void app_bud_loc_update_in_ear_recover_a2dp(bool flag)
{
    if (app_cfg_const.restore_a2dp_when_bud_in_ear_in_15s)
    {
        if (flag)
        {
            gap_start_timer(&timer_handle_in_ear_restore_a2dp,
                            "in_ear_restore_a2dp",
                            app_bud_loc_timer_queue_id,
                            APP_IO_TIMER_IN_EAR_RESTORE_A2DP,
                            0,
                            15000);
        }
        else
        {
            gap_stop_timer(&timer_handle_in_ear_restore_a2dp);
        }
    }

    app_db.in_ear_recover_a2dp = flag;
    if ((app_cfg_nv.bud_role == REMOTE_SESSION_ROLE_PRIMARY) &&
        (app_db.remote_session_state == REMOTE_SESSION_STATE_CONNECTED))
    {
        app_relay_async_single(APP_MODULE_TYPE_BUD_LOC, APP_BUD_LOC_EVENT_SENSOR_OUT_EAR_A2DP_PLAYING);
    }
}

void app_bud_loc_update_detect_suspend_by_out_ear(bool flag)
{
    /* start a timer to check if get the a2dp suspend event */
    gap_start_timer(&timer_handle_detect_pause_by_out_ear,
                    "detect_avrcp_pause_by_out_ear", app_bud_loc_timer_queue_id,
                    APP_IO_TIMER_DETECT_AVRCP_PAUSE_BY_OUT_EAR, 0, 2000);

    app_db.detect_suspend_by_out_ear = flag;
}

void app_bud_loc_stop_pause_by_out_ear_timer(void)
{
    gap_stop_timer(&timer_handle_detect_pause_by_out_ear);
}

static uint8_t app_bud_loc_out_ear_a2dp_action(uint8_t play_status, uint8_t cause_action)
{
    uint8_t action = APP_AUDIO_RESULT_NOTHING;

    if (play_status == BT_AVRCP_PLAY_STATUS_PLAYING)
    {
        action = APP_AUDIO_RESULT_PAUSE_AND_MUTE;
    }
    else
    {
        action = APP_AUDIO_RESULT_MUTE;
    }

    if (cause_action == 0)
    {
        action = APP_AUDIO_RESULT_NOTHING;
    }

    return action;
}

static uint8_t app_bud_loc_in_ear_a2dp_action(uint8_t play_status, bool from_remote,
                                              uint8_t cause_action)
{
    uint8_t action = APP_AUDIO_RESULT_NOTHING;

    if ((app_cfg_const.in_ear_auto_playing || app_db.in_ear_recover_a2dp ||
         /* not receive a2dp suspend yet but in ear again;
         *  trigger avrcp play immediately to prevent in ear not play immediately
         *  (but this may have some side effect for video app which not suppor avrcp)
         */
         timer_handle_detect_pause_by_out_ear) && ((play_status == BT_AVRCP_PLAY_STATUS_PAUSED)  ||
                                                   (play_status == BT_AVRCP_PLAY_STATUS_STOPPED)) && (cause_action))
    {
        if (!app_cfg_const.in_ear_auto_playing)
        {
            app_bud_loc_stop_pause_by_out_ear_timer();
            app_db.detect_suspend_by_out_ear = false;
            app_bud_loc_update_in_ear_recover_a2dp(false);
        }

        action = APP_AUDIO_RESULT_RESUME_AND_UNMUTE;
    }
    else
    {
        action = APP_AUDIO_RESULT_UNMUTE;
    }

    if (cause_action == 0)
    {
        action = APP_AUDIO_RESULT_NOTHING;
    }

    return action;
}

static uint8_t app_bud_loc_out_case_a2dp_action(bool from_remote)
{
    uint8_t action = APP_AUDIO_RESULT_NOTHING;

    return action;
}

static uint8_t app_bud_loc_in_case_a2dp_action(uint8_t play_status)
{
    uint8_t action = APP_AUDIO_RESULT_NOTHING;

    if ((app_db.local_loc_pre == BUD_LOC_IN_EAR) && (app_db.local_loc == BUD_LOC_IN_CASE))
    {
        if (app_cfg_nv.light_sensor_enable)
        {
            if (play_status == BT_AVRCP_PLAY_STATUS_PLAYING)
            {
                action = APP_AUDIO_RESULT_PAUSE_AND_MUTE;
            }
            else
            {
                action = APP_AUDIO_RESULT_MUTE;
            }
        }
    }

    return action;
}

#if (F_APP_SENSOR_SUPPORT == 1)
void app_bud_loc_cause_action_flag_set(bool sensor_cause_action)
{
    ld_sensor_cause_action = sensor_cause_action;
}

bool app_bud_loc_cause_action_flag_get(void)
{
    return ld_sensor_cause_action;
}
#endif

void app_bud_loc_changed_action(uint8_t *action, uint8_t event, bool from_remote, uint8_t para)
{
    uint8_t play_status = app_db.avrcp_play_status;

    if (app_cfg_nv.bud_role != REMOTE_SESSION_ROLE_SECONDARY)
    {
        if (app_audio_get_bud_stream_state() == BUD_STREAM_STATE_VOICE)
        {
            if (app_roleswap_call_transfer_check(event))
            {
                *action = APP_AUDIO_RESULT_VOICE_CALL_TRANSFER;
            }
        }
        else if ((app_audio_get_bud_stream_state() == BUD_STREAM_STATE_IDLE) ||
                 (app_audio_get_bud_stream_state() == BUD_STREAM_STATE_AUDIO))
        {
            if (event == APP_BUD_LOC_EVENT_CASE_IN_CASE)
            {
                *action = app_bud_loc_in_case_a2dp_action(play_status);
            }
            else if (event == APP_BUD_LOC_EVENT_CASE_OUT_CASE)
            {
                *action = app_bud_loc_out_case_a2dp_action(from_remote);
            }
            else if (event == APP_BUD_LOC_EVENT_SENSOR_IN_EAR)
            {
                *action = app_bud_loc_in_ear_a2dp_action(play_status, from_remote, para);
            }
            else if (event == APP_BUD_LOC_EVENT_SENSOR_OUT_EAR)
            {
                *action = app_bud_loc_out_ear_a2dp_action(play_status, para);
            }
        }
    }
}

void app_bud_loc_changed(uint8_t *action, uint8_t event, bool from_remote, uint8_t para)
{
    bool local_out_case = false;
    bool local_from_case_to_ear = false;

    app_db.last_bud_loc_event = event;

    app_bud_loc_changed_action(action, event, from_remote, para);

    if ((app_db.local_loc_pre == BUD_LOC_IN_CASE) && (app_db.local_loc == BUD_LOC_IN_EAR))
    {
        local_from_case_to_ear = true;
    }

    if ((app_db.local_loc_pre == BUD_LOC_IN_CASE) && (app_db.local_loc == BUD_LOC_IN_AIR))
    {
        local_out_case = true;
    }

    if ((app_db.local_loc_pre != BUD_LOC_IN_CASE) && (app_db.local_loc == BUD_LOC_IN_CASE))
    {
#if F_APP_LISTENING_MODE_SUPPORT
#if (F_APP_AVP_INIT_SUPPORT == 1)
//rsv for avp
#else
        app_listening_rtk_in_case();
#endif
#endif
    }

    if (local_out_case || local_from_case_to_ear)
    {
#if F_APP_LISTENING_MODE_SUPPORT
#if (F_APP_AVP_INIT_SUPPORT == 1)
//rsv for avp
#else
        app_listening_rtk_out_case();
#endif
#endif
    }

    APP_PRINT_INFO6("app_bud_loc_changed: event %02x loc (%d->%d) remote_loc (%d->%d) local_out_case %d",
                    event, app_db.local_loc_pre, app_db.local_loc,
                    app_db.remote_loc_pre, app_db.remote_loc, local_out_case);
}

#if F_APP_IO_OUTPUT_SUPPORT
static void app_bud_loc_handle_ext_mcu_pin(uint8_t event)
{
    if (event == APP_BUD_LOC_EVENT_CASE_IN_CASE)
    {
        if (app_db.device_state == APP_DEVICE_STATE_ON)
        {
            app_io_output_ctrl_ext_mcu_pin(ENABLE);
        }
    }
    else if (event == APP_BUD_LOC_EVENT_CASE_OUT_CASE)
    {
        app_io_output_ctrl_ext_mcu_pin(DISABLE);
    }
}
#endif

static void app_bud_loc_roleswap_handle(bool from_remote, uint8_t event)
{
    if (from_remote)
    {
        if (event == APP_BUD_LOC_EVENT_CASE_IN_CASE)
        {
            app_roleswap_ctrl_check(APP_ROLESWAP_CTRL_EVENT_REMOTE_IN_CASE);
        }
        else if (event == APP_BUD_LOC_EVENT_CASE_OUT_CASE)
        {
            app_roleswap_ctrl_check(APP_ROLESWAP_CTRL_EVENT_REMOTE_OUT_CASE);
        }
        else if (event == APP_BUD_LOC_EVENT_SENSOR_IN_EAR)
        {
            app_roleswap_ctrl_check(APP_ROLESWAP_CTRL_EVENT_REMOTE_IN_EAR);
        }
        else if (event == APP_BUD_LOC_EVENT_SENSOR_OUT_EAR)
        {
            app_roleswap_ctrl_check(APP_ROLESWAP_CTRL_EVENT_REMOTE_OUT_EAR);
        }
    }
    else
    {
        if (event == APP_BUD_LOC_EVENT_CASE_IN_CASE)
        {
            if (app_cfg_nv.is_dut_test_mode)
            {
                app_mmi_handle_action(MMI_DEV_POWER_OFF);
            }
            else
            {
                bool roleswap_is_triggered = app_roleswap_ctrl_check(APP_ROLESWAP_CTRL_EVENT_LOCAL_IN_CASE);

                /* if dut mode in box; it must power off */
                if (app_cfg_const.enable_inbox_power_off && roleswap_is_triggered == false)
                {
                    app_mmi_handle_action(MMI_DEV_POWER_OFF);
                }
            }
        }
        else if (event == APP_BUD_LOC_EVENT_CASE_OUT_CASE)
        {
            app_roleswap_ctrl_check(APP_ROLESWAP_CTRL_EVENT_LOCAL_OUT_CASE);
        }
        else if (event == APP_BUD_LOC_EVENT_SENSOR_IN_EAR)
        {
            app_roleswap_ctrl_check(APP_ROLESWAP_CTRL_EVENT_LOCAL_IN_EAR);
        }
        else if (event == APP_BUD_LOC_EVENT_SENSOR_OUT_EAR)
        {
            app_roleswap_ctrl_check(APP_ROLESWAP_CTRL_EVENT_LOCAL_OUT_EAR);
        }
    }
}

static void app_bud_loc_handle_spk_mute_unmute(uint8_t event)
{
    if (event == APP_BUD_LOC_EVENT_CASE_OUT_CASE)
    {
        app_audio_spk_mute_unmute(false);
    }
    else if (event == APP_BUD_LOC_EVENT_CASE_IN_CASE)
    {
        app_audio_spk_mute_unmute(true);
    }
}

void app_bud_loc_evt_handle(uint8_t event, bool from_remote, uint8_t para)
{
    uint8_t action = APP_AUDIO_RESULT_NOTHING;

    if ((event == APP_BUD_LOC_EVENT_CASE_IN_CASE || event == APP_BUD_LOC_EVENT_CASE_OUT_CASE) &&
        from_remote == false)
    {
        app_bud_loc_handle_spk_mute_unmute(event);
    }

    switch (event)
    {
    case APP_BUD_LOC_EVENT_CASE_IN_CASE:
        {
            if (from_remote)
            {
                app_db.remote_loc = BUD_LOC_IN_CASE;
                app_db.remote_loc_received = true;
            }
            else
            {
                app_db.local_loc = BUD_LOC_IN_CASE;
            }

            if (app_cfg_const.enable_rtk_charging_box)
            {
                app_adp_bud_change_handle(event, from_remote, para);
            }

#if (F_APP_AVP_INIT_SUPPORT == 1)
            if (app_roleswap_ear_changed_hook)
            {
                app_roleswap_ear_changed_hook(&action, event, from_remote, para);
            }
#else
            {
                app_bud_loc_changed(&action, event, from_remote, para);
            }
#endif

#if F_APP_RTK_FAST_PAIR_ADV_FEATURE_SUPPORT
            if (app_cfg_const.enable_rtk_fast_pair_adv)
            {
                app_rtk_fast_pair_handle_in_out_case();
            }
#endif

#if GFPS_FEATURE_SUPPORT
            if (extend_app_cfg_const.gfps_support)
            {
                app_gfps_battery_info_report(GFPS_BATTERY_REPORT_BUD_IN_CASE);
            }
#endif

            app_db.local_loc_pre = app_db.local_loc;
            app_db.remote_loc_pre = app_db.remote_loc;

        }
        break;

    case APP_BUD_LOC_EVENT_CASE_OUT_CASE:
        {
            if (from_remote)
            {
                app_db.remote_loc = BUD_LOC_IN_AIR;
                app_db.remote_loc_received = true;
            }
            else
            {
                app_db.local_loc = BUD_LOC_IN_AIR;
                app_db.out_case_at_least = true;
            }

            if (app_cfg_const.enable_rtk_charging_box)
            {
                app_adp_bud_change_handle(event, from_remote, para);
            }

#if (F_APP_AVP_INIT_SUPPORT == 1)
            if (app_roleswap_ear_changed_hook)
            {
                app_roleswap_ear_changed_hook(&action, event, from_remote, para);
            }
#else
            {
                app_bud_loc_changed(&action, event, from_remote, para);
            }
#endif

#if F_APP_RTK_FAST_PAIR_ADV_FEATURE_SUPPORT
            if (app_cfg_const.enable_rtk_fast_pair_adv)
            {
                app_rtk_fast_pair_handle_in_out_case();
            }
#endif

#if GFPS_FEATURE_SUPPORT
            if (extend_app_cfg_const.gfps_support)
            {
                if ((app_db.local_loc != BUD_LOC_IN_CASE) && (app_db.remote_loc != BUD_LOC_IN_CASE))
                {
                    app_gfps_battery_info_report(GFPS_BATTERY_REPORT_BUDS_OUT_CASE);
                }
            }
#endif

            app_db.local_loc_pre = app_db.local_loc;
            app_db.remote_loc_pre = app_db.remote_loc;
        }
        break;

    case APP_BUD_LOC_EVENT_CASE_OPEN_CASE:
        {
            app_adp_open_case_cmd_check();

            if (from_remote)
            {
                app_db.remote_case_closed = false;
            }
            else
            {
                app_db.local_case_closed =  false;
            }
        }
        break;

    case APP_BUD_LOC_EVENT_CASE_CLOSE_CASE:
        {
            if (from_remote)
            {
                app_db.remote_case_closed = true;
                app_adp_close_case_cmd_check();

#if (F_APP_AVP_INIT_SUPPORT == 1)
                if (app_adp_close_case_hook)
                {
                    app_adp_close_case_hook();
                }
#endif
            }
            else
            {
                app_db.local_case_closed =  true;
            }
        }
        break;
#if (F_APP_SENSOR_SUPPORT == 1)
    case APP_BUD_LOC_EVENT_SENSOR_IN_EAR:
        {
            //update state
            if (from_remote)
            {
                app_db.remote_loc = BUD_LOC_IN_EAR;
                app_db.remote_loc_received = true;
            }
            else
            {
                app_db.local_loc = BUD_LOC_IN_EAR;
            }

            if (app_cfg_const.enable_rtk_charging_box)
            {
                app_adp_bud_change_handle(event, from_remote, para);
            }

#if (F_APP_AVP_INIT_SUPPORT == 1)
            if (app_roleswap_ear_changed_hook)
            {
                app_roleswap_ear_changed_hook(&action, event, from_remote, para);
            }
#else
            {
                app_bud_loc_changed(&action, event, from_remote, para);
#if F_APP_LISTENING_MODE_SUPPORT
                app_listening_rtk_in_ear();
#endif
            }
#endif
            app_db.local_loc_pre = app_db.local_loc;
            app_db.remote_loc_pre = app_db.remote_loc;
        }
        break;

    case APP_BUD_LOC_EVENT_SENSOR_OUT_EAR:
        {
            //update state
            if (from_remote)
            {
                app_db.remote_loc = BUD_LOC_IN_AIR;
                app_db.remote_loc_received = true;
            }
            else
            {
                app_db.local_loc = BUD_LOC_IN_AIR;
                app_db.out_case_at_least = true;
            }

#if (F_APP_AVP_INIT_SUPPORT == 1)
            if (app_roleswap_ear_changed_hook)
            {
                app_roleswap_ear_changed_hook(&action, event, from_remote, para);
            }
#else
            {
                app_bud_loc_changed(&action, event, from_remote, para);
#if F_APP_LISTENING_MODE_SUPPORT
                app_listening_rtk_out_ear();
#endif
            }
#endif

            app_db.local_loc_pre = app_db.local_loc;
            app_db.remote_loc_pre = app_db.remote_loc;
        }
        break;

    case APP_BUD_LOC_EVENT_SENSOR_LD_CONFIG:
        {
#if (F_APP_SENSOR_SUPPORT == 1)
            if (from_remote)
            {
                app_sensor_control(para, false, false);
            }
#endif
        }
        break;

    case APP_BUD_LOC_EVENT_SENSOR_OUT_EAR_A2DP_PLAYING:
        {
            if (from_remote)
            {
                app_db.in_ear_recover_a2dp = para;
            }
        }
        break;

    case APP_BUD_LOC_EVENT_SENSOR_SUSPEND_BY_OUT_EAR:
        {
            if (from_remote)
            {
                app_bud_loc_update_detect_suspend_by_out_ear(para);
            }
        }
        break;
#endif
    default:
        break;
    }

    APP_PRINT_TRACE4("app_bud_loc_evt_handle: local_loc %d remote_loc %d local_case_closed %d remote_case_closed %d",
                     app_db.local_loc, app_db.remote_loc, app_db.local_case_closed, app_db.remote_case_closed);

#if F_APP_IO_OUTPUT_SUPPORT
    if (app_cfg_const.enable_external_mcu_reset && from_remote == false)
    {
        app_bud_loc_handle_ext_mcu_pin(event);
    }
#endif

    app_audio_action_when_roleswap(action);

    app_bud_loc_roleswap_handle(from_remote, event);

    if (!from_remote)
    {
        app_relay_async_single(APP_MODULE_TYPE_BUD_LOC, event);
    }
}

uint16_t app_bud_loc_relay_cback(uint8_t *buf, uint8_t msg_type, bool total)
{
    uint16_t payload_len = 0;
    uint8_t *msg_ptr = NULL;
    bool skip = true;


    switch (msg_type)
    {
    case APP_BUD_LOC_EVENT_SENSOR_LD_CONFIG:
        {
            payload_len = 1;
            msg_ptr = &app_cfg_nv.light_sensor_enable;
        }
        break;

    case APP_BUD_LOC_EVENT_SENSOR_OUT_EAR_A2DP_PLAYING:
        {
            payload_len = 1;
            msg_ptr = &app_db.in_ear_recover_a2dp;
            skip = false;
        }
        break;

    case APP_BUD_LOC_EVENT_SENSOR_SUSPEND_BY_OUT_EAR:
        {
            payload_len = 1;
            msg_ptr = (uint8_t *)&app_db.detect_suspend_by_out_ear;
        }
        break;

#if (F_APP_SENSOR_SUPPORT == 1)
    case APP_BUD_LOC_EVENT_SENSOR_IN_EAR:
        {
            payload_len = 1;
            msg_ptr = (uint8_t *)&ld_sensor_cause_action;
        }
        break;

    case APP_BUD_LOC_EVENT_SENSOR_OUT_EAR:
        {
            payload_len = 1;
            msg_ptr = (uint8_t *)&ld_sensor_cause_action;
        }
        break;
#endif

    default:
        break;

    }

    return app_relay_msg_pack(buf, msg_type, APP_MODULE_TYPE_BUD_LOC, payload_len, msg_ptr, skip,
                              total);
}

static void app_bud_loc_parse_cback(uint8_t msg_type, uint8_t *buf, uint16_t len,
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

        app_bud_loc_evt_handle(event, 1, para);
    }
}

void app_bud_loc_init(void)
{
    app_db.local_loc = BUD_LOC_UNKNOWN;
    app_db.remote_loc = BUD_LOC_UNKNOWN;
    app_db.last_bud_loc_event = APP_BUD_LOC_EVENT_MAX;

    gap_reg_timer_cb(app_bud_loc_timeout_cb, &app_bud_loc_timer_queue_id);

    app_relay_cback_register(app_bud_loc_relay_cback, app_bud_loc_parse_cback,
                             APP_MODULE_TYPE_BUD_LOC, APP_BUD_LOC_EVENT_MAX);
}
#endif
