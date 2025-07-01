/*
 * Copyright (c) 2018, Realsil Semiconductor Corporation. All rights reserved.
 */

#ifndef _MULTIPRO_CTRLOR_H_
#define _MULTIPRO_CTRLOR_H_


#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */
#include "multitopology_if.h"
/*============================================================================*
 *                              Constants
 *============================================================================*/
typedef enum
{
    MTC_CIS_GET   = 0,
    MTC_CIS_INCREASE,
    MTC_CIS_DECREASE,
} T_MTC_CIS_NUM;

typedef enum
{
    MTC_CIS_TMR_PAIRING   = 0,
    MTC_CIS_TMR_LINKBACK,
    MTC_CIS_TMR_LINKLOSS_BACK,
    MTC_CIS_TMR_MAX,
} T_MTC_CIS_TMR;

typedef enum
{
    MTC_BIS_TMR_SCAN,
    MTC_BIS_TMR_RESYNC,
    MTC_BIS_TMR_DELEGATOR,
    MTC_BIS_TMR_MAX,
} T_MTC_BIS_TMR;

typedef enum
{
    T_MTC_OTHER_TMR_LINKBACK_TOTAL,
    T_MTC_OTHER_TMR_MAX,
} T_MTC_OTHER_TMR;

typedef enum
{
    MTC_TMR_CIS      = 0x00,
    MTC_TMR_BIS,
    MTC_TMR_OTHER,

    MTC_TMR_MAX,
} T_MTC_TMR;

typedef enum
{
    LE_AUDIO_NO    = 0x00,
    LE_AUDIO_CIS     = 0x01,
    LE_AUDIO_BIS   = 0x02,
    LE_AUDIO_ALL   = 0x03,
} T_MTC_AUDIO_MODE;

typedef enum t_mtc_pro_beep
{
    MTC_PRO_BEEP_NONE,
    MTC_PRO_BEEP_PROMPTLY,
    MTC_PRO_BEEP_DELAY,
} T_MTC_PRO_BEEP;

typedef enum t_multi_pro_key
{
    MULTI_PRO_BT_BREDR,
    MULTI_PRO_BT_CIS,
    MULTI_PRO_BT_BIS,
} T_MTC_BT_MODE;

typedef enum t_multi_pro_sniffing_ststus
{
    MULTI_PRO_SNIFI_NOINVO = 0x0,
    MULTI_PRO_SNIFI_INVO,
} T_MTC_SNIFI_STATUS;

typedef enum
{
    MTC_BUD_STATUS_NONE      = 0x00,
    MTC_BUD_STATUS_SNIFF      = 0x01,
    MTC_BUD_STATUS_ACTIVE      = 0x02,

    MTC_BUD_STATUS_TOTAL,
} T_MTC_BUD_STATUS;

typedef enum
{
    MULTI_PRO_REMOTE_RELAY_STATUS_ASYNC_RCVD          = 0x00,
    MULTI_PRO_REMOTE_RELAY_STATUS_ASYNC_LOOPBACK      = 0x01,
    MULTI_PRO_REMOTE_RELAY_STATUS_SYNC_RCVD           = 0x02,
    MULTI_PRO_REMOTE_RELAY_STATUS_SYNC_TOUT           = 0x03,
    MULTI_PRO_REMOTE_RELAY_STATUS_SYNC_EXPIRED        = 0x04,
    MULTI_PRO_REMOTE_RELAY_STATUS_SYNC_LOOPBACK       = 0x06,
    MULTI_PRO_REMOTE_RELAY_STATUS_SYNC_REF_CHANGED    = 0x07,
    MULTI_PRO_REMOTE_RELAY_STATUS_SYNC_SENT_OUT       = 0x08,
    MULTI_PRO_REMOTE_RELAY_STATUS_ASYNC_SENT_OUT      = 0x09,
    MULTI_PRO_REMOTE_RELAY_STATUS_SEND_FAILED         = 0x0A,
} T_MULTI_PRO_REMOTE_RELAY_STATUS;

typedef enum
{
    MTC_GAP_VENDOR_NONE    = 0x00,
    MTC_GAP_VENDOR_INIT    = 0x01,
    MTC_GAP_VENDOR_ADV,
    MTC_GAP_VENDOR_TOTAL,

} T_MTC_GAP_VENDOR_MODE;

typedef enum
{
    MTC_EVENT_BIS_SCAN_TO                       = 0x00,
    MTC_EVENT_BIS_RESYNC_TO,
    MTC_EVENT_BIS_DELEGATOR_TO,
    MTC_EVENT_CIS_PAIRING_TO,
    MTC_EVENT_CIS_LINKBACK_TO,
    MTC_EVENT_CIS_LOSS_LINKBACK_TO,
    MTC_EVENT_LEGACY_PAIRING_TO,
    MTC_EVENT_MAX,
} T_MTC_EVENT;

