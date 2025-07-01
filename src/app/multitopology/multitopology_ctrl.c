/**
*****************************************************************************************
*     Copyright(c) 2021, Realtek Semiconductor Corporation. All rights reserved.
*****************************************************************************************
   * @file      multitopology_ctrl.c
   * @brief     This file handles Multi-Topology controller(MTC) process application routines.
   * @author    mj.mengjie.han
   * @date      2021-10-28
   * @version   v1.0
   **************************************************************************************
   * @attention
   * <h2><center>&copy; COPYRIGHT 2021 Realtek Semiconductor Corporation</center></h2>
   **************************************************************************************
  */

/*============================================================================*
*                              Header Files
*============================================================================*/
#include <string.h>
#include "trace.h"
#include "mem_types.h"
#include "os_mem.h"
#include "ascs_mgr.h"
#include "bass_mgr.h"
#include "ble_conn.h"
#include "ble_isoch_def.h"
#include "bt_direct_msg.h"
#include "ccp_client.h"
#include "gap_iso_data.h"
#include "mcs_client.h"
#include "vcs_mgr.h"
//#include "app_broadcast_sync.h"
#include "app_ccp.h"
#include "app_cfg.h"
#include "app_hfp.h"
#include "app_le_audio_adv.h"
#include "app_le_audio_lib.h"
#include "app_le_audio_mgr.h"
#include "app_le_audio_scan.h"
#include "app_le_profile.h"
#include "app_link_util.h"
#include "app_mcp.h"
#include "app_mmi.h"
#include "app_multilink.h"
#include "app_vcs.h"
#include "app_sniff_mode.h"
#include "app_auto_power_off.h"
#include "app_audio_policy.h"
#include "multitopology_ctrl.h"
#include "multitopology_pro.h"
#include "audio_type.h"
#include "bt_a2dp.h"
#include "app_bt_policy_api.h"

/*============================================================================*
 *                              Constants
 *============================================================================*/
#define MTC_MAX_LINK_NUM 2
#define MTC_MAX_LINK_LIMIT_NUM 3

typedef enum t_mtc_bt_mask
{
    MTC_BT_MASK_NONE = 0x00,
    MTC_BT_MASK_CIS  = 0x01,
    MTC_BT_MASK_BIS  = 0x02,
    MTC_BT_MASK_EDR  = 0x04,
} T_MTC_BT_MASK;

typedef enum t_mtc_cis_ter
{
    MTC_CIS_TER_DEF = 0x00,
    MTC_CIS_TER_SET_DISALLOW = 0x01,

} T_MTC_CIS_TER;

typedef enum t_mtc_bis_ter
{
    MTC_BIS_TER_DEF = 0x00,
    MTC_BIS_TER_STOP_ONLY = 0x01,

} T_MTC_BIS_TER;

typedef struct
{
    uint8_t resume_a2dp_idx;
    uint8_t resume_sco_idx;
    uint8_t call_state;
    uint8_t wait_a2dp;
} T_MTC_BREDR_DB;

typedef struct
{
    uint8_t resume_audio_idx;
    uint8_t resume_voice_idx;
} T_MTC_LEA_DB;

typedef struct
{
    T_MTC_BUD_STATUS b2b;
    T_MTC_BUD_STATUS b2s;
    T_MTC_AUDIO_MODE pending;
} T_MTC_BUS_STATUS;

typedef struct
{
    uint8_t               max_conn_num : 3;
    uint8_t               source_legacy_num : 3;
    uint8_t               source_cis_num : 2;

    uint8_t               source_bis_num : 2;
    uint8_t               b2b_conn : 1;
    uint8_t               beeptype : 5;

    uint8_t               mtc_pro_bt_mode : 3;
    uint8_t               mtc_last_bt_mode : 3;
    uint8_t               sniffing_stop : 2;
    T_MTC_BUS_STATUS            bud_linkstatus;
} T_MTC_DEVICE_DB;

typedef struct
{

    T_MTC_BREDR_DB bredr_db;
    T_MTC_LEA_DB lea_db;
    T_MTC_DEVICE_DB device;
} T_MTC_DB;


/*============================================================================*
 *                              Variables
 *============================================================================*/
static T_MTC_DB mtc_db =
{
    .bredr_db = {0xFF, 0xFF, 0, false},
    .lea_db = {0xFF, 0xFF},
    .device = {
        0, 0, 0, 0, 0, MTC_PRO_BEEP_PROMPTLY, MULTI_PRO_BT_BREDR, MULTI_PRO_BT_BREDR, MULTI_PRO_SNIFI_NOINVO,
        {
            .b2b = MTC_BUD_STATUS_ACTIVE,
            .b2s = MTC_BUD_STATUS_ACTIVE,
            .pending = LE_AUDIO_NO,
        }
    },
};
//for keep cis voice and sco testing
//Right now, it must be 0
uint8_t test_bd_addr[6] = {0};
static P_MTC_IF_CB mtc_if_table[MTC_IF_MAX] = {NULL};
static P_MTC_RELAY_PARSE_CB p_audio_policy_relay_pase_cb = NULL;
T_REMOTE_RELAY_HANDLE mtc_relay_handle;


