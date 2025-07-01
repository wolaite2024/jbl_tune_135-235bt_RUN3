/**
*********************************************************************************************************
*               Copyright(c) 2016, Realtek Semiconductor Corporation. All rights reserved.
**********************************************************************************************************
* @file     rtc_overflow_demo.c
* @brief        This file provides demo code to realize RTC Overflow function.
* @details
* @author   elliot chen
* @date         2016-12-15
* @version  v1.0
*********************************************************************************************************
*/

/*============================================================================*
 *                              Header Files
 *============================================================================*/
#include "rtl876x_rcc.h"
#include "rtl876x_rtc.h"
#include "rtl876x_nvic.h"

/** @defgroup  RTC_OVERFLOW_DEMO  RTC OVERFLOW DEMO
    * @brief  RTC detect overflow event implementation demo code
    * @{
    */

/*============================================================================*
 *                              Macros
 *============================================================================*/

/** @defgroup RTC_Overflow_Demo_Exported_Macros RTC Overflow Demo Exported Macros
  * @brief
  * @{
  */

#define RTC_PRESCALER_VALUE     0

/** @} */ /* End of group RTC_Overflow_Demo_Exported_Macros */


/*============================================================================*
 *                              Functions
 *============================================================================*/
/** @defgroup RTC_Overflow_Demo_Exported_Functions RTC Overflow Demo Exported Functions
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
    RTC_MaskINTConfig(RTC_INT_OVF, DISABLE);

    /* Config RTC interrupt */
    NVIC_InitTypeDef NVIC_InitStruct;
    NVIC_InitStruct.NVIC_IRQChannel = RTC_IRQn;
    NVIC_InitStruct.NVIC_IRQChannelPriority = 2;
    NVIC_InitStruct.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStruct);

    /* Start RTC */
    RTC_RunCmd(ENABLE);
}

/**
  * @brief  demo code of operation about RTC.
  * @param   No parameter.
  * @return  void
  */
void rtc_overflow_demo_code(void)
{
    /* Initialize RTC peripheral */
    driver_rtc_init();

    //Add application code here
}

/**
  * @brief  RTC interrupt handle function.
  * @param  None.
  * @return None.
  */
void RTC_Handler(void)
{
    /* RTC overflow interrupt handle */
    if (RTC_GetINTStatus(RTC_INT_OVF) == SET)
    {
        // Add application code here

        RTC_ClearOverFlowINT();
    }
}

/** @} */ /* End of group RTC_Overflow_Demo_Exported_Functions */
/** @} */ /* End of group RTC_OVERFLOW_DEMO */



/******************* (C) COPYRIGHT 2016 Realtek Semiconductor Corporation *****END OF FILE****/

