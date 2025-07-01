#ifndef _CCP_CLIENT_H_
#define _CCP_CLIENT_H_

#ifdef  __cplusplus
extern "C" {
#endif      /* __cplusplus */

#include "tbs_def.h"
#include "profile_client.h"

#if LE_AUDIO_TBS_CLIENT_SUPPORT
#define TBS_BPN_NOTIFY_CCCD_FLAG                      0x0001
#define TBS_TECH_NOTIFY_CCCD_FLAG                     0x0002
#define TBS_URI_SCHEM_SUP_LIST_NOTIFY_CCCD_FLAG       0x0004
#define TBS_SIG_STREN_NOTIFY_CCCD_FLAG                0x0008
#define TBS_LIST_CUR_CALL_NOTIFY_CCCD_FLAG            0x0010
#define TBS_STATUS_FLAGS_NOTIFY_CCCD_FLAG             0x0020
#define TBS_IN_CALL_TG_URI_CCCD_FLAG                  0x0040
#define TBS_CALL_STATE_NOTIFY_CCCD_FLAG               0x0080
#define TBS_CCP_CCCD_FLAG                             0x0100
#define TBS_TERM_REASON_NOTIFY_CCCD_FLAG              0x0200
#define TBS_INCOMING_CALL_NOTIFY_CCCD_FLAG            0x0400
#define TBS_CALL_FIR_NAME_NOTIFY_CCCD_FLAG            0x0800

typedef struct
{
    uint16_t    conn_handle;
    bool        general;
    bool        is_found;
    bool        load_form_ftl;
    uint8_t     srv_num;
} T_CCP_CLIENT_DIS_DONE;

typedef struct
{
    bool      general;
    uint16_t  conn_handle;
    uint16_t  cause;
} T_CCP_CFG_CCCD_RESULT;

typedef struct
{
    uint8_t     *p_call_state;
    uint16_t    len;
} T_TBS_CALL_STATE_CB;

typedef struct
{
    uint8_t    req_opcode;
    uint8_t    call_idx;
    uint8_t    res_code;
} T_TBS_CCP_OP_RESULT;

typedef struct
{
    uint16_t          conn_handle;
    uint8_t           srv_id;
    uint16_t          cause;
    bool              general;

    uint16_t          uuid;
    union
    {
        uint16_t                status_flags;
        uint16_t                ccp_opt_opcodes;
        T_TBS_BPN               bpn;
        T_TBS_UCI               uci;
        T_TBS_TECH              tech;
        T_TBS_URI_SUP_LIST      uri_sup_list;
        T_TBS_SIG_STREN         sig_param;
        T_TBS_CUR_CALL_LIST     cur_call_list;
        T_TBS_CALL_STATE_CB     call_state;
        T_TBS_IN_CALL_TG_URI    in_call_tg_uri;
        T_TBS_INCOMING_CALL     in_call;
        T_TBS_CALL_FRI_NAME     call_fri_name;
    } value;
} T_CCP_READ_RESULT;

typedef struct
{
    uint16_t            conn_handle;
    uint16_t            uuid;
    bool                general;
    union
    {
        uint16_t                status_flags;
        uint16_t                ccp_opt_opcodes;
        T_TBS_BPN               bpn;
        T_TBS_UCI               uci;
        T_TBS_TECH              tech;
        T_TBS_URI_SUP_LIST      uri_sup_list;
        T_TBS_SIG_STREN         sig_param;
        T_TBS_CUR_CALL_LIST     cur_call_list;
        T_TBS_CALL_STATE_CB     call_state;
        T_TBS_IN_CALL_TG_URI    in_call_tg_uri;
        T_TBS_CCP_OP_RESULT     ccp_op_result;
        T_TBS_TERM_REASON       term_reason;
        T_TBS_INCOMING_CALL     in_call;
        T_TBS_CALL_FRI_NAME     call_fri_name;
    } value;
} T_CCP_NOTIFY_DATA;

bool ccp_cfg_cccd(uint16_t conn_handle, uint8_t srv_idx, uint16_t cfg_flags, bool enable,
                  bool general);
bool ccp_read_tbs_char(uint16_t conn_handle,  uint8_t srv_idx, uint16_t uuid, bool general);

bool ccp_write_sig_rpt_interval(uint16_t conn_handle,  uint8_t srv_idx, uint8_t interval,
                                bool is_cmd,
                                bool general);
bool ccp_send_accept_operation(uint16_t conn_handle, uint8_t srv_idx, uint8_t call_idx, bool is_cmd,
                               bool general);
bool ccp_send_terminate_operation(uint16_t conn_handle, uint8_t srv_idx, uint8_t call_idx,
                                  bool is_cmd,
                                  bool general);
bool ccp_send_loc_hold_operation(uint16_t conn_handle, uint8_t srv_idx, uint8_t call_idx,
                                 bool is_cmd,
                                 bool general);
bool ccp_send_loc_retrieve_operation(uint16_t conn_handle, uint8_t srv_idx, uint8_t call_idx,
                                     bool is_cmd,
                                     bool general);
bool ccp_send_originate_operation(uint16_t conn_handle, uint8_t srv_idx,
                                  uint8_t *p_uri, uint16_t len, bool is_cmd, bool general);
bool ccp_send_join_operation(uint16_t conn_handle, uint8_t srv_idx,
                             uint8_t *p_call_list, uint16_t len, bool is_cmd, bool general);
bool ccp_client_init(void);

#endif
#ifdef  __cplusplus
}
#endif      /*  __cplusplus */

#endif