typedef enum
{
    MTC_TOPO_EVENT_A2DP       = 0x00,
    MTC_TOPO_EVENT_MCP        = 0x01,
    MTC_TOPO_EVENT_HFP        = 0x02,
    MTC_TOPO_EVENT_CCP        = 0x03,
    MTC_TOPO_EVENT_C_LINK     = 0x04,
    MTC_TOPO_EVENT_C_DIS_LINK     = 0x05,
    MTC_TOPO_EVENT_L_LINK     = 0x06,
    MTC_TOPO_EVENT_L_DIS_LINK     = 0x07,
    MTC_TOPO_EVENT_BIS        = 0x08,
    MTC_TOPO_EVENT_BIS_STOP = 0x09,

    MTC_TOPO_EVENT_TOTAL,
} T_MTC_TOPO_EVENT;

typedef enum
{
    MTC_LINKBACK_SAME         = 0x00,
    MTC_LINKBACK_CIS_H,
    MTC_LINKBACK_LEGACY_H,
    MTC_LINKBACK_TOTAL,
} T_MTC_LINKBACK_PRIO;

typedef enum
{
    MTC_LINK_ACT_CIS_H     = 0x00,
    MTC_LINK_ACT_LEGACY_H,
    MTC_LINK_ACT_LAST_ONE,
    MTC_LINK_ACT_NONE,
    MTC_LINK_ACTIVE_TOTAL,

} T_MTC_LINK_ACT;

typedef enum
{
    MTC_REMOTE_NO                   = 0x0000,
    MTC_REMOTE_SYNC_BIS_STOP        = 0x0001,
    MTC_REMOTE_SYNC_CIS_STOP        = 0x0002,
} T_MTC_REMOTE_MSG;


typedef struct
{
    uint8_t msg_type;
    uint8_t *buf;
    uint16_t len;
    T_MULTI_PRO_REMOTE_RELAY_STATUS status;
} T_RELAY_PARSE_PARA;


/*============================================================================*
 *                              Functions
 *============================================================================*/
typedef void (*P_MTC_RELAY_PARSE_CB)(uint8_t msg_type, uint8_t *buf, uint16_t len,
                                     T_MULTI_PRO_REMOTE_RELAY_STATUS status);
typedef void *MTC_GAP_HANDLER;

void mtc_start_timer(T_MTC_TMR tmt, uint8_t opt, uint32_t timeout);
void mtc_stop_timer(T_MTC_TMR tmt);
bool mtc_exist_handler(T_MTC_TMR tmt);


T_MTC_BUD_STATUS mtc_get_b2d_sniff_status(void);
T_MTC_BUD_STATUS mtc_get_b2s_sniff_status(void);
void mtc_set_pending(T_MTC_AUDIO_MODE action);
uint8_t mtc_cis_link_num(T_MTC_CIS_NUM para);
void mtc_resume_a2dp(uint8_t app_idx);
void mtc_set_resume_a2dp_idx(uint8_t index);
bool mtc_stream_switch(void);
uint8_t mtc_get_resume_a2dp_idx(void);
void mtc_pro_hook(uint8_t hook_point, T_RELAY_PARSE_PARA *info);
void mtc_set_beep(T_MTC_PRO_BEEP para);
T_MTC_PRO_BEEP mtc_get_beep(void);
void mtc_set_btmode(T_MTC_BT_MODE para);
T_MTC_BT_MODE mtc_get_btmode(void);
T_MTC_SNIFI_STATUS mtc_get_sniffing(void);
void mtc_legacy_update_call_status(void);
bool mtc_is_lea_cis_stream(void);
bool mtc_device_poweroff_check(T_MTC_EVENT event);
bool mtc_topology_dm(uint8_t event);

uint8_t mtc_gap_set_pri(T_MTC_GAP_VENDOR_MODE para);
uint8_t mtc_gap_callback(uint8_t cb_type, void *p_cb_data);
void mtc_gap_handle_state_evt_callback(uint8_t new_state, uint16_t cause);
bool mtc_gap_is_ready(void);

/*============================================================================*
 *                              Interface
 *============================================================================*/
typedef T_MTC_RESULT(*P_MTC_IF_CB)(T_MTC_IF_MSG msg, void *inbuf, void *outbuf);
T_MTC_RESULT mtc_if_fm_ap(T_MTC_IF_MSG msg, void *inbuf, void *outbuf);
T_MTC_RESULT mtc_if_ap_reg(P_MTC_IF_CB para);
void mtc_init(void);
#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
