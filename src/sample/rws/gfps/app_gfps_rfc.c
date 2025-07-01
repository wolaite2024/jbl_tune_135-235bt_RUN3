/*
 * Copyright (c) 2018, Realsil Semiconductor Corporation. All rights reserved.
 */

#if GFPS_FEATURE_SUPPORT
#include <string.h>
#include "os_mem.h"
#include "trace.h"
#include "gap_timer.h"
#include "audio.h"
#include "remote.h"
#include "bt_gfps.h"
#include "app_audio_policy.h"
#include "app_bt_policy_api.h"
#include "app_gfps.h"
#include "app_cfg.h"
#include "app_gfps_rfc.h"
#include "app_link_util.h"
#include "app_main.h"
#include "app_sdp.h"
#include "stdlib.h"
#include "app_relay.h"
#include "app_gfps_battery.h"
#include "app_sensor.h"
#include "app_multilink.h"
#include "app_sass_policy.h"
#include "app_gfps_account_key.h"
#include "app_bond.h"
#include "app_anc.h"
#include "app_audio_passthrough.h"
#include "app_mmi.h"
#include "app_relay.h"
#include "app_hfp.h"
#if GFPS_FINDER_SUPPORT
#include "app_gfps_finder.h"
#include "gfps_find_my_device.h"
#include "audio_volume.h"
#endif
#if HARMAN_OPEN_LR_FEATURE
#include "app_harman_vendor_cmd.h"
#endif

static uint8_t gfps_rfc_timer_queue_id = 0;
static void *timer_handle_ring_period = NULL;
static void *timer_handle_ring_timeout = NULL;
static T_GFPS_RFC_RING_STATE gfps_rfc_ring_state = GFPS_RFC_RING_STOP;
static uint8_t gfps_rfc_ring_timeout = 0;

#define GFPS_VOICE_PROMPT_OFFSET   0x80

void app_gfps_reverse_data(uint8_t *data, uint16_t len)
{
    uint8_t *header = data;
    uint8_t *tail = data;
    uint8_t temp;

    tail += (len - 1);

    while (header < tail)
    {
        temp = *header;
        *header++ = *tail;
        *tail-- = temp;
    }
}

#if GFPS_FINDER_SUPPORT
uint8_t app_gfps_rfc_get_ring_timeout(void)
{
    return gfps_rfc_ring_timeout;
}

void app_gfps_rfc_set_ring_timeout(uint8_t ring_timeout)
{
    gfps_rfc_ring_timeout = ring_timeout;
}
#endif

T_GFPS_RFC_RING_STATE app_gfps_rfc_get_ring_state(void)
{
    return gfps_rfc_ring_state;
}

static void app_gfps_rfc_ring_play(void)
{
    audio_volume_out_mute(AUDIO_STREAM_TYPE_PLAYBACK);
#if F_APP_HARMAN_FEATURE_SUPPORT
    if (app_cfg_nv.language_status == 0)
    {
        app_audio_tone_type_play(TONE_ANC_SCENARIO_4, false, false);
    }
    else
#endif
    {
        app_audio_tone_type_play(TONE_GFPS_FINDME, false, false);
    }

    if (gfps_rfc_ring_timeout)
    {
        gap_start_timer(&timer_handle_ring_timeout, "gfps_ring_timeout",
                        gfps_rfc_timer_queue_id, GFPS_RFC_RING_TIMEOUT,
                        0, gfps_rfc_ring_timeout * 1000);
    }
}

static void app_gfps_rfc_ring_stop(void)
{
    audio_volume_out_unmute(AUDIO_STREAM_TYPE_PLAYBACK);
    gfps_rfc_ring_timeout = 0;

#if F_APP_HARMAN_FEATURE_SUPPORT
    if (app_cfg_nv.language_status == 0)
    {
        app_audio_tone_type_cancel(TONE_ANC_SCENARIO_4, false);
    }
    else
#endif
    {
        app_audio_tone_type_cancel(TONE_GFPS_FINDME, false);
    }

    gap_stop_timer(&timer_handle_ring_period);
    gap_stop_timer(&timer_handle_ring_timeout);
}

static void app_gfps_rfc_ring_stop_state(uint8_t event)
{
    APP_PRINT_TRACE3("app_gfps_rfc_ring_stop_state: state 0x%02x, event 0x%02x, bud_side %d",
                     gfps_rfc_ring_state, event, app_cfg_const.bud_side);
    switch (event)
    {
    case GFPS_ALL_STOP:
        {
        }
        break;

    case GFPS_RIGHT_RING:
        {
            if (app_cfg_const.bud_role == REMOTE_SESSION_ROLE_SINGLE)
            {
                app_gfps_rfc_ring_play();
                gfps_rfc_ring_state = GFPS_RFC_RIGHT_RING;
            }
            else
            {
                if (app_cfg_const.bud_side == BUD_SIDE_RIGHT)
                {
                    app_gfps_rfc_ring_play();
                    gfps_rfc_ring_state = GFPS_RFC_RIGHT_RING;
                }
                else
                {
                    app_relay_async_single_with_raw_msg(APP_MODULE_TYPE_GFPS_RFC,
                                                        GFPS_RFC_REMOTE_MSG_FINDME_START,
                                                        &event, 1);
                    gfps_rfc_ring_state = GFPS_RFC_RIGHT_RING;
                }
            }
        }
        break;

    case GFPS_LEFT_RING:
        {
            if (app_cfg_const.bud_side == BUD_SIDE_LEFT)
            {
                app_gfps_rfc_ring_play();
                gfps_rfc_ring_state = GFPS_RFC_LEFT_RING;
            }
            else
            {
                app_relay_async_single_with_raw_msg(APP_MODULE_TYPE_GFPS_RFC,
                                                    GFPS_RFC_REMOTE_MSG_FINDME_START,
                                                    &event, 1);
                gfps_rfc_ring_state = GFPS_RFC_LEFT_RING;
            }
        }
        break;

    case GFPS_ALL_RING:
        {
            if (app_db.remote_session_state == REMOTE_SESSION_STATE_CONNECTED)
            {
                app_relay_sync_single_with_raw_msg(APP_MODULE_TYPE_GFPS_RFC,
                                                   GFPS_RFC_REMOTE_MSG_FINDME_START,
                                                   &event, 1, REMOTE_TIMER_HIGH_PRECISION, 0, false);
                gfps_rfc_ring_state = GFPS_RFC_ALL_RING;
            }
            else
            {
                if (app_cfg_const.bud_side == BUD_SIDE_RIGHT)
                {
                    app_gfps_rfc_ring_play();
                    gfps_rfc_ring_state = GFPS_RFC_RIGHT_RING;
                }
                else
                {
                    app_gfps_rfc_ring_play();
                    gfps_rfc_ring_state = GFPS_RFC_LEFT_RING;
                }
            }
        }
        break;

    default:
        break;
    }
}

static void app_gfps_rfc_right_ring_state(uint8_t event)
{
    APP_PRINT_TRACE3("app_gfps_rfc_right_ring_state: state 0x%02x, event 0x%02x, bud_side %d",
                     gfps_rfc_ring_state, event, app_cfg_const.bud_side);
    switch (event)
    {
    case GFPS_ALL_STOP:
        {
            if (app_cfg_const.bud_role == REMOTE_SESSION_ROLE_SINGLE)
            {
                app_gfps_rfc_ring_stop();
                gfps_rfc_ring_state = GFPS_RFC_RING_STOP;
            }
            else
            {
                if (app_cfg_const.bud_side == BUD_SIDE_RIGHT)
                {
                    app_gfps_rfc_ring_stop();
                    gfps_rfc_ring_state = GFPS_RFC_RING_STOP;
                }
                else
                {
                    app_relay_async_single_with_raw_msg(APP_MODULE_TYPE_GFPS_RFC, GFPS_RFC_REMOTE_MSG_FINDME_STOP,
                                                        &event, 1);
                    gfps_rfc_ring_state = GFPS_RFC_RING_STOP;
                }
            }
        }
        break;

    case GFPS_RIGHT_RING:
        {

        }
        break;

    case GFPS_LEFT_RING:
        {
            if (app_cfg_const.bud_side == BUD_SIDE_RIGHT)
            {
                app_gfps_rfc_ring_stop();
                gfps_rfc_ring_state = GFPS_RFC_RING_STOP;
                app_relay_async_single_with_raw_msg(APP_MODULE_TYPE_GFPS_RFC,
                                                    GFPS_RFC_REMOTE_MSG_FINDME_START,
                                                    &event, 1);
                gfps_rfc_ring_state = GFPS_RFC_LEFT_RING;
            }
            else
            {
                app_gfps_rfc_ring_play();
                app_relay_async_single_with_raw_msg(APP_MODULE_TYPE_GFPS_RFC, GFPS_RFC_REMOTE_MSG_FINDME_STOP,
                                                    &event, 1);
                gfps_rfc_ring_state = GFPS_RFC_RING_STOP;
                gfps_rfc_ring_state = GFPS_RFC_LEFT_RING;
            }
        }
        break;

    case GFPS_ALL_RING:
        {
            if (app_cfg_const.bud_side == BUD_SIDE_LEFT)
            {
                app_gfps_rfc_ring_play();
                gfps_rfc_ring_state = GFPS_RFC_ALL_RING;
            }
            else
            {
                app_relay_async_single_with_raw_msg(APP_MODULE_TYPE_GFPS_RFC,
                                                    GFPS_RFC_REMOTE_MSG_FINDME_START,
                                                    &event, 1);
                gfps_rfc_ring_state = GFPS_RFC_ALL_RING;
            }
        }
        break;

    default:
        break;
    }
}

static void app_gfps_rfc_left_ring_state(uint8_t event)
{
    APP_PRINT_TRACE3("app_gfps_rfc_left_ring_state: state 0x%02x, event 0x%02x, bud_side %d",
                     gfps_rfc_ring_state, event, app_cfg_const.bud_side);
    switch (event)
    {
    case GFPS_ALL_STOP:
        {
            if (app_cfg_const.bud_side == BUD_SIDE_LEFT)
            {
                app_gfps_rfc_ring_stop();
                gfps_rfc_ring_state = GFPS_RFC_RING_STOP;
            }
            else
            {
                app_relay_async_single_with_raw_msg(APP_MODULE_TYPE_GFPS_RFC, GFPS_RFC_REMOTE_MSG_FINDME_STOP,
                                                    &event, 1);
                gfps_rfc_ring_state = GFPS_RFC_RING_STOP;
            }
        }
        break;

    case GFPS_RIGHT_RING:
        {
            if (app_cfg_const.bud_side == BUD_SIDE_LEFT)
            {
                app_gfps_rfc_ring_stop();
                gfps_rfc_ring_state = GFPS_RFC_RING_STOP;
                app_relay_async_single_with_raw_msg(APP_MODULE_TYPE_GFPS_RFC,
                                                    GFPS_RFC_REMOTE_MSG_FINDME_START,
                                                    &event, 1);
                gfps_rfc_ring_state = GFPS_RFC_RIGHT_RING;
            }
            else
            {
                app_gfps_rfc_ring_play();
                app_relay_async_single_with_raw_msg(APP_MODULE_TYPE_GFPS_RFC, GFPS_RFC_REMOTE_MSG_FINDME_STOP,
                                                    &event, 1);
                gfps_rfc_ring_state = GFPS_RFC_RING_STOP;
                gfps_rfc_ring_state = GFPS_RFC_RIGHT_RING;
            }
        }
        break;

    case GFPS_LEFT_RING:
        {

        }
        break;

    case GFPS_ALL_RING:
        {
            if (app_cfg_const.bud_side == BUD_SIDE_RIGHT)
            {
                app_gfps_rfc_ring_play();
                gfps_rfc_ring_state = GFPS_RFC_ALL_RING;
            }
            else
            {
                app_relay_async_single_with_raw_msg(APP_MODULE_TYPE_GFPS_RFC,
                                                    GFPS_RFC_REMOTE_MSG_FINDME_START,
                                                    &event, 1);
                gfps_rfc_ring_state = GFPS_RFC_ALL_RING;
            }
        }
        break;

    default:
        break;
    }
}

