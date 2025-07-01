#include <string.h>
#include "trace.h"
#include "bt_gatt_client.h"
#include "bt_types.h"
#include "bt_gattc_storage.h"
#include "os_mem.h"
#include "os_queue.h"
#include "os_mem.h"

#if BT_GATT_CLIENT_SUPPORT

#define LE_AUDIO_DEBUG 1
#define GATT_CLIENT_SPEC_CB_NUM 16

typedef struct
{
    T_ATTR_UUID srv_uuid;
    P_FUN_GATT_CLIENT_CB p_fun_cb;
} T_SPEC_GATTC_CB;

VECTOR spec_gattc_cb = NULL;
T_OS_QUEUE bt_gattc_queue;
static T_CLIENT_ID gattc_storage_cl_id = CLIENT_PROFILE_GENERAL_ID;
P_FUN_GATT_STORAGE_CB gatt_storage_cb = NULL;

#define gatt_client_check_link(conn_handle) gatt_client_check_link_int(__func__, conn_handle)
static void gatt_client_send_dis_result(uint16_t conn_handle, T_GATTC_STORAGE_CB  *p_gattc_cb,
                                        bool load_form_ftl,
                                        bool is_success);

T_GATTC_STORAGE_CB *gatt_client_find(uint16_t conn_handle)
{
    uint8_t i;
    for (i = 0; i < bt_gattc_queue.count; i++)
    {
        T_GATTC_STORAGE_CB *p_gatt_client = (T_GATTC_STORAGE_CB *)os_queue_peek(&bt_gattc_queue, i);
        if (p_gatt_client->conn_handle == conn_handle)
        {
            return p_gatt_client;
        }
    }
    return NULL;
}

T_GATTC_STORAGE_CB *gatt_client_find_by_conn_id(uint16_t conn_id)
{
    uint8_t i;
    for (i = 0; i < bt_gattc_queue.count; i++)
    {
        T_GATTC_STORAGE_CB *p_gatt_client = (T_GATTC_STORAGE_CB *)os_queue_peek(&bt_gattc_queue, i);
        if (p_gatt_client->conn_id == conn_id)
        {
            return p_gatt_client;
        }
    }
    return NULL;
}

T_GATTC_STORAGE_CB  *gatt_client_check_link_int(const char *p_func_name, uint16_t conn_handle)
{
    T_GATTC_STORAGE_CB *p_gatt_client = NULL;
    p_gatt_client = gatt_client_find(conn_handle);
    if (p_gatt_client == NULL || p_gatt_client->conn_state != GAP_CONN_STATE_CONNECTED)
    {
        PROTOCOL_PRINT_ERROR2("gatt_client_check_link_int: failed %s, conn_handle 0x%x",
                              TRACE_STRING(p_func_name), conn_handle);
        return NULL;
    }
    return p_gatt_client;
}

T_GATTC_STORAGE_CB  *gatt_client_allcate(uint8_t conn_id)
{
    T_GATTC_STORAGE_CB *p_gatt_client = gatt_client_find_by_conn_id(conn_id);
    if (p_gatt_client)
    {
        PROTOCOL_PRINT_ERROR0("gatt_client_allcate: already exist");
        return p_gatt_client;
    }

    p_gatt_client = (T_GATTC_STORAGE_CB *)os_mem_zalloc(RAM_TYPE_GATT_CLIENT,
                                                        sizeof(T_GATTC_STORAGE_CB));
    if (p_gatt_client == NULL)
    {
        PROTOCOL_PRINT_ERROR0("simp_client_allcate: alloc fail");
        return NULL;
    }
    p_gatt_client->conn_id = conn_id;
    os_queue_in(&bt_gattc_queue, (void *)p_gatt_client);
    PROTOCOL_PRINT_INFO2("gatt_client_allcate: conn_id 0x%x, count %d",
                         conn_id, bt_gattc_queue.count);
    return p_gatt_client;
}

void gatt_client_del(T_GATTC_STORAGE_CB *p_gatt_client)
{
    if (p_gatt_client)
    {
        PROTOCOL_PRINT_INFO2("gatt_client_del: conn_handle 0x%x, p_gatt_client %p",
                             p_gatt_client->conn_handle, p_gatt_client);
        if (os_queue_delete(&bt_gattc_queue, (void *)p_gatt_client))
        {
            os_mem_free((void *) p_gatt_client);
        }
        else
        {
            PROTOCOL_PRINT_ERROR1("simp_client_del: failed, not find conn_handle 0x%x",
                                  p_gatt_client->conn_handle);
        }
    }
}

static bool match_spec_client_by_uuid(T_SPEC_GATTC_CB *p_db, T_ATTR_UUID *p_cmp)
{
    if (p_cmp->is_uuid16)
    {
        if (p_db->srv_uuid.is_uuid16 &&
            p_db->srv_uuid.p.uuid16 == p_cmp->p.uuid16)
        {
            return true;
        }
    }
    else
    {
        if (p_db->srv_uuid.is_uuid16 == false &&
            memcmp(p_db->srv_uuid.p.uuid128, p_cmp->p.uuid128, 16) == 0)
        {
            return true;
        }
    }

    return false;
}

T_SPEC_GATTC_CB *gatt_spec_client_find_by_uuid(T_ATTR_UUID *p_srv_uuid)
{
    T_SPEC_GATTC_CB *p_db;
    p_db = vector_search(spec_gattc_cb,
                         (V_PREDICATE) match_spec_client_by_uuid,
                         p_srv_uuid);
    return p_db;
}

void gattc_req_list_free(T_GATTC_STORAGE_CB *p_gattc_cb)
{
    T_GATTC_REQ *p_req;

    while (p_gattc_cb->gattc_req_list.count > 0)
    {
        p_req = os_queue_out(&p_gattc_cb->gattc_req_list);
        if (p_req)
        {
            if (p_req->req_type == GATTC_REQ_TYPE_WRITE && p_req->p.write.p_data)
            {
                os_mem_free(p_req->p.write.p_data);
            }
            os_mem_free(p_req);
        }
    }
    p_gattc_cb->pending_handle = 0;
    p_gattc_cb->pending_cccd = false;
    p_gattc_cb->pending_uuid16 = 0;
}

T_GAP_CAUSE gatt_client_execute_cccd_cfg(T_GATTC_STORAGE_CB  *p_gattc_cb,
                                         T_ATTR_SRV_CB *p_srv_cb, uint16_t uuid16, uint16_t start_handle, bool enable)
{
    T_GAP_CAUSE cause = GAP_CAUSE_INVALID_PARAM;
    bool is_found = false;
    T_ATTR_CHAR_CB *p_attr_char = NULL;
    VECTOR_ITERATOR char_tbl_iterator = NULL;
    if (p_srv_cb && p_srv_cb->char_tbl != NULL)
    {
        char_tbl_iterator = vector_iterator_create(p_srv_cb->char_tbl);
        while (vector_iterator_step(char_tbl_iterator, (VECTOR_ELE *)&p_attr_char))
        {
            if (p_attr_char->cccd_descriptor.hdr.att_handle != 0 &&
                p_attr_char->cccd_descriptor.hdr.att_handle > start_handle)
            {
                if (uuid16 != 0)
                {
                    if (p_attr_char->char_data.hdr.attr_type == ATTR_TYPE_CHAR_UUID16)
                    {
                        if (uuid16 == p_attr_char->char_data.char_uuid16.uuid16)
                        {
                            is_found = true;
                            break;
                        }
                    }
                }
                else
                {
                    is_found = true;
                    break;
                }
            }
        }
        vector_iterator_delete(char_tbl_iterator);
    }
    if (is_found)
    {
        uint16_t cccd_value = 0;
        if (enable)
        {
            if (p_attr_char->char_data.char_uuid16.properties & GATT_CHAR_PROP_NOTIFY)
            {
                cccd_value = GATT_CLT_CONFIG_NOTIFICATION;
            }
            else
            {
                cccd_value = GATT_CLT_CONFIG_INDICATION;
            }
        }
        cause = client_attr_write(p_gattc_cb->conn_id, gattc_storage_cl_id, GATT_WRITE_TYPE_REQ,
                                  p_attr_char->cccd_descriptor.hdr.att_handle,
                                  2, (uint8_t *)&cccd_value);
        if (cause == GAP_CAUSE_SUCCESS)
        {
            p_gattc_cb->pending_handle = p_attr_char->cccd_descriptor.hdr.att_handle;
            p_gattc_cb->p_curr_srv = p_srv_cb;
            p_gattc_cb->pending_uuid16 = uuid16;
            p_gattc_cb->pending_cccd = true;
            p_gattc_cb->pending_cccd_enable = enable;
        }
        return cause;
    }
    else
    {
        return GAP_CAUSE_NOT_FIND;
    }
}