/*============================================================================*
 *                              Functions
 *============================================================================*/
/**
 * @brief   Set limitation of bt link .
 *
 * @param[in] num Limitation of link quantity.
 * @retval true Success.
 * @retval false Failed, exceed limitation of bt-module.
 */
void mtc_get_budstatus(T_MTC_BUS_STATUS **status)
{
    *status = &mtc_db.device.bud_linkstatus;
}

void mtc_set_pending(T_MTC_AUDIO_MODE action)
{
    mtc_db.device.bud_linkstatus.pending = action;
}

T_MTC_AUDIO_MODE mtc_lea_get_pending(void)
{
    return mtc_db.device.bud_linkstatus.pending;
}

T_MTC_BUD_STATUS mtc_get_b2d_sniff_status(void)
{
    return mtc_db.device.bud_linkstatus.b2b;
}

T_MTC_BUD_STATUS mtc_get_b2s_sniff_status(void)
{
    return mtc_db.device.bud_linkstatus.b2s;
}

bool mtc_set_max_link_num(uint8_t num)
{
    if (num > MTC_MAX_LINK_LIMIT_NUM)
    {
        return false;
    }
    else
    {
        mtc_db.device.max_conn_num = num;
    }
    return true;
}

void mtc_pro_hook(uint8_t hook_point, T_RELAY_PARSE_PARA *info)
{
    if (p_audio_policy_relay_pase_cb)
    {
        p_audio_policy_relay_pase_cb(info->msg_type, info->buf, info->len,
                                     info->status);
    }
}
uint8_t mtc_get_resume_a2dp_idx(void)
{
    return mtc_db.bredr_db.resume_a2dp_idx;
}

void mtc_set_resume_a2dp_idx(uint8_t index)
{
    mtc_db.bredr_db.resume_a2dp_idx = index;
}

void mtc_set_beep(T_MTC_PRO_BEEP para)
{
    APP_PRINT_TRACE1("mtc_set_beep: para %d",
                     para);
    mtc_db.device.beeptype = para;
}
T_MTC_PRO_BEEP mtc_get_beep(void)
{
    APP_PRINT_TRACE1("mtc_get_beep: para %d",
                     (T_MTC_PRO_BEEP)mtc_db.device.beeptype);
    return (T_MTC_PRO_BEEP)mtc_db.device.beeptype;
}

void mtc_set_btmode(T_MTC_BT_MODE para)
{
    APP_PRINT_TRACE1("mtc_set_btmode: app_idx %d",
                     para);
    mtc_db.device.mtc_pro_bt_mode = para;
}

T_MTC_BT_MODE mtc_get_btmode(void)
{
    APP_PRINT_TRACE1("mtc_get_btmode: mode %d",
                     mtc_db.device.mtc_pro_bt_mode);
    return (T_MTC_BT_MODE)mtc_db.device.mtc_pro_bt_mode;
}

T_MTC_SNIFI_STATUS mtc_get_sniffing(void)
{
    return (T_MTC_SNIFI_STATUS)mtc_db.device.sniffing_stop;
}

void mtc_terminate_cis(T_MTC_CIS_TER option)
{
    if (app_lea_audio_get_device_state() == LE_AUDIO_DEVICE_ADV)
    {
        app_disallow_legacy_stream(false);
        if (option == MTC_CIS_TER_SET_DISALLOW)
        {
            return;
        }
        if (app_bt_policy_get_b2b_connected())
        {

            remote_sync_msg_relay(mtc_relay_handle,
                                  MTC_REMOTE_SYNC_CIS_STOP,
                                  NULL,
                                  0,
                                  REMOTE_TIMER_HIGH_PRECISION,
                                  0,
                                  false);
        }
        else
        {
            app_le_audio_device_sm(LE_AUDIO_ADV_STOP, NULL);
        }
    }
}

void mtc_terminate_bis(T_MTC_BIS_TER option)
{
    if (app_bt_policy_get_b2b_connected() && option == MTC_BIS_TER_DEF)
    {
        T_MTC_PRO_BEEP para = MTC_PRO_BEEP_NONE;
        remote_sync_msg_relay(mtc_relay_handle,
                              MTC_REMOTE_SYNC_BIS_STOP,
                              &para,
                              1,
                              REMOTE_TIMER_HIGH_PRECISION,
                              0,
                              false);
    }
    else
    {
        mtc_set_beep(MTC_PRO_BEEP_NONE);
        app_mmi_handle_action(MMI_BIG_STOP);
    }
}

