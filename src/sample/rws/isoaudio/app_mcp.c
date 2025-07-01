#include "trace.h"
#include "ble_audio.h"
#include "mcs_client.h"
#include "app_link_util.h"

#if F_APP_MCP_SUPPORT
static uint16_t active_conn_handle = 0;

uint16_t app_mcp_get_active_conn_handle(void)
{
    return active_conn_handle;
}

bool app_mcp_set_active_conn_handle(uint16_t conn_handle)
{
    bool ret = false;
    T_APP_LE_LINK *p_link;

    p_link = app_find_le_link_by_conn_handle(conn_handle);
    if (p_link != NULL)
    {
        active_conn_handle = conn_handle;
        ret = true;
    }
    APP_PRINT_INFO2("app_mcp_set_active_conn_handle: active_conn_handle 0x%x, ret %x",
                    active_conn_handle, ret);
    return ret;
}

T_APP_RESULT app_mcp_handle_msg(T_LE_AUDIO_MSG msg, void *buf)
{
    T_APP_RESULT app_result = APP_RESULT_SUCCESS;

    APP_PRINT_INFO1("app_mcp_handle_msg: msg %x", msg);
    switch (msg)
    {
    case LE_AUDIO_MSG_MCS_CLIENT_DIS_DONE:
        {
            T_APP_LE_LINK *p_link;
            T_MCS_CLIENT_DIS_DONE *p_dis_done = (T_MCS_CLIENT_DIS_DONE *)buf;

            p_link = app_find_le_link_by_conn_handle(p_dis_done->conn_handle);
            if (p_link == NULL)
            {
                return APP_RESULT_APP_ERR;
            }

            if (p_dis_done->is_found)
            {
                if (p_dis_done->general_mcs)
                {
                    app_mcp_set_active_conn_handle(p_dis_done->conn_handle);
                    mcs_cfg_cccd(p_dis_done->conn_handle, MCS_MEDIA_CTL_POINT_CCCD_FLAG | MCS_TK_CHG_CCCD_FLAG,
                                 true, 0, true);
                    mcs_read_char_value(p_dis_done->conn_handle, 0, MEDIA_STATE_CHAR_UUID, true);
                    mcs_read_char_value(p_dis_done->conn_handle, 0, MEDIA_PLAYER_NAME_CHAR_UUID, true);
                    p_link->general_mcs = p_dis_done->general_mcs;
                }
            }
        }
        break;

    case LE_AUDIO_MSG_MCS_CLIENT_READ_RESULT:
        {
            T_APP_LE_LINK *p_link;
            T_MCS_CLIENT_READ_RES *p_read_result = (T_MCS_CLIENT_READ_RES *)buf;

            p_link = app_find_le_link_by_conn_handle(p_read_result->conn_handle);
            if (p_link == NULL)
            {
                return APP_RESULT_APP_ERR;
            }

            if (p_read_result->general_mcs)
            {
                if (p_read_result->uuid == MEDIA_STATE_CHAR_UUID)
                {
                    if (p_read_result->data.media_state != MCS_MEDIA_INACTIVE_STATE)
                    {
                        p_link->media_state = p_read_result->data.media_state;
                    }

                    if (p_link->media_state == MCS_MEDIA_PLAYING_STATE)
                    {
                        app_mcp_set_active_conn_handle(p_read_result->conn_handle);
                    }
                    APP_PRINT_INFO1("app_mcp_handle_msg: media_state %x", p_link->media_state);
                }
                else if (p_read_result->uuid == MEDIA_PLAYER_NAME_CHAR_UUID)
                {
                    if (p_read_result->data.media_player_name.name_len != 0)
                    {
                        uint8_t *p_name = p_read_result->data.media_player_name.player_name;
                        APP_PRINT_INFO1("app_mcp_handle_msg: player_name %s", TRACE_STRING(p_name));
                    }
                }
            }
        }
        break;

    case LE_AUDIO_MSG_MCS_CLIENT_NOTIFY_RESULT:
        {
            T_APP_LE_LINK *p_link;
            T_MCS_CLIENT_NOTFY_RES *p_notify_result = (T_MCS_CLIENT_NOTFY_RES *)buf;

            p_link = app_find_le_link_by_conn_handle(p_notify_result->conn_handle);
            if (p_link == NULL)
            {
                return APP_RESULT_APP_ERR;
            }

            switch (p_notify_result->uuid)
            {
            case MEDIA_STATE_CHAR_UUID:
                {
                    // Because dongle set MCP states is pause forever, not change state.
                    // here is workaround, when headset is streaming, local state will be change.
                    if (p_notify_result->general_mcs)
                    {
                        if (p_notify_result->data.media_state != MCS_MEDIA_INACTIVE_STATE)
                        {
                            p_link->media_state = p_notify_result->data.media_state;
                        }
                        APP_PRINT_INFO1("LE_AUDIO_MSG_MCS_CLIENT_NOTIFY_RESULT: state %x", p_link->media_state);
                        app_le_audio_link_sm(p_link->conn_handle, LE_AUDIO_MCP_STATE, NULL);
                    }
                }
                break;

            case TRACK_CHANGE_CHAR_UUID:
                {
                    APP_PRINT_INFO0("LE_AUDIO_MSG_MCS_CLIENT_NOTIFY_RESULT: track change");
                }
                break;

            default:
                break;
            }
        }
        break;

    default:
        break;
    }
    return app_result;
}
#endif
