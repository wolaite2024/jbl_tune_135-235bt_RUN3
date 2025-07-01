/*
 * Copyright (c) 2018, Realsil Semiconductor Corporation. All rights reserved.
 */
#include <stdlib.h>
#include "string.h"
#include "trace.h"
#include "os_mem.h"
#include "os_sched.h"
#include "gap_timer.h"
#include "btm.h"
#include "audio.h"
#include "remote.h"
#include "bt_bond.h"
#include "app_cfg.h"
#include "audio_probe.h"
#include "app_main.h"
#include "app_report.h"
#include "app_roleswap.h"
#include "app_roleswap_control.h"
#include "app_link_util.h"
#include "app_sdp.h"
#include "app_hfp.h"
#include "app_audio_policy.h"
#include "app_bt_policy_int.h"
#include "audio_volume.h"
#include "app_mmi.h"
#if F_APP_ANC_SUPPORT
#include "app_anc.h"
#endif
#include "app_relay.h"
#include "app_bond.h"
#include "app_line_in.h"
#include "app_auto_power_off.h"
#include "audio_track.h"
#include "app_multilink.h"
#include "app_cmd.h"
#if F_APP_LISTENING_MODE_SUPPORT
#include "app_listening_mode.h"
#endif
#if (F_APP_DONGLE_FEATURE_SUPPORT == 1)
#include "app_dongle_record.h"
#include "app_transfer.h"
#endif
#if F_APP_TEAMS_FEATURE_SUPPORT
#include "app_teams_extend_led.h"
#include "app_teams_audio_policy.h"
#endif
#if F_APP_SINGLE_MUTLILINK_SCENERIO_1
#include "app_single_multilink_customer.h"
#if F_APP_USB_AUDIO_SUPPORT
#include "app_usb_audio.h"
#endif
#endif
#include "app_sensor.h"
#if F_APP_HARMAN_FEATURE_SUPPORT
#include "app_harman_vendor_cmd.h"
#include "sidetone.h"
#endif

typedef enum
{
    APP_HFP_TIMER_AUTO_ANSWER_CALL  = 0x00,
    APP_HFP_TIMER_RING              = 0x01,
    APP_HFP_TIMER_MIC_MUTE_ALARM    = 0x02,
    APP_HFP_TIMER_NO_SERVICE        = 0x03,
    APP_HFP_TIMER_ADD_SCO           = 0x04,
    APP_HFP_TIMER_CANCEL_VOICE_DAIL = 0x05,
} T_APP_HFP_TIMER;

#if (F_APP_AVP_INIT_SUPPORT == 1)
bool (*app_hfp_batt_report_hook)(void) = NULL;
#endif

static uint8_t hfp_timer_queue_id = 0;
static uint8_t org_hfp_idx = 0xff;
static void *hfp_auto_answer_call = NULL;
void *timer_handle_hfp_ring = NULL;
static void *timer_handle_mic_mute_alarm = NULL;
static void *timer_handle_hfp_no_service = NULL;
static void *timer_handle_hfp_add_sco = NULL;
static void *timer_handle_iphone_voice_dail = NULL;

#if F_APP_ERWS_SUPPORT
static uint32_t hfp_auto_answer_call_start_time;
#endif
static uint32_t hfp_auto_answer_call_interval;

static T_BT_HFP_CALL_STATUS app_call_status;
static uint8_t active_hf_index = 0;
static bool hf_ring_active = false;


void app_hfp_voice_nr_enable(void)
{
    uint8_t enable;

    enable = (app_hfp_get_call_status() >= BT_HFP_CALL_ACTIVE) ? 1 : 0;
    if (app_relay_sync_single_with_raw_msg(APP_MODULE_TYPE_AUDIO_POLICY,
                                           APP_REMOTE_MSG_VOICE_NR_SWITCH,
                                           &enable, 1, REMOTE_TIMER_HIGH_PRECISION, 0, false) == false)
    {
#if !GFPS_SASS_SUPPORT
        audio_probe_dsp_ipc_send_call_status(enable);
#endif
    }
}

void app_hfp_inband_tone_mute_ctrl(void)
{
    uint8_t cmd_ptr[8];
    T_APP_BR_LINK *p_link;

    p_link = &(app_db.br_link[active_hf_index]);

    if (p_link != NULL)
    {
        if (p_link->connected_profile & (HFP_PROFILE_MASK | HSP_PROFILE_MASK))
        {
            if (p_link->call_status == BT_HFP_INCOMING_CALL_ONGOING)
            {
                p_link->voice_muted = true;
            }
            else
            {
                p_link->voice_muted = false;
            }

            if (app_db.remote_session_state == REMOTE_SESSION_STATE_CONNECTED)
            {
                memcpy(&cmd_ptr[0], app_db.br_link[active_hf_index].bd_addr, 6);
                cmd_ptr[6] = AUDIO_STREAM_TYPE_VOICE;
                cmd_ptr[7] = p_link->voice_muted;

                app_relay_sync_single_with_raw_msg(APP_MODULE_TYPE_AUDIO_POLICY,
                                                   APP_REMOTE_MSG_INBAND_TONE_MUTE_SYNC,
                                                   cmd_ptr, 8, REMOTE_TIMER_HIGH_PRECISION, 0, false);
            }
            else
            {
                if (!p_link->voice_muted)
                {
                    audio_track_volume_out_unmute(p_link->sco_track_handle);
                }
            }
        }
    }

    APP_PRINT_INFO1("app_hfp_inband_tone_mute_ctrl voice_muted: %d", app_db.voice_muted);
}

static void app_hfp_ring_alert(T_APP_BR_LINK *p_link)
{
    T_APP_AUDIO_TONE_TYPE tone_type = TONE_TYPE_INVALID;
    tone_type = (T_APP_AUDIO_TONE_TYPE)app_hfp_get_call_in_tone_type(p_link);
#if F_APP_MUTILINK_VA_PREEMPTIVE
    if ((app_cfg_const.enable_multi_link) &&
        (app_find_b2s_link_num() == MULTILINK_SRC_CONNECTED) &&
        app_multilink_sass_bitmap(p_link->id, MULTI_HFP_PREEM, false))
    {
        if (app_cfg_const.enable_multi_sco_disc_resume)
        {
            app_pause_inactive_a2dp_link_stream(p_link->id, true);
        }
        else
        {
            app_pause_inactive_a2dp_link_stream(p_link->id, false);
        }
    }

    if ((app_cfg_const.enable_multi_link) &&
        (p_link->id != active_hf_index))
    {
        APP_PRINT_INFO1("app_hfp_ring_alert already sco %d", app_cfg_const.enable_multi_outband_call_tone);
        if (app_cfg_const.enable_multi_outband_call_tone)
        {
            app_audio_tone_type_play(tone_type, false, true);

            if (app_cfg_const.timer_hfp_ring_period != 0)
            {
                gap_start_timer(&timer_handle_hfp_ring, "hfp_ring", hfp_timer_queue_id,
                                APP_HFP_TIMER_RING, p_link->id, app_cfg_const.timer_hfp_ring_period * 1000);
            }
            else
            {
                hf_ring_active = false;
            }
        }
        else
        {
            hf_ring_active = false;
        }
    }
    else
#endif
    {
        app_audio_tone_type_play(tone_type, false, true);

        if (app_cfg_const.timer_hfp_ring_period != 0)
        {
            gap_start_timer(&timer_handle_hfp_ring, "hfp_ring", hfp_timer_queue_id,
                            APP_HFP_TIMER_RING, p_link->id, app_cfg_const.timer_hfp_ring_period * 1000);
        }
        else
        {
            hf_ring_active = false;
        }
    }
#if F_APP_HARMAN_FEATURE_SUPPORT
    if ((app_harman_ble_ota_get_upgrate_status()) && (timer_handle_hfp_ring != NULL))
    {
        app_harman_ota_exit(OTA_EXIT_REASON_OUTBAND_RING);
    }

    if (timer_handle_hfp_ring != NULL)
    {
#if HARMAN_SUPPORT_WATER_EJECTION
        app_harman_water_ejection_stop();
#endif
        app_harman_sco_status_notify();
    }
#endif
}

void app_hfp_always_playing_outband_tone(void)
{
    T_APP_BR_LINK *p_link;

    p_link = &(app_db.br_link[active_hf_index]);

    if (p_link != NULL)
    {
        app_hfp_inband_tone_mute_ctrl();

        if (p_link->call_status == BT_HFP_INCOMING_CALL_ONGOING)
        {
            app_hfp_ring_alert(p_link);
        }

        APP_PRINT_INFO2("app_hfp_always_playing_outband_tone: active_hf_index %d, call_status %d",
                        active_hf_index,
                        p_link->call_status);
    }
}