bool mtc_check_sniff(uint8_t *bd_addr)
{
    T_APP_BR_LINK *p_link;
    bool result = false;
    p_link = app_find_br_link(bd_addr);
    if (p_link == NULL)
    {
        mtc_db.bredr_db.wait_a2dp = MTC_BT_MASK_NONE;
        result = true;
        goto END_CHECK;
    }
    APP_PRINT_TRACE1("mtc_check_sniff bt_sniffing_state: %d",
                     p_link->bt_sniffing_state);
    if (p_link->bt_sniffing_state == APP_BT_SNIFFING_STATE_STARTING ||
        p_link->bt_sniffing_state == APP_BT_SNIFFING_STATE_SNIFFING)
    {
        mtc_db.bredr_db.wait_a2dp = MTC_BT_MASK_NONE;
        result = true;
    }
END_CHECK:
    return result;
}

void mtc_resume_a2dp(uint8_t app_idx)
{
    T_APP_BR_LINK *plink = NULL;
    mtc_db.device.sniffing_stop = MULTI_PRO_SNIFI_NOINVO;

    if (mtc_db.bredr_db.resume_a2dp_idx == 0xff)
    {
        return;
    }
    plink = app_find_b2s_link_by_index(app_idx);
    if (plink->connected_profile &
        (A2DP_PROFILE_MASK | AVRCP_PROFILE_MASK))
    {
        app_a2dp_active_link_set(plink->bd_addr);
        //   app_bond_set_priority(app_db.br_link[app_db.wait_resume_a2dp_idx].bd_addr);
        if (plink->avrcp_play_status !=
            BT_AVRCP_PLAY_STATUS_PLAYING)
        {
            plink->avrcp_play_status =
                BT_AVRCP_PLAY_STATUS_PLAYING;
            bt_avrcp_play(plink->bd_addr);
        }
    }

    mtc_db.bredr_db.resume_a2dp_idx = 0xFF;

    APP_PRINT_TRACE1("app_le_audio_resume_stream: app_idx %d",
                     app_idx);
}

bool mtc_stream_switch(void)
{
    T_APP_BR_LINK *p_link = NULL;
    bool stopsniffing = false;
    p_link = app_find_b2s_link_by_index(app_get_active_a2dp_idx());
    APP_PRINT_TRACE6("mtc_stream_switch: app_idx %d, p_link->id=%d, %d, %d, %d, bd_addr %s",
                     app_get_active_a2dp_idx(), p_link->id, p_link->streaming_fg, app_bt_policy_get_b2b_connected(),
                     app_cfg_nv.bud_role, TRACE_BDADDR(p_link->bd_addr));

    if (p_link->streaming_fg)
    {
        if (app_bt_policy_get_b2b_connected())
        {
            if (app_cfg_nv.bud_role == REMOTE_SESSION_ROLE_PRIMARY)
            {
                app_bt_sniffing_param_update(APP_BT_SNIFFING_EVENT_ISO_SUSPEND);
                mtc_db.device.sniffing_stop = MULTI_PRO_SNIFI_INVO;
                //mtc_db.bredr_db.resume_a2dp_idx = p_link->id;
                if (p_link->avrcp_play_status == BT_AVRCP_PLAY_STATUS_PLAYING)
                {
                    bt_avrcp_pause(p_link->bd_addr);
                    p_link->avrcp_play_status = BT_AVRCP_PLAY_STATUS_PAUSED;
                }
                app_disallow_legacy_stream(true);
                // Stop Sniffing Now
                app_bt_sniffing_stop(p_link->bd_addr, BT_SNIFFING_TYPE_A2DP);
                stopsniffing = true;
            }
        }
        else
        {
            if (p_link->avrcp_play_status == BT_AVRCP_PLAY_STATUS_PLAYING)
            {
                bt_avrcp_pause(p_link->bd_addr);
                p_link->avrcp_play_status = BT_AVRCP_PLAY_STATUS_PAUSED;
            }
        }
    }
    return stopsniffing;
}

void mtc_legacy_update_call_status(void)
{
    if (app_hfp_get_call_status() == BT_HFP_CALL_IDLE)
    {
        return;
    }
    if (mtc_get_btmode() == MULTI_PRO_BT_CIS)
    {
        app_le_audio_link_sm(app_mcp_get_active_conn_handle(), LE_AUDIO_HFP_CALL_STATE, NULL);
    }
    else if (mtc_get_btmode() == MULTI_PRO_BT_BIS)
    {
        mtc_terminate_bis(MTC_BIS_TER_STOP_ONLY);
    }
}

void mtc_terminate_hfp_call(void)
{
    T_BT_HFP_CALL_STATUS callstate =  app_hfp_get_call_status();
    if (callstate == BT_HFP_VOICE_ACTIVATION_ONGOING)
    {
        app_mmi_handle_action(MMI_CIG_HF_CANCEL_VOICE_DIAL);
    }
    else  if (callstate == BT_HFP_INCOMING_CALL_ONGOING)
    {
        app_mmi_handle_action(MMI_CIG_HF_REJECT_CALL);
    }
    else if (callstate == BT_HFP_OUTGOING_CALL_ONGOING)
    {
        app_mmi_handle_action(MMI_CIG_HF_END_OUTGOING_CALL);
    }
    else if (callstate >= BT_HFP_CALL_ACTIVE)
    {
        app_mmi_handle_action(MMI_CIG_HF_END_ACTIVE_CALL);
    }

}

