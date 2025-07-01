#include <stdlib.h>
#include <string.h>
#include "trace.h"
#include "ascs_mgr.h"
#include "bass_mgr.h"
#include "ble_conn.h"
#include "ble_isoch_def.h"
#include "bt_direct_msg.h"
#include "bt_types.h"
#include "ccp_client.h"
#include "gap_iso_data.h"
#include "mcs_client.h"
#include "vcs_mgr.h"
#include "app_broadcast_sync.h"
#include "app_ccp.h"
#include "app_cfg.h"
#include "app_hfp.h"
#include "app_le_audio_adv.h"
#include "app_le_audio_lib.h"
#include "app_le_audio_mgr.h"
#include "app_le_audio_scan.h"
#include "app_le_profile.h"
#include "app_link_util.h"
#include "app_main.h"
#include "app_mcp.h"
#include "app_mmi.h"
#include "app_multilink.h"
#include "app_vcs.h"
#include "app_sniff_mode.h"
#include "app_auto_power_off.h"
#include "app_audio_policy.h"
#include "app_bass.h"

#if F_APP_LE_AUDIO_SM
#define DATA_PATH_MASK        (DATA_PATH_INPUT_FLAG|DATA_PATH_OUTPUT_FLAG)

bool big_passive_flag = 0;

#if F_APP_TMAP_BMR_SUPPORT
extern uint8_t source_id;
#endif

static T_LE_AUDIO_BIS_SM bis_audio_state_machine = LE_AUDIO_BIS_STATE_IDLE;
static T_LE_AUDIO_DEVICE_SM le_audio_device_sm = LE_AUDIO_DEVICE_IDLE;
static T_BT_CCP_CALL_STATUS ccp_call_status = BT_CCP_CALL_IDLE;

static T_BIS_CB bis_cb[APP_SYNC_RECEIVER_MAX_BIS_NUM];
T_ISOCH_DATA_PKT_STATUS media_state = ISOCH_DATA_PKT_STATUS_LOST_DATA;

extern void *audio_evt_queue_handle;  //!< Event queue handle
extern void *audio_io_queue_handle;   //!< IO queue handle

T_BLE_ASE_LINK *app_le_audio_find_ase_by_conn_handle(uint16_t conn_handle);
T_BT_CCP_CALL_STATUS app_le_audio_get_call_status(void)
{
    return ccp_call_status;
}

void app_le_audio_device_state_change(T_LE_AUDIO_DEVICE_SM state)
{
    APP_PRINT_INFO2("app_le_audio_device_state_change: change from %d to %d", le_audio_device_sm,
                    state);
    le_audio_device_sm = state;
}

void app_lea_bis_state_change(T_LE_AUDIO_BIS_SM state)
{
    APP_PRINT_INFO2("app_lea_bis_state_change: change from %d to %d", bis_audio_state_machine,
                    state);
    bis_audio_state_machine = state;
}

T_LE_AUDIO_DEVICE_SM app_lea_audio_get_device_state(void)
{
    APP_PRINT_INFO1("app_lea_audio_get_device_state: %d ", le_audio_device_sm);
    return le_audio_device_sm;
}

T_LE_AUDIO_BIS_SM app_lea_bis_get_state(void)
{
    APP_PRINT_INFO1("app_lea_bis_get_state: bis_audio_state_machine %d", bis_audio_state_machine);
    return bis_audio_state_machine;
}

void app_le_audio_dev(bool *result)
{
    uint8_t i;
    *result = false;
    if (!app_cfg_const.legacy_enable)
    {
        for (i = 0; i < MAX_BR_LINK_NUM; i++)
        {
            if (app_cfg_nv.bud_role == REMOTE_SESSION_ROLE_PRIMARY &&
                app_db.br_link[i].connected_profile == RDTP_PROFILE_MASK)
            {
                bt_device_mode_set(BT_DEVICE_MODE_IDLE);
                *result = true;
                break;
            }
        }
    }
}

bool app_le_audio_dev_ctrl(uint8_t para, uint8_t *data)
{
    bool result = false;
    switch ((T_LEA_BUD_DEV_CRL)para)
    {
    case T_LEA_DEV_CRL_SET_IDLE:
        {
            app_le_audio_dev(&result);
        }
        break;

    case T_LEA_DEV_CRL_GET_LEGACY:
        {
            if (app_cfg_const.legacy_enable)
            {
                result = true;
            }
        }
        break;

    case T_LEA_DEV_CRL_SET_RADIO:
        {
            if (app_cfg_const.legacy_enable)
            {
                result = bt_device_mode_set((T_BT_DEVICE_MODE) * data);
            }
            else
            {
                if ((T_BT_DEVICE_MODE) *data != BT_DEVICE_MODE_DISCOVERABLE_CONNECTABLE)
                {
                    result = bt_device_mode_set((T_BT_DEVICE_MODE) * data);
                }
            }
        } break;

    case T_LEA_DEV_CRL_GET_CIS_POLICY:
        {
            if (app_cfg_const.cis_autolink == T_LEA_CIS_AUTO &&
                (app_cfg_const.iso_mode == LE_AUDIO_CIS || app_cfg_const.iso_mode == LE_AUDIO_ALL))
            {
                result = true;
            }
        }
        break;
    }
    return result;
}


void app_lea_bis_data_path_reset(T_BIS_CB *p_bis_cb)
{
    if (p_bis_cb != NULL && p_bis_cb->audio_track_handle != NULL)
    {
        media_state  = ISOCH_DATA_PKT_STATUS_LOST_DATA;
        audio_track_release(p_bis_cb->audio_track_handle);
    }
    if (app_cfg_const.bis_mode == T_LEA_BROADCAST_SINK ||
        app_cfg_const.bis_mode == T_LEA_BROADCAST_DELEGATOR)
    {
        app_le_audio_device_sm(LE_AUDIO_BIGTERMINATE, NULL);
    }
    //mtc_resume_a2dp(mtc_get_resume_a2dp_idx());
    app_sniff_mode_b2s_enable_all(SNIFF_DISABLE_MASK_ISOAUDIO);
    app_sniff_mode_b2b_enable(app_cfg_nv.bud_peer_addr, SNIFF_DISABLE_MASK_ISOAUDIO);
    app_auto_power_off_enable(AUTO_POWER_OFF_MASK_BLE_AUDIO, app_cfg_const.timer_auto_power_off);

}

bool app_lea_sniff_check(T_MTC_AUDIO_MODE mode)
{
    if ((mtc_get_b2d_sniff_status() != MTC_BUD_STATUS_ACTIVE) &&
        (app_bt_policy_get_b2b_connected()))
    {
        if (app_cfg_nv.bud_role == REMOTE_SESSION_ROLE_PRIMARY)
        {

            app_sniff_mode_b2s_disable_all(SNIFF_DISABLE_MASK_ISOAUDIO);
            app_sniff_mode_b2b_disable(app_cfg_nv.bud_peer_addr, SNIFF_DISABLE_MASK_ISOAUDIO);

        }
        mtc_set_pending(mode);
        return true;
    }
    return false;
}

#if F_APP_TMAP_BMR_SUPPORT
void app_lea_trigger_mmi_handle_action(uint8_t action, bool inter)
{
    if (!app_cfg_const.iso_mode)
    {
        return;
    }
    APP_PRINT_INFO5("app_lea_trigger_mmi_handle_action: action: 0x%x, b2s: %d, b2b:%d local:%d, %d",
                    action,
                    mtc_get_b2s_sniff_status(), mtc_get_b2d_sniff_status(), inter, app_cfg_const.iso_mode);
    switch (action)
    {

    case MMI_BIG_START:
        {
            if (!(app_cfg_const.iso_mode & LE_AUDIO_BIS))
            {
                break;
            }

            if (app_hfp_get_call_status() != BT_HFP_CALL_IDLE)
            {
                break;
            }

            if (app_lea_bis_get_state() == LE_AUDIO_BIS_STATE_IDLE)
            {
                if (mtc_get_beep() == MTC_PRO_BEEP_PROMPTLY)
                {
                    app_audio_tone_type_play(TONE_BIS_START, false, false);
                }

                mtc_set_beep(MTC_PRO_BEEP_PROMPTLY);

                if (app_cfg_const.bis_mode == T_LEA_BROADCAST_SINK)
                {
                    app_lea_bis_state_change(LE_AUDIO_BIS_STATE_PRE_SCAN);
                }
                else if (app_cfg_const.bis_mode == T_LEA_BROADCAST_DELEGATOR)
                {
                    app_lea_bis_state_change(LE_AUDIO_BIS_STATE_PRE_ADV);
                }
            }
            else if (app_lea_bis_get_state() == LE_AUDIO_BIS_STATE_CONN)
            {
                ;
            }
            else if (!inter)
            {
                app_auto_power_off_disable(AUTO_POWER_OFF_MASK_BLE_AUDIO);
                break;
            }

            if (app_lea_sniff_check(LE_AUDIO_BIS))
            {
                break;
            }
            mtc_stream_switch();
            mtc_topology_dm(MTC_TOPO_EVENT_BIS);
            app_sniff_mode_b2s_disable_all(SNIFF_DISABLE_MASK_ISOAUDIO);
            app_sniff_mode_b2b_disable(app_cfg_nv.bud_peer_addr, SNIFF_DISABLE_MASK_ISOAUDIO);
            if (app_cfg_const.bis_mode == T_LEA_BROADCAST_SINK)
            {
                app_le_audio_device_sm(LE_AUDIO_BIGSYNC, NULL);
            }
            else if (app_cfg_const.bis_mode == T_LEA_BROADCAST_DELEGATOR)
            {
                app_le_audio_device_sm(LE_AUDIO_SCAN_DELEGATOR, NULL);
            }
            app_auto_power_off_disable(AUTO_POWER_OFF_MASK_BLE_AUDIO);
        }
        break;

    case MMI_BIG_STOP:
        {
            if (!(app_cfg_const.iso_mode & LE_AUDIO_BIS))
            {
                break;
            }
            T_BIS_CB *p_bis_cb = NULL;
            if (app_lea_bis_get_state() == LE_AUDIO_BIS_STATE_IDLE)
            {
                mtc_set_beep(MTC_PRO_BEEP_PROMPTLY);
                break;
            }
            if (app_cfg_const.bis_mode == T_LEA_BROADCAST_SINK)
            {
                p_bis_cb = app_le_audio_find_bis_by_conn_handle(app_bc_get_bis_handle());
            }
            else  if (app_cfg_const.bis_mode == T_LEA_BROADCAST_DELEGATOR)
            {
                p_bis_cb = app_le_audio_find_bis_by_source_id(app_bass_get_source());
            }
            if (mtc_get_beep() == MTC_PRO_BEEP_PROMPTLY)
            {
                app_audio_tone_type_play(TONE_BIS_STOP, false, false);
            }

            mtc_set_beep(MTC_PRO_BEEP_PROMPTLY);
            app_lea_bis_data_path_reset(p_bis_cb);

        }
        break;

    case MMI_CIG_START:
        {
            if (!(app_cfg_const.iso_mode & LE_AUDIO_CIS) &&
                (app_cfg_const.cis_autolink != T_LEA_CIS_MMI))
            {
                break;

            }
            if (!inter)
            {
                app_audio_tone_type_play(TONE_CIS_START, false, false);
            }

            if (app_lea_sniff_check(LE_AUDIO_CIS))
            {
                break;
            }

            app_sniff_mode_b2s_disable_all(SNIFF_DISABLE_MASK_ISOAUDIO);
            app_sniff_mode_b2b_disable(app_cfg_nv.bud_peer_addr, SNIFF_DISABLE_MASK_ISOAUDIO);
            mtc_stream_switch();
            app_le_audio_device_sm(LE_AUDIO_CIG_START, NULL);
            app_auto_power_off_disable(AUTO_POWER_OFF_MASK_BLE_AUDIO);
        }
        break;

    case MMI_CIG_STOP:
        {
            //reserved
        }
        break;

    default:
        break;
    }

}
#endif

bool app_le_audio_switch_remote_sync(void)
{
    bool ret = false;

    if (app_cfg_nv.bud_role == REMOTE_SESSION_ROLE_PRIMARY)
    {
        if (remote_session_state_get() == REMOTE_SESSION_STATE_CONNECTED)
        {
            uint8_t lea_key_type = mtc_get_btmode();
            app_relay_async_single_with_raw_msg(APP_MODULE_TYPE_ISOAUDIO,
                                                LEA_REMOTE_MMI_SWITCH_SYNC,
                                                (uint8_t *)&lea_key_type, sizeof(lea_key_type));
        }
    }
    return ret;
}

bool app_le_audio_call_allocate(uint8_t conn_id, uint8_t call_index, uint16_t call_state)
{
    bool ret = false;
    T_APP_LE_LINK *p_link;

    p_link = app_find_le_link_by_conn_id(conn_id);
    if (p_link != NULL)
    {
        T_BLE_CALL_LINK *p_call_link = NULL;
        uint8_t i;

        for (i = 0; i < CCP_CALL_LIST_NUM; i++)
        {
            if (p_link->ble_call_link[i].used == false)
            {
                p_call_link = &p_link->ble_call_link[i];

                p_call_link->used = true;
                p_call_link->call_index = call_index;
                p_call_link->call_state = call_state;
                ret = true;
                break;
            }
        }
    }

    return ret;
}


T_BLE_CALL_LINK *app_le_audio_find_call_link_by_idx(uint8_t conn_id, uint8_t call_index)
{
    T_APP_LE_LINK *p_link;
    T_BLE_CALL_LINK *p_call_link = NULL;

    p_link = app_find_le_link_by_conn_id(conn_id);
    if (p_link != NULL)
    {
        uint8_t i;

        for (i = 0; i < CCP_CALL_LIST_NUM; i++)
        {
            if (p_link->ble_call_link[i].used == true &&
                p_link->ble_call_link[i].call_index == call_index)
            {
                p_call_link = &p_link->ble_call_link[i];
                break;
            }
        }
    }

    return p_call_link;
}

T_BLE_CALL_LINK *app_le_audio_find_inactive_call_link(uint8_t conn_id, uint8_t active_call_index)
{
    T_APP_LE_LINK *p_link;
    T_BLE_CALL_LINK *p_inactive_call_link = NULL;

    p_link = app_find_le_link_by_conn_id(conn_id);
    if (p_link != NULL)
    {
        uint8_t i;

        for (i = 0; i < CCP_CALL_LIST_NUM; i++)
        {
            if (p_link->ble_call_link[i].used == true &&
                p_link->ble_call_link[i].call_index != active_call_index)
            {
                p_inactive_call_link = &p_link->ble_call_link[i];
                break;
            }
        }
    }

    return p_inactive_call_link;
}

T_BLE_ASE_LINK *app_le_audio_find_ase_by_context(uint16_t conn_handle, uint16_t audio_context)
{
    T_APP_LE_LINK *p_link;
    T_BLE_ASE_LINK *p_ase_link = NULL;

    p_link = app_find_le_link_by_conn_handle(conn_handle);
    if (p_link != NULL)
    {
        uint8_t i;

        for (i = 0; i < ASCS_ASE_NUM; i++)
        {
            if (p_link->ble_ase_link[i].used == true &&
                p_link->ble_ase_link[i].audio_context == audio_context)
            {
                p_ase_link = &p_link->ble_ase_link[i];
                break;
            }
        }
    }

    return p_ase_link;
}