static void app_gfps_rfc_all_ring_state(uint8_t event)
{
    APP_PRINT_TRACE3("app_gfps_rfc_all_ring_state: state 0x%02x, event 0x%02x, bud_side %d",
                     gfps_rfc_ring_state, event, app_cfg_const.bud_side);
    switch (event)
    {
    case GFPS_ALL_STOP:
        {
            if (app_db.remote_session_state == REMOTE_SESSION_STATE_CONNECTED)
            {
                app_relay_sync_single_with_raw_msg(APP_MODULE_TYPE_GFPS_RFC, GFPS_RFC_REMOTE_MSG_FINDME_STOP,
                                                   &event, 1, REMOTE_TIMER_HIGH_PRECISION, 0, false);
                gfps_rfc_ring_state = GFPS_RFC_RING_STOP;
            }
        }
        break;

    case GFPS_RIGHT_RING:
        {
            if (app_cfg_const.bud_side == BUD_SIDE_LEFT)
            {
                app_gfps_rfc_ring_stop();
                gfps_rfc_ring_state = GFPS_RFC_RIGHT_RING;
            }
            else
            {
                app_relay_async_single_with_raw_msg(APP_MODULE_TYPE_GFPS_RFC, GFPS_RFC_REMOTE_MSG_FINDME_STOP,
                                                    &event, 1);
                gfps_rfc_ring_state = GFPS_RFC_RIGHT_RING;
            }
        }
        break;

    case GFPS_LEFT_RING:
        {
            if (app_cfg_const.bud_side == BUD_SIDE_RIGHT)
            {
                app_gfps_rfc_ring_stop();
                gfps_rfc_ring_state = GFPS_RFC_LEFT_RING;
            }
            else
            {
                app_relay_async_single_with_raw_msg(APP_MODULE_TYPE_GFPS_RFC, GFPS_RFC_REMOTE_MSG_FINDME_STOP,
                                                    &event, 1);
                gfps_rfc_ring_state = GFPS_RFC_LEFT_RING;
            }
        }
        break;

    case GFPS_ALL_RING:
        {

        }
        break;

    default:
        break;
    }
}

void app_gfps_rfc_handle_ring_event(uint8_t event)
{
    switch (gfps_rfc_ring_state)
    {
    case GFPS_RFC_RING_STOP:
        app_gfps_rfc_ring_stop_state(event);
        break;

    case GFPS_RFC_RIGHT_RING:
        app_gfps_rfc_right_ring_state(event);
        break;

    case GFPS_RFC_LEFT_RING:
        app_gfps_rfc_left_ring_state(event);
        break;

    case GFPS_RFC_ALL_RING:
        app_gfps_rfc_all_ring_state(event);
        break;

    default:
        break;
    }

    if (app_db.remote_session_state == REMOTE_SESSION_STATE_CONNECTED)
    {
        uint8_t data[3];
        data[0] = gfps_rfc_ring_state;
        data[1] = gfps_rfc_ring_timeout >> 8;
        data[2] = gfps_rfc_ring_timeout;
        app_relay_async_single_with_raw_msg(APP_MODULE_TYPE_GFPS_RFC, GFPS_RFC_REMOTE_MSG_FINDME_SYNC,
                                            data, 3);
    }
    APP_PRINT_TRACE2("app_gfps_rfc_handle_ring_event: state 0x%02x, event 0x%02x", gfps_rfc_ring_state,
                     event);
}
#if GFPS_SASS_SUPPORT
bool app_gfps_rfc_report_session_nonce(uint8_t *bd_addr, uint8_t local_server_chann)
{
    uint8_t tmp[8] = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07};
    uint8_t session_nonce[8] = {0};
    uint8_t *p = session_nonce;
    uint8_t data_len = 8;
    T_APP_BR_LINK *p_link = app_find_br_link(bd_addr);
    gfps_gen_random_nonce(tmp);
    memcpy(p_link->gfps_session_nonce, tmp, 8);
    ARRAY_TO_STREAM(p, p_link->gfps_session_nonce, 8);

    if (bt_gfps_send_session_nonce(bd_addr, local_server_chann, session_nonce, data_len))
    {
        return true;
    }
    return false;
}
bool app_gfps_rfc_report_sass_cap(uint8_t *bd_addr, uint8_t local_server_chann)
{
    uint8_t p_data[4] = {0};
    uint8_t data_len = 4;
    uint8_t *p = p_data;
    uint16_t sass_version_code = GFPS_SASS_VERSION;
    uint16_t  sass_cap_flags = 0;
    if (app_sass_policy_support_easy_connection_switch())
    {
        sass_cap_flags |= GFPS_SASS_STATE_ON;
    }
    if (app_cfg_const.enable_multi_link)
    {
        sass_cap_flags |= GFPS_SASS_MULTI_POINT_ENABLE;
    }
    if ((app_sass_policy_get_org_enable_multi_link()) &&
        (extend_app_cfg_const.gfps_sass_support_dyamic_multilink_switch))
    {
        sass_cap_flags |= GFPS_SASS_MULTI_POINT_CFG_SUPPORT;
    }
    if (app_cfg_const.sensor_support)
    {
#if GFPS_SASS_ON_HEAD_DETECT_SUPPORT
        sass_cap_flags |= GFPS_SASS_ON_HEAD_SUPPORT;
        if (app_sensor_ld_is_light_sensor_enabled())
        {
            sass_cap_flags |= GFPS_SASS_ON_HEAD_ENABLE;
        }
#endif
    }
    BE_UINT16_TO_STREAM(p, sass_version_code);
    BE_UINT16_TO_STREAM(p, sass_cap_flags);
    if (bt_gfps_notify_sass_cap(bd_addr, local_server_chann, p_data, data_len))
    {
        return true;
    };
    return false;
}

bool app_gfps_rfc_report_sass_switch_preference(uint8_t *bd_addr, uint8_t local_server_chann)
{
    uint8_t p_data[2] = {0};
    uint8_t data_len = 2;
    uint8_t *p = p_data;
    uint8_t switch_prefer_flag = GFPS_SWITCH_PREFER_HFP_A2DP;
    uint8_t switch_setting = 0;
    switch_prefer_flag = app_sass_policy_get_switch_preference();
    switch_setting = app_sass_policy_get_switch_setting();
    BE_UINT8_TO_STREAM(p, switch_prefer_flag);
    BE_UINT8_TO_STREAM(p, switch_setting);
    if (bt_gfps_notify_switch_prefer(bd_addr, local_server_chann, p_data, data_len))
    {
        return true;
    };
    return false;

}
void  app_gfps_rfc_notify_multipoint_switch(uint8_t active_index, uint8_t switch_reason)
{
    uint8_t target_device = GFPS_MULTIPOINT_SWITCH_THIS_DEVICE;
    uint8_t p_data[4] = {0};
    uint8_t data_len = 4;
    if ((app_db.remote_session_state == REMOTE_SESSION_STATE_CONNECTED
         && app_cfg_nv.bud_role == REMOTE_SESSION_ROLE_PRIMARY) ||
        (app_cfg_const.bud_role == REMOTE_SESSION_ROLE_SINGLE))
    {
        for (uint8_t i = 0; i < MAX_BR_LINK_NUM; i++)
        {
            if (app_db.br_link[i].used == true && (app_db.br_link[i].connected_profile & GFPS_PROFILE_MASK))
            {
                uint8_t *p = p_data;
                T_APP_BR_LINK *p_link = &app_db.br_link[i];
                target_device = active_index == i ? GFPS_MULTIPOINT_SWITCH_THIS_DEVICE :
                                GFPS_MULTIPOINT_SWITCH_OTHER_DEVICE;
                BE_UINT8_TO_STREAM(p, switch_reason);
                BE_UINT8_TO_STREAM(p, target_device);
                //TODO
                //use bd address now, review how much ram increased if need to use remote BT device name
                BE_UINT8_TO_STREAM(p, app_db.br_link[active_index].bd_addr[0]);
                BE_UINT8_TO_STREAM(p, app_db.br_link[active_index].bd_addr[1]);
                bt_gfps_notify_multipoint_switch(p_link->bd_addr, p_link->gfps_rfc_chann, p_data, data_len);
            }
        }
    }
}

uint8_t app_gfps_get_conn_state(T_APP_BR_LINK *p_link, uint8_t active_idx)
{
    uint8_t state = CONNECTION_STATE_CONNECTED_WITH_NO_DATA_TRANSFERRING;

    if (p_link->call_status != BT_HFP_CALL_IDLE)
    {
        state = CONNECTION_STATE_HFP_STREAMING;
    }
    else if (p_link->streaming_fg)
    {
        if (p_link->avrcp_play_status == BT_AVRCP_PLAY_STATUS_PLAYING &&
            !app_multilink_get_stream_only(active_idx))
        {
            state = CONNECTION_STATE_A2DP_STREAMING_WITH_AVRCP;
        }
        else
        {
            state = CONNECTION_STATE_A2DP_STREAMING_ONLY;
        }
    }
    else
    {
        state = CONNECTION_STATE_CONNECTED_WITH_NO_DATA_TRANSFERRING;
        //check LE audio
    }

    return state;
}

bool app_gfps_rfc_notify_connection_status(void)
{
    bool ret = false;
    uint8_t msg_nonce[8] = {0x66, 0x5B, 0x52, 0x91, 0x7F, 0x25, 0x07, 0x16};
    uint8_t flag = GFPS_NON_ACTIVE_DEV;
    uint8_t active_index = 0;
    T_SASS_CONN_STATUS_FIELD conn_status;
    uint8_t inuse_account_key_index = gfps_get_inuse_account_key_index();
    active_index = app_multilink_get_active_idx();
    gfps_gen_random_nonce(msg_nonce);
    gfps_sass_get_conn_status(&conn_status);
    app_gfps_gen_conn_status(&conn_status);
    gfps_sass_set_conn_status(&conn_status);
    if ((app_db.remote_session_state == REMOTE_SESSION_STATE_CONNECTED
         && app_cfg_nv.bud_role == REMOTE_SESSION_ROLE_PRIMARY) ||
        (app_cfg_const.bud_role == REMOTE_SESSION_ROLE_SINGLE))
    {
        for (uint8_t i = 0; i < MAX_BR_LINK_NUM; i++)
        {
            T_APP_BR_LINK *p_link = &app_db.br_link[i];

            app_gfps_gen_conn_status(&conn_status);
            if (i == active_index)
            {
                flag = GFPS_ACTIVE_DEV;
            }
            else
            {
                flag = GFPS_NON_ACTIVE_DEV;

                if ((app_db.br_link[active_index].gfps_inuse_account_key == 0xff) &&
                    ((app_db.br_link[active_index].streaming_fg == 1) ||
                     (app_db.br_link[active_index].call_status != BT_HFP_CALL_IDLE)))
                {
                    flag = GFPS_NON_ACTIVE_DEV_WITH_ACTIVE_NON_SASS;
                }
            }
            //notify the active account key user only
            if (p_link->used == true &&
                (p_link->connected_profile & GFPS_PROFILE_MASK))
            {
                uint8_t session_nonce_tmp[8];
                uint8_t *p = session_nonce_tmp;

                ARRAY_TO_STREAM(p, p_link->gfps_session_nonce, 8);

                if ((i != active_index) &&
                    (app_db.br_link[active_index].gfps_inuse_account_key != 0xff) &&
                    (app_db.br_link[active_index].gfps_inuse_account_key != p_link->gfps_inuse_account_key))
                {
                    conn_status.conn_status_info.conn_state = app_gfps_get_conn_state(p_link, active_index);
                }
                gfps_sass_set_conn_status(&conn_status);

                APP_PRINT_INFO6("app_gfps_rfc_notify_connection_status: cur_index: %d, active_index: %d, "
                                "cur_account_key_index: %d, active_account_key_index: %d, "
                                "conn_state: %d, on_head_detection: %d",
                                i, active_index,
                                p_link->gfps_inuse_account_key, app_db.br_link[active_index].gfps_inuse_account_key,
                                conn_status.conn_status_info.conn_state,
                                conn_status.conn_status_info.on_head_detection);
                ret = bt_gfps_notify_connection_status(p_link->bd_addr, p_link->gfps_rfc_chann,
                                                       p_link->gfps_inuse_account_key, flag, conn_status,
                                                       msg_nonce, session_nonce_tmp);
            }
        }
    }
    return ret;
}

