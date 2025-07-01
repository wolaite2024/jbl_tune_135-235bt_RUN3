#include <string.h>
#include "trace.h"
#include "os_mem.h"
#include "bt_gattc_storage.h"
#include "vector.h"
#include "gap_conn_le.h"

#if BT_GATT_CLIENT_SUPPORT

uint8_t gattc_storage_get_srv_instance_id(T_GATTC_STORAGE_CB *p_gattc_cb, bool is_uuid16,
                                          uint16_t uuid16, uint8_t *p_uuid128)
{
    uint8_t instance_id = 0;
    VECTOR_ITERATOR srv_tbl_iterator = NULL;
    T_ATTR_SRV_CB *p_srv_cb = NULL;
    if (p_gattc_cb->srv_tbl == NULL)
    {
        return instance_id;
    }
    srv_tbl_iterator = vector_iterator_create(p_gattc_cb->srv_tbl);
    while (vector_iterator_step(srv_tbl_iterator, (VECTOR_ELE *)&p_srv_cb))
    {
        if (is_uuid16)
        {
            if (p_srv_cb->srv_data.hdr.attr_type == ATTR_TYPE_PRIMARY_SRV_UUID16 ||
                p_srv_cb->srv_data.hdr.attr_type == ATTR_TYPE_SECONDARY_SRV_UUID16)
            {
                if (uuid16 == p_srv_cb->srv_data.srv_uuid16.uuid16)
                {
                    instance_id++;
                }
            }
        }
        else
        {
            if (p_srv_cb->srv_data.hdr.attr_type == ATTR_TYPE_PRIMARY_SRV_UUID128 ||
                p_srv_cb->srv_data.hdr.attr_type == ATTR_TYPE_SECONDARY_SRV_UUID128)
            {
                if (memcmp(p_uuid128, p_srv_cb->srv_data.srv_uuid128.uuid128, 16) == 0)
                {
                    instance_id++;
                }
            }
        }
    }
    vector_iterator_delete(srv_tbl_iterator);
    return instance_id;
}

uint8_t gattc_storage_get_char_instance_id(T_ATTR_SRV_CB *p_srv_cb, bool is_uuid16, uint16_t uuid16,
                                           uint8_t *p_uuid128)
{
    uint8_t instance_id = 0;
    VECTOR_ITERATOR char_tbl_iterator = NULL;

    if (p_srv_cb->char_tbl != NULL)
    {
        T_ATTR_CHAR_CB *p_char_data = NULL;
        char_tbl_iterator = vector_iterator_create(p_srv_cb->char_tbl);
        while (vector_iterator_step(char_tbl_iterator, (VECTOR_ELE *)&p_char_data))
        {
            if (is_uuid16)
            {
                if (p_char_data->char_data.hdr.attr_type == ATTR_TYPE_CHAR_UUID16)
                {
                    if (uuid16 == p_char_data->char_data.char_uuid16.uuid16)
                    {
                        instance_id++;
                    }
                }
            }
            else
            {
                if (p_char_data->char_data.hdr.attr_type == ATTR_TYPE_CHAR_UUID128)
                {
                    if (memcmp(p_uuid128, p_char_data->char_data.char_uuid128.uuid128, 16) == 0)
                    {
                        instance_id++;
                    }
                }
            }
        }
        vector_iterator_delete(char_tbl_iterator);
    }
    return instance_id;
}

bool gattc_storage_add_primary_service(T_GATTC_STORAGE_CB *p_gattc_cb, bool uuid16,
                                       T_DISCOVERY_RESULT_DATA *p_result_data)
{
    T_ATTR_SRV_CB *p_srv_cb = NULL;
    if (p_gattc_cb->srv_tbl == NULL)
    {
        p_gattc_cb->srv_tbl = vector_create(GATTC_STORAGE_SRV_MAX_NUM);
        if (p_gattc_cb->srv_tbl == NULL)
        {
            goto error;
        }
    }
    if (uuid16)
    {
        p_srv_cb = os_mem_zalloc(RAM_TYPE_GATT_CLIENT,
                                 sizeof(T_ATTR_SRV_CB) - ATTR_DATA_STRUCT_LEN + sizeof(T_ATTR_SRV_UUID16));
        if (p_srv_cb == NULL)
        {
            goto error;
        }
        p_srv_cb->srv_data.srv_uuid16.hdr.attr_type = ATTR_TYPE_PRIMARY_SRV_UUID16;
        p_srv_cb->srv_data.srv_uuid16.hdr.instance_id = gattc_storage_get_srv_instance_id(p_gattc_cb, true,
                                                        p_result_data->p_srv_uuid16_disc_data->uuid16, NULL);
        p_srv_cb->srv_data.srv_uuid16.hdr.att_handle = p_result_data->p_srv_uuid16_disc_data->att_handle;
        p_srv_cb->srv_data.srv_uuid16.end_group_handle =
            p_result_data->p_srv_uuid16_disc_data->end_group_handle;
        p_srv_cb->srv_data.srv_uuid16.uuid16 = p_result_data->p_srv_uuid16_disc_data->uuid16;
    }
    else
    {
        p_srv_cb = os_mem_zalloc(RAM_TYPE_GATT_CLIENT,
                                 sizeof(T_ATTR_SRV_CB) - ATTR_DATA_STRUCT_LEN + sizeof(T_ATTR_SRV_UUID128));
        if (p_srv_cb == NULL)
        {
            goto error;
        }
        p_srv_cb->srv_data.srv_uuid128.hdr.attr_type = ATTR_TYPE_PRIMARY_SRV_UUID128;
        p_srv_cb->srv_data.srv_uuid128.hdr.instance_id = gattc_storage_get_srv_instance_id(p_gattc_cb,
                                                         false, 0, p_result_data->p_srv_uuid128_disc_data->uuid128);;
        p_srv_cb->srv_data.srv_uuid128.hdr.att_handle = p_result_data->p_srv_uuid128_disc_data->att_handle;
        p_srv_cb->srv_data.srv_uuid128.end_group_handle =
            p_result_data->p_srv_uuid128_disc_data->end_group_handle;
        memcpy(p_srv_cb->srv_data.srv_uuid128.uuid128, p_result_data->p_srv_uuid128_disc_data->uuid128, 16);
    }
    if (vector_add(p_gattc_cb->srv_tbl, p_srv_cb) == false)
    {
        goto error;
    }
    return true;
error:
    PROTOCOL_PRINT_ERROR0("gattc_storage_add_primary_service: failed");
    return false;
}

