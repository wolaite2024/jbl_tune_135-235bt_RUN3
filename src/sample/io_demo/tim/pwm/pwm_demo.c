/**
*********************************************************************************************************
*               Copyright(c) 2017, Realtek Semiconductor Corporation. All rights reserved.
**********************************************************************************************************
* @file     pwm_demo.c
* @brief    tim + pwm demo and Deadzone demo
* @details
* @author   renee
* @date     2017-01-23
* @version  v0.1
*********************************************************************************************************
*/

/*============================================================================*
 *                              Header Files
 *============================================================================*/
#include <string.h>
#include "rtl876x.h"
#include "trace.h"
#include "pwm.h"


/** @defgroup  PWM_DEMO  PWM DEMO
    * @brief  Pwm implementation demo code
    * @{
    */

/*============================================================================*
 *                              Macros
 *============================================================================*/
/** @defgroup PWM_Demo_Exported_Macros PWM Demo Exported Macros
  * @brief
  * @{
  */

#define PWM_OUT_PIN              P2_1
#define PWM_OUT_PIN_P            ADC_2
#define PWM_OUT_PIN_N            ADC_3
#define PWM_LOW_LEVEL_CNT                       2000    //LOW LEVEL count ,count frequnce is 1Mhz
#define PWM_HIGH_LEVEL_CNT                      2000    //High LEVEL count ,count frequnce is 1Mhz
/** @} */ /* End of group PWM_Demo_Exported_Macros */

T_PWM_HANDLE demo_pwm_handle;
T_PWM_HANDLE demo_pwm_deadzone_handle;

/*============================================================================*
 *                              Functions
 *============================================================================*/
/** @defgroup PWM_Demo_Exported_Functions PWM Demo Exported Functions
  * @brief
  * @{
  */

/**
  * @brief  Initialize PWM peripheral.
  * @param   No parameter.
  * @return  void
  */
static void driver_pwm_init(void)
{
    demo_pwm_handle = pwm_create("demo_pwm", PWM_HIGH_LEVEL_CNT, PWM_LOW_LEVEL_CNT, false);
    if (demo_pwm_handle == NULL)
    {
        APP_PRINT_ERROR0("Fail to create pwm handle!");
        return;
    }

    pwm_pin_config(demo_pwm_handle, PWM_OUT_PIN, PWM_FUNC);
    pwm_start(demo_pwm_handle);

    demo_pwm_deadzone_handle = pwm_create("demo_pwm_deadzone", PWM_HIGH_LEVEL_CNT, PWM_LOW_LEVEL_CNT,
                                          true);
    if (demo_pwm_deadzone_handle == NULL)
    {
        APP_PRINT_ERROR0("Fail to create pwm handle!");
        return;
    }

    pwm_pin_config(demo_pwm_deadzone_handle, PWM_OUT_PIN_P, PWM_FUNC_P);
    pwm_pin_config(demo_pwm_deadzone_handle, PWM_OUT_PIN_N, PWM_FUNC_N);
    pwm_start(demo_pwm_deadzone_handle);
}

/**
  * @brief  demo code of operation about PWM.
  * @param   No parameter.
  * @return  void
  */
void pwm_demo(void)
{
    /* Initialize PWM peripheral */
    driver_pwm_init();
}

/** @} */ /* End of group PWM_Demo_Exported_Functions */
/** @} */ /* End of group PWM_DEMO */

