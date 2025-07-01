/*
 * Copyright (c) 2018, Realsil Semiconductor Corporation. All rights reserved.
 */

#ifndef _APP_GFPS_RFC_H_
#define _APP_GFPS_RFC_H_

#include "stdint.h"
#include "bt_gfps.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/** @defgroup APP_RWS_GFPS App Gfps
  * @brief App Gfps
  * @{
  */

#define GFPS_RING_PERIOD_VALUE      2

#define GFPS_STANARD_ACK_PAYLOAD_LEN    2 //MSG gourp + MSG id

#define GFPS_GET_SASS_CAP_DATA_LEN                  0
#define GFPS_NOTIFY_SASS_CAP_PARA_LEN               (4)
#define GFPS_NOTIFY_SASS_CAP_DATA_LEN               (GFPS_NOTIFY_SASS_CAP_PARA_LEN+GFPS_MESSAGE_NONCE_LEN+GFPS_MAC_LEN)
#define GFPS_SET_MULTI_POINT_STATE_PARA_LEN         (1)
#define GFPS_SET_MULTI_POINT_STATE_DATA_LEN         (GFPS_SET_MULTI_POINT_STATE_PARA_LEN+GFPS_MESSAGE_NONCE_LEN+GFPS_MAC_LEN)
#define GFPS_SET_SWITCH_PREFER_PARA_LEN             (2)
#define GFPS_SET_SWITCH_PREFER_DATA_LEN             (GFPS_SET_SWITCH_PREFER_PARA_LEN+GFPS_MESSAGE_NONCE_LEN+GFPS_MAC_LEN)
#define GFPS_GET_SWITCH_PREFER_PARA_LEN             0
#define GFPS_GET_SWITCH_PREFER_DATA_LEN             0
#define GFPS_NOTIFY_SWITCH_PREFER_PARA_LEN          (2)
#define GFPS_NOTIFY_SWITCH_PREFER_DATA_LEN          (GFPS_NOTIFY_SWITCH_PREFER_PARA_LEN+GFPS_MESSAGE_NONCE_LEN+GFPS_MAC_LEN)
#define GFPS_SWITCH_ACTIVE_AUDIO_SOURCE_PARA_LEN    (1)
#define GFPS_SWITCH_ACTIVE_AUDIO_SOURCE_DATA_LEN    (GFPS_SWITCH_ACTIVE_AUDIO_SOURCE_PARA_LEN+GFPS_MESSAGE_NONCE_LEN+GFPS_MAC_LEN)
#define GFPS_SWITCH_BACK_PARA_LEN                   (1)
#define GFPS_SWITCH_BACK_DATA_LEN                   (GFPS_SWITCH_BACK_PARA_LEN+GFPS_MESSAGE_NONCE_LEN+GFPS_MAC_LEN)
#define GFPS_GET_CONN_STATUS_DATA_LEN               0
#define GFPS_GET_CONN_STATUS_DATA_LEN               0
#define GFPS_NOTIFY_SASS_INITIAL_CONN_PARA_LEN      (1)
#define GFPS_NOTIFY_SASS_INITIAL_CONN_DATA_LEN      (GFPS_NOTIFY_SASS_INITIAL_CONN_PARA_LEN+GFPS_MESSAGE_NONCE_LEN+GFPS_MAC_LEN)
#define GFPS_IND_INUSE_ACCOUNTKEY_PARA_LEN          (6)
#define GFPS_IND_INUSE_ACCOUNTKEY_DATA_LEN          (GFPS_IND_INUSE_ACCOUNTKEY_PARA_LEN+GFPS_MESSAGE_NONCE_LEN+GFPS_MAC_LEN)
#define GFPS_SEND_CUSTOM_DATA_PARA_LEN              (1)
#define GFPS_SEND_CUSTOM_DATA_DATA_LEN              (GFPS_SEND_CUSTOM_DATA_PARA_LEN+GFPS_MESSAGE_NONCE_LEN+GFPS_MAC_LEN)
#define GFPS_SET_DROP_CONN_TARGET_PARA_LEN          (1)
#define GFPS_SET_DROP_CONN_TARGET_DATA_LEN          (GFPS_SET_DROP_CONN_TARGET_PARA_LEN+GFPS_MESSAGE_NONCE_LEN+GFPS_MAC_LEN)

#define GFPS_GET_ANC_STATE_PARA_LEN                 0
#define GFPS_GET_ANC_STATE_DATA_LEN                 0
#define GFPS_SET_ANC_STATE_PARA_LEN                 (3)
#define GFPS_SET_ANC_STATE_DATA_LEN                 (GFPS_SET_ANC_STATE_PARA_LEN+GFPS_MESSAGE_NONCE_LEN+GFPS_MAC_LEN)

#define GFPS_REQ_CAPABILITY_UPDATE_PARA_LEN         0
#define GFPS_REQ_CAPABILITY_UPDATE_DATA_LEN         0

#define GFPS_DYNAMIC_BUFFER_SIZING_PARA_LEN         (3)
#define GFPS_DYNAMIC_BUFFER_SIZING_DATA_LEN         (GFPS_DYNAMIC_BUFFER_SIZING_PARA_LEN+GFPS_MESSAGE_NONCE_LEN+GFPS_MAC_LEN)

#define GFPS_BUFFER_SIZE_INFO_LEN_PER_CODEC_SBC_ONLY    (7)
#define GFPS_BUFFER_SIZE_INFO_LEN_PER_CODEC_AAC         (14)
#define GFPS_BUFFER_SIZE_INFO_LEN_PER_CODEC_LDAC        (21)

#define GFPS_MAC_ERROR_NAK_LEN  2

typedef enum
{
    BUD_SIDE_LEFT      = 0,
    BUD_SIDE_RIGHT     = 1,
} T_GFPS_RFC_BUD_SIDE;

typedef enum
{
    GFPS_RFC_REMOTE_MSG_FINDME_START                        = 0x00,
    GFPS_RFC_REMOTE_MSG_FINDME_STOP                         = 0x01,
    GFPS_RFC_REMOTE_MSG_FINDME_SYNC                         = 0x02,
    GFPS_RFC_REMOTE_MSG_SYNC_CUSTOM_DATA                    = 0x03,
    GFPS_RFC_REMOTE_MSG_SYNC_TARGET_DROP_DEVICE             = 0x04,
    GFPS_RFC_REMOTE_MSG_MAX_MSG_NUM,
} T_GFPS_RFC_REMOTE_MSG;

/** @brief gfps rfc timer */
typedef enum
{
    GFPS_RFC_RING_PERIOD,
    GFPS_RFC_RING_TIMEOUT,

    GFPS_RFC_TOTAL
} T_GFPS_RFC_TIMER;

typedef enum
{
    GFPS_RFC_RING_STOP    = 0x00,
    GFPS_RFC_RIGHT_RING   = 0x01,
    GFPS_RFC_LEFT_RING    = 0x02,
    GFPS_RFC_ALL_RING     = 0x03,
} T_GFPS_RFC_RING_STATE;

/** @defgroup APP_GFPS_RFC App gfps rfc
  * @brief App gfps rfc
  * @{
  */
void app_gfps_rfc_init(void);

void app_gfps_rfc_battery_info_set(uint8_t *battery);
void app_gfps_rfc_update_ble_addr(void);
bool app_gfps_rfc_notify_connection_status(void);
void  app_gfps_rfc_notify_multipoint_switch(uint8_t active_index, uint8_t switch_reason);
void app_gfps_rfc_handle_ring_event(uint8_t event);

void app_gfps_rfc_set_ring_timeout(uint8_t ring_timeout);
uint8_t app_gfps_rfc_get_ring_timeout(void);
T_GFPS_RFC_RING_STATE app_gfps_rfc_get_ring_state(void);
void app_gfps_reverse_data(uint8_t *data, uint16_t len);
/** End of APP_GFPS_RFC
* @}
*/

/** End of APP_RWS_GFPS
* @}
*/

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _APP_GFPS_RFC_H_ */


