/*
 * Copyright (c) 2020, Realsil Semiconductor Corporation. All rights reserved.
 */
#if F_APP_SINGLE_MUTLILINK_SCENERIO_1
#include "string.h"
#include "stdlib.h"
#include "os_mem.h"
#include "os_sched.h"
#include "gap_timer.h"
#include "trace.h"
#include "app_cfg.h"
#include "sysm.h"
#include "app_link_util.h"
#include "app_main.h"
#include "bt_hfp.h"
#include "btm.h"
#include "asp_device_link.h"
#include "app_multilink.h"
#include "app_single_multilink_customer.h"
#include "app_hfp.h"
#include "app_bt_policy_api.h"
#include "app_usb_audio.h"
#include "app_usb_audio_hid.h"
#include "app_audio_policy.h"
#include "app_teams_audio_policy.h"
#include "app_teams_bt_policy.h"
#include "app_teams_extend_led.h"
#include "app_mmi.h"
#include "app_usb_mmi.h"

static T_BT_HFP_CALL_STATUS app_teams_call_status = BT_HFP_CALL_IDLE;
static T_APP_JUDGE_A2DP_EVENT app_teams_music_status = JUDGE_EVENT_A2DP_DISC;
static bool app_teams_record_status = false;
static T_APP_TEAMS_MULTILINK_PRIORITY_DATA
app_teams_multilink_device_priority_mgr[APP_TEAMS_MAX_LINK_NUM] = {0};
static T_APP_TEAMS_MULTILINK_APP_HIGH_PRIORITY_ARRAY_DATA
app_teams_multilink_app_high_priority_mgr[APP_TEAMS_MAX_LINK_NUM] = {0};
static T_APP_TEAMS_MULTILINK_APP_LOW_PRIORITY_ARRAY_DATA
app_teams_multilink_app_low_priority_mgr[APP_TEAMS_MAX_LINK_NUM] = {0};
static uint8_t teams_active_device_idx = APP_TEAMS_MAX_LINK_NUM;
static uint8_t teams_active_voice_idx = APP_TEAMS_MAX_LINK_NUM;
static uint8_t teams_active_music_idx = APP_TEAMS_MAX_LINK_NUM;
static uint8_t teams_active_record_idx = APP_TEAMS_MAX_LINK_NUM;
static uint8_t teams_multilink_app_low_priority_active_index = APP_TEAMS_MAX_LINK_NUM;
static uint8_t teams_music_resume_mask = 0;
static uint8_t teams_record_resume_mask = 0;
//static uint8_t teams_resume_music_index = APP_TEAMS_MAX_LINK_NUM;
static uint8_t teams_usb_stream_status = 0;
static uint8_t teams_multilink_timer_queue_id = 0;
// static void *teams_multilink_music_playing_check_index = NULL;
static bool teams_usb_music_data_stream_running = false;
static bool teams_usb_recorder_running = false;
static void *teams_usb_timer_handle_hfp_ring = NULL;
static T_APP_TEAMS_MULTILINK_USB_RING_TYPE teams_usb_running_ring_type =
    T_APP_TEAMS_MULTILINK_USB_INVALID_RING;
bool teams_wl_mic_stream_running = false;
bool teams_wl_audio_stream_running = false;

uint8_t multilink_usb_addr[6] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff};
uint8_t multilink_usb_idx = 0xff;
T_APP_TEAMS_MULTILINK_APP_HIGH_PRIORITY_ARRAY_DATA
*app_teams_multilink_find_app_high_priority_array_data_by_link_idx(
    uint8_t index);

T_APP_TEAMS_MULTILINK_APP_LOW_PRIORITY_ARRAY_DATA
*app_teams_multilink_find_app_low_priority_array_data_by_link_idx(
    uint8_t index);

void app_teams_wl_audio_stream_notify(bool is_stream_running)
{
    if (teams_wl_audio_stream_running != is_stream_running)
    {
        teams_wl_audio_stream_running = is_stream_running;
        //call api to notify host about audio stream
    }
}

void app_teams_wl_mic_stream_notify(bool is_stream_running)
{
    if (teams_wl_mic_stream_running != is_stream_running)
    {
        teams_wl_mic_stream_running = is_stream_running;
        //call api to notify host about mic stream
    }
}

void app_teams_multilink_report_app_change_event(T_APP_TEAMS_MULTILINK_APP_CHANGE_EVENT event)
{
    APP_PRINT_TRACE3("app_teams_multilink_report_app_change_event 1: event %d, teams_active_device_idx %d, active device idx %d",
                     event, teams_active_device_idx, app_teams_multilink_get_active_index());
    if (event == T_APP_TEAMS_MULTILINK_APP_VOICE_ACTIVE_IDX_CHANGE)
    {
        //voice active index change
    }
    else if (event == T_APP_TEAMS_MULTILINK_APP_MUSIC_ACTIVE_IDX_CHANGE)
    {
        //music active index change
    }
    else if (event == T_APP_TEAMS_MULTILINK_APP_RECORD_ACTIVE_IDX_CHANGE)
    {
        //record active index change
    }
    if (event == T_APP_TEAMS_MULTILINK_APP_VOICE_STATUS_CHANGE)
    {
        //device voice status change
    }
    else if (event == T_APP_TEAMS_MULTILINK_APP_MUSIC_STATUS_CHANGE)
    {
        //device music status change
    }
    else if (event == T_APP_TEAMS_MULTILINK_APP_RECORD_STATUS_CHANGE)
    {
        //device record status change
    }
    else if (event == T_APP_TEAMS_MULTILINK_APP_LINK_STATUS_CHANGE)
    {
        //link status change
    }
#if F_APP_TEAMS_GLOBAL_MUTE_SUPPORT
    if (teams_active_device_idx != app_teams_multilink_get_active_index())
    {
        teams_active_device_idx = app_teams_multilink_get_active_index();

        if (teams_active_device_idx == 0x03)
        {
            return;
        }

        bool link_is_mute;
#if F_APP_USB_AUDIO_SUPPORT
        if (teams_active_device_idx == 0xff)
        {
            link_is_mute = app_usb_audio_is_mic_mute();
        }
        else
#endif
        {
            link_is_mute = app_db.br_link[teams_active_device_idx].is_mute;
        }

        bool global_is_mute = app_teams_audio_get_global_mute_status();
        APP_PRINT_TRACE3("app_teams_multilink_report_app_change_event 2: new active idx 0x%1x, link_is_mute %d, global_is_mute %d",
                         teams_active_device_idx, link_is_mute, global_is_mute);
        if (link_is_mute != global_is_mute)
        {
            if (link_is_mute)
            {
                app_teams_audio_set_global_mute_status(1);
                app_audio_tone_type_play(TONE_MIC_MUTE_ON, true, true);
                if (teams_active_device_idx != 0xff)
                {
                    app_audio_set_mic_mute_status(1);
                }
            }
            else
            {
                // follow global mute status, only need to handle global mute
#if F_APP_USB_AUDIO_SUPPORT
                if (teams_active_device_idx == 0xff)
                {
                    app_usb_audio_sync_mute_status_handle();
                }
                else
#endif
                {
                    app_teams_audio_sync_mute_status_handle();
                }
            }
        }
    }
#endif
}

void app_teams_multilink_print_priority_mgr_data(T_APP_TEAMS_MULTILINK_APP_TYPE app_type)
{
    if (app_type == T_APP_TEAMS_MULTILINK_APP_TYPE_VOICE)
    {
        //for (uint8_t i = 0; i<APP_TEAMS_MAX_LINK_NUM; i++)
        //{
        APP_PRINT_TRACE6("app_teams_multilink_print_priority_mgr_data: voice status: index0 %d, status %d, index1 %d, status %d, index2 %d, status %d",
                         app_teams_multilink_app_high_priority_mgr[0].idx,
                         app_teams_multilink_app_high_priority_mgr[0].voice_status,
                         app_teams_multilink_app_high_priority_mgr[1].idx,
                         app_teams_multilink_app_high_priority_mgr[1].voice_status,
                         app_teams_multilink_app_high_priority_mgr[2].idx,
                         app_teams_multilink_app_high_priority_mgr[2].voice_status);
        //}
    }
    else if (app_type == T_APP_TEAMS_MULTILINK_APP_TYPE_MUSIC)
    {
        APP_PRINT_TRACE6("app_teams_multilink_print_priority_mgr_data: music status: index0 %d, status %d, index1 %d, status %d, index2 %d, status %d",
                         app_teams_multilink_app_low_priority_mgr[0].idx,
                         app_teams_multilink_app_low_priority_mgr[0].music_event,
                         app_teams_multilink_app_low_priority_mgr[1].idx,
                         app_teams_multilink_app_low_priority_mgr[1].music_event,
                         app_teams_multilink_app_low_priority_mgr[2].idx,
                         app_teams_multilink_app_low_priority_mgr[2].music_event);
    }
    else if (app_type == T_APP_TEAMS_MULTILINK_APP_TYPE_RECORD)
    {
        APP_PRINT_TRACE6("app_teams_multilink_print_priority_mgr_data: record status: index0 %d, status %d, index1 %d, status %d, index2 %d, status %d",
                         app_teams_multilink_app_low_priority_mgr[0].idx,
                         app_teams_multilink_app_low_priority_mgr[0].record_status,
                         app_teams_multilink_app_low_priority_mgr[1].idx,
                         app_teams_multilink_app_low_priority_mgr[1].record_status,
                         app_teams_multilink_app_low_priority_mgr[2].idx,
                         app_teams_multilink_app_low_priority_mgr[2].record_status);
    }
}

uint8_t app_teams_multilink_find_newest_active_idx(void)
{
    uint8_t index = APP_TEAMS_MAX_LINK_NUM;
    uint32_t active_time = 0;
    for (uint8_t i = 0; i < APP_TEAMS_MAX_LINK_NUM; i++)
    {
        if (app_teams_multilink_device_priority_mgr[i].app_actived)
        {
            if (app_teams_multilink_device_priority_mgr[i].app_end_time > active_time)
            {
                index = app_teams_multilink_device_priority_mgr[i].idx;
                active_time = app_teams_multilink_device_priority_mgr[i].app_end_time;
            }
        }
    }
    return index;
}

uint8_t app_teams_multilink_find_newest_connected_idx(void)
{
    uint8_t index = APP_TEAMS_MAX_LINK_NUM;
    uint32_t connected_time = 0;
    for (uint8_t i = 0; i < APP_TEAMS_MAX_LINK_NUM; i++)
    {
        if (app_teams_multilink_device_priority_mgr[i].connect_time > connected_time)
        {
            index = app_teams_multilink_device_priority_mgr[i].idx;
            connected_time = app_teams_multilink_device_priority_mgr[i].connect_time;
        }
    }
    return index;
}

uint8_t app_teams_multilink_get_active_index(void)
{
    if (app_teams_call_status != BT_HFP_CALL_IDLE)
    {
        return app_teams_multilink_app_high_priority_mgr[0].idx;
    }
    else if (app_teams_multilink_app_low_priority_mgr[0].music_event == JUDGE_EVENT_A2DP_START ||
             app_teams_multilink_app_low_priority_mgr[0].record_status)
    {
        return app_teams_multilink_app_low_priority_mgr[0].idx;
    }
    else if (app_teams_multilink_find_newest_active_idx() != APP_TEAMS_MAX_LINK_NUM)
    {
        return app_teams_multilink_find_newest_active_idx();
    }
    else
    {
        return app_teams_multilink_find_newest_connected_idx();
    }
}