bool gattc_storage_add_secondary_service(T_GATTC_STORAGE_CB *p_gattc_cb)
{
    bool is_exist = false;
    VECTOR_ITERATOR srv_tbl_iterator = NULL;
    T_ATTR_SRV_CB *p_srv_cb = NULL;
    if (p_gattc_cb->srv_tbl == NULL)
    {
        return is_exist;
    }
    srv_tbl_iterator = vector_iterator_create(p_gattc_cb->srv_tbl);
    while (vector_iterator_step(srv_tbl_iterator, (VECTOR_ELE *)&p_srv_cb))
    {
        VECTOR_ITERATOR include_tbl_iterator = NULL;
        if (p_srv_cb->include_srv_tbl != NULL)
        {
            T_ATTR_DATA *p_include_data = NULL;
            include_tbl_iterator = vector_iterator_create(p_srv_cb->include_srv_tbl);
            while (vector_iterator_step(include_tbl_iterator, (VECTOR_ELE *)&p_include_data))
            {
                bool is_new = true;
                VECTOR_ITERATOR srv_tbl_temp = NULL;
                T_ATTR_SRV_CB *p_srv_temp = NULL;
                srv_tbl_temp = vector_iterator_create(p_gattc_cb->srv_tbl);
                while (vector_iterator_step(srv_tbl_temp, (VECTOR_ELE *)&p_srv_temp))
                {
                    if (p_include_data->hdr.attr_type == ATTR_TYPE_INCLUDE_UUID16 &&
                        (p_srv_temp->srv_data.hdr.attr_type == ATTR_TYPE_PRIMARY_SRV_UUID16 ||
                         p_srv_temp->srv_data.hdr.attr_type == ATTR_TYPE_SECONDARY_SRV_UUID16))
                    {
                        if (p_include_data->include_uuid16.uuid16 == p_srv_temp->srv_data.srv_uuid16.uuid16 &&
                            p_include_data->include_uuid16.start_handle == p_srv_temp->srv_data.srv_uuid16.hdr.att_handle &&
                            p_include_data->include_uuid16.end_handle == p_srv_temp->srv_data.srv_uuid16.end_group_handle)
                        {
                            is_new = false;
                            break;
                        }
                    }
                    if (p_include_data->hdr.attr_type == ATTR_TYPE_INCLUDE_UUID128 &&
                        (p_srv_temp->srv_data.hdr.attr_type == ATTR_TYPE_PRIMARY_SRV_UUID128 ||
                         p_srv_temp->srv_data.hdr.attr_type == ATTR_TYPE_SECONDARY_SRV_UUID128))
                    {
                        if (p_include_data->include_uuid128.start_handle == p_srv_temp->srv_data.srv_uuid128.hdr.att_handle
                            &&
                            p_include_data->include_uuid128.end_handle == p_srv_temp->srv_data.srv_uuid128.end_group_handle &&
                            memcmp(p_include_data->include_uuid128.uuid128, p_srv_temp->srv_data.srv_uuid128.uuid128, 16) == 0)
                        {
                            is_new = false;
                            break;
                        }
                    }
                }
                vector_iterator_delete(srv_tbl_temp);
                if (is_new)
                {
                    if (p_include_data->hdr.attr_type == ATTR_TYPE_INCLUDE_UUID16)
                    {
                        p_srv_temp = os_mem_zalloc(RAM_TYPE_GATT_CLIENT,
                                                   sizeof(T_ATTR_SRV_CB) - ATTR_DATA_STRUCT_LEN + sizeof(T_ATTR_SRV_UUID16));
                        if (p_srv_temp == NULL)
                        {
                            break;
                        }
                        p_srv_temp->srv_data.srv_uuid16.hdr.attr_type = ATTR_TYPE_SECONDARY_SRV_UUID16;
                        p_srv_temp->srv_data.srv_uuid16.hdr.instance_id = gattc_storage_get_srv_instance_id(p_gattc_cb,
                                                                          true, p_include_data->include_uuid16.uuid16, NULL);
                        p_srv_temp->srv_data.srv_uuid16.hdr.att_handle = p_include_data->include_uuid16.start_handle;
                        p_srv_temp->srv_data.srv_uuid16.end_group_handle =
                            p_include_data->include_uuid16.end_handle;
                        p_srv_temp->srv_data.srv_uuid16.uuid16 = p_include_data->include_uuid16.uuid16;
                    }
                    else if (p_include_data->hdr.attr_type == ATTR_TYPE_INCLUDE_UUID128)
                    {
                        p_srv_temp = os_mem_zalloc(RAM_TYPE_GATT_CLIENT,
                                                   sizeof(T_ATTR_SRV_CB) - ATTR_DATA_STRUCT_LEN + sizeof(T_ATTR_SRV_UUID128));
                        if (p_srv_temp == NULL)
                        {
                            break;
                        }
                        p_srv_temp->srv_data.srv_uuid128.hdr.attr_type = ATTR_TYPE_SECONDARY_SRV_UUID128;
                        p_srv_temp->srv_data.srv_uuid128.hdr.instance_id = gattc_storage_get_srv_instance_id(p_gattc_cb,
                                                                           false, 0, p_include_data->include_uuid128.uuid128);
                        p_srv_temp->srv_data.srv_uuid128.hdr.att_handle = p_include_data->include_uuid128.start_handle;
                        p_srv_temp->srv_data.srv_uuid128.end_group_handle =
                            p_include_data->include_uuid128.end_handle;
                        memcpy(p_srv_temp->srv_data.srv_uuid128.uuid128, p_include_data->include_uuid128.uuid128, 16);
                    }
                    if (vector_add(p_gattc_cb->srv_tbl, p_srv_temp) == false)
                    {
                        break;
                    }
                    is_exist = true;
                }
            }
            vector_iterator_delete(include_tbl_iterator);
        }
    }
    vector_iterator_delete(srv_tbl_iterator);
    return is_exist;
}

bool gattc_storage_add_include(T_ATTR_SRV_CB *p_srv_cb, bool uuid16,
                               T_DISCOVERY_RESULT_DATA *p_result_data)
{
    if (p_srv_cb->include_srv_tbl == NULL)
    {
        p_srv_cb->include_srv_tbl = vector_create(GATTC_STORAGE_INCLUDE_MAX_NUM);
        if (p_srv_cb->include_srv_tbl == NULL)
        {
            goto error;
        }
    }
    if (uuid16)
    {
        T_ATTR_INCLUDE_UUID16 *p_include16 = os_mem_zalloc(RAM_TYPE_GATT_CLIENT,
                                                           sizeof(T_ATTR_INCLUDE_UUID16));
        if (p_include16 == NULL)
        {
            goto error;
        }
        p_include16->hdr.attr_type = ATTR_TYPE_INCLUDE_UUID16;
        p_include16->hdr.instance_id = 0;
        p_include16->hdr.att_handle = p_result_data->p_relation_uuid16_disc_data->decl_handle;
        p_include16->start_handle = p_result_data->p_relation_uuid16_disc_data->att_handle;
        p_include16->end_handle = p_result_data->p_relation_uuid16_disc_data->end_group_handle;
        p_include16->uuid16 = p_result_data->p_relation_uuid16_disc_data->uuid16;
        if (vector_add(p_srv_cb->include_srv_tbl, p_include16) == false)
        {
            goto error;
        }
    }
    else
    {
        T_ATTR_INCLUDE_UUID128 *p_include128 = os_mem_zalloc(RAM_TYPE_GATT_CLIENT,
                                                             sizeof(T_ATTR_INCLUDE_UUID128));
        if (p_include128 == NULL)
        {
            goto error;
        }
        p_include128->hdr.attr_type = ATTR_TYPE_INCLUDE_UUID128;
        p_include128->hdr.instance_id = 0;
        p_include128->hdr.att_handle = p_result_data->p_relation_uuid128_disc_data->decl_handle;
        p_include128->start_handle = p_result_data->p_relation_uuid128_disc_data->att_handle;
        p_include128->end_handle = p_result_data->p_relation_uuid128_disc_data->end_group_handle;
        memcpy(p_include128->uuid128, p_result_data->p_relation_uuid128_disc_data->uuid128, 16);
        if (vector_add(p_srv_cb->include_srv_tbl, p_include128) == false)
        {
            goto error;
        }
    }
    return true;
error:
    PROTOCOL_PRINT_ERROR0("gattc_storage_add_include: failed");
    return false;
}

