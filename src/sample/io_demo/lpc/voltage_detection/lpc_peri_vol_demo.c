/**
*********************************************************************************************************
*               Copyright(c) 2016, Realtek Semiconductor Corporation. All rights reserved.
**********************************************************************************************************
* @file     lpc_peri_vol_demo.c
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

/** @defgroup  LPC_VOL_DEMO  LPC VOLTAGE DEMO
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

/** @} */ /* End of group LPC_Voltage_Detection_Exported_Macros */

/*============================================================================*
 *                              Functions
 *============================================================================*/
/** @defgroup LPC_Voltage_Detection_Exported_Functions LPC Voltage Detection Exported Functions
  * @brief
  * @{
  */
void lpc_handler(void);
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
    LPC_InitStruct.LPC_Edge      = LPC_Vin_Below_Vth;
    LPC_InitStruct.LPC_Threshold = LPC_800_mV;
    LPC_Init(&LPC_InitStruct);
    LPC_Cmd(ENABLE);

    /* Enable voltage detection interrupt.If Vin<Vth, cause this interrupt */
    LPC_INTConfig(LPC_INT_VOLTAGE_COMP, ENABLE);
    RamVectorTableUpdate(LPCOMP_VECTORn, lpc_handler);
    /* Config LPC interrupt */
    NVIC_InitTypeDef NVIC_InitStruct;
    NVIC_InitStruct.NVIC_IRQChannel = LPCOMP_IRQn;
    NVIC_InitStruct.NVIC_IRQChannelPriority = 2;
    NVIC_InitStruct.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStruct);
}

/**
  * @brief  demo code of operation about LPC.
  * @param   No parameter.
  * @return  void
  */
void lpc_peri_vol_demo(void)
{
    DBG_DIRECT("lpc_peri_vol_demo start");
    /* Configure PAD and pinmux firstly! */
    board_lpc_init();

    /* Initialize LPC peripheral */
    driver_lpc_init();
}

/**
  * @brief  LPC battery detection interrupt handle function.
  * @param  None.
  * @return None.
  */
void lpc_handler(void)
{
    DBG_DIRECT("lpc_handler");
    LPC_ClearINTPendingBit(LPC_INT_VOLTAGE_COMP);//Add Application code here
    //Add Application code here
}

/** @} */ /* End of group LPC_Voltage_Detection_Exported_Functions */
/** @} */ /* End of group LPC_VOL_DEMO */


/******************* (C) COPYRIGHT 2016 Realtek Semiconductor Corporation *****END OF FILE****/