T_GAP_CAUSE gatt_client_execute_req(T_GATTC_STORAGE_CB  *p_gattc_cb,
                                    T_GATTC_REQ *p_req)
{
    T_GAP_CAUSE cause = GAP_CAUSE_INVALID_PARAM;
    if (p_req->req_type == GATTC_REQ_TYPE_READ)
    {
        cause = client_attr_read(p_gattc_cb->conn_id, gattc_storage_cl_id, p_req->handle);
        if (cause == GAP_CAUSE_SUCCESS)
        {
            p_gattc_cb->pending_uuid16 = 0;
        }
    }
    else if (p_req->req_type == GATTC_REQ_TYPE_READ_UUID)
    {
        cause = client_attr_read_using_uuid(p_gattc_cb->conn_id, gattc_storage_cl_id,
                                            p_req->p.read_uuid.start_handle,
                                            p_req->p.read_uuid.end_handle, p_req->p.read_uuid.uuid16, NULL);
        if (cause == GAP_CAUSE_SUCCESS)
        {
            p_gattc_cb->pending_uuid16 = p_req->p.read_uuid.uuid16;
        }
    }
    else if (p_req->req_type == GATTC_REQ_TYPE_WRITE)
    {
        cause = client_attr_write(p_gattc_cb->conn_id, gattc_storage_cl_id, p_req->p.write.write_type,
                                  p_req->handle,
                                  p_req->p.write.length, p_req->p.write.p_data);
    }
    else if (p_req->req_type == GATTC_REQ_TYPE_CCCD)
    {
        cause = gatt_client_execute_cccd_cfg(p_gattc_cb, p_req->p.cccd_cfg.p_srv_cb,
                                             p_req->p.cccd_cfg.uuid16,
                                             0, p_req->p.cccd_cfg.enable);
        return cause;
    }
    if (cause == GAP_CAUSE_SUCCESS)
    {
        p_gattc_cb->pending_handle = p_req->handle;
        p_gattc_cb->pending_req_cb = p_req->req_cb;
    }
    return cause;
}

void gatt_client_handle_read_result(T_GATTC_STORAGE_CB  *p_gattc_cb,
                                    uint16_t cause,
                                    uint16_t handle, uint16_t value_size, uint8_t *p_value)
{
    bool is_cccd_desc;
    T_ATTR_DATA char_data;
    T_SPEC_GATTC_CB *p_spec_cb = NULL;
    T_ATTR_SRV_CB *p_srv_cb = gattc_storage_find_srv_by_handle(p_gattc_cb, handle);
    if (p_srv_cb)
    {
        T_ATTR_UUID srv_uuid;
        att_data_covert_to_uuid(&p_srv_cb->srv_data, &srv_uuid);
        p_spec_cb = gatt_spec_client_find_by_uuid(&srv_uuid);
        if (gattc_storage_find_char_desc(p_srv_cb, handle, &is_cccd_desc, &char_data) == false)
        {
            return;
        }
    }

    if (p_spec_cb != NULL && p_spec_cb->p_fun_cb != NULL)
    {
        T_GATT_CLIENT_READ_RESULT read_result;
        read_result.is_cccd_desc = is_cccd_desc;
        read_result.cause = cause;
        read_result.handle = handle;
        read_result.value_size = value_size;
        read_result.p_value = p_value;
        read_result.srv_instance_id = p_srv_cb->srv_data.hdr.instance_id;
        att_data_covert_to_uuid(&char_data, &read_result.char_uuid);
        if (p_gattc_cb->pending_handle == handle &&
            p_gattc_cb->pending_req_cb != NULL)
        {
            p_gattc_cb->pending_req_cb(p_gattc_cb->conn_handle, GATT_CLIENT_EVENT_READ_RESULT,
                                       (void *)&read_result);
        }
        else
        {
            p_spec_cb->p_fun_cb(p_gattc_cb->conn_handle, GATT_CLIENT_EVENT_READ_RESULT, (void *)&read_result);
        }
    }
}

void gatt_client_handle_write_result(T_GATTC_STORAGE_CB  *p_gattc_cb,
                                     T_GATT_WRITE_TYPE type,
                                     uint16_t handle, uint16_t cause)
{
    bool is_cccd_desc;
    T_ATTR_DATA char_data;
    T_SPEC_GATTC_CB *p_spec_cb = NULL;
    T_ATTR_SRV_CB *p_srv_cb = gattc_storage_find_srv_by_handle(p_gattc_cb, handle);
    if (p_srv_cb)
    {
        T_ATTR_UUID srv_uuid;
        att_data_covert_to_uuid(&p_srv_cb->srv_data, &srv_uuid);
        p_spec_cb = gatt_spec_client_find_by_uuid(&srv_uuid);
        if (gattc_storage_find_char_desc(p_srv_cb, handle, &is_cccd_desc, &char_data) == false)
        {
            return;
        }
    }
    if (p_spec_cb != NULL && p_spec_cb->p_fun_cb != NULL)
    {
        T_GATT_CLIENT_WRITE_RESULT write_result;
        write_result.is_cccd_desc = is_cccd_desc;
        write_result.cause = cause;
        write_result.handle = handle;
        write_result.type = type;
        write_result.srv_instance_id = p_srv_cb->srv_data.hdr.instance_id;
        att_data_covert_to_uuid(&char_data, &write_result.char_uuid);
        if (p_gattc_cb->pending_handle == handle &&
            p_gattc_cb->pending_req_cb != NULL)
        {
            p_gattc_cb->pending_req_cb(p_gattc_cb->conn_handle, GATT_CLIENT_EVENT_WRITE_RESULT,
                                       (void *)&write_result);
        }
        else
        {
            p_spec_cb->p_fun_cb(p_gattc_cb->conn_handle, GATT_CLIENT_EVENT_WRITE_RESULT, (void *)&write_result);
        }
    }
}

void gatt_client_send_cccd_cfg_result(T_GATTC_STORAGE_CB  *p_gattc_cb,
                                      T_ATTR_SRV_CB *p_srv_cb, bool enable, uint16_t char_uuid16, uint16_t cause)
{
    T_SPEC_GATTC_CB *p_spec_cb = NULL;
    if (p_srv_cb)
    {
        T_ATTR_UUID srv_uuid;
        att_data_covert_to_uuid(&p_srv_cb->srv_data, &srv_uuid);
        p_spec_cb = gatt_spec_client_find_by_uuid(&srv_uuid);
    }
    if (p_spec_cb != NULL && p_spec_cb->p_fun_cb != NULL)
    {
        T_GATT_CLIENT_CCCD_CFG cccd_cfg;
        cccd_cfg.cause = cause;
        cccd_cfg.srv_instance_id = p_srv_cb->srv_data.hdr.instance_id;
        cccd_cfg.enable = enable;
        cccd_cfg.char_uuid16 = char_uuid16;
        p_spec_cb->p_fun_cb(p_gattc_cb->conn_handle, GATT_CLIENT_EVENT_CCCD_CFG, (void *)&cccd_cfg);
    }
}

