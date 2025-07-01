/*
 * Copyright (c) 2018, Realsil Semiconductor Corporation. All rights reserved.
 */
#include <string.h>
#include <stdlib.h>
#include "trace.h"
#include "os_sched.h"
#include "stdlib.h"
#include "gap_timer.h"
#include "gap_legacy.h"
#include "gap_le.h"
#include "bt_avrcp.h"
#include "bt_spp.h"
#include "bt_bond.h"
#include "bt_rdtp.h"
#include "bt_hfp.h"
#include "sysm.h"
#include "anc_tuning.h"
#include "engage.h"
#include "app_cfg.h"
#include "app_main.h"
#include "app_relay.h"
#include "app_bt_policy_api.h"
#include "app_bt_policy_int.h"
#include "app_sniff_mode.h"
#include "app_ble_gap.h"
#include "app_hfp.h"
#include "app_linkback.h"
#include "app_multilink.h"
#include "app_roleswap.h"
#include "app_audio_policy.h"
#include "app_mmi.h"
#include "app_relay.h"
#include "app_bond.h"
#include "app_auto_power_off.h"
#include "app_cmd.h"
#include "app_adv_stop_cause.h"

#if F_APP_HARMAN_FEATURE_SUPPORT
#include "app_harman_ble.h"
#endif

#if GFPS_SASS_SUPPORT
#include "app_sass_policy.h"
#endif

#if F_APP_NFC_SUPPORT
#include "app_nfc.h"
#endif

#include "app_link_util.h"
#if XM_XIAOAI_FEATURE_SUPPORT
#include "app_xiaoai_transport.h"
#endif

#if F_APP_XIAOWEI_FEATURE_SUPPORT
#include "app_xiaowei_transport.h"
#endif

#if F_APP_ANC_SUPPORT
#include "app_anc.h"
#endif

#if F_APP_LISTENING_MODE_SUPPORT
#include "app_listening_mode.h"
#endif

#if F_APP_LINEIN_SUPPORT
#include "app_line_in.h"
#endif

#include "audio_type.h"
#include "app_adp.h"
#include "app_sniff_mode.h"
#include "app_sdp.h"
#include "app_hfp.h"
#include "app_ota.h"
#if F_APP_TEAMS_FEATURE_SUPPORT
#include "app_asp_device.h"
#if F_APP_TEAMS_BT_POLICY
#include "app_teams_bt_policy.h"
#endif
#endif
#if F_APP_SINGLE_MUTLILINK_SCENERIO_1
#include "app_teams_audio_policy.h"
#include "app_teams_cmd.h"
#include "os_mem.h"
#endif

#if F_APP_BLE_SWIFT_PAIR_SUPPORT
#include "app_ble_swift_pair.h"
#endif

#if (F_APP_DONGLE_FEATURE_SUPPORT == 1)
#include "app_dongle_spp.h"
#include "app_dongle_record.h"
#endif

#if F_APP_QOL_MONITOR_SUPPORT
#include "app_qol.h"
#endif

#if F_APP_USB_AUDIO_SUPPORT
#include "app_usb_audio.h"
#endif

#if F_APP_LE_AUDIO_SM
#include "app_le_audio_mgr.h"
#endif

#if F_APP_AVP_INIT_SUPPORT
#include "app_avp.h"
#endif

#if F_APP_HARMAN_FEATURE_SUPPORT
#include "app_harman_vendor_cmd.h"
extern void *timer_handle_harman_power_off_option;
#endif

#if HARMAN_DISCONN_ACTIVE_A2DP_WHEN_OTA
#include "app_harman_ble_ota.h"
#endif

#define ROLE_SWITCH_COUNT_MAX                 (5)
#define ROLE_SWITCH_DELAY_MS                  (250)
#define ROLE_SWITCH_WINDOW_DELAY_MS           (2000)

#define NORMAL_PAGESCAN_INTERVAL              (0x800)
#define NORMAL_PAGESCAN_WINDOW                (0x12)

#define PRI_FAST_PAGESCAN_INTERVAL            (0x200)
#define PRI_FAST_PAGESCAN_WINDOW              (0x12)

#define PRI_VERY_FAST_PAGESCAN_INTERVAL       (0x40)
#define PRI_VERY_FAST_PAGESCAN_WINDOW         (0x12)

#define SEC_FAST_PAGESCAN_INTERVAL            (0x300)
#define SEC_FAST_PAGESCAN_WINDOW              (0x90)

#define PRIMARY_BLE_ADV_INTERVAL_VOICE        (0x20)
#define PRIMARY_BLE_ADV_INTERVAL_AUDIO        (0xa0)
#define PRIMARY_BLE_ADV_INTERVAL_NORMAL       (0xd0)

#define SECONDARY_BLE_ADV_INTERVAL_NORMAL     (0xb0)

#define PRIMARY_WAIT_COUPLING_TIMEOUT_MS      (500)

#define SRC_CONN_IND_MAX_NUM                  (2)
#define SRC_CONN_IND_DELAY_MS                 (1500)

#define SHUT_DOWN_STEP_TIMER_MS               (100)
#define DISCONN_B2S_PROFILE_WAIT_TIMES        (4) /* wait time(ms): 4 * SHUT_DOWN_STEP_TIMER_MS */
#define DISCONN_B2S_LINK_WAIT_TIMES_MAX       (4) /* max wait time(ms): 4 * SHUT_DOWN_STEP_TIMER_MS */
#define DISCONN_B2B_LINK_WAIT_TIMES_MAX       (3) /* max wait time(ms): 3 * SHUT_DOWN_STEP_TIMER_MS */

#define GOLDEN_RANGE_B2B_MAX                  (-45)
#define GOLDEN_RANGE_B2B_MIN                  (-55)
#define GOLDEN_RANGE_B2S_MAX                  (10)
#define GOLDEN_RANGE_B2S_MIN                  (0)

extern T_LINKBACK_ACTIVE_NODE linkback_active_node;

#if (F_APP_AVP_INIT_SUPPORT == 1)
void (*app_roleswap_src_connect_delay_hook)(void) = NULL;
#endif

T_STATE cur_state = STATE_SHUTDOWN;
T_BP_STATE bp_state = BP_STATE_IDLE;
T_BP_STATE pri_bp_state = BP_STATE_IDLE;
T_EVENT cur_event;

T_BT_DEVICE_MODE radio_mode = BT_DEVICE_MODE_IDLE;

static bool engage_done = false;
bool b2b_connected = false;
T_B2S_CONNECTED b2s_connected = {0};
bool first_connect_sync_default_volume_to_src = false;

static bool dedicated_enter_pairing_mode_flag = false;
static bool is_visiable_flag = false;
static bool is_force_flag = false;
static bool startup_linkback_done_flag = false;
static bool is_pairing_timeout = false;
static bool bond_list_load_flag = false;
static bool linkback_flag = false;
static bool is_user_action_pairing_mode = false;
static uint16_t last_src_conn_idx = 0;
static bool disconnect_for_pairing_mode = false;
static uint16_t linkback_retry_timeout = 0;
static bool after_stop_sdp_todo_linkback_run_flag = false;
static bool discoverable_when_one_link = false;
static bool is_src_authed = false;
static bool roleswap_suc_flag = false;

static uint8_t timer_queue_id = 0;
static void *timer_handle_shutdown_step = NULL;
static void *timer_handle_first_engage_check = NULL;
static void *timer_handle_pairing_mode = NULL;
static void *timer_handle_discoverable = NULL;
static void *timer_handle_bud_linklost = NULL;
static void *timer_handle_linkback = NULL;
static void *timer_handle_linkback_delay = NULL;
static void *timer_handle_wait_coupling = NULL;
static void *timer_handle_engage_action_adjust = NULL;
static void *timer_handle_page_scan_param = NULL;
static void *timer_handle_reconnect = NULL;
static void *timer_handle_b2s_conn = NULL;

static uint8_t original_bud_role = 0;
static bool rws_link_lost = false;

static T_SRC_CONN_IND src_conn_ind[SRC_CONN_IND_MAX_NUM];

static T_BP_IND_FUN ind_fun = NULL;

static T_SHUTDOWN_STEP shutdown_step;
static uint8_t shutdown_step_retry_cnt;

static uint8_t old_peer_factory_addr[6];

static void engage_sched(void);
static void linkback_sched(void);
static void stable_sched(T_STABLE_ENTER_MODE mode);
static void linkback_run(void);
static void engage_ind(T_ENGAGE_IND ind);
static void disconnect_all_event_handle(void);
static bool judge_dedicated_enter_pairing_mode(void);
static void prepare_for_dedicated_enter_pairing_mode(void);
static void timer_cback(uint8_t timer_id, uint16_t timer_chann);
static void stop_all_active_action(void);
static void save_engage_related_nv(void);

#if F_APP_ERWS_SUPPORT
static void new_pri_apply_relay_info_when_roleswap_suc(void);
static T_BP_ROLESWAP_INFO bp_roleswap_info_temp = {0};
#endif


#if (F_APP_OTA_TOOLING_SUPPORT == 1)
static bool validate_bd_addr_with_dongle_id(uint8_t dongle_id, uint8_t *bd_addr);
static const uint32_t ota_dongle_addr[31] =
{
    0x56238821, 0x56050BC7, 0x56EBF7D2, 0x56F9BCF5, 0x56B31083, 0x5647927F, 0x566B389B, 0x560933ED,
    0x5629C8AC, 0x563A50E8, 0x568805F5, 0x56e31104, 0x564fc1c6, 0x5615a63f, 0x5678f37b, 0x56DF5FA8,
    0x5601535b, 0x564a7230, 0x5681941b, 0x565d1d94, 0x56570017, 0x56af0473, 0x563fc9dc, 0x5671418d,
    0x56247bf2, 0x5612f939, 0x56D5E2F9, 0x56B89C33, 0x56D90599, 0x56AA34d4, 0x5690f14d
};
#endif

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#if F_APP_TEAMS_FEATURE_SUPPORT
uint8_t conn_to_new_device;
#endif

static void connected(void)
{
    uint32_t i;

    ENGAGE_PRINT_TRACE3("connected: bud_role %d, b2b %d, b2s %d",
                        app_cfg_nv.bud_role, b2b_connected, b2s_connected.num);

    for (i = 0; i < MAX_BR_LINK_NUM; i++)
    {
        if (app_check_b2s_link_by_id(i))
        {
            ENGAGE_PRINT_TRACE2("connected: b2s, bd_addr %s, profs 0x%08x",
                                TRACE_BDADDR(app_db.br_link[i].bd_addr),
                                app_db.br_link[i].connected_profile);
        }

        if (app_check_b2b_link_by_id(i))
        {
            ENGAGE_PRINT_TRACE2("connected: b2b, bd_addr %s, profs 0x%08x",
                                TRACE_BDADDR(app_db.br_link[i].bd_addr),
                                app_db.br_link[i].connected_profile);
        }
    }
}

static void new_state(T_STATE state)
{
    ENGAGE_PRINT_TRACE1("new_state: state 0x%02x", state);
}

static void event_info(T_EVENT event)
{
    ENGAGE_PRINT_TRACE1("event_info: event 0x%02x", event);
}

static void new_radio_mode(T_BT_DEVICE_MODE mode)
{
    ENGAGE_PRINT_TRACE1("new_radio_mode: radio_mode 0x%02x", mode);
}

static void stable_enter_mode(uint8_t mode)
{
    ENGAGE_PRINT_TRACE1("stable_enter_mode: mode 0x%02x", mode);
}

static void shutdown_step_info(uint8_t step)
{
    ENGAGE_PRINT_TRACE1("shutdown_step_state: step 0x%02x", step);
}

#if F_APP_ERWS_SUPPORT
static void new_pri_bp_state(uint8_t state)
{
    ENGAGE_PRINT_TRACE1("new_pri_bp_state: state 0x%02x", state);
}
#endif

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void app_bt_policy_b2s_conn_start_timer(void)
{
    gap_start_timer(&timer_handle_b2s_conn,
                    "b2s_connect_timer",
                    timer_queue_id,
                    TIMER_ID_B2S_CONN_TIMER_MS,
                    0,
                    APP_BT_POLICY_B2S_CONN_TIMER_MS);
}

uint32_t get_profs_by_bond_flag(uint32_t bond_flag)
{
    uint32_t profs = 0;

    if ((T_LINKBACK_SCENARIO)app_cfg_const.link_scenario == LINKBACK_SCENARIO_HFP_BASE)
    {
        if (bond_flag & BOND_FLAG_HFP)
        {
            profs |= HFP_PROFILE_MASK;

            if (app_cfg_const.supported_profile_mask & PBAP_PROFILE_MASK)
            {
                profs |= PBAP_PROFILE_MASK;
            }
        }
        else if (bond_flag & BOND_FLAG_HSP)
        {
            profs |= HSP_PROFILE_MASK;

            if (app_cfg_const.supported_profile_mask & PBAP_PROFILE_MASK)
            {
                profs |= PBAP_PROFILE_MASK;
            }
        }

        if (bond_flag & BOND_FLAG_SPP)
        {
            if (app_cfg_const.supported_profile_mask & SPP_PROFILE_MASK)
            {
                profs |= SPP_PROFILE_MASK;
            }
        }
        else if (bond_flag & BOND_FLAG_IAP)
        {
            if (app_cfg_const.supported_profile_mask & IAP_PROFILE_MASK)
            {
                profs |= IAP_PROFILE_MASK;
            }
        }
    }
    else if ((T_LINKBACK_SCENARIO)app_cfg_const.link_scenario == LINKBACK_SCENARIO_A2DP_BASE)
    {
        if (bond_flag & (BOND_FLAG_A2DP))
        {
            profs |= A2DP_PROFILE_MASK;
            profs |= AVRCP_PROFILE_MASK;
        }

#if F_APP_HID_SUPPORT
        if (bond_flag & (BOND_FLAG_HID))
        {
            if (app_cfg_const.supported_profile_mask & HID_PROFILE_MASK)
            {
                profs |= HID_PROFILE_MASK;
            }
        }
#endif
    }
    else if ((T_LINKBACK_SCENARIO)app_cfg_const.link_scenario == LINKBACK_SCENARIO_HF_A2DP_LAST_DEVICE)
    {
        if (bond_flag & BOND_FLAG_HFP)
        {
            profs |= HFP_PROFILE_MASK;

            if (app_cfg_const.supported_profile_mask & PBAP_PROFILE_MASK)
            {
                profs |= PBAP_PROFILE_MASK;
            }
        }
        else if (bond_flag & BOND_FLAG_HSP)
        {
            profs |= HSP_PROFILE_MASK;

            if (app_cfg_const.supported_profile_mask & PBAP_PROFILE_MASK)
            {
                profs |= PBAP_PROFILE_MASK;
            }
        }

        if (bond_flag & (BOND_FLAG_A2DP))
        {
            profs |= A2DP_PROFILE_MASK;
            profs |= AVRCP_PROFILE_MASK;
        }

#if F_APP_HID_SUPPORT
        if (bond_flag & (BOND_FLAG_HID))
        {
            if (app_cfg_const.supported_profile_mask & HID_PROFILE_MASK)
            {
                profs |= HID_PROFILE_MASK;
            }
        }
#endif

        if (bond_flag & BOND_FLAG_SPP)
        {
            if (app_cfg_const.supported_profile_mask & SPP_PROFILE_MASK)
            {
                profs |= SPP_PROFILE_MASK;
            }
        }
        else if (bond_flag & BOND_FLAG_IAP)
        {
            if (app_cfg_const.supported_profile_mask & IAP_PROFILE_MASK)
            {
                profs |= IAP_PROFILE_MASK;
            }
        }
    }
    else if ((T_LINKBACK_SCENARIO)app_cfg_const.link_scenario == LINKBACK_SCENARIO_SPP_BASE)
    {
        if (bond_flag & BOND_FLAG_SPP)
        {
            if (app_cfg_const.supported_profile_mask & SPP_PROFILE_MASK)
            {
                profs |= SPP_PROFILE_MASK;
            }
        }
        else if (bond_flag & BOND_FLAG_IAP)
        {
            if (app_cfg_const.supported_profile_mask & IAP_PROFILE_MASK)
            {
                profs |= IAP_PROFILE_MASK;
            }
        }
    }
#if F_APP_HID_SUPPORT
    else if ((T_LINKBACK_SCENARIO)app_cfg_const.link_scenario == LINKBACK_SCENARIO_HID_BASE)
    {
        if (bond_flag & (BOND_FLAG_HID))
        {
            if (app_cfg_const.supported_profile_mask & HID_PROFILE_MASK)
            {
                profs |= HID_PROFILE_MASK;
            }
        }
    }
#endif

    return profs;
}

static void set_bd_addr(void)
{
    ENGAGE_PRINT_TRACE3("set_bd_addr: role %d, local_addr %s, peer_addr %s",
                        app_cfg_nv.bud_role,
                        TRACE_BDADDR(app_cfg_nv.bud_local_addr),
                        TRACE_BDADDR(app_cfg_nv.bud_peer_addr));

    gap_set_bd_addr(app_cfg_nv.bud_local_addr);

    remote_local_addr_set(app_cfg_nv.bud_local_addr);
    remote_peer_addr_set(app_cfg_nv.bud_peer_addr);
    remote_session_role_set((T_REMOTE_SESSION_ROLE)app_cfg_nv.bud_role);

    legacy_vendor_data_rate_set(1); //link for 2M
}

