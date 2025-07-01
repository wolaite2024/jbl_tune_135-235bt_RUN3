#include "trace.h"
#include "codec_def.h"
#include "metadata_def.h"
#include "pacs_mgr.h"
#include "app_pacs.h"

#if (F_APP_TMAP_CT_SUPPORT || F_APP_TMAP_UMR_SUPPORT || F_APP_TMAP_BMR_SUPPORT)
//Mandary
const uint8_t pac_sink_codec[] =
{
    //Number_of_PAC_records
    2,
    //PAC Record
    LC3_CODEC_ID, 0, 0, 0, 0,//Codec_ID
    //Codec_Specific_Capabilities_Length
    16,
    //Codec_Specific_Capabilities
    0x03,
    CODEC_CAP_TYPE_SUPPORTED_SAMPLING_FREQUENCIES,
    (uint8_t)(SAMPLING_FREQUENCY_48K),
    (uint8_t)((SAMPLING_FREQUENCY_48K) >> 8),
    0x02,
    CODEC_CAP_TYPE_SUPPORTED_FRAME_DURATIONS,
    FRAME_DURATION_PREFER_10_MS_BIT | FRAME_DURATION_10_MS_BIT,
    0x02,
    CODEC_CAP_TYPE_AUDIO_CHANNEL_COUNTS,
    AUDIO_CHANNEL_COUNTS_1 | AUDIO_CHANNEL_COUNTS_2,
    0x05,
    CODEC_CAP_TYPE_SUPPORTED_OCTETS_PER_CODEC_FRAME,
    0x78, 0x00, 0x78, 0x00,
    //Metadata_Length
    0x04,
    //Metadata
    0x03,
    METADATA_TYPE_PREFERRED_AUDIO_CONTEXTS,
    (uint8_t)(AUDIO_CONTEXT_MEDIA),
    (uint8_t)(AUDIO_CONTEXT_MEDIA >> 8), //must fit dongle
    //PAC Record
    LC3_CODEC_ID, 0, 0, 0, 0,//Codec_ID
    //Codec_Specific_Capabilities_Length
    16,
    //Codec_Specific_Capabilities
    0x03,
    CODEC_CAP_TYPE_SUPPORTED_SAMPLING_FREQUENCIES,
    (uint8_t)(SAMPLING_FREQUENCY_16K),
    (uint8_t)((SAMPLING_FREQUENCY_16K) >> 8),
    0x02,
    CODEC_CAP_TYPE_SUPPORTED_FRAME_DURATIONS,
    FRAME_DURATION_PREFER_10_MS_BIT | FRAME_DURATION_10_MS_BIT,
    0x02,
    CODEC_CAP_TYPE_AUDIO_CHANNEL_COUNTS,
    AUDIO_CHANNEL_COUNTS_1 | AUDIO_CHANNEL_COUNTS_2,
    0x05,
    CODEC_CAP_TYPE_SUPPORTED_OCTETS_PER_CODEC_FRAME,
    0x28, 0x00, 0x28, 0x00,
    //Metadata_Length
    0x04,
    //Metadata
    0x03,
    METADATA_TYPE_PREFERRED_AUDIO_CONTEXTS,
    (uint8_t)(AUDIO_CONTEXT_CONVERSATIONAL),
    (uint8_t)(AUDIO_CONTEXT_CONVERSATIONAL >> 8)
};

