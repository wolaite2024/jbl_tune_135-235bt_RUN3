#include "trace.h"
#include "os_mem.h"
#include "ftl.h"
#include "crc16btx.h"
#include "bt_gatt_client.h"
#include "gattc_tbl_storage.h"

#if GATTC_TBL_STORAGE_SUPPORT
#include <string.h>
#include "gap_bond_le.h"
#define GATTC_FTL_SYNC_WORD 0x55335533
#define GATTC_FTL_MAX_DEV_NUM   2

#define GATTC_FTL_MAX_BLOCK_LEN BT_EXT_FTL_BLOCK_LEN
#define GATTC_FTL_MAX_BLOCK_NUM 16
#define GATTC_FTL_CB_LEN GATTC_FTL_MAX_BLOCK_LEN
#define GATTC_FTL_MAX_BLOCK_MASK ((1 << ((GATTC_FTL_MAX_BLOCK_LEN * GATTC_FTL_MAX_BLOCK_NUM -GATTC_FTL_CB_LEN)/GATTC_FTL_MAX_BLOCK_LEN)) -1)

typedef struct
{
    uint8_t  used: 1;
    uint8_t  reserved: 7;
    uint8_t  remote_bd_type;
    uint8_t  addr[6];
    uint16_t total_len;
    uint16_t crc16;
    uint32_t block_used_bitmap;
} T_GATTC_FTL_DEV;

typedef struct
{
    uint32_t sync_word;
    uint32_t block_unused_bitmap;
    T_GATTC_FTL_DEV dev_tbl[GATTC_FTL_MAX_DEV_NUM];
} T_GATTC_FTL_CB;

typedef union
{
    uint8_t data[GATTC_FTL_CB_LEN];
    T_GATTC_FTL_CB ftl_cb;
} T_GATTC_FTL_HEADER;

bool gattc_tbl_get_identity_addr(uint8_t *bd_addr, uint8_t bd_type,
                                 uint8_t *identity_addr, uint8_t *identity_addr_type)
{
    uint8_t err_idx = 0;
    T_LE_KEY_ENTRY *p_key_entry = NULL;
    if (bd_type == GAP_REMOTE_ADDR_LE_RANDOM)
    {
        if ((bd_addr[5] & RANDOM_ADDR_MASK) == RANDOM_ADDR_MASK_RESOLVABLE)
        {
            if (le_resolve_random_address(bd_addr, identity_addr, identity_addr_type) == false)
            {
                err_idx = 1;
                goto failed;
            }
            else
            {
                return true;
            }
        }
        else if ((bd_addr[5] & RANDOM_ADDR_MASK) == RANDOM_ADDR_MASK_NON_RESOLVABLE)
        {
            err_idx = 2;
            goto failed;
        }
    }

    p_key_entry = le_find_key_entry(bd_addr, (T_GAP_REMOTE_ADDR_TYPE)bd_type);
    if (p_key_entry == NULL)
    {
        err_idx = 3;
        goto failed;
    }
    if ((bd_type == GAP_REMOTE_ADDR_LE_PUBLIC_IDENTITY)
        || (bd_type == GAP_REMOTE_ADDR_LE_RANDOM_IDENTITY))
    {
        bd_type -= 2;
    }
    *identity_addr_type = bd_type;
    memcpy(identity_addr, bd_addr, 6);
    return true;
failed:
    PROTOCOL_PRINT_ERROR3("gattc_tbl_get_identity_addr: bd_type %d, bd_addr %s, err_idx %d", bd_type,
                          TRACE_BDADDR(bd_addr), err_idx);
    return false;
}