void gatt_client_handle_pending_req(T_GATTC_STORAGE_CB  *p_gattc_cb)
{
    T_GAP_CAUSE cause;
    T_GATTC_REQ *p_req;

    while (p_gattc_cb->gattc_req_list.count > 0)
    {
        p_req = os_queue_out(&p_gattc_cb->gattc_req_list);
        if (p_req)
        {
            cause = gatt_client_execute_req(p_gattc_cb, p_req);

            if (cause != GAP_CAUSE_SUCCESS)
            {
                if (p_req->req_type == GATTC_REQ_TYPE_READ ||
                    p_req->req_type == GATTC_REQ_TYPE_READ_UUID)
                {
                    gatt_client_handle_read_result(p_gattc_cb, GAP_ERR_REQ_FAILED,
                                                   p_req->handle, 0, NULL);
                }
                else if (p_req->req_type == GATTC_REQ_TYPE_WRITE)
                {
                    gatt_client_handle_write_result(p_gattc_cb, p_req->p.write.write_type,
                                                    p_req->handle, GAP_ERR_REQ_FAILED);
                }
            }
            if (p_req->req_type == GATTC_REQ_TYPE_WRITE && p_req->p.write.p_data)
            {
                os_mem_free(p_req->p.write.p_data);
            }
            os_mem_free(p_req);
            if (cause == GAP_CAUSE_SUCCESS)
            {
                break;
            }
        }
    }
}

T_GAP_CAUSE gatt_client_add_req(uint16_t conn_handle, T_GATTC_STORAGE_CB  *p_gattc_cb,
                                T_GATTC_REQ *p_req)
{
    T_GATTC_REQ *p_pending_req;
    if (p_gattc_cb->gattc_req_list.count == 0 && p_gattc_cb->pending_handle == 0)
    {
        return gatt_client_execute_req(p_gattc_cb, p_req);
    }
    p_pending_req = os_mem_zalloc(RAM_TYPE_GATT_CLIENT, sizeof(T_GATTC_REQ));
    if (p_pending_req == NULL)
    {
        goto error;
    }
    memcpy(p_pending_req, p_req, sizeof(T_GATTC_REQ));
    if (p_req->req_type == GATTC_REQ_TYPE_WRITE && p_req->p.write.length != 0)
    {
        p_pending_req->p.write.p_data = os_mem_zalloc(RAM_TYPE_GATT_CLIENT, p_req->p.write.length);
        if (p_pending_req->p.write.p_data == NULL)
        {
            os_mem_free(p_pending_req);
            goto error;
        }
        memcpy(p_pending_req->p.write.p_data, p_req->p.write.p_data, p_req->p.write.length);
    }
    os_queue_in(&p_gattc_cb->gattc_req_list, p_pending_req);
    return GAP_CAUSE_SUCCESS;
error:
    return GAP_CAUSE_NO_RESOURCE;
}

T_GAP_CAUSE gatt_client_start_discovery_all(uint16_t conn_handle, P_FUN_GATT_CLIENT_CB p_dis_cb)
{
    T_GAP_CAUSE cause;
    T_GATTC_STORAGE_CB  *p_gattc_cb = gatt_client_check_link(conn_handle);
    if (p_gattc_cb == NULL)
    {
        return GAP_CAUSE_INVALID_STATE;
    }

    if (p_gattc_cb->state == GATTC_STORAGE_STATE_DONE)
    {
        gattc_storage_refresh(p_gattc_cb);
    }
    else if (p_gattc_cb->state == GATTC_STORAGE_STATE_DISCOVERY)
    {
        return GAP_CAUSE_ALREADY_IN_REQ;
    }
    p_gattc_cb->p_dis_cb = p_dis_cb;
    if (p_gattc_cb->state == GATTC_STORAGE_STATE_IDLE)
    {
        if (gattc_storage_load(p_gattc_cb))
        {
#if LE_AUDIO_DEBUG
            gattc_storage_print(p_gattc_cb);
#endif
            gatt_client_send_dis_result(conn_handle, p_gattc_cb, true, true);
            return GAP_CAUSE_SUCCESS;
        }
    }

    gattc_storage_init(p_gattc_cb);

    cause = client_all_primary_srv_discovery(p_gattc_cb->conn_id, gattc_storage_cl_id);
    if (cause != GAP_CAUSE_SUCCESS)
    {
        PROTOCOL_PRINT_ERROR2("gatt_client_start_discovery_all: failed, conn_handle 0x%x, cause %d",
                              conn_handle, cause);
    }
    else
    {
        p_gattc_cb->state = GATTC_STORAGE_STATE_DISCOVERY;
        if (p_gattc_cb->p_dis_cb)
        {
            T_GATT_CLIENT_DIS_ALL_DONE dis_all_done;
            dis_all_done.state = p_gattc_cb->state;
            dis_all_done.load_form_ftl = false;
            p_gattc_cb->p_dis_cb(conn_handle, GATT_CLIENT_EVENT_DIS_ALL_STATE, (void *)&dis_all_done);
        }
    }

    return cause;
}

T_GAP_CAUSE gatt_client_enable_char_cccd(uint16_t conn_handle, T_ATTR_UUID *p_srv_uuid,
                                         uint16_t char_uuid16, bool enable)
{
    T_GATTC_REQ cccd_req;
    T_GAP_CAUSE cause = GAP_CAUSE_INVALID_PARAM;
    T_GATTC_STORAGE_CB  *p_gattc_cb = gatt_client_check_link(conn_handle);
    T_ATTR_SRV_CB *p_srv_cb;
    if (p_gattc_cb == NULL || p_gattc_cb->state != GATTC_STORAGE_STATE_DONE)
    {
        cause = GAP_CAUSE_INVALID_STATE;
        goto result;
    }
    p_srv_cb = gattc_storage_find_srv_by_uuid(p_gattc_cb, p_srv_uuid);
    if (p_srv_cb == NULL)
    {
        goto result;
    }
    memset(&cccd_req, 0, sizeof(T_GATTC_REQ));
    cccd_req.req_type = GATTC_REQ_TYPE_CCCD;
    cccd_req.handle = 0;
    cccd_req.p.cccd_cfg.p_srv_cb = p_srv_cb;
    cccd_req.p.cccd_cfg.enable = enable;
    cccd_req.p.cccd_cfg.uuid16 = char_uuid16;
    cause = gatt_client_add_req(conn_handle, p_gattc_cb, &cccd_req);
result:
    if (cause != GAP_CAUSE_SUCCESS)
    {
        PROTOCOL_PRINT_ERROR1("gatt_client_enable_char_cccd: failed, cause 0x%x", cause);
    }
    return cause;
}

T_GAP_CAUSE gatt_client_enable_srv_cccd(uint16_t conn_handle, T_ATTR_UUID *p_srv_uuid, bool enable)
{
    return gatt_client_enable_char_cccd(conn_handle, p_srv_uuid, 0, enable);
}

T_GAP_CAUSE gatt_client_read(uint16_t conn_handle, uint16_t handle, P_FUN_GATT_CLIENT_CB p_req_cb)
{
    T_GATTC_REQ read_req;
    T_GAP_CAUSE cause = GAP_CAUSE_INVALID_PARAM;
    T_GATTC_STORAGE_CB  *p_gattc_cb = gatt_client_check_link(conn_handle);
    if (p_gattc_cb == NULL || p_gattc_cb->state != GATTC_STORAGE_STATE_DONE)
    {
        cause = GAP_CAUSE_INVALID_STATE;
        goto result;
    }
    if (gattc_storage_check_prop(p_gattc_cb, handle, GATT_CHAR_PROP_READ) == false)
    {
        goto result;
    }
    memset(&read_req, 0, sizeof(T_GATTC_REQ));
    read_req.req_type = GATTC_REQ_TYPE_READ;
    read_req.handle = handle;
    read_req.req_cb = p_req_cb;
    cause = gatt_client_add_req(conn_handle, p_gattc_cb, &read_req);

result:
    if (cause != GAP_CAUSE_SUCCESS)
    {
        PROTOCOL_PRINT_ERROR2("gatt_client_read: failed, handle 0x%x, cause 0x%x", handle, cause);
    }
    return cause;
}

