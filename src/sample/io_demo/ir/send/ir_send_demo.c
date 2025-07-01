/**
*********************************************************************************************************
*               Copyright(c) 2016, Realtek Semiconductor Corporation. All rights reserved.
**********************************************************************************************************
* @file     ir_send_demo.c
* @brief        This file provides IR demo code to send data by interrupt.
* @details
* @author   elliot chen
* @date         2016-12-07
* @version  v1.0
*********************************************************************************************************
*/

/*============================================================================*
 *                              Header Files
 *============================================================================*/
#include "rtl876x_pinmux.h"
#include "rtl876x_rcc.h"
#include "rtl876x_nvic.h"
#include "rtl876x_ir.h"
#include "ir_nec_protocol.h"

/** @defgroup  IR_SEND_DEMO  IR SEND DEMO
    * @brief  Ir send data implementation demo code
    * @{
    */

/*============================================================================*
 *                              Macros
 *============================================================================*/
/** @defgroup IR_Send_Demo_Exported_Macros IR_Send_Demo_Exported_Macros
  * @brief
  * @{
  */

#define IR_SEND_PIN                     0
#define IR_TX_FIFO_THR_LEVEL            2

/** @} */ /* End of group IR_Send_Demo_Exported_Macros */

/*============================================================================*
 *                              Variables
 *============================================================================*/
/** @defgroup IR_Send_Demo_Exported_Variables IR_Send_Demo_Exported_Variables
  * @brief
  * @{
  */

/* Buffer which store encoded data */
IR_DataTypeDef IR_DataStruct;
/* Number of data which has been sent */
uint8_t tx_count = 0;

/** @} */ /* End of group IR_Send_Demo_Exported_Variables */


/*============================================================================*
 *                              Functions
 *============================================================================*/
/** @defgroup IR_Send_Demo_Exported_Functions IR_Send_Demo_Exported_Functions
  * @brief
  * @{
  */

/**
  * @brief  initialization of pinmux settings and pad settings.
  * @param   No parameter.
  * @return  void
  */
void board_ir_init(void)
{
    Pad_Config(IR_SEND_PIN, PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_NONE, PAD_OUT_ENABLE, PAD_OUT_LOW);
    Pinmux_Config(IR_SEND_PIN, IRDA_TX);
}

/**
  * @brief  Initialize IR peripheral.
  * @param   No parameter.
  * @return  void
  */
void driver_ir_init(void)
{
    /* Enable IR clock */
    RCC_PeriphClockCmd(APBPeriph_IR, APBPeriph_IR_CLOCK, ENABLE);

    /* Initialize IR */
    IR_InitTypeDef IR_InitStruct;
    IR_StructInit(&IR_InitStruct);
    IR_InitStruct.IR_Freq           = 38;
    IR_InitStruct.IR_DutyCycle      = 2; /* !< 1/2 duty cycle */
    IR_InitStruct.IR_Mode           = IR_MODE_TX;
    IR_InitStruct.IR_TxInverse      = IR_TX_DATA_NORMAL;
    IR_InitStruct.IR_TxFIFOThrLevel = IR_TX_FIFO_THR_LEVEL;
    IR_Init(&IR_InitStruct);
    IR_Cmd(IR_MODE_TX, ENABLE);

    /* Configure NVIC */
    NVIC_InitTypeDef NVIC_InitStruct;
    NVIC_InitStruct.NVIC_IRQChannel = IR_IRQn;
    NVIC_InitStruct.NVIC_IRQChannelPriority = 2;
    NVIC_InitStruct.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStruct);
}

/**
  * @brief  demo code of IR send data.
  * @param   No parameter.
  * @return  void
  */
void ir_send_demo_code(void)
{
    /* Initialize IR */
    board_ir_init();
    driver_ir_init();

    /* Data to send */
    uint8_t ir_code[2] = {0x16, 0x28};

    /* Encode by NEC protocol */
    IR_NECEncode(38, ir_code[0], ir_code[1], &IR_DataStruct);

    /* Start to send first bytes data of encoded data */
    IR_SendBuf(IR_DataStruct.irBuf, IR_TX_FIFO_SIZE, DISABLE);
    /* Record number which has been sent */
    tx_count = IR_TX_FIFO_SIZE;

    /* Enable IR threshold interrupt. when TX FIFO offset <= threshold value, trigger interrupt*/
    IR_INTConfig(IR_INT_TF_LEVEL, ENABLE);
}

/**
* @brief IR interrupt handler function.
* @param   No parameter.
* @return  void
*/
void IR_Handler(void)
{
    /* Mask IR interrupt */
    IR_MaskINTConfig(IR_INT_TF_LEVEL, ENABLE);

    /* Continue to send by interrupt */
    if (IR_GetINTStatus(IR_INT_TF_LEVEL) == SET)
    {
        /* The remaining data is larger than the TX FIFO length */
        if ((NEC_LENGTH - tx_count) >= IR_TX_FIFO_SIZE)
        {
            IR_SendBuf(IR_DataStruct.irBuf + tx_count, (IR_TX_FIFO_SIZE - IR_TX_FIFO_THR_LEVEL), DISABLE);
            tx_count += (IR_TX_FIFO_SIZE - IR_TX_FIFO_THR_LEVEL);

            /* Clear threshold interrupt */
            IR_ClearINTPendingBit(IR_INT_TF_LEVEL_CLR);
        }
        else if ((NEC_LENGTH - tx_count) > 0)
        {
            /* The remaining data is less than the TX FIFO length */

            /*  Configure TX threshold level to zero and trigger interrupt when TX FIFO is empty */
            IR_SetTxThreshold(0);
            IR_SendBuf(IR_DataStruct.irBuf + tx_count, NEC_LENGTH - tx_count, ENABLE);
            tx_count += (NEC_LENGTH - tx_count);

            /* Clear threshold interrupt */
            IR_ClearINTPendingBit(IR_INT_TF_LEVEL_CLR);
        }
        else
        {
            /* Tx completed */
            /* Disable IR tx empty interrupt */
            IR_INTConfig(IR_INT_TF_LEVEL, DISABLE);
            tx_count = 0;

            /* Clear threshold interrupt */
            IR_ClearINTPendingBit(IR_INT_TF_LEVEL_CLR);
        }
    }

    /* Unmask IR interrupt */
    IR_MaskINTConfig(IR_INT_TF_LEVEL, DISABLE);
}

/** @} */ /* End of group IR_Send_Demo_Exported_Functions */
/** @} */ /* End of group IR_SEND_DEMO */


/******************* (C) COPYRIGHT 2016 Realtek Semiconductor Corporation *****END OF FILE****/