void mtc_audio_policy_cback(uint8_t msg_type, uint8_t *buf, uint16_t len,
                            T_MULTI_PRO_REMOTE_RELAY_STATUS status)
{
    APP_PRINT_TRACE1("mtc_audio_policy_cback: msg_type %d",
                     msg_type);
    switch (msg_type)
    {
    case APP_REMOTE_MSG_SYNC_CALL_STATUS:
        {
            //mtc_db.bredr_db.call_state

            if (app_hfp_get_call_status() == BT_HFP_CALL_IDLE)
            {
                break;
            }
            if (mtc_get_btmode() == MULTI_PRO_BT_BIS)
            {
                mtc_terminate_bis(MTC_BIS_TER_DEF);
            }
            else  if (mtc_get_btmode() == MULTI_PRO_BT_CIS)
            {
                app_le_audio_link_sm(app_mcp_get_active_conn_handle(), LE_AUDIO_HFP_CALL_STATE, NULL);
            }

        } break;
    }
}

void mtc_legacy_link_status(T_BT_EVENT event_type, T_BT_EVENT_PARAM *param)
{
    switch (event_type)//(param->acl_link_status.info->status)
    {
    case BT_EVENT_ACL_CONN_SUCCESS:
        {
            if (!app_check_b2b_link(param->acl_conn_success.bd_addr))
            {
                mtc_topology_dm(MTC_TOPO_EVENT_L_LINK);
                mtc_db.device.bud_linkstatus.b2s = MTC_BUD_STATUS_ACTIVE;
                mtc_db.bredr_db.wait_a2dp = MTC_BT_MASK_NONE;
            }
            else
            {
                mtc_db.device.bud_linkstatus.b2b = MTC_BUD_STATUS_ACTIVE;
                mtc_set_pending(LE_AUDIO_NO);
            }

        }
        break;

    case BT_EVENT_ACL_CONN_FAIL:
        {

        }
        break;

    case BT_EVENT_ACL_AUTHEN_SUCCESS:
        {

        }
        break;

    case BT_EVENT_ACL_AUTHEN_FAIL:
        {

        }
        break;

    case BT_EVENT_ACL_CONN_DISCONN:
        {
            if (!app_check_b2b_link(param->acl_conn_disconn.bd_addr))
            {
                mtc_topology_dm(MTC_TOPO_EVENT_L_DIS_LINK);
            }
        }
        break;

    case BT_EVENT_ACL_ROLE_MASTER:
        {

        }
        break;

    case BT_EVENT_ACL_ROLE_SLAVE:
        {

        }
        break;

    case BT_EVENT_ACL_CONN_SNIFF:
        {
            if (app_check_b2b_link(param->acl_conn_sniff.bd_addr))
            {
                mtc_db.device.bud_linkstatus.b2b = MTC_BUD_STATUS_SNIFF;
            }
            else
            {
                mtc_db.device.bud_linkstatus.b2s = MTC_BUD_STATUS_SNIFF;
            }
        }
        break;

    case BT_EVENT_ACL_CONN_ACTIVE:
        {

            if (app_check_b2b_link(param->acl_conn_active.bd_addr))
            {
                mtc_db.device.bud_linkstatus.b2b = MTC_BUD_STATUS_ACTIVE;

                if (mtc_lea_get_pending() == LE_AUDIO_BIS)
                {
                    app_lea_trigger_mmi_handle_action(MMI_BIG_START, true);
                }
                else if (mtc_lea_get_pending() == LE_AUDIO_CIS)
                {
                    app_lea_trigger_mmi_handle_action(MMI_CIG_START, true);
                }
                mtc_set_pending(LE_AUDIO_NO);
            }
            else
            {
                mtc_db.device.bud_linkstatus.b2s = MTC_BUD_STATUS_ACTIVE;
            }
        }
        break;

    default:
        break;
    }
}
void mtc_resume_sniffing(void)
{
    T_APP_BR_LINK *p_link = NULL;

    app_disallow_legacy_stream(false);
    if (app_find_b2s_link_num() >= 2)
    {
        return;
    }
    APP_PRINT_TRACE0("mtc_resume_sniffing: start");
    p_link = app_find_b2s_link_by_index(app_get_active_a2dp_idx());
    if (app_find_br_link(p_link->bd_addr) == NULL)
    {
        APP_PRINT_TRACE0("mtc_resume_sniffing: a2dp idx ir wrong");
        return;
    }
    if (!app_bt_sniffing_start(p_link->bd_addr, BT_SNIFFING_TYPE_A2DP))
    {

    }
    //app_bt_sniffing_param_update(APP_BT_SNIFFING_EVENT_BLE_DEV_STATE_CHANGE);

}
void mtc_legacy_bt_cback(T_BT_EVENT event_type, void *event_buf, uint16_t buf_len)
{
    T_BT_EVENT_PARAM *param = event_buf;
    T_MTC_BT_MODE mode = mtc_get_btmode();
    APP_PRINT_INFO1("mtc_legacy_bt_cback: event %x", event_type);
    switch (event_type)
    {
    case BT_EVENT_ACL_CONN_SUCCESS:
    case BT_EVENT_ACL_CONN_DISCONN:
    case BT_EVENT_ACL_CONN_FAIL:
    case BT_EVENT_ACL_AUTHEN_SUCCESS:
    case BT_EVENT_ACL_AUTHEN_FAIL:
    case BT_EVENT_ACL_ROLE_MASTER:
    case BT_EVENT_ACL_ROLE_SLAVE:
    case BT_EVENT_ACL_CONN_SNIFF:
    case BT_EVENT_ACL_CONN_ACTIVE:
        {
            mtc_legacy_link_status(event_type, param);
        }
        break;

    case BT_EVENT_A2DP_STREAM_START_IND:
        {

            mtc_db.bredr_db.wait_a2dp = MTC_BT_MASK_NONE;
            if (!mode)
            {
                mtc_terminate_cis(MTC_CIS_TER_DEF);
                break;
            }

            if (mode == MULTI_PRO_BT_BIS)
            {
                mtc_resume_sniffing();
                mtc_terminate_bis(MTC_BIS_TER_DEF);
            }
            else if (mode == MULTI_PRO_BT_CIS)
            {
                app_disallow_legacy_stream(false);
                mtc_resume_sniffing();
                app_le_audio_link_sm(app_mcp_get_active_conn_handle(), LE_AUDIO_AVRCP_PLAYING, NULL);
            }

        }

        break;

    case BT_EVENT_AVRCP_PLAY_STATUS_RSP:
    case BT_EVENT_AVRCP_PLAY_STATUS_CHANGED:
        {
            T_APP_BR_LINK *p_link = NULL;

            p_link = app_find_br_link(param->avrcp_play_status_changed.bd_addr);
            if (p_link != NULL)
            {
                if (mode == MULTI_PRO_BT_BIS)
                {
                    if (param->avrcp_play_status_changed.play_status == BT_AVRCP_PLAY_STATUS_PLAYING)
                    {
                        mtc_db.bredr_db.wait_a2dp = MTC_BT_MASK_BIS;
                    }
                }
                else if (mode == MULTI_PRO_BT_CIS)
                {
                    if (param->avrcp_play_status_changed.play_status == BT_AVRCP_PLAY_STATUS_PLAYING)
                    {
                        mtc_db.bredr_db.wait_a2dp = MTC_BT_MASK_CIS;
                    }
                    else if (param->avrcp_play_status_changed.play_status == BT_AVRCP_PLAY_STATUS_STOPPED ||
                             param->avrcp_play_status_changed.play_status == BT_AVRCP_PLAY_STATUS_PAUSED)
                    {
                        app_le_audio_link_sm(app_mcp_get_active_conn_handle(), LE_AUDIO_AVRCP_PAUSE, NULL);
                    }
                }
                else if (mode == MULTI_PRO_BT_BREDR)
                {
                    if ((p_link->id == app_get_active_a2dp_idx()) &&
                        (param->avrcp_play_status_changed.play_status == BT_AVRCP_PLAY_STATUS_PLAYING))
                    {
                        mtc_db.bredr_db.wait_a2dp = MTC_BT_MASK_EDR;
                        mtc_terminate_cis(MTC_CIS_TER_SET_DISALLOW);
                    }
                }
            }
        }
        break;

    case BT_EVENT_HFP_CALL_STATUS:
        {
            app_le_audio_link_sm(app_mcp_get_active_conn_handle(), LE_AUDIO_HFP_CALL_STATE, NULL);
        }
        break;

    case BT_EVENT_REMOTE_ROLESWAP_STATUS:
        {
            if (param->remote_roleswap_status.status == BT_ROLESWAP_STATUS_SUCCESS)
            {
                app_le_audio_switch_remote_sync();
            }
        }
        break;

    case BT_EVENT_REMOTE_CONN_CMPL:
        {
            app_le_audio_switch_remote_sync();
        }
        break;

    case BT_EVENT_A2DP_STREAM_DATA_IND:
        {

            if (mtc_db.bredr_db.wait_a2dp & MTC_BT_MASK_BIS)
            {
                mtc_resume_sniffing();
                mtc_terminate_bis(MTC_BIS_TER_DEF);
            }
            else if (mtc_db.bredr_db.wait_a2dp & MTC_BT_MASK_CIS)
            {
                app_disallow_legacy_stream(false);
                mtc_resume_sniffing();
                app_le_audio_link_sm(app_mcp_get_active_conn_handle(), LE_AUDIO_AVRCP_PLAYING, NULL);
            }
            else if (mtc_db.bredr_db.wait_a2dp & MTC_BT_MASK_EDR)
            {
                mtc_terminate_cis(MTC_CIS_TER_DEF);
                if (mtc_check_sniff(param->a2dp_stream_data_ind.bd_addr))
                {
                    break;
                }
                mtc_resume_sniffing();
            }
            mtc_db.bredr_db.wait_a2dp = MTC_BT_MASK_NONE;
        }
        break;
    case BT_EVENT_SCO_CONN_CMPL://according to option,this will change
        {
            T_MTC_BT_MODE mode = mtc_get_btmode();
            if (mode == MULTI_PRO_BT_BIS)
            {
                mtc_terminate_bis(MTC_BIS_TER_DEF);
            }
            else  if (mode == MULTI_PRO_BT_CIS)
            {
                app_le_audio_mmi_handle_action(LEA_MMI_AV_PAUSE);
            }
        }
        break;
    default:

        break;
    }
}