T_GAP_CAUSE gatt_client_read_uuid(uint16_t conn_handle, uint16_t start_handle,
                                  uint16_t end_handle, uint16_t uuid16, P_FUN_GATT_CLIENT_CB p_req_cb)
{
    T_GATTC_REQ read_req;
    T_GAP_CAUSE cause = GAP_CAUSE_INVALID_PARAM;
    T_GATTC_STORAGE_CB  *p_gattc_cb = gatt_client_check_link(conn_handle);
    if (p_gattc_cb == NULL)
    {
        cause = GAP_CAUSE_INVALID_STATE;
        goto result;
    }
    memset(&read_req, 0, sizeof(T_GATTC_REQ));
    read_req.req_type = GATTC_REQ_TYPE_READ_UUID;
    read_req.handle = start_handle;
    read_req.p.read_uuid.start_handle = start_handle;
    read_req.p.read_uuid.end_handle = end_handle;
    read_req.p.read_uuid.uuid16 = uuid16;
    read_req.req_cb = p_req_cb;
    cause = gatt_client_add_req(conn_handle, p_gattc_cb, &read_req);

result:
    if (cause != GAP_CAUSE_SUCCESS)
    {
        PROTOCOL_PRINT_ERROR2("gatt_client_read_uuid: failed, uuid16 0x%x, cause 0x%x", uuid16, cause);
    }
    return cause;
}

//FIX TODO if the write type is write cmd, for now p_req_cb shall be null
T_GAP_CAUSE gatt_client_write(uint16_t conn_handle, T_GATT_WRITE_TYPE write_type,
                              uint16_t handle, uint16_t length, uint8_t *p_data, P_FUN_GATT_CLIENT_CB p_req_cb)
{
    T_GATTC_REQ write_req;
    T_GAP_CAUSE cause = GAP_CAUSE_INVALID_PARAM;
    T_GATTC_STORAGE_CB  *p_gattc_cb = gatt_client_check_link(conn_handle);
    if (p_gattc_cb == NULL || p_gattc_cb->state != GATTC_STORAGE_STATE_DONE)
    {
        cause = GAP_CAUSE_INVALID_STATE;
        goto result;
    }
    if (write_type == GATT_WRITE_TYPE_REQ)
    {
        if (gattc_storage_check_prop(p_gattc_cb, handle, GATT_CHAR_PROP_WRITE) == false)
        {
            goto result;
        }
    }
    else if (write_type == GATT_WRITE_TYPE_CMD)
    {
        if (gattc_storage_check_prop(p_gattc_cb, handle, GATT_CHAR_PROP_WRITE_NO_RSP) == false)
        {
            goto result;
        }
        cause = client_attr_write(p_gattc_cb->conn_id, gattc_storage_cl_id, write_type, handle, length,
                                  p_data);
        goto result;
        //if(cause != GAP_CAUSE_ERROR_CREDITS)
    }
    else
    {
        goto result;
    }
    memset(&write_req, 0, sizeof(T_GATTC_REQ));
    write_req.req_type = GATTC_REQ_TYPE_WRITE;
    write_req.p.write.write_type = write_type;
    write_req.handle = handle;
    write_req.p.write.length = length;
    write_req.p.write.p_data = p_data;
    write_req.req_cb = p_req_cb;
    cause = gatt_client_add_req(conn_handle, p_gattc_cb, &write_req);
result:
    if (cause != GAP_CAUSE_SUCCESS)
    {
        PROTOCOL_PRINT_ERROR2("gatt_client_write: failed, handle 0x%x, cause 0x%x", handle, cause);
    }
    return cause;
}

bool gatt_client_dis_next_secondary_service(uint16_t conn_handle, T_GATTC_STORAGE_CB *p_gattc_cb,
                                            bool *p_cmpl)
{
    T_GAP_CAUSE  cause;
    T_ATTR_SRV_CB *p_srv_cb = NULL;
    *p_cmpl = false;
    while (vector_iterator_step(p_gattc_cb->srv_list, (VECTOR_ELE *)&p_srv_cb))
    {
        if (p_srv_cb->srv_data.hdr.attr_type == ATTR_TYPE_SECONDARY_SRV_UUID16 ||
            p_srv_cb->srv_data.hdr.attr_type == ATTR_TYPE_SECONDARY_SRV_UUID128)
        {
            cause = client_all_char_discovery(p_gattc_cb->conn_id, gattc_storage_cl_id,
                                              p_srv_cb->srv_data.srv_uuid16.hdr.att_handle,
                                              p_srv_cb->srv_data.srv_uuid16.end_group_handle);
            if (cause == GAP_CAUSE_SUCCESS)
            {
                p_gattc_cb->p_curr_srv = p_srv_cb;
                return true;
            }
            else
            {
                goto error;
            }
        }
    }
    p_gattc_cb->p_curr_srv = NULL;
    *p_cmpl = true;
    return true;
error:
    return false;
}

bool gatt_client_dis_next_service(uint16_t conn_handle, T_GATTC_STORAGE_CB *p_gattc_cb,
                                  bool *p_cmpl)
{
    T_GAP_CAUSE  cause;
    T_ATTR_SRV_CB *p_srv_cb = NULL;
    *p_cmpl = false;

    if ((p_gattc_cb == NULL) || (p_gattc_cb->srv_list == NULL))
    {
        return false;
    }

    if (vector_iterator_step(p_gattc_cb->srv_list, (VECTOR_ELE *)&p_srv_cb))
    {
        cause = client_relationship_discovery(p_gattc_cb->conn_id, gattc_storage_cl_id,
                                              p_srv_cb->srv_data.srv_uuid16.hdr.att_handle,
                                              p_srv_cb->srv_data.srv_uuid16.end_group_handle);
        if (cause == GAP_CAUSE_SUCCESS)
        {
            p_gattc_cb->p_curr_srv = p_srv_cb;
            return true;
        }
    }
    else
    {
        if (gattc_storage_add_secondary_service(p_gattc_cb))
        {
            p_gattc_cb->p_curr_srv = NULL;
            if (p_gattc_cb->srv_list)
            {
                vector_iterator_delete(p_gattc_cb->srv_list);
                p_gattc_cb->srv_list = NULL;
                p_gattc_cb->srv_list = vector_iterator_create(p_gattc_cb->srv_tbl);
                if (p_gattc_cb->srv_list == NULL)
                {
                    return false;
                }
            }
            else
            {
                return false;
            }

            return gatt_client_dis_next_secondary_service(conn_handle, p_gattc_cb, p_cmpl);
        }
        else
        {
            p_gattc_cb->p_curr_srv = NULL;
            *p_cmpl = true;
            return true;
        }
    }

    return false;
}

