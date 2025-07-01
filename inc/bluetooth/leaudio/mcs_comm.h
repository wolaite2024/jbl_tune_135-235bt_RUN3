/**
*****************************************************************************************
*     Copyright(c) 2017, Realtek Semiconductor Corporation. All rights reserved.
*****************************************************************************************
  * @file     mcs.h
  * @brief    Head file for Media Control Service.
  * @details  mcs data structs and external functions declaration.
  * @author   cheng_cai
  * @date
  * @version  v1.0
  * *************************************************************************************
  */

/* Define to prevent recursive inclusion */
#ifndef _MCS_COMM_H_
#define _MCS_COMM_H_

#ifdef  __cplusplus
extern "C" {
#endif      /* __cplusplus */

/* Add Includes here */
#include "profile_server_def.h"
#include "ots_comm.h"
#include "mcs_def.h"

#define MEDIA_PLAYER_ICON_OBJID_CHAR_MSK        0x0001
#define MEDIA_PLAYER_ICON_URL_CHAR_MSK          0x0002
#define PLAYBACK_SPEED_CHAR_MSK                 0x0004
#define SEEKING_SPEED_CHAR_MSK                  0x0008
#define CUR_TRACK_OBJID_CHAR_MSK                0x0010
#define PLAYING_ORDER_CHAR_MSK                  0x0020
#define PLAYING_ORDER_SUP_CHAR_MSK              0x0040
#define MEDIA_CONTROL_POINT_CHAR_MSK            0x0080
#define SEARCH_CONTROL_POINT_CHAR_MSK           0x0100
#define OTS_INCLUDE_MSK                         0x0200

#define MEDIA_PLAYER_NAME_NOTIFY_MSK            0x0001
#define TRACK_TITLE_NOTIFY_MSK                  0x0002
#define TRACK_DURATION_NOTIFY_MSK               0x0004
#define TRACK_POSITION_NOTIFY_MSK               0x0008
#define PLAYBACK_SPEED_NOTIFY_MSK               0x0010
#define SEEKING_SPEED_NOTIFY_MSK                0x0020
#define CUR_TRACK_OBJID_NOTIFY_MSK              0x0040
#define NEXT_TRACK_OBJID_NOTIFY_MSK             0x0080
#define PARENT_GRP_OBJID_NOTIFY_MSK             0x0100
#define CUR_GRP_OBJID_NOTIFY_MSK                0x0200
#define PLAYING_ORDER_NOTIFY_MSK                0x0400
#define MEDIA_CTPOINT_OPCODE_SUP_NOTIFY_MSK     0x0800

#define MCS_MAX_ICON_URL_LEN        80

#define MCS_ALL_PLAYING_ORDER_SUP       0x3FF
#define MCS_ALL_MEDIA_CTLP_OP_SUP       0x1FFFFF
#define MCS_INVALID_TRACK_DURATION      0xFFFFFFFF


typedef struct
{
    uint8_t *p_search_param;
    uint16_t search_param_len;
} T_MCS_SCP_REQ;

typedef struct
{
    uint16_t  conn_handle;
    uint8_t   srv_idx;
    uint16_t  uuid;
    uint16_t  attrib_index;
    uint16_t  offset;
} T_MCS_READ_REQ;

typedef struct
{
    uint16_t  conn_handle;
    uint8_t   srv_id;
    uint16_t  uuid;
    union
    {
        int                 pos;
        int8_t              playback_speed;
        uint8_t             playing_order;
        uint64_t            obj_id;
        T_MCS_SCP_REQ       search_req_cb;
    } value;
} T_MCS_WRITE_REQ;

typedef struct
{
    uint16_t  conn_handle;
    uint8_t   srv_id;
    uint8_t   opcode;
    union
    {
        int     comm;
        int     relative_offset;
        int     seg_num;
        int     tk_num;
        int     gp_num;
    } value;
} T_MCS_MCP_REQ;


typedef struct
{
    uint8_t       srv_id;
    uint64_t      search_obj_id;
    T_MCS_GP_OBJ  *p_value;
    uint8_t       num;
} T_MCS_SCP_RES;

typedef struct
{
    uint8_t      seg_name_len;
    uint8_t       *p_seg_name;
    int           seg_pos;
} T_MCS_TR_SEG;

typedef struct
{
    uint8_t       *p_tk_name;
    uint16_t      tk_name_len;
    uint8_t       *p_artist_name;
    uint16_t      artist_name_len;
    uint8_t       *p_alb_name;
    uint16_t      alb_name_len;
    uint8_t       *p_elst_year;
    uint16_t      elst_year_len;
    uint8_t       *p_latest_year;
    uint16_t      latest_year_len;
    uint8_t       *p_genre;
    uint16_t      genre_len;

    uint8_t       tk_seg_num;
    T_MCS_TR_SEG  *p_tk_seg;
} T_MCS_TK_ELEM;

typedef struct
{
    uint16_t      gp_name_len;
    uint8_t       *p_gp_name;
    uint8_t       *p_artist_name;
    uint16_t      artist_name_len;

    uint16_t      gp_value_len;
    uint8_t       *p_gp_value;
} T_MCS_GP_ELEM;

typedef struct
{
    uint8_t       *p_title;
    uint16_t      title_len;

    int           tk_duration;
    int           tk_pos;
    uint64_t      tk_obj_id;
    uint64_t      next_tk_obj_id;
} T_MCS_TR_INFO;

typedef struct
{
    uint8_t   *p_py_name;
    uint16_t  py_name_len;

    uint8_t   *p_icon_fmt;
    uint16_t  icon_fmt_len;

    uint8_t   *p_url;
    uint16_t  url_len;
    uint16_t  play_order_sup;
} T_MCS_MEDPY_INFO;

uint8_t mcs_audio_get_ccid(T_SERVER_ID service_id);
void mcs_audio_update_media_state(T_SERVER_ID service_id, T_MEDIA_STATE state);
void mcs_set_mcp_sup_opcode(T_SERVER_ID service_id, uint32_t media_ctl_point_op_sup);
void mcs_set_media_track_info(T_SERVER_ID service_id, T_MCS_TR_INFO *p_tk_info);
void mcs_audio_update_playback_speed(T_SERVER_ID service_id, int8_t speed);
void mcs_audio_update_seeking_speed(T_SERVER_ID srv_id, int8_t speed);
void mcs_audio_update_playing_order(T_SERVER_ID service_id, uint8_t order);
void mcs_audio_update_track_pos(T_SERVER_ID service_id, int pos, bool force_notify);
void mcs_set_media_player_info(T_SERVER_ID service_id, T_MCS_MEDPY_INFO *p_info);
uint64_t mcs_create_root_group(T_SERVER_ID service_id, T_MCS_GP_ELEM *p_gp_elem);
uint64_t mcs_create_major_group(T_SERVER_ID service_id, T_MCS_GP_ELEM *p_gp_elem,
                                uint64_t parent_obj_id);
uint64_t mcs_create_major_track(T_SERVER_ID service_id, T_MCS_TK_ELEM *p_tk_elem,
                                uint64_t gp_obj_id,
                                uint8_t *p_id3v2_fmt, uint16_t fmt_len);
void mcs_delete_major_gp(T_SERVER_ID service_id, uint64_t gp_obj_id);
void mcs_delete_major_track(T_SERVER_ID service_id, uint64_t gp_obj_id, uint64_t tk_obj_id);
void mcs_update_major_gp_value(T_SERVER_ID service_id, uint64_t gp_obj_id);
void mcs_make_audio_struct_complt(T_SERVER_ID service_id);
void mcs_read_req_confirm(T_MCS_READ_REQ *p_read_req, uint8_t *p_value, uint16_t len);
bool mcs_local_search_op(T_SERVER_ID service_id, uint8_t *p_value, uint16_t len);
T_SERVER_ID mcs_add_server(uint16_t char_msk, uint16_t attr_feature, T_SERVER_ID ots_srv_id,
                           bool general);
bool mcs_server_init(uint8_t mcs_num);
#ifdef  __cplusplus
}
#endif      /*  __cplusplus */

#endif  /* _MCS_COMM_H_ */
