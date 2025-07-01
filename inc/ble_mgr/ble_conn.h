/*
 * Copyright (c) 2018, Realsil Semiconductor Corporation. All rights reserved.
 */

#ifndef _BLE_CONN_H_
#define _BLE_CONN_H_

#include "gap_le.h"
#include "gap_msg.h"

#ifdef __cplusplus
extern "C" {
#endif


/** @defgroup BLE_CONN Ble Conn
  * @brief Ble Conn Param Update
  * @{
  */

/*============================================================================*
 *                              Functions
 *============================================================================*/
/** @defgroup BLE_CONN_Exported_Functions Ble Conn Functions
    * @{
    */


/**
*@brief  used to set prefer conn param
*@note
*@param[in]  conn_id  ble conn id
*@param[in]  conn_interval_min the minimum of conn interval
*@param[in]  conn_interval_max the maximum of conn interval
*@param[in]  conn_latency the latency of ble conn
*@param[in]  supervision_timeout the supervision timeout of ble conn
*@return  bool
*@retval  true success
*@retval  false fail
*/
bool ble_set_prefer_conn_param(uint8_t conn_id, uint16_t conn_interval_min,
                               uint16_t conn_interval_max, uint16_t conn_latency,
                               uint16_t supervision_timeout);


/** @} */ /* End of group BLE_CONN_Exported_Functions */
/** End of BLE_CONN
* @}
*/


#ifdef __cplusplus
}
#endif

#endif /* _BLE_CONN_H_ */
