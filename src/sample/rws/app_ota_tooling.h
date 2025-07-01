/*
 * Copyright (c) 2018, Realsil Semiconductor Corporation. All rights reserved.
 */

#ifndef _APP_OTA_ADV_H_
#define _APP_OTA_ADV_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

void app_ota_adv_init(void);
void app_ota_adv_start(uint8_t dongle_id, uint16_t timeout_s);
void app_ota_adv_stop(void);

#ifdef __cplusplus
}
#endif

#endif