bool gattc_storage_add_char(T_ATTR_SRV_CB *p_srv_cb, bool uuid16,
                            T_DISCOVERY_RESULT_DATA *p_result_data)
{
    T_ATTR_CHAR_CB *p_char_cb = NULL;
    if (p_srv_cb->char_tbl == NULL)
    {
        p_srv_cb->char_tbl = vector_create(GATTC_STORAGE_CHAR_MAX_NUM);
        if (p_srv_cb->char_tbl == NULL)
        {
            goto error;
        }
    }
    if (uuid16)
    {
        p_char_cb = os_mem_zalloc(RAM_TYPE_GATT_CLIENT,
                                  sizeof(T_ATTR_CHAR_CB) - ATTR_DATA_STRUCT_LEN + sizeof(T_ATTR_CHAR_UUID16));
        if (p_char_cb == NULL)
        {
            goto error;
        }
        p_char_cb->char_data.char_uuid16.hdr.attr_type = ATTR_TYPE_CHAR_UUID16;
        p_char_cb->char_data.char_uuid16.hdr.instance_id = gattc_storage_get_char_instance_id(p_srv_cb,
                                                           true, p_result_data->p_char_uuid16_disc_data->uuid16, NULL);
        p_char_cb->char_data.char_uuid16.hdr.att_handle =
            p_result_data->p_char_uuid16_disc_data->decl_handle;
        p_char_cb->char_data.char_uuid16.properties = p_result_data->p_char_uuid16_disc_data->properties;
        p_char_cb->char_data.char_uuid16.value_handle =
            p_result_data->p_char_uuid16_disc_data->value_handle;
        p_char_cb->char_data.char_uuid16.uuid16 = p_result_data->p_char_uuid16_disc_data->uuid16;
    }
    else
    {
        p_char_cb = os_mem_zalloc(RAM_TYPE_GATT_CLIENT,
                                  sizeof(T_ATTR_CHAR_CB) - ATTR_DATA_STRUCT_LEN + sizeof(T_ATTR_CHAR_UUID128));
        if (p_char_cb == NULL)
        {
            goto error;
        }
        p_char_cb->char_data.char_uuid128.hdr.attr_type = ATTR_TYPE_CHAR_UUID128;
        p_char_cb->char_data.char_uuid128.hdr.instance_id = gattc_storage_get_char_instance_id(p_srv_cb,
                                                            false, 0, p_result_data->p_char_uuid128_disc_data->uuid128);
        p_char_cb->char_data.char_uuid128.hdr.att_handle =
            p_result_data->p_char_uuid128_disc_data->decl_handle;
        p_char_cb->char_data.char_uuid128.properties = p_result_data->p_char_uuid128_disc_data->properties;
        p_char_cb->char_data.char_uuid128.value_handle =
            p_result_data->p_char_uuid128_disc_data->value_handle;
        memcpy(p_char_cb->char_data.char_uuid128.uuid128, p_result_data->p_char_uuid128_disc_data->uuid128,
               16);
    }
    if (vector_add(p_srv_cb->char_tbl, p_char_cb) == false)
    {
        goto error;
    }
    return true;
error:
    PROTOCOL_PRINT_ERROR0("gattc_storage_add_char: failed");
    return false;
}

bool gattc_storage_add_cccd_desc(T_ATTR_CHAR_CB *p_char_cb, uint16_t handle)
{
    if (p_char_cb != NULL)
    {
        p_char_cb->cccd_descriptor.hdr.attr_type = ATTR_TYPE_CCCD_DESC;
        p_char_cb->cccd_descriptor.hdr.att_handle = handle;
        return true;
    }
    return false;
}

void gattc_storage_init(T_GATTC_STORAGE_CB *p_gattc_cb)
{
    p_gattc_cb->state = GATTC_STORAGE_STATE_IDLE;
    p_gattc_cb->p_curr_srv = NULL;
    p_gattc_cb->p_curr_char = NULL;
}

void gattc_storage_release(T_GATTC_STORAGE_CB *p_gattc_cb)
{
    VECTOR_ITERATOR srv_tbl_iterator = NULL;
    T_ATTR_SRV_CB *p_srv_cb = NULL;
    if (p_gattc_cb->srv_tbl == NULL)
    {
        return;
    }
    srv_tbl_iterator = vector_iterator_create(p_gattc_cb->srv_tbl);
    while (vector_iterator_step(srv_tbl_iterator, (VECTOR_ELE *)&p_srv_cb))
    {
        VECTOR_ITERATOR include_tbl_iterator = NULL;
        VECTOR_ITERATOR char_tbl_iterator = NULL;

        if (p_srv_cb->include_srv_tbl != NULL)
        {
            T_ATTR_DATA *p_include_data = NULL;
            include_tbl_iterator = vector_iterator_create(p_srv_cb->include_srv_tbl);
            while (vector_iterator_step(include_tbl_iterator, (VECTOR_ELE *)&p_include_data))
            {
                os_mem_free(p_include_data);
            }
            vector_iterator_delete(include_tbl_iterator);
            vector_delete(p_srv_cb->include_srv_tbl);
        }
        if (p_srv_cb->char_tbl != NULL)
        {
            T_ATTR_DATA *p_char_data = NULL;
            char_tbl_iterator = vector_iterator_create(p_srv_cb->char_tbl);
            while (vector_iterator_step(char_tbl_iterator, (VECTOR_ELE *)&p_char_data))
            {
                os_mem_free(p_char_data);
            }
            vector_iterator_delete(char_tbl_iterator);
            vector_delete(p_srv_cb->char_tbl);
        }
        os_mem_free(p_srv_cb);
    }
    vector_iterator_delete(srv_tbl_iterator);
    vector_delete(p_gattc_cb->srv_tbl);
    p_gattc_cb->srv_tbl = NULL;
    gattc_storage_init(p_gattc_cb);
}

bool gattc_storage_refresh(T_GATTC_STORAGE_CB *p_gattc_cb)
{
    PROTOCOL_PRINT_INFO1("gattc_storage_refresh: conn_handle 0x%x", p_gattc_cb->conn_handle);
    gattc_storage_release(p_gattc_cb);
    return true;
}

