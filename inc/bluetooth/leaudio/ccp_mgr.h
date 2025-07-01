#ifndef _CCP_MGR_H_
#define _CCP_MGR_H_

#ifdef  __cplusplus
extern "C" {
#endif      /* __cplusplus */

#include "tbs_def.h"
#if LE_AUDIO_TBS_SUPPORT
typedef struct
{
    uint16_t     uuid;
    union
    {
        uint16_t                status_flags;
        uint16_t                ccp_opt_opcodes;
        T_TBS_BPN               bpn;
        T_TBS_UCI               uci;
        T_TBS_TECH              tech;
        T_TBS_URI_SUP_LIST      uri_sup_list;
        T_TBS_SIG_STREN         sig_param;
        T_TBS_IN_CALL_TG_URI    in_call_tg_uri;
        T_TBS_INCOMING_CALL     in_call;
        T_TBS_CALL_FRI_NAME     call_fri_name;
    } param;
} T_CCP_SET_PARAM;

typedef struct
{
    uint8_t     *p_uri;
    uint16_t    uri_len;
} T_CCP_URI;

typedef struct
{
    uint8_t     *p_call_list;
    uint16_t    call_list_len;
} T_CCP_CALL_LIST;

typedef union
{
    uint8_t           call_idx;   //if the opcode is originate or join, the call_idx is alloc by app
    T_CCP_URI         uri_cb;
    T_CCP_CALL_LIST   call_list_cb;
} T_TBS_CCP_DATA;

typedef struct
{
    uint16_t        conn_handle;
    uint8_t         srv_id;
    uint8_t         opcode;
    T_TBS_CCP_DATA  ccp_data;
} T_CCP_OP_VAL;

typedef struct
{
    uint16_t        conn_handle;
    uint8_t         srv_id;
    uint8_t         opcode;
    uint8_t         call_idx;
    uint8_t         result;
} T_CCP_OP_RES;


typedef struct
{
    uint16_t conn_handle;
    uint8_t srv_id;
    uint8_t rpt_int;
} T_CCP_WRITE_RPT_INT;

typedef struct
{
    uint16_t    conn_handle;
    uint8_t     srv_id;
    uint16_t    uuid;
    uint16_t    offset;
} T_CCP_READ_REQ;

uint8_t ccp_get_ccid(uint8_t srv_id);
bool ccp_set_param(uint8_t srv_id, T_CCP_SET_PARAM *p_param);
void ccp_update_call_state_by_idx(uint8_t srv_id, uint8_t call_idx, uint8_t state,
                                  uint8_t call_flags, bool update_notify);
void ccp_update_call_list_and_state(uint8_t srv_id);
void ccp_term_call(uint8_t srv_id, T_TBS_TERM_REASON *p_reason);
void ccp_handle_op_result(T_CCP_OP_RES res);
T_SERVER_ID ccp_reg_special_tbs(uint8_t feature);
uint8_t ccp_alloc_new_call(uint8_t srv_id, T_TBS_CALL_TYPE call_type, uint8_t *p_call_uri,
                           uint16_t len);
T_SERVER_ID ccp_init(uint8_t tbs_num, uint8_t general_feature);

#endif

#ifdef  __cplusplus
}
#endif      /*  __cplusplus */

#endif
