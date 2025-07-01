/**
*********************************************************************************************************
*               Copyright(c) 2015, Realtek Semiconductor Corporation. All rights reserved.
**********************************************************************************************************
* @file     qdec_demo.c
* @brief    qdec demo
* @details
* @author   renee
* @date     2017-02-20
* @version  v1.0
*********************************************************************************************************
*/

/*============================================================================*
 *                              Header Files
 *============================================================================*/
#include "rtl876x_nvic.h"
#include "rtl876x_qdec.h"
#include "rtl876x_pinmux.h"
#include "rtl876x_rcc.h"

/** @defgroup  QDEC_DEMO  QDEC DEMO
    * @brief  Qdecode implementation demo code
    * @{
    */

/*============================================================================*
 *                              Macros
 *============================================================================*/
/** @defgroup QDECODE_Demo_Exported_Macros QDECODE Demo Exported Macros
  * @brief
  * @{
  */

/* phase A */
#define QDEC_Y_PHA_PIN          P1_0
/* phase B */
#define QDEC_Y_PHB_PIN          P1_1

/** @} */ /* End of group QDECODE_Demo_Exported_Macros */

/*============================================================================*
 *                              Variables
 *============================================================================*/
/** @defgroup QDECODE_Demo_Exported_Variables QDECODE Demo Exported Variables
  * @brief
  * @{
  */

int16_t   y_axis;
uint16_t  dir;

/** @} */ /* End of group QDECODE_Demo_Exported_Variables */

/*============================================================================*
 *                              Functions
 *============================================================================*/
/** @defgroup QDECODE_Demo_Exported_Functions QDECODE Demo Exported Functions
  * @brief
  * @{
  */
void QDEC_Handler(void);
/**
  * @brief  initialization of pinmux settings and pad settings.
  * @param   No parameter.
  * @return  void
  */
void board_qdec_init(void)
{
    /* Qdecoder pad config */
    Pad_Config(QDEC_Y_PHA_PIN, PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_UP, PAD_OUT_DISABLE,
               PAD_OUT_LOW);
    Pad_Config(QDEC_Y_PHB_PIN, PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_UP, PAD_OUT_DISABLE,
               PAD_OUT_LOW);

    /* Qdecoder pinmux config */
    Pinmux_Config(QDEC_Y_PHA_PIN, QDEC_PHASE_A_Y);
    Pinmux_Config(QDEC_Y_PHB_PIN, QDEC_PHASE_B_Y);
}

/**
  * @brief  Initialize Qdecoder peripheral.
  * @param   No parameter.
  * @return  void
  */
void driver_qdec_init(void)
{
    RCC_PeriphClockCmd(APBPeriph_QDEC, APBPeriph_QDEC_CLOCK, ENABLE);

    QDEC_InitTypeDef qdecInitStruct;
    QDEC_StructInit(&qdecInitStruct);
    qdecInitStruct.axisConfigY       = ENABLE;
    qdecInitStruct.debounceEnableY   = Debounce_Enable;
    QDEC_Init(QDEC, &qdecInitStruct);
    QDEC_INTConfig(QDEC, QDEC_Y_INT_NEW_DATA, ENABLE);

    NVIC_InitTypeDef nvic_init_struct;
    nvic_init_struct.NVIC_IRQChannel         = QDEC_IRQn;
    nvic_init_struct.NVIC_IRQChannelCmd      = (FunctionalState)ENABLE;
    nvic_init_struct.NVIC_IRQChannelPriority = 3;
    NVIC_Init(&nvic_init_struct);
}

/**
  * @brief  demo code of operation about qdec.
  * @param   No parameter.
  * @return  void
  */
void qdec_demo(void)
{
    /* Configure PAD and pinmux firstly! */
    board_qdec_init();

    /* Initialize qdec peripheral */
    driver_qdec_init();

    /* Enable qdec */
    QDEC_Cmd(QDEC, QDEC_AXIS_Y, ENABLE);
}

/**
* @brief  Qdecode interrupt handler function.
* @param   No parameter.
* @return  void
*/
void QDEC_Handler(void)
{
    if (QDEC_GetFlagState(QDEC, QDEC_FLAG_NEW_CT_STATUS_Y) == SET)
    {
        /* Mask qdec interrupt */
        QDEC_INTMask(QDEC, QDEC_Y_CT_INT_MASK, ENABLE);

        /* Read direction & count */
        dir = QDEC_GetAxisDirection(QDEC, QDEC_AXIS_Y);
        y_axis = QDEC_GetAxisCount(QDEC, QDEC_AXIS_Y);

        /* clear qdec interrupt flags */
        QDEC_ClearFlags(QDEC, QDEC_CLR_NEW_CT_Y);
        /* Unmask qdec interrupt */
        QDEC_INTMask(QDEC, QDEC_Y_CT_INT_MASK, DISABLE);
    }
}

/** @} */ /* End of group QDECODE_Demo_Exported_Functions */
/** @} */ /* End of group QDEC_DEMO */