static void app_gfps_rfc_report_nac(uint8_t *bd_addr, uint8_t local_server_chann,
                                    uint8_t msg_group, uint8_t msg_code, uint8_t error_code)
{
    uint8_t ack_pkt[2];
    uint8_t *temp = ack_pkt;
    BE_UINT8_TO_STREAM(temp, msg_group);
    BE_UINT8_TO_STREAM(temp, msg_code);
    bt_gfps_send_nak(bd_addr, local_server_chann, ack_pkt, 2, error_code);
}
static void app_gfps_rfc_report_mac_error(uint8_t *bd_addr, uint8_t local_server_chann,
                                          uint8_t msg_group, uint8_t msg_code)
{
    app_gfps_rfc_report_nac(bd_addr, local_server_chann, msg_group, msg_code, GFPS_ERROR_MAC);
}

void app_gfps_report_anc_state(uint8_t *bd_addr, uint8_t local_server_chann)
{
    uint8_t sass_version = GFPS_HEARABLE_VERSION;
    uint16_t anc_state = GFPS_NO_ANC;
#if (F_APP_ANC_SUPPORT && F_APP_APT_SUPPORT)
    uint8_t llapt_cnt = app_apt_get_llapt_selected_scenario_cnt() == 0 ?
                        app_apt_get_llapt_activated_scenario_cnt() : app_apt_get_llapt_selected_scenario_cnt();
    uint8_t anc_cnt = app_anc_get_selected_scenario_cnt() == 0 ? app_anc_get_activated_scenario_cnt() :
                      app_anc_get_selected_scenario_cnt();

    if ((llapt_cnt != 0) || (anc_cnt != 0))
    {
        if ((app_cfg_nv.listening_mode_cycle == 0) ||
            (app_cfg_nv.listening_mode_cycle == 1) ||
            (app_cfg_nv.listening_mode_cycle == 2))
        {
            anc_state |= GFPS_ANC_PATTERN3;
            if (app_apt_is_apt_on_state((T_ANC_APT_STATE)*app_db.final_listening_state))
            {
                anc_state |= GFPS_ANC_PATTERN3_TRANS_MODE;
            }
            else if (app_anc_is_anc_on_state((T_ANC_APT_STATE)*app_db.final_listening_state))
            {
                anc_state |= GFPS_ANC_PATTERN3_ON;
            }
            else
            {
                anc_state |= GFPS_ANC_PATTERN3_OFF;
            }
        }
        else if (app_cfg_nv.listening_mode_cycle == 3)
        {
            anc_state |= GFPS_ANC_PATTERN1;
            if (app_anc_is_anc_on_state((T_ANC_APT_STATE)*app_db.final_listening_state))
            {
                anc_state |= GFPS_ANC_PATTERN1_ON;
            }
            else
            {
                anc_state |= GFPS_ANC_PATTERN1_OFF;
            }
        }
    }
#endif
    uint8_t p_data[3] = {0};
    uint8_t data_len = 3;
    uint8_t *p = p_data;
    BE_UINT8_TO_STREAM(p, sass_version);
    BE_UINT16_TO_STREAM(p, anc_state);
    bt_gfps_send_anc_state(bd_addr, local_server_chann, p_data, data_len);

}
void app_gfps_notify_anc_state(void)
{
    if ((app_cfg_nv.bud_role == REMOTE_SESSION_ROLE_PRIMARY) ||
        (app_cfg_const.bud_role == REMOTE_SESSION_ROLE_SINGLE))
    {
        for (uint8_t i = 0; i < MAX_BR_LINK_NUM; i++)
        {
            if (app_db.br_link[i].used == true && (app_db.br_link[i].connected_profile & GFPS_PROFILE_MASK))
            {
                T_APP_BR_LINK *p_link = &app_db.br_link[i];
                app_gfps_report_anc_state(p_link->bd_addr, p_link->gfps_rfc_chann);
            }
        }
    }
}
bool app_gfps_set_anc_state(uint8_t version, uint16_t anc_state)
{
    bool ret = true;
    uint8_t mmi_action =  MMI_NULL;
    if (version == GFPS_HEARABLE_VERSION)
    {
        ret = true;
        uint16_t pattern = anc_state & 0xff00;
        uint8_t action = (uint8_t) anc_state;
        switch (pattern)
        {
        case GFPS_ANC_PATTERN1:
            {
                if (action == GFPS_ANC_PATTERN1_OFF)
                {
                    mmi_action = MMI_AUDIO_ANC_APT_ALL_OFF;
                }
                else
                {
                    mmi_action = MMI_AUDIO_ANC_ON;
                }
            }
            break;
        case GFPS_ANC_PATTERN2:
            {
                if (action == GFPS_ANC_PATTERN2_OFF)
                {
                    mmi_action = MMI_AUDIO_ANC_APT_ALL_OFF;
                }
                else
                {
                    mmi_action = MMI_AUDIO_ANC_ON;
                }

            }
            break;
        case GFPS_ANC_PATTERN3:
            {
                if (action == GFPS_ANC_PATTERN3_TRANS_MODE)
                {
                    mmi_action = MMI_AUDIO_APT_ON;
                }
                else if (action == GFPS_ANC_PATTERN3_OFF)
                {
                    mmi_action = MMI_AUDIO_ANC_APT_ALL_OFF;
                }
                else
                {
                    mmi_action = MMI_AUDIO_ANC_ON;
                }
            }
            break;
        default:
            ret = false;
            break;
        }

    }
    if (ret)
    {
        if (app_db.remote_session_state == REMOTE_SESSION_STATE_CONNECTED)
        {
            app_relay_sync_single(APP_MODULE_TYPE_MMI, mmi_action, REMOTE_TIMER_HIGH_PRECISION,
                                  0, false);
        }
        else
        {
            app_mmi_handle_action(mmi_action);

        }
    }
    return ret;
}
void app_gfps_rfc_report_dynamic_buffer_size(uint8_t *bd_addr, uint8_t local_server_chann)
{
    uint8_t *p_data = NULL;
    uint8_t *p;
    uint8_t codec_num = 1;

    uint8_t data_len  = GFPS_BUFFER_SIZE_INFO_LEN_PER_CODEC_SBC_ONLY;
    if (app_cfg_const.a2dp_codec_type_aac)
    {
        data_len  = GFPS_BUFFER_SIZE_INFO_LEN_PER_CODEC_AAC;
    }
#if (F_APP_A2DP_CODEC_LDAC_SUPPORT == 1)
    if (app_cfg_const.a2dp_codec_type_ldac)
    {
        data_len  = GFPS_BUFFER_SIZE_INFO_LEN_PER_CODEC_LDAC;
    }
#endif

#if F_APP_HARMAN_FEATURE_SUPPORT
    uint16_t default_latency = app_cfg_nv.cmd_latency_in_ms;
#else
    uint16_t default_latency = A2DP_LATENCY_MS;
#endif
    uint16_t max_latency = default_latency;
    uint16_t min_latency = 20;
    if (app_cfg_const.a2dp_codec_type_aac)
    {
        codec_num ++;
    }
#if (F_APP_A2DP_CODEC_LDAC_SUPPORT == 1)
    if (app_cfg_const.a2dp_codec_type_ldac)
    {
        codec_num ++;
    }
#endif
    p_data = malloc(data_len);

    if (p_data != NULL)
    {
        p = p_data;
        /*
         1 byte codec type
         2 bytes maximum buffer size
         2 bytes minimum buffer size
         2 bytes default buffer size
        */
        app_audio_get_latency_value_by_level(AUDIO_FORMAT_TYPE_SBC, 0, &min_latency);
        BE_UINT8_TO_STREAM(p, GFPS_CODEC_SBC);
        BE_UINT16_TO_STREAM(p, max_latency);//use defefault buffer size as max
        BE_UINT16_TO_STREAM(p, min_latency);//use defefault buffer size as max
        BE_UINT16_TO_STREAM(p, default_latency);//use defefault buffer size as max
        if (app_cfg_const.a2dp_codec_type_aac)
        {
            app_audio_get_latency_value_by_level(AUDIO_FORMAT_TYPE_AAC, 0, &min_latency);
            BE_UINT8_TO_STREAM(p, GFPS_CODEC_AAC);
            BE_UINT16_TO_STREAM(p, max_latency);//use defefault buffer size as max
            BE_UINT16_TO_STREAM(p, min_latency);//use defefault buffer size as max
            BE_UINT16_TO_STREAM(p, default_latency);//use defefault buffer size as max

        }
#if (F_APP_A2DP_CODEC_LDAC_SUPPORT == 1)
        if (app_cfg_const.a2dp_codec_type_ldac)
        {
            //no LDAC low latency setting, use AAC
            app_audio_get_latency_value_by_level(AUDIO_FORMAT_TYPE_AAC, 0, &min_latency);
            BE_UINT8_TO_STREAM(p, GFPS_CODEC_LDAC);
            BE_UINT16_TO_STREAM(p, max_latency);//use defefault buffer size as max
            BE_UINT16_TO_STREAM(p, min_latency);//use defefault buffer size as max
            BE_UINT16_TO_STREAM(p, default_latency);//use defefault buffer size as max
        }
#endif

        bt_gfps_send_dynamic_buffer_size(bd_addr, local_server_chann, p_data, data_len);
        free(p_data);
    }

}
void app_gfps_rfc_set_buffer_size(T_GFPS_CODEC_TYPE codec_type, uint16_t latency)
{
    //TODO
}
void app_gfps_rfc_update_inuse_account_key_index(uint8_t *bd_addr, uint8_t index)
{
    T_APP_BR_LINK *p_link = NULL;
    p_link = app_find_br_link(bd_addr);
    if (p_link != NULL)
    {
        uint8_t active_idx = app_multilink_get_active_idx() < MAX_BR_LINK_NUM ?
                             app_multilink_get_active_idx() : p_link->id;
        p_link->gfps_inuse_account_key = index;
    }
}
void app_gfps_rfc_update_custom_data(uint8_t custom_data)
{
    T_SASS_CONN_STATUS_FIELD conn_status;
    gfps_sass_get_conn_status(&conn_status);
    conn_status.custom_data = custom_data;
    gfps_sass_set_conn_status(&conn_status);
    if ((app_cfg_nv.bud_role == REMOTE_SESSION_ROLE_PRIMARY) &&
        (app_db.remote_session_state == REMOTE_SESSION_STATE_CONNECTED))
    {
        uint8_t buf = custom_data;
        app_relay_async_single_with_raw_msg(APP_MODULE_TYPE_GFPS_RFC,
                                            GFPS_RFC_REMOTE_MSG_SYNC_CUSTOM_DATA,
                                            &buf, 1);

    }

}
void app_gfps_rfc_update_target_drop_device(uint8_t *bd_addr)
{
    if ((app_cfg_nv.bud_role == REMOTE_SESSION_ROLE_PRIMARY) &&
        (app_db.remote_session_state == REMOTE_SESSION_STATE_CONNECTED))
    {
        uint8_t buf[6] = {0};
        memcpy(buf, bd_addr, 6);
        app_relay_async_single_with_raw_msg(APP_MODULE_TYPE_GFPS_RFC,
                                            GFPS_RFC_REMOTE_MSG_SYNC_TARGET_DROP_DEVICE,
                                            buf, 6);

    }
    memcpy(app_db.sass_target_drop_device, bd_addr, 6);
}
#endif

