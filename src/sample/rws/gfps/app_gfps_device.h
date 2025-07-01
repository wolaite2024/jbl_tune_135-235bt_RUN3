/*
 * Copyright (c) 2018, Realsil Semiconductor Corporation. All rights reserved.
 */

#ifndef _APP_GFPS_DEVICE_H_
#define _APP_GFPS_DEVICE_H_

#include "btm.h"

#if GFPS_FEATURE_SUPPORT
#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/** @defgroup APP_RWS_GFPS App Gfps
  * @brief App Gfps
  * @{
  */

void app_gfps_handle_remote_conn_cmpl(void);
void app_gfps_handle_remote_disconnect_cmpl(void);
void app_gfps_handle_factory_reset(void);

void app_gfps_handle_power_off(void);
void app_gfps_handle_power_on(bool is_pairing);
void app_gfps_handle_radio_mode(T_BT_DEVICE_MODE radio_mode);
void app_gfps_handle_role_swap(T_REMOTE_SESSION_ROLE device_role);
void app_gfps_handle_engage_role_decided(T_BT_DEVICE_MODE radio_mode);
void app_gfps_handle_b2s_link_disconnected(void);
void app_gfps_handle_ble_link_disconnected(void);

/** End of APP_RWS_GFPS
* @}
*/
#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
#endif
