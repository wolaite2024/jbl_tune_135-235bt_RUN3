/*
 * Copyright (c) 2018, Realsil Semiconductor Corporation. All rights reserved.
 */

#ifndef _APP_DONGLE_RECORD_H_
#define _APP_DONGLE_RECORD_H_


#ifdef __cplusplus
extern "C" {
#endif
/*============================================================================*
  *                           Header Files
  *============================================================================*/

#include <stdint.h>
#include <stdbool.h>
#include <string.h>

/** @defgroup APP_RWS_DONGLE
  * @brief
  * @{
  */

#define BD_ADDR_LENGTH       6

void app_dongle_force_stop_recording(void);

/**
    * @brief        This function can stop the record.
    * @return       void
    */
void app_dongle_stop_recording(uint8_t bd_addr[6]);

/**
    * @brief        This function can start the record.
    * @return       void
    */
void app_dongle_start_recording(uint8_t bd_addr[6]);

/**
    * @brief        This function can request enter or exit gaming mode.
    * @return       void
    */
void app_dongle_gaming_mode_request(bool status);

/**
    * @brief        This function can report mic data to dongle.
    * @return       void
    */
void app_dongle_mic_data_report(void  *data, uint16_t  required_len);

void app_dongle_record_init(void);

/** @} End of APP_RWS_DONGLE */

#ifdef __cplusplus
}
#endif

#endif //_VOICE_SPP_H_
