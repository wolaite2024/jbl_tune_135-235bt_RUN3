/*
 * Copyright (c) 2018, Realsil Semiconductor Corporation. All rights reserved.
 */

#include <string.h>
#include <stdbool.h>
#include "trace.h"
#include "gap_timer.h"
#include "bt_types.h"
#include "bt_bond.h"
#include "bt_a2dp.h"
#include "bt_rdtp.h"
#include "bt_hfp.h"
#include "btm.h"
#include "sysm.h"
#include "remote.h"
#include "app_cfg.h"
#include "app_main.h"
#include "app_mmi.h"
#include "app_multilink.h"
#include "app_roleswap.h"
#include "app_roleswap_control.h"
#include "app_bt_sniffing.h"
#include "app_ble_device.h"
#include "app_bt_policy_int.h"
#include "app_bt_policy_api.h"
#include "app_audio_policy.h"
#include "app_hfp.h"
#include "app_bond.h"
#if F_APP_QOL_MONITOR_SUPPORT
#include "app_qol.h"
#include "audio_track.h"
#endif
#include "ble_ext_adv.h"
#if (F_APP_DONGLE_FEATURE_SUPPORT == 1)
#include "app_dongle_spp.h"
#endif

#if GFPS_SASS_SUPPORT
#include "app_sass_policy.h"
#endif

#if F_APP_ERWS_SUPPORT

#define APP_SNIFFING_ERWS2_0

#define SNIFFING_RECONNECT_TIMER_MS     800
#define SNIFFING_RETRY_TIMER_MS         300
#define SNIFFING_PROECTED_TIMER_MS      4000
#define ROLESWITCH_MAX_RETRY_TIMES      10

static void *timer_handle_reconnect_inactive_link = NULL;
static void *timer_handle_retry_low_latency_mode = NULL;
static void *timer_handle_retry_sniffing = NULL;
static void *timer_handle_protected_timer = NULL;
static uint8_t bt_sniffing_timer_queue_id = 0;
static uint16_t recovery_link_a2dp_interval;
static uint16_t recovery_link_a2dp_flush_timeout;
static uint8_t recovery_link_a2dp_rsvd_slot;
static uint16_t recovery_link_a2dp_idle_slot;
static uint16_t recovery_link_a2dp_idle_skip;
static bool recovery_link_quick_flush = false;
static uint8_t recovery_link_nack_num = 0;
static bool setting_recovery_param = false;
static uint16_t pre_recovery_link_a2dp_interval;
static uint16_t pre_recovery_link_a2dp_flush_timeout;
static uint8_t pre_recovery_link_a2dp_rsvd_slot;
static uint16_t pre_recovery_link_a2dp_idle_slot;
static uint16_t pre_recovery_link_a2dp_idle_skip;
static bool app_bt_sniffing_param_diff_from_pre_value(uint16_t interval, uint16_t flush_tout,
                                                      uint8_t rsvd_slot,
                                                      uint8_t idle_slot, uint8_t idle_skip);
static void app_bt_sniffing_param_set_pre_value(bool set_value);
static uint8_t sniffing_protect_id = MAX_BR_LINK_NUM;
#if F_APP_MUTILINK_VA_PREEMPTIVE
static bool sniffing_starting_a2dp_stop = false;
static bool sniffing_starting_a2dp_mute = false;
#endif

typedef enum
{
    APP_BT_SNIFFING_TIMER_ID_ROLE_SWITCH            = 0x01,
    APP_BT_SNIFFING_TIMER_ID_RETRY_ROLESWAP         = 0x02,
    APP_BT_SNIFFING_TIMER_ID_SET_LOW_LAT_PARAM      = 0x03,
    APP_BT_SNIFFING_TIMER_ID_RETRY_SNIFFING         = 0x04,
    APP_BT_SNIFFING_TIMER_ID_PROTECTED_TIMER        = 0x05,
    APP_BT_SNIFFING_TIMER_ID_CHECK_ROLE_SWITCH      = 0x06,
    APP_BT_SNIFFING_TIMER_ID_RECONNECT_INACTIVE_LINK = 0x07,
} T_APP_BT_SNIFFING_TIMER;

const T_APP_BT_SNIFFING_HTPOLL htpoll[APP_BT_SNIFFING_SCENE_MAX] =
{
    {0, 0},       //0, no ble link and multilink is disabled, corresponding to APP_BT_SNIFFING_NO_BLE_MULTILINK_DISABLED
    {0x0C, 0},    //1, linkback is going on, corresponding to APP_BT_SNIFFING_LINKBACK
    {0x0A, 0},    //2, a2dp preemping is going on, corresponding to APP_BT_SNIFFING_PREEMP
    //////////////////////////////////////////////////////////////////////////////////////////
    {0x06, 0x02}, //3, ble adv is enabled in normal mode, corresponding to APP_BT_SNIFFING_NORMAL_BLE_ADV_ONGOING
    {0x0C, 0},    //4, ble link is active in normal mode, corresponding to APP_BT_SNIFFING_NORMAL_BLE_ACTIVE
    {0x06, 0x02}, //5, ble link is connected in normal mode, corresponding to APP_BT_SNIFFING_NORMAL_BLE_CONNECTED
    {0x06, 0x02}, //6, multilink is supported in normal mode, corresponding to APP_BT_SNIFFING_NORMAL_MULTILINK_ENABLED
    ////////////////////dividing line during normal mode and gaming mode//////////////////////
    {0x06, 0x03}, //7, ble adv is enabled in gaming mode, corresponding to APP_BT_SNIFFING_GAMING_BLE_ADV_ONGOING
    {0x06, 0x08}, //8, ble link is encrypted in gaming mode, corresponding to APP_BT_SNIFFING_GAMING_BLE_ENCRYPTED
    {0x06, 0x03}, //9, ble link is connected but not encrypted in gaming mode, corresponding to APP_BT_SNIFFING_GAMING_BLE_CONNECTED
    {0x06, 0x05}  //10, multilink is supported in gaming mode, corresponding to APP_BT_SNIFFING_GAMING_MULTILINK_ENABLED
};

const uint8_t sniffing_params_low_latency_table[3][LOW_LATENCY_LEVEL_MAX] =
{
    /* LOW_LATENCY_LEVEL1: 0;   LOW_LATENCY_LEVEL2: 1;  LOW_LATENCY_LEVEL3: 2.  LOW_LATENCY_LEVEL4: 3;  LOW_LATENCY_LEVEL5: 4. */
    {0x20,                      0x20,                   0x20,                   0x20,                   0x20},      /* A2DP_INTERVAL: 0 */
    {0x38,                      0x52,                   0x72,                   0x72,                   0x72},      /* A2DP_FLUSH_TIMEOUT: 1 */
    {0x0A,                      0x0A,                   0x0A,                   0x0A,                   0x0A},      /* A2DP_CTRL_RSVD_SLOT: 2 */
};

static bool app_bt_sniffing_state_machine(uint8_t *bd_addr,
                                          T_APP_BT_SNIFFING_STATE_EVENT sniffing_event);
static T_SNIFFING_CHECK_RESULT app_bt_sniffing_check(uint8_t *bd_addr);
static void app_bt_sniffing_change_state(uint8_t *bd_addr, T_APP_BT_SNIFFING_STATE new_state);
#if F_APP_QOL_MONITOR_SUPPORT
static void app_bt_sniffing_handle_src_away_multilink_enabled(void);
#endif

static void app_roleswap_start(void)
{
    APP_PRINT_ERROR0("app_roleswap_start");

    app_relay_total_async();
}

static bool app_bt_sniffing_nack_num(uint8_t *bd_addr, uint8_t *nack_num)
{
    T_APP_BR_LINK *p_link = NULL;

    *nack_num = 0;
    p_link = app_find_br_link(bd_addr);
    if (p_link != NULL)
    {
        if (p_link->a2dp_codec_type == BT_A2DP_CODEC_TYPE_SBC)
        {
            *nack_num = 0;
        }
        else if (p_link->a2dp_codec_type == BT_A2DP_CODEC_TYPE_AAC)
        {
#if (F_APP_AVP_INIT_SUPPORT == 1)
            if ((p_link->remote_device_vendor_id == APP_REMOTE_DEVICE_IOS) && app_cfg_nv.id_is_display)
            {
                *nack_num = 1;
            }
#endif
        }
        return true;
    }
    else
    {
        return false;
    }

}

void app_bt_sniffing_set_nack_flush_param(uint8_t *bd_addr)
{
    T_APP_BR_LINK *p_link = NULL;

    p_link = app_find_br_link(bd_addr);
    if (p_link == NULL)
    {
        return;
    }
    app_bt_sniffing_nack_num(bd_addr, &recovery_link_nack_num);
#if (F_APP_AVP_INIT_SUPPORT == 1)
    if ((p_link->remote_device_vendor_id == APP_REMOTE_DEVICE_IOS) && app_cfg_nv.id_is_display)
    {
        bt_sniffing_set_a2dp_dup_num(true, recovery_link_nack_num, recovery_link_quick_flush);
    }
    else
#endif
    {
        bt_sniffing_set_a2dp_dup_num(app_db.gaming_mode, recovery_link_nack_num, recovery_link_quick_flush);
    }
}

void app_bt_sniffing_hfp_connect(uint8_t *bd_addr)
{
    T_APP_BR_LINK *p_link = NULL;
    p_link = app_find_br_link(bd_addr);
    if (p_link != NULL)
    {
        APP_PRINT_TRACE3("app_bt_sniffing_hfp_connect: p_link %x, remote_hfp_brsf_capability %x, hfp_brsf_capability %x",
                         p_link, p_link->remote_hfp_brsf_capability, app_cfg_const.hfp_brsf_capability);

        if ((app_cfg_const.hfp_brsf_capability & BT_HFP_HF_LOCAL_CODEC_NEGOTIATION)
            && (p_link->remote_hfp_brsf_capability & BT_HFP_HF_REMOTE_CAPABILITY_CODEC_NEGOTIATION))
        {
            bt_hfp_audio_connect_req(bd_addr);
        }
        else
        {
            if (app_bt_sniffing_start(bd_addr, BT_SNIFFING_TYPE_SCO))
            {
                p_link->sco_initial = true;
            }
            else
            {
                bt_hfp_audio_connect_req(bd_addr);
                p_link->sco_initial = false;
            }
        }
        APP_PRINT_TRACE1("app_bt_sniffing_hfp_connect: sco_initial %d", p_link->sco_initial);
    }
}

bool app_bt_sniffing_roleswap_check(void)
{
    bool check_result = false;
    uint8_t app_idx;
    for (app_idx = 0; app_idx < MAX_BR_LINK_NUM; app_idx++)
    {
        if (app_check_b2s_link_by_id(app_idx))
        {
            // only sniffing state and ready state can do roleswap
            if (app_db.br_link[app_idx].bt_sniffing_state == APP_BT_SNIFFING_STATE_SNIFFING)
            {
                if (!setting_recovery_param)
                {
                    check_result = true;
                }
            }
            else if (app_db.br_link[app_idx].bt_sniffing_state == APP_BT_SNIFFING_STATE_READY)
            {
                if ((app_db.br_link[app_idx].streaming_fg == false) &&
                    (app_db.br_link[app_idx].sco_handle == 0))
                {
                    check_result = true;
                }
            }
            APP_PRINT_TRACE6("app_bt_sniffing_roleswap_check: app_idx %d, bt_sniffing_state %d, setting_recovery_param %d,"
                             "streaming_fg %d, sco_handle %d, check_result %d",
                             app_idx, app_db.br_link[app_idx].bt_sniffing_state, setting_recovery_param,
                             app_db.br_link[app_idx].streaming_fg, app_db.br_link[app_idx].sco_handle, check_result);
        }
    }

    return check_result;
}

bool app_bt_sniffing_roleswap(bool stop_after_shadow)
{
    uint8_t app_idx;
    bool result = false;

    //stop reconnect inactive b2s link when start roleswap
    if (timer_handle_reconnect_inactive_link != NULL)
    {
        gap_stop_timer(&timer_handle_reconnect_inactive_link);
    }

    if (app_cfg_const.local_playback_support && app_bt_policy_get_b2s_connected_num() == 0)
    {
        //local playback support & no b2s connected
        uint8_t *bd_addr = {0};

        result = remote_roleswap_start(bd_addr, 0, true, app_roleswap_start);
    }
    else
    {
        for (app_idx = 0; app_idx < MAX_BR_LINK_NUM; app_idx++)
        {
            if (app_check_b2s_link_by_id(app_idx))
            {
                app_db.br_link[app_idx].stop_after_shadow = stop_after_shadow;

                result = app_bt_sniffing_state_machine(app_db.br_link[app_idx].bd_addr,
                                                       APP_BT_SNIFFING_EVENT_ROLESWAP_START);

                APP_PRINT_TRACE5("app_bt_sniffing_roleswap: app_idx %u, profile mask 0x%02x,"
                                 "bd_addr %s, stop_after_shadow %d, result %d",
                                 app_idx, app_db.br_link[app_idx].connected_profile,
                                 TRACE_BDADDR(app_db.br_link[app_idx].bd_addr), stop_after_shadow, result);

            }
        }
    }

    return result;
}

