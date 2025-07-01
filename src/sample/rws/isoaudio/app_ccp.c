#include <stdlib.h>
#include <string.h>
#include "trace.h"
#include "ccp_client.h"
#include "app_ccp.h"
#include "app_main.h"

#if F_APP_CCP_SUPPORT
static uint16_t active_voice_conn_handle = 0;

uint16_t app_ccp_get_active_conn_handle(void)
{
    return active_voice_conn_handle;
}

bool app_ccp_set_active_conn_handle(uint16_t conn_handle)
{
    bool ret = false;
    T_APP_LE_LINK *p_link;

    p_link = app_find_le_link_by_conn_handle(conn_handle);
    if (p_link != NULL)
    {
        active_voice_conn_handle = conn_handle;
        ret = true;
    }
    APP_PRINT_INFO2("app_ccp_set_active_conn_handle: active_voice_conn_handle %x, ret %x",
                    active_voice_conn_handle, ret);
    return ret;
}

T_APP_RESULT app_ccp_handle_msg(T_LE_AUDIO_MSG msg, void *buf)
{
    T_APP_RESULT app_result = APP_RESULT_SUCCESS;

    APP_PRINT_INFO1("app_ccp_handle_msg: msg %x", msg);
    switch (msg)
    {
    case LE_AUDIO_MSG_CCP_CLIENT_DIS_DONE:
        {
            T_APP_LE_LINK *p_link;
            T_CCP_CLIENT_DIS_DONE *p_dis_done = (T_CCP_CLIENT_DIS_DONE *)buf;

            p_link = app_find_le_link_by_conn_handle(p_dis_done->conn_handle);
            if (p_link == NULL)
            {
                return APP_RESULT_APP_ERR;
            }

            if (p_dis_done->general)
            {
                APP_PRINT_INFO1("find GTBS: conn_handle 0x%x", p_dis_done->conn_handle);
                uint16_t cfg_flags = TBS_LIST_CUR_CALL_NOTIFY_CCCD_FLAG | TBS_STATUS_FLAGS_NOTIFY_CCCD_FLAG |
                                     TBS_IN_CALL_TG_URI_CCCD_FLAG | TBS_CALL_STATE_NOTIFY_CCCD_FLAG |
                                     TBS_CCP_CCCD_FLAG | TBS_TERM_REASON_NOTIFY_CCCD_FLAG |
                                     TBS_INCOMING_CALL_NOTIFY_CCCD_FLAG;
                ccp_cfg_cccd(p_dis_done->conn_handle, 0, cfg_flags, true, p_dis_done->general);
                ccp_read_tbs_char(p_dis_done->conn_handle, 0, TBS_UUID_CHAR_CALL_STATE, p_dis_done->general);
                p_link->general_tbs = p_dis_done->general;
                if (app_get_ble_link_num() == 1)
                {
                    app_ccp_set_active_conn_handle(p_dis_done->conn_handle);
                }
            }
            else
            {
                APP_PRINT_INFO1("find TBS, num: %d", p_dis_done->srv_num);
            }
        }
        break;

    case LE_AUDIO_MSG_CCP_CLIENT_READ_RESULT:
        {
            T_APP_LE_LINK *p_link;
            T_CCP_READ_RESULT *p_read_result = (T_CCP_READ_RESULT *)buf;

            p_link = app_find_le_link_by_conn_handle(p_read_result->conn_handle);
            if (p_link == NULL)
            {
                return APP_RESULT_APP_ERR;
            }

            if (p_read_result->uuid == TBS_UUID_CHAR_CALL_STATE)
            {
                if (p_read_result->value.call_state.len < CCP_CALL_STATE_CHARA_LEN)
                {
                    APP_PRINT_WARN0("app_ccp_handle_msg: invalid call state");
                    return APP_RESULT_APP_ERR;
                }

                app_le_audio_link_sm(p_link->conn_handle, LE_AUDIO_CCP_READ_RESULT, p_read_result);
            }
        }
        break;

    case LE_AUDIO_MSG_CCP_CLIENT_CALL_STATE_NOTIFY:
        {
            T_APP_LE_LINK *p_link;
            T_CCP_NOTIFY_DATA *p_notify_data = (T_CCP_NOTIFY_DATA *)buf;

            p_link = app_find_le_link_by_conn_handle(p_notify_data->conn_handle);
            if (p_link == NULL)
            {
                return APP_RESULT_APP_ERR;
            }

            if (p_notify_data->uuid == TBS_UUID_CHAR_CALL_STATE)
            {
                if (p_notify_data->value.call_state.len < CCP_CALL_STATE_CHARA_LEN)
                {
                    APP_PRINT_WARN0("app_ccp_handle_msg: invalid call state");
                    return APP_RESULT_APP_ERR;
                }

                app_le_audio_link_sm(p_link->conn_handle, LE_AUDIO_CCP_CALL_STATE, p_notify_data);
            }
            else if (p_notify_data->uuid == TBS_UUID_CHAR_INCOMING_CALL ||
                     p_notify_data->uuid == TBS_UUID_CHAR_URI_SUP_LIST)
            {
                if (p_notify_data->value.in_call.uri_len != 0)
                {
                    uint8_t  *p_buf;
                    uint8_t  *p_uri;
                    uint16_t uri_len;

                    p_uri = p_notify_data->value.in_call.p_uri;
                    uri_len = p_notify_data->value.in_call.uri_len;
                    p_buf = malloc(uri_len);
                    if (p_buf != NULL)
                    {
                        if (p_link->call_uri != NULL)
                        {
                            free(p_link->call_uri);
                            p_link->call_uri = NULL;
                        }
                        p_link->call_uri = p_buf;
                        memcpy(p_link->call_uri, p_uri, uri_len);
                        APP_PRINT_INFO1("app_ccp_handle_msg: call_uri %s", TRACE_STRING(p_link->call_uri));
                    }
                }
            }
        }
        break;

    case LE_AUDIO_MSG_CCP_CLIENT_CCP_RES_NOTIFY:
        {
            T_APP_LE_LINK *p_link;
            T_CCP_NOTIFY_DATA *p_notify_data = (T_CCP_NOTIFY_DATA *)buf;

            p_link = app_find_le_link_by_conn_handle(p_notify_data->conn_handle);
            if (p_link == NULL)
            {
                return APP_RESULT_APP_ERR;
            }

            if (p_notify_data->uuid == TBS_UUID_CHAR_CCP)
            {
                APP_PRINT_INFO3("app_ccp_handle_msg: response notify req_opcode %d, call_index %d, result %d",
                                p_notify_data->value.ccp_op_result.req_opcode, p_notify_data->value.ccp_op_result.call_idx,
                                p_notify_data->value.ccp_op_result.res_code);
            }
        }
        break;

    case LE_AUDIO_MSG_CCP_CLIENT_TERM_REASON_NOTIFY:
        {
            T_APP_LE_LINK *p_link;
            T_CCP_NOTIFY_DATA *p_notify_data = (T_CCP_NOTIFY_DATA *)buf;

            p_link = app_find_le_link_by_conn_handle(p_notify_data->conn_handle);
            if (p_link == NULL)
            {
                return APP_RESULT_APP_ERR;
            }

            if (p_notify_data->uuid == TBS_UUID_CHAR_TERM_REASON)
            {
                app_le_audio_link_sm(p_link->conn_handle, LE_AUDIO_CCP_TERM_NOTIFY, p_notify_data);
            }
        }
        break;

    default:
        break;
    }
    return app_result;
}
#endif