bool gatt_client_dis_next_desc(uint16_t conn_handle, T_GATTC_STORAGE_CB *p_gattc_cb,
                               T_ATTR_SRV_CB *p_srv_cb, bool start, bool *p_cmpl)
{
    T_GAP_CAUSE  cause;
    T_ATTR_CHAR_CB *p_char_cb = NULL;
    *p_cmpl = false;
    if (p_srv_cb->char_tbl != NULL)
    {
        if (start)
        {
            if (p_gattc_cb->char_list)
            {
                vector_iterator_delete(p_gattc_cb->char_list);
            }
            p_gattc_cb->char_list = vector_iterator_create(p_srv_cb->char_tbl);
        }
    }
    else
    {
        goto next_srv;
    }

    if (p_gattc_cb->char_list == NULL)
    {
        goto failed;
    }

    while (vector_iterator_step(p_gattc_cb->char_list, (VECTOR_ELE *)&p_char_cb))
    {
        if (p_char_cb->char_data.char_uuid16.properties & (GATT_CHAR_PROP_NOTIFY | GATT_CHAR_PROP_INDICATE))
        {
            uint16_t end_handle;
            if (p_gattc_cb->char_list->idx == p_gattc_cb->char_list->size)
            {
                end_handle = p_srv_cb->srv_data.srv_uuid16.end_group_handle;
            }
            else
            {
                T_ATTR_CHAR_CB *p_next_char = NULL;
                p_next_char = p_gattc_cb->char_list->array[p_gattc_cb->char_list->idx];
                end_handle = p_next_char->char_data.char_uuid16.value_handle - 2;
            }
            cause = client_all_char_descriptor_discovery(p_gattc_cb->conn_id, gattc_storage_cl_id,
                                                         p_char_cb->char_data.char_uuid16.value_handle + 1,
                                                         end_handle);
            if (cause == GAP_CAUSE_SUCCESS)
            {
                p_gattc_cb->p_curr_char = p_char_cb;
                goto success;
            }
            else
            {
                goto failed;
            }
        }
    }
next_srv:
    if (p_srv_cb->srv_data.hdr.attr_type == ATTR_TYPE_PRIMARY_SRV_UUID16 ||
        p_srv_cb->srv_data.hdr.attr_type == ATTR_TYPE_PRIMARY_SRV_UUID128)
    {
        return gatt_client_dis_next_service(conn_handle, p_gattc_cb, p_cmpl);
    }
    else
    {
        return gatt_client_dis_next_secondary_service(conn_handle, p_gattc_cb, p_cmpl);
    }
success:
    return true;
failed:
    return false;
}

static void gatt_client_send_dis_result(uint16_t conn_handle, T_GATTC_STORAGE_CB  *p_gattc_cb,
                                        bool load_form_ftl,
                                        bool is_success)
{
    T_SPEC_GATTC_CB *p_spec_cb = NULL;
    PROTOCOL_PRINT_TRACE2("gatt_client_send_dis_result: conn_handle 0x%x, load_form_ftl %d",
                          conn_handle,
                          load_form_ftl);
    if (spec_gattc_cb != NULL)
    {
        VECTOR_ITERATOR gattc_cb_iterator = NULL;
        gattc_cb_iterator = vector_iterator_create(spec_gattc_cb);
        while (vector_iterator_step(gattc_cb_iterator, (VECTOR_ELE *)&p_spec_cb))
        {
            T_GATT_CLIENT_DIS_DONE dis_done = {false, 0};
            if (p_spec_cb->p_fun_cb != NULL && is_success)
            {
                if (p_spec_cb->srv_uuid.is_uuid16)
                {
                    dis_done.srv_instance_num = gattc_storage_get_srv_instance_id(p_gattc_cb, true,
                                                                                  p_spec_cb->srv_uuid.p.uuid16, NULL);
                }
                else
                {
                    dis_done.srv_instance_num = gattc_storage_get_srv_instance_id(p_gattc_cb, false,
                                                                                  0, p_spec_cb->srv_uuid.p.uuid128);
                }
                if (dis_done.srv_instance_num)
                {
                    dis_done.is_found = true;
                }
                dis_done.load_form_ftl = load_form_ftl;
                p_spec_cb->p_fun_cb(conn_handle, GATT_CLIENT_EVENT_DIS_DONE, (void *)&dis_done);
            }
        }
        vector_iterator_delete(gattc_cb_iterator);

    }
    if (p_gattc_cb->p_dis_cb)
    {
        T_GATT_CLIENT_DIS_ALL_DONE dis_all_done;
        dis_all_done.state = p_gattc_cb->state;
        dis_all_done.load_form_ftl = load_form_ftl;
        p_gattc_cb->p_dis_cb(conn_handle, GATT_CLIENT_EVENT_DIS_ALL_STATE, (void *)&dis_all_done);
    }
}

static void gatt_client_discover_state_cb(uint8_t conn_id,  T_DISCOVERY_STATE discovery_state)
{
    bool is_cmpl = false;
    T_GATTC_STORAGE_CB  *p_gattc_cb = gatt_client_find_by_conn_id(conn_id);;
    if (p_gattc_cb == NULL)
    {
        return;
    }

    switch (discovery_state)
    {
    case DISC_STATE_SRV_DONE:
        {
            if (p_gattc_cb->srv_tbl != NULL)
            {
                if (p_gattc_cb->srv_list == NULL)
                {
                    p_gattc_cb->srv_list = vector_iterator_create(p_gattc_cb->srv_tbl);
                    if (p_gattc_cb->srv_list == NULL)
                    {
                        goto failed;
                    }
                }
            }
            else
            {
                goto failed;
            }
            if (gatt_client_dis_next_service(p_gattc_cb->conn_handle, p_gattc_cb, &is_cmpl) == false)
            {
                goto failed;
            }
        }
        break;

    case DISC_STATE_RELATION_DONE:
        {
            T_GAP_CAUSE  cause;
            if (p_gattc_cb->p_curr_srv != NULL)
            {
                cause = client_all_char_discovery(p_gattc_cb->conn_id, gattc_storage_cl_id,
                                                  p_gattc_cb->p_curr_srv->srv_data.srv_uuid16.hdr.att_handle,
                                                  p_gattc_cb->p_curr_srv->srv_data.srv_uuid16.end_group_handle);
                if (cause != GAP_CAUSE_SUCCESS)
                {
                    goto failed;
                }
            }
            else
            {
                goto failed;
            }
        }
        break;

    case DISC_STATE_CHAR_DONE:
        {
            if (p_gattc_cb->p_curr_srv != NULL)
            {
                if (gatt_client_dis_next_desc(p_gattc_cb->conn_handle, p_gattc_cb, p_gattc_cb->p_curr_srv, true,
                                              &is_cmpl) == false)
                {
                    goto failed;
                }
            }
            else
            {
                goto failed;
            }
        }
        break;

    case DISC_STATE_CHAR_DESCRIPTOR_DONE:
        {
            if (p_gattc_cb->p_curr_srv != NULL)
            {
                if (gatt_client_dis_next_desc(p_gattc_cb->conn_handle, p_gattc_cb, p_gattc_cb->p_curr_srv, false,
                                              &is_cmpl) == false)
                {
                    goto failed;
                }
            }
            else
            {
                goto failed;
            }
        }
        break;

    case DISC_STATE_FAILED:
        {
            goto failed;
        }

    default:
        break;
    }

    if (is_cmpl)
    {
        p_gattc_cb->state = GATTC_STORAGE_STATE_DONE;
        PROTOCOL_PRINT_INFO0("GATTC_STORAGE_STATE_DONE");
        if (p_gattc_cb->srv_list)
        {
            vector_iterator_delete(p_gattc_cb->srv_list);
            p_gattc_cb->srv_list = NULL;
        }
        if (p_gattc_cb->char_list)
        {
            vector_iterator_delete(p_gattc_cb->char_list);
            p_gattc_cb->char_list = NULL;
        }
#if LE_AUDIO_DEBUG
        gattc_storage_print(p_gattc_cb);
#endif
        gattc_storage_write(p_gattc_cb);
        gatt_client_send_dis_result(p_gattc_cb->conn_handle, p_gattc_cb, false, true);
    }
    return;
failed:
    {
        uint16_t conn_handle = p_gattc_cb->conn_handle;
        p_gattc_cb->state = GATTC_STORAGE_STATE_FAILED;
        PROTOCOL_PRINT_ERROR0("GATTC_STORAGE_STATE_FAILED");
        if (p_gattc_cb->srv_list)
        {
            vector_iterator_delete(p_gattc_cb->srv_list);
            p_gattc_cb->srv_list = NULL;
        }
        if (p_gattc_cb->char_list)
        {
            vector_iterator_delete(p_gattc_cb->char_list);
            p_gattc_cb->char_list = NULL;
        }
        gattc_storage_release(p_gattc_cb);
        gatt_client_send_dis_result(conn_handle, p_gattc_cb, false, false);
    }
    return;
}

