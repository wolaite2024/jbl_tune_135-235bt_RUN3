/**
*********************************************************************************************************
*               Copyright(c) 2016, Realtek Semiconductor Corporation. All rights reserved.
**********************************************************************************************************
* @file     user_cmd.h
* @brief    Define user command.
* @details
* @author   jane
* @date     2016-02-18
* @version  v0.1
*********************************************************************************************************
*/
#ifndef _USER_CMD_H_
#define _USER_CMD_H_

#ifdef  __cplusplus
extern "C" {
#endif      /* __cplusplus */

#include <data_uart.h>
#include <user_cmd_parse.h>


/** @defgroup CENTRAL_CMD Central User Command
  * @brief Central User Command
  * @{
  */


extern const T_USER_CMD_TABLE_ENTRY user_cmd_table[];
extern T_USER_CMD_IF    user_cmd_if;


/** End of CENTRAL_CMD
* @}
*/


#ifdef  __cplusplus
}
#endif      /*  __cplusplus */

#endif

