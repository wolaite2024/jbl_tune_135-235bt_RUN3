/**
*********************************************************************************************************
*               Copyright(c) 2015, Realtek Semiconductor Corporation. All rights reserved.
**********************************************************************************************************
* @file     led_demo.c
* @brief    led demo, LED Pin: P2_0, P2_1, P2_2, ADC_0, ADC_1, ADC_6, ADC_7, P1_0, P1_1, P1_4
* @details
* @author   renee
* @date     2017-01-23
* @version  v1.0
*********************************************************************************************************
*/

/*============================================================================*
 *                              Header Files
 *============================================================================*/
#include "rtl876x.h"
#include "rtl876x_pinmux.h"
#include "rtl876x_sleep_led.h"

/** @defgroup  LED_DEMO  LED DEMO
    * @brief  Led work in breathe mode and blink implementation demo code
    * @{
    */

/*============================================================================*
 *                              Macros
 *============================================================================*/
/** @defgroup Led_Demo_Exported_Macros Led Demo Exported Macros
  * @brief
  * @{
  */

#define LED_OUT_0       P2_0
#define LED_OUT_1       P2_1
#define LED_OUT_2       P2_2

/** @} */ /* End of group Led_Demo_Exported_Macros */

/*============================================================================*
 *                              Functions
 *============================================================================*/
/** @defgroup Led_Demo_Exported_Functions Led Demo Exported Functions
  * @brief
  * @{
  */

/**
  * @brief  initialization of pinmux settings and pad settings.
  * @param   No parameter.
  * @return  void
  */
void board_led_init(void)
{
    Pad_Config(LED_OUT_0, PAD_SW_MODE, PAD_IS_PWRON, PAD_PULL_UP, PAD_OUT_ENABLE, PAD_OUT_HIGH);
    Pad_Config(LED_OUT_1, PAD_SW_MODE, PAD_IS_PWRON, PAD_PULL_UP, PAD_OUT_ENABLE, PAD_OUT_HIGH);
    Pad_Config(LED_OUT_2, PAD_SW_MODE, PAD_IS_PWRON, PAD_PULL_UP, PAD_OUT_ENABLE, PAD_OUT_HIGH);

    Pad_FunctionConfig(LED_OUT_0, LED0);
    Pad_FunctionConfig(LED_OUT_1, LED1);
    Pad_FunctionConfig(LED_OUT_2, LED2);
}

/**
  * @brief  Initialize sleep led peripheral in breahe mode.
  * @param   No parameter.
  * @return  void
  */