void app_hfp_update_call_status(void)
{
    uint8_t i = 0;
    uint8_t inactive_hf_index = 0;
    uint8_t exchange_active_inactive_index_fg = 0;
    uint8_t call_status_old;
    T_BT_HFP_CALL_STATUS active_hf_status = BT_HFP_CALL_IDLE;
    T_BT_HFP_CALL_STATUS inactive_hf_status = BT_HFP_CALL_IDLE;

    call_status_old = app_hfp_get_call_status();

    active_hf_status = (T_BT_HFP_CALL_STATUS)app_db.br_link[active_hf_index].call_status;

    APP_PRINT_INFO3("app_hfp_update_call_status: call_status_old 0x%04x, active_hf_status 0x%04x, active_hf_index 0x%04x",
                    call_status_old, active_hf_status, active_hf_index);

    //find inactive device
    for (i = 0; i < MAX_BR_LINK_NUM; i++)
    {
        if (i != active_hf_index)
        {
            if ((app_db.br_link[i].connected_profile & HFP_PROFILE_MASK) ||
                (app_db.br_link[i].connected_profile & HSP_PROFILE_MASK))
            {
                inactive_hf_index = i;
                inactive_hf_status = (T_BT_HFP_CALL_STATUS)app_db.br_link[inactive_hf_index].call_status;
                break;
            }
        }
    }

    if (active_hf_status == BT_HFP_CALL_IDLE)
    {
        if (inactive_hf_status != BT_HFP_CALL_IDLE)
        {
            exchange_active_inactive_index_fg = 1;
            if (app_find_b2s_link_num() == MULTILINK_SRC_CONNECTED)
            {
                if (app_find_sco_conn_num() &&
                    (inactive_hf_status == BT_HFP_INCOMING_CALL_ONGOING) &&
                    (app_db.br_link[inactive_hf_index].sco_track_handle != NULL))
                {
                    audio_track_start(app_db.br_link[inactive_hf_index].sco_track_handle);
                }
            }
        }
    }
#if F_APP_MUTILINK_VA_PREEMPTIVE
//    else if (app_cfg_const.enable_multi_link)
//    {
//        if ((active_hf_status != BT_HFP_CALL_ACTIVE) &&
//            (active_hf_status != BT_HFP_INCOMING_CALL_ONGOING) &&
//            (active_hf_status != BT_HFP_CALL_ACTIVE_WITH_CALL_HOLD) &&
//            (active_hf_status != BT_HFP_CALL_ACTIVE_WITH_CALL_WAITING))
//        {
//            if ((inactive_hf_status == BT_HFP_CALL_ACTIVE) ||
//                (inactive_hf_status == BT_HFP_INCOMING_CALL_ONGOING) ||
//                (inactive_hf_status == BT_HFP_CALL_ACTIVE_WITH_CALL_HOLD) ||
//                (inactive_hf_status == BT_HFP_CALL_ACTIVE_WITH_CALL_WAITING))
//            {
//                exchange_active_inactive_index_fg = 1;
//            }
//            else if (inactive_hf_status == BT_HFP_VOICE_ACTIVATION_ONGOING)
//            {
//                if (active_hf_status == BT_HFP_VOICE_ACTIVATION_ONGOING)
//                {
//                    exchange_active_inactive_index_fg = 1;
//                }
//            }
//        }
//    }
    else if (app_cfg_const.enable_multi_link)
    {
        if ((active_hf_status != BT_HFP_CALL_ACTIVE) &&
            (active_hf_status != BT_HFP_INCOMING_CALL_ONGOING) &&
            (active_hf_status != BT_HFP_CALL_ACTIVE_WITH_CALL_HOLD) &&
            (active_hf_status != BT_HFP_CALL_ACTIVE_WITH_CALL_WAITING) &&
            (active_hf_status != BT_HFP_VOICE_ACTIVATION_ONGOING))
        {
            if ((inactive_hf_status == BT_HFP_CALL_ACTIVE) ||
                (inactive_hf_status == BT_HFP_INCOMING_CALL_ONGOING) ||
                (inactive_hf_status == BT_HFP_CALL_ACTIVE_WITH_CALL_HOLD) ||
                (inactive_hf_status == BT_HFP_CALL_ACTIVE_WITH_CALL_WAITING) ||
                (inactive_hf_status == BT_HFP_VOICE_ACTIVATION_ONGOING))
            {
                exchange_active_inactive_index_fg = 1;
            }
        }
    }
#endif
    else if ((active_hf_status != BT_HFP_CALL_ACTIVE) &&
             (active_hf_status != BT_HFP_CALL_ACTIVE_WITH_CALL_HOLD) &&
             (active_hf_status != BT_HFP_CALL_ACTIVE_WITH_CALL_WAITING))
    {
        if ((inactive_hf_status == BT_HFP_CALL_ACTIVE) ||
            (inactive_hf_status == BT_HFP_CALL_ACTIVE_WITH_CALL_HOLD) ||
            (inactive_hf_status == BT_HFP_CALL_ACTIVE_WITH_CALL_WAITING))
        {
            exchange_active_inactive_index_fg = 1;
        }
    }

    if (exchange_active_inactive_index_fg)
    {
        uint8_t i;
        //exchange index
        i = inactive_hf_index;
        inactive_hf_index = active_hf_index;
        app_hfp_set_active_hf_index(app_db.br_link[i].bd_addr);
#if (F_APP_HARMAN_FEATURE_SUPPORT == 0)
        app_bond_set_priority(app_db.br_link[i].bd_addr);
#endif

        //exchange status
        i = inactive_hf_status;
        inactive_hf_status = active_hf_status;
        active_hf_status = (T_BT_HFP_CALL_STATUS)i;
    }

    //update app_call_status
    switch (active_hf_status)
    {
    case BT_HFP_INCOMING_CALL_ONGOING:
        if (inactive_hf_status >= BT_HFP_CALL_ACTIVE)
        {
            app_hfp_set_call_status(BT_HFP_MULTILINK_CALL_ACTIVE_WITH_CALL_WAIT);
        }
        else
        {
            app_hfp_set_call_status(BT_HFP_INCOMING_CALL_ONGOING);
        }
        break;

    case BT_HFP_CALL_ACTIVE:
        if (inactive_hf_status >= BT_HFP_CALL_ACTIVE)
        {
            if (app_db.br_link[inactive_hf_index].sco_handle)
            {
                app_multilink_sco_preemptive(inactive_hf_index);
            }
            app_hfp_set_call_status(BT_HFP_MULTILINK_CALL_ACTIVE_WITH_CALL_HOLD);
        }
        else if (inactive_hf_status == BT_HFP_INCOMING_CALL_ONGOING)
        {
            app_hfp_set_call_status(BT_HFP_MULTILINK_CALL_ACTIVE_WITH_CALL_WAIT);
        }
        else
        {
            app_hfp_set_call_status(BT_HFP_CALL_ACTIVE);
        }
        break;

    default:
        app_hfp_set_call_status(active_hf_status);
        break;
    }

    APP_PRINT_INFO5("app_hfp_update_call_status: call_status 0x%04x, active_hf_status 0x%04x, mac : %s,"
                    "inactive_hf_status 0x%04x, inactive_hf_index 0x%04x",
                    app_hfp_get_call_status(), active_hf_status,
                    TRACE_BDADDR(app_db.br_link[active_hf_index].bd_addr), inactive_hf_status,
                    inactive_hf_index);

#if F_APP_HARMAN_FEATURE_SUPPORT
    if ((app_hfp_sco_is_connected() == false && app_hfp_get_call_status() == BT_HFP_CALL_IDLE)
        && (app_db.br_link[app_get_active_a2dp_idx()].streaming_fg == false))
    {
        au_connect_idle_to_power_off(CONNECT_IDLE_POWER_OFF_START, active_hf_index);
    }
    else
    {
        au_connect_idle_to_power_off(ACTIVE_NEED_STOP_COUNT, active_hf_index);
    }
#endif

#if ISOC_AUDIO_SUPPORT
    mtc_legacy_update_call_status();
#endif

    if (app_cfg_const.enable_multi_sco_disc_resume)
    {
        if ((app_cfg_const.enable_multi_link) &&
            (app_find_b2s_link_num() == MULTILINK_SRC_CONNECTED))
        {
            APP_PRINT_TRACE4("call_idle_resume sco_num:%d, active_status:%d, inactive_status:%d, hfp_status%d",
                             app_find_sco_conn_num(),
                             active_hf_status,
                             inactive_hf_status,
                             app_hfp_get_call_status());
            if ((app_find_sco_conn_num() == 0) &&
                (active_hf_status == BT_HFP_CALL_IDLE) &&
                inactive_hf_status == BT_HFP_CALL_IDLE)
            {
                app_resume_a2dp_link_stream(app_hfp_get_active_hf_index());
            }
        }
    }
    if (call_status_old != app_hfp_get_call_status())
    {
        if ((app_cfg_nv.bud_role == REMOTE_SESSION_ROLE_PRIMARY) &&
            (app_db.remote_session_state == REMOTE_SESSION_STATE_CONNECTED))
        {
            app_relay_async_single(APP_MODULE_TYPE_AUDIO_POLICY, APP_REMOTE_MSG_SYNC_CALL_STATUS);
        }

        if (app_hfp_get_call_status() == BT_HFP_CALL_IDLE)
        {
            if (app_audio_is_mic_mute())
            {
                app_audio_set_mic_mute_status(0);
                audio_volume_in_unmute(AUDIO_STREAM_TYPE_VOICE);
            }
        }

        if ((call_status_old == BT_HFP_INCOMING_CALL_ONGOING) ||
            (call_status_old == BT_HFP_CALL_ACTIVE_WITH_CALL_WAITING) ||
            (call_status_old == BT_HFP_MULTILINK_CALL_ACTIVE_WITH_CALL_WAIT))
        {
            T_APP_BR_LINK *p_link;
            T_APP_AUDIO_TONE_TYPE tone_type = TONE_TYPE_INVALID;

            if (exchange_active_inactive_index_fg)
            {
                p_link = &(app_db.br_link[inactive_hf_index]);
            }
            else
            {
                p_link = &(app_db.br_link[active_hf_index]);
            }

            tone_type = (T_APP_AUDIO_TONE_TYPE)app_hfp_get_call_in_tone_type(p_link);
            app_audio_tone_type_cancel(tone_type, true);
        }

#if F_APP_LISTENING_MODE_SUPPORT
        if (call_status_old == BT_HFP_CALL_IDLE)
        {
            app_listening_judge_sco_event(APPLY_LISTENING_MODE_CALL_NOT_IDLE);
        }
        else if (app_hfp_get_call_status() == BT_HFP_CALL_IDLE)
        {
            app_listening_judge_sco_event(APPLY_LISTENING_MODE_CALL_IDLE);
        }
#endif

#if F_APP_LINEIN_SUPPORT
        if (app_cfg_const.line_in_support)
        {
            if (app_db.device_state == APP_DEVICE_STATE_ON)
            {
                app_line_in_call_status_update(app_hfp_get_call_status() == BT_HFP_CALL_IDLE);
            }
        }
#endif

        if (hf_ring_active && (active_hf_status != BT_HFP_INCOMING_CALL_ONGOING))
        {
            hf_ring_active = false;
        }

        if (app_cfg_const.always_play_hf_incoming_tone_when_incoming_call)
        {
            app_hfp_always_playing_outband_tone();
        }
    }

    app_hfp_voice_nr_enable();
}