void app_le_audio_mmi_handle_action(uint8_t action)
{
    uint16_t conn_handle = 0;
    bool handle = true;
    T_APP_LE_LINK *p_link = NULL;
#if F_APP_MCP_SUPPORT
    T_MCS_CLIENT_MCP_PARAM param;
    param.p_mcp_cb = NULL;
#endif
    APP_PRINT_INFO2("app_le_audio_mmi_handle_action: device action 0x%x, %d", action,
                    app_cfg_const.bis_mode);

    if (action == MMI_BIG_START ||
        action == MMI_BIG_STOP ||
        action == MMI_CIG_START ||
        action == MMI_CIG_STOP)
    {
        app_lea_trigger_mmi_handle_action(action, false);
        return;
    }

    if (app_cfg_nv.bud_role == REMOTE_SESSION_ROLE_SECONDARY &&
        action != MMI_AV_PLAY_PAUSE)
    {
        if (mtc_get_btmode() == MULTI_PRO_BT_BIS &&
            (action == MMI_DEV_SPK_VOL_UP  || action == MMI_DEV_SPK_VOL_DOWN))
            ;
        else
        {
            return;
        }
    }

    if (mtc_get_btmode() == MULTI_PRO_BT_CIS)
    {
        if (action <= MMI_HF_TRANSFER_CALL)
        {
#if F_APP_CCP_SUPPORT
            conn_handle = app_ccp_get_active_conn_handle();
#endif
            p_link = app_find_le_link_by_conn_handle(conn_handle);

            if (p_link == NULL)
            {
                app_le_audio_device_sm(LE_AUDIO_MMI, NULL);
                return;
            }
        }
        else if ((action >= MMI_AV_PLAY_PAUSE && action <= MMI_AV_REWIND_STOP) ||
                 action == LEA_MMI_AV_PAUSE)
        {
#if F_APP_MCP_SUPPORT
            conn_handle = app_mcp_get_active_conn_handle();
#endif
            p_link = app_find_le_link_by_conn_handle(conn_handle);

            if (p_link == NULL)
            {
                app_le_audio_device_sm(LE_AUDIO_MMI, NULL);
                return;
            }
        }
        else if (action == MMI_DEV_SPK_VOL_UP || action == MMI_DEV_SPK_VOL_DOWN)
        {

            if (app_le_audio_get_call_status() != BT_CCP_CALL_IDLE)
            {
#if F_APP_CCP_SUPPORT
                conn_handle = app_ccp_get_active_conn_handle();
#endif
                p_link = app_find_le_link_by_conn_handle(conn_handle);

                if (p_link == NULL)
                {
                    return;
                }
            }
            else
            {
#if F_APP_MCP_SUPPORT
                conn_handle = app_mcp_get_active_conn_handle();
#endif
                p_link = app_find_le_link_by_conn_handle(conn_handle);

                if (p_link == NULL)
                {
                    return;
                }
            }
        }
        else
        {
            APP_PRINT_INFO1("app_le_audio_mmi_handle_action: not support action %x", action);
            return;
        }
    }
    switch (action)
    {
#if F_APP_MCP_SUPPORT
    case MMI_AV_PLAY_PAUSE:
        {
            if (p_link != NULL && mtc_get_btmode() == MULTI_PRO_BT_CIS)
            {
                APP_PRINT_INFO1("app_audio_data_mmi_handle_action: MMI_AV_PLAY_PAUSE %x", p_link->media_state);
                if (p_link->media_state == MCS_MEDIA_PLAYING_STATE)
                {
                    // CIS will effect A2DP stutter when switch to A2DP
                    param.opcode = PAUSE_CONTROL_OPCODE;
                }
                else
                {
                    param.opcode = PLAY_CONTROL_OPCODE;
                }

                if (mcs_send_mcp_op(conn_handle, 0, p_link->general_mcs, &param) == false)
                {
                    APP_PRINT_INFO0("app_audio_data_mmi_handle_action: send cmd fail %d");
                }
                else
                {
                    if (param.opcode == PAUSE_CONTROL_OPCODE)
                    {
                        p_link->media_state = MCS_MEDIA_PAUSED_STATE;
                    }
                    else
                    {
                        p_link->media_state = MCS_MEDIA_PLAYING_STATE;
                    }
                }
            }
            else
            {
                app_disallow_legacy_stream(false);
                app_le_audio_device_sm(LE_AUDIO_ADV_STOP, NULL);
            }
        }
        break;

    case LEA_MMI_AV_PAUSE:
        {
            if (p_link->media_state == MCS_MEDIA_PLAYING_STATE)
            {
                param.opcode = PAUSE_CONTROL_OPCODE;
            }

            if (mcs_send_mcp_op(conn_handle, 0, p_link->general_mcs, &param) == false)
            {
                APP_PRINT_INFO0("app_audio_data_mmi_handle_action: send cmd fail %d");
            }
        }
        break;

    case MMI_AV_STOP:
        {
            if (p_link->media_state == MCS_MEDIA_PLAYING_STATE)
            {
                param.opcode = STOP_CONTROL_OPCODE;
            }

            if (mcs_send_mcp_op(conn_handle, 0, p_link->general_mcs, &param) == false)
            {
                APP_PRINT_INFO0("app_audio_data_mmi_handle_action: send cmd fail %d");
            }
        }
        break;

    case MMI_AV_FWD:
        {
            if (p_link != NULL && mtc_get_btmode() == MULTI_PRO_BT_CIS)
            {
                param.opcode = NEXT_TRACK_CONTROL_OPCODE;
                if (mcs_send_mcp_op(conn_handle, 0, p_link->general_mcs, &param) == false)
                {
                    APP_PRINT_INFO0("app_audio_data_mmi_handle_action: send cmd fail %d");
                }
            }
        }
        break;

    case MMI_AV_BWD:
        {
            param.opcode = PREVOUS_TRACK_CONTROL_OPCODE;
            if (mcs_send_mcp_op(conn_handle, 0, p_link->general_mcs, &param) == false)
            {
                APP_PRINT_INFO0("app_audio_data_mmi_handle_action: send cmd fail %d");
            }
        }
        break;

    case MMI_AV_REWIND:
        {
            param.opcode = FAST_REWIND_CONTROL_OPCODE;
            if (mcs_send_mcp_op(conn_handle, 0, p_link->general_mcs, &param) == false)
            {
                APP_PRINT_INFO0("app_audio_data_mmi_handle_action: send cmd fail %d");
            }
        }
        break;

    case MMI_AV_FASTFORWARD:
        {
            param.opcode = FAST_FORWARD_CONTROL_OPCODE;
            if (mcs_send_mcp_op(conn_handle, 0, p_link->general_mcs, &param) == false)
            {
                APP_PRINT_INFO0("app_audio_data_mmi_handle_action: send cmd fail %d");
            }
        }
        break;

    case MMI_AV_REWIND_STOP:
        {
            param.opcode = PLAY_CONTROL_OPCODE;
            if (mcs_send_mcp_op(conn_handle, 0, p_link->general_mcs, &param) == false)
            {
                APP_PRINT_INFO0("app_audio_data_mmi_handle_action: send cmd fail %d");
            }
        }
        break;

    case MMI_AV_FASTFORWARD_STOP:
        {
            param.opcode = PLAY_CONTROL_OPCODE;
            if (mcs_send_mcp_op(conn_handle, 0, p_link->general_mcs, &param) == false)
            {
                APP_PRINT_INFO0("app_audio_data_mmi_handle_action: send cmd fail %d");
            }
        }
        break;
#endif

#if F_APP_CCP_SUPPORT
    case MMI_HF_ANSWER_CALL:
        {
            T_BLE_CALL_LINK *p_call_link;

            p_call_link = app_le_audio_find_call_link_by_idx(p_link->conn_id, p_link->active_call_index);
            if (p_call_link != NULL)
            {
                if (p_call_link->call_state == TBS_CALL_STATE_INCOMING)
                {
                    ccp_send_accept_operation(p_link->conn_handle, 0, p_call_link->call_index, true,
                                              p_link->general_tbs);
                }
            }
        }
        break;

    case MMI_HF_REJECT_CALL:
        {
            T_BLE_CALL_LINK *p_call_link;

            p_call_link = app_le_audio_find_call_link_by_idx(p_link->conn_id, p_link->active_call_index);
            if (p_call_link != NULL)
            {
                if (p_call_link->call_state == TBS_CALL_STATE_INCOMING)
                {
                    ccp_send_terminate_operation(p_link->conn_handle, 0, p_call_link->call_index, true,
                                                 p_link->general_tbs);
                }
            }
        }
        break;

    case MMI_HF_END_ACTIVE_CALL:
        {
            T_BLE_CALL_LINK *p_call_link;

            p_call_link = app_le_audio_find_call_link_by_idx(p_link->conn_id, p_link->active_call_index);
            if (p_call_link != NULL)
            {
                ccp_send_terminate_operation(p_link->conn_handle, 0, p_call_link->call_index, true,
                                             p_link->general_tbs);
            }
        }
        break;

    case MMI_HF_SWITCH_TO_SECOND_CALL:
        {
            uint8_t i;
            T_APP_LE_LINK *p_active_link = NULL;
            T_APP_LE_LINK *p_inactive_link = NULL;
            T_BLE_CALL_LINK *p_active_call_link = NULL;
            T_BLE_CALL_LINK *p_inactive_call_link = NULL;
            T_BT_CCP_CALL_STATUS call_status = app_le_audio_get_call_status();

            for (i = 0; i < MAX_BLE_LINK_NUM; i++)
            {
                if (app_db.le_link[i].used == true)
                {
                    if (app_db.le_link[i].conn_handle == app_ccp_get_active_conn_handle())
                    {
                        p_active_link = &app_db.le_link[i];
                    }
                    else
                    {
                        p_inactive_link = &app_db.le_link[i];
                    }
                }
            }

            if ((call_status == BT_CCP_MULTILINK_CALL_ACTIVE_WITH_CALL_WAIT) ||
                (call_status == BT_CCP_MULTILINK_CALL_ACTIVE_WITH_CALL_HOLD))
            {
                if (p_inactive_link == NULL)
                {
                    APP_PRINT_WARN0("MMI_HF_SWITCH_TO_SECOND_CALL: no multilink call");
                    return;
                }

                for (i = 0; i < CCP_CALL_LIST_NUM; i++)
                {
                    if (p_inactive_link->ble_call_link[i].used == true &&
                        p_inactive_link->ble_call_link[i].call_index == p_inactive_link->active_call_index)
                    {
                        p_active_call_link = &p_inactive_link->ble_call_link[i];
                        break;
                    }
                }

                if (p_active_call_link == NULL)
                {
                    APP_PRINT_WARN0("MMI_HF_SWITCH_TO_SECOND_CALL: no active call link");
                    return;
                }

                if ((p_active_call_link->call_state == TBS_CALL_STATE_LOC_HELD) ||
                    (p_active_call_link->call_state == TBS_CALL_STATE_LOC_REM_HELD))
                {
                    ccp_send_loc_retrieve_operation(p_inactive_link->conn_handle, 0, p_active_call_link->call_index,
                                                    true,
                                                    p_inactive_link->general_tbs);
                }
            }
            else
            {
                if (p_active_link == NULL)
                {
                    APP_PRINT_WARN0("MMI_HF_SWITCH_TO_SECOND_CALL: no multilink call");
                    return;
                }

                for (i = 0; i < CCP_CALL_LIST_NUM; i++)
                {
                    if (p_active_link->ble_call_link[i].used == true &&
                        p_active_link->ble_call_link[i].call_index == p_active_link->active_call_index)
                    {
                        p_active_call_link = &p_active_link->ble_call_link[i];
                        break;
                    }
                }

                if (p_active_call_link == NULL)
                {
                    APP_PRINT_WARN0("MMI_HF_SWITCH_TO_SECOND_CALL: no active call link");
                    return;
                }

                if (p_active_call_link->call_state == TBS_CALL_STATE_ACTIVE)
                {
                    ccp_send_loc_hold_operation(p_active_link->conn_handle, 0, p_active_call_link->call_index, true,
                                                p_active_link->general_tbs);
                }

                for (i = 0; i < CCP_CALL_LIST_NUM; i++)
                {
                    if (p_active_link->ble_call_link[i].used == true &&
                        p_active_link->ble_call_link[i].call_index != p_active_link->active_call_index)
                    {
                        p_inactive_call_link = &p_active_link->ble_call_link[i];
                        break;
                    }
                }

                if (p_inactive_call_link == NULL)
                {
                    APP_PRINT_WARN0("MMI_HF_SWITCH_TO_SECOND_CALL: no inactive call");
                    return;
                }

                if ((p_inactive_call_link->call_state == TBS_CALL_STATE_LOC_HELD) ||
                    (p_inactive_call_link->call_state == TBS_CALL_STATE_LOC_REM_HELD))
                {
                    ccp_send_loc_retrieve_operation(p_active_link->conn_handle, 0, p_inactive_call_link->call_index,
                                                    true,
                                                    p_active_link->general_tbs);
                }
                else if (p_inactive_call_link->call_state == TBS_CALL_STATE_INCOMING)
                {
                    ccp_send_accept_operation(p_active_link->conn_handle, 0, p_inactive_call_link->call_index, true,
                                              p_active_link->general_tbs);
                }
            }
        }
        break;

    case MMI_HF_TRANSFER_CALL:
        {
            T_BLE_CALL_LINK *p_call_link;

            p_call_link = app_le_audio_find_call_link_by_idx(p_link->conn_id, p_link->active_call_index);
            if (p_call_link != NULL)
            {
                if (p_call_link->call_state == TBS_CALL_STATE_ACTIVE)
                {
                    ccp_send_loc_hold_operation(p_link->conn_handle, 0, p_call_link->call_index, true,
                                                p_link->general_tbs);
                }
                else if ((p_call_link->call_state == TBS_CALL_STATE_LOC_HELD) ||
                         (p_call_link->call_state == TBS_CALL_STATE_LOC_REM_HELD))
                {
                    ccp_send_loc_retrieve_operation(p_link->conn_handle, 0, p_call_link->call_index, true,
                                                    p_link->general_tbs);
                }
            }
        }
        break;

    case MMI_HF_JOIN_TWO_CALLS:
        {
            uint8_t i, j = 0;
            uint8_t call_index[CCP_CALL_LIST_NUM];

            for (i = 0; i < CCP_CALL_LIST_NUM; i++)
            {
                if (p_link->ble_call_link[i].used == true &&
                    p_link->ble_call_link[i].call_index != 0)
                {
                    call_index[j++] = p_link->ble_call_link[i].call_index;
                }
            }

            if (j == CCP_CALL_LIST_NUM)
            {
                ccp_send_join_operation(p_link->conn_handle, 0, call_index, CCP_CALL_LIST_NUM, true,
                                        p_link->general_tbs);
            }
        }
        break;
#endif

#if F_APP_VCS_SUPPORT
    case MMI_DEV_SPK_VOL_UP:
        {
            bool max_tone = false, releay = false;
            if (mtc_get_btmode() == MULTI_PRO_BT_CIS)
            {
                uint8_t volume_step;
                T_VCS_PARAM vcs_param;
                T_BLE_ASE_LINK *p_ase_link;

                vcs_get_param(&vcs_param);
                volume_step = MAX_VCS_VOLUME_SETTING / MAX_VCS_OUTPUT_LEVEL;
                vcs_param.volume_flags = VCS_USER_SET_VOLUME_SETTING;
                vcs_param.mute = VCS_NOT_MUTED;
                p_link->mute = VCS_NOT_MUTED;
                vcs_param.change_counter++;
                if ((int16_t)(vcs_param.volume_setting + volume_step) < MAX_VCS_VOLUME_SETTING)
                {
                    vcs_param.volume_setting += volume_step;
                }
                else
                {
                    vcs_param.volume_setting = MAX_VCS_VOLUME_SETTING;
                    max_tone = true;
                    releay = true;
                }
                vcs_set_param(&vcs_param);

                if (app_le_audio_get_call_status() != BT_CCP_CALL_IDLE)
                {
                    p_ase_link = app_le_audio_find_ase_by_context(p_link->conn_handle, AUDIO_CONTEXT_CONVERSATIONAL);
                }
                else
                {
                    p_ase_link = app_le_audio_find_ase_by_context(p_link->conn_handle, AUDIO_CONTEXT_MEDIA);
                }

                if (p_ase_link != NULL)
                {
                    uint8_t level;

                    level = (vcs_param.volume_setting * MAX_VCS_OUTPUT_LEVEL) / MAX_VCS_VOLUME_SETTING;
#if HARMAN_OPEN_LR_FEATURE
                    app_harman_lr_balance_set(AUDIO_STREAM_TYPE_PLAYBACK, level, __func__, __LINE__);
#endif
                    audio_track_volume_out_set(p_ase_link->handle, level);
                }
            }
            else  if (mtc_get_btmode() == MULTI_PRO_BT_BIS)
            {
                T_BIS_CB *p_bis_cb = NULL;
                uint8_t volume_step, cur_vol;
                cur_vol =   app_cfg_nv.bis_audio_gain_level * MAX_BIS_AUDIO_VOLUME_SETTING /
                            MAX_BIS_AUDIO_OUTPUT_LEVEL;
                volume_step =  MAX_BIS_AUDIO_VOLUME_SETTING / MAX_BIS_AUDIO_OUTPUT_LEVEL  ;

                if (app_cfg_const.bis_mode == T_LEA_BROADCAST_SINK)
                {
                    p_bis_cb = app_le_audio_find_bis_by_conn_handle(app_bc_get_bis_handle());
                }
                else  if (app_cfg_const.bis_mode == T_LEA_BROADCAST_DELEGATOR)
                {
                    p_bis_cb = app_le_audio_find_bis_by_source_id(app_bass_get_source());
                }

                if ((int16_t)(cur_vol + volume_step) < MAX_BIS_AUDIO_VOLUME_SETTING)
                {
                    cur_vol += volume_step;
                }
                else
                {
                    cur_vol  = MAX_BIS_AUDIO_VOLUME_SETTING;
                    max_tone = true;
                }

                app_cfg_nv.bis_audio_gain_level = (cur_vol * MAX_BIS_AUDIO_OUTPUT_LEVEL) /
                                                  MAX_BIS_AUDIO_VOLUME_SETTING;

                if (p_bis_cb != NULL && p_bis_cb->audio_track_handle != NULL)
                {
#if HARMAN_OPEN_LR_FEATURE
                    app_harman_lr_balance_set(AUDIO_STREAM_TYPE_PLAYBACK, app_cfg_nv.bis_audio_gain_level,
                                              __func__, __LINE__);
#endif
                    audio_track_volume_out_set(p_bis_cb->audio_track_handle, app_cfg_nv.bis_audio_gain_level);
                }
                else
                {
                    APP_PRINT_INFO0("app_le_audio_mmi_handle_action: no bis cb!!!");
                }
            }
            if (max_tone)
            {
#if F_APP_HARMAN_FEATURE_SUPPORT
                if (app_cfg_nv.language_status == 0)
                {
                    app_audio_tone_type_play(TONE_BATTERY_PERCENTAGE_90, false, releay);
                }
                else
#endif
                {
                    app_audio_tone_type_play(TONE_VOL_MAX, false, releay);
                }
            }
        }
        break;

    case MMI_DEV_SPK_VOL_DOWN:
        {
            bool min_tone = false, releay = false;
            if (mtc_get_btmode() == MULTI_PRO_BT_CIS)
            {
                uint8_t volume_step;
                T_VCS_PARAM vcs_param;
                T_BLE_ASE_LINK *p_ase_link;

                vcs_get_param(&vcs_param);
                volume_step = MAX_VCS_VOLUME_SETTING / MAX_VCS_OUTPUT_LEVEL;
                vcs_param.volume_flags = VCS_USER_SET_VOLUME_SETTING;
                vcs_param.mute = VCS_NOT_MUTED;
                p_link->mute = VCS_NOT_MUTED;
                vcs_param.change_counter++;
                if ((int16_t)(vcs_param.volume_setting - volume_step) > 0)
                {
                    vcs_param.volume_setting -= volume_step;
                }
                else
                {
                    vcs_param.volume_setting = 0;
                    min_tone = true;
                    releay =  true;
                }
                vcs_set_param(&vcs_param);

                if (app_le_audio_get_call_status() != BT_CCP_CALL_IDLE)
                {
                    p_ase_link = app_le_audio_find_ase_by_context(p_link->conn_handle, AUDIO_CONTEXT_CONVERSATIONAL);
                }
                else
                {
                    p_ase_link = app_le_audio_find_ase_by_context(p_link->conn_handle, AUDIO_CONTEXT_MEDIA);
                }

                if (p_ase_link != NULL)
                {
                    uint8_t level;

                    level = (vcs_param.volume_setting * MAX_VCS_OUTPUT_LEVEL) / MAX_VCS_VOLUME_SETTING;
#if HARMAN_OPEN_LR_FEATURE
                    app_harman_lr_balance_set(AUDIO_STREAM_TYPE_PLAYBACK, level, __func__, __LINE__);
#endif
                    audio_track_volume_out_set(p_ase_link->handle, level);
                }
            }
            else  if (mtc_get_btmode() == MULTI_PRO_BT_BIS)
            {
                T_BIS_CB *p_bis_cb = NULL;
                uint8_t volume_step, cur_vol;
                volume_step =  MAX_BIS_AUDIO_VOLUME_SETTING / MAX_BIS_AUDIO_OUTPUT_LEVEL  ;
                cur_vol =   app_cfg_nv.bis_audio_gain_level * MAX_BIS_AUDIO_VOLUME_SETTING /
                            MAX_BIS_AUDIO_OUTPUT_LEVEL;

                if (app_cfg_const.bis_mode == T_LEA_BROADCAST_SINK)
                {
                    p_bis_cb = app_le_audio_find_bis_by_conn_handle(app_bc_get_bis_handle());
                }
                else  if (app_cfg_const.bis_mode == T_LEA_BROADCAST_DELEGATOR)
                {
                    p_bis_cb = app_le_audio_find_bis_by_source_id(app_bass_get_source());
                }

                if ((int16_t)(cur_vol - volume_step) > 0)
                {
                    cur_vol -= volume_step;
                }
                else
                {
                    cur_vol = 0;
                    min_tone = true;
                }


                app_cfg_nv.bis_audio_gain_level = (cur_vol * MAX_BIS_AUDIO_OUTPUT_LEVEL) /
                                                  MAX_BIS_AUDIO_VOLUME_SETTING;
                if (p_bis_cb != NULL && p_bis_cb->audio_track_handle != NULL)
                {
#if HARMAN_OPEN_LR_FEATURE
                    app_harman_lr_balance_set(AUDIO_STREAM_TYPE_PLAYBACK, app_cfg_nv.bis_audio_gain_level,
                                              __func__, __LINE__);
#endif
                    audio_track_volume_out_set(p_bis_cb->audio_track_handle, app_cfg_nv.bis_audio_gain_level);
                }

            }
            if (min_tone)
            {
                app_audio_tone_type_play(TONE_VOL_MIN, false, releay);
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
        APP_PRINT_INFO2("app_audio_data_mmi_handle_action: action 0x%x, conn_handle 0x%x", action,
                        conn_handle);
    }
}

void app_le_audio_dump_ase_info(T_APP_LE_LINK *p_link)
{
    for (uint8_t i = 0; i < ASCS_ASE_NUM; i++)
    {
        T_BLE_ASE_LINK *p_ase_link = &p_link->ble_ase_link[i];
        APP_PRINT_INFO8("dump ASE info: used %d,conn_handle 0x%x,ase_id %d,context %x,cis %x,state%d,handle %x,frame_num %d",
                        p_ase_link->used, p_ase_link->conn_handle, p_ase_link->ase_id, p_ase_link->audio_context,
                        p_ase_link->cis_conn_handle, p_ase_link->state, p_ase_link->handle, p_ase_link->frame_num);
        APP_PRINT_INFO1("dump ASE info: presentation_delay %x", p_ase_link->presentation_delay);

    }
}

bool app_le_audio_ase_allocate(uint16_t conn_handle, uint8_t ase_id, T_CODEC_CFG codec_cfg)
{
    bool ret = false;
    T_APP_LE_LINK *p_link;

    p_link = app_find_le_link_by_conn_handle(conn_handle);
    if (p_link != NULL)
    {
        T_BLE_ASE_LINK *p_ase_link = NULL;
        uint8_t i;

        for (i = 0; i < ASCS_ASE_NUM; i++)
        {
            if (p_link->ble_ase_link[i].used == false)
            {
                p_ase_link = &p_link->ble_ase_link[i];

                p_ase_link->used = true;
                p_ase_link->ase_id = ase_id;
                p_ase_link->conn_handle = conn_handle;
                memcpy(&p_ase_link->codec_cfg, &codec_cfg, sizeof(T_CODEC_CFG));
                ret = true;
                break;
            }
        }
    }

    return ret;
}

bool app_le_audio_free_ase(T_BLE_ASE_LINK *p_ase_link)
{
    if (p_ase_link != NULL)
    {
        if (p_ase_link->used == true)
        {
            if (p_ase_link->handle != NULL)
            {
                audio_track_release(p_ase_link->handle);
            }
            if (p_ase_link->handshake_fg)
            {
                ascs_app_ctl_handshake(p_ase_link->conn_handle, p_ase_link->ase_id, false);
            }
            memset(p_ase_link, 0, sizeof(T_BLE_ASE_LINK));
            return true;
        }
    }

    return false;
}

bool app_le_audio_free_link(T_APP_LE_LINK *p_link)
{
    uint8_t i;

    for (i = 0; i < ASCS_ASE_NUM; i++)
    {
        T_BLE_ASE_LINK *p_ase_link = &p_link->ble_ase_link[i];
        if (p_ase_link->used == true)
        {
            if (p_ase_link->handle != NULL)
            {
                ascs_action_release(p_ase_link->conn_handle, p_ase_link->ase_id);
                audio_track_release(p_ase_link->handle);
            }
            memset(p_ase_link, 0, sizeof(T_BLE_ASE_LINK));
        }
    }

    for (i = 0; i < CCP_CALL_LIST_NUM; i++)
    {
        T_BLE_CALL_LINK *p_call_link = &p_link->ble_call_link[i];
        if (p_call_link->used == true)
        {
            memset(p_call_link, 0, sizeof(T_BLE_CALL_LINK));
        }
    }

    if (p_link->call_uri != NULL)
    {
        free(p_link->call_uri);
        p_link->call_uri = NULL;
    }
    return true;
}

T_BLE_ASE_LINK *app_le_audio_find_ase_by_id(uint16_t conn_handle, uint8_t ase_id)
{
    T_APP_LE_LINK *p_link;
    T_BLE_ASE_LINK *p_ase_link = NULL;

    p_link = app_find_le_link_by_conn_handle(conn_handle);
    if (p_link != NULL)
    {
        uint8_t i;

        for (i = 0; i < ASCS_ASE_NUM; i++)
        {
            if (p_link->ble_ase_link[i].used == true &&
                p_link->ble_ase_link[i].ase_id == ase_id)
            {
                p_ase_link = &p_link->ble_ase_link[i];
                break;
            }
        }
    }

    return p_ase_link;
}

T_BLE_ASE_LINK *app_le_audio_find_ase_by_conn_handle(uint16_t conn_handle)
{
    T_APP_LE_LINK *p_link;
    T_BLE_ASE_LINK *p_ase_link = NULL;

    p_link = app_find_le_link_by_conn_handle(conn_handle);
    if (p_link != NULL)
    {
        uint8_t i;

        for (i = 0; i < ASCS_ASE_NUM; i++)
        {
            if (p_link->ble_ase_link[i].used == true &&
                p_link->ble_ase_link[i].conn_handle == conn_handle)
            {
                p_ase_link = &p_link->ble_ase_link[i];
                break;
            }
        }
    }

    return p_ase_link;
}

T_BLE_ASE_LINK *app_le_audio_find_ase_by_conn(uint16_t conn_handle, uint16_t cis_conn_handle)
{
    T_APP_LE_LINK *p_link;
    T_BLE_ASE_LINK *p_ase_link = NULL;

    p_link = app_find_le_link_by_conn_handle(conn_handle);
    if (p_link != NULL)
    {
        uint8_t i;

        for (i = 0; i < ASCS_ASE_NUM; i++)
        {
            if (p_link->ble_ase_link[i].used == true &&
                p_link->ble_ase_link[i].cis_conn_handle == cis_conn_handle)
            {
                p_ase_link = &p_link->ble_ase_link[i];
                break;
            }
        }
    }

    return p_ase_link;
}

T_BLE_ASE_LINK *app_le_audio_find_ase_by_handle(uint16_t conn_handle, T_AUDIO_TRACK_HANDLE handle)
{
    T_APP_LE_LINK *p_link;
    T_BLE_ASE_LINK *p_ase_link = NULL;

    p_link = app_find_le_link_by_conn_handle(conn_handle);
    if (p_link != NULL)
    {
        uint8_t i;

        for (i = 0; i < ASCS_ASE_NUM; i++)
        {
            if (p_link->ble_ase_link[i].used == true &&
                p_link->ble_ase_link[i].handle == handle)
            {
                p_ase_link = &p_link->ble_ase_link[i];
                break;
            }
        }
    }

    return p_ase_link;
}

T_BLE_ASE_LINK *app_le_audio_find_ase_by_loop_conn(uint16_t cis_conn_handle)
{
    uint8_t i, j;
    T_APP_LE_LINK *p_link;
    T_BLE_ASE_LINK *p_ase_link = NULL;

    for (i = 0; i < MAX_BLE_LINK_NUM; i++)
    {
        if (app_db.le_link[i].used == true)
        {
            p_link = &app_db.le_link[i];
            for (j = 0; j < ASCS_ASE_NUM; j++)
            {
                if (p_link->ble_ase_link[j].used == true &&
                    p_link->ble_ase_link[j].cis_conn_handle == cis_conn_handle)
                {
                    p_ase_link = &p_link->ble_ase_link[j];
                    return p_ase_link;
                }
            }
        }
    }

    return p_ase_link;
}

#if F_APP_CCP_SUPPORT
void app_le_audio_update_link_call_status(T_APP_LE_LINK *p_link)
{
    uint8_t i;
    uint8_t active_call_link_exist = true;
    T_BLE_CALL_LINK *p_active_call_link = NULL;
    T_BLE_CALL_LINK *p_inactive_call_link = NULL;
    uint8_t active_call_state = TBS_CALL_STATE_RFU;
    uint8_t inactive_call_state = TBS_CALL_STATE_RFU;

    // find active call link
    for (i = 0; i < CCP_CALL_LIST_NUM; i++)
    {
        if (p_link->ble_call_link[i].used == true &&
            p_link->ble_call_link[i].call_index == p_link->active_call_index)
        {
            p_active_call_link = &p_link->ble_call_link[i];
            active_call_state = p_active_call_link->call_state;
            break;
        }
    }

    if (p_active_call_link == NULL)
    {
        active_call_link_exist = false;
    }

    // find inactive call link
    for (i = 0; i < CCP_CALL_LIST_NUM; i++)
    {
        if (p_link->ble_call_link[i].used == true &&
            p_link->ble_call_link[i].call_index != p_link->active_call_index)
        {
            p_inactive_call_link = &p_link->ble_call_link[i];
            inactive_call_state = p_inactive_call_link->call_state;
            break;
        }
    }

    // check active call link need to exchange or not
    if (active_call_link_exist == false)
    {
        if (inactive_call_state != TBS_CALL_STATE_RFU)
        {
            active_call_link_exist = true;
            p_active_call_link = p_inactive_call_link;

            active_call_state = p_active_call_link->call_state;
            inactive_call_state = TBS_CALL_STATE_RFU;
            p_link->active_call_index = p_inactive_call_link->call_index;
        }
    }
    else
    {
        bool exchange_call_link = false;

        if (active_call_state < TBS_CALL_STATE_ACTIVE)
        {
            if ((inactive_call_state >= TBS_CALL_STATE_ACTIVE) &&
                (inactive_call_state <= TBS_CALL_STATE_LOC_REM_HELD))
            {
                exchange_call_link = true;
            }
        }
        else if ((active_call_state >= TBS_CALL_STATE_LOC_HELD) &&
                 (active_call_state <= TBS_CALL_STATE_LOC_REM_HELD))
        {
            if (inactive_call_state == TBS_CALL_STATE_ACTIVE)
            {
                exchange_call_link = true;
            }
        }

        if (exchange_call_link == true)
        {
            T_BLE_CALL_LINK *p_temp_call_link = NULL;

            p_temp_call_link = p_inactive_call_link;
            p_inactive_call_link = p_active_call_link;
            p_active_call_link = p_temp_call_link;

            active_call_state = p_active_call_link->call_state;
            inactive_call_state = p_inactive_call_link->call_state;
            p_link->active_call_index = p_active_call_link->call_index;
        }
    }

    if (active_call_link_exist == true)
    {
        switch (active_call_state)
        {
        case TBS_CALL_STATE_INCOMING:
            if ((inactive_call_state < TBS_CALL_STATE_ACTIVE) ||
                (inactive_call_state == TBS_CALL_STATE_RFU))
            {
                p_link->call_status = BT_CCP_INCOMING_CALL_ONGOING;
            }
            break;

        case TBS_CALL_STATE_DIALING:
        case TBS_CALL_STATE_ALERTING:
            {
                if ((inactive_call_state < TBS_CALL_STATE_ACTIVE) ||
                    (inactive_call_state == TBS_CALL_STATE_RFU))
                {
                    p_link->call_status = BT_CCP_OUTGOING_CALL_ONGOING;
                }
            }
            break;

        case TBS_CALL_STATE_ACTIVE:
            {
                if (inactive_call_state == TBS_CALL_STATE_INCOMING)
                {
                    p_link->call_status = BT_CCP_CALL_ACTIVE_WITH_CALL_WAITING;
                }
                else if (inactive_call_state == TBS_CALL_STATE_ACTIVE)
                {
                    p_link->call_status = BT_CCP_CALL_ACTIVE;
                }
                else if ((inactive_call_state == TBS_CALL_STATE_LOC_HELD) ||
                         (inactive_call_state == TBS_CALL_STATE_REM_HELD))
                {
                    p_link->call_status = BT_CCP_CALL_ACTIVE_WITH_CALL_HOLD;
                }
                else if (inactive_call_state == TBS_CALL_STATE_RFU)
                {
                    p_link->call_status = BT_CCP_CALL_ACTIVE;
                }
            }
            break;

        case TBS_CALL_STATE_LOC_HELD:
        case TBS_CALL_STATE_REM_HELD:
            {
                if ((inactive_call_state < TBS_CALL_STATE_ACTIVE) ||
                    (inactive_call_state == TBS_CALL_STATE_RFU))
                {
                    p_link->call_status = BT_CCP_CALL_ACTIVE;
                }
            }
            break;

        default:
            break;
        }
        APP_PRINT_INFO2("app_le_audio_update_link_call_status: active call_state %d, inactive call_state %d",
                        active_call_state, inactive_call_state);
    }
    else
    {
        p_link->call_status = BT_CCP_CALL_IDLE;
    }
}

void app_le_audio_update_call_status(void)
{
    uint8_t i;
    uint16_t conn_handle;
    T_APP_LE_LINK *p_active_link;
    T_APP_LE_LINK *p_inactive_link;
    uint8_t active_link_exist = true;
    T_BT_CCP_CALL_STATUS active_call_status = BT_CCP_CALL_IDLE;
    T_BT_CCP_CALL_STATUS inactive_call_status = BT_CCP_CALL_IDLE;

    conn_handle = app_ccp_get_active_conn_handle();
    // find active call link
    p_active_link = app_find_le_link_by_conn_handle(conn_handle);
    if (p_active_link == NULL)
    {
        active_link_exist = false;
    }
    else
    {
        active_call_status = p_active_link->call_status;
    }

    // find inactive call link
    for (i = 0; i < MAX_BLE_LINK_NUM; i++)
    {
        if ((app_db.le_link[i].used == true) && ((p_active_link != NULL) &&
                                                 (app_db.le_link[i].conn_handle != p_active_link->conn_handle)))
        {
            p_inactive_link = &app_db.le_link[i];
            inactive_call_status = p_inactive_link->call_status;
            break;
        }
    }

    // check active call link need to exchange or not
    if (active_link_exist == false)
    {
        if (inactive_call_status != BT_CCP_CALL_IDLE)
        {
            active_link_exist = true;
            p_active_link = p_inactive_link;
            inactive_call_status = BT_CCP_CALL_IDLE;
            app_ccp_set_active_conn_handle(p_active_link->conn_handle);
        }
    }
    else
    {
        if (active_call_status < BT_CCP_CALL_ACTIVE)
        {
            if ((inactive_call_status >= BT_CCP_CALL_ACTIVE) &&
                (inactive_call_status <= BT_CCP_CALL_ACTIVE_WITH_CALL_HOLD))
            {
                T_APP_LE_LINK *p_temp_link = NULL;

                p_temp_link = p_inactive_link;
                p_inactive_link = p_active_link;
                p_active_link = p_temp_link;

                active_call_status = p_active_link->call_status;
                inactive_call_status = p_inactive_link->call_status;
                app_ccp_set_active_conn_handle(p_active_link->conn_handle);
            }
        }
    }

    if (active_link_exist == true)
    {
        switch (active_call_status)
        {
        case BT_CCP_INCOMING_CALL_ONGOING:
            if (inactive_call_status < BT_CCP_CALL_ACTIVE)
            {
                ccp_call_status = BT_CCP_INCOMING_CALL_ONGOING;
            }
            break;

        case BT_CCP_CALL_ACTIVE:
            if (inactive_call_status == BT_CCP_INCOMING_CALL_ONGOING)
            {
                ccp_call_status = BT_CCP_MULTILINK_CALL_ACTIVE_WITH_CALL_WAIT;
            }
            else if (inactive_call_status > BT_CCP_CALL_ACTIVE)
            {
                ccp_call_status = BT_CCP_MULTILINK_CALL_ACTIVE_WITH_CALL_HOLD;
            }
            else
            {
                ccp_call_status = BT_CCP_CALL_ACTIVE;
            }
            break;

        default:
            ccp_call_status = active_call_status;
            break;
        }
        APP_PRINT_INFO2("app_le_audio_update_call_status: active call_status %d, inactive call_status %d",
                        active_call_status, inactive_call_status);
    }
    else
    {
        ccp_call_status = BT_CCP_CALL_IDLE;
    }
}
#endif

bool app_le_audio_free_call_link(T_BLE_CALL_LINK *p_call_link)
{
    if (p_call_link != NULL)
    {
        if (p_call_link->used == true)
        {
            memset(p_call_link, 0, sizeof(T_BLE_CALL_LINK));
            return true;
        }
    }

    return false;
}

void app_le_audio_dump_call_info(T_APP_LE_LINK *p_link)
{
    for (uint8_t i = 0; i < CCP_CALL_LIST_NUM; i++)
    {
        T_BLE_CALL_LINK *p_call_link = &p_link->ble_call_link[i];
        APP_PRINT_INFO3("dump call info: used %d, call_index %d, call_state %d",
                        p_call_link->used, p_call_link->call_index, p_call_link->call_state);
    }
    APP_PRINT_INFO3("dump call info: active_call_index %d, call_status %d, ccp_call_status %d",
                    p_link->active_call_index, p_link->call_status, ccp_call_status);
}

void app_stop_inactive_ble_link_stream(T_APP_LE_LINK *p_link)
{
    uint8_t i;

    for (i = 0; i < MAX_BLE_LINK_NUM; i++)
    {
        T_APP_LE_LINK *p_temp_link = &app_db.le_link[i];
        if (p_temp_link != p_link)
        {
            if (p_temp_link->used == true)
            {
                if (p_temp_link->link_state == LE_AUDIO_LINK_STREAMING)
                {
                    T_MCS_CLIENT_MCP_PARAM param;
                    param.p_mcp_cb = NULL;
                    param.opcode = STOP_CONTROL_OPCODE;
                    mcs_send_mcp_op(p_temp_link->conn_handle, 0, p_temp_link->general_mcs, &param);
                }
            }
        }
    }

    APP_PRINT_TRACE1("app_stop_inactive_ble_link_stream: active conn_handle 0x%x", p_link->conn_handle);
}

uint16_t app_le_audio_get_contexts(uint8_t *p_data, uint16_t len)
{
    uint16_t audio_context = 0;
    uint8_t length = 0;
    uint8_t type = 0;
    uint8_t *p = p_data;

    if (p_data == NULL || len == 0)
    {
        return false;
    }

    while (len > 0)
    {
        LE_STREAM_TO_UINT8(length, p);
        LE_STREAM_TO_UINT8(type, p);

        if (type == METADATA_TYPE_STREAMING_AUDIO_CONTEXTS)
        {
            if (length == 3)
            {
                LE_STREAM_TO_UINT16(audio_context, p);
                break;
            }
        }

        len -= (length + 1);
    }

    return audio_context;
}

void app_le_audio_track_create_for_cis(T_APP_LE_LINK *p_link, uint8_t ase_id)
{
    T_AUDIO_STREAM_TYPE type;
    uint8_t volume_out;
    uint8_t volume_in;
    uint32_t device;
    T_BLE_ASE_LINK *p_ase_link;
    T_CODEC_CFG *p_codec;

    media_state = ISOCH_DATA_PKT_STATUS_LOST_DATA;
    p_ase_link = app_le_audio_find_ase_by_id(p_link->conn_handle, ase_id);

    if (p_ase_link == NULL)
    {
        APP_PRINT_ERROR0("app_le_audio_track_create_for_cis: not find ase datebase");
        return;
    }

    p_codec = &p_ase_link->codec_cfg;
    p_ase_link->frame_num = p_codec->codec_frame_blocks_per_sdu * count_bits_1(
                                p_codec->audio_channel_allocation);
    if (p_ase_link->audio_context == AUDIO_CONTEXT_VOICE_ASSISTANTS)
    {
        type = AUDIO_STREAM_TYPE_RECORD;
        volume_out = 0;
        volume_in = app_cfg_const.record_volume_default;
        device = AUDIO_DEVICE_IN_DEFAULT;
    }
    else if (p_ase_link->audio_context == AUDIO_CONTEXT_MEDIA)
    {
#if F_APP_MCP_SUPPORT_WORKAROUND
        p_link->media_state = MCS_MEDIA_PLAYING_STATE;
        //app_le_audio_link_sm(p_link->conn_handle, LE_AUDIO_MCP_STATE, NULL);
#endif
        T_VCS_PARAM vcs_param;
        type = AUDIO_STREAM_TYPE_PLAYBACK;

        vcs_get_param(&vcs_param);
        vcs_param.mute = VCS_NOT_MUTED;
        vcs_param.change_counter++;
        vcs_set_param(&vcs_param);
        volume_out = (vcs_param.volume_setting * MAX_VCS_OUTPUT_LEVEL) / MAX_VCS_VOLUME_SETTING;
        volume_in = 0;
        device = AUDIO_DEVICE_OUT_DEFAULT;
    }
    else if (p_ase_link->audio_context == AUDIO_CONTEXT_CONVERSATIONAL)
    {
        T_VCS_PARAM vcs_param;
        type = AUDIO_STREAM_TYPE_VOICE;

        vcs_get_param(&vcs_param);
        vcs_param.mute = VCS_NOT_MUTED;
        vcs_param.change_counter++;
        vcs_set_param(&vcs_param);
        volume_out = (vcs_param.volume_setting * MAX_VCS_OUTPUT_LEVEL) / MAX_VCS_VOLUME_SETTING;
        volume_in = app_cfg_const.voice_volume_in_default;
        device = AUDIO_DEVICE_OUT_DEFAULT | AUDIO_DEVICE_IN_DEFAULT;
    }

    T_AUDIO_FORMAT_INFO format_info = {};
    format_info.type = AUDIO_FORMAT_TYPE_LC3;
    if (p_codec->sample_frequency == SAMPLING_FREQUENCY_CFG_8K)
    {
        format_info.attr.lc3.sample_rate = 8000;
    }
    else if (p_codec->sample_frequency == SAMPLING_FREQUENCY_CFG_16K)
    {
        format_info.attr.lc3.sample_rate = 16000;
    }
    else if (p_codec->sample_frequency == SAMPLING_FREQUENCY_CFG_24K)
    {
        format_info.attr.lc3.sample_rate = 24000;
    }
    else if (p_codec->sample_frequency == SAMPLING_FREQUENCY_CFG_32K)
    {
        format_info.attr.lc3.sample_rate = 32000;
    }
    else if (p_codec->sample_frequency == SAMPLING_FREQUENCY_CFG_44_1K)
    {
        format_info.attr.lc3.sample_rate = 44100;
    }
    else if (p_codec->sample_frequency == SAMPLING_FREQUENCY_CFG_48K)
    {
        format_info.attr.lc3.sample_rate = 48000;
    }

    format_info.attr.lc3.chann_location = p_codec->audio_channel_allocation;
    format_info.attr.lc3.frame_length = p_codec->octets_per_codec_frame;
    format_info.attr.lc3.frame_duration = (T_AUDIO_LC3_FRAME_DURATION)p_codec->frame_duration;
    format_info.attr.lc3.presentation_delay = p_ase_link->presentation_delay;

    if (p_ase_link->audio_context == AUDIO_CONTEXT_CONVERSATIONAL)
    {
        for (uint8_t i = 0; i < ASCS_ASE_NUM; i++)
        {
            if (p_link->ble_ase_link[i].used == true && p_ase_link != &p_link->ble_ase_link[i])
            {
                if (p_link->ble_ase_link[i].audio_context == AUDIO_CONTEXT_CONVERSATIONAL)
                {
                    if (p_link->ble_ase_link[i].handle)
                    {
                        p_ase_link->handle = p_link->ble_ase_link[i].handle;
                        return;
                    }
                }
            }
        }
    }

    p_ase_link->handle = audio_track_create(type,
                                            AUDIO_STREAM_MODE_DIRECT,
                                            AUDIO_STREAM_USAGE_LOCAL,
                                            format_info,
                                            volume_out,
                                            volume_in,
                                            device,
                                            NULL,
                                            NULL);
    if (p_ase_link->handle != NULL)
    {
        audio_track_start(p_ase_link->handle);
    }

    APP_PRINT_INFO6("app_le_audio_track_create_for_cis: conn_id 0x%x, frame_duration 0x%x, sample_frequency 0x%x,\
                    octets_per_codec_frame %d, codec_frame_blocks_per_sdu %d, audio_channel_allocation %d",
                    p_link->conn_id, p_codec->frame_duration, p_codec->sample_frequency,
                    p_codec->octets_per_codec_frame, p_codec->codec_frame_blocks_per_sdu,
                    p_codec->audio_channel_allocation);
}

void app_le_audio_track_release_for_cis(T_APP_LE_LINK *p_link, uint8_t ase_id)
{
    APP_PRINT_INFO1("app_le_audio_track_release_for_cis: conn_handle 0x%x", p_link->conn_handle);
#if F_APP_MCP_SUPPORT_WORKAROUND
    p_link->media_state = MCS_MEDIA_PAUSED_STATE;
    //app_le_audio_link_sm(p_link->conn_handle, LE_AUDIO_MCP_STATE, NULL);
#endif
#if F_APP_CCP_SUPPORT_WORKAROUND
    p_link->call_status = BT_CCP_CALL_IDLE;
    ccp_call_status = BT_CCP_CALL_IDLE;
    mtc_topology_dm(MTC_TOPO_EVENT_CCP);
#endif
    T_BLE_ASE_LINK *p_ase_link = app_le_audio_find_ase_by_id(p_link->conn_handle, ase_id);
    if (p_ase_link != NULL)
    {

        media_state = ISOCH_DATA_PKT_STATUS_LOST_DATA;
        audio_track_release(p_ase_link->handle);
    }
}

T_BIS_CB *app_find_bis_cb(uint8_t bis_idx)
{
    uint8_t i;
    /* Check if the bis is already in bis list*/
    for (i = 0; i < APP_SYNC_RECEIVER_MAX_BIS_NUM; i++)
    {
        if ((bis_cb[i].used == true) &&
            (bis_cb[i].bis_idx == bis_idx))
        {
            return &bis_cb[i];
        }
    }
    return NULL;
}

bool app_le_audio_bis_cb_allocate(uint8_t bis_idx, T_CODEC_CFG *bis_codec_cfg)
{
    if (app_find_bis_cb(bis_idx) != NULL)
    {
        return true;
    }

    bool ret = false;
    T_BIS_CB *p_bis_cb = NULL;
    uint8_t i;

    for (i = 0; i < APP_SYNC_RECEIVER_MAX_BIS_NUM; i++)
    {
        if (bis_cb[i].used == false)
        {
            p_bis_cb = &bis_cb[i];

            p_bis_cb->used = true;
            p_bis_cb->bis_idx = bis_idx;
            if (bis_codec_cfg != NULL)
            {
                p_bis_cb->bis_codec_cfg.frame_duration = bis_codec_cfg->frame_duration;
                p_bis_cb->bis_codec_cfg.sample_frequency = bis_codec_cfg->sample_frequency;
                p_bis_cb->bis_codec_cfg.codec_frame_blocks_per_sdu = bis_codec_cfg->codec_frame_blocks_per_sdu;
                p_bis_cb->bis_codec_cfg.octets_per_codec_frame = bis_codec_cfg->octets_per_codec_frame;
                p_bis_cb->bis_codec_cfg.audio_channel_allocation = bis_codec_cfg->audio_channel_allocation;
                p_bis_cb->bis_codec_cfg.presentation_delay = bis_codec_cfg->presentation_delay;
            }
            APP_PRINT_INFO6("app_le_audio_bis_cb_allocate: bis_idx [%d], frame_duration: 0x%x, sample_frequency: 0x%x, codec_frame_blocks_per_sdu: 0x%x, octets_per_codec_frame: 0x%x, audio_channel_allocation: 0x%x",
                            p_bis_cb->bis_idx, p_bis_cb->bis_codec_cfg.frame_duration, p_bis_cb->bis_codec_cfg.sample_frequency,
                            p_bis_cb->bis_codec_cfg.codec_frame_blocks_per_sdu, p_bis_cb->bis_codec_cfg.octets_per_codec_frame,
                            p_bis_cb->bis_codec_cfg.audio_channel_allocation);
            ret = true;
            break;
        }
    }

    return ret;
}

void app_lea_check_bis_cb(uint8_t *index)
{
    uint8_t i;
    for (i = 0; i < APP_SYNC_RECEIVER_MAX_BIS_NUM; i++)
    {
        if (i != *index)
        {
            T_BIS_CB *temp = app_find_bis_cb(i) ;
            if (temp != NULL)
            {
                app_le_audio_free_bis(temp);
            }
        }
    }
}

T_BIS_CB *app_le_audio_find_bis_by_conn_handle(uint16_t conn_handle)
{
    T_BIS_CB *p_bis_cb = NULL;
    uint8_t        i;

    for (i = 0; i < APP_SYNC_RECEIVER_MAX_BIS_NUM; i++)
    {
        if (bis_cb[i].used == true &&
            bis_cb[i].bis_conn_handle == conn_handle)
        {
            p_bis_cb = &bis_cb[i];
            break;
        }
    }

    return p_bis_cb;
}

T_BIS_CB *app_le_audio_find_bis_by_source_id(uint8_t source_id)
{
    T_BIS_CB *p_bis_cb = NULL;
    uint8_t        i;

    for (i = 0; i < APP_SYNC_RECEIVER_MAX_BIS_NUM; i++)
    {
        if (bis_cb[i].used == true &&
            bis_cb[i].source_id == source_id)
        {
            p_bis_cb = &bis_cb[i];
            break;
        }
    }

    return p_bis_cb;
}

T_BIS_CB *app_le_audio_find_bis_by_bis_idx(uint8_t bis_idx)
{
    T_BIS_CB *p_bis_cb = NULL;
    uint8_t        i;

    for (i = 0; i < APP_SYNC_RECEIVER_MAX_BIS_NUM; i++)
    {
        if (bis_cb[i].used == true &&
            bis_cb[i].bis_idx == bis_idx)
        {
            p_bis_cb = &bis_cb[i];
            break;
        }
    }

    return p_bis_cb;
}

bool app_le_audio_free_bis(T_BIS_CB *p_bis_cb)
{
    if (p_bis_cb != NULL)
    {
        if (p_bis_cb->used == true)
        {
            memset(p_bis_cb, 0, sizeof(T_BIS_CB));
            return true;
        }
    }

    return false;
}



void app_le_audio_device_idle(uint8_t event, void *p_data)
{
    bool change = true;
    APP_PRINT_INFO2("app_le_audio_device_idle: event %x, state %x", event, le_audio_device_sm);
    switch (event)
    {
    case LE_AUDIO_FE_SHAKING_DONE:
    case LE_AUDIO_AFE_SHAKING_DONE:
        {
            if (app_cfg_const.cis_autolink != T_LEA_CIS_AUTO)
            {
                change = false;
                break;
            }
            app_le_audio_adv_start(true, (uint8_t)LEA_ADV_MODE_PAIRING);
        } break;

    case LE_AUDIO_ENTER_PAIRING:
        {
            if (app_cfg_const.cis_autolink != T_LEA_CIS_AUTO)
            {
                change = false;
                break;
            }
            app_le_audio_adv_start(true, (uint8_t)LEA_ADV_MODE_PAIRING);
        } break;

    case LE_AUDIO_CIG_START:
        {

            if ((*(uint16_t *)p_data == (HCI_ERR | HCI_ERR_REMOTE_USER_TERMINATE)) ||
                (*(uint16_t *)p_data == (HCI_ERR | HCI_ERR_CONN_TIMEOUT)))
            {
                app_le_audio_adv_start(true, (uint8_t)LEA_ADV_MODE_LOSSBACK);
            }
            else
            {
                app_le_audio_adv_start(true, (uint8_t)LEA_ADV_MODE_PAIRING);
            }
        }
        break;
    case LE_AUDIO_MMI:
        {
            app_le_audio_adv_start(false, (uint8_t)LEA_ADV_MODE_PAIRING);
        } break;

    case LE_AUDIO_ADV_START:
        {
            app_le_audio_adv_start(false, (uint8_t)LEA_ADV_MODE_PAIRING);
        } break;

    default:
        change = false;
        break;
    }

    if (change)
    {
        app_le_audio_device_state_change(LE_AUDIO_DEVICE_ADV);
    }
}

void app_le_audio_device_adv(uint8_t event, void *p_data)
{
    APP_PRINT_INFO2("app_le_audio_device_adv: event %x, state %x", event, le_audio_device_sm);
    switch (event)
    {
    case LE_AUDIO_POWER_OFF:
        app_le_audio_adv_stop(true);
        app_le_audio_device_state_change(LE_AUDIO_DEVICE_IDLE);
        break;

    case LE_AUDIO_ENTER_PAIRING:
        app_le_audio_adv_stop(false);
        app_le_audio_adv_start(true, (uint8_t)LEA_ADV_MODE_PAIRING);
        break;

    case LE_AUDIO_EXIT_PAIRING:
        app_le_audio_adv_stop(true);
        app_le_audio_device_state_change(LE_AUDIO_DEVICE_IDLE);
        break;

    case LE_AUDIO_ADV_STOP:
        {
            app_le_audio_adv_stop(true);
            app_le_audio_device_state_change(LE_AUDIO_DEVICE_IDLE);
        }
        break;

    case LE_AUDIO_ADV_TIMEOUT:
        {
            if ((*(T_LEA_ADV_TO *)p_data) == LEA_ADV_TO_CIS)
            {
                app_audio_tone_type_play(TONE_CIS_TIMEOUT, false, false);
            }
            app_le_audio_adv_stop(true);
            app_le_audio_device_state_change(LE_AUDIO_DEVICE_IDLE);
        } break;

    case LE_AUDIO_ADV_START:
        app_le_audio_adv_start(true, (uint8_t)LEA_ADV_MODE_PAIRING);
        break;

    default:
        break;
    }
}

void app_le_audio_cis_state_machine(uint8_t event, void *p_data)
{
    switch (le_audio_device_sm)
    {
    case LE_AUDIO_DEVICE_IDLE:
        {
            app_le_audio_device_idle(event, p_data);
        }
        break;

    case LE_AUDIO_DEVICE_ADV:
        {
            app_le_audio_device_adv(event, p_data);
        }
        break;

    default:
        break;
    }
}

void app_le_audio_device_sm(uint8_t event, void *p_data)
{
    APP_PRINT_INFO4("app_le_audio_device_sm: iso_mode %x event %x, state %x, bis_audio_state_machine %x",
                    app_cfg_const.iso_mode, event, le_audio_device_sm, bis_audio_state_machine);
    if (app_cfg_const.iso_mode == LE_AUDIO_CIS || app_cfg_const.iso_mode == LE_AUDIO_ALL)
    {
        app_le_audio_cis_state_machine(event, p_data);
    }
    if (app_cfg_const.iso_mode == LE_AUDIO_BIS || app_cfg_const.iso_mode == LE_AUDIO_ALL)
    {
        app_le_audio_bis_state_machine(event, p_data);
    }
}

void app_le_audio_link_state_change(T_APP_LE_LINK *p_link, T_LE_AUDIO_LINK_SM state)
{
    APP_PRINT_INFO2("app_le_audio_link_state_change: change from %d to %d", p_link->link_state, state);
    p_link->link_state = state;
}

void app_le_audio_link_idle(T_APP_LE_LINK *p_link, uint8_t event, void *p_data)
{
    APP_PRINT_INFO2("app_le_audio_link_idle: event %x, state %x", event, p_link->link_state);
    switch (event)
    {
    case LE_AUDIO_CONNECT:
        mtc_cis_link_num(MTC_CIS_INCREASE);
        // TODO: to avoid service discovery taking long time, change to 7.5ms
        //ble_set_prefer_conn_param(p_link->conn_id, 0x06, 0x06, 0, 500);
        app_le_audio_link_state_change(p_link, LE_AUDIO_LINK_CONNECTED);
        mtc_topology_dm(MTC_TOPO_EVENT_C_LINK);
        break;

    case LE_AUDIO_A2DP_START:
    case LE_AUDIO_AVRCP_PLAYING:
        mtc_topology_dm(MTC_TOPO_EVENT_A2DP);
        break;

    case LE_AUDIO_HFP_CALL_STATE:
        mtc_topology_dm(MTC_TOPO_EVENT_HFP);
        break;

    default:
        break;
    }
}

void app_le_audio_link_connected(T_APP_LE_LINK *p_link, uint8_t event, void *p_data)
{
    APP_PRINT_INFO2("app_le_audio_link_connected: event %x, state %x", event, p_link->link_state);
    switch (event)
    {
    case LE_AUDIO_DISCONNECT:
        {
            mtc_cis_link_num(MTC_CIS_DECREASE);
            app_le_audio_free_link(p_link);
            app_le_audio_link_state_change(p_link, LE_AUDIO_LINK_IDLE);
#if F_APP_CCP_SUPPORT
            app_le_audio_update_call_status();
#endif
            mtc_topology_dm(MTC_TOPO_EVENT_C_DIS_LINK);
        }
        break;

    case LE_AUDIO_CONFIG_CODEC:
        {
            uint8_t i;
            T_ASCS_CP_CONFIG_CODEC *p_info = (T_ASCS_CP_CONFIG_CODEC *)p_data;

            for (i = 0; i < p_info->number_of_ase; i++)
            {
                app_le_audio_ase_allocate(p_info->conn_handle, p_info->param[i].data.ase_id,
                                          p_info->param[i].codec_parsed_data);
            }
            // TODO: service discovery done, change to 60ms
            //ble_set_prefer_conn_param(p_link->conn_id, 0x30, 0x30, 0, 500);
        }
        break;

    case LE_AUDIO_CONFIG_QOS:
        {
            uint8_t i;
            T_BLE_ASE_LINK *p_ase_link;
            T_ASCS_CP_CONFIG_QOS *p_info = (T_ASCS_CP_CONFIG_QOS *)p_data;

            for (i = 0; i < p_info->number_of_ase; i++)
            {
                p_ase_link = app_le_audio_find_ase_by_id(p_info->conn_handle, p_info->param[i].ase_id);
                if (p_ase_link != NULL)
                {
                    LE_ARRAY_TO_UINT24(p_ase_link->presentation_delay, p_info->param[i].presentation_delay);
                }
            }
        }
        break;

    case LE_AUDIO_ENABLE:
        {
            uint8_t i;
            T_BLE_ASE_LINK *p_ase_link;
            T_ASCS_CP_ENABLE *p_info = (T_ASCS_CP_ENABLE *)p_data;

            for (i = 0; i < p_info->number_of_ase; i++)
            {
                uint16_t audio_context = 0;
                audio_context = app_le_audio_get_contexts(p_info->param[i].p_metadata,
                                                          p_info->param[i].metadata_length);
                p_ase_link = app_le_audio_find_ase_by_id(p_info->conn_handle, p_info->param[i].ase_id);
                if (p_ase_link != NULL)
                {
                    p_ase_link->audio_context = audio_context;
                    app_le_audio_track_create_for_cis(p_link, p_info->param[i].ase_id);
                }
            }

            p_ase_link = app_le_audio_find_ase_by_conn_handle(p_info->conn_handle);
            if (p_ase_link != NULL)
            {
                if (mtc_stream_switch())
                {
                    p_ase_link->handshake_fg = true;
                    ascs_app_ctl_handshake(p_ase_link->conn_handle, p_ase_link->ase_id, false);
                }
            }
            if (app_bt_policy_get_b2b_connected())
            {
                if (app_cfg_nv.bud_role == REMOTE_SESSION_ROLE_PRIMARY)
                {
                    if ((app_le_audio_get_call_status() != BT_CCP_CALL_IDLE) ||
                        (app_hfp_get_call_status() != BT_HFP_CALL_IDLE))
                    {
                        T_MCS_CLIENT_MCP_PARAM param;
                        param.p_mcp_cb = NULL;
                        param.opcode = PAUSE_CONTROL_OPCODE;
                        mcs_send_mcp_op(p_link->conn_handle, 0, p_link->general_mcs, &param);
                    }
                }
            }
            else
            {
                if ((app_le_audio_get_call_status() != BT_CCP_CALL_IDLE) ||
                    (app_hfp_get_call_status() != BT_HFP_CALL_IDLE))
                {
                    T_MCS_CLIENT_MCP_PARAM param;
                    param.p_mcp_cb = NULL;
                    param.opcode = PAUSE_CONTROL_OPCODE;
                    mcs_send_mcp_op(p_link->conn_handle, 0, p_link->general_mcs, &param);
                }
            }
        }
        break;

    case LE_AUDIO_SETUP_DATA_PATH:
        {
            T_BLE_ASE_LINK *p_ase_link;
            T_ASCS_SETUP_DATA_PATH *p_info = (T_ASCS_SETUP_DATA_PATH *)p_data;

            p_ase_link = app_le_audio_find_ase_by_id(p_link->conn_handle, p_info->ase_id);
            if (p_ase_link != NULL)
            {
                p_ase_link->cis_conn_handle = p_info->cis_conn_handle;
            }
        }
        break;

    case LE_AUDIO_REMOVE_DATA_PATH:
        {
            T_ASCS_REMOVE_DATA_PATH *p_info = (T_ASCS_REMOVE_DATA_PATH *)p_data;

            app_le_audio_track_release_for_cis(p_link, p_info->ase_id);
            app_disallow_legacy_stream(false);
        }
        break;

    case LE_AUDIO_STREAMING:
        {
            uint8_t event = (uint8_t)MTC_TOPO_EVENT_TOTAL;
            T_BLE_ASE_LINK *p_ase_link;

            p_ase_link = app_le_audio_find_ase_by_conn_handle(p_link->conn_handle);
            if (p_ase_link != NULL)
            {

                if (p_ase_link->audio_context == AUDIO_CONTEXT_MEDIA)
                {
#if F_APP_MCP_SUPPORT
                    app_mcp_set_active_conn_handle(p_ase_link->conn_handle);
                    event = MTC_TOPO_EVENT_MCP;
#endif
                }
                else if (p_ase_link->audio_context == AUDIO_CONTEXT_CONVERSATIONAL)
                {
#if F_APP_CCP_SUPPORT
                    app_ccp_set_active_conn_handle(p_ase_link->conn_handle);
                    event = MTC_TOPO_EVENT_CCP;
#if F_APP_CCP_SUPPORT_WORKAROUND
                    p_link->call_status = BT_CCP_CALL_ACTIVE;
                    ccp_call_status = BT_CCP_CALL_ACTIVE;
#endif
#endif
                }
                app_stop_inactive_ble_link_stream(p_link);
            }

            mtc_topology_dm(event);
            app_le_audio_link_state_change(p_link, LE_AUDIO_LINK_STREAMING);
            // TODO: streaming state, change to 40ms
            //ble_set_prefer_conn_param(p_link->conn_id, 0x20, 0x20, 0, 500);
        }
        break;

    case LE_AUDIO_PAUSE:
        {
            T_BLE_ASE_LINK *p_ase_link;

            p_ase_link = app_le_audio_find_ase_by_conn_handle(p_link->conn_handle);
            if (p_ase_link != NULL)
            {
                app_le_audio_free_ase(p_ase_link);
            }
            //is need legacy
        }
        break;

    case LE_AUDIO_A2DP_START:
    case LE_AUDIO_AVRCP_PLAYING:
        if (p_link->call_status != BT_CCP_CALL_IDLE)
        {
            T_APP_BR_LINK *p_edr_link;
            p_edr_link = &app_db.br_link[app_get_active_a2dp_idx()];

            if (p_edr_link != NULL)
            {
                if (p_edr_link->avrcp_play_status == BT_AVRCP_PLAY_STATUS_PLAYING)
                {
                    bt_avrcp_pause(p_edr_link->bd_addr);
                    p_edr_link->avrcp_play_status = BT_AVRCP_PLAY_STATUS_PAUSED;
                }
            }
        }//voice ignore a2dp
        mtc_topology_dm(MTC_TOPO_EVENT_A2DP);
        break;

#if F_APP_MCP_SUPPORT
    case LE_AUDIO_MCP_STATE:
        {
            if (p_link->media_state == MCS_MEDIA_PLAYING_STATE)
            {
                if ((app_le_audio_get_call_status() != BT_CCP_CALL_IDLE) ||
                    (app_hfp_get_call_status() != BT_HFP_CALL_IDLE))
                {
                    T_MCS_CLIENT_MCP_PARAM param;
                    param.p_mcp_cb = NULL;
                    param.opcode = STOP_CONTROL_OPCODE;
                    mcs_send_mcp_op(p_link->conn_handle, 0, p_link->general_mcs, &param);
                }
                else
                {
                    app_mcp_set_active_conn_handle(p_link->conn_handle);
                }
            }
        }
        break;
#endif
    case LE_AUDIO_HFP_CALL_STATE:
        {
            mtc_topology_dm(MTC_TOPO_EVENT_HFP);
        }
        break;

#if F_APP_CCP_SUPPORT
    case LE_AUDIO_CCP_CALL_STATE:
        {
            uint8_t call_index;
            uint8_t call_state;
            T_BLE_CALL_LINK *p_call_link;
            uint8_t call_list_num = 0;
            T_CCP_NOTIFY_DATA *p_notify_data = (T_CCP_NOTIFY_DATA *)p_data;
            call_list_num = p_notify_data->value.call_state.len / CCP_CALL_STATE_CHARA_LEN;

            mtc_stream_switch();
            app_stop_inactive_ble_link_stream(p_link);

            // each le audio link could have several calls
            // allocate call link and update call state, active call index
            for (uint8_t i = 0; i < call_list_num; i++)
            {
                call_index = p_notify_data->value.call_state.p_call_state[i * CCP_CALL_STATE_CHARA_LEN];
                call_state = p_notify_data->value.call_state.p_call_state[i * CCP_CALL_STATE_CHARA_LEN + 1];

                p_call_link = app_le_audio_find_call_link_by_idx(p_link->conn_id, call_index);
                if (p_call_link != NULL)
                {
                    if (p_call_link->call_state != call_state)
                    {
                        p_call_link->call_state = call_state;
                    }
                }
                else
                {
                    app_le_audio_call_allocate(p_link->conn_id, call_index, call_state);
                }
            }

            app_le_audio_update_link_call_status(p_link);
            app_le_audio_update_call_status();
            mtc_topology_dm(MTC_TOPO_EVENT_CCP);
        }
        break;

    case LE_AUDIO_CCP_READ_RESULT:
        {
            uint8_t call_index;
            uint8_t call_state;
            T_BLE_CALL_LINK *p_call_link;
            uint8_t call_list_num = 0;
            T_CCP_READ_RESULT *p_read_result = (T_CCP_READ_RESULT *)p_data;

            call_list_num = p_read_result->value.call_state.len / CCP_CALL_STATE_CHARA_LEN;
            // each le audio link could have several calls
            // allocate call link and update call state, active call index
            for (uint8_t i = 0; i < call_list_num; i++)
            {
                call_index = p_read_result->value.call_state.p_call_state[i * CCP_CALL_STATE_CHARA_LEN];
                call_state = p_read_result->value.call_state.p_call_state[i * CCP_CALL_STATE_CHARA_LEN + 1];

                p_call_link = app_le_audio_find_call_link_by_idx(p_link->conn_id, call_index);
                if (p_call_link != NULL)
                {
                    if (p_call_link->call_state != call_state)
                    {
                        p_call_link->call_state = call_state;
                    }
                }
                else
                {
                    app_le_audio_call_allocate(p_link->conn_id, call_index, call_state);
                }
            }

            app_le_audio_update_link_call_status(p_link);
            app_le_audio_update_call_status();
            mtc_topology_dm(MTC_TOPO_EVENT_CCP);
        }
        break;

    case LE_AUDIO_CCP_TERM_NOTIFY:
        {
            T_BLE_CALL_LINK *p_call_link;
            T_CCP_NOTIFY_DATA *p_notify_data = (T_CCP_NOTIFY_DATA *)p_data;

            uint8_t call_index = p_notify_data->value.term_reason.call_index;

            p_call_link = app_le_audio_find_call_link_by_idx(p_link->conn_id, call_index);
            if (p_call_link != NULL)
            {
                app_le_audio_free_call_link(p_call_link);
            }
            app_le_audio_update_link_call_status(p_link);
            app_le_audio_update_call_status();
        }
        break;
#endif

    case LE_AUDIO_SNIFFING_STOP:
        {
            T_BLE_ASE_LINK *p_ase_link;

            p_ase_link = app_le_audio_find_ase_by_conn_handle(p_link->conn_handle);
            if (p_ase_link != NULL)
            {
                if (p_ase_link->handshake_fg)
                {
                    p_ase_link->handshake_fg = false;
                    ascs_action_rec_start_ready(p_ase_link->conn_handle, p_ase_link->ase_id);
                    ascs_app_ctl_handshake(p_ase_link->conn_handle, p_ase_link->ase_id, false);
                }
            }
        }
        break;

    default:
        break;
    }
}

void app_le_audio_link_streaming(T_APP_LE_LINK *p_link, uint8_t event, void *p_data)
{
    APP_PRINT_INFO2("app_le_audio_link_streaming: event %x, state %x", event, p_link->link_state);
    switch (event)
    {
    case LE_AUDIO_DISCONNECT:
        {
            app_le_audio_free_link(p_link);
            app_le_audio_link_state_change(p_link, LE_AUDIO_LINK_IDLE);
#if F_APP_CCP_SUPPORT
            app_le_audio_update_call_status();
#endif
            mtc_topology_dm(MTC_TOPO_EVENT_C_DIS_LINK);
        }
        break;

    case LE_AUDIO_CONFIG_CODEC:
        {
            uint8_t i;
            T_ASCS_CP_CONFIG_CODEC *p_info = (T_ASCS_CP_CONFIG_CODEC *)p_data;

            for (i = 0; i < p_info->number_of_ase; i++)
            {
                app_le_audio_ase_allocate(p_info->conn_handle, p_info->param[i].data.ase_id,
                                          p_info->param[i].codec_parsed_data);
            }
            // TODO: service discovery done, change to 60ms
            //ble_set_prefer_conn_param(p_link->conn_handle, 0x30, 0x30, 0, 500);
        }
        break;

    case LE_AUDIO_CONFIG_QOS:
        {
            uint8_t i;
            T_BLE_ASE_LINK *p_ase_link;
            T_ASCS_CP_CONFIG_QOS *p_info = (T_ASCS_CP_CONFIG_QOS *)p_data;

            for (i = 0; i < p_info->number_of_ase; i++)
            {
                p_ase_link = app_le_audio_find_ase_by_id(p_info->conn_handle, p_info->param[i].ase_id);
                if (p_ase_link != NULL)
                {
                    LE_ARRAY_TO_UINT24(p_ase_link->presentation_delay, p_info->param[i].presentation_delay);
                }
            }
        }
        break;

    case LE_AUDIO_ENABLE:
        {
            uint8_t i;
            T_BLE_ASE_LINK *p_ase_link;
            T_ASCS_CP_ENABLE *p_info = (T_ASCS_CP_ENABLE *)p_data;

            for (i = 0; i < p_info->number_of_ase; i++)
            {
                uint16_t audio_context = 0;
                audio_context = app_le_audio_get_contexts(p_info->param[i].p_metadata,
                                                          p_info->param[i].metadata_length);
                p_ase_link = app_le_audio_find_ase_by_id(p_info->conn_handle, p_info->param[i].ase_id);
                if (p_ase_link != NULL)
                {
                    p_ase_link->audio_context = audio_context;
                    app_le_audio_track_create_for_cis(p_link, p_info->param[i].ase_id);
                }
            }
        }
        break;

    case LE_AUDIO_SETUP_DATA_PATH:
        {
            T_BLE_ASE_LINK *p_ase_link;
            T_ASCS_SETUP_DATA_PATH *p_info = (T_ASCS_SETUP_DATA_PATH *)p_data;

            p_ase_link = app_le_audio_find_ase_by_id(p_link->conn_handle, p_info->ase_id);
            if (p_ase_link != NULL)
            {
                p_ase_link->cis_conn_handle = p_info->cis_conn_handle;
            }
        }
        break;

    case LE_AUDIO_REMOVE_DATA_PATH:
        {
            T_ASCS_REMOVE_DATA_PATH *p_info = (T_ASCS_REMOVE_DATA_PATH *)p_data;

            app_le_audio_track_release_for_cis(p_link, p_info->ase_id);
        }
        break;

    case LE_AUDIO_PAUSE:
        {
            bool le_audio_fg = false;
            T_BLE_ASE_LINK *p_ase_link;

            p_ase_link = app_le_audio_find_ase_by_conn_handle(p_link->conn_handle);
            if (p_ase_link != NULL)
            {
                app_le_audio_free_ase(p_ase_link);
            }
            app_le_audio_link_state_change(p_link, LE_AUDIO_LINK_CONNECTED);

            for (uint8_t i = 0; i < MAX_BLE_LINK_NUM; i++)
            {
                if (app_db.le_link[i].used == true &&
                    app_db.le_link[i].link_state == LE_AUDIO_LINK_STREAMING)
                {
                    le_audio_fg = true;
                    break;
                }
            }

            if (le_audio_fg == false)
            {
                T_AUDIO_TRACK_STATE state;
                T_APP_BR_LINK *p_edr_link;

                p_edr_link = &app_db.br_link[app_get_active_a2dp_idx()];
                audio_track_state_get(p_edr_link->a2dp_track_handle, &state);
                if (state == AUDIO_TRACK_STATE_PAUSED ||
                    state == AUDIO_TRACK_STATE_STOPPED)
                {
                    if (p_edr_link->streaming_fg == true)
                    {
                        audio_track_start(p_edr_link->a2dp_track_handle);
                    }
                }
            }
            // TODO: pause state, change to 60ms
            //ble_set_prefer_conn_param(p_link->conn_id, 0x30, 0x30, 0, 500);
            APP_PRINT_INFO1("LE_AUDIO_PAUSE: le_audio_fg %x", le_audio_fg);
        }
        break;

    case LE_AUDIO_A2DP_START:
    case LE_AUDIO_AVRCP_PLAYING:
        {
            if (p_link->media_state == MCS_MEDIA_PLAYING_STATE)
            {
                T_BLE_ASE_LINK *p_ase_link;

                p_ase_link = app_le_audio_find_ase_by_conn_handle(p_link->conn_handle);
                if (p_ase_link != NULL)
                {
                    T_MCS_CLIENT_MCP_PARAM param;
                    param.p_mcp_cb = NULL;
                    param.opcode = PAUSE_CONTROL_OPCODE;
                    mcs_send_mcp_op(p_ase_link->conn_handle, 0, p_link->general_mcs, &param);
                }
            }

            if (p_link->call_status != BT_CCP_CALL_IDLE)
            {
                T_APP_BR_LINK *p_edr_link;
                p_edr_link = &app_db.br_link[app_get_active_a2dp_idx()];

                if (p_edr_link != NULL)
                {
                    if (p_edr_link->avrcp_play_status == BT_AVRCP_PLAY_STATUS_PLAYING)
                    {
                        bt_avrcp_pause(p_edr_link->bd_addr);
                        p_edr_link->avrcp_play_status = BT_AVRCP_PLAY_STATUS_PAUSED;
                    }
                }
            }
            mtc_topology_dm(MTC_TOPO_EVENT_A2DP);
        }
        break;

#if F_APP_MCP_SUPPORT
    case LE_AUDIO_MCP_STATE:
        {
            if (p_link->media_state == MCS_MEDIA_PLAYING_STATE)
            {
                if ((app_le_audio_get_call_status() != BT_CCP_CALL_IDLE) ||
                    (app_hfp_get_call_status() != BT_HFP_CALL_IDLE))
                {
                    if (app_bt_policy_get_b2b_connected() && (app_cfg_nv.bud_role == REMOTE_SESSION_ROLE_SECONDARY))
                    {
                        break;
                    }
                    T_MCS_CLIENT_MCP_PARAM param;
                    param.p_mcp_cb = NULL;
                    param.opcode = PAUSE_CONTROL_OPCODE;
                    mcs_send_mcp_op(p_link->conn_handle, 0, p_link->general_mcs, &param);
                }
                else
                {
                    app_mcp_set_active_conn_handle(p_link->conn_handle);
                }
            }
        }
        break;
#endif
    case LE_AUDIO_HFP_CALL_STATE:
        {
            if (app_hfp_get_call_status() != BT_HFP_CALL_IDLE)
            {
                if (p_link->media_state == MCS_MEDIA_PLAYING_STATE)
                {
                    app_le_audio_mmi_handle_action(LEA_MMI_AV_PAUSE);
                }
            }
            mtc_topology_dm(MTC_TOPO_EVENT_HFP);
        }
        break;

#if F_APP_CCP_SUPPORT
    case LE_AUDIO_CCP_CALL_STATE:
        {
            uint8_t call_index;
            uint8_t call_state;
            T_BLE_CALL_LINK *p_call_link;
            uint8_t call_list_num = 0;
            T_CCP_NOTIFY_DATA *p_notify_data = (T_CCP_NOTIFY_DATA *)p_data;

            call_list_num = p_notify_data->value.call_state.len / CCP_CALL_STATE_CHARA_LEN;
            // each le audio link could have several calls
            // allocate call link and update call state, active call index
            for (uint8_t i = 0; i < call_list_num; i++)
            {
                call_index = p_notify_data->value.call_state.p_call_state[i * CCP_CALL_STATE_CHARA_LEN];
                call_state = p_notify_data->value.call_state.p_call_state[i * CCP_CALL_STATE_CHARA_LEN + 1];

                p_call_link = app_le_audio_find_call_link_by_idx(p_link->conn_id, call_index);
                if (p_call_link != NULL)
                {
                    if (p_call_link->call_state != call_state)
                    {
                        p_call_link->call_state = call_state;
                    }
                }
                else
                {
                    app_le_audio_call_allocate(p_link->conn_id, call_index, call_state);
                }
            }

            app_le_audio_update_link_call_status(p_link);
            app_le_audio_update_call_status();
            mtc_topology_dm(MTC_TOPO_EVENT_CCP);
        }
        break;

    case LE_AUDIO_CCP_TERM_NOTIFY:
        {
            T_BLE_CALL_LINK *p_call_link;
            T_CCP_NOTIFY_DATA *p_notify_data = (T_CCP_NOTIFY_DATA *)p_data;

            uint8_t call_index = p_notify_data->value.term_reason.call_index;

            p_call_link = app_le_audio_find_call_link_by_idx(p_link->conn_id, call_index);
            if (p_call_link != NULL)
            {
                app_le_audio_free_call_link(p_call_link);
            }
            app_le_audio_update_link_call_status(p_link);
            app_le_audio_update_call_status();
        }
        break;
#endif

#if F_APP_VCS_SUPPORT
    case LE_AUDIO_VCS_VOL_CHANGE:
        {
            T_BLE_ASE_LINK *p_ase_link;

            if (app_le_audio_get_call_status() != BT_CCP_CALL_IDLE)
            {
                if (p_link->conn_handle != app_ccp_get_active_conn_handle())
                {
                    APP_PRINT_INFO2("vcs vol change: conn_handle %x is not conn_handle 0x%x",
                                    p_link->conn_handle, app_ccp_get_active_conn_handle);
                    break;
                }
                p_ase_link = app_le_audio_find_ase_by_context(p_link->conn_handle, AUDIO_CONTEXT_CONVERSATIONAL);
            }
            else
            {
                if (p_link->conn_handle != app_mcp_get_active_conn_handle())
                {
                    APP_PRINT_INFO2("vcs vol change: conn_handle 0x%x is not active_conn_handle 0x%x",
                                    p_link->conn_handle, app_mcp_get_active_conn_handle);
                    break;
                }
                p_ase_link = app_le_audio_find_ase_by_context(p_link->conn_handle, AUDIO_CONTEXT_MEDIA);
            }

            if (p_ase_link != NULL)
            {
                uint8_t level;

                level = (p_link->volume_setting * MAX_VCS_OUTPUT_LEVEL) / MAX_VCS_VOLUME_SETTING;
#if HARMAN_OPEN_LR_FEATURE
                app_harman_lr_balance_set(AUDIO_STREAM_TYPE_PLAYBACK, (p_link->mute) ? 0 : level, __func__,
                                          __LINE__);
#endif
                audio_track_volume_out_set(p_ase_link->handle, (p_link->mute) ? 0 : level);
            }
        }
        break;
#endif
    default:
        break;
    }
}

void app_le_audio_link_sm(uint16_t conn_handle, uint8_t event, void *p_data)
{

    T_APP_LE_LINK *p_link;
    p_link = app_find_le_link_by_conn_handle(conn_handle);

    if (p_link == NULL)
    {
        return;
    }
    APP_PRINT_INFO3("app_le_audio_link_sm: conn_handle 0x%x, event %x, %d", conn_handle, event,
                    p_link->link_state);
    switch (p_link->link_state)
    {
    case LE_AUDIO_LINK_IDLE:
        app_le_audio_link_idle(p_link, event, p_data);
        break;

    case LE_AUDIO_LINK_CONNECTED:
        app_le_audio_link_connected(p_link, event, p_data);
        break;

    case LE_AUDIO_LINK_STREAMING:
        app_le_audio_link_streaming(p_link, event, p_data);
        break;

    default:
        break;
    }
    app_le_audio_dump_ase_info(p_link);
    app_le_audio_dump_call_info(p_link);
}

void app_le_audio_a2dp_pause(void)
{
    APP_PRINT_INFO0("app_le_audio_a2dp_pause");
    uint8_t app_idx = app_get_active_a2dp_idx();
    if (app_db.br_link[app_idx].avrcp_play_status == BT_AVRCP_PLAY_STATUS_PLAYING)
    {
        bt_avrcp_pause(app_db.br_link[app_idx].bd_addr);
    }
}

void app_le_audio_bis_state_idle(uint8_t event, void *p_data)
{
    APP_PRINT_INFO2("app_le_audio_bis_state_idle: event %x, state %x", event, bis_audio_state_machine);
    switch (event)
    {
    case LE_AUDIO_BIGSYNC:
        {
            uint8_t dev_idx =  APP_BC_SOURCE_NUM;
            app_bc_get_active(&dev_idx);

            if (dev_idx < APP_BC_SOURCE_NUM)
            {
                app_bc_sync_pa_sync(dev_idx);
                APP_PRINT_INFO1("app_mmi_handle_action: g_dev_idx: 0x%x", dev_idx);
            }
            else
            {
                APP_PRINT_ERROR1("MMI_BIG_START failed, g_dev_idx: 0x%x", dev_idx);

                app_le_audio_scan_start(LE_AUDIO_SCAN_TIME);
                app_lea_bis_state_change(LE_AUDIO_BIS_STATE_SCAN);
            }
        }
        break;
    case LE_AUDIO_SCAN_DELEGATOR:
        {
            app_le_audio_adv_start(true, (uint8_t)LEA_ADV_MODE_DELEGATOR);

        }
        break;

    default:
        break;
    }
}

void app_le_audio_bis_state_conn(uint8_t event, void *p_data)
{
    APP_PRINT_INFO2("app_le_audio_bis_state_conn: event %x, state %x", event, bis_audio_state_machine);
    switch (event)
    {
    case LE_AUDIO_STREAMING:
        {
            app_lea_bis_state_change(LE_AUDIO_BIS_STATE_STREAMING);
        }
        break;

    case LE_AUDIO_SCAN_DELEGATOR:
        {
            T_BIG_MGR_SYNC_RECEIVER_BIG_CREATE_SYNC_PARAM sync_param;
            sync_param.encryption = 0;
            sync_param.mse = 0;
            sync_param.big_sync_timeout = 100;
            sync_param.num_bis = 1;
            sync_param.bis[0] = app_cfg_const.subgroup;
            ble_audio_big_sync_establish(app_bass_get_sync_handle(source_id), &sync_param);
        }
        break;
    default:
        break;
    }
}

void app_le_audio_bis_state_scan(uint8_t event, void *p_data)
{
    APP_PRINT_INFO2("app_le_audio_bis_state_scan: event %x, state %x", event, bis_audio_state_machine);
    switch (event)
    {
    case LE_AUDIO_BIGTERMINATE:
    case LE_AUDIO_SCAN_STOP:
    case LE_AUDIO_SCAN_TIMEOUT:
        {
            uint8_t dev_idx =  APP_BC_SOURCE_NUM;
            T_BLE_AUDIO_SYNC_HANDLE handle = NULL;
            app_le_audio_scan_stop();
            app_bc_get_active(&dev_idx);
            //APP_PRINT_INFO1("app_le_audio_bis_state_scan: dev_idx %d", dev_idx);
            if (dev_idx < APP_BC_SOURCE_NUM)
            {
                app_bc_sync_pa_terminate(dev_idx);
                app_bc_sync_big_terminate(dev_idx);
                app_bc_find_handle(&handle, &dev_idx);
                if (handle != NULL && ble_audio_sync_realese(handle))
                {
                    //APP_PRINT_INFO0("app_le_audio_bis_state_scan: handle_exist");
                    app_bc_sync_clear_device(handle);
                    app_bc_reset();
                    app_lea_bis_state_change(LE_AUDIO_BIS_STATE_IDLE);
                }
            }
            else
            {
                app_lea_bis_state_change(LE_AUDIO_BIS_STATE_IDLE);
            }
            app_disallow_legacy_stream(false);
        }
        break;

    case LE_AUDIO_STREAMING:
        {
            app_lea_bis_state_change(LE_AUDIO_BIS_STATE_STREAMING);
        }
        break;

    default:
        break;
    }
}

void app_le_audio_bis_state_streaming(uint8_t event, void *p_data)
{
    APP_PRINT_INFO3("app_le_audio_bis_state_streaming: event %x, state %x, %x", event,
                    bis_audio_state_machine, big_passive_flag);
#if F_APP_TMAP_BMR_SUPPORT
    switch (event)
    {
    case LE_AUDIO_A2DP_START:
    case LE_AUDIO_BIGTERMINATE:
        {

            //when enter bass mode, set big_passive_flag to 1
            if (!big_passive_flag)
            {
                uint8_t dev_idx =  APP_BC_SOURCE_NUM;
                app_bc_get_active(&dev_idx);
                if (dev_idx < APP_BC_SOURCE_NUM)
                {
                    app_bc_sync_big_terminate(dev_idx);
                }
                else
                {
                    APP_PRINT_ERROR1("LE_AUDIO_BIGTERMINATE: invaild dev_idx: 0x%x", g_dev_idx);
                }
            }
            else
            {
                ble_audio_big_terminate(app_bass_get_sync_handle(source_id));
                app_lea_bis_state_change(LE_AUDIO_BIS_STATE_CONN);
            }
        }
        break;

    default:
        break;
    }
#endif
}

void app_le_audio_bis_state_machine(uint8_t event, void *p_data)
{
    APP_PRINT_INFO2("app_le_audio_bis_state_machine: event %x, state %x", event,
                    bis_audio_state_machine);

    switch (bis_audio_state_machine)
    {
    case LE_AUDIO_BIS_STATE_IDLE:
    case LE_AUDIO_BIS_STATE_PRE_SCAN:
    case LE_AUDIO_BIS_STATE_PRE_ADV:
        app_le_audio_bis_state_idle(event, p_data);
        break;

    case LE_AUDIO_BIS_STATE_CONN:
        app_le_audio_bis_state_conn(event, p_data);
        break;

    case LE_AUDIO_BIS_STATE_SCAN:
        app_le_audio_bis_state_scan(event, p_data);
        break;

    case LE_AUDIO_BIS_STATE_STREAMING:
        app_le_audio_bis_state_streaming(event, p_data);
        break;

    default:
        break;
    }
}
bool app_set_samplerate(uint8_t  *in,  uint32_t  *out)
{
    bool result = true;
    switch (*in)
    {
    case  SAMPLING_FREQUENCY_CFG_8K:
        {
            *out = 8000;
        } break;
    case SAMPLING_FREQUENCY_CFG_11K:
        {
            *out = 11000;
        } break;
    case SAMPLING_FREQUENCY_CFG_16K:
        {
            *out = 16000;
        } break;
    case SAMPLING_FREQUENCY_CFG_22K:
        {
            *out = 22000;
        } break;
    case SAMPLING_FREQUENCY_CFG_24K:
        {
            *out = 24000;
        } break;
    case SAMPLING_FREQUENCY_CFG_32K:
        {
            *out = 32000;
        } break;
    case SAMPLING_FREQUENCY_CFG_44_1K:
        {
            *out = 44100;
        } break;
    case SAMPLING_FREQUENCY_CFG_48K:
        {
            *  out = 48000;
        } break;
    case SAMPLING_FREQUENCY_CFG_88K:
        {
            *out = 88000;
        } break;
    case  SAMPLING_FREQUENCY_CFG_96K:
        {
            *out = 96000;
        } break;
    case  SAMPLING_FREQUENCY_CFG_176K:
        {
            *out = 176000;
        } break;
    case  SAMPLING_FREQUENCY_CFG_192K:
        {
            *out = 192000;
        } break;
    case  SAMPLING_FREQUENCY_CFG_384K:
        {
            *out = 384000;
        } break;

    default:
        {
            *  out = 48000;
            result = false;
        } break;
    }

    return result;
}
void app_le_audio_track_create_for_bis(uint16_t bis_conn_handle, uint8_t source_id, uint8_t bis_idx)
{
    T_AUDIO_STREAM_TYPE type = AUDIO_STREAM_TYPE_PLAYBACK;
    uint8_t volume_out = app_cfg_nv.bis_audio_gain_level;
    uint8_t volume_in = 0;
    uint32_t device = AUDIO_DEVICE_OUT_DEFAULT;
    T_AUDIO_FORMAT_INFO format_info;
    T_BIS_CB *p_bis_cb = app_le_audio_find_bis_by_bis_idx(bis_idx);
    if (p_bis_cb == NULL)
    {
        APP_PRINT_ERROR0("app_le_audio_track_create_for_bis: not found bis_cb");
        return;
    }

    p_bis_cb->bis_conn_handle = bis_conn_handle;
    p_bis_cb->source_id = source_id;
    p_bis_cb->frame_num = p_bis_cb->bis_codec_cfg.codec_frame_blocks_per_sdu *
                          count_bits_1(p_bis_cb->bis_codec_cfg.audio_channel_allocation);

    format_info.type = AUDIO_FORMAT_TYPE_LC3;
    format_info.attr.lc3.chann_location = p_bis_cb->bis_codec_cfg.audio_channel_allocation;
    app_set_samplerate(&p_bis_cb->bis_codec_cfg.sample_frequency, &format_info.attr.lc3.sample_rate);
    format_info.attr.lc3.frame_length = p_bis_cb->bis_codec_cfg.octets_per_codec_frame;
    format_info.attr.lc3.frame_duration = (T_AUDIO_LC3_FRAME_DURATION)
                                          p_bis_cb->bis_codec_cfg.frame_duration;
    format_info.attr.lc3.presentation_delay = p_bis_cb->bis_codec_cfg.presentation_delay;
    APP_PRINT_ERROR8("chann_location: 0x%x, sample_rate: 0x%x, sample_freq: 0x%x, frame_num: 0x%x, frame_duration: 0x%x, frame_length: 0x%x, %x, %x",
                     format_info.attr.lc3.chann_location, format_info.attr.lc3.sample_rate,
                     p_bis_cb->bis_codec_cfg.sample_frequency, p_bis_cb->frame_num,
                     format_info.attr.lc3.frame_duration, format_info.attr.lc3.frame_length,
                     p_bis_cb->bis_codec_cfg.codec_frame_blocks_per_sdu,
                     p_bis_cb->bis_codec_cfg.presentation_delay);
    if (format_info.attr.lc3.frame_length == 0)
    {
        format_info.attr.lc3.frame_length = 120;
        format_info.attr.lc3.chann_location = 0x800;
    }
    APP_PRINT_INFO1("app_le_audio_track_create_for_bis: volume_out: 0x%x", volume_out);
    media_state = ISOCH_DATA_PKT_STATUS_LOST_DATA;
    p_bis_cb->audio_track_handle = audio_track_create(type,
                                                      AUDIO_STREAM_MODE_DIRECT,
                                                      AUDIO_STREAM_USAGE_LOCAL,
                                                      format_info,
                                                      volume_out,
                                                      volume_in,
                                                      device,
                                                      NULL,
                                                      NULL);
    if (p_bis_cb->audio_track_handle == NULL)
    {
        APP_PRINT_ERROR0("p_bis_cb->audio_track_handle create failed.");
        return;
    }

    audio_track_start(p_bis_cb->audio_track_handle);
}

void app_le_audio_track_release_for_bis(uint16_t bis_conn_handle)
{
    T_BIS_CB *p_bis_cb = NULL;
    APP_PRINT_INFO1("app_le_audio_track_release_for_bis:  bis_conn_handle 0x%x", bis_conn_handle);
    p_bis_cb = app_le_audio_find_bis_by_conn_handle(bis_conn_handle);
    if (p_bis_cb != NULL && p_bis_cb->audio_track_handle != NULL)
    {

        media_state  = ISOCH_DATA_PKT_STATUS_LOST_DATA;
        audio_track_release(p_bis_cb->audio_track_handle);
    }
}


void app_lea_direct_cback(uint8_t cb_type, void *p_cb_data)
{
    T_BT_DIRECT_CB_DATA *p_data = (T_BT_DIRECT_CB_DATA *)p_cb_data;

    switch (cb_type)
    {
    case BT_DIRECT_MSG_ISO_DATA_IND:
        {
            if (p_data->p_bt_direct_iso->pkt_status_flag == ISOCH_DATA_PKT_STATUS_VALID_DATA)
            {
                media_state = p_data->p_bt_direct_iso->pkt_status_flag;
            }

            if (media_state != ISOCH_DATA_PKT_STATUS_VALID_DATA)
            {
                gap_iso_data_cfm(p_data->p_bt_direct_iso->p_buf);
                break;
            }
#if 1
            uint8_t *p_iso_data = p_data->p_bt_direct_iso->p_buf + p_data->p_bt_direct_iso->offset;
            APP_PRINT_INFO6("app_lea_direct_cback: BT_DIRECT_MSG_ISO_DATA_IND, pkt_status_flag 0x%x, cis_conn_handle 0x%x, pkt_seq_num 0x%x, ts_flag 0x%x, time_stamp 0x%x.media_state=%d",
                            p_data->p_bt_direct_iso->pkt_status_flag, p_data->p_bt_direct_iso->conn_handle,
                            p_data->p_bt_direct_iso->pkt_seq_num, p_data->p_bt_direct_iso->ts_flag,
                            p_data->p_bt_direct_iso->time_stamp,
                            media_state);
            APP_PRINT_INFO5("app_lea_direct_cback: BT_DIRECT_MSG_ISO_DATA_IND, iso_sdu_len 0x%x, p_buf %p, offset %d, p_data %p, data %b",
                            p_data->p_bt_direct_iso->iso_sdu_len, p_data->p_bt_direct_iso->p_buf,
                            p_data->p_bt_direct_iso->offset, p_iso_data, TRACE_BINARY(p_data->p_bt_direct_iso->iso_sdu_len,
                                                                                      p_iso_data));
#endif
            T_BLE_ASE_LINK *p_ase_link;
            p_ase_link = app_le_audio_find_ase_by_loop_conn(p_data->p_bt_direct_iso->conn_handle);
            if (p_ase_link != NULL)
            {
                uint16_t written_len;
                T_AUDIO_STREAM_STATUS status;

                if (p_data->p_bt_direct_iso->iso_sdu_len != 0)
                {
                    status = AUDIO_STREAM_STATUS_CORRECT;
                }
                else
                {
                    status = AUDIO_STREAM_STATUS_LOST;
                }
                audio_track_write(p_ase_link->handle, p_data->p_bt_direct_iso->time_stamp,
                                  p_data->p_bt_direct_iso->pkt_seq_num,
                                  status,
                                  p_ase_link->frame_num,
                                  p_data->p_bt_direct_iso->p_buf + p_data->p_bt_direct_iso->offset,
                                  p_data->p_bt_direct_iso->iso_sdu_len,
                                  &written_len);
            }
#if F_APP_TMAP_BMR_SUPPORT
            T_BIS_CB *p_bis_cb = app_le_audio_find_bis_by_conn_handle(p_data->p_bt_direct_iso->conn_handle);
            if (p_bis_cb != NULL)
            {
                T_AUDIO_STREAM_STATUS status;
                uint16_t written_len;

                if (p_data->p_bt_direct_iso->iso_sdu_len != 0)
                {
                    status = AUDIO_STREAM_STATUS_CORRECT;
                }
                else
                {
                    status = AUDIO_STREAM_STATUS_LOST;
                }
                audio_track_write(p_bis_cb->audio_track_handle, p_data->p_bt_direct_iso->time_stamp,
                                  p_data->p_bt_direct_iso->pkt_seq_num,
                                  status,
                                  p_bis_cb->frame_num, //frame_num
                                  p_data->p_bt_direct_iso->p_buf + p_data->p_bt_direct_iso->offset,
                                  p_data->p_bt_direct_iso->iso_sdu_len,
                                  &written_len);
            }
#endif
            gap_iso_data_cfm(p_data->p_bt_direct_iso->p_buf);
        }
        break;

    default:
        APP_PRINT_ERROR1("app_lea_direct_cback: unhandled cb_type 0x%x", cb_type);
        break;
    }
}

void app_le_audio_track_cback(T_AUDIO_EVENT event_type, void *event_buf, uint16_t buf_len)
{
    T_AUDIO_EVENT_PARAM *param = event_buf;

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
                uint16_t conn_handle = 0;
                T_APP_LE_LINK *p_link;
#if F_APP_CCP_SUPPORT
                conn_handle = app_ccp_get_active_conn_handle();
#endif
                p_link = app_find_le_link_by_conn_handle(conn_handle);
                if (p_link != NULL)
                {
                    T_BLE_ASE_LINK *p_ase_link = app_le_audio_find_ase_by_handle(conn_handle,
                                                                                 param->track_state_changed.handle);
                    if (p_ase_link != NULL)
                    {
                        if (param->track_state_changed.handle == p_ase_link->handle)
                        {
                            p_ase_link->state = param->track_state_changed.state;
                            if (p_ase_link->audio_context == AUDIO_CONTEXT_CONVERSATIONAL)
                            {
                                for (uint8_t i = 0; i < ASCS_ASE_NUM; i++)
                                {
                                    if (p_link->ble_ase_link[i].used == true && p_ase_link != &p_link->ble_ase_link[i])
                                    {
                                        if (p_link->ble_ase_link[i].audio_context == AUDIO_CONTEXT_CONVERSATIONAL)
                                        {
                                            p_link->ble_ase_link[i].state = p_ase_link->state;
                                            break;
                                        }
                                    }
                                }
                            }

                            APP_PRINT_INFO1("app_le_audio_track_cback: state %d", p_ase_link->state);
                        }
                    }
                }
            }
        }
        break;

    case AUDIO_EVENT_TRACK_DATA_IND:
        {
            uint32_t timestamp;
            uint16_t seq_num;
            uint8_t frame_num;
            uint16_t read_len;
            uint8_t *buf;
            T_AUDIO_STREAM_STATUS status;

            if (param->track_data_ind.len == 0)
            {
                return;
            }

            buf = malloc(param->track_data_ind.len);

            if (buf == NULL)
            {
                return;
            }

            if (audio_track_read(param->track_data_ind.handle,
                                 &timestamp,
                                 &seq_num,
                                 &status,
                                 &frame_num,
                                 buf,
                                 param->track_data_ind.len,
                                 &read_len) == true)
            {
                uint16_t conn_handle = 0;

#if F_APP_CCP_SUPPORT
                conn_handle = app_ccp_get_active_conn_handle();
#endif
                T_BLE_ASE_LINK *p_ase_link = app_le_audio_find_ase_by_handle(conn_handle,
                                                                             param->track_data_ind.handle);
                if (p_ase_link != NULL)
                {
                    gap_iso_send_data((uint8_t *)buf, p_ase_link->cis_conn_handle, param->track_data_ind.len, true,
                                      timestamp,
                                      seq_num);
                }
            }
            free(buf);
        }
        break;
    }
}

