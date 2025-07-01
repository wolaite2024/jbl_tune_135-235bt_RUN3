/**
*****************************************************************************************
*     Copyright(c) 2017, Realtek Semiconductor Corporation. All rights reserved.
*****************************************************************************************
  * @file     mcs_client.h
  * @brief    Head file for Media Control Service.
  * @details  mcs data structs and external functions declaration.
  * @author   cheng_cai
  * @date
  * @version  v1.0
  * *************************************************************************************
  */

/* Define to prevent recursive inclusion */
#ifndef _MCS_CLIENT_H_
#define _MCS_CLIENT_H_

#ifdef  __cplusplus
extern "C" {
#endif      /* __cplusplus */

#include "mcs_def.h"

#if LE_AUDIO_MCS_CLIENT_SUPPORT
#define MCS_MPN_CCCD_FLAG                       0x0001
#define MCS_TK_CHG_CCCD_FLAG                    0x0002
#define MCS_TK_TITLE_CCCD_FLAG                  0x0004
#define MCS_TK_DUR_CCCD_FLAG                    0x0008
#define MCS_TK_POS_CCCD_FLAG                    0x0010
#define MCS_PY_SPEED_CCCD_FLAG                  0x0020
#define MCS_SK_SPEED_CCCD_FLAG                  0x0040
#define MCS_CUR_TK_OBJID_CCCD_FLAG              0x0080
#define MCS_NEXT_TK_OBJID_CCCD_FLAG             0x0100
#define MCS_PARENT_GRP_OBJID_CCCD_FLAG          0x0200
#define MCS_CUR_GRP_OBJID_CCCD_FLAG             0x0400
#define MCS_PY_ORDER_CCCD_FLAG                  0x0800
#define MCS_MEDIA_ST_CCCD_FLAG                  0x1000
#define MCS_MEDIA_CTL_POINT_CCCD_FLAG           0x2000
#define MCS_MEDIA_CTLPOINT_OPCODE_SUP_CCCD_FLAG 0x4000
#define MCS_SEARCH_CTL_POINT_CCCD_FLAG          0x8000
#define MCS_SEARCH_RES_OBJID_CCCD_FLAG          0x10000


typedef struct
{
    uint8_t  *player_name;     //UTF-8 string
    uint16_t  name_len;
} T_CL_MPN_CB;

typedef struct
{
    uint8_t  *p_icon_url;     //UTF-8 string
    uint16_t  url_len;
} T_CL_MPIURL_CB;

typedef struct
{
    uint8_t  *title_name;     //UTF-8 string
    uint16_t  title_len;
} T_CL_TKTL_CB;


typedef struct
{
    uint16_t conn_handle;
    bool    general_mcs;
    bool    is_found;
    bool    load_form_ftl;
    uint8_t srv_num;
} T_MCS_CLIENT_DIS_DONE;

typedef struct
{
    uint16_t    conn_handle;
    bool        general_mcs;
    uint8_t     srv_idx;
    uint16_t    uuid;
    uint16_t    cause;
    union
    {
        T_CL_MPN_CB       media_player_name;
        T_CL_MPIURL_CB    media_player_icon_url;
        T_CL_TKTL_CB      track_title;
        int32_t           track_duration;
        int               track_position;
        int8_t            playback_speed;
        int8_t            seeking_speed;
        uint8_t           playing_order;
        uint8_t           media_state;

        uint64_t          read_obj_id;
        uint64_t          search_res_obj_id;
    } data;
} T_MCS_CLIENT_READ_RES;

typedef struct
{
    uint16_t    conn_handle;
    bool        general_mcs;
    uint8_t     srv_idx;
    uint16_t    cause;

    uint64_t    cur_tk_obj_id;
    uint64_t    cur_tk_seg_obj_id;
    uint64_t    next_tk_obj_id;
    uint64_t    cur_gp_obj_id;
    uint64_t    paraent_gp_obj_id;
} T_MCS_CLIENT_READ_OBJ_IDS;

typedef struct
{
    uint8_t     opcode;
    void        *p_mcp_cb;
    union
    {
        int32_t   mv_rel_val;
        int32_t   goto_seg_val;
        int32_t   goto_tk_val;
        int32_t   goto_gp_val;
    } param;
} T_MCS_CLIENT_MCP_PARAM;

typedef struct
{
    uint8_t     type;
    uint8_t     *p_value;
    uint16_t    value_len;
} T_MCS_CLIENT_SCP_PARAM;

typedef struct
{
    uint16_t    conn_handle;
    bool        general_mcs;
    uint8_t     srv_idx;
    uint16_t    uuid;
    union
    {
        T_CL_MPN_CB       media_player_name;
        T_CL_MPIURL_CB    media_player_icon_url;
        T_CL_TKTL_CB      track_title;
        int32_t           track_duration;
        int               track_position;
        int8_t            playback_speed;
        int8_t            seeking_speed;
        uint8_t           playing_order;
        uint8_t           media_state;

        uint64_t          cur_tk_obj_id;
        uint64_t          next_tk_obj_id;
        uint64_t          cur_gp_obj_id;
        uint64_t          paraent_gp_obj_id;
        uint64_t          search_res_obj_id;
    } data;
} T_MCS_CLIENT_NOTFY_RES;

typedef struct
{
    uint16_t    conn_handle;
    bool        general_mcs;
    bool        is_success;
    uint8_t     srv_idx;
    uint64_t    obj_id;
    uint16_t    uuid;

    uint8_t     *p_value;
    uint16_t    len;
} T_MCS_CLIENT_READ_OBJ_RES;



typedef void (*P_MCS_MCP_CB)(uint8_t opcode, uint8_t result);
typedef void (*P_MCS_SCP_CB)(uint8_t result);

bool mcs_set_seeking_speed(uint16_t conn_handle, uint8_t srv_idx, bool general,
                           int8_t seeking_speed);
bool mcs_set_playback_speed(uint16_t conn_handle, uint8_t srv_idx, bool is_cmd, bool general,
                            int8_t playback_speed);
bool mcs_set_playing_order(uint16_t conn_handle, uint8_t srv_idx, bool is_cmd, bool general,
                           uint8_t order);
bool mcs_set_abs_track_pos(uint16_t conn_handle, uint8_t srv_idx, bool is_cmd, bool general,
                           int32_t pos);
bool mcs_cfg_cccd(uint16_t conn_handle, uint32_t cfg_flags, bool general, uint8_t srv_idx,
                  bool enable);
bool mcs_read_obj_value(uint16_t conn_handle, uint8_t srv_idx, bool general, uint16_t uuid,
                        uint64_t obj_id);
bool mcs_client_init(void);
bool mcs_read_char_value(uint16_t conn_handle, uint8_t srv_idx, uint16_t uuid, bool general);
bool mcs_send_mcp_op(uint16_t conn_handle, uint8_t srv_idx, bool general,
                     T_MCS_CLIENT_MCP_PARAM *p_param);
bool mcs_send_mcp_op_no_check(uint16_t conn_handle, uint8_t srv_idx, bool general,
                              T_MCS_CLIENT_MCP_PARAM *p_param);
bool mcs_read_relevant_tk_obj_id(uint16_t conn_handle, uint8_t srv_idx, bool general);
bool mcs_send_scp_op(uint16_t conn_handle, uint8_t srv_idx, bool general,
                     T_MCS_CLIENT_SCP_PARAM *p_param, uint8_t param_num,
                     P_MCS_SCP_CB p_scp_cb);
bool mcs_set_cur_tk_obj_id(uint16_t conn_handle, uint8_t srv_idx, bool is_cmd,
                           bool general, uint64_t obj_id);
bool mcs_set_next_tk_obj_id(uint16_t conn_handle, uint8_t srv_idx, bool is_cmd,
                            bool general, uint64_t obj_id);
bool mcs_set_cur_gp_obj_id(uint16_t conn_handle, uint8_t srv_idx, bool is_cmd,
                           bool general, uint64_t obj_id);
uint8_t mcs_get_ots_idx(uint16_t conn_handle, uint8_t srv_idx, bool general);

#endif

#ifdef  __cplusplus
}
#endif      /*  __cplusplus */

#endif  /* _MCS_CLIENT_H_ */
