/**
*********************************************************************************************************
*               Copyright(c) 2016, Realtek Semiconductor Corporation. All rights reserved.
**********************************************************************************************************
* @file     rtc_demo.c
* @brief    This file provides demo code to realize RTC comparator function.
* @details
* @author   elliot chen
* @date     2016-12-15
* @version  v1.0
*********************************************************************************************************
*/

/*============================================================================*
 *                              Header Files
 *============================================================================*/
#include "rtl876x_rcc.h"
#include "rtl876x_rtc.h"
#include "rtl876x_nvic.h"
#include "trace.h"
#include "vector_table.h"
/** @defgroup  RTC_DEMO  RTC DEMO
    * @brief  RTC work in count comparator mode implementation demo code
    * @{
    */


/*============================================================================*
 *                              Macros
 *============================================================================*/
/** @defgroup RTC_Demo_Exported_Macros RTC Demo Exported Macros
  * @brief
  * @{
  */

#define RTC_PRESCALER_VALUE     0
#define RTC_COMP_INDEX          1
#define RTC_INT_CMP_1           RTC_INT_CMP1
#define RTC_COMP_VALUE          (0xC000)

/** @} */ /* End of group RTC_Demo_Exported_Macros */

/*============================================================================*
 *                              Functions
 *============================================================================*/
/** @defgroup RTC_Demo_Exported_Functions RTC Demo Exported Functions
  * @brief
  * @{
  */

/**
  * @brief  Initialize LPC peripheral.
  * @param   No parameter.
  * @return  void
  */
void driver_rtc_init(void)
{
    RTC_DeInit();
    RTC_SetPrescaler(RTC_PRESCALER_VALUE);
    RTC_SetComp(RTC_COMP_INDEX, RTC_COMP_VALUE);
    RTC_MaskINTConfig(RTC_INT_CMP_1, DISABLE);
    RTC_CompINTConfig(RTC_INT_CMP_1, ENABLE);
    RamVectorTableUpdate(RTC_VECTORn, RTC_Handler);

    /* Config RTC interrupt */
    NVIC_InitTypeDef NVIC_InitStruct;
    NVIC_InitStruct.NVIC_IRQChannel = RTC_IRQn;
    NVIC_InitStruct.NVIC_IRQChannelPriority = 3;
    NVIC_InitStruct.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStruct);

    /* Start RTC */
    RTC_SystemWakeupConfig(ENABLE);
    RTC_RunCmd(ENABLE);
}

/**
  * @brief  demo code of operation about RTC.
  * @param   No parameter.
  * @return  void
  */
void rtc_demo_code(void)
{
    /* Initialize RTC peripheral */
    driver_rtc_init();

    APP_PRINT_INFO0(" run rtc_demo_code ");//Add application code here
}

/**
  * @brief  RTC interrupt handle function.
  * @param  None.
  * @return None.
  */
void RTC_Handler(void)
{
    APP_PRINT_INFO0(" RTC_Handler ");
    if (RTC_GetINTStatus(RTC_INT_CMP_1) == SET)
    {
        RTC_SetComp(RTC_COMP_INDEX, RTC_GetCounter() + RTC_COMP_VALUE);
        RTC_ClearCompINT(RTC_COMP_INDEX);
    }
}

/** @} */ /* End of group RTC_Demo_Exported_Functions */
/** @} */ /* End of group RTC_DEMO */


/******************* (C) COPYRIGHT 2016 Realtek Semiconductor Corporation *****END OF FILE****/

