/*
 * Copyright (c) 2018, Realsil Semiconductor Corporation. All rights reserved.
 */

#ifndef _APP_AUTO_POWER_OFF_H_
#define _APP_AUTO_POWER_OFF_H_

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

void app_auto_power_off_enable(uint32_t flag, uint16_t timeout);

void app_auto_power_off_disable(uint32_t flag);

void app_auto_power_off_init(void);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _APP_AUTO_POWER_OFF_H_ */
