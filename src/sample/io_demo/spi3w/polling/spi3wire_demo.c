/**
*********************************************************************************************************
*               Copyright(c) 2016, Realtek Semiconductor Corporation. All rights reserved.
**********************************************************************************************************
* @file     spi3wire_demo.c
* @brief    This file provides demo code of three wire SPI comunication.
* @details
* @author   elliot chen
* @date         2016-12-14
* @version  v1.0
*********************************************************************************************************
*/


/*============================================================================*
 *                              Header Files
 *============================================================================*/
#include "rtl876x_rcc.h"
#include "rtl876x_pinmux.h"
#include "rtl876x_3wire_spi.h"
#include "stdbool.h"

/** @defgroup  SPI3WIRE_DEMO  SPI3WIRE DEMO
    * @brief  3WireSPI implementation demo code
    * @{
    */

/*============================================================================*
 *                              Macros
 *============================================================================*/
/** @defgroup SPI3Wire_Demo_Exported_Macros SPI3Wire Demo Exported Macros
  * @brief
  * @{
  */

#define SENSOR_SPI              SPI3WIRE
#define SENSOR_SPI_CLK          0
#define SENSOR_SPI_DATA         1

/** @} */ /* End of group SPI3Wire_Demo_Exported_Macros */

/*============================================================================*
 *                              Functions
 *============================================================================*/
/** @defgroup SPI3Wire_Demo_Exported_Functions SPI3Wire Demo Exported Functions
  * @brief
  * @{
  */

/**
  * @brief  initialization of pinmux settings and pad settings.
  * @param   No parameter.
  * @return  void
  */
void Board_SPI3WIRE_Init(void)
{
    Pad_Config(SENSOR_SPI_CLK, PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_NONE, PAD_OUT_ENABLE,
               PAD_OUT_HIGH);
    Pad_Config(SENSOR_SPI_DATA, PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_NONE, PAD_OUT_ENABLE,
               PAD_OUT_HIGH);
    Pinmux_Config(SENSOR_SPI_CLK, SPI2W_CLK);
    Pinmux_Config(SENSOR_SPI_DATA, SPI2W_DATA);
}

/**
  * @brief  Initialize SPI3WIRE peripheral.
  * @param   No parameter.
  * @return  void
  */
void Driver_SPI3WIRE_Init(void)
{
    /* Enable 20M clock source */
    SYSBLKCTRL->u_200.REG_200 |= 0x3 << 13;
    //SYSBLKCTRL->PERI_FUNC0_EN |= 1<<24;
    //SYSBLKCTRL->PESOC_PERI_CLK_CTRL1 |=0x03<<16;
    /* Enable SPI3WIRE clock */
    RCC_PeriphClockCmd(APBPeriph_SPI2W, APBPeriph_SPI2W_CLOCK, ENABLE);

    /* Initialize IR */
    SPI3WIRE_InitTypeDef SPI3WIRE_InitStruct;
    SPI3WIRE_StructInit(&SPI3WIRE_InitStruct);

    SPI3WIRE_InitStruct.SPI3WIRE_SysClock       = 20000000;
    SPI3WIRE_InitStruct.SPI3WIRE_Speed          = 1000000;
    SPI3WIRE_InitStruct.SPI3WIRE_Mode           = SPI3WIRE_2WIRE_MODE;
    /* delay time = (SPI3WIRE_ReadDelay +1)/(2*SPI3WIRE_Speed). The delay time from the end of address phase to the start of read data phase */
    //delay time = (0x06 + 3)/(2 * speed) = 3us
    SPI3WIRE_InitStruct.SPI3WIRE_ReadDelay      = 0x5;
    SPI3WIRE_InitStruct.SPI3WIRE_OutputDelay    = SPI3WIRE_OE_DELAY_1T;
    SPI3WIRE_InitStruct.SPI3WIRE_ExtMode        = SPI3WIRE_NORMAL_MODE;
    SPI3WIRE_Init(&SPI3WIRE_InitStruct);
}