uint8_t app_teams_multilink_get_second_hfp_priority_index(void)
{
    return app_teams_multilink_app_high_priority_mgr[1].idx;
}

bool app_teams_multilink_check_device_music_is_playing(void)
{
    if (app_teams_call_status)
    {
        return false;
    }
    else
    {
        if (app_teams_multilink_app_low_priority_mgr[0].music_event == JUDGE_EVENT_A2DP_START)
        {
            return true;
        }
    }

    return false;
}

T_BT_HFP_CALL_STATUS app_teams_multilink_get_voice_status(void)
{
    return app_teams_call_status;
}

uint8_t app_teams_multilink_get_active_voice_index(void)
{
    return teams_active_voice_idx;
}

uint8_t app_teams_multilink_get_active_music_index(void)
{
    return teams_active_music_idx;
}

uint8_t app_teams_multilink_get_active_record_index(void)
{
    return teams_active_record_idx;
}

bool app_teams_multiink_get_record_status_by_link_id(uint8_t index)
{
    T_APP_TEAMS_MULTILINK_APP_LOW_PRIORITY_ARRAY_DATA *record_data = NULL;

    record_data = app_teams_multilink_find_app_low_priority_array_data_by_link_idx(index);
    if (record_data)
    {
        return record_data->record_status;
    }
    else
    {
        return false;
    }
}

uint8_t app_teams_multilink_get_active_low_priority_app_index(void)
{
    return teams_multilink_app_low_priority_active_index;
}

void app_teams_multilink_handle_link_connected(uint8_t index)
{
    if (index == multilink_usb_idx)
    {
        app_teams_multilink_device_priority_mgr[APP_TEAMS_MAX_LINK_NUM - 1].connect_time =
            os_sys_time_get();
    }
    else
    {
        app_teams_multilink_device_priority_mgr[index].connect_time = os_sys_time_get();
    }
    app_teams_multilink_report_app_change_event(T_APP_TEAMS_MULTILINK_APP_LINK_STATUS_CHANGE);
}

void app_teams_multilink_handle_link_disconnected(uint8_t index)
{
    if (index == multilink_usb_idx)
    {
        memset(&app_teams_multilink_device_priority_mgr[APP_TEAMS_MAX_LINK_NUM - 1], 0,
               sizeof(T_APP_TEAMS_MULTILINK_PRIORITY_DATA));
        app_teams_multilink_device_priority_mgr[APP_TEAMS_MAX_LINK_NUM - 1].idx = index;
    }
    else
    {
        memset(&app_teams_multilink_device_priority_mgr[index], 0,
               sizeof(T_APP_TEAMS_MULTILINK_PRIORITY_DATA));
        app_teams_multilink_device_priority_mgr[index].idx = index;
    }
    app_teams_multilink_report_app_change_event(T_APP_TEAMS_MULTILINK_APP_LINK_STATUS_CHANGE);
}

bool app_teams_multilink_check_link_connected(uint8_t index)
{
    if (index == multilink_usb_idx)
    {
        if (app_teams_multilink_device_priority_mgr[APP_TEAMS_MAX_LINK_NUM - 1].connect_time)
        {
            return true;
        }
    }
    else
    {
        if (app_teams_multilink_device_priority_mgr[index].connect_time)
        {
            return true;
        }
    }
    return false;
}

void app_teams_multilink_handle_app_active(uint8_t index)
{
    if (index == multilink_usb_idx)
    {
        app_teams_multilink_device_priority_mgr[APP_TEAMS_MAX_LINK_NUM - 1].app_actived = true;
    }
    else
    {
        app_teams_multilink_device_priority_mgr[index].app_actived = true;
    }
}

void app_teams_multilink_handle_app_end(uint8_t index)
{
    if (index == multilink_usb_idx)
    {
        app_teams_multilink_device_priority_mgr[APP_TEAMS_MAX_LINK_NUM - 1].app_end_time =
            os_sys_time_get();
    }
    else
    {
        app_teams_multilink_device_priority_mgr[index].app_end_time = os_sys_time_get();
    }
}

bool app_teams_multilink_check_index_valid(uint8_t index)
{
    if (index >= MAX_BR_LINK_NUM && index != multilink_usb_idx)
    {
        APP_PRINT_ERROR1("app_teams_multilink_check_index_valid: index error: %d", index);
        return false;
    }
    return true;
}

bool app_teams_multilink_check_usb_voice_data_stream_running(void)
{
    T_APP_TEAMS_MULTILINK_APP_HIGH_PRIORITY_ARRAY_DATA *p_voice_priority_data = NULL;
    T_BT_HFP_CALL_STATUS hfp_status;

    p_voice_priority_data = app_teams_multilink_find_app_high_priority_array_data_by_link_idx(
                                multilink_usb_idx);
    if (p_voice_priority_data)
    {
        hfp_status = p_voice_priority_data->voice_status;
        if ((hfp_status == BT_HFP_INCOMING_CALL_ONGOING || hfp_status == BT_HFP_OUTGOING_CALL_ONGOING) &&
            (teams_usb_stream_status & APP_TEAMS_MULTILINK_USB_DOWNSTREAM_ACTIVE))
        {
            return true;
        }
        else if (((hfp_status == BT_HFP_CALL_ACTIVE || hfp_status == BT_HFP_CALL_ACTIVE_WITH_CALL_WAITING ||
                   hfp_status == BT_HFP_CALL_ACTIVE_WITH_CALL_HOLD) &&
                  teams_usb_stream_status == (APP_TEAMS_MULTILINK_USB_DOWNSTREAM_ACTIVE |
                                              APP_TEAMS_MULTILINK_USB_UPSTREAM_ACTIVE)))
        {
            return true;
        }
    }
    return false;
}

bool app_teams_multilink_check_usb_music_data_stream_running(void)
{
    return teams_usb_music_data_stream_running;
}

bool app_teams_multilink_check_usb_record_data_stream_running(void)
{
    return teams_usb_recorder_running;
}

uint8_t app_teams_multilink_find_sco_conn_num(void)
{
    uint8_t bt_num = app_find_sco_conn_num();
    uint8_t usb_num = app_teams_multilink_check_usb_voice_data_stream_running() ? 1 : 0;

    APP_PRINT_TRACE2("app_teams_multilink_find_sco_conn_num: bt_num %d, usb_num %d",
                     bt_num, usb_num);

    return bt_num + usb_num;
}

uint8_t app_teams_multilink_find_app_low_priority_array_index_by_link_idx(uint8_t index)
{
    uint8_t i = 0;
    for (i = 0; i < APP_TEAMS_MAX_LINK_NUM; i++)
    {
        if (app_teams_multilink_app_low_priority_mgr[i].idx == index)
        {
            break;
        }
    }
    return i;
}

uint8_t app_teams_multilink_find_app_high_priority_array_index_by_link_idx(uint8_t index)
{
    uint8_t i = 0;
    for (i = 0; i < APP_TEAMS_MAX_LINK_NUM; i++)
    {
        if (app_teams_multilink_app_high_priority_mgr[i].idx == index)
        {
            break;
        }
    }
    return i;
}

T_APP_TEAMS_MULTILINK_APP_HIGH_PRIORITY_ARRAY_DATA
*app_teams_multilink_find_app_high_priority_array_data_by_link_idx(
    uint8_t index)
{
    for (uint8_t i = 0; i < APP_TEAMS_MAX_LINK_NUM; i++)
    {
        if (app_teams_multilink_app_high_priority_mgr[i].idx == index)
        {
            return &app_teams_multilink_app_high_priority_mgr[i];
        }
    }

    return NULL;
}

T_APP_TEAMS_MULTILINK_APP_LOW_PRIORITY_ARRAY_DATA
*app_teams_multilink_find_app_low_priority_array_data_by_link_idx(
    uint8_t index)
{
    for (uint8_t i = 0; i < APP_TEAMS_MAX_LINK_NUM; i++)
    {
        if (app_teams_multilink_app_low_priority_mgr[i].idx == index)
        {
            return &app_teams_multilink_app_low_priority_mgr[i];
        }
    }

    return NULL;
}

void app_teams_multilink_set_voice_status(T_BT_HFP_CALL_STATUS voice_status)
{
    T_BT_HFP_CALL_STATUS old_status = app_teams_call_status;
    APP_PRINT_TRACE1("app_teams_multilink_set_voice_status: voice_status %d",
                     voice_status);
    app_teams_call_status = voice_status;

    if ((old_status == BT_HFP_CALL_IDLE && voice_status != BT_HFP_CALL_IDLE) ||
        (old_status != BT_HFP_CALL_IDLE && voice_status == BT_HFP_CALL_IDLE))
    {
        app_teams_multilink_report_app_change_event(T_APP_TEAMS_MULTILINK_APP_VOICE_STATUS_CHANGE);
    }
}

void app_teams_multilink_set_music_status(T_APP_JUDGE_A2DP_EVENT music_status)
{
    T_APP_JUDGE_A2DP_EVENT old_status = app_teams_music_status;
    APP_PRINT_TRACE1("app_teams_multilink_set_music_status: music_status %d",
                     music_status);
    app_teams_music_status = music_status;

    if ((old_status == JUDGE_EVENT_A2DP_START && music_status != JUDGE_EVENT_A2DP_START) ||
        (old_status != JUDGE_EVENT_A2DP_START && music_status == JUDGE_EVENT_A2DP_START))
    {
        app_teams_multilink_report_app_change_event(T_APP_TEAMS_MULTILINK_APP_MUSIC_STATUS_CHANGE);
    }
}

void app_teams_multilink_set_record_status(bool record_status)
{
    bool old_status = app_teams_record_status;
    APP_PRINT_TRACE1("app_teams_multilink_set_record_status: record_status %d",
                     record_status);
    app_teams_record_status = record_status;

    if ((old_status && !record_status) ||
        (!old_status && record_status))
    {
        app_teams_multilink_report_app_change_event(T_APP_TEAMS_MULTILINK_APP_RECORD_STATUS_CHANGE);
    }
}

void app_teams_multilink_set_active_voice_index(uint8_t index)
{
    if (teams_active_voice_idx == index)
    {
        return;
    }

    APP_PRINT_TRACE1("app_teams_multilink_set_active_voice_index: index %d", index);
    teams_active_voice_idx = index;
    if (teams_active_voice_idx != multilink_usb_idx)
    {
        app_hfp_only_set_active_hf_index(index);
        app_bond_set_priority(app_db.br_link[teams_active_voice_idx].bd_addr);
    }

    app_teams_multilink_report_app_change_event(T_APP_TEAMS_MULTILINK_APP_VOICE_ACTIVE_IDX_CHANGE);
}

void app_teams_multilink_set_active_music_index(uint8_t index)
{
    if (teams_active_music_idx == index)
    {
        return;
    }

    APP_PRINT_TRACE1("app_teams_multilink_set_active_music_index: index %d", index);
    teams_active_music_idx = index;
    if (teams_active_music_idx != multilink_usb_idx)
    {
        app_set_active_a2dp_idx(index);
        app_bond_set_priority(app_db.br_link[teams_active_music_idx].bd_addr);
    }

    app_teams_multilink_report_app_change_event(T_APP_TEAMS_MULTILINK_APP_MUSIC_ACTIVE_IDX_CHANGE);
}

void app_teams_multilink_set_active_record_index(uint8_t index)
{
    if (teams_active_record_idx == index)
    {
        return;
    }

    APP_PRINT_TRACE1("app_teams_multilink_set_active_record_index: index %d", index);
    teams_active_record_idx = index;
    if (teams_active_record_idx != multilink_usb_idx)
    {
        app_bond_set_priority(app_db.br_link[teams_active_music_idx].bd_addr);
    }

    app_teams_multilink_report_app_change_event(T_APP_TEAMS_MULTILINK_APP_RECORD_ACTIVE_IDX_CHANGE);
}