static void app_gfps_rfc_handle_data_transfer(uint8_t *bd_addr, uint8_t server_chann,
                                              uint8_t *p_cmd,
                                              uint16_t cmd_len)
{
    uint8_t msg_group;
    uint8_t msg_code;
    uint8_t *p;
    uint16_t len;

    p = p_cmd;
    len = cmd_len;

    while (len >= GFPS_HEADER_LEN)
    {
        bool unknow_cmd = false;
        BE_STREAM_TO_UINT8(msg_group, p);
        BE_STREAM_TO_UINT8(msg_code, p);
        APP_PRINT_TRACE3("app_gfps_rfc_dm_cback: msg_group 0x%02x, msg_code 0x%02x, len %d",
                         msg_group, msg_code, len);

        if (msg_group == GFPS_DEVICE_INFO_EVENT)
        {
            switch (msg_code)
            {
            case GFPS_ACITVE_COMPONENTS_REQ:
                {
                    uint8_t active_components = GFPS_RIGHT_ACTIVE;

                    if (app_cfg_const.bud_role == REMOTE_SESSION_ROLE_SINGLE)
                    {
                        active_components = GFPS_RIGHT_ACTIVE;
                    }
                    else
                    {
                        // default left bud as primary role
                        if (app_cfg_const.bud_side == BUD_SIDE_LEFT)
                        {
                            active_components = GFPS_LEFT_ACTIVE;
                        }
                        else if (app_cfg_const.bud_side == BUD_SIDE_RIGHT)
                        {
                            active_components = GFPS_RIGHT_ACTIVE;
                        }
                        if (app_db.remote_session_state == REMOTE_SESSION_STATE_CONNECTED)
                        {
                            active_components = (GFPS_LEFT_ACTIVE | GFPS_RIGHT_ACTIVE);
                        }
                    }

                    bt_gfps_active_components_rsp(bd_addr, server_chann,
                                                  &active_components,
                                                  GFPS_ACTIVE_COMPONENTS_LEN);
                    len -= GFPS_HEADER_LEN;
                    p += GFPS_ADDITIONAL_DATA_LEN;
                }
                break;

            case GFPS_CAPABILITIES:
                {
                    uint8_t capabilities;
                    p += GFPS_ADDITIONAL_DATA_LEN;
                    BE_STREAM_TO_UINT8(capabilities, p);
                    if (capabilities & GFPS_COMPANION_APP)
                    {
                        APP_PRINT_TRACE0("app_gfps_rfc_dm_cback: support companent app");
                    }

                    if (capabilities & GFPS_SILENCE_MODE)
                    {
                        APP_PRINT_TRACE0("app_gfps_rfc_dm_cback: support silence mode");
                    }
                    APP_PRINT_TRACE1("app_gfps_rfc_dm_cback: capabilities 0x%02x", capabilities);
                    len -= (GFPS_HEADER_LEN + GFPS_SUPPORT_CPBS_LEN);
                }
                break;
            case GFPS_PLATFORM_TYPE:
                {
                    uint8_t platform_type = 0;
                    uint8_t platform_version = 0;
                    p += GFPS_ADDITIONAL_DATA_LEN;
                    BE_STREAM_TO_UINT8(platform_type, p);
                    BE_STREAM_TO_UINT8(platform_version, p);
                    APP_PRINT_TRACE2("app_gfps_rfc_dm_cback: platform type 0x%x, platform version 0x%x", platform_type,
                                     platform_version);
                    len -= (GFPS_HEADER_LEN + GFPS_PLATFORM_TYPE_LEN);
                }
                break;
            default:
                {
                    unknow_cmd = true;
                }
                break;
            }
        }
        else if (msg_group == GFPS_DEVICE_ACTION_EVENT)
        {
            switch (msg_code)
            {
            case GFPS_RING:
                {
                    T_GFPS_RFC_RING_STATE pre_state = app_gfps_rfc_get_ring_state();
                    uint8_t ring_type;
                    uint16_t ring_len;
                    uint8_t data[GFPS_RING_ACK_LEN];
                    uint8_t *temp = data;
                    BE_STREAM_TO_UINT16(ring_len, p);
                    BE_STREAM_TO_UINT8(ring_type, p);
                    if (ring_type == GFPS_ALL_STOP)
                    {
                        app_gfps_rfc_handle_ring_event(ring_type);
                    }
                    else
                    {
                        // The second byte in additional data, if present, represents the timeout in seconds
                        if (ring_len != GFPS_RING_LEN)
                        {
                            BE_STREAM_TO_UINT8(gfps_rfc_ring_timeout, p);
                        }
                        app_gfps_rfc_handle_ring_event(ring_type);
                    }
                    APP_PRINT_TRACE2("app_gfps_rfc_dm_cback: ring_type 0x%02x, gfps_rfc_ring_timeout 0x%02x", ring_type,
                                     gfps_rfc_ring_timeout);

                    BE_UINT8_TO_STREAM(temp, msg_group);
                    BE_UINT8_TO_STREAM(temp, msg_code);
                    BE_UINT8_TO_STREAM(temp, ring_type);
                    BE_UINT8_TO_STREAM(temp, gfps_rfc_ring_timeout);

                    if (pre_state != app_gfps_rfc_get_ring_state())
                    {
                        bt_gfps_send_ack(bd_addr, server_chann, data, GFPS_RING_ACK_LEN);
                    }
                    else
                    {
                        bt_gfps_send_nak(bd_addr, server_chann, data, GFPS_RING_NAK_LEN, GFPS_NOT_ALLOW);
                    }
                    len -= (GFPS_HEADER_LEN + ring_len);
                }
                break;

            default:
                {
                    unknow_cmd = true;
                }
                break;
            }
        }
        else if (msg_group == GFPS_DEVICE_CAPABILITY_SYNC_EVENT)
        {
            uint16_t additional_data_len = 0;
            uint8_t message_nonce[8];
            uint8_t mac[8];
            BE_STREAM_TO_UINT16(additional_data_len, p);
            len -= (GFPS_HEADER_LEN + additional_data_len);
            switch (msg_code)
            {
            case GFPS_REQ_CAPABILITY_UPDATE:
                {
#if GFPS_SASS_SUPPORT
                    if (app_sass_policy_support_easy_connection_switch() &&
                        (additional_data_len == GFPS_REQ_CAPABILITY_UPDATE_PARA_LEN))
                    {
                        app_gfps_rfc_report_dynamic_buffer_size(bd_addr, server_chann);
                    }
#endif

#if GFPS_FINDER_SUPPORT
                    if (extend_app_cfg_const.gfps_finder_support)
                    {
                        app_gfps_finder_send_eddystone_capability(bd_addr, server_chann);
                    }
#endif
                }
                break;
            }
        }
#if GFPS_SASS_SUPPORT
        else if ((msg_group == GFPS_DEVICE_RUNTIME_CFG_EVENT) &&
                 app_sass_policy_support_easy_connection_switch())
        {
            uint16_t additional_data_len = 0;
            uint8_t message_nonce[8];
            uint8_t mac[8];
            BE_STREAM_TO_UINT16(additional_data_len, p);
            len -= (GFPS_HEADER_LEN + additional_data_len);
            switch (msg_code)
            {
            case GFPS_DYNAMIC_BUFFER_SIZING:
                {
                    uint8_t codec_type = 0;
                    uint16_t latency = 0;
                    BE_STREAM_TO_UINT8(codec_type, p);
                    BE_STREAM_TO_UINT16(latency, p);
                    STREAM_TO_ARRAY(message_nonce, p, 8);
                    STREAM_TO_ARRAY(mac, p, 8);
                    if (additional_data_len == GFPS_DYNAMIC_BUFFER_SIZING_PARA_LEN)
                    {
                        uint8_t msg[3];
                        msg[0] = (uint8_t)codec_type;
                        msg[1] = latency >> 8;
                        msg[2] = (uint8_t) latency;
                        if (app_gfps_account_key_verify_mac(bd_addr, msg, GFPS_SET_ANC_STATE_PARA_LEN, message_nonce,
                                                            mac))
                        {
                            uint8_t ack_len = GFPS_STANARD_ACK_PAYLOAD_LEN + GFPS_DYNAMIC_BUFFER_SIZING_PARA_LEN;
                            uint8_t ack_pkt[ack_len];
                            uint8_t *temp = ack_pkt;
                            BE_UINT8_TO_STREAM(temp, msg_group);
                            BE_UINT8_TO_STREAM(temp, msg_code);
                            BE_UINT16_TO_STREAM(temp, codec_type);
                            BE_UINT16_TO_STREAM(temp, latency);
                            bt_gfps_send_ack(bd_addr, server_chann, ack_pkt, ack_len);
                            app_gfps_rfc_set_buffer_size((T_GFPS_CODEC_TYPE)codec_type, codec_type);

                        }
                        else
                        {
                            uint8_t ack_pkt[GFPS_MAC_ERROR_NAK_LEN + GFPS_DYNAMIC_BUFFER_SIZING_PARA_LEN];
                            uint8_t *temp = ack_pkt;
                            uint16_t current_latency = 220;
                            app_audio_get_last_used_latency(&current_latency);
                            BE_UINT8_TO_STREAM(temp, msg_group);
                            BE_UINT8_TO_STREAM(temp, msg_code);
                            BE_UINT8_TO_STREAM(temp, codec_type);
                            BE_UINT16_TO_STREAM(temp, current_latency);
                            bt_gfps_send_nak(bd_addr, server_chann, ack_pkt,
                                             GFPS_MAC_ERROR_NAK_LEN + GFPS_DYNAMIC_BUFFER_SIZING_PARA_LEN, GFPS_ERROR_MAC);

                        }

                    }
                }
                break;
            }

        }
        else if ((msg_group == GFPS_SMART_AUDIO_SWITCH) &&
                 app_sass_policy_support_easy_connection_switch())
        {
            uint16_t additional_data_len = 0;
            uint8_t message_nonce[8];
            uint8_t mac[8];
            BE_STREAM_TO_UINT16(additional_data_len, p);
            len -= (GFPS_HEADER_LEN + additional_data_len);
            switch (msg_code)
            {
            case GFPS_GET_SASS_CAP:
                {
                    if (additional_data_len == GFPS_GET_SASS_CAP_DATA_LEN)
                    {
                        app_sass_policy_set_support(bd_addr);
                        app_gfps_rfc_report_sass_cap(bd_addr, server_chann);
                    }
                }
                break;
            case GFPS_NOTIFY_SASS_CAP:
                {
                    uint16_t sass_version = 0;
                    uint16_t sass_cap_flags = 0;
                    BE_STREAM_TO_UINT16(sass_version, p);
                    BE_STREAM_TO_UINT16(sass_cap_flags, p);
                    STREAM_TO_ARRAY(message_nonce, p, 8);
                    STREAM_TO_ARRAY(mac, p, 8);
                    if (additional_data_len == GFPS_NOTIFY_SASS_CAP_DATA_LEN)
                    {
                        //verify mac
                        uint8_t ack_len = GFPS_STANARD_ACK_PAYLOAD_LEN + GFPS_NOTIFY_SASS_CAP_PARA_LEN;
                        uint8_t ack_pkt[ack_len];
                        uint8_t *temp = ack_pkt;
                        BE_UINT8_TO_STREAM(temp, msg_group);
                        BE_UINT8_TO_STREAM(temp, msg_code);
                        BE_UINT16_TO_STREAM(temp, sass_version);
                        BE_UINT16_TO_STREAM(temp, sass_cap_flags);
                        bt_gfps_send_ack(bd_addr, server_chann, ack_pkt, ack_len);
                    }
                }
                break;
            case GFPS_SET_MULTI_POINT_STATE:
                {
                    uint8_t multi_point_state = 0;
                    BE_STREAM_TO_UINT8(multi_point_state, p);
                    STREAM_TO_ARRAY(message_nonce, p, 8);
                    STREAM_TO_ARRAY(mac, p, 8);
                    if (additional_data_len == GFPS_SET_MULTI_POINT_STATE_DATA_LEN)
                    {
                        uint8_t msg[1] = {0};
                        msg[0] = multi_point_state;
                        if (app_gfps_account_key_verify_mac(bd_addr, msg, GFPS_SET_MULTI_POINT_STATE_PARA_LEN,
                                                            message_nonce,
                                                            mac))
                        {
                            uint8_t ack_len = GFPS_STANARD_ACK_PAYLOAD_LEN + GFPS_SET_MULTI_POINT_STATE_PARA_LEN;
                            uint8_t ack_pkt[ack_len];
                            uint8_t *temp = ack_pkt;
                            BE_UINT8_TO_STREAM(temp, msg_group);
                            BE_UINT8_TO_STREAM(temp, msg_code);
                            BE_UINT8_TO_STREAM(temp, multi_point_state);
                            bt_gfps_send_ack(bd_addr, server_chann, ack_pkt, ack_len);
                            //set multipoint enable
                            app_sass_policy_set_multipoint_state(multi_point_state);
                        }
                        else
                        {
                            app_gfps_rfc_report_mac_error(bd_addr, server_chann, msg_group, msg_code);
                        }

                    }
                }
                break;
            case GFPS_SET_SWITCH_PREFER:
                {
                    uint8_t switch_preference = 0;
                    uint8_t advanced_switching_setting = 0;
                    BE_STREAM_TO_UINT8(switch_preference, p);
                    BE_STREAM_TO_UINT8(advanced_switching_setting, p);
                    STREAM_TO_ARRAY(message_nonce, p, 8);
                    STREAM_TO_ARRAY(mac, p, 8);
                    if (additional_data_len == GFPS_SET_SWITCH_PREFER_DATA_LEN)
                    {
                        uint8_t msg[2] = {0};
                        msg[0] = switch_preference;
                        msg[1] = advanced_switching_setting;
                        if (app_gfps_account_key_verify_mac(bd_addr, msg, GFPS_SET_SWITCH_PREFER_PARA_LEN, message_nonce,
                                                            mac))
                        {
                            uint8_t ack_len = GFPS_STANARD_ACK_PAYLOAD_LEN + GFPS_SET_SWITCH_PREFER_PARA_LEN;
                            uint8_t ack_pkt[ack_len];
                            uint8_t *temp = ack_pkt;
                            BE_UINT8_TO_STREAM(temp, msg_group);
                            BE_UINT8_TO_STREAM(temp, msg_code);
                            BE_UINT8_TO_STREAM(temp, switch_preference);
                            BE_UINT8_TO_STREAM(temp, advanced_switching_setting);
                            bt_gfps_send_ack(bd_addr, server_chann, ack_pkt, ack_len);
                            app_sass_policy_set_switch_preference(switch_preference);
                            app_sass_policy_set_switch_setting(advanced_switching_setting);
                        }
                        else
                        {
                            app_gfps_rfc_report_mac_error(bd_addr, server_chann, msg_group, msg_code);
                        }

                    }
                }
                break;
            case GFPS_GET_SWITCH_PREFER:
                {
                    if (additional_data_len == GFPS_GET_SWITCH_PREFER_DATA_LEN)
                    {
                        app_gfps_rfc_report_sass_switch_preference(bd_addr, server_chann);
                    }
                }
                break;
            case GFPS_NOTIFY_SWITCH_PREFER:
                {
                    uint8_t switch_preference = 0;
                    uint8_t advanced_switching_setting = 0;
                    BE_STREAM_TO_UINT8(switch_preference, p);
                    BE_STREAM_TO_UINT8(advanced_switching_setting, p);
                    STREAM_TO_ARRAY(message_nonce, p, 8);
                    STREAM_TO_ARRAY(mac, p, 8);
                    if (additional_data_len == GFPS_NOTIFY_SWITCH_PREFER_DATA_LEN)
                    {
                        uint8_t msg[2] = {0};
                        msg[0] = switch_preference;
                        msg[1] = advanced_switching_setting;
                        if (app_gfps_account_key_verify_mac(bd_addr, msg, GFPS_NOTIFY_SWITCH_PREFER_PARA_LEN, message_nonce,
                                                            mac))
                        {
                            uint8_t ack_len = GFPS_STANARD_ACK_PAYLOAD_LEN + GFPS_NOTIFY_SWITCH_PREFER_PARA_LEN;
                            uint8_t ack_pkt[ack_len];
                            uint8_t *temp = ack_pkt;
                            BE_UINT8_TO_STREAM(temp, msg_group);
                            BE_UINT8_TO_STREAM(temp, msg_code);
                            BE_UINT8_TO_STREAM(temp, switch_preference);
                            BE_UINT8_TO_STREAM(temp, advanced_switching_setting);
                            bt_gfps_send_ack(bd_addr, server_chann, ack_pkt, ack_len);
                            app_sass_policy_set_switch_preference(switch_preference);
                            app_sass_policy_set_switch_setting(advanced_switching_setting);
                        }
                        else
                        {
                            app_gfps_rfc_report_mac_error(bd_addr, server_chann, msg_group, msg_code);
                        }

                    }
                }
                break;
            case GFPS_SWITCH_ACTIVE_AUDIO_SOURCE:
                {
                    uint8_t sass_event_flags = 0;
                    BE_STREAM_TO_UINT8(sass_event_flags, p);
                    STREAM_TO_ARRAY(message_nonce, p, 8);
                    STREAM_TO_ARRAY(mac, p, 8);
                    if (additional_data_len == GFPS_SWITCH_ACTIVE_AUDIO_SOURCE_DATA_LEN)
                    {
                        uint8_t msg[1];
                        msg[0] = sass_event_flags;
                        if (app_gfps_account_key_verify_mac(bd_addr, msg, GFPS_SWITCH_ACTIVE_AUDIO_SOURCE_PARA_LEN,
                                                            message_nonce, mac))
                        {
                            uint8_t ack_len = GFPS_STANARD_ACK_PAYLOAD_LEN + GFPS_SWITCH_ACTIVE_AUDIO_SOURCE_PARA_LEN;
                            uint8_t ack_pkt[ack_len];
                            uint8_t *temp = ack_pkt;
                            uint8_t ret = SWITCH_RET_OK;
                            BE_UINT8_TO_STREAM(temp, msg_group);
                            BE_UINT8_TO_STREAM(temp, msg_code);
                            BE_UINT8_TO_STREAM(temp, sass_event_flags);
                            ret = app_sass_policy_switch_active_link(bd_addr, sass_event_flags, sass_event_flags & 0b10000000);
                            if (ret != SWITCH_RET_OK)
                            {
                                if (ret == SWITCH_RET_REDUNDANT)
                                {
                                    app_gfps_rfc_report_nac(bd_addr, server_chann, msg_group, msg_code, GFPS_REDUNDANT_DEVICE_ACTION);
                                }
                                else
                                {
                                    app_gfps_rfc_report_nac(bd_addr, server_chann, msg_group, msg_code, GFPS_NOT_ALLOW);
                                }
                            }
                            else
                            {
                                bt_gfps_send_ack(bd_addr, server_chann, ack_pkt, ack_len);
                            }

                        }
                        else
                        {
                            app_gfps_rfc_report_mac_error(bd_addr, server_chann, msg_group, msg_code);
                        }

                    }
                }
                break;
            case GFPS_SWITCH_BACK:
                {
                    uint8_t switch_back_event = 0;
                    BE_STREAM_TO_UINT8(switch_back_event, p);
                    STREAM_TO_ARRAY(message_nonce, p, 8);
                    STREAM_TO_ARRAY(mac, p, 8);
                    if (additional_data_len == GFPS_SWITCH_BACK_DATA_LEN)
                    {
                        uint8_t msg[1];
                        msg[0] = switch_back_event;
                        if (app_gfps_account_key_verify_mac(bd_addr, msg, GFPS_SWITCH_BACK_PARA_LEN, message_nonce, mac))
                        {
                            uint8_t ack_len = GFPS_STANARD_ACK_PAYLOAD_LEN + GFPS_SWITCH_BACK_PARA_LEN;
                            uint8_t ack_pkt[ack_len];
                            uint8_t *temp = ack_pkt;
                            BE_UINT8_TO_STREAM(temp, msg_group);
                            BE_UINT8_TO_STREAM(temp, msg_code);
                            BE_UINT8_TO_STREAM(temp, switch_back_event);
                            bt_gfps_send_ack(bd_addr, server_chann, ack_pkt, ack_len);
                            app_sass_policy_switch_back(switch_back_event);
                        }
                        else
                        {
                            app_gfps_rfc_report_mac_error(bd_addr, server_chann, msg_group, msg_code);
                        }

                    }
                }
                break;
            case GFPS_GET_CONN_STATUS:
                {
                    if (additional_data_len == GFPS_GET_CONN_STATUS_DATA_LEN)
                    {
                        app_gfps_rfc_notify_connection_status();
                    }
                }
                break;

            case GFPS_NOTIFY_SASS_INITIAL_CONN:
                {
                    uint8_t sass_initiated_connection = false;
                    BE_STREAM_TO_UINT8(sass_initiated_connection, p);
                    STREAM_TO_ARRAY(message_nonce, p, 8);
                    STREAM_TO_ARRAY(mac, p, 8);
                    if (additional_data_len == GFPS_NOTIFY_SASS_INITIAL_CONN_DATA_LEN)
                    {
                        uint8_t msg[1];
                        msg[0] = sass_initiated_connection;
                        if (app_gfps_account_key_verify_mac(bd_addr, msg, GFPS_NOTIFY_SASS_INITIAL_CONN_PARA_LEN,
                                                            message_nonce, mac))
                        {
                            uint8_t ack_len = GFPS_STANARD_ACK_PAYLOAD_LEN + GFPS_NOTIFY_SASS_INITIAL_CONN_PARA_LEN;
                            uint8_t ack_pkt[ack_len];
                            uint8_t *temp = ack_pkt;
                            BE_UINT8_TO_STREAM(temp, msg_group);
                            BE_UINT8_TO_STREAM(temp, msg_code);
                            BE_UINT8_TO_STREAM(temp, sass_initiated_connection);
                            bt_gfps_send_ack(bd_addr, server_chann, ack_pkt, ack_len);
                            //app_sass_policy_switch_active_link(bd_addr, 0x80, true);

                            app_sass_policy_initial_conn_handle(bd_addr, sass_initiated_connection);
                        }
                        else
                        {
                            app_gfps_rfc_report_mac_error(bd_addr, server_chann, msg_group, msg_code);
                        }

                    }
                }
                break;

            case GFPS_IND_INUSE_ACCOUNTKEY:
                {
                    uint8_t in_use_string[6] = {0};
                    STREAM_TO_ARRAY(in_use_string, p, 6);
                    STREAM_TO_ARRAY(message_nonce, p, 8);
                    STREAM_TO_ARRAY(mac, p, 8);
                    if (additional_data_len == GFPS_IND_INUSE_ACCOUNTKEY_DATA_LEN)
                    {
                        {
                            uint8_t pair_idx_mapping = 0;
                            uint8_t ack_len = GFPS_STANARD_ACK_PAYLOAD_LEN + GFPS_IND_INUSE_ACCOUNTKEY_PARA_LEN;
                            uint8_t ack_pkt[ack_len];
                            uint8_t *temp = ack_pkt;
                            uint8_t inuse_account_key_idx = 0;
                            BE_UINT8_TO_STREAM(temp, msg_group);
                            BE_UINT8_TO_STREAM(temp, msg_code);
                            ARRAY_TO_STREAM(temp, in_use_string, 6);
                            bt_gfps_send_ack(bd_addr, server_chann, ack_pkt, ack_len);
                            //update connection status with inuse account key
                            if (app_gfps_account_key_find_inuse_account_key(&inuse_account_key_idx, bd_addr, in_use_string,
                                                                            GFPS_IND_INUSE_ACCOUNTKEY_PARA_LEN, message_nonce, mac))
                            {
                                app_gfps_rfc_update_inuse_account_key_index(bd_addr, inuse_account_key_idx);
                            }
                            app_gfps_notify_conn_status();
                        }
                    }
                }
                break;
            case GFPS_SEND_CUSTOM_DATA:
                {
                    uint8_t custom_data = 0;
                    BE_STREAM_TO_UINT8(custom_data, p);
                    STREAM_TO_ARRAY(message_nonce, p, 8);
                    STREAM_TO_ARRAY(mac, p, 8);
                    if (additional_data_len == GFPS_SEND_CUSTOM_DATA_DATA_LEN)
                    {
                        uint8_t msg[1];
                        msg[0] = custom_data;
                        if (app_gfps_account_key_verify_mac(bd_addr, msg, GFPS_SEND_CUSTOM_DATA_PARA_LEN, message_nonce,
                                                            mac))
                        {
                            uint8_t ack_len = GFPS_STANARD_ACK_PAYLOAD_LEN + GFPS_SEND_CUSTOM_DATA_PARA_LEN;
                            uint8_t ack_pkt[ack_len];
                            uint8_t *temp = ack_pkt;
                            BE_UINT8_TO_STREAM(temp, msg_group);
                            BE_UINT8_TO_STREAM(temp, msg_code);
                            BE_UINT8_TO_STREAM(temp, custom_data);
                            bt_gfps_send_ack(bd_addr, server_chann, ack_pkt, ack_len);
                            app_gfps_rfc_update_custom_data(custom_data);
                            app_gfps_notify_conn_status();
                        }
                        else
                        {
                            app_gfps_rfc_report_mac_error(bd_addr, server_chann, msg_group, msg_code);
                        }
                    }
                }
                break;
            case GFPS_SET_DROP_CONN_TARGET:
                {
                    uint8_t is_this_device = 0;
                    BE_STREAM_TO_UINT8(is_this_device, p);
                    STREAM_TO_ARRAY(message_nonce, p, 8);
                    STREAM_TO_ARRAY(mac, p, 8);
                    if (additional_data_len == GFPS_SET_DROP_CONN_TARGET_DATA_LEN)
                    {
                        uint8_t msg[1];
                        msg[0] = is_this_device;
                        if (app_gfps_account_key_verify_mac(bd_addr, msg, GFPS_SET_DROP_CONN_TARGET_PARA_LEN, message_nonce,
                                                            mac))
                        {
                            uint8_t ack_len = GFPS_STANARD_ACK_PAYLOAD_LEN + GFPS_SET_DROP_CONN_TARGET_PARA_LEN;
                            uint8_t ack_pkt[ack_len];
                            uint8_t *temp = ack_pkt;
                            BE_UINT8_TO_STREAM(temp, msg_group);
                            BE_UINT8_TO_STREAM(temp, msg_code);
                            BE_UINT8_TO_STREAM(temp, is_this_device);
                            bt_gfps_send_ack(bd_addr, server_chann, ack_pkt, ack_len);
                            if (is_this_device)
                            {
                                app_gfps_rfc_update_target_drop_device(bd_addr);
                            }
                        }
                        else
                        {
                            app_gfps_rfc_report_mac_error(bd_addr, server_chann, msg_group, msg_code);
                        }
                    }

                }
                break;
            default:
                {
                    unknow_cmd = true;
                }
                break;
            }
        }
        else if ((msg_group == GFPS_HEARABLE_CONTROLEVENT) &&
                 app_sass_policy_support_easy_connection_switch())
        {
            uint16_t additional_data_len = 0;
            uint8_t message_nonce[8];
            uint8_t mac[8];
            BE_STREAM_TO_UINT16(additional_data_len, p);
            len -= (GFPS_HEADER_LEN + additional_data_len);
            switch (msg_code)
            {
            case GFPS_GET_ANC_STATE:
                {
                    if (additional_data_len == GFPS_GET_ANC_STATE_DATA_LEN)
                    {
                        app_gfps_report_anc_state(bd_addr, server_chann);
                    }
                }
                break;
            case GFPS_SET_ANC_STATE:
                {
                    uint8_t version = 0;
                    uint16_t anc_state = 0;
                    BE_STREAM_TO_UINT8(version, p);
                    BE_STREAM_TO_UINT16(anc_state, p);
                    STREAM_TO_ARRAY(message_nonce, p, 8);
                    STREAM_TO_ARRAY(mac, p, 8);

                    if (additional_data_len == GFPS_SET_ANC_STATE_DATA_LEN)
                    {
                        uint8_t msg[3];
                        msg[0] = version;
                        msg[1] = anc_state >> 8;
                        msg[2] = (uint8_t) anc_state;
                        if (app_gfps_account_key_verify_mac(bd_addr, msg, GFPS_SET_ANC_STATE_PARA_LEN, message_nonce,
                                                            mac))
                        {
                            uint8_t ack_len = GFPS_STANARD_ACK_PAYLOAD_LEN + GFPS_SET_ANC_STATE_PARA_LEN;
                            uint8_t ack_pkt[ack_len];
                            uint8_t *temp = ack_pkt;
                            BE_UINT8_TO_STREAM(temp, msg_group);
                            BE_UINT8_TO_STREAM(temp, msg_code);
                            BE_UINT8_TO_STREAM(temp, version);
                            BE_UINT16_TO_STREAM(temp, anc_state);
                            bt_gfps_send_ack(bd_addr, server_chann, ack_pkt, ack_len);
                            app_gfps_set_anc_state(version, anc_state);

                        }
                        else
                        {
                            app_gfps_rfc_report_mac_error(bd_addr, server_chann, msg_group, msg_code);
                        }
                    }
                }
                break;
            }
        }
#endif
        else
        {
            unknow_cmd = true;
        }

        if (unknow_cmd == true)
        {
            APP_PRINT_WARN0("app_gfps_rfc_dm_cback: unknow cmd");
            break;
        }
    }
}