void app_bt_sniffing_process(uint8_t *bd_addr)
{
    T_SNIFFING_CHECK_RESULT check_result = SNIFFING_ALLOW;
    T_APP_BR_LINK *p_link = NULL;

    APP_PRINT_TRACE1("app_bt_sniffing_process: bd_addr %s", TRACE_BDADDR(bd_addr));
    if (app_check_b2b_link(bd_addr))
    {
        // b2b link connected
        for (int i = 0; i < MAX_BR_LINK_NUM; i++)
        {
            if (app_db.br_link[i].used == true)
            {
                check_result = app_bt_sniffing_check(app_db.br_link[i].bd_addr);
                if (check_result == SNIFFING_ALLOW)
                {
                    if (app_db.br_link[i].bt_sniffing_state <= APP_BT_SNIFFING_STATE_READY)
                    {
                        app_bt_sniffing_change_state(app_db.br_link[i].bd_addr, APP_BT_SNIFFING_STATE_READY);
                    }

                    if (app_active_link_check(app_db.br_link[i].bd_addr))
                    {
                        p_link = &app_db.br_link[i];
                    }
                }

            }
        }
    }
    else
    {
        p_link = app_find_br_link(bd_addr);
    }

    if (p_link)
    {
        check_result = app_bt_sniffing_check(p_link->bd_addr);
        APP_PRINT_TRACE4("app_bt_sniffing_process: p_link %p, check_result %x, bt_sniffing_state %x, bd_addr %s",
                         p_link, check_result, p_link->bt_sniffing_state, TRACE_BDADDR(p_link->bd_addr));
        if (check_result == SNIFFING_ALLOW &&
            p_link->bt_sniffing_state <= APP_BT_SNIFFING_STATE_READY)
        {
            app_bt_sniffing_change_state(p_link->bd_addr, APP_BT_SNIFFING_STATE_READY);

            if (app_active_link_check(p_link->bd_addr))
            {
                // Sec Access to Pri
                if (p_link->streaming_fg)
                {
                    if (app_bt_sniffing_start(p_link->bd_addr, BT_SNIFFING_TYPE_A2DP))
                    {

                    }
                }
                else if (p_link->sco_handle != 0)
                {
                    if (app_bt_sniffing_start(p_link->bd_addr, BT_SNIFFING_TYPE_SCO))
                    {

                    }
                }
            }
        }
    }
}

bool app_bt_sniffing_start(uint8_t *bd_addr, T_BT_SNIFFING_TYPE type)
{
    bool result = false;
    T_APP_BR_LINK *p_link = app_find_br_link(bd_addr);

    if (app_cfg_nv.bud_role == REMOTE_SESSION_ROLE_SECONDARY || p_link == NULL)
    {
        APP_PRINT_ERROR2("app_bt_sniffing_start: p_link %p, bud role wrong %d", p_link,
                         app_cfg_nv.bud_role);
        return result;
    }

    T_SNIFFING_CHECK_RESULT check_result = SNIFFING_ALLOW;
    check_result = app_bt_sniffing_check(bd_addr);
    if (check_result == SNIFFING_ALLOW)
    {
        for (int i = 0; i < MAX_BR_LINK_NUM; i++)
        {
            if (app_check_b2s_link_by_id(i) &&
                (memcmp(app_db.br_link[i].bd_addr, bd_addr, 6) != 0) &&
                (app_db.br_link[i].acl_link_role != BT_LINK_ROLE_SLAVE))
            {
                APP_PRINT_TRACE1("app_bt_sniffing_start: need disconnect b2s before start sniffing, addr %s",
                                 TRACE_BDADDR(app_db.br_link[i].bd_addr));

                if (app_db.br_link[i].b2s_connected_vp_is_played)
                {
                    //if connected vp is played, it will not play again after reconnected
                    app_db.disallow_connected_tone_after_inactive_connected = true;
                }

                memcpy(app_db.resume_addr, app_db.br_link[i].bd_addr, 6);
                legacy_send_acl_disconn_req(app_db.resume_addr);

                //inactive b2s link will be reconnected after timeout
                gap_start_timer(&timer_handle_reconnect_inactive_link,
                                "reconnect_inactive_b2s_link",
                                bt_sniffing_timer_queue_id,
                                APP_BT_SNIFFING_TIMER_ID_RECONNECT_INACTIVE_LINK,
                                0,
                                SNIFFING_RECONNECT_TIMER_MS);

                break;
            }
        }
        // Check Sniffing state
        if (p_link->bt_sniffing_state == APP_BT_SNIFFING_STATE_READY ||
            p_link->bt_sniffing_state == APP_BT_SNIFFING_STATE_SNIFFING)
        {
            if (type == BT_SNIFFING_TYPE_A2DP)
            {
                result = app_bt_sniffing_state_machine(bd_addr, APP_BT_SNIFFING_EVENT_A2DP_START_IND);
            }
            else if (type == BT_SNIFFING_TYPE_SCO)
            {
                result = app_bt_sniffing_state_machine(bd_addr, APP_BT_SNIFFING_EVENT_SCO_CONN_IND);
            }

            if (result)
            {
                p_link->sniffing_type = type;
                if ((type == BT_SNIFFING_TYPE_A2DP && p_link->streaming_fg == false) ||
                    (type == BT_SNIFFING_TYPE_SCO && p_link->sco_handle == 0)
                   )
                {
                    // need to confirm after sniffing complete
                    p_link->pending_ind_confirm = true;
                }
            }
            else
            {
                if (p_link->bt_sniffing_state == APP_BT_SNIFFING_STATE_READY)
                {
                    p_link->sniffing_type = BT_SNIFFING_TYPE_NONE;
                }
            }
        }
    }

    APP_PRINT_TRACE6("app_bt_sniffing_start: p_link %p, bt_sniffing_state %d, type %x, check_result %x, bd_addr %s, result %d, pending_action %d",
                     p_link, p_link->bt_sniffing_state, type, check_result, TRACE_BDADDR(bd_addr), result);
    return result;
}

void app_bt_sniffing_stop(uint8_t *bd_addr, T_BT_SNIFFING_TYPE type)
{
    T_APP_BR_LINK *p_link = app_find_br_link(bd_addr);

    if (p_link)
    {
        APP_PRINT_TRACE3("app_bt_sniffing_stop: p_link %p, type %x, sniffing_type %x",
                         p_link, type, p_link->sniffing_type);

        if (type == BT_SNIFFING_TYPE_A2DP)
        {
            app_bt_sniffing_state_machine(bd_addr, APP_BT_SNIFFING_EVENT_A2DP_STOP);
        }
        else if (type == BT_SNIFFING_TYPE_SCO)
        {
            app_bt_sniffing_state_machine(bd_addr, APP_BT_SNIFFING_EVENT_SCO_DISCONN_CMPL);
        }
    }

    return;
}
#if F_APP_MUTILINK_VA_PREEMPTIVE
void app_bt_a2dp_sniffing_mute(uint8_t *bd_addr, T_BT_SNIFFING_TYPE type)
{
    T_APP_BR_LINK *p_link = app_find_br_link(bd_addr);

    if (app_cfg_const.enable_multi_link && p_link)
    {
        APP_PRINT_TRACE3("app_bt_a2dp_sniffing_mute: p_link %p, type %x, sniffing_type %x",
                         p_link, type, p_link->sniffing_type);

        if (type == BT_SNIFFING_TYPE_A2DP)
        {
            app_bt_sniffing_state_machine(bd_addr, APP_BT_SNIFFING_EVENT_A2DP_MUTE);
        }
    }

    return;
}

void app_bt_a2dp_sniffing_unmute(uint8_t *bd_addr, T_BT_SNIFFING_TYPE type)
{
    T_APP_BR_LINK *p_link = app_find_br_link(bd_addr);

    if (app_cfg_const.enable_multi_link && p_link)
    {
        if (type == BT_SNIFFING_TYPE_A2DP)
        {
            app_bt_sniffing_state_machine(bd_addr, APP_BT_SNIFFING_EVENT_A2DP_UNMUTE);
        }
    }

    return;
}
#endif
/////////////////////////////////////////////////////////////////////////////////
static void app_bt_sniffing_change_state(uint8_t *bd_addr, T_APP_BT_SNIFFING_STATE new_state)
{
    T_APP_BR_LINK *p_link = app_find_br_link(bd_addr);
    if (p_link)
    {
        APP_PRINT_INFO3("app_bt_sniffing_change_state: p_link %p, sniffing state change from %x to %x",
                        p_link, p_link->bt_sniffing_state, new_state);

        if (new_state == APP_BT_SNIFFING_STATE_ROLESWAP)
        {
            if (p_link->bt_sniffing_state <= APP_BT_SNIFFING_STATE_READY)
            {
                // For Sec in idle
                p_link->sniffing_type_before_roleswap = APP_BT_SNIFFING_STATE_READY;
            }
            else
            {
                // both Pri and Sec
                p_link->sniffing_type_before_roleswap = p_link->bt_sniffing_state;
            }
        }

        p_link->bt_sniffing_state = new_state;

        if ((app_cfg_nv.bud_role == REMOTE_SESSION_ROLE_PRIMARY) &&
            (p_link->bt_sniffing_state == APP_BT_SNIFFING_STATE_STARTING ||
             p_link->bt_sniffing_state == APP_BT_SNIFFING_STATE_STOPPING))
        {
            gap_stop_timer(&timer_handle_protected_timer);
            gap_start_timer(&timer_handle_protected_timer,
                            "sniffing_protected_timer",
                            bt_sniffing_timer_queue_id,
                            APP_BT_SNIFFING_TIMER_ID_PROTECTED_TIMER,
                            p_link->id,
                            SNIFFING_PROECTED_TIMER_MS);
            sniffing_protect_id = p_link->id;
        }
        else
        {
            if (p_link->id == sniffing_protect_id)
            {
                gap_stop_timer(&timer_handle_protected_timer);
            }
        }

        if (p_link->bt_sniffing_state == APP_BT_SNIFFING_STATE_READY ||
            p_link->bt_sniffing_state == APP_BT_SNIFFING_STATE_IDLE)
        {
            p_link->sniffing_active = false;
            p_link->sco_initial = false;
        }
        else if ((p_link->bt_sniffing_state == APP_BT_SNIFFING_STATE_SNIFFING) &&
                 (app_cfg_nv.bud_role == REMOTE_SESSION_ROLE_SECONDARY))
        {
            p_link->sniffing_active = true;
        }

        if (new_state != APP_BT_SNIFFING_STATE_ROLESWAP)
        {
            app_roleswap_ctrl_check(APP_ROLESWAP_CTRL_EVENT_SNIFF_EVENT);
        }
    }
}

static T_SNIFFING_CHECK_RESULT app_bt_sniffing_check(uint8_t *bd_addr)
{
    T_SNIFFING_CHECK_RESULT check_result = SNIFFING_ALLOW;
    T_APP_BR_LINK *p_link = app_find_br_link(bd_addr);
    bool is_b2b_link = app_check_b2b_link(bd_addr);

    if (!is_b2b_link && p_link != NULL)
    {
        {
            // Check b2b link
            T_APP_BR_LINK *p_b2b_link = app_find_br_link(app_cfg_nv.bud_peer_addr);
            if (app_db.disallow_sniff)
            {
                check_result = SNIFFING_B2B_POWER_OFF_ING;
            }
            else if (p_b2b_link)
            {
                APP_PRINT_TRACE3("app_bt_sniffing_check: p_link %p, "
                                 "connected_profile %x, acl_link_role %d",
                                 p_b2b_link, p_b2b_link->connected_profile,
                                 p_b2b_link->acl_link_role);
                if ((p_b2b_link->connected_profile & RDTP_PROFILE_MASK) == 0)
                {
                    check_result = SNIFFING_B2B_RTDP_NOT_EXIST;
                }

                if (p_b2b_link->acl_link_role == BT_LINK_ROLE_SLAVE)
                {
                    check_result = SNIFFING_B2B_ROLE_SWITCH;
                }

                if (p_b2b_link->acl_decrypted == false)
                {
                    check_result = SNIFFING_B2B_DECRYPT;
                }
            }
            else
            {
                check_result = SNIFFING_B2B_RTDP_NOT_EXIST;
            }
        }

        {
            // Check b2s link
            if ((p_link->connected_profile & (A2DP_PROFILE_MASK | HFP_PROFILE_MASK)) == 0)
            {
                check_result = SNIFFING_SRC_PROFILE_NOT_EXIST;
            }

            if (p_link->acl_link_role == BT_LINK_ROLE_MASTER)
            {
                check_result = SNIFFING_SRC_ROLE_SWITCH;
            }
        }

    }
    else
    {
        check_result = SNIFFING_LINK_ERROR;
    }

    APP_PRINT_INFO1("app_bt_sniffing_check: check_result %d", check_result);
    return check_result;
}