void app_teams_multilink_swap_app_high_priority_array_data(uint8_t fir_index, uint8_t sec_index)
{
    T_APP_TEAMS_MULTILINK_APP_HIGH_PRIORITY_ARRAY_DATA temp;
    if (fir_index >= APP_TEAMS_MAX_LINK_NUM || sec_index >= APP_TEAMS_MAX_LINK_NUM)
    {
        return;
    }

    memcpy(&temp, &app_teams_multilink_app_high_priority_mgr[fir_index],
           sizeof(T_APP_TEAMS_MULTILINK_APP_HIGH_PRIORITY_ARRAY_DATA));
    memcpy(&app_teams_multilink_app_high_priority_mgr[fir_index],
           &app_teams_multilink_app_high_priority_mgr[sec_index],
           sizeof(T_APP_TEAMS_MULTILINK_APP_HIGH_PRIORITY_ARRAY_DATA));
    memcpy(&app_teams_multilink_app_high_priority_mgr[sec_index], &temp,
           sizeof(T_APP_TEAMS_MULTILINK_APP_HIGH_PRIORITY_ARRAY_DATA));
}

void app_teams_multilink_swap_app_low_priority_array_data(uint8_t fir_index, uint8_t sec_index)
{
    T_APP_TEAMS_MULTILINK_APP_LOW_PRIORITY_ARRAY_DATA temp;
    if (fir_index >= APP_TEAMS_MAX_LINK_NUM || sec_index >= APP_TEAMS_MAX_LINK_NUM)
    {
        return;
    }

    memcpy(&temp, &app_teams_multilink_app_low_priority_mgr[fir_index],
           sizeof(T_APP_TEAMS_MULTILINK_APP_LOW_PRIORITY_ARRAY_DATA));
    memcpy(&app_teams_multilink_app_low_priority_mgr[fir_index],
           &app_teams_multilink_app_low_priority_mgr[sec_index],
           sizeof(T_APP_TEAMS_MULTILINK_APP_LOW_PRIORITY_ARRAY_DATA));
    memcpy(&app_teams_multilink_app_low_priority_mgr[sec_index], &temp,
           sizeof(T_APP_TEAMS_MULTILINK_APP_LOW_PRIORITY_ARRAY_DATA));
}

T_APP_TEAMS_MULTILINK_VOICE_PRIORITY_LEVEL app_teams_multilink_convert_hfp_status_to_priority(
    T_BT_HFP_CALL_STATUS hfp_status)
{
    if (hfp_status == BT_HFP_VOICE_ACTIVATION_ONGOING ||
        hfp_status == BT_HFP_CALL_ACTIVE ||
        hfp_status == BT_HFP_CALL_ACTIVE_WITH_CALL_WAITING ||
        hfp_status == BT_HFP_CALL_ACTIVE_WITH_CALL_HOLD)
    {
        return APP_TEAMS_MULTILINK_VOICE_PRIORITY_HIGH;
    }
    else if (hfp_status == BT_HFP_INCOMING_CALL_ONGOING ||
             hfp_status == BT_HFP_OUTGOING_CALL_ONGOING)
    {
        return APP_TEAMS_MULTILINK_VOICE_PRIORITY_MIDM;
    }
    else
    {
        return APP_TEAMS_MULTILINK_VOICE_PRIORITY_LOW;
    }
}

// 1: sec > fir;
// 0: sec == fir
//-1: sec < fir
int8_t app_teams_multilink_compare_voice_priority(uint8_t fir_index, uint8_t sec_index)
{
    APP_PRINT_TRACE4("app_teams_multilink_compare_voice_priority: fir_index %d, fir_index voice_status %d, sec_index %d, sec_index voice_status %d",
                     fir_index, app_teams_multilink_app_high_priority_mgr[fir_index].voice_status,
                     sec_index, app_teams_multilink_app_high_priority_mgr[sec_index].voice_status);
    T_APP_TEAMS_MULTILINK_VOICE_PRIORITY_LEVEL fir_level =
        app_teams_multilink_convert_hfp_status_to_priority(
            app_teams_multilink_app_high_priority_mgr[fir_index].voice_status);
    T_APP_TEAMS_MULTILINK_VOICE_PRIORITY_LEVEL sec_level =
        app_teams_multilink_convert_hfp_status_to_priority(
            app_teams_multilink_app_high_priority_mgr[sec_index].voice_status);

    if (fir_level < sec_level)
    {
        return 1;
    }
    else if (fir_level == sec_level)
    {
        // in active call scene, second > first
        if (fir_level == APP_TEAMS_MULTILINK_VOICE_PRIORITY_HIGH)
        {
            return 1;
        }
        // incoming or outgoing, second == first
        else if (fir_level == APP_TEAMS_MULTILINK_VOICE_PRIORITY_MIDM)
        {
            // if fir and sec are not active voice link, second > first.
            if (fir_index && sec_index)
            {
                return 1;
            }
        }
        else
        {
            // first device is link disconnect and sec device is link connected
            if (!app_teams_multilink_check_link_connected(
                    app_teams_multilink_app_high_priority_mgr[fir_index].idx) &&
                app_teams_multilink_check_link_connected(app_teams_multilink_app_high_priority_mgr[sec_index].idx))
            {
                return 1;
            }
        }
        return 0;
    }
    else
    {
        return -1;
    }
}

// 1: sec > fir;
// 0: sec == fir
//-1: sec < fir
int8_t app_teams_multilink_compare_app_high_priority_array_data(uint8_t fir_index,
                                                                uint8_t sec_index)
{
    return app_teams_multilink_compare_voice_priority(fir_index, sec_index);
}

// 1: sec > fir;
// 0: sec == fir
//-1: sec < fir
int8_t app_teams_multilink_compare_music_priority(uint8_t fir_index, uint8_t sec_index)
{
    APP_PRINT_TRACE4("app_teams_multilink_compare_music_priority: fir_index %d, fir_index music_event %d, sec_index %d, sec_index music_event %d",
                     fir_index, app_teams_multilink_app_low_priority_mgr[fir_index].music_event,
                     sec_index, app_teams_multilink_app_low_priority_mgr[sec_index].music_event);
    // playing music has highest priority
    if (app_teams_multilink_app_low_priority_mgr[sec_index].music_event == JUDGE_EVENT_A2DP_START &&
        app_teams_multilink_app_low_priority_mgr[fir_index].music_event != JUDGE_EVENT_A2DP_START)
    {
        return 1;
    }
    // disconnected a2dp profile has lowest priority
    else if (app_teams_multilink_app_low_priority_mgr[sec_index].music_event != JUDGE_EVENT_A2DP_DISC &&
             app_teams_multilink_app_low_priority_mgr[fir_index].music_event == JUDGE_EVENT_A2DP_DISC)
    {
        return 1;
    }
    // profile connect and music stop has same priority
    else if ((app_teams_multilink_app_low_priority_mgr[sec_index].music_event == JUDGE_EVENT_A2DP_STOP
              &&
              app_teams_multilink_app_low_priority_mgr[fir_index].music_event == JUDGE_EVENT_A2DP_CONNECTED) ||
             (app_teams_multilink_app_low_priority_mgr[sec_index].music_event == JUDGE_EVENT_A2DP_CONNECTED &&
              app_teams_multilink_app_low_priority_mgr[fir_index].music_event == JUDGE_EVENT_A2DP_STOP))
    {
        return 0;
    }
    else if (app_teams_multilink_app_low_priority_mgr[sec_index].music_event ==
             app_teams_multilink_app_low_priority_mgr[fir_index].music_event)
    {
        // if no voice, active music link can not be grab by other link, other link can grap
        // if voice is not idle, all music can grab
        if ((app_teams_multilink_app_low_priority_mgr[sec_index].music_event == JUDGE_EVENT_A2DP_START) &&
            (fir_index || app_teams_call_status))
        {
            return 1;
        }
        return 0;
    }
    else
    {
        return -1;
    }
}

// 1: sec > fir;
// 0: sec == fir
//-1: sec < fir
int8_t app_teams_multilink_compare_record_priority(uint8_t fir_index, uint8_t sec_index)
{
    APP_PRINT_TRACE4("app_teams_multilink_compare_record_priority: fir_index %d, fir_index record_status %d, sec_index %d, sec_index record_status %d",
                     fir_index, app_teams_multilink_app_low_priority_mgr[fir_index].record_status,
                     sec_index, app_teams_multilink_app_low_priority_mgr[sec_index].record_status);
    // playing record > other status
    if (app_teams_multilink_app_low_priority_mgr[sec_index].record_status &&
        !app_teams_multilink_app_low_priority_mgr[fir_index].record_status)
    {
        return 1;
    }
    else if (app_teams_multilink_app_low_priority_mgr[sec_index].record_status ==
             app_teams_multilink_app_low_priority_mgr[fir_index].record_status)
    {
        // if no voice, active record link can not be grab by other link, other link can grap
        // if voice is not idle, all record can grab
        if (app_teams_multilink_app_low_priority_mgr[sec_index].record_status && (fir_index ||
                                                                                  app_teams_call_status))
        {
            return 1;
        }
        return 0;
    }
    else
    {
        return -1;
    }
}

bool app_teams_multilink_check_force_swap_action_condition(uint8_t fir_index, uint8_t sec_index,
                                                           int8_t *ret)
{
    bool check_ret = false;
    //if active voice device enable record or music, level up the priority of device to highest in low app pri array,
    //and can not be grabed by the other device until voice status end
    if (app_teams_multilink_get_voice_status())
    {
        if (teams_active_voice_idx == sec_index &&
            (app_teams_multilink_app_low_priority_mgr[sec_index].record_status ||
             app_teams_multilink_app_low_priority_mgr[sec_index].music_event == JUDGE_EVENT_A2DP_START))
        {
            APP_PRINT_TRACE5("app_teams_multilink_check_force_swap_condition: force swap, voice status %d, voice index %d, index %d, record status %d, music status %d",
                             app_teams_multilink_get_voice_status(), teams_active_voice_idx, sec_index,
                             app_teams_multilink_app_low_priority_mgr[sec_index].record_status,
                             app_teams_multilink_app_low_priority_mgr[sec_index].music_event);
            *ret =  1;
            check_ret = true;
        }
        else if (teams_active_voice_idx == fir_index &&
                 (app_teams_multilink_app_low_priority_mgr[fir_index].record_status ||
                  app_teams_multilink_app_low_priority_mgr[fir_index].music_event == JUDGE_EVENT_A2DP_START))
        {
            APP_PRINT_TRACE5("app_teams_multilink_check_force_swap_condition: force not swap, voice status %d, voice index %d, index %d, record status %d, music status %d",
                             app_teams_multilink_get_voice_status(), teams_active_voice_idx, fir_index,
                             app_teams_multilink_app_low_priority_mgr[fir_index].record_status,
                             app_teams_multilink_app_low_priority_mgr[fir_index].music_event);
            *ret = -1;
            check_ret = true;
        }
    }
    return check_ret;
}