void gattc_storage_print(T_GATTC_STORAGE_CB *p_gattc_cb)
{
    VECTOR_ITERATOR srv_tbl_iterator = NULL;
    T_ATTR_SRV_CB *p_srv_cb = NULL;
    if (p_gattc_cb->srv_tbl == NULL)
    {
        return;
    }
    srv_tbl_iterator = vector_iterator_create(p_gattc_cb->srv_tbl);
    while (vector_iterator_step(srv_tbl_iterator, (VECTOR_ELE *)&p_srv_cb))
    {
        VECTOR_ITERATOR include_tbl_iterator = NULL;
        VECTOR_ITERATOR char_tbl_iterator = NULL;

        if (p_srv_cb->srv_data.hdr.attr_type == ATTR_TYPE_PRIMARY_SRV_UUID16)
        {
            PROTOCOL_PRINT_INFO4("Primary Service: handle[0x%x - 0x%x], inst[%d], uuid16[0x%x]",
                                 p_srv_cb->srv_data.srv_uuid16.hdr.att_handle,
                                 p_srv_cb->srv_data.srv_uuid16.end_group_handle,
                                 p_srv_cb->srv_data.srv_uuid16.hdr.instance_id,
                                 p_srv_cb->srv_data.srv_uuid16.uuid16);
        }
        else if (p_srv_cb->srv_data.hdr.attr_type == ATTR_TYPE_PRIMARY_SRV_UUID128)
        {
            PROTOCOL_PRINT_INFO4("Primary Service: handle[0x%x - 0x%x], inst[%d], uuid128[%b]",
                                 p_srv_cb->srv_data.srv_uuid128.hdr.att_handle,
                                 p_srv_cb->srv_data.srv_uuid128.end_group_handle,
                                 p_srv_cb->srv_data.srv_uuid128.hdr.instance_id,
                                 TRACE_BINARY(16, p_srv_cb->srv_data.srv_uuid128.uuid128));
        }
        else if (p_srv_cb->srv_data.hdr.attr_type == ATTR_TYPE_SECONDARY_SRV_UUID16)
        {
            PROTOCOL_PRINT_INFO4("Secondary Service: handle[0x%x - 0x%x], inst[%d], uuid16[0x%x]",
                                 p_srv_cb->srv_data.srv_uuid16.hdr.att_handle,
                                 p_srv_cb->srv_data.srv_uuid16.end_group_handle,
                                 p_srv_cb->srv_data.srv_uuid16.hdr.instance_id,
                                 p_srv_cb->srv_data.srv_uuid16.uuid16);
        }
        else if (p_srv_cb->srv_data.hdr.attr_type == ATTR_TYPE_SECONDARY_SRV_UUID128)
        {
            PROTOCOL_PRINT_INFO4("Secondary Service: handle[0x%x - 0x%x], inst[%d], uuid128[%b]",
                                 p_srv_cb->srv_data.srv_uuid128.hdr.att_handle,
                                 p_srv_cb->srv_data.srv_uuid128.end_group_handle,
                                 p_srv_cb->srv_data.srv_uuid128.hdr.instance_id,
                                 TRACE_BINARY(16, p_srv_cb->srv_data.srv_uuid128.uuid128));
        }

        if (p_srv_cb->include_srv_tbl != NULL)
        {
            T_ATTR_DATA *p_include_data = NULL;
            include_tbl_iterator = vector_iterator_create(p_srv_cb->include_srv_tbl);
            while (vector_iterator_step(include_tbl_iterator, (VECTOR_ELE *)&p_include_data))
            {
                if (p_include_data->hdr.attr_type == ATTR_TYPE_INCLUDE_UUID16)
                {
                    PROTOCOL_PRINT_INFO5("\tInclude: decl handle[0x%x], handle[0x%x - 0x%x], inst[%d], uuid16[0x%x]",
                                         p_include_data->include_uuid16.hdr.att_handle,
                                         p_include_data->include_uuid16.start_handle,
                                         p_include_data->include_uuid16.end_handle,
                                         p_include_data->include_uuid16.hdr.instance_id,
                                         p_include_data->include_uuid16.uuid16);
                }
                else if (p_include_data->hdr.attr_type == ATTR_TYPE_INCLUDE_UUID128)
                {
                    PROTOCOL_PRINT_INFO5("\tInclude: decl handle[0x%x], handle[0x%x - 0x%x], inst[%d], uuid128[%b]",
                                         p_include_data->include_uuid128.hdr.att_handle,
                                         p_include_data->include_uuid128.start_handle,
                                         p_include_data->include_uuid128.end_handle,
                                         p_include_data->include_uuid128.hdr.instance_id,
                                         TRACE_BINARY(16, p_include_data->include_uuid128.uuid128));
                }
            }
            vector_iterator_delete(include_tbl_iterator);
        }
        if (p_srv_cb->char_tbl != NULL)
        {
            T_ATTR_CHAR_CB *p_char_data = NULL;
            char_tbl_iterator = vector_iterator_create(p_srv_cb->char_tbl);
            while (vector_iterator_step(char_tbl_iterator, (VECTOR_ELE *)&p_char_data))
            {
                if (p_char_data->char_data.hdr.attr_type == ATTR_TYPE_CHAR_UUID16)
                {
                    PROTOCOL_PRINT_INFO5("\tChar: decl handle[0x%x], value handle[0x%x], inst[%d], properties[0x%x], uuid16[0x%x]",
                                         p_char_data->char_data.char_uuid16.hdr.att_handle,
                                         p_char_data->char_data.char_uuid16.value_handle,
                                         p_char_data->char_data.char_uuid16.hdr.instance_id,
                                         p_char_data->char_data.char_uuid16.properties,
                                         p_char_data->char_data.char_uuid16.uuid16);
                }
                else if (p_char_data->char_data.hdr.attr_type == ATTR_TYPE_CHAR_UUID128)
                {
                    PROTOCOL_PRINT_INFO5("\tChar: decl handle[0x%x], value handle[0x%x], inst[%d], properties[0x%x], uuid128[%b]",
                                         p_char_data->char_data.char_uuid128.hdr.att_handle,
                                         p_char_data->char_data.char_uuid128.value_handle,
                                         p_char_data->char_data.char_uuid128.hdr.instance_id,
                                         p_char_data->char_data.char_uuid128.properties,
                                         TRACE_BINARY(16, p_char_data->char_data.char_uuid128.uuid128));
                }
                if (p_char_data->cccd_descriptor.hdr.att_handle != 0)
                {
                    PROTOCOL_PRINT_INFO1("\t\tCCCD Desc: handle[0x%x]",
                                         p_char_data->cccd_descriptor.hdr.att_handle);
                }
            }
            vector_iterator_delete(char_tbl_iterator);
        }
    }
    vector_iterator_delete(srv_tbl_iterator);
}

T_ATTR_SRV_CB *gattc_storage_find_srv_by_handle(T_GATTC_STORAGE_CB *p_gattc_cb, uint16_t handle)
{
    VECTOR_ITERATOR srv_tbl_iterator = NULL;
    T_ATTR_SRV_CB *p_srv_cb = NULL;
    if (p_gattc_cb == NULL || p_gattc_cb->srv_tbl == NULL)
    {
        goto error;
    }
    srv_tbl_iterator = vector_iterator_create(p_gattc_cb->srv_tbl);
    while (vector_iterator_step(srv_tbl_iterator, (VECTOR_ELE *)&p_srv_cb))
    {
        if (handle >= p_srv_cb->srv_data.srv_uuid16.hdr.att_handle &&
            handle <= p_srv_cb->srv_data.srv_uuid16.end_group_handle)
        {
            vector_iterator_delete(srv_tbl_iterator);
            return p_srv_cb;
        }
    }
    vector_iterator_delete(srv_tbl_iterator);
error:
    PROTOCOL_PRINT_ERROR1("gattc_storage_find_srv_by_handle: failed, handle 0x%x", handle);
    return NULL;
}

bool gattc_storage_find_char_desc(T_ATTR_SRV_CB *p_srv_cb, uint16_t handle, bool *p_is_cccd,
                                  T_ATTR_DATA *p_char_data)
{
    bool is_found = false;
    VECTOR_ITERATOR char_tbl_iterator = NULL;
    if (p_srv_cb && p_srv_cb->char_tbl != NULL && handle != 0)
    {
        T_ATTR_CHAR_CB *p_attr_char = NULL;
        char_tbl_iterator = vector_iterator_create(p_srv_cb->char_tbl);
        while (vector_iterator_step(char_tbl_iterator, (VECTOR_ELE *)&p_attr_char))
        {
            if (handle == p_attr_char->char_data.char_uuid16.value_handle)
            {
                *p_is_cccd = false;
                memcpy(p_char_data, &p_attr_char->char_data, sizeof(T_ATTR_DATA));
                is_found = true;
                break;
            }
            if (handle == p_attr_char->cccd_descriptor.hdr.att_handle)
            {
                *p_is_cccd = true;
                memcpy(p_char_data,  &p_attr_char->char_data, sizeof(T_ATTR_DATA));
                is_found = true;
                break;
            }
        }
        vector_iterator_delete(char_tbl_iterator);
    }
    if (is_found == false)
    {
        PROTOCOL_PRINT_ERROR1("gattc_storage_find_char_desc: failed, handle 0x%x", handle);
    }
    return is_found;
}

