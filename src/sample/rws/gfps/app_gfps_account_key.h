/*
 * Copyright (c) 2018, Realsil Semiconductor Corporation. All rights reserved.
 */

#ifndef _APP_GFPS_ACCOUNT_KEY_H_
#define _APP_GFPS_ACCOUNT_KEY_H_

#include <stdbool.h>
#include <stdint.h>
#include "gfps.h"
#include "app_flags.h"
#include "app_gfps_personalized_name.h"
#if GFPS_FEATURE_SUPPORT
#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/** @defgroup APP_RWS_GFPS App Gfps
  * @brief App Gfps
  * @{
  */
#define GFPS_ACCOUNT_KEY_MAX           5

typedef enum
{
    APP_REMOTE_MSG_GFPS_ACCOUNT_KEY_ADD         = 0x00,
    APP_REMOTE_MSG_GFPS_ACCOUNT_KEY             = 0x01,
    APP_REMOTE_MSG_GFPS_PERSONALIZED_NAME       = 0x02,
    APP_REMOTE_MSG_GFPS_OWNER_ADDRESS           = 0x03,
    APP_REMOTE_MSG_GFPS_MAX_MSG_NUM             = 0x04,
} T_APP_GFPS_REMOTE_MSG;

#define ACCOUNT_KEY_FLASH_OFFSET   0xAF0  //account key store offset

bool app_gfps_relay_init(void);
bool app_gfps_account_key_init(uint8_t key_num);
void app_gfps_account_key_clear(void);
bool app_gfps_account_key_store(uint8_t key[16], uint8_t *bd_addr);

bool app_gfps_remote_account_key_add(uint8_t key[16], uint8_t *bd_addr);
bool app_gfps_remote_account_key_sync(void);
bool app_gfps_remote_owner_address_sync(void);

/**
 * @brief print all account key info
 */
void app_gfps_account_key_table_print(void);
#if GFPS_PERSONALIZED_NAME_SUPPORT
bool app_gfps_remote_personalized_name(void);
#endif
bool app_gfps_account_key_verify_mac(uint8_t *bd_addr, uint8_t *msg, uint16_t data_len,
                                     uint8_t *msg_nonce, uint8_t *mac);
bool app_gfps_account_key_find_inuse_account_key(uint8_t *inuse_accout_idx, uint8_t *bd_addr,
                                                 uint8_t *msg, uint16_t data_len, uint8_t *msg_nonce, uint8_t *mac);

void app_gfps_account_key_save_ownerkey_valid(void);
/** End of APP_RWS_GFPS
* @}
*/
#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
#endif