void app_big_handle_switch(uint8_t event, void *p_data)
{
    APP_PRINT_INFO1("app_big_handle_switch: event %x", event);
    switch (event)
    {
    case LE_AUDIO_STREAMING:
        {
            app_le_audio_a2dp_pause();
        }
        break;

    default:
        break;
    }
}

uint16_t app_isoaudio_relay_cback(uint8_t *buf, uint8_t msg_type, bool total)
{
    return app_relay_msg_pack(buf, msg_type, APP_MODULE_TYPE_ISOAUDIO, 0, NULL, true, total);
}
static void app_isoaudio_parse_cback(uint8_t msg_type, uint8_t *buf, uint16_t len,
                                     T_REMOTE_RELAY_STATUS status)
{
    switch (msg_type)
    {
    case LEA_REMOTE_MMI_SWITCH_SYNC:
        {
            if (status == REMOTE_RELAY_STATUS_SYNC_TOUT ||
                status == REMOTE_RELAY_STATUS_SYNC_EXPIRED ||
                status == REMOTE_RELAY_STATUS_SYNC_REF_CHANGED ||
                status == REMOTE_RELAY_STATUS_ASYNC_RCVD)
            {
                mtc_set_btmode((T_MTC_BT_MODE) * (uint8_t *)buf);
                APP_PRINT_TRACE2("app_isoaudio_parse_cback: le_audio_switch %d, %x", * (uint8_t *)buf,
                                 mtc_get_btmode());
            }
        }
        break;

    default:
        break;
    }
}

static void app_lea_parse_cback(uint8_t msg_type, uint8_t *buf, uint16_t len,
                                T_REMOTE_RELAY_STATUS status)
{
    APP_PRINT_TRACE2("app_lea_parse_cback: msg = 0x%x, status = %x", msg_type, status);
}

uint16_t app_lea_relay_cback(uint8_t *buf, uint8_t msg_type, bool total)
{
    APP_PRINT_TRACE2("app_lea_relay_cback: msg = 0x%x, status = %x", msg_type, total);
    return 0;
}

void app_le_audio_init(void)
{
    gap_register_direct_cb(app_lea_direct_cback);
    audio_mgr_cback_register(app_le_audio_track_cback);
    app_relay_cback_register(app_isoaudio_relay_cback, app_isoaudio_parse_cback,
                             APP_MODULE_TYPE_ISOAUDIO, LEA_REMOTE_MSG_TOTAL);

    ble_audio_init(app_le_audio_msg_cback, audio_evt_queue_handle, audio_io_queue_handle);
    app_le_audio_adv_init();


    mtc_init();
}
#endif
