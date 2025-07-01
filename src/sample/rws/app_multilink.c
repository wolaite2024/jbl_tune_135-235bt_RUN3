/*
 * Copyright (c) 2018, Realsil Semiconductor Corporation. All rights reserved.
 */

#include <stdint.h>
#include <string.h>
#include "trace.h"
#include "gap_timer.h"
#include "gap_legacy.h"
#include "btm.h"
#include "bt_bond.h"
#include "remote.h"
#include "bt_a2dp.h"
#include "bt_avrcp.h"
#include "bt_hfp.h"
#include "app_a2dp.h"
#include "app_cfg.h"
#include "app_hfp.h"
#include "audio_track.h"
#include "app_link_util.h"
#include "app_main.h"
#include "app_roleswap.h"
#include "app_bond.h"
#include "app_listening_mode.h"
#include "app_multilink.h"
#include "app_audio_policy.h"
#include "app_bt_policy_int.h"
#include "app_bt_policy_api.h"
#include "app_bt_sniffing.h"
#include "audio_volume.h"
#include "audio_track.h"
#include "app_relay.h"
#include "app_iphone_abs_vol_handle.h"
#include "app_ble_gap.h"
#include "app_ota.h"
#include "app_gfps.h"
#include "app_gfps_rfc.h"
#if F_APP_LINEIN_SUPPORT
#include "app_line_in.h"
#endif

#if GFPS_SASS_SUPPORT
#include "app_sass_policy.h"
#endif

#if F_APP_SINGLE_MUTLILINK_SCENERIO_1
#include "app_single_multilink_customer.h"
#endif

#if F_APP_QOL_MONITOR_SUPPORT
#include "app_qol.h"
#endif
#include "app_mmi.h"
#include "bt_gfps.h"

#if F_APP_HARMAN_FEATURE_SUPPORT
#include <stdlib.h>
#include "transmit_service.h"
#include "app_ble_common_adv.h"
#include "gap_conn_le.h"
#include "app_harman_vendor_cmd.h"
#include "app_harman_ble.h"
#include "app_ble_service.h"
#endif

#define ERWS_MULTILINK_SUPPORT

static uint8_t multilink_silence_packet_idx = MAX_BR_LINK_NUM;
static uint8_t active_idx = MAX_BR_LINK_NUM;
static uint8_t active_a2dp_idx = 0;
static uint8_t multilink_timer_queue_id = 0;
static void *multilink_update_active_a2dp_index = NULL;
static void *multilink_pause_inactive_a2dp = NULL;
static void *multilink_wait_disconn_acl = NULL;
static bool sco_wait_from_sco_sniffing_stop = false;
static bool multilink_is_on_by_mmi = false;
static uint8_t pending_sco_index_from_sco = 0xFF;
static void *multilink_silence_packet_judge = NULL;
static void *multilink_silence_packet_timer = NULL;
static void *multilink_a2dp_stop_timer = NULL;
#if F_APP_HARMAN_FEATURE_SUPPORT
static void *multilink_notify_deviceinfo_timer = NULL;
uint8_t aling_active_a2dp_hfp_index = 0xff;
#endif
#if F_APP_MUTILINK_VA_PREEMPTIVE
static void *multilink_disc_va_sco = NULL;
static bool va_wait_from_va_sniffing_stop = false;
static uint8_t pending_va_index_from_va = 0xFF;
static uint8_t pending_va_sniffing_type = 0xFF;
static uint8_t active_va_idx = MAX_BR_LINK_NUM;
#endif
static bool stream_only[MAX_BR_LINK_NUM] = {false};
static uint8_t active_hfp_transfer = 0xFF;

void *timer_handle_harman_power_off_option = NULL;
static bool is_record_a2dp_active_ever = false;
static bool power_on_link_back_fg = false;
static bool is_harman_connect_only_one = false;
static uint8_t pending_id = 0xff;

static void app_multi_a2dp_stop(uint8_t id)
{
    T_APP_BR_LINK *p_link;
    p_link = app_find_br_link(app_db.br_link[id].bd_addr);
    if (p_link != NULL)
    {
        app_judge_active_a2dp_idx_and_qos(p_link->id, JUDGE_EVENT_A2DP_STOP);
    }

    if (app_cfg_const.rws_gaming_mode == 0)
    {
        if (app_multi_check_in_sniffing())
        {
            app_bt_sniffing_stop(p_link->bd_addr, BT_SNIFFING_TYPE_A2DP);
        }
        else if (p_link->id == active_idx)
        {
            app_handle_sniffing_link_disconnect_event(p_link->id);
        }
    }

    app_bt_policy_primary_engage_action_adjust();
}

void app_multi_active_hfp_transfer(uint8_t idx, bool disc, bool force)
{
    uint8_t cur_hfp = app_hfp_get_active_hf_index();
    if (idx != cur_hfp)
    {
        active_hfp_transfer = idx;
        if (force)
        {
            if (app_db.br_link[idx].call_status != BT_HFP_CALL_IDLE)
            {
                sco_wait_from_sco_sniffing_stop = true;
                pending_sco_index_from_sco = idx;
            }
            bt_hfp_audio_disconnect_req(app_db.br_link[cur_hfp].bd_addr);
        }
        else if ((app_db.br_link[cur_hfp].call_status == BT_HFP_INCOMING_CALL_ONGOING ||
                  app_db.br_link[cur_hfp].sco_handle == 0) && disc)
        {
            if (app_db.br_link[cur_hfp].bt_sniffing_state > APP_BT_SNIFFING_STATE_READY)
            {
                app_bt_sniffing_stop(app_db.br_link[cur_hfp].bd_addr, app_db.br_link[cur_hfp].sniffing_type);
            }
            else
            {
                app_handle_sniffing_link_disconnect_event(0xFF);
            }
        }
        else
        {
            if (app_db.br_link[idx].call_status != BT_HFP_CALL_IDLE &&
                app_db.br_link[idx].call_status != BT_HFP_INCOMING_CALL_ONGOING)
            {
                sco_wait_from_sco_sniffing_stop = true;
                pending_sco_index_from_sco = idx;
            }

            if (disc)
            {
#if (F_APP_HARMAN_FEATURE_SUPPORT == 0)
                bt_hfp_audio_disconnect_req(app_db.br_link[cur_hfp].bd_addr); //terminate call so no disc
#else
                if (app_db.br_link[cur_hfp].bt_sniffing_state > APP_BT_SNIFFING_STATE_READY)
                {
                    app_bt_sniffing_stop(app_db.br_link[cur_hfp].bd_addr, app_db.br_link[cur_hfp].sniffing_type);
                }
                else
                {
                    app_handle_sniffing_link_disconnect_event(0xFF);
                }
#endif
            }
        }
    }
}

bool app_multi_check_in_sniffing(void)
{
    uint8_t i;
    for (i = 0; i < MAX_BR_LINK_NUM; i++)
    {
        if (app_db.br_link[i].bt_sniffing_state > APP_BT_SNIFFING_STATE_READY)
        {
            return true;
        }
    }
    return false;
}

bool app_allow_a2dp_sniffing(uint8_t id)
{
//    if ((app_hfp_get_call_status() == BT_HFP_CALL_IDLE) ||
//        ((app_find_sco_conn_num() == 0) &&
//         (app_hfp_get_active_hf_index() == id)))
//    {
//        return true;
//    }
//    return false;
    return app_multilink_sass_bitmap(id, MULTI_A2DP_PREEM, true);
}

void app_multilink_stream_avrcp_set(uint8_t idx)
{
    active_a2dp_idx = idx;
    if (app_db.br_link[idx].avrcp_play_status != BT_AVRCP_PLAY_STATUS_PLAYING)
    {
        stream_only[idx] = true;
    }
    app_db.br_link[idx].avrcp_play_status = BT_AVRCP_PLAY_STATUS_PLAYING;
    app_a2dp_active_link_set(app_db.br_link[idx].bd_addr);
#if (F_APP_HARMAN_FEATURE_SUPPORT == 0)
    app_bond_set_priority(app_db.br_link[idx].bd_addr);
#endif
}

bool app_active_link_check(uint8_t *bd_addr)
{
    T_APP_BR_LINK *p_link = app_find_br_link(bd_addr);

    if (p_link)
    {
        APP_PRINT_TRACE4("app_active_link_check: p_link %x, streaming_fg %d, sco_handle %d, call status %d",
                         p_link, p_link->streaming_fg, p_link->sco_handle, app_hfp_get_call_status());
        if (p_link->streaming_fg || p_link->sco_handle != 0)
        {
            if (p_link->sco_handle != 0 && app_hfp_get_active_hf_index() == p_link->id)
            {
                return true;
            }

            if (app_allow_a2dp_sniffing(p_link->id))
            {
                if (p_link->streaming_fg && app_get_active_a2dp_idx() == p_link->id)
                {
                    return true;
                }
            }
        }
    }

    return false;
}

void app_multilink_disconnect_inactive_link(uint8_t app_idx)
{
    uint8_t call_num = 0;
    uint8_t i;

    uint8_t target_drop_index = MAX_BR_LINK_NUM;

    for (i = 0; i < MAX_BR_LINK_NUM; i++)
    {
        if (app_db.br_link[i].used == true &&
            app_db.br_link[i].call_status != BT_HFP_CALL_IDLE)
        {
            call_num++;
        }
    }

    APP_PRINT_TRACE4("app_multilink_disconnect_inactive_link: active_a2dp_idx %d, app_idx %d, "
                     "call_num %d, max_legacy_multilink_devices %d",
                     active_a2dp_idx, app_idx, call_num,
                     app_cfg_const.max_legacy_multilink_devices);

    target_drop_index = app_multilink_get_inactive_index(app_idx, call_num, false);

    if (call_num > 0)
    {
        if (app_cfg_const.enable_multi_link)
        {
            //inactive index may change according to switch preference

            if (target_drop_index != MAX_BR_LINK_NUM)
            {
#if GFPS_SASS_SUPPORT
                if (app_sass_policy_support_easy_connection_switch())
                {
                    app_bt_policy_disconnect(app_db.br_link[target_drop_index].bd_addr, ALL_PROFILE_MASK);
                }
                else
#endif
                {
                    if (call_num >= app_cfg_const.max_legacy_multilink_devices)
                    {
                        app_bt_policy_disconnect(app_db.br_link[app_idx].bd_addr, ALL_PROFILE_MASK);
                    }
                    else
                    {
                        app_bt_policy_disconnect(app_db.br_link[target_drop_index].bd_addr, ALL_PROFILE_MASK);
                    }
                }
            }

        }
        else
        {
#if GFPS_SASS_SUPPORT
            //single link disconnect active/new link directly
            if (app_sass_policy_support_easy_connection_switch())
            {
                app_bt_policy_disconnect(app_db.br_link[app_hfp_get_active_hf_index()].bd_addr, ALL_PROFILE_MASK);
            }
            else
#endif
            {
                app_bt_policy_disconnect(app_db.br_link[app_idx].bd_addr, ALL_PROFILE_MASK);
            }
        }
    }
    else
    {

        //disconnect one device if no bond and priority information when ACL connected
        if (target_drop_index != MAX_BR_LINK_NUM)
        {
            app_bt_policy_disconnect(app_db.br_link[target_drop_index].bd_addr, ALL_PROFILE_MASK);
        }
        else
        {
            for (i = 0; i < MAX_BR_LINK_NUM; i++)
            {
                if (app_check_b2s_link_by_id(i))
                {
                    if (i != app_idx)
                    {
                        app_bt_policy_disconnect(app_db.br_link[i].bd_addr, ALL_PROFILE_MASK);
                        break;
                    }
                }
            }
        }
    }
}

void app_disconnect_inactive_link(void)
{
    uint8_t call_num = 0;
    uint8_t i;
    uint8_t exit_cause = 0;

    app_db.disconnect_inactive_link_actively = true;

    for (i = 0; i < MAX_BR_LINK_NUM; i++)
    {
        if (app_db.br_link[i].used == true &&
            app_db.br_link[i].call_status != BT_HFP_CALL_IDLE)
        {
            call_num++;
        }
    }

    app_db.connected_num_before_roleswap = app_bt_policy_get_b2s_connected_num();

    if (call_num > 0)
    {
        for (i = 0; i < MAX_BR_LINK_NUM; i++)
        {
            if (app_check_b2s_link_by_id(i))
            {
                if (i != app_hfp_get_active_hf_index())
                {
                    memcpy(app_db.resume_addr, app_db.br_link[i].bd_addr, 6);

                    if (!app_db.br_link[i].connected_profile)
                    {
                        //If no profile connected before disconnecting acl, connected tone need.
                        app_audio_set_connected_tone_need(true);
                    }

                    app_bt_policy_disconnect(app_db.br_link[i].bd_addr, ALL_PROFILE_MASK);
                    gap_start_timer(&multilink_wait_disconn_acl, "wait_disconn_acl", multilink_timer_queue_id,
                                    MULTILINK_DISCONN_INACTIVE_ACL, i, TIMER_TO_DISCONN_ACL);
                    exit_cause = 1;
                    goto exit;
                }
            }
        }
    }
    else
    {
        for (i = 0; i < MAX_BR_LINK_NUM; i++)
        {
            if (app_check_b2s_link_by_id(i))
            {
                if (i != app_get_active_a2dp_idx())
                {
                    memcpy(app_db.resume_addr, app_db.br_link[i].bd_addr, 6);

                    if (!app_db.br_link[i].connected_profile)
                    {
                        //If no profile connected before disconnecting acl, connected tone need.
                        app_audio_set_connected_tone_need(true);
                    }

                    app_bt_policy_disconnect(app_db.br_link[i].bd_addr, ALL_PROFILE_MASK);
                    gap_start_timer(&multilink_wait_disconn_acl, "wait_disconn_acl", multilink_timer_queue_id,
                                    MULTILINK_DISCONN_INACTIVE_ACL, i, TIMER_TO_DISCONN_ACL);
                    exit_cause = 2;
                    goto exit;
                }
            }
        }
    }

exit:
    APP_PRINT_TRACE7("app_disconnect_inactive_link: active_a2dp_idx %d, "
                     "call_num %d, active_hf_index %d, connected_num_before_roleswap %d, resume_addr %s exit_cause %d connected_tone_need %d",
                     active_a2dp_idx, call_num, app_hfp_get_active_hf_index(),
                     app_db.connected_num_before_roleswap, TRACE_BDADDR(app_db.resume_addr), exit_cause,
                     app_db.connected_tone_need);
}

void app_reconnect_inactive_link()
{
    uint8_t b2s_connected_num = app_bt_policy_get_b2s_connected_num();

    APP_PRINT_TRACE3("app_reconnect_inactive_link: connected_num_before_roleswap %d, resume_addr %s b2s_num %d",
                     app_db.connected_num_before_roleswap, TRACE_BDADDR(app_db.resume_addr), b2s_connected_num);

    if (app_cfg_nv.bud_role == REMOTE_SESSION_ROLE_PRIMARY &&
        app_db.connected_num_before_roleswap > app_bt_policy_get_b2s_connected_num())
    {
        uint32_t bond_flag;
        uint32_t plan_profs;

        app_db.connected_num_before_roleswap = 0;
        bt_bond_flag_get(app_db.resume_addr, &bond_flag);
        if (bond_flag & (BOND_FLAG_HFP | BOND_FLAG_HSP | BOND_FLAG_A2DP))
        {
            plan_profs = get_profs_by_bond_flag(bond_flag);
            app_bt_policy_default_connect(app_db.resume_addr, plan_profs, false);
        }
    }
}

void app_multilink_stop_acl_disconn_timer(void)
{
    gap_stop_timer(&multilink_wait_disconn_acl);
}

void app_multilink_switch_by_mmi(bool is_on_by_mmi)
{
    multilink_is_on_by_mmi = is_on_by_mmi;
}

bool app_multilink_is_on_by_mmi(void)
{
    return multilink_is_on_by_mmi;
}