void app_hfp_set_no_service_timer(bool all_service_status)
{
    gap_stop_timer(&timer_handle_hfp_no_service);

    if (all_service_status == false)
    {
        gap_start_timer(&timer_handle_hfp_no_service, "hfp_no_service",
                        hfp_timer_queue_id, APP_HFP_TIMER_NO_SERVICE,
                        0, app_cfg_const.timer_no_service_warning * 1000);
    }
}

static void app_hfp_check_service_status()
{
    if (app_cfg_const.timer_no_service_warning > 0)
    {
        bool all_service_status = true;
        uint8_t i;

        if (app_cfg_nv.bud_role == REMOTE_SESSION_ROLE_SINGLE ||
            app_cfg_nv.bud_role == REMOTE_SESSION_ROLE_PRIMARY)
        {
            for (i = 0; i < MAX_BR_LINK_NUM; i++)
            {
                if ((app_db.br_link[i].connected_profile & HFP_PROFILE_MASK) &&
                    (app_check_b2s_link_by_id(i) == true))
                {
                    if (app_db.br_link[i].service_status == false)
                    {
                        all_service_status = false;
                        break;
                    }
                }
            }

            if ((all_service_status == false) &&
                (timer_handle_hfp_no_service == NULL) &&
                ((app_cfg_nv.bud_role == REMOTE_SESSION_ROLE_SINGLE) ||
                 (app_cfg_nv.bud_role == REMOTE_SESSION_ROLE_PRIMARY)))
            {
                app_audio_tone_type_play(TONE_HF_NO_SERVICE, false, true);
            }

            if (((all_service_status == true) && (timer_handle_hfp_no_service != NULL)) ||
                ((all_service_status == false) && (timer_handle_hfp_no_service == NULL)))
            {
                app_hfp_set_no_service_timer(all_service_status);

                if (app_cfg_nv.bud_role == REMOTE_SESSION_ROLE_PRIMARY)
                {
                    app_relay_async_single_with_raw_msg(APP_MODULE_TYPE_BT_POLICY, BT_POLICY_MSG_SYNC_SERVICE_STATUS,
                                                        (uint8_t *)&all_service_status, 1);
                }
            }
        }
    }
}

void app_hfp_batt_level_report(uint8_t *bd_addr)
{
    uint8_t batt_level = app_db.local_batt_level;
    bool need_report = false;

    if ((app_cfg_const.enable_report_lower_battery_volume) &&
        (app_db.remote_session_state == REMOTE_SESSION_STATE_CONNECTED))
    {
        if ((app_db.remote_batt_level < app_db.local_batt_level) &&
            (app_db.remote_batt_level != 0))
        {
            batt_level = app_db.remote_batt_level;
        }
    }

#if (F_APP_AVP_INIT_SUPPORT == 1)
    if (app_hfp_batt_report_hook)
    {
        need_report = app_hfp_batt_report_hook();
    }
#else
    if (app_db.hfp_report_batt)
    {
        if (app_roleswap_ctrl_get_status() == APP_ROLESWAP_STATUS_IDLE)
        {
            need_report = true;
        }
    }
#endif

    if (need_report)
    {
        bt_hfp_batt_level_report(bd_addr, batt_level);
    }
}

static void app_hfp_set_mic_mute(void)
{
    APP_PRINT_INFO1("app_hfp_set_mic_mute is_mic_mute %d", app_audio_is_mic_mute());

#if (F_APP_TEAMS_GLOBAL_MUTE_SUPPORT == 0)
    T_AUDIO_STREAM_TYPE stream_type = AUDIO_STREAM_TYPE_VOICE;

#if (F_APP_DONGLE_FEATURE_SUPPORT == 1)
    if (app_db.remote_is_8753bau)
    {
        stream_type = AUDIO_STREAM_TYPE_RECORD;
    }
#endif
#endif

    if (app_audio_is_mic_mute())
    {
#if F_APP_TEAMS_GLOBAL_MUTE_SUPPORT
        T_APP_BR_LINK *p_link;
        uint8_t app_idx = app_hfp_get_active_hf_index();
        p_link = &(app_db.br_link[app_idx]);
        audio_track_volume_in_mute(p_link->sco_track_handle);
        app_audio_tone_type_play(TONE_MIC_MUTE_ON, true, true);
#else
        audio_volume_in_mute(stream_type);
#endif
        if (app_cfg_const.timer_mic_mute_alarm != 0)
        {
            gap_start_timer(&timer_handle_mic_mute_alarm, "mic_mute_alarm",
                            hfp_timer_queue_id,
                            APP_HFP_TIMER_MIC_MUTE_ALARM, 0, app_cfg_const.timer_mic_mute_alarm * 1000);
        }
    }
    else
    {
        gap_stop_timer(&timer_handle_mic_mute_alarm);
#if F_APP_TEAMS_GLOBAL_MUTE_SUPPORT
        T_APP_BR_LINK *p_link;
        uint8_t app_idx = app_hfp_get_active_hf_index();
        p_link = &(app_db.br_link[app_idx]);
        audio_track_volume_in_unmute(p_link->sco_track_handle);
        app_audio_tone_type_play(TONE_MIC_MUTE_OFF, true, true);
#else
        audio_volume_in_unmute(stream_type);
#endif
    }
}

#if F_APP_ERWS_SUPPORT
static void app_hfp_transfer_sec_auto_answer_call_timer(void)
{
    uint32_t left_ms = 0;
    uint32_t run_ms = 0;

    if (hfp_auto_answer_call != NULL)
    {
        gap_stop_timer(&hfp_auto_answer_call);

        run_ms = os_sys_time_get() - hfp_auto_answer_call_start_time;

        if (hfp_auto_answer_call_interval > run_ms)
        {
            left_ms = hfp_auto_answer_call_interval - run_ms;
            app_relay_sync_single_with_raw_msg(APP_MODULE_TYPE_HFP, APP_REMOTE_HFP_AUTO_ANSWER_CALL_TIMER,
                                               (uint8_t *) &left_ms, 4, REMOTE_TIMER_HIGH_PRECISION, 0, false);
        }
    }
}
#endif