// 1: sec > fir;
// 0: sec == fir
//-1: sec < fir
int8_t app_teams_multilink_compare_app_low_priority_array_data(uint8_t fir_index, uint8_t sec_index)
{
    int8_t force_ret = 0;
    int8_t music_ret = 0;
    int8_t record_ret = 0;

    if (app_teams_multilink_check_force_swap_action_condition(fir_index, sec_index, &force_ret))
    {
        return force_ret;
    }

    music_ret = app_teams_multilink_compare_music_priority(fir_index, sec_index);
    record_ret = app_teams_multilink_compare_record_priority(fir_index, sec_index);

    //music and record all priority of first device > sec device
    if (music_ret == 1 && record_ret == 1)
    {
        return 1;
    }
    //music priority of first device > sec device, but record priority not have swap and not swap effect
    else if (music_ret == 1 && record_ret == 0 &&
             !app_teams_multilink_app_low_priority_mgr[fir_index].record_status)
    {
        return 1;
    }
    //record priority of first device > sec device, but music priority not have swap and not swap effect
    else if (music_ret == 0 && record_ret == 1 &&
             (app_teams_multilink_app_low_priority_mgr[fir_index].music_event != JUDGE_EVENT_A2DP_START))
    {
        return 1;
    }
    else if (music_ret == 0 && record_ret == 0)
    {
        return 0;
    }
    else
    {
        return -1;
    }
}

void app_teams_multilink_sort_app_high_priority_array(uint8_t index,
                                                      T_APP_TEAMS_MULTILINK_PRIORITY_DIRECTION direction)
{
    uint8_t i = app_teams_multilink_find_app_high_priority_array_index_by_link_idx(index);

    APP_PRINT_TRACE3("app_teams_multilink_sort_app_high_priority_array: index %d, priority_index %d, direction %d",
                     index, i, direction);

    if (i == APP_TEAMS_MAX_LINK_NUM)
    {
        return;
    }

    if (direction == APP_TEAMS_MULTILINK_PRIORITY_UP)
    {
        if (i == 0)
        {
            return;
        }

        while (i > 0 &&
               (app_teams_multilink_compare_app_high_priority_array_data(i - 1, i) > 0))
        {
            app_teams_multilink_swap_app_high_priority_array_data(i - 1, i);
            i--;
        }
    }
    else
    {
        if (i == (APP_TEAMS_MAX_LINK_NUM - 1))
        {
            return;
        }

        while (i < (APP_TEAMS_MAX_LINK_NUM - 1) &&
               (app_teams_multilink_compare_app_high_priority_array_data(i, i + 1) > 0))
        {
            app_teams_multilink_swap_app_high_priority_array_data(i, i + 1);
            i++;
        }
    }
}

void app_teams_multilink_sort_app_low_priority_array(uint8_t index,
                                                     T_APP_TEAMS_MULTILINK_PRIORITY_DIRECTION direction)
{
    uint8_t i = app_teams_multilink_find_app_low_priority_array_index_by_link_idx(index);
    APP_PRINT_TRACE3("app_teams_multilink_sort_app_low_priority_array: index %d, priority index %d, direction %d",
                     index, i, direction);

    if (i == APP_TEAMS_MAX_LINK_NUM)
    {
        return;
    }

    if (direction == APP_TEAMS_MULTILINK_PRIORITY_UP)
    {
        if (i == 0)
        {
            return;
        }

        while (i > 0 &&
               (app_teams_multilink_compare_app_low_priority_array_data(i - 1, i) > 0))
        {
            app_teams_multilink_swap_app_low_priority_array_data(i - 1, i);
            i--;
        }
    }
    else
    {
        if (i == (APP_TEAMS_MAX_LINK_NUM - 1))
        {
            return;
        }

        while (i < (APP_TEAMS_MAX_LINK_NUM - 1) &&
               (app_teams_multilink_compare_app_low_priority_array_data(i, i + 1) > 0))
        {
            app_teams_multilink_swap_app_low_priority_array_data(i, i + 1);
            i++;
        }
    }
}

void app_teams_multilink_sort_app_array(T_APP_TEAMS_MULTILINK_APP_PRIORITY_LEVEL array_type,
                                        uint8_t base_index,
                                        T_APP_TEAMS_MULTILINK_PRIORITY_DIRECTION direction)
{
    if (array_type == T_APP_TEAMS_MULTILINK_APP_PRIORITY_HIGH_LEVEL)
    {
        app_teams_multilink_sort_app_high_priority_array(base_index, direction);
    }
    else
    {
        app_teams_multilink_sort_app_low_priority_array(base_index, direction);
    }
}

void app_teams_multilink_cal_device_multilink_hfp_status(T_BT_HFP_CALL_STATUS active_hf_status,
                                                         T_BT_HFP_CALL_STATUS inactive_hf_status)
{
    switch (active_hf_status)
    {
    case BT_HFP_INCOMING_CALL_ONGOING:
        if (inactive_hf_status >= BT_HFP_CALL_ACTIVE)
        {
            app_teams_multilink_set_voice_status(BT_HFP_MULTILINK_CALL_ACTIVE_WITH_CALL_WAIT);
        }
        else
        {
            app_teams_multilink_set_voice_status(BT_HFP_INCOMING_CALL_ONGOING);
        }
        break;

    case BT_HFP_CALL_ACTIVE:
        if ((inactive_hf_status >= BT_HFP_CALL_ACTIVE) ||
            (inactive_hf_status == BT_HFP_VOICE_ACTIVATION_ONGOING))
        {
            app_teams_multilink_set_voice_status(BT_HFP_MULTILINK_CALL_ACTIVE_WITH_CALL_HOLD);
        }
        else if (inactive_hf_status == BT_HFP_INCOMING_CALL_ONGOING)
        {
            app_teams_multilink_set_voice_status(BT_HFP_MULTILINK_CALL_ACTIVE_WITH_CALL_WAIT);
        }
        else
        {
            app_teams_multilink_set_voice_status(BT_HFP_CALL_ACTIVE);
        }
        break;

    case BT_HFP_VOICE_ACTIVATION_ONGOING:
        if ((inactive_hf_status >= BT_HFP_CALL_ACTIVE) ||
            (inactive_hf_status == BT_HFP_VOICE_ACTIVATION_ONGOING))
        {
            app_teams_multilink_set_voice_status(BT_HFP_MULTILINK_CALL_ACTIVE_WITH_CALL_HOLD);
        }
        else if (inactive_hf_status == BT_HFP_INCOMING_CALL_ONGOING)
        {
            app_teams_multilink_set_voice_status(BT_HFP_MULTILINK_CALL_ACTIVE_WITH_CALL_WAIT);
        }
        else
        {
            app_teams_multilink_set_voice_status(BT_HFP_VOICE_ACTIVATION_ONGOING);
        }
        break;

    default:
        app_teams_multilink_set_voice_status(active_hf_status);
        break;
    }
}

void app_teams_multilink_cal_device_bt_hfp_status(T_BT_HFP_CALL_STATUS active_hf_status,
                                                  T_BT_HFP_CALL_STATUS inactive_hf_status)
{
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
        if ((inactive_hf_status >= BT_HFP_CALL_ACTIVE) ||
            (inactive_hf_status == BT_HFP_VOICE_ACTIVATION_ONGOING))
        {
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

    case BT_HFP_VOICE_ACTIVATION_ONGOING:
        if ((inactive_hf_status >= BT_HFP_CALL_ACTIVE) ||
            (inactive_hf_status == BT_HFP_VOICE_ACTIVATION_ONGOING))
        {
            app_hfp_set_call_status(BT_HFP_MULTILINK_CALL_ACTIVE_WITH_CALL_HOLD);
        }
        else if (inactive_hf_status == BT_HFP_INCOMING_CALL_ONGOING)
        {
            app_hfp_set_call_status(BT_HFP_MULTILINK_CALL_ACTIVE_WITH_CALL_WAIT);
        }
        else
        {
            app_hfp_set_call_status(BT_HFP_VOICE_ACTIVATION_ONGOING);
        }
        break;

    default:
        app_hfp_set_call_status(active_hf_status);
        break;
    }
}

void app_teams_multilink_cal_device_hfp_status(void)
{
    T_BT_HFP_CALL_STATUS active_hf_status = app_teams_multilink_app_high_priority_mgr[0].voice_status;
    T_BT_HFP_CALL_STATUS inactive_hf_status = app_teams_multilink_app_high_priority_mgr[1].voice_status;

    app_teams_multilink_cal_device_multilink_hfp_status(active_hf_status, inactive_hf_status);

    if (app_teams_multilink_app_high_priority_mgr[0].idx != multilink_usb_idx &&
        app_teams_multilink_app_high_priority_mgr[1].idx != multilink_usb_idx)
    {
        app_teams_multilink_cal_device_bt_hfp_status(active_hf_status, inactive_hf_status);
    }
    else if (app_teams_multilink_app_high_priority_mgr[0].idx != multilink_usb_idx)
    {
        app_teams_multilink_cal_device_bt_hfp_status(
            app_teams_multilink_app_high_priority_mgr[0].voice_status,
            app_teams_multilink_app_high_priority_mgr[2].voice_status);
    }
    else
    {
        app_teams_multilink_cal_device_bt_hfp_status(
            app_teams_multilink_app_high_priority_mgr[1].voice_status,
            app_teams_multilink_app_high_priority_mgr[2].voice_status);
    }
}

void app_teams_multilink_stop_all_bt_sco_audio_track(void)
{
    T_APP_BR_LINK *p_link;
    T_AUDIO_TRACK_STATE audio_track_state;
    bool ret = false;

    for (uint8_t i = 0; i < MAX_BR_LINK_NUM; i++)
    {
        p_link = &app_db.br_link[i];
        ret = audio_track_state_get(p_link->sco_track_handle, &audio_track_state);
        if (ret && audio_track_state == AUDIO_TRACK_STATE_STARTED)
        {
            audio_track_stop(p_link->sco_track_handle);
        }
    }
}

void app_teams_multilink_usb_start_alert(T_APP_TEAMS_MULTILINK_USB_RING_TYPE ring_type)
{
    T_APP_AUDIO_TONE_TYPE tone_type;

    if (ring_type == T_APP_TEAMS_MULTILINK_USB_INCOMING_RING)
    {
        tone_type = TONE_HF_CALL_IN;
    }
    else
    {
        tone_type = TONE_HF_OUTGOING_CALL;
    }

    app_audio_tone_type_play(tone_type, false, false);

    if (app_cfg_const.timer_hfp_ring_period != 0)
    {
        teams_usb_running_ring_type = ring_type;
        gap_start_timer(&teams_usb_timer_handle_hfp_ring, "usb_hfp_ring", teams_multilink_timer_queue_id,
                        APP_TEAMS_MULTILINK_HANDLE_USB_ALERT, ring_type, app_cfg_const.timer_hfp_ring_period * 1000);
    }
}

void app_teams_multilink_usb_stop_alert(void)
{
    T_APP_AUDIO_TONE_TYPE tone_type;

    if (teams_usb_running_ring_type != T_APP_TEAMS_MULTILINK_USB_INVALID_RING)
    {
        if (teams_usb_running_ring_type == T_APP_TEAMS_MULTILINK_USB_INCOMING_RING)
        {
            tone_type = TONE_HF_CALL_IN;
        }
        else
        {
            tone_type = TONE_HF_OUTGOING_CALL;
        }

        app_audio_tone_type_cancel(tone_type, false);
        gap_stop_timer(&teams_usb_timer_handle_hfp_ring);
    }
}