static void app_gfps_rfc_loop_check_data_complete(uint8_t *bd_addr, uint8_t *data, uint16_t len,
                                                  uint8_t local_server_chann)
{
    if (data == NULL)
    {
        return;
    }

    bool ret = false, cont = false;
    T_APP_BR_LINK *p_link = NULL;

    p_link = app_find_br_link(bd_addr);

    if (p_link != NULL)
    {
        uint8_t *p_buf;
        if (p_link->p_gfps_cmd == NULL)
        {
            p_buf = malloc(len);
            if (p_buf != NULL)
            {
                p_link->p_gfps_cmd = p_buf;
                memcpy(p_link->p_gfps_cmd, data, len);
                p_link->gfps_cmd_len = len;

                if (len >= GFPS_HEADER_LEN)
                {
                    uint8_t *p_temp;
                    uint16_t temp_len = 0;

                    p_temp = p_link->p_gfps_cmd;
                    p_temp += (GFPS_HEADER_LEN - GFPS_ADDITIONAL_DATA_LEN);
                    BE_STREAM_TO_UINT16(temp_len, p_temp);
                    uint8_t chunck_len = temp_len + GFPS_HEADER_LEN;

                    if (chunck_len == len)
                    {
                        ret = true; //one packet one cmd
                    }
                    else if (chunck_len < len)
                    {
                        //one packet several cmds
                        ret = true;
                        cont = true;
                        p_link->gfps_cmd_len = chunck_len;
                        data += chunck_len;
                        len -= chunck_len;
                    }
                    else
                    {
                        //not whole cmd, do nothing
                    }
                }
            }
        }
        else
        {
            p_buf = malloc(len + p_link->gfps_cmd_len);
            if (p_buf != NULL)
            {
                uint8_t pre_len = p_link->gfps_cmd_len;
                memcpy(p_buf, p_link->p_gfps_cmd, p_link->gfps_cmd_len);
                memcpy(p_buf + p_link->gfps_cmd_len, data, len);
                os_mem_free(p_link->p_gfps_cmd);
                p_link->p_gfps_cmd = p_buf;
                p_link->gfps_cmd_len += len;

                if (p_link->gfps_cmd_len >= GFPS_HEADER_LEN)
                {
                    uint8_t *p_temp;
                    uint16_t temp_len = 0;

                    p_temp = p_buf;
                    p_temp += (GFPS_HEADER_LEN - GFPS_ADDITIONAL_DATA_LEN);
                    BE_STREAM_TO_UINT16(temp_len, p_temp);

                    uint8_t chunck_len = temp_len + GFPS_HEADER_LEN;

                    if (chunck_len == p_link->gfps_cmd_len)
                    {
                        ret = true;
                    }
                    else if (chunck_len < p_link->gfps_cmd_len)
                    {
                        ret = true;
                        cont = true;
                        p_link->gfps_cmd_len = chunck_len;
                        data += (chunck_len - pre_len);
                        len -= (chunck_len - pre_len);
                    }
                }
            }
            else
            {
                os_mem_free(p_link->p_gfps_cmd);
                p_link->gfps_cmd_len = 0;
                p_link->p_gfps_cmd = NULL;
            }
        }
        APP_PRINT_TRACE4("app_gfps_rfc_loop_check_data_complete: len %d, cmd_len %d, ret %d, cont %d\n",
                         len,
                         p_link->gfps_cmd_len, ret, cont);
        if (ret)
        {
            app_gfps_rfc_handle_data_transfer(bd_addr, local_server_chann,
                                              p_link->p_gfps_cmd, p_link->gfps_cmd_len);

            os_mem_free(p_link->p_gfps_cmd);
            p_link->gfps_cmd_len = 0;
            p_link->p_gfps_cmd = NULL;
        }

        if (cont)
        {
            app_gfps_rfc_loop_check_data_complete(bd_addr, data, len, local_server_chann);
        }
    }

    return;
}

