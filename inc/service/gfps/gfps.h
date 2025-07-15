/*
 * Copyright (c) 2018, Realsil Semiconductor Corporation. All rights reserved.
 */


#ifndef _GFPS_H_
#define _GFPS_H_

#include "profile_server.h"

#ifdef __cplusplus
extern "C"
{
#endif

/** @defgroup GFPS GFPS
  * @brief
  * @{
  */


/** @defgroup APP_GFPS GFPS LIB
  * @brief Google fast pair service lib
  * @{
  */

/*============================================================================*
 *                              Macros
 *============================================================================*/
/** @defgroup GFPS_Exported_Macros Gfps Macros
    * @{
    */
#define GFPS_DATA_ID_PERSONALIZED_NAME         (0x01)
#define GFPS_FIRMWARE_VERSION                 "V3.1.0"

#define GFPS_ADDTIONAL_DATA_SUPPORT            (1 && GFPS_FEATURE_SUPPORT)
#define GFPS_PERSONALIZED_NAME_SUPPORT         (1 && GFPS_ADDTIONAL_DATA_SUPPORT)

#if GFPS_FEATURE_SUPPORT
#define GFPS_PERSONALIZED_NAME_MAX_LEN         (64)
#define GFPS_BATTERY_VALUE_UNKNOWN             (0x7F)
#define GFPS_ACCOUNT_KEY_LENGTH                (16)
/** End of GFPS_Exported_Macros
    * @}
    */

/*============================================================================*
 *                              Types
 *============================================================================*/
/** @defgroup GFPS_Exported_Types Gfps Types
    * @{
    */

typedef enum
{
    GFPS_WRITE_RESULT_SUCCESS                   = 0,
    GFPS_WRITE_RESULT_ERROR_DECRYPT_FAIL        = 1,
    GFPS_WRITE_RESULT_ERROR_NOT_IN_PAIR_MODE    = 2,
    GFPS_WRITE_RESULT_ERROR_MESSAGE_TYPE        = 3,
    GFPS_WRITE_RESULT_ERROR_PROVIDER_ADDRESS    = 4,
    GFPS_WRITE_RESULT_ERROR_ADDITIONAL_DATA_LEN = 5,
    GFPS_WRITE_RESULT_ERROR_INVALID_PARAM       = 6,

    GFPS_WRITE_RESULT_ERROR_UNKNOWN             = 0xff,
} T_GFPS_KBP_WRITE_RESULT;

typedef struct
{
    bool    battery_enable;         //!< Whether including the battery notification
    bool    battery_show_ui;        //!< Whether showing UI: true(show UI indication) or false(hide UI indication)
    bool    battery_left_charging;  //!< Whether in charging: true(charging) or false(not charging)
    uint8_t battery_left_level;     //!< Left bud battery level. Range: 0-100 (0x7F-unknown)
    bool    battery_right_charging; //!< Whether in charging: true(charging) or false(not charging)
    uint8_t battery_right_level;    //!< Right bud battery level. Range: 0-100 (0x7F-unknown)
    bool    battery_case_charging;  //!< Whether in charging: true(charging) or false(not charging)
    uint8_t battery_case_level;     //!< Case battery level. Range: 0-100 (0x7F-unknown)
    bool    battery_remain_time_enable;
    uint16_t battery_remain_time;
} T_GFPS_BATTERY_INFO;

typedef struct
{
    T_GFPS_KBP_WRITE_RESULT result;
    uint8_t pk_field_exist;
    uint8_t account_key_idx;

    uint8_t provider_init_bond: 1;
    uint8_t notify_existing_name: 1;
    uint8_t retroactively_account_key: 1;
    uint8_t rfu: 5;

    uint8_t provider_addr[6];
    uint8_t seek_br_edr_addr[6];

} T_GFPS_KBP_WRITE_MSG_DATA;

typedef struct
{
    T_GFPS_KBP_WRITE_RESULT result;
    uint8_t pk_field_exist;
    uint8_t account_key_idx;

    uint8_t device_action: 1;
    uint8_t additional_data: 1;
    //true:it will be followed by Additional Data characteristic (#AdditionalData), 0 otherwise
    uint8_t rfu: 6;

    uint8_t message_group;
    uint8_t message_code;
    uint8_t additional_data_length;
    uint8_t add_data[6];//array to store Additional data
    uint8_t data_id;

    uint8_t provider_addr[6];
} T_GFPS_ACTION_REQ_MSG_DATA;

typedef enum
{
    GFPS_NOTIFICATION_ENABLE_KBP        = 1,
    GFPS_NOTIFICATION_DISABLE_KBP       = 2,
    GFPS_NOTIFICATION_ENABLE_PASSKEY    = 3,
    GFPS_NOTIFICATION_DISABLE_PASSKEY   = 4,
    GFPS_NOTIFICATION_ENABLE_ADDITIONAL_DATA    = 5,
    GFPS_NOTIFICATION_DISABLE_ADDITIONAL_DATA   = 6
} T_GFPS_NOTIFICATION_TYPE;

typedef struct
{
    uint8_t data_id;
    uint8_t data_len;
    uint8_t *p_data;
} T_GFPS_ADDITIONAL_DATA;

typedef union
{
    T_GFPS_NOTIFICATION_TYPE   notify_type;
    uint32_t                   passkey;
    T_GFPS_KBP_WRITE_MSG_DATA  kbp;
    T_GFPS_ACTION_REQ_MSG_DATA action_req;
    uint8_t                    account_key[16];
    T_GFPS_ADDITIONAL_DATA     additional_data;
} T_GFPS_UPSTREAM_MSG_DATA;

typedef enum
{
    GFPS_CALLBACK_TYPE_UNKNOWN                  = 0,
    GFPS_CALLBACK_TYPE_NOTIFICATION_ENABLE      = 1,
    GFPS_CALLBACK_TYPE_KBP_WRITE_REQ            = 2,
    GFPS_CALLBACK_TYPE_ACTION_REQ               = 3,
    GFPS_CALLBACK_TYPE_PASSKEY                  = 4,
    GFPS_CALLBACK_TYPE_ACCOUNT_KEY              = 5,
#if GFPS_ADDTIONAL_DATA_SUPPORT
    GFPS_CALLBACK_TYPE_ADDITIONAL_DATA          = 6,
#endif
} T_GFPS_CALLBACK_TYPE;

typedef struct
{
    uint8_t                  conn_id;
    T_GFPS_CALLBACK_TYPE     msg_type;
    T_GFPS_UPSTREAM_MSG_DATA msg_data;
} T_GFPS_CALLBACK_DATA;

typedef enum
{
    DISCOVERABLE_MODE_WITH_MODEL_ID,
    NOT_DISCOVERABLE_MODE,
} T_GFPS_ADV_MODE;

/**
 * @brief Maintain a mapping table to map Seeker's public address to account key
 */
typedef struct
{
    uint8_t key[GFPS_ACCOUNT_KEY_LENGTH];//account key
} T_GFPS_ACCOUNT_INFO;

/**
 * @brief account key table
 * uint8_t num; the number of key that has stored in account key table
 * uint8_t del; Indicates the index of element that needs to be deleted when a new key needs to be added while the table is full.
 * uint8_t gap[2];reserved
 */
typedef struct
{
    uint8_t num;
    uint8_t del;
    bool owner_key_valid;
    uint8_t gap[1];
    T_GFPS_ACCOUNT_INFO account_info[1];
} T_ACCOUNT_KEY;
/** End of GFPS_Exported_Types
    * @}
    */

/*============================================================================*
 *                              Functions
 *============================================================================*/
/** @defgroup GFPS_Exported_Functions Ble Adv Functions
    * @{
    */

/**
 * @brief gfps_account_key_init
 *
 * @param[in] p_account_key_table point to account key table which malloced by APP.
 * @param[in] max_key_num The maximum of keys allowed to be stroed in accout key table.
 */
void gfps_account_key_init(T_ACCOUNT_KEY *p_account_key_table, uint8_t max_key_num);

/**
  * @brief Fast pair service initialize, this fuction shall be invoke before using fast pair.
  * The mode id, anti spoofing public / private key aquire from mode registation.
  * @param[in] model_id Mode ID.
  * @param[in] anti_spoofing_public Anti spoofing public key.
  * @param[in] anti_spoofing_private Anti spoofing private key.
  * @param[in] account_key Account key stored by app.
  * @param[in] key_num Account key number.
  * @retval true Initialize success.
  * @retval false Initialize fail.
  */
bool gfps_init(uint8_t model_id[3], uint8_t anti_spoofing_public[64],
               uint8_t anti_spoofing_private[32]);

/**
  * @brief Add fast pair service, this fuction shall be invoke before using fast pair.
  * @param[in] p_func Callback when service attribute was read, write or cccd update..
  * @return Service id generated by the BLE stack.
  * @retval 0xFF Operation failure.
  */
T_SERVER_ID gfps_add_service(void *p_func);

/**
  * @brief Set the TX power used in advertising data.
  *         NOTE: This function can be called before @ref gfps_gen_adv_data is invoked.
  *
  * @param[in] tx_power_exist
  * @param[in] adv_tx_power Adverting TX power.
  * @retval void
  */
void gfps_set_tx_power(bool tx_power_exist, int8_t adv_tx_power);

/**
  * @brief Set battery notification information in not discoverable advertising data.
  *         NOTE: This function can be called before @ref gfps_gen_adv_data is invoked.
  *
  * @param[in] p_battery_info Point to the battery notification
  * @retval true Success.
  * @retval false Fail.
  */
bool gfps_set_battery_info(T_GFPS_BATTERY_INFO *p_battery_info);

/**
  * @brief Generate session/message nonce
  * @param[out] nonce.
  * @retval void
  */
void gfps_gen_random_nonce(uint8_t *nonce);
/**
  * @brief Generate fast pair advertising data, using to trigger android device find audio device.
  * @param[in] mode Adverting mode @ref T_GFPS_ADV_MODE.
  * @param[out] p_adv
  * @param[out] p_adv_len
  * @param[in] show_ui
  * @retval true Get advertising data success.
  * @retval false Get advertising data fail.
  */
bool gfps_gen_adv_data(T_GFPS_ADV_MODE mode, uint8_t *p_adv, uint8_t *p_adv_len, bool show_ui);

/**
  * @brief Send passkey to android device during pairing procedure.
  * @param[in] conn_id BLE link connection id.
  * @param[in] service_id Fast pair service id return by gfps_add_service function.
  * @param[in] passkey Passkey display on provide device.
  * @retval true Send passkey success.
  * @retval false Start advertising fail.
  */
bool gfps_send_passkey(uint8_t conn_id, T_SERVER_ID service_id, uint32_t passkey);

#if GFPS_PERSONALIZED_NAME_SUPPORT
/**
  * @brief send personalized name when seeker set notify_existing_name flags to 1
  * @param[in] personalized_name the pointer of personalized name
  * @param[in] personalized_name_len the length of personalized name
  * @param[in] conn_id
  * @param[in] service_id
  * @retval true send personalized name success.
  * @retval false send personalized name fail.
  */
bool gfps_send_personalized_name(uint8_t *personalized_name, uint8_t personalized_name_len,
                                 uint8_t conn_id, uint8_t service_id);
#endif

T_APP_RESULT gfps_handle_pending_char_kbp(void);

/**
 * @brief get current subsequent pairing account key.
 * This function only can be used when gfps ble link state is connected,
* if gfps ble link state is disconnected(for example:gfps_conn_id ==0xFF)
 * this account key maybe a invalid value.
 * @param[out] key  current subsequent pairing account key.
 * @return true success get current subsequent pairing account key.
 * @return false not in subsequent pairing mode or have an invalid account key.
 */
bool gfps_get_subsequent_pairing_account_key(uint8_t *key);

/**
 * @brief shall discard aes key and pairing mode if this LE link disconnects
 */
void gfps_reset_aeskey_and_pairing_mode(void);
/**
 * @brief set inuse account key index
 * @param[in] inuse account key index
 */
void gfps_set_inuse_account_key_index(uint8_t index);
/**
 * @brief set most recently inuse account key index
 * @param[in] most recently inuse account key index
 */

void gfps_set_most_recently_inuse_account_key_index(uint8_t index);
/**
 * @brief Get inuse account key index
 * @retval inuse account key index
 */
uint8_t gfps_get_inuse_account_key_index(void);
/**
 * @brief Get inuse account key index
 * @retval inuse account key index
 */
uint8_t gfps_get_recently_inuse_account_key_index(void);
/**
 * @brief Get inuse account key index
 * @retval successfully get account_key
 */
bool gfps_sass_get_account_key_by_index(uint8_t *key, uint8_t key_index);
/**
 * @brief get the highest priority device in account key table.
 * @param[out] p_idx if account key table is empty then *p_idx = 0xFF.
 * @retval false get fail
 * @retval true get success
 */
bool gfps_get_highest_priority_device(uint8_t *p_idx);

/**
 * @brief enable or disable finder in gfps_lib
 *
 * @param enable true:enable, false:disable
 */
void gfps_set_finder_enable(bool enable);

bool gfps_get_owner_key_valid(void);

void gfps_set_owner_key_valid(bool valid);

/**
 * @brief enable or disable sass in gfps_lib
 *
 * @param enable true:enable, false:disable
 */
void gfps_set_sass_enable(bool enable);

bool gfps_hmac_sha256(uint8_t *p_encrypted_data, uint8_t data_len,
                      uint8_t p_out[8], uint8_t *p_key, uint8_t key_len);
uint8_t gfps_get_pairing_mode_status(void);

/** @} */ /* End of group GFPS_Exported_Functions */
/** End of APP_GFPS
* @}
*/

/** @} End of GFPS */

#endif

#ifdef __cplusplus
}
#endif

#endif

