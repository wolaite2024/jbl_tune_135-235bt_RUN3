#ifndef _BT_GATTC_STORAGE_H_
#define _BT_GATTC_STORAGE_H_

#ifdef  __cplusplus
extern "C" {
#endif      /* __cplusplus */

#include "vector.h"
#include "bt_gatt_client.h"
#include "os_queue.h"

#if BT_GATT_CLIENT_SUPPORT

#define GATTC_STORAGE_SRV_MAX_NUM      24
#define GATTC_STORAGE_CHAR_MAX_NUM     36
#define GATTC_STORAGE_INCLUDE_MAX_NUM  16

#define ATTR_DATA_STRUCT_LEN sizeof(T_ATTR_DATA)

typedef enum
{
    ATTR_TYPE_PRIMARY_SRV_UUID16    = 0x01,
    ATTR_TYPE_PRIMARY_SRV_UUID128   = 0x02,
    ATTR_TYPE_SECONDARY_SRV_UUID16  = 0x03,
    ATTR_TYPE_SECONDARY_SRV_UUID128 = 0x04,
    ATTR_TYPE_INCLUDE_UUID16        = 0x05,
    ATTR_TYPE_INCLUDE_UUID128       = 0x06,
    ATTR_TYPE_CHAR_UUID16           = 0x07,
    ATTR_TYPE_CHAR_UUID128          = 0x08,
    ATTR_TYPE_CCCD_DESC             = 0x09,
} T_ATTR_TYPE;

typedef struct
{
    uint8_t     attr_type;
    uint8_t     instance_id;
    uint16_t    att_handle;
} T_ATTR_HEADER;

typedef struct
{
    T_ATTR_HEADER hdr;

    uint16_t    end_group_handle;
    uint16_t    uuid16;
} T_ATTR_SRV_UUID16;

typedef struct
{
    T_ATTR_HEADER hdr;

    uint16_t    end_group_handle;
    uint8_t     uuid128[16];
} T_ATTR_SRV_UUID128;

typedef struct
{
    T_ATTR_HEADER hdr;

    uint16_t    properties;
    uint16_t    value_handle;
    uint16_t    uuid16;
} T_ATTR_CHAR_UUID16;

typedef struct
{
    T_ATTR_HEADER hdr;

    uint16_t    properties;
    uint16_t    value_handle;
    uint8_t     uuid128[16];
} T_ATTR_CHAR_UUID128;

typedef struct
{
    T_ATTR_HEADER hdr;
} T_ATTR_CCCD_DESC;

typedef struct
{
    T_ATTR_HEADER hdr;

    uint16_t    start_handle;
    uint16_t    end_handle;
    uint16_t    uuid16;
} T_ATTR_INCLUDE_UUID16;

typedef struct
{
    T_ATTR_HEADER hdr;

    uint16_t    start_handle;
    uint16_t    end_handle;
    uint8_t     uuid128[16];
} T_ATTR_INCLUDE_UUID128;

typedef union
{
    T_ATTR_HEADER          hdr;
    T_ATTR_SRV_UUID16      srv_uuid16;
    T_ATTR_SRV_UUID128     srv_uuid128;
    T_ATTR_INCLUDE_UUID16  include_uuid16;
    T_ATTR_INCLUDE_UUID128 include_uuid128;
    T_ATTR_CHAR_UUID16     char_uuid16;
    T_ATTR_CHAR_UUID128    char_uuid128;
    T_ATTR_CCCD_DESC       cccd_desc;
} T_ATTR_DATA;

typedef struct
{
    VECTOR include_srv_tbl;
    VECTOR char_tbl;
    T_ATTR_DATA srv_data;
} T_ATTR_SRV_CB;

typedef struct
{
    T_ATTR_CCCD_DESC cccd_descriptor;
    T_ATTR_DATA char_data;
} T_ATTR_CHAR_CB;

typedef enum
{
    GATTC_REQ_TYPE_READ    = 0x00,
    GATTC_REQ_TYPE_WRITE   = 0x01,
    GATTC_REQ_TYPE_CCCD    = 0x02,
    GATTC_REQ_TYPE_READ_UUID = 0x03,
} T_GATTC_REQ_TYPE;

typedef struct
{
    T_GATT_WRITE_TYPE write_type;
    uint16_t length;
    uint8_t *p_data;
} T_GATTC_WRITE_REQ;

typedef struct
{
    uint16_t uuid16;
    uint16_t start_handle;
    uint16_t end_handle;
} T_GATTC_READ_UUID;

typedef struct
{
    bool enable;
    uint16_t uuid16;
    T_ATTR_SRV_CB *p_srv_cb;
} T_GATTC_CCCD_CFG;

typedef struct t_gattc_req
{
    struct t_gattc_req  *p_next;
    T_GATTC_REQ_TYPE req_type;
    uint16_t handle;
    P_FUN_GATT_CLIENT_CB req_cb;
    union
    {
        T_GATTC_WRITE_REQ write;
        T_GATTC_CCCD_CFG  cccd_cfg;
        T_GATTC_READ_UUID read_uuid;
    } p;
} T_GATTC_REQ;

typedef struct t_gattc_storage_cb
{
    struct t_gattc_storage_cb *p_next;
    uint16_t conn_handle;
    uint8_t conn_id;
    T_GAP_CONN_STATE conn_state;
    T_GATTC_STORAGE_STATE state;
    //GATTC queue
    uint16_t pending_handle;
    uint16_t pending_uuid16;
    bool     pending_cccd;
    bool     pending_cccd_enable;
    T_OS_QUEUE gattc_req_list;
    P_FUN_GATT_CLIENT_CB pending_req_cb;
    P_FUN_GATT_CLIENT_CB p_dis_cb;

    //GATT Client discovery
    VECTOR_ITERATOR srv_list;
    VECTOR_ITERATOR char_list;
    T_ATTR_SRV_CB *p_curr_srv;
    T_ATTR_CHAR_CB *p_curr_char;
    VECTOR srv_tbl;
} T_GATTC_STORAGE_CB;

extern P_FUN_GATT_STORAGE_CB gatt_storage_cb;
//storage
bool gattc_storage_add_primary_service(T_GATTC_STORAGE_CB *p_gattc_cb, bool uuid16,
                                       T_DISCOVERY_RESULT_DATA *p_result_data);
bool gattc_storage_add_secondary_service(T_GATTC_STORAGE_CB *p_gattc_cb);
bool gattc_storage_add_include(T_ATTR_SRV_CB *p_srv_cb, bool uuid16,
                               T_DISCOVERY_RESULT_DATA *p_result_data);
bool gattc_storage_add_char(T_ATTR_SRV_CB *p_srv_cb, bool uuid16,
                            T_DISCOVERY_RESULT_DATA *p_result_data);
bool gattc_storage_add_cccd_desc(T_ATTR_CHAR_CB *p_char_cb, uint16_t handle);
void gattc_storage_init(T_GATTC_STORAGE_CB *p_gattc_cb);
void gattc_storage_release(T_GATTC_STORAGE_CB *p_gattc_cb);
bool gattc_storage_refresh(T_GATTC_STORAGE_CB *p_gattc_cb);
bool gattc_storage_load(T_GATTC_STORAGE_CB *p_gattc_cb);
bool gattc_storage_write(T_GATTC_STORAGE_CB *p_gattc_cb);
void gattc_storage_print(T_GATTC_STORAGE_CB *p_gattc_cb);
uint8_t gattc_storage_get_srv_instance_id(T_GATTC_STORAGE_CB *p_gattc_cb, bool is_uuid16,
                                          uint16_t uuid16, uint8_t *p_uuid128);
uint8_t gattc_storage_get_char_instance_id(T_ATTR_SRV_CB *p_srv_cb, bool is_uuid16, uint16_t uuid16,
                                           uint8_t *p_uuid128);
T_ATTR_SRV_CB *gattc_storage_find_srv_by_handle(T_GATTC_STORAGE_CB *p_gattc_cb, uint16_t handle);
bool gattc_storage_find_char_desc(T_ATTR_SRV_CB *p_srv_cb, uint16_t handle, bool *p_is_cccd,
                                  T_ATTR_DATA *p_char_data);
T_ATTR_SRV_CB *gattc_storage_find_srv_by_uuid(T_GATTC_STORAGE_CB *p_gattc_cb,
                                              T_ATTR_UUID *p_attr_uuid);
T_ATTR_SRV_CB *gattc_storage_find_inc_srv_by_uuid(T_GATTC_STORAGE_CB *p_gattc_cb,
                                                  T_ATTR_SRV_CB *p_inc_srv);
T_ATTR_CHAR_CB *gattc_storage_find_char_by_uuid(T_ATTR_SRV_CB *p_srv_cb, T_ATTR_UUID *p_attr_uuid);
bool gattc_storage_check_prop(T_GATTC_STORAGE_CB *p_gattc_cb, uint16_t handle,
                              uint16_t properties_bit);
void att_data_covert_to_uuid(T_ATTR_DATA *p_attr_data, T_ATTR_UUID *p_attr_uuid);
#endif

#ifdef  __cplusplus
}
#endif      /*  __cplusplus */

#endif
