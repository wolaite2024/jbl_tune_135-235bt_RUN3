/**
*****************************************************************************************
*     Copyright(c) 2017, Realtek Semiconductor Corporation. All rights reserved.
*****************************************************************************************
  * @file     ots_client.h
  * @brief    Head file for Object transfer Service.
  * @details  mcs data structs and external functions declaration.
  * @author   cheng_cai
  * @date
  * @version  v1.0
  * *************************************************************************************
  */

/* Define to prevent recursive inclusion */
#ifndef _OTS_CLIENT_H_
#define _OTS_CLIENT_H_

#ifdef  __cplusplus
extern "C" {
#endif      /* __cplusplus */

#include "ots_def.h"
#include "srv_util.h"

#if LE_AUDIO_OTS_CLIENT_SUPPORT
#define OTS_OACP_CCCD_FLAG        0x01
#define OTS_OLCP_CCCD_FLAG        0x02
#define OTS_OBJ_CHG_CCCD_FLAG     0x04


enum
{
    OTS_TR_CHAN_ST_NONE = 0,
    OTS_TR_CHAN_ST_CL_OPENING,
    OTS_TR_CHAN_ST_CL_OPEN,
    OTS_TR_CHAN_ST_CL_CLOSING,
};


typedef enum
{
    OACP_IND_SUPPORT = 0x00,
    OLCP_IND_SUPPORT,
    OBJ_CHANGE_IND_SUPPORT,
    OTS_IND_NUM_MAX
} T_OTS_IND_SUP;

typedef struct
{
    uint16_t conn_handle;
    bool    is_found;
    bool    load_form_ftl;
    uint8_t ots_srv_num;
} T_OTS_CLIENT_DIS_DONE;


typedef struct
{
    uint32_t          size;
    T_CHAR_UUID       gatt_uuid;
} T_OTS_OACP_CREATE;

typedef struct
{
    uint32_t          offset;
    uint32_t          length;
} T_OTS_OACP_CHKSUM;

typedef struct
{
    uint8_t           *p_value;
    uint16_t          length;
} T_OTS_OACP_EXC;

typedef struct
{
    uint32_t          offset;
    uint32_t          length;
    uint8_t           *p_value;   // caller set it to NULL
} T_OTS_OACP_READ;

typedef struct
{
    uint32_t          offset;
    uint32_t          length;
    uint8_t           mode;
    uint8_t           *p_value;
} T_OTS_OACP_WRITE;

typedef struct
{
    uint8_t   opcode;
    void      *p_oacp_cb;
    union
    {
        T_OTS_OACP_CREATE   oacp_create;
        T_OTS_OACP_CHKSUM   oacp_chksum;
        T_OTS_OACP_EXC      oacp_exc;
        T_OTS_OACP_READ     oacp_read;
        T_OTS_OACP_WRITE    oacp_write;
    } value;
} T_OTS_OACP_PARAM;

typedef struct
{
    uint8_t   opcode;
    void      *p_olcp_cb;
    union
    {
        uint64_t    obj_id;
        uint8_t     list_sort_order;
    } value;
} T_OTS_OLCP_PARAM;

typedef struct
{
    uint8_t   *p_name;
    uint16_t  name_len;
} T_OTS_NAME_PARAM;


typedef struct
{
    uint8_t   filt_type;
    union
    {
        T_OTS_NAME_PARAM      olf_name;
        T_CHAR_UUID           char_uuid;
        uint32_t              size[2];
    } value;
} T_OTS_OLF_PARAM;

typedef struct
{
    uint16_t    conn_handle;
    bool        notify;
    uint16_t    ind_res;
    uint8_t     instance_id;
    uint8_t     olcp_opcode;
    uint32_t    obj_num;
} T_OTS_CLT_OLCP_IND;

typedef struct
{
    uint16_t    conn_handle;
    bool        notify;
    uint16_t    cause;
    uint8_t     srv_id;
    uint16_t    uuid;

    union
    {
        uint32_t            ots_features[2];
        T_OTS_NAME_PARAM    ots_name_cb;
        T_OTS_OLF_PARAM     olf_param_cb;
        T_CHAR_UUID         obj_type;

        uint32_t            size[2];
        uint64_t            first_created;
        uint64_t            last_modify;
        uint64_t            id;       //48 bytes
        uint32_t            props;
    } value;
} T_OTS_CLT_READ_RES;


typedef void (*P_OTS_OACP_CB)(uint16_t conn_handle, uint8_t opcode, uint8_t result,
                              uint8_t *p_value,
                              uint16_t len);
typedef void (*P_OTS_OLCP_CB)(T_OTS_CLT_OLCP_IND *p_olcp_res);


bool ots_client_init(void);
void ots_read_char(uint8_t srv_idx, uint16_t conn_handle, uint16_t uuid, uint8_t char_inst_id);
bool ots_client_write_obj_name(uint16_t conn_handle, uint8_t srv_idx, uint8_t *p_name,
                               uint16_t name_len);
bool ots_client_write_olf(uint16_t conn_handle, uint8_t srv_idx, uint8_t char_inst_id,
                          T_OTS_OLF_PARAM *p_param);
bool ots_client_oacp_op(uint16_t conn_handle, uint8_t srv_idx, T_OTS_OACP_PARAM *p_param);
bool ots_client_olcp_op(uint16_t conn_handle, uint8_t srv_idx, T_OTS_OLCP_PARAM *p_param);
bool ots_cfg_cccd(uint8_t srv_idx, uint16_t conn_handle, uint8_t cfg_flags, bool enable);

#endif

#ifdef  __cplusplus
}
#endif      /*  __cplusplus */

#endif  /* _OTS_CLIENT_H_ */