uint8_t mtc_cis_link_num(T_MTC_CIS_NUM para)
{
    APP_PRINT_INFO2("mtc_cis_link_num: para %d, %d", para, mtc_db.device.source_cis_num);
    if (para == MTC_CIS_INCREASE)
    {
        mtc_db.device.source_cis_num++;
    }
    else if (para == MTC_CIS_DECREASE)
    {
        mtc_db.device.source_cis_num--;
    }
    return mtc_db.device.source_cis_num;
}

bool mtc_is_lea_cis_stream(void)
{
    uint8_t i;
    bool streaming = false;

    for (i = 0; i < MAX_BLE_LINK_NUM; i++)
    {
        T_APP_LE_LINK *link = app_find_le_link_by_index(i);
        if (link->used &&
            link->link_state == LE_AUDIO_LINK_STREAMING)
        {
            streaming = true;
            break;
        }
    }
    return streaming;
}

bool mtc_streaming_exist(void)
{
    uint8_t i, result = 0;
    T_APP_BR_LINK *p_link = NULL;
    T_MTC_BT_MODE cur_mode = mtc_get_btmode();
    p_link = app_find_b2s_link_by_index(app_get_active_a2dp_idx());
    if (p_link->streaming_fg)
    {
        if (cur_mode  != MULTI_PRO_BT_BREDR)
        {
            mtc_set_btmode(MULTI_PRO_BT_BREDR);
        }
        result = 1;
        goto CHECK_RESULT;
    }

    for (i = 0; i < MAX_BLE_LINK_NUM; i++)
    {
        T_APP_LE_LINK *link = app_find_le_link_by_index(i);
        if ((link->used == true) &&
            (link->link_state == LE_AUDIO_LINK_STREAMING))
        {

            if (cur_mode  != MULTI_PRO_BT_CIS)
            {
                mtc_set_btmode(MULTI_PRO_BT_CIS);
            }
            result = 2;
            goto CHECK_RESULT;
        }
    }

    if (app_lea_bis_get_state() == LE_AUDIO_BIS_STATE_STREAMING)
    {
        if (cur_mode  != MULTI_PRO_BT_BIS)
        {
            mtc_set_btmode(MULTI_PRO_BT_BIS);
        }
        result = 3;
    }
CHECK_RESULT:
    APP_PRINT_TRACE5("mtc_streaming_exist: idx %d, %d, %d, %d, %d",
                     app_get_active_a2dp_idx(), p_link->streaming_fg, app_bt_policy_get_b2b_connected(),
                     app_cfg_nv.bud_role, result);
    if (result)
    {
        return true;
    }
    else
    {
        return false;
    }
}