void app_teams_mulitlink_usb_handle_hfp_status_change(T_BT_HFP_CALL_STATUS old_status,
                                                      T_BT_HFP_CALL_STATUS new_status)
{
    APP_PRINT_TRACE2("app_teams_mulitlink_usb_handle_hfp_status_change: old_status %d, new_status %d",
                     old_status, new_status);

    if ((old_status == BT_HFP_CALL_IDLE) && (new_status != BT_HFP_CALL_IDLE) &&
        (app_bt_policy_get_radio_mode() == BT_DEVICE_MODE_DISCOVERABLE_CONNECTABLE))
    {
        app_bt_policy_exit_pairing_mode();
    }

    // incoming call or outgoing call
    if (old_status == BT_HFP_CALL_IDLE &&
        (new_status == BT_HFP_INCOMING_CALL_ONGOING || new_status == BT_HFP_OUTGOING_CALL_ONGOING))
    {
        //if usb voice priority is first
        if (app_teams_multilink_app_high_priority_mgr[0].idx == multilink_usb_idx)
        {
            // if data stream is running
            if ((teams_usb_stream_status & APP_TEAMS_MULTILINK_USB_DOWNSTREAM_ACTIVE))
            {
                app_teams_multilink_high_app_action_trigger_low_app_action(multilink_usb_idx,
                                                                           T_APP_TEAMS_MULTILINK_HIGH_PRIORITY_APP_START);
            }
        }
        else
        {
            //if usb voice priority is not first and voice status is incoming call or outgoing call, it should play outband ringtone
            if (new_status == BT_HFP_INCOMING_CALL_ONGOING)
            {
                app_teams_multilink_usb_start_alert(T_APP_TEAMS_MULTILINK_USB_INCOMING_RING);
            }
            else if (new_status == BT_HFP_OUTGOING_CALL_ONGOING)
            {
                app_teams_multilink_usb_start_alert(T_APP_TEAMS_MULTILINK_USB_OUTGOING_RING);
            }
        }
    }
    // active call
    else if (new_status == BT_HFP_CALL_ACTIVE || new_status == BT_HFP_CALL_ACTIVE_WITH_CALL_WAITING ||
             new_status == BT_HFP_CALL_ACTIVE_WITH_CALL_HOLD)
    {
        //if usb is running incoming alert, so need to stop it
        app_teams_multilink_usb_stop_alert();

        //audio track of usb voice is playback and record, and usb could not grab active link from bt in framework,
        //so stop bt sco audio track while strat usb voice audio track
        if ((teams_usb_stream_status == (APP_TEAMS_MULTILINK_USB_DOWNSTREAM_ACTIVE |
                                         APP_TEAMS_MULTILINK_USB_UPSTREAM_ACTIVE)) &&
            app_teams_multilink_app_high_priority_mgr[0].idx == multilink_usb_idx)
        {
            app_teams_multilink_stop_all_bt_sco_audio_track();
            app_teams_multilink_high_app_action_trigger_low_app_action(multilink_usb_idx,
                                                                       T_APP_TEAMS_MULTILINK_HIGH_PRIORITY_APP_START);
        }
    }
    // idle
    else if (new_status == BT_HFP_CALL_IDLE && old_status != BT_HFP_CALL_IDLE)
    {
        //if usb is running outgoing alert, so need to stop it
        app_teams_multilink_usb_stop_alert();

        if (!app_teams_call_status)
        {
            app_teams_multilink_high_app_action_trigger_low_app_action(multilink_usb_idx,
                                                                       T_APP_TEAMS_MULTILINK_HIGH_PRIORITY_APP_STOP);
        }
    }
}

void app_teams_mulitlink_bt_handle_hfp_status_change(uint8_t index,
                                                     T_BT_HFP_CALL_STATUS old_status,
                                                     T_BT_HFP_CALL_STATUS new_status)
{
    APP_PRINT_TRACE4("app_teams_mulitlink_bt_handle_hfp_status_change: idx %d, old_status %d, new_status %d, sco num %d",
                     index, old_status, new_status, app_find_sco_conn_num());
    /*hfp state not idle, trigger voice coupling music handle*/
    if (old_status == BT_HFP_CALL_IDLE && new_status != BT_HFP_CALL_IDLE &&
        index == app_teams_multilink_app_high_priority_mgr[0].idx)
    {
        app_teams_multilink_high_app_action_trigger_low_app_action(index,
                                                                   T_APP_TEAMS_MULTILINK_HIGH_PRIORITY_APP_START);
    }
    else if (old_status != BT_HFP_CALL_IDLE && new_status == BT_HFP_CALL_IDLE && !app_teams_call_status)
    {
        app_teams_multilink_high_app_action_trigger_low_app_action(index,
                                                                   T_APP_TEAMS_MULTILINK_HIGH_PRIORITY_APP_STOP);
    }
}

void app_teams_multilink_handle_hfp_state_change(uint8_t idx, T_BT_HFP_CALL_STATUS old_status,
                                                 T_BT_HFP_CALL_STATUS new_status)
{
    // update data for which link is most high priority
    if (new_status != BT_HFP_CALL_IDLE && old_status == BT_HFP_CALL_IDLE)
    {
        app_teams_multilink_handle_app_active(idx);
    }
    else if (new_status == BT_HFP_CALL_IDLE && old_status != BT_HFP_CALL_IDLE)
    {
        app_teams_multilink_handle_app_end(idx);
    }

    // handle voice and music coupling due to state
    if (idx == multilink_usb_idx)
    {
        app_teams_mulitlink_usb_handle_hfp_status_change(old_status, new_status);
    }
    else
    {
        app_teams_mulitlink_bt_handle_hfp_status_change(idx, old_status, new_status);
    }
}

void app_teams_multilink_handle_hfp_status(uint8_t idx, T_BT_HFP_CALL_STATUS hfp_status)
{
    T_APP_TEAMS_MULTILINK_APP_HIGH_PRIORITY_ARRAY_DATA *p_voice_priority_data = NULL;
    T_BT_HFP_CALL_STATUS hfp_old_status;

    p_voice_priority_data = app_teams_multilink_find_app_high_priority_array_data_by_link_idx(idx);
    if (p_voice_priority_data)
    {
        hfp_old_status = p_voice_priority_data->voice_status;
        p_voice_priority_data->voice_status = hfp_status;

        // sort and get all link priority about voice
        if (hfp_status > hfp_old_status)
        {
            app_teams_multilink_sort_app_array(T_APP_TEAMS_MULTILINK_APP_PRIORITY_HIGH_LEVEL, idx,
                                               APP_TEAMS_MULTILINK_PRIORITY_UP);
        }
        else if (hfp_status < hfp_old_status)
        {
            app_teams_multilink_sort_app_array(T_APP_TEAMS_MULTILINK_APP_PRIORITY_HIGH_LEVEL, idx,
                                               APP_TEAMS_MULTILINK_PRIORITY_DOWN);
        }

        app_teams_multilink_print_priority_mgr_data(T_APP_TEAMS_MULTILINK_APP_TYPE_VOICE);

        // update device hfp status
        app_teams_multilink_cal_device_hfp_status();

        // some special handle on bt or usb channel
        app_teams_multilink_handle_hfp_state_change(idx, hfp_old_status, hfp_status);
    }
}

void app_teams_multilink_voice_track_start(uint8_t index)
{
    APP_PRINT_TRACE1("app_teams_multilink_voice_track_start: idx %d", index);

    if (index == multilink_usb_idx) // usb channel
    {
        // TO_DO
#if F_APP_USB_AUDIO_SUPPORT
        app_usb_audio_start();
#endif
        return;
    }
    else // BT channel
    {
        app_hfp_set_active_hf_index(app_db.br_link[index].bd_addr);
    }
}

void app_teams_multilink_music_track_start(uint8_t index)
{
    APP_PRINT_TRACE1("app_teams_multilink_music_track_start: idx %d", index);

    if (index == multilink_usb_idx) // usb channel
    {
        // TO_DO
#if F_APP_USB_AUDIO_SUPPORT
        app_usb_audio_start();
#endif
        return;
    }
    else // BT channel
    {
        app_a2dp_active_link_set(app_db.br_link[index].bd_addr);
    }
}

void app_teams_multilink_record_track_start(uint8_t index)
{
    APP_PRINT_TRACE1("app_teams_multilink_record_track_start: idx %d", index);
    T_APP_BR_LINK *p_link;
    T_AUDIO_TRACK_STATE audio_track_state;

    if (index == multilink_usb_idx) // usb channel
    {
        // TO_DO
#if F_APP_USB_AUDIO_SUPPORT
        app_usb_audio_start();
#endif
        return;
    }
    else // BT channel
    {
        p_link = app_find_br_link(app_db.br_link[index].bd_addr);
        if (p_link)
        {
            audio_track_state_get(p_link->sco_track_handle, &audio_track_state);
            if (audio_track_state != AUDIO_TRACK_STATE_STARTED)
            {
                audio_track_start(p_link->sco_track_handle);
            }
        }
    }
}

void app_teams_multilink_voice_priority_rearrangment(uint8_t index, T_BT_HFP_CALL_STATUS hfp_status)
{
    APP_PRINT_TRACE2("app_teams_multilink_voice_priority_rearrangment: idx %d, hfp_status %d",
                     index, hfp_status);
    // update link hfp status
    app_teams_multilink_handle_hfp_status(index, hfp_status);

    // set new active hfp index due to new voice priority
    app_teams_multilink_set_active_voice_index(app_teams_multilink_app_high_priority_mgr[0].idx);

    // if device is in call non-idle status
    if (app_teams_call_status)
    {
        // if is bt channel, audio track start direct, usb need to check data stream
        if (teams_active_voice_idx != multilink_usb_idx ||
            app_teams_multilink_check_usb_voice_data_stream_running())
        {
            app_teams_multilink_voice_track_start(teams_active_voice_idx);
        }
    }

    //notify wl the mic stream status
    if (app_teams_multilink_get_voice_status())
    {
        app_teams_wl_mic_stream_notify(true);
    }
    else
    {
        app_teams_wl_mic_stream_notify(false);
    }
}

void app_teams_multilink_handle_first_voice_profile_connect(uint8_t index)
{
    APP_PRINT_TRACE1("app_teams_multilink_handle_first_voice_profile_connect: idx %d",
                     index);
    if (app_teams_multilink_app_high_priority_mgr[0].idx != index)
    {
        for (uint8_t i = 1; i < APP_TEAMS_MAX_LINK_NUM; i++)
        {
            if (app_teams_multilink_app_high_priority_mgr[i].idx == index)
            {
                app_teams_multilink_swap_app_high_priority_array_data(0, i);
            }
        }
    }

    app_teams_multilink_set_active_voice_index(index);
}

void app_teams_multilink_handle_voice_profile_disconnect(uint8_t index)
{
    APP_PRINT_TRACE1("app_teams_multilink_handle_voice_profile_disconnect: idx %d",
                     index);
    app_teams_multilink_voice_priority_rearrangment(index, BT_HFP_CALL_IDLE);
}

void app_teams_multilink_qos_param_update(uint8_t index, T_APP_JUDGE_A2DP_EVENT event)
{
    APP_PRINT_TRACE2("app_teams_multilink_qos_param_update: idx %d, event %d", index,
                     event);
    if (event == JUDGE_EVENT_A2DP_START || event == JUDGE_EVENT_MEDIAPLAYER_PLAYING)
    {
        app_bt_policy_qos_param_update(app_db.br_link[index].bd_addr, BP_TPOLL_A2DP_PLAY_EVENT);
    }
    else if (event == JUDGE_EVENT_A2DP_DISC || event == JUDGE_EVENT_A2DP_STOP)
    {
        app_bt_policy_qos_param_update(app_db.br_link[index].bd_addr, BP_TPOLL_A2DP_STOP_EVENT);
    }
}