void gattc_tbl_storage_print(void)
{
    T_GATTC_FTL_HEADER *p_ftl_header = NULL;
    p_ftl_header = os_mem_zalloc(RAM_TYPE_DATA_ON, sizeof(T_GATTC_FTL_HEADER));

    if (p_ftl_header)
    {
        if (ftl_load_from_storage(p_ftl_header, g_bt_ext_ftl_start_offset,
                                  sizeof(T_GATTC_FTL_HEADER)) != 0)
        {
            PROTOCOL_PRINT_INFO0("gattc_tbl_storage_print: load header failed");
        }
        else
        {
            if (p_ftl_header->ftl_cb.sync_word == GATTC_FTL_SYNC_WORD)
            {
                PROTOCOL_PRINT_INFO1("gattc_tbl_storage_print: block_unused_bitmap 0x%x",
                                     p_ftl_header->ftl_cb.block_unused_bitmap);
                for (uint8_t i = 0; i < GATTC_FTL_MAX_DEV_NUM; i++)
                {
                    if (p_ftl_header->ftl_cb.dev_tbl[i].used)
                    {
                        PROTOCOL_PRINT_INFO6("dev_tbl[%d]: remote_bd_type %d, addr %s, total_len %d, crc16 0x%x, block_used_bitmap 0x%x",
                                             i,
                                             p_ftl_header->ftl_cb.dev_tbl[i].remote_bd_type,
                                             TRACE_BDADDR(p_ftl_header->ftl_cb.dev_tbl[i].addr),
                                             p_ftl_header->ftl_cb.dev_tbl[i].total_len,
                                             p_ftl_header->ftl_cb.dev_tbl[i].crc16,
                                             p_ftl_header->ftl_cb.dev_tbl[i].block_used_bitmap);
                    }
                }
            }
            else
            {
                PROTOCOL_PRINT_INFO0("gattc_tbl_storage_print: no info");
            }
        }
    }
    if (p_ftl_header)
    {
        os_mem_free(p_ftl_header);
    }
}

T_GATTC_FTL_DEV *gattc_tbl_storage_find_dev(T_GATTC_FTL_CB *p_ftl_cb,
                                            uint8_t *identity_addr, uint8_t identity_addr_type)
{
    if (p_ftl_cb->sync_word != GATTC_FTL_SYNC_WORD)
    {
        return NULL;
    }
    for (uint8_t i = 0; i < GATTC_FTL_MAX_DEV_NUM; i++)
    {
        if (p_ftl_cb->dev_tbl[i].used && p_ftl_cb->dev_tbl[i].remote_bd_type == identity_addr_type &&
            memcmp(p_ftl_cb->dev_tbl[i].addr, identity_addr, 6) == 0)
        {
            return &p_ftl_cb->dev_tbl[i];
        }
    }
    return NULL;
}

T_GATTC_FTL_DEV *gattc_tbl_storage_find_unused_dev(T_GATTC_FTL_CB *p_ftl_cb)
{
    if (p_ftl_cb->sync_word != GATTC_FTL_SYNC_WORD)
    {
        return NULL;
    }
    for (uint8_t i = 0; i < GATTC_FTL_MAX_DEV_NUM; i++)
    {
        if (p_ftl_cb->dev_tbl[i].used == false)
        {
            return &p_ftl_cb->dev_tbl[i];
        }
    }
    return NULL;
}

bool gattc_tbl_storage_clear(void)
{
    T_GATTC_FTL_HEADER ftl_header;
    memset(&ftl_header, 0, sizeof(ftl_header));
    if (ftl_save_to_storage(&ftl_header, g_bt_ext_ftl_start_offset, sizeof(ftl_header)) != 0)
    {
        return false;
    }
    PROTOCOL_PRINT_INFO0("gattc_tbl_storage_clear");
    return true;
}

bool gattc_tbl_storage_remove(uint8_t *bd_addr, uint8_t bd_type)
{
    T_GATTC_FTL_HEADER ftl_header;
    T_GATTC_FTL_DEV *p_ftl_dev = NULL;
    uint8_t  identity_addr[6];
    uint8_t  identity_addr_type;
    if (gattc_tbl_get_identity_addr(bd_addr, bd_type,
                                    identity_addr, &identity_addr_type) == false)
    {
        goto failed;
    }
    if (ftl_load_from_storage(&ftl_header, g_bt_ext_ftl_start_offset, sizeof(ftl_header)) != 0)
    {
        goto failed;
    }
    p_ftl_dev = gattc_tbl_storage_find_dev(&ftl_header.ftl_cb, identity_addr, identity_addr_type);
    if (p_ftl_dev)
    {
        ftl_header.ftl_cb.block_unused_bitmap |= p_ftl_dev->block_used_bitmap;
        memset(p_ftl_dev, 0, sizeof(T_GATTC_FTL_DEV));
    }
    else
    {
        goto failed;
    }
    if (ftl_save_to_storage(&ftl_header, g_bt_ext_ftl_start_offset, sizeof(ftl_header)) != 0)
    {
        goto failed;
    }
    PROTOCOL_PRINT_INFO2("gattc_tbl_storage_remove: bd_type %d, bd_addr %s", bd_type,
                         TRACE_BDADDR(bd_addr));
    return true;
failed:
    return false;
}