bool mtc_topology_dm(uint8_t event)
{
    bool ret = true;
    uint8_t pre_le_audio_switch = mtc_get_btmode();

    APP_PRINT_INFO4("mtc_topology_dm: curr mode %x, event %x, bud_role %x, %d",
                    mtc_get_btmode(), event, app_cfg_nv.bud_role, app_cfg_const.active_prio_connected_device_after_bis);

    if (remote_session_state_get() == REMOTE_SESSION_STATE_CONNECTED &&
        app_cfg_nv.bud_role == REMOTE_SESSION_ROLE_SECONDARY)
    {
        return false;
    }


    switch (event)
    {
    case MTC_TOPO_EVENT_CCP:
        {
            if (app_le_audio_get_call_status() == BT_CCP_CALL_IDLE)
            {
                if (app_hfp_get_call_status() != BT_HFP_CALL_IDLE)
                {
                    uint8_t zero_addr[6] = {0};
                    mtc_set_btmode(MULTI_PRO_BT_BREDR);
                    if (memcmp(test_bd_addr, zero_addr, 6) != 0)
                    {
                        mtc_if_to_ap(MTC_IF_TO_AP_RESUME_SCO, test_bd_addr, NULL);
                        memset(test_bd_addr, 0, 6);
                    }
                }
            }
            else
            {
                //There is error on ccp state, when voice data path is setup,
                //that means user is pick up cis voice
                mtc_set_btmode(MULTI_PRO_BT_CIS);
                mtc_terminate_hfp_call();
            }
        }
        break;

    case MTC_TOPO_EVENT_HFP:
        {
            if (app_hfp_get_call_status() == BT_HFP_CALL_IDLE)
            {
                if (app_le_audio_get_call_status() != BT_CCP_CALL_IDLE)
                {
                    mtc_set_btmode(MULTI_PRO_BT_CIS);
                }
                memset(test_bd_addr, 0, 6);
            }
            else
            {
                if (app_le_audio_get_call_status() == BT_CCP_CALL_IDLE)
                {
                    mtc_set_btmode(MULTI_PRO_BT_BREDR);
                }
            }
        }
        break;

    case MTC_TOPO_EVENT_MCP:
        {
            if (app_hfp_get_call_status() != BT_HFP_CALL_IDLE)
            {
                return false;
            }

            if (app_le_audio_get_call_status() != BT_CCP_CALL_IDLE)
            {
                return false;
            }
            mtc_set_btmode(MULTI_PRO_BT_CIS);
        }
        break;

    case MTC_TOPO_EVENT_A2DP:
        {
            if (app_hfp_get_call_status() != BT_HFP_CALL_IDLE)
            {
                return false;
            }

            if (app_le_audio_get_call_status() != BT_CCP_CALL_IDLE)
            {
                return false;
            }
            mtc_set_btmode(MULTI_PRO_BT_BREDR);
        }
        break;

    case MTC_TOPO_EVENT_C_LINK:
    case MTC_TOPO_EVENT_L_LINK:
    case MTC_TOPO_EVENT_C_DIS_LINK:
    case MTC_TOPO_EVENT_L_DIS_LINK:
    case MTC_TOPO_EVENT_BIS_STOP:
        {
            uint8_t i;
            bool edr_conn_fg = false;
            bool le_conn_fg = false;
            bool edr_h = false;
            bool prio_mode = app_cfg_const.active_prio_connected_device_after_bis;
            T_MTC_BT_MODE cur_mode = mtc_get_btmode();

            if (event == MTC_TOPO_EVENT_L_LINK)
            {
                mtc_db.device.mtc_last_bt_mode = MULTI_PRO_BT_BREDR;
            }
            else if (event == MTC_TOPO_EVENT_C_LINK)
            {
                mtc_db.device.mtc_last_bt_mode = MULTI_PRO_BT_CIS;
            }

            if (mtc_streaming_exist())
            {
                break;
            }

            if (event == MTC_TOPO_EVENT_C_LINK ||
                event == MTC_TOPO_EVENT_L_LINK ||
                event == MTC_TOPO_EVENT_C_DIS_LINK ||
                event == MTC_TOPO_EVENT_L_DIS_LINK)
            {
                prio_mode = app_cfg_const.active_prio_connected_device;
            }
            if (app_find_b2s_link_num() != 0)
            {
                edr_conn_fg = true;
            }

            for (i = 0; i < MAX_BLE_LINK_NUM; i++)
            {
                if (app_find_le_link_by_index(i)->used)
                {
                    le_conn_fg = true;
                    break;
                }
            }

            if (app_lea_bis_get_state() != LE_AUDIO_BIS_STATE_IDLE)
            {
                mtc_set_btmode(MULTI_PRO_BT_BIS);
                break;
            }

            if (prio_mode == MTC_LINK_ACT_CIS_H)
            {
                edr_h = false;
            }
            else  if (prio_mode == MTC_LINK_ACT_LEGACY_H)
            {
                edr_h = true;
            }
            else  if (prio_mode == MTC_LINK_ACT_LAST_ONE)
            {
                if (mtc_db.device.mtc_last_bt_mode == MULTI_PRO_BT_BREDR)
                {
                    edr_h = true;
                }
                else if (mtc_db.device.mtc_last_bt_mode == MULTI_PRO_BT_CIS)
                {
                    edr_h = false;
                }
            }
            if (edr_h)
            {
                if (edr_conn_fg)
                {
                    mtc_set_btmode(MULTI_PRO_BT_BREDR);
                }
                else if (le_conn_fg)
                {
                    mtc_set_btmode(MULTI_PRO_BT_CIS);
                }
            }
            else
            {
                if (le_conn_fg)
                {
                    mtc_set_btmode(MULTI_PRO_BT_CIS);
                }
                else if (edr_conn_fg)
                {
                    mtc_set_btmode(MULTI_PRO_BT_BREDR);
                }
            }
            APP_PRINT_INFO2("mtc_topology_dm: le_conn_fg %d, edr_conn_fg %d",
                            le_conn_fg,
                            edr_conn_fg);
        }
        break;

    case MTC_TOPO_EVENT_BIS:
        {
            mtc_set_btmode(MULTI_PRO_BT_BIS);
        }
        break;
    default:
        ret = false;
        break;
    }

    if (pre_le_audio_switch != mtc_get_btmode())
    {
        if (remote_session_state_get() == REMOTE_SESSION_STATE_CONNECTED)
        {
            uint8_t lea_key_type = mtc_get_btmode();
            app_relay_async_single_with_raw_msg(APP_MODULE_TYPE_ISOAUDIO,
                                                LEA_REMOTE_MMI_SWITCH_SYNC,
                                                (uint8_t *)&lea_key_type, sizeof(lea_key_type));
        }
    }
    APP_PRINT_INFO2("mtc_topology_dm: mode change %x to %x", pre_le_audio_switch,
                    mtc_get_btmode());
    return ret;
}


