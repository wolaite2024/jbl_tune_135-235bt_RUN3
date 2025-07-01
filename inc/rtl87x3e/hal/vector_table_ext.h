/**
*********************************************************************************************************
*               Copyright(c) 2015, Realtek Semiconductor Corporation. All rights reserved.
**********************************************************************************************************
* @file     vector_table_ext.h
* @brief    Vector table extend implementaion header file
* @details
* @author   yuhungweng
* @date     2021-9-02
* @version  v1.0
*********************************************************************************************************
*/

#include "stdbool.h"
#include "vector_table.h"

bool RamVectorTableUpdate(VECTORn_Type v_num, IRQ_Fun isr_handler);

