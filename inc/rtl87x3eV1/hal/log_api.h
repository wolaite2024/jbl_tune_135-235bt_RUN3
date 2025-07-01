/**
*****************************************************************************************
*     Copyright(c) 2017, Realtek Semiconductor Corporation. All rights reserved.
*****************************************************************************************
* @file    log_api.h
* @brief   This file provides log api wrapper for sdk customer.
* @author  Sandy
* @date    2021-05-20
* @version v1.0
* *************************************************************************************
*/

#ifndef __LOG_API_H_
#define __LOG_API_H_
#include "stdbool.h"
#include "stdint.h"


bool log_enable_get(void);
void log_enable_set(bool enable);

#endif
