#if F_APP_IAP_RTK_SUPPORT

/*
 * Copyright (c) 2018, Realsil Semiconductor Corporation. All rights reserved.
 */

#include <string.h>
#include "trace.h"
#include "os_mem.h"

#include "common_stream.h"
#include "iap_stream.h"
#include "app_cfg.h"
#include "app_cmd.h"
#include "app_main.h"
#include "app_iap_rtk.h"
#include "app_iap.h"
#include "os_queue.h"


typedef enum
{
    RTK_IAP_CREATE = 0,
    RTK_IAP_DELETE = 1,
    SYNC_ALL       = 2,
    RTK_IAP_MSG_MAX,
} RELAY_MSG_TYPE;


typedef struct
{
    uint32_t num;
    uint8_t bd_addrs[MAX_BR_LINK_NUM][6];
} RELAY_INFO;


typedef struct _ADDR_ELEM ADDR_ELEM;


typedef struct _ADDR_ELEM
{
    ADDR_ELEM *p_next;
    uint8_t bd_addr[6];
} ADDR_ELEM;


static struct
{
    T_OS_QUEUE addr_queue;
} app_iap_rtk;


static void send_msg_to_sec(RELAY_INFO *p_info, RELAY_MSG_TYPE msg_type)
{
    if (app_cfg_nv.bud_role != REMOTE_SESSION_ROLE_PRIMARY)
    {
        return;
    }

    APP_PRINT_TRACE1("app_iap_rtk send_msg_to_sec: msg_type %d", msg_type);
    app_relay_async_single_with_raw_msg(APP_MODULE_TYPE_RTK_IAP, msg_type,
                                        (uint8_t *)p_info, sizeof(RELAY_INFO));
}


static void send_addr_to_sec(uint8_t *bd_addr, RELAY_MSG_TYPE msg_type)
{
    RELAY_INFO info = {0};
    info.num = 1;
    memcpy(info.bd_addrs[0], bd_addr, sizeof(info.bd_addrs[0]));
    send_msg_to_sec(&info, msg_type);
}



static void app_iap_rtk_ready_to_read_process(COMMON_STREAM stream)
{
    T_APP_BR_LINK *p_link;
    uint8_t bd_addr[6];
    uint8_t *p_data = NULL;
    uint32_t len = 0;
    uint16_t data_len;
    uint8_t     app_idx;
    uint16_t    total_len;

    uint8_t *data = NULL;

    common_stream_read(stream, &data, &len);

    p_data = data;
    data_len = len;

    common_stream_get_addr(stream, bd_addr);

    p_link = app_find_br_link(bd_addr);
    if (p_link == NULL)
    {
        APP_PRINT_ERROR0("app_iap_rtk_ready_to_read_process: no acl link found");
        free(data);
        return;
    }
    app_idx = p_link->id;

    if (app_cfg_const.enable_embedded_cmd)
    {

        if (app_db.br_link[app_idx].p_embedded_cmd == NULL)
        {
            uint16_t cmd_len;

            //ios will auto combine two cmd into one pkt
            while (data_len)
            {
                if (p_data[0] == CMD_SYNC_BYTE)
                {
                    cmd_len = (p_data[2] | (p_data[3] << 8)) + 4; //sync_byte, seqn, length
                    if (data_len >= cmd_len)
                    {
                        app_handle_cmd_set(&p_data[4], (cmd_len - 4), CMD_PATH_IAP, p_data[1], app_idx);
                        data_len -= cmd_len;
                        p_data += cmd_len;
                    }
                    else
                    {
                        break;
                    }
                }
                else
                {
                    data_len--;
                    p_data++;
                }
            }

            if (data_len)
            {
                app_db.br_link[app_idx].p_embedded_cmd = malloc(data_len);
                memcpy(app_db.br_link[app_idx].p_embedded_cmd, p_data, data_len);
                app_db.br_link[app_idx].embedded_cmd_len = data_len;
            }
        }
        else
        {
            uint8_t *p_temp;
            uint16_t cmd_len;

            p_temp = app_db.br_link[app_idx].p_embedded_cmd;
            total_len = app_db.br_link[app_idx].embedded_cmd_len + data_len;
            app_db.br_link[app_idx].p_embedded_cmd = malloc(total_len);
            memcpy(app_db.br_link[app_idx].p_embedded_cmd, p_temp,
                   app_db.br_link[app_idx].embedded_cmd_len);
            free(p_temp);
            memcpy(app_db.br_link[app_idx].p_embedded_cmd +
                   app_db.br_link[app_idx].embedded_cmd_len,
                   p_data, data_len);
            app_db.br_link[app_idx].embedded_cmd_len = total_len;

            p_data = app_db.br_link[app_idx].p_embedded_cmd;
            data_len = total_len;
            p_temp = app_db.br_link[app_idx].p_embedded_cmd;
            app_db.br_link[app_idx].p_embedded_cmd = NULL;
            //ios will auto combine two cmd into one pkt
            while (data_len)
            {
                if (p_data[0] == CMD_SYNC_BYTE)
                {
                    cmd_len = (p_data[2] | (p_data[3] << 8)) + 4; //sync_byte, seqn, length
                    if (data_len >= cmd_len)
                    {
                        app_handle_cmd_set(&p_data[4], (cmd_len - 4), CMD_PATH_IAP, p_data[1], app_idx);
                        data_len -= cmd_len;
                        p_data += cmd_len;
                    }
                    else
                    {
                        break;
                    }
                }
                else
                {
                    data_len--;
                    p_data++;
                }
            }

            if (data_len)
            {
                app_db.br_link[app_idx].p_embedded_cmd = malloc(data_len);
                memcpy(app_db.br_link[app_idx].p_embedded_cmd, p_data, data_len);
            }
            app_db.br_link[app_idx].embedded_cmd_len = data_len;
            free(p_temp);
        }
    }
}



