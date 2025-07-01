/**
*****************************************************************************************
*     Copyright(c) 2017, Realtek Semiconductor Corporation. All rights reserved.
*****************************************************************************************
  * @file     ots_common.h
  * @brief    Head file for Object Transfer Service.
  * @details  mcs data structs and external functions declaration.
  * @author   cheng_cai
  * @date
  * @version  v1.0
  * *************************************************************************************
  */

/* Define to prevent recursive inclusion */
#ifndef _OTS_COMM_H_
#define _OTS_COMM_H_

#ifdef  __cplusplus
extern "C" {
#endif      /* __cplusplus */

/* Add Includes here */
#include "profile_server_def.h"
#include "ots_def.h"
#include "os_queue.h"
#include "srv_util.h"


#define OBJ_NAME_WRITE_MSK            0x01
#define OBJ_FIRST_CR_WRITE_MSK        0x02
#define OBJ_LAST_MOD_WRITE_MSK        0x04
#define OBJ_PROPS_WRITE_MSK           0x08

#define OBJ_FIRST_CR_CHAR_MSK         0x01
#define OBJ_LAST_MOD_CHAR_MSK         0x02
#define OBJ_LIST_FILTER_CHAR_MSK      0x04
#define OBJ_CHG_CHAR_MSK              0x08


#define OTS_ALLOCATE_SIZE_MASK      0xFFFFFFFF00000000
#define OTS_CURRENT_SEIZE_MASK      0x00000000FFFFFFFF


#define OTS_OACP_TRUNCATE_MODE        1

typedef struct
{
    uint8_t   name[OTS_OBJ_NAME_LEN_MAX + 1];
    uint16_t  name_len;
} T_OBJ_NAME;

typedef struct t_object_cb
{
    struct t_object_cb     *p_next;
    T_OBJ_NAME              obj_name;
    T_CHAR_UUID             obj_type;
    uint32_t                obj_size[2];
    uint64_t                obj_first_created;
    uint64_t                obj_last_modify;
    uint64_t                obj_id;         //As stack doesn't have uint48_t, so we use uint64_t instead
    uint32_t                obj_props;
    uint8_t                *alloc_buf;
    uint8_t                *temp_write_buf;

    uint32_t                wr_offset;          //write or read offset
    uint32_t                wr_len;             //write or read len
    uint32_t                transferd_len;
    uint8_t                 obj_lock_state;
    uint8_t                 mode;
} T_OBJECT_CB;

typedef struct
{
    uint8_t *p_uuid;
    uint16_t uuid_size;
    uint8_t *p_name;
    uint16_t name_len;
    uint8_t *p_value;
    uint16_t len;
    uint32_t props;
} T_OTS_OBJ_PARAM;


bool update_obj_value_by_id(T_SERVER_ID service_id, uint64_t obj_id, uint8_t *p_value,
                            uint16_t len);
T_OBJECT_CB *create_ots_object_internal(T_SERVER_ID ots_service_id, T_OTS_OBJ_PARAM *p_param);
void delete_ots_object_internal(T_SERVER_ID service_id, uint64_t obj_id);
T_OS_QUEUE *ots_get_obj_queue(T_SERVER_ID ots_service_id);

T_OBJECT_CB *find_obj_by_id(T_SERVER_ID service_id, uint64_t obj_id);
void ots_set_feature(T_SERVER_ID ots_service_id, uint32_t oacp_feature, uint32_t olcp_feature);
T_SERVER_ID ots_add_server(uint16_t char_msk, uint16_t attr_feature);
bool ots_server_init(uint8_t ots_num);

#ifdef  __cplusplus
}
#endif      /*  __cplusplus */

#endif  /* _OTS_COMM_H_ */