bool gattc_tbl_storage_get_dev(T_GATT_STORATE_SRV_TBL_GET_IND *p_get_ind)
{
    T_GATTC_FTL_HEADER *p_ftl_header = NULL;
    T_GATTC_FTL_DEV *p_ftl_dev = NULL;
    uint8_t error_idx = 0;
    uint8_t *p_temp_buf = NULL;
    uint16_t crc16 = 0;
    uint16_t read_len;
    uint16_t read_idx = 0;
    uint32_t block_used_bitmap;
    uint16_t block_idx = 0;
    uint8_t  identity_addr[6];
    uint8_t  identity_addr_type;
    if (gattc_tbl_get_identity_addr(p_get_ind->addr, p_get_ind->remote_bd_type,
                                    identity_addr, &identity_addr_type) == false)
    {
        error_idx = 1;
        goto failed;
    }
    p_ftl_header = os_mem_zalloc(RAM_TYPE_DATA_ON, sizeof(T_GATTC_FTL_HEADER));

    if (p_ftl_header == NULL)
    {
        error_idx = 2;
        goto failed;
    }
    if (ftl_load_from_storage(p_ftl_header, g_bt_ext_ftl_start_offset,
                              sizeof(T_GATTC_FTL_HEADER)) != 0)
    {
        error_idx = 3;
        goto failed;
    }
    p_ftl_dev = gattc_tbl_storage_find_dev(&p_ftl_header->ftl_cb, identity_addr, identity_addr_type);
    if (p_ftl_dev == NULL)
    {
        error_idx = 4;
        goto failed;
    }
    read_len = p_ftl_dev->total_len;
    if ((read_len % GATTC_FTL_MAX_BLOCK_LEN) != 0)
    {
        read_len = (read_len / GATTC_FTL_MAX_BLOCK_LEN + 1) * GATTC_FTL_MAX_BLOCK_LEN;
    }

    p_temp_buf = os_mem_zalloc(RAM_TYPE_DATA_ON, read_len);

    if (p_temp_buf == NULL)
    {
        error_idx = 5;
        goto failed;
    }
    block_used_bitmap = p_ftl_dev->block_used_bitmap;
    while (block_used_bitmap)
    {
        if (block_used_bitmap & 0x01)
        {
            if (ftl_load_from_storage(p_temp_buf + read_idx,
                                      g_bt_ext_ftl_start_offset + GATTC_FTL_CB_LEN + GATTC_FTL_MAX_BLOCK_LEN * block_idx,
                                      GATTC_FTL_MAX_BLOCK_LEN) != 0)
            {
                error_idx = 6;
                goto failed;
            }
            else
            {
                read_idx += GATTC_FTL_MAX_BLOCK_LEN;
                if (read_idx > read_len)
                {
                    error_idx = 7;
                    goto failed;
                }
            }
        }
        block_used_bitmap >>= 1;
        block_idx++;
    }

    if (read_idx != read_len)
    {
        error_idx = 8;
        goto failed;
    }

    crc16 = btxfcs(crc16, p_temp_buf, p_ftl_dev->total_len);

    if (p_ftl_dev->crc16 != crc16)
    {
        error_idx = 9;
        goto failed;
    }
    p_get_ind->data_len = p_ftl_dev->total_len;
    p_get_ind->p_data = p_temp_buf;
    if (p_ftl_header != NULL)
    {
        os_mem_free(p_ftl_header);
    }
    return true;
failed:
    PROTOCOL_PRINT_ERROR1("gattc_tbl_storage_get_dev:failed, error_idx %d", error_idx);
    if (p_temp_buf != NULL)
    {
        os_mem_free(p_temp_buf);
    }
    if (p_ftl_header != NULL)
    {
        os_mem_free(p_ftl_header);
    }
    return false;
}

