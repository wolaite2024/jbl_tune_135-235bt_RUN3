/**
*****************************************************************************************
*     Copyright(c) 2018, Realtek Semiconductor Corporation. All rights reserved.
*****************************************************************************************
   * @file      ams.c
   * @brief     This file handles AMS Client routines.
   * @author
   * @date
   * @version
   **************************************************************************************
*/
#if F_APP_BLE_AMS_CLIENT_SUPPORT
#include <trace.h>
#include <string.h>
#include <os_msg.h>
#include <os_mem.h>
#include <ams.h>
#include <ams_client.h>

typedef struct
{
    void                  *ams_queue_handle;
    uint8_t               *ptr;
} T_APP_AMS_LINK;

T_CLIENT_ID     ams_client;
T_APP_AMS_LINK  *ams_link_table;
uint8_t         ams_link_number;

extern void *evt_queue_handle;
extern void *io_queue_handle;

void ams_send_msg_to_app(uint8_t conn_id)
{
    T_IO_MSG io_msg;
    uint8_t event = EVENT_IO_TO_APP;
    io_msg.type = IO_MSG_TYPE_LE_AMS;
    io_msg.subtype = 0;
    io_msg.u.param = conn_id;

    if (os_msg_send(io_queue_handle, &io_msg, 0) == false)
    {
        GAP_PRINT_ERROR0("ams_send_msg_to_app: io_queue_handle fail");
    }
    else if (os_msg_send(evt_queue_handle, &event, 0) == false)
    {
        GAP_PRINT_ERROR0("ams_send_msg_to_app: evt_queue_handle fail");
    }
}



/**
 * @brief  ams clinet callback handle message from upperstack
 */
T_APP_RESULT ams_client_cb(T_CLIENT_ID client_id, uint8_t conn_id, void *p_data)
{
    T_APP_RESULT  result = APP_RESULT_SUCCESS;
    T_AMS_CB_DATA *p_cb_data = (T_AMS_CB_DATA *)p_data;

    switch (p_cb_data->cb_type)
    {
    case AMS_CLIENT_CB_TYPE_DISC_STATE:
        switch (p_cb_data->cb_content.disc_state)
        {
        case AMS_DISC_DONE:
            APP_PRINT_INFO0("ams_client_cb: discover procedure done.");
            ams_subscribe_entity_upd(conn_id, true);
            break;
        case AMS_DISC_FAILED:
            APP_PRINT_ERROR0("ams_client_cb: discover request failed.");
            break;
        default:
            break;
        }
        break;

    case AMS_CLIENT_CB_TYPE_NOTIF_IND_RESULT:
        switch (p_cb_data->cb_content.notify_data.type)
        {
        case AMS_NOTIFY_FROM_REMOTE_CMD:
            break;
        case AMS_NOTIFY_FROM_ENTITY_UPD:
            APP_PRINT_INFO2("AMS_NOTIFY_FROM_ENTITY_UPD: data[%d]: %b",
                            p_cb_data->cb_content.notify_data.value_size,
                            TRACE_BINARY(p_cb_data->cb_content.notify_data.value_size,
                                         p_cb_data->cb_content.notify_data.p_value));
//            ams_write_remote_cmd(conn_id, REMOTE_CMD_ID_TOGGLE_PLAY_PAUSE);
            break;
        default:
            break;
        }
        break;

    case AMS_CLIENT_CB_TYPE_WRITE_RESULT:
        {
            if (p_cb_data->cb_content.write_result.cause != ATT_SUCCESS)
            {
                APP_PRINT_ERROR1("AMS_CLIENT_CB_TYPE_WRITE_RESULT: Failed, cause 0x%x",
                                 p_cb_data->cb_content.write_result.cause);
            }
            switch (p_cb_data->cb_content.write_result.type)
            {
            case AMS_WRITE_REMOTE_CMD_NOTIFY_ENABLE:
                APP_PRINT_INFO0("AMS_WRITE_REMOTE_CMD_NOTIFY_ENABLE");
                break;

            case AMS_WRITE_REMOTE_CMD_NOTIFY_DISABLE:
                APP_PRINT_INFO0("AMS_WRITE_REMOTE_CMD_NOTIFY_DISABLE");
                break;

            case AMS_WRITE_ENTITY_UPD_NOTIFY_ENABLE:
                {
                    APP_PRINT_INFO0("AMS_WRITE_ENTITY_UPD_NOTIFY_ENABLE");
                    uint8_t entity_needed[] = {ENTITY_ID_TRACK, TRACK_ATTR_ID_ARTIST, TRACK_ATTR_ID_ALBUM, TRACK_ATTR_ID_TITLE};
                    ams_write_entity_upd_cmd(conn_id, entity_needed, sizeof(entity_needed));
                }
                break;
            case AMS_WRITE_ENTITY_UPD_NOTIFY_DISABLE:
                APP_PRINT_INFO0("AMS_WRITE_ENTITY_UPD_NOTIFY_DISABLE");
                break;

            case AMS_WRITE_ENTITY_ATTR_VALUE:
                APP_PRINT_INFO0("AMS_WRITE_ENTITY_ATTR_VALUE");

                break;

            default:
                break;
            }
        }
        break;

    case AMS_CLIENT_CB_TYPE_DISCONNECT_INFO:
        {
            T_AMS_MSG ams_msg;
            void *ams_queue_handle = ams_link_table[conn_id].ams_queue_handle;
            APP_PRINT_INFO1("AMS_CLIENT_CB_TYPE_DISCONNECT_INFO: conn_id = 0x%x", conn_id);
            memset(&ams_link_table[conn_id], 0, sizeof(T_APP_AMS_LINK));
            ams_link_table[conn_id].ams_queue_handle = ams_queue_handle;
            /*release msg queue*/
            while (os_msg_recv(ams_link_table[conn_id].ams_queue_handle, &ams_msg, 0));
        }
        break;

    default:
        break;
    }
    return result;
}


void ams_handle_msg(T_IO_MSG *p_io_msg)
{
    uint8_t conn_id = p_io_msg->u.param;
    T_AMS_MSG ams_msg;
    if (os_msg_recv(ams_link_table[conn_id].ams_queue_handle, &ams_msg, 0) == false)
    {
        APP_PRINT_INFO1("ams_handle_msg: conn_id 0x%x os_msg_recv failed", conn_id);
        return;
    }

}





/**
 * @brief  App register ams client to upperstack.
 */
void ams_init(uint8_t link_num)
{
    uint8_t i;
    ams_link_number = link_num;
    ams_link_table = os_mem_zalloc(RAM_TYPE_DATA_ON, ams_link_number * sizeof(T_APP_AMS_LINK));
    if (ams_link_table == NULL)
    {
        APP_PRINT_ERROR0("ams_init: allocate buffer failed");
        return;
    }
    for (i = 0; i < ams_link_number; i++)
    {
        if (os_msg_queue_create(&(ams_link_table[i].ams_queue_handle), "ams", AMS_MSG_QUEUE_NUM,
                                sizeof(T_AMS_MSG)) == false)
        {
            APP_PRINT_ERROR2("ams_init: link_num %d, i 0x%x create queue failed", link_num, i);
        }
    }
    ams_client = ams_add_client(ams_client_cb, link_num);
}

#endif