static void gatt_client_discover_result_cb(uint8_t conn_id,
                                           T_DISCOVERY_RESULT_TYPE result_type,
                                           T_DISCOVERY_RESULT_DATA result_data)
{
    T_GATTC_STORAGE_CB  *p_gattc_cb = gatt_client_find_by_conn_id(conn_id);;
    if (p_gattc_cb == NULL)
    {
        return;
    }

    switch (result_type)
    {
    case DISC_RESULT_ALL_SRV_UUID16:
        {
            gattc_storage_add_primary_service(p_gattc_cb, true, &result_data);
        }
        break;

    case DISC_RESULT_ALL_SRV_UUID128:
        {
            gattc_storage_add_primary_service(p_gattc_cb, false, &result_data);
        }
        break;

    case DISC_RESULT_RELATION_UUID16:
        {
            gattc_storage_add_include(p_gattc_cb->p_curr_srv, true, &result_data);
        }
        break;

    case DISC_RESULT_RELATION_UUID128:
        {
            gattc_storage_add_include(p_gattc_cb->p_curr_srv, false, &result_data);
        }
        break;

    case DISC_RESULT_CHAR_UUID16:
        {
            gattc_storage_add_char(p_gattc_cb->p_curr_srv, true, &result_data);
        }
        break;

    case DISC_RESULT_CHAR_UUID128:
        {
            gattc_storage_add_char(p_gattc_cb->p_curr_srv, false, &result_data);
        }
        break;

    case DISC_RESULT_CHAR_DESC_UUID16:
        {
            if (result_data.p_char_desc_uuid16_disc_data->uuid16 == GATT_UUID_CHAR_CLIENT_CONFIG)
            {
                gattc_storage_add_cccd_desc(p_gattc_cb->p_curr_char,
                                            result_data.p_char_desc_uuid16_disc_data->handle);
            }
        }
        break;

    default:
        break;
    }

    return;
}


static void gatt_client_read_result_cb(uint8_t conn_id,  uint16_t cause,
                                       uint16_t handle, uint16_t value_size, uint8_t *p_value)
{
    T_GATTC_STORAGE_CB  *p_gattc_cb = gatt_client_find_by_conn_id(conn_id);
    if (p_gattc_cb == NULL)
    {
        return;
    }
    uint16_t conn_handle = p_gattc_cb->conn_handle;
    PROTOCOL_PRINT_INFO5("gatt_client_read_result_cb: conn_handle 0x%x, cause 0x%x, handle 0x%x, pending_handle 0x%x, pending_uuid16 0x%x",
                         conn_handle, cause, handle, p_gattc_cb->pending_handle,
                         p_gattc_cb->pending_uuid16);
    if (p_gattc_cb->pending_uuid16)
    {
        //rsp for gatt_client_read_uuid
        T_GATT_CLIENT_READ_RESULT read_result;
        read_result.is_cccd_desc = false;
        read_result.cause = cause;
        read_result.handle = handle;
        read_result.value_size = value_size;
        read_result.p_value = p_value;
        read_result.srv_instance_id = 0;
        read_result.char_uuid.is_uuid16 = true;
        read_result.char_uuid.p.uuid16 = p_gattc_cb->pending_uuid16;
        if (p_gattc_cb->pending_req_cb != NULL)
        {
            p_gattc_cb->pending_req_cb(conn_handle, GATT_CLIENT_EVENT_READ_RESULT, (void *)&read_result);
        }
        p_gattc_cb->pending_handle = 0;
        p_gattc_cb->pending_req_cb = NULL;
        p_gattc_cb->pending_uuid16 = 0;
        gatt_client_handle_pending_req(p_gattc_cb);
        return;
    }
    gatt_client_handle_read_result(p_gattc_cb, cause,
                                   handle, value_size, p_value);
    if (p_gattc_cb->pending_handle == handle)
    {
        p_gattc_cb->pending_handle = 0;
        p_gattc_cb->pending_req_cb = NULL;
        gatt_client_handle_pending_req(p_gattc_cb);
    }
    else
    {
        PROTOCOL_PRINT_ERROR2("gatt_client_read_result_cb: failed, pending_handle 0x%x != 0x%x",
                              p_gattc_cb->pending_handle, handle);
    }
}

static void gatt_client_write_result_cb(uint8_t conn_id, T_GATT_WRITE_TYPE type,
                                        uint16_t handle, uint16_t cause,
                                        uint8_t credits)
{
    T_GATTC_STORAGE_CB  *p_gattc_cb = gatt_client_find_by_conn_id(conn_id);
    if (p_gattc_cb == NULL)
    {
        return;
    }
    PROTOCOL_PRINT_INFO5("gatt_client_write_result_cb: conn_id %d, cause 0x%x, handle 0x%x, pending_handle 0x%x, type: %d",
                         conn_id, cause, handle, p_gattc_cb->pending_handle, type);
    if (p_gattc_cb->pending_cccd)
    {
        if (p_gattc_cb->pending_handle == handle)
        {
            T_GAP_CAUSE gap_cause;
            if (cause == GAP_SUCCESS)
            {
                gap_cause = gatt_client_execute_cccd_cfg(p_gattc_cb,
                                                         p_gattc_cb->p_curr_srv, p_gattc_cb->pending_uuid16,
                                                         handle, p_gattc_cb->pending_cccd_enable);
                if (gap_cause == GAP_CAUSE_SUCCESS)
                {
                    return;
                }
                else if (gap_cause != GAP_CAUSE_NOT_FIND)
                {
                    cause = GAP_ERR_REQ_FAILED;
                }
            }
            p_gattc_cb->pending_cccd = false;
            gatt_client_send_cccd_cfg_result(p_gattc_cb,
                                             p_gattc_cb->p_curr_srv, p_gattc_cb->pending_cccd_enable, p_gattc_cb->pending_uuid16, cause);
        }
    }
    else
    {
        gatt_client_handle_write_result(p_gattc_cb, type, handle, cause);
    }

    if (type == GATT_WRITE_TYPE_REQ && p_gattc_cb->pending_handle == handle)
    {
        p_gattc_cb->pending_handle = 0;
        p_gattc_cb->pending_req_cb = NULL;
        gatt_client_handle_pending_req(p_gattc_cb);
    }
    else if (type == GATT_WRITE_TYPE_CMD)
    {
        PROTOCOL_PRINT_INFO1("gatt_client_write_result_cb: write cmd handle 0x%x != 0x%x", handle);
    }
    else
    {
        PROTOCOL_PRINT_ERROR2("gatt_client_write_result_cb: failed, pending_handle 0x%x != 0x%x",
                              p_gattc_cb->pending_handle, handle);
    }

}