static void app_hfp_bt_cback(T_BT_EVENT event_type, void *event_buf, uint16_t buf_len)
{
    T_BT_EVENT_PARAM *param = event_buf;
    bool handle = true;

    switch (event_type)
    {
    case BT_EVENT_HFP_CONN_IND:
        {
            T_APP_BR_LINK *p_link;

            p_link = app_find_br_link(param->hfp_conn_ind.bd_addr);
            if (p_link == NULL)
            {
                APP_PRINT_ERROR0("app_hfp_bt_cback: no acl link found");
                return;
            }
            bt_hfp_connect_cfm(p_link->bd_addr, true);
        }
        break;

    case BT_EVENT_HFP_CONN_CMPL:
        {
            T_APP_BR_LINK *p_link;

            p_link = app_find_br_link(param->hfp_conn_cmpl.bd_addr);
            if (p_link != NULL)
            {
                uint8_t link_number;
                uint8_t pair_idx_mapping;

                p_link->call_id_type_check = true;
                p_link->call_id_type_num = false;

                app_bond_get_pair_idx_mapping(p_link->bd_addr, &pair_idx_mapping);
                bt_hfp_speaker_gain_level_report(p_link->bd_addr, app_cfg_nv.voice_gain_level[pair_idx_mapping]);
#if F_APP_TEAMS_GLOBAL_MUTE_SUPPORT
                if (p_link->is_mute)
                {
                    bt_hfp_microphone_gain_level_report(p_link->bd_addr, 0x00);
                }
                else
                {
                    bt_hfp_microphone_gain_level_report(p_link->bd_addr, 0x0f);
                }
#else
                bt_hfp_microphone_gain_level_report(p_link->bd_addr, 0x0a);
#endif

                {
                    app_hfp_batt_level_report(p_link->bd_addr);
                }

                link_number = app_connected_profile_link_num(HFP_PROFILE_MASK | HSP_PROFILE_MASK);
#if F_APP_SINGLE_MUTLILINK_SCENERIO_1
#if F_APP_USB_AUDIO_SUPPORT
                if ((link_number == 1) && !app_usb_audio_is_active())
#else
                if (link_number == 1)
#endif
                {
                    app_teams_multilink_handle_first_voice_profile_connect((app_find_br_link(p_link->bd_addr))->id);
                }
#else
#if F_APP_HARMAN_FEATURE_SUPPORT
                uint8_t find_another_hfp_idx = MAX_BR_LINK_NUM;

                for (uint8_t i = 0; i < MAX_BR_LINK_NUM; i++)
                {
                    if (app_check_b2s_link_by_id(i))
                    {
                        if (i != p_link->id)
                        {
                            find_another_hfp_idx = i;
                        }
                    }
                }
#endif
                if (link_number == 1)
                {
                    app_hfp_set_active_hf_index(p_link->bd_addr);
                    app_bond_set_priority(p_link->bd_addr);
                }
#if F_APP_HARMAN_FEATURE_SUPPORT
                else if (find_another_hfp_idx != MAX_BR_LINK_NUM)
                {
                    APP_PRINT_TRACE4("BT_EVENT_HFP_CONN_CMPL: active_a2dp_idx: %d, %d, find_another_hfp_idx: %d, %d",
                                     p_link->id, app_db.br_link[p_link->id].streaming_fg,
                                     find_another_hfp_idx, app_db.br_link[find_another_hfp_idx].streaming_fg);
                    if (app_db.br_link[find_another_hfp_idx].streaming_fg ||
                        app_db.br_link[find_another_hfp_idx].call_status != BT_HFP_CALL_IDLE)
                    {
                        app_bond_set_priority(p_link->bd_addr);
                        app_bond_set_priority(app_db.br_link[find_another_hfp_idx].bd_addr);
                    }
                    else
                    {
                        app_bond_set_priority(app_db.br_link[find_another_hfp_idx].bd_addr);
                        app_bond_set_priority(p_link->bd_addr);
                    }
                }
#endif
#endif
                if (app_db.br_link[app_db.first_hf_index].app_hf_state == APP_HF_STATE_STANDBY)
                {
                    app_db.first_hf_index = p_link->id;
                }
                else
                {
                    app_db.last_hf_index = p_link->id;
                }
                p_link->app_hf_state = APP_HF_STATE_CONNECTED;
            }
        }
        break;

    case BT_EVENT_HFP_CALL_STATUS:
        {
            T_APP_BR_LINK *p_link;
#if (C_APP_END_OUTGOING_CALL_PLAY_CALL_END_TONE == 0)
            uint8_t temp_idx;
            temp_idx = app_hfp_get_active_hf_index();
#endif
            p_link = app_find_br_link(param->hfp_call_status.bd_addr);
            if (p_link != NULL)
            {
                p_link->call_status = param->hfp_call_status.curr_status;

                if ((app_cfg_const.enable_auto_answer_incoming_call == 1) &&
                    (p_link->call_status == BT_HFP_INCOMING_CALL_ONGOING))
                {
#if F_APP_ERWS_SUPPORT
                    hfp_auto_answer_call_start_time = os_sys_time_get();
#endif
                    hfp_auto_answer_call_interval = app_cfg_const.timer_hfp_auto_answer_call * 1000;
                    gap_start_timer(&hfp_auto_answer_call, "auto_answer_call",
                                    hfp_timer_queue_id, APP_HFP_TIMER_AUTO_ANSWER_CALL,
                                    p_link->id, hfp_auto_answer_call_interval);
                }

                if (p_link->call_status == BT_HFP_CALL_IDLE)
                {
                    p_link->call_id_type_check = true;
                    p_link->call_id_type_num = false;
                }

                org_hfp_idx = app_hfp_get_active_hf_index();
#if F_APP_SINGLE_MUTLILINK_SCENERIO_1
                app_teams_multilink_voice_priority_rearrangment(p_link->id,
                                                                (T_BT_HFP_CALL_STATUS)param->hfp_call_status.curr_status);
#else
                app_hfp_update_call_status();
#endif
                if (p_link->call_status == BT_HFP_CALL_IDLE)
                {
#if F_APP_MUTILINK_VA_PREEMPTIVE
                    if (!app_multi_check_in_sniffing())
                    {
                        if (p_link->id == org_hfp_idx && p_link->sco_handle == NULL)
                        {
                            app_handle_sniffing_link_disconnect_event(p_link->id);
                        }
                    }
#endif
                }

#if (F_APP_AVP_INIT_SUPPORT == 1)
//rsv for avp
#else
                //Workaround: iOS may not create SCO and not issue +BVRA:0 when trigger siri repeatedly
                //Add a protection timer to exit voice activation state
                if ((p_link->call_status == BT_HFP_VOICE_ACTIVATION_ONGOING) &&
                    (p_link->remote_device_vendor_id == APP_REMOTE_DEVICE_IOS))
                {
                    gap_start_timer(&timer_handle_iphone_voice_dail, "cancel_iphone_voice_dail", hfp_timer_queue_id,
                                    APP_HFP_TIMER_CANCEL_VOICE_DAIL, p_link->id, 5000);
                }
#endif

                if (app_cfg_nv.bud_role == REMOTE_SESSION_ROLE_PRIMARY ||
                    app_cfg_nv.bud_role == REMOTE_SESSION_ROLE_SINGLE)
                {
                    if (param->hfp_call_status.prev_status == BT_HFP_INCOMING_CALL_ONGOING &&
                        param->hfp_call_status.curr_status == BT_HFP_CALL_IDLE)
                    {
                        if (app_db.reject_call_by_key)
                        {
                            app_db.reject_call_by_key = false;
                            app_audio_tone_type_play(TONE_HF_CALL_REJECT, false, true);
                        }
                    }

                    if (p_link->id == temp_idx &&
                        param->hfp_call_status.prev_status == BT_HFP_CALL_ACTIVE &&
                        param->hfp_call_status.curr_status == BT_HFP_CALL_IDLE)
                    {
                        app_audio_tone_type_play(TONE_HF_CALL_END, false, true);
                    }

                    if (p_link->id == app_hfp_get_active_hf_index() &&
                        param->hfp_call_status.prev_status != BT_HFP_CALL_ACTIVE &&
                        param->hfp_call_status.curr_status == BT_HFP_CALL_ACTIVE)
                    {
                        app_audio_tone_type_play(TONE_HF_CALL_ACTIVE, false, true);
                    }

                    if (param->hfp_call_status.prev_status != BT_HFP_OUTGOING_CALL_ONGOING &&
                        param->hfp_call_status.curr_status == BT_HFP_OUTGOING_CALL_ONGOING)
                    {
                        app_audio_tone_type_play(TONE_HF_OUTGOING_CALL, false, true);
                    }
                }
            }

            if (param->hfp_call_status.curr_status == BT_HFP_CALL_ACTIVE ||
                param->hfp_call_status.curr_status == BT_HFP_INCOMING_CALL_ONGOING ||
                param->hfp_call_status.curr_status == BT_HFP_OUTGOING_CALL_ONGOING)
            {
                if (app_cfg_const.timer_auto_power_off_while_phone_connected_and_anc_apt_off)
                {
                    app_auto_power_off_disable(AUTO_POWER_OFF_MASK_SOURCE_LINK);
                }

                app_audio_set_bud_stream_state(BUD_STREAM_STATE_VOICE);
            }
            else if (param->hfp_call_status.curr_status == BT_HFP_CALL_IDLE)
            {
#if F_APP_SINGLE_MUTLILINK_SCENERIO_1
                if ((app_cfg_const.timer_auto_power_off_while_phone_connected_and_anc_apt_off) &&
                    !app_teams_multilink_check_device_music_is_playing())
#else
                if (app_cfg_const.timer_auto_power_off_while_phone_connected_and_anc_apt_off)
#endif
                {
                    app_auto_power_off_enable(AUTO_POWER_OFF_MASK_SOURCE_LINK,
                                              app_cfg_const.timer_auto_power_off_while_phone_connected_and_anc_apt_off);
                }

                app_audio_set_bud_stream_state(BUD_STREAM_STATE_IDLE);
            }

            if (param->hfp_call_status.curr_status == BT_HFP_CALL_ACTIVE)
            {
#if (F_APP_AVP_INIT_SUPPORT == 1)
//rsv for avp
#else
                if (LIGHT_SENSOR_ENABLED &&
                    ((app_db.local_loc == BUD_LOC_IN_EAR) || (app_db.remote_loc == BUD_LOC_IN_EAR)))
                {
                    gap_stop_timer(&timer_handle_hfp_add_sco);
                    gap_start_timer(&timer_handle_hfp_add_sco, "hfp_add_sco", hfp_timer_queue_id,
                                    APP_HFP_TIMER_ADD_SCO, active_hf_index, 400);
                }
#endif
            }
        }
        break;

    case BT_EVENT_HFP_SERVICE_STATUS:
        {
            T_APP_BR_LINK *p_link;

            p_link = app_find_br_link(param->hfp_service_status.bd_addr);

            if (p_link != NULL)
            {
                p_link->service_status = param->hfp_service_status.status;
                app_hfp_check_service_status();
            }
        }
        break;

    case BT_EVENT_HFP_CALL_WAITING_IND:
    case BT_EVENT_HFP_CALLER_ID_IND:
        {
            if (app_cfg_nv.bud_role == REMOTE_SESSION_ROLE_SINGLE)
            {
                T_APP_BR_LINK *br_link;
                T_APP_LE_LINK *le_link;
                char *number;
                uint16_t num_len;

                if (event_type == BT_EVENT_HFP_CALLER_ID_IND)
                {
                    br_link = app_find_br_link(param->hfp_caller_id_ind.bd_addr);
                    le_link = app_find_le_link_by_addr(param->hfp_caller_id_ind.bd_addr);
                    number = (char *)param->hfp_caller_id_ind.number;
                    num_len = strlen(param->hfp_caller_id_ind.number);
                }
                else
                {
                    br_link = app_find_br_link(param->hfp_call_waiting_ind.bd_addr);
                    le_link = app_find_le_link_by_addr(param->hfp_call_waiting_ind.bd_addr);
                    number = (char *)param->hfp_call_waiting_ind.number;
                    num_len = strlen(param->hfp_call_waiting_ind.number);
                }

                if (br_link != NULL)
                {
                    /* Sanity check if BR/EDR TTS session is ongoing */
                    if (br_link->cmd_set_enable == true &&
                        br_link->tts_info.tts_handle != NULL)
                    {
                        break;
                    }

                    /* Sanity check if BLE TTS session is ongoing */
                    if (le_link != NULL &&
                        le_link->cmd_set_enable == true &&
                        le_link->tts_info.tts_handle != NULL)
                    {
                        break;
                    }

                    if (br_link->call_id_type_check == true)
                    {
                        if (br_link->connected_profile & PBAP_PROFILE_MASK)
                        {
                            if (bt_pbap_vcard_listing_by_number_pull(br_link->bd_addr, number) == false)
                            {
                                br_link->call_id_type_check = false;
                                br_link->call_id_type_num = true;
                            }
                        }
                        else
                        {
                            br_link->call_id_type_check = false;
                            br_link->call_id_type_num = true;
                        }
                    }

                    if (br_link->call_id_type_check == false)
                    {
                        if (br_link->call_id_type_num == true)
                        {
                            uint8_t *data;
                            uint16_t len;
                            len = num_len + 3;
                            data = malloc(len);

                            if (data != NULL)
                            {
                                data[1] = CALLER_ID_TYPE_NUMBER;
                                data[2] = num_len;
                                memcpy(data + 3, number, num_len);

                                if (br_link->connected_profile & SPP_PROFILE_MASK)
                                {
                                    data[0] = br_link->id;
                                    app_report_event(CMD_PATH_SPP, EVENT_CALLER_ID, br_link->id, data, len);
                                }
                                else if (br_link->connected_profile & IAP_PROFILE_MASK)
                                {
                                    data[0] = br_link->id;
                                    app_report_event(CMD_PATH_IAP, EVENT_CALLER_ID, br_link->id, data, len);
                                }
                                else if (le_link != NULL)
                                {
                                    data[0] = le_link->id;
                                    app_report_event(CMD_PATH_LE, EVENT_CALLER_ID, le_link->id, data, len);
                                }

                                free(data);
                            }
                        }
                    }
                }
            }
        }
        break;

    case BT_EVENT_HFP_RING_ALERT:
        {
            T_APP_BR_LINK *p_link;

            p_link = app_find_br_link(param->hfp_ring_alert.bd_addr);
            if (p_link != NULL)
            {
                p_link->is_inband_ring = param->hfp_ring_alert.is_inband;

                if ((app_cfg_const.always_play_hf_incoming_tone_when_incoming_call == false) &&
                    ((p_link->is_inband_ring == false) ||
#if F_APP_SINGLE_MUTLILINK_SCENERIO_1
                     (p_link->id != app_teams_multilink_get_active_voice_index()))) /* TODO check active sco link */
#else
                     (p_link->id != active_hf_index))) /* TODO check active sco link */
#endif
                {
                    if (hf_ring_active == false && p_link->call_status == BT_HFP_INCOMING_CALL_ONGOING)
                    {
                        hf_ring_active = true;

                        app_hfp_ring_alert(p_link);
                    }
                }
            }
        }
        break;

    case BT_EVENT_HFP_SPK_VOLUME_CHANGED:
        {
            T_APP_BR_LINK *p_link;
            p_link = app_find_br_link(param->hfp_spk_volume_changed.bd_addr);
            if (p_link != NULL)
            {
                if (app_db.remote_session_state == REMOTE_SESSION_STATE_DISCONNECTED)
                {
                    uint8_t pair_idx_mapping;

                    app_bond_get_pair_idx_mapping(p_link->bd_addr, &pair_idx_mapping);

                    app_cfg_nv.voice_gain_level[pair_idx_mapping] = (param->hfp_spk_volume_changed.volume *
                                                                     app_cfg_const.voice_out_volume_max +
                                                                     0x0f / 2) / 0x0f;

                    app_audio_vol_set(p_link->sco_track_handle, app_cfg_nv.voice_gain_level[pair_idx_mapping]);
                    app_audio_track_spk_unmute(AUDIO_STREAM_TYPE_VOICE);
                }
                else
                {
                    uint8_t cmd_ptr[8];
                    uint8_t pair_idx_mapping;

                    app_bond_get_pair_idx_mapping(p_link->bd_addr, &pair_idx_mapping);
                    app_cfg_nv.voice_gain_level[pair_idx_mapping] = (param->hfp_spk_volume_changed.volume *
                                                                     app_cfg_const.voice_out_volume_max +
                                                                     0x0f / 2) / 0x0f;
                    memcpy(&cmd_ptr[0], p_link->bd_addr, 6);
                    cmd_ptr[6] = AUDIO_STREAM_TYPE_VOICE;
                    cmd_ptr[7] = app_cfg_nv.voice_gain_level[pair_idx_mapping];
                    app_relay_sync_single_with_raw_msg(APP_MODULE_TYPE_AUDIO_POLICY, APP_REMOTE_MSG_HFP_VOLUME_SYNC,
                                                       cmd_ptr, 8, REMOTE_TIMER_HIGH_PRECISION, 0, false);
                }
            }
        }
        break;

    case BT_EVENT_HFP_MIC_VOLUME_CHANGED:
        {
#if F_APP_TEAMS_FEATURE_SUPPORT
            T_APP_BR_LINK *p_link;
            p_link = app_find_br_link(param->hfp_spk_volume_changed.bd_addr);

            if (p_link != NULL)
            {
                uint8_t teams_active_device_idx = app_hfp_get_active_hf_index();

                APP_PRINT_TRACE2("app_hfp_bt_cback: BT_EVENT_HFP_MIC_VOLUME_CHANGED active idx 0x%1x, p_link id 0x%1x",
                                 teams_active_device_idx, p_link->id);
                if (teams_active_device_idx == 0xff)
                {
                    break;
                }

                if (param->hfp_mic_volume_changed.volume == 0)
                {
#if F_APP_TEAMS_GLOBAL_MUTE_SUPPORT
                    p_link->is_mute = true;
#endif
                    if (p_link->id == teams_active_device_idx)
                    {
                        app_mmi_handle_action(MMI_DEV_MIC_MUTE);
                    }
                }
                else if (param->hfp_mic_volume_changed.volume == 15)
                {
#if F_APP_TEAMS_GLOBAL_MUTE_SUPPORT
                    p_link->is_mute = false;
#endif
                    if (p_link->id == teams_active_device_idx)
                    {
                        app_mmi_handle_action(MMI_DEV_MIC_UNMUTE);
                    }
                }
            }
#endif
        }
        break;

    case BT_EVENT_REMOTE_CONN_CMPL:
        {
            if (app_cfg_nv.bud_role == REMOTE_SESSION_ROLE_PRIMARY)
            {
                app_hfp_voice_nr_enable();
            }
        }
        break;

    case BT_EVENT_HFP_DISCONN_CMPL:
        {
            T_APP_BR_LINK *p_link;

            p_link = app_find_br_link(param->hfp_disconn_cmpl.bd_addr);
            if (p_link != NULL)
            {
                if (param->hfp_disconn_cmpl.cause == (HCI_ERR | HCI_ERR_CONN_ROLESWAP))
                {
                    //do nothing
                }
                else
                {
                    p_link->call_status = BT_HFP_CALL_IDLE;
                    p_link->app_hf_state = APP_HF_STATE_STANDBY;
                    if (app_db.first_hf_index == p_link->id)
                    {
                        app_db.first_hf_index = app_db.last_hf_index;
                    }

#if F_APP_SINGLE_MUTLILINK_SCENERIO_1
                    app_teams_multilink_handle_voice_profile_disconnect((app_find_br_link(
                                                                             param->hfp_disconn_cmpl.bd_addr))->id);
#else
                    for (uint8_t i = 0; i < MAX_BR_LINK_NUM; i++)
                    {
                        if (app_db.br_link[i].connected_profile & (HFP_PROFILE_MASK | HSP_PROFILE_MASK))
                        {
                            app_hfp_set_active_hf_index(app_db.br_link[i].bd_addr);
#if (F_APP_HARMAN_FEATURE_SUPPORT == 0)
                            app_bond_set_priority(app_db.br_link[i].bd_addr);
#endif
                            break;
                        }
                    }

                    if (app_hfp_get_call_status() != BT_HFP_CALL_IDLE)
                    {
                        app_hfp_update_call_status();
#if F_APP_MUTILINK_VA_PREEMPTIVE
                        if (p_link->id == app_multilink_get_active_idx() &&
                            !app_multi_check_in_sniffing())
                        {
                            app_handle_sniffing_link_disconnect_event(p_link->id);
                        }
#endif
                    }
#endif
                }

                if ((param->hfp_disconn_cmpl.cause & ~HCI_ERR) != HCI_ERR_CONN_ROLESWAP)
                {
                    app_hfp_check_service_status();
                }
            }
        }
        break;