// true: fir_index > sec_index
// false: fir_index < sec_index
bool app_teams_multilink_compare_app_low_priority_by_link_idx(uint8_t fir_index, uint8_t sec_index)
{
    uint8_t fir_index_priority = 0;
    uint8_t sec_index_priority = 0;

    for (uint8_t i = 0; i < APP_TEAMS_MAX_LINK_NUM; i++)
    {
        if (app_teams_multilink_app_low_priority_mgr[i].idx == fir_index)
        {
            fir_index_priority = i;
        }
        else if (app_teams_multilink_app_low_priority_mgr[i].idx == sec_index)
        {
            sec_index_priority = i;
        }
    }

    if (fir_index_priority < sec_index_priority)
    {
        return true;
    }
    else
    {
        return false;
    }
}

void app_teams_multilink_update_br_link_avrcp_status(uint8_t index, T_APP_JUDGE_A2DP_EVENT event)
{
    APP_PRINT_TRACE2("app_teams_multilink_update_br_link_avrcp_status: index %d, event %d",
                     index, event);
    if (index < MAX_BR_LINK_NUM)
    {
        if (event == JUDGE_EVENT_A2DP_START)
        {
            app_db.br_link[index].avrcp_play_status = BT_AVRCP_PLAY_STATUS_PLAYING;
        }
        else if (event == JUDGE_EVENT_A2DP_STOP)
        {
            app_db.br_link[index].avrcp_play_status = BT_AVRCP_PLAY_STATUS_PAUSED;
        }
    }
}

#if 0
bool app_teams_multilink_check_is_resume_process(uint8_t idx, T_APP_JUDGE_A2DP_EVENT event)
{
    APP_PRINT_TRACE3("app_teams_multilink_check_is_resume_process: index %d, event %d, teams_resume_music_index %d",
                     idx, event, teams_resume_music_index);
    if (teams_resume_music_index == APP_TEAMS_MAX_LINK_NUM)
    {
        return false;
    }

    if (!(teams_music_resume_mask & (uint8_t)(1 << idx)))
    {
        return false;
    }

    if (event == JUDGE_EVENT_A2DP_START && idx == teams_resume_music_index)
    {
        return true;
    }
    else if (event == JUDGE_EVENT_A2DP_START)
    {
        // no resume this index, because host start palying music
        if (app_teams_multilink_compare_app_low_priority_by_link_idx(teams_resume_music_index, idx))
        {
            teams_music_resume_mask &= ~(uint8_t)(1 << idx);
        }
    }

    return false;
}
#endif

void app_teams_multilink_set_low_priority_app_active_link(
    T_APP_TEAMS_MULTILINK_APP_LOW_PRIORITY_TYPE app_type)
{
    //if no running app, keep the last active index
    if (app_type == T_APP_TEAMS_MULTILINK_APP_LOW_PRIORITY_MUSIC)
    {
        for (uint8_t i = 0; i < APP_TEAMS_MAX_LINK_NUM; i++)
        {
            if (app_teams_multilink_app_low_priority_mgr[i].music_event == JUDGE_EVENT_A2DP_START)
            {
                app_teams_multilink_set_active_music_index(app_teams_multilink_app_low_priority_mgr[i].idx);
                return;
            }
        }
    }
    else if (app_type == T_APP_TEAMS_MULTILINK_APP_LOW_PRIORITY_RECORD)
    {
        for (uint8_t i = 0; i < APP_TEAMS_MAX_LINK_NUM; i++)
        {
            if (app_teams_multilink_app_low_priority_mgr[i].record_status)
            {
                app_teams_multilink_set_active_record_index(app_teams_multilink_app_low_priority_mgr[i].idx);
                return;
            }
        }
    }
}

// true: low priority app could start
// false: low priority app could not start
bool app_teams_multilink_check_low_priority_app_start_condition(void)
{
    // 1. voice status is idle,
    // 2. voice status is not idle but sco num is zero
    if (app_teams_call_status == BT_HFP_CALL_IDLE || !app_teams_multilink_find_sco_conn_num())
    {
        return true;
    }
    return false;
}

void app_teams_multilink_start_low_priority_app(void)
{
    if (app_teams_multilink_app_low_priority_mgr[0].music_event == JUDGE_EVENT_A2DP_START)
    {
        app_teams_multilink_music_track_start(app_teams_multilink_app_low_priority_mgr[0].idx);
    }

    if (app_teams_multilink_app_low_priority_mgr[0].record_status)
    {
        app_teams_multilink_record_track_start(app_teams_multilink_app_low_priority_mgr[0].idx);
    }
}

void app_teams_multilink_handle_music_event(uint8_t idx, T_APP_JUDGE_A2DP_EVENT event)
{
    T_APP_TEAMS_MULTILINK_APP_LOW_PRIORITY_ARRAY_DATA *p_music_priority_data = NULL;
    T_APP_JUDGE_A2DP_EVENT music_old_event;
    //app_teams_multilink_update_br_link_avrcp_status(idx, event);

    // update app status to device priority array
    if (event == JUDGE_EVENT_A2DP_START)
    {
        app_teams_multilink_handle_app_active(idx);
    }
    else if (event == JUDGE_EVENT_A2DP_STOP)
    {
        app_teams_multilink_handle_app_end(idx);
    }

    // only handle four music event
    if (event == JUDGE_EVENT_A2DP_CONNECTED || event == JUDGE_EVENT_A2DP_START ||
        event == JUDGE_EVENT_A2DP_DISC || event == JUDGE_EVENT_A2DP_STOP)
    {
        // update app status to app low priority array
        p_music_priority_data = app_teams_multilink_find_app_low_priority_array_data_by_link_idx(idx);
        if (p_music_priority_data)
        {
            music_old_event = p_music_priority_data->music_event;
            p_music_priority_data->music_event = event;

            //host restart music, delete music resume mask bit
            if ((teams_music_resume_mask & (uint8_t)(1 << ((idx == multilink_usb_idx) ?
                                                           (APP_TEAMS_MAX_LINK_NUM - 1) : idx))) && event == JUDGE_EVENT_A2DP_START)
            {
                teams_music_resume_mask &= ~(uint8_t)(1 << ((idx == multilink_usb_idx) ?
                                                            (APP_TEAMS_MAX_LINK_NUM - 1) : idx));
            }

            //music status become high priority
            if ((event == JUDGE_EVENT_A2DP_START && music_old_event != JUDGE_EVENT_A2DP_START) ||
                (event != JUDGE_EVENT_A2DP_DISC && music_old_event == JUDGE_EVENT_A2DP_DISC))
            {
                app_teams_multilink_sort_app_array(T_APP_TEAMS_MULTILINK_APP_PRIORITY_LOW_LEVEL, idx,
                                                   APP_TEAMS_MULTILINK_PRIORITY_UP);
            }
            //music status become low priority
            else if ((music_old_event == JUDGE_EVENT_A2DP_START && event != JUDGE_EVENT_A2DP_START) ||
                     (music_old_event != JUDGE_EVENT_A2DP_DISC && event == JUDGE_EVENT_A2DP_DISC))
            {
                app_teams_multilink_sort_app_array(T_APP_TEAMS_MULTILINK_APP_PRIORITY_LOW_LEVEL, idx,
                                                   APP_TEAMS_MULTILINK_PRIORITY_DOWN);
            }

            app_teams_multilink_set_music_status(app_teams_multilink_app_low_priority_mgr[0].music_event);

            app_teams_multilink_print_priority_mgr_data(T_APP_TEAMS_MULTILINK_APP_TYPE_MUSIC);
        }
    }
}

void app_teams_multilink_music_priority_rearrangment(uint8_t index, T_APP_JUDGE_A2DP_EVENT event)
{
    APP_PRINT_TRACE3("app_teams_multilink_music_priority_rearrangment: 1 event %d, app_idx %d, active_a2dp_idx %d",
                     event, index, teams_active_music_idx);

    // handle music event
    app_teams_multilink_handle_music_event(index, event);

    // set new active music index
    app_teams_multilink_set_low_priority_app_active_link(T_APP_TEAMS_MULTILINK_APP_LOW_PRIORITY_MUSIC);

    // set new low priority app active index
    teams_multilink_app_low_priority_active_index = app_teams_multilink_app_low_priority_mgr[0].idx;

    // start music audio track due to newest active music link index
    if (app_teams_multilink_check_low_priority_app_start_condition())
    {
        app_teams_multilink_start_low_priority_app();
    }

    // update bt qos
    if (teams_active_music_idx != multilink_usb_idx)
    {
        app_teams_multilink_qos_param_update(index, event);
    }

    //notify wl the audio stream status
    if (!app_teams_multilink_get_voice_status() &&
        (JUDGE_EVENT_A2DP_START == app_teams_multilink_app_low_priority_mgr[0].music_event))
    {
        app_teams_wl_audio_stream_notify(true);
    }
    else
    {
        app_teams_wl_audio_stream_notify(false);
    }

    APP_PRINT_TRACE2("app_teams_multilink_music_priority_rearrangment: 2 active_app_idx %d, active_music_idx %d",
                     teams_multilink_app_low_priority_active_index, teams_active_music_idx);
}

void app_teams_multilink_handle_record_event(uint8_t idx, bool record_status)
{
    T_APP_TEAMS_MULTILINK_APP_LOW_PRIORITY_ARRAY_DATA *p_record_priority_data = NULL;
    bool record_old_status;

    // update app status to device priority array
    if (record_status)
    {
        app_teams_multilink_handle_app_active(idx);
    }
    else
    {
        app_teams_multilink_handle_app_end(idx);
    }

    // update app status to app low priority array
    p_record_priority_data = app_teams_multilink_find_app_low_priority_array_data_by_link_idx(idx);
    if (p_record_priority_data)
    {
        record_old_status = p_record_priority_data->record_status;
        p_record_priority_data->record_status = record_status;

        //host restart record, delete record resume mask bit
        if ((teams_record_resume_mask & (uint8_t)(1 << ((idx == multilink_usb_idx) ?
                                                        (APP_TEAMS_MAX_LINK_NUM - 1) : idx))) && record_status)
        {
            teams_record_resume_mask &= ~(uint8_t)(1 << ((idx == multilink_usb_idx) ?
                                                         (APP_TEAMS_MAX_LINK_NUM - 1) : idx));
        }

        // record status change from idle to active
        if (!record_old_status && record_status)
        {
            app_teams_multilink_sort_app_array(T_APP_TEAMS_MULTILINK_APP_PRIORITY_LOW_LEVEL, idx,
                                               APP_TEAMS_MULTILINK_PRIORITY_UP);
        }
        // record status change from active to idle
        else if (record_old_status && !record_status)
        {
            app_teams_multilink_sort_app_array(T_APP_TEAMS_MULTILINK_APP_PRIORITY_LOW_LEVEL, idx,
                                               APP_TEAMS_MULTILINK_PRIORITY_DOWN);
        }

        app_teams_multilink_set_record_status(app_teams_multilink_app_low_priority_mgr[0].record_status);

        app_teams_multilink_print_priority_mgr_data(T_APP_TEAMS_MULTILINK_APP_TYPE_RECORD);
    }
}