static T_APP_RESULT gatt_client_notify_ind_cb(uint8_t conn_id, bool notify,
                                              uint16_t handle,
                                              uint16_t value_size, uint8_t *p_value)
{
    T_APP_RESULT result = APP_RESULT_SUCCESS;
    bool is_cccd_desc;
    T_ATTR_DATA char_data;
    T_SPEC_GATTC_CB *p_spec_cb = NULL;
    T_GATTC_STORAGE_CB  *p_gattc_cb = gatt_client_find_by_conn_id(conn_id);
    T_ATTR_SRV_CB *p_srv_cb = gattc_storage_find_srv_by_handle(p_gattc_cb, handle);
    if (p_srv_cb)
    {
        T_ATTR_UUID srv_uuid;
        att_data_covert_to_uuid(&p_srv_cb->srv_data, &srv_uuid);
        p_spec_cb = gatt_spec_client_find_by_uuid(&srv_uuid);
        if (gattc_storage_find_char_desc(p_srv_cb, handle, &is_cccd_desc, &char_data) == false)
        {
            return APP_RESULT_SUCCESS;
        }
    }
    if (p_spec_cb != NULL && p_spec_cb->p_fun_cb != NULL)
    {
        T_GATT_CLIENT_NOTIFY_IND notify_ind;
        notify_ind.notify = notify;
        notify_ind.handle = handle;
        notify_ind.value_size = value_size;
        notify_ind.p_value = p_value;
        notify_ind.srv_instance_id = p_srv_cb->srv_data.hdr.instance_id;
        att_data_covert_to_uuid(&char_data, &notify_ind.char_uuid);
        result = p_spec_cb->p_fun_cb(p_gattc_cb->conn_handle, GATT_CLIENT_EVENT_NOTIFY_IND,
                                     (void *)&notify_ind);
    }
    return result;
}

const T_FUN_CLIENT_CBS gatt_client_cbs =
{
    gatt_client_discover_state_cb,   //!< Discovery State callback function pointer
    gatt_client_discover_result_cb,  //!< Discovery result callback function pointer
    gatt_client_read_result_cb,     //!< Read response callback function pointer
    gatt_client_write_result_cb,     //!< Write result callback function pointer
    gatt_client_notify_ind_cb,     //!< Notify Indicate callback function pointer
    NULL      //!< Link disconnection callback function pointer
};

void gatt_client_handle_conn_state_evt(uint8_t conn_id, T_GAP_CONN_STATE new_state)
{
    T_GATTC_STORAGE_CB *p_gatt_client;
    if (gattc_storage_cl_id == CLIENT_PROFILE_GENERAL_ID)
    {
        APP_PRINT_ERROR1("gatt_client_handle_conn_state_evt: gattc_storage_cl_id %d", gattc_storage_cl_id);
        return;
    }
    p_gatt_client = gatt_client_find_by_conn_id(conn_id);
    APP_PRINT_ERROR3("gatt_client_handle_conn_state_evt: conn_id %d, new_state %d, p_gatt_client %p",
                     conn_id, new_state, p_gatt_client);
    switch (new_state)
    {
    case GAP_CONN_STATE_DISCONNECTING:
        if (p_gatt_client != NULL)
        {
            p_gatt_client->conn_state = GAP_CONN_STATE_DISCONNECTING;
        }
        break;

    case GAP_CONN_STATE_CONNECTED:
        if (p_gatt_client != NULL)
        {
            p_gatt_client->conn_state = GAP_CONN_STATE_CONNECTED;
            p_gatt_client->conn_handle = le_get_conn_handle(conn_id);
        }
        break;

    case GAP_CONN_STATE_DISCONNECTED:
        if (p_gatt_client != NULL)
        {
            gattc_req_list_free(p_gatt_client);
            gattc_storage_release(p_gatt_client);
            gatt_client_del(p_gatt_client);
        }
        break;

    case GAP_CONN_STATE_CONNECTING:
        if (p_gatt_client == NULL)
        {
            p_gatt_client = gatt_client_allcate(conn_id);
            if (p_gatt_client != NULL)
            {
                p_gatt_client->conn_state = GAP_CONN_STATE_CONNECTING;
            }
        }
        break;

    default:
        break;
    }
}

bool gatt_client_init(void)
{
    if (gattc_storage_cl_id == CLIENT_PROFILE_GENERAL_ID)
    {
        if (false == client_register_spec_client_cb(&gattc_storage_cl_id, &gatt_client_cbs))
        {
            gattc_storage_cl_id = CLIENT_PROFILE_GENERAL_ID;
            PROTOCOL_PRINT_ERROR0("gatt_client_init: register fail");
            return false;
        }
        os_queue_init(&bt_gattc_queue);

        PROTOCOL_PRINT_INFO1("gatt_client_init: client id %d", gattc_storage_cl_id);
        return true;
    }
    return true;
}

bool gatt_storage_register(P_FUN_GATT_STORAGE_CB p_fun_cb)
{
    if (gatt_storage_cb == NULL)
    {
        gatt_storage_cb = p_fun_cb;
        return true;
    }
    return false;
}

T_GAP_CAUSE gatt_spec_client_register(T_ATTR_UUID *p_srv_uuid, P_FUN_GATT_CLIENT_CB p_fun_cb)
{
    T_GAP_CAUSE cause = GAP_CAUSE_INVALID_PARAM;
    T_SPEC_GATTC_CB *p_db;
    if (spec_gattc_cb == NULL)
    {
        spec_gattc_cb = vector_create(GATT_CLIENT_SPEC_CB_NUM);
        if (spec_gattc_cb == NULL)
        {
            cause = GAP_CAUSE_NO_RESOURCE;
            goto error;
        }
    }
    p_db = gatt_spec_client_find_by_uuid(p_srv_uuid);
    if (p_db)
    {
        cause = GAP_CAUSE_ALREADY_IN_REQ;
        goto error;
    }
    else
    {
        p_db = os_mem_zalloc(RAM_TYPE_GATT_CLIENT, sizeof(T_SPEC_GATTC_CB));

        if (p_db == NULL)
        {
            cause = GAP_CAUSE_NO_RESOURCE;
            goto error;
        }
        memcpy(&p_db->srv_uuid, p_srv_uuid, sizeof(T_ATTR_UUID));
        p_db->p_fun_cb = p_fun_cb;
        if (vector_add(spec_gattc_cb, p_db) == false)
        {
            goto error;
        }
    }
    return GAP_CAUSE_SUCCESS;
error:
    PROTOCOL_PRINT_ERROR1("gatt_spec_client_register: failed, cause %d", cause);
    return cause;
}

bool gatt_client_find_char_handle(uint16_t conn_handle, T_ATTR_UUID *p_srv_uuid,
                                  T_ATTR_UUID *p_char_uuid, uint16_t *p_handle)
{
    T_GATTC_STORAGE_CB  *p_gattc_cb = gatt_client_check_link(conn_handle);
    T_ATTR_SRV_CB *p_srv_cb;
    T_ATTR_CHAR_CB *p_char_cb;
    if (p_gattc_cb == NULL || p_gattc_cb->state != GATTC_STORAGE_STATE_DONE)
    {
        goto error;
    }
    p_srv_cb = gattc_storage_find_srv_by_uuid(p_gattc_cb, p_srv_uuid);
    if (p_srv_cb == NULL)
    {
        goto error;
    }
    p_char_cb = gattc_storage_find_char_by_uuid(p_srv_cb, p_char_uuid);
    if (p_char_cb == NULL)
    {
        goto error;
    }
    *p_handle = p_char_cb->char_data.char_uuid16.value_handle;
    return true;
error:
    PROTOCOL_PRINT_ERROR3("gatt_client_find_char_handle: failed, conn_handle 0x%x, srv_uuid 0x%x, char_uuid 0x%x",
                          conn_handle,
                          p_srv_uuid->p.uuid16, p_char_uuid->p.uuid16);
    return false;
}