T_ATTR_SRV_CB *gattc_storage_find_srv_by_uuid(T_GATTC_STORAGE_CB *p_gattc_cb,
                                              T_ATTR_UUID *p_attr_uuid)
{
    VECTOR_ITERATOR srv_tbl_iterator = NULL;
    T_ATTR_SRV_CB *p_srv_cb = NULL;
    T_ATTR_SRV_CB *p_result = NULL;
    if (p_gattc_cb == NULL || p_gattc_cb->srv_tbl == NULL || p_attr_uuid == NULL)
    {
        return NULL;
    }
    srv_tbl_iterator = vector_iterator_create(p_gattc_cb->srv_tbl);
    while (vector_iterator_step(srv_tbl_iterator, (VECTOR_ELE *)&p_srv_cb))
    {
        if (p_attr_uuid->is_uuid16)
        {
            if (p_srv_cb->srv_data.hdr.attr_type == ATTR_TYPE_PRIMARY_SRV_UUID16 ||
                p_srv_cb->srv_data.hdr.attr_type == ATTR_TYPE_SECONDARY_SRV_UUID16)
            {
                if ((p_attr_uuid->instance_id == p_srv_cb->srv_data.hdr.instance_id) &&
                    (p_attr_uuid->p.uuid16 == p_srv_cb->srv_data.srv_uuid16.uuid16))
                {
                    p_result = p_srv_cb;
                    break;
                }
            }
        }
        else
        {
            if (p_srv_cb->srv_data.hdr.attr_type == ATTR_TYPE_PRIMARY_SRV_UUID128 ||
                p_srv_cb->srv_data.hdr.attr_type == ATTR_TYPE_SECONDARY_SRV_UUID128)
            {
                if ((p_attr_uuid->instance_id == p_srv_cb->srv_data.hdr.instance_id) &&
                    memcmp(p_attr_uuid->p.uuid128, p_srv_cb->srv_data.srv_uuid128.uuid128, 16) == 0)
                {
                    p_result = p_srv_cb;
                    break;
                }
            }
        }
    }
    vector_iterator_delete(srv_tbl_iterator);
    return p_result;
}

T_ATTR_SRV_CB *gattc_storage_find_inc_srv_by_uuid(T_GATTC_STORAGE_CB *p_gattc_cb,
                                                  T_ATTR_SRV_CB *p_inc_srv)
{
    VECTOR_ITERATOR srv_tbl_iterator = NULL;
    T_ATTR_SRV_CB *p_srv_cb = NULL;
    T_ATTR_SRV_CB *p_result = NULL;
    if (p_gattc_cb == NULL || p_gattc_cb->srv_tbl == NULL || p_inc_srv == NULL)
    {
        return NULL;
    }
    srv_tbl_iterator = vector_iterator_create(p_gattc_cb->srv_tbl);
    while (vector_iterator_step(srv_tbl_iterator, (VECTOR_ELE *)&p_srv_cb))
    {
        if (p_srv_cb->include_srv_tbl != NULL)
        {
            VECTOR_ITERATOR include_tbl_iterator = NULL;
            T_ATTR_DATA *p_include_data = NULL;
            include_tbl_iterator = vector_iterator_create(p_srv_cb->include_srv_tbl);
            while (vector_iterator_step(include_tbl_iterator, (VECTOR_ELE *)&p_include_data))
            {
                if (p_include_data->hdr.attr_type == ATTR_TYPE_INCLUDE_UUID16 &&
                    (p_inc_srv->srv_data.hdr.attr_type == ATTR_TYPE_PRIMARY_SRV_UUID16 ||
                     p_inc_srv->srv_data.hdr.attr_type == ATTR_TYPE_SECONDARY_SRV_UUID16))
                {
                    if (p_include_data->include_uuid16.uuid16 == p_inc_srv->srv_data.srv_uuid16.uuid16 &&
                        p_include_data->include_uuid16.start_handle == p_inc_srv->srv_data.srv_uuid16.hdr.att_handle &&
                        p_include_data->include_uuid16.end_handle == p_inc_srv->srv_data.srv_uuid16.end_group_handle)
                    {
                        p_result = p_srv_cb;
                        break;
                    }
                }
                if (p_include_data->hdr.attr_type == ATTR_TYPE_INCLUDE_UUID128 &&
                    (p_inc_srv->srv_data.hdr.attr_type == ATTR_TYPE_PRIMARY_SRV_UUID128 ||
                     p_inc_srv->srv_data.hdr.attr_type == ATTR_TYPE_SECONDARY_SRV_UUID128))
                {
                    if (p_include_data->include_uuid128.start_handle == p_inc_srv->srv_data.srv_uuid128.hdr.att_handle
                        &&
                        p_include_data->include_uuid128.end_handle == p_inc_srv->srv_data.srv_uuid128.end_group_handle &&
                        memcmp(p_include_data->include_uuid128.uuid128, p_inc_srv->srv_data.srv_uuid128.uuid128, 16) == 0)
                    {
                        p_result = p_srv_cb;
                        break;
                    }
                }
            }
            vector_iterator_delete(include_tbl_iterator);
        }
        if (p_result)
        {
            break;
        }
    }
    vector_iterator_delete(srv_tbl_iterator);
    return p_result;
}

T_ATTR_CHAR_CB *gattc_storage_find_char_by_uuid(T_ATTR_SRV_CB *p_srv_cb, T_ATTR_UUID *p_attr_uuid)
{
    VECTOR_ITERATOR char_tbl_iterator = NULL;
    T_ATTR_CHAR_CB *p_result = NULL;
    if (p_srv_cb && p_srv_cb->char_tbl != NULL && p_attr_uuid != NULL)
    {
        T_ATTR_CHAR_CB *p_attr_char = NULL;
        char_tbl_iterator = vector_iterator_create(p_srv_cb->char_tbl);
        while (vector_iterator_step(char_tbl_iterator, (VECTOR_ELE *)&p_attr_char))
        {
            if (p_attr_uuid->is_uuid16)
            {
                if (p_attr_char->char_data.hdr.attr_type == ATTR_TYPE_CHAR_UUID16)
                {
                    if ((p_attr_uuid->instance_id == p_attr_char->char_data.hdr.instance_id) &&
                        (p_attr_uuid->p.uuid16 == p_attr_char->char_data.char_uuid16.uuid16))
                    {
                        p_result = p_attr_char;
                        break;
                    }
                }
            }
            else
            {
                if (p_attr_char->char_data.hdr.attr_type == ATTR_TYPE_CHAR_UUID128)
                {
                    if ((p_attr_uuid->instance_id == p_attr_char->char_data.hdr.instance_id) &&
                        memcmp(p_attr_uuid->p.uuid128, p_attr_char->char_data.char_uuid128.uuid128, 16) == 0)
                    {
                        p_result = p_attr_char;
                        break;
                    }
                }
            }
        }
        vector_iterator_delete(char_tbl_iterator);
    }
    return p_result;
}

