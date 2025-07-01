/**
*********************************************************************************************************
*               Copyright(c) 2015, Realtek Semiconductor Corporation. All rights reserved.
**********************************************************************************************************
* @file     tim8_11_demo.c
* @brief    tim interrupt demo
* @details
* @author   renee
* @date     2020-05-23
* @version  v0.1
*********************************************************************************************************
*/

/*============================================================================*
 *                              Header Files
 *============================================================================*/
#include <string.h>
#include "trace.h"
#include "hw_tim.h"
#include "section.h"

T_HW_TIMER_HANDLE demo_timer_handle = NULL;
/** @defgroup  TIM_DEMO  TIM DEMO
    * @brief  Tim implementation demo code
    * @{
    */
/*============================================================================*
 *                              Functions
 *============================================================================*/
/** @defgroup TIM_Demo_Exported_Functions TIM Demo Exported Functions
  * @brief
  * @{
  */

/**
* @brief  HW timer test callback function.
* @param   No parameter.
* @return  void
*/
RAM_TEXT_SECTION
void demo_hw_timer_callback(T_HW_TIMER_HANDLE handle)
{
    //Add User code here
    APP_PRINT_TRACE0("demo_hw_timer_callback");
}

/**
  * @brief  Initialize TIM peripheral.
  * @param   No parameter.
  * @return  void
  */
void tim_driver_init(void)
{
    demo_timer_handle = hw_timer_create("demo_hw_timer", 3000 * 1000, true, demo_hw_timer_callback);
    if (demo_timer_handle == NULL)
    {
        APP_PRINT_TRACE0("fail to create hw timer, check hw timer usage!!");
        return;
    }

    APP_PRINT_TRACE1("create hw timer instance successfully, id %d",
                     hw_timer_get_id(demo_timer_handle));

    hw_timer_start(demo_timer_handle);
}


/** @} */ /* End of group TIM_Demo_Exported_Functions */
/** @} */ /* End of group TIM_DEMO */