bool gatt_client_find_primary_srv_by_include(uint16_t conn_handle, T_ATTR_UUID *p_included_srv,
                                             T_ATTR_UUID *p_primary_srv)
{
    T_GATTC_STORAGE_CB  *p_gattc_cb = gatt_client_check_link(conn_handle);
    T_ATTR_SRV_CB *p_srv_cb;
    T_ATTR_SRV_CB *p_inc_srv_cb;
    if (p_gattc_cb == NULL || p_primary_srv == NULL || p_gattc_cb->state != GATTC_STORAGE_STATE_DONE)
    {
        goto error;
    }
    p_srv_cb = gattc_storage_find_srv_by_uuid(p_gattc_cb, p_included_srv);
    if (p_srv_cb == NULL)
    {
        goto error;
    }
    p_inc_srv_cb = gattc_storage_find_inc_srv_by_uuid(p_gattc_cb, p_srv_cb);
    if (p_inc_srv_cb == NULL)
    {
        goto error;
    }
    att_data_covert_to_uuid(&p_inc_srv_cb->srv_data, p_primary_srv);
    return true;
error:
    PROTOCOL_PRINT_ERROR1("gatt_client_find_char_handle: failed, conn_handle 0x%x", conn_handle);
    return false;
}

bool gatt_client_find_include_srv_by_primary(uint16_t conn_handle, T_ATTR_UUID *p_primary_srv,
                                             T_ATTR_UUID *p_included_srv,
                                             T_ATTR_INSTANCE *p_attr_instance)
{
    uint8_t instance_num = 0;
    T_GATTC_STORAGE_CB  *p_gattc_cb = gatt_client_check_link(conn_handle);
    T_ATTR_SRV_CB *p_srv_cb;
    if (p_gattc_cb == NULL || p_primary_srv == NULL || p_included_srv == NULL ||
        p_attr_instance == NULL || p_gattc_cb->state != GATTC_STORAGE_STATE_DONE)
    {
        goto error;
    }
    p_attr_instance->instance_num = 0;
    p_srv_cb = gattc_storage_find_srv_by_uuid(p_gattc_cb, p_primary_srv);
    if (p_srv_cb == NULL)
    {
        goto error;
    }
    if (p_srv_cb->include_srv_tbl != NULL)
    {
        VECTOR_ITERATOR include_tbl_iterator = NULL;
        T_ATTR_DATA *p_include_data = NULL;
        include_tbl_iterator = vector_iterator_create(p_srv_cb->include_srv_tbl);
        while (vector_iterator_step(include_tbl_iterator, (VECTOR_ELE *)&p_include_data))
        {
            if (instance_num == ATTR_INSTANCE_NUM_MAX)
            {
                PROTOCOL_PRINT_ERROR1("gatt_client_find_include_srv_by_primary: instance num is full %d",
                                      instance_num);
                break;
            }
            VECTOR_ITERATOR srv_tbl_iterator = NULL;
            T_ATTR_SRV_CB *p_srv_cb_temp;
            if (p_include_data->hdr.attr_type == ATTR_TYPE_INCLUDE_UUID16 &&
                p_included_srv->is_uuid16 == true &&
                p_include_data->include_uuid16.uuid16 == p_included_srv->p.uuid16)
            {
                srv_tbl_iterator = vector_iterator_create(p_gattc_cb->srv_tbl);
                while (vector_iterator_step(srv_tbl_iterator, (VECTOR_ELE *)&p_srv_cb_temp))
                {
                    if (p_srv_cb_temp->srv_data.hdr.attr_type == ATTR_TYPE_PRIMARY_SRV_UUID16 ||
                        p_srv_cb_temp->srv_data.hdr.attr_type == ATTR_TYPE_SECONDARY_SRV_UUID16)
                    {
                        if (p_include_data->include_uuid16.uuid16 == p_srv_cb_temp->srv_data.srv_uuid16.uuid16 &&
                            p_include_data->include_uuid16.start_handle == p_srv_cb_temp->srv_data.srv_uuid16.hdr.att_handle &&
                            p_include_data->include_uuid16.end_handle == p_srv_cb_temp->srv_data.srv_uuid16.end_group_handle)
                        {
                            p_attr_instance->instance_id[instance_num] = p_srv_cb_temp->srv_data.srv_uuid16.hdr.instance_id;
                            instance_num++;
                            break;
                        }
                    }
                }
            }
            if (p_include_data->hdr.attr_type == ATTR_TYPE_INCLUDE_UUID128 &&
                p_included_srv->is_uuid16 == false &&
                memcmp(p_include_data->include_uuid128.uuid128, p_included_srv->p.uuid128, 16) == 0)
            {
                srv_tbl_iterator = vector_iterator_create(p_gattc_cb->srv_tbl);
                while (vector_iterator_step(srv_tbl_iterator, (VECTOR_ELE *)&p_srv_cb_temp))
                {
                    if (p_srv_cb_temp->srv_data.hdr.attr_type == ATTR_TYPE_PRIMARY_SRV_UUID128 ||
                        p_srv_cb_temp->srv_data.hdr.attr_type == ATTR_TYPE_SECONDARY_SRV_UUID128)
                    {
                        if (p_include_data->include_uuid128.start_handle ==
                            p_srv_cb_temp->srv_data.srv_uuid128.hdr.att_handle
                            &&
                            p_include_data->include_uuid128.end_handle == p_srv_cb_temp->srv_data.srv_uuid128.end_group_handle
                            &&
                            memcmp(p_include_data->include_uuid128.uuid128, p_srv_cb_temp->srv_data.srv_uuid128.uuid128,
                                   16) == 0)
                        {
                            p_attr_instance->instance_id[instance_num] = p_srv_cb_temp->srv_data.srv_uuid128.hdr.instance_id;
                            instance_num++;
                            break;
                        }
                    }
                }
            }
            if (srv_tbl_iterator)
            {
                vector_iterator_delete(srv_tbl_iterator);
            }
        }
        vector_iterator_delete(include_tbl_iterator);
    }
    if (instance_num == 0)
    {
        goto error;
    }
    p_attr_instance->instance_num = instance_num;
    return true;
error:
    return false;
}

uint8_t gatt_client_get_char_num(uint16_t conn_handle, T_ATTR_UUID *p_srv_uuid,
                                 T_ATTR_UUID *p_char_uuid)
{
    T_GATTC_STORAGE_CB  *p_gattc_cb = gatt_client_check_link(conn_handle);
    T_ATTR_SRV_CB *p_srv_cb;
    if (p_gattc_cb == NULL || p_gattc_cb->state != GATTC_STORAGE_STATE_DONE)
    {
        goto error;
    }
    p_srv_cb = gattc_storage_find_srv_by_uuid(p_gattc_cb, p_srv_uuid);
    if (p_srv_cb == NULL)
    {
        goto error;
    }
    return gattc_storage_get_char_instance_id(p_srv_cb, p_char_uuid->is_uuid16, p_char_uuid->p.uuid16,
                                              p_char_uuid->p.uuid128);
error:
    PROTOCOL_PRINT_ERROR1("gatt_client_get_char_num: failed, conn_handle 0x%x", conn_handle);
    return 0;
}

bool gatt_client_get_char_prop(uint16_t conn_handle, T_ATTR_UUID *p_srv_uuid,
                               T_ATTR_UUID *p_char_uuid, uint16_t *p_properties)
{
    T_GATTC_STORAGE_CB  *p_gattc_cb = gatt_client_check_link(conn_handle);
    T_ATTR_SRV_CB *p_srv_cb;
    T_ATTR_CHAR_CB *p_char_cb;
    if (p_gattc_cb == NULL || p_gattc_cb->state != GATTC_STORAGE_STATE_DONE)
    {
        goto error;
    }
    p_srv_cb = gattc_storage_find_srv_by_uuid(p_gattc_cb, p_srv_uuid);
    if (p_srv_cb == NULL)
    {
        goto error;
    }
    p_char_cb = gattc_storage_find_char_by_uuid(p_srv_cb, p_char_uuid);
    if (p_char_cb == NULL)
    {
        goto error;
    }
    *p_properties = p_char_cb->char_data.char_uuid16.properties;
    return true;
error:
    PROTOCOL_PRINT_ERROR1("gatt_client_get_char_prop: failed, conn_handle 0x%x", conn_handle);
    return false;
}
#endif