#if F_APP_SINGLE_MUTLILINK_SCENERIO_1
#else
static uint8_t app_find_new_active_a2dp_link_by_priority(uint8_t curr_idx)
{
    uint8_t priority;
    uint8_t bond_num;
    uint8_t bd_addr[6];
    T_APP_BR_LINK *p_link = NULL;
    uint8_t active_idx = 0;

    bond_num = app_b2s_bond_num_get();

    for (priority = 1; priority <= bond_num; priority++)
    {
        if (app_b2s_bond_addr_get(priority, bd_addr) == true)
        {
            p_link = app_find_b2s_link(bd_addr);
            if (p_link != NULL)
            {
                if (memcmp(p_link->bd_addr, app_db.br_link[curr_idx].bd_addr, 6))
                {
                    active_idx = p_link->id;
                    break;
                }
            }
        }
    }

    APP_PRINT_TRACE2("app_find_new_active_a2dp_link_by_priority: curr_idx %d, active_idx %d",
                     curr_idx, active_idx);

    return active_idx;
}
#endif

uint8_t app_get_active_a2dp_idx(void)
{
    return active_a2dp_idx;
}

void app_set_active_a2dp_idx(uint8_t idx)
{
    active_a2dp_idx = idx;
    if ((app_cfg_const.enable_multi_link) &&
        (app_find_b2s_link_num() == MULTILINK_SRC_CONNECTED) &&
        (app_db.br_link[active_a2dp_idx].harman_silent_check == true))
    {
        APP_PRINT_TRACE1("silent_dont_update1 %d", active_idx);
    }
    else
    {
        au_set_harman_aling_active_idx(active_a2dp_idx);
    }
    APP_PRINT_TRACE1("app_set_active_a2dp_idx to %d", active_a2dp_idx);
}

bool app_a2dp_active_link_set(uint8_t *bd_addr)
{
    T_APP_BR_LINK *p_link;
    p_link = app_find_br_link(bd_addr);
    if (p_link)
    {
        active_a2dp_idx = p_link->id;
        if ((app_cfg_const.enable_multi_link) &&
            (app_find_b2s_link_num() == MULTILINK_SRC_CONNECTED) &&
            (app_db.br_link[active_a2dp_idx].harman_silent_check == true))
        {
            APP_PRINT_TRACE1("silent_dont_update2 %d", active_idx);
        }
        else
        {
            au_set_harman_aling_active_idx(active_a2dp_idx);
        }
#if F_APP_MUTILINK_VA_PREEMPTIVE
        if (app_cfg_const.enable_multi_link)
        {
            uint8_t pair_idx_mapping;
            app_bond_get_pair_idx_mapping(bd_addr, &pair_idx_mapping);
#if F_APP_IPHONE_ABS_VOL_HANDLE
            app_iphone_abs_vol_wrap_audio_track_volume_out_set(
                app_db.br_link[active_a2dp_idx].a2dp_track_handle,
                app_cfg_nv.audio_gain_level[pair_idx_mapping],
                app_cfg_nv.abs_vol[pair_idx_mapping]);
#else
            audio_track_volume_out_set(app_db.br_link[active_a2dp_idx].a2dp_track_handle,
                                       app_cfg_nv.audio_gain_level[pair_idx_mapping]);
#endif
            app_audio_track_spk_unmute(AUDIO_STREAM_TYPE_PLAYBACK);
            if (app_db.remote_session_state == REMOTE_SESSION_STATE_CONNECTED)
            {
                uint8_t cmd_ptr[9] = {0};
                memcpy(&cmd_ptr[0], app_db.br_link[active_a2dp_idx].bd_addr, 6);
                cmd_ptr[6] = AUDIO_STREAM_TYPE_PLAYBACK;
                cmd_ptr[7] = app_cfg_nv.audio_gain_level[pair_idx_mapping];
                cmd_ptr[8] = 0;
                app_relay_sync_single_with_raw_msg(APP_MODULE_TYPE_AUDIO_POLICY, APP_REMOTE_MSG_A2DP_VOLUME_SYNC,
                                                   cmd_ptr, 9, REMOTE_TIMER_HIGH_PRECISION, 0, false);
            }
        }
#endif
#if F_APP_LOCAL_PLAYBACK_SUPPORT
        if ((app_db.sd_playback_switch == 0) && (p_link->a2dp_track_handle))
#else
        if (p_link->a2dp_track_handle)
#endif
        {
#if F_APP_LINEIN_SUPPORT
            if (app_cfg_const.line_in_support)
            {
                if (app_line_in_start_a2dp_check())
                {
                    audio_track_start(p_link->a2dp_track_handle);
                }
            }
            else
#endif
            {
                audio_track_start(p_link->a2dp_track_handle);
            }
        }

        app_audio_set_avrcp_status(p_link->avrcp_play_status);
        app_avrcp_sync_status();

        return bt_a2dp_active_link_set(bd_addr);
    }
    return false;
}

//multi-link a2dp
bool app_pause_inactive_a2dp_link_stream(uint8_t keep_active_a2dp_idx, uint8_t resume_fg)
{
    uint8_t i;
    bool play_pause = false;

    for (i = 0; i < MAX_BR_LINK_NUM; i++)
    {
        if ((i != keep_active_a2dp_idx) &&
            (app_db.br_link[i].connected_profile & (A2DP_PROFILE_MASK | AVRCP_PROFILE_MASK)))
        {
            if (app_db.br_link[i].streaming_fg == true)
            {
                if (resume_fg && app_db.br_link[i].avrcp_play_status == BT_AVRCP_PLAY_STATUS_PLAYING &&
                    !stream_only[i])
                {
                    app_db.wait_resume_a2dp_idx = i;
                }

                if (app_db.br_link[i].avrcp_play_status == BT_AVRCP_PLAY_STATUS_PLAYING &&
                    !stream_only[i])
                {
                    bt_avrcp_pause(app_db.br_link[i].bd_addr);
                }
                audio_track_stop(app_db.br_link[i].a2dp_track_handle);
                app_db.br_link[i].avrcp_play_status = BT_AVRCP_PLAY_STATUS_PAUSED;
                play_pause = true;
                // Stop Sniffing Now
                app_bt_sniffing_stop(app_db.br_link[i].bd_addr, BT_SNIFFING_TYPE_A2DP);
            }
        }
    }
    APP_PRINT_TRACE5("app_pause_inactive_a2dp_link_stream: active_a2dp_idx %d, wait_resume_a2dp_idx %d, "
                     "i %d, keep_active_a2dp_idx %d, play_pause %d",
                     active_a2dp_idx, app_db.wait_resume_a2dp_idx, i, keep_active_a2dp_idx, play_pause);
    return play_pause;
}

#if F_APP_SINGLE_MUTLILINK_SCENERIO_1
#else
void app_resume_a2dp_link_stream(uint8_t app_idx)
{
    if (app_db.wait_resume_a2dp_idx < MAX_BR_LINK_NUM)
    {
        //if (app_db.wait_resume_a2dp_idx != app_idx)
        {
            if (app_db.br_link[app_db.wait_resume_a2dp_idx].connected_profile &
                (A2DP_PROFILE_MASK | AVRCP_PROFILE_MASK))
            {
                app_a2dp_active_link_set(app_db.br_link[app_db.wait_resume_a2dp_idx].bd_addr);
#if (F_APP_HARMAN_FEATURE_SUPPORT == 0)
                app_bond_set_priority(app_db.br_link[app_db.wait_resume_a2dp_idx].bd_addr);
#endif
                if (app_db.br_link[app_db.wait_resume_a2dp_idx].avrcp_play_status !=
                    BT_AVRCP_PLAY_STATUS_PLAYING)
                {
                    app_db.br_link[app_db.wait_resume_a2dp_idx].avrcp_play_status =
                        BT_AVRCP_PLAY_STATUS_PLAYING;
                    bt_avrcp_play(app_db.br_link[app_db.wait_resume_a2dp_idx].bd_addr);
                }
            }
        }
        //all sco are removed, the variable must be clear
        app_db.wait_resume_a2dp_idx = MAX_BR_LINK_NUM;
    }
    APP_PRINT_TRACE4("app_resume_a2dp_link_stream: active_a2dp_idx %d, wait_resume_a2dp_idx %d, "
                     "app_idx %d, avrcp_play_status 0x%02x",
                     active_a2dp_idx, app_db.wait_resume_a2dp_idx, app_idx,
                     app_db.br_link[active_a2dp_idx].avrcp_play_status);
}
#endif

void app_handle_sniffing_link_disconnect_event(uint8_t id)
{
    APP_PRINT_TRACE7("app_handle_sniffing_link_disconnect_event:  %d, %d, %d, %d, %d, %d, %d",
                     sco_wait_from_sco_sniffing_stop, pending_sco_index_from_sco,
                     app_db.sco_wait_a2dp_sniffing_stop, app_db.pending_sco_idx,
                     app_db.a2dp_wait_a2dp_sniffing_stop, app_db.pending_a2dp_idx,
                     active_hfp_transfer);

    if (active_hfp_transfer != 0xFF)
    {
#if (F_APP_HARMAN_FEATURE_SUPPORT == 0)
        app_bond_set_priority(app_db.br_link[active_hfp_transfer].bd_addr);
#endif
        if (app_db.br_link[active_hfp_transfer].sco_handle)
        {
            app_multilink_set_active_idx(active_hfp_transfer);
            if (!app_bt_sniffing_start(app_db.br_link[active_hfp_transfer].bd_addr,
                                       BT_SNIFFING_TYPE_SCO))
            {

            }
        }
        else if (app_db.br_link[active_hfp_transfer].streaming_fg) //hfp data go a2dp
        {
            app_multilink_set_active_idx(active_hfp_transfer);
            if (app_bt_sniffing_start(app_db.br_link[active_hfp_transfer].bd_addr,
                                      BT_SNIFFING_TYPE_A2DP)) {}
        }
        else if (sco_wait_from_sco_sniffing_stop == true)
        {
            app_multilink_set_active_idx(pending_sco_index_from_sco);
            app_bt_sniffing_hfp_connect(app_db.br_link[pending_sco_index_from_sco].bd_addr);
        }

        if (app_db.br_link[active_hfp_transfer].call_status != BT_HFP_CALL_IDLE)
        {
            app_hfp_set_active_hf_index(app_db.br_link[active_hfp_transfer].bd_addr);
        }

        uint8_t inactive_hf_index;

        sco_wait_from_sco_sniffing_stop = false;
        pending_sco_index_from_sco = 0xFF;

        for (inactive_hf_index = 0; inactive_hf_index < MAX_BR_LINK_NUM; inactive_hf_index++)
        {
            if (inactive_hf_index != active_hfp_transfer)
            {
                if (app_db.br_link[inactive_hf_index].call_status >= BT_HFP_CALL_ACTIVE &&
                    app_db.br_link[inactive_hf_index].streaming_fg == false)
                {
                    sco_wait_from_sco_sniffing_stop = true;
                    pending_sco_index_from_sco = inactive_hf_index;
                    break;
                }
            }
        }
        active_hfp_transfer = 0xFF;
    }
    else if (sco_wait_from_sco_sniffing_stop == true && id != pending_sco_index_from_sco)
    {
        app_multilink_set_active_idx(pending_sco_index_from_sco);
        app_bt_sniffing_hfp_connect(app_db.br_link[pending_sco_index_from_sco].bd_addr);
        sco_wait_from_sco_sniffing_stop = false;
        pending_sco_index_from_sco = 0xFF;
        active_hfp_transfer = 0xFF;
    }
    else if (app_db.sco_wait_a2dp_sniffing_stop == true)
    {
        app_multilink_set_active_idx(app_db.pending_sco_idx);
        if (!app_bt_sniffing_start(app_db.br_link[app_db.pending_sco_idx].bd_addr, BT_SNIFFING_TYPE_SCO))
        {
            bt_sco_conn_cfm(app_db.br_link[app_db.pending_sco_idx].bd_addr, true);
        }
        app_db.sco_wait_a2dp_sniffing_stop = false;
    }
//    else if (app_db.a2dp_wait_a2dp_sniffing_stop == true)
//    {
//        app_multilink_set_active_idx(app_db.pending_a2dp_idx);
//        if (app_bt_sniffing_start(app_db.br_link[app_db.pending_a2dp_idx].bd_addr,
//                                  BT_SNIFFING_TYPE_A2DP)) {};
//        app_db.a2dp_wait_a2dp_sniffing_stop = false;

//        app_audio_a2dp_stream_start_event_handle();
//        app_relay_async_single(APP_MODULE_TYPE_AUDIO_POLICY, APP_REMOTE_MSG_A2DP_STREAM_START);
//    }
#if F_APP_MUTILINK_VA_PREEMPTIVE
    else if (va_wait_from_va_sniffing_stop == true &&
             (pending_va_sniffing_type == BT_SNIFFING_TYPE_SCO ||
              pending_va_sniffing_type == BT_SNIFFING_TYPE_A2DP))
    {
        APP_PRINT_TRACE2("app_handle_sniffing_link_disconnect_event: va_wait_va %d %d",
                         pending_va_index_from_va,
                         pending_va_sniffing_type);
        app_multilink_set_active_idx(pending_va_index_from_va);
        if (pending_va_sniffing_type == BT_SNIFFING_TYPE_SCO)
        {
            app_hfp_set_active_hf_index(app_db.br_link[app_hfp_get_active_hf_index()].bd_addr);
            if (!app_bt_sniffing_start(app_db.br_link[pending_va_index_from_va].bd_addr,
                                       BT_SNIFFING_TYPE_SCO))
            {

            }
        }
        else if (pending_va_sniffing_type == BT_SNIFFING_TYPE_A2DP)
        {
            if (app_bt_sniffing_start(app_db.br_link[pending_va_index_from_va].bd_addr,
                                      BT_SNIFFING_TYPE_A2DP)) {}

            app_audio_a2dp_stream_start_event_handle();
            app_relay_async_single(APP_MODULE_TYPE_AUDIO_POLICY, APP_REMOTE_MSG_A2DP_STREAM_START);
        }
        va_wait_from_va_sniffing_stop = false;
        pending_va_index_from_va = 0xFF;
        pending_va_sniffing_type = 0xFF;
    }
#endif
    else
    {
        // Check what ???
        // will cause single link problem: https://jira.realtek.com/browse/BBPRO2BUG-3827
        APP_PRINT_TRACE2("app_handle_sniffing_link_disconnect_event hfp_active_idx:%d, soc_num:%d",
                         app_hfp_get_active_hf_index(),
                         app_find_sco_conn_num());
        if ((app_db.br_link[app_hfp_get_active_hf_index()].sco_handle != NULL) &&
            ((app_cfg_const.enable_rtk_charging_box && !app_db.remote_case_closed) ||
             (!app_cfg_const.enable_rtk_charging_box && app_db.remote_loc != BUD_LOC_IN_CASE)))
        {
            app_multilink_set_active_idx(app_hfp_get_active_hf_index());
            app_hfp_set_active_hf_index(app_db.br_link[app_hfp_get_active_hf_index()].bd_addr);
            if (!app_bt_sniffing_start(app_db.br_link[app_hfp_get_active_hf_index()].bd_addr,
                                       BT_SNIFFING_TYPE_SCO))
            {
                APP_PRINT_TRACE0("app_handle_sniffing_link_disconnect_event: app_bt_sniffing_start error");
            }
        }
        else
        {
            app_judge_active_a2dp_idx_and_qos(active_a2dp_idx, JUDGE_EVENT_SNIFFING_STOP);
        }
        if (sco_wait_from_sco_sniffing_stop == true && id != pending_sco_index_from_sco)
        {
            sco_wait_from_sco_sniffing_stop = false;
            pending_sco_index_from_sco = 0xFF;
        }
#if F_APP_MUTILINK_VA_PREEMPTIVE
        va_wait_from_va_sniffing_stop = false;
        pending_va_index_from_va = 0xFF;
        pending_va_sniffing_type = 0xFF;
#endif
    }
#if GFPS_SASS_SUPPORT
    if (app_sass_policy_support_easy_connection_switch())
    {
        app_gfps_notify_conn_status();
    }
#endif
}

bool app_multilink_check_pend(void)
{
    if (app_db.br_link[active_a2dp_idx].streaming_fg == false &&
        app_db.br_link[active_a2dp_idx].bt_sniffing_state == APP_BT_SNIFFING_STATE_STARTING)
    {
        return false;
    }
    return true;
}