void app_teams_multilink_record_priority_rearrangment(uint8_t index, bool record_status)
{
    APP_PRINT_TRACE3("app_teams_multilink_record_priority_rearrangment: 1 app_idx %d, record_status %d, active_record_index %d",
                     index, record_status, teams_active_record_idx);

    // handle record event
    app_teams_multilink_handle_record_event(index, record_status);

    // set new active record index
    app_teams_multilink_set_low_priority_app_active_link(T_APP_TEAMS_MULTILINK_APP_LOW_PRIORITY_RECORD);

    // set new low priority app active index
    teams_multilink_app_low_priority_active_index = app_teams_multilink_app_low_priority_mgr[0].idx;

    // start record audio track due to newest active music link index
    if (app_teams_multilink_check_low_priority_app_start_condition())
    {
        app_teams_multilink_start_low_priority_app();
    }

    //notify wl the audio stream status
    if (!app_teams_multilink_get_voice_status() &&
        app_teams_multilink_app_low_priority_mgr[0].record_status)
    {
        app_teams_wl_mic_stream_notify(true);
    }
    else
    {
        app_teams_wl_mic_stream_notify(false);
    }

    APP_PRINT_TRACE2("app_teams_multilink_record_priority_rearrangment: 2 active_app_idx %d, active_record_idx %d",
                     teams_multilink_app_low_priority_active_index, teams_active_record_idx);
}

void app_teams_multilink_pause_record_link_stream(uint8_t index, uint8_t resume_fg)
{
    T_APP_TEAMS_MULTILINK_APP_LOW_PRIORITY_ARRAY_DATA *record_data = NULL;
    record_data = app_teams_multilink_find_app_low_priority_array_data_by_link_idx(index);

    if (record_data->record_status)
    {
        app_teams_multilink_record_priority_rearrangment(index, false);
        //stop sco handle for usb if active voice link is usb channel
        if (index != multilink_usb_idx)
        {
            audio_track_stop(app_db.br_link[index].sco_track_handle);
        }
        if (resume_fg)
        {
            teams_record_resume_mask |= 1 << ((index == multilink_usb_idx) ? (APP_TEAMS_MAX_LINK_NUM - 1) :
                                              index);
        }
    }
}

bool app_teams_multilink_check_recorder_resume_condition(uint8_t index)
{
    if (index != multilink_usb_idx)
    {
        if (app_db.br_link[index].sco_track_handle)
        {
            return true;
        }
    }
    else
    {
        if (teams_usb_stream_status & APP_TEAMS_MULTILINK_USB_UPSTREAM_ACTIVE)
        {
            return true;
        }
    }
    return false;
}

void app_teams_multilink_resume_record_link_stream(uint8_t index)
{
    T_APP_TEAMS_MULTILINK_APP_LOW_PRIORITY_ARRAY_DATA *record_data = NULL;
    record_data = app_teams_multilink_find_app_low_priority_array_data_by_link_idx(index);
    if (teams_record_resume_mask && record_data)
    {
        if (teams_record_resume_mask & (uint8_t)(1 << ((index == multilink_usb_idx) ?
                                                       (APP_TEAMS_MAX_LINK_NUM - 1) : index)))
        {
            if (!record_data->record_status && app_teams_multilink_check_recorder_resume_condition(index))
            {
                app_teams_multilink_record_priority_rearrangment(index, true);
            }

            teams_record_resume_mask &= ~(uint8_t)(1 << ((index == multilink_usb_idx) ?
                                                         (APP_TEAMS_MAX_LINK_NUM - 1) : index));
        }
    }
}

void app_teams_multilink_pause_music_link_stream(uint8_t index, uint8_t resume_fg)
{
    if (((index != multilink_usb_idx) &&
         (app_db.br_link[index].connected_profile & (A2DP_PROFILE_MASK | AVRCP_PROFILE_MASK)) &&
         (app_db.br_link[index].streaming_fg == true)) ||
        ((index == multilink_usb_idx) && app_teams_multilink_check_usb_music_data_stream_running()))
    {
        if (resume_fg)
        {
            teams_music_resume_mask |= 1 << ((index == multilink_usb_idx) ? (APP_TEAMS_MAX_LINK_NUM - 1) :
                                             index);
        }

        if (index != multilink_usb_idx)
        {
            bt_avrcp_pause(app_db.br_link[index].bd_addr);
            app_db.br_link[index].avrcp_play_status = BT_AVRCP_PLAY_STATUS_PAUSED;
        }
        else
        {
#if F_APP_USB_AUDIO_SUPPORT
            app_usb_mmi_handle_action(MMI_USB_AUDIO_PLAY_PAUSE);
#endif
        }
    }
    return;
}

void app_teams_multilink_resume_music_link_stream(uint8_t index)
{
    if (teams_music_resume_mask)
    {
        if (teams_music_resume_mask & (uint8_t)(1 << ((index == multilink_usb_idx) ?
                                                      (APP_TEAMS_MAX_LINK_NUM - 1) : index)))
        {
            if (index == multilink_usb_idx)
            {
                if (!app_teams_multilink_check_usb_music_data_stream_running())
                {
#if F_APP_USB_AUDIO_SUPPORT
                    app_usb_mmi_handle_action(MMI_USB_AUDIO_PLAY_PAUSE);
#endif
                }
            }
            else
            {
                if ((app_db.br_link[index].connected_profile &
                     (A2DP_PROFILE_MASK |
                      AVRCP_PROFILE_MASK)) &&
                    (app_db.br_link[index].avrcp_play_status !=
                     BT_AVRCP_PLAY_STATUS_PLAYING))
                {
                    app_db.br_link[index].avrcp_play_status = BT_AVRCP_PLAY_STATUS_PLAYING;
                    bt_avrcp_play(app_db.br_link[index].bd_addr);
                }
            }

            teams_music_resume_mask &= ~(uint8_t)(1 << ((index == multilink_usb_idx) ?
                                                        (APP_TEAMS_MAX_LINK_NUM - 1) : index));
        }
    }
}

void app_teams_multilink_high_app_action_trigger_music_action(uint8_t high_index,
                                                              T_APP_TEAMS_MULTILINK_HIGH_PRIORITY_APP_ACTION_TYPE action_type, uint8_t low_index)
{
    APP_PRINT_TRACE6("app_teams_multilink_high_app_action_trigger_music_action event %d, high idx %d, low idx %d, teams_active_music_idx %d, teams_music_resume_mask %d, sco num %d",
                     action_type, high_index, low_index, teams_active_music_idx, teams_music_resume_mask,
                     app_find_sco_conn_num());
    if (action_type == T_APP_TEAMS_MULTILINK_HIGH_PRIORITY_APP_START)
    {
        /* sco cmpl, pause a2dp data stream, if this is dongle sco or usb sco, do not pause a2dp stream if it has,
        because of dongle or usb would mix sco and a2dp data */
#if F_APP_TEAMS_BT_POLICY
        if ((high_index == low_index) &&
            ((low_index == multilink_usb_idx) ||
             (T_TEAMS_DEVICE_DONGLE_TYPE == app_bt_policy_get_cod_type_by_addr(
                  app_db.br_link[low_index].bd_addr))))
#else
        if ((high_index == low_index) && (low_index == multilink_usb_idx))
#endif
        {
            APP_PRINT_TRACE0("app_teams_multilink_high_app_action_trigger_music_action dongle or usb");
        }
        else
        {
            app_teams_multilink_pause_music_link_stream(low_index, true);
        }
    }
    else if (action_type == T_APP_TEAMS_MULTILINK_HIGH_PRIORITY_APP_STOP)
    {
        /*sco disconnect, resume all wait a2dp stream, start audio track with active a2dp index */
        //if (app_teams_multilink_find_sco_conn_num() == 0)
        {
            app_teams_multilink_resume_music_link_stream(low_index);
        }
    }
}

void app_teams_multilink_high_app_action_trigger_record_action(uint8_t high_index,
                                                               T_APP_TEAMS_MULTILINK_HIGH_PRIORITY_APP_ACTION_TYPE action_type, uint8_t low_index)
{
    APP_PRINT_TRACE6("app_teams_multilink_high_app_action_trigger_record_action event %d, high idx %d, low idx %d, teams_active_record_idx %d, teams_record_resume_mask %d, sco num %d",
                     action_type, high_index, low_index, teams_active_record_idx, teams_record_resume_mask,
                     app_find_sco_conn_num());
    if (action_type == T_APP_TEAMS_MULTILINK_HIGH_PRIORITY_APP_START)
    {
        if (high_index == low_index)
        {
            APP_PRINT_TRACE0("app_teams_multilink_high_app_action_trigger_record_action start high app and low app device is same, not stop record");
        }
        else
        {
            app_teams_multilink_pause_record_link_stream(low_index, true);
        }
    }
    else if (action_type == T_APP_TEAMS_MULTILINK_HIGH_PRIORITY_APP_STOP)
    {
        /*sco disconnect, resume all wait a2dp stream, start audio track with active a2dp index */
        //if (app_teams_multilink_find_sco_conn_num() == 0)
        {
            app_teams_multilink_resume_record_link_stream(low_index);
        }
    }
}

void app_teams_multilink_high_app_action_trigger_low_app_action(uint8_t index,
                                                                T_APP_TEAMS_MULTILINK_HIGH_PRIORITY_APP_ACTION_TYPE action_type)
{
    if (!app_teams_multilink_check_index_valid(index))
    {
        return;
    }

    if (action_type == T_APP_TEAMS_MULTILINK_HIGH_PRIORITY_APP_START)
    {
        for (int8_t i = APP_TEAMS_MAX_LINK_NUM - 1; i >= 0; i--)
        {
            if (app_teams_multilink_app_low_priority_mgr[i].music_event == JUDGE_EVENT_A2DP_START)
            {
                app_teams_multilink_high_app_action_trigger_music_action(index,
                                                                         T_APP_TEAMS_MULTILINK_HIGH_PRIORITY_APP_START, app_teams_multilink_app_low_priority_mgr[i].idx);
            }

            if (app_teams_multilink_app_low_priority_mgr[i].record_status)
            {
                app_teams_multilink_high_app_action_trigger_record_action(index,
                                                                          T_APP_TEAMS_MULTILINK_HIGH_PRIORITY_APP_START, app_teams_multilink_app_low_priority_mgr[i].idx);
            }
        }
    }
    else if (action_type == T_APP_TEAMS_MULTILINK_HIGH_PRIORITY_APP_STOP)
    {
        for (uint8_t i = 0; i < APP_TEAMS_MAX_LINK_NUM; i++)
        {
            if (app_teams_multilink_app_low_priority_mgr[i].music_event == JUDGE_EVENT_A2DP_STOP)
            {
                app_teams_multilink_high_app_action_trigger_music_action(index,
                                                                         T_APP_TEAMS_MULTILINK_HIGH_PRIORITY_APP_STOP, app_teams_multilink_app_low_priority_mgr[i].idx);
            }

            if (!app_teams_multilink_app_low_priority_mgr[i].record_status)
            {
                app_teams_multilink_high_app_action_trigger_record_action(index,
                                                                          T_APP_TEAMS_MULTILINK_HIGH_PRIORITY_APP_STOP, app_teams_multilink_app_low_priority_mgr[i].idx);
            }
        }
    }
}

