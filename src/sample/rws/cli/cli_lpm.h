/*
 * Copyright (c) 2018, Realsil Semiconductor Corporation. All rights reserved.
 */


#ifndef _CLI_LPM_H_
#define _CLI_LPM_H_

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


/** @defgroup APP_RWS_CLI App Cli
  * @brief App Cli
  * @{
  */

/** @defgroup APP_RWS_CLI_LPM App Cli Power
  * @brief App Cli lpm
  * @{
  */

#define LPM_CMD                           0xE0

#define LPM_ACTION_DLPS_ENABLE            0x02
#define LPM_ACTION_DLPS_DISABLE           0x03

/**
 * @brief This api is used to register power command
 *
 * @retval true  Command register successfully
 * @retval false Command register failed
 */
bool lpm_cmd_register(void);


/** End of APP_RWS_CLI_LPM
* @}
*/

/** End of APP_RWS_CLI
* @}
*/


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _CLI_LPM_H_ */
