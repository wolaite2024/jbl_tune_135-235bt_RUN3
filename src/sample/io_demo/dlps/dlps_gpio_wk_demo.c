/**
*********************************************************************************************************
*               Copyright(c) 2017, Realtek Semiconductor Corporation. All rights reserved.
**********************************************************************************************************
* @file     dlps_gpio_wk_demo.c
* @brief    This file provides demo code of GPIO wakeup from DLPS mode.
* @details
* @author   justin
* @date     2021-03-02
* @version  v1.0
*********************************************************************************************************
*/



/*============================================================================*
 *                              Header Files
 *============================================================================*/
#include "trace.h"
#include "io_dlps.h"
#include "pm.h"
#include "rtl876x_gpio.h"
#include "rtl876x_nvic.h"
#include "rtl876x_pinmux.h"
#include "rtl876x_rcc.h"
#include "rtl8763_syson_reg.h"

/** @defgroup  DLPS_GPIO_WK_DEMO  GPIO INTERRUPT DEMO
    * @brief  DLPS GPIO wakeup implementation demo code
    * @{
    */

/*============================================================================*
 *                              Macros
 *============================================================================*/
/** @defgroup DLPS_GPIO_Exported_Macros DLPS GPIO wakeup Exported Macros
  * @brief
  * @{
  */
#define TEST_Pin                      P0_0
#define GPIO_IRQ                      GPIO0_IRQn
#define gpio_handler                  GPIO0_Handler
#define GPIO_Test_Pin                 GPIO_GetPin(TEST_Pin)


/** @} */ /* End of group DLPS_GPIO_Exported_Macros */

void dlps_gpio_wk_init(void);

/*============================================================================*
 *                              Functions
 *============================================================================*/
/** @defgroup DLPS_GPIO_wakeup_Exported_Functions
  * @brief
  * @{
  */
/**
 * @brief  initialization of pinmux settings and pad settings.
 * @param   No parameter.
 * @return  void  */
void board_gpio_init(void)
{
    Pad_Config(TEST_Pin, PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_UP, PAD_OUT_DISABLE, PAD_OUT_HIGH);
    Pinmux_Config(TEST_Pin, DWGPIO);
}


/**
  * @brief  Initialize GPIO peripheral.
  * @param   No parameter.
  * @return  void
  */
void driver_gpio_init(void)
{
    /* turn on GPIO clock */
    RCC_PeriphClockCmd(APBPeriph_GPIOA, APBPeriph_GPIOA_CLOCK, ENABLE);

    GPIO_InitTypeDef GPIO_InitStruct;

    GPIO_InitStruct.GPIO_PinBit  = GPIO_Test_Pin;
    GPIO_InitStruct.GPIO_Mode = GPIO_Mode_IN;
    GPIO_InitStruct.GPIO_ITCmd = ENABLE;
    GPIO_InitStruct.GPIO_ITTrigger = GPIO_INT_Trigger_EDGE;
    GPIO_InitStruct.GPIO_ITPolarity = GPIO_INT_POLARITY_ACTIVE_LOW;
    GPIO_InitStruct.GPIO_ITDebounce = GPIO_INT_DEBOUNCE_ENABLE;
    GPIO_InitStruct.GPIO_DebounceTime = 30;
    GPIOx_Init(GPIOA, &GPIO_InitStruct);
    GPIOx_MaskINTConfig(GPIOA, GPIO_Test_Pin, DISABLE);
    GPIOx_INTConfig(GPIOA, GPIO_Test_Pin, ENABLE);

    NVIC_InitTypeDef NVIC_InitStruct;
    NVIC_InitStruct.NVIC_IRQChannel = GPIO_IRQ;
    NVIC_InitStruct.NVIC_IRQChannelPriority = 1;
    NVIC_InitStruct.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStruct);
}

/**
  * @brief  read GPIO input.
  * @param   No parameter.
  * @return  void
  */
void gpio_test(void)
{
    uint32_t gpio_data = 0;
    gpio_data = GPIOx_ReadInputData(GPIOA);
    APP_PRINT_INFO1(" GPIOx_ReadInputData =0x%x\n", gpio_data);
}

/**
  * @brief  demo code of operation about GPIO.
  * @param   No parameter.
  * @return  void
  */
void dlps_gpio_wk_demo(void)
{
    /* Configure PAD and pinmux firstly! */
    board_gpio_init();

    /* Initialize GPIO peripheral */
    driver_gpio_init();

    /* GPIO function */
    gpio_test();

    /* dlps gpio wakup init */
    dlps_gpio_wk_init();
}

/**
* @brief  GPIO interrupt handler function.
* @param   No parameter.
* @return  void
*/
void gpio_handler(void)
{
    GPIOx_MaskINTConfig(GPIOA, GPIO_Test_Pin, ENABLE);

    // add user code here
    APP_PRINT_INFO0("gpio_handler\n");
    GPIOx_ClearINTPendingBit(GPIOA, GPIO_Test_Pin);
    GPIOx_MaskINTConfig(GPIOA, GPIO_Test_Pin, DISABLE);
}

void dlps_store(void)
{
    /*change PAD as SW mode*/
    Pad_Config(TEST_Pin, PAD_SW_MODE, PAD_IS_PWRON, PAD_PULL_UP, PAD_OUT_DISABLE, PAD_OUT_HIGH);

    /*set wakeup pin and polarity*/
    System_WakeUpPinEnable(TEST_Pin, PAD_WAKEUP_POL_LOW);
    DBG_DIRECT("enter dlps!!");
}


void dlps_restore(void)
{
    APP_PRINT_TRACE0("exit dlps!!");
    Pad_Config(TEST_Pin, PAD_SW_MODE, PAD_IS_PWRON, PAD_PULL_UP, PAD_OUT_DISABLE, PAD_OUT_HIGH);
    /*set platfrom as active mode*/
    power_mode_set(POWER_ACTIVE_MODE);
    if (System_WakeUpInterruptValue(TEST_Pin) == SET)
    {
        /* recall gpio interrupte handler for enter dlps IO interrupt lose*/
        gpio_handler();
    }
}

void dlps_gpio_wk_init(void)
{
    /*set btmac to dsm mode*/
    bt_power_mode_set(BTPOWER_DEEP_SLEEP);
    /*set platfrom as dlps mode*/
    power_mode_set(POWER_DLPS_MODE);
    /*IO register  store restore callback register*/
    DLPS_IORegister();
    /*user callback fuciton register*/
    DLPS_IORegUserDlpsEnterCb(dlps_store);
    DLPS_IORegUserDlpsExitCb(dlps_restore);
}

/*pin wakup will trigger this interrupt for enable it in rtl876x_io_dlps.c*/
void System_Handler(void)
{
    uint8_t i;
    APP_PRINT_INFO0("Wakeup pin Value set mode System_Handler ");
    NVIC_DisableIRQ(System_IRQn);  //Disable SYSTEM_ON Interrupt
    for (i = 0; i < TOTAL_PIN_NUM; i++)
    {
        if (System_WakeUpInterruptValue(i) == SET)
        {
            APP_PRINT_INFO1("wakeup for power down pin= %d ", i);
        }
    }
    Pad_ClearAllWakeupINT();
}
/** @} */ /* End of group DLPS_GPIO_wkup_Exported_Functions */
/** @} */ /* End of group DLPS_GPIO_WK_DEMO */
