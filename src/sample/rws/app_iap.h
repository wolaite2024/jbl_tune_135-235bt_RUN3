/*
 * Copyright (c) 2018, Realsil Semiconductor Corporation. All rights reserved.
 */

#ifndef _APP_IAP_H_
#define _APP_IAP_H_

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


/** @defgroup APP_IAP App Iap
  * @brief This file provide api for iap function.
  * @{
  */

#define EA_PROTOCOL_ID_ALEXA   1

#define EA_PROTOCOL_ID_RTK      10

/*============================================================================*
 *                              Functions
 *============================================================================*/
/** @defgroup APP_IAP_Exported_Functions App Iap Functions
    * @{
    */
/**
    * @brief  hardware init for i2c communication with external chip.
    * @param  p_i2c pointer to struct @ref I2C_TypeDef object
    * @return void
    */
void app_cp_hw_init(void *p_i2c);

/**
    * @brief  Iap module init.
    * @param  void
    * @return void
    */
void app_iap_init(void);


void app_iap_handle_remote_conn_cmpl(void);
/** @} */ /* End of group APP_IAP_Exported_Functions */
/** End of APP_IAP
* @}
*/


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _APP_IAP_H_ */