bool gattc_storage_check_prop(T_GATTC_STORAGE_CB *p_gattc_cb, uint16_t handle,
                              uint16_t properties_bit)
{
    T_ATTR_SRV_CB *p_srv_cb = gattc_storage_find_srv_by_handle(p_gattc_cb, handle);
    T_ATTR_DATA char_data;
    bool is_cccd;
    if (p_srv_cb == NULL)
    {
        goto error;
    }
    if (gattc_storage_find_char_desc(p_srv_cb, handle, &is_cccd, &char_data))
    {
        if (is_cccd)
        {
            if ((GATT_CHAR_PROP_WRITE | GATT_CHAR_PROP_READ) & properties_bit)
            {
                return true;
            }
        }
        else
        {
            if (char_data.char_uuid16.properties & properties_bit)
            {
                return true;
            }
        }
    }
error:
    PROTOCOL_PRINT_ERROR2("gattc_storage_check_prop: failed, handle 0x%x, properties_bit 0x%x", handle,
                          properties_bit);
    return false;
}

void att_data_covert_to_uuid(T_ATTR_DATA *p_attr_data, T_ATTR_UUID *p_attr_uuid)
{
    p_attr_uuid->instance_id = p_attr_data->hdr.instance_id;
    if (p_attr_data->hdr.attr_type == ATTR_TYPE_PRIMARY_SRV_UUID16 ||
        p_attr_data->hdr.attr_type == ATTR_TYPE_SECONDARY_SRV_UUID16)
    {
        p_attr_uuid->is_uuid16 = true;
        p_attr_uuid->p.uuid16 = p_attr_data->srv_uuid16.uuid16;
    }
    else if (p_attr_data->hdr.attr_type == ATTR_TYPE_PRIMARY_SRV_UUID128 ||
             p_attr_data->hdr.attr_type == ATTR_TYPE_SECONDARY_SRV_UUID128)
    {
        p_attr_uuid->is_uuid16 = false;
        memcpy(p_attr_uuid->p.uuid128, p_attr_data->srv_uuid128.uuid128, 16);
    }
    else if (p_attr_data->hdr.attr_type == ATTR_TYPE_CHAR_UUID16)
    {
        p_attr_uuid->is_uuid16 = true;
        p_attr_uuid->p.uuid16 = p_attr_data->char_uuid16.uuid16;
    }
    else if (p_attr_data->hdr.attr_type == ATTR_TYPE_CHAR_UUID128)
    {
        p_attr_uuid->is_uuid16 = false;
        memcpy(p_attr_uuid->p.uuid128, p_attr_data->char_uuid128.uuid128, 16);
    }
    else if (p_attr_data->hdr.attr_type == ATTR_TYPE_INCLUDE_UUID16)
    {
        p_attr_uuid->is_uuid16 = true;
        p_attr_uuid->p.uuid16 = p_attr_data->include_uuid16.uuid16;
    }
    else if (p_attr_data->hdr.attr_type == ATTR_TYPE_INCLUDE_UUID128)
    {
        p_attr_uuid->is_uuid16 = false;
        memcpy(p_attr_uuid->p.uuid128, p_attr_data->include_uuid128.uuid128, 16);
    }
    else if (p_attr_data->hdr.attr_type == ATTR_TYPE_CCCD_DESC)
    {
        p_attr_uuid->is_uuid16 = true;
        p_attr_uuid->p.uuid16 = GATT_UUID_CHAR_CLIENT_CONFIG;
    }
}

uint8_t gattc_get_attr_data_len(uint8_t type)
{
    uint8_t len = 0;
    switch (type)
    {
    case ATTR_TYPE_PRIMARY_SRV_UUID16:
    case ATTR_TYPE_SECONDARY_SRV_UUID16:
        {
            len = sizeof(T_ATTR_SRV_UUID16);
        }
        break;

    case ATTR_TYPE_PRIMARY_SRV_UUID128:
    case ATTR_TYPE_SECONDARY_SRV_UUID128:
        {
            len = sizeof(T_ATTR_SRV_UUID128);
        }
        break;

    case ATTR_TYPE_INCLUDE_UUID16:
        {
            len = sizeof(T_ATTR_INCLUDE_UUID16);
        }
        break;

    case ATTR_TYPE_INCLUDE_UUID128:
        {
            len = sizeof(T_ATTR_INCLUDE_UUID128);
        }
        break;

    case ATTR_TYPE_CHAR_UUID16:
        {
            len = sizeof(T_ATTR_CHAR_UUID16);
        }
        break;

    case ATTR_TYPE_CHAR_UUID128:
        {
            len = sizeof(T_ATTR_CHAR_UUID128);
        }
        break;

    case ATTR_TYPE_CCCD_DESC:
        {
            len = sizeof(T_ATTR_CCCD_DESC);
        }
        break;

    default:
        break;
    }
    return len;
}