const uint8_t pac_source_codec[] =
{
    //Number_of_PAC_records
    1,
    //PAC Record
    LC3_CODEC_ID, 0, 0, 0, 0,//Codec_ID
    //Codec_Specific_Capabilities_Length
    16,
    //Codec_Specific_Capabilities
    0x03,
    CODEC_CAP_TYPE_SUPPORTED_SAMPLING_FREQUENCIES,
    (uint8_t)(SAMPLING_FREQUENCY_16K),
    (uint8_t)(SAMPLING_FREQUENCY_16K >> 8),
    0x02,
    CODEC_CAP_TYPE_SUPPORTED_FRAME_DURATIONS,
    FRAME_DURATION_PREFER_10_MS_BIT | FRAME_DURATION_10_MS_BIT,
    0x02,
    CODEC_CAP_TYPE_AUDIO_CHANNEL_COUNTS,
    AUDIO_CHANNEL_COUNTS_1 | AUDIO_CHANNEL_COUNTS_2,
    0x05,
    CODEC_CAP_TYPE_SUPPORTED_OCTETS_PER_CODEC_FRAME,
    0x28, 0x00, 0x28, 0x00,
    //Metadata_Length
    0x04,
    //Metadata
    0x03,
    METADATA_TYPE_PREFERRED_AUDIO_CONTEXTS,
    (uint8_t)(AUDIO_CONTEXT_CONVERSATIONAL),
    (uint8_t)(AUDIO_CONTEXT_CONVERSATIONAL >> 8)
};

uint16_t sink_available_contexts = AUDIO_CONTEXT_MEDIA | AUDIO_CONTEXT_CONVERSATIONAL;
uint16_t source_available_contexts = AUDIO_CONTEXT_MEDIA | AUDIO_CONTEXT_CONVERSATIONAL;


T_APP_RESULT app_pacs_handle_msg(T_LE_AUDIO_MSG msg, void *buf)
{
    T_APP_RESULT app_result = APP_RESULT_SUCCESS;
    switch (msg)
    {
    case LE_AUDIO_MSG_PACS_AVAILABLE_CONTEXTS_READ_IND:
        {
            T_PACS_VAILABLE_CONTEXTS_READ_IND *p_data = (T_PACS_VAILABLE_CONTEXTS_READ_IND *)buf;
            APP_PRINT_INFO1("LE_AUDIO_MSG_PACS_AVAILABLE_CONTEXTS_READ_IND: conn_handle 0x%x",
                            p_data->conn_handle);
            pacs_available_contexts_read_cfm(p_data->conn_handle, sink_available_contexts,
                                             source_available_contexts);
        }
        break;

    case LE_AUDIO_MSG_PACS_WRITE_SINK_AUDIO_LOC:
        {
            T_PACS_WRITE_SINK_AUDIO_LOC *p_data = (T_PACS_WRITE_SINK_AUDIO_LOC *)buf;
            APP_PRINT_INFO2("LE_AUDIO_MSG_PACS_WRITE_SINK_AUDIO_LOC: conn_handle 0x%x, sink_audio_locations 0x%x",
                            p_data->conn_handle,
                            p_data->sink_audio_locations);
        }
        break;

    case LE_AUDIO_MSG_PACS_WRITE_SOURCE_AUDIO_LOC:
        {
            T_PACS_WRITE_SOURCE_AUDIO_LOC *p_data = (T_PACS_WRITE_SOURCE_AUDIO_LOC *)buf;
            APP_PRINT_INFO2("LE_AUDIO_MSG_PACS_WRITE_SOURCE_AUDIO_LOC: conn_handle 0x%x, source_audio_locations 0x%x",
                            p_data->conn_handle,
                            p_data->source_audio_locations);
        }
        break;
    default:
        break;
    }
    return app_result;
}

void app_pacs_init(void)
{
    uint32_t sink_loc = AUDIO_LOCATION_FL | AUDIO_LOCATION_FR;
    uint32_t source_loc = AUDIO_LOCATION_FL | AUDIO_LOCATION_FR;

    pac_register(SERVER_AUDIO_SOURCE, pac_source_codec,
                 sizeof(pac_source_codec), false);
    pac_register(SERVER_AUDIO_SINK, pac_sink_codec, sizeof(pac_sink_codec), false);

    pacs_char_sink_audio_locations_prop(true, true, true);
    pacs_char_source_audio_locations_prop(true, true, true);
    pacs_char_supported_audio_contexts_prop(true);
    pacs_audio_supported_contexts_set(sink_available_contexts, source_available_contexts, true);
    pacs_sink_audio_locations_set(sink_loc, true);
    pacs_source_audio_locations_set(source_loc, true);

    pacs_init();
}
#endif
