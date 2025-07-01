/**
*********************************************************************************************************
*               Copyright(c) 2015, Realtek Semiconductor Corporation. All rights reserved.
**********************************************************************************************************
* @file     i2c_demo.c
* @brief    This file provides demo code of I2C in interrupt mode which as a slave.
* @details
* @author   renee
* @date     2017-03-14
* @version  v1.0
*********************************************************************************************************
*/

/*============================================================================*
 *                              Header Files
 *============================================================================*/
#include "rtl876x_rcc.h"
#include "rtl876x_i2c.h"
#include "rtl876x_nvic.h"
#include "rtl876x_pinmux.h"

/** @defgroup  I2C_SLAVE_DEMO  I2C SLAVE DEMO
    * @brief  I2C work in slave mode implementation demo code
    * @{
    */

/*============================================================================*
 *                              Macros
 *============================================================================*/
/** @defgroup I2C_Slave_Demo_Exported_Macros I2C Slave Demo Exported Macros
  * @brief
  * @{
  */

#define I2C1_SCL                P1_4
#define I2C1_SDA                P1_5

/** @} */ /* End of group I2C_Slave_Demo_Exported_Macros */

/*============================================================================*
 *                              Functions
 *============================================================================*/
/** @defgroup I2C_Slave_Demo_Exported_Functions I2C Slave Demo Exported Functions
  * @brief
  * @{
  */

/**
  * @brief  initialization of pinmux settings and pad settings.
  * @param   No parameter.
  * @return  void
  */
void board_i2c1_init(void)
{
    Pinmux_Config(I2C1_SDA, I2C1_DAT);
    Pinmux_Config(I2C1_SCL, I2C1_CLK);
    Pad_Config(I2C1_SDA, PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_UP, PAD_OUT_ENABLE, PAD_OUT_HIGH);
    Pad_Config(I2C1_SCL, PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_UP, PAD_OUT_ENABLE, PAD_OUT_HIGH);
}

/**
  * @brief  Initialize I2C peripheral.
  * @param   No parameter.
  * @return  void
  */
void driver_i2c1_init(void)
{
    RCC_PeriphClockCmd(APBPeriph_I2C1, APBPeriph_I2C1_CLOCK, ENABLE);

    I2C_InitTypeDef  I2C_InitStructure;

    I2C_InitStructure.I2C_ClockSpeed = 100000;
    I2C_InitStructure.I2C_DeviveMode = I2C_DeviveMode_Slave;
    I2C_InitStructure.I2C_AddressMode = I2C_AddressMode_7BIT;
    I2C_InitStructure.I2C_SlaveAddress = 0x50;
    I2C_InitStructure.I2C_Ack = I2C_Ack_Enable;

    I2C_Init(I2C1, &I2C_InitStructure);
    I2C_Cmd(I2C1, ENABLE);

    /* detect stop signal */
    I2C_INTConfig(I2C1, I2C_INT_STOP_DET, ENABLE);
    /* Config I2C interrupt */
    NVIC_InitTypeDef NVIC_InitStruct;
    NVIC_InitStruct.NVIC_IRQChannel = I2C1_IRQn;
    NVIC_InitStruct.NVIC_IRQChannelPriority = 3;
    NVIC_InitStruct.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStruct);
}


/**
  * @brief  demo code of operation about I2C.
  * @param   No parameter.
  * @return  void
  */
void i2c1_slave_demo(void)
{
    board_i2c1_init();
    driver_i2c1_init();
}

/**
* @brief  I2C1 interrupt handler function.
* @param   No parameter.
* @return  void
*/
void I2C1_Handler(void)
{
    // any master want to read data will result this type interrupt.
    if (I2C_GetINTStatus(I2C1, I2C_INT_RD_REQ) == SET)
    {
        // add user code here
        //write data and not generate a stop signal. attention: slave have no right to generate stop signal. If ENABLE, it will cause stop signal which is not allowed .
        I2C_SendCmd(I2C1, 0x66, DISABLE, I2C_STOP_DISABLE);

        // clear interrupt
        I2C_ClearINTPendingBit(I2C1, I2C_INT_RD_REQ);
    }

    if (I2C_GetINTStatus(I2C1, I2C_INT_STOP_DET) == SET)
    {
        // add user code here

        // clear interrupt
        I2C_ClearINTPendingBit(I2C1, I2C_INT_STOP_DET);
    }
}

/** @} */ /* End of group I2C_Slave_Demo_Exported_Functions */
/** @} */ /* End of group I2C_SLAVE_DEMO */