static bool app_gfps_rfc_check_data_complete(uint8_t *bd_addr, uint8_t *data, uint16_t len)
{
    bool ret = false;
    T_APP_BR_LINK *p_link = NULL;

    p_link = app_find_br_link(bd_addr);

    if (p_link != NULL)
    {
        uint8_t *p_buf;
        if (p_link->p_gfps_cmd == NULL)
        {
            p_buf = malloc(len);
            if (p_buf != NULL)
            {
                p_link->p_gfps_cmd = p_buf;
                memcpy(p_link->p_gfps_cmd, data, len);
                p_link->gfps_cmd_len = len;

                if (len >= GFPS_HEADER_LEN)
                {
                    uint8_t *p_temp;
                    uint16_t temp_len = 0;

                    p_temp = p_link->p_gfps_cmd;
                    p_temp += (GFPS_HEADER_LEN - GFPS_ADDITIONAL_DATA_LEN);
                    BE_STREAM_TO_UINT16(temp_len, p_temp);

                    if (temp_len + GFPS_HEADER_LEN == len)
                    {
                        ret = true;
                    }
                }
            }
        }
        else
        {
            p_buf = malloc(len + p_link->gfps_cmd_len);
            if (p_buf != NULL)
            {
                memcpy(p_buf, p_link->p_gfps_cmd, p_link->gfps_cmd_len);
                memcpy(p_buf + p_link->gfps_cmd_len, data, len);
                os_mem_free(p_link->p_gfps_cmd);
                p_link->p_gfps_cmd = p_buf;
                p_link->gfps_cmd_len += len;

                if (p_link->gfps_cmd_len >= GFPS_HEADER_LEN)
                {
                    uint8_t *p_temp;
                    uint16_t temp_len = 0;

                    p_temp = p_buf;
                    p_temp += (GFPS_HEADER_LEN - GFPS_ADDITIONAL_DATA_LEN);
                    BE_STREAM_TO_UINT16(temp_len, p_temp);

                    if (temp_len + GFPS_HEADER_LEN == p_link->gfps_cmd_len)
                    {
                        ret = true;
                    }
                    else if (temp_len + GFPS_HEADER_LEN < p_link->gfps_cmd_len)
                    {
                        os_mem_free(p_link->p_gfps_cmd);
                        p_link->gfps_cmd_len = 0;
                        p_link->p_gfps_cmd = NULL;
                    }
                }
            }
            else
            {
                os_mem_free(p_link->p_gfps_cmd);
                p_link->gfps_cmd_len = 0;
                p_link->p_gfps_cmd = NULL;
            }
        }
        APP_PRINT_TRACE3("app_gfps_rfc_check_data_complete: len %d, cmd_len %d, ret %d", len,
                         p_link->gfps_cmd_len, ret);
    }
    return ret;
}