void mtc_relay_cback(uint16_t event, T_REMOTE_RELAY_STATUS status, void *buf,
                     uint16_t len)
{
    AUDIO_PRINT_TRACE2("mtc_relay_cback: event 0x%04x, status %u", event, status);

    switch (event)
    {
    case MTC_REMOTE_SYNC_BIS_STOP:
        {
            if (status == REMOTE_RELAY_STATUS_SYNC_TOUT ||
                status == REMOTE_RELAY_STATUS_SYNC_RCVD ||
                status == REMOTE_RELAY_STATUS_SYNC_SENT_OUT)
            {

                mtc_set_beep((T_MTC_PRO_BEEP) * ((uint8_t *)buf));
                app_mmi_handle_action(MMI_BIG_STOP);
            }
        }
        break;

    case MTC_REMOTE_SYNC_CIS_STOP:
        {
            if (status == REMOTE_RELAY_STATUS_SYNC_TOUT ||
                status == REMOTE_RELAY_STATUS_SYNC_RCVD ||
                status == REMOTE_RELAY_STATUS_SYNC_SENT_OUT)
            {
                app_le_audio_device_sm(LE_AUDIO_ADV_STOP, NULL);
            }
        }
        break;

    default:
        break;
    }
}

/*============================================================================*
 *                              Interface
 *============================================================================*/
