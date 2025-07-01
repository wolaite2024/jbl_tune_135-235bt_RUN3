/**
*********************************************************************************************************
*               Copyright(c) 2016, Realtek Semiconductor Corporation. All rights reserved.
**********************************************************************************************************
* @file     lpc_rtc_vol_demo.c
* @brief        This file provides demo code to detect voltage.
* @details
* @author   elliot chen
* @date         2016-11-30
* @version  v1.0
*********************************************************************************************************
*/

/*============================================================================*
 *                              Header Files
 *============================================================================*/
#include "rtl876x_rcc.h"
#include "rtl876x_lpc.h"
#include "rtl876x_pinmux.h"
#include "rtl876x_nvic.h"
#include "vector_table.h"
#include "trace.h"
#include "rtl876x.h"
/** @defgroup  LPC_RTC_VOL_DEMO  LPC VOLTAGE DEMO
    * @brief  Led work in voltage detection mode implementation demo code
    * @{
    */

/*============================================================================*
 *                              Macros
 *============================================================================*/
/** @defgroup LPC_Voltage_Detection_Exported_Macros LPC Voltage Detection Exported Macros
  * @brief
  * @{
  */

#define LPC_CAPTURE_PIN         1//adc_1 as input
void lpc_rtc_handler(void);
/** @} */ /* End of group LPC_Voltage_Detection_Exported_Macros */

/*============================================================================*
 *                              Functions
 *============================================================================*/
/** @defgroup LPC_Voltage_Detection_Exported_Functions LPC Voltage Detection Exported Functions
  * @brief
  * @{
  */

/**
  * @brief  initialization of pinmux settings and pad settings.
  * @param   No parameter.
  * @return  void
  */
void board_lpc_init(void)
{
    Pad_Config(LPC_CAPTURE_PIN, PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_NONE, PAD_OUT_DISABLE,
               PAD_OUT_HIGH);
    Pinmux_Config(LPC_CAPTURE_PIN, IDLE_MODE);
}

/**
  * @brief  Initialize LPC peripheral.
  * @param   No parameter.
  * @return  void
  */
void driver_lpc_init(void)
{
    LPC_InitTypeDef LPC_InitStruct;
    LPC_StructInit(&LPC_InitStruct);
    LPC_InitStruct.LPC_Channel   = LPC_CAPTURE_PIN;
    LPC_InitStruct.LPC_Edge      = LPC_Vin_Over_Vth;
    LPC_InitStruct.LPC_Threshold = LPC_100_mV;
    LPC_Init(&LPC_InitStruct);
    LPC_CounterReset();
    LPC_Cmd(ENABLE);

    /* Enable voltage detection interrupt.If Vin<Vth, cause this interrupt */
    LPC_INTConfig(LPC_INT_RTC_VOLTAGE_COMP, ENABLE);
    RTC_CpuNVICEnable(ENABLE);
    RamVectorTableUpdate(RTC_VECTORn, lpc_rtc_handler);

    /* Config LPC interrupt */
    NVIC_InitTypeDef NVIC_InitStruct;
    NVIC_InitStruct.NVIC_IRQChannel = RTC_IRQn;
    NVIC_InitStruct.NVIC_IRQChannelPriority = 3;
    NVIC_InitStruct.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStruct);
}

/**
  * @brief  demo code of operation about LPC.
  * @param   No parameter.
  * @return  void
  */
void lpc_rtc_vol_demo(void)
{
    /* Configure PAD and pinmux firstly! */
    DBG_DIRECT("lpc_rtc_vol_demo_code start");
    board_lpc_init();

    /* Initialize LPC peripheral */
    driver_lpc_init();

}

/**
  * @brief  LPC battery detection interrupt handle function.
  * @param  None.
  * @return None.
  */

void lpc_rtc_handler(void)
{
    LPC_ClearINTPendingBit(LPC_INT_RTC_VOLTAGE_COMP);//Add Application code here
    DBG_DIRECT("lpc_rtc_handler");
}

/** @} */ /* End of group LPC_Voltage_Detection_Exported_Functions */
/** @} */ /* End of group LPC_RTC_VOL_DEMO */


/******************* (C) COPYRIGHT 2016 Realtek Semiconductor Corporation *****END OF FILE****/
