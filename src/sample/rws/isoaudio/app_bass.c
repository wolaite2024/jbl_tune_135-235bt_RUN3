#include <string.h>
#include "trace.h"
#include "os_mem.h"
#include "base_data_parse.h"
#include "bass_mgr.h"
#include "gap_conn_le.h"
#include "app_bass.h"
#include "app_cfg.h"
#include "app_le_audio_mgr.h"

#if F_APP_TMAP_BMR_SUPPORT
extern uint8_t bis_idx;
uint16_t conn_handle;
uint8_t source_id;
T_BLE_AUDIO_SYNC_HANDLE sync_handle;

uint16_t app_bass_get_conn_handle(void)
{
    return conn_handle;
}

uint8_t app_bass_get_source(void)
{
    return source_id;
}

T_BLE_AUDIO_SYNC_HANDLE app_bass_get_sync_handle(uint8_t src_id)
{
    if (source_id == src_id)
    {
        return sync_handle;
    }
    else
    {
        return NULL;
    }
}
static void app_bass_pa_sync_cb(T_BLE_AUDIO_SYNC_HANDLE handle, uint8_t cb_type, void *p_cb_data)
{
    T_BLE_AUDIO_SYNC_CB_DATA *p_sync_cb = (T_BLE_AUDIO_SYNC_CB_DATA *)p_cb_data;
    switch (cb_type)
    {
    case MSG_BLE_AUDIO_SYNC_HANDLE_RELEASED:
        {
            APP_PRINT_TRACE1("MSG_BLE_AUDIO_SYNC_HANDLE_RELEASED: action_role %d",
                             p_sync_cb->p_sync_handle_released->action_role);
        }
        break;

    case MSG_BLE_AUDIO_ADDR_UPDATE:
        {
            APP_PRINT_TRACE1("MSG_BLE_AUDIO_ADDR_UPDATE: advertiser_address %s",
                             TRACE_BDADDR(p_sync_cb->p_addr_update->advertiser_address));
        }
        break;

    case MSG_BLE_AUDIO_PA_SYNC_STATE:
        {
            APP_PRINT_TRACE3("MSG_BLE_AUDIO_PA_SYNC_STATE: sync_state %d, action %d, cause 0x%x",
                             p_sync_cb->p_pa_sync_state->sync_state,
                             p_sync_cb->p_pa_sync_state->action,
                             p_sync_cb->p_pa_sync_state->cause);
#if 0
            if (p_sync_cb->p_pa_sync_state->sync_state == GAP_PA_SYNC_STATE_SYNCHRONIZING_WAIT_SCANNING)
            {
                app_start_extended_scan();
            }
            if (p_sync_cb->p_pa_sync_state->sync_state == GAP_PA_SYNC_STATE_SYNCHRONIZED ||
                p_sync_cb->p_pa_sync_state->sync_state == GAP_PA_SYNC_STATE_TERMINATED)
            {
                app_stop_extended_scan();
            }
#endif
        }
        break;

    case MSG_BLE_AUDIO_BASE_DATA_MODIFY_INFO:
        {
        }
        break;

    case MSG_BLE_AUDIO_PA_REPORT_INFO:
        {
            T_BASE_DATA_MAPPING *p_mapping = base_data_parse_data(
                                                 p_sync_cb->p_le_periodic_adv_report_info->data_len,
                                                 p_sync_cb->p_le_periodic_adv_report_info->p_data);

            if (p_mapping != NULL)
            {
                T_CODEC_CFG bis_codec_cfg;
                uint8_t i, j;

                for (i = 0; i < p_mapping->num_subgroups; i++)
                {
                    for (j = 0; j < p_mapping->p_subgroup[i].num_bis; j ++)
                    {
                        bis_codec_cfg.frame_duration = p_mapping->p_subgroup[i].p_bis_param[j].bis_codec_cfg.frame_duration;
                        bis_codec_cfg.sample_frequency =
                            p_mapping->p_subgroup[i].p_bis_param[j].bis_codec_cfg.sample_frequency;
                        bis_codec_cfg.codec_frame_blocks_per_sdu =
                            p_mapping->p_subgroup[i].p_bis_param[j].bis_codec_cfg.codec_frame_blocks_per_sdu;
                        bis_codec_cfg.octets_per_codec_frame =
                            p_mapping->p_subgroup[i].p_bis_param[j].bis_codec_cfg.octets_per_codec_frame;
                        bis_codec_cfg.audio_channel_allocation =
                            p_mapping->p_subgroup[i].p_bis_param[j].bis_codec_cfg.audio_channel_allocation;
                        bis_codec_cfg.presentation_delay  = p_mapping->presentation_delay;

                        app_le_audio_bis_cb_allocate(p_mapping->p_subgroup[i].p_bis_param[j].bis_index, &bis_codec_cfg);
                        //    base_data_print(p_mapping);

                    }
                }
            }
        }
        break;

    case MSG_BLE_AUDIO_PA_BIGINFO:
        {
            APP_PRINT_INFO2("MSG_BLE_AUDIO_PA_BIGINFO: num_bis %d, encryption %d",
                            p_sync_cb->p_le_biginfo_adv_report_info->num_bis,
                            p_sync_cb->p_le_biginfo_adv_report_info->encryption);
#if 0
            if (bqb_bap_sde_sink.bisinfo_received == false)
            {
                bqb_bap_sde_sink.bisinfo_received = true;
            }
#endif
        }
        break;

    case MSG_BLE_AUDIO_BIG_SYNC_STATE:
        {
            APP_PRINT_INFO4("MSG_BLE_AUDIO_BIG_SYNC_STATE: sync_state %d, action %d, action role %d, cause 0x%x\r\n",
                            p_sync_cb->p_big_sync_state->sync_state,
                            p_sync_cb->p_big_sync_state->action,
                            p_sync_cb->p_big_sync_state->action_role,
                            p_sync_cb->p_big_sync_state->cause);

            if ((p_sync_cb->p_big_sync_state->sync_state == BIG_SYNC_RECEIVER_SYNC_STATE_TERMINATED) &&
                (p_sync_cb->p_big_sync_state->cause == 0))
            {
                APP_PRINT_INFO0("LE_AUDIO_MSG_BASS_BIG_SYNC_STATE: BIG_SYNC_RECEIVER_SYNC_STATE_TERMINATED");

                T_BIS_CB *p_bis_cb = app_le_audio_find_bis_by_source_id(source_id);
                if (p_bis_cb)
                {
                    audio_track_release(p_bis_cb->audio_track_handle);
                    if (!app_le_audio_free_bis(p_bis_cb))
                    {
                        APP_PRINT_ERROR0("app_bass_handle_msg: free_bis_cb failed.");
                        return;
                    }
                }
                else
                {
                    APP_PRINT_ERROR0("LE_AUDIO_MSG_BASS_BIG_SYNC_STATE: terminate bis: bis_cb not found");
                    return;
                }
            }
            else if ((p_sync_cb->p_big_sync_state->sync_state == BIG_SYNC_RECEIVER_SYNC_STATE_TERMINATED) &&
                     (p_sync_cb->p_big_sync_state->cause == HCI_ERR | HCI_ERR_CONN_TIMEOUT))
            {
                //big sync lost
                T_BIS_CB *p_bis_cb = app_le_audio_find_bis_by_source_id(source_id);
                if (p_bis_cb)
                {
                    audio_track_release(p_bis_cb->audio_track_handle);
                }
                else
                {
                    APP_PRINT_ERROR0("LE_AUDIO_MSG_BASS_BIG_SYNC_STATE: bis_cb not found");
                    return;
                }
            }
            else if (p_sync_cb->p_big_sync_state->sync_state == BIG_SYNC_RECEIVER_SYNC_STATE_SYNCHRONIZED)
            {
                uint8_t codec_id[5] = {1, 0, 0, 0, 0};
                codec_id[0] = LC3_CODEC_ID;
                uint32_t controller_delay = 0x1122;
                uint8_t codec_len = 0;
                uint8_t *p_codec_data = NULL;

                T_BLE_AUDIO_BIS_INFO bis_sync_info;
                bool result = ble_audio_get_bis_sync_info(sync_handle,
                                                          &bis_sync_info);
                if (result == false)
                {
                    // goto failed;
                }
                APP_PRINT_ERROR4("BIG_SYNC_RECEIVER_SYNC_STATE_SYNCHRONIZED%d,%d,%d,%d: ", result,
                                 bis_sync_info.bis_num, bis_idx, bis_sync_info.bis_info[0].bis_idx);
                if (bis_sync_info.bis_num > 0)
                {
                    uint8_t i;
                    bis_idx = bis_sync_info.bis_info[0].bis_idx;
                    app_lea_check_bis_cb(&bis_idx);
                    ble_audio_bis_setup_data_path(handle, bis_idx, codec_id, controller_delay,
                                                  codec_len, p_codec_data);
                }
                //terminate PA sync to save bandwitch
                ble_audio_pa_terminate(app_bass_get_sync_handle(source_id));
            }
        }
        break;

    case MSG_BLE_AUDIO_BIG_SETUP_DATA_PATH:
        {
            APP_PRINT_INFO2("MSG_BLE_AUDIO_BIG_SETUP_DATA_PATH: bis_idx %d, cause 0x%x\r\n",
                            p_sync_cb->p_setup_data_path->bis_idx,
                            p_sync_cb->p_setup_data_path->cause);

            app_le_audio_track_create_for_bis(p_sync_cb->p_setup_data_path->bis_conn_handle,
                                              source_id,
                                              p_sync_cb->p_setup_data_path->bis_idx);
            app_le_audio_bis_state_machine(LE_AUDIO_STREAMING, NULL);
        }
        break;

    default:
        break;
    }
    return;
}