T_MTC_RESULT mtc_if_routine(T_MTC_IF_INFO *para)
{
    if (mtc_if_table[para->if_index])
    {
        mtc_if_table[para->if_index](para->msg, para->inbuf, para->outbuf);
        return MTC_RESULT_SUCCESS;
    }
    else
    {
        return MTC_RESULT_NOT_REGISTER;
    }
}

T_MTC_RESULT mtc_if_routine_reg(uint8_t if_index, P_MTC_IF_CB para)
{
    T_MTC_RESULT result = MTC_RESULT_SUCCESS;
    if (mtc_if_table[if_index] != NULL)
    {
        result = MTC_RESULT_NOT_RELEASE;
    }

    mtc_if_table[if_index] = para;
    return result;
}
T_MTC_RESULT mtc_if_fm_ap_handle(T_MTC_IF_MSG msg, void *inbuf, void *outbuf)
{
    T_MTC_RESULT mtc_result = MTC_RESULT_SUCCESS;
    //APP_PRINT_INFO1("mtc_if_fm_ap_handle: msg %x", msg);
    switch (msg)
    {
    case MTC_IF_FROM_AP_SCO_CMPL:
        {
            if (outbuf == NULL || outbuf == NULL)
            {
                mtc_result = MTC_RESULT_REJECT;
                break;
            }

            // memcpy(test_bd_addr, (uint8_t *)inbuf, 6);
            //APP_PRINT_TRACE1("MTC_IF_FROM_AP_SCO_CMPL: test_bd_addr %s", TRACE_BDADDR(test_bd_addr));
            memset(test_bd_addr, 0, 6);
            if (app_le_audio_get_call_status() != BT_CCP_CALL_IDLE)
            {
                mtc_terminate_hfp_call();
                *((uint8_t *)outbuf) = true;
            }
            else
            {
                *((uint8_t *)outbuf) = false;
            }
        }
        break;

    default:
        break;
    }
    return mtc_result;
}
T_MTC_RESULT mtc_if_fm_ap(T_MTC_IF_MSG msg, void *inbuf, void *outbuf)
{
    T_MTC_IF_INFO fm_ap_info;
    fm_ap_info.if_index = MTC_IF_FM_AUDIO_POLICY;
    fm_ap_info.msg = msg;
    fm_ap_info.inbuf = inbuf;
    fm_ap_info.outbuf = outbuf;

    return mtc_if_routine(&fm_ap_info);
}

T_MTC_RESULT mtc_if_to_ap(T_MTC_IF_MSG msg, void *inbuf, void *outbuf)
{
    T_MTC_IF_INFO fm_ap_info;
    fm_ap_info.if_index = MTC_IF_TO_AUDIO_POLICY;
    fm_ap_info.msg = msg;
    fm_ap_info.inbuf = inbuf;
    fm_ap_info.outbuf = outbuf;
    return mtc_if_routine(&fm_ap_info);
}

T_MTC_RESULT mtc_if_ap_reg(P_MTC_IF_CB para)
{
    T_MTC_RESULT result = MTC_RESULT_SUCCESS;
    //From audio policy
    result =  mtc_if_routine_reg(MTC_IF_FM_AUDIO_POLICY, mtc_if_fm_ap_handle);
    //TO audio policy
    result =  mtc_if_routine_reg(MTC_IF_TO_AUDIO_POLICY, para);
    return result;
}

void mtc_init(void)
{
    mtc_db.device.max_conn_num = MTC_MAX_LINK_NUM;
    bt_mgr_cback_register(mtc_legacy_bt_cback);
    p_audio_policy_relay_pase_cb = mtc_audio_policy_cback;
    mtc_relay_handle = remote_relay_register(mtc_relay_cback);
    mct_gap_timer_init();
}

