/*
 * Copyright (c) 2018, Realsil Semiconductor Corporation. All rights reserved.
 */

#ifndef _TUYA_BLE_SERVICE_H_
#define _TUYA_BLE_SERVICE_H_

#ifdef __cplusplus
extern "C"  {
#endif

#include <profile_server.h>

/** @defgroup APP_RWS_TUYA       RWS TUYA
  * @brief
  * @{
  */

/** @defgroup TUYA_Service XIaowei Ble Service
  * @brief TUYA BLE service
  * @{
  */
/*============================================================================*
 *                              Macros
 *============================================================================*/
/** @defgroup TUYA_Service_Exported_Macros TUYA Service Exported Macros
  * @brief
  * @{
  */
#define GATT_UUID_SERVICE_TUYA         0xFD50

#define GATT_UUID_CHAR_RX_WRITE        0xD0,0x07,0x9B,0x5F,0x80,0x00,0x01,0x80,0x01,0x10,0x00,0x00,0x01,0x00,0x00,0x00
#define GATT_UUID_CHAR_TX_NOTIFY       0xD0,0x07,0x9B,0x5F,0x80,0x00,0x01,0x80,0x01,0x10,0x00,0x00,0x02,0x00,0x00,0x00
#define GATT_UUID_CHAR_RX_READ         0xD0,0x07,0x9B,0x5F,0x80,0x00,0x01,0x80,0x01,0x10,0x00,0x00,0x03,0x00,0x00,0x00

#define TUYA_CHAR_TX_NOTIFY_ENABLE     1
#define TUYA_CHAR_TX_NOTIFY_DISABLE    2
#define TUYA_CHAR_RX_READ_VALUE        3
/** @} End of SIMP_Service_Exported_Macros */

/*============================================================================*
 *                              Types
 *============================================================================*/
/** @defgroup TUYA_Service_Exported_Types TUYA Service Exported Types
  * @brief
  * @{
  */

/** @defgroup T_TUYA_SERVICE_WRITE_MSG T_TUYA_SERVICE_WRITE_MSG
  * @brief TUYA BLE service written msg to application.
  * @{
  */
typedef struct
{
    T_WRITE_TYPE write_type;
    uint16_t len;
    uint8_t *p_value;
} T_TUYA_SERVICE_WRITE_MSG;
/** @} End of T_TUYA_SERVICE_WRITE_MSG */

/** @defgroup T_TUYA_SERVICE_UPSTREAM_MSG_DATA T_TUYA_SERVICE_UPSTREAM_MSG_DATA
  * @brief TUYA BLE service callback message content.
  * @{
  */
typedef union
{
    uint8_t notification_indification_index;
    uint8_t read_value_index;
    T_TUYA_SERVICE_WRITE_MSG write;
} T_TUYA_SERVICE_UPSTREAM_MSG_DATA;
/** @} End of T_TUYA_SERVICE_UPSTREAM_MSG_DATA */

/** @defgroup T_TUYA_SERVICE_CALLBACK_DATA T_TUYA_SERVICE_CALLBACK_DATA
  * @brief TUYA BLE service data to inform application.
  * @{
  */
typedef struct
{
    uint8_t                 conn_id;
    T_SERVICE_CALLBACK_TYPE msg_type;
    T_TUYA_SERVICE_UPSTREAM_MSG_DATA msg_data;
} T_TUYA_SERVICE_CALLBACK_DATA;
/** @} End of T_TUYA_SERVICE_CALLBACK_DATA */

/** @} End of TUYA_Service_Exported_Types */

/*============================================================================*
 *                              Functions
 *============================================================================*/
/** @defgroup TUYA_Service_Exported_Functions TUYA Service Exported Functions
  * @brief
  * @{
  */

/**
  * @brief Add app_tuya BLE service to the BLE stack database.
  *
  * @param[in] p_func  Callback when service attribute was read, write or cccd update.
  * @return Service id generated by the BLE stack: @ref T_SERVER_ID.
  * @retval 0xFF Operation failure.
  * @retval others Service id assigned by stack.
  *
  */
T_SERVER_ID app_tuya_ble_service_add_service(void *p_func);

/**
  * @brief send app_tuya tx notify characteristic value.
  *
  * @param[in] conn_id           connection id
  * @param[in] service_id        service ID of service.
  * @param[in] p_value           characteristic value to notify
  * @param[in] length            characteristic value length to notify
  * @return notification action result
  * @retval 1 true
  * @retval 0 false
  */
bool app_tuya_ble_service_tx_send_notify(uint8_t conn_id, uint8_t *p_value, uint16_t length);

void app_tuya_ble_set_read_value(uint8_t *value, uint8_t length);

/** @} End of TUYA_Service_Exported_Functions */

/** @} End of TUYA_Service */

/** @} End of APP_RWS_TUYA */

#ifdef __cplusplus
}
#endif
#endif /* _TUYA_BLE_SERVICE_H_ */