void app_teams_multilink_handle_usb_stream_event(T_APP_TEAMS_MULTILINK_USB_STREAM_STATUS_EVENT
                                                 event)
{
    APP_PRINT_TRACE3("app_teams_multilink_handle_usb_stream_event event %d, teams_usb_stream_status %x, teams_usb_recorder_running %d",
                     event,
                     teams_usb_stream_status, teams_usb_recorder_running);
    T_APP_TEAMS_MULTILINK_APP_LOW_PRIORITY_ARRAY_DATA *p_music_priority_data = NULL;
    T_APP_TEAMS_MULTILINK_APP_HIGH_PRIORITY_ARRAY_DATA *p_voice_priority_data = NULL;

    p_voice_priority_data = app_teams_multilink_find_app_high_priority_array_data_by_link_idx(
                                multilink_usb_idx);
    if (event == APP_TEAMS_MULTILINK_USB_UPSTREAM_START)
    {
        // call active
        if ((teams_usb_stream_status & APP_TEAMS_MULTILINK_USB_DOWNSTREAM_ACTIVE) &&
            (p_voice_priority_data->voice_status == BT_HFP_CALL_ACTIVE ||
             p_voice_priority_data->voice_status == BT_HFP_CALL_ACTIVE_WITH_CALL_WAITING ||
             p_voice_priority_data->voice_status == BT_HFP_CALL_ACTIVE_WITH_CALL_HOLD))
        {
            //audio track of usb voice is playback and record, and usb could not grab active link from bt in framework,
            //so stop bt sco audio track while strat usb voice audio track
            if (multilink_usb_idx == teams_active_voice_idx)
            {
                app_teams_multilink_stop_all_bt_sco_audio_track();
                app_teams_multilink_high_app_action_trigger_low_app_action(multilink_usb_idx,
                                                                           T_APP_TEAMS_MULTILINK_HIGH_PRIORITY_APP_START);
                // voice start
                app_teams_multilink_voice_track_start(multilink_usb_idx);
            }
        }
        //else
        {
            // recorder start
            teams_usb_recorder_running = true;
            app_teams_multilink_record_priority_rearrangment(multilink_usb_idx, true);
        }
    }
    else if (event == APP_TEAMS_MULTILINK_USB_UPSTREAM_STOP)
    {
        // recorder stop
        if (teams_usb_recorder_running)
        {
            teams_usb_recorder_running = false;
            app_teams_multilink_record_priority_rearrangment(multilink_usb_idx, false);
        }
    }
    else if (event == APP_TEAMS_MULTILINK_USB_DOWNSTREAM_START)
    {
        // call active
        if ((teams_usb_stream_status & APP_TEAMS_MULTILINK_USB_UPSTREAM_ACTIVE) &&
            (p_voice_priority_data->voice_status == BT_HFP_CALL_ACTIVE ||
             p_voice_priority_data->voice_status == BT_HFP_CALL_ACTIVE_WITH_CALL_WAITING ||
             p_voice_priority_data->voice_status == BT_HFP_CALL_ACTIVE_WITH_CALL_HOLD))
        {
            //audio track of usb voice is playback and record, and usb could not grab active link from bt in framework,
            //so stop bt sco audio track while strat usb voice audio track
            if (multilink_usb_idx == teams_active_voice_idx)
            {
                app_teams_multilink_stop_all_bt_sco_audio_track();
                app_teams_multilink_high_app_action_trigger_low_app_action(multilink_usb_idx,
                                                                           T_APP_TEAMS_MULTILINK_HIGH_PRIORITY_APP_START);
                // voice start
                app_teams_multilink_voice_track_start(multilink_usb_idx);
            }
        }
        // incoming call or outgoing call
        else if (p_voice_priority_data &&
                 (p_voice_priority_data->voice_status == BT_HFP_INCOMING_CALL_ONGOING ||
                  p_voice_priority_data->voice_status == BT_HFP_OUTGOING_CALL_ONGOING))
        {
            if (multilink_usb_idx == teams_active_voice_idx)
            {
                app_teams_multilink_high_app_action_trigger_low_app_action(multilink_usb_idx,
                                                                           T_APP_TEAMS_MULTILINK_HIGH_PRIORITY_APP_START);
                // voice start
                app_teams_multilink_voice_track_start(multilink_usb_idx);
            }
        }
        //else
        {
            // music start
            // gap_start_timer(&teams_multilink_music_playing_check_index, "teams_multilink_check_music_playing",
            //                 teams_multilink_timer_queue_id, APP_TEAMS_MULTILINK_CHECK_MUSIC_PLAYING,
            //                 0, APP_TEAMS_MULTILINK_MUSIC_PLAYING_CHECK_TIMEOUT);
            teams_usb_music_data_stream_running = true;
            app_teams_multilink_music_priority_rearrangment(multilink_usb_idx, JUDGE_EVENT_A2DP_START);
        }
    }
    else if (event == APP_TEAMS_MULTILINK_USB_DOWNSTREAM_STOP)
    {
        p_music_priority_data = app_teams_multilink_find_app_low_priority_array_data_by_link_idx(
                                    multilink_usb_idx);
        if (p_music_priority_data && (p_music_priority_data->music_event == JUDGE_EVENT_A2DP_START))
        {
            // music stop
            app_teams_multilink_music_priority_rearrangment(multilink_usb_idx, JUDGE_EVENT_A2DP_STOP);
            teams_usb_music_data_stream_running = false;
        }
    }
}

void app_teams_multilink_update_usb_stream_status(T_APP_TEAMS_MULTILINK_USB_STREAM_STATUS_EVENT
                                                  event)
{
    if (event == APP_TEAMS_MULTILINK_USB_UPSTREAM_START)
    {
        teams_usb_stream_status |= APP_TEAMS_MULTILINK_USB_UPSTREAM_ACTIVE;
    }
    else if (event == APP_TEAMS_MULTILINK_USB_UPSTREAM_STOP)
    {
        teams_usb_stream_status &= ~APP_TEAMS_MULTILINK_USB_UPSTREAM_ACTIVE;
    }
    else if (event == APP_TEAMS_MULTILINK_USB_DOWNSTREAM_START)
    {
        teams_usb_stream_status |= APP_TEAMS_MULTILINK_USB_DOWNSTREAM_ACTIVE;
    }
    else if (event == APP_TEAMS_MULTILINK_USB_DOWNSTREAM_STOP)
    {
        teams_usb_stream_status &= ~APP_TEAMS_MULTILINK_USB_DOWNSTREAM_ACTIVE;
    }

    app_teams_multilink_handle_usb_stream_event(event);
}


static void app_teams_multilink_timeout_cb(uint8_t timer_id, uint16_t timer_chann)
{
    APP_PRINT_TRACE3("app_teams_multilink_timeout_cb: timer_id %d, timer_chann %d, teams_usb_stream_status %d",
                     timer_id, timer_chann, teams_usb_stream_status);
    uint8_t index = 0;
    T_APP_AUDIO_TONE_TYPE tone_type;

    switch (timer_id)
    {
    case APP_TEAMS_MULTILINK_CHECK_MUSIC_PLAYING:
        {
#if 0
            T_APP_TEAMS_MULTILINK_APP_HIGH_PRIORITY_ARRAY_DATA *p_voice_priority_data = NULL;
            gap_stop_timer(&teams_multilink_music_playing_check_index);
            p_voice_priority_data = app_teams_multilink_find_app_high_priority_array_data_by_link_idx(
                                        multilink_usb_idx);
            if (teams_usb_stream_status & APP_TEAMS_MULTILINK_USB_UPSTREAM_ACTIVE)
            {
                APP_PRINT_TRACE0("app_teams_multilink_timeout_cb: voice start 1");
                //voice start
                break;
            }
            else if (p_voice_priority_data &&
                     (p_voice_priority_data->voice_status == BT_HFP_INCOMING_CALL_ONGOING ||
                      p_voice_priority_data->voice_status == BT_HFP_OUTGOING_CALL_ONGOING))
            {
                APP_PRINT_TRACE0("app_teams_multilink_timeout_cb: voice start 2");
                //voice start
                break;
            }
            else
            {
                //muisc start
                teams_usb_music_data_stream_running = true;
                app_teams_multilink_music_priority_rearrangment(multilink_usb_idx, JUDGE_EVENT_A2DP_START);
                //app_teams_multilink_music_track_start(multilink_usb_idx);
            }
#endif
        }
        break;
    case APP_TEAMS_MULTILINK_HANDLE_USB_ALERT:
        {
            gap_stop_timer(&teams_usb_timer_handle_hfp_ring);
            teams_usb_running_ring_type = T_APP_TEAMS_MULTILINK_USB_INVALID_RING;
            if (timer_chann == T_APP_TEAMS_MULTILINK_USB_INCOMING_RING)
            {
                tone_type = TONE_HF_CALL_IN;
            }
            else if (timer_chann == T_APP_TEAMS_MULTILINK_USB_OUTGOING_RING)
            {
                tone_type = TONE_HF_OUTGOING_CALL;
            }

            //usb is not active voice link and voice status is incoming call
            index = app_teams_multilink_find_app_high_priority_array_index_by_link_idx(multilink_usb_idx);
            if (index &&
                (app_teams_multilink_app_high_priority_mgr[index].voice_status == BT_HFP_INCOMING_CALL_ONGOING ||
                 app_teams_multilink_app_high_priority_mgr[index].voice_status == BT_HFP_OUTGOING_CALL_ONGOING))
            {
                app_audio_tone_type_play(tone_type, false, false);
                teams_usb_running_ring_type = (T_APP_TEAMS_MULTILINK_USB_RING_TYPE)timer_chann;
                gap_start_timer(&teams_usb_timer_handle_hfp_ring, "usb_hfp_ring", teams_multilink_timer_queue_id,
                                APP_TEAMS_MULTILINK_HANDLE_USB_ALERT, 0, app_cfg_const.timer_hfp_ring_period * 1000);
            }
        }
        break;
    default:
        break;
    }
}

void app_teams_multilink_handle_power_on(void)
{
    //
}

void app_teams_multilink_handle_power_off(void)
{
    // gap_stop_timer(&teams_multilink_music_playing_check_index);
}

static void app_teams_multilink_dm_cback(T_SYS_EVENT event_type, void *event_buf, uint16_t buf_len)
{
    bool handle = true;

    switch (event_type)
    {
    case SYS_EVENT_POWER_ON:
        {
            app_teams_multilink_handle_power_on();
        }
        break;

    case SYS_EVENT_POWER_OFF:
        {
            app_teams_multilink_handle_power_off();
        }
        break;
    default:
        handle = false;
        break;
    }

    if (handle == true)
    {
        APP_PRINT_INFO1("app_teams_multilink_dm_cback: event_type 0x%04x", event_type);
    }
}

void app_teams_multilink_init(void)
{
    APP_PRINT_TRACE0("app_teams_multilink_init");
    for (uint8_t i = 0; i < APP_TEAMS_MAX_LINK_NUM - 1; i++)
    {
        app_teams_multilink_app_high_priority_mgr[i].idx = i;
        app_teams_multilink_app_low_priority_mgr[i].idx = i;
        app_teams_multilink_device_priority_mgr[i].idx = i;
        app_teams_multilink_app_low_priority_mgr[i].music_event = JUDGE_EVENT_A2DP_DISC;
    }

    app_teams_multilink_app_high_priority_mgr[APP_TEAMS_MAX_LINK_NUM - 1].idx = multilink_usb_idx;
    app_teams_multilink_app_low_priority_mgr[APP_TEAMS_MAX_LINK_NUM - 1].idx = multilink_usb_idx;
    app_teams_multilink_device_priority_mgr[APP_TEAMS_MAX_LINK_NUM - 1].idx = multilink_usb_idx;
    app_teams_multilink_app_low_priority_mgr[APP_TEAMS_MAX_LINK_NUM - 1].music_event =
        JUDGE_EVENT_A2DP_DISC;

    sys_mgr_cback_register(app_teams_multilink_dm_cback);
    gap_reg_timer_cb(app_teams_multilink_timeout_cb, &teams_multilink_timer_queue_id);
}

#endif