bool gattc_storage_load_attr_data(T_GATTC_STORAGE_CB *p_gattc_cb, uint8_t len, uint8_t *p_value)
{
    uint8_t error_idx = 0;
    T_ATTR_HEADER header;
    uint8_t check_len;

    if (len == 0)
    {
        return true;
    }
    memcpy(&header, p_value, sizeof(header));

    check_len = gattc_get_attr_data_len(header.attr_type);
    if (check_len != len)
    {
        error_idx = 1;
        goto error;
    }
    switch (header.attr_type)
    {
    case ATTR_TYPE_PRIMARY_SRV_UUID16:
    case ATTR_TYPE_SECONDARY_SRV_UUID16:
    case ATTR_TYPE_PRIMARY_SRV_UUID128:
    case ATTR_TYPE_SECONDARY_SRV_UUID128:
        {
            T_ATTR_SRV_CB *p_srv_cb = NULL;
            if (p_gattc_cb->srv_tbl == NULL)
            {
                p_gattc_cb->srv_tbl = vector_create(GATTC_STORAGE_SRV_MAX_NUM);
                if (p_gattc_cb->srv_tbl == NULL)
                {
                    error_idx = 2;
                    goto error;
                }
            }
            if (header.attr_type == ATTR_TYPE_PRIMARY_SRV_UUID16 ||
                header.attr_type == ATTR_TYPE_SECONDARY_SRV_UUID16)
            {
                p_srv_cb = os_mem_zalloc(RAM_TYPE_GATT_CLIENT,
                                         sizeof(T_ATTR_SRV_CB) - ATTR_DATA_STRUCT_LEN + sizeof(T_ATTR_SRV_UUID16));
                if (p_srv_cb == NULL)
                {
                    error_idx = 3;
                    goto error;
                }
                memcpy(&p_srv_cb->srv_data, p_value, sizeof(T_ATTR_SRV_UUID16));
            }
            else
            {
                p_srv_cb = os_mem_zalloc(RAM_TYPE_GATT_CLIENT,
                                         sizeof(T_ATTR_SRV_CB) - ATTR_DATA_STRUCT_LEN + sizeof(T_ATTR_SRV_UUID128));
                if (p_srv_cb == NULL)
                {
                    error_idx = 4;
                    goto error;
                }
                memcpy(&p_srv_cb->srv_data, p_value, sizeof(T_ATTR_SRV_UUID128));
            }
            if (vector_add(p_gattc_cb->srv_tbl, p_srv_cb) == false)
            {
                error_idx = 5;
                goto error;
            }
            p_gattc_cb->p_curr_srv = p_srv_cb;
        }
        break;

    case ATTR_TYPE_INCLUDE_UUID16:
    case ATTR_TYPE_INCLUDE_UUID128:
        {
            T_ATTR_SRV_CB *p_srv_cb = p_gattc_cb->p_curr_srv;
            if (p_gattc_cb->p_curr_srv == NULL)
            {
                error_idx = 6;
                goto error;
            }
            if (p_srv_cb->include_srv_tbl == NULL)
            {
                p_srv_cb->include_srv_tbl = vector_create(GATTC_STORAGE_INCLUDE_MAX_NUM);
                if (p_srv_cb->include_srv_tbl == NULL)
                {
                    error_idx = 7;
                    goto error;
                }
            }
            if (header.attr_type == ATTR_TYPE_INCLUDE_UUID16)
            {
                T_ATTR_INCLUDE_UUID16 *p_include16 = os_mem_zalloc(RAM_TYPE_GATT_CLIENT,
                                                                   sizeof(T_ATTR_INCLUDE_UUID16));
                if (p_include16 == NULL)
                {
                    error_idx = 8;
                    goto error;
                }
                memcpy(p_include16, p_value, sizeof(T_ATTR_INCLUDE_UUID16));
                if (vector_add(p_srv_cb->include_srv_tbl, p_include16) == false)
                {
                    error_idx = 9;
                    goto error;
                }
            }
            else
            {
                T_ATTR_INCLUDE_UUID128 *p_include128 = os_mem_zalloc(RAM_TYPE_GATT_CLIENT,
                                                                     sizeof(T_ATTR_INCLUDE_UUID128));
                if (p_include128 == NULL)
                {
                    error_idx = 10;
                    goto error;
                }
                memcpy(p_include128, p_value, sizeof(T_ATTR_INCLUDE_UUID128));
                if (vector_add(p_srv_cb->include_srv_tbl, p_include128) == false)
                {
                    error_idx = 11;
                    goto error;
                }
            }
        }
        break;

    case ATTR_TYPE_CHAR_UUID16:
    case ATTR_TYPE_CHAR_UUID128:
        {
            T_ATTR_CHAR_CB *p_char_cb = NULL;
            T_ATTR_SRV_CB *p_srv_cb = p_gattc_cb->p_curr_srv;
            if (p_gattc_cb->p_curr_srv == NULL)
            {
                error_idx = 12;
                goto error;
            }
            if (p_srv_cb->char_tbl == NULL)
            {
                p_srv_cb->char_tbl = vector_create(GATTC_STORAGE_CHAR_MAX_NUM);
                if (p_srv_cb->char_tbl == NULL)
                {
                    error_idx = 13;
                    goto error;
                }
            }
            if (header.attr_type == ATTR_TYPE_CHAR_UUID16)
            {
                p_char_cb = os_mem_zalloc(RAM_TYPE_GATT_CLIENT,
                                          sizeof(T_ATTR_CHAR_CB) - ATTR_DATA_STRUCT_LEN + sizeof(T_ATTR_CHAR_UUID16));
                if (p_char_cb == NULL)
                {
                    error_idx = 14;
                    goto error;
                }
                memcpy(&p_char_cb->char_data, p_value, sizeof(T_ATTR_CHAR_UUID16));
            }
            else
            {
                p_char_cb = os_mem_zalloc(RAM_TYPE_GATT_CLIENT,
                                          sizeof(T_ATTR_CHAR_CB) - ATTR_DATA_STRUCT_LEN + sizeof(T_ATTR_CHAR_UUID128));
                if (p_char_cb == NULL)
                {
                    error_idx = 15;
                    goto error;
                }
                memcpy(&p_char_cb->char_data, p_value, sizeof(T_ATTR_CHAR_UUID128));
            }
            if (vector_add(p_srv_cb->char_tbl, p_char_cb) == false)
            {
                error_idx = 16;
                goto error;
            }
            p_gattc_cb->p_curr_char = p_char_cb;
        }
        break;

    case ATTR_TYPE_CCCD_DESC:
        {
            if (p_gattc_cb->p_curr_char == NULL)
            {
                error_idx = 17;
                goto error;
            }
            memcpy(&p_gattc_cb->p_curr_char->cccd_descriptor, p_value, sizeof(T_ATTR_CCCD_DESC));
        }
        break;

    default:
        break;
    }
    return true;
error:
    PROTOCOL_PRINT_ERROR2("gattc_storage_load_attr_data: failed, attr_type %d, error_idx %d",
                          header.attr_type, error_idx);
    return false;
}

bool gattc_storage_load(T_GATTC_STORAGE_CB *p_gattc_cb)
{
    uint8_t error_idx = 0;
    T_GATT_STORATE_SRV_TBL_GET_IND get_ind;
    uint8_t *p_temp_buf = NULL;
    uint16_t offset = 0;
    uint8_t len = 0;
    T_GAP_CONN_INFO conn_info;

    if (p_gattc_cb->srv_tbl != NULL)
    {
        error_idx = 1;
        goto error1;
    }

    if (le_get_conn_info(p_gattc_cb->conn_id, &conn_info) == false)
    {
        error_idx = 2;
        goto error1;
    }

    if (gatt_storage_cb)
    {
        memset(&get_ind, 0, sizeof(get_ind));
        get_ind.remote_bd_type = conn_info.remote_bd_type;
        memcpy(get_ind.addr, conn_info.remote_bd, 6);
        if (gatt_storage_cb(GATT_STORATE_EVENT_SRV_TBL_GET_IND, &get_ind) != APP_RESULT_SUCCESS)
        {
            error_idx = 3;
            goto error1;
        }
    }
    else
    {
        error_idx = 4;
        goto error1;
    }

    p_temp_buf = get_ind.p_data;
    if (p_temp_buf == NULL)
    {
        error_idx = 5;
        goto error;
    }

    for (; offset < get_ind.data_len;)
    {
        len = p_temp_buf[offset];
        offset++;
        if (len > ATTR_DATA_STRUCT_LEN)
        {
            error_idx = 6;
            goto error;
        }
        //PROTOCOL_PRINT_INFO3("offset %d, data[%d] %b", offset, len, TRACE_BINARY(len, p_temp_buf + offset));
        if (gattc_storage_load_attr_data(p_gattc_cb, len, p_temp_buf + offset) == false)
        {
            error_idx = 7;
            goto error;
        }
        offset += len;
    }
    if (offset != get_ind.data_len)
    {
        error_idx = 8;
        goto error;
    }
    if (p_temp_buf)
    {
        os_mem_free(p_temp_buf);
    }
    p_gattc_cb->state = GATTC_STORAGE_STATE_DONE;
    PROTOCOL_PRINT_INFO0("gattc_storage_load: success");
    return true;
error1:
    PROTOCOL_PRINT_ERROR1("gattc_storage_load: failed 1, error_idx %d", error_idx);
    return false;
error:
    if (p_temp_buf)
    {
        os_mem_free(p_temp_buf);
    }
    PROTOCOL_PRINT_ERROR1("gattc_storage_load: failed 2, error_idx %d", error_idx);
    gattc_storage_release(p_gattc_cb);
    return false;
}

bool gattc_block_save(uint8_t *p_write_buf, uint16_t *p_offset, uint8_t attr_type,
                      uint8_t *p_value)
{
    uint16_t offset;
    uint8_t len = gattc_get_attr_data_len(attr_type);

    if (p_write_buf == NULL || p_offset == NULL || p_value == NULL)
    {
        return false;
    }
    offset = *p_offset;

    p_write_buf[offset] = len;
    offset++;
    memcpy(p_write_buf + offset, p_value, len);
    offset += len;
    *p_offset = offset;
    return true;
}