void app_judge_active_a2dp_idx_and_qos(uint8_t app_idx, T_APP_JUDGE_A2DP_EVENT event)
{
#if F_APP_SINGLE_MUTLILINK_SCENERIO_1
    return app_teams_multilink_music_priority_rearrangment(app_idx, event);
#else
    APP_PRINT_TRACE7("app_judge_active_a2dp_idx_and_qos: 1 event %d, active_a2dp_idx %d, app_idx %d, "
                     "wait_resume_a2dp_idx %d, update_active_a2dp_idx %d, streaming_fg %d, active_media_paused %d",
                     event, active_a2dp_idx, app_idx, app_db.wait_resume_a2dp_idx, app_db.update_active_a2dp_idx,
                     app_db.br_link[active_a2dp_idx].streaming_fg, app_db.active_media_paused);
    switch (event)
    {
    case JUDGE_EVENT_A2DP_CONNECTED:
        {
            uint8_t link_number = app_connected_profile_link_num(A2DP_PROFILE_MASK);
            if (link_number <= 1)
            {
                app_a2dp_active_link_set(app_db.br_link[app_idx].bd_addr);
                app_bond_set_priority(app_db.br_link[app_idx].bd_addr);
                if (link_number <= 0)
                {
                    //exception
                }
            }
            else
            {
                if ((app_db.br_link[active_a2dp_idx].streaming_fg == false) &&
                    (app_hfp_get_call_status() == BT_HFP_CALL_IDLE))
                {
                    if (app_cfg_const.enable_multi_link)
                    {
#if F_APP_HARMAN_FEATURE_SUPPORT
                        app_bond_set_priority(app_db.br_link[app_find_new_active_a2dp_link_by_priority(app_idx)].bd_addr);
                        app_bond_set_priority(app_db.br_link[app_idx].bd_addr);
                        if (au_get_record_a2dp_active_ever() != true)
                        {
                            app_a2dp_active_link_set(app_db.br_link[app_idx].bd_addr);
                        }
#else
                        app_bond_set_priority(app_db.br_link[app_idx].bd_addr);
                        app_bond_set_priority(app_db.br_link[app_find_new_active_a2dp_link_by_priority(app_idx)].bd_addr);
#endif
                    }
                    else
                    {
                        app_multilink_set_active_idx(app_idx);
                        app_a2dp_active_link_set(app_db.br_link[app_idx].bd_addr);
                        app_bond_set_priority(app_db.br_link[app_idx].bd_addr);
                    }
                }
            }
        }
        break;

    case JUDGE_EVENT_A2DP_START:
        {
            APP_PRINT_TRACE3("JUDGE_EVENT_A2DP_START: active_a2dp_idx %d, avrcp %d, stream %d",
                             active_a2dp_idx,
                             app_db.br_link[active_a2dp_idx].avrcp_play_status,
                             app_db.br_link[active_a2dp_idx].streaming_fg);

            if (app_multilink_preemptive_judge(app_idx, MULTI_A2DP_PREEM))
            {
            }

            app_bt_policy_qos_param_update(app_db.br_link[app_idx].bd_addr, BP_TPOLL_A2DP_PLAY_EVENT);
#if F_APP_HARMAN_FEATURE_SUPPORT
#if HARMAN_SUPPORT_WATER_EJECTION
            app_harman_water_ejection_stop();
#endif
            au_connect_idle_to_power_off(ACTIVE_NEED_STOP_COUNT, app_idx);
#endif
        }
        break;

    case JUDGE_EVENT_A2DP_DISC:
        {
            if (active_a2dp_idx == app_idx)
            {
                if (app_multi_check_in_sniffing())
                {
                    app_bt_sniffing_stop(app_db.br_link[app_idx].bd_addr, BT_SNIFFING_TYPE_A2DP);
                }
                else if (app_idx == active_idx)
                {
                    app_handle_sniffing_link_disconnect_event(app_idx);
                }

                if (app_connected_profile_link_num(A2DP_PROFILE_MASK))
                {
                    app_a2dp_active_link_set(app_db.br_link[app_find_new_active_a2dp_link_by_priority(
                                                                app_idx)].bd_addr);
#if (F_APP_HARMAN_FEATURE_SUPPORT == 0)
                    app_bond_set_priority(app_db.br_link[app_find_new_active_a2dp_link_by_priority(app_idx)].bd_addr);
#endif
                }
#if F_APP_HARMAN_FEATURE_SUPPORT
                au_set_record_a2dp_active_ever(false);
#endif
            }
            app_bt_policy_qos_param_update(app_db.br_link[app_idx].bd_addr, BP_TPOLL_A2DP_STOP_EVENT);
        }
        break;

    case JUDGE_EVENT_A2DP_STOP:
        {
            if (app_db.update_active_a2dp_idx == false)
            {
                app_db.br_link[app_idx].avrcp_play_status = BT_AVRCP_PLAY_STATUS_PAUSED;
                stream_only[app_idx] = false;
#if F_APP_MUTILINK_VA_PREEMPTIVE
                if (va_wait_from_va_sniffing_stop && pending_va_index_from_va == app_idx)
                {
                    va_wait_from_va_sniffing_stop = false;
                    pending_va_index_from_va = 0xFF;
                    pending_va_sniffing_type = 0xFF;
                }
#endif
                if ((active_a2dp_idx == app_idx) && (app_db.wait_resume_a2dp_idx == MAX_BR_LINK_NUM))
                {
                    gap_stop_timer(&multilink_silence_packet_judge);
                    gap_stop_timer(&multilink_silence_packet_timer);
                    uint8_t i;
                    for (i = 0; i < MAX_BR_LINK_NUM; i++)
                    {
                        uint8_t idx = app_find_new_active_a2dp_link_by_priority(active_a2dp_idx);
                        if (app_db.br_link[idx].connected_profile &
                            (A2DP_PROFILE_MASK | AVRCP_PROFILE_MASK))
                        {
                            if ((idx != active_a2dp_idx) &&
                                (app_db.br_link[idx].streaming_fg == true))
                            {
                                APP_PRINT_TRACE2("JUDGE_EVENT_A2DP_STOP: active_a2dp_idx %d, idx %d", active_a2dp_idx, idx);
                                app_multilink_stream_avrcp_set(idx);
                                break;
                            }
                        }
                    }
                }
                /*  a2dp pause to resume
                else if ((active_a2dp_idx == app_idx) && (app_db.wait_resume_a2dp_idx != active_a2dp_idx))
                {
                    app_a2dp_active_link_set(app_db.br_link[app_db.wait_resume_a2dp_idx].bd_addr);
                    APP_PRINT_TRACE2("JUDGE_EVENT_A2DP_STOP: active_a2dp_idx %d, wait_resume_a2dp_idx %d",
                                     active_a2dp_idx, app_db.wait_resume_a2dp_idx);
                    app_db.active_media_paused = false;
                    active_a2dp_idx = app_db.wait_resume_a2dp_idx;
                    app_resume_a2dp_link_stream(app_idx);
                    app_bond_set_priority(app_db.br_link[app_db.wait_resume_a2dp_idx].bd_addr);
                }*/
            }

            if (app_cfg_nv.bud_role == REMOTE_SESSION_ROLE_PRIMARY)
            {
                //Clear a2dp preemptive flag if any link is disconnected during preemptive process.
                app_db.a2dp_preemptive_flag = false;

#if F_APP_ERWS_SUPPORT
                if (app_idx != app_get_active_a2dp_idx() &&
                    (app_db.remote_session_state == REMOTE_SESSION_STATE_CONNECTED))
                {
                    //preemp ends, htpoll param return to normal value
                    app_bt_sniffing_param_update(APP_BT_SNIFFING_EVENT_MULTILINK_CHANGE);
                    app_bt_sniffing_judge_dynamic_set_gaming_mode();
                }

#if F_APP_QOL_MONITOR_SUPPORT
                if (app_a2dp_is_streaming() == false)
                {
                    //close b2b link monitor when no a2dp stream
                    app_qol_link_monitor(app_cfg_nv.bud_peer_addr, false);
                }
#endif
#endif
            }
            app_bt_policy_qos_param_update(app_db.br_link[app_idx].bd_addr, BP_TPOLL_A2DP_STOP_EVENT);

#if F_APP_HARMAN_FEATURE_SUPPORT
            au_multilink_harman_check_silent(app_idx, 0xff);

            if ((app_hfp_sco_is_connected() == false && app_hfp_get_call_status() == BT_HFP_CALL_IDLE)
                && (app_db.br_link[app_get_active_a2dp_idx()].streaming_fg == false))
            {
                au_connect_idle_to_power_off(CONNECT_IDLE_POWER_OFF_START, app_idx);
            }
#endif
        }
        break;

    case JUDGE_EVENT_MEDIAPLAYER_PLAYING:
        {
            app_multilink_preemptive_judge(app_idx, MULTI_AVRCP_PREEM);
#if HARMAN_SUPPORT_WATER_EJECTION
            if ((active_a2dp_idx == app_idx) && app_db.br_link[active_a2dp_idx].streaming_fg)
            {
                app_harman_water_ejection_stop();
            }
#endif
            app_bt_policy_qos_param_update(app_db.br_link[app_idx].bd_addr, BP_TPOLL_A2DP_PLAY_EVENT);
        }
        break;

    case JUDGE_EVENT_MEDIAPLAYER_PAUSED:
        {
            //if (app_allow_a2dp_sniffing(app_idx))
            {
                APP_PRINT_TRACE4("JUDGE_EVENT_MEDIAPLAYER_PAUSED: active_a2dp_idx %d, app_idx %d, "
                                 "streaming_fg %d, harman_silent_check: %d",
                                 active_a2dp_idx, app_idx, app_db.br_link[active_a2dp_idx].streaming_fg,
                                 app_db.br_link[active_a2dp_idx].harman_silent_check);
                if (active_a2dp_idx == app_idx)
                {
                    app_db.active_media_paused = true;
                    ///TODO: mute Music ???
                    // if (app_db.br_link[active_a2dp_idx].streaming_fg == true)
                    // {
                    //     app_db.br_link[active_a2dp_idx].streaming_fg = false;
                    //     app_bt_sniffing_stop(app_db.br_link[active_a2dp_idx].bd_addr, BT_SNIFFING_TYPE_A2DP);
                    // }
                    if (app_db.wait_resume_a2dp_idx == MAX_BR_LINK_NUM)
                    {
                        uint8_t idx = app_find_new_active_a2dp_link_by_priority(active_a2dp_idx);

                        if ((app_db.br_link[idx].connected_profile & (A2DP_PROFILE_MASK | AVRCP_PROFILE_MASK))
                            && (idx != active_a2dp_idx)
#if F_APP_HARMAN_FEATURE_SUPPORT
                            && (app_db.br_link[active_a2dp_idx].harman_silent_check)
#endif
                           )
                        {
                            multilink_silence_packet_idx = app_idx;
#if F_APP_HARMAN_FEATURE_SUPPORT
                            gap_start_timer(&multilink_silence_packet_judge, "multilink_silence_packet_judge",
                                            multilink_timer_queue_id, MULTILINK_SILENCE_PACKET_JUDGE,
                                            idx, 5000);
#else
                            gap_start_timer(&multilink_silence_packet_judge, "multilink_silence_packet_judge",
                                            multilink_timer_queue_id, MULTILINK_SILENCE_PACKET_JUDGE,
                                            idx, 750);
#endif
                            break;
                        }
                    }
                }
            }
        }
        break;

    case JUDGE_EVENT_SNIFFING_STOP:
        {
            ///TODO: check Sniffing state for active link
            APP_PRINT_TRACE4("JUDGE_EVENT_SNIFFING_STOP: active_a2dp_idx %d, bd_addr %s, stream %d, remote_loc %d",
                             active_a2dp_idx,
                             TRACE_BDADDR(app_db.br_link[active_a2dp_idx].bd_addr),
                             app_db.br_link[active_a2dp_idx].streaming_fg,
                             app_db.remote_loc);
            uint8_t inactive_a2dp_idx = MAX_BR_LINK_NUM;
            for (uint8_t i = 0; i < MAX_BR_LINK_NUM; i++)
            {
                if ((app_db.br_link[i].connected_profile & (A2DP_PROFILE_MASK | AVRCP_PROFILE_MASK)) &&
                    (i != active_a2dp_idx))
                {
                    inactive_a2dp_idx = i;
                    break;
                }
            }
#if ISOC_AUDIO_SUPPORT
            if (mtc_get_sniffing() == MULTI_PRO_SNIFI_INVO)
            {
                APP_PRINT_TRACE0("JUDGE_EVENT_SNIFFING_STOP: STOP SNIFFING");
                break;
            }
#endif
            if ((app_db.br_link[active_a2dp_idx].streaming_fg) &&
                ((app_cfg_const.enable_rtk_charging_box && !app_db.remote_case_closed) ||
                 (!app_cfg_const.enable_rtk_charging_box && app_db.remote_loc != BUD_LOC_IN_CASE)))
            {
                app_audio_a2dp_stream_start_event_handle();
                app_relay_async_single(APP_MODULE_TYPE_AUDIO_POLICY, APP_REMOTE_MSG_A2DP_STREAM_START);

#if F_APP_MUTILINK_VA_PREEMPTIVE
                if (app_allow_a2dp_sniffing(active_a2dp_idx))
                {
                    app_multilink_set_active_idx(active_a2dp_idx);
#if F_APP_QOL_MONITOR_SUPPORT
                    int8_t b2b_rssi = 0;
                    app_qol_get_aggregate_rssi(true, &b2b_rssi);

                    if ((b2b_rssi != 0) && (b2b_rssi > B2B_RSSI_THRESHOLD_START_SNIFFING))
                    {
                        app_bt_sniffing_start(app_db.br_link[active_a2dp_idx].bd_addr, BT_SNIFFING_TYPE_A2DP);
                    }
#else
                    app_bt_sniffing_start(app_db.br_link[active_a2dp_idx].bd_addr, BT_SNIFFING_TYPE_A2DP);
#endif

                    if (remote_session_role_get() != REMOTE_SESSION_ROLE_SECONDARY)
                    {
                        if (app_db.br_link[active_a2dp_idx].a2dp_track_handle)
                        {
                            audio_track_start(app_db.br_link[active_a2dp_idx].a2dp_track_handle);
                        }
                    }
                }
#else
#if F_APP_QOL_MONITOR_SUPPORT
                if ((b2b_rssi != 0) && (b2b_rssi > B2B_RSSI_THRESHOLD_START_SNIFFING))
                {
                    app_bt_sniffing_start(app_db.br_link[active_a2dp_idx].bd_addr, BT_SNIFFING_TYPE_A2DP);
                }
#else
                app_bt_sniffing_start(app_db.br_link[active_a2dp_idx].bd_addr, BT_SNIFFING_TYPE_A2DP);
#endif
#endif
            }
            else if ((app_db.br_link[active_a2dp_idx].used) &&
                     (app_db.br_link[active_a2dp_idx].streaming_fg == false))
            {
                if (inactive_a2dp_idx == MAX_BR_LINK_NUM)
                {
                    app_multilink_set_active_idx(active_a2dp_idx);
                }
                else if (app_db.br_link[inactive_a2dp_idx].streaming_fg == false)
                {
                    APP_PRINT_TRACE3("JUDGE_EVENT_SNIFFING_STOP: inactive_a2dp_idx %d, bd_addr %s, stream %d",
                                     inactive_a2dp_idx,
                                     TRACE_BDADDR(app_db.br_link[inactive_a2dp_idx].bd_addr),
                                     app_db.br_link[inactive_a2dp_idx].streaming_fg);
                }
#if F_APP_MUTILINK_VA_PREEMPTIVE
                else if (app_db.br_link[inactive_a2dp_idx].streaming_fg == true)
                {
                    if (app_db.remote_loc != BUD_LOC_IN_CASE)
                    {
                        app_multilink_stream_avrcp_set(inactive_a2dp_idx);

                        app_audio_a2dp_stream_start_event_handle();
                        app_relay_async_single(APP_MODULE_TYPE_AUDIO_POLICY, APP_REMOTE_MSG_A2DP_STREAM_START);
                        if (app_allow_a2dp_sniffing(inactive_a2dp_idx))
                        {
                            app_multilink_set_active_idx(inactive_a2dp_idx);
                            if (app_bt_sniffing_start(app_db.br_link[inactive_a2dp_idx].bd_addr, BT_SNIFFING_TYPE_A2DP)) {};
                        }
                    }
                }
#endif
            }
            else
            {
                uint8_t idx = MAX_BR_LINK_NUM;
                for (uint8_t i = 0; i < MAX_BR_LINK_NUM; i++)
                {
                    if (app_check_b2s_link_by_id(i) &&
                        app_db.br_link[i].connected_profile & (A2DP_PROFILE_MASK | AVRCP_PROFILE_MASK | HFP_PROFILE_MASK |
                                                               HSP_PROFILE_MASK))
                    {
                        idx = i;
                        break;
                    }
                }
                app_multilink_set_active_idx(idx);
            }
        }
        break;

    default:
        break;
    }

    if (app_cfg_nv.bud_role != REMOTE_SESSION_ROLE_SECONDARY && active_a2dp_idx == app_idx)
    {
        if (event == JUDGE_EVENT_A2DP_START)
        {
            app_audio_a2dp_play_status_update(APP_A2DP_STREAM_A2DP_START);
        }
        else if (event == JUDGE_EVENT_A2DP_STOP)
        {
            app_audio_a2dp_play_status_update(APP_A2DP_STREAM_A2DP_STOP);
        }
    }

    APP_PRINT_TRACE5("app_judge_active_a2dp_idx_and_qos: 2 event %d, active_a2dp_idx %d, app_idx %d, "
                     "wait_resume_a2dp_idx %d, update_active_a2dp_idx %d",
                     event, active_a2dp_idx, app_idx, app_db.wait_resume_a2dp_idx, app_db.update_active_a2dp_idx);
#endif
}