/**
 * @brief read one byte through 3wire SPI perpherial .
 * @param  address: address of register which need to read .
 * @return value of register.
*/
uint8_t Mouse_SingleRead(uint8_t address)
{
    uint8_t reg_value = 0;
    uint32_t timeout = 0;

    /* Check SPI busy or not */
    while (SPI3WIRE_GetFlagStatus(SPI3WIRE_FLAG_BUSY) == SET)
    {
        timeout++;
        if (timeout > 0x1ffff)
        {
            break;
        }
    }

    /* Clear Receive data length */
    SPI3WIRE_ClearRxDataLen();

    SPI3WIRE_StartRead(address, 1);

    timeout = 0;
    while (1)
    {
        /* Wait for the end of communication */
        if (SPI3WIRE_GetFlagStatus(SPI3WIRE_FLAG_BUSY) == SET)
        {
            break;
        }
        timeout++;
        if (timeout > 0x1ffff)
        {
            break;
        }
    }

    /* Get the length of received data */
    while (SPI3WIRE_GetRxDataLen() == 0);
    /* Read data */
    SPI3WIRE_ReadBuf(&reg_value, 1);

    return reg_value;
}

/**
 * @brief write one byte.
 * @param address: address of register which need to write data.
 * @param data: data which need to write.
 * @return TRUE: write success, FALSE: write failure.
*/
bool Mouse_SingleWrite(uint8_t address, uint8_t data)
{
    uint32_t timeout = 0;

    /* Check SPI busy or not */
    while (SPI3WIRE_GetFlagStatus(SPI3WIRE_FLAG_BUSY) == SET)
    {
        timeout++;
        if (timeout > 0x1ffff)
        {
            return false;
        }
    }
    /* Write data */
    SPI3WIRE_StartWrite(address, data);

    timeout = 0;
    while (1)
    {
        /* Wait communication to end */
        if (SPI3WIRE_GetFlagStatus(SPI3WIRE_FLAG_BUSY) == SET)
        {
            break;
        }
        timeout++;
        if (timeout > 0x1ffff)
        {
            return false;
        }
    }
    return true;
}

/**
 * @brief reset mouse.
 * @param none.
 * @return none.
*/
void Mouse_Reset(void)
{
    uint32_t delay = 0;

    Mouse_SingleWrite(0x06, (1 << 7));

    /* Delay 0.28ms */
    for (delay = 0; delay < 0x1fff; delay++) {;}
}

/**
 * @brief read mouse product ID.
 * @param  p_id, --pointer to production id buffer,buffer length should more than two.
 * @return ture.
*/
bool Mouse_GetProductID(uint8_t *p_id)
{

    /* Read product ID number high byte */
    *p_id++ = Mouse_SingleRead(0x20);

    /* Read product ID number low byte */
    *p_id = Mouse_SingleRead(0x21);

    return true;
}

/**
  * @brief  demo code of SPI3WIRE communication.
  * @param   No parameter.
  * @return  void
  */
void SPI3WIRE_DemoCode(void)
{
    uint8_t id[2] = {0, 0};

    Board_SPI3WIRE_Init();
    Driver_SPI3WIRE_Init();

    /* Send resync time. Resync signal time = 2*1/(2*SPI3WIRE_Speed) = 1us */
    SPI3WIRE_SetResyncTime(2);
    SPI3WIRE_ResyncSignalCmd(ENABLE);
    while (SPI3WIRE_GetFlagStatus(SPI3WIRE_FLAG_RESYNC_BUSY) == SET);
    SPI3WIRE_ResyncSignalCmd(DISABLE);

    /* Enable SPI3WIRE to normal communication */
    SPI3WIRE_Cmd(ENABLE);

    Mouse_Reset();
    Mouse_GetProductID(&id[0]);
    if ((0x58 != id[0]) || (0x20 != id[1]))
    {
        //Read mouse sensor ID error!
    }
}

/** @} */ /* End of group SPI3Wire_Demo_Exported_Functions */
/** @} */ /* End of group SPI3WIRE_DEMO */


/******************* (C) COPYRIGHT 2016 Realtek Semiconductor Corporation *****END OF FILE****/