void app_gfps_rfc_battery_info_set(uint8_t *battery)
{
    bool local_charger_state = (app_db.local_charger_state == APP_CHARGER_STATE_CHARGING) ? true :
                               false;
    bool remote_charger_state = (app_db.remote_charger_state == APP_CHARGER_STATE_CHARGING) ? true :
                                false;

    if (app_cfg_const.bud_side == DEVICE_BUD_SIDE_LEFT)
    {
        if (extend_app_cfg_const.gfps_left_ear_batetry_support)
        {
            battery[0] = (local_charger_state << 7) | (app_db.local_batt_level & 0x7F);
        }
        else
        {
            battery[0] = 0x7F;
        }

        if (extend_app_cfg_const.gfps_right_ear_batetry_support)
        {
            battery[1] = (remote_charger_state << 7) | (app_db.remote_batt_level & 0x7F);
        }
        else
        {
            battery[1] = 0x7F;
        }
    }
    else
    {
        if (extend_app_cfg_const.gfps_left_ear_batetry_support)
        {
            battery[0] = (remote_charger_state << 7) | (app_db.remote_batt_level & 0x7F);
        }
        else
        {
            battery[0] = 0x7F;
        }

        if (extend_app_cfg_const.gfps_right_ear_batetry_support)
        {
            battery[1] = (local_charger_state << 7) | (app_db.local_batt_level & 0x7F);
        }
        else
        {
            battery[1] = 0x7F;
        }
    }

    if (extend_app_cfg_const.gfps_case_battery_support)
    {
        uint8_t battery_case_charging = ((app_db.case_battery >> 7) == 0) ? 1 : 0;
        uint8_t battery_case_level = app_db.case_battery & 0x7F;
        battery[2] = (battery_case_charging << 7) | (battery_case_level & 0x7F);
    }
    else
    {
        battery[2] = 0x7F;
    }
}

static void app_gfps_rfc_cback(uint8_t *bd_addr, T_BT_RFC_MSG_TYPE msg_type, void *msg_buf)
{
    T_APP_BR_LINK *p_link;
    bool handle = true;

    switch (msg_type)
    {
    case BT_RFC_MSG_CONN_IND:
        {
            T_BT_RFC_CONN_IND *p_ind = (T_BT_RFC_CONN_IND *)msg_buf;

            p_link = app_find_br_link(bd_addr);
            if (p_link != NULL)
            {
                bt_gfps_connect_cfm(p_link->bd_addr, p_ind->local_server_chann, true, p_ind->frame_size, 7);
            }
            else
            {
                APP_PRINT_WARN0("app_gfps_rfc_dm_cback: no acl link found");
                bt_gfps_connect_cfm(bd_addr, p_ind->local_server_chann, false, p_ind->frame_size, 7);
            }
        }
        break;

    case BT_RFC_MSG_CONN_CMPL:
        {
            T_BT_RFC_CONN_CMPL *p_cmpl = (T_BT_RFC_CONN_CMPL *)msg_buf;

            p_link = app_find_br_link(bd_addr);
            if (p_link != NULL)
            {
                uint8_t ble_addr[6];
                //uint8_t battery[3] = {app_db.local_batt_level, app_db.remote_batt_level, app_db.case_battery};

                p_link->gfps_rfc_chann = p_cmpl->local_server_chann;
                app_bt_policy_msg_prof_conn(bd_addr, GFPS_PROFILE_MASK);
                bt_gfps_send_model_id(bd_addr, p_cmpl->local_server_chann,
                                      extend_app_cfg_const.gfps_model_id,
                                      GFPS_MODEL_ID_LEN);

                app_gfps_get_random_addr(ble_addr);
                app_gfps_reverse_data(ble_addr, 6);
                bt_gfps_send_ble_addr(bd_addr, p_cmpl->local_server_chann, ble_addr);

                app_gfps_battery_info_report(GFPS_BATTERY_REPORT_RFCOMM);

                uint8_t gfps_version[] = GFPS_FIRMWARE_VERSION;
                bt_gfps_send_firmware_revision(bd_addr, p_cmpl->local_server_chann,
                                               gfps_version, sizeof(gfps_version));

#if GFPS_SASS_SUPPORT
                if (app_sass_policy_support_easy_connection_switch())
                {
                    uint8_t pair_idx_mapping;
                    app_gfps_rfc_report_session_nonce(bd_addr, p_link->gfps_rfc_chann);
                    app_gfps_rfc_notify_connection_status();
                }
#endif
#if GFPS_FINDER_SUPPORT
                if (extend_app_cfg_const.gfps_finder_support)
                {
                    app_gfps_finder_send_eddystone_capability(bd_addr, p_cmpl->local_server_chann);
                }
#endif
            }
        }
        break;

    case BT_RFC_MSG_DISCONN_CMPL:
        {
            T_BT_RFC_DISCONN_CMPL *p_info = (T_BT_RFC_DISCONN_CMPL *)msg_buf;

            p_link = app_find_br_link(bd_addr);

            if (p_link != NULL)
            {
                if (p_link->gfps_rfc_chann == p_info->local_server_chann)
                {
#if GFPS_FINDER_SUPPORT
#else
                    app_gfps_rfc_ring_stop();
                    gfps_rfc_ring_state = GFPS_RFC_RING_STOP;
#endif

                    app_bt_policy_msg_prof_disconn(bd_addr, GFPS_PROFILE_MASK, p_info->cause);
                    if (p_link->p_gfps_cmd != NULL)
                    {
                        os_mem_free(p_link->p_gfps_cmd);
                        p_link->p_gfps_cmd = NULL;
                    }
                }
            }
        }
        break;

    case BT_RFC_MSG_DATA_IND:
        {
            T_BT_RFC_DATA_IND *p_ind = (T_BT_RFC_DATA_IND *)msg_buf;

            p_link = app_find_br_link(bd_addr);
            if (p_link == NULL)
            {
                APP_PRINT_WARN0("app_gfps_rfc_dm_cback: no acl link found");
                return;
            }

            bt_rfc_credits_give(bd_addr, p_ind->local_server_chann, 1);

            app_gfps_rfc_loop_check_data_complete(bd_addr, p_ind->buf, p_ind->length,
                                                  p_ind->local_server_chann);
//            if (app_gfps_rfc_check_data_complete(bd_addr, p_ind->buf, p_ind->length) == false)
//            {
//                break;
//            }

//            app_gfps_rfc_handle_data_transfer(bd_addr, p_ind->local_server_chann,
//                                              p_link->p_gfps_cmd, p_link->gfps_cmd_len);

//            os_mem_free(p_link->p_gfps_cmd);
//            p_link->gfps_cmd_len = 0;
//            p_link->p_gfps_cmd = NULL;
        }
        break;

    case BT_RFC_MSG_CREDIT_INFO:
        break;

    default:
        {
            handle = false;
        }
        break;
    }

    if (handle == true)
    {
        APP_PRINT_INFO1("app_gfps_rfc_dm_cback: msg_type 0x%02x", msg_type);
    }

}