#if F_APP_ERWS_SUPPORT
    case BT_EVENT_SCO_SNIFFING_CONN_CMPL:
        {
        }
        break;

    case BT_EVENT_REMOTE_ROLESWAP_STATUS:
        {
            T_BT_EVENT_PARAM *param = event_buf;
            T_BT_ROLESWAP_STATUS event;
#if(F_APP_DONGLE_FEATURE_SUPPORT == 1)
            uint8_t app_idx = app_get_active_a2dp_idx();
#endif

            event = param->remote_roleswap_status.status;

            if (event == BT_ROLESWAP_STATUS_SUCCESS)
            {
                if (app_cfg_nv.bud_role == REMOTE_SESSION_ROLE_PRIMARY)
                {
                    if (app_audio_check_mic_mute_enable())
                    {
                        app_hfp_set_mic_mute();
                    }

#if(F_APP_DONGLE_FEATURE_SUPPORT == 1)
                    if (app_db.dongle_is_enable_mic)
                    {
                        app_dongle_record_init();
                        app_dongle_start_recording(app_db.br_link[app_idx].bd_addr);
                    }
#endif
                }
                else
                {
                    gap_stop_timer(&timer_handle_mic_mute_alarm);
                    if (app_audio_check_mic_mute_enable())
                    {
#if F_APP_RWS_MULTI_SPK_SUPPORT
#else
                        audio_volume_in_mute(AUDIO_STREAM_TYPE_VOICE);
#endif
                    }

#if(F_APP_DONGLE_FEATURE_SUPPORT == 1)
                    if (app_db.dongle_is_enable_mic)
                    {
                        app_transfer_queue_reset(CMD_PATH_SPP);
                        app_dongle_stop_recording(app_db.br_link[app_idx].bd_addr);
                    }
#endif
                    app_hfp_transfer_sec_auto_answer_call_timer();
                }
            }
        }
        break;
