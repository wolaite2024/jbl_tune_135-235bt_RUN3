#ifndef _APP_DONGLE_SPP_H_
#define _APP_DONGLE_SPP_H_

#include <stdbool.h>
#include "app_sdp.h"

#define VOICE_OVER_SPP_START_BIT                0x52
#define VOICE_OVER_SPP_STOP_BIT                 0x54

#define VOICE_OVER_SPP_PAYLOAD_LEHGTH           0x02

#define VOICE_OVER_SPP_DATA                     0x00
#define VOICE_OVER_SPP_CMD                      0x01
#define VOICE_OVER_SPP_CMD_SET_GAMING_MOE       0x01
#define VOICE_OVER_SPP_CMD_REQUEST_GAMING_MOE   0x02
#define VOICE_OVER_SPP_CMD_REQ_OPEN_MIC         0x03

void app_dongle_set_ext_eir(void);
bool app_dongle_get_spp_conn_flag(void);
void app_dongle_updata_mic_data_idx(bool is_reset);
bool app_dongle_spp_init(void);
void app_dongle_handle_gaming_mode_cmd(uint8_t action);
#endif
