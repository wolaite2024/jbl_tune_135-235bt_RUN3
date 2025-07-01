/*
 * Copyright (c) 2018, Realsil Semiconductor Corporation. All rights reserved.
 */

#ifndef _APP_IAP_RTK_H_
#define _APP_IAP_RTK_H_

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

void app_iap_rtk_init(void);

bool app_iap_rtk_create(uint8_t *bd_addr);

bool app_iap_rtk_delete(uint8_t *bd_addr);

void app_iap_rtk_handle_remote_conn_cmpl(void);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _APP_IAP_RTK_H_ */