#endif

    default:
        handle = false;
        break;
    }

    if (handle == true)
    {
        APP_PRINT_INFO1("app_hfp_bt_cback: event_type 0x%04x", event_type);
    }
}

static void app_hfp_parse_cback(uint8_t msg_type, uint8_t *buf, uint16_t len,
                                T_REMOTE_RELAY_STATUS status)
{
    APP_PRINT_TRACE2("app_hfp_parse_cback: msg %d status %d", msg_type, status);

    switch (msg_type)
    {
    case APP_REMOTE_HFP_SYNC_SCO_GAIN_WHEN_CALL_ACTIVE:
        {
            if (status == REMOTE_RELAY_STATUS_SYNC_TOUT ||
                status == REMOTE_RELAY_STATUS_SYNC_EXPIRED ||
                status == REMOTE_RELAY_STATUS_SYNC_REF_CHANGED ||
                status == REMOTE_RELAY_STATUS_SEND_FAILED)
            {
                uint8_t addr[6] = {0};
                uint8_t voice_gain = 0;

                memcpy(addr, buf, 6);
                voice_gain = buf[6];

                app_hfp_set_link_voice_gain(addr, voice_gain);
            }
        }
        break;

    case APP_REMOTE_HFP_AUTO_ANSWER_CALL_TIMER:
        {
            if (status == REMOTE_RELAY_STATUS_SYNC_TOUT ||
                status == REMOTE_RELAY_STATUS_SYNC_EXPIRED ||
                status == REMOTE_RELAY_STATUS_SYNC_REF_CHANGED ||
                status == REMOTE_RELAY_STATUS_SEND_FAILED)
            {
                if (app_cfg_nv.bud_role == REMOTE_SESSION_ROLE_PRIMARY)
                {
                    T_APP_BR_LINK *p_link = NULL;
                    uint8_t app_idx;
                    uint32_t *left_ms = (uint32_t *)buf;

                    for (app_idx = 0; app_idx < MAX_BR_LINK_NUM; app_idx++)
                    {
                        if (app_check_b2s_link_by_id(app_idx))
                        {
                            p_link = &app_db.br_link[app_idx];
                            if (p_link->call_status == BT_HFP_INCOMING_CALL_ONGOING)
                            {
#if F_APP_ERWS_SUPPORT
                                hfp_auto_answer_call_start_time = os_sys_time_get();
#endif
                                hfp_auto_answer_call_interval = *left_ms;

                                gap_start_timer(&hfp_auto_answer_call, "auto_answer_call",
                                                hfp_timer_queue_id, APP_HFP_TIMER_AUTO_ANSWER_CALL,
                                                p_link->id, hfp_auto_answer_call_interval);
                                break;
                            }
                        }
                    }
                }
            }
        }

    default:
        break;
    }
}

T_BT_HFP_CALL_STATUS app_hfp_get_call_status(void)
{
    return app_call_status;
}

void app_hfp_set_call_status(T_BT_HFP_CALL_STATUS call_status)
{
    static T_BT_HFP_CALL_STATUS pre_call_status = BT_HFP_CALL_IDLE;
    uint8_t active_idx = active_hf_index;
    uint8_t pair_idx_mapping = 0;
    bool get_pair_idx = app_bond_get_pair_idx_mapping(app_db.br_link[active_idx].bd_addr,
                                                      &pair_idx_mapping);

    APP_PRINT_TRACE2("app_hfp_set_call_status: %d -> %d", pre_call_status, call_status);

    app_call_status = call_status;

    app_roleswap_ctrl_check(APP_ROLESWAP_CTRL_EVENT_CALL_STATUS_CHAGNED);

    if (app_cfg_const.fixed_inband_tone_gain && get_pair_idx == true)
    {
        uint8_t voice_gain = app_cfg_nv.voice_gain_level[pair_idx_mapping];

        if (pre_call_status == BT_HFP_OUTGOING_CALL_ONGOING ||
            pre_call_status == BT_HFP_INCOMING_CALL_ONGOING)
        {
            if (call_status == BT_HFP_CALL_ACTIVE)
            {
                if (app_cfg_nv.bud_role == REMOTE_SESSION_ROLE_PRIMARY)
                {
                    if (app_db.remote_session_state == REMOTE_SESSION_STATE_CONNECTED)
                    {
                        uint8_t tmp[7] = {0};

                        memcpy(tmp, app_db.br_link[active_idx].bd_addr, 6);
                        tmp[6] = voice_gain;

                        app_relay_sync_single_with_raw_msg(APP_MODULE_TYPE_HFP,
                                                           APP_REMOTE_HFP_SYNC_SCO_GAIN_WHEN_CALL_ACTIVE,
                                                           tmp, sizeof(tmp), REMOTE_TIMER_HIGH_PRECISION, 0, false);
                    }
                    else
                    {
                        app_hfp_set_link_voice_gain(app_db.br_link[active_idx].bd_addr, voice_gain);
                    }
                }
            }
        }
        else if (call_status == BT_HFP_OUTGOING_CALL_ONGOING || call_status == BT_HFP_INCOMING_CALL_ONGOING)
        {
            app_hfp_set_link_voice_gain(app_db.br_link[active_idx].bd_addr, app_cfg_const.inband_tone_gain_lv);
        }
    }

    pre_call_status = call_status;
}

uint8_t app_hfp_get_active_hf_index(void)
{
#if F_APP_SINGLE_MUTLILINK_SCENERIO_1
    return app_teams_multilink_get_active_index();
#else
    return active_hf_index;
#endif
}

void app_hfp_only_set_active_hf_index(uint8_t index)
{
    active_hf_index = index;
}

bool app_hfp_sco_is_connected(void)
{
    return (app_db.br_link[active_hf_index].sco_handle == 0) ? false : true;
}

bool app_hfp_set_link_voice_gain(uint8_t *addr, uint8_t gain)
{
    T_APP_BR_LINK *p_link = NULL;
    bool ret = false;

    p_link = app_find_br_link(addr);


    if (p_link && p_link->sco_track_handle)
    {
        app_audio_vol_set(p_link->sco_track_handle, gain);
        app_audio_track_spk_unmute(AUDIO_STREAM_TYPE_VOICE);
        ret = true;
    }

    return ret;
}

bool app_hfp_set_active_hf_index(uint8_t *bd_addr)
{
    T_APP_BR_LINK *p_link;

    p_link = app_find_br_link(bd_addr);
    if (p_link)
    {
        active_hf_index = p_link->id;
#if F_APP_HARMAN_FEATURE_SUPPORT
        au_set_harman_aling_active_idx(active_hf_index);
#endif
        if (p_link->id == app_multilink_get_active_idx())
        {
            if (p_link->sco_track_handle)
            {
                audio_track_start(p_link->sco_track_handle);
            }
            return bt_sco_link_switch(bd_addr);
        }
    }
    return false;
}