void driver_led_init_breathe(void)
{
    SleepLed_Reset();
    SLEEP_LED_InitTypeDef Led_Initsturcture;
    SleepLed_StructInit(&Led_Initsturcture);
    Led_Initsturcture.prescale                    = 32;
    Led_Initsturcture.mode                        = LED_BREATHE_MODE;
    Led_Initsturcture.polarity                    = LED_OUTPUT_NORMAL;

    Led_Initsturcture.phase_uptate_rate[0]              = 2;
    Led_Initsturcture.phase_phase_tick[0]               = 60;
    Led_Initsturcture.phase_initial_duty[0]             = 5;
    Led_Initsturcture.phase_increase_duty[0]            = 1;
    Led_Initsturcture.phase_duty_step[0]                = 2;

    Led_Initsturcture.phase_uptate_rate[1]              = 2;
    Led_Initsturcture.phase_phase_tick[1]               = 60;
    Led_Initsturcture.phase_initial_duty[1]             = 40;
    Led_Initsturcture.phase_increase_duty[1]            = 0;
    Led_Initsturcture.phase_duty_step[1]                = 2;

    Led_Initsturcture.phase_uptate_rate[2]              = 2;
    Led_Initsturcture.phase_phase_tick[2]               = 60;
    Led_Initsturcture.phase_initial_duty[2]             = 40;
    Led_Initsturcture.phase_increase_duty[2]            = 0;
    Led_Initsturcture.phase_duty_step[2]                = 5;

    Led_Initsturcture.phase_uptate_rate[3]              = 2;
    Led_Initsturcture.phase_phase_tick[3]               = 60;
    Led_Initsturcture.phase_initial_duty[3]             = 0;
    Led_Initsturcture.phase_increase_duty[3]            = 1;
    Led_Initsturcture.phase_duty_step[3]                = 5;

    Led_Initsturcture.phase_uptate_rate[4]              = 2;
    Led_Initsturcture.phase_phase_tick[4]               = 60;
    Led_Initsturcture.phase_initial_duty[4]             = 40;
    Led_Initsturcture.phase_increase_duty[4]            = 0;
    Led_Initsturcture.phase_duty_step[4]                = 2;

    Led_Initsturcture.phase_uptate_rate[5]              = 2;
    Led_Initsturcture.phase_phase_tick[5]               = 50;
    Led_Initsturcture.phase_initial_duty[5]             = 0;
    Led_Initsturcture.phase_increase_duty[5]            = 1;
    Led_Initsturcture.phase_duty_step[5]                = 5;

    Led_Initsturcture.phase_uptate_rate[6]              = 2;
    Led_Initsturcture.phase_phase_tick[6]               = 50;
    Led_Initsturcture.phase_initial_duty[6]             = 45;
    Led_Initsturcture.phase_increase_duty[6]            = 0;
    Led_Initsturcture.phase_duty_step[6]                = 5;

    Led_Initsturcture.phase_uptate_rate[7]              = 2;
    Led_Initsturcture.phase_phase_tick[7]               = 50;
    Led_Initsturcture.phase_initial_duty[7]             = 5;
    Led_Initsturcture.phase_increase_duty[7]            = 0;
    Led_Initsturcture.phase_duty_step[7]                = 5;

    SleepLed_Init(LED_CHANNEL_0, &Led_Initsturcture);
    SleepLed_Init(LED_CHANNEL_1, &Led_Initsturcture);
    SleepLed_Init(LED_CHANNEL_2, &Led_Initsturcture);
    SleepLed_Cmd(LED_CHANNEL_1 | LED_CHANNEL_0 | LED_CHANNEL_2, ENABLE);
}

/**
  * @brief  Initialize sleep led peripheral blink mode.
  * @param   No parameter.
  * @return  void
  */
void driver_led_init_blink(void)
{
    SleepLed_Reset();
    SLEEP_LED_InitTypeDef Led_Initsturcture;
    SleepLed_StructInit(&Led_Initsturcture);
    Led_Initsturcture.mode                        = LED_BLINK_MODE;
    Led_Initsturcture.polarity                    = LED_OUTPUT_NORMAL;
    Led_Initsturcture.prescale                    = 32;
    Led_Initsturcture.period_high[0]                = 10;
    Led_Initsturcture.period_low[0]                  = 10;
    Led_Initsturcture.period_high[1]                = 20;
    Led_Initsturcture.period_low[1]                  = 20;
    Led_Initsturcture.period_high[2]                = 40;
    Led_Initsturcture.period_low[2]                  = 40;
    SleepLed_Init(LED_CHANNEL_0, &Led_Initsturcture);
    Led_Initsturcture.period_high[2]                = 0;
    Led_Initsturcture.period_low[2]                  = 0;
    SleepLed_Init(LED_CHANNEL_1, &Led_Initsturcture);
    Led_Initsturcture.period_high[1]                = 0;
    Led_Initsturcture.period_low[1]                  = 0;
    SleepLed_Init(LED_CHANNEL_2, &Led_Initsturcture);
    SleepLed_Cmd(LED_CHANNEL_1 | LED_CHANNEL_0 | LED_CHANNEL_2, ENABLE);
}

/**
  * @brief  demo code of operation about LED breathe mode.
  * @param   No parameter.
  * @return  void
  */
void led_breathe_demo(void)
{
    /* Configure PAD firstly! */
    board_led_init();

    /* Initialize LED peripheral */
    driver_led_init_breathe();
}

/**
  * @brief  demo code of operation about LED blink mode.
  * @param   No parameter.
  * @return  void
  */
void led_blink_demo(void)
{
    /* Configure PAD firstly! */
    board_led_init();

    /* Initialize LED peripheral */
    driver_led_init_blink();
}

/** @} */ /* End of group Led_Demo_Exported_Functions */
/** @} */ /* End of group LED_DEMO */