static void app_iap_rtk_stream_cb(COMMON_STREAM stream, COMMON_STREAM_EVENT event)
{
    APP_PRINT_TRACE1("app_iap_rtk_stream_cb: event %d", event);

    switch (event)
    {
    case COMMON_STREAM_CONNECTED_EVENT:
        {

        }
        break;

    case COMMON_STREAM_DISCONNECTED_EVENT:
        {

        }
        break;

    case COMMON_STREAM_READY_TO_READ_EVENT:
        {
            app_iap_rtk_ready_to_read_process(stream);
        }
        break;

    case COMMON_STREAM_READY_TO_WRITE_EVENT:
        {

        }
        break;

    default:
        {
            APP_PRINT_ERROR1("app_iap_rtk_stream_cb: unknown event %d", event);
        }
        break;
    }
}

static ADDR_ELEM *addr_queue_search(uint8_t *addr)
{
    ADDR_ELEM *p_elem = (ADDR_ELEM *)app_iap_rtk.addr_queue.p_first;

    for (; p_elem != NULL; p_elem = p_elem->p_next)
    {
        if (memcmp(p_elem->bd_addr, addr, 6) == 0)
        {
            return p_elem;
        }
    }

    return NULL;
}


bool app_iap_rtk_delete(uint8_t *bd_addr)
{
    IAP_STREAM iap_rtk_stream = NULL;

    APP_PRINT_TRACE1("app_iap_rtk_delete: bd_addr %s", TRACE_BDADDR(bd_addr));

    iap_rtk_stream = iap_stream_find_by_ea_protocol_id(EA_PROTOCOL_ID_RTK, bd_addr);

    if (iap_rtk_stream == NULL)
    {
        APP_PRINT_ERROR0("app_iap_rtk_delete: iap_rtk_stream is NULL");
        return false;
    }

    common_stream_delete((COMMON_STREAM)iap_rtk_stream);

    ADDR_ELEM *p_elem = NULL;
    p_elem = addr_queue_search(bd_addr);
    if (p_elem != NULL)
    {
        os_queue_delete(&app_iap_rtk.addr_queue, p_elem);
    }

    send_addr_to_sec(bd_addr, RTK_IAP_DELETE);

    return true;
}