static void modify_white_list(void)
{
    uint8_t wl_addr_type = GAP_REMOTE_ADDR_LE_PUBLIC;

    app_ble_modify_white_list(GAP_WHITE_LIST_OP_ADD, app_cfg_nv.bud_peer_addr,
                              (T_GAP_REMOTE_ADDR_TYPE)wl_addr_type);
    app_ble_modify_white_list(GAP_WHITE_LIST_OP_ADD, app_cfg_nv.bud_local_addr,
                              (T_GAP_REMOTE_ADDR_TYPE)wl_addr_type);
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static void src_conn_ind_init(void)
{
    memset(src_conn_ind, 0, sizeof(src_conn_ind));
}

static void src_conn_ind_add(uint8_t *bd_addr, bool is_update)
{
    uint16_t index;

    for (index = 0; index < SRC_CONN_IND_MAX_NUM; index++)
    {
        if (src_conn_ind[index].timer_handle != NULL)
        {
            if (!memcmp(src_conn_ind[index].bd_addr, bd_addr, 6))
            {
                break;
            }
        }
    }

    if (!is_update)
    {
        if (index == SRC_CONN_IND_MAX_NUM)
        {
            for (index = 0; index < SRC_CONN_IND_MAX_NUM; index++)
            {
                if (src_conn_ind[index].timer_handle == NULL)
                {
                    memcpy(src_conn_ind[index].bd_addr, bd_addr, 6);
                    break;
                }
            }
        }
    }

    if (index < SRC_CONN_IND_MAX_NUM)
    {
        gap_stop_timer(&src_conn_ind[index].timer_handle);
        gap_start_timer(&src_conn_ind[index].timer_handle,
                        "src_conn_ind",
                        timer_queue_id,
                        TIMER_ID_SRC_CONN_IND_TIMEOUT,
                        index,
                        SRC_CONN_IND_DELAY_MS);
    }
}

static void src_conn_ind_del(uint16_t index)
{
    memset(&src_conn_ind[index], 0, sizeof(T_SRC_CONN_IND));
}

bool src_conn_ind_check(uint8_t *bd_addr)
{
    uint16_t index;
    bool found = false;

    for (index = 0; index < SRC_CONN_IND_MAX_NUM; index++)
    {
        if (src_conn_ind[index].timer_handle != NULL)
        {
            if (!memcmp(src_conn_ind[index].bd_addr, bd_addr, 6))
            {
                ENGAGE_PRINT_TRACE1("src_conn_ind_check: found index %d", index);
                found = true;
                break;
            }
        }
    }

    return found;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static void b2b_connected_init(void)
{
    if (app_cfg_nv.bud_role == REMOTE_SESSION_ROLE_SINGLE)
    {
        engage_done = true;
    }
    else
    {
        engage_done = false;

        if (app_cfg_nv.first_engaged)
        {
            modify_white_list();
        }
    }

    b2b_connected = false;
}

static void b2b_connected_add_node(uint8_t *bd_addr)
{
    T_APP_BR_LINK *p_link;

    p_link = app_find_br_link(bd_addr);
    if (p_link == NULL)
    {
        p_link = app_alloc_br_link(bd_addr);
    }

    ENGAGE_PRINT_TRACE2("b2b_connected_add_node: bd_addr %s, p_link %p",
                        TRACE_BDADDR(bd_addr), p_link);

    if (p_link != NULL)
    {
        engage_done = true;
        b2b_connected = true;

        original_bud_role = app_cfg_nv.bud_role;
        rws_link_lost = false;
    }
}

static void b2b_connected_del_node(uint8_t *bd_addr)
{
    T_APP_BR_LINK *p_link;

    p_link = app_find_br_link(bd_addr);
    ENGAGE_PRINT_TRACE2("b2b_connected_del_node: bd_addr %s, p_link %p",
                        TRACE_BDADDR(bd_addr), p_link);
    if (p_link != NULL)
    {
        app_free_br_link(p_link);
        engage_done = false;
        b2b_connected = false;
    }
}

static void b2b_connected_add_prof(uint8_t *bd_addr, uint32_t prof)
{
    T_APP_BR_LINK *p_link;

    p_link = app_find_br_link(bd_addr);
    if (p_link != NULL)
    {
        p_link->connected_profile |= prof;
    }
}

static void b2b_connected_del_prof(uint8_t *bd_addr, uint32_t prof)
{
    T_APP_BR_LINK *p_link;

    p_link = app_find_br_link(bd_addr);
    if (p_link != NULL)
    {
        p_link->connected_profile &= ~prof;
    }
}

static void b2s_connected_init(void)
{
    b2s_connected.num = 0;
    if (app_cfg_const.enable_multi_link)
    {
        b2s_connected.num_max = app_cfg_const.max_legacy_multilink_devices;
    }
    else
    {
        b2s_connected.num_max = 1;
    }
}

uint8_t b2s_connected_get_num_max(void)
{
    return b2s_connected.num_max;
}

static uint8_t b2s_connected_is_empty(void)
{
    if (b2s_connected.num == 0)
    {
        return true;
    }
    else
    {
        return false;
    }
}

static bool b2s_connected_is_full(void)
{
#if (F_APP_AVP_INIT_SUPPORT == 1)
    T_APP_BR_LINK *p_link;
    p_link = app_find_br_link(app_db.br_link[app_get_active_a2dp_idx()].bd_addr);

    if ((b2s_connected.num >= b2s_connected.num_max)
        || (p_link->remote_device_vendor_id == APP_REMOTE_DEVICE_OTHERS && b2s_connected.num == 1))
#else
    if (b2s_connected.num >= b2s_connected.num_max)
#endif
    {
        return true;
    }
    else
    {
        return false;
    }
}

static bool b2s_connected_is_over(void)
{
    if (b2s_connected.num > b2s_connected.num_max)
    {
        return true;
    }
    else
    {
        return false;
    }
}

static bool b2s_connected_add_node(uint8_t *bd_addr, uint8_t *id)
{
    T_APP_BR_LINK *p_link;

    p_link = app_find_br_link(bd_addr);
    if (p_link == NULL)
    {
        p_link = app_alloc_br_link(bd_addr);
        if (p_link != NULL)
        {
            b2s_connected.num++;
        }
    }
    if (p_link != NULL)
    {
        *id = p_link->id;
        return true;
    }
    else
    {
        return false;
    }
}

static void b2s_connected_add_prof(uint8_t *bd_addr, uint32_t prof)
{
    T_APP_BR_LINK *p_link;
    bool sync_flag = true;

    p_link = app_find_br_link(bd_addr);
    if (p_link != NULL)
    {
        p_link->connected_profile |= prof;

        switch (prof)
        {
        case A2DP_PROFILE_MASK:
            bt_bond_flag_add(bd_addr, BOND_FLAG_A2DP);
            break;

        case HFP_PROFILE_MASK:
            {
                bt_bond_flag_remove(bd_addr, BOND_FLAG_HSP);
                bt_bond_flag_add(bd_addr, BOND_FLAG_HFP);
            }
            break;

        case HSP_PROFILE_MASK:
            {
                bt_bond_flag_remove(bd_addr, BOND_FLAG_HFP);
                bt_bond_flag_add(bd_addr, BOND_FLAG_HSP);
            }
            break;

        case SPP_PROFILE_MASK:
            bt_bond_flag_add(bd_addr, BOND_FLAG_SPP);
            break;

        case PBAP_PROFILE_MASK:
            bt_bond_flag_add(bd_addr, BOND_FLAG_PBAP);
            break;

#if F_APP_HID_SUPPORT
        case HID_PROFILE_MASK:
            bt_bond_flag_add(bd_addr, BOND_FLAG_HID);
            break;
#endif

        case IAP_PROFILE_MASK:
            bt_bond_flag_add(bd_addr, BOND_FLAG_IAP);
            break;

        default:
            sync_flag = false;
            break;
        }

        if (sync_flag == true)
        {
            T_APP_REMOTE_MSG_PAYLOAD_PROFILE_CONNECTED msg;

            memcpy(msg.bd_addr, bd_addr, 6);
            msg.prof_mask = prof;
            app_relay_async_single_with_raw_msg(APP_MODULE_TYPE_MULTI_LINK, APP_REMOTE_MSG_PROFILE_CONNECTED,
                                                (uint8_t *)&msg, sizeof(T_APP_REMOTE_MSG_PAYLOAD_PROFILE_CONNECTED));

        }
    }
}

static void b2s_connected_del_prof(uint8_t *bd_addr, uint32_t prof)
{
    T_APP_BR_LINK *p_link;

    p_link = app_find_br_link(bd_addr);
    if (p_link != NULL)
    {
        p_link->connected_profile &= ~prof;
    }
}

static bool b2s_connected_del_node(uint8_t *bd_addr)
{
    T_APP_BR_LINK *p_link;

    p_link = app_find_br_link(bd_addr);
    if (p_link != NULL)
    {
        gap_stop_timer(&p_link->timer_handle_role_switch);
        gap_stop_timer(&p_link->timer_handle_later_avrcp);
        gap_stop_timer(&p_link->timer_handle_check_role_switch);
        gap_stop_timer(&p_link->timer_handle_later_hid);
        app_free_br_link(p_link);

#if XM_XIAOAI_FEATURE_SUPPORT
        if (extend_app_cfg_const.xiaoai_support)
        {
            app_xiaoai_free_br_link(bd_addr);
        }
#endif

#if F_APP_XIAOWEI_FEATURE_SUPPORT
        if (extend_app_cfg_const.xiaowei_support)
        {
            app_xiaowei_free_br_link(bd_addr);
        }
#endif
        if (b2s_connected.num > 0)
        {
            b2s_connected.num--;
        }
        return true;
    }
    else
    {
        return false;
    }
}

static bool b2s_connected_find_node(uint8_t *bd_addr, uint32_t *profs)
{
    T_APP_BR_LINK *p_link;

    p_link = app_find_br_link(bd_addr);
    if (p_link != NULL)
    {
        if (profs != NULL)
        {
            *profs = p_link->connected_profile;
        }
        return true;
    }
    else
    {
        return false;
    }
}

static bool b2s_connected_no_profs(uint8_t *bd_addr)
{
    T_APP_BR_LINK *p_link;
    bool ret = true;

    p_link = app_find_br_link(bd_addr);
    if (p_link != NULL)
    {
        if (p_link->connected_profile != 0)
        {
            ret = false;
        }
    }

    return ret;
}

void b2s_connected_set_last_conn_index(uint8_t conn_idx)
{
    last_src_conn_idx = conn_idx;
}

uint8_t b2s_connected_get_last_conn_index(void)
{
    return last_src_conn_idx;
}

static void b2s_connected_mark_index(uint8_t *bd_addr)
{
    T_APP_BR_LINK *p_link;

    p_link = app_find_br_link(bd_addr);
    if (p_link != NULL)
    {
        ++last_src_conn_idx;

        if (b2s_connected.num > 1)
        {
            p_link->src_conn_idx = last_src_conn_idx;
        }
        else if (b2s_connected.num == 1)
        {
            p_link->src_conn_idx = last_src_conn_idx - 1;
        }
    }
}

static bool b2s_connected_is_first_src(uint8_t *bd_addr)
{
    T_APP_BR_LINK *p_link;
    bool ret = true;

    if (b2s_connected.num_max >= 2)
    {
        p_link = app_find_br_link(bd_addr);
        if (p_link != NULL)
        {
            if (p_link->src_conn_idx >= last_src_conn_idx)
            {
                ret = false;
            }
        }
    }

    return ret;
}

static void connected_node_auth_suc(uint8_t *bd_addr)
{
    T_APP_BR_LINK *p_link;

    p_link = app_find_br_link(bd_addr);
    if (p_link != NULL)
    {
        p_link->auth_flag = true;
    }
}

static bool connected_node_is_authed(uint8_t *bd_addr)
{
    T_APP_BR_LINK *p_link;
    bool ret = false;

    p_link = app_find_br_link(bd_addr);
    if (p_link != NULL)
    {
        ret = p_link->auth_flag;
    }

    return ret;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


static void event_ind(T_BP_EVENT event, T_BP_EVENT_PARAM *event_param)
{
    if (NULL == ind_fun)
    {
        return;
    }

    if (cur_state >= STATE_SHUTDOWN_STEP)
    {
        switch (event)
        {
        case BP_EVENT_STATE_CHANGED:
        case BP_EVENT_RADIO_MODE_CHANGED:
        case BP_EVENT_PROFILE_DISCONN:
        case BP_EVENT_SRC_DISCONN_LOST:
        case BP_EVENT_SRC_DISCONN_NORMAL:
            ind_fun(event, event_param);
            break;

        default:
            ENGAGE_PRINT_TRACE0("event_ind: cur_state >= STATE_SHUTDOWN_STEP, not ind");
            break;
        }
    }
    else
    {
        ind_fun(event, event_param);
    }
}

#if F_APP_ERWS_SUPPORT
static void relay_pri_bp_state(void)
{
    if (app_cfg_nv.bud_role == REMOTE_SESSION_ROLE_PRIMARY &&
        app_db.remote_session_state == REMOTE_SESSION_STATE_CONNECTED)
    {
        if (cur_state < STATE_AFE_SECONDARY)
        {
            new_pri_bp_state(bp_state);
            app_relay_async_single(APP_MODULE_TYPE_BT_POLICY, BT_POLICY_MSG_PRI_BP_STATE);
        }
    }
}

static void recv_relay_pri_bp_state(void *buf, uint16_t len)
{
    uint8_t data = *((uint8_t *)buf);

    if (app_cfg_nv.bud_role == REMOTE_SESSION_ROLE_SECONDARY)
    {
        new_pri_bp_state(data);

        pri_bp_state = (T_BP_STATE)data;
    }
}

static void relay_pri_req(uint8_t req)
{
    if (app_cfg_nv.bud_role == REMOTE_SESSION_ROLE_PRIMARY &&
        app_db.remote_session_state == REMOTE_SESSION_STATE_CONNECTED)
    {
        ENGAGE_PRINT_TRACE1("relay_pri_req: 0x%x", req);
        app_relay_async_single_with_raw_msg(APP_MODULE_TYPE_BT_POLICY, BT_POLICY_MSG_PRI_REQ, &req, 1);
    }
}

static void recv_relay_pri_req(void *buf, uint16_t len)
{
    if (app_cfg_nv.bud_role == REMOTE_SESSION_ROLE_SECONDARY)
    {
        uint8_t req = *((uint8_t *)buf);

        ENGAGE_PRINT_TRACE1("recv_relay_pri_req: 0x%x", req);

        if (req & PRI_REQ_LET_SEC_TO_DISCONN)
        {
            legacy_send_acl_disconn_req(app_cfg_nv.bud_peer_addr);
        }
    }
}

static void relay_pri_linkback_node(void)
{
    uint32_t node_num;
    T_LINKBACK_NODE *p_node;
    uint32_t i;

    if (app_cfg_nv.bud_role == REMOTE_SESSION_ROLE_PRIMARY &&
        app_db.remote_session_state == REMOTE_SESSION_STATE_CONNECTED)
    {
        node_num = linkback_todo_queue_node_num();
        if (node_num != 0)
        {
            p_node = malloc(node_num * sizeof(T_LINKBACK_NODE));
            if (p_node != NULL)
            {
                for (i = 0; i < node_num; i++)
                {
                    linkback_todo_queue_take_first_node(p_node + i);
                }

                app_relay_async_single_with_raw_msg(APP_MODULE_TYPE_BT_POLICY, BT_POLICY_MSG_LINKBACK_NODE,
                                                    (uint8_t *)p_node, node_num * sizeof(T_LINKBACK_NODE));

                free(p_node);
            }
        }
    }
}

static void recv_relay_pri_linkback_node(void *buf, uint16_t len)
{
    if (app_cfg_nv.bud_role == REMOTE_SESSION_ROLE_SECONDARY)
    {
        T_LINKBACK_NODE *p_node = (T_LINKBACK_NODE *)buf;
        uint32_t node_num = len / sizeof(T_LINKBACK_NODE);
        T_LINKBACK_NODE *node;
        uint32_t i;

        ENGAGE_PRINT_TRACE1("recv_relay_pri_linkback_node: node_num %d", node_num);

        startup_linkback_done_flag = true;
        bond_list_load_flag = true;

        linkback_todo_queue_init();
        linkback_active_node_init();

        for (i = 0; i < node_num; i++)
        {
            node = p_node + i;

            linkback_todo_queue_insert_normal_node(node->bd_addr, node->plan_profs, node->retry_timeout,
                                                   node->is_group_member, false);
        }
    }
}
#endif

static void app_bt_policy_primary_adv(void)
{
    uint8_t adv_interval;

    if (app_find_sco_conn_num() != 0)
    {
        adv_interval = PRIMARY_BLE_ADV_INTERVAL_VOICE;
    }
    else if (app_find_a2dp_start_num() != 0)
    {
        adv_interval = PRIMARY_BLE_ADV_INTERVAL_AUDIO;
    }
    else
    {
        adv_interval = PRIMARY_BLE_ADV_INTERVAL_NORMAL;
    }

    ENGAGE_PRINT_TRACE1("app_bt_policy_primary_adv: adv_interval 0x%x", adv_interval);
#if ISOC_AUDIO_SUPPORT
    mtc_gap_set_pri(MTC_GAP_VENDOR_ADV);
#endif
    engage_afe_primary_adv_start(adv_interval);
}

void app_bt_policy_primary_engage_action_adjust(void)
{
    if (!engage_done && app_cfg_nv.bud_role == REMOTE_SESSION_ROLE_PRIMARY)
    {
        gap_stop_timer(&timer_handle_engage_action_adjust);

        switch (cur_state)
        {
        case STATE_AFE_LINKBACK:
            {
                app_bt_policy_primary_adv();
            }
            break;

        case STATE_AFE_CONNECTED:
        case STATE_AFE_PAIRING_MODE:
        case STATE_AFE_STANDBY:
            {
                if (b2s_connected.num == 0 && original_bud_role != REMOTE_SESSION_ROLE_PRIMARY)
                {
                    engage_afe_forever_shaking_start();
                }
                else
                {
                    app_bt_policy_primary_adv();
                }
            }
            break;

        default:
            break;
        }
    }
}

static void enter_state(T_STATE state, T_BT_DEVICE_MODE mode)
{
    bool state_changed = false;
    T_BP_EVENT_PARAM event_param;
    uint8_t to;

    if (state != STATE_AFE_LINKBACK)
    {
        gap_stop_timer(&timer_handle_linkback_delay);
    }

    if (cur_state != state || roleswap_suc_flag)
    {
        roleswap_suc_flag = false;

        new_state(state);

        cur_state = state;
        state_changed = true;
    }

    if (STATE_AFE_LINKBACK == cur_state)
    {
        if (!app_cfg_const.enable_not_discoverable_when_linkback)
        {
            mode = BT_DEVICE_MODE_DISCOVERABLE_CONNECTABLE;
        }
    }

    if (STATE_AFE_STANDBY == cur_state)
    {
        if (app_cfg_const.enable_discoverable_in_standby_mode)
        {
            mode = BT_DEVICE_MODE_DISCOVERABLE_CONNECTABLE;
        }
    }

    if (app_cfg_nv.bud_role != REMOTE_SESSION_ROLE_SECONDARY && b2s_connected.num == 1)
    {
        if (app_cfg_const.enable_multi_link && app_cfg_const.enable_always_discoverable)
        {
            mode = BT_DEVICE_MODE_DISCOVERABLE_CONNECTABLE;
        }
    }

    if (STATE_AFE_CONNECTED == cur_state ||
        STATE_AFE_PAIRING_MODE == cur_state ||
        STATE_AFE_STANDBY == cur_state)
    {
        if (discoverable_when_one_link)
        {
            discoverable_when_one_link = false;

            if (mode != BT_DEVICE_MODE_DISCOVERABLE_CONNECTABLE &&
                app_hfp_get_call_status() == BT_HFP_CALL_IDLE)
            {
                mode = BT_DEVICE_MODE_DISCOVERABLE_CONNECTABLE;

                gap_stop_timer(&timer_handle_discoverable);
#if F_APP_LE_AUDIO_SM
                if (app_le_audio_dev_ctrl(T_LEA_DEV_CRL_GET_LEGACY, NULL))
#endif
                    gap_start_timer(&timer_handle_discoverable, "discoverable", timer_queue_id,
                                    TIMER_ID_DISCOVERABLE_TIMEOUT, 0, app_cfg_const.timer_pairing_while_one_conn_timeout * 1000);
            }
        }
#if GFPS_SASS_SUPPORT
        else if (app_sass_policy_support_easy_connection_switch())
        {
            if (mode != BT_DEVICE_MODE_DISCOVERABLE_CONNECTABLE)
            {
                mode = BT_DEVICE_MODE_CONNECTABLE;
            }
        }
#endif

        if ((app_hfp_get_call_status() != BT_HFP_CALL_IDLE) ||
            b2s_connected_is_full())
        {
            gap_stop_timer(&timer_handle_discoverable);
        }
    }
    else
    {
        gap_stop_timer(&timer_handle_discoverable);
    }

    if (timer_handle_discoverable != NULL)
    {
        mode = BT_DEVICE_MODE_DISCOVERABLE_CONNECTABLE;
    }

    if (radio_mode != mode)
    {
        new_radio_mode(mode);
        radio_mode = mode;
#if (F_APP_HARMAN_FEATURE_SUPPORT && HARMAN_ONLY_CONN_ONE_DEVICE_WHEN_PAIRING)
        if (app_cfg_const.enable_multi_link)
        {
            if (radio_mode == BT_DEVICE_MODE_DISCOVERABLE_CONNECTABLE)
            {
                at_pairing_radio_setting(true);
                au_set_harman_already_connect_one(false);
            }
            else
            {
                at_pairing_radio_setting(false);
                if (app_find_b2s_link_num())
                {
                    au_set_harman_already_connect_one(true);
                }
            }
        }
#endif

#if F_APP_BLE_SWIFT_PAIR_SUPPORT
        if (radio_mode == BT_DEVICE_MODE_DISCOVERABLE_CONNECTABLE)
        {
            swift_pair_adv_start(APP_SWIFT_PAIR_DEFAULT_TIMEOUT);
        }
        else
        {
            swift_pair_adv_stop(APP_STOP_ADV_CAUSE_EXIT_PAIRING_MODE);
        }
#endif

#if F_APP_LE_AUDIO_SM
        app_le_audio_dev_ctrl(T_LEA_DEV_CRL_SET_RADIO, &mode);
#else
        bt_device_mode_set(mode);
#endif
        event_ind(BP_EVENT_RADIO_MODE_CHANGED, NULL);
    }

    if (is_user_action_pairing_mode)
    {
        is_user_action_pairing_mode = false;
#if (F_APP_AVP_INIT_SUPPORT == 1)
//rsv for avp
#else
        state_changed = true;
#endif
    }

    if (state_changed)
    {
        switch (cur_state)
        {
        case STATE_STARTUP:
        case STATE_SHUTDOWN:
            bp_state = BP_STATE_IDLE;
            break;

        case STATE_FE_SHAKING:
            bp_state = BP_STATE_FIRST_ENGAGE;

            engage_fe_shaking_start();

            to = app_cfg_const.timer_first_engage;
            if (to < FE_TO_MIN)
            {
                to = FE_TO_MIN;
            }

            gap_stop_timer(&timer_handle_first_engage_check);
            gap_start_timer(&timer_handle_first_engage_check, "first_engage_check", timer_queue_id,
                            TIMER_ID_FIRST_ENGAGE_CHECK, 0, (to * FE_TO_UNIT));

            app_auto_power_off_disable(AUTO_POWER_OFF_MASK_BUD_COUPLING);
            break;

        case STATE_FE_COUPLING:
            bp_state = BP_STATE_FIRST_ENGAGE;
            break;

        case STATE_AFE_TIMEOUT_SHAKING:
            bp_state = BP_STATE_ENGAGE;
            engage_afe_timeout_shaking_start();
            break;

        case STATE_AFE_COUPLING:
            bp_state = BP_STATE_ENGAGE;
            startup_linkback_done_flag = false;
            bond_list_load_flag = false;
            linkback_todo_queue_init();
            linkback_active_node_init();
            break;

        case STATE_AFE_WAIT_COUPLING:
            bp_state = BP_STATE_ENGAGE;
            break;

        case STATE_AFE_LINKBACK:
            bp_state = BP_STATE_LINKBACK;
            app_bt_policy_qos_param_update(app_db.br_link[app_get_active_a2dp_idx()].bd_addr,
                                           BP_TPOLL_LINKBACK_START);
            app_bt_policy_primary_engage_action_adjust();
            break;

        case STATE_AFE_CONNECTED:
            bp_state = BP_STATE_CONNECTED;
            app_bt_policy_primary_engage_action_adjust();
            break;

        case STATE_AFE_PAIRING_MODE:
            bp_state = BP_STATE_PAIRING_MODE;
            app_bt_policy_primary_engage_action_adjust();
            break;

        case STATE_AFE_STANDBY:
            bp_state = BP_STATE_STANDBY;
            app_bt_policy_primary_engage_action_adjust();
            if (app_cfg_const.timer_auto_power_off_while_phone_connected_and_anc_apt_off)
            {
                app_auto_power_off_enable(AUTO_POWER_OFF_MASK_SOURCE_LINK,
                                          app_cfg_const.timer_auto_power_off_while_phone_connected_and_anc_apt_off);
            }
            break;

        case STATE_AFE_SECONDARY:
            bp_state = BP_STATE_SECONDARY;
            break;

        case STATE_DUT_TEST_MODE:
            bp_state = BP_STATE_CONNECTED;
            engage_done = true;
            break;

#if (F_APP_OTA_TOOLING_SUPPORT == 1)
        case STATE_OTA_SHAKING:
            engage_afe_forever_shaking_start();
            break;
#endif

        case STATE_PREPARE_FOR_ROLESWAP:
            bp_state = BP_STATE_PREPARE_ROLESWAP;
            break;

        case STATE_SHUTDOWN_STEP:
        case STATE_STOP:
            bp_state = BP_STATE_IDLE;
            engage_off();
            break;

        default:
            break;
        }

        if (app_cfg_const.timer_pairing_timeout != 0)
        {
            gap_stop_timer(&timer_handle_pairing_mode);
            if (STATE_AFE_PAIRING_MODE == cur_state)
            {
#if F_APP_LE_AUDIO_SM
                if (app_le_audio_dev_ctrl(T_LEA_DEV_CRL_GET_LEGACY, NULL) ||
                    app_le_audio_dev_ctrl(T_LEA_DEV_CRL_GET_CIS_POLICY, NULL))
#endif
                    gap_start_timer(&timer_handle_pairing_mode, "pairing_timer", timer_queue_id,
                                    TIMER_ID_PAIRING_TIMEOUT, 0, app_cfg_const.timer_pairing_timeout * 1000);
            }
        }

        event_param.is_shut_down = false;
        if (STATE_SHUTDOWN == cur_state)
        {
            event_param.is_shut_down = true;
        }

        event_param.is_ignore = true;
        if (STATE_AFE_PAIRING_MODE == cur_state)
        {
            event_param.is_ignore = false;
        }

        event_ind(BP_EVENT_STATE_CHANGED, &event_param);

        if (STATE_AFE_LINKBACK == cur_state)
        {
            app_sniff_mode_b2s_disable_all(SNIFF_DISABLE_MASK_LINKBACK);

            linkback_run();
        }
        else
        {
            app_sniff_mode_b2s_enable_all(SNIFF_DISABLE_MASK_LINKBACK);
        }
    }
}

static void linkback_sched(void)
{
    ENGAGE_PRINT_TRACE3("linkback_sched: enable_power_on_linkback %d, startup_linkback_done_flag %d, bond_list_load_flag %d",
                        app_cfg_const.enable_power_on_linkback,
                        startup_linkback_done_flag,
                        bond_list_load_flag);

    linkback_flag = false;

    if (dedicated_enter_pairing_mode_flag)
    {
        dedicated_enter_pairing_mode_flag = false;

        if (judge_dedicated_enter_pairing_mode())
        {
            prepare_for_dedicated_enter_pairing_mode();
            return;
        }
    }

    if (app_cfg_const.enable_power_on_linkback)
    {
        if (!startup_linkback_done_flag)
        {
            if (!bond_list_load_flag)
            {
                bond_list_load_flag = true;

                linkback_load_bond_list(0, linkback_retry_timeout);
            }
        }
    }

    if (0 != linkback_todo_queue_node_num() || linkback_active_node.is_valid)
    {
        enter_state(STATE_AFE_LINKBACK, BT_DEVICE_MODE_CONNECTABLE);
    }
    else
    {
        stable_sched(STABLE_ENTER_MODE_NORMAL);
    }
}

static void linkback_run(void)
{
    T_LINKBACK_NODE node;
    uint32_t profs;

    ENGAGE_PRINT_TRACE0("linkback_run: start");

RETRY:
    if (!linkback_active_node.is_valid)
    {
        if (linkback_todo_queue_take_first_node(&node))
        {
            linkback_active_node_load(&node);

            if (linkback_active_node.is_valid)
            {
                linkback_active_node.linkback_node.gtiming = os_sys_time_get();

                gap_stop_timer(&timer_handle_linkback);
                gap_start_timer(&timer_handle_linkback, "linkback", timer_queue_id,
                                TIMER_ID_LINKBACK_TIMEOUT, 0, linkback_active_node.retry_timeout);
            }
        }
    }

    if (!linkback_active_node.is_valid)
    {
        ENGAGE_PRINT_TRACE0("linkback_run: have no valid node, finish");
        app_harman_le_common_adv_update();
#if GFPS_SASS_SUPPORT
        if (app_sass_policy_support_easy_connection_switch())
        {
            app_sass_policy_link_back_end();
        }
#endif
        stable_sched(STABLE_ENTER_MODE_NORMAL);

        goto EXIT;
    }

    if (linkback_active_node.is_exit)
    {
        linkback_active_node.is_valid = false;

        goto RETRY;
    }

    if (0 == linkback_active_node.remain_profs)
    {
        linkback_active_node.is_valid = false;

        goto RETRY;
    }

    if (src_conn_ind_check(linkback_active_node.linkback_node.bd_addr))
    {
        gap_stop_timer(&timer_handle_linkback_delay);
        gap_start_timer(&timer_handle_linkback_delay, "linkback_delay", timer_queue_id,
                        TIMER_ID_LINKBACK_DELAY, 0, 500);

        goto EXIT;
    }

    if (b2s_connected_find_node(linkback_active_node.linkback_node.bd_addr, &profs))
    {
        if (profs & linkback_active_node.doing_prof)
        {
            ENGAGE_PRINT_TRACE1("linkback_run: prof 0x%08x, already connected",
                                linkback_active_node.doing_prof);

            linkback_active_node_step_suc_adjust_remain_profs();

            goto RETRY;
        }
        else
        {
            goto LINKBACK;
        }
    }
    else
    {
        if (linkback_active_node.linkback_node.is_force)
        {
            goto LINKBACK;
        }
        else
        {
            if (b2s_connected_is_full())
            {
                ENGAGE_PRINT_TRACE0("linkback_run: b2s is full, abort this node");

                linkback_active_node.is_valid = false;

                goto RETRY;
            }
            else
            {
                goto LINKBACK;
            }
        }
    }

LINKBACK:
    linkback_flag = true;

    if (linkback_active_node.linkback_node.check_bond_flag)
    {
        if (!linkback_check_bond_flag(linkback_active_node.linkback_node.bd_addr,
                                      linkback_active_node.doing_prof))
        {
            linkback_active_node_step_fail_adjust_remain_profs();

            goto RETRY;
        }
    }

    if (linkback_profile_is_need_search(linkback_active_node.doing_prof))
    {
        app_hfp_adjust_sco_window(linkback_active_node.linkback_node.bd_addr, APP_SCO_ADJUST_LINKBACK_EVT);

        if (!linkback_profile_search_start(linkback_active_node.linkback_node.bd_addr,
                                           linkback_active_node.doing_prof, linkback_active_node.linkback_node.is_special,
                                           &linkback_active_node.linkback_node.search_param))
        {
            linkback_active_node_step_fail_adjust_remain_profs();

            goto RETRY;
        }
        else
        {
            ENGAGE_PRINT_TRACE0("linkback_run: wait search result");

            goto EXIT;
        }
    }
    else
    {
        linkback_active_node.is_sdp_ok = true;

        app_hfp_adjust_sco_window(linkback_active_node.linkback_node.bd_addr, APP_SCO_ADJUST_LINKBACK_EVT);

        if (linkback_profile_connect_start(linkback_active_node.linkback_node.bd_addr,
                                           linkback_active_node.doing_prof, &linkback_active_node.linkback_conn_param))
        {
            goto EXIT;
        }
        else
        {
            linkback_active_node_step_fail_adjust_remain_profs();

            goto RETRY;
        }
    }

EXIT:
    return;
}

#if F_APP_ERWS_SUPPORT
static void roleswap_event_handle(T_BT_PARAM *param)
{
    uint8_t app_idx;

    ENGAGE_PRINT_TRACE2("roleswap_event_handle: bud_role %d, is_suc %d", param->bud_role,
                        param->is_suc);

    if (param->is_suc)
    {
        T_APP_BR_LINK *p_link = NULL;

        app_cfg_nv.bud_role = param->bud_role;

        cur_event = EVENT_ROLESWAP;

        engage_done = true;
        b2b_connected = true;
        b2s_connected.num = app_find_b2s_link_num();
        connected();
#if F_APP_LE_AUDIO_SM
        app_le_audio_dev_ctrl(T_LEA_DEV_CRL_SET_IDLE, NULL);
#endif
        dedicated_enter_pairing_mode_flag = false;
        is_visiable_flag = false;
        is_force_flag = false;
        is_user_action_pairing_mode = false;
        disconnect_for_pairing_mode = false;
        discoverable_when_one_link = false;

        linkback_flag = false;
        linkback_retry_timeout = app_cfg_const.timer_linkback_timeout;
        after_stop_sdp_todo_linkback_run_flag = false;

        gap_stop_timer(&timer_handle_pairing_mode);
        gap_stop_timer(&timer_handle_discoverable);
        gap_stop_timer(&timer_handle_linkback);
        gap_stop_timer(&timer_handle_linkback_delay);
        gap_stop_timer(&timer_handle_wait_coupling);
        gap_stop_timer(&timer_handle_engage_action_adjust);
        gap_stop_timer(&timer_handle_page_scan_param);

        original_bud_role = app_cfg_nv.bud_role;
        rws_link_lost = false;

        src_conn_ind_init();

        new_pri_apply_relay_info_when_roleswap_suc();

        ENGAGE_PRINT_TRACE1("roleswap_event_handle: last_src_conn_idx %d", last_src_conn_idx);

        for (app_idx = 0; app_idx < MAX_BR_LINK_NUM; app_idx++)
        {
            if (app_check_b2s_link_by_id(app_idx))
            {
                ENGAGE_PRINT_TRACE2("roleswap_event_handle: app_db.br_link[%d].src_conn_idx %d", app_idx,
                                    app_db.br_link[app_idx].src_conn_idx);
                p_link = &app_db.br_link[app_idx];
                if (p_link)
                {
                    if ((app_cfg_nv.bud_role == REMOTE_SESSION_ROLE_PRIMARY) &&
                        (p_link->sniffing_type == BT_SNIFFING_TYPE_A2DP))
                    {
                        /*for a2dp sniffing type, set flag to keep small htpoll value whatever happens,
                        until receive 7 a2dp packets from src*/
                        app_db.recover_param = true;
                        app_db.down_count = 0;
                    }
                }
            }
        }

        linkback_active_node_init();
        app_sniff_mode_roleswap_suc();

        if (cur_state != STATE_SHUTDOWN_STEP &&
            cur_state != STATE_SHUTDOWN)
        {
            if (app_cfg_nv.bud_role == REMOTE_SESSION_ROLE_PRIMARY)
            {
                startup_linkback_done_flag = true;
                bond_list_load_flag = true;

                if ((app_cfg_const.enter_pairing_while_only_one_device_connected) && (b2s_connected.num == 1)
#if (F_APP_AVP_INIT_SUPPORT == 1)
                    && (p_link->remote_device_vendor_id == APP_REMOTE_DEVICE_IOS)
#endif
                   )
                {
                    discoverable_when_one_link = true;
                }

                if (linkback_todo_queue_node_num() != 0)
                {
                    enter_state(STATE_AFE_LINKBACK, BT_DEVICE_MODE_CONNECTABLE);
                }
                else
                {
                    linkback_todo_queue_init();

                    if (BP_STATE_PAIRING_MODE == pri_bp_state)
                    {
                        cur_state = STATE_AFE_PAIRING_MODE;
                    }
                    else if (BP_STATE_STANDBY == pri_bp_state)
                    {
                        cur_state = STATE_AFE_STANDBY;
                    }
                    else
                    {
                        cur_state = STATE_AFE_CONNECTED;
                    }

                    stable_sched(STABLE_ENTER_MODE_AGAIN);
                }

                roleswap_suc_flag = true;
                pri_bp_state = BP_STATE_IDLE;
            }
            else
            {
                linkback_todo_queue_init();
                startup_linkback_done_flag = false;
                bond_list_load_flag = false;

                pri_bp_state = bp_state;

                enter_state(STATE_AFE_SECONDARY, BT_DEVICE_MODE_IDLE);
            }
        }
    }
    else
    {
        if (cur_state != STATE_SHUTDOWN_STEP &&
            cur_state != STATE_SHUTDOWN)
        {
            stable_sched(STABLE_ENTER_MODE_AGAIN);
        }
    }

    if (cur_state != STATE_SHUTDOWN_STEP && cur_state != STATE_SHUTDOWN)
    {
        app_reconnect_inactive_link();
    }

    if (STATE_SHUTDOWN_STEP == cur_state)
    {
        disconnect_all_event_handle();
    }
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static void roleswitch_b2b_check(T_APP_BR_LINK *p_link)
{
    if (app_bt_policy_get_b2b_connected() && cur_state < STATE_SHUTDOWN_STEP)
    {
        if (0 == app_bt_policy_get_b2s_connected_num() &&
            !app_cfg_const.rws_disable_codec_mute_when_linkback)
        {
            if (BT_LINK_ROLE_MASTER == p_link->acl_link_role)
            {
                app_sniff_mode_b2b_disable(p_link->bd_addr, SNIFF_DISABLE_MASK_ROLESWITCH);

                if (p_link->acl_link_in_sniffmode_flg)
                {
                    p_link->roleswitch_check_after_unsniff_flg = true;
                }
                else
                {
                    ENGAGE_PRINT_TRACE0("roleswitch_b2b_check: switch to slave");

                    if (bt_link_role_switch(p_link->bd_addr, false))
                    {
                        app_db.b2b_role_switch_led_pending = 1;
                    }
                }
            }
        }
        else
        {
            if (BT_LINK_ROLE_SLAVE == p_link->acl_link_role)
            {
                app_sniff_mode_b2b_disable(p_link->bd_addr, SNIFF_DISABLE_MASK_ROLESWITCH);

                if (p_link->acl_link_in_sniffmode_flg)
                {
                    p_link->roleswitch_check_after_unsniff_flg = true;
                }
                else
                {
                    ENGAGE_PRINT_TRACE0("roleswitch_b2b_check: switch to master");

                    if (bt_link_role_switch(p_link->bd_addr, true))
                    {
                        app_db.b2b_role_switch_led_pending = 1;
                    }
                }
            }
        }
    }
}
#endif
static void roleswitch_b2s_check(T_APP_BR_LINK *p_link)
{
    if (BT_LINK_ROLE_MASTER == p_link->acl_link_role)
    {
        app_sniff_mode_b2s_disable(p_link->bd_addr, SNIFF_DISABLE_MASK_ROLESWITCH);

        if (p_link->acl_link_in_sniffmode_flg)
        {
            p_link->roleswitch_check_after_unsniff_flg = true;
        }
        else
        {
            ENGAGE_PRINT_TRACE0("roleswitch_b2s_check: switch to slave");

            bt_link_role_switch(p_link->bd_addr, false);
        }
    }
}

static void roleswitch_handle(uint8_t *bd_addr, uint32_t event)
{
    T_APP_BR_LINK *p_link;

    if ((app_cfg_nv.bud_role == REMOTE_SESSION_ROLE_PRIMARY) ||
        (app_cfg_nv.bud_role == REMOTE_SESSION_ROLE_SINGLE))
    {
        if (ROLESWITCH_EVENT_LINK_DISCONNECTED == event)
        {
            ENGAGE_PRINT_TRACE1("roleswitch_handle: b2s, event %d, role_switch_count 0", event);

            p_link = app_find_br_link(app_cfg_nv.bud_peer_addr);
            if (p_link != NULL)
            {
                p_link->link_role_switch_count = 0;
#if F_APP_ERWS_SUPPORT
                roleswitch_b2b_check(p_link);
#endif
            }
            return;
        }

        p_link = app_find_br_link(bd_addr);
        if (p_link != NULL)
        {
            if (event != ROLESWITCH_EVENT_FAIL_RETRY)
            {
                p_link->link_role_switch_count = 0;
            }

            if (app_check_b2b_link(bd_addr))
            {
                ENGAGE_PRINT_TRACE2("roleswitch_handle: b2b, event %d, role_switch_count %d", event,
                                    p_link->link_role_switch_count);

                switch (event)
                {
                case ROLESWITCH_EVENT_LINK_CONNECTED:
                case ROLESWITCH_EVENT_LINK_ACTIVE:
                case ROLESWITCH_EVENT_FAIL_RETRY:
                    {
#if F_APP_ERWS_SUPPORT
                        roleswitch_b2b_check(p_link);
#endif
                    }
                    break;

                case ROLESWITCH_EVENT_FAIL_RETRY_MAX:
                    {
                        app_sniff_mode_b2b_enable(bd_addr, SNIFF_DISABLE_MASK_ROLESWITCH);
#if F_APP_ERWS_SUPPORT
                        relay_pri_req(PRI_REQ_LET_SEC_TO_DISCONN);
#endif
                    }
                    break;

                case ROLESWITCH_EVENT_ROLE_CHANGED:
                    {
                        if (0 == app_bt_policy_get_b2s_connected_num() &&
                            !app_cfg_const.rws_disable_codec_mute_when_linkback)
                        {
                            if (BT_LINK_ROLE_SLAVE == p_link->acl_link_role)
                            {
                                app_sniff_mode_b2b_enable(bd_addr, SNIFF_DISABLE_MASK_ROLESWITCH);
                            }
                        }
                        else
                        {
                            if (BT_LINK_ROLE_MASTER == p_link->acl_link_role)
                            {
                                app_sniff_mode_b2b_enable(bd_addr, SNIFF_DISABLE_MASK_ROLESWITCH);

                                app_bt_sniffing_process(bd_addr);
                            }
                        }
                    }
                    break;

                default:
                    break;
                }
            }
            else
            {
                ENGAGE_PRINT_TRACE2("roleswitch_handle: b2s, event %d, role_switch_count %d", event,
                                    p_link->link_role_switch_count);

                switch (event)
                {
                case ROLESWITCH_EVENT_LINK_CONNECTED:
                case ROLESWITCH_EVENT_LINK_ACTIVE:
                    {
                        roleswitch_b2s_check(p_link);

                        p_link = app_find_br_link(app_cfg_nv.bud_peer_addr);
                        if (p_link != NULL)
                        {
#if F_APP_ERWS_SUPPORT
                            roleswitch_b2b_check(p_link);
#endif
                        }
                    }
                    break;

                case ROLESWITCH_EVENT_FAIL_RETRY:
                    {
                        if (app_db.br_link[app_get_active_a2dp_idx()].bt_sniffing_state >= APP_BT_SNIFFING_STATE_SNIFFING)
                        {
                            app_disconnect_inactive_link();
                            gap_start_timer(&timer_handle_reconnect, "reconnect_timer", timer_queue_id,
                                            TIMER_ID_RECONNECT, 0, 1000);
                        }
                        else
                        {
                            roleswitch_b2s_check(p_link);
                        }
                    }
                    break;

                case ROLESWITCH_EVENT_FAIL_RETRY_MAX:
                    {
                        p_link->link_role_switch_count = 0;
                        app_sniff_mode_b2s_enable(bd_addr, SNIFF_DISABLE_MASK_ROLESWITCH);
                        gap_start_timer(&p_link->timer_handle_role_switch,
                                        "retry_role_switch_window",
                                        timer_queue_id,
                                        TIMER_ID_ROLE_SWITCH,
                                        p_link->id,
                                        ROLE_SWITCH_WINDOW_DELAY_MS);
                    }
                    break;

                case ROLESWITCH_EVENT_ROLE_CHANGED:
                    {
                        if (BT_LINK_ROLE_SLAVE == p_link->acl_link_role)
                        {
                            app_sniff_mode_b2s_enable(bd_addr, SNIFF_DISABLE_MASK_ROLESWITCH);

                            app_bt_sniffing_process(bd_addr);
                            if (linkback_active_node.sdp_cmpl_pending_flag == true)
                            {
                                T_BT_PARAM bt_param;
                                bt_param.bd_addr = bd_addr;
                                bt_param.cause = 0;
                                state_machine(EVENT_PROFILE_SDP_DISCOV_CMPL, &bt_param);
                            }
                        }
                    }
                    break;

                default:
                    break;
                }
            }
        }
    }
}

static void roleswitch_fail_event_handle(T_BT_PARAM *param)
{
    T_APP_BR_LINK *p_link;

    p_link = app_find_br_link(param->bd_addr);
    if (p_link != NULL)
    {
        p_link->link_role_switch_count++;

        if (p_link->link_role_switch_count < ROLE_SWITCH_COUNT_MAX)
        {
            gap_start_timer(&p_link->timer_handle_role_switch,
                            "retry_role_switch",
                            timer_queue_id,
                            TIMER_ID_ROLE_SWITCH,
                            p_link->id,
                            ROLE_SWITCH_DELAY_MS);
        }
        else
        {
            roleswitch_handle(param->bd_addr, ROLESWITCH_EVENT_FAIL_RETRY_MAX);
        }
    }
}

static void roleswitch_timeout_event_handle(T_BT_PARAM *param)
{
    T_APP_BR_LINK *p_link = (T_APP_BR_LINK *)param;

    if (p_link)
    {
        roleswitch_handle(p_link->bd_addr, ROLESWITCH_EVENT_FAIL_RETRY);
    }
}

static void role_master_event_handle(T_BT_PARAM *param)
{
    T_APP_BR_LINK *p_link;

    p_link = app_find_br_link(param->bd_addr);
    if (p_link != NULL)
    {
        p_link->acl_link_role = BT_LINK_ROLE_MASTER;

        roleswitch_handle(param->bd_addr, ROLESWITCH_EVENT_ROLE_CHANGED);
    }
}

static void role_slave_event_handle(T_BT_PARAM *param)
{
    T_APP_BR_LINK *p_link;

    p_link = app_find_br_link(param->bd_addr);
    if (p_link != NULL)
    {
        p_link->acl_link_role = BT_LINK_ROLE_SLAVE;

        roleswitch_handle(param->bd_addr, ROLESWITCH_EVENT_ROLE_CHANGED);
    }
}

static void prepare_for_roleswap_event_handle(void)
{
    stop_all_active_action();

    enter_state(STATE_PREPARE_FOR_ROLESWAP, BT_DEVICE_MODE_IDLE);
}

void mmi_disconnect_all_link_event_handle(void)
{
    //stop_all_active_action();

    app_bt_policy_disconnect_all_link();
}

static void conn_sniff_event_handle(T_BT_PARAM *param)
{
    T_APP_BR_LINK *p_link;

    p_link = app_find_br_link(param->bd_addr);
    if (p_link != NULL)
    {
        p_link->acl_link_in_sniffmode_flg = true;
#if F_APP_HARMAN_FEATURE_SUPPORT
        uint8_t to_do_connect_idle_power_off = true;
        T_APP_BR_LINK *p_link_check;

        for (uint8_t i = 0; i < MAX_BR_LINK_NUM; i++)
        {
            if (app_check_b2s_link_by_id(i))
            {
                p_link_check = app_find_br_link(app_db.br_link[i].bd_addr);
                if (p_link_check->acl_link_in_sniffmode_flg == false)
                {
                    to_do_connect_idle_power_off = false;
                }
            }
        }
        APP_PRINT_INFO1("conn_sniff_event_handle =%d", to_do_connect_idle_power_off);

        if ((timer_handle_harman_power_off_option == NULL) &&
            (to_do_connect_idle_power_off))
        {
            au_connect_idle_to_power_off(CONNECT_IDLE_POWER_OFF_START, p_link->id);
        }
#endif
    }
}

static void conn_active_event_handle(T_BT_PARAM *param)
{
    T_APP_BR_LINK *p_link;

    p_link = app_find_br_link(param->bd_addr);
    if (p_link != NULL)
    {
        p_link->acl_link_in_sniffmode_flg = false;

        if (p_link->roleswitch_check_after_unsniff_flg)
        {
            p_link->roleswitch_check_after_unsniff_flg = false;

            roleswitch_handle(param->bd_addr, ROLESWITCH_EVENT_LINK_ACTIVE);
        }
#if F_APP_HARMAN_FEATURE_SUPPORT
        //au_connect_idle_to_power_off(ACTIVE_NEED_STOP_COUNT, p_link->id);
#endif
    }
}

void app_stop_reconnect_timer()
{
    APP_PRINT_TRACE1("app_stop_reconnect_timer: connected_num_before_roleswap %d",
                     app_db.connected_num_before_roleswap);
    gap_stop_timer(&timer_handle_reconnect);
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static void engage_bat_state(uint8_t *bat, uint8_t *state)
{
    if (*bat == 0)
    {
        *bat = app_db.local_batt_level;
    }
    *state = 0x66;
}

static void startup_event_handle(T_STARTUP_PARAM *param)
{
    T_ENGAGE_CFG cfg;

    if (NULL == param)
    {
        return;
    }


    ind_fun = param->ind_fun;

    radio_mode = BT_DEVICE_MODE_IDLE;

    bp_state = BP_STATE_IDLE;
    pri_bp_state = BP_STATE_IDLE;

    dedicated_enter_pairing_mode_flag = false;
    startup_linkback_done_flag = false;
    bond_list_load_flag = false;
    linkback_flag = false;
    is_user_action_pairing_mode = false;
    last_src_conn_idx = 0;
    disconnect_for_pairing_mode = false;
    linkback_retry_timeout = app_cfg_const.timer_linkback_timeout;
    after_stop_sdp_todo_linkback_run_flag = false;
    discoverable_when_one_link = false;

    timer_handle_shutdown_step = NULL;
    timer_handle_first_engage_check = NULL;
    timer_handle_pairing_mode = NULL;
    timer_handle_discoverable = NULL;
    timer_handle_bud_linklost = NULL;
    timer_handle_linkback = NULL;
    timer_handle_linkback_delay = NULL;
    timer_handle_wait_coupling = NULL;
    timer_handle_engage_action_adjust = NULL;
    timer_handle_page_scan_param = NULL;

    original_bud_role = 0;
    rws_link_lost = false;

    src_conn_ind_init();

    enter_state(STATE_STARTUP, BT_DEVICE_MODE_IDLE);

#if F_APP_ANC_SUPPORT
    if (anc_tool_check_resp_meas_mode() == ANC_RESP_MEAS_MODE_NONE)
#endif
    {
        memcpy(app_cfg_nv.bud_local_addr, app_db.factory_addr, 6);
        memcpy(app_cfg_nv.bud_peer_addr, app_cfg_nv.bud_peer_factory_addr, 6);
    }

    app_cfg_nv.bud_role = app_cfg_const.bud_role;
    set_bd_addr();

    memcpy(old_peer_factory_addr, app_cfg_nv.bud_peer_factory_addr, 6);

    cfg.const_bud_role           = app_cfg_const.bud_role;
    cfg.nv_first_engaged         = &app_cfg_nv.first_engaged;
    cfg.nv_bud_role              = &app_cfg_nv.bud_role;
    cfg.nv_bud_local_addr        = app_cfg_nv.bud_local_addr;
    cfg.nv_bud_peer_addr         = app_cfg_nv.bud_peer_addr;
    cfg.nv_bud_peer_factory_addr = app_cfg_nv.bud_peer_factory_addr;
    cfg.factory_addr             = app_db.factory_addr;
    cfg.const_stop_adv_cause     = APP_STOP_ADV_CAUSE_ENGAGE;
    cfg.const_fe_mp_rssi         = (0 - app_cfg_const.rws_pairing_required_rssi);
    cfg.const_rws_custom_uuid    = app_cfg_const.rws_custom_uuid;
    cfg.const_high_batt_pri      = app_cfg_const.enable_high_batt_primary;
    cfg.const_addition           = app_db.jig_dongle_id;
    engage_on(&cfg, engage_ind, engage_bat_state, &app_db.remote_bat_vol);

    b2b_connected_init();
    b2s_connected_init();

    linkback_todo_queue_init();
    linkback_active_node_init();

    if (param->at_once_trigger)
    {
        engage_sched();
    }
}

void enter_dut_test_mode_event_handle(void)
{
    enter_state(STATE_DUT_TEST_MODE, BT_DEVICE_MODE_IDLE);
}

#if (F_APP_OTA_TOOLING_SUPPORT == 1)
static void start_ota_shaking_event_handle(void)
{
    enter_state(STATE_OTA_SHAKING, BT_DEVICE_MODE_IDLE);
}

static void enter_ota_mode_event_handle(T_BT_PARAM *param)
{
    if ((app_cfg_nv.bud_role == REMOTE_SESSION_ROLE_SECONDARY) &&
        ((app_db.jig_subcmd == APP_ADP_SPECIAL_CMD_OTA) ||
         (app_db.jig_subcmd == APP_ADP_SPECIAL_CMD_FACTORY_MODE)))
    {
        // Secondary role should be transer to Primary role during OTA TOOLING
        engage_afe_change_role(REMOTE_SESSION_ROLE_PRIMARY);
        set_bd_addr();
        event_ind(BP_EVENT_ROLE_DECIDED, NULL);
    }

    if (param->is_connectable)
    {
        if (app_db.jig_subcmd == APP_ADP_SPECIAL_CMD_FACTORY_MODE)
        {
            enter_state(STATE_OTA_MODE, BT_DEVICE_MODE_DISCOVERABLE_CONNECTABLE);
        }
        else
        {
            enter_state(STATE_OTA_MODE, BT_DEVICE_MODE_CONNECTABLE);
        }
    }
    else
    {
        enter_state(STATE_OTA_MODE, BT_DEVICE_MODE_IDLE);
    }
}
#endif

static void shutdown_step_handle(void)
{
    uint32_t i;

    shutdown_step_info(shutdown_step);

    switch (shutdown_step)
    {
    case SHUTDOWN_STEP_START_DISCONN_B2S_PROFILE:
        {
            if (app_cfg_nv.bud_role != REMOTE_SESSION_ROLE_SECONDARY)
            {
                for (i = 0; i < MAX_BR_LINK_NUM; i++)
                {
                    if (app_check_b2s_link_by_id(i))
                    {
                        app_db.br_link[i].disconn_acl_flg = false;
                        linkback_profile_disconnect_start(app_db.br_link[i].bd_addr, app_db.br_link[i].connected_profile);
                    }
                }
            }
            shutdown_step = SHUTDOWN_STEP_WAIT_DISCONN_B2S_PROFILE;
            shutdown_step_retry_cnt = 0;
        }
        break;

    case SHUTDOWN_STEP_WAIT_DISCONN_B2S_PROFILE:
        {
            if (++shutdown_step_retry_cnt >= DISCONN_B2S_PROFILE_WAIT_TIMES)
            {
                shutdown_step = SHUTDOWN_STEP_START_DISCONN_B2S_LINK;
            }
        }
        break;

    case SHUTDOWN_STEP_START_DISCONN_B2S_LINK:
        {
            for (i = 0; i < MAX_BR_LINK_NUM; i++)
            {
                if (app_check_b2s_link_by_id(i))
                {
                    if (app_cfg_nv.bud_role != REMOTE_SESSION_ROLE_SECONDARY)
                    {
                        if (app_db.br_link[i].sco_handle)
                        {
                            legacy_send_sco_disconn_req(app_db.br_link[i].bd_addr);
                        }
                        legacy_send_acl_disconn_req(app_db.br_link[i].bd_addr);
                    }
                    else
                    {
                        bt_sniffing_link_disconnect(app_db.br_link[i].bd_addr);
                    }
                }
            }
            shutdown_step = SHUTDOWN_STEP_WAIT_DISCONN_B2S_LINK;
            shutdown_step_retry_cnt = 0;
        }
        break;

    case SHUTDOWN_STEP_WAIT_DISCONN_B2S_LINK:
        {
            ++shutdown_step_retry_cnt;
            if ((0 == b2s_connected.num) || shutdown_step_retry_cnt >= DISCONN_B2S_LINK_WAIT_TIMES_MAX)
            {
                if (app_cfg_nv.bud_role != REMOTE_SESSION_ROLE_SINGLE)
                {
                    shutdown_step = SHUTDOWN_STEP_START_DISCONN_B2B_LINK;
                }
                else
                {
                    shutdown_step = SHUTDOWN_STEP_END;
                }
            }
        }
        break;

    case SHUTDOWN_STEP_START_DISCONN_B2B_LINK:
        {
            if (b2b_connected)
            {
                legacy_send_acl_disconn_req(app_cfg_nv.bud_peer_addr);

                shutdown_step = SHUTDOWN_STEP_WAIT_DISCONN_B2B_LINK;
                shutdown_step_retry_cnt = 0;
            }
            else
            {
                shutdown_step = SHUTDOWN_STEP_END;
            }
        }
        break;

    case SHUTDOWN_STEP_WAIT_DISCONN_B2B_LINK:
        {
            ++shutdown_step_retry_cnt;

            if (!b2b_connected || shutdown_step_retry_cnt >= DISCONN_B2B_LINK_WAIT_TIMES_MAX)
            {
                shutdown_step = SHUTDOWN_STEP_END;
            }
        }
        break;

    default:
        break;
    }

    if (SHUTDOWN_STEP_END == shutdown_step)
    {
        enter_state(STATE_SHUTDOWN, BT_DEVICE_MODE_IDLE);
    }
    else
    {
        gap_start_timer(&timer_handle_shutdown_step, "shutdown_step", timer_queue_id,
                        TIMER_ID_SHUTDOWN_STEP, 0, SHUT_DOWN_STEP_TIMER_MS);
    }
}

static void stop_all_active_action(void)
{
    uint8_t *bd_addr;

    gap_stop_timer(&timer_handle_first_engage_check);
    gap_stop_timer(&timer_handle_pairing_mode);
    gap_stop_timer(&timer_handle_discoverable);
    gap_stop_timer(&timer_handle_bud_linklost);
    gap_stop_timer(&timer_handle_linkback);
    gap_stop_timer(&timer_handle_linkback_delay);
    gap_stop_timer(&timer_handle_wait_coupling);
    gap_stop_timer(&timer_handle_engage_action_adjust);
    gap_stop_timer(&timer_handle_page_scan_param);

    if (!engage_done)
    {
        remote_session_close(app_cfg_nv.bud_peer_addr);
    }

    if (linkback_active_node.is_valid)
    {
        bd_addr = linkback_active_node.linkback_node.bd_addr;

        after_stop_sdp_todo_linkback_run_flag = false;
        legacy_stop_sdp_discov(bd_addr);
        if (!b2s_connected_find_node(bd_addr, NULL))
        {
            legacy_send_acl_disconn_req(bd_addr);
        }

        linkback_active_node_use_left_time_insert_to_queue_again(false);

        linkback_active_node.is_valid = false;
    }
}

static void shutdown_event_handle(void)
{
    stop_all_active_action();

#if F_APP_ERWS_SUPPORT
    relay_pri_bp_state();
    relay_pri_linkback_node();
#endif

    if (cur_state == STATE_FE_SHAKING || cur_state == STATE_FE_COUPLING)
    {
        memcpy(app_cfg_nv.bud_peer_addr, old_peer_factory_addr, 6);
        memcpy(app_cfg_nv.bud_peer_factory_addr, old_peer_factory_addr, 6);

        if (PEER_VALID_MAGIC == app_cfg_nv.peer_valid_magic)
        {
            app_cfg_nv.first_engaged = 1;
        }

        save_engage_related_nv();
    }

    enter_state(STATE_SHUTDOWN_STEP, BT_DEVICE_MODE_IDLE);

    if (!b2b_connected && (b2s_connected.num == 0))
    {
        shutdown_step = SHUTDOWN_STEP_END;
    }
    else
    {
        shutdown_step = SHUTDOWN_STEP_START_DISCONN_B2S_PROFILE;
    }
    shutdown_step_handle();
}

static void stop_event_handle(void)
{
    stop_all_active_action();

    enter_state(STATE_STOP, BT_DEVICE_MODE_IDLE);
}

static void restore_event_handle(void)
{
    if (cur_state == STATE_STOP)
    {
        startup_linkback_done_flag = false;
        bond_list_load_flag = false;

        linkback_todo_queue_init();
        linkback_active_node_init();

        engage_sched();
    }
}

static void state_shutdown_step_event_handle(T_EVENT event)
{
    switch (event)
    {
    case EVENT_SHUTDOWN_STEP_TIMEOUT:
        {
            shutdown_step_handle();
        }
        break;

    default:
        break;
    }
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static void legacy_set_page_scan_param(uint16_t interval, uint16_t window)
{
    ENGAGE_PRINT_TRACE2("legacy_set_page_scan_param: interval 0x%x, window 0x%x",
                        interval, window);

    gap_stop_timer(&timer_handle_page_scan_param);

    legacy_cfg_page_scan_param(GAP_PAGE_SCAN_TYPE_INTERLACED, interval, window);
}

static void primary_page_scan_param_adjust(void)
{
    if (app_cfg_nv.bud_role == REMOTE_SESSION_ROLE_PRIMARY)
    {
        if (engage_done)
        {
#if GFPS_FEATURE_SUPPORT
#if GFPS_SASS_SUPPORT
            if (extend_app_cfg_const.gfps_support)
            {
                legacy_set_page_scan_param(SASS_PAGESCAN_INTERVAL, SASS_PAGESCAN_WINDOW);
            }
            else
#endif
#endif
            {
                legacy_set_page_scan_param(NORMAL_PAGESCAN_INTERVAL, NORMAL_PAGESCAN_WINDOW);
            }
        }
        else
        {
            legacy_set_page_scan_param(PRI_FAST_PAGESCAN_INTERVAL, PRI_FAST_PAGESCAN_WINDOW);

            gap_start_timer(&timer_handle_page_scan_param, "page_scan_param", timer_queue_id,
                            TIMER_ID_PAGESCAN_PARAM_TIMEOUT, 0, 30 * 1000);
        }
    }
}

static void save_engage_related_nv(void)
{
    app_cfg_store(&app_cfg_nv.le_single_random_addr[4], 4);
    app_cfg_store(&app_cfg_nv.bud_local_addr[0], 20);
}

static void fe_timeout_handle(bool is_coupling)
{
    app_auto_power_off_enable(AUTO_POWER_OFF_MASK_BUD_COUPLING, app_cfg_const.timer_auto_power_off);

    if (is_coupling)
    {
        memcpy(app_cfg_nv.bud_peer_addr, old_peer_factory_addr, 6);
        memcpy(app_cfg_nv.bud_peer_factory_addr, old_peer_factory_addr, 6);

        if (app_cfg_const.bud_role == REMOTE_SESSION_ROLE_PRIMARY)
        {
            remote_session_close(app_cfg_nv.bud_peer_addr);
        }
    }

    if (app_cfg_const.bud_role == REMOTE_SESSION_ROLE_SECONDARY)
    {
        legacy_set_page_scan_param(NORMAL_PAGESCAN_INTERVAL, NORMAL_PAGESCAN_WINDOW);
    }

    if (PEER_VALID_MAGIC == app_cfg_nv.peer_valid_magic)
    {
        app_cfg_nv.first_engaged = 1;
        save_engage_related_nv();
        modify_white_list();

        engage_sched();
    }
    else
    {
        engage_afe_done();
        engage_done = true;

        if (app_cfg_const.still_linkback_if_first_engage_fail)
        {
            if (app_cfg_const.bud_role == REMOTE_SESSION_ROLE_SECONDARY)
            {
                engage_afe_change_role(REMOTE_SESSION_ROLE_PRIMARY);
                set_bd_addr();
                event_ind(BP_EVENT_ROLE_DECIDED, NULL);
            }

            linkback_sched();
        }
        else
        {
            stable_sched(STABLE_ENTER_MODE_DIRECT_PAIRING);
        }
    }
}

static void state_fe_shaking_event_handle(T_EVENT event)
{
    switch (event)
    {
    case EVENT_FE_TIMEOUT:
        {
            fe_timeout_handle(false);
        }
        break;

    case EVENT_FE_SHAKING_DONE:
        {
            set_bd_addr();

            if (app_cfg_const.bud_role == REMOTE_SESSION_ROLE_PRIMARY)
            {
                remote_session_open(app_cfg_nv.bud_peer_addr);
                event_ind(BP_EVENT_BUD_CONN_START, NULL);
                enter_state(STATE_FE_COUPLING, BT_DEVICE_MODE_IDLE);
            }
            else
            {
                legacy_set_page_scan_param(SEC_FAST_PAGESCAN_INTERVAL, SEC_FAST_PAGESCAN_WINDOW);
                enter_state(STATE_FE_COUPLING, BT_DEVICE_MODE_CONNECTABLE);
            }

            event_ind(BP_EVENT_ROLE_DECIDED, NULL);
        }
        break;

    default:
        break;
    }
}

static void state_fe_coupling_event_handle(T_EVENT event)
{
    switch (event)
    {
    case EVENT_FE_TIMEOUT:
        {
            if (!b2b_connected)
            {
                fe_timeout_handle(true);
            }
            else
            {
                gap_start_timer(&timer_handle_first_engage_check, "first_engage_check", timer_queue_id,
                                TIMER_ID_FIRST_ENGAGE_CHECK, 0, 2000);
            }
        }
        break;

    case EVENT_BUD_CONN_SUC:
        {
            bt_bond_delete(app_cfg_nv.bud_peer_addr);
        }
        break;

    case EVENT_BUD_CONN_FAIL:
    case EVENT_BUD_AUTH_FAIL:
        {
            if (app_cfg_const.bud_role == REMOTE_SESSION_ROLE_PRIMARY)
            {
                remote_session_open(app_cfg_nv.bud_peer_addr);
            }
        }
        break;

    case EVENT_BUD_REMOTE_CONN_CMPL:
        {
            gap_stop_timer(&timer_handle_first_engage_check);

            if (app_cfg_const.bud_role == REMOTE_SESSION_ROLE_SECONDARY)
            {
                legacy_set_page_scan_param(NORMAL_PAGESCAN_INTERVAL, NORMAL_PAGESCAN_WINDOW);
            }

            if (memcmp(app_cfg_nv.bud_peer_factory_addr, old_peer_factory_addr, 6))
            {
                bt_bond_delete(old_peer_factory_addr);
            }

            app_cfg_nv.peer_valid_magic = PEER_VALID_MAGIC;
            app_cfg_nv.first_engaged = 1;
            save_engage_related_nv();
            modify_white_list();

            stable_sched(STABLE_ENTER_MODE_NORMAL);
        }
        break;

    default:
        break;
    }
}

static void shaking_done(bool is_shaking_to)
{
    set_bd_addr();

    event_ind(BP_EVENT_ROLE_DECIDED, NULL);

    if (app_cfg_nv.bud_role == REMOTE_SESSION_ROLE_SECONDARY)
    {
        startup_linkback_done_flag = false;
        bond_list_load_flag = false;

        gap_stop_timer(&timer_handle_linkback);
        dedicated_enter_pairing_mode_flag = false;

        enter_state(STATE_AFE_COUPLING, BT_DEVICE_MODE_IDLE);

        remote_session_open(app_cfg_nv.bud_peer_addr);

        event_ind(BP_EVENT_BUD_CONN_START, NULL);
    }
    else if (app_cfg_nv.bud_role == REMOTE_SESSION_ROLE_PRIMARY)
    {
        if (is_shaking_to)
        {
            primary_page_scan_param_adjust();

            linkback_sched();
        }
        else
        {
            legacy_set_page_scan_param(PRI_VERY_FAST_PAGESCAN_INTERVAL, PRI_VERY_FAST_PAGESCAN_WINDOW);

            enter_state(STATE_AFE_WAIT_COUPLING, BT_DEVICE_MODE_CONNECTABLE);

            gap_stop_timer(&timer_handle_wait_coupling);
            gap_start_timer(&timer_handle_wait_coupling, "wait_coupling", timer_queue_id,
                            TIMER_ID_WAIT_COUPLING, 0, PRIMARY_WAIT_COUPLING_TIMEOUT_MS);
        }
    }
}

static void state_afe_timeout_shaking_event_handle(T_EVENT event, T_BT_PARAM *param)
{
    switch (event)
    {
    case EVENT_BUD_CONN_SUC:
        {
            linkback_sched();
        }
        break;

    case EVENT_AFE_SHAKING_DONE:
        {
            shaking_done(param->is_shaking_to);
        }
        break;

    default:
        break;
    }
}

static void state_afe_coupling_event_handle(T_EVENT event)
{
    switch (event)
    {
    case EVENT_BUD_CONN_FAIL:
    case EVENT_BUD_AUTH_FAIL:
    case EVENT_BUD_DISCONN_LOCAL:
    case EVENT_BUD_DISCONN_NORMAL:
        {
#if (F_APP_OTA_TOOLING_SUPPORT == 1)
            if (app_db.ota_tooling_start)
            {
                enter_state(STATE_OTA_SHAKING, BT_DEVICE_MODE_IDLE);
            }
            else
#endif
            {
                enter_state(STATE_AFE_TIMEOUT_SHAKING, BT_DEVICE_MODE_IDLE);
            }
        }
        break;

    case EVENT_BUD_AUTH_SUC:
        {
#if (F_APP_OTA_TOOLING_SUPPORT == 1)
            if (!app_db.ota_tooling_start)
#endif
            {
                stable_sched(STABLE_ENTER_MODE_NORMAL);
            }
        }
        break;

    default:
        break;
    }
}


static void state_afe_wait_coupling_event_handle(T_EVENT event)
{
    switch (event)
    {
    case EVENT_BUD_CONN_SUC:
        {
            gap_stop_timer(&timer_handle_wait_coupling);

#if (F_APP_OTA_TOOLING_SUPPORT == 1)
            if (!app_db.ota_tooling_start)
#endif
            {
                linkback_sched();
            }
        }
        break;

    case EVENT_BUD_WAIT_COUPLING_TO:
        {
#if (F_APP_OTA_TOOLING_SUPPORT == 1)
            if (app_db.ota_tooling_start)
            {
                enter_state(STATE_OTA_SHAKING, BT_DEVICE_MODE_IDLE);
            }
            else
#endif
            {
                primary_page_scan_param_adjust();

                linkback_sched();
            }
        }
        break;

    default:
        break;
    }
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static void state_afe_linkback_event_handle(T_EVENT event, T_BT_PARAM *param)
{
    uint8_t *bd_addr;

    if (!param->not_check_addr_flag)
    {
        if (!linkback_active_node_judge_cur_conn_addr(param->bd_addr))
        {
            return;
        }
    }

    switch (event)
    {
    case EVENT_SRC_CONN_FAIL:
        {
            if (0 == linkback_todo_queue_node_num())
            {
                linkback_active_node_src_conn_fail_adjust_remain_profs();

                if (rws_link_lost && b2s_connected_is_empty())
                {
                    engage_sched();
                }
                else
                {
                    linkback_run();
                }
            }
            else
            {
                if (rws_link_lost && b2s_connected_is_empty())
                {
                    engage_sched();
                }
                else
                {
                    gap_stop_timer(&timer_handle_linkback);

                    linkback_active_node_use_left_time_insert_to_queue_again(true);

                    linkback_active_node.is_valid = false;

                    linkback_run();
                }
            }
        }
        break;

    case EVENT_SRC_CONN_FAIL_ACL_EXIST:
        {
            gap_stop_timer(&timer_handle_linkback_delay);
            gap_start_timer(&timer_handle_linkback_delay, "linkback_delay", timer_queue_id,
                            TIMER_ID_LINKBACK_DELAY, event, 1300);
        }
        break;

    case EVENT_SRC_CONN_FAIL_CONTROLLER_BUSY:
        {
            gap_stop_timer(&timer_handle_linkback_delay);
            gap_start_timer(&timer_handle_linkback_delay, "linkback_delay", timer_queue_id,
                            TIMER_ID_LINKBACK_DELAY, event, 500);
        }
        break;

    case EVENT_LINKBACK_DELAY_TIMEOUT:
        {
            linkback_active_node_src_conn_fail_adjust_remain_profs();

            if (EVENT_SRC_CONN_FAIL_ACL_EXIST == param->cause)
            {
                if (!engage_done && b2s_connected_is_empty())
                {
                    engage_sched();
                }
                else
                {
                    linkback_run();
                }
            }
            else
            {
                linkback_run();
            }
        }
        break;

    case EVENT_SRC_AUTH_SUC:
        {
            if (!linkback_active_node_judge_cur_conn_addr(param->bd_addr))
            {
                if (b2s_connected_is_full())
                {
                    if (linkback_active_node.is_valid)
                    {
                        bd_addr = linkback_active_node.linkback_node.bd_addr;

                        if (!b2s_connected_find_node(bd_addr, NULL))
                        {
                            ENGAGE_PRINT_TRACE0("state_afe_linkback_event_handle: bring forward the linkback timout");

                            timer_cback(TIMER_ID_LINKBACK_TIMEOUT, 0);
                        }
                    }
                }
            }
        }
        break;

    case EVENT_SRC_AUTH_FAIL:
        {
            ENGAGE_PRINT_TRACE0("state_afe_linkback_event_handle: wait EVENT_PROFILE_CONN_FAIL");
        }
        break;

    case EVENT_SRC_DISCONN_LOST:
    case EVENT_SRC_DISCONN_NORMAL:
    case EVENT_SRC_DISCONN_ROLESWAP:
        {
            linkback_active_node.is_exit = true;

            linkback_run();
        }
        break;

    case EVENT_SRC_CONN_TIMEOUT:
        {
            if (linkback_active_node.is_valid)
            {
                bd_addr = linkback_active_node.linkback_node.bd_addr;

                if (!memcmp(bd_addr, app_db.resume_addr, 6))
                {
                    memset(app_db.resume_addr, 0, 6);
                    app_audio_set_connected_tone_need(false);
                }

                if (!b2s_connected_find_node(bd_addr, NULL))
                {
                    linkback_active_node.is_valid = false;

                    after_stop_sdp_todo_linkback_run_flag = true;
                    legacy_stop_sdp_discov(bd_addr);
                    legacy_send_acl_disconn_req(bd_addr);
                }
                else
                {
                    linkback_active_node.linkback_node.plan_profs = 0;
                }
            }
            else
            {
                linkback_run();
            }
        }
        break;

    case EVENT_PROFILE_SDP_ATTR_INFO:
        {
            linkback_active_node.linkback_conn_param.protocol_version = param->sdp_info->protocol_version;
            uint8_t server_channel = param->sdp_info->server_channel;

            if (SPP_PROFILE_MASK == linkback_active_node.doing_prof)
            {
                uint8_t local_server_chann;

                if (bt_spp_registered_uuid_check((T_BT_SPP_UUID_TYPE)param->sdp_info->srv_class_uuid_type,
                                                 (T_BT_SPP_UUID_DATA *)(&param->sdp_info->srv_class_uuid_data), &local_server_chann))
                {
                    bool is_sdp_ok = false;

                    if (param->sdp_info->srv_class_uuid_data.uuid_16 == UUID_SERIAL_PORT)
                    {
                        //standard spp
                        if (!linkback_active_node.linkback_conn_param.spp_has_vendor)

                        {
                            //when has no vendor spp attr, standard spp sdp suc
                            is_sdp_ok = true;
                        }
                    }
                    else
                    {
                        //whether has standard spp attr or not, vendor spp sdp always suc
                        linkback_active_node.linkback_conn_param.spp_has_vendor = true;
                        is_sdp_ok = true;
                    }

                    if (is_sdp_ok)
                    {
                        linkback_active_node.is_sdp_ok = true;
                        linkback_active_node.linkback_conn_param.server_channel = server_channel;
                        linkback_active_node.linkback_conn_param.local_server_chann = local_server_chann;
                    }
                }
            }
            else
            {
                linkback_active_node.is_sdp_ok = true;
                linkback_active_node.linkback_conn_param.server_channel = server_channel;

                if (PBAP_PROFILE_MASK == linkback_active_node.doing_prof)
                {
                    linkback_active_node.linkback_conn_param.feature = (param->sdp_info->supported_feat) ? true : false;
                }
            }
        }
        break;

    case EVENT_PROFILE_DID_ATTR_INFO:
        {
            linkback_active_node.is_sdp_ok = true;
        }
        break;

    case EVENT_PROFILE_SDP_DISCOV_CMPL:
        {
            if (param->cause == 0)
            {
                T_APP_BR_LINK *p_link;
                p_link = app_find_br_link(param->bd_addr);
                if (p_link && p_link->acl_link_role == BT_LINK_ROLE_MASTER)
                {
                    linkback_active_node.sdp_cmpl_pending_flag = true;
                    break;
                }
                linkback_active_node.sdp_cmpl_pending_flag = false;
            }
            if ((!b2s_connected_find_node(param->bd_addr, NULL)) ||
                (param->cause == (HCI_ERR | HCI_ERR_REMOTE_USER_TERMINATE)) ||
                (param->cause == (HCI_ERR | HCI_ERR_LOCAL_HOST_TERMINATE)) ||
                (param->cause == (HCI_ERR | HCI_ERR_CONN_TIMEOUT)) ||
                (param->cause == (HCI_ERR | HCI_ERR_LMP_RESPONSE_TIMEOUT)))
            {
                ENGAGE_PRINT_TRACE0("state_afe_linkback_event_handle: wait EVENT_SRC_CONN_FAIL");
            }
            else
            {
                if (linkback_active_node.is_sdp_ok)
                {
                    uint32_t profs = 0;

                    b2s_connected_find_node(param->bd_addr, &profs);

                    if ((DID_PROFILE_MASK == linkback_active_node.doing_prof) ||
                        (profs & linkback_active_node.doing_prof))
                    {
                        linkback_active_node_step_suc_adjust_remain_profs();
                        linkback_run();
                    }
                    else
                    {
                        if (linkback_profile_connect_start(linkback_active_node.linkback_node.bd_addr,
                                                           linkback_active_node.doing_prof, &linkback_active_node.linkback_conn_param))
                        {
                            if (PBAP_PROFILE_MASK == linkback_active_node.doing_prof)
                            {
                                linkback_active_node_step_suc_adjust_remain_profs();
                                linkback_run();
                            }
                        }
                        else
                        {
                            linkback_active_node_step_fail_adjust_remain_profs();
                            linkback_run();
                        }
                    }
                }
                else
                {
                    linkback_active_node_step_fail_adjust_remain_profs();
                    linkback_run();
                }
            }
        }
        break;

    case EVENT_PROFILE_SDP_DISCOV_STOP:
        {
            if (after_stop_sdp_todo_linkback_run_flag)
            {
                after_stop_sdp_todo_linkback_run_flag = false;

                linkback_run();
            }
        }
        break;

    case EVENT_PROFILE_CONN_SUC:
        {
            if (linkback_active_node_judge_cur_conn_prof(param->bd_addr, param->prof))
            {
                T_APP_BR_LINK *p_link = app_find_b2s_link(param->bd_addr);
                p_link->connected_by_linkback = true;
                linkback_active_node_step_suc_adjust_remain_profs();
                linkback_run();
            }
        }
        break;

    case EVENT_PROFILE_CONN_FAIL:
        {
            ENGAGE_PRINT_TRACE1("state_afe_linkback_event_handle: EVENT_PROFILE_CONN_FAIL cause %x",
                                param->cause);
            if ((param->cause == (HCI_ERR | HCI_ERR_REMOTE_USER_TERMINATE)) ||
                (param->cause == (HCI_ERR | HCI_ERR_CONN_TIMEOUT)) ||
                (param->cause == (HCI_ERR | HCI_ERR_LMP_RESPONSE_TIMEOUT)))
            {
            }
            else if (param->cause == (L2C_ERR | L2C_ERR_SECURITY_BLOCK))
            {
                // ENGAGE_PRINT_TRACE0("state_afe_linkback_event_handle: L2C_ERR_SECURITY_BLOCK");

                legacy_send_acl_disconn_req(param->bd_addr);
            }
            else if (param->cause == (L2C_ERR | L2C_ERR_NO_RESOURCE))
            {
                // ENGAGE_PRINT_TRACE0("state_afe_linkback_event_handle: L2C_ERR_NO_RESOURCE");

                gap_stop_timer(&timer_handle_linkback_delay);
                gap_start_timer(&timer_handle_linkback_delay, "linkback_delay", timer_queue_id,
                                TIMER_ID_LINKBACK_DELAY, event, 500);
            }
            else
            {
                if (linkback_active_node_judge_cur_conn_prof(param->bd_addr, param->prof))
                {
                    linkback_active_node_step_fail_adjust_remain_profs();
                    linkback_run();
                }
            }
        }
        break;

    default:
        break;
    }
}

static void state_afe_stable_event_handle(T_EVENT event)
{
    if (app_cfg_nv.bud_role == REMOTE_SESSION_ROLE_SECONDARY)
    {
        switch (event)
        {
        case EVENT_BUD_AUTH_SUC:
            {
                enter_state(STATE_AFE_SECONDARY, BT_DEVICE_MODE_IDLE);
            }
            break;

        case EVENT_BUD_DISCONN_LOCAL:
            {
                enter_state(STATE_AFE_TIMEOUT_SHAKING, BT_DEVICE_MODE_IDLE);
            }
            break;

        case EVENT_BUD_DISCONN_NORMAL:
            {
                linkback_retry_timeout = app_cfg_const.timer_linkback_timeout;

                if (pri_bp_state == BP_STATE_LINKBACK)
                {
                    original_bud_role = REMOTE_SESSION_ROLE_PRIMARY;

                    engage_afe_change_role(REMOTE_SESSION_ROLE_PRIMARY);
                    set_bd_addr();
                    event_ind(BP_EVENT_ROLE_DECIDED, NULL);

                    linkback_sched();
                }
                else if (BP_STATE_PAIRING_MODE == pri_bp_state ||
                         BP_STATE_STANDBY == pri_bp_state)
                {
                    original_bud_role = REMOTE_SESSION_ROLE_PRIMARY;

                    engage_afe_change_role(REMOTE_SESSION_ROLE_PRIMARY);
                    set_bd_addr();
                    event_ind(BP_EVENT_ROLE_DECIDED, NULL);

                    is_visiable_flag = false;
                    if (BP_STATE_PAIRING_MODE == pri_bp_state)
                    {
                        is_visiable_flag = true;
                    }
                    stable_sched(STABLE_ENTER_MODE_DEDICATED_PAIRING);
                }
                else
                {
                    enter_state(STATE_AFE_TIMEOUT_SHAKING, BT_DEVICE_MODE_IDLE);
                }
            }
            break;

        case EVENT_BUD_DISCONN_LOST:
            {
                linkback_retry_timeout = app_cfg_const.timer_link_back_loss;

                enter_state(STATE_AFE_TIMEOUT_SHAKING, BT_DEVICE_MODE_IDLE);
            }
            break;

        case EVENT_AFE_SHAKING_DONE:
            {
                set_bd_addr();
                event_ind(BP_EVENT_ROLE_DECIDED, NULL);

                enter_state(STATE_AFE_COUPLING, BT_DEVICE_MODE_IDLE);

                remote_session_open(app_cfg_nv.bud_peer_addr);
                event_ind(BP_EVENT_BUD_CONN_START, NULL);
            }
            break;

        default:
            break;
        }
    }
    else
    {
        switch (event)
        {
        case EVENT_BUD_AUTH_SUC:
        case EVENT_BUD_DISCONN_LOCAL:
        case EVENT_BUD_DISCONN_NORMAL:
        case EVENT_BUD_DISCONN_LOST:
        case EVENT_SRC_AUTH_SUC:
            {
                stable_sched(STABLE_ENTER_MODE_AGAIN);
            }
            break;

        case EVENT_SRC_DISCONN_NORMAL:
            {
                if (is_src_authed || (cur_state == STATE_AFE_CONNECTED && b2s_connected_is_empty()))
                {
                    is_src_authed = false;

                    stable_sched(STABLE_ENTER_MODE_AGAIN);
                }
            }
            break;

        case EVENT_SRC_DISCONN_LOST:
            {
                enter_state(STATE_AFE_LINKBACK, BT_DEVICE_MODE_CONNECTABLE);
            }
            break;

        case EVENT_DISCOVERABLE_TIMEOUT:
            {
                stable_sched(STABLE_ENTER_MODE_AGAIN);
            }
            break;

        case EVENT_AFE_SHAKING_DONE:
            {
                event_ind(BP_EVENT_ROLE_DECIDED, NULL);

                gap_stop_timer(&timer_handle_engage_action_adjust);
                gap_start_timer(&timer_handle_engage_action_adjust, "engage_action_adjust", timer_queue_id,
                                TIMER_ID_ENGAGE_ACTION_ADJUST, 0, 2500);
            }
            break;

        case EVENT_BUD_ENGAGE_ACTION_ADJUST:
            {
                app_bt_policy_primary_engage_action_adjust();
            }
            break;

        default:
            break;
        }
    }
}

static void state_dut_test_mode_event_handle(T_EVENT event)
{
    switch (event)
    {
    case EVENT_SRC_CONN_SUC:
        {
            enter_state(STATE_DUT_TEST_MODE, BT_DEVICE_MODE_IDLE);
        }
        break;

    case EVENT_SRC_DISCONN_NORMAL:
    case EVENT_SRC_DISCONN_LOST:
        {
            enter_state(STATE_DUT_TEST_MODE, BT_DEVICE_MODE_CONNECTABLE);
        }
        break;

    default:
        break;
    }
}

#if (F_APP_OTA_TOOLING_SUPPORT == 1)
static void state_ota_shaking_event_handle(T_EVENT event, T_BT_PARAM *param)
{
    switch (event)
    {
    case EVENT_AFE_SHAKING_DONE:
        {
            shaking_done(param->is_shaking_to);
        }
        break;

    default:
        break;
    }
}

static void state_ota_mode_event_handle(T_EVENT event)
{
    switch (event)
    {
    case EVENT_BUD_AUTH_SUC:
        {
            app_cfg_nv.peer_valid_magic = PEER_VALID_MAGIC;
            app_cfg_nv.first_engaged = 1;
            save_engage_related_nv();
            modify_white_list();

            stable_sched(STABLE_ENTER_MODE_NORMAL);
        }
        break;

    default:
        break;
    }
}
#endif

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static bool judge_dedicated_enter_pairing_mode(void)
{
    bool ret = true;

    if (!is_force_flag)
    {
        if (b2s_connected_is_full())
        {
            ret = false;
        }
    }

    ENGAGE_PRINT_TRACE1("judge_dedicated_enter_pairing_mode: ret %d", ret);

    return ret;
}

static void prepare_for_dedicated_enter_pairing_mode(void)
{
    ENGAGE_PRINT_TRACE0("prepare_for_dedicated_enter_pairing_mode");

    disconnect_for_pairing_mode = false;

    if (!b2s_connected_is_full())
    {
#if (F_APP_AVP_INIT_SUPPORT == 1)
        if (!app_avp_cmd_multilink_is_on())
        {
            for (uint8_t i = 0; i < MAX_BR_LINK_NUM; i++)
            {
                if (app_check_b2s_link_by_id(i))
                {
                    app_bt_policy_disconnect(app_db.br_link[i].bd_addr, ALL_PROFILE_MASK);
                    disconnect_for_pairing_mode = true;
                }
            }
        }
#endif

        if (!disconnect_for_pairing_mode)
        {
            stable_sched(STABLE_ENTER_MODE_DEDICATED_PAIRING);
        }
    }
    else
    {
        if (is_force_flag)
        {
            uint8_t active_a2dp_index = app_get_active_a2dp_idx();
#if F_APP_TEAMS_BT_POLICY
            disconnect_for_pairing_mode = app_teams_bt_prepare_for_dedicated_enter_pairing_mode();
            if (disconnect_for_pairing_mode)
            {
                is_force_flag = false;
                return;
            }
#endif

#if (F_APP_AVP_INIT_SUPPORT == 1)
            for (uint8_t i = 0; i < MAX_BR_LINK_NUM; i++)
            {
                if (app_check_b2s_link_by_id(i))
                {
                    app_bt_policy_disconnect(app_db.br_link[i].bd_addr, ALL_PROFILE_MASK);
                    disconnect_for_pairing_mode = true;
                }
            }
#else
            if (((app_db.br_link[active_a2dp_index].avrcp_play_status == BT_AVRCP_PLAY_STATUS_PLAYING)
                 || (app_db.br_link[active_a2dp_index].streaming_fg == true))
                && (app_cfg_const.enable_multi_link))
            {
                for (uint8_t inactive_a2dp_index = 0; inactive_a2dp_index < MAX_BR_LINK_NUM; inactive_a2dp_index++)
                {
                    if (app_check_b2s_link_by_id(inactive_a2dp_index))
                    {
                        if (inactive_a2dp_index != active_a2dp_index)
                        {
#if HARMAN_DISCONN_ACTIVE_A2DP_WHEN_OTA
                            if (app_harman_ble_ota_get_upgrate_status())
                            {
                                for (int i = 0; i < MAX_BLE_LINK_NUM; i++)
                                {
                                    if (app_db.le_link[i].used == true &&
                                        (memcmp(app_db.le_link[i].bd_addr, app_db.br_link[inactive_a2dp_index].bd_addr, 6) == 0))
                                    {
                                        //disconn active a2dp, when inactive a2dp src is in OTA
                                        app_bt_policy_disconnect(app_db.br_link[active_a2dp_index].bd_addr, ALL_PROFILE_MASK);
                                        disconnect_for_pairing_mode = true;
                                        break;
                                    }
                                }
                            }
                            else
#endif
                            {
                                app_bt_policy_disconnect(app_db.br_link[inactive_a2dp_index].bd_addr, ALL_PROFILE_MASK);
                                disconnect_for_pairing_mode = true;
                                break;
                            }
                        }
                    }
                }
            }
            else
            {
                for (uint8_t i = 0; i < MAX_BR_LINK_NUM; i++)
                {
                    if (app_check_b2s_link_by_id(i))
                    {
#if HARMAN_DISCONN_ACTIVE_A2DP_WHEN_OTA
                        if (app_harman_ble_ota_get_upgrate_status())
                        {
                            for (int idx = 0; idx < MAX_BLE_LINK_NUM; idx++)
                            {
                                if (app_db.le_link[idx].used == true &&
                                    (memcmp(app_db.le_link[idx].bd_addr, app_db.br_link[i].bd_addr, 6) != 0))
                                {
                                    //disconn the link which is not in OTA
                                    app_bt_policy_disconnect(app_db.br_link[i].bd_addr, ALL_PROFILE_MASK);
                                    disconnect_for_pairing_mode = true;
                                    break;
                                }
                            }
                        }
                        else
#endif
                        {
                            if (b2s_connected_is_first_src(app_db.br_link[i].bd_addr))
                            {
                                app_bt_policy_disconnect(app_db.br_link[i].bd_addr, ALL_PROFILE_MASK);
                                disconnect_for_pairing_mode = true;
                                break;
                            }
                        }
                    }
                }
            }
#endif
        }
    }
    is_force_flag = false;
}

static void dedicated_enter_pairing_mode_event_handle(T_BT_PARAM *param)
{
    is_visiable_flag = param->is_visiable;
    is_force_flag = param->is_force;

    switch (cur_state)
    {
    case STATE_STARTUP:
        {
            stable_sched(STABLE_ENTER_MODE_DIRECT_PAIRING);
        }
        break;

    case STATE_AFE_LINKBACK:
        {
            if (judge_dedicated_enter_pairing_mode())
            {
                if (!b2s_connected_find_node(linkback_active_node.linkback_node.bd_addr, NULL))
                {
                    gap_stop_timer(&timer_handle_linkback);

                    if (linkback_active_node.is_valid)
                    {
                        after_stop_sdp_todo_linkback_run_flag = false;
                        legacy_stop_sdp_discov(linkback_active_node.linkback_node.bd_addr);
                        legacy_send_acl_disconn_req(linkback_active_node.linkback_node.bd_addr);
                    }

                    linkback_todo_queue_init();
                    linkback_active_node_init();

                    prepare_for_dedicated_enter_pairing_mode();
                }
                else
                {
                    dedicated_enter_pairing_mode_flag = true;
                }
            }
        }
        break;

    case STATE_AFE_CONNECTED:
    case STATE_AFE_PAIRING_MODE:
    case STATE_AFE_STANDBY:
        {
            if (judge_dedicated_enter_pairing_mode())
            {
                prepare_for_dedicated_enter_pairing_mode();
            }
        }
        break;

    default:
        {
            dedicated_enter_pairing_mode_flag = true;
        }
        break;
    }
}

static void dedicated_exit_pairing_mode_event_handle(void)
{
    dedicated_enter_pairing_mode_flag = false;

    switch (cur_state)
    {
    case STATE_AFE_CONNECTED:
    case STATE_AFE_PAIRING_MODE:
    case STATE_AFE_STANDBY:
        {
            stable_sched(STABLE_ENTER_MODE_NOT_PAIRING);
        }
        break;

    default:
        break;
    }
}

static void dedicated_connect_event_handle(T_BT_PARAM *bt_param)
{
    T_APP_BR_LINK *p_link;

    switch (cur_state)
    {
    case STATE_AFE_TIMEOUT_SHAKING:
    case STATE_AFE_WAIT_COUPLING:
        {
            linkback_todo_queue_insert_force_node(bt_param->bd_addr, bt_param->prof, bt_param->is_special,
                                                  bt_param->search_param, bt_param->check_bond_flag, app_cfg_const.timer_linkback_timeout * 1000,
                                                  false, true);
        }
        break;

    case STATE_AFE_LINKBACK:
        {
            if (!bt_param->is_special && linkback_active_node_judge_cur_conn_addr(bt_param->bd_addr))
            {
                linkback_active_node_remain_profs_add(bt_param->prof, bt_param->check_bond_flag,
                                                      app_cfg_const.timer_linkback_timeout * 1000);

                gap_stop_timer(&timer_handle_linkback);
                gap_start_timer(&timer_handle_linkback, "linkback", timer_queue_id,
                                TIMER_ID_LINKBACK_TIMEOUT, 0, linkback_active_node.retry_timeout);
            }
            else
            {
                linkback_todo_queue_insert_force_node(bt_param->bd_addr, bt_param->prof, bt_param->is_special,
                                                      bt_param->search_param, bt_param->check_bond_flag, app_cfg_const.timer_linkback_timeout * 1000,
                                                      false, true);
            }
        }
        break;

    case STATE_AFE_CONNECTED:
    case STATE_AFE_PAIRING_MODE:
    case STATE_AFE_STANDBY:
        {
            if (bt_param->is_later_avrcp)
            {
                p_link = app_find_br_link(bt_param->bd_addr);
                if (p_link != NULL)
                {
                    if (0 == (AVRCP_PROFILE_MASK & p_link->connected_profile))
                    {
                        linkback_profile_connect_start(bt_param->bd_addr, AVRCP_PROFILE_MASK, NULL);
                    }
                }
            }
            else if (DID_PROFILE_MASK == bt_param->prof)
            {
                p_link = app_find_br_link(bt_param->bd_addr);
                if (p_link != NULL)
                {
                    legacy_start_did_discov(bt_param->bd_addr);
                }
            }
            else if (bt_param->is_later_hid)
            {
                p_link = app_find_br_link(bt_param->bd_addr);
                if (p_link != NULL)
                {
                    if (0 == (HID_PROFILE_MASK & p_link->connected_profile))
                    {
                        linkback_profile_connect_start(bt_param->bd_addr, HID_PROFILE_MASK, NULL);
                    }
                }
            }
            else
            {
                linkback_todo_queue_insert_force_node(bt_param->bd_addr, bt_param->prof, bt_param->is_special,
                                                      bt_param->search_param, bt_param->check_bond_flag, app_cfg_const.timer_linkback_timeout * 1000,
                                                      false, true);

                if (app_cfg_nv.bud_role != REMOTE_SESSION_ROLE_SECONDARY)
                {
                    enter_state(STATE_AFE_LINKBACK, BT_DEVICE_MODE_CONNECTABLE);
                }
            }
        }
        break;

    default:
        break;
    }
}

static void dedicated_disconnect_event_handle(T_BT_PARAM *bt_param)
{
    uint8_t *bd_addr;
    uint32_t plan_profs;
    uint32_t exist_profs;
    T_APP_BR_LINK *p_link;

    bd_addr = bt_param->bd_addr;
    plan_profs = bt_param->prof;

    if (linkback_active_node_judge_cur_conn_addr(bd_addr))
    {
        linkback_active_node_remain_profs_sub(plan_profs);
    }

    linkback_todo_queue_remove_plan_profs(bd_addr, plan_profs);

    if (b2s_connected_find_node(bd_addr, &exist_profs))
    {
        plan_profs &= exist_profs;

        p_link = app_find_br_link(bd_addr);

        if (p_link != NULL)
        {
            p_link->disconn_acl_flg = false;
            if (plan_profs == exist_profs)
            {
                p_link->disconn_acl_flg = true;
            }

            linkback_profile_disconnect_start(bd_addr, plan_profs);
        }

    }
}

static void disconnect_all_event_handle(void)
{
    uint32_t i;

    ENGAGE_PRINT_TRACE0("disconnect_all_event_handle");

    if (b2b_connected)
    {
        legacy_send_acl_disconn_req(app_cfg_nv.bud_peer_addr);
    }

    if ((app_cfg_nv.bud_role == REMOTE_SESSION_ROLE_PRIMARY) ||
        (app_cfg_nv.bud_role == REMOTE_SESSION_ROLE_SINGLE))
    {
        for (i = 0; i < MAX_BR_LINK_NUM; i++)
        {
            if (app_check_b2s_link_by_id(i))
            {
                if (app_db.br_link[i].sco_handle)
                {
                    legacy_send_sco_disconn_req(app_db.br_link[i].bd_addr);
                }
                legacy_send_acl_disconn_req(app_db.br_link[i].bd_addr);
            }
        }
    }
}

static void engage_ind(T_ENGAGE_IND ind)
{
    T_BT_PARAM bt_param;

    memset(&bt_param, 0, sizeof(T_BT_PARAM));

    switch (ind)
    {
    case ENGAGE_IND_FE_SHAKING_DONE:
        {
            state_machine(EVENT_FE_SHAKING_DONE, NULL);
#if F_APP_LE_AUDIO_SM
            app_le_audio_device_sm(LE_AUDIO_FE_SHAKING_DONE, NULL);
#endif
        }
        break;

    case ENGAGE_IND_AFE_SHAKING_DONE:
        {
            bt_param.is_shaking_to = false;
            state_machine(EVENT_AFE_SHAKING_DONE, &bt_param);
#if F_APP_LE_AUDIO_SM
            app_le_audio_device_sm(LE_AUDIO_AFE_SHAKING_DONE, NULL);
#endif
        }
        break;

    case ENGAGE_IND_AFE_SHAKING_TO:
        {
            bt_param.is_shaking_to = true;
            state_machine(EVENT_AFE_SHAKING_DONE, &bt_param);
        }
        break;

    default:
        break;
    }
}

static void engage_sched(void)
{
    ENGAGE_PRINT_TRACE0("engage_sched");

    if (!engage_done)
    {
        if (!app_cfg_nv.first_engaged)
        {
            if (PEER_VALID_MAGIC != app_cfg_nv.peer_valid_magic)
            {
                if (app_cfg_const.first_time_engagement_only_by_5v_command)
                {
                    /* first engage in factory */
                    if (app_db.jig_dongle_id == 0 && app_cfg_nv.one_wire_start_force_engage == 0)
                    {
                        ENGAGE_PRINT_TRACE0("disallow first engage not trigger by 5v command");
                        stable_sched(STABLE_ENTER_MODE_DIRECT_PAIRING);
                        return;
                    }
                }
            }

            enter_state(STATE_FE_SHAKING, BT_DEVICE_MODE_IDLE);
        }
        else
        {
            enter_state(STATE_AFE_TIMEOUT_SHAKING, BT_DEVICE_MODE_IDLE);
        }
    }
    else
    {
        linkback_sched();
    }
}

static bool enable_b2s_disconnect_enter_pairing(void)
{
    bool ret = true;

    if (!app_cfg_const.rws_disconnect_enter_pairing ||
        (!app_cfg_const.enable_rtk_charging_box && app_device_is_in_the_box()) ||
        anc_tool_check_resp_meas_mode() != ANC_RESP_MEAS_MODE_NONE ||
        app_ota_reset_check() || app_db.disconnect_inactive_link_actively ||
        ((!b2s_connected_is_empty()) && (!app_cfg_const.enter_pairing_while_only_one_device_connected))
#if F_APP_HARMAN_FEATURE_SUPPORT
        || (au_get_power_on_link_back_fg() == false)
#endif
       )
    {
        ret = false;
    }

    return ret;
}


static void stable_sched(T_STABLE_ENTER_MODE mode)
{
    stable_enter_mode(mode);

    gap_stop_timer(&timer_handle_linkback);

    if (app_cfg_nv.bud_role == REMOTE_SESSION_ROLE_SECONDARY)
    {
        startup_linkback_done_flag = false;
        bond_list_load_flag = false;

        switch (mode)
        {
        case STABLE_ENTER_MODE_NORMAL:
            {
                enter_state(STATE_AFE_SECONDARY, BT_DEVICE_MODE_IDLE);
            }
            break;

        case STABLE_ENTER_MODE_DIRECT_PAIRING:
            {
                engage_afe_change_role(REMOTE_SESSION_ROLE_PRIMARY);
                set_bd_addr();

                enter_state(STATE_AFE_PAIRING_MODE, BT_DEVICE_MODE_DISCOVERABLE_CONNECTABLE);

                event_ind(BP_EVENT_ROLE_DECIDED, NULL);
            }
            break;

        default:
            break;
        }
    }
    else
    {
        startup_linkback_done_flag = true;

        switch (mode)
        {
        case STABLE_ENTER_MODE_NORMAL:
            {
                if (b2s_connected_is_empty())
                {
                    if ((!linkback_flag || app_cfg_const.enable_power_on_linkback_fail_enter_pairing) &&
                        (app_hfp_get_call_status() == BT_HFP_CALL_IDLE)
#if F_APP_HARMAN_FEATURE_SUPPORT
                        && (au_get_power_on_link_back_fg() == false)
#endif
                       )
                    {
                        enter_state(STATE_AFE_PAIRING_MODE, BT_DEVICE_MODE_DISCOVERABLE_CONNECTABLE);
#if (F_APP_AVP_INIT_SUPPORT == 1)
                        if (app_mmi_enter_pairing_hook)
                        {
                            app_mmi_enter_pairing_hook();
                        }
#endif
                    }
                    else
                    {
#if F_APP_HARMAN_FEATURE_SUPPORT
                        app_harman_ble_adv_stop();
#endif
                        enter_state(STATE_AFE_STANDBY, BT_DEVICE_MODE_CONNECTABLE);
                    }
                }
                else
                {
                    if (b2s_connected_is_full() && engage_done)
                    {
                        enter_state(STATE_AFE_CONNECTED, BT_DEVICE_MODE_IDLE);
                    }
                    else
                    {
                        enter_state(STATE_AFE_CONNECTED, BT_DEVICE_MODE_CONNECTABLE);
                    }
                }
                app_bt_policy_qos_param_update(app_db.br_link[app_get_active_a2dp_idx()].bd_addr,
                                               BP_TPOLL_LINKBACK_STOP);
                linkback_flag = false;
            }
            break;

        case STABLE_ENTER_MODE_AGAIN:
            {
                if (b2s_connected_is_empty())
                {
                    if (((cur_state == STATE_AFE_PAIRING_MODE) ||
                         (cur_event == EVENT_SRC_DISCONN_NORMAL && enable_b2s_disconnect_enter_pairing()) ||
                         (cur_event == EVENT_BUD_AUTH_SUC)) &&
                        (app_hfp_get_call_status() == BT_HFP_CALL_IDLE))
                    {
                        if (cur_event == EVENT_SRC_DISCONN_NORMAL && enable_b2s_disconnect_enter_pairing())
                        {
                            is_user_action_pairing_mode = true;
                        }
                        else
                        {
                            is_user_action_pairing_mode = false;
                        }
                        enter_state(STATE_AFE_PAIRING_MODE, BT_DEVICE_MODE_DISCOVERABLE_CONNECTABLE);
                    }
                    else
                    {
                        enter_state(STATE_AFE_STANDBY, BT_DEVICE_MODE_CONNECTABLE);
                    }
                }
                else
                {
                    if (((cur_state == STATE_AFE_PAIRING_MODE && cur_event != EVENT_SRC_AUTH_SUC) ||
                         (cur_event == EVENT_SRC_DISCONN_NORMAL && enable_b2s_disconnect_enter_pairing())) &&
                        (app_hfp_get_call_status() == BT_HFP_CALL_IDLE))
                    {
                        if (cur_event == EVENT_SRC_DISCONN_NORMAL)
                        {
                            is_user_action_pairing_mode = true;
                        }
                        else
                        {
                            is_user_action_pairing_mode = false;
                        }
                        enter_state(STATE_AFE_PAIRING_MODE, BT_DEVICE_MODE_DISCOVERABLE_CONNECTABLE);
                    }
                    else
                    {
                        if (b2s_connected_is_full() && engage_done)
                        {
                            enter_state(STATE_AFE_CONNECTED, BT_DEVICE_MODE_IDLE);
                        }
                        else
                        {
                            enter_state(STATE_AFE_CONNECTED, BT_DEVICE_MODE_CONNECTABLE);
                        }
                    }
                    app_db.disconnect_inactive_link_actively = false;
                }
            }
            break;

        case STABLE_ENTER_MODE_DEDICATED_PAIRING:
            {
                event_ind(BP_EVENT_DEDICATED_PAIRING, NULL);

                is_user_action_pairing_mode = true;

                if (is_visiable_flag)
                {
                    enter_state(STATE_AFE_PAIRING_MODE, BT_DEVICE_MODE_DISCOVERABLE_CONNECTABLE);
                }
                else
                {
                    enter_state(STATE_AFE_STANDBY, BT_DEVICE_MODE_CONNECTABLE);
                }
            }
            break;

        case STABLE_ENTER_MODE_NOT_PAIRING:
            {
#if F_APP_ANC_SUPPORT
                T_ANC_FEATURE_MAP feature_map;
                feature_map.d32 = anc_tool_get_feature_map();
#endif
                if ((is_pairing_timeout
#if F_APP_HARMAN_FEATURE_SUPPORT
                     && (au_ever_link_information() == 0)
#endif
                     && app_cfg_const.enable_pairing_timeout_to_power_off
                    ) &&
#if F_APP_ANC_SUPPORT
                    (feature_map.user_mode == ENABLE) && !app_anc_ramp_tool_is_busy() &&
#endif
#if F_APP_LE_AUDIO_SM
                    (mtc_device_poweroff_check(MTC_EVENT_LEGACY_PAIRING_TO)) &&
#endif
                    b2s_connected_is_empty() &&
                    (app_cfg_const.enable_auto_power_off_when_anc_apt_on
#if F_APP_LISTENING_MODE_SUPPORT
                     || (app_db.current_listening_state == ANC_OFF_APT_OFF)
#endif
                    )
#if F_APP_LINEIN_SUPPORT
                    && !app_line_in_plug_state_get()
#endif
#if F_APP_USB_AUDIO_SUPPORT
                    && (!app_usb_connected())
#endif
                   )
                {
                    if (app_db.remote_session_state == REMOTE_SESSION_STATE_DISCONNECTED)
                    {
                        app_db.power_off_cause = POWER_OFF_CAUSE_EXIT_PAIRING_MODE;
                        app_mmi_handle_action(MMI_DEV_POWER_OFF);
                    }
                    else
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
                }
                else
                {
                    if (b2s_connected_is_empty())
                    {
                        enter_state(STATE_AFE_STANDBY, BT_DEVICE_MODE_CONNECTABLE);
                    }
                    else
                    {
                        if (b2s_connected_is_full() && engage_done)
                        {
                            enter_state(STATE_AFE_CONNECTED, BT_DEVICE_MODE_IDLE);
                        }
                        else
                        {
                            enter_state(STATE_AFE_CONNECTED, BT_DEVICE_MODE_CONNECTABLE);
                        }
                    }
                }

                is_pairing_timeout = false;
            }
            break;

        case STABLE_ENTER_MODE_DIRECT_PAIRING:
            {
                enter_state(STATE_AFE_PAIRING_MODE, BT_DEVICE_MODE_DISCOVERABLE_CONNECTABLE);

                event_ind(BP_EVENT_ROLE_DECIDED, NULL);
            }
            break;

        default:
            break;
        }
#if F_APP_HARMAN_FEATURE_SUPPORT
        au_set_power_on_link_back_fg(true);
#endif
    }
}

static bool bt_event_handle(T_EVENT event, T_BT_PARAM *param)
{
    bool is_done = false;

    switch (event)
    {
    case EVENT_SRC_DISCONN_NORMAL:
        {
            if (disconnect_for_pairing_mode)
            {
                disconnect_for_pairing_mode = false;

                stable_sched(STABLE_ENTER_MODE_DEDICATED_PAIRING);

                is_done = true;
            }
        }
        break;

    case EVENT_PAIRING_MODE_TIMEOUT:
        {
            if (app_cfg_nv.bud_role != REMOTE_SESSION_ROLE_SECONDARY)
            {
                stable_sched(STABLE_ENTER_MODE_NOT_PAIRING);
            }
        }
        break;

    default:
        break;
    }

    return is_done;
}

static bool bt_event_pre_handle(T_EVENT event, T_BT_PARAM *param)
{
    bool is_done = true;
    T_BP_EVENT_PARAM event_param;
    T_APP_BR_LINK *p_link;

    event_param.bd_addr = param->bd_addr;
    event_param.cause = param->cause;

    switch (event)
    {
    case EVENT_BUD_CONN_SUC:
        {
            b2b_connected_add_node(param->bd_addr);

            engage_afe_done();

            primary_page_scan_param_adjust();

            if (STATE_FE_SHAKING == cur_state)
            {
                legacy_send_acl_disconn_req(param->bd_addr);
            }
        }
        break;

    case EVENT_BUD_CONN_FAIL:
        {
            event_ind(BP_EVENT_BUD_CONN_FAIL, NULL);
        }
        break;

    case EVENT_BUD_AUTH_FAIL:
        {
            event_ind(BP_EVENT_BUD_AUTH_FAIL, NULL);
        }
        break;

    case EVENT_BUD_AUTH_SUC:
        {
            gap_stop_timer(&timer_handle_bud_linklost);
            gap_stop_timer(&timer_handle_engage_action_adjust);
            connected_node_auth_suc(param->bd_addr);
            event_ind(BP_EVENT_BUD_AUTH_SUC, NULL);
        }
        break;

    case EVENT_BUD_DISCONN_LOCAL:
    case EVENT_BUD_DISCONN_NORMAL:
        {
            bool is_authed;

            is_authed = connected_node_is_authed(param->bd_addr);

            b2b_connected_del_node(param->bd_addr);

            if (app_cfg_nv.bud_role == REMOTE_SESSION_ROLE_PRIMARY)
            {
                primary_page_scan_param_adjust();
            }
            else
            {
                for (uint8_t i = 0; i < MAX_BR_LINK_NUM; i++)
                {
                    if (app_check_b2s_link_by_id(i))
                    {
                        b2s_connected_del_node(app_db.br_link[i].bd_addr);
                    }
                }
            }
            app_bt_policy_primary_engage_action_adjust();


            if (is_authed)
            {
                event_ind(BP_EVENT_BUD_DISCONN_NORMAL, NULL);
            }
        }
        break;

    case EVENT_BUD_DISCONN_LOST:
        {
            bool is_authed;

            is_authed = connected_node_is_authed(param->bd_addr);

            b2b_connected_del_node(param->bd_addr);
            rws_link_lost = true;

            if (app_cfg_nv.bud_role == REMOTE_SESSION_ROLE_PRIMARY)
            {
                primary_page_scan_param_adjust();
            }
            else
            {
                for (uint8_t i = 0; i < MAX_BR_LINK_NUM; i++)
                {
                    if (app_check_b2s_link_by_id(i))
                    {
                        b2s_connected_del_node(app_db.br_link[i].bd_addr);
                    }
                }
            }


            app_bt_policy_primary_engage_action_adjust();

            if (app_cfg_const.rws_linkloss_linkback_timeout != 0)
            {
                gap_stop_timer(&timer_handle_bud_linklost);
                gap_start_timer(&timer_handle_bud_linklost, "rws_linklost", timer_queue_id,
                                TIMER_ID_BUD_LINKLOST_TIMEOUT, 0, app_cfg_const.rws_linkloss_linkback_timeout * 5000);
            }

            if (is_authed)
            {
                event_ind(BP_EVENT_BUD_DISCONN_LOST, NULL);
            }
        }
        break;

    case EVENT_BUD_REMOTE_CONN_CMPL:
        {
            b2b_connected_add_prof(param->bd_addr, param->prof);

            event_ind(BP_EVENT_BUD_REMOTE_CONN_CMPL, NULL);

#if F_APP_ERWS_SUPPORT
            if (app_cfg_nv.first_engaged)
            {
                roleswitch_handle(param->bd_addr, ROLESWITCH_EVENT_LINK_CONNECTED);
            }
#endif
        }
        break;

    case EVENT_BUD_REMOTE_DISCONN_CMPL:
        {
            b2b_connected_del_prof(param->bd_addr, param->prof);

            event_ind(BP_EVENT_BUD_REMOTE_DISCONN_CMPL, NULL);
        }
        break;

    case EVENT_BUD_LINKLOST_TIMEOUT:
        {
            if (!b2b_connected && b2s_connected_is_empty())
            {
                event_ind(BP_EVENT_BUD_LINKLOST_TIMEOUT, NULL);
            }
        }
        break;

    case EVENT_SRC_CONN_SUC:
        {
            bool suc;
            uint8_t id;

            suc = b2s_connected_add_node(param->bd_addr, &id);
            if (suc)
            {
                if (b2s_connected_is_over())
                {
                    app_multilink_disconnect_inactive_link(id);
                }
                app_bt_policy_sync_b2s_connected();
            }

            if (BT_DEVICE_MODE_IDLE == radio_mode)
            {
                legacy_send_acl_disconn_req(param->bd_addr);
            }

#if F_APP_LISTENING_MODE_SUPPORT
            if (app_cfg_const.disallow_listening_mode_before_phone_connected)
            {
                app_listening_judge_conn_disc_evnet(APPLY_LISTENING_MODE_SRC_CONNECTED);
            }
#endif
        }
        break;

    case EVENT_SRC_CONN_TIMEOUT:
        {
            if (linkback_active_node.is_valid)
            {
                if (linkback_active_node.linkback_node.is_group_member)
                {
                    linkback_todo_queue_delete_group_member();
                }
            }

            if (STATE_AFE_LINKBACK != cur_state)
            {
                linkback_active_node.is_valid = false;
            }

            gap_stop_timer(&timer_handle_linkback_delay);
        }
        break;

    case EVENT_SRC_AUTH_LINK_KEY_INFO:
        {
            T_APP_REMOTE_MSG_PAYLOAD_LINK_KEY_ADDED link_key;

#if F_APP_TEAMS_BT_POLICY
            if (app_teams_bt_handle_auth_link_key_info(param))
            {
                break;
            }
#endif
            app_bond_key_set(param->bd_addr, param->link_key, param->key_type);

            link_key.key_type = param->key_type;
            memcpy(link_key.bd_addr, param->bd_addr, 6);
            memcpy(link_key.link_key, param->link_key, 16);
            app_relay_async_single_with_raw_msg(APP_MODULE_TYPE_DEVICE, APP_REMOTE_MSG_LINK_RECORD_ADD,
                                                (uint8_t *)&link_key, sizeof(T_APP_REMOTE_MSG_PAYLOAD_LINK_KEY_ADDED));

            uint8_t mapping_idx;
            app_bt_update_pair_idx_mapping();
            if (app_bond_get_pair_idx_mapping(param->bd_addr, &mapping_idx))
            {
                app_cfg_nv.audio_gain_level[mapping_idx] = app_cfg_const.playback_volume_default;
                app_cfg_nv.voice_gain_level[mapping_idx] = app_cfg_const.voice_out_volume_default;
            }

            app_relay_async_single_with_raw_msg(APP_MODULE_TYPE_AUDIO_POLICY, APP_REMOTE_MSG_AUDIO_VOLUME_RESET,
                                                param->bd_addr, 6);
        }
        break;

    case EVENT_B2B_AUTH_LINK_KEY_INFO:
        {
            uint8_t local_addr[6];

            bt_bond_key_set(param->bd_addr, param->link_key, param->key_type);

            if (remote_local_addr_get(local_addr))
            {
                bt_bond_key_set(local_addr, param->link_key, param->key_type);
            }
        }
        break;


    case EVENT_SRC_AUTH_LINK_KEY_REQ:
    case EVENT_B2B_AUTH_LINK_KEY_REQ:
        {
            uint8_t link_key[16];
            T_BT_LINK_KEY_TYPE type;

            if (bt_bond_key_get(param->bd_addr, link_key, (uint8_t *)&type))
            {
                bt_link_key_cfm(param->bd_addr, true, type, link_key);
            }
            else
            {
                bt_link_key_cfm(param->bd_addr, false, type, link_key);
            }
        }
        break;

    case EVENT_SRC_AUTH_LINK_PIN_CODE_REQ:
        {
            uint8_t pin_code[4] = {1, 2, 3, 4};

            bt_link_pin_code_cfm(param->bd_addr, pin_code, 4, true);
        }
        break;

    case EVENT_SRC_AUTH_FAIL:
        {
            if ((param->cause == (HCI_ERR | HCI_ERR_AUTHEN_FAIL)) ||
                (param->cause == (HCI_ERR | HCI_ERR_KEY_MISSING)))
            {
                T_APP_LINK_RECORD link_record;

                if (app_bond_pop_sec_diff_link_record(param->bd_addr, &link_record))
                {
                    app_bond_key_set(param->bd_addr, link_record.link_key, link_record.key_type);
                    bt_bond_flag_set(param->bd_addr, link_record.bond_flag);
                }
                else
                {
#if F_APP_TEAMS_BT_POLICY
                    app_bt_policy_del_cod(param->bd_addr);
#endif
                    bt_bond_delete(param->bd_addr);
                    app_relay_async_single_with_raw_msg(APP_MODULE_TYPE_DEVICE, APP_REMOTE_MSG_LINK_RECORD_DEL,
                                                        param->bd_addr, 6);
                }
            }

            event_ind(BP_EVENT_SRC_AUTH_FAIL, &event_param);
        }
        break;

    case EVENT_SRC_AUTH_SUC:
        {
            if ((app_cfg_const.enter_pairing_while_only_one_device_connected) && (b2s_connected.num == 1))
            {
                discoverable_when_one_link = true;
            }
            else
            {
                discoverable_when_one_link = false;
            }
#if F_APP_TEAMS_FEATURE_SUPPORT
            conn_to_new_device = 1;
            uint8_t bond_num = bt_bond_num_get();
            uint8_t bd_addr[6];
            for (uint8_t i = 1; i <= bond_num; i++)
            {
                bt_bond_addr_get(i, bd_addr);
                if (!memcmp(bd_addr, param->bd_addr, 6))
                {
                    conn_to_new_device = 0;
                }
            }
#endif
            gap_stop_timer(&timer_handle_bud_linklost);
            connected_node_auth_suc(param->bd_addr);
            b2s_connected_mark_index(param->bd_addr);
            event_ind(BP_EVENT_SRC_AUTH_SUC, &event_param);
        }
        break;

    case EVENT_SRC_DISCONN_LOST:
        {
            bool is_authed;
            bool is_first_src;
            uint32_t bond_flag = 0;
            uint32_t plan_profs = 0;

            is_authed = connected_node_is_authed(param->bd_addr);
            is_first_src = b2s_connected_is_first_src(param->bd_addr);

            if (b2s_connected_del_node(param->bd_addr))
            {
                if (memcmp(param->bd_addr, app_db.resume_addr, 6))
                {
                    app_multilink_stop_acl_disconn_timer();
                }

                bt_bond_flag_get(param->bd_addr, &bond_flag);

                if (bond_flag & (BOND_FLAG_HFP | BOND_FLAG_HSP | BOND_FLAG_A2DP))
                {
                    plan_profs = get_profs_by_bond_flag(bond_flag);

                    linkback_todo_queue_insert_normal_node(param->bd_addr, plan_profs,
                                                           app_cfg_const.timer_link_back_loss * 1000, false, true);
                }

                ENGAGE_PRINT_TRACE2("bt_event_pre_handle: bond_flag 0x%x, plan_profs 0x%x", bond_flag, plan_profs);

                if (is_authed)
                {
                    event_param.is_first_src = is_first_src;
                    event_ind(BP_EVENT_SRC_DISCONN_LOST, &event_param);
                }
                app_bt_policy_sync_b2s_connected();

#if F_APP_LISTENING_MODE_SUPPORT
                if (app_cfg_const.disallow_listening_mode_before_phone_connected)
                {
                    app_listening_judge_conn_disc_evnet(APPLY_LISTENING_MODE_SRC_DISCONNECTED);
                }
#endif

#if F_APP_TEAMS_BT_POLICY
                app_bt_policy_delete_cod_temp_info_by_addr(param->bd_addr);
#endif

#if F_APP_ERWS_SUPPORT
                roleswitch_handle(param->bd_addr, ROLESWITCH_EVENT_LINK_DISCONNECTED);
#endif
            }
        }
        break;

    case EVENT_SRC_DISCONN_NORMAL:
        {
            bool is_first_src;

            is_src_authed = connected_node_is_authed(param->bd_addr);
            is_first_src = b2s_connected_is_first_src(param->bd_addr);

            if (b2s_connected_del_node(param->bd_addr))
            {
                if (is_src_authed)
                {
                    event_param.is_first_src = is_first_src;
                    event_ind(BP_EVENT_SRC_DISCONN_NORMAL, &event_param);
                }
                app_bt_policy_sync_b2s_connected();

                if (memcmp(param->bd_addr, app_db.resume_addr, 6))
                {
                    app_multilink_stop_acl_disconn_timer();
                }

#if F_APP_LISTENING_MODE_SUPPORT
                if (app_cfg_const.disallow_listening_mode_before_phone_connected)
                {
                    app_listening_judge_conn_disc_evnet(APPLY_LISTENING_MODE_SRC_DISCONNECTED);
                }
#endif
#if F_APP_ERWS_SUPPORT
#if F_APP_TEAMS_BT_POLICY
                app_bt_policy_delete_cod_temp_info_by_addr(param->bd_addr);
#endif

                roleswitch_handle(param->bd_addr, ROLESWITCH_EVENT_LINK_DISCONNECTED);
#endif
            }

        }
        break;

    case EVENT_SRC_DISCONN_ROLESWAP:
        {
            b2s_connected_del_node(param->bd_addr);

            linkback_todo_queue_delete_all_node();

            linkback_active_node.is_valid = false;

            event_ind(BP_EVENT_SRC_DISCONN_ROLESWAP, &event_param);
        }
        break;

    case EVENT_PROFILE_CONN_SUC:
        {
            event_param.is_first_prof = b2s_connected_no_profs(param->bd_addr);
            event_param.is_first_src = b2s_connected_is_first_src(param->bd_addr);

            b2s_connected_add_prof(param->bd_addr, param->prof);

            p_link = app_find_br_link(param->bd_addr);

            if (p_link != NULL)
            {
                src_conn_ind_add(param->bd_addr, true);

                if (A2DP_PROFILE_MASK == param->prof)
                {
                    if (0 == (AVRCP_PROFILE_MASK & p_link->connected_profile))
                    {
                        gap_stop_timer(&p_link->timer_handle_later_avrcp);
                        gap_start_timer(&p_link->timer_handle_later_avrcp, "later_avrcp", timer_queue_id,
                                        TIMER_ID_LATER_AVRCP, p_link->id, app_cfg_const.timer_link_avrcp);

                        //linkback_active_node_remain_profs_sub(AVRCP_PROFILE_MASK);
                    }
                }
                else if (AVRCP_PROFILE_MASK == param->prof)
                {
                    gap_stop_timer(&p_link->timer_handle_later_avrcp);
                }
#if F_APP_HID_SUPPORT
                else if (HID_PROFILE_MASK == param->prof)
                {
                    gap_stop_timer(&p_link->timer_handle_later_hid);
                }
#endif

                if (event_param.is_first_prof)
                {
                    uint32_t bond_flag = 0;

                    app_sniff_mode_b2s_enable(param->bd_addr, SNIFF_DISABLE_MASK_ACL);

                    bt_bond_flag_get(param->bd_addr, &bond_flag);
#if F_APP_HID_SUPPORT
                    if (bond_flag & BOND_FLAG_HID)
                    {
                        if (0 == (HID_PROFILE_MASK & p_link->connected_profile))
                        {
                            gap_stop_timer(&p_link->timer_handle_later_hid);
                            gap_start_timer(&p_link->timer_handle_later_hid, "later_hid", timer_queue_id,
                                            TIMER_ID_LATER_HID, p_link->id, 2000);

                            //linkback_active_node_remain_profs_sub(HID_PROFILE_MASK);
                        }
                    }
#endif
                }
            }

            event_ind(BP_EVENT_PROFILE_CONN_SUC, &event_param);
        }
        break;

    case EVENT_PROFILE_DISCONN:
        {
            if (param->cause != (L2C_ERR | L2C_ERR_NO_RESOURCE))
            {
                if (A2DP_PROFILE_MASK == param->prof)
                {
                    bt_avrcp_disconnect_req(param->bd_addr);
                }

                b2s_connected_del_prof(param->bd_addr, param->prof);

                event_param.is_last_prof = false;
                event_param.prof = param->prof;

                p_link = app_find_br_link(param->bd_addr);
                if (p_link != NULL)
                {
                    if ((p_link->connected_profile & ~HID_PROFILE_MASK) == 0)
                    {
                        event_param.is_last_prof = true;

                        if (p_link->disconn_acl_flg
#if (F_APP_ANC_SUPPORT == 1)
                            || app_anc_get_measure_mode()
#endif
                           )
                        {
                            p_link->disconn_acl_flg = false;
                            legacy_send_acl_disconn_req(param->bd_addr);
                        }
                    }
                }

                event_ind(BP_EVENT_PROFILE_DISCONN, &event_param);
            }
        }
        break;

    case EVENT_PAIRING_MODE_TIMEOUT:
        {
            is_pairing_timeout = true;

            event_ind(BP_EVENT_PAIRING_MODE_TIMEOUT, NULL);
        }
        break;

    case EVENT_PAGESCAN_PARAM_TIMEOUT:
        {
#if GFPS_FEATURE_SUPPORT
#if GFPS_SASS_SUPPORT
            if (extend_app_cfg_const.gfps_support)
            {
                legacy_set_page_scan_param(SASS_PAGESCAN_INTERVAL, SASS_PAGESCAN_WINDOW);
            }
            else
#endif
#endif
            {
                legacy_set_page_scan_param(NORMAL_PAGESCAN_INTERVAL, NORMAL_PAGESCAN_WINDOW);
            }
        }
        break;

    case EVENT_SRC_CONN_IND_TIMEOUT:
        {
            src_conn_ind_del(param->cause);
        }
        break;

    default:
        {
            is_done = false;
        }
        break;
    }

    return is_done;
}

static bool sec_src_handle(T_EVENT event, T_BT_PARAM *param)
{
    bool is_done = true;

    switch (event)
    {
    case EVENT_SRC_CONN_SUC:
        {
            uint8_t id;

            b2s_connected_add_node(param->bd_addr, &id);
        }
        break;

    case EVENT_SRC_DISCONN_LOST:
    case EVENT_SRC_DISCONN_NORMAL:
    case EVENT_SRC_DISCONN_ROLESWAP:
        {
            b2s_connected_del_node(param->bd_addr);
        }
        break;

    case EVENT_PROFILE_CONN_SUC:
        {
            b2s_connected_add_prof(param->bd_addr, param->prof);
        }
        break;

    case EVENT_PROFILE_DISCONN:
        {
            b2s_connected_del_prof(param->bd_addr, param->prof);
        }
        break;

    default:
        {
            is_done = false;
        }
        break;
    }

    return is_done;
}

static bool event_handle(T_EVENT event, void *param)
{
    bool ret = true;

    switch (event)
    {
    case EVENT_STARTUP:
        {
            startup_event_handle(param);
        }
        break;

    case EVENT_ENTER_DUT_TEST_MODE:
        {
            enter_dut_test_mode_event_handle();
        }
        break;

#if (F_APP_OTA_TOOLING_SUPPORT == 1)
    case EVENT_START_OTA_SHAKING:
        {
            start_ota_shaking_event_handle();
        }
        break;

    case EVENT_ENTER_OTA_MODE:
        {
            enter_ota_mode_event_handle(param);
        }
        break;
#endif

    case EVENT_SHUTDOWN:
        {
            shutdown_event_handle();
        }
        break;

    case EVENT_STOP:
        {
            stop_event_handle();
        }
        break;

    case EVENT_RESTORE:
        {
            restore_event_handle();
        }
        break;

    case EVENT_CONN_SNIFF:
        {
            conn_sniff_event_handle(param);
        }
        break;

    case EVENT_CONN_ACTIVE:
        {
            conn_active_event_handle(param);
        }
        break;
#if F_APP_ERWS_SUPPORT
    case EVENT_PREPARE_FOR_ROLESWAP:
        {
            prepare_for_roleswap_event_handle();
        }
        break;

    case EVENT_ROLESWAP:
        {
            roleswap_event_handle(param);
        }
        break;
#endif
    case EVENT_ROLE_MASTER:
        {
            role_master_event_handle(param);
        }
        break;

    case EVENT_ROLE_SLAVE:
        {
            role_slave_event_handle(param);
        }
        break;

    case EVENT_ROLESWITCH_FAIL:
        {
            roleswitch_fail_event_handle(param);
        }
        break;

    case EVENT_ROLESWITCH_TIMEOUT:
        {
            roleswitch_timeout_event_handle(param);
        }
        break;

    case EVENT_DEDICATED_ENTER_PAIRING_MODE:
        {
            dedicated_enter_pairing_mode_event_handle(param);
        }
        break;

    case EVENT_DEDICATED_EXIT_PAIRING_MODE:
        {
            dedicated_exit_pairing_mode_event_handle();
        }
        break;

    case EVENT_DEDICATED_CONNECT:
        {
            dedicated_connect_event_handle(param);
        }
        break;

    case EVENT_DEDICATED_DISCONNECT:
        {
            dedicated_disconnect_event_handle(param);
        }
        break;

    case EVENT_DISCONNECT_ALL:
        {
            disconnect_all_event_handle();
        }
        break;

    default:
        {
            ret = false;
        }
        break;
    }

    return ret;
}

void state_machine(T_EVENT event, void *param)
{
    cur_event = event;

    event_info(event);

    if (app_cfg_nv.bud_role == REMOTE_SESSION_ROLE_SECONDARY)
    {
        if (EVENT_SRC_CONN_SUC <= event && event <= EVENT_PROFILE_DISCONN)
        {
            if (sec_src_handle(event, param))
            {
                connected();
            }
            return;
        }
    }

    if (!event_handle(event, param))
    {
        if (bt_event_pre_handle(event, param))
        {
            connected();
        }

        if (bt_event_handle(event, param))
        {
            return;
        }

        switch (cur_state)
        {
        case STATE_SHUTDOWN_STEP:
            {
                state_shutdown_step_event_handle(event);
            }
            break;

        case STATE_FE_SHAKING:
            {
                state_fe_shaking_event_handle(event);
            }
            break;

        case STATE_FE_COUPLING:
            {
                state_fe_coupling_event_handle(event);
            }
            break;

        case STATE_AFE_TIMEOUT_SHAKING:
            {
                state_afe_timeout_shaking_event_handle(event, param);
            }
            break;

        case STATE_AFE_COUPLING:
            {
                state_afe_coupling_event_handle(event);
            }
            break;

        case STATE_AFE_WAIT_COUPLING:
            {
                state_afe_wait_coupling_event_handle(event);
            }
            break;

        case STATE_AFE_LINKBACK:
            {
                state_afe_linkback_event_handle(event, param);
            }
            break;

        case STATE_AFE_CONNECTED:
        case STATE_AFE_PAIRING_MODE:
        case STATE_AFE_STANDBY:
        case STATE_AFE_SECONDARY:
            {
                state_afe_stable_event_handle(event);
            }
            break;

        case STATE_DUT_TEST_MODE:
            {
                state_dut_test_mode_event_handle(event);
            }
            break;

#if (F_APP_OTA_TOOLING_SUPPORT == 1)
        case STATE_OTA_SHAKING:
            {
                state_ota_shaking_event_handle(event, param);
            }
            break;

        case STATE_OTA_MODE:
            {
                state_ota_mode_event_handle(event);
            }
            break;
#endif

        default:
            break;
        }
    }
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static void timer_cback(uint8_t timer_id, uint16_t timer_chann)
{
    switch (timer_id)
    {
    case TIMER_ID_SHUTDOWN_STEP:
        {
            if (timer_handle_shutdown_step != NULL)
            {
                gap_stop_timer(&timer_handle_shutdown_step);
                state_machine(EVENT_SHUTDOWN_STEP_TIMEOUT, NULL);
            }
        }
        break;

    case TIMER_ID_FIRST_ENGAGE_CHECK:
        {
            if (timer_handle_first_engage_check != NULL)
            {
                gap_stop_timer(&timer_handle_first_engage_check);
                state_machine(EVENT_FE_TIMEOUT, NULL);
            }
        }
        break;

    case TIMER_ID_PAIRING_TIMEOUT:
        {
            if (timer_handle_pairing_mode != NULL)
            {
#if F_APP_LE_AUDIO_SM
                bt_device_mode_set(BT_DEVICE_MODE_IDLE);
#endif
                gap_stop_timer(&timer_handle_pairing_mode);
                state_machine(EVENT_PAIRING_MODE_TIMEOUT, NULL);
            }
        }
        break;

    case TIMER_ID_DISCOVERABLE_TIMEOUT:
        {
            if (timer_handle_discoverable != NULL)
            {
                gap_stop_timer(&timer_handle_discoverable);
                state_machine(EVENT_DISCOVERABLE_TIMEOUT, NULL);
            }
        }
        break;

    case TIMER_ID_ROLE_SWITCH:
        {
            T_APP_BR_LINK *p_link;

            p_link = &app_db.br_link[timer_chann];
            if (p_link->timer_handle_role_switch != NULL)
            {
                gap_stop_timer(&p_link->timer_handle_role_switch);
                state_machine(EVENT_ROLESWITCH_TIMEOUT, p_link);
            }
        }
        break;

    case TIMER_ID_BUD_LINKLOST_TIMEOUT:
        {
            if (timer_handle_bud_linklost != NULL)
            {
                gap_stop_timer(&timer_handle_bud_linklost);
                state_machine(EVENT_BUD_LINKLOST_TIMEOUT, NULL);
            }
        }
        break;

    case TIMER_ID_LINKBACK_TIMEOUT:
        {
            T_BT_PARAM bt_param;

            if (timer_handle_linkback != NULL)
            {
                memset(&bt_param, 0, sizeof(T_BT_PARAM));
                bt_param.not_check_addr_flag = true;

                gap_stop_timer(&timer_handle_linkback);
                state_machine(EVENT_SRC_CONN_TIMEOUT, &bt_param);
            }
        }
        break;

    case TIMER_ID_LINKBACK_DELAY:
        {
            T_BT_PARAM bt_param;

            if (timer_handle_linkback_delay != NULL)
            {
                memset(&bt_param, 0, sizeof(T_BT_PARAM));
                bt_param.not_check_addr_flag = true;
                bt_param.cause = timer_chann;

                gap_stop_timer(&timer_handle_linkback_delay);
                state_machine(EVENT_LINKBACK_DELAY_TIMEOUT, &bt_param);
            }
        }
        break;

    case TIMER_ID_LATER_AVRCP:
        {
            T_APP_BR_LINK *p_link;
            T_BT_PARAM bt_param;

            p_link = &app_db.br_link[timer_chann];
            if (p_link->timer_handle_later_avrcp != NULL)
            {
                memset(&bt_param, 0, sizeof(T_BT_PARAM));
                bt_param.bd_addr = p_link->bd_addr;
                bt_param.prof = AVRCP_PROFILE_MASK;
                bt_param.is_special = false;
                bt_param.check_bond_flag = false;
                bt_param.is_later_avrcp = true;

                gap_stop_timer(&p_link->timer_handle_later_avrcp);
                state_machine(EVENT_DEDICATED_CONNECT, &bt_param);
            }
        }
        break;

    case TIMER_ID_WAIT_COUPLING:
        {
            if (timer_handle_wait_coupling != NULL)
            {
                gap_stop_timer(&timer_handle_wait_coupling);
                state_machine(EVENT_BUD_WAIT_COUPLING_TO, NULL);
            }
        }
        break;

    case TIMER_ID_ENGAGE_ACTION_ADJUST:
        {
            if (timer_handle_engage_action_adjust != NULL)
            {
                gap_stop_timer(&timer_handle_engage_action_adjust);
                state_machine(EVENT_BUD_ENGAGE_ACTION_ADJUST, NULL);
            }
        }
        break;

    case TIMER_ID_LATER_HID:
        {
            T_APP_BR_LINK *p_link;
            T_BT_PARAM bt_param;

            p_link = &app_db.br_link[timer_chann];
            if (p_link->timer_handle_later_hid != NULL)
            {
                memset(&bt_param, 0, sizeof(T_BT_PARAM));
                bt_param.bd_addr = p_link->bd_addr;
                bt_param.prof = HID_PROFILE_MASK;
                bt_param.is_special = false;
                bt_param.check_bond_flag = true;
                bt_param.is_later_hid = true;

                gap_stop_timer(&p_link->timer_handle_later_hid);
                state_machine(EVENT_DEDICATED_CONNECT, &bt_param);
            }
        }
        break;

    case TIMER_ID_PAGESCAN_PARAM_TIMEOUT:
        {
            if (timer_handle_page_scan_param != NULL)
            {
                gap_stop_timer(&timer_handle_page_scan_param);
                state_machine(EVENT_PAGESCAN_PARAM_TIMEOUT, NULL);
            }
        }
        break;

    case TIMER_ID_RECONNECT:
        {
            if (timer_handle_reconnect != NULL)
            {
                gap_stop_timer(&timer_handle_reconnect);
                app_reconnect_inactive_link();
            }
        }
        break;

    case TIMER_ID_SRC_CONN_IND_TIMEOUT:
        {
            T_BT_PARAM bt_param;

            if (src_conn_ind[timer_chann].timer_handle != NULL)
            {
                memset(&bt_param, 0, sizeof(T_BT_PARAM));
                bt_param.cause = timer_chann;

                gap_stop_timer(&src_conn_ind[timer_chann].timer_handle);

                app_hfp_adjust_sco_window(src_conn_ind[timer_chann].bd_addr, APP_SCO_ADJUST_ACL_CONN_END_EVT);

                state_machine(EVENT_SRC_CONN_IND_TIMEOUT, &bt_param);
            }
        }
        break;

    case TIMER_ID_B2S_CONN_TIMER_MS:
        {
            if (timer_handle_b2s_conn != NULL)
            {
                gap_stop_timer(&timer_handle_b2s_conn);
                if (app_cfg_nv.bud_role == REMOTE_SESSION_ROLE_PRIMARY)
                {
                    if (app_db.remote_session_state == REMOTE_SESSION_STATE_CONNECTED)
                    {
                        if (app_cfg_const.enable_low_bat_role_swap)
                        {
                            app_roleswap_req_battery_level();
                        }
                    }

#if (F_APP_AVP_INIT_SUPPORT == 1)
                    if (app_roleswap_src_connect_delay_hook)
                    {
                        app_roleswap_src_connect_delay_hook();
                    }
#endif
                }
            }
        }
        break;

    default:
        break;
    }
}

void app_bt_update_pair_idx_mapping(void)
{
    uint8_t bond_num = app_b2s_bond_num_get();
    uint8_t pair_idx[8] = {0};
    uint8_t bd_addr[6] = {0};
    uint8_t pair_idx_temp = 0;
    uint8_t i = 0;
    uint8_t j = 0;
    bool mapping_flag = false;

    APP_PRINT_TRACE1("app_bt_update_pair_idx_mapping bond_num = %d", bond_num);

    // APP_PRINT_TRACE8("start app_bt_update_pair_idx_mapping: app_pair_idx_mapping %d,  %d, %d, %d, %d, %d, %d, %d",
    //                  app_cfg_nv.app_pair_idx_mapping[0], app_cfg_nv.app_pair_idx_mapping[1],
    //                  app_cfg_nv.app_pair_idx_mapping[2], app_cfg_nv.app_pair_idx_mapping[3],
    //                  app_cfg_nv.app_pair_idx_mapping[4], app_cfg_nv.app_pair_idx_mapping[5],
    //                  app_cfg_nv.app_pair_idx_mapping[6], app_cfg_nv.app_pair_idx_mapping[7]);

    if (bond_num > 8)
    {
        bond_num = 8;
    }

    for (i = 1; i <= bond_num; i++)
    {
        if (app_b2s_bond_addr_get(i, bd_addr))
        {
            if (bt_bond_index_get(bd_addr, &pair_idx_temp))
            {
                pair_idx[i - 1] = pair_idx_temp;
            }
        }
    }

    for (j = 0; j < 8; j++)
    {
        for (i = 0; i < bond_num; i++)
        {
            if (app_cfg_nv.app_pair_idx_mapping[j] == pair_idx[i])
            {
                mapping_flag = true;
                break;
            }
        }

        if (!mapping_flag)
        {
            app_cfg_nv.audio_gain_level[j] = app_cfg_const.playback_volume_default;
            app_cfg_nv.voice_gain_level[j] = app_cfg_const.voice_out_volume_default;
            app_cfg_nv.app_pair_idx_mapping[j] = 0xFF;
            app_cfg_nv.sass_bit_map &= ~(1 << j);

        }

        mapping_flag = false;
    }

    for (i = 0; i < bond_num; i++)
    {
        for (j = 0; j < 8; j++)
        {
            if (app_cfg_nv.app_pair_idx_mapping[j] == pair_idx[i])
            {
                mapping_flag = true;
                break;
            }
        }

        if (!mapping_flag)
        {
            for (j = 0; j < 8; j++)
            {
                if (app_cfg_nv.app_pair_idx_mapping[j] == 0xFF)
                {
                    app_cfg_nv.app_pair_idx_mapping[j] = pair_idx[i];
                    app_cfg_nv.audio_gain_level[j] = app_cfg_const.playback_volume_default;
                    app_cfg_nv.voice_gain_level[j] = app_cfg_const.voice_out_volume_default;
                    //new device, it's sass bit should always be 0
                    app_cfg_nv.sass_bit_map &= ~(1 << j);

                    break;
                }
            }
        }
        mapping_flag = false;
    }

    // APP_PRINT_TRACE8("end app_bt_update_pair_idx_mapping: app_pair_idx_mapping %d,  %d, %d, %d, %d, %d, %d, %d",
    //                  app_cfg_nv.app_pair_idx_mapping[0], app_cfg_nv.app_pair_idx_mapping[1],
    //                  app_cfg_nv.app_pair_idx_mapping[2], app_cfg_nv.app_pair_idx_mapping[3],
    //                  app_cfg_nv.app_pair_idx_mapping[4], app_cfg_nv.app_pair_idx_mapping[5],
    //                  app_cfg_nv.app_pair_idx_mapping[6], app_cfg_nv.app_pair_idx_mapping[7]);

    // APP_PRINT_TRACE8("end app_bt_update_pair_idx_mapping: pair_idx %d,  %d, %d, %d, %d, %d, %d, %d",
    //                  pair_idx[0], pair_idx[1],
    //                  pair_idx[2], pair_idx[3],
    //                  pair_idx[4], pair_idx[5],
    //                  pair_idx[6], pair_idx[7]);
}

static void app_bt_policy_cback(T_BT_EVENT event_type, void *event_buf, uint16_t buf_len)
{
    T_BT_EVENT_PARAM *param = event_buf;
    T_BT_PARAM bt_param;
    T_APP_BR_LINK *p_link = NULL;

    memset(&bt_param, 0, sizeof(T_BT_PARAM));

    switch (event_type)
    {
    case BT_EVENT_ACL_CONN_SUCCESS:
        {
            bt_param.bd_addr = param->acl_conn_success.bd_addr;

            if (app_check_b2b_link(param->acl_conn_success.bd_addr))
            {
                bt_param.is_b2b = true;
                state_machine(EVENT_BUD_CONN_SUC, &bt_param);
#if (F_APP_GOLDEN_RANGE == 1)
                bt_link_rssi_golden_range_set(param->acl_conn_success.bd_addr, GOLDEN_RANGE_B2B_MAX,
                                              GOLDEN_RANGE_B2B_MIN);
#endif
            }
            else
            {
                bt_param.is_b2b = false;

                if (app_cfg_const.enable_align_default_volume_from_bud_to_phone)
                {
                    first_connect_sync_default_volume_to_src = true;
                }
                else if (app_cfg_const.enable_align_default_volume_after_factory_reset)
                {
                    uint8_t key_type;
                    uint8_t link_key[16];

                    first_connect_sync_default_volume_to_src = false;
                    if (bt_bond_key_get(param->acl_conn_success.bd_addr, link_key,
                                        (uint8_t *)&key_type) == false)
                    {
                        first_connect_sync_default_volume_to_src = true;
                    }
                }
                else
                {
                    first_connect_sync_default_volume_to_src = false;
                }

                state_machine(EVENT_SRC_CONN_SUC, &bt_param);
                app_bt_sniffing_param_update(APP_BT_SNIFFING_EVENT_LEGACY_LINK_CHANGE);
#if (F_APP_GOLDEN_RANGE == 1)
                bt_link_rssi_golden_range_set(param->acl_conn_success.bd_addr, GOLDEN_RANGE_B2S_MAX,
                                              GOLDEN_RANGE_B2S_MIN);
#endif
#if F_APP_HARMAN_FEATURE_SUPPORT
                if (app_cfg_const.enable_multi_link)
                {
                    if (au_get_harman_already_connect_one() == false)
                    {
                        APP_PRINT_ERROR1("acl link connect success ,bdaddr %s, phone is first_connected",
                                         TRACE_BDADDR(param->acl_conn_success.bd_addr));
                        au_set_harman_already_connect_one(true);
                    }
                }
#endif
            }
        }
        break;

    case BT_EVENT_ACL_CONN_FAIL:
        {
            bt_param.bd_addr = param->acl_conn_fail.bd_addr;

            if (app_check_b2b_link(param->acl_conn_fail.bd_addr))
            {
                bt_param.is_b2b = true;
                state_machine(EVENT_BUD_CONN_FAIL, &bt_param);

            }
            else
            {
                bt_param.is_b2b = false;

                if (param->acl_conn_fail.cause == (HCI_ERR | HCI_ERR_ACL_CONN_EXIST))
                {
                    state_machine(EVENT_SRC_CONN_FAIL_ACL_EXIST, &bt_param);
                }
                else if (param->acl_conn_fail.cause == (HCI_ERR | HCI_ERR_CONTROLLER_BUSY))
                {
                    state_machine(EVENT_SRC_CONN_FAIL_CONTROLLER_BUSY, &bt_param);
                }
                else
                {
                    state_machine(EVENT_SRC_CONN_FAIL, &bt_param);
                }
            }
        }
        break;

    case BT_EVENT_ACL_AUTHEN_SUCCESS:
        {
            bt_param.bd_addr = param->acl_authen_success.bd_addr;

            if (app_check_b2b_link(param->acl_authen_success.bd_addr))
            {
                bt_param.is_b2b = true;
                state_machine(EVENT_BUD_AUTH_SUC, &bt_param);
                app_sniff_mode_b2b_disable(bt_param.bd_addr, SNIFF_DISABLE_MASK_ACL);
            }
            else
            {
                bt_param.is_b2b = false;
                bt_param.not_check_addr_flag = true;
                state_machine(EVENT_SRC_AUTH_SUC, &bt_param);
                app_sniff_mode_b2s_disable(bt_param.bd_addr, SNIFF_DISABLE_MASK_ACL);
            }
        }
        break;

    case BT_EVENT_ACL_AUTHEN_FAIL:
        {
            uint8_t   local_addr[6];
            uint8_t   peer_addr[6];

            remote_local_addr_get(local_addr);
            remote_peer_addr_get(peer_addr);

            if ((param->acl_authen_fail.cause == (HCI_ERR | HCI_ERR_AUTHEN_FAIL)) ||
                (param->acl_authen_fail.cause == (HCI_ERR | HCI_ERR_KEY_MISSING)))
            {
                if (memcmp(param->acl_authen_fail.bd_addr, peer_addr, 6) == 0)
                {
#if F_APP_HARMAN_FEATURE_SUPPORT
                    if (((app_check_b2b_link(param->acl_authen_fail.bd_addr)) == false) &&
                        (param->acl_authen_fail.cause == (HCI_ERR | HCI_ERR_KEY_MISSING)))
                    {
                    }
                    else
#endif
                    {
                        bt_bond_delete(local_addr);
                        bt_bond_delete(peer_addr);
                    }
                }
            }

            bt_param.bd_addr = param->acl_authen_fail.bd_addr;

            if (app_check_b2b_link(param->acl_authen_fail.bd_addr))
            {
                bt_param.is_b2b = true;
                state_machine(EVENT_BUD_AUTH_FAIL, &bt_param);
            }
            else
            {
                bt_param.is_b2b = false;
                bt_param.cause = param->acl_authen_fail.cause;
                state_machine(EVENT_SRC_AUTH_FAIL, &bt_param);
            }
        }
        break;

    case BT_EVENT_ACL_CONN_DISCONN:
        {
            bt_param.bd_addr = param->acl_conn_disconn.bd_addr;

            if (app_check_b2b_link(param->acl_conn_disconn.bd_addr))
            {
                bt_param.is_b2b = true;
                app_db.src_roleswitch_fail_no_sniffing = false;
                p_link = app_find_br_link(param->acl_conn_disconn.bd_addr);
                if (p_link != NULL)
                {
                    p_link->acl_decrypted = false;
                }

#if F_APP_QOL_MONITOR_SUPPORT
                if (app_cfg_nv.bud_role == REMOTE_SESSION_ROLE_PRIMARY)
                {
                    //disable link monitor when b2b link disconn
                    app_qol_link_monitor(param->acl_conn_disconn.bd_addr, false);
                }
#endif
                if ((param->acl_conn_disconn.cause == (HCI_ERR | HCI_ERR_CONN_TIMEOUT)) ||
                    (param->acl_conn_disconn.cause == (HCI_ERR | HCI_ERR_LMP_RESPONSE_TIMEOUT)))
                {
                    state_machine(EVENT_BUD_DISCONN_LOST, &bt_param);
                }
                else if (param->acl_conn_disconn.cause == (HCI_ERR | HCI_ERR_LOCAL_HOST_TERMINATE))
                {
                    state_machine(EVENT_BUD_DISCONN_LOCAL, &bt_param);
                }
                else
                {
                    state_machine(EVENT_BUD_DISCONN_NORMAL, &bt_param);
                }
                if (memcmp(param->acl_conn_disconn.bd_addr, app_cfg_nv.bud_peer_addr, 6) == 0)
                {
                    app_db.disallow_sniff = false;
                }
            }
            else
            {
                bt_param.is_b2b = false;
                bt_param.cause = param->acl_conn_disconn.cause;

#if F_APP_HARMAN_FEATURE_SUPPORT
                app_harman_remote_device_name_crc_set(param->acl_conn_disconn.bd_addr, false);
                app_harman_le_common_adv_update();
#endif
                if ((param->acl_conn_disconn.cause == (HCI_ERR | HCI_ERR_CONN_TIMEOUT)) ||
                    (param->acl_conn_disconn.cause == (HCI_ERR | HCI_ERR_LMP_RESPONSE_TIMEOUT)))
                {
                    state_machine(EVENT_SRC_DISCONN_LOST, &bt_param);
                }
                else if (param->acl_conn_disconn.cause == (HCI_ERR | HCI_ERR_CONN_ROLESWAP))
                {
                    state_machine(EVENT_SRC_DISCONN_ROLESWAP, &bt_param);
                }
                else
                {
                    state_machine(EVENT_SRC_DISCONN_NORMAL, &bt_param);
                }
#if F_APP_HARMAN_FEATURE_SUPPORT
                if (app_find_b2s_link_num() == 0)
                {
                    app_harman_ble_adv_stop();

                    if (app_cfg_const.enable_multi_link)
                    {
                        au_set_harman_already_connect_one(false);
                    }
                }
#endif
                app_bt_policy_qos_param_update(bt_param.bd_addr, BP_TPOLL_ACL_DISC_EVENT);
            }
        }
        break;

    case BT_EVENT_ACL_ROLE_MASTER:
        {
            bt_param.bd_addr = param->acl_role_master.bd_addr;

            if (app_check_b2b_link(param->acl_role_master.bd_addr))
            {
                bt_param.is_b2b = true;
                // set b2b link Supervision timeout
                legacy_cfg_acl_link_supv_tout(bt_param.bd_addr, 0xc80);
                state_machine(EVENT_ROLE_MASTER, &bt_param);
            }
            else
            {
                bt_param.is_b2b = false;
                state_machine(EVENT_ROLE_MASTER, &bt_param);
            }
        }
        break;

    case BT_EVENT_ACL_ROLE_SLAVE:
        {
            bt_param.bd_addr = param->acl_role_slave.bd_addr;

            if (app_check_b2b_link(param->acl_role_slave.bd_addr))
            {
                bt_param.is_b2b = true;
                state_machine(EVENT_ROLE_SLAVE, &bt_param);
            }
            else
            {
                bt_param.is_b2b = false;
                state_machine(EVENT_ROLE_SLAVE, &bt_param);
            }
        }
        break;

    case BT_EVENT_ACL_CONN_ACTIVE:
        {
            bt_param.bd_addr = param->acl_conn_active.bd_addr;

            if (app_check_b2b_link(param->acl_conn_active.bd_addr))
            {
                bt_param.is_b2b = true;
                state_machine(EVENT_CONN_ACTIVE, &bt_param);
            }
            else
            {
                bt_param.is_b2b = false;
                state_machine(EVENT_CONN_ACTIVE, &bt_param);
            }
        }
        break;

    case BT_EVENT_ACL_CONN_SNIFF:
        {
            bt_param.bd_addr = param->acl_conn_sniff.bd_addr;

            if (app_check_b2b_link(param->acl_conn_sniff.bd_addr))
            {
                bt_param.is_b2b = true;
                state_machine(EVENT_CONN_SNIFF, &bt_param);
            }
            else
            {
                bt_param.is_b2b = false;
                state_machine(EVENT_CONN_SNIFF, &bt_param);
            }
        }
        break;

    case BT_EVENT_ACL_CONN_ENCRYPTED:
        {
            if (app_check_b2b_link(param->acl_conn_encrypted.bd_addr) == false)
            {
                app_bt_policy_qos_param_update(param->acl_conn_encrypted.bd_addr, BP_TPOLL_ACL_CONN_EVENT);
            }
        }
        break;

    case BT_EVENT_ACL_CONN_NOT_ENCRYPTED:
        {
            p_link = app_find_br_link(param->acl_conn_not_encrypted.bd_addr);
            if (p_link != NULL)
            {
                p_link->acl_decrypted = true;
            }

            if (app_cfg_nv.bud_role == REMOTE_SESSION_ROLE_PRIMARY)
            {
                app_bt_sniffing_process(param->acl_conn_not_encrypted.bd_addr);
            }
        }
        break;

    case BT_EVENT_ACL_CONN_READY:
        {
            if (app_check_b2b_link(param->acl_conn_ready.bd_addr) == true)
            {
                bt_acl_pkt_type_set(param->acl_conn_ready.bd_addr, BT_ACL_PKT_TYPE_2M);
            }
            else
            {
                /* TODO not set pkt 2m in DUT Test Mode */
                bt_acl_pkt_type_set(param->acl_conn_ready.bd_addr, BT_ACL_PKT_TYPE_2M);

                roleswitch_handle(param->acl_conn_ready.bd_addr, ROLESWITCH_EVENT_LINK_CONNECTED);
            }
        }
        break;

    case BT_EVENT_ACL_CONN_IND:
        {
            APP_PRINT_TRACE1("app_bt_policy_cback: conn ind device cod 0x%08x", param->acl_conn_ind.cod);
            p_link = app_find_br_link(param->acl_conn_ind.bd_addr);
            if (p_link != NULL)
            {
                bt_acl_conn_reject(param->acl_conn_ind.bd_addr, BT_ACL_REJECT_UNACCEPTABLE_ADDR);
            }
            else if (app_check_b2s_link(param->acl_conn_ind.bd_addr) && b2s_connected_is_full())
            {
#if GFPS_SASS_SUPPORT
                if (app_sass_policy_support_easy_connection_switch())
                {
#if (F_APP_OTA_TOOLING_SUPPORT == 1)
                    if (validate_bd_addr_with_dongle_id(app_db.jig_dongle_id, param->acl_conn_ind.bd_addr) == true)
#endif
                    {
#if F_APP_TEAMS_BT_POLICY
                        app_teams_bt_handle_acl_conn_ind(param->acl_conn_ind.bd_addr, param->acl_conn_ind.cod);
#endif
                        src_conn_ind_add(param->acl_conn_ind.bd_addr, false);

                        app_hfp_adjust_sco_window(param->acl_conn_ind.bd_addr, APP_SCO_ADJUST_ACL_CONN_IND_EVT);

                        bt_acl_conn_accept(param->acl_conn_ind.bd_addr, BT_LINK_ROLE_SLAVE);
                    }

                }
                else
#endif
                {
                    bt_acl_conn_reject(param->acl_conn_ind.bd_addr, BT_ACL_REJECT_UNACCEPTABLE_ADDR);
                }
            }
            else
            {
#if F_APP_DEVICE_CMD_SUPPORT
                if (app_cmd_get_auto_accept_conn_req_flag() == true)
#endif
                {
                    if (app_check_b2s_link(param->acl_conn_ind.bd_addr))
                    {
#if (F_APP_OTA_TOOLING_SUPPORT == 1)
                        if (validate_bd_addr_with_dongle_id(app_db.jig_dongle_id, param->acl_conn_ind.bd_addr) == true)
#endif
                        {
#if F_APP_TEAMS_BT_POLICY
                            app_teams_bt_handle_acl_conn_ind(param->acl_conn_ind.bd_addr, param->acl_conn_ind.cod);
#endif
#if (F_APP_HARMAN_FEATURE_SUPPORT && HARMAN_ONLY_CONN_ONE_DEVICE_WHEN_PAIRING)
                            APP_PRINT_TRACE3("ACL_CONN_REQ_IND bd_addr %s, state %d, %d",
                                             TRACE_BDADDR(param->acl_conn_ind.bd_addr),
                                             app_bt_policy_get_state(),
                                             au_get_harman_already_connect_one());
                            uint8_t addr_temp[6];
                            uint32_t bond_flag;
                            uint32_t plan_profs;
                            uint8_t bond_num = app_b2s_bond_num_get();
                            uint8_t ACL_CONN_REQ_EVER_CONN = false;

                            for (uint8_t i = 1; i <= bond_num; i++)
                            {
                                bond_flag = 0;
                                if (app_b2s_bond_addr_get(i, addr_temp))
                                {
                                    if (memcmp(addr_temp, app_cfg_nv.bud_peer_addr, 6) &&
                                        memcmp(addr_temp, app_cfg_nv.bud_local_addr, 6))
                                    {
                                        bt_bond_flag_get(addr_temp, &bond_flag);

                                        APP_PRINT_INFO3("au_ever_link_information, dump priority: %d, bond_flag: %d, addr: %s",
                                                        i,
                                                        bond_flag,
                                                        TRACE_BDADDR(addr_temp));

                                        if (bond_flag != 0)
                                        {
                                            if (memcmp(addr_temp, param->acl_conn_ind.bd_addr, 6) == 0)
                                            {
                                                ACL_CONN_REQ_EVER_CONN = true;
                                                break;
                                            }
                                        }
                                    }
                                }
                            }

                            APP_PRINT_ERROR1("ACL_CONN_REQ_IND allow connect %d", ACL_CONN_REQ_EVER_CONN);

                            if (app_bt_policy_get_state() == BP_STATE_PAIRING_MODE)
                            {
                                if (au_get_harman_already_connect_one() == false)
                                {
                                    src_conn_ind_add(param->acl_conn_ind.bd_addr, false);

                                    app_hfp_adjust_sco_window(param->acl_conn_ind.bd_addr, APP_SCO_ADJUST_ACL_CONN_IND_EVT);

                                    bt_acl_conn_accept(param->acl_conn_ind.bd_addr, BT_LINK_ROLE_SLAVE);
                                }
                                else
                                {
                                    bt_acl_conn_reject(param->acl_conn_ind.bd_addr, BT_ACL_REJECT_UNACCEPTABLE_ADDR);
                                }
                            }
                            else
                            {
                                if (ACL_CONN_REQ_EVER_CONN == true)
                                {
                                    src_conn_ind_add(param->acl_conn_ind.bd_addr, false);

                                    app_hfp_adjust_sco_window(param->acl_conn_ind.bd_addr, APP_SCO_ADJUST_ACL_CONN_IND_EVT);

                                    bt_acl_conn_accept(param->acl_conn_ind.bd_addr, BT_LINK_ROLE_SLAVE);
                                }
                                else
                                {
                                    bt_acl_conn_reject(param->acl_conn_ind.bd_addr, BT_ACL_REJECT_UNACCEPTABLE_ADDR);
                                }
                            }
#else
                            src_conn_ind_add(param->acl_conn_ind.bd_addr, false);

                            app_hfp_adjust_sco_window(param->acl_conn_ind.bd_addr, APP_SCO_ADJUST_ACL_CONN_IND_EVT);

                            bt_acl_conn_accept(param->acl_conn_ind.bd_addr, BT_LINK_ROLE_SLAVE);
#endif
                        }
                    }
                    else
                    {
                        if (0 == app_bt_policy_get_b2s_connected_num())
                        {
                            if (app_cfg_nv.bud_role == REMOTE_SESSION_ROLE_PRIMARY)
                            {
                                bt_acl_conn_accept(param->acl_conn_ind.bd_addr, BT_LINK_ROLE_SLAVE);
                            }
                            else
                            {
                                bt_acl_conn_accept(param->acl_conn_ind.bd_addr, BT_LINK_ROLE_MASTER);
                            }
                        }
                        else
                        {
                            if (app_cfg_nv.bud_role == REMOTE_SESSION_ROLE_PRIMARY)
                            {
                                bt_acl_conn_accept(param->acl_conn_ind.bd_addr, BT_LINK_ROLE_MASTER);
                            }
                            else
                            {
                                bt_acl_conn_accept(param->acl_conn_ind.bd_addr, BT_LINK_ROLE_SLAVE);
                            }
                        }
                    }
                }
            }
        }
        break;

    case BT_EVENT_ACL_ROLE_SWITCH_FAIL:
        {
            bt_param.bd_addr = param->sdp_discov_cmpl.bd_addr;
            bt_param.cause = param->acl_role_switch_fail.cause;

            state_machine(EVENT_ROLESWITCH_FAIL, &bt_param);
        }
        break;

    case BT_EVENT_LINK_KEY_INFO:
        {
            bt_param.bd_addr = param->link_key_info.bd_addr;
            bt_param.key_type = param->link_key_info.key_type;
            bt_param.link_key = param->link_key_info.link_key;

            uint8_t   peer_addr[6];

            remote_peer_addr_get(peer_addr);
            if (memcmp(peer_addr, bt_param.bd_addr, 6) == 0)
            {
                state_machine(EVENT_B2B_AUTH_LINK_KEY_INFO, &bt_param);
            }
            else
            {
                state_machine(EVENT_SRC_AUTH_LINK_KEY_INFO, &bt_param);
            }
        }
        break;

    case BT_EVENT_LINK_KEY_REQ:
        {
            bt_param.bd_addr = param->link_key_req.bd_addr;

            uint8_t   peer_addr[6];

            remote_peer_addr_get(peer_addr);
            if (memcmp(peer_addr, bt_param.bd_addr, 6) == 0)
            {
                state_machine(EVENT_B2B_AUTH_LINK_KEY_REQ, &bt_param);
            }
            else
            {
                state_machine(EVENT_SRC_AUTH_LINK_KEY_REQ, &bt_param);
            }
        }
        break;

    case BT_EVENT_LINK_PIN_CODE_REQ:
        {
            bt_param.bd_addr = param->link_pin_code_req.bd_addr;
            bt_link_pin_code_cfm(param->link_pin_code_req.bd_addr, app_cfg_nv.pin_code,
                                 app_cfg_nv.pin_code_size, true);
            state_machine(EVENT_SRC_AUTH_LINK_PIN_CODE_REQ, &bt_param);
        }
        break;

    case BT_EVENT_SCO_CONN_IND:
        {
            app_sniff_mode_b2s_disable_all(SNIFF_DISABLE_MASK_SCO);
        }
        break;

    case BT_EVENT_SCO_CONN_RSP:
        {
            if (param->sco_conn_rsp.cause == 0)
            {
                app_sniff_mode_b2s_disable_all(SNIFF_DISABLE_MASK_SCO);
            }
        }
        break;

    case BT_EVENT_SCO_CONN_CMPL:
        {
            T_APP_BR_LINK *p_link = NULL;
            p_link = app_find_br_link(param->sco_conn_cmpl.bd_addr);

            if (p_link != NULL && param->sco_conn_cmpl.cause == 0)
            {
                p_link->sco_handle = param->sco_conn_cmpl.handle;

                if (app_bt_policy_get_radio_mode() == BT_DEVICE_MODE_DISCOVERABLE_CONNECTABLE)
                {
                    app_bt_policy_exit_pairing_mode();
                }

                state_machine(EVENT_SCO_CONN_CMPL, NULL);
                app_bt_policy_qos_param_update(param->sco_conn_cmpl.bd_addr, BP_TPOLL_SCO_CONN_EVENT);
                app_hfp_adjust_sco_window(param->sco_conn_cmpl.bd_addr, APP_SCO_ADJUST_SCO_CONN_CMPL_EVT);

#if F_APP_HARMAN_FEATURE_SUPPORT
#if HARMAN_SUPPORT_WATER_EJECTION
                app_harman_water_ejection_stop();
#endif
                app_harman_sco_status_notify();
                au_connect_idle_to_power_off(ACTIVE_NEED_STOP_COUNT, p_link->id);
#endif
            }
            else
            {
                if (app_find_sco_conn_num() == 0)
                {
                    app_sniff_mode_b2s_enable_all(SNIFF_DISABLE_MASK_SCO);
                }
            }
        }
        break;

    case BT_EVENT_SCO_DISCONNECTED:
        {
            if (app_find_sco_conn_num() <= 1)
            {
                app_sniff_mode_b2s_enable_all(SNIFF_DISABLE_MASK_SCO);
            }

            state_machine(EVENT_SCO_DISCONNECTED, NULL);
        }
        break;

    case BT_EVENT_SDP_ATTR_INFO:
        {
            bt_param.bd_addr = param->sdp_attr_info.bd_addr;
            bt_param.sdp_info = &param->sdp_attr_info.info;

#if F_APP_TEAMS_BT_POLICY
            app_teams_bt_handle_sdp_info(bt_param.bd_addr, bt_param.sdp_info);
#endif

            state_machine(EVENT_PROFILE_SDP_ATTR_INFO, &bt_param);
        }
        break;

    case  BT_EVENT_DID_ATTR_INFO:
        {
            T_APP_BR_LINK *p_link = NULL;
            uint8_t pair_idx_mapping;

            p_link = app_find_br_link(param->did_attr_info.bd_addr);

            if (p_link != NULL)
            {
                if (param->did_attr_info.vendor_id == 0x004c)
                {
                    p_link->remote_device_vendor_id = APP_REMOTE_DEVICE_IOS;
                }
                else
                {
                    p_link->remote_device_vendor_id = APP_REMOTE_DEVICE_OTHERS;
                }

                app_bond_get_pair_idx_mapping(p_link->bd_addr, &pair_idx_mapping);
                app_cfg_nv.remote_device_vendor_id[pair_idx_mapping] = p_link->remote_device_vendor_id;

                bt_param.bd_addr = param->did_attr_info.bd_addr;
                state_machine(EVENT_PROFILE_DID_ATTR_INFO, &bt_param);

#if (F_APP_AVP_INIT_SUPPORT == 1)
                if (p_link != NULL)
                {
                    if (p_link->remote_device_vendor_id == APP_REMOTE_DEVICE_OTHERS)
                    {
                        if ((app_cfg_const.enable_multi_link) && (b2s_connected.num == 1) &&
                            (discoverable_when_one_link == true))
                        {
                            discoverable_when_one_link = false;
                        }

                        if (timer_handle_discoverable != NULL)
                        {
                            gap_stop_timer(&timer_handle_discoverable);
                            state_machine(EVENT_DISCOVERABLE_TIMEOUT, NULL);
                        }
                    }
                }
#endif

#if (F_APP_DONGLE_FEATURE_SUPPORT == 1)
                if ((param->did_attr_info.vendor_id == 0x0bda) &&
                    ((param->did_attr_info.product_id == 0x875a) || (param->did_attr_info.product_id == 0x875b)))
                {
                    app_dongle_record_init();
                    app_db.remote_is_8753bau = true;
                    app_relay_async_single(APP_MODULE_TYPE_AUDIO_POLICY, APP_REMOTE_MSG_SYNC_REMOTE_IS_8753BAU);
                    /*
                    //reserve for keeping dongle link record
                    bt_bond_flag_add(param->acl_conn_success.bd_addr, BOND_FLAG_DONGLE);
                    T_APP_REMOTE_MSG_PAYLOAD_PROFILE_CONNECTED msg;
                    memcpy(msg.bd_addr, param->acl_conn_success.bd_addr, 6);
                    msg.prof_mask = DID_PROFILE_MASK;
                    app_relay_async_single_with_raw_msg(APP_MODULE_TYPE_MULTI_LINK, APP_REMOTE_MSG_PROFILE_CONNECTED,
                                                        (uint8_t *)&msg, sizeof(T_APP_REMOTE_MSG_PAYLOAD_PROFILE_CONNECTED));
                    */
                    if (app_db.gaming_mode)
                    {
                        app_db.restore_gaming_mode = true;
                        app_db.disallow_play_gaming_mode_vp = true;

                        app_mmi_switch_gaming_mode();

                        if (app_db.remote_session_state == REMOTE_SESSION_STATE_CONNECTED)
                        {
                            uint8_t cmd = true;

                            app_relay_async_single(APP_MODULE_TYPE_AUDIO_POLICY,
                                                   APP_REMOTE_MSG_SYNC_DISALLOW_PLAY_GAMING_MODE_VP);
                            app_relay_async_single_with_raw_msg(APP_MODULE_TYPE_AUDIO_POLICY,
                                                                APP_REMOTE_MSG_ASK_TO_EXIT_GAMING_MODE,
                                                                (uint8_t *)&cmd, sizeof(cmd));
                        }

                        if (p_link->connected_profile & SPP_PROFILE_MASK)
                        {
                            //Exception handle: bau had sent cmd before receiving DID info.
                            app_db.restore_gaming_mode = false;
                            app_db.gaming_mode_request_is_received = false;
                            app_dongle_handle_gaming_mode_cmd(MMI_DEV_GAMING_MODE_SWITCH);
                        }
                    }
                }
#endif
            }
        }
        break;

    case BT_EVENT_SDP_DISCOV_CMPL:
        {
            bt_param.bd_addr = param->sdp_discov_cmpl.bd_addr;
            bt_param.cause = param->sdp_discov_cmpl.cause;
#if F_APP_TEAMS_BT_POLICY
            app_teams_bt_handle_sdp_cmpl_or_stop(param->sdp_discov_cmpl.bd_addr);
#endif

            state_machine(EVENT_PROFILE_SDP_DISCOV_CMPL, &bt_param);
        }
        break;

    case BT_EVENT_SDP_DISCOV_STOP:
        {
            bt_param.not_check_addr_flag = true;
#if F_APP_TEAMS_BT_POLICY
            app_teams_bt_handle_sdp_cmpl_or_stop(param->sdp_discov_stop.bd_addr);
#endif

            state_machine(EVENT_PROFILE_SDP_DISCOV_STOP, &bt_param);
        }
        break;

    case BT_EVENT_HFP_CONN_CMPL:
        {
#if F_APP_NFC_SUPPORT
            uint8_t i;
            uint8_t app_idx;
#endif
            T_APP_BR_LINK *p_link;

            p_link = app_find_br_link(param->hfp_conn_cmpl.bd_addr);

            if (p_link != NULL)
            {
                bt_param.bd_addr = param->hfp_conn_cmpl.bd_addr;
                if (param->hfp_conn_cmpl.is_hfp)
                {
                    bt_param.prof = HFP_PROFILE_MASK;
                }
                else
                {
                    bt_param.prof = HSP_PROFILE_MASK;
                }
#if F_APP_NFC_SUPPORT
                app_idx = p_link->id;
                if (nfc_data.nfc_multi_link_switch & NFC_MULTI_LINK_SWITCH_HF)
                {
                    nfc_data.nfc_multi_link_switch &= ~NFC_MULTI_LINK_SWITCH_HF;

                    for (i = 0; i < MAX_BR_LINK_NUM; i++)
                    {
                        if ((app_db.br_link[app_idx].connected_profile) &&
                            (i != app_idx))
                        {
                            app_bt_policy_disconnect(app_db.br_link[i].bd_addr, ALL_PROFILE_MASK);
                        }
                    }
                }
#endif
            }
            state_machine(EVENT_PROFILE_CONN_SUC, &bt_param);
        }
        break;

    case BT_EVENT_HFP_SNIFFING_DISCONN_CMPL:
    case BT_EVENT_HFP_DISCONN_CMPL:
        {
#if F_APP_NFC_SUPPORT
            nfc_data.nfc_multi_link_switch &= ~NFC_MULTI_LINK_SWITCH_HF;
#endif
            bt_param.bd_addr = param->hfp_disconn_cmpl.bd_addr;
            if (param->hfp_conn_cmpl.is_hfp)
            {
                bt_param.prof = HFP_PROFILE_MASK;
            }
            else
            {
                bt_param.prof = HSP_PROFILE_MASK;
            }
            bt_param.cause = param->hfp_disconn_cmpl.cause;

            state_machine(EVENT_PROFILE_DISCONN, &bt_param);
        }
        break;

    case BT_EVENT_HFP_CONN_FAIL:
        {
            bt_param.bd_addr = param->hfp_conn_fail.bd_addr;
            if (param->hfp_conn_fail.is_hfp)
            {
                bt_param.prof = HFP_PROFILE_MASK;
            }
            else
            {
                bt_param.prof = HSP_PROFILE_MASK;
            }
            bt_param.cause = param->hfp_conn_fail.cause;

            state_machine(EVENT_PROFILE_CONN_FAIL, &bt_param);
        }
        break;

    case BT_EVENT_PBAP_CONN_CMPL:
        {
            bt_param.bd_addr = param->pbap_conn_cmpl.bd_addr;
            bt_param.prof = PBAP_PROFILE_MASK;

            state_machine(EVENT_PROFILE_CONN_SUC, &bt_param);
        }
        break;

    case BT_EVENT_PBAP_SNIFFING_DISCONN_CMPL:
    case BT_EVENT_PBAP_DISCONN_CMPL:
        {
            bt_param.bd_addr = param->pbap_disconn_cmpl.bd_addr;
            bt_param.prof = PBAP_PROFILE_MASK;
            bt_param.cause = param->pbap_disconn_cmpl.cause;

            state_machine(EVENT_PROFILE_DISCONN, &bt_param);
        }
        break;

    case BT_EVENT_PBAP_CONN_FAIL:
        {
            bt_param.bd_addr = param->pbap_conn_fail.bd_addr;
            bt_param.prof = PBAP_PROFILE_MASK;
            bt_param.cause = param->pbap_conn_fail.cause;

            state_machine(EVENT_PROFILE_CONN_FAIL, &bt_param);
        }
        break;

#if F_APP_HID_SUPPORT
    case BT_EVENT_HID_CONN_CMPL:
        {
            bt_param.bd_addr = param->hid_conn_cmpl.bd_addr;
            bt_param.prof = HID_PROFILE_MASK;

            state_machine(EVENT_PROFILE_CONN_SUC, &bt_param);
        }
        break;

    case BT_EVENT_HID_SNIFFING_DISCONN_CMPL:
    case BT_EVENT_HID_DISCONN_CMPL:
        {
            bt_param.bd_addr = param->hid_disconn_cmpl.bd_addr;
            bt_param.prof = HID_PROFILE_MASK;
            bt_param.cause = param->hid_disconn_cmpl.cause;

            state_machine(EVENT_PROFILE_DISCONN, &bt_param);
        }
        break;

    case BT_EVENT_HID_CONN_FAIL:
        {
            bt_param.bd_addr = param->hid_conn_fail.bd_addr;
            bt_param.prof = HID_PROFILE_MASK;
            bt_param.cause = param->hid_conn_fail.cause;

            state_machine(EVENT_PROFILE_CONN_FAIL, &bt_param);
        }
        break;
#endif

    //case BT_EVENT_A2DP_CONN_CMPL:
    case BT_EVENT_A2DP_STREAM_OPEN:
        {
#if F_APP_NFC_SUPPORT
            uint8_t i;
            uint8_t app_idx;
#endif
            T_APP_BR_LINK *p_link;

            p_link = app_find_br_link(param->a2dp_stream_open.bd_addr);
            if (p_link != NULL)
            {
                bt_param.bd_addr = param->a2dp_stream_open.bd_addr;
                bt_param.prof = A2DP_PROFILE_MASK;
#if F_APP_NFC_SUPPORT
                app_idx = p_link->id;
                if (nfc_data.nfc_multi_link_switch & NFC_MULTI_LINK_SWITCH_A2DP)
                {
                    nfc_data.nfc_multi_link_switch &= ~NFC_MULTI_LINK_SWITCH_A2DP;

                    for (i = 0; i < MAX_BR_LINK_NUM; i++)
                    {
                        if ((app_db.br_link[app_idx].connected_profile) &&
                            (i != app_idx))
                        {
                            app_bt_policy_disconnect(app_db.br_link[i].bd_addr, ALL_PROFILE_MASK);
                        }
                    }
                }
#endif
            }

            app_bt_policy_default_connect(param->a2dp_stream_open.bd_addr, DID_PROFILE_MASK, false);

            state_machine(EVENT_PROFILE_CONN_SUC, &bt_param);
        }
        break;

    case BT_EVENT_A2DP_SNIFFING_DISCONN_CMPL:
    case BT_EVENT_A2DP_DISCONN_CMPL:
        {
#if F_APP_NFC_SUPPORT
            nfc_data.nfc_multi_link_switch &= ~NFC_MULTI_LINK_SWITCH_A2DP;
#endif
            bt_param.bd_addr = param->a2dp_disconn_cmpl.bd_addr;
            bt_param.prof = A2DP_PROFILE_MASK;
            bt_param.cause = param->a2dp_disconn_cmpl.cause;

            state_machine(EVENT_PROFILE_DISCONN, &bt_param);
        }
        break;

    case BT_EVENT_A2DP_CONN_FAIL:
        {
            bt_param.bd_addr = param->a2dp_conn_fail.bd_addr;
            bt_param.prof = A2DP_PROFILE_MASK;
            bt_param.cause = param->a2dp_conn_fail.cause;

            state_machine(EVENT_PROFILE_CONN_FAIL, &bt_param);
        }
        break;

    case BT_EVENT_AVRCP_CONN_CMPL:
        {
            bt_param.bd_addr = param->avrcp_conn_cmpl.bd_addr;
            bt_param.prof = AVRCP_PROFILE_MASK;

            state_machine(EVENT_PROFILE_CONN_SUC, &bt_param);
        }
        break;

    case BT_EVENT_AVRCP_SNIFFING_DISCONN_CMPL:
    case BT_EVENT_AVRCP_DISCONN_CMPL:
        {
            bt_param.bd_addr = param->avrcp_disconn_cmpl.bd_addr;
            bt_param.prof = AVRCP_PROFILE_MASK;
            bt_param.cause = param->avrcp_disconn_cmpl.cause;

            state_machine(EVENT_PROFILE_DISCONN, &bt_param);
        }
        break;

    case BT_EVENT_AVRCP_CONN_FAIL:
        {
            bt_param.bd_addr = param->avrcp_conn_fail.bd_addr;
            bt_param.prof = AVRCP_PROFILE_MASK;
            bt_param.cause = param->avrcp_conn_fail.cause;

            state_machine(EVENT_PROFILE_CONN_FAIL, &bt_param);
        }
        break;

    case BT_EVENT_SPP_CONN_CMPL:
        {
            bt_param.bd_addr = param->spp_conn_cmpl.bd_addr;
            bt_param.prof = SPP_PROFILE_MASK;

            state_machine(EVENT_PROFILE_CONN_SUC, &bt_param);
        }
        break;

    case BT_EVENT_SPP_SNIFFING_DISCONN_CMPL:
    case BT_EVENT_SPP_DISCONN_CMPL:
        {
            bt_param.bd_addr = param->spp_disconn_cmpl.bd_addr;
            bt_param.prof = SPP_PROFILE_MASK;
            bt_param.cause = param->spp_disconn_cmpl.cause;

            state_machine(EVENT_PROFILE_DISCONN, &bt_param);
        }
        break;

    case BT_EVENT_SPP_CONN_FAIL:
        {
            bt_param.bd_addr = param->spp_conn_fail.bd_addr;
            bt_param.prof = SPP_PROFILE_MASK;
            bt_param.cause = param->spp_conn_fail.cause;

            state_machine(EVENT_PROFILE_CONN_FAIL, &bt_param);
        }
        break;

    case BT_EVENT_IAP_CONN_CMPL:
        {
            bt_param.bd_addr = param->iap_conn_cmpl.bd_addr;
            bt_param.prof = IAP_PROFILE_MASK;

            state_machine(EVENT_PROFILE_CONN_SUC, &bt_param);
        }
        break;

    case BT_EVENT_IAP_SNIFFING_DISCONN_CMPL:
    case BT_EVENT_IAP_DISCONN_CMPL:
        {
            bt_param.bd_addr = param->iap_disconn_cmpl.bd_addr;
            bt_param.prof = IAP_PROFILE_MASK;
            bt_param.cause = param->iap_disconn_cmpl.cause;

            state_machine(EVENT_PROFILE_DISCONN, &bt_param);
        }
        break;

    case BT_EVENT_IAP_CONN_FAIL:
        {
            bt_param.bd_addr = param->iap_conn_fail.bd_addr;
            bt_param.prof = IAP_PROFILE_MASK;
            bt_param.cause = param->iap_conn_fail.cause;

            state_machine(EVENT_PROFILE_CONN_FAIL, &bt_param);
        }
        break;

#if F_APP_ERWS_SUPPORT
    case BT_EVENT_ACL_SNIFFING_CONN_CMPL:
        {
            T_APP_BR_LINK *p_link = NULL;
            uint8_t temp_buff[7] = {0};

            bt_param.bd_addr = param->acl_sniffing_conn_cmpl.bd_addr;

            if (param->acl_sniffing_conn_cmpl.cause == HCI_SUCCESS)
            {
                if (app_cfg_nv.bud_role == REMOTE_SESSION_ROLE_SECONDARY)
                {
                    state_machine(EVENT_SRC_CONN_SUC, &bt_param);
                }
                else
                {
                    p_link = app_find_br_link(bt_param.bd_addr);
                    if (p_link != NULL)
                    {
                        temp_buff[0] = (uint8_t)p_link->remote_device_vendor_id;
                        memcpy(&temp_buff[1], bt_param.bd_addr, 6);

                        app_relay_async_single_with_raw_msg(APP_MODULE_TYPE_DEVICE, APP_REMOTE_MSG_SPK1_REPLY_SRC_IS_IOS,
                                                            temp_buff, sizeof(temp_buff));

                        app_avrcp_sync_abs_vol_state(bt_param.bd_addr, p_link->abs_vol_state);
                    }
                }
            }
        }
        break;

    case BT_EVENT_ACL_SNIFFING_DISCONN_CMPL:
        if (app_cfg_nv.bud_role == REMOTE_SESSION_ROLE_SECONDARY)
        {
            bt_param.bd_addr = param->acl_sniffing_disconn_cmpl.bd_addr;

            state_machine(EVENT_SRC_DISCONN_NORMAL, &bt_param);
        }
        break;


    case BT_EVENT_REMOTE_ROLESWAP_STATUS:
        {
            if (param->remote_roleswap_status.status == BT_ROLESWAP_STATUS_SUCCESS)
            {
                bt_param.is_suc = true;
                bt_param.bud_role = param->remote_roleswap_status.device_role;
                state_machine(EVENT_ROLESWAP, &bt_param);
            }
            else if (param->remote_roleswap_status.status == BT_ROLESWAP_STATUS_FAIL ||
                     param->remote_roleswap_status.status == BT_ROLESWAP_STATUS_TERMINATED ||
                     ((param->remote_roleswap_status.status == BT_ROLESWAP_STATUS_START_RSP) &&
                      (!param->remote_roleswap_status.u.start_rsp.accept)))
            {
                bt_param.is_suc = false;
                bt_param.bud_role = param->remote_roleswap_status.device_role;
                state_machine(EVENT_ROLESWAP, &bt_param);
            }
            else if ((param->remote_roleswap_status.status == BT_ROLESWAP_STATUS_START_RSP) &&
                     (param->remote_roleswap_status.u.start_rsp.accept))
            {
                app_relay_async_single(APP_MODULE_TYPE_BT_POLICY, BT_POLICY_MSG_ROLESWAP_INFO);
                app_sniff_mode_send_roleswap_info();
                relay_pri_linkback_node();
            }
        }
        break;

    case BT_EVENT_REMOTE_CONN_CMPL:
        {
            bt_param.bd_addr = param->remote_conn_cmpl.bd_addr;
            bt_param.prof = RDTP_PROFILE_MASK;

            state_machine(EVENT_BUD_REMOTE_CONN_CMPL, &bt_param);
            app_sniff_mode_b2b_enable(bt_param.bd_addr, SNIFF_DISABLE_MASK_ACL);

            if (app_cfg_const.rws_remote_link_encryption_off)
            {
                if (app_cfg_nv.bud_role == REMOTE_SESSION_ROLE_PRIMARY)
                {
                    bt_link_encryption_set(bt_param.bd_addr, false);
                }
            }

            if (app_db.need_sync_case_battery_to_pri &&
                (app_cfg_const.bud_role == REMOTE_SESSION_ROLE_SECONDARY))
            {
                app_db.need_sync_case_battery_to_pri = false;
                app_relay_async_single(APP_MODULE_TYPE_CHARGER, APP_CHARGER_EVENT_BATT_CASE_LEVEL);
            }
        }
        break;

    case BT_EVENT_REMOTE_DISCONN_CMPL:
        {
            bt_param.bd_addr = param->remote_disconn_cmpl.bd_addr;
            bt_param.prof = RDTP_PROFILE_MASK;

            state_machine(EVENT_BUD_REMOTE_DISCONN_CMPL, &bt_param);
        }
        break;
#endif

    default:
        break;
    }
}

#if (F_APP_OTA_TOOLING_SUPPORT == 1)
static bool validate_bd_addr_with_dongle_id(uint8_t dongle_id, uint8_t *bd_addr)
{
    bool ret = false;

    if ((dongle_id == 0) || (app_cfg_nv.bud_role == REMOTE_SESSION_ROLE_SECONDARY))
    {
        // not OTA tooling mode or secondary
        ret = true;
    }
    else if (dongle_id >= 1)
    {
        uint8_t dongle_bd_addr[6] = {0, 0, 0, 0, 0xff, 0xaf};

        memcpy(dongle_bd_addr, &ota_dongle_addr[dongle_id - 1], 4);

        if (!memcmp(bd_addr, dongle_bd_addr, 6))
        {
            ret = true;
        }

        APP_PRINT_TRACE3("validate_bd_addr_with_dongle_id: ret = %d, dongle_id = %d, bd_addr = %s", ret,
                         dongle_id, TRACE_BDADDR(bd_addr));
    }

    return ret;
}
#endif

void app_bt_policy_abandon_engage(void)
{
    if (!engage_done)
    {
        engage_done = true;
        engage_afe_done();
    }
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#if F_APP_ERWS_SUPPORT

static void pri_collect_relay_info_for_roleswap(T_BP_ROLESWAP_INFO *bp_roleswap_info)
{
    uint8_t app_idx;

    if (app_cfg_nv.bud_role == REMOTE_SESSION_ROLE_PRIMARY)
    {
        bp_roleswap_info->last_src_conn_idx = last_src_conn_idx;
        bp_roleswap_info->bp_state = bp_state;

        for (app_idx = 0; app_idx < MAX_BR_LINK_NUM; app_idx++)
        {
            if (app_check_b2s_link_by_id(app_idx))
            {
                bp_roleswap_info->link.link_src_conn_idx = app_db.br_link[app_idx].src_conn_idx;
                bp_roleswap_info->link.remote_device_vendor_id = app_db.br_link[app_idx].remote_device_vendor_id;
                bp_roleswap_info->link.abs_vol_state = app_db.br_link[app_idx].abs_vol_state;
                bp_roleswap_info->link.rtk_vendor_spp_active = app_db.br_link[app_idx].rtk_vendor_spp_active;
                bp_roleswap_info->link.tx_event_seqn = app_db.br_link[app_idx].tx_event_seqn;
                bp_roleswap_info->link.b2s_connected_vp_is_played =
                    app_db.br_link[app_idx].b2s_connected_vp_is_played;
#if GFPS_FEATURE_SUPPORT
                bp_roleswap_info->link.connected_by_linkback = app_db.br_link[app_idx].connected_by_linkback;
                bp_roleswap_info->link.gfps_rfc_chann = app_db.br_link[app_idx].gfps_rfc_chann;
                bp_roleswap_info->link.gfps_inuse_account_key = app_db.br_link[app_idx].gfps_inuse_account_key;
                memcpy(bp_roleswap_info->link.gfps_session_nonce, app_db.br_link[app_idx].gfps_session_nonce, 8);
                if (app_db.br_link[app_idx].connected_profile & GFPS_PROFILE_MASK)
                {
                    bp_roleswap_info->link.gfps_rfc_connected = true;
                }
#endif

            }
        }

    }
}

static void sec_recv_relay_info_for_roleswap(void *buf, uint16_t len)
{
    T_BP_ROLESWAP_INFO *p_info = (T_BP_ROLESWAP_INFO *)buf;

    if (app_cfg_nv.bud_role == REMOTE_SESSION_ROLE_SECONDARY)
    {
        if (sizeof(*p_info) == len)
        {
            memcpy(&bp_roleswap_info_temp, p_info, len);
        }
    }
}

static void new_pri_apply_relay_info_when_roleswap_suc(void)
{
    uint8_t app_idx;

    if (app_cfg_nv.bud_role == REMOTE_SESSION_ROLE_PRIMARY)
    {
        last_src_conn_idx = bp_roleswap_info_temp.last_src_conn_idx;
        pri_bp_state = bp_roleswap_info_temp.bp_state;

        for (app_idx = 0; app_idx < MAX_BR_LINK_NUM; app_idx++)
        {
            if (app_check_b2s_link_by_id(app_idx))
            {
                app_db.br_link[app_idx].src_conn_idx = bp_roleswap_info_temp.link.link_src_conn_idx;
                app_db.br_link[app_idx].remote_device_vendor_id =
                    bp_roleswap_info_temp.link.remote_device_vendor_id;
                app_db.br_link[app_idx].abs_vol_state = bp_roleswap_info_temp.link.abs_vol_state;
                app_db.br_link[app_idx].rtk_vendor_spp_active = bp_roleswap_info_temp.link.rtk_vendor_spp_active;
                app_db.br_link[app_idx].tx_event_seqn = bp_roleswap_info_temp.link.tx_event_seqn;
                app_db.br_link[app_idx].b2s_connected_vp_is_played =
                    bp_roleswap_info_temp.link.b2s_connected_vp_is_played;
#if GFPS_FEATURE_SUPPORT
                app_db.br_link[app_idx].connected_by_linkback = bp_roleswap_info_temp.link.connected_by_linkback;
                app_db.br_link[app_idx].gfps_rfc_chann = bp_roleswap_info_temp.link.gfps_rfc_chann;
                app_db.br_link[app_idx].gfps_inuse_account_key = bp_roleswap_info_temp.link.gfps_inuse_account_key;
                memcpy(app_db.br_link[app_idx].gfps_session_nonce, bp_roleswap_info_temp.link.gfps_session_nonce,
                       8);
                if (bp_roleswap_info_temp.link.gfps_rfc_connected)
                {
                    app_db.br_link[app_idx].connected_profile |= GFPS_PROFILE_MASK;
                }
#endif
            }
        }
    }
}

static uint16_t app_bt_policy_relay_cback(uint8_t *buf, uint8_t msg_type, bool total)
{
    uint16_t payload_len = 0;
    uint8_t *msg_ptr = NULL;
    bool skip = true;

    T_BP_ROLESWAP_INFO bp_roleswap_info;

    switch (msg_type)
    {
    case BT_POLICY_MSG_ROLESWAP_INFO:
        {
            pri_collect_relay_info_for_roleswap(&bp_roleswap_info);

            payload_len = sizeof(bp_roleswap_info);
            msg_ptr = (uint8_t *)&bp_roleswap_info;
        }
        break;

    case BT_POLICY_MSG_PRI_BP_STATE:
        {
            payload_len = sizeof(bp_state);
            msg_ptr = (uint8_t *)&bp_state;
        }
        break;

    default:
        break;
    }

    return app_relay_msg_pack(buf, msg_type, APP_MODULE_TYPE_BT_POLICY, payload_len, msg_ptr, skip,
                              total);
}

static void app_bt_policy_parse_cback(uint8_t msg_type, uint8_t *buf, uint16_t len,
                                      T_REMOTE_RELAY_STATUS status)
{
    if (status == REMOTE_RELAY_STATUS_ASYNC_RCVD)
    {
        switch (msg_type)
        {
        case BT_POLICY_MSG_ROLESWAP_INFO:
            {
                sec_recv_relay_info_for_roleswap(buf, len);
            }
            break;

        case BT_POLICY_MSG_PRI_BP_STATE:
            {
                recv_relay_pri_bp_state(buf, len);
            }
            break;

        case BT_POLICY_MSG_PRI_REQ:
            {
                recv_relay_pri_req(buf, len);
            }
            break;

        case BT_POLICY_MSG_SNIFF_MODE:
            {
                app_sniff_mode_recv_roleswap_info(buf, len);
            }
            break;

        case BT_POLICY_MSG_LINKBACK_NODE:
            {
                recv_relay_pri_linkback_node(buf, len);
            }
            break;

        case BT_POLICY_MSG_B2S_ACL_CONN:
            {
                app_bt_policy_b2s_conn_start_timer();
            }
            break;

        case BT_POLICY_MSG_SYNC_SERVICE_STATUS:
            {
                if (app_cfg_nv.bud_role == REMOTE_SESSION_ROLE_SECONDARY)
                {
                    bool all_service_status = *((bool *)buf);
                    app_hfp_set_no_service_timer(all_service_status);
                }
            }
            break;

        default:
            break;
        }
    }
}
#endif

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void app_bt_stop_a2dp_and_sco(void)
{
    T_APP_BR_LINK *p_link = NULL;

    for (uint8_t i = 0; i < MAX_BR_LINK_NUM; i++)
    {
        if (app_check_b2s_link_by_id(i))
        {
            p_link = &app_db.br_link[i];

            if (p_link)
            {
                if (p_link->a2dp_track_handle)
                {
                    audio_track_stop(p_link->a2dp_track_handle);
                }

                if (p_link->sco_track_handle)
                {
                    audio_track_stop(p_link->sco_track_handle);
                }
            }
        }
    }
}

#if F_APP_LE_AUDIO_SM
void app_bt_policy_set_legacy(uint8_t para)
{
    APP_PRINT_INFO2("app_bt_policy_set_legacy: para %d, state %d", para, app_bt_policy_get_state());
    if (app_cfg_nv.bud_role == REMOTE_SESSION_ROLE_SECONDARY)
    {
        return;
    }

    if (para == 0)
    {
        if (timer_handle_pairing_mode  != NULL)
        {
            gap_stop_timer(&timer_handle_pairing_mode);
        }
    }
    else if (para == 1)
    {

        if ((app_le_audio_dev_ctrl(T_LEA_DEV_CRL_GET_LEGACY, NULL) ||
             app_le_audio_dev_ctrl(T_LEA_DEV_CRL_GET_CIS_POLICY, NULL)) &&
            app_cfg_const.timer_pairing_timeout  &&
            (app_bt_policy_get_state() == BP_STATE_PAIRING_MODE))
        {
            gap_stop_timer(&timer_handle_pairing_mode);
            gap_start_timer(&timer_handle_pairing_mode, "pairing_timer", timer_queue_id,
                            TIMER_ID_PAIRING_TIMEOUT, 0, app_cfg_const.timer_pairing_timeout * 1000);
        }
    }
}

bool app_bt_policy_is_pairing(void)
{
    if (timer_handle_pairing_mode)
    {
        return true;
    }
    else
    {
        return false;
    }
}

bool app_bt_policy_listening_allow_poweroff(void)
{
#if F_APP_LISTENING_MODE_SUPPORT

    if (app_cfg_const.enable_auto_power_off_when_anc_apt_on
        || (app_db.current_listening_state == ANC_OFF_APT_OFF))
    {
        return true;
    }
    else
    {
        return false;
    }
#else
    return true;
#endif

}
#endif

void app_bt_policy_init(void)
{
#if F_APP_ERWS_SUPPORT
    app_relay_cback_register(app_bt_policy_relay_cback, app_bt_policy_parse_cback,
                             APP_MODULE_TYPE_BT_POLICY, BT_POLICY_MSG_MAX);
#endif

    bt_mgr_cback_register(app_bt_policy_cback);
    gap_reg_timer_cb(timer_cback, &timer_queue_id);

#if F_APP_ERWS_SUPPORT
    {
        engage_init();
    }
#endif
}
