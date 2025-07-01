/*
 * Copyright (c) 2018, Realsil Semiconductor Corporation. All rights reserved.
 */


#ifndef _MP_CMD_H_
#define _MP_CMD_H_

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


/** @defgroup APP_RWS_MP App mp
  * @brief App mp
  * @{
  */

#define MP_CMD_RX_BUFFER_SIZE  512

/** @brief  mp command module */
typedef struct
{
    uint16_t            rx_count;
    uint16_t            rx_process_offset;
    uint8_t             *rx_buffer;
    uint32_t            rx_w_idx;
    uint32_t            rx_buf_size;
} T_MP_CMD_MODULE;

/**
 * @brief   mp command module initial
 * @return  void
 */
void mp_cmd_module_init(void);

/**
 * @brief   Register mp command
 * @retval  true   Success
 * @retval  false  Failed
 */
bool mp_cmd_register(void);

/** End of APP_RWS_MP
* @}
*/


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _MP_CMD_H_ */