bool gattc_storage_write(T_GATTC_STORAGE_CB *p_gattc_cb)
{
    bool ret = false;
    uint8_t error_idx = 0;
    T_GATT_STORATE_SRV_TBL_SET_IND set_ind;
    uint16_t write_len = 0;
    uint16_t offset = 0;
    uint8_t *p_write_buf = NULL;
    VECTOR_ITERATOR srv_tbl_iterator = NULL;
    VECTOR_ITERATOR char_tbl_iterator = NULL;
    VECTOR_ITERATOR include_tbl_iterator = NULL;
    T_ATTR_SRV_CB *p_srv_cb = NULL;
    T_GAP_CONN_INFO conn_info;

    memset(&set_ind, 0, sizeof(set_ind));

    if (p_gattc_cb == NULL || p_gattc_cb->srv_tbl == NULL)
    {
        error_idx = 1;
        goto result;
    }

    if (p_gattc_cb->state != GATTC_STORAGE_STATE_DONE)
    {
        error_idx = 2;
        goto result;
    }

    if (gatt_storage_cb == NULL)
    {
        error_idx = 3;
        goto result;
    }

    if (le_get_conn_info(p_gattc_cb->conn_id, &conn_info))
    {
        set_ind.remote_bd_type = conn_info.remote_bd_type;
        memcpy(set_ind.addr, conn_info.remote_bd, 6);
    }
    else
    {
        error_idx = 4;
        goto result;
    }

    srv_tbl_iterator = vector_iterator_create(p_gattc_cb->srv_tbl);
    if (srv_tbl_iterator == NULL)
    {
        error_idx = 5;
        goto result;
    }
    while (vector_iterator_step(srv_tbl_iterator, (VECTOR_ELE *)&p_srv_cb))
    {
        write_len += gattc_get_attr_data_len(p_srv_cb->srv_data.hdr.attr_type);
        write_len += 1;
        if (p_srv_cb->include_srv_tbl != NULL)
        {
            T_ATTR_DATA *p_include_data = NULL;
            include_tbl_iterator = vector_iterator_create(p_srv_cb->include_srv_tbl);
            if (include_tbl_iterator == NULL)
            {
                error_idx = 6;
                goto result;
            }
            while (vector_iterator_step(include_tbl_iterator, (VECTOR_ELE *)&p_include_data))
            {
                write_len += gattc_get_attr_data_len(p_include_data->hdr.attr_type);
                write_len += 1;
            }
            vector_iterator_delete(include_tbl_iterator);
            include_tbl_iterator = NULL;
        }
        if (p_srv_cb->char_tbl != NULL)
        {
            T_ATTR_CHAR_CB *p_attr_char = NULL;
            char_tbl_iterator = vector_iterator_create(p_srv_cb->char_tbl);
            if (char_tbl_iterator == NULL)
            {
                error_idx = 7;
                goto result;
            }
            while (vector_iterator_step(char_tbl_iterator, (VECTOR_ELE *)&p_attr_char))
            {
                write_len += gattc_get_attr_data_len(p_attr_char->char_data.hdr.attr_type);
                write_len += 1;
                if (p_attr_char->cccd_descriptor.hdr.att_handle != 0)
                {
                    write_len += gattc_get_attr_data_len(ATTR_TYPE_CCCD_DESC);
                    write_len += 1;
                }
            }
            vector_iterator_delete(char_tbl_iterator);
            char_tbl_iterator = NULL;
        }
    }
    vector_iterator_delete(srv_tbl_iterator);
    srv_tbl_iterator = NULL;

    if ((write_len % 4) != 0)
    {
        write_len = (write_len / 4 + 1) * 4;
    }
    p_write_buf = os_mem_zalloc(RAM_TYPE_GATT_CLIENT, write_len);

    if (p_write_buf == NULL)
    {
        error_idx = 8;
        goto result;
    }

    srv_tbl_iterator = vector_iterator_create(p_gattc_cb->srv_tbl);
    if (srv_tbl_iterator == NULL)
    {
        error_idx = 9;
        goto result;
    }
    while (vector_iterator_step(srv_tbl_iterator, (VECTOR_ELE *)&p_srv_cb))
    {
        if (gattc_block_save(p_write_buf, &offset, p_srv_cb->srv_data.hdr.attr_type,
                             (uint8_t *)&p_srv_cb->srv_data) == false)
        {
            error_idx = 10;
            goto result;
        }
        if (p_srv_cb->include_srv_tbl != NULL)
        {
            T_ATTR_DATA *p_include_data = NULL;
            include_tbl_iterator = vector_iterator_create(p_srv_cb->include_srv_tbl);
            if (include_tbl_iterator == NULL)
            {
                error_idx = 11;
                goto result;
            }
            while (vector_iterator_step(include_tbl_iterator, (VECTOR_ELE *)&p_include_data))
            {
                if (gattc_block_save(p_write_buf, &offset, p_include_data->hdr.attr_type,
                                     (uint8_t *)p_include_data) == false)
                {
                    error_idx = 12;
                    goto result;
                }
            }
            vector_iterator_delete(include_tbl_iterator);
            include_tbl_iterator = NULL;
        }
        if (p_srv_cb->char_tbl != NULL)
        {
            T_ATTR_CHAR_CB *p_attr_char = NULL;
            char_tbl_iterator = vector_iterator_create(p_srv_cb->char_tbl);
            if (char_tbl_iterator == NULL)
            {
                error_idx = 13;
                goto result;
            }
            while (vector_iterator_step(char_tbl_iterator, (VECTOR_ELE *)&p_attr_char))
            {
                if (gattc_block_save(p_write_buf, &offset, p_attr_char->char_data.hdr.attr_type,
                                     (uint8_t *)&p_attr_char->char_data) == false)
                {
                    error_idx = 14;
                    goto result;
                }
                if (p_attr_char->cccd_descriptor.hdr.att_handle != 0)
                {
                    if (gattc_block_save(p_write_buf, &offset, ATTR_TYPE_CCCD_DESC,
                                         (uint8_t *)&p_attr_char->cccd_descriptor) == false)
                    {
                        error_idx = 15;
                        goto result;
                    }
                }
            }
            vector_iterator_delete(char_tbl_iterator);
            char_tbl_iterator = NULL;
        }
    }
    vector_iterator_delete(srv_tbl_iterator);
    srv_tbl_iterator = NULL;

    set_ind.data_len = write_len;
    set_ind.p_data = p_write_buf;
    if (gatt_storage_cb(GATT_STORATE_EVENT_SRV_TBL_SET_IND, &set_ind) != APP_RESULT_SUCCESS)
    {
        error_idx = 16;
        goto result;
    }

    ret = true;
    PROTOCOL_PRINT_INFO2("gattc_storage_write: write_len %d, offset %d", write_len, offset);
result:
    if (srv_tbl_iterator != NULL)
    {
        vector_iterator_delete(srv_tbl_iterator);
    }
    if (include_tbl_iterator != NULL)
    {
        vector_iterator_delete(include_tbl_iterator);
    }
    if (char_tbl_iterator != NULL)
    {
        vector_iterator_delete(char_tbl_iterator);
    }
    if (p_write_buf != NULL)
    {
        os_mem_free(p_write_buf);
    }
    if (ret == false)
    {
        PROTOCOL_PRINT_ERROR1("gattc_storage_write: failed, error_idx %d", error_idx);
    }
    return ret;
}
#endif


