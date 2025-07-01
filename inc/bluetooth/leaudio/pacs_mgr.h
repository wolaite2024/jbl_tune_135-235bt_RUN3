#ifndef _PACS_MGR_H_
#define _PACS_MGR_H_

#ifdef  __cplusplus
extern "C" {
#endif      /* __cplusplus */

#include "ble_audio_def.h"
#if LE_AUDIO_PACS_SUPPORT

//LE_AUDIO_MSG_PACS_WRITE_SINK_AUDIO_LOC
typedef struct
{
    uint16_t conn_handle;
    uint32_t sink_audio_locations;
} T_PACS_WRITE_SINK_AUDIO_LOC;

//LE_AUDIO_MSG_PACS_WRITE_SOURCE_AUDIO_LOC
typedef struct
{
    uint16_t conn_handle;
    uint32_t source_audio_locations;
} T_PACS_WRITE_SOURCE_AUDIO_LOC;

//LE_AUDIO_MSG_PACS_AVAILABLE_CONTEXTS_READ_IND
typedef struct
{
    uint16_t conn_handle;
} T_PACS_VAILABLE_CONTEXTS_READ_IND;

// **** Call these api before pacs_init **** //
uint8_t pac_register(T_AUDIO_DIRECTION direction, const uint8_t *pac_data, uint8_t pac_data_len,
                     bool notify);
bool pacs_char_sink_audio_locations_prop(bool is_notify, bool is_write, bool is_exist);
bool pacs_char_source_audio_locations_prop(bool is_notify, bool is_write, bool is_exist);
bool pacs_char_supported_audio_contexts_prop(bool is_notify);
void pacs_init(void);
//
bool pacs_pac_update(uint8_t pac_id, const uint8_t *pac_data, uint8_t pac_data_len);
bool pacs_sink_audio_locations_set(uint32_t sink_audio_location, bool init);
bool pacs_source_audio_locations_set(uint32_t source_audio_location, bool init);
bool pacs_available_contexts_read_cfm(uint16_t conn_handle, uint16_t sink_available_contexts,
                                      uint16_t source_available_contexts);
bool pacs_available_contexts_notify(uint16_t conn_handle, uint16_t sink_available_contexts,
                                    uint16_t source_available_contexts);
bool pacs_audio_supported_contexts_set(uint16_t sink_supported_contexts,
                                       uint16_t source_supported_contexts, bool init);
#endif

#ifdef  __cplusplus
}
#endif      /*  __cplusplus */

#endif