void app_hfp_timeout_cb(uint8_t timer_id, uint16_t timer_chann)
{
    APP_PRINT_TRACE2("app_hfp_timeout_cb: timer_id %d, timer_chann %d", timer_id, timer_chann);

    switch (timer_id)
    {
    case APP_HFP_TIMER_AUTO_ANSWER_CALL:
        if (hfp_auto_answer_call != NULL)
        {
            T_APP_BR_LINK *p_link;
            p_link = &(app_db.br_link[timer_chann]);
            gap_stop_timer(&hfp_auto_answer_call);
            if (p_link->call_status == BT_HFP_INCOMING_CALL_ONGOING)
            {
                bt_hfp_call_answer_req(p_link->bd_addr);
            }
        }
        break;

    case APP_HFP_TIMER_RING:
        if (timer_handle_hfp_ring != NULL)
        {
            T_APP_BR_LINK *link1 = NULL;
            T_APP_BR_LINK *link2 = NULL;
            bool           ring_trigger = false;
            uint8_t        i;

            gap_stop_timer(&timer_handle_hfp_ring);

            for (i = 0; i < MAX_BR_LINK_NUM; i++)
            {
                if (app_db.br_link[i].call_status == BT_HFP_INCOMING_CALL_ONGOING)
                {
                    link1 = &(app_db.br_link[i]);

                    i++;
                    break;
                }
            }

            for (; i < MAX_BR_LINK_NUM; i++)
            {
                if (app_db.br_link[i].call_status == BT_HFP_INCOMING_CALL_ONGOING)
                {
                    link2 = &(app_db.br_link[i]);
                    break;
                }
            }

            if (link1 != NULL || link2 != NULL)
            {
                if (link1 != NULL)
                {
                    if (app_cfg_const.enable_multi_outband_call_tone)
                    {
                        if ((app_cfg_const.always_play_hf_incoming_tone_when_incoming_call) ||
                            (link1->is_inband_ring == false || link1->id != active_hf_index)) /* TODO check active sco link */
                        {
                            ring_trigger = true;
                        }
                    }
                    else
                    {
                        if ((app_cfg_const.always_play_hf_incoming_tone_when_incoming_call) ||
                            ((link1->is_inband_ring == false) &&
                             (link1->id == active_hf_index))) /* TODO check active sco link */
                        {
                            ring_trigger = true;
                        }
                    }
#if F_APP_SINGLE_MUTLILINK_SCENERIO_1
                    if (link1->id != app_teams_multilink_get_active_voice_index())
                    {
                        ring_trigger = true;
                    }
#endif
                }

                if (link2 != NULL)
                {
                    if (app_cfg_const.enable_multi_outband_call_tone)
                    {
                        if ((app_cfg_const.always_play_hf_incoming_tone_when_incoming_call) ||
                            (link2->is_inband_ring == false || link2->id != active_hf_index)) /* TODO check active sco link */
                        {
                            ring_trigger = true;
                        }
                    }
                    else
                    {
                        if ((app_cfg_const.always_play_hf_incoming_tone_when_incoming_call) ||
                            ((link2->is_inband_ring == false) &&
                             (link2->id == active_hf_index))) /* TODO check active sco link */
                        {
                            ring_trigger = true;
                        }
                    }
#if F_APP_SINGLE_MUTLILINK_SCENERIO_1
                    if (link2->id != app_teams_multilink_get_active_voice_index())
                    {
                        ring_trigger = true;
                    }
#endif
                }
            }

            if (ring_trigger == true)
            {
                if (remote_session_role_get() != REMOTE_SESSION_ROLE_SECONDARY)
                {
#if F_APP_HARMAN_FEATURE_SUPPORT
                    if (app_cfg_nv.language_status == 0)
                    {
                        app_audio_tone_type_play(TONE_BATTERY_PERCENTAGE_70, true, true);
                    }
                    else
#endif
                    {
                        app_audio_tone_type_play(TONE_HF_CALL_IN, true, true);
                    }
                    gap_start_timer(&timer_handle_hfp_ring, "hfp_ring", hfp_timer_queue_id,
                                    APP_HFP_TIMER_RING, timer_chann, app_cfg_const.timer_hfp_ring_period * 1000);
                }
                else
                {
                    hf_ring_active = false;
                }
            }
            else
            {
                hf_ring_active = false;
            }
#if F_APP_HARMAN_FEATURE_SUPPORT
            if (timer_handle_hfp_ring == NULL)
            {
#if HARMAN_SUPPORT_WATER_EJECTION
                app_harman_water_ejection_stop();
#endif
                app_harman_sco_status_notify();
            }
#endif
        }
        break;

    case APP_HFP_TIMER_MIC_MUTE_ALARM:
        if (timer_handle_mic_mute_alarm != NULL)
        {
            T_APP_BR_LINK *p_link;

            p_link = &(app_db.br_link[active_hf_index]);

            gap_stop_timer(&timer_handle_mic_mute_alarm);

            if (app_audio_is_mic_mute() &&
                ((p_link->call_status != BT_HFP_CALL_IDLE)
#if (F_APP_DONGLE_FEATURE_SUPPORT == 1)
                 || (app_db.dongle_is_enable_mic)
#endif
                ))
            {
                gap_start_timer(&timer_handle_mic_mute_alarm, "mic_mute_alarm",
                                hfp_timer_queue_id,
                                APP_HFP_TIMER_MIC_MUTE_ALARM, 0, app_cfg_const.timer_mic_mute_alarm * 1000);

                app_audio_tone_type_play(TONE_MIC_MUTE, false, true);
            }
        }
        break;

    case APP_HFP_TIMER_NO_SERVICE:
        if (timer_handle_hfp_no_service != NULL)
        {
            gap_start_timer(&timer_handle_hfp_no_service, "hfp_no_service",
                            hfp_timer_queue_id, APP_HFP_TIMER_NO_SERVICE,
                            timer_chann, app_cfg_const.timer_no_service_warning * 1000);

            if ((app_cfg_nv.bud_role == REMOTE_SESSION_ROLE_PRIMARY) ||
                (app_cfg_nv.bud_role == REMOTE_SESSION_ROLE_SINGLE))
            {
                app_audio_tone_type_play(TONE_HF_NO_SERVICE, false, true);
            }
        }
        break;

    case APP_HFP_TIMER_ADD_SCO:
        if (timer_handle_hfp_add_sco != NULL)
        {
            T_APP_BR_LINK *p_link;
            p_link = &(app_db.br_link[timer_chann]);

            gap_stop_timer(&timer_handle_hfp_add_sco);
            if (!app_hfp_sco_is_connected())
            {
                //rebulid sco
                app_bt_sniffing_hfp_connect(p_link->bd_addr);
            }
        }
        break;

    case APP_HFP_TIMER_CANCEL_VOICE_DAIL:
        if (timer_handle_iphone_voice_dail != NULL)
        {
            T_APP_BR_LINK *p_link;
            p_link = &(app_db.br_link[active_hf_index]);

            gap_stop_timer(&timer_handle_iphone_voice_dail);
            if ((p_link->call_status == BT_HFP_VOICE_ACTIVATION_ONGOING) &&
                (p_link->remote_device_vendor_id == APP_REMOTE_DEVICE_IOS) &&
                (app_hfp_sco_is_connected() == false))
            {
                bt_hfp_voice_recognition_disable_req(p_link->bd_addr);
            }
        }
        break;

    default:
        break;
    }
}

static void app_hfp_audio_cback(T_AUDIO_EVENT event_type, void *event_buf, uint16_t buf_len)
{
    T_AUDIO_EVENT_PARAM *param = event_buf;
    bool handle = true;

    switch (event_type)
    {
    case AUDIO_EVENT_TRACK_STATE_CHANGED:
        {
            T_AUDIO_STREAM_TYPE stream_type;

            if (audio_track_stream_type_get(param->track_state_changed.handle, &stream_type) == false)
            {
                break;
            }

            if (stream_type == AUDIO_STREAM_TYPE_VOICE)
            {
                if (param->track_state_changed.state == AUDIO_TRACK_STATE_STARTED ||
                    param->track_state_changed.state == AUDIO_TRACK_STATE_RESTARTED)
                {
                    if (app_hfp_get_call_status() >= BT_HFP_CALL_ACTIVE)
                    {
#if !GFPS_SASS_SUPPORT
                        audio_probe_dsp_ipc_send_call_status(true);
#endif
                    }
                }
#if F_APP_HARMAN_FEATURE_SUPPORT
                //refine sidetone setting
                T_AUDIO_TRACK_STATE state;
                T_APP_BR_LINK *p_link;

                p_link = app_find_br_link(app_db.br_link[app_hfp_get_active_hf_index()].bd_addr);
                audio_track_state_get(p_link->sco_track_handle, &state);
                T_APP_BR_LINK *check_p_link;
                check_p_link = &app_db.br_link[app_hfp_get_active_hf_index()];

                APP_PRINT_TRACE7("refine sidetone setting %d,%d,%d,%d, hfp_active=%d, call_status=%d,device_os=%d",
                                 app_cfg_nv.sidetone_switch,
                                 app_cfg_nv.sidetone_level,
                                 app_hfp_sco_is_connected(),
                                 param->track_state_changed.state,
                                 app_hfp_get_active_hf_index(),
                                 app_hfp_get_call_status(),
                                 check_p_link->remote_device_vendor_id);

                if (param->track_state_changed.state == AUDIO_TRACK_STATE_STARTED) //sco
                {
                    if ((app_hfp_get_call_status() == BT_HFP_VOICE_ACTIVATION_ONGOING)
                        //&&(check_p_link->remote_device_vendor_id == APP_REMOTE_DEVICE_IOS)
                       )
                    {
                        APP_PRINT_TRACE0("VA not open sidetone2");
                    }
                    else
                    {
                        if (app_cfg_nv.sidetone_switch)
                        {
                            harman_sidetone_set_dsp();
                        }
                        else
                        {
                            if (p_link->sidetone_instance != NULL)
                            {
                                sidetone_disable(p_link->sidetone_instance);
                                sidetone_release(p_link->sidetone_instance);
                                p_link->sidetone_instance = NULL;
                            }
                        }
                    }
                }
#endif
            }
        }
        break;

    default:
        handle = false;
        break;
    }

    if (handle == true)
    {
        APP_PRINT_INFO1("app_hfp_policy_cback: event_type 0x%04x", event_type);
    }
}