bool gattc_tbl_storage_add_dev(T_GATT_STORATE_SRV_TBL_SET_IND *p_set_ind)
{
    uint8_t error_idx = 0;
    uint16_t crc16 = 0;
    T_GATTC_FTL_HEADER *p_ftl_header = NULL;
    T_GATTC_FTL_DEV *p_ftl_dev = NULL;
    uint8_t block_num;
    uint16_t write_len;
    uint16_t write_idx = 0;
    uint32_t block_used_bitmap;
    uint32_t block_unused_bitmap;
    uint8_t block_idx = 0;
    uint8_t block_assign_num = 0;
    uint8_t  identity_addr[6];
    uint8_t  identity_addr_type;
    if (gattc_tbl_get_identity_addr(p_set_ind->addr, p_set_ind->remote_bd_type,
                                    identity_addr, &identity_addr_type) == false)
    {
        error_idx = 1;
        goto failed;
    }

    p_ftl_header = os_mem_zalloc(RAM_TYPE_DATA_ON, sizeof(T_GATTC_FTL_HEADER));

    if (p_ftl_header == NULL)
    {
        error_idx = 2;
        goto failed;
    }
    crc16 = btxfcs(crc16, p_set_ind->p_data, p_set_ind->data_len);
    ftl_load_from_storage(p_ftl_header, g_bt_ext_ftl_start_offset, sizeof(T_GATTC_FTL_HEADER));
    if (p_ftl_header->ftl_cb.sync_word != GATTC_FTL_SYNC_WORD)
    {
        memset(p_ftl_header, 0, sizeof(T_GATTC_FTL_HEADER));
        p_ftl_header->ftl_cb.sync_word = GATTC_FTL_SYNC_WORD;
        p_ftl_header->ftl_cb.block_unused_bitmap = GATTC_FTL_MAX_BLOCK_MASK;
    }
    else
    {
        p_ftl_dev = gattc_tbl_storage_find_dev(&p_ftl_header->ftl_cb, identity_addr, identity_addr_type);
        if (p_ftl_dev != NULL)
        {
            if (p_ftl_dev->crc16 == crc16 && p_ftl_dev->total_len == p_set_ind->data_len)
            {
                PROTOCOL_PRINT_INFO0("gattc_tbl_storage_add_dev:already exist");
                if (p_ftl_header != NULL)
                {
                    os_mem_free(p_ftl_header);
                }
                return true;
            }
            else
            {
                PROTOCOL_PRINT_INFO0("gattc_tbl_storage_add_dev: table changed");
                p_ftl_header->ftl_cb.block_unused_bitmap |= p_ftl_dev->block_used_bitmap;
                memset(p_ftl_dev, 0, sizeof(T_GATTC_FTL_DEV));
            }
        }
    }
    p_ftl_dev = gattc_tbl_storage_find_unused_dev(&p_ftl_header->ftl_cb);
    if (p_ftl_dev == NULL)
    {
        error_idx = 3;
        goto failed;
    }
    p_ftl_dev->used = true;
    p_ftl_dev->remote_bd_type = identity_addr_type;
    memcpy(p_ftl_dev->addr, identity_addr, 6);
    p_ftl_dev->total_len = p_set_ind->data_len;
    p_ftl_dev->crc16 = crc16;
    p_ftl_dev->block_used_bitmap = 0;

    if ((p_set_ind->data_len % GATTC_FTL_MAX_BLOCK_LEN) != 0)
    {
        block_num = p_set_ind->data_len / GATTC_FTL_MAX_BLOCK_LEN + 1;
    }
    else
    {
        block_num = p_set_ind->data_len / GATTC_FTL_MAX_BLOCK_LEN;
    }
    write_len = block_num * GATTC_FTL_MAX_BLOCK_LEN;

    block_unused_bitmap = p_ftl_header->ftl_cb.block_unused_bitmap;
    while (block_unused_bitmap)
    {
        if (block_unused_bitmap & 0x01)
        {
            p_ftl_dev->block_used_bitmap |= (1 << block_idx);
            p_ftl_header->ftl_cb.block_unused_bitmap &= ~(1 << block_idx);
            block_assign_num++;
            if (block_assign_num == block_num)
            {
                break;
            }
        }
        block_unused_bitmap >>= 1;
        block_idx++;
    }
    if (block_assign_num != block_num)
    {
        error_idx = 4;
        goto failed;
    }
    block_idx = 0;
    block_used_bitmap = p_ftl_dev->block_used_bitmap;
    while (block_used_bitmap)
    {
        if (block_used_bitmap & 0x01)
        {
            if (ftl_save_to_storage(p_set_ind->p_data + write_idx,
                                    g_bt_ext_ftl_start_offset + GATTC_FTL_CB_LEN + GATTC_FTL_MAX_BLOCK_LEN * block_idx,
                                    GATTC_FTL_MAX_BLOCK_LEN) != 0)
            {
                error_idx = 5;
                goto failed;
            }
            else
            {
                write_idx += GATTC_FTL_MAX_BLOCK_LEN;
                if (write_idx > write_len)
                {
                    error_idx = 6;
                    goto failed;
                }
            }
        }
        block_used_bitmap >>= 1;
        block_idx++;
    }
    if (write_idx != write_len)
    {
        error_idx = 7;
        goto failed;
    }
    if (ftl_save_to_storage(p_ftl_header, g_bt_ext_ftl_start_offset,
                            sizeof(T_GATTC_FTL_HEADER)) != 0)
    {
        error_idx = 8;
        goto failed;
    }
    if (p_ftl_header != NULL)
    {
        os_mem_free(p_ftl_header);
    }
    return true;
failed:
    PROTOCOL_PRINT_ERROR1("gattc_tbl_storage_add_dev:failed, error_idx %d", error_idx);
    if (p_ftl_header != NULL)
    {
        os_mem_free(p_ftl_header);
    }
    return false;
}