void app_bt_sniffing_audio_cfg(uint8_t *bd_addr, uint16_t interval, uint16_t flush_tout,
                               uint8_t rsvd_slot, uint8_t idle_slot, uint8_t idle_skip)
{
    T_APP_BR_LINK *p_link = app_find_br_link(bd_addr);
    if (p_link != NULL)
    {
        APP_PRINT_TRACE7("app_bt_sniffing_audio_cfg: p_link %p, sniffing_type %x, cfg: %x:%x:%x:%x:%x",
                         p_link, p_link->sniffing_type, interval, flush_tout, rsvd_slot, idle_slot, idle_skip);

        if (p_link->sniffing_type == BT_SNIFFING_TYPE_A2DP)
        {
            // app_bt_sniffing_state_machine(bd_addr, APP_BT_SNIFFING_EVENT_A2DP_RECONFIG);
            if (APP_BT_SNIFFING_STATE_SNIFFING == p_link->bt_sniffing_state)
            {
                if (app_bt_sniffing_param_diff_from_pre_value(interval, flush_tout, rsvd_slot, idle_slot,
                                                              idle_skip))
                {
                    setting_recovery_param = true;
                    bt_sniffing_link_audio_cfg(bd_addr, interval, flush_tout, rsvd_slot, idle_slot, idle_skip);
                    app_bt_sniffing_param_set_pre_value(true);
                }
                else
                {
                    APP_PRINT_TRACE0("app_bt_sniffing_audio_cfg: same param has been set");
                }
            }
        }
    }
    return;
}

/////////////////////////////////////////////////////////////////////////////////
static void app_bt_pending_ind_confirm(uint8_t *bd_addr)
{
    T_APP_BR_LINK *p_link = app_find_br_link(bd_addr);
    if (p_link != NULL && app_cfg_nv.bud_role == REMOTE_SESSION_ROLE_PRIMARY)
    {
        APP_PRINT_TRACE6("app_bt_pending_ind_confirm: p_link %p, sniffing_type %d, "
                         "streaming_fg %d, sco_initial %d, sco_handle %d, pending_ind_confirm %d",
                         p_link, p_link->sniffing_type, p_link->streaming_fg, p_link->sco_initial,
                         p_link->sco_handle, p_link->pending_ind_confirm);
        if (p_link->sniffing_type == BT_SNIFFING_TYPE_A2DP)
        {
            // a2dp recovery link ready, confirm a2dp start
            if (p_link->streaming_fg == false)
            {
                if (bt_a2dp_stream_start_cfm(bd_addr, true))
                {
                    p_link->streaming_fg = true;
                    p_link->pending_ind_confirm = false;
                    app_judge_active_a2dp_idx_and_qos(p_link->id, JUDGE_EVENT_A2DP_START);
                }
            }
        }
        else if (p_link->sniffing_type == BT_SNIFFING_TYPE_SCO)
        {
            // accept sco req to build sco link
            if (!p_link->sco_initial)
            {
                if (p_link->sco_handle == 0)
                {
                    bt_sco_conn_cfm(bd_addr, true);
                    p_link->pending_ind_confirm = false;
                }
            }
            else
            {
                if (p_link->sco_handle == 0)
                {
                    bt_hfp_audio_connect_req(bd_addr);
                }
                p_link->sco_initial = false;
            }
        }
    }
}

static void acl_link_disconn_event_handle(uint8_t *bd_addr, bool is_b2b_link)
{
    APP_PRINT_TRACE2("acl_link_disconn_event_handle:  bud_role %d, is_b2b_link %d",
                     app_cfg_nv.bud_role, is_b2b_link);

    if (is_b2b_link)
    {
        // htpoll param pre value set as 0
        app_bt_sniffing_param_set_pre_value(false);
        setting_recovery_param = false;

        // all b2s link set to idle
        for (int i = 0; i < MAX_BR_LINK_NUM; i++)
        {
            if (app_db.br_link[i].used == true)
            {
                app_bt_sniffing_change_state(app_db.br_link[i].bd_addr, APP_BT_SNIFFING_STATE_IDLE);
            }
        }
    }
    // cannot get b2s link here
}


static bool app_bt_sniffing_state_idle_handle(uint8_t *bd_addr,
                                              T_APP_BT_SNIFFING_STATE_EVENT sniffing_event)
{
    switch (sniffing_event)
    {
    case APP_BT_SNIFFING_EVENT_ACL_SNIFFING_CMPL:
        if (app_cfg_nv.bud_role == REMOTE_SESSION_ROLE_SECONDARY)
        {
            app_bt_sniffing_change_state(bd_addr, APP_BT_SNIFFING_STATE_STARTING);
            return true;
        }
        break;

    case APP_BT_SNIFFING_EVENT_ROLESWAP_CMPL:
        if (app_cfg_nv.bud_role == REMOTE_SESSION_ROLE_PRIMARY)
        {
            app_bt_sniffing_change_state(bd_addr, APP_BT_SNIFFING_STATE_READY);
            return true;
        }
        break;
    }

    return false;
}

static bool app_bt_sniffing_param_diff_from_pre_value(uint16_t interval, uint16_t flush_tout,
                                                      uint8_t rsvd_slot,
                                                      uint8_t idle_slot, uint8_t idle_skip)
{
    if ((interval != pre_recovery_link_a2dp_interval) ||
        (flush_tout != pre_recovery_link_a2dp_flush_timeout) ||
        (rsvd_slot != pre_recovery_link_a2dp_rsvd_slot) ||
        (idle_slot != pre_recovery_link_a2dp_idle_slot) ||
        (idle_skip != pre_recovery_link_a2dp_idle_skip))
    {
        return true;
    }
    else
    {
        return false;
    }
}

static void app_bt_sniffing_param_set_pre_value(bool set_value)
{
    if (set_value)
    {
        pre_recovery_link_a2dp_interval = recovery_link_a2dp_interval;
        pre_recovery_link_a2dp_flush_timeout = recovery_link_a2dp_flush_timeout;
        pre_recovery_link_a2dp_rsvd_slot = recovery_link_a2dp_rsvd_slot;
        pre_recovery_link_a2dp_idle_slot = recovery_link_a2dp_idle_slot;
        pre_recovery_link_a2dp_idle_skip = recovery_link_a2dp_idle_skip;
    }
    else
    {
        pre_recovery_link_a2dp_interval = 0;
        pre_recovery_link_a2dp_flush_timeout = 0;
        pre_recovery_link_a2dp_rsvd_slot = 0;
        pre_recovery_link_a2dp_idle_slot = 0;
        pre_recovery_link_a2dp_idle_skip = 0;
    }
    APP_PRINT_TRACE5("app_bt_sniffing_param_set_pre_value pre_interval %x, pre_flush_timeout %x, pre_rsvd_slot %x, pre_idle_slot %x, pre_idle_skip %x",
                     pre_recovery_link_a2dp_interval, pre_recovery_link_a2dp_flush_timeout,
                     pre_recovery_link_a2dp_rsvd_slot,
                     pre_recovery_link_a2dp_idle_slot, pre_recovery_link_a2dp_idle_skip);
}

static bool app_bt_sniffing_state_ready_handle(uint8_t *bd_addr,
                                               T_APP_BT_SNIFFING_STATE_EVENT sniffing_event)
{
    T_APP_BR_LINK *p_link = app_find_br_link(bd_addr);
    if (!p_link)
    {
        return false;
    }

    switch (sniffing_event)
    {
    case APP_BT_SNIFFING_EVENT_ACL_SNIFFING_CMPL:
        if (app_cfg_nv.bud_role == REMOTE_SESSION_ROLE_SECONDARY)
        {
            app_bt_sniffing_change_state(bd_addr, APP_BT_SNIFFING_STATE_STARTING);
            return true;
        }
        break;

    case APP_BT_SNIFFING_EVENT_ACL_SNIFFING_DISCONN_CMPL:
        {
            // error handling
            p_link->sniffing_active = false;
            if (app_cfg_nv.bud_role == REMOTE_SESSION_ROLE_PRIMARY)
            {
                if (p_link->streaming_fg || p_link->sco_handle != 0)
                {
                    if (p_link->sco_handle != 0)
                    {
                        p_link->sniffing_type = BT_SNIFFING_TYPE_SCO;
                    }
                    else if (p_link->streaming_fg)
                    {
                        p_link->sniffing_type = BT_SNIFFING_TYPE_A2DP;
                    }

                    if (bt_sniffing_link_connect(bd_addr))
                    {
                        app_bt_sniffing_change_state(bd_addr, APP_BT_SNIFFING_STATE_STARTING);
                        p_link->sniffing_active = true;
                    }
                    else
                    {
                        p_link->sniffing_type = BT_SNIFFING_TYPE_NONE;
                    }
                }
                else
                {
                    p_link->sniffing_type = BT_SNIFFING_TYPE_NONE;
                }
                APP_PRINT_TRACE4("app_bt_sniffing_state_ready_handle: p_link %p, sco_handle %d, streaming_fg %d, sniffing_type %d",
                                 p_link, p_link->sco_handle, p_link->streaming_fg, p_link->sniffing_type);
            }
        }
        break;

    case APP_BT_SNIFFING_EVENT_SCO_CONN_CMPL:
        {
            if (bt_sniffing_link_connect(bd_addr))
            {
                app_bt_sniffing_change_state(bd_addr, APP_BT_SNIFFING_STATE_STARTING);
                p_link->sniffing_active = true;
                p_link->sniffing_type = BT_SNIFFING_TYPE_SCO;
                return true;
            }
            else
            {
                p_link->sniffing_active = false;
                p_link->sniffing_type = BT_SNIFFING_TYPE_NONE;
            }
        }
        break;

    case APP_BT_SNIFFING_EVENT_A2DP_START_IND:
    case APP_BT_SNIFFING_EVENT_SCO_CONN_IND:
        {
            if (bt_sniffing_link_connect(bd_addr))
            {
                app_bt_sniffing_change_state(bd_addr, APP_BT_SNIFFING_STATE_STARTING);
                p_link->sniffing_active = true;
                return true;
            }
            else
            {
                p_link->sniffing_active = false;
            }
        }
        break;

    case APP_BT_SNIFFING_EVENT_ROLESWAP_START:
        {
            // allow to do roleswap
            app_bt_sniffing_change_state(bd_addr, APP_BT_SNIFFING_STATE_ROLESWAP);
            if (remote_roleswap_start(bd_addr, 0, p_link->stop_after_shadow, app_roleswap_start))
            {
                return true;
            }
        }
        break;

    }

    return false;
}

