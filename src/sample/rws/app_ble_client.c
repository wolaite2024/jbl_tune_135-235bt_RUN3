/*
 * Copyright (c) 2018, Realsil Semiconductor Corporation. All rights reserved.
 */

#include "trace.h"
#include "app_ble_client.h"
#include "ancs_client.h"
#include "gap_timer.h"
#include "os_mem.h"
#include "app_main.h"
#include "app_report.h"
#include "app_cmd.h"

#if BISTO_FEATURE_SUPPORT
#include "app_bisto_ble.h"
#endif

#include <profile_client.h>

#if 0
static T_APP_RESULT ancs_client_cb(T_CLIENT_ID client_id, uint8_t conn_id, void *p_data)
{
    T_APP_RESULT  result = APP_RESULT_SUCCESS;
    T_ANCS_CB_DATA *p_cb_data = (T_ANCS_CB_DATA *)p_data;
    T_APP_LE_LINK *p_link;

    p_link = app_find_le_link_by_conn_id(conn_id);
    if (p_link != NULL)
    {
        switch (p_cb_data->cb_type)
        {
        case ANCS_CLIENT_CB_TYPE_DISC_STATE:
            {
                APP_PRINT_TRACE1("ancs_client_cb: ANCS_CLIENT_CB_TYPE_DISC_STATE disc_state %d",
                                 p_cb_data->cb_content.disc_state);
                switch (p_cb_data->cb_content.disc_state)
                {
                case DISC_ANCS_DONE:
                    /* Discovery Simple BLE service procedure successfully done. */
                    ancs_set_data_source_notify(conn_id, true);
                    break;

                case DISC_ANCS_FAILED:
                    /* Discovery Request failed. */
                    break;

                default:
                    break;
                }
            }
            break;

        case ANCS_CLIENT_CB_TYPE_NOTIF_IND_RESULT:
            {
                uint8_t     *p_data;
                uint8_t     *tx_ptr;
                uint16_t    pkt_len;
                uint16_t    total_len;
                uint16_t    data_len;
                uint8_t     pkt_type;

                pkt_type = PKT_TYPE_SINGLE;
                p_data = p_cb_data->cb_content.notify_data.p_value;
                total_len = data_len = p_cb_data->cb_content.notify_data.value_size;
                while (data_len)
                {
                    if (data_len > (app_db.external_mcu_mtu - 12))
                    {
                        pkt_len = app_db.external_mcu_mtu - 12;
                        if (pkt_type == PKT_TYPE_SINGLE)
                        {
                            pkt_type = PKT_TYPE_START;
                        }
                        else
                        {
                            pkt_type = PKT_TYPE_CONT;
                        }
                    }
                    else
                    {
                        pkt_len = data_len;
                        if (pkt_type != PKT_TYPE_SINGLE)
                        {
                            pkt_type = PKT_TYPE_END;
                        }
                    }

                    tx_ptr = malloc((pkt_len + 7));
                    if (tx_ptr != NULL)
                    {
                        tx_ptr[0] = p_link->id;
                        tx_ptr[1] = p_cb_data->cb_content.notify_data.type;;
                        tx_ptr[2] = pkt_type;
                        tx_ptr[3] = (uint8_t)total_len;
                        tx_ptr[4] = (uint8_t)(total_len >> 8);
                        tx_ptr[5 = (uint8_t)pkt_len;
                                 tx_ptr[6] = (uint8_t)(pkt_len >> 8);
                                 memcpy(&tx_ptr[7], p_data, pkt_len);

                                 app_report_event(CMD_PATH_UART, EVENT_ANCS_NOTIFICATION, 0, tx_ptr, pkt_len + 7);

                                 free(tx_ptr);
                      }

                      p_data += pkt_len;
                      data_len -= pkt_len;
                  }
              }
              break;

          case ANCS_CLIENT_CB_TYPE_WRITE_RESULT:
              {
                  APP_PRINT_TRACE2("ancs_client_cb: ANCS_CLIENT_CB_TYPE_WRITE_RESULT cause 0x%04x, type 0x%02x",
                                 p_cb_data->cb_content.write_result.cause, p_cb_data->cb_content.write_result.type);

                switch (p_cb_data->cb_content.write_result.type)
                {
                case ANCS_WRITE_NOTIFICATION_SOURCE_NOTIFY_ENABLE:
                    {
                        uint8_t event_buff[2];

                        event_buff[0] = p_link->id;
                        event_buff[1] = 0x00;//successful
                        app_report_event(CMD_PATH_UART, EVENT_ANCS_REGISTER_COMPLETE, 0, &event_buff[0], 2);
                    }
                    break;

                case ANCS_WRITE_NOTIFICATION_SOURCE_NOTIFY_DISABLE:
                    break;

                case ANCS_WRITE_DATA_SOURCE_NOTIFY_ENABLE:
                    ancs_set_notification_source_notify(conn_id, true);
                    break;
                case ANCS_WRITE_DATA_SOURCE_NOTIFY_DISABLE:
                    break;

                case ANCS_WRITE_CONTROL_POINT:
                    {
                        uint8_t event_buff[3];

                        event_buff[0] = p_link->id;
                        event_buff[1] = (uint8_t)(p_cb_data->cb_content.write_result.cause);
                        event_buff[2] = (uint8_t)(p_cb_data->cb_content.write_result.cause >> 8);
                        app_report_event(CMD_PATH_UART, EVENT_ANCS_ACTION_RESULT, 0, &event_buff[0], 3);
                    }
                    break;

                default:
                    break;
                }
            }
            break;

        case ANCS_CLIENT_CB_TYPE_DISCONNECT_INFO:
            APP_PRINT_INFO1("ancs_client_cb: ANCS_CLIENT_CB_TYPE_DISCONNECT_INFO conn_id %d", conn_id);
            break;

        default:
            break;
        }
    }
    return result;
}
#endif


void app_ble_client_init(void)
{
    volatile uint8_t client_num = 0;

#if BISTO_FEATURE_SUPPORT
    client_num = client_num + BISTO_GATT_CLIENT_NUM;
#endif
    //ANCS
    if (client_num)
    {
        client_init(client_num);
    }
    else
    {
        APP_PRINT_INFO0("app_ble_client_init: num is 0");
    }
    //ancs_add_client(ancs_client_cb, MAX_BLE_LINK_NUM);
}