static void app_multilink_bt_cback(T_BT_EVENT event_type, void *event_buf, uint16_t buf_len)
{
    bool handle = true;
    T_APP_BR_LINK *p_link = NULL;
    T_BT_EVENT_PARAM *param = event_buf;

    switch (event_type)
    {
    case BT_EVENT_SCO_CONN_IND:
        {
#ifdef ERWS_MULTILINK_SUPPORT
            p_link = app_find_br_link(param->sco_conn_ind.bd_addr);
            if (p_link != NULL)
            {
                if (app_cfg_nv.bud_role == REMOTE_SESSION_ROLE_PRIMARY)
                {
                    app_hfp_adjust_sco_window(param->sco_conn_ind.bd_addr, APP_SCO_ADJUST_SCO_CONN_IND_EVT);
                    if (app_cfg_const.enable_multi_sco_disc_resume)
                    {
#if F_APP_MUTILINK_VA_PREEMPTIVE
                        if (va_wait_from_va_sniffing_stop && pending_va_index_from_va == p_link->id)
                        {
                            pending_va_sniffing_type = BT_SNIFFING_TYPE_SCO;
                        }
                        if (app_multilink_sass_bitmap(p_link->id, MULTI_HFP_PREEM, false) &&
                            app_pause_inactive_a2dp_link_stream(p_link->id, true) && app_multi_check_in_sniffing())
#else
                        if (app_pause_inactive_a2dp_link_stream(p_link->id, true))
#endif
                        {
                            // A2DP Paused
                            app_db.sco_wait_a2dp_sniffing_stop = true;
                            app_db.pending_sco_idx = p_link->id;
                        }
                        else
                        {
                            // No A2DP
                            if (!app_bt_sniffing_start(param->sco_conn_ind.bd_addr, BT_SNIFFING_TYPE_SCO))
                            {
                                p_link = app_find_br_link(param->sco_conn_ind.bd_addr);

                                if (p_link != NULL)
                                {
                                    bt_sco_conn_cfm(param->sco_conn_ind.bd_addr, true);
                                }
                            }
                        }
                    }
                    else
                    {
#if F_APP_MUTILINK_VA_PREEMPTIVE
                        if (va_wait_from_va_sniffing_stop && pending_va_index_from_va == p_link->id)
                        {
                            pending_va_sniffing_type = BT_SNIFFING_TYPE_SCO;
                        }
                        if (app_multilink_sass_bitmap(p_link->id, MULTI_HFP_PREEM, false) &&
                            app_pause_inactive_a2dp_link_stream(p_link->id, false) && app_multi_check_in_sniffing())
#else
                        if (app_pause_inactive_a2dp_link_stream(p_link->id, false))
#endif
                        {
                            // A2DP Paused
                            app_db.sco_wait_a2dp_sniffing_stop = true;
                            app_db.pending_sco_idx = p_link->id;
                        }
                        else
                        {
                            // No A2DP
                            if (!app_bt_sniffing_start(param->sco_conn_ind.bd_addr, BT_SNIFFING_TYPE_SCO))
                            {
                                p_link = app_find_br_link(param->sco_conn_ind.bd_addr);

                                if (p_link != NULL)
                                {
                                    bt_sco_conn_cfm(param->sco_conn_ind.bd_addr, true);
                                }
                            }
                        }
                    }
                }
                else    // only for single mode
                {
                    bt_sco_conn_cfm(param->sco_conn_ind.bd_addr, true);
                }
            }

#else
            if (!app_bt_sniffing_start(param->sco_conn_ind.bd_addr, BT_SNIFFING_TYPE_SCO))
            {
                p_link = app_find_br_link(param->sco_conn_ind.bd_addr);

                if (p_link != NULL)
                {
                    bt_sco_conn_cfm(param->sco_conn_ind.bd_addr, true);
                }
            }
#endif
        }
        break;

    case BT_EVENT_SCO_CONN_RSP:
        {
        }
        break;

    case BT_EVENT_SCO_CONN_CMPL:
        {
            p_link = app_find_br_link(param->sco_conn_cmpl.bd_addr);
            uint8_t active_hf_index = app_hfp_get_active_hf_index();

            if (p_link != NULL)
            {
                if (param->sco_conn_cmpl.cause == 0)
                {
#if F_APP_SINGLE_MUTLILINK_SCENERIO_1
                    //if (!p_link->call_status)
                    //{
                    app_teams_multilink_record_priority_rearrangment(p_link->id, true);
                    //app_teams_multilink_high_app_action_trigger_low_app_action(p_link->id, T_APP_TEAMS_MULTILINK_HIGH_PRIORITY_APP_START);
                    //}
#else

#if F_APP_HARMAN_FEATURE_SUPPORT
                    if (app_harman_ble_ota_get_upgrate_status())
                    {
                        app_harman_ota_exit(OTA_EXIT_REASON_CALL);
                    }
#endif

                    if (app_find_sco_conn_num() == 1)
                    {
                        bt_sco_link_switch(param->sco_conn_cmpl.bd_addr);
                    }
                    else
                    {
                        APP_PRINT_TRACE2("app_multilink_bt_cback: active sco link %s, current link %s",
                                         TRACE_BDADDR(app_db.br_link[active_hf_index].bd_addr),
                                         TRACE_BDADDR(param->sco_conn_cmpl.bd_addr));

                        bt_sco_link_switch(app_db.br_link[active_hf_index].bd_addr);
                    }

                    bool disc = app_multilink_preemptive_judge(p_link->id, MULTI_HFP_PREEM);
#if F_APP_MUTILINK_VA_PREEMPTIVE
                    if (p_link->call_status == BT_HFP_VOICE_ACTIVATION_ONGOING)
                    {
                        if (!disc || p_link->id != active_va_idx
//                           || ((active_hf_index != p_link->id) &&
//                             (p_link->sco_handle) &&
//                             (app_db.br_link[active_hf_index].call_status != APP_HFP_VOICE_ACTIVATION_ONGOING))
                           )
                        {
                            bt_hfp_voice_recognition_disable_req(p_link->bd_addr);
                            gap_stop_timer(&multilink_disc_va_sco);
                            gap_start_timer(&multilink_disc_va_sco, "multilink_disc_va_sco",
                                            multilink_timer_queue_id, MULTILINK_DISC_VA_SCO,
                                            p_link->id, 1500);
                        }
                    }
                    else if (((active_hf_index != p_link->id) && (p_link->sco_handle)) || !disc)
                    {
                        if (app_db.br_link[p_link->id].call_status != BT_HFP_INCOMING_CALL_ONGOING &&
                            p_link->id != active_hfp_transfer)
                        {
                            bt_hfp_audio_disconnect_req(p_link->bd_addr);
                            if (app_cfg_const.enable_multi_link)
                            {
                                sco_wait_from_sco_sniffing_stop = true;
                                pending_sco_index_from_sco = p_link->id;
                            }
                        }
                    }
#else
                    if ((active_hf_index != p_link->id) && (p_link->sco_handle))
                    {
                        bt_hfp_audio_disconnect_req(p_link->bd_addr);
                        sco_wait_from_sco_sniffing_stop = true;
                        pending_sco_index_from_sco = p_link->id;
                    }
#endif
#endif
                }
                else
                {
                    if ((active_hf_index != p_link->id))
                    {
                        sco_wait_from_sco_sniffing_stop = true;
                        pending_sco_index_from_sco = p_link->id;
                    }
                }
            }
            if (app_cfg_const.always_play_hf_incoming_tone_when_incoming_call)
            {
                app_hfp_inband_tone_mute_ctrl();
            }

            app_bt_policy_primary_engage_action_adjust();
        }
        break;

    case BT_EVENT_SCO_DISCONNECTED:
        {
            if (param->sco_disconnected.cause == (HCI_ERR | HCI_ERR_CMD_DISALLOWED))
            {
                break;
            }

            app_bt_sniffing_stop(param->sco_disconnected.bd_addr, BT_SNIFFING_TYPE_SCO);

            p_link = app_find_br_link(param->sco_disconnected.bd_addr);

            if (p_link != NULL)
            {
                p_link->sco_handle = 0;
#if F_APP_SINGLE_MUTLILINK_SCENERIO_1
                if (app_teams_multiink_get_record_status_by_link_id(p_link->id))
                {
                    app_teams_multilink_record_priority_rearrangment(p_link->id, false);
                }
#else

                //if (p_link->id == app_hfp_get_active_hf_index())
                {
                    uint8_t i;

                    //set actvie sco
                    for (i = 0; i < MAX_BR_LINK_NUM; i++)
                    {
                        if (app_db.br_link[i].sco_handle)
                        {
                            bt_sco_link_switch(app_db.br_link[i].bd_addr);
                            break;
                        }
                    }
                }

                if (app_cfg_const.enable_multi_sco_disc_resume)
                {
                    if ((app_cfg_const.enable_multi_link) &&
                        (app_find_b2s_link_num() == MULTILINK_SRC_CONNECTED))
                    {
                        APP_PRINT_TRACE4("sco_disc_resume sco_num:%d, hfp_active:%d, p_link->id:%d, hfp_status:%d",
                                         app_find_sco_conn_num(),
                                         app_hfp_get_active_hf_index(),
                                         p_link->id,
                                         app_hfp_get_call_status());
                        if ((app_find_sco_conn_num() == 0) &&
                            (app_hfp_get_call_status() == BT_HFP_CALL_IDLE))
                        {
                            app_resume_a2dp_link_stream(p_link->id);
                        }
                    }
                }

#endif
#if F_APP_MUTILINK_VA_PREEMPTIVE
                if (p_link->id == pending_va_index_from_va)
                {
                    va_wait_from_va_sniffing_stop = false;
                    pending_va_index_from_va = 0xFF;
                    pending_va_sniffing_type = 0xFF;
                }
#endif

                if (active_hfp_transfer != 0xFF || (p_link->id == app_hfp_get_org_hfp_idx() &&
                                                    p_link->call_status == BT_HFP_CALL_IDLE))
                {
                    if (!app_multi_check_in_sniffing())
                    {
                        app_handle_sniffing_link_disconnect_event(p_link->id);
                    }
                }
            }
            if (app_cfg_const.always_play_hf_incoming_tone_when_incoming_call)
            {
                app_hfp_inband_tone_mute_ctrl();
            }
            app_bt_policy_primary_engage_action_adjust();

            app_bt_policy_qos_param_update(param->sco_disconnected.bd_addr, BP_TPOLL_SCO_DISC_EVENT);
#if F_APP_HARMAN_FEATURE_SUPPORT
#if HARMAN_SUPPORT_WATER_EJECTION
            app_harman_water_ejection_stop();
#endif
            app_harman_sco_status_notify();
            if ((app_hfp_sco_is_connected() == false && app_hfp_get_call_status() == BT_HFP_CALL_IDLE)
                && (app_db.br_link[app_get_active_a2dp_idx()].streaming_fg == false))
            {
                au_connect_idle_to_power_off(CONNECT_IDLE_POWER_OFF_START, p_link->id);
            }
#endif
        }
        break;

    case BT_EVENT_ACL_CONN_SUCCESS:
        {
            p_link = app_find_br_link(param->acl_conn_success.bd_addr);
            if (p_link != NULL)
            {
                if (app_multilink_get_active_idx() >= MAX_BR_LINK_NUM)
                {
                    app_multilink_set_active_idx(p_link->id);
                }
            }
        }
        break;

    case BT_EVENT_HFP_CONN_CMPL:
        {
            p_link = app_find_br_link(param->hfp_conn_cmpl.bd_addr);
            if (p_link != NULL)
            {
                if (app_multilink_get_active_idx() >= MAX_BR_LINK_NUM)
                {
                    app_multilink_set_active_idx(p_link->id);
                }
            }
        }
        break;

    case BT_EVENT_A2DP_STREAM_OPEN:
        {
            p_link = app_find_br_link(param->a2dp_stream_open.bd_addr);
            if (p_link != NULL)
            {
                if (app_multilink_get_active_idx() >= MAX_BR_LINK_NUM)
                {
                    app_multilink_set_active_idx(p_link->id);
                }
                app_judge_active_a2dp_idx_and_qos(p_link->id, JUDGE_EVENT_A2DP_CONNECTED);
            }
        }
        break;

    case BT_EVENT_A2DP_DISCONN_CMPL:
        {
            p_link = app_find_br_link(param->a2dp_disconn_cmpl.bd_addr);
            if (p_link != NULL)
            {
                app_judge_active_a2dp_idx_and_qos(p_link->id, JUDGE_EVENT_A2DP_DISC);
            }
        }
        break;

    case BT_EVENT_AVRCP_PLAY_STATUS_RSP:
    case BT_EVENT_AVRCP_PLAY_STATUS_CHANGED:
        {
            p_link = app_find_br_link(param->avrcp_play_status_changed.bd_addr);
            if (p_link != NULL)
            {
                APP_PRINT_TRACE2("BT_EVENT_AVRCP_PLAY_STATUS_CHANGED %d ,%d", p_link->id,
                                 param->avrcp_play_status_changed.play_status);
                if (param->avrcp_play_status_changed.play_status == BT_AVRCP_PLAY_STATUS_PLAYING)
                {
                    app_audio_set_bud_stream_state(BUD_STREAM_STATE_AUDIO);
                    stream_only[p_link->id] = false;

                    if (p_link->id == multilink_silence_packet_idx)
                    {
                        gap_stop_timer(&multilink_silence_packet_judge);
                    }

                    gap_stop_timer(&multilink_silence_packet_timer);
                    app_judge_active_a2dp_idx_and_qos(p_link->id, JUDGE_EVENT_MEDIAPLAYER_PLAYING);
#if F_APP_HARMAN_FEATURE_SUPPORT
                    au_set_record_a2dp_active_ever(true);
#endif
                }
                else if (param->avrcp_play_status_changed.play_status == BT_AVRCP_PLAY_STATUS_PAUSED)
                {
                    app_judge_active_a2dp_idx_and_qos(p_link->id, JUDGE_EVENT_MEDIAPLAYER_PAUSED);
                }
            }
        }
        break;

    case BT_EVENT_A2DP_STREAM_START_IND:
        {
            if (app_cfg_const.rws_gaming_mode == 0)
            {
                p_link = app_find_br_link(param->a2dp_stream_start_ind.bd_addr);
                if (p_link != NULL)
                {
                    APP_PRINT_TRACE5("app_multilink_bt_cback: p_link %p, id %d, active id %d, streaming_fg %d / %d",
                                     p_link, p_link->id, active_a2dp_idx, p_link->streaming_fg,
                                     app_db.br_link[active_a2dp_idx].streaming_fg);
#if F_APP_MUTILINK_VA_PREEMPTIVE
                    uint8_t i;
                    for (i = 0; i < MAX_BR_LINK_NUM; i++)
                    {
                        if (app_check_b2s_link_by_id(i))
                        {
                            if (i != p_link->id)
                            {
                                break;
                            }
                        }
                    }

                    if ((app_cfg_const.enable_multi_link) &&
                        (i < MAX_BR_LINK_NUM) &&
                        (app_db.br_link[i].streaming_fg == true))
                    {
                        app_db.a2dp_preemptive_flag = true;
                        app_bt_sniffing_param_update(APP_BT_SNIFFING_EVENT_MULTILINK_CHANGE);
                    }

                    if (va_wait_from_va_sniffing_stop && pending_va_index_from_va == p_link->id)
                    {
                        pending_va_sniffing_type = BT_SNIFFING_TYPE_A2DP;
                    }
#endif
                    if (p_link->id != active_a2dp_idx)
                    {
                        // not active link
                        if (app_db.br_link[active_a2dp_idx].streaming_fg == false)
                        {
#if F_APP_MUTILINK_VA_PREEMPTIVE
                            if (!app_allow_a2dp_sniffing(p_link->id) ||
                                !app_bt_sniffing_start(param->a2dp_stream_start_ind.bd_addr, BT_SNIFFING_TYPE_A2DP))
#else
                            if (!app_bt_sniffing_start(param->a2dp_stream_start_ind.bd_addr, BT_SNIFFING_TYPE_A2DP))
#endif
                            {
                                bt_a2dp_stream_start_cfm(param->a2dp_stream_start_ind.bd_addr, true);
                                p_link->streaming_fg = true;
                                app_judge_active_a2dp_idx_and_qos(p_link->id, JUDGE_EVENT_A2DP_START);
                            }
                            if (app_cfg_const.disable_multilink_preemptive)
                            {
                                app_judge_active_a2dp_idx_and_qos(p_link->id, JUDGE_EVENT_A2DP_START);
                            }
                        }
                        else
                        {
                            bt_a2dp_stream_start_cfm(param->a2dp_stream_start_ind.bd_addr, true);
                            p_link->streaming_fg = true;
                            app_judge_active_a2dp_idx_and_qos(p_link->id, JUDGE_EVENT_A2DP_START);
                        }
                    }
                    else
                    {
#if F_APP_HARMAN_FEATURE_SUPPORT
                        if (pending_id == active_a2dp_idx)
                        {
                            gap_stop_timer(&multilink_a2dp_stop_timer);
                            pending_id = 0xff;
                            app_multi_a2dp_stop(active_a2dp_idx);
                        }
#endif
                        // active link
#if F_APP_MUTILINK_VA_PREEMPTIVE
                        if (p_link->streaming_fg == false && (va_wait_from_va_sniffing_stop ||
                                                              !app_allow_a2dp_sniffing(p_link->id) ||
                                                              !app_bt_sniffing_start(param->a2dp_stream_start_ind.bd_addr, BT_SNIFFING_TYPE_A2DP)))
                        {
                            bt_a2dp_stream_start_cfm(param->a2dp_stream_start_ind.bd_addr, true);
                            p_link->streaming_fg = true;
                            app_judge_active_a2dp_idx_and_qos(p_link->id, JUDGE_EVENT_A2DP_START);
                        }
#else
                        if (p_link->streaming_fg == false &&
                            !app_bt_sniffing_start(param->a2dp_stream_start_ind.bd_addr, BT_SNIFFING_TYPE_A2DP))
                        {
                            bt_a2dp_stream_start_cfm(param->a2dp_stream_start_ind.bd_addr, true);
                            p_link->streaming_fg = true;
                            app_judge_active_a2dp_idx_and_qos(p_link->id, JUDGE_EVENT_A2DP_START);
                        }
#endif
                    }
                }
            }
            else
            {
                p_link = app_find_br_link(param->a2dp_stream_start_ind.bd_addr);
                if (p_link != NULL)
                {
                    if (bt_a2dp_stream_start_cfm(param->a2dp_stream_start_ind.bd_addr, true))
                    {
                        p_link->streaming_fg = true;
                    }
                    app_judge_active_a2dp_idx_and_qos(p_link->id, JUDGE_EVENT_A2DP_START);
                }
            }
            app_bt_policy_primary_engage_action_adjust();
        }
        break;

    case BT_EVENT_A2DP_STREAM_START_RSP:
        {
            p_link = app_find_br_link(param->a2dp_stream_start_rsp.bd_addr);
            if (p_link != NULL)
            {
                p_link->streaming_fg = true;
                app_judge_active_a2dp_idx_and_qos(p_link->id, JUDGE_EVENT_A2DP_START);
            }
            app_bt_policy_primary_engage_action_adjust();

        }
        break;

    case BT_EVENT_A2DP_STREAM_STOP:
        {
            p_link = app_find_br_link(param->a2dp_stream_stop.bd_addr);
#if F_APP_HARMAN_FEATURE_SUPPORT
            if (p_link != NULL)
            {
                app_harman_total_playback_time_update();
                p_link->streaming_fg = false;
                if (multilink_a2dp_stop_timer == NULL && p_link->id == active_a2dp_idx)
                {
                    gap_start_timer(&multilink_a2dp_stop_timer, "multilink_a2dp_stop_timer",
                                    multilink_timer_queue_id, MULTILINK_A2DP_STOP_TIMER,
                                    p_link->id, 1000);
                    pending_id = p_link->id;
                }
                else
                {
                    app_multi_a2dp_stop(p_link->id);
                }
            }
#else
            if (p_link != NULL)
            {
                app_harman_total_playback_time_update();
                p_link->streaming_fg = false;
                app_judge_active_a2dp_idx_and_qos(p_link->id, JUDGE_EVENT_A2DP_STOP);
            }

            if (app_cfg_const.rws_gaming_mode == 0)
            {
                if (app_multi_check_in_sniffing())
                {
                    app_bt_sniffing_stop(param->a2dp_stream_stop.bd_addr, BT_SNIFFING_TYPE_A2DP);
                }
                else if (p_link->id == active_idx)
                {
                    app_handle_sniffing_link_disconnect_event(p_link->id);
                }
            }

            app_bt_policy_primary_engage_action_adjust();
#endif
        }
        break;

    case BT_EVENT_REMOTE_ROLESWAP_STATUS:
        {
            if (param->remote_roleswap_status.status == BT_ROLESWAP_STATUS_SUCCESS &&
                param->remote_roleswap_status.device_role == REMOTE_SESSION_ROLE_PRIMARY)
            {
                p_link = app_find_br_link(app_db.active_hfp_addr);
                if (p_link != NULL)
                {
                    app_multilink_set_active_idx(p_link->id);
                }

                APP_PRINT_TRACE2("app_multilink_bt_cback: roleswap status %x, %s",
                                 param->remote_roleswap_status.status, TRACE_BDADDR(app_db.active_hfp_addr));
            }
        }
        break;

    case BT_EVENT_A2DP_STREAM_CLOSE:
        {
            p_link = app_find_br_link(param->a2dp_stream_close.bd_addr);
            if (p_link != NULL)
            {
                app_harman_total_playback_time_update();
                p_link->streaming_fg = false;
                app_judge_active_a2dp_idx_and_qos(p_link->id, JUDGE_EVENT_A2DP_STOP);
            }

            if (app_cfg_const.rws_gaming_mode == 0)
            {
                app_bt_sniffing_stop(param->a2dp_stream_stop.bd_addr, BT_SNIFFING_TYPE_A2DP);
            }
        }
        break;

    case BT_EVENT_SCO_SNIFFING_CONN_CMPL:
        {
            if (app_cfg_nv.bud_role == REMOTE_SESSION_ROLE_SECONDARY)
            {
                p_link = app_find_br_link(param->sco_sniffing_conn_cmpl.bd_addr);

                if (p_link == NULL)
                {
                    p_link = app_alloc_br_link(param->sco_sniffing_conn_cmpl.bd_addr);
                }

                if (p_link != NULL)
                {
                    p_link->sco_handle = param->sco_sniffing_conn_cmpl.handle;
                }
            }
        }
        break;

    case BT_EVENT_SCO_SNIFFING_DISCONN_CMPL:
        {
            if (app_cfg_nv.bud_role == REMOTE_SESSION_ROLE_SECONDARY)
            {
                p_link = app_find_br_link(param->sco_sniffing_disconn_cmpl.bd_addr);

                if (p_link != NULL)
                {
                    p_link->sco_handle = 0;
                }

                app_audio_set_mic_mute_status(0);
            }
        }
        break;

    case BT_EVENT_ACL_CONN_DISCONN:
        {
            //b2b or b2s link disconnected => reset preemptive flag
            app_db.a2dp_preemptive_flag = false;
#if F_APP_HARMAN_FEATURE_SUPPORT
            au_connect_idle_to_power_off(DISC_RESET_TO_COUNT, 0xff);
            au_set_harman_aling_active_idx(0xff);
#endif
            if (memcmp(app_db.active_hfp_addr, param->acl_conn_disconn.bd_addr, 6) == 0)
            {
//                for (uint8_t i = 0; i < MAX_BR_LINK_NUM; i++)
//                {
//                    if (app_check_b2s_link_by_id(i) &&
//                        memcmp(param->acl_conn_disconn.bd_addr, app_db.br_link[i].bd_addr, 6) != 0 &&
//                        app_db.br_link[i].connected_profile & (A2DP_PROFILE_MASK | AVRCP_PROFILE_MASK | HFP_PROFILE_MASK | HSP_PROFILE_MASK))
//                    {
//                        idx = i;
//                        break;
//                    }
//                }
                if (app_find_b2s_link_num() == 0)
                {
                    app_multilink_set_active_idx(MAX_BR_LINK_NUM);
                }
                else if (!app_multi_check_in_sniffing())
                {
                    app_handle_sniffing_link_disconnect_event(0xff);
                }
            }
        }
        break;

    case BT_EVENT_HFP_CALL_STATUS:
        {
            uint8_t app_idx;
            uint8_t cmd_ptr[7];
            T_APP_BR_LINK *p_link, *p_link_call;
            app_idx = app_get_active_a2dp_idx();
            p_link = &(app_db.br_link[app_idx]);
            p_link_call = app_find_br_link(param->hfp_call_status.bd_addr);


            if (p_link_call != NULL && p_link_call->call_status == BT_HFP_CALL_IDLE)
            {
                if (p_link_call->id == pending_sco_index_from_sco)
                {
                    sco_wait_from_sco_sniffing_stop = false;
                    pending_sco_index_from_sco = 0xFF;
                }
                if (p_link->id == active_hfp_transfer)
                {
                    active_hfp_transfer = 0xFF;
                }
            }

            if (p_link != NULL)
            {
                if (param->hfp_call_status.curr_status != BT_HFP_CALL_IDLE)
                {
#if F_APP_MUTILINK_VA_PREEMPTIVE
                    if (p_link_call != NULL)
                    {
                        if (p_link_call->call_status == BT_HFP_VOICE_ACTIVATION_ONGOING)
                        {
                            active_va_idx = p_link_call->id;
                        }
                        app_multilink_preemptive_judge(p_link_call->id, MULTI_HFP_PREEM);
                    }

#endif
                    if (app_db.remote_session_state == REMOTE_SESSION_STATE_CONNECTED)
                    {
                        if (app_cfg_nv.bud_role == REMOTE_SESSION_ROLE_PRIMARY &&
                            app_multilink_sass_bitmap(app_find_br_link(param->hfp_call_status.bd_addr)->id,
                                                      MULTI_HFP_PREEM, false))
                        {
#if F_APP_MUTILINK_VA_PREEMPTIVE && F_APP_ERWS_SUPPORT
                            if (app_db.br_link[app_idx].bt_sniffing_state == APP_BT_SNIFFING_STATE_STARTING)
                            {
                                app_bt_a2dp_sniffing_mute(app_db.br_link[app_idx].bd_addr, BT_SNIFFING_TYPE_A2DP);
                            }
                            else
#endif
                            {
                                memcpy(&cmd_ptr[0], app_db.br_link[app_idx].bd_addr, 6);
                                cmd_ptr[6] = AUDIO_STREAM_TYPE_PLAYBACK;
                                app_relay_async_single_with_raw_msg(APP_MODULE_TYPE_MULTI_LINK,
                                                                    APP_REMOTE_MSG_MUTE_AUDIO_WHEN_MULTI_CALL_NOT_IDLE,
                                                                    cmd_ptr, 7);
                            }
                        }
                    }
                    else
                    {
                        //audio_track_volume_out_mute(p_link->a2dp_track_handle);
                    }
                }
                else
                {
                    if (app_db.remote_session_state == REMOTE_SESSION_STATE_CONNECTED)
                    {
                        if (app_cfg_nv.bud_role == REMOTE_SESSION_ROLE_PRIMARY)
                        {
#if F_APP_MUTILINK_VA_PREEMPTIVE && F_APP_ERWS_SUPPORT
                            if (app_db.br_link[app_idx].bt_sniffing_state == APP_BT_SNIFFING_STATE_STARTING)
                            {
                                app_bt_a2dp_sniffing_unmute(app_db.br_link[app_idx].bd_addr, BT_SNIFFING_TYPE_A2DP);
                            }
#endif
                            memcpy(&cmd_ptr[0], app_db.br_link[app_idx].bd_addr, 6);
                            cmd_ptr[6] = AUDIO_STREAM_TYPE_PLAYBACK;
                            app_relay_async_single_with_raw_msg(APP_MODULE_TYPE_MULTI_LINK,
                                                                APP_REMOTE_MSG_UNMUTE_AUDIO_WHEN_MULTI_CALL_IDLE,
                                                                cmd_ptr, 7);
                        }
                    }
                    else
                    {
                        audio_track_volume_out_unmute(p_link->a2dp_track_handle);
                        p_link->playback_muted = false;
                    }
                }
            }
        }
        break;

    case BT_EVENT_HFP_DISCONN_CMPL:
        {
            T_APP_BR_LINK *p_link;

            p_link = app_find_br_link(param->hfp_disconn_cmpl.bd_addr);
            if (p_link != NULL)
            {
                if (p_link->id == pending_sco_index_from_sco)
                {
                    sco_wait_from_sco_sniffing_stop = false;
                    pending_sco_index_from_sco = 0xFF;
                }

                if (p_link->id == active_hfp_transfer)
                {
                    active_hfp_transfer = 0xFF;
                }
#if F_APP_MUTILINK_VA_PREEMPTIVE
                if (p_link->id == pending_va_index_from_va)
                {
                    va_wait_from_va_sniffing_stop = false;
                    pending_va_index_from_va = 0xFF;
                    pending_va_sniffing_type = 0xFF;
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
        APP_PRINT_INFO1("app_multilink_bt_cback: event_type 0x%04x", event_type);
    }
}

static void app_multilink_timeout_cb(uint8_t timer_id, uint16_t timer_chann)
{
    APP_PRINT_TRACE2("app_multilink_timeout_cb: timer_id %d, timer_chann %d",
                     timer_id, timer_chann);
    switch (timer_id)
    {
    case MULTILINK_UPDATE_ACTIVE_A2DP_INDEX:
        {
            app_db.update_active_a2dp_idx = false;
            gap_stop_timer(&multilink_update_active_a2dp_index);
            if (app_connected_profile_link_num(A2DP_PROFILE_MASK) >= app_cfg_const.max_legacy_multilink_devices)
            {
                uint8_t i;
                for (i = 0; i < MAX_BR_LINK_NUM; i++)
                {
                    if ((i != timer_chann) && ((app_db.br_link[i].streaming_fg == true) ||
                                               (app_db.br_link[i].avrcp_play_status == BT_AVRCP_PLAY_STATUS_PLAYING)))
                    {
                        APP_PRINT_TRACE2("app_multilink_timeout_cb: bt_avrcp_pause: i %d, active_a2dp_idx %d",
                                         i, timer_chann);
                        app_pause_inactive_a2dp_link_stream(timer_chann, false);
                    }
                }
            }
            else if (app_connected_profile_link_num(A2DP_PROFILE_MASK) == 1)
            {
                uint8_t i;
                for (i = 0; i < MAX_BR_LINK_NUM; i++)
                {
                    if ((i != timer_chann) && (app_db.br_link[i].streaming_fg == true))
                    {
                        app_a2dp_active_link_set(app_db.br_link[i].bd_addr);
#if (F_APP_HARMAN_FEATURE_SUPPORT == 0)
                        app_bond_set_priority(app_db.br_link[i].bd_addr);
#endif
                        break;
                    }
                }
            }
        }
        break;

    case MULTILINK_PAUSE_INACTIVE_A2DP:
        {
            gap_stop_timer(&multilink_pause_inactive_a2dp);
            if ((app_db.br_link[timer_chann].streaming_fg == true) ||
                (app_db.br_link[timer_chann].avrcp_play_status == BT_AVRCP_PLAY_STATUS_PLAYING))
            {
                // app_pause_inactive_a2dp_link_stream(timer_chann, true);
                bt_avrcp_pause(app_db.br_link[timer_chann].bd_addr);
                app_db.br_link[timer_chann].avrcp_play_status = BT_AVRCP_PLAY_STATUS_PAUSED;
            }
            else
            {
                APP_PRINT_WARN2("app_multilink_timeout_cb: streaming_fg %d, avrcp_play_status %d",
                                app_db.br_link[timer_chann].streaming_fg, app_db.br_link[timer_chann].avrcp_play_status);
            }
        }
        break;

    case MULTILINK_DISCONN_INACTIVE_ACL:
        {
            gap_stop_timer(&multilink_wait_disconn_acl);

            if (app_db.br_link[timer_chann].used == true)
            {
                legacy_send_acl_disconn_req(app_db.br_link[timer_chann].bd_addr);
            }
        }
        break;

#if F_APP_MUTILINK_VA_PREEMPTIVE
    case MULTILINK_DISC_VA_SCO:
        {
            gap_stop_timer(&multilink_disc_va_sco);
            if (app_db.br_link[timer_chann].sco_handle != NULL)
            {
                APP_PRINT_TRACE0("SCO_DISCONNECT_BY_DUT");
#if (F_APP_HARMAN_FEATURE_SUPPORT == 0)
                bt_hfp_audio_disconnect_req(app_db.br_link[timer_chann].bd_addr);
                app_bt_sniffing_stop(app_db.br_link[timer_chann].bd_addr, BT_SNIFFING_TYPE_SCO);
#endif
            }
        }
        break;
#endif
    case MULTILINK_SILENCE_PACKET_JUDGE:
        {
            gap_stop_timer(&multilink_silence_packet_judge);

            if (app_db.br_link[timer_chann].streaming_fg == true)
            {
                app_multilink_set_active_idx(timer_chann);
                app_multilink_stream_avrcp_set(timer_chann);
                app_pause_inactive_a2dp_link_stream(timer_chann, false);
            }
            else
            {
                gap_start_timer(&multilink_silence_packet_timer, "multilink_silence_packet_timer",
                                multilink_timer_queue_id, MULTILINK_SILENCE_PACKET_TIMER,
                                0, 7500);
                app_multilink_set_active_idx(app_multilink_get_active_idx());
            }
        }
        break;

    case MULTILINK_SILENCE_PACKET_TIMER:
        {
            gap_stop_timer(&multilink_silence_packet_timer);
        }
        break;

    case MULTILINK_A2DP_STOP_TIMER:
        {
            gap_stop_timer(&multilink_a2dp_stop_timer);
            pending_id = 0xff;
            app_multi_a2dp_stop(timer_chann);
        }
        break;

    case AU_TIMER_HARMAN_POWER_OFF_OPTION:
        {
            gap_stop_timer(&timer_handle_harman_power_off_option);
            if (app_db.device_state == APP_DEVICE_STATE_ON)
            {
                if (app_harman_ble_ota_get_upgrate_status())
                {
                    if ((app_hfp_sco_is_connected() == false && app_hfp_get_call_status() == BT_HFP_CALL_IDLE)
                        && (app_db.br_link[app_get_active_a2dp_idx()].streaming_fg == false))
                    {
                        au_connect_idle_to_power_off(CONNECT_IDLE_POWER_OFF_START, 0xff);
                    }
                }
                else
                {
                    app_mmi_handle_action(MMI_DEV_POWER_OFF);
                }
            }
        }
        break;

#if F_APP_HARMAN_FEATURE_SUPPORT
    case HARMAN_NOTIFY_DEVICEINFO:
        {
            gap_stop_timer(&multilink_notify_deviceinfo_timer);

            uint16_t mtu_size;
            T_GAP_CAUSE cause = GAP_CAUSE_ERROR_UNKNOWN;
            uint8_t ble_conn_id = le_common_adv_get_conn_id();
            uint8_t ble_paired_status = 0x00;

            if ((app_db.le_link[ble_conn_id].state == LE_LINK_STATE_CONNECTED) &&
                (app_db.le_link[ble_conn_id].used == true))
            {
                ble_paired_status = 0x01;
            }
            app_harman_ble_paired_status_notify(ble_paired_status, ble_conn_id);

            cause = le_get_conn_param(GAP_PARAM_CONN_MTU_SIZE, &mtu_size, ble_conn_id);

            APP_PRINT_TRACE4("HARMAN_NOTIFY_DEVICEINFO: ble_conn_id: %d, ble_paired_status: %d, cause: %d, mtu_size: %d",
                             ble_conn_id, ble_paired_status, cause, mtu_size);
            if (40 <= (mtu_size - 3))
            {
                app_harman_devinfo_notify(ble_conn_id);
            }
            else
            {
                gap_start_timer(&multilink_notify_deviceinfo_timer, "harman_notify_deviceinfo",
                                multilink_timer_queue_id, HARMAN_NOTIFY_DEVICEINFO,
                                0, 1000);
            }
        }
        break;
#endif

    default:
        break;
    }
}

uint16_t app_multilink_relay_cback(uint8_t *buf, uint8_t msg_type, bool total)
{
    uint16_t payload_len = 0;
    uint8_t *msg_ptr = NULL;
    bool skip = true;

    switch (msg_type)
    {
    case APP_REMOTE_MSG_ACTIVE_BD_ADDR_SYNC:
        {
            skip = false;
            payload_len = 6;
            msg_ptr = (uint8_t *)app_db.br_link[app_multilink_get_active_idx()].bd_addr;
        }
        break;

    case APP_REMOTE_MSG_PHONE_CONNECTED:
        {
            payload_len = 1;
            msg_ptr = (uint8_t *)&app_db.b2s_connected_num;
        }
        break;

    case APP_REMOTE_MSG_SASS_PREEM_BIT_SYNC:
        {
            skip = false;
            payload_len = 2;
            msg_ptr = (uint8_t *)&app_cfg_nv.sass_preemptive;
        }
        break;

    case APP_REMOTE_MSG_SASS_SWITCH_SYNC:
        {
            skip = false;
            payload_len = 1;
            msg_ptr = (uint8_t *)&app_cfg_nv.sass_switch_setting;
        }
        break;
    case APP_REMOTE_MSG_SASS_MULTILINK_STATE_SYNC:
        {
            skip = false;
            payload_len = 1;
            msg_ptr = (uint8_t *) &app_cfg_nv.sass_multilink_cfg;
        }
        break;
    case APP_REMOTE_MSG_SASS_DEVICE_SUPPORT_SYNC:
        {
            skip = false;
            payload_len = 1;
            msg_ptr = (uint8_t *) &app_cfg_nv.sass_bit_map;
        }
        break;
    case APP_REMOTE_MSG_SASS_DEVICE_BITMAP_SYNC:
        {
            skip = false;
            payload_len = 1;
            msg_ptr = (uint8_t *) &app_db.conn_bit_map;

        }
        break;
    default:
        break;
    }

    return app_relay_msg_pack(buf, msg_type, APP_MODULE_TYPE_MULTI_LINK, payload_len, msg_ptr, skip,
                              total);
}

#if GFPS_SASS_SUPPORT
static void app_multilink_sass_parse(uint8_t msg_type, uint8_t *buf, uint16_t len,
                                     T_REMOTE_RELAY_STATUS status)
{
    switch (msg_type)
    {
    case APP_REMOTE_MSG_SASS_PREEM_BIT_SYNC:
        {
            if ((status == REMOTE_RELAY_STATUS_ASYNC_SENT_OUT) ||
                (status == REMOTE_RELAY_STATUS_ASYNC_RCVD))
            {
                uint8_t *p_info = (uint8_t *)buf;
                memcpy(&app_cfg_nv.sass_preemptive, &p_info[0], 2);
            }
        }
        break;

    case APP_REMOTE_MSG_SASS_SWITCH_SYNC:
        {
            if ((status == REMOTE_RELAY_STATUS_ASYNC_SENT_OUT) ||
                (status == REMOTE_RELAY_STATUS_ASYNC_RCVD))
            {
                uint8_t *p_info = (uint8_t *)buf;
                memcpy(&app_cfg_nv.sass_switch_setting, &p_info[0], 1);
            }
        }
        break;
    case APP_REMOTE_MSG_SASS_MULTILINK_STATE_SYNC:
        {
            if (status == REMOTE_RELAY_STATUS_ASYNC_RCVD)
            {
                uint8_t *p_info = (uint8_t *)buf;
                app_cfg_nv.sass_multilink_cfg = *p_info;
            }
        }
        break;
    case APP_REMOTE_MSG_SASS_DEVICE_BITMAP_SYNC:
        {
            if (status == REMOTE_RELAY_STATUS_ASYNC_RCVD)
            {
                uint8_t *p_info = (uint8_t *)buf;
                app_sass_policy_sync_bitmap(*p_info);
            }

        }
        break;
    case APP_REMOTE_MSG_SASS_DEVICE_SUPPORT_SYNC:
        {
            if (status == REMOTE_RELAY_STATUS_ASYNC_RCVD)
            {
                uint8_t *p_info = (uint8_t *)buf;
                app_cfg_nv.sass_bit_map = *p_info;
            }
        }
        break;

    default:
        break;
    }
}
#endif

static void app_multilink_parse_cback(uint8_t msg_type, uint8_t *buf, uint16_t len,
                                      T_REMOTE_RELAY_STATUS status)
{
    switch (msg_type)
    {
    case APP_REMOTE_MSG_MUTE_AUDIO_WHEN_MULTI_CALL_NOT_IDLE:
        {
            if ((status == REMOTE_RELAY_STATUS_ASYNC_SENT_OUT) ||
                (status == REMOTE_RELAY_STATUS_ASYNC_RCVD))
            {
                uint8_t *p_info = (uint8_t *)buf;
                uint8_t bd_addr[6];
                T_AUDIO_STREAM_TYPE stream_type;
                T_APP_BR_LINK *p_link;

                memcpy(bd_addr, &p_info[0], 6);
                p_link = app_find_br_link(bd_addr);
                stream_type = (T_AUDIO_STREAM_TYPE)p_info[6];
                if (p_link != NULL)
                {
                    if (stream_type == AUDIO_STREAM_TYPE_PLAYBACK)
                    {
                        audio_track_volume_out_mute(p_link->a2dp_track_handle);
                        p_link->playback_muted = true;
                    }
                }
            }
        }
        break;

    case APP_REMOTE_MSG_UNMUTE_AUDIO_WHEN_MULTI_CALL_IDLE:
        {
            if ((status == REMOTE_RELAY_STATUS_ASYNC_SENT_OUT) ||
                (status == REMOTE_RELAY_STATUS_ASYNC_RCVD))
            {
                uint8_t *p_info = (uint8_t *)buf;
                uint8_t bd_addr[6];
                T_AUDIO_STREAM_TYPE stream_type;
                T_APP_BR_LINK *p_link;

                memcpy(bd_addr, &p_info[0], 6);
                p_link = app_find_br_link(bd_addr);
                stream_type = (T_AUDIO_STREAM_TYPE)p_info[6];
                if (p_link != NULL)
                {
                    if (stream_type == AUDIO_STREAM_TYPE_PLAYBACK)
                    {
                        audio_track_volume_out_unmute(p_link->a2dp_track_handle);
                        p_link->playback_muted = false;
                    }
                }
            }
        }
        break;

    case APP_REMOTE_MSG_RESUME_BT_ADDRESS:
        {
            memcpy(app_db.resume_addr, (uint8_t *)buf, 6);
            app_db.connected_num_before_roleswap = *((uint8_t *)buf + 6);
            APP_PRINT_TRACE2("app_multilink_parse_cback: connected_num_before_roleswap %d, resume_addr %s",
                             app_db.connected_num_before_roleswap, TRACE_BDADDR(app_db.resume_addr));
        }
        break;

    case APP_REMOTE_MSG_ACTIVE_BD_ADDR_SYNC:
        {
            {
                if ((status == REMOTE_RELAY_STATUS_ASYNC_SENT_OUT) ||
                    (status == REMOTE_RELAY_STATUS_ASYNC_RCVD))
                {
                    uint8_t *p_info = (uint8_t *)buf;
                    memcpy(app_db.active_hfp_addr, &p_info[0], 6);
                }
            }
        }
        break;
#if GFPS_SASS_SUPPORT

    case APP_REMOTE_MSG_SASS_PREEM_BIT_SYNC:
    case APP_REMOTE_MSG_SASS_SWITCH_SYNC:
    case APP_REMOTE_MSG_SASS_MULTILINK_STATE_SYNC:
    case APP_REMOTE_MSG_SASS_DEVICE_BITMAP_SYNC:
    case APP_REMOTE_MSG_SASS_DEVICE_SUPPORT_SYNC:
        {
            if (app_sass_policy_support_easy_connection_switch())
            {
                app_multilink_sass_parse(msg_type, buf, len, status);
            }
        }
        break;
#endif

    case APP_REMOTE_MSG_PROFILE_CONNECTED:
        if (status == REMOTE_RELAY_STATUS_ASYNC_RCVD)
        {
            app_handle_remote_profile_connected_msg(buf);
        }
        break;

    case APP_REMOTE_MSG_PHONE_CONNECTED:
        {
            if (status == REMOTE_RELAY_STATUS_ASYNC_RCVD &&
                app_cfg_nv.bud_role == REMOTE_SESSION_ROLE_SECONDARY)
            {
                app_db.b2s_connected_num = *((uint8_t *)buf);

#if F_APP_LISTENING_MODE_SUPPORT
                if (app_cfg_const.disallow_listening_mode_before_phone_connected)
                {
                    if (app_listening_b2s_connected())
                    {
                        app_listening_judge_conn_disc_evnet(APPLY_LISTENING_MODE_SRC_CONNECTED);
                    }
                    else
                    {
                        app_listening_judge_conn_disc_evnet(APPLY_LISTENING_MODE_SRC_DISCONNECTED);
                    }
                }
#endif
            }
        }
        break;

    default:
        break;
    }
}

void app_multilink_init(void)
{
    app_db.wait_resume_a2dp_idx = MAX_BR_LINK_NUM;
    bt_mgr_cback_register(app_multilink_bt_cback);
    gap_reg_timer_cb(app_multilink_timeout_cb, &multilink_timer_queue_id);
    app_relay_cback_register(app_multilink_relay_cback, app_multilink_parse_cback,
                             APP_MODULE_TYPE_MULTI_LINK, APP_REMOTE_MSG_MULTILINK_TOTAL);

    //test only
    //extend_app_cfg_const.gfps_sass_support = 1;
    //uint8_t tmp[6] = {0x31, 0x8e, 0x1f, 0x01, 0x51, 0x0c};
    //memcpy(app_cfg_nv.common_used_device, tmp, 6);
}

void app_multilink_sco_preemptive(uint8_t inactive_idx)
{
    APP_PRINT_TRACE1("app_multilink_sco_preemptive:%d", inactive_idx);
    bt_hfp_audio_disconnect_req(app_db.br_link[inactive_idx].bd_addr);
    if (app_cfg_const.enable_multi_link)
    {
        sco_wait_from_sco_sniffing_stop = true;
        pending_sco_index_from_sco = inactive_idx;
    }
}

uint8_t app_multi_get_acl_connect_num(void)
{
    uint8_t conn_num = 0;
    uint8_t i = 0;

    for (i = 0; i < MAX_BLE_LINK_NUM; i++)
    {
        APP_PRINT_TRACE3("get_acl_le_link_connect_num %d %d %d",
                         i,
                         app_db.le_link[i].state,
                         app_db.le_link[i].used);

        if ((app_db.le_link[i].state == LE_LINK_STATE_CONNECTED) &&
            (app_db.le_link[i].used == true) &&
            (app_db.le_link[i].cmd_set_enable == true))
        {
            conn_num++;
        }
    }
    for (i = 0; i < MAX_BR_LINK_NUM; i++)
    {
        APP_PRINT_TRACE3("get_acl_br_link_connect_num %d %d 0x%x",
                         i,
                         app_db.br_link[i].acl_handle,
                         app_db.br_link[i].connected_profile);

        if ((app_db.br_link[i].connected_profile & (SPP_PROFILE_MASK | IAP_PROFILE_MASK)) &&
            (app_db.br_link[i].cmd_set_enable == true))
        {
            conn_num++;
        }
    }
    APP_PRINT_TRACE1("CONN_NUM = %d", conn_num);
    return conn_num;
}

#if F_APP_HARMAN_FEATURE_SUPPORT
void au_set_harman_aling_active_idx(uint8_t active_idx)
{
    if (active_idx == 0xff)
    {
        if (app_find_b2s_link_num() == 0)
        {
            aling_active_a2dp_hfp_index = 0xff;
        }
        else if (app_find_b2s_link_num() == 1)
        {
            for (uint8_t i = 0; i < MAX_BR_LINK_NUM; i++)
            {
                if (app_check_b2s_link_by_id(i))
                {
                    aling_active_a2dp_hfp_index = i;
                }
            }
        }
    }
    else
    {
        aling_active_a2dp_hfp_index = active_idx;
    }
    APP_PRINT_TRACE1("au_set_harman_aling_active_index =%d", aling_active_a2dp_hfp_index);
}

uint8_t au_get_harman_aling_active_idx(void)
{
    APP_PRINT_TRACE1("au_get_harman_aling_active_index =%d", aling_active_a2dp_hfp_index);
    return aling_active_a2dp_hfp_index;
}

void au_multilink_harman_check_silent(uint8_t set_idx, uint8_t silent_check)
{
    APP_PRINT_TRACE3("au_multilink_harman_check_silent set_idx=%d, silent_check=%d, app_multilink_get_active_idx=%d",
                     set_idx, silent_check, app_multilink_get_active_idx());
    //silent = true
    //un_silent = false
    if (silent_check == 0xff)
    {
        app_db.br_link[set_idx].harman_silent_check = false;
    }
    else if (app_db.br_link[set_idx].harman_silent_check != silent_check)
    {
        if ((app_db.br_link[set_idx].harman_silent_check == true) &&
            (silent_check == false) &&
            (set_idx == app_multilink_get_active_idx()))
        {
            //active silent->un_silent need update
            au_set_harman_aling_active_idx(app_multilink_get_active_idx());
        }
        app_db.br_link[set_idx].harman_silent_check = silent_check;
    }
}

void au_set_record_a2dp_active_ever(bool res)
{
    is_record_a2dp_active_ever = res;
    //APP_PRINT_TRACE1("au_set_record_a2dp_active_ever=%d", is_record_a2dp_active_ever);
}

bool au_get_record_a2dp_active_ever(void)
{
    //APP_PRINT_TRACE1("au_get_record_a2dp_active_ever=%d", is_record_a2dp_active_ever);
    return is_record_a2dp_active_ever;
}

void au_set_power_on_link_back_fg(bool res)
{
    power_on_link_back_fg = res;
//    APP_PRINT_TRACE1("au_set_power_on_link_back_fg=%d", power_on_link_back_fg);
}

bool au_get_power_on_link_back_fg(void)
{
//    APP_PRINT_TRACE1("au_get_power_on_link_back_fg=%d", power_on_link_back_fg);
    return power_on_link_back_fg;
}

void au_connect_idle_to_power_off(T_CONNECT_IDLE_POWER_OFF action, uint8_t index)
{
    APP_PRINT_INFO4("au_connect_idle_harman_power_off_option action =%d ,index =%d, en=%d, timer=%d",
                    action,
                    index,
                    app_cfg_nv.auto_power_off_status,
                    app_cfg_nv.auto_power_off_time);

    if (action == CONNECT_IDLE_POWER_OFF_START)
    {
        if (app_cfg_nv.auto_power_off_status)
        {
            if (timer_handle_harman_power_off_option != NULL)
            {
                gap_stop_timer(&timer_handle_harman_power_off_option);
            }
            gap_start_timer(&timer_handle_harman_power_off_option, "harman_power_off_option",
                            multilink_timer_queue_id,
                            AU_TIMER_HARMAN_POWER_OFF_OPTION, 0,
                            app_cfg_nv.auto_power_off_time * 1000);
        }
    }
    else if (action == DISC_RESET_TO_COUNT)
    {
        uint8_t app_idx;
        for (app_idx = 0; app_idx < MAX_BR_LINK_NUM; app_idx++)
        {
            if ((app_check_b2s_link_by_id(app_idx)) &&
                ((app_db.br_link[app_idx].connected_profile & ALL_PROFILE_MASK) != 0))
            {
//                APP_PRINT_TRACE2("[SD_CHECK]: au_connect_idle_to_power_off find_index %d, sniffmode_fg %d",
//                                 app_idx,
//                                 app_db.br_link[app_idx].acl_link_in_sniffmode_flg);
                break;
            }
        }

        if ((app_cfg_nv.auto_power_off_status) && (app_idx != MAX_BR_LINK_NUM))
        {
            T_APP_BR_LINK *p_link;
            p_link = app_find_br_link(app_db.br_link[app_idx].bd_addr);

            if (timer_handle_harman_power_off_option != NULL)
            {
                gap_stop_timer(&timer_handle_harman_power_off_option);
            }
            if (p_link->acl_link_in_sniffmode_flg)
            {
                gap_start_timer(&timer_handle_harman_power_off_option, "harman_power_off_option",
                                multilink_timer_queue_id,
                                AU_TIMER_HARMAN_POWER_OFF_OPTION, 0,
                                app_cfg_nv.auto_power_off_time * 1000);
            }
        }
    }
    else if (action == ACTIVE_NEED_STOP_COUNT)
    {
        gap_stop_timer(&timer_handle_harman_power_off_option);
    }
}

void au_dump_link_information(void)
{
    uint8_t bd_addr[6];
    uint8_t bond_num;
    uint32_t bond_flag;
    uint32_t plan_profs;

    bond_num = app_b2s_bond_num_get();

    for (uint8_t i = 1; i <= bond_num; i++)
    {
        bond_flag = 0;
        if (app_b2s_bond_addr_get(i, bd_addr))
        {
            if (memcmp(bd_addr, app_cfg_nv.bud_peer_addr, 6) &&
                memcmp(bd_addr, app_cfg_nv.bud_local_addr, 6))
            {
                bt_bond_flag_get(bd_addr, &bond_flag);
            }
        }
        APP_PRINT_INFO3("au_dump_link_information, dump priority: %d, bond_flag: %d, addr: %s",
                        i,
                        bond_flag,
                        TRACE_BDADDR(bd_addr));
    }
}

uint8_t au_ever_link_information(void)
{
    uint8_t bd_addr[6];
    uint8_t count_ever_link = 0;
    uint8_t bond_num = app_b2s_bond_num_get();
    uint32_t bond_flag;
    uint32_t plan_profs;

    for (uint8_t i = 1; i <= bond_num; i++)
    {
        bond_flag = 0;
        if (app_b2s_bond_addr_get(i, bd_addr))
        {
            if (memcmp(bd_addr, app_cfg_nv.bud_peer_addr, 6) &&
                memcmp(bd_addr, app_cfg_nv.bud_local_addr, 6))
            {
                bt_bond_flag_get(bd_addr, &bond_flag);

//                APP_PRINT_INFO3("au_ever_link_information, dump priority: %d, bond_flag: %d, addr: %s",
//                                i,
//                                bond_flag,
//                                TRACE_BDADDR(bd_addr));

                if (bond_flag != 0)
                {
                    count_ever_link++;
                }
            }
        }
    }

//    APP_PRINT_INFO1("au_ever_link_information count =%d", count_ever_link);
    return count_ever_link;
}

void at_pairing_radio_setting(uint8_t enable)
{
    uint8_t pair_mode = enable;
//    APP_PRINT_INFO1("at_pairing_radio_setting %d", pair_mode);
    gap_set_param(GAP_PARAM_BOND_BR_PAIRING_MODE, sizeof(uint8_t), &pair_mode);
    gap_set_pairable_mode();
}

void au_set_harman_already_connect_one(bool res)
{
    //pairing mode need reset
    is_harman_connect_only_one = res;
//    APP_PRINT_TRACE1("au_set_harman_already_connect_one =%d",
//                     is_harman_connect_only_one);
}

bool au_get_harman_already_connect_one(void)
{
//    APP_PRINT_TRACE1("au_get_harman_already_connect_one =%d",
//                     is_harman_connect_only_one);
    return is_harman_connect_only_one;
}
#endif

uint8_t app_multilink_find_other_idx(uint8_t app_idx)
{
    uint8_t another_idx = MAX_BR_LINK_NUM;
    for (uint8_t i = 0; i < MAX_BR_LINK_NUM; i++)
    {
        if (i != app_idx &&
            app_db.br_link[i].connected_profile & (A2DP_PROFILE_MASK | HFP_PROFILE_MASK | HSP_PROFILE_MASK))
        {
            another_idx = i;
            break;
        }
    }
    return another_idx;
}

bool app_multilink_sass_bitmap(uint8_t app_idx, uint8_t profile, bool a2dp_check)
{
    uint8_t another_idx = app_multilink_find_other_idx(app_idx);
    bool ret = false;

    if (another_idx == MAX_BR_LINK_NUM || profile == MULTI_FORCE_PREEM ||
        app_idx == app_multilink_get_active_idx())
    {
        ret = true;
    }
    else if (profile == MULTI_A2DP_PREEM || profile == MULTI_AVRCP_PREEM)
    {

        APP_PRINT_TRACE3("app_multilink_sass_bitmap: profile: %d, avrcp_play_status %d, stream %d",
                         profile,
                         app_db.br_link[app_idx].avrcp_play_status,
                         app_db.br_link[app_idx].streaming_fg);
        if (profile == MULTI_AVRCP_PREEM && app_db.br_link[app_idx].streaming_fg == false)
        {
            ret = false;
        }
        else if (((app_db.br_link[app_idx].avrcp_play_status == BT_AVRCP_PLAY_STATUS_PLAYING) ||
                  multilink_silence_packet_timer != NULL ||
                  a2dp_check ||
                  ((app_db.br_link[another_idx].streaming_fg == false &&
                    app_db.br_link[another_idx].call_status == BT_HFP_CALL_IDLE) &&
                   (app_db.br_link[app_idx].avrcp_play_status != BT_AVRCP_PLAY_STATUS_PLAYING))) &&
                 app_multilink_check_pend())
        {
            if (app_db.br_link[another_idx].call_status != BT_HFP_CALL_IDLE)
            {
                if (app_db.br_link[app_idx].call_status != BT_HFP_CALL_IDLE) //hfp but has a2dp
                {
                    if (app_hfp_get_active_hf_index() == app_idx)
                    {
                        ret = true;
                    }
                    else
                    {
                        ret = app_multilink_sass_bitmap(app_idx, MULTI_HFP_PREEM, false);
                    }
                }
                else if (app_db.br_link[another_idx].call_status == BT_HFP_VOICE_ACTIVATION_ONGOING)
                {
                    ret = app_cfg_nv.sass_preemptive & sass_a2dp_va;
                }
                else
                {
                    ret = app_cfg_nv.sass_preemptive & sass_a2dp_sco;
                }
            }
            else if (app_db.br_link[another_idx].streaming_fg)
            {
#if F_APP_HARMAN_FEATURE_SUPPORT
                if (app_get_active_a2dp_idx() == app_idx ||
                    multilink_silence_packet_timer != NULL ||
                    app_db.br_link[another_idx].avrcp_play_status == BT_AVRCP_PLAY_STATUS_PAUSED ||
                    app_db.br_link[another_idx].avrcp_play_status == BT_AVRCP_PLAY_STATUS_STOPPED)
#else
                if (app_get_active_a2dp_idx() == app_idx || multilink_silence_packet_timer != NULL)
#endif
                {
                    ret = true;
                }
                else
                {
                    ret = app_cfg_nv.sass_preemptive & sass_a2dp_a2dp;
                }
            }
            else // other idle
            {
#if F_APP_HARMAN_FEATURE_SUPPORT
                if (another_idx == pending_id)
                {
                    ret = false;
                }
                else
                {
                    ret = true;
                }
#else
                ret = true;
#endif
            }
        }
        else
        {
            ret = false;
        }
    }
    else if (profile == MULTI_HFP_PREEM)
    {
        APP_PRINT_TRACE5("app_multilink_sass_bitmap VA: app_idx_status = %d %d, another_idx_status = %d %d, hfp_active_idx = %d",
                         app_idx,
                         app_db.br_link[app_idx].call_status,
                         another_idx,
                         app_db.br_link[another_idx].call_status,
                         app_hfp_get_active_hf_index());
        if (app_db.br_link[another_idx].call_status == BT_HFP_VOICE_ACTIVATION_ONGOING)
        {
            if (app_hfp_get_active_hf_index() == app_idx)
            {
                ret = true;
            }
            else if (app_db.br_link[app_idx].call_status == BT_HFP_VOICE_ACTIVATION_ONGOING)
            {
                ret = app_cfg_nv.sass_preemptive & sass_va_va;
            }
            else if (app_db.br_link[app_idx].call_status != BT_HFP_CALL_IDLE)
            {
                ret = app_cfg_nv.sass_preemptive & sass_sco_va;
            }
            else // wait hfp
            {
                ret = false;
            }
        }
        else if (app_db.br_link[another_idx].call_status != BT_HFP_CALL_IDLE)
        {
            if (app_hfp_get_active_hf_index() == app_idx)
            {
                ret = true;
            }
            else if (app_db.br_link[app_idx].call_status == BT_HFP_VOICE_ACTIVATION_ONGOING)
            {
                ret = app_cfg_nv.sass_preemptive & sass_va_sco;
            }
            else if (app_db.br_link[app_idx].call_status != BT_HFP_CALL_IDLE)
            {
                ret = app_cfg_nv.sass_preemptive & sass_sco_sco;
            }
            else // wait hfp
            {
                ret = false;
            }
        }
        else if (app_db.br_link[another_idx].streaming_fg)
        {
            if (app_db.br_link[app_idx].call_status == BT_HFP_VOICE_ACTIVATION_ONGOING)
            {
                ret = app_cfg_nv.sass_preemptive & sass_va_a2dp;
            }
            else if (app_db.br_link[app_idx].call_status != BT_HFP_CALL_IDLE)
            {
                ret = app_cfg_nv.sass_preemptive & sass_sco_a2dp;
            }
            else // wait hfp
            {
                ret = false;
            }
        }
        else //other idle
        {
            ret = true;
        }
    }

    APP_PRINT_TRACE4("app_multilink_sass_bitmap: type = %d, idx = %d, active_idx = %d, ret = %d",
                     profile,
                     app_idx,
                     app_multilink_get_active_idx(),
                     ret);
    return ret;
}

#if F_APP_MUTILINK_VA_PREEMPTIVE
static void app_multi_voice_ongoing_preemptive(uint8_t disable_idx, uint8_t idx)
{
#if (F_APP_AVP_INIT_SUPPORT == 1)
    if (app_db.br_link[disable_idx].connected_profile & AVP_PROFILE_MASK)
    {
        app_avp_voice_recognition_disable_req(app_db.br_link[disable_idx].bd_addr);
    }
    else
#endif
    {
        bt_hfp_voice_recognition_disable_req(app_db.br_link[disable_idx].bd_addr);

        va_wait_from_va_sniffing_stop = true;
        pending_va_index_from_va = idx;
        pending_va_sniffing_type = 0xFF;
        gap_stop_timer(&multilink_disc_va_sco);
        gap_start_timer(&multilink_disc_va_sco, "va_sco_disc", multilink_timer_queue_id,
                        MULTILINK_DISC_VA_SCO, disable_idx, 1500);
    }
}
#endif

bool app_multilink_preemptive_judge(uint8_t app_idx, uint8_t type)
{
    bool ret = app_multilink_sass_bitmap(app_idx, type, false);
    uint8_t org_type = type;

    if (type == MULTI_FORCE_PREEM)
    {
        if (app_db.br_link[app_idx].streaming_fg)
        {
            type = MULTI_A2DP_PREEM;
        }
        else if (app_db.br_link[app_idx].sco_handle)
        {
            type = MULTI_HFP_PREEM;
        }
    }

    APP_PRINT_TRACE2("app_multilink_preemptive_judge: ret = %d, type = %d", ret, type);

    uint8_t another_idx = app_multilink_find_other_idx(app_idx);

    APP_PRINT_TRACE6("app_multilink_preemptive_judge: active_a2dp_idx: %d, %d, another_idx: %d, %d, app_idx: %d, %d",
                     active_a2dp_idx, app_db.br_link[active_a2dp_idx].streaming_fg,
                     another_idx, app_db.br_link[another_idx].streaming_fg,
                     app_idx, app_db.br_link[app_idx].streaming_fg);
    if (type == MULTI_A2DP_PREEM)
    {
        if (ret)
        {
            if (active_a2dp_idx != app_idx)
            {
                gap_stop_timer(&multilink_silence_packet_timer);
                app_pause_inactive_a2dp_link_stream(app_idx, false);
                app_multilink_stream_avrcp_set(app_idx);
                if (!app_bt_sniffing_start(app_db.br_link[active_a2dp_idx].bd_addr, BT_SNIFFING_TYPE_A2DP))
                {

                }
            }
            else
            {
                if (app_db.br_link[app_idx].avrcp_play_status != BT_AVRCP_PLAY_STATUS_PLAYING)
                {
                    app_db.br_link[app_idx].avrcp_play_status = BT_AVRCP_PLAY_STATUS_PLAYING;
                    stream_only[app_idx] = true;
                }

                if ((app_cfg_const.enable_multi_link) &&
                    (app_find_b2s_link_num() == MULTILINK_SRC_CONNECTED))
                {
                    app_pause_inactive_a2dp_link_stream(app_idx, false);
                }

                if (app_db.br_link[active_a2dp_idx].streaming_fg == true)
                {
                    app_bond_set_priority(app_db.br_link[active_a2dp_idx].bd_addr);
                }
            }
        }
        else
        {
            if (app_db.br_link[another_idx].streaming_fg == true)
            {
                if ((app_db.br_link[active_a2dp_idx].streaming_fg == false) && app_multilink_check_pend())
                {
                    active_a2dp_idx = another_idx;

                    app_a2dp_active_link_set(app_db.br_link[another_idx].bd_addr);
                    app_bond_set_priority(app_db.br_link[another_idx].bd_addr);
                }
                else
                {
                    if (app_db.active_media_paused)
                    {
                        // A2DP Paused
                        app_db.a2dp_wait_a2dp_sniffing_stop = true;
                        app_db.pending_a2dp_idx = app_idx;
                    }
                    else
                    {
#if F_APP_ENABLE_PAUSE_SECOND_LINK
                        if ((app_db.br_link[app_idx].avrcp_play_status == BT_AVRCP_PLAY_STATUS_PLAYING) &&
                            (app_db.br_link[another_idx].avrcp_play_status == BT_AVRCP_PLAY_STATUS_PLAYING))
                        {
                            bt_avrcp_pause(app_db.br_link[app_idx].bd_addr);
                            audio_track_stop(app_db.br_link[app_idx].a2dp_track_handle);
                            app_db.br_link[app_idx].avrcp_play_status = BT_AVRCP_PLAY_STATUS_PAUSED;
                        }
#endif
                    }
                }
            }
            else if (app_db.br_link[another_idx].call_status != BT_HFP_CALL_IDLE)
            {
                if (app_cfg_const.enable_multi_sco_disc_resume)
                {
                    app_pause_inactive_a2dp_link_stream(another_idx, true);
                }
                else
                {
                    app_pause_inactive_a2dp_link_stream(another_idx, false);
                }
            }
        }
    }
    else if (type == MULTI_AVRCP_PREEM)
    {
        if (ret)
        {
            APP_PRINT_TRACE3("JUDGE_EVENT_MEDIAPLAYER_PLAYING: preemptive active_a2dp_idx %d, app_idx %d, streaming_fg %d",
                             active_a2dp_idx, app_idx, app_db.br_link[app_idx].streaming_fg);
            if (active_a2dp_idx == app_idx)
            {
                app_db.active_media_paused = false;
            }

            if (app_db.br_link[app_idx].streaming_fg == true)
            {
                app_a2dp_active_link_set(app_db.br_link[app_idx].bd_addr);
                app_pause_inactive_a2dp_link_stream(app_idx, false);
                app_bond_set_priority(app_db.br_link[app_idx].bd_addr);
            }
        }
        else
        {
            if (app_db.br_link[another_idx].streaming_fg == true)
            {
#if GFPS_SASS_SUPPORT
                if (app_sass_policy_support_easy_connection_switch())
                {
                    if (app_multilink_pause_inactive_for_sass() == false)
                    {
#if F_APP_ENABLE_PAUSE_SECOND_LINK
                        if ((app_db.br_link[another_idx].avrcp_play_status == BT_AVRCP_PLAY_STATUS_PLAYING))
                        {
                            bt_avrcp_pause(app_db.br_link[app_idx].bd_addr);
                            audio_track_stop(app_db.br_link[app_idx].a2dp_track_handle);
                            app_db.br_link[app_idx].avrcp_play_status = BT_AVRCP_PLAY_STATUS_PAUSED;
                        }
#endif
                    }
                }
                else
#endif
                {
#if F_APP_ENABLE_PAUSE_SECOND_LINK
                    if (app_db.br_link[another_idx].avrcp_play_status == BT_AVRCP_PLAY_STATUS_PLAYING)
                    {
                        bt_avrcp_pause(app_db.br_link[app_idx].bd_addr);
                        audio_track_stop(app_db.br_link[app_idx].a2dp_track_handle);
                        app_db.br_link[app_idx].avrcp_play_status = BT_AVRCP_PLAY_STATUS_PAUSED;

                    }
#endif
                }
            }
        }
    }
    else if (type == MULTI_HFP_PREEM)
    {
        if (ret)
        {
            if (app_cfg_const.enable_multi_sco_disc_resume)
            {
                app_pause_inactive_a2dp_link_stream(app_idx, true);
            }
            else
            {
                app_pause_inactive_a2dp_link_stream(app_idx, false);
            }

            if (app_db.br_link[app_idx].sco_track_handle)
            {
                audio_track_start(app_db.br_link[app_idx].sco_track_handle);
            }
        }
        else if (app_db.br_link[another_idx].streaming_fg)
        {
            if (app_db.br_link[app_idx].sco_handle)// && app_db.br_link[app_idx].call_status == BT_HFP_VOICE_ACTIVATION_ONGOING)
            {
                legacy_vendor_set_active_sco(app_db.br_link[app_idx].sco_handle, 0, 2);
            }
            app_db.a2dp_preemptive_flag = true;
            app_bt_sniffing_param_update(APP_BT_SNIFFING_EVENT_MULTILINK_CHANGE);
        }
    }

    if (ret)
    {
        app_multilink_set_active_idx(app_idx);
        if (app_db.br_link[another_idx].call_status != BT_HFP_CALL_IDLE ||
            app_db.br_link[another_idx].sco_handle)
        {
            if (app_db.br_link[another_idx].sco_handle &&
                app_db.br_link[app_idx].streaming_fg)
            {
                legacy_vendor_set_active_sco(app_db.br_link[another_idx].sco_handle, 0, 2);
            }

            if (app_db.br_link[another_idx].call_status == BT_HFP_VOICE_ACTIVATION_ONGOING)
            {
                app_multi_voice_ongoing_preemptive(another_idx, app_idx);
            }
            else
            {
                app_multi_active_hfp_transfer(app_idx, true, org_type == MULTI_FORCE_PREEM);
            }
        }
    }
    return ret;
}

uint8_t app_multilink_get_active_idx(void)
{
    return active_idx;
}

void app_multilink_set_active_idx(uint8_t idx)
{
    APP_PRINT_TRACE2("app_multilink_set_active_idx: %d, %d", active_idx, idx);
    if (active_idx != idx)
    {
        active_idx = idx;
        if (idx < MAX_BR_LINK_NUM)
        {
            memcpy(app_db.active_hfp_addr, app_db.br_link[idx].bd_addr, 6);
        }
        else
        {
            memset(app_db.active_hfp_addr, 0, 6);
        }
        if (app_db.remote_session_state == REMOTE_SESSION_STATE_CONNECTED)
        {
            app_relay_async_single(APP_MODULE_TYPE_MULTI_LINK, APP_REMOTE_MSG_ACTIVE_BD_ADDR_SYNC);
        }
    }
#if GFPS_SASS_SUPPORT
    if (app_sass_policy_support_easy_connection_switch())
    {
        app_gfps_notify_conn_status();
        if (app_db.br_link[idx].call_status != BT_HFP_CALL_IDLE)
        {
            app_gfps_rfc_notify_multipoint_switch(idx, GFPS_MULTIPOINT_SWITCH_REASON_HFP);
        }
        else if (app_db.br_link[idx].streaming_fg)
        {
            app_gfps_rfc_notify_multipoint_switch(idx, GFPS_MULTIPOINT_SWITCH_REASON_A2DP);
        }
        else
        {
            app_gfps_rfc_notify_multipoint_switch(idx, GFPS_MULTIPOINT_SWITCH_REASON_UNKNOWN);
        }
    }
#endif
}

#if F_APP_HARMAN_FEATURE_SUPPORT
void multilink_notify_deviceinfo(uint8_t conn_id, uint32_t time)
{
    T_APP_LE_LINK *p_link;

    p_link = app_find_le_link_by_conn_id(conn_id);
    if (p_link != NULL)
    {
        APP_PRINT_INFO3("multilink_notify_deviceinfo: conn_id: %d, transmit_srv_tx_enable_fg: %d, encryption_status: %d",
                        conn_id, p_link->transmit_srv_tx_enable_fg, p_link->encryption_status);
        if ((p_link->transmit_srv_tx_enable_fg & TX_ENABLE_CCCD_BIT) &&
            (p_link->encryption_status == LE_LINK_ENCRYPTIONED))
        {
            gap_start_timer(&multilink_notify_deviceinfo_timer, "harman_notify_deviceinfo",
                            multilink_timer_queue_id, HARMAN_NOTIFY_DEVICEINFO,
                            0, time);
        }
    }
}

void multilink_notify_deviceinfo_stop(void)
{
    if (multilink_notify_deviceinfo_timer != NULL)
    {
        //APP_PRINT_TRACE0("multilink_notify_deviceinfo_stop");
        gap_stop_timer(&multilink_notify_deviceinfo_timer);
    }
}
#endif

int8_t app_multilink_get_available_connection_num(void)
{
    uint8_t b2s_link_num = app_bt_policy_get_b2s_connected_num();
    //check NV
    uint8_t max_link_num = app_cfg_const.enable_multi_link ? app_cfg_const.max_legacy_multilink_devices
                           : 1;
    int8_t ret = max_link_num - b2s_link_num;
    APP_PRINT_TRACE1("app_multilink_get_available_connection_num %d", ret);
    return ret;
}
uint8_t app_multilink_get_inactive_index(uint8_t new_link_idx, uint8_t call_num, bool force)
{
    uint8_t inactive_index = MAX_BR_LINK_NUM;

#if GFPS_SASS_SUPPORT
    if (app_sass_policy_support_easy_connection_switch())
    {
        inactive_index = app_sass_policy_get_disc_link();

        if (inactive_index != MAX_BR_LINK_NUM)
        {
            return inactive_index;
        }
    }
#endif

    for (uint8_t i = 0; i < MAX_BR_LINK_NUM; i++)
    {
        if ((i != app_multilink_get_active_idx() &&
             i != new_link_idx &&
             (app_db.br_link[i].connected_profile & (A2DP_PROFILE_MASK | AVRCP_PROFILE_MASK | HFP_PROFILE_MASK |
                                                     HSP_PROFILE_MASK))))
        {
            inactive_index = i;
            break;
        }
    }

    if (inactive_index == MAX_BR_LINK_NUM && (app_multilink_get_available_connection_num() == 0 ||
                                              force))
    {
        inactive_index = app_multilink_get_active_idx();
    }


    return inactive_index;
}

#if GFPS_SASS_SUPPORT
bool app_multilink_get_stream_only(uint8_t idx)
{
    if (idx < MAX_BR_LINK_NUM)
    {
        return stream_only[idx];
    }
    return false;
}

bool app_multilink_pause_inactive_for_sass(void)
{
    uint8_t active_idx = app_multilink_get_active_idx();
    uint8_t inactive_idx  = app_multilink_find_other_idx(active_idx);
    uint8_t ret = false;
    APP_PRINT_TRACE4("app_multilink_pause_inactive_for_sass active %d,key 0x%x, inactive %d, key 0x%x",
                     active_idx,
                     app_db.br_link[active_idx].gfps_inuse_account_key,
                     inactive_idx,
                     app_db.br_link[inactive_idx].gfps_inuse_account_key);
    if ((app_db.br_link[inactive_idx].gfps_inuse_account_key != 0xff) &&
        (app_db.br_link[active_idx].gfps_inuse_account_key != 0xff))
    {
        //both sass device, do not pause directly
        ret = true;
    }
    else if ((app_db.br_link[inactive_idx].gfps_inuse_account_key == 0xff) &&
             (app_db.br_link[active_idx].gfps_inuse_account_key == 0xff))
    {
        //both non-sass, do not pause, follow orignial behavior
    }
    else
    {
        //non-sass and sass, pause directly
        ret = true;
        bt_avrcp_pause(app_db.br_link[inactive_idx].bd_addr);
        app_db.br_link[inactive_idx].avrcp_play_status = BT_AVRCP_PLAY_STATUS_PAUSED;
    }
    return ret;
}
#endif

bool app_multilink_get_in_silence_packet(void)
{
    return multilink_silence_packet_timer != NULL;
}

#if F_APP_MUTILINK_TRIGGER_HIGH_PRIORITY
uint8_t app_multilink_get_highest_priority_index(uint32_t profile_mask)
{
    uint8_t app_idx = MAX_BR_LINK_NUM;
    uint8_t link_num = app_connected_profile_link_num(profile_mask);
    uint8_t bd_addr[6];
    uint32_t bond_flag;
    uint8_t bond_num = bt_bond_num_get();
    for (uint8_t i = 1; i <= bond_num; i++)
    {
        if (bt_bond_addr_get(i, bd_addr) == false)
        {
            break;
        }
        bt_bond_flag_get(bd_addr, &bond_flag);
        for (uint8_t j = 0; j < MAX_BR_LINK_NUM; j++)
        {
            if ((memcmp(bd_addr, app_db.br_link[j].bd_addr, 6) == 0) &&
                (app_db.br_link[j].connected_profile & profile_mask))
            {
                app_idx = j;
                break;
            }
        }

        if (app_idx != MAX_BR_LINK_NUM)
        {
            break;
        }
    }
    return app_idx;
}
#endif