void app_hfp_init(void)
{
    if (app_cfg_const.supported_profile_mask & (HFP_PROFILE_MASK | HSP_PROFILE_MASK))
    {
        bt_hfp_init(app_cfg_const.hfp_link_number, RFC_HFP_CHANN_NUM,
                    RFC_HSP_CHANN_NUM, app_cfg_const.hfp_brsf_capability);
        audio_mgr_cback_register(app_hfp_audio_cback);
        bt_mgr_cback_register(app_hfp_bt_cback);
        app_relay_cback_register(NULL, app_hfp_parse_cback,
                                 APP_MODULE_TYPE_HFP, APP_REMOTE_HFP_TOTAL);
        gap_reg_timer_cb(app_hfp_timeout_cb, &hfp_timer_queue_id);
    }
}

void app_hfp_mute_ctrl(void)
{
    uint8_t hf_active = 0;
    T_APP_BR_LINK *p_link;

    p_link = &(app_db.br_link[active_hf_index]);

    if (p_link->app_hf_state == APP_HF_STATE_CONNECTED)
    {
        if (p_link->sco_handle != 0)
        {
            hf_active = 1;
        }
    }
    else
    {
        if (p_link->sco_handle)
        {
            hf_active = 1;
        }
    }

#if (F_APP_DONGLE_FEATURE_SUPPORT == 1)
    if (app_db.dongle_is_enable_mic)
    {
        hf_active = 1;
    }
#endif

    if ((app_cfg_nv.bud_role == REMOTE_SESSION_ROLE_PRIMARY) ||
        (app_cfg_nv.bud_role == REMOTE_SESSION_ROLE_SINGLE))
    {
        if (hf_active)
        {
            app_hfp_set_mic_mute();
            if (app_audio_is_mic_mute())
            {
#if (F_APP_TEAMS_FEATURE_SUPPORT || F_APP_CONFERENCE_HEADSET_SUPPORT)
                bt_hfp_microphone_gain_level_report(p_link->bd_addr, 0);
#endif
            }
            else
            {
#if (F_APP_TEAMS_FEATURE_SUPPORT || F_APP_CONFERENCE_HEADSET_SUPPORT)
                bt_hfp_microphone_gain_level_report(p_link->bd_addr, 0x0f);
#endif
            }
        }
    }
}

void app_hfp_stop_ring_alert_timer(void)
{
    gap_stop_timer(&timer_handle_hfp_ring);
#if F_APP_HARMAN_FEATURE_SUPPORT
#if HARMAN_SUPPORT_WATER_EJECTION
    app_harman_water_ejection_stop();
#endif
    app_harman_sco_status_notify();
#endif
}

uint8_t app_hfp_get_call_in_tone_type(T_APP_BR_LINK *p_link)
{
    T_APP_AUDIO_TONE_TYPE tone_type = TONE_TYPE_INVALID;

#if F_APP_HARMAN_FEATURE_SUPPORT
    if ((app_cfg_nv.language_status == 0) &&
        (app_cfg_const.tone_battery_percentage_70 != TONE_INVALID_INDEX))
    {
        tone_type = TONE_BATTERY_PERCENTAGE_70;
    }
    else if ((app_cfg_nv.language_status == 1) && (app_cfg_const.tone_hf_call_in != TONE_INVALID_INDEX))
    {
        tone_type = TONE_HF_CALL_IN;
    }
#else
    if (app_cfg_const.tone_hf_call_in != TONE_INVALID_INDEX)
    {
        tone_type = TONE_HF_CALL_IN;
    }
#endif
    else if ((p_link == &(app_db.br_link[app_db.first_hf_index])) &&
             (app_cfg_const.tone_first_hf_call_in != TONE_INVALID_INDEX))
    {
        tone_type = TONE_FIRST_HF_CALL_IN;
    }
    else if ((p_link == &(app_db.br_link[app_db.last_hf_index])) &&
             (app_cfg_const.tone_last_hf_call_in != TONE_INVALID_INDEX))
    {
        tone_type = TONE_LAST_HF_CALL_IN;
    }

    APP_PRINT_TRACE1("app_hfp_get_call_in_tone_type: tone_type 0x%02x", tone_type);

    return tone_type;
}

void app_hfp_adjust_sco_window(uint8_t *bd_addr, T_APP_HFP_SCO_ADJUST_EVT evt)
{
    uint8_t active_hf_addr[6];
    uint8_t need_set = 0;

    if ((evt != APP_SCO_ADJUST_LINKBACK_END_EVT) && (bd_addr == NULL))
    {
        return;
    }

    if (app_hfp_sco_is_connected())
    {
        if (app_db.br_link[active_hf_index].bd_addr != NULL)
        {
            memcpy(active_hf_addr, app_db.br_link[active_hf_index].bd_addr, 6);
        }

        if ((app_bt_policy_get_state() == BP_STATE_LINKBACK) && (memcmp(active_hf_addr, bd_addr, 6)))
        {
            //bud is paging
            if (memcmp(active_hf_addr, bd_addr, 6))
            {
                need_set = 1;
            }
        }
        else if (src_conn_ind_check(bd_addr) && memcmp(active_hf_addr, bd_addr, 6))
        {
            //src is connecting bud
            need_set = 1;
        }
        else if ((evt == APP_SCO_ADJUST_SCO_CONN_IND_EVT) && (app_find_sco_conn_num() == 1) &&
                 memcmp(active_hf_addr, bd_addr, 6))
        {
            //sco conn ind from inactive hf link
            need_set = 1;
        }
        else
        {
            need_set = 0;
        }

        APP_PRINT_TRACE4("app_hfp_adjust_sco_window bd_addr %s, active hf addr %s, evt 0x%04x, need_set %d",
                         TRACE_BDADDR(bd_addr), TRACE_BDADDR(active_hf_addr), evt, need_set);

        bt_sco_link_retrans_window_set(active_hf_addr, need_set);
    }
}

#if F_APP_HFP_CMD_SUPPORT
void app_hfp_cmd_handle(uint8_t *cmd_ptr, uint16_t cmd_len, uint8_t cmd_path, uint8_t app_idx,
                        uint8_t *ack_pkt)
{
    uint16_t cmd_id = (uint16_t)(cmd_ptr[0] | (cmd_ptr[1] << 8));
    T_APP_BR_LINK *p_link = &app_db.br_link[active_hf_index];

    switch (cmd_id)
    {
    case CMD_SEND_DTMF:
        {
            char number = (char)cmd_ptr[2];

            if (app_cfg_nv.bud_role == REMOTE_SESSION_ROLE_SECONDARY)
            {
                ack_pkt[2] = CMD_SET_STATUS_DISALLOW;
            }
            else
            {
                if ((p_link->app_hf_state == APP_HF_STATE_CONNECTED) &&
                    (app_hfp_get_call_status() >= BT_HFP_CALL_ACTIVE))
                {
                    if (bt_hfp_dtmf_code_transmit(p_link->bd_addr, number) == false)
                    {
                        ack_pkt[2] = CMD_SET_STATUS_PROCESS_FAIL;
                    }
                }
                else
                {
                    ack_pkt[2] = CMD_SET_STATUS_DISALLOW;
                }
            }
            app_report_event(cmd_path, EVENT_ACK, app_idx, ack_pkt, 3);

            if (ack_pkt[2] == CMD_SET_STATUS_COMPLETE)
            {
                uint8_t temp_buff[1];
                temp_buff[0] = cmd_ptr[2];

                app_report_event(cmd_path, EVENT_DTMF, app_idx, temp_buff, sizeof(temp_buff));
            }
        }
        break;

    case CMD_GET_SUBSCRIBER_NUM:
        {
            if (app_cfg_nv.bud_role == REMOTE_SESSION_ROLE_SECONDARY)
            {
                ack_pkt[2] = CMD_SET_STATUS_DISALLOW;
            }
            else
            {
                if ((p_link->app_hf_state == APP_HF_STATE_CONNECTED))
                {
                    if (bt_hfp_subscriber_number_query(p_link->bd_addr) == false)
                    {
                        ack_pkt[2] = CMD_SET_STATUS_PROCESS_FAIL;
                    }
                }
                else
                {
                    ack_pkt[2] = CMD_SET_STATUS_DISALLOW;
                }
            }
            app_report_event(cmd_path, EVENT_ACK, app_idx, ack_pkt, 3);
        }
        break;

    case CMD_GET_OPERATOR:
        {
            if (app_cfg_nv.bud_role == REMOTE_SESSION_ROLE_SECONDARY)
            {
                ack_pkt[2] = CMD_SET_STATUS_DISALLOW;
            }
            else
            {
                if ((p_link->app_hf_state == APP_HF_STATE_CONNECTED))
                {
                    if (bt_hfp_network_operator_name_query(p_link->bd_addr) == false)
                    {
                        ack_pkt[2] = CMD_SET_STATUS_PROCESS_FAIL;
                    }
                }
                else
                {
                    ack_pkt[2] = CMD_SET_STATUS_DISALLOW;
                }
            }
            app_report_event(cmd_path, EVENT_ACK, app_idx, ack_pkt, 3);
        }
        break;

    default:
        break;
    }
}
#endif

uint8_t app_hfp_get_org_hfp_idx(void)
{
    return org_hfp_idx;
}