static void app_gfps_rfc_bt_cback(T_BT_EVENT event_type, void *event_buf, uint16_t buf_len)
{
    bool handle = true;
    T_BT_EVENT_PARAM *param = event_buf;

    switch (event_type)
    {
    case BT_EVENT_REMOTE_DISCONN_CMPL:
        {
            if (app_cfg_nv.bud_role == REMOTE_SESSION_ROLE_PRIMARY)
            {
                if (app_cfg_const.bud_side == BUD_SIDE_RIGHT)
                {
                    if (gfps_rfc_ring_state == GFPS_RFC_LEFT_RING)
                    {
                        gfps_rfc_ring_state = GFPS_RFC_RING_STOP;
                    }
                    else if (gfps_rfc_ring_state == GFPS_RFC_ALL_RING)
                    {
                        gfps_rfc_ring_state = GFPS_RFC_RIGHT_RING;
                    }
                }
                else if (app_cfg_const.bud_side == BUD_SIDE_LEFT)
                {
                    if (gfps_rfc_ring_state == GFPS_RFC_RIGHT_RING)
                    {
                        gfps_rfc_ring_state = GFPS_RFC_RING_STOP;
                    }
                    else if (gfps_rfc_ring_state == GFPS_RFC_ALL_RING)
                    {
                        gfps_rfc_ring_state = GFPS_RFC_LEFT_RING;
                    }
                }
            }
            else if (app_cfg_nv.bud_role == REMOTE_SESSION_ROLE_SECONDARY)
            {
                app_gfps_rfc_ring_stop();
                gfps_rfc_ring_state = GFPS_RFC_RING_STOP;
            }
            APP_PRINT_INFO3("app_gfps_rfc_bt_cback: gfps_rfc_ring_state 0x%02x, bud_role %d, bud_side %d",
                            gfps_rfc_ring_state, app_cfg_nv.bud_role, app_cfg_const.bud_side);
        }
        break;
#if GFPS_SASS_SUPPORT
    case BT_EVENT_ACL_CONN_DISCONN:
        {
            if (!memcmp(param->acl_conn_disconn.bd_addr, app_db.sass_target_drop_device, 6))
            {
                uint8_t reset_value[6] = {0};
                app_gfps_rfc_update_target_drop_device(reset_value);
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
        APP_PRINT_INFO1("app_gfps_rfc_bt_cback: event_type 0x%04x", event_type);
    }
}

static void app_gfps_rfc_audio_cback(T_AUDIO_EVENT event_type, void *event_buf, uint16_t buf_len)
{
    T_AUDIO_EVENT_PARAM *param = event_buf;
    bool handle = true;

#if GFPS_FINDER_SUPPORT
    static uint8_t default_volume = 0;
    T_GFPS_FINDER_RING_VOLUME_LEVEL ring_volume_level = GFPS_FINDER_RING_VOLUME_DEFAULT;

    if (extend_app_cfg_const.gfps_finder_support)
    {
        ring_volume_level = app_gfps_finder_get_ring_volume_level();
    }
#endif

    switch (event_type)
    {
    case AUDIO_EVENT_RINGTONE_STARTED:
    case AUDIO_EVENT_VOICE_PROMPT_STARTED:
        {
            uint8_t tone_started_idx;

            if (event_type == AUDIO_EVENT_VOICE_PROMPT_STARTED)
            {
#if F_APP_HARMAN_FEATURE_SUPPORT
                if (param->voice_prompt_started.index == GFPS_VOICE_PROMPT_OFFSET)
                {
                    tone_started_idx = GFPS_VOICE_PROMPT_OFFSET;
                }
                else
#endif
                {
                    tone_started_idx = param->voice_prompt_started.index + GFPS_VOICE_PROMPT_OFFSET;
                }
            }
            else
            {
                tone_started_idx = param->ringtone_started.index;
            }
            APP_PRINT_TRACE3("[sd_check]app_gfps_rfc_audio_cback STARTED %d,%d,%d", event_type,
                             param->voice_prompt_started.index, tone_started_idx);
#if F_APP_HARMAN_FEATURE_SUPPORT
            if (tone_started_idx == app_cfg_const.tone_gfps_findme ||
                tone_started_idx == app_cfg_const.tone_anc_scenario_4 ||
                tone_started_idx == GFPS_VOICE_PROMPT_OFFSET)
#else
            if (tone_started_idx == app_cfg_const.tone_gfps_findme)
#endif
            {
#if GFPS_FINDER_SUPPORT
                if (extend_app_cfg_const.gfps_finder_support)
                {
                    default_volume = voice_prompt_volume_get();

#if HARMAN_OPEN_LR_FEATURE
                    harman_set_vp_ringtone_balance(__func__, __LINE__);
#endif
                    //danni todo set ring volume level
                    if (ring_volume_level == GFPS_FINDER_RING_VOLUME_LOW)
                    {
                        voice_prompt_volume_set(app_cfg_const.voice_prompt_volume_min + 5);
                    }
                    else if (ring_volume_level == GFPS_FINDER_RING_VOLUME_HIGH)
                    {
                        voice_prompt_volume_set(app_cfg_const.voice_prompt_volume_max);
                    }
                    else if (ring_volume_level == GFPS_FINDER_RING_VOLUME_MEDIUM)
                    {
                        voice_prompt_volume_set(app_cfg_const.voice_prompt_volume_default);
                    }
                }
#endif
            }
        }
        break;

    case AUDIO_EVENT_RINGTONE_STOPPED:
    case AUDIO_EVENT_VOICE_PROMPT_STOPPED:
        {
            uint8_t tone_stopped_idx;

            if (event_type == AUDIO_EVENT_VOICE_PROMPT_STOPPED)
            {
#if F_APP_HARMAN_FEATURE_SUPPORT
                if (param->voice_prompt_stopped.index == GFPS_VOICE_PROMPT_OFFSET)
                {
                    tone_stopped_idx = GFPS_VOICE_PROMPT_OFFSET;
                }
                else
#endif
                {
                    tone_stopped_idx = param->voice_prompt_stopped.index + GFPS_VOICE_PROMPT_OFFSET;
                }
            }
            else
            {
                tone_stopped_idx = param->ringtone_stopped.index;
            }
            APP_PRINT_TRACE3("[sd_check]app_gfps_rfc_audio_cback STOPPED %d,%d,%d", event_type,
                             param->voice_prompt_stopped.index, tone_stopped_idx);
#if F_APP_HARMAN_FEATURE_SUPPORT
            if (tone_stopped_idx == app_cfg_const.tone_gfps_findme ||
                tone_stopped_idx == app_cfg_const.tone_anc_scenario_4 ||
                tone_stopped_idx == GFPS_VOICE_PROMPT_OFFSET)
#else
            if (tone_stopped_idx == app_cfg_const.tone_gfps_findme)
#endif
            {
#if GFPS_FINDER_SUPPORT
                if (extend_app_cfg_const.gfps_finder_support)
                {
                    if (ring_volume_level != GFPS_FINDER_RING_VOLUME_DEFAULT)
                    {
#if HARMAN_OPEN_LR_FEATURE
                        harman_set_vp_ringtone_balance(__func__, __LINE__);
#endif
                        voice_prompt_volume_set(default_volume);
                    }
                }
#endif
                bool period_fg = false;

                if (app_cfg_const.bud_role == REMOTE_SESSION_ROLE_SINGLE)
                {
                    if (gfps_rfc_ring_state != GFPS_RFC_RING_STOP)
                    {
                        period_fg = true;
                    }
                }
                else
                {
                    if ((gfps_rfc_ring_state == GFPS_RFC_RIGHT_RING) &&
                        (app_cfg_const.bud_side == BUD_SIDE_RIGHT))
                    {
                        period_fg = true;
                    }
                    else if ((gfps_rfc_ring_state == GFPS_RFC_LEFT_RING) &&
                             (app_cfg_const.bud_side == BUD_SIDE_LEFT))
                    {
                        period_fg = true;
                    }
                    else if (gfps_rfc_ring_state == GFPS_RFC_ALL_RING)
                    {
                        period_fg = true;
                    }
                }

                if (period_fg == true)
                {
                    gap_start_timer(&timer_handle_ring_period, "gfps_ring_period",
                                    gfps_rfc_timer_queue_id, GFPS_RFC_RING_PERIOD,
                                    0, GFPS_RING_PERIOD_VALUE * 1000);
                }
            }
        }
        break;
#if GFPS_SASS_SUPPORT
    case AUDIO_EVENT_ANC_ENABLED:
    case AUDIO_EVENT_ANC_DISABLED:
    case AUDIO_EVENT_PASSTHROUGH_ENABLED:
    case AUDIO_EVENT_PASSTHROUGH_DISABLED:
        {
            if (app_sass_policy_support_easy_connection_switch())
            {
                app_gfps_notify_anc_state();
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
        APP_PRINT_INFO1("app_gfps_rfc_audio_cback: event_type 0x%04x", event_type);
    }
}

static void app_gfps_rfc_timeout_cb(uint8_t timer_id, uint16_t timer_chann)
{
    APP_PRINT_TRACE4("app_gfps_rfc_timeout_cb: timer_id %d, timer_chann %d, state 0x%02x, bud_side %d",
                     timer_id, timer_chann, gfps_rfc_ring_state, app_cfg_const.bud_side);
    switch (timer_id)
    {
    case GFPS_RFC_RING_PERIOD:
        {
            if (timer_handle_ring_period != NULL)
            {
                gap_stop_timer(&timer_handle_ring_period);

#if F_APP_HARMAN_FEATURE_SUPPORT
                if (app_cfg_nv.language_status == 0)
                {
                    app_audio_tone_type_play(TONE_ANC_SCENARIO_4, false, false);
                }
                else
#endif
                {
                    app_audio_tone_type_play(TONE_GFPS_FINDME, false, false);
                }
            }
        }
        break;

    case GFPS_RFC_RING_TIMEOUT:
        {
            if (timer_handle_ring_timeout != NULL)
            {
                app_gfps_rfc_handle_ring_event(GFPS_ALL_STOP);
#if GFPS_FINDER_SUPPORT
                if (extend_app_cfg_const.gfps_finder_support)
                {
                    uint8_t ring_state = GFPS_FINDER_RING_TIMEOUT_STOP;
                    app_gfps_finder_send_ring_rsp(ring_state);
                }
#endif
            }
        }
        break;

    default:
        break;
    }
}
#if F_APP_ERWS_SUPPORT

uint16_t app_gfps_rfc_relay_cback(uint8_t *buf, uint8_t msg_type, bool total)
{
    bool skip = true;
    APP_PRINT_INFO1("app_gfps_rfc_relay_cback: msg_type %d", msg_type);
    uint8_t *msg_ptr = NULL;
    uint16_t payload_len = 0;
    uint16_t msg_len = 0;
    uint8_t tmp[20] = {0};
    T_SASS_CONN_STATUS_FIELD conn_status;
    switch (msg_type)
    {
#if GFPS_SASS_SUPPORT
    case GFPS_RFC_REMOTE_MSG_SYNC_CUSTOM_DATA:
        {
            gfps_sass_get_conn_status(&conn_status);
            skip = false;
            payload_len = 1 ;
            msg_ptr = &conn_status.custom_data;
        }
        break;
    case GFPS_RFC_REMOTE_MSG_SYNC_TARGET_DROP_DEVICE:
        {
            skip = false;
            payload_len = 6;
            //TODO: Get target drop device
            memcpy(tmp, app_db.sass_target_drop_device, 6);
            msg_ptr = tmp;
        }
        break;
#endif
    }

    return app_relay_msg_pack(buf, msg_type, APP_MODULE_TYPE_GFPS_RFC, payload_len, msg_ptr, skip,
                              total);
}

static void app_gfps_rfc_parse_cback(uint8_t msg_type, uint8_t *buf, uint16_t len,
                                     T_REMOTE_RELAY_STATUS status)
{
    APP_PRINT_TRACE3("app_gfps_rfc_parse_cback: msg_type 0x%04x, status %d, ring_state 0x%02x",
                     msg_type, status, gfps_rfc_ring_state);

    switch (msg_type)
    {
    case GFPS_RFC_REMOTE_MSG_FINDME_START:
        if (status == REMOTE_RELAY_STATUS_ASYNC_RCVD)
        {
            app_gfps_rfc_ring_play();
        }
        else if (status == REMOTE_RELAY_STATUS_SYNC_TOUT)
        {
            app_gfps_rfc_ring_play();
        }
        break;

    case GFPS_RFC_REMOTE_MSG_FINDME_STOP:
        if (status == REMOTE_RELAY_STATUS_ASYNC_RCVD)
        {
            app_gfps_rfc_ring_stop();
        }
        else if (status == REMOTE_RELAY_STATUS_SYNC_TOUT)
        {
            app_gfps_rfc_ring_stop();
        }
        break;

    case GFPS_RFC_REMOTE_MSG_FINDME_SYNC:
        {
            uint8_t *data = (uint8_t *)buf;
            gfps_rfc_ring_state = (T_GFPS_RFC_RING_STATE)data[0];
            gfps_rfc_ring_timeout = data[1] << 8 | data[2];
            APP_PRINT_TRACE2("app_gfps_rfc_parse_cback: change to state 0x%02x, ring_timeout 0x%02x",
                             gfps_rfc_ring_state, gfps_rfc_ring_timeout);
        }
        break;
#if GFPS_SASS_SUPPORT
    case GFPS_RFC_REMOTE_MSG_SYNC_CUSTOM_DATA:
        {
            if ((status == REMOTE_RELAY_STATUS_ASYNC_RCVD) &&
                app_sass_policy_support_easy_connection_switch())
            {
                app_gfps_rfc_update_custom_data(*buf);
            }
        }
        break;
    case GFPS_RFC_REMOTE_MSG_SYNC_TARGET_DROP_DEVICE:
        {
            if ((status == REMOTE_RELAY_STATUS_ASYNC_RCVD) &&
                app_sass_policy_support_easy_connection_switch())
            {
                app_gfps_rfc_update_target_drop_device(buf);
            }
        }
        break;
#endif
    default:
        break;
    }
}
#endif
void app_gfps_rfc_update_ble_addr(void)
{
    uint8_t i;
    uint8_t ble_addr[6] = {0};
    app_gfps_get_random_addr(ble_addr);
    app_gfps_reverse_data(ble_addr, 6);

    for (i = 0; i < MAX_BR_LINK_NUM; i++)
    {
        if (app_db.br_link[i].used == true &&
            (app_db.br_link[i].connected_profile & GFPS_PROFILE_MASK))
        {
            T_APP_BR_LINK *p_link = &app_db.br_link[i];

            if (p_link != NULL)
            {
                bt_gfps_send_ble_addr(p_link->bd_addr, p_link->gfps_rfc_chann, ble_addr);
            }

            APP_PRINT_INFO1("app_gfps_rfc_update_ble_addr: link_id %d", i);
        }
    }
}
void app_gfps_rfc_init(void)
{
    if (app_cfg_const.supported_profile_mask & GFPS_PROFILE_MASK)
    {
        bt_gfps_rfc_init(app_gfps_rfc_cback, RFC_GFPS_CHANN_NUM);
        bt_mgr_cback_register(app_gfps_rfc_bt_cback);
        audio_mgr_cback_register(app_gfps_rfc_audio_cback);
        gap_reg_timer_cb(app_gfps_rfc_timeout_cb, &gfps_rfc_timer_queue_id);
#if F_APP_ERWS_SUPPORT
        app_relay_cback_register(app_gfps_rfc_relay_cback, app_gfps_rfc_parse_cback,
                                 APP_MODULE_TYPE_GFPS_RFC, GFPS_RFC_REMOTE_MSG_MAX_MSG_NUM);
#endif
    }
}
#endif