static bool app_bt_sniffing_state_starting_handle(uint8_t *bd_addr,
                                                  T_APP_BT_SNIFFING_STATE_EVENT sniffing_event)
{
    T_APP_BR_LINK *p_link = app_find_br_link(bd_addr);
    if (!p_link)
    {
        return false;
    }

    APP_PRINT_TRACE6("app_bt_sniffing_state_starting_handle: event %d, sniffing_type %d, "
                     "streaming_fg %d, sco_initial %d, sco_handle %d, pending_ind_confirm %d",
                     sniffing_event, p_link->sniffing_type, p_link->streaming_fg, p_link->sco_initial,
                     p_link->sco_handle, p_link->pending_ind_confirm);

    switch (sniffing_event)
    {
    case APP_BT_SNIFFING_EVENT_ACL_SNIFFING_CMPL:
        {
            if (app_cfg_nv.bud_role == REMOTE_SESSION_ROLE_PRIMARY)
            {
                if (p_link->sniffing_type == BT_SNIFFING_TYPE_A2DP)
                {
                    if (bt_sniffing_link_audio_start(bd_addr,
                                                     recovery_link_a2dp_interval,
                                                     recovery_link_a2dp_flush_timeout,
                                                     recovery_link_a2dp_rsvd_slot,
                                                     recovery_link_a2dp_idle_slot,
                                                     recovery_link_a2dp_idle_skip))
                    {
                        return true;
                    }
                    else
                    {
                        bt_sniffing_link_disconnect(bd_addr);
                        app_bt_sniffing_change_state(bd_addr, APP_BT_SNIFFING_STATE_STOPPING);
                    }
                }
                else if (p_link->sniffing_type == BT_SNIFFING_TYPE_SCO)
                {
                    if (p_link->pending_ind_confirm == true)
                    {
                        app_bt_pending_ind_confirm(bd_addr);
                    }

                    if (p_link->sco_handle != 0)
                    {
                        //sco connected
                        if (bt_sniffing_link_voice_start(bd_addr))
                        {
                            return true;
                        }
                        else
                        {
                            bt_sniffing_link_disconnect(bd_addr);
                            app_bt_sniffing_change_state(bd_addr, APP_BT_SNIFFING_STATE_STOPPING);
                        }
                    }
                }
                else
                {
                    bt_sniffing_link_disconnect(bd_addr);
                    app_bt_sniffing_change_state(bd_addr, APP_BT_SNIFFING_STATE_STOPPING);
                }
            }
#if F_APP_MUTILINK_VA_PREEMPTIVE
            if (app_cfg_const.enable_multi_link && sniffing_starting_a2dp_mute)
            {
                sniffing_starting_a2dp_mute = false;
                uint8_t cmd_ptr[7];
                memcpy(&cmd_ptr[0], app_db.br_link[p_link->id].bd_addr, 6);
                cmd_ptr[6] = AUDIO_STREAM_TYPE_PLAYBACK;
                app_relay_async_single_with_raw_msg(APP_MODULE_TYPE_MULTI_LINK,
                                                    APP_REMOTE_MSG_MUTE_AUDIO_WHEN_MULTI_CALL_NOT_IDLE,
                                                    cmd_ptr, 7);
            }
#endif
        }
        break;

    case APP_BT_SNIFFING_EVENT_ACL_SNIFFING_DISCONN_CMPL:
        {
            p_link->sniffing_active = false;
            app_bt_sniffing_change_state(bd_addr, APP_BT_SNIFFING_STATE_READY);
        }
        break;

    case APP_BT_SNIFFING_EVENT_A2DP_SNIFFING_STARTED:
        {
            if (app_cfg_nv.bud_role == REMOTE_SESSION_ROLE_PRIMARY)
            {
                if (p_link->pending_ind_confirm == true && app_cfg_const.rws_gaming_mode == 0)
                {
                    app_bt_pending_ind_confirm(bd_addr);
                }
            }

            p_link->sniffing_type = BT_SNIFFING_TYPE_A2DP;
            app_bt_sniffing_change_state(bd_addr, APP_BT_SNIFFING_STATE_SNIFFING);
            app_db.active_media_paused = false;

            app_bt_sniffing_judge_dynamic_set_gaming_mode();

#if F_APP_MUTILINK_VA_PREEMPTIVE
            if ((p_link->sco_handle != 0 && app_cfg_nv.bud_role == REMOTE_SESSION_ROLE_PRIMARY) ||
                (app_cfg_const.enable_multi_link && sniffing_starting_a2dp_stop))
#else
            if (p_link->sco_handle != 0 && app_cfg_nv.bud_role == REMOTE_SESSION_ROLE_PRIMARY)
#endif
            {
#ifdef APP_SNIFFING_ERWS2_0
#if F_APP_MUTILINK_VA_PREEMPTIVE
                sniffing_starting_a2dp_stop = false;
#endif
                if (bt_sniffing_link_voice_start(bd_addr))
                {
                    p_link->sniffing_type = BT_SNIFFING_TYPE_SCO;
                    app_bt_sniffing_change_state(bd_addr, APP_BT_SNIFFING_STATE_STARTING);
                    return true;
                }
                else
                {
                    bt_sniffing_link_disconnect(bd_addr);
                    app_bt_sniffing_change_state(bd_addr, APP_BT_SNIFFING_STATE_STOPPING);
                }
#else
                // disconnect recovery link
                if (bt_sniffing_link_audio_stop(bd_addr, HCI_ERR_AUDIO_STOP))
                {
#if F_APP_MUTILINK_VA_PREEMPTIVE
                    sniffing_starting_a2dp_stop = false;
#endif
                    app_bt_sniffing_change_state(bd_addr, APP_BT_SNIFFING_STATE_STOPPING);
                }
#endif
            }
        }
        break;

    case APP_BT_SNIFFING_EVENT_SCO_CONN_CMPL:
        {
            if (p_link->sniffing_type == BT_SNIFFING_TYPE_SCO)
            {
                ///TODO: set SCO parameter
                if (bt_sniffing_link_voice_start(bd_addr))
                {
                    return true;
                }
            }
        }
        break;

    case APP_BT_SNIFFING_EVENT_SCO_SNIFFIG_STARTED:
        {
            // sco recovery link ready
            p_link->sniffing_type = BT_SNIFFING_TYPE_SCO;
            app_bt_sniffing_change_state(bd_addr, APP_BT_SNIFFING_STATE_SNIFFING);
        }
        break;

    case APP_BT_SNIFFING_EVENT_SCO_DISCONN_CMPL:
        {
            if (p_link->sniffing_type == BT_SNIFFING_TYPE_SCO)
            {
                if (p_link->streaming_fg != false)
                {
                    if (bt_sniffing_link_audio_start(bd_addr,
                                                     recovery_link_a2dp_interval,
                                                     recovery_link_a2dp_flush_timeout,
                                                     recovery_link_a2dp_rsvd_slot,
                                                     recovery_link_a2dp_idle_slot,
                                                     recovery_link_a2dp_idle_skip))
                    {
                        p_link->sniffing_type = BT_SNIFFING_TYPE_A2DP;
                        break;
                    }
                }
                else
                {
                    if (bt_sniffing_link_disconnect(bd_addr))
                    {
                        // disconnect sniffing link (disconnect recovery link first)
                        app_bt_sniffing_change_state(bd_addr, APP_BT_SNIFFING_STATE_STOPPING);
                    }
                }
            }
        }
        break;

#if F_APP_MUTILINK_VA_PREEMPTIVE
    case APP_BT_SNIFFING_EVENT_A2DP_STOP:
        {
            if (!app_cfg_const.enable_multi_link)
            {
                break;
            }

            if (app_cfg_nv.bud_role == REMOTE_SESSION_ROLE_PRIMARY &&
                p_link->sniffing_type == BT_SNIFFING_TYPE_A2DP)
            {
                sniffing_starting_a2dp_stop = true;
            }
        }
        break;

    case APP_BT_SNIFFING_EVENT_A2DP_MUTE:
        {
            if (!app_cfg_const.enable_multi_link)
            {
                break;
            }

            if (app_cfg_nv.bud_role == REMOTE_SESSION_ROLE_PRIMARY &&
                p_link->sniffing_type == BT_SNIFFING_TYPE_A2DP)
            {
                sniffing_starting_a2dp_mute = true;
            }
        }
        break;

    case APP_BT_SNIFFING_EVENT_A2DP_UNMUTE:
        {
            if (!app_cfg_const.enable_multi_link)
            {
                break;
            }

            if (app_cfg_nv.bud_role == REMOTE_SESSION_ROLE_PRIMARY &&
                p_link->sniffing_type == BT_SNIFFING_TYPE_A2DP)
            {
                sniffing_starting_a2dp_mute = false;
            }
        }
        break;
#endif
    }

    return false;
}

static bool app_bt_sniffing_state_sniffing_handle(uint8_t *bd_addr,
                                                  T_APP_BT_SNIFFING_STATE_EVENT sniffing_event)
{
    bool result = false;
    T_APP_BR_LINK *p_link = app_find_br_link(bd_addr);
    if (!p_link)
    {
        return false;
    }

    switch (sniffing_event)
    {
    case APP_BT_SNIFFING_EVENT_SCO_DISCONN_CMPL:
        {
            if (p_link->streaming_fg != false)
            {
                if (bt_sniffing_link_audio_start(bd_addr,
                                                 recovery_link_a2dp_interval,
                                                 recovery_link_a2dp_flush_timeout,
                                                 recovery_link_a2dp_rsvd_slot,
                                                 recovery_link_a2dp_idle_slot,
                                                 recovery_link_a2dp_idle_skip))
                {
                    p_link->sniffing_type = BT_SNIFFING_TYPE_A2DP;
                    app_bt_sniffing_change_state(bd_addr, APP_BT_SNIFFING_STATE_STARTING);
                }
            }
#ifdef APP_SNIFFING_ERWS2_0
            else if (bt_sniffing_link_voice_stop(bd_addr, HCI_ERR_AUDIO_STOP))
            {
                app_bt_sniffing_change_state(bd_addr, APP_BT_SNIFFING_STATE_STOPPING);
            }
#else
            else if (bt_sniffing_link_disconnect(bd_addr))
            {
                // disconnect sniffing link (disconnect recovery link first)
                app_bt_sniffing_change_state(bd_addr, APP_BT_SNIFFING_STATE_STOPPING);
            }
#endif
        }
        break;

    case APP_BT_SNIFFING_EVENT_ACL_SNIFFING_DISCONN_CMPL:
        {
            // link loss
            p_link->sniffing_active = false;
            app_bt_sniffing_change_state(bd_addr, APP_BT_SNIFFING_STATE_READY);
            ///TODO: Retry Sniffing ???
        }
        break;

    case APP_BT_SNIFFING_EVENT_SCO_CONN_CMPL:
    // switch to sco sniffing
    case APP_BT_SNIFFING_EVENT_A2DP_STOP:
        {
#ifdef APP_SNIFFING_ERWS2_0
            if (p_link->sco_handle != 0 && app_cfg_nv.bud_role == REMOTE_SESSION_ROLE_PRIMARY)
            {
                if (bt_sniffing_link_voice_start(bd_addr))
                {
                    p_link->sniffing_type = BT_SNIFFING_TYPE_SCO;
                    app_bt_sniffing_change_state(bd_addr, APP_BT_SNIFFING_STATE_STARTING);
                    return true;
                }
                else
                {
                    if (bt_sniffing_link_audio_stop(bd_addr, HCI_ERR_AUDIO_STOP))
                    {
                        app_bt_sniffing_change_state(bd_addr, APP_BT_SNIFFING_STATE_STOPPING);
                    }
                }
            }
            else if (app_cfg_nv.bud_role == REMOTE_SESSION_ROLE_PRIMARY &&
                     p_link->sniffing_type == BT_SNIFFING_TYPE_A2DP)
            {
                if (bt_sniffing_link_audio_stop(bd_addr, HCI_ERR_AUDIO_STOP))
                {
                    app_bt_sniffing_change_state(bd_addr, APP_BT_SNIFFING_STATE_STOPPING);
                }
            }
#else
            if (app_cfg_nv.bud_role == REMOTE_SESSION_ROLE_PRIMARY &&
                p_link->sniffing_type == BT_SNIFFING_TYPE_A2DP)
            {
                if (bt_sniffing_link_audio_stop(bd_addr, HCI_ERR_AUDIO_STOP))
                {
                    app_bt_sniffing_change_state(bd_addr, APP_BT_SNIFFING_STATE_STOPPING);
                }
            }
#endif
        }
        break;

    case APP_BT_SNIFFING_EVENT_A2DP_START_IND:
    case APP_BT_SNIFFING_EVENT_SCO_CONN_IND:
        {
            result = false;
        }
        break;

    case APP_BT_SNIFFING_EVENT_ROLESWAP_START:
        {
            // allow to do roleswap
            app_bt_sniffing_change_state(bd_addr, APP_BT_SNIFFING_STATE_ROLESWAP);
            result = remote_roleswap_start(bd_addr, 0, p_link->stop_after_shadow, app_roleswap_start);
        }
        break;
    }

    return result;
}

static bool app_bt_sniffing_state_stopping_handle(uint8_t *bd_addr,
                                                  T_APP_BT_SNIFFING_STATE_EVENT sniffing_event)
{
    T_APP_BR_LINK *p_link = app_find_br_link(bd_addr);
    if (!p_link)
    {
        return false;
    }

    switch (sniffing_event)
    {
    case APP_BT_SNIFFING_EVENT_ACL_SNIFFING_DISCONN_CMPL:
        {
            // sniffing link disconnect complete
            app_bt_sniffing_change_state(bd_addr, APP_BT_SNIFFING_STATE_READY);
            p_link->sniffing_active = false;

            if (app_cfg_nv.bud_role == REMOTE_SESSION_ROLE_PRIMARY)
            {
                if (app_active_link_check(bd_addr) && !app_db.disallow_sniff)
                {
                    if (p_link->sco_handle != 0)
                    {
                        p_link->sniffing_type = BT_SNIFFING_TYPE_SCO;
                    }
                    else if (p_link->streaming_fg)
                    {
                        p_link->sniffing_type = BT_SNIFFING_TYPE_A2DP;
                    }

                    if (bt_sniffing_link_connect(bd_addr))
                    {
                        p_link->sniffing_active = true;
                        app_bt_sniffing_change_state(bd_addr, APP_BT_SNIFFING_STATE_STARTING);
                    }
                    else
                    {
                        p_link->sniffing_type = BT_SNIFFING_TYPE_NONE;
                    }
                }
                APP_PRINT_TRACE4("app_bt_sniffing_state_stopping_handle: p_link %p, sco_handle %d, "
                                 "streaming_fg %d, sniffing_type",
                                 p_link, p_link->sco_handle, p_link->streaming_fg, p_link->sniffing_type);
            }
        }
        break;

    case APP_BT_SNIFFING_EVENT_A2DP_SNIFFING_STOPPED:
        if (p_link->sniffing_type == BT_SNIFFING_TYPE_A2DP)
        {
            if (app_cfg_nv.bud_role == REMOTE_SESSION_ROLE_PRIMARY)
            {
                if (p_link->sco_handle != 0 && !app_db.disallow_sniff)
                {
                    if (bt_sniffing_link_voice_start(bd_addr))
                    {
                        p_link->sniffing_type = BT_SNIFFING_TYPE_SCO;
                        app_bt_sniffing_change_state(bd_addr, APP_BT_SNIFFING_STATE_STARTING);
                        break;
                    }
                }
                bt_sniffing_link_disconnect(bd_addr);
            }
        }
        break;

#ifdef APP_SNIFFING_ERWS2_0
    case APP_BT_SNIFFING_EVENT_SCO_SNIFFING_STOPPED:
        if (p_link->sniffing_type == BT_SNIFFING_TYPE_SCO)
        {
            if (app_cfg_nv.bud_role == REMOTE_SESSION_ROLE_PRIMARY)
            {
                if (p_link->streaming_fg != false && !app_db.disallow_sniff)
                {
                    if (bt_sniffing_link_audio_start(bd_addr,
                                                     recovery_link_a2dp_interval,
                                                     recovery_link_a2dp_flush_timeout,
                                                     recovery_link_a2dp_rsvd_slot,
                                                     recovery_link_a2dp_idle_slot,
                                                     recovery_link_a2dp_idle_skip))
                    {
                        p_link->sniffing_type = BT_SNIFFING_TYPE_A2DP;
                        app_bt_sniffing_change_state(bd_addr, APP_BT_SNIFFING_STATE_STARTING);
                        break;
                    }
                }
                bt_sniffing_link_disconnect(bd_addr);
            }
        }
        break;
#endif
    }
    return true;
}

