#include "trace.h"
#include "ascs_def.h"
#include "ascs_mgr.h"
#include "ble_conn.h"
#include "app_ascs.h"
#include "app_link_util.h"

#if (F_APP_TMAP_CT_SUPPORT || F_APP_TMAP_UMR_SUPPORT)
void app_ascs_init(void)
{
    uint8_t snk_ase_num = 2;
    uint8_t src_ase_num = 1;

    ascs_init(snk_ase_num, src_ase_num);
}

T_APP_RESULT app_ascs_handle_msg(T_LE_AUDIO_MSG msg, void *buf)
{
    T_APP_RESULT app_result = APP_RESULT_SUCCESS;
    uint8_t i = 0;
    APP_PRINT_INFO1("app_ascs_handle_msg: msg %x", msg);

    switch (msg)
    {
    case LE_AUDIO_MSG_ASCS_SETUP_DATA_PATH:
        {
            T_ASCS_SETUP_DATA_PATH *p_data = (T_ASCS_SETUP_DATA_PATH *)buf;
            APP_PRINT_INFO4("LE_AUDIO_MSG_ASCS_SETUP_DATA_PATH: conn_handle 0x%x, ase_id %d, path_direction 0x%x, cis_conn_handle 0x%x",
                            p_data->conn_handle,
                            p_data->ase_id,
                            p_data->path_direction,
                            p_data->cis_conn_handle);
            app_le_audio_link_sm(p_data->conn_handle, LE_AUDIO_SETUP_DATA_PATH, p_data);
        }
        break;

    case LE_AUDIO_MSG_ASCS_REMOVE_DATA_PATH:
        {
            T_ASCS_REMOVE_DATA_PATH *p_data = (T_ASCS_REMOVE_DATA_PATH *)buf;
            APP_PRINT_INFO4("LE_AUDIO_MSG_ASCS_REMOVE_DATA_PATH: conn_handle 0x%x, ase_id %d, path_direction 0x%x, cis_conn_handle 0x%x",
                            p_data->conn_handle,
                            p_data->ase_id,
                            p_data->path_direction,
                            p_data->cis_conn_handle);
            app_le_audio_link_sm(p_data->conn_handle, LE_AUDIO_REMOVE_DATA_PATH, p_data);
        }
        break;

    case LE_AUDIO_MSG_ASCS_ASE_STATE:
        {
            T_ASCS_ASE_STATE *p_data = (T_ASCS_ASE_STATE *)buf;
            APP_PRINT_INFO3("LE_AUDIO_MSG_ASCS_ASE_STATE: conn_handle 0x%x, ase_id %d, ase_state %d",
                            p_data->conn_handle,
                            p_data->ase_data.ase_id,
                            p_data->ase_data.ase_state);

            switch (p_data->ase_data.ase_state)
            {
            case ASE_STATE_IDLE:
                APP_PRINT_INFO0("ASE_STATE_IDLE");
                break;

            case ASE_STATE_RELEASING:
                APP_PRINT_INFO0("ASE_STATE_RELEASING");
                app_le_audio_link_sm(p_data->conn_handle, LE_AUDIO_PAUSE, NULL);
                break;

            case ASE_STATE_CODEC_CONFIGURED:
                {
                    uint32_t presentation_delay_min;
                    uint32_t presentation_delay_max;
                    uint16_t max_transport_latency;
                    LE_ARRAY_TO_UINT24(presentation_delay_min,
                                       p_data->ase_data.param.codec_configured.data.presentation_delay_min);
                    LE_ARRAY_TO_UINT24(presentation_delay_max,
                                       p_data->ase_data.param.codec_configured.data.presentation_delay_max);
                    LE_ARRAY_TO_UINT16(max_transport_latency,
                                       p_data->ase_data.param.codec_configured.data.max_transport_latency);

                    APP_PRINT_INFO4("[BAP][ASCS] ASE_STATE_CODEC_CONFIGURED: supported_framing 0x%01x, preferred_phy 0x%01x, preferred_retrans_number 0x%01x",
                                    p_data->ase_data.param.codec_configured.data.supported_framing,
                                    p_data->ase_data.param.codec_configured.data.preferred_phy,
                                    p_data->ase_data.param.codec_configured.data.preferred_retrans_number,
                                    p_data->ase_data.param.codec_configured.data.preferred_retrans_number);
                    APP_PRINT_INFO3("[BAP][ASCS] ASE_STATE_CODEC_CONFIGURED: max_transport_latency 0x%02x, presentation_delay_min 0x%03x, presentation_delay_max 0x%03x",
                                    max_transport_latency,
                                    presentation_delay_min, presentation_delay_max);
                    APP_PRINT_INFO2("[BAP][ASCS] ASE_STATE_CODEC_CONFIGURED: codec_id %b, codec_spec_cfg_len %d",
                                    TRACE_BINARY(5, p_data->ase_data.param.codec_configured.data.codec_id),
                                    p_data->ase_data.param.codec_configured.data.codec_spec_cfg_len);
                    if (p_data->ase_data.param.codec_configured.data.codec_spec_cfg_len)
                    {
                        APP_PRINT_INFO1("[BAP][ASCS] codec_spec_cfg %b",
                                        TRACE_BINARY(p_data->ase_data.param.codec_configured.data.codec_spec_cfg_len,
                                                     p_data->ase_data.param.codec_configured.p_codec_spec_cfg));
                    }
                }
                break;

            case ASE_STATE_QOS_CONFIGURED:
                {
                    uint16_t max_sdu_size;
                    uint16_t max_transport_latency;
                    uint32_t sdu_interval;
                    uint32_t presentation_delay;
                    LE_ARRAY_TO_UINT24(sdu_interval, p_data->ase_data.param.qos_configured.sdu_interval);
                    LE_ARRAY_TO_UINT24(presentation_delay, p_data->ase_data.param.qos_configured.presentation_delay);
                    LE_ARRAY_TO_UINT16(max_sdu_size, p_data->ase_data.param.qos_configured.max_sdu);
                    LE_ARRAY_TO_UINT16(max_transport_latency,
                                       p_data->ase_data.param.qos_configured.max_transport_latency);
                    APP_PRINT_INFO5("[BAP][ASCS] ASE_STATE_QOS_CONFIGURED: cig_id %d, cis_id %d, sdu_interval 0x%03x, framing 0x%01x, phy 0x%x",
                                    p_data->ase_data.param.qos_configured.cig_id,
                                    p_data->ase_data.param.qos_configured.cis_id, sdu_interval,
                                    p_data->ase_data.param.qos_configured.framing,
                                    p_data->ase_data.param.qos_configured.phy);
                    APP_PRINT_INFO4("[BAP][ASCS] ASE_STATE_QOS_CONFIGURED: max_sdu 0x%02x, retransmission_number %d, max_transport_latency 0x%02x, presentation_delay 0x%03x",
                                    max_sdu_size, p_data->ase_data.param.qos_configured.retransmission_number,
                                    max_transport_latency, presentation_delay);
                }
                break;

            case ASE_STATE_ENABLING:
                APP_PRINT_INFO4("[BAP][ASCS] ASE_STATE_ENABLING: cig_id %d, cis_id %d, metadata[%d] %b",
                                p_data->ase_data.param.enabling.cig_id,
                                p_data->ase_data.param.enabling.cis_id,
                                p_data->ase_data.param.enabling.metadata_length,
                                TRACE_BINARY(p_data->ase_data.param.enabling.metadata_length,
                                             p_data->ase_data.param.enabling.p_metadata));
                break;

            case ASE_STATE_STREAMING:
                {
                    APP_PRINT_INFO4("[BAP][ASCS] ASE_STATE_STREAMING: cig_id %d, cis_id %d, metadata[%d] %b",
                                    p_data->ase_data.param.streaming.cig_id,
                                    p_data->ase_data.param.streaming.cis_id,
                                    p_data->ase_data.param.streaming.metadata_length,
                                    TRACE_BINARY(p_data->ase_data.param.streaming.metadata_length,
                                                 p_data->ase_data.param.streaming.p_metadata));
                    app_le_audio_link_sm(p_data->conn_handle, LE_AUDIO_STREAMING, NULL);
                }
                break;

            case ASE_STATE_DISABLING:
                {
                    APP_PRINT_INFO4("[BAP][ASCS] ASE_STATE_DISABLING: cig_id %d, cis_id %d, metadata[%d] %b",
                                    p_data->ase_data.param.disabling.cig_id,
                                    p_data->ase_data.param.disabling.cis_id,
                                    p_data->ase_data.param.disabling.metadata_length,
                                    TRACE_BINARY(p_data->ase_data.param.disabling.metadata_length,
                                                 p_data->ase_data.param.disabling.p_metadata));
                    // ascs_action_rec_stop_ready(p_data->conn_handle, ASCS_TEST_ASE_ID);
                }
                break;

            default:
                break;
            }
        }
        break;

    case LE_AUDIO_MSG_ASCS_CP_CONFIG_CODEC:
        {
            T_ASCS_CP_CONFIG_CODEC *p_data = (T_ASCS_CP_CONFIG_CODEC *)buf;
            APP_PRINT_INFO2("LE_AUDIO_MSG_ASCS_CP_CONFIG_CODEC: conn_handle 0x%x, number_of_ase %d",
                            p_data->conn_handle,
                            p_data->number_of_ase);
            for (i = 0; i < p_data->number_of_ase; i++)
            {
                APP_PRINT_INFO5("ase param[%d]: ase_id %d, target_latency %d, target_phy %d, codec_id %b",
                                i, p_data->param[i].data.ase_id,
                                p_data->param[i].data.target_latency,
                                p_data->param[i].data.target_phy,
                                TRACE_BINARY(5, p_data->param[i].data.codec_id));

                APP_PRINT_INFO7("ase param[%d]: type_exist 0x%x, frame_duration 0x%x, sample_frequency 0x%x, codec_frame_blocks_per_sdu %d, octets_per_codec_frame %d, audio_channel_allocation 0x%x",
                                i, p_data->param[i].codec_parsed_data.type_exist,
                                p_data->param[i].codec_parsed_data.frame_duration,
                                p_data->param[i].codec_parsed_data.sample_frequency,
                                p_data->param[i].codec_parsed_data.codec_frame_blocks_per_sdu,
                                p_data->param[i].codec_parsed_data.octets_per_codec_frame,
                                p_data->param[i].codec_parsed_data.audio_channel_allocation
                               );
            }
            app_le_audio_link_sm(p_data->conn_handle, LE_AUDIO_CONFIG_CODEC, p_data);
        }
        break;

    case LE_AUDIO_MSG_ASCS_CP_CONFIG_QOS:
        {
            T_ASCS_CP_CONFIG_QOS *p_data = (T_ASCS_CP_CONFIG_QOS *)buf;
            APP_PRINT_INFO2("LE_AUDIO_MSG_ASCS_CP_CONFIG_QOS: conn_handle 0x%x, number_of_ase %d",
                            p_data->conn_handle,
                            p_data->number_of_ase);
            for (i = 0; i < p_data->number_of_ase; i++)
            {
                APP_PRINT_INFO4("ase param[%d]: ase_id %d,  cig_id %d, cis_id %d",
                                i, p_data->param[i].ase_id,
                                p_data->param[i].cig_id, p_data->param[i].cis_id);
                APP_PRINT_INFO8("ase param[%d]: sdu_interval %x %x %x, framing %d, phy 0x%x, max_sdu  %x %x",
                                i, p_data->param[i].sdu_interval[0], p_data->param[i].sdu_interval[1],
                                p_data->param[i].sdu_interval[2],
                                p_data->param[i].framing, p_data->param[i].phy,
                                p_data->param[i].max_sdu[0], p_data->param[i].max_sdu[1]);

                APP_PRINT_INFO7("ase param[%d]: retransmission_number %d, max_transport_latency %x %x, presentation_delay  %x %x %x",
                                i,
                                p_data->param[i].retransmission_number,
                                p_data->param[i].max_transport_latency[0],
                                p_data->param[i].max_transport_latency[1],
                                p_data->param[i].presentation_delay[0],
                                p_data->param[i].presentation_delay[1],
                                p_data->param[i].presentation_delay[2]);
            }
            app_le_audio_link_sm(p_data->conn_handle, LE_AUDIO_CONFIG_QOS, p_data);
        }
        break;

    case LE_AUDIO_MSG_ASCS_CP_ENABLE:
        {
            T_ASCS_CP_ENABLE *p_data = (T_ASCS_CP_ENABLE *)buf;
            APP_PRINT_INFO2("LE_AUDIO_MSG_ASCS_CP_ENABLE: conn_handle 0x%x, number_of_ase %d",
                            p_data->conn_handle,
                            p_data->number_of_ase);
            for (i = 0; i < p_data->number_of_ase; i++)
            {
                APP_PRINT_INFO4("ase param[%d]: ase_id %d, metadata_length %d, metadata %b",
                                i, p_data->param[i].ase_id,
                                p_data->param[i].metadata_length,
                                TRACE_BINARY(p_data->param[i].metadata_length, p_data->param[i].p_metadata));
            }
            app_le_audio_link_sm(p_data->conn_handle, LE_AUDIO_ENABLE, p_data);
            app_result = APP_RESULT_ACCEPT;
        }
        break;

    case LE_AUDIO_MSG_ASCS_CP_DISABLE:
        {
            T_ASCS_CP_DISABLE *p_data = (T_ASCS_CP_DISABLE *)buf;
            app_le_audio_link_sm(p_data->conn_handle, LE_AUDIO_PAUSE, NULL);
            APP_PRINT_INFO3("LE_AUDIO_MSG_ASCS_CP_DISABLE: conn_handle 0x%x, number_of_ase %d, ase_ids %b",
                            p_data->conn_handle,
                            p_data->number_of_ase, TRACE_BINARY(p_data->number_of_ase, p_data->ase_id));
        }
        break;

    case LE_AUDIO_MSG_ASCS_CP_UPDATE_METADATA:
        {
            T_ASCS_CP_UPDATE_METADATA *p_data = (T_ASCS_CP_UPDATE_METADATA *)buf;
            APP_PRINT_INFO2("LE_AUDIO_MSG_ASCS_CP_UPDATE_METADATA: conn_handle 0x%x, number_of_ase %d",
                            p_data->conn_handle,
                            p_data->number_of_ase);
            for (i = 0; i < p_data->number_of_ase; i++)
            {
                APP_PRINT_INFO4("ase param[%d]: ase_id %d, metadata_length %d, metadata %b",
                                i, p_data->param[i].ase_id,
                                p_data->param[i].metadata_length,
                                TRACE_BINARY(p_data->param[i].metadata_length, p_data->param[i].p_metadata));
            }
        }
        break;
    case LE_AUDIO_MSG_ASCS_CIS_REQUEST_IND:
        {
            T_ASCS_CIS_REQUEST_IND *p_data = (T_ASCS_CIS_REQUEST_IND *)buf;
            APP_PRINT_INFO6("LE_AUDIO_MSG_ASCS_CIS_REQUEST_IND: conn_handle 0x%x, cis_conn_handle 0x%x, snk_ase_id %d, snk_ase_state %d, src_ase_id %d, src_ase_state %d",
                            p_data->conn_handle, p_data->cis_conn_handle,
                            p_data->snk_ase_id, p_data->snk_ase_state,
                            p_data->src_ase_id, p_data->src_ase_state);
            app_result = APP_RESULT_ACCEPT;
        }
        break;

    default:
        break;
    }
    return app_result;
}
#endif