T_APP_RESULT app_bass_handle_msg(T_LE_AUDIO_MSG msg, void *buf)
{
    T_APP_RESULT app_result = APP_RESULT_SUCCESS;

    uint8_t i = 0;
    uint8_t j;
    switch (msg)
    {
    case LE_AUDIO_MSG_BASS_REMOTE_CP:
        {
            T_BASS_REMOTE_CP *p_data = (T_BASS_REMOTE_CP *)buf;
            APP_PRINT_INFO2("LE_AUDIO_MSG_BASS_REMOTE_CP: conn_handle 0x%x, cp opcode 0x%x",
                            p_data->conn_handle, p_data->p_cp_data->opcode);
            conn_handle = p_data->conn_handle;

            switch (p_data->p_cp_data->opcode)
            {
            case BASS_CP_OP_ADD_SOURCE:
                {
                    //when enter bass mode, big_passive_flag set to 1
                    big_passive_flag = 1;
                    //TODO: to reset when big termintate
                    if (big_passive_flag)
                    {
                        app_lea_bis_state_change(LE_AUDIO_BIS_STATE_CONN);
                    }
                    //before big sync, pause a2dp
                    app_le_audio_a2dp_pause();
                }
                break;

            case BASS_CP_OP_MODIFY_SOURCE:
                /* to synchronize to, or to stop synchronization to, a PA and/or a BIS.
                 Add or update Metadata*/

                break;

            case BASS_CP_OP_REMOVE_SOURCE:
                app_lea_bis_state_change(LE_AUDIO_BIS_STATE_IDLE);
                break;

            default:
                break;
            }
            app_result = APP_RESULT_ACCEPT;

        }
        break;

    case LE_AUDIO_MSG_BASS_BRS_MODIFY:
        {
            T_BASS_BRS_MODIFY *p_data = (T_BASS_BRS_MODIFY *)buf;
            APP_PRINT_INFO2("LE_AUDIO_MSG_BASS_BRS_MODIFY: sync handle %p, source_id %d",
                            p_data->handle, p_data->source_id);
            if (p_data->p_brs_data)
            {
                if (p_data->p_brs_data->brs_is_used)
                {
                    APP_PRINT_INFO2("source_address_type 0x%02x, source_adv_sid 0x%02x",
                                    p_data->p_brs_data->source_address_type, p_data->p_brs_data->source_adv_sid);
                    APP_PRINT_INFO4("pa_sync_state %d, bis_sync_state 0x%x, big_encryption %d, num_subgroups %d",
                                    p_data->p_brs_data->pa_sync_state, p_data->p_brs_data->bis_sync_state,
                                    p_data->p_brs_data->big_encryption, p_data->p_brs_data->num_subgroups);
                    for (uint8_t i = 0; i < p_data->p_brs_data->num_subgroups; i++)
                    {
                        if (p_data->p_brs_data->p_cp_bis_info[i].metadata_len == 0)
                        {
                            APP_PRINT_INFO2("Subgroup[%d], BIS Sync: [0x%x], Metadata Length: [0]", i,
                                            p_data->p_brs_data->p_cp_bis_info[i].bis_sync);
                        }
                        else
                        {
                            APP_PRINT_INFO4("Subgroup[%d], BIS Sync: [0x%x] Metadata data[%d]: %b", i,
                                            p_data->p_brs_data->p_cp_bis_info[i].bis_sync,
                                            p_data->p_brs_data->p_cp_bis_info[i].metadata_len,
                                            TRACE_BINARY(p_data->p_brs_data->p_cp_bis_info[i].metadata_len,
                                                         p_data->p_brs_data->p_cp_bis_info[i].p_metadata));
                        }
                    }
                }
                else
                {
                    APP_PRINT_INFO0("LE_AUDIO_MSG_BASS_BRS_MODIFY: brs char not used");
                }
            }
        }
        break;

    case LE_AUDIO_MSG_BASS_BA_ADD_SOURCE:
        {
            T_BASS_BA_ADD_SOURCE *p_data = (T_BASS_BA_ADD_SOURCE *)buf;
            APP_PRINT_INFO2("LE_AUDIO_MSG_BASS_BA_ADD_SOURCE: sync handle %p, source_id %d",
                            p_data->handle, p_data->source_id);
            sync_handle = p_data->handle;
            source_id = p_data->source_id;
            ble_audio_sync_update_cb(p_data->handle, app_bass_pa_sync_cb);
        }
        break;

    case LE_AUDIO_MSG_BASS_LOCAL_ADD_SOURCE:
        {
            T_BASS_BASS_LOCAL_ADD_SOURCE *p_data = (T_BASS_BASS_LOCAL_ADD_SOURCE *)buf;
            source_id = p_data->source_id;
            APP_PRINT_INFO2("LE_AUDIO_MSG_BASS_LOCAL_ADD_SOURCE: sync handle %p, source_id %d",
                            p_data->handle, p_data->source_id);
        }
        break;

    case LE_AUDIO_MSG_BASS_GET_PA_SYNC_PARAM:
        {
            T_BASS_GET_PA_SYNC_PARAM *p_data = (T_BASS_GET_PA_SYNC_PARAM *)buf;
            APP_PRINT_INFO4("LE_AUDIO_MSG_BASS_GET_PA_SYNC_PARAM: sync handle %p, source_id %d, is_past %d, pa_interval 0x%x",
                            p_data->handle, p_data->source_id,
                            p_data->is_past, p_data->pa_interval);
#if 0
            uint16_t pa_sync_timeout_10ms = 200;
            bass_set_pa_sync_param(p_data->source_id, 0, 0, pa_sync_timeout_10ms, 300);
#endif
        }
        break;

    case LE_AUDIO_MSG_BASS_GET_BIG_SYNC_PARAM:
        {
            T_BASS_GET_BIG_SYNC_PARAM *p_data = (T_BASS_GET_BIG_SYNC_PARAM *)buf;
            APP_PRINT_INFO2("LE_AUDIO_MSG_BASS_GET_BIG_SYNC_PARAM: sync handle %p, source_id %d",
                            p_data->handle, p_data->source_id);
#if 0
            T_BLE_AUDIO_SYNC_INFO sync_info;
            uint16_t big_sync_timeout_10ms = 200;
            if (ble_audio_sync_get_info(p_data->handle, &sync_info))
            {
                if (sync_info.big_info_received)
                {
                    big_sync_timeout_10ms = ((sync_info.big_info.iso_interval * 1.25) / 10) * 20;
                }

            }
            bass_set_big_sync_param(p_data->source_id, 0, big_sync_timeout_10ms);
#endif
        }
        break;

    case LE_AUDIO_MSG_BASS_GET_BROADCAST_CODE:
        {
            T_BASS_GET_BROADCAST_CODE *p_data = (T_BASS_GET_BROADCAST_CODE *)buf;
            APP_PRINT_INFO2("LE_AUDIO_MSG_BASS_GET_BROADCAST_CODE: sync handle %p, source_id %d",
                            p_data->handle, p_data->source_id);
            //If local save broadcast code, use bass_set_broadcast_code to set
            //bass_set_broadcast_code(p_data->source_id, TSPX_Broadcast_Code);
        }
        break;

    case LE_AUDIO_MSG_BASS_GET_PREFER_BIS_SYNC:
        {
            T_BASS_SET_PREFER_BIS_SYNC *p_data = (T_BASS_SET_PREFER_BIS_SYNC *)buf;
            APP_PRINT_INFO3("LE_AUDIO_MSG_BASS_GET_PREFER_BIS_SYNC: sync handle %p, source_id %d, num_subgroups %d",
                            p_data->handle, p_data->source_id,
                            p_data->num_subgroups);
#if 0
            T_BLE_AUDIO_SYNC_INFO sync_info;
            for (uint8_t i = 0; i < p_data->num_subgroups; i++)
            {
                APP_PRINT_INFO2("bis_data[%d]: bis_sync 0x%x", i,
                                p_data->p_cp_bis_info[i].bis_sync);
                APP_PRINT_INFO3("bis_data[%d]: metadata[%d] %b", i,
                                p_data->p_cp_bis_info[i].metadata_len,
                                TRACE_BINARY(p_data->p_cp_bis_info[i].metadata_len, p_data->p_cp_bis_info[i].p_metadata));
                if (p_data->p_cp_bis_info->bis_sync == BASS_CP_BIS_SYNC_NO_PREFER)
                {
                    if (ble_audio_sync_get_info(p_data->handle, &sync_info))
                    {
                        if (sync_info.p_base_mapping)
                        {
                            if (i < sync_info.p_base_mapping->num_subgroups)
                            {
                                if (bass_set_prefer_bis_sync(p_data->source_id, i,
                                                             sync_info.p_base_mapping->p_subgroup[i].bis_array) == false)
                                {
                                    goto failed;
                                }
                            }
                        }

                    }

                }
            }
#endif
        }
        break;

    case LE_AUDIO_MSG_BASS_BRS_CHAR_NO_EMPTY:
        {
            T_BASS_BRS_CHAR_NO_EMPTY *p_data = (T_BASS_BRS_CHAR_NO_EMPTY *)buf;
            APP_PRINT_INFO1("LE_AUDIO_MSG_BASS_BRS_CHAR_NO_EMPTY:conn_handle 0x%x",
                            p_data->conn_handle);
            //ble_audio_sync_realese(bqb_bap_sde_sink.sync_handle);
        }
        break;
    default:
        break;
    }
    return app_result;
}


#endif

