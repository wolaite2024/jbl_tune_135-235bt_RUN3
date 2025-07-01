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
#ifndef _MCS_DEF_H_
#define _MCS_DEF_H_

#ifdef  __cplusplus
extern "C" {
#endif      /* __cplusplus */

/* Add Includes here */
#include "ots_comm.h"

#define MEDIA_CONTROL_SERVICE_UUID                  0x1848
#define GEN_MEDIA_CONTROL_SERVICE_UUID              0x1849

#define MEDIA_PLAYER_NAME_CHAR_UUID                 0x2B93
#define MEDIA_PLAYER_ICON_OBJID_CHAR_UUID           0x2B94
#define MEDIA_PLAYER_ICON_URL_CHAR_UUID             0x2B95
#define TRACK_CHANGE_CHAR_UUID                      0x2B96
#define TRACK_TITLE_CHAR_UUID                       0x2B97
#define TRACK_DURATION_CHAR_UUID                    0x2B98
#define TRACK_POSITION_CHAR_UUID                    0x2B99
#define PLAYBACK_SPEED_CHAR_UUID                    0x2B9A
#define SEEKING_SPEED_CHAR_UUID                     0x2B9B
#define CUR_TRACK_SEG_OBJID_CHAR_UUID               0x2B9C
#define CUR_TRACK_OBJID_CHAR_UUID                   0x2B9D
#define NEXT_TRACK_OBJID_CHAR_UUID                  0x2B9E
#define PARENT_GRP_OBJID_CHAR_UUID                  0x2B9F
#define CUR_GRP_OBJID_CHAR_UUID                     0x2BA0
#define PLAYING_ORDER_CHAR_UUID                     0x2BA1
#define PLAYING_ORDER_SUP_CHAR_UUID                 0x2BA2
#define MEDIA_STATE_CHAR_UUID                       0x2BA3
#define MEDIA_CONTROL_POINT_CHAR_UUID               0x2BA4
#define MEDIA_CTPOINT_OPCODE_SUP_CHAR_UUID          0x2BA5
#define SEARCH_CONTROL_POINT_CHAR_UUID              0x2BA7
#define SEARCH_RESULT_OBJID_CHAR_UUID               0x2BA6
#define CONTENT_CONTROL_ID_CHAR_UUID                0x2BBA

//Object Transfer Service object types
#define MEDIA_PLAYER_ICON_OBJ_TYPE                  0x2BA9
#define TRACK_SEGMENGS_OBJ_TYPE                     0x2BAA
#define TRACK_OBJ_TYPE                              0x2BAB
#define GROUP_OBJ_TYPE                              0x2BAC

//object type field
typedef enum
{
    MCS_TRACK_OBJ_TYPE        = 0,
    MCS_GROUP_OBJ_TYPE,
    MCS_OBJ_TYPE_RFU
} T_OBJ_TYPE_FLD;

//Media State
typedef enum
{
    MCS_MEDIA_INACTIVE_STATE = 0x00,
    MCS_MEDIA_PLAYING_STATE,
    MCS_MEDIA_PAUSED_STATE,
    MCS_MEDIA_SEEKING_STATE
} T_MEDIA_STATE;

//Playing Order
typedef enum
{
    MCS_SINGLE_ONCE_ORDER = 0x01,
    MCS_SINGLER_REPEAT_ORDER,
    MCS_IN_ORDER_ONCE_ORDER,
    MCS_IN_ORDER_REPEAT_ORDER,
    MCS_OLDEST_ONCE_ORDER,
    MCS_OLDEST_REPEAT_ORDER,
    MCS_NEWEST_ONCE_ORDER,
    MCS_NEWEST_REPEAT_ORDER,
    MCS_SHUFFLE_ONCE_ORDER,
    MCS_SHUFFLE_REPEAT_ORDER,
    MCS_PLAYING_ORDER_MAX,
} T_PLAYING_ORDER;

//Search control point type
typedef enum
{
    MCS_TACK_NAME_TYPE = 0x01,
    MCS_ARTIST_NAME_TYPE,
    MCS_ALBUM_NAME_TYPE,
    MCS_GROUP_NAME_TYPE,
    MCS_EARLIEST_YEAR_TYPE,
    MCS_LATEST_YEAR_TYPE,
    MCS_GENRE_TYPE,
    MCS_ONLY_TRACKS_TYPE,
    MCS_ONLY_GROUPS_TYPE,
    MCS_SEARCH_TYPE_MAX,
} T_SEARCH_TYPE;

#define SEARCH_POINT_SUCCESS_CODE     0x01
#define SEARCH_POINT_FAILURE_CODE     0x02

#define PLAY_CONTROL_OPCODE                   0x01
#define PAUSE_CONTROL_OPCODE                  0x02
#define FAST_REWIND_CONTROL_OPCODE            0x03
#define FAST_FORWARD_CONTROL_OPCODE           0x04
#define STOP_CONTROL_OPCODE                   0x05
#define MOVE_RELATIVE_CONTROL_OPCODE          0x10
#define PREVIOUS_SEGM_CONTROL_OPCODE          0x20
#define NEXT_SEGM_CONTROL_OPCODE              0x21
#define FIRST_SEGM_CONTROL_OPCODE             0x22
#define LAST_SEGM_CONTROL_OPCODE              0x23
#define GOTO_SEGM_CONTROL_OPCODE              0x24
#define PREVOUS_TRACK_CONTROL_OPCODE          0x30
#define NEXT_TRACK_CONTROL_OPCODE             0x31
#define FIRST_TRACK_CONTROL_OPCODE            0x32
#define LAST_TRACK_CONTROL_OPCODE             0x33
#define GOTO_TRACK_CONTROL_OPCODE             0x34
#define PREVIOUS_GRP_CONTROL_OPCODE           0x40
#define NEXT_GRP_CONTROL_OPCODE               0x41
#define FIRST_GRP_CONTROL_OPCODE              0x42
#define LAST_GRP_CONTROL_OPCODE               0x43
#define GOTO_GRP_CONTROL_OPCODE               0x44

#define PLAY_SUPPORTED_VAL                   0x01
#define PAUSE_SUPPORTED_VAL                  0x02
#define FAST_REWIND_SUPPORTED_VAL            0x04
#define FAST_FORWARD_SUPPORTED_VAL           0x08
#define STOP_SUPPORTED_VAL                   0x10
#define MOVE_RELATIVE_SUPPORTED_VAL          0x20
#define PREVIOUS_SEGM_SUPPORTED_VAL          0x40
#define NEXT_SEGM_SUPPORTED_VAL              0x80
#define FIRST_SEGM_SUPPORTED_VAL             0x100
#define LAST_SEGM_SUPPORTED_VAL              0x200
#define GOTO_SEGM_SUPPORTED_VAL              0x400
#define PREVOUS_TRACK_SUPPORTED_VAL          0x800
#define NEXT_TRACK_SUPPORTED_VAL             0x1000
#define FIRST_TRACK_SUPPORTED_VAL            0x2000
#define LAST_TRACK_SUPPORTED_VAL             0x4000
#define GOTO_TRACK_SUPPORTED_VAL             0x8000
#define PREVIOUS_GRP_SUPPORTED_VAL           0x10000
#define NEXT_GRP_SUPPORTED_VAL               0x20000
#define FIRST_GRP_SUPPORTED_VAL              0x40000
#define LAST_GRP_SUPPORTED_VAL               0x80000
#define GOTO_GRP_SUPPORTED_VAL               0x100000


#define MCS_MCP_RESULT_SUCCESS            1
#define MCS_MCP_OPCODE_NOT_SUP            2
#define MCS_MCP_MEDIA_PLAYER_INACTIVE     3
#define MCS_MCP_CMD_NOT_COMPLT            4

#define MCS_READ_LONG_ERR_CODE            0x80

typedef struct
{
    uint8_t   type;
    uint8_t   obj_id[OTS_OBJ_ID_LEN];
} T_MCS_GP_OBJ;
#ifdef  __cplusplus
}
#endif      /*  __cplusplus */

#endif  /* _MCS_COMM_H_ */