static bool app_bt_sniffing_state_roleswap_handle(uint8_t *bd_addr,
                                                  T_APP_BT_SNIFFING_STATE_EVENT sniffing_event)
{
    T_APP_BR_LINK *p_link = app_find_br_link(bd_addr);
    if (!p_link)
    {
        return false;
    }

    switch (sniffing_event)
    {
    case APP_BT_SNIFFING_EVENT_ROLESWAP_CMPL:
        {
            app_bt_sniffing_change_state(bd_addr, p_link->sniffing_type_before_roleswap);

            if (p_link->sniffing_type == BT_SNIFFING_TYPE_A2DP &&
                p_link->sniffing_type_before_roleswap == APP_BT_SNIFFING_STATE_SNIFFING &&
                app_cfg_nv.bud_role == REMOTE_SESSION_ROLE_PRIMARY)
            {
                if (p_link->streaming_fg == 0)
                {
                    app_bt_sniffing_stop(bd_addr, BT_SNIFFING_TYPE_A2DP);
                }
                break;
            }

#if F_APP_QOL_MONITOR_SUPPORT
            if ((p_link->sniffing_type == BT_SNIFFING_TYPE_A2DP) &&
                (p_link->sniffing_type_before_roleswap) == APP_BT_SNIFFING_STATE_SNIFFING)
            {
                APP_PRINT_TRACE1("app_bt_sniffing_state_roleswap_handle: bd_addr %s",
                                 TRACE_BDADDR(app_db.br_link[app_get_active_a2dp_idx()].bd_addr));

                app_qol_link_monitor(app_db.br_link[app_get_active_a2dp_idx()].bd_addr, true);
            }
#endif
        }
        break;

    case APP_BT_SNIFFING_EVENT_ACL_SNIFFING_DISCONN_CMPL:
        {
            p_link->sniffing_active = false;
            app_bt_sniffing_change_state(bd_addr, APP_BT_SNIFFING_STATE_READY);
        }
        break;

    case APP_BT_SNIFFING_EVENT_A2DP_STOP:
    case APP_BT_SNIFFING_EVENT_SCO_DISCONN_CMPL:
        {
            if (app_cfg_nv.bud_role == REMOTE_SESSION_ROLE_PRIMARY)
            {
                remote_roleswap_stop(bd_addr);
            }
        }
        break;

    case APP_BT_SNIFFING_EVENT_ROLESWAP_TERMINATED:
        {
            if (p_link->sniffing_type_before_roleswap == APP_BT_SNIFFING_STATE_SNIFFING)
            {
                if ((p_link->sco_handle == 0) && (p_link->streaming_fg == false))
                {
                    bt_sniffing_link_disconnect(bd_addr);
                    app_bt_sniffing_change_state(bd_addr, APP_BT_SNIFFING_STATE_STOPPING);
                }
                else
                {
                    app_bt_sniffing_change_state(bd_addr, APP_BT_SNIFFING_STATE_SNIFFING);
                }
            }
            else if (p_link->sniffing_type_before_roleswap == APP_BT_SNIFFING_STATE_READY)
            {
                app_bt_sniffing_change_state(bd_addr, APP_BT_SNIFFING_STATE_READY);
            }
        }

    }

    return true;
}

static bool app_bt_sniffing_state_machine(uint8_t *bd_addr,
                                          T_APP_BT_SNIFFING_STATE_EVENT sniffing_event)
{
    bool result = false;
    T_APP_BR_LINK *p_link = app_find_br_link(bd_addr);
    if (!p_link)
    {
        return false;
    }

    APP_PRINT_TRACE4("app_bt_sniffing_state_machine: p_link %p, bd_addr %s, bt_sniffing_state %x, sniffing_event %x",
                     p_link, TRACE_BDADDR(p_link->bd_addr), p_link->bt_sniffing_state, sniffing_event);
    switch (p_link->bt_sniffing_state)
    {
    case APP_BT_SNIFFING_STATE_IDLE:
        result = app_bt_sniffing_state_idle_handle(bd_addr, sniffing_event);
        break;
    case APP_BT_SNIFFING_STATE_READY:
        result = app_bt_sniffing_state_ready_handle(bd_addr, sniffing_event);
        break;
    case APP_BT_SNIFFING_STATE_STARTING:
        result = app_bt_sniffing_state_starting_handle(bd_addr, sniffing_event);
        break;
    case APP_BT_SNIFFING_STATE_SNIFFING:
        result = app_bt_sniffing_state_sniffing_handle(bd_addr, sniffing_event);
        break;
    case APP_BT_SNIFFING_STATE_STOPPING:
        result = app_bt_sniffing_state_stopping_handle(bd_addr, sniffing_event);
        break;
    case APP_BT_SNIFFING_STATE_ROLESWAP:
        result = app_bt_sniffing_state_roleswap_handle(bd_addr, sniffing_event);
        break;
    default:
        break;
    }
    return result;
}

