/**
*********************************************************************************************************
*               Copyright(c) 2018, Realtek Semiconductor Corporation. All rights reserved.
*********************************************************************************************************
* @file      ams.h
* @brief
* @details
* @author
* @date
* @version
* *********************************************************************************************************
*/


#ifndef _AMS_H__
#define _AMS_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <gap_le.h>
#include <app_msg.h>

#define AMS_MSG_QUEUE_NUM 5



/**@brief  AMS Message Data */
typedef struct
{

} T_AMS_MSG;


void ams_handle_msg(T_IO_MSG *p_io_msg);


void ams_init(uint8_t link_num);
#ifdef __cplusplus
}
#endif

#endif

