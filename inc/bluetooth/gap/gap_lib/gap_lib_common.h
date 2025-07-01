/*
* Copyright (c) 2018, Realsil Semiconductor Corporation. All rights reserved.
*/

#ifndef _GAP_LIB_COMMON_H_
#define _GAP_LIB_COMMON_H_

#ifdef __cplusplus
extern "C"
{
#endif

/** @addtogroup GAP GAP Module
  * @{
  */

/** @addtogroup GAP_COMMON_MODULE   GAP Common Module
  * @{
  */

/** @addtogroup GAP_COMMON_MP GAP Common MP
  * @{
  */

bool gap_vendor_cmd_req(uint16_t op, uint8_t len, uint8_t *p_param);

bool is_mp_test_mode(void);

void set_mp_mode_flag(bool flag);

/** End of GAP_COMMON_MP
  * @}
  */

/** End of GAP_COMMON_MODULE
  * @}
  */

/** End of GAP
  * @}
  */
#ifdef __cplusplus
}
#endif

#endif /* _GAP_LIB_COMMON_H_ */
