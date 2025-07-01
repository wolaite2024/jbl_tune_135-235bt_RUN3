#ifndef _APP_BROADCAST_SYNC_H_
#define _APP_BROADCAST_SYNC_H_

#ifdef  __cplusplus
extern "C" {
#endif      /* __cplusplus */
#include "ble_audio_sync.h"

extern uint8_t g_dev_idx;

#define APP_BC_SOURCE_NUM 4
#define MAX_BIS_AUDIO_OUTPUT_LEVEL    15
#define MAX_BIS_AUDIO_VOLUME_SETTING  255

typedef struct
{
    bool                    used;
    uint8_t                 advertiser_address[GAP_BD_ADDR_LEN];
    uint8_t                 advertiser_address_type;
    uint8_t                 advertiser_sid;
    T_BLE_AUDIO_SYNC_HANDLE sync_handle;

    T_GAP_PA_SYNC_STATE     sync_state;
} T_BC_SOURCE_INFO;

extern T_BC_SOURCE_INFO source_list[];
bool app_bc_sync_add_device(uint8_t *bd_addr, uint8_t bd_type, uint8_t adv_sid);
bool app_bc_sync_pa_sync(uint8_t dev_idx);
bool app_bc_set_active(uint8_t dev_idx);
bool app_bc_get_active(uint8_t *dev_idx);
void app_bc_reset(void);
bool app_bc_find_handle(T_BLE_AUDIO_SYNC_HANDLE *handle, uint8_t *dev_idx);
bool app_bc_sync_clear_device(T_BLE_AUDIO_SYNC_HANDLE handle);


bool app_bc_sync_pa_terminate(uint8_t dev_idx);
bool app_bc_sync_big_establish(uint8_t dev_idx);
bool app_bc_sync_big_terminate(uint8_t dev_idx);

bool app_sink_pa_terminate(uint8_t source_id);
uint16_t app_bc_get_bis_handle(void);

#ifdef  __cplusplus
}
#endif      /*  __cplusplus */

#endif