bool app_iap_rtk_create(uint8_t *bd_addr)
{
    APP_PRINT_TRACE1("app_iap_rtk_create: %s", TRACE_BDADDR(bd_addr));

    IAP_INIT_SETTINGS iap_stream_settings =
    {
        .bd_addr            = bd_addr,
        .stream_cb          = app_iap_rtk_stream_cb,
        .ea_protocol_id     = EA_PROTOCOL_ID_RTK,
    };

    iap_stream_create(&iap_stream_settings);

    ADDR_ELEM *p_elem = NULL;
    p_elem = addr_queue_search(bd_addr);
    if (p_elem == NULL)
    {
        p_elem = calloc(1, sizeof(*p_elem));
        memcpy(p_elem->bd_addr, bd_addr, sizeof(p_elem->bd_addr));
        os_queue_in(&app_iap_rtk.addr_queue, p_elem);
    }


    send_addr_to_sec(bd_addr, RTK_IAP_CREATE);

    return true;
}


static void sync_all_delete(RELAY_INFO *p_info)
{
    ADDR_ELEM *p_elem = (ADDR_ELEM *)app_iap_rtk.addr_queue.p_first;
    uint32_t addr_num = p_info->num;
    for (; p_elem != NULL; p_elem = p_elem->p_next)
    {
        uint32_t i = 0;
        uint8_t *bd_addr = p_elem->bd_addr;
        APP_PRINT_TRACE1("app_iap_rtk sync_all_delete: local bd_addr %s", TRACE_BDADDR(bd_addr));
        for (; i < addr_num; i++)
        {
            if (memcmp(bd_addr, p_info->bd_addrs[i], 6) == 0)
            {
                break;
            }
        }
        if (i == addr_num)
        {
            app_iap_rtk_delete(bd_addr);
        }
    }
}


static void sync_all_create(RELAY_INFO *p_info)
{
    ADDR_ELEM *p_elem = NULL;
    uint32_t addr_num = p_info->num;

    for (uint32_t i = 0; i < addr_num; i++)
    {
        uint8_t *bd_addr = p_info->bd_addrs[i];
        APP_PRINT_TRACE1("app_iap_rtk sync_all_create: remote bd_addr %s", TRACE_BDADDR(bd_addr));
        p_elem = addr_queue_search(bd_addr);
        if (p_elem == NULL)
        {
            app_iap_rtk_create(bd_addr);
        }
    }
}


static void sync_all(RELAY_INFO *p_info)
{
    sync_all_delete(p_info);
    sync_all_create(p_info);
}


static void relay_parse(RELAY_MSG_TYPE msg_type, RELAY_INFO *p_info, uint16_t len,
                        T_REMOTE_RELAY_STATUS status)
{
    if (status != REMOTE_RELAY_STATUS_ASYNC_RCVD)
    {
        return;
    }

    APP_PRINT_TRACE1("app_iap_rtk relay_parse: msg_type %d", msg_type);

    switch (msg_type)
    {
    case RTK_IAP_CREATE:
        app_iap_rtk_create(p_info->bd_addrs[0]);
        break;

    case RTK_IAP_DELETE:
        app_iap_rtk_delete(p_info->bd_addrs[0]);
        break;

    case SYNC_ALL:
        sync_all(p_info);
        break;

    default:
        break;
    }
}


void app_iap_rtk_handle_remote_conn_cmpl(void)
{
    if (app_cfg_nv.bud_role != REMOTE_SESSION_ROLE_PRIMARY)
    {
        return;
    }

    RELAY_INFO info = {0};
    ADDR_ELEM *p_elem = (ADDR_ELEM *)app_iap_rtk.addr_queue.p_first;

    for (uint32_t i = 0; i < MAX_BR_LINK_NUM && p_elem != NULL; i++, p_elem = p_elem->p_next)
    {
        APP_PRINT_TRACE2("app_iap_rtk_handle_remote_conn_cmpl: i %d, bd_addr %s", i,
                         TRACE_BDADDR(p_elem->bd_addr));
        memcpy(info.bd_addrs[i], p_elem->bd_addr, 6);
        info.num++;
    }
    send_msg_to_sec(&info, SYNC_ALL);
}


void app_iap_rtk_init(void)
{
    app_relay_cback_register(NULL, (P_APP_PARSE_CBACK)relay_parse, APP_MODULE_TYPE_RTK_IAP,
                             RTK_IAP_MSG_MAX);

    os_queue_init(&app_iap_rtk.addr_queue);
}

#endif