static void app_bt_sniffing_event_cback(T_BT_EVENT event_type, void *event_buf, uint16_t buf_len)
{
    T_BT_EVENT_PARAM *param = event_buf;
    uint8_t *bd_addr = NULL;
    T_APP_BR_LINK *p_link = NULL;
    bool is_b2b_link;
    bool handle = true;

    switch (event_type)
    {
    case BT_EVENT_ACL_CONN_DISCONN:
        {
            is_b2b_link = app_check_b2b_link(param->acl_conn_disconn.bd_addr);

#if F_APP_QOL_MONITOR_SUPPORT
            app_db.src_going_away_multilink_enabled = false;
#endif
            acl_link_disconn_event_handle(param->acl_conn_disconn.bd_addr, is_b2b_link);
        }
        break;



    case BT_EVENT_SCO_CONN_CMPL:
        // recovery link connected
#if F_APP_MUTILINK_VA_PREEMPTIVE
        bd_addr = param->sco_conn_cmpl.bd_addr;
        p_link = app_find_br_link(bd_addr);
        if ((p_link != NULL) && ((!app_cfg_const.enable_multi_link) ||
                                 (app_cfg_const.enable_multi_link &&
                                  app_hfp_get_active_hf_index() == p_link->id)))
#endif
        {
            app_bt_sniffing_state_machine(param->sco_conn_cmpl.bd_addr,
                                          APP_BT_SNIFFING_EVENT_SCO_CONN_CMPL);
        }
        break;

    case BT_EVENT_ACL_SNIFFING_CONN_CMPL:
        APP_PRINT_TRACE2("app_bt_sniffing_event_cback: BT_EVENT_ACL_SNIFFING_CONN_CMPL, cause %x %s",
                         param->acl_sniffing_conn_cmpl.cause, TRACE_BDADDR(param->acl_sniffing_conn_cmpl.bd_addr));
        if (param->acl_sniffing_conn_cmpl.cause == HCI_SUCCESS)
        {
            // Pri: sniffing link setup successfully, Pri need to check current a2dp or hfp state
            // if (app_cfg_nv.bud_role == REMOTE_SESSION_ROLE_PRIMARY)
            {
                app_db.recover_param = false;
                app_db.down_count = 0;

#if F_APP_MUTILINK_VA_PREEMPTIVE
                uint8_t i;
                for (i = 0; i < MAX_BR_LINK_NUM; i++)
                {
                    if (app_check_b2s_link_by_id(i))
                    {
                        if (i != app_get_active_a2dp_idx())
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
                    app_bt_sniffing_param_update(APP_BT_SNIFFING_EVENT_ACL_SNIFFING_LINK_CMPL_MULTILINK_PREEMPTIVE);
                }
                else
#endif
                {
                    app_bt_sniffing_param_update(APP_BT_SNIFFING_EVENT_ACL_SNIFFING_LINK_CMPL_NORMAL);
                }

                if (app_cfg_nv.bud_role == REMOTE_SESSION_ROLE_SECONDARY)
                {
                    bd_addr = param->acl_sniffing_conn_cmpl.bd_addr;
                    p_link = app_find_br_link(bd_addr);

                    if (p_link)
                    {
                        app_set_active_a2dp_idx(p_link->id);
                        app_hfp_set_active_hf_index(bd_addr);
                    }
                }
                app_bt_sniffing_state_machine(param->acl_sniffing_conn_cmpl.bd_addr,
                                              APP_BT_SNIFFING_EVENT_ACL_SNIFFING_CMPL);
            }
        }
        else
        {
            p_link = app_find_br_link(param->acl_sniffing_conn_cmpl.bd_addr);

            if (p_link)
            {
                // sniffing link setup fail
                app_bt_sniffing_change_state(param->acl_sniffing_conn_cmpl.bd_addr, APP_BT_SNIFFING_STATE_READY);

                // retry if error code is HCI_ERR_CONN_TIMEOUT
                if (param->acl_sniffing_conn_cmpl.cause == (HCI_ERR | HCI_ERR_CONN_TIMEOUT) ||
                    param->acl_sniffing_conn_cmpl.cause == (HCI_ERR | HCI_ERR_CONTROLLER_BUSY))
                {
                    if (bt_sniffing_link_connect(param->acl_sniffing_conn_cmpl.bd_addr))
                    {
                        APP_PRINT_TRACE0("app_bt_sniffing_event_cback: setup sniffing link retry");
                        app_bt_sniffing_change_state(param->acl_sniffing_conn_cmpl.bd_addr,
                                                     APP_BT_SNIFFING_STATE_STARTING);
                    }
                }
                else
                {
                    ///TODO: Sniffing build fail, accept or not ???
                    if (p_link->pending_ind_confirm == true)
                    {
                        app_bt_pending_ind_confirm(param->acl_sniffing_conn_cmpl.bd_addr);
                    }
                }
            }
            else
            {
                APP_PRINT_ERROR1("app_bt_sniffing_event_cback: BT_EVENT_ACL_SNIFFING_CONN_CMPL, bd_addr %s",
                                 TRACE_BDADDR(param->acl_sniffing_conn_cmpl.bd_addr));
            }
        }
        break;

    case BT_EVENT_ACL_SNIFFING_DISCONN_CMPL:
        {
            app_bt_sniffing_param_set_pre_value(false);
            app_db.recover_param = false;
            app_db.down_count = 0;
            app_db.a2dp_preemptive_flag = false;
            setting_recovery_param = false;

            p_link = app_find_br_link(param->acl_sniffing_disconn_cmpl.bd_addr);
            if (p_link != NULL)
            {
                if (p_link->bt_sniffing_state == APP_BT_SNIFFING_STATE_SNIFFING
                    && ((HCI_ERR | HCI_ERR_CONN_TIMEOUT) == param->acl_sniffing_disconn_cmpl.cause))
                {
                    // retry sniffing
                    gap_start_timer(&timer_handle_retry_sniffing,
                                    "start_sniffing_retry",
                                    bt_sniffing_timer_queue_id,
                                    APP_BT_SNIFFING_TIMER_ID_RETRY_SNIFFING,
                                    p_link->id,
                                    SNIFFING_RETRY_TIMER_MS);
                }
#if F_APP_QOL_MONITOR_SUPPORT
                app_qol_link_monitor(param->acl_sniffing_disconn_cmpl.bd_addr, false);
#endif
                app_bt_sniffing_state_machine(param->acl_sniffing_disconn_cmpl.bd_addr,
                                              APP_BT_SNIFFING_EVENT_ACL_SNIFFING_DISCONN_CMPL);
                app_handle_sniffing_link_disconnect_event(p_link->id);

            }
        }
        break;

    case BT_EVENT_A2DP_SNIFFING_CONFIG_CMPL:
        {
            setting_recovery_param = false;
        }
        break;

    case BT_EVENT_A2DP_SNIFFING_STARTED:
        /// error handling: recovery link fail
        APP_PRINT_TRACE1("app_bt_sniffing_event_cback: BT_EVENT_A2DP_SNIFFING_STARTED, cause %x",
                         param->a2dp_sniffing_started.cause);
        p_link = app_find_br_link(param->a2dp_sniffing_started.bd_addr);

        if (param->a2dp_sniffing_started.cause == HCI_SUCCESS)
        {
            // recovery link connected
            app_bt_sniffing_param_set_pre_value(true);

#if F_APP_QOL_MONITOR_SUPPORT
            if (app_cfg_nv.bud_role == REMOTE_SESSION_ROLE_PRIMARY)
            {
                app_qol_link_monitor(param->a2dp_sniffing_started.bd_addr, true);
                app_qol_link_monitor(app_cfg_nv.bud_peer_addr, true);
            }
#endif
            app_bt_sniffing_state_machine(param->a2dp_sniffing_started.bd_addr,
                                          APP_BT_SNIFFING_EVENT_A2DP_SNIFFING_STARTED);

        }
        else
        {
            // disconnect sniffing link
            if (app_cfg_nv.bud_role == REMOTE_SESSION_ROLE_PRIMARY)
            {
                bt_sniffing_link_disconnect(param->a2dp_sniffing_started.bd_addr);
                app_bt_sniffing_change_state(param->a2dp_sniffing_started.bd_addr,
                                             APP_BT_SNIFFING_STATE_STOPPING);
            }

            ///TODO: Recoovery build fail, accept or not ???
            if (p_link && (p_link->pending_ind_confirm == true))
            {
                app_bt_pending_ind_confirm(param->a2dp_sniffing_started.bd_addr);
            }
        }
        break;

    case BT_EVENT_A2DP_SNIFFING_STOPPED:
        // a2dp recovery link disconnected
        APP_PRINT_TRACE1("app_bt_sniffing_event_cback: BT_EVENT_A2DP_SNIFFING_STOPPED %x",
                         param->a2dp_sniffing_stopped.cause);
        // ignore sniffing link disconnect event caused by roleswap
#if F_APP_QOL_MONITOR_SUPPORT
        app_qol_link_monitor(param->a2dp_sniffing_stopped.bd_addr, false);
#endif
        setting_recovery_param = false;

        if (param->a2dp_sniffing_stopped.cause != (HCI_ERR | HCI_ERR_CONN_ROLESWAP))
        {
            app_bt_sniffing_state_machine(param->a2dp_sniffing_stopped.bd_addr,
                                          APP_BT_SNIFFING_EVENT_A2DP_SNIFFING_STOPPED);
        }
        break;

    case BT_EVENT_SCO_SNIFFING_STARTED:
        // recovery link connected
        APP_PRINT_TRACE1("app_bt_sniffing_event_cback: BT_EVENT_SCO_SNIFFING_STARTED, cause %x",
                         param->sco_sniffing_started.cause);

        if (param->sco_sniffing_started.cause == HCI_SUCCESS)
        {
            app_bt_sniffing_state_machine(param->sco_sniffing_started.bd_addr,
                                          APP_BT_SNIFFING_EVENT_SCO_SNIFFIG_STARTED);
        }
        else
        {
            if (app_cfg_nv.bud_role == REMOTE_SESSION_ROLE_PRIMARY)
            {
                bt_sniffing_link_disconnect(param->sco_sniffing_started.bd_addr);
                app_bt_sniffing_change_state(param->sco_sniffing_started.bd_addr, APP_BT_SNIFFING_STATE_STOPPING);
            }
        }
        break;

    case BT_EVENT_SCO_SNIFFING_STOPPED:
        // sco recovery link disconnected
        setting_recovery_param = false;
#ifdef APP_SNIFFING_ERWS2_0
        APP_PRINT_TRACE1("app_bt_sniffing_event_cback: BT_EVENT_SCO_SNIFFING_STOPPED %x",
                         param->a2dp_sniffing_stopped.cause);
        if (param->sco_sniffing_stopped.cause != (HCI_ERR | HCI_ERR_CONN_ROLESWAP))
        {
            app_bt_sniffing_state_machine(param->sco_sniffing_stopped.bd_addr,
                                          APP_BT_SNIFFING_EVENT_SCO_SNIFFING_STOPPED);
        }
#endif
        break;

    case BT_EVENT_REMOTE_ROLESWAP_STATUS:
        {
            T_BT_ROLESWAP_STATUS status;
            uint8_t app_idx = MAX_BR_LINK_NUM;
            uint16_t  cause;

            status = param->remote_roleswap_status.status;
            cause = param->remote_roleswap_status.cause;

            for (int i = 0; i < MAX_BR_LINK_NUM; i++)
            {
                if (app_check_b2s_link_by_id(i))
                {
                    app_idx = i;
                    break;
                }
            }

            APP_PRINT_INFO4("app_bt_sniffing_event_cback: roleswap status %x, cause %x, app_idx %d, bud_role %d",
                            status, cause, app_idx, app_cfg_nv.bud_role);

            if (status == BT_ROLESWAP_STATUS_SUCCESS)
            {
                app_cfg_nv.bud_role = param->remote_roleswap_status.device_role;

                app_bt_sniffing_param_set_pre_value(false);//reset pre htpoll when roleswap success

#if (F_APP_DONGLE_FEATURE_SUPPORT == 1)
                app_dongle_updata_mic_data_idx(true);
#endif

#if F_APP_QOL_MONITOR_SUPPORT
                if (app_cfg_nv.bud_role == REMOTE_SESSION_ROLE_SECONDARY)
                {
                    //new sec needn't start buffer level report timer or b2b rssi monitor
                    app_qol_disable_buffer_level_report_timer();
                    app_qol_link_monitor(app_cfg_nv.bud_peer_addr, false);
                }
                else
                {
                    //new pri needn't start periodic timer to check b2s rssi and need monitor b2b rssi
                    app_qol_disable_link_monitor_timer();
                }
#endif

                if (app_idx < MAX_BR_LINK_NUM)
                {
                    app_bt_sniffing_state_machine(app_db.br_link[app_idx].bd_addr, APP_BT_SNIFFING_EVENT_ROLESWAP_CMPL);

                    p_link = app_find_br_link(app_db.br_link[app_idx].bd_addr);

                    if (p_link)
                    {
                        if (app_cfg_nv.bud_role == REMOTE_SESSION_ROLE_PRIMARY)
                        {
                            if (p_link->bt_sniffing_state != APP_BT_SNIFFING_STATE_SNIFFING)
                            {
                                app_bt_policy_default_connect(p_link->bd_addr, DID_PROFILE_MASK, false);
                            }
#if F_APP_QOL_MONITOR_SUPPORT
                            else
                            {
                                if (p_link->sniffing_type == BT_SNIFFING_TYPE_A2DP)
                                {
                                    //new pri need start link monitor between b2b when roleswap cmpl in a2dp sniffing condition
                                    app_qol_link_monitor(app_cfg_nv.bud_peer_addr, true);
                                }
                            }
#endif
                        }
                    }
                }
            }

            else if (status == BT_ROLESWAP_STATUS_BDADDR)
            {
                uint8_t b2b_bd_addr[6];
                memcpy(b2b_bd_addr, param->remote_roleswap_status.u.bdaddr.pre_bd_addr, 6);

                p_link = app_find_br_link(b2b_bd_addr);
                if (p_link)
                {
                    p_link->acl_link_role = (T_BT_LINK_ROLE)param->remote_roleswap_status.u.bdaddr.curr_link_role;
                    memcpy(p_link->bd_addr, param->remote_roleswap_status.u.bdaddr.curr_bd_addr, 6);
                }

                /* update bud link's local and peer addresses */
                memcpy(app_cfg_nv.bud_local_addr, param->remote_roleswap_status.u.bdaddr.pre_bd_addr, 6);
                memcpy(app_cfg_nv.bud_peer_addr, param->remote_roleswap_status.u.bdaddr.curr_bd_addr, 6);

                ///TODO: need to add active b2s link info ???
                if (app_idx == MAX_BR_LINK_NUM)
                {
                    break;
                }
            }
            else if (status == BT_ROLESWAP_STATUS_TERMINATED)
            {
                if (app_idx == MAX_BR_LINK_NUM)
                {
                    break;
                }

                if (app_cfg_nv.bud_role == REMOTE_SESSION_ROLE_PRIMARY)
                {
                    app_bt_sniffing_state_machine(app_db.br_link[app_idx].bd_addr,
                                                  APP_BT_SNIFFING_EVENT_ROLESWAP_TERMINATED);
                }
            }
        }
        break;

    default:
        handle = false;
        break;
    }

    if (handle == true)
    {
        APP_PRINT_TRACE2("app_bt_sniffing_event_cback: event_type 0x%04x, bud_role %d", event_type,
                         app_cfg_nv.bud_role);
    }
}

#if (F_APP_DONGLE_FEATURE_SUPPORT == 1)
void app_bt_sniffing_bau_record_set_nack_num(void)
{
    uint8_t *bd_addr = NULL;
    uint8_t i = 0;
    uint8_t find_bau_app_idx = 0;

    //only one link now, TODO: support multilink case.
    for (i = 0; i < MAX_BR_LINK_NUM; i++)
    {
        if (app_check_b2s_link_by_id(i))
        {
            find_bau_app_idx = i;
            break;
        }
    }

    if (find_bau_app_idx == MAX_BR_LINK_NUM)
    {
        APP_PRINT_ERROR0("There is no bau link");
        return;
    }

    bd_addr = app_db.br_link[find_bau_app_idx].bd_addr;

    APP_PRINT_TRACE3("app_bt_sniffing_bau_record_set_nack_num: %s, %d, %d", TRACE_BDADDR(bd_addr),
                     app_db.gaming_mode, app_db.dongle_is_enable_mic);

    app_bt_sniffing_set_nack_flush_param(bd_addr);
}
#endif

bool app_bt_sniffing_set_low_latency(void)
{
    uint8_t *bd_addr = NULL;
    uint8_t app_idx;
    T_APP_BR_LINK *p_link;

    gap_stop_timer(&timer_handle_retry_low_latency_mode);

    ///TODO: check active link
    for (app_idx = 0; app_idx < MAX_BR_LINK_NUM; app_idx++)
    {
        if (app_check_b2s_link_by_id(app_idx))
        {
            bd_addr = app_db.br_link[app_idx].bd_addr;
            break;
        }
    }

    if (bd_addr == NULL)
    {
        return false;
    }

    p_link = app_find_br_link(bd_addr);
    if (!p_link)
    {
        return false;
    }

    if (p_link->bt_sniffing_state == APP_BT_SNIFFING_STATE_SNIFFING)
    {
        if (p_link->sniffing_type == BT_SNIFFING_TYPE_A2DP)
        {
            app_bt_sniffing_set_nack_flush_param(bd_addr);
        }
        if (remote_session_role_get() == REMOTE_SESSION_ROLE_PRIMARY)
        {
            app_bt_sniffing_param_update(APP_BT_SNIFFING_EVENT_LOW_LATENCY_CHANGE);
            app_bt_policy_qos_param_update(bd_addr, BP_TPOLL_LOW_LATENCY_EVENT);
        }
    }
    else if (p_link->bt_sniffing_state == APP_BT_SNIFFING_STATE_ROLESWAP ||
             p_link->bt_sniffing_state == APP_BT_SNIFFING_STATE_STOPPING ||
             p_link->bt_sniffing_state == APP_BT_SNIFFING_STATE_STARTING)
    {
        gap_start_timer(&timer_handle_retry_low_latency_mode,
                        "set_low_latency_mode",
                        bt_sniffing_timer_queue_id,
                        APP_BT_SNIFFING_TIMER_ID_SET_LOW_LAT_PARAM,
                        0,
                        SNIFFING_RETRY_TIMER_MS);
    }
    else
    {
        if (remote_session_role_get() != REMOTE_SESSION_ROLE_SECONDARY)
        {
            app_bt_policy_qos_param_update(bd_addr, BP_TPOLL_LOW_LATENCY_EVENT);
        }
    }

    return true;
}

static void app_bt_sniffing_timer_cback(uint8_t timer_id, uint16_t timer_chann)
{
    APP_PRINT_TRACE2("app_bt_sniffing_timer_cback: timer_id 0x%x, timer_chann 0x%x",
                     timer_id, timer_chann);

    switch (timer_id)
    {
    case APP_BT_SNIFFING_TIMER_ID_SET_LOW_LAT_PARAM:
        {
            gap_stop_timer(&timer_handle_retry_low_latency_mode);
            app_bt_sniffing_set_low_latency();
        }
        break;

    case APP_BT_SNIFFING_TIMER_ID_RETRY_SNIFFING:
        {
            uint8_t link_index = (uint8_t)timer_chann;
            T_APP_BR_LINK *p_link;
            p_link = &app_db.br_link[link_index];
            gap_stop_timer(&timer_handle_retry_sniffing);
            if (!app_bt_sniffing_check(p_link->bd_addr))
            {
                APP_PRINT_TRACE0("app_bt_sniffing_timer_cback: sniffing start again");

                if (!app_bt_sniffing_start(p_link->bd_addr, p_link->sniffing_type))
                {
                    APP_PRINT_ERROR0("app_bt_sniffing_timer_cback: app bt sniffing start error!");
                }
            }
        }
        break;

    case APP_BT_SNIFFING_TIMER_ID_PROTECTED_TIMER:
        {
            gap_stop_timer(&timer_handle_protected_timer);
            sniffing_protect_id = MAX_BR_LINK_NUM;
            uint8_t link_index = (uint8_t)timer_chann;
            T_APP_BR_LINK *p_link;
            p_link = &app_db.br_link[link_index];
#if F_APP_MUTILINK_VA_PREEMPTIVE
            sniffing_starting_a2dp_mute = false;
            sniffing_starting_a2dp_stop = false;
#endif
            if (p_link->bt_sniffing_state == APP_BT_SNIFFING_STATE_STARTING ||
                p_link->bt_sniffing_state == APP_BT_SNIFFING_STATE_STOPPING)
            {
                if (bt_sniffing_link_disconnect(p_link->bd_addr))
                {
                    app_bt_sniffing_change_state(p_link->bd_addr, APP_BT_SNIFFING_STATE_STOPPING);
                }
                else
                {
                    app_bt_sniffing_change_state(p_link->bd_addr, APP_BT_SNIFFING_STATE_READY);
                }
            }
        }
        break;

    case APP_BT_SNIFFING_TIMER_ID_RECONNECT_INACTIVE_LINK:
        {
            gap_stop_timer(&timer_handle_reconnect_inactive_link);

            if (app_cfg_nv.bud_role == REMOTE_SESSION_ROLE_PRIMARY)
            {
                uint32_t bond_flag;
                uint32_t plan_profs;

                bt_bond_flag_get(app_db.resume_addr, &bond_flag);
                if (bond_flag & (BOND_FLAG_HFP | BOND_FLAG_HSP | BOND_FLAG_A2DP))
                {
                    plan_profs = get_profs_by_bond_flag(bond_flag);
                    app_bt_policy_default_connect(app_db.resume_addr, plan_profs, false);
                }
            }
        }
        break;

    default:
        break;
    }
}

#if F_APP_QOL_MONITOR_SUPPORT
void app_bt_sniffing_handle_periodic_evt(T_APP_BT_SNIFFING_PERIODIC_EVT event)
{
    //not handle src go away if roleswap is ongoing
    if ((app_roleswap_ctrl_get_status() != APP_ROLESWAP_STATUS_IDLE))
    {
        return;
    }

    //rssi roleswap first if condition is met
    if (!app_db.gaming_mode)
    {
        if ((app_db.rssi_local + app_cfg_const.roleswap_rssi_threshold < app_db.rssi_remote) &&
            (app_db.rssi_remote >= -85) && (app_db.rssi_local >= -85) && (app_db.rssi_local <= -60))
        {
            return;
        }
    }

    switch (event)
    {
    case APP_BT_SNIFFING_PERIODIC_EVENT_RSSI_UPDATE:
        {
            if ((!app_db.gaming_mode) && (app_bt_policy_get_b2s_connected_num() > 1))
            {
                app_bt_sniffing_handle_src_away_multilink_enabled();
                return;
            }
        }
        break;

    case APP_BT_SNIFFING_PERIODIC_EVENT_LEVEL_UPDATE:
        {
            if ((!app_db.gaming_mode) && (app_bt_policy_get_b2s_connected_num() > 1))
            {
                app_bt_sniffing_handle_src_away_multilink_enabled();
                return;
            }
        }
        break;

    default:
        break;
    }

    APP_PRINT_TRACE1("app_bt_sniffing_handle_periodic_evt:event %u", event);
}

static void app_bt_sniffing_handle_src_away_multilink_enabled(void)
{
    if ((app_db.remote_session_state == REMOTE_SESSION_STATE_CONNECTED) &&
        (app_cfg_nv.bud_role == REMOTE_SESSION_ROLE_PRIMARY))
    {
        T_APP_BR_LINK *p_link = NULL;

        p_link = &app_db.br_link[app_get_active_a2dp_idx()];
        if (p_link && p_link->a2dp_track_handle)
        {
            audio_track_buffer_level_get(p_link->a2dp_track_handle, &app_db.buffer_level_local);
        }

        APP_PRINT_TRACE4("app_bt_sniffing_handle_src_away_multilink_enabled:rssi_local %d, rss_remote %d, buffer_level_local %d,"
                         "app_db.src_going_away_multilink_enabled %d", app_db.rssi_local, app_db.rssi_remote,
                         app_db.buffer_level_local,
                         app_db.src_going_away_multilink_enabled);

        if ((app_db.rssi_local <= RSSI_MINIMUM_THRESHOLD_TO_RECEIVE_PACKET_BAD) &&
            (app_db.rssi_remote <= RSSI_MINIMUM_THRESHOLD_TO_RECEIVE_PACKET_BAD))
        {
            //src goes away
            if (app_db.buffer_level_local < 100)
            {
                if (!app_db.src_going_away_multilink_enabled)
                {
                    app_db.src_going_away_multilink_enabled = true;
                    app_relay_async_single(APP_MODULE_TYPE_QOL, APP_REMOTE_MSG_SYNC_MULTI_SRC_AWAY);
                    app_bt_sniffing_param_update(APP_BT_SNIFFING_EVENT_MULTI_SRC_GOES_AWAY);
                }
            }
            else
            {
                //do nothing
                //If idle skip has been set, the idle skip need to be maintained.
                //If idle skip hasn't been set, the idle skip just need to be maintained as original value.
            }
        }
        else if ((app_db.rssi_local > RSSI_MINIMUM_THRESHOLD_TO_RECEIVE_PACKET_BAD) &&
                 (app_db.rssi_remote > RSSI_MINIMUM_THRESHOLD_TO_RECEIVE_PACKET_BAD) &&
                 app_db.src_going_away_multilink_enabled)
        {
            //src goes near
            app_db.src_going_away_multilink_enabled = false;
            app_relay_async_single(APP_MODULE_TYPE_QOL, APP_REMOTE_MSG_SYNC_MULTI_SRC_AWAY);
            app_bt_sniffing_param_update(APP_BT_SNIFFING_EVENT_MULTI_SRC_GOES_NEAR);
        }
    }
}
#endif

void app_bt_sniffing_param_update(T_APP_BT_SNIFFING_CFG_EVT event)
{
    T_APP_BR_LINK *p_link = NULL;
    T_APP_BT_SNIFFING_PARAM param;

    p_link = &app_db.br_link[app_get_active_a2dp_idx()];

    if (p_link && (app_cfg_nv.bud_role == REMOTE_SESSION_ROLE_PRIMARY) &&
        (app_db.remote_session_state == REMOTE_SESSION_STATE_CONNECTED))
    {
        if ((event != APP_BT_SNIFFING_EVENT_ACL_SNIFFING_LINK_CMPL_MULTILINK_PREEMPTIVE) &&
            (event != APP_BT_SNIFFING_EVENT_ACL_SNIFFING_LINK_CMPL_NORMAL))
        {
            if (!((p_link->sniffing_type == BT_SNIFFING_TYPE_A2DP) &&
                  (APP_BT_SNIFFING_STATE_SNIFFING == p_link->bt_sniffing_state)))
            {
                APP_PRINT_TRACE0("app_bt_sniffing_param_update: a2dp recovery link hasn't been set up, not set htpoll param");
                return;
            }
        }

        //When bud is paging or a2dp preemping, the param of normal mode and gaming mode is set as the same value.
        if (app_bt_policy_get_state() == BP_STATE_LINKBACK)
        {
            //page
            recovery_link_a2dp_interval = app_cfg_const.recovery_link_a2dp_interval;

            if (p_link->a2dp_codec_type == BT_A2DP_CODEC_TYPE_AAC)
            {
                recovery_link_a2dp_flush_timeout = A2DP_FLUSH_TIME_AAC;
            }
            else if (p_link->a2dp_codec_type == BT_A2DP_CODEC_TYPE_SBC)
            {
                recovery_link_a2dp_flush_timeout = A2DP_FLUSH_TIME_SBC;
            }

            recovery_link_a2dp_rsvd_slot = app_cfg_const.recovery_link_a2dp_rsvd_slot;
            recovery_link_a2dp_idle_slot = htpoll[APP_BT_SNIFFING_LINKBACK].idle_slot;
            recovery_link_a2dp_idle_skip = htpoll[APP_BT_SNIFFING_LINKBACK].idle_skip;
        }
        else if (app_db.a2dp_preemptive_flag)
        {
            //a2dp preemptive
            recovery_link_a2dp_interval = A2DP_INTERVAL_MULTILINK_PREEMPTIVE;

            if (p_link->a2dp_codec_type == BT_A2DP_CODEC_TYPE_AAC)
            {
                recovery_link_a2dp_flush_timeout = A2DP_FLUSH_TIME_AAC;
            }
            else if (p_link->a2dp_codec_type == BT_A2DP_CODEC_TYPE_SBC)
            {
                recovery_link_a2dp_flush_timeout = A2DP_FLUSH_TIME_SBC;
            }

            recovery_link_a2dp_rsvd_slot = app_cfg_const.recovery_link_a2dp_rsvd_slot;
            recovery_link_a2dp_idle_slot = htpoll[APP_BT_SNIFFING_PREEMP].idle_slot;
            recovery_link_a2dp_idle_skip = htpoll[APP_BT_SNIFFING_PREEMP].idle_skip;
        }
#ifdef APP_SNIFFING_ERWS2_0
        else
        {
            recovery_link_a2dp_interval = app_cfg_const.recovery_link_a2dp_interval;
            recovery_link_a2dp_rsvd_slot = app_cfg_const.recovery_link_a2dp_rsvd_slot;

            if (app_db.gaming_mode)
            {
                if (app_db.remote_is_8753bau)
                {
                    recovery_link_a2dp_interval = A2DP_INTERVAL_ULTRA_LOW_LATENCY;
                    recovery_link_a2dp_flush_timeout = A2DP_FLUSH_TIME_ULTRA_LOW_LATENCY;
                    recovery_link_a2dp_rsvd_slot = A2DP_CTRL_RSVD_SLOT_ULTRA_LOW_LATENCY;
                }
                else
                {
                    recovery_link_a2dp_flush_timeout = app_cfg_const.recovery_link_a2dp_flush_timeout_gaming_mode;
                }
            }
            else
            {
                if (p_link->a2dp_codec_type == BT_A2DP_CODEC_TYPE_AAC)
                {
                    recovery_link_a2dp_flush_timeout = A2DP_FLUSH_TIME_AAC;
                }
                else if (p_link->a2dp_codec_type == BT_A2DP_CODEC_TYPE_SBC)
                {
                    recovery_link_a2dp_flush_timeout = A2DP_FLUSH_TIME_SBC;
                }
            }

            if (app_ble_device_is_active())
            {
                recovery_link_a2dp_idle_slot = htpoll[APP_BT_SNIFFING_NORMAL_BLE_ACTIVE].idle_slot;
                recovery_link_a2dp_idle_skip = htpoll[APP_BT_SNIFFING_NORMAL_BLE_ACTIVE].idle_skip;
            }
            else if (ble_ext_adv_is_ongoing())
            {
                recovery_link_a2dp_idle_slot = htpoll[APP_BT_SNIFFING_NORMAL_BLE_ADV_ONGOING].idle_slot;
                recovery_link_a2dp_idle_skip = htpoll[APP_BT_SNIFFING_NORMAL_BLE_ADV_ONGOING].idle_skip;
            }
            else if (app_get_ble_link_num() != 0)
            {
                //ble link connected
                recovery_link_a2dp_idle_slot = htpoll[APP_BT_SNIFFING_NORMAL_BLE_CONNECTED].idle_slot;
                recovery_link_a2dp_idle_skip = htpoll[APP_BT_SNIFFING_NORMAL_BLE_CONNECTED].idle_skip;
            }
            else if (app_cfg_const.enable_multi_link)
            {
                //no ble adv or ble link with multilink enabled => set idle slot & idle skip as 6,2
                recovery_link_a2dp_idle_slot = htpoll[APP_BT_SNIFFING_NORMAL_MULTILINK_ENABLED].idle_slot;
                recovery_link_a2dp_idle_skip = htpoll[APP_BT_SNIFFING_NORMAL_MULTILINK_ENABLED].idle_skip;
            }
#if GFPS_SASS_SUPPORT
            else if (app_sass_policy_support_easy_connection_switch())
            {
                //support easy connection switch same as multilink enable => set idle slot & idle skip as 6,2
                recovery_link_a2dp_idle_slot = htpoll[APP_BT_SNIFFING_NORMAL_MULTILINK_ENABLED].idle_slot;
                recovery_link_a2dp_idle_skip = htpoll[APP_BT_SNIFFING_NORMAL_MULTILINK_ENABLED].idle_skip;
            }
#endif
#if  ISOC_AUDIO_SUPPORT
            else if (event == APP_BT_SNIFFING_EVENT_ISO_SUSPEND)
            {
                recovery_link_a2dp_idle_slot = 0x0C;
                recovery_link_a2dp_idle_skip = 0;
            }
#endif
            else
            {
                //no ble with multilink disabled
                recovery_link_a2dp_idle_slot = htpoll[APP_BT_SNIFFING_NO_BLE_MULTILINK_DISABLED].idle_slot;
                recovery_link_a2dp_idle_skip = htpoll[APP_BT_SNIFFING_NO_BLE_MULTILINK_DISABLED].idle_skip;
            }
        }
#else
        else if (app_db.gaming_mode)
        {
            //gaming mode
            if (app_db.remote_is_8753bau)
            {
                recovery_link_a2dp_interval = A2DP_INTERVAL_ULTRA_LOW_LATENCY;
                recovery_link_a2dp_flush_timeout = A2DP_FLUSH_TIME_ULTRA_LOW_LATENCY;
                recovery_link_a2dp_rsvd_slot = A2DP_CTRL_RSVD_SLOT_ULTRA_LOW_LATENCY;
            }
            else
            {
                recovery_link_a2dp_interval = app_cfg_const.recovery_link_a2dp_interval_gaming_mode;
                recovery_link_a2dp_flush_timeout = app_cfg_const.recovery_link_a2dp_flush_timeout_gaming_mode;
                recovery_link_a2dp_rsvd_slot = app_cfg_const.recovery_link_a2dp_rsvd_slot_gaming_mode;
            }

            if (app_cfg_const.enable_multi_link)
            {
                if (ble_ext_adv_is_ongoing())
                {
                    //ble adv
                    recovery_link_a2dp_idle_slot = htpoll[APP_BT_SNIFFING_GAMING_BLE_ADV_ONGOING].idle_slot;
                    recovery_link_a2dp_idle_skip = htpoll[APP_BT_SNIFFING_GAMING_BLE_ADV_ONGOING].idle_skip;
                }
                else if (app_get_ble_encrypted_link_num() != 0)
                {
                    //ble link encrypted with multilink enabled, follow the value of multilink enabled
                    recovery_link_a2dp_idle_slot = htpoll[APP_BT_SNIFFING_GAMING_MULTILINK_ENABLED].idle_slot;
                    recovery_link_a2dp_idle_skip = htpoll[APP_BT_SNIFFING_GAMING_MULTILINK_ENABLED].idle_skip;
                }
                else if (app_get_ble_link_num() != 0)
                {
                    //ble link connected but not encrypted with multilink enabled
                    recovery_link_a2dp_idle_slot = htpoll[APP_BT_SNIFFING_GAMING_BLE_CONNECTED].idle_slot;
                    recovery_link_a2dp_idle_skip = htpoll[APP_BT_SNIFFING_GAMING_BLE_CONNECTED].idle_skip;
                }
                else
                {
                    //no ble with multilink enabled
                    recovery_link_a2dp_idle_slot = htpoll[APP_BT_SNIFFING_GAMING_MULTILINK_ENABLED].idle_slot;
                    recovery_link_a2dp_idle_skip = htpoll[APP_BT_SNIFFING_GAMING_MULTILINK_ENABLED].idle_skip;
                }
            }
            else
            {
                //disable multilink
                if (ble_ext_adv_is_ongoing())
                {
                    recovery_link_a2dp_idle_slot = htpoll[APP_BT_SNIFFING_GAMING_BLE_ADV_ONGOING].idle_slot;
                    recovery_link_a2dp_idle_skip = htpoll[APP_BT_SNIFFING_GAMING_BLE_ADV_ONGOING].idle_skip;
                }
                else if (app_get_ble_encrypted_link_num() != 0)
                {
                    //le link encrypted with multilink disabled
                    recovery_link_a2dp_idle_slot = htpoll[APP_BT_SNIFFING_GAMING_BLE_ENCRYPTED].idle_slot;
                    recovery_link_a2dp_idle_skip = htpoll[APP_BT_SNIFFING_GAMING_BLE_ENCRYPTED].idle_skip;
                }
                else if (app_get_ble_link_num() != 0)
                {
                    //1 legacy link with le link connected but not encrypted
                    recovery_link_a2dp_idle_slot = htpoll[APP_BT_SNIFFING_GAMING_BLE_CONNECTED].idle_slot;
                    recovery_link_a2dp_idle_skip = htpoll[APP_BT_SNIFFING_GAMING_BLE_CONNECTED].idle_skip;
                }
                else
                {
                    //no ble and disable multilink
                    recovery_link_a2dp_idle_slot = htpoll[APP_BT_SNIFFING_NO_BLE_MULTILINK_DISABLED].idle_slot;
                    recovery_link_a2dp_idle_skip = htpoll[APP_BT_SNIFFING_NO_BLE_MULTILINK_DISABLED].idle_skip;
                }
            }
        }
        else
        {
            //normal mode
            recovery_link_a2dp_interval = app_cfg_const.recovery_link_a2dp_interval;
            if (p_link->a2dp_codec_type == BT_A2DP_CODEC_TYPE_AAC)
            {
                recovery_link_a2dp_flush_timeout = A2DP_FLUSH_TIME_AAC;
            }
            else if (p_link->a2dp_codec_type == BT_A2DP_CODEC_TYPE_SBC)
            {
                recovery_link_a2dp_flush_timeout = A2DP_FLUSH_TIME_SBC;
            }

            recovery_link_a2dp_rsvd_slot = app_cfg_const.recovery_link_a2dp_rsvd_slot;

            if (app_ble_device_is_active())
            {
                recovery_link_a2dp_idle_slot = htpoll[APP_BT_SNIFFING_NORMAL_BLE_ACTIVE].idle_slot;
                recovery_link_a2dp_idle_skip = htpoll[APP_BT_SNIFFING_NORMAL_BLE_ACTIVE].idle_skip;
            }
            else if (ble_ext_adv_is_ongoing())
            {
                recovery_link_a2dp_idle_slot = htpoll[APP_BT_SNIFFING_NORMAL_BLE_ADV_ONGOING].idle_slot;
                recovery_link_a2dp_idle_skip = htpoll[APP_BT_SNIFFING_NORMAL_BLE_ADV_ONGOING].idle_skip;
            }
            else if (app_get_ble_link_num() != 0)
            {
                //ble link connected
                recovery_link_a2dp_idle_slot = htpoll[APP_BT_SNIFFING_NORMAL_BLE_CONNECTED].idle_slot;
                recovery_link_a2dp_idle_skip = htpoll[APP_BT_SNIFFING_NORMAL_BLE_CONNECTED].idle_skip;
            }
            else if (app_cfg_const.enable_multi_link)
            {
                //no ble adv or ble link with multilink enabled => set idle slot & idle skip as 6,2
                recovery_link_a2dp_idle_slot = htpoll[APP_BT_SNIFFING_NORMAL_MULTILINK_ENABLED].idle_slot;
                recovery_link_a2dp_idle_skip = htpoll[APP_BT_SNIFFING_NORMAL_MULTILINK_ENABLED].idle_skip;
            }
#if  ISOC_AUDIO_SUPPORT
            else if (event == APP_BT_SNIFFING_EVENT_ISO_SUSPEND)
            {
                recovery_link_a2dp_idle_slot = 0x0A;
                recovery_link_a2dp_idle_skip = 0;
                return;
            }
#endif
            else
            {
                //no ble with multilink disabled
                recovery_link_a2dp_idle_slot = htpoll[APP_BT_SNIFFING_NO_BLE_MULTILINK_DISABLED].idle_slot;
                recovery_link_a2dp_idle_skip = htpoll[APP_BT_SNIFFING_NO_BLE_MULTILINK_DISABLED].idle_skip;
            }
        }
#endif

#if F_APP_QOL_MONITOR_SUPPORT
        if (event == APP_BT_SNIFFING_EVENT_MULTI_SRC_GOES_AWAY)
        {
            recovery_link_a2dp_idle_skip = A2DP_IDLE_SKIP_PERIOD_MULTILINK_SRC_AWAY;
        }
#endif

        param.a2dp_interval = recovery_link_a2dp_interval;
        param.flush_timeout = recovery_link_a2dp_flush_timeout;
        param.rsvd_slot = recovery_link_a2dp_rsvd_slot;
        param.idle_slot = recovery_link_a2dp_idle_slot;
        param.idle_skip = recovery_link_a2dp_idle_skip;
        param.quick_flush = recovery_link_quick_flush;
        param.nack_num = recovery_link_nack_num;

        app_relay_async_single_with_raw_msg(APP_MODULE_TYPE_AUDIO_POLICY,
                                            APP_REMOTE_MSG_AUDIO_CFG_PARAM_SYNC,
                                            (uint8_t *)&param, sizeof(T_APP_BT_SNIFFING_PARAM));

        APP_PRINT_TRACE7("app_bt_sniffing_param_update:event %u,interval %x,flush_timeout %x,rsvd_slot %x,idle_slot %x,idle_skip %x, recover %d",
                         event, recovery_link_a2dp_interval, recovery_link_a2dp_flush_timeout, recovery_link_a2dp_rsvd_slot,
                         recovery_link_a2dp_idle_slot, recovery_link_a2dp_idle_skip, app_db.recover_param);


        if ((event != APP_BT_SNIFFING_EVENT_ACL_SNIFFING_LINK_CMPL_MULTILINK_PREEMPTIVE) &&
            (event != APP_BT_SNIFFING_EVENT_ACL_SNIFFING_LINK_CMPL_NORMAL) &&
            (app_db.recover_param == false))
        {
            app_bt_sniffing_audio_cfg(p_link->bd_addr, recovery_link_a2dp_interval,
                                      recovery_link_a2dp_flush_timeout,
                                      recovery_link_a2dp_rsvd_slot, recovery_link_a2dp_idle_slot, recovery_link_a2dp_idle_skip);
        }
    }
}

void app_bt_sniffing_sec_set_audio_cfg_param(T_APP_BT_SNIFFING_PARAM *p_msg)
{
    recovery_link_a2dp_interval = p_msg->a2dp_interval;
    recovery_link_a2dp_flush_timeout = p_msg->flush_timeout;
    recovery_link_a2dp_rsvd_slot = p_msg->rsvd_slot;
    recovery_link_a2dp_idle_slot = p_msg->idle_slot;
    recovery_link_a2dp_idle_skip = p_msg->idle_skip;
    recovery_link_quick_flush = p_msg->quick_flush;
    recovery_link_nack_num = p_msg->nack_num;

    app_bt_sniffing_param_set_pre_value(true);
}

void app_bt_sniffing_judge_dynamic_set_gaming_mode(void)
{
    APP_PRINT_TRACE1("app_bt_sniffing_judge_dynamic_set_gaming_mode gaming_mode %d",
                     app_db.gaming_mode);
    T_APP_BR_LINK *p_link = &app_db.br_link[app_get_active_a2dp_idx()];

    if ((app_db.remote_session_state == REMOTE_SESSION_STATE_CONNECTED) &&
        (app_cfg_nv.bud_role == REMOTE_SESSION_ROLE_PRIMARY))
    {
        if ((p_link->bt_sniffing_state == APP_BT_SNIFFING_STATE_SNIFFING) &&
            (p_link->sniffing_type == BT_SNIFFING_TYPE_A2DP))
        {
            //gaming mode has an effect only when a2dp sniffing started
            if ((app_bt_policy_get_state() == BP_STATE_LINKBACK) || app_db.a2dp_preemptive_flag)
            {
                if (app_db.gaming_mode)
                {
                    app_relay_async_single(APP_MODULE_TYPE_AUDIO_POLICY, APP_REMOTE_MSG_GAMING_MODE_SET_OFF);
                }
                else
                {
#if (F_APP_AVP_INIT_SUPPORT == 1)
                    if (p_link->remote_device_vendor_id == APP_REMOTE_DEVICE_IOS && app_cfg_nv.id_is_display)
                    {
                        app_relay_async_single(APP_MODULE_TYPE_AUDIO_POLICY, APP_REMOTE_MSG_GAMING_MODE_SET_OFF);
                    }
#endif
                }
            }
            else
            {
#if F_APP_QOL_MONITOR_SUPPORT
                if (!app_db.sec_going_away)
                {
                    //Whether to set gaming mode on is judged when a2dp sniffing started but sec is in away state.
                    app_relay_async_single(APP_MODULE_TYPE_AUDIO_POLICY, APP_REMOTE_MSG_GAMING_MODE_SET_ON);
                }
#else
                app_relay_async_single(APP_MODULE_TYPE_AUDIO_POLICY, APP_REMOTE_MSG_GAMING_MODE_SET_ON);
#endif
            }
        }
    }
}

void app_bt_sniffing_init(void)
{
    bt_mgr_cback_register(app_bt_sniffing_event_cback);
    gap_reg_timer_cb(app_bt_sniffing_timer_cback, &bt_sniffing_timer_queue_id);
    recovery_link_a2dp_interval = app_cfg_const.recovery_link_a2dp_interval;
    //just initial value, it will be set according to codec type when set up recovery link
    recovery_link_a2dp_flush_timeout = A2DP_FLUSH_TIME_AAC;
    recovery_link_a2dp_rsvd_slot = app_cfg_const.recovery_link_a2dp_rsvd_slot;
    APP_PRINT_TRACE6("app_bt_sniffing_init recovery param : normal 0x%x 0x%x 0x%x gaming 0x%x 0x%x 0x%x",
                     app_cfg_const.recovery_link_a2dp_interval,
                     recovery_link_a2dp_flush_timeout,
                     app_cfg_const.recovery_link_a2dp_rsvd_slot,
                     app_cfg_const.recovery_link_a2dp_interval_gaming_mode,
                     app_cfg_const.recovery_link_a2dp_flush_timeout_gaming_mode,
                     app_cfg_const.recovery_link_a2dp_rsvd_slot_gaming_mode);

#if F_APP_QOL_MONITOR_SUPPORT
    //qol module is connected with sniffing module
    app_qol_init();
#endif
}


#else
void app_bt_sniffing_init(void)
{
    return;
}

bool app_bt_sniffing_roleswap(bool stop_after_shadow)
{
    return false;
}

bool app_bt_sniffing_start(uint8_t *bd_addr, T_BT_SNIFFING_TYPE type)
{
    return false;
}

void app_bt_sniffing_stop(uint8_t *bd_addr, T_BT_SNIFFING_TYPE type)
{
    return;
}

void app_bt_sniffing_process(uint8_t *bd_addr)
{
    return;
}

#if (F_APP_DONGLE_FEATURE_SUPPORT == 1)
void app_bt_sniffing_bau_record_set_nack_num(void)
{
    return;
}
#endif

bool app_bt_sniffing_set_low_latency(void)
{
    return false;
}

void app_bt_sniffing_hfp_connect(uint8_t *bd_addr)
{
    bt_hfp_audio_connect_req(bd_addr);
    return;
}

void app_bt_sniffing_audio_cfg(uint8_t *bd_addr, uint16_t interval, uint16_t flush_tout,
                               uint8_t rsvd_slot, uint8_t idle_slot, uint8_t idle_skip)
{
    return;
}

void app_bt_sniffing_param_update(T_APP_BT_SNIFFING_CFG_EVT event)
{
    return;
}

#if F_APP_QOL_MONITOR_SUPPORT
void app_bt_sniffing_handle_periodic_evt(T_APP_BT_SNIFFING_PERIODIC_EVT event)
{
    return;
}
#endif

#endif