T_APP_RESULT gattc_tbl_storage_cb(T_GATT_STORATE_EVENT type, void *p_data)
{
    T_APP_RESULT app_result = APP_RESULT_REJECT;
    switch (type)
    {
    case GATT_STORATE_EVENT_SRV_TBL_GET_IND:
        {
            T_GATT_STORATE_SRV_TBL_GET_IND *p_get_ind = (T_GATT_STORATE_SRV_TBL_GET_IND *)p_data;
            if (gattc_tbl_storage_get_dev(p_get_ind))
            {
                app_result = APP_RESULT_SUCCESS;
            }
        }
        break;

    case GATT_STORATE_EVENT_SRV_TBL_SET_IND:
        {
            T_GATT_STORATE_SRV_TBL_SET_IND *p_set_ind = (T_GATT_STORATE_SRV_TBL_SET_IND *)p_data;
            if (gattc_tbl_storage_add_dev(p_set_ind))
            {
                app_result = APP_RESULT_SUCCESS;
            }
        }
        break;

    default:
        break;
    }
    return app_result;
}

void gattc_tbl_storage_init(void)
{
    if (g_bt_ext_ftl_start_offset != 0)
    {
        gatt_storage_register(gattc_tbl_storage_cb);
    }
}

void gattc_tbl_storage_handle_bond_modify(T_LE_BOND_MODIFY_INFO *p_info)
{
    if (g_bt_ext_ftl_start_offset == 0)
    {
        return;
    }
    if (p_info->type == LE_BOND_CLEAR)
    {
        gattc_tbl_storage_clear();
    }
    else if (p_info->type == LE_BOND_DELETE ||
             p_info->type == LE_BOND_ADD)
    {
        if (p_info->p_entry)
        {
            gattc_tbl_storage_remove(p_info->p_entry->remote_bd.addr,
                                     p_info->p_entry->remote_bd.remote_bd_type);
        }
    }
}
#endif
