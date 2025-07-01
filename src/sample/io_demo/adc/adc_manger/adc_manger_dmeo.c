/**
*****************************************************************************************
*     Copyright(c) 2019, Realtek Semiconductor Corporation. All rights reserved.
*****************************************************************************************
   * @file     adc_demo.c
   * @brief    This file provides demo code of adc in one-shot mode.
   * @details
   * @author   sandy
   * @date     2019-03-14
   * @version   v1.0
   **************************************************************************************
   * @attention
   * <h2><center>&copy; COPYRIGHT 2019 Realtek Semiconductor Corporation</center></h2>
   **************************************************************************************
*/

/*============================================================================*
 *                              Header Files
 *============================================================================*/
#include "rtl876x_adc.h"
#include "trace.h"
#include "adc_manager.h"
#include "os_timer.h"


#if ADC_MANGER_DEMO
/** @defgroup  ADC_MANAGER_DEMO ADC Demo
    * @brief  adc implementation demo code
    * @{
    */

/*============================================================================*
 *                              Macros
 *============================================================================*/
/** @defgroup ADC_MANAGER_DEMO_Exported_Macros Adc Manager Exported Macros
    * @{
    */

/**  @brief  The  number of read back and read cycle*/
#define TEST_COUNT (100)
#define TEST_CYCLE (3)


/** End of ADC_MANAGER_DEMO_Exported_Macros
    * @}
    */

/*============================================================================*
 *                              Variables
 *============================================================================*/
/** @defgroup ADC_MANAGER_DEMO_Private_Variables  Private Variables
    * @{
    */

uint8_t test_adc_channel_handler;/*hander  for every adc read which register to the adc manger*/
int32_t test_count = TEST_COUNT;
int32_t test_cycle = TEST_CYCLE;
void *test_timer;

/** End of ADC_MANAGER_DEMO_Private_Variables
    * @}
    */


/*============================================================================*
 *                              Private Functions
 *============================================================================*/
/** @defgroup ADC_MANAGER_DEMO_Functions  Private Functions
    * @{
    */

/**
  * @brief  adc callback function execute when adc_mgr_enable_req be called  .
  * @param   No used.
  * @return  void
  */
void adc_cb(void *pvPara, uint32_t int_status)
{
    uint16_t adc_data[3] = {0};
    uint16_t sched_bit_map = 0x0007;

    adc_mgr_read_data_req(test_adc_channel_handler, adc_data, sched_bit_map);
    APP_PRINT_INFO4("adc_cb seq %d : %d, %d, %d", TEST_COUNT - test_count, adc_data[0], adc_data[1],
                    adc_data[2]);
    if (test_count > 0)
    {
        test_count--;
    }
    if (test_count == 0)
    {
        adc_mgr_free_chann(test_adc_channel_handler);
    }
}

/**
  * @brief  os timer handler function , enable the adc channel every timer count  to end.
  * @param   No used.
  * @return  void
  */
void test_timer_handler(void *xTimer)
{
    adc_mgr_enable_req(test_adc_channel_handler);
    if (test_count == 0)
    {
        test_cycle--;
        if (test_cycle > 0)
        {
            test_count = TEST_COUNT;
            ADC_InitTypeDef ADC_InitStruct;
            ADC_StructInit(&ADC_InitStruct);
            ADC_InitStruct.adcClock = ADC_CLK_4_88K;
            ADC_InitStruct.bitmap = 0x0007;
            ADC_InitStruct.schIndex[0] = INTERNAL_ICHARGE_MODE;
            ADC_InitStruct.schIndex[1] = INTERNAL_VBAT_MODE;
            ADC_InitStruct.schIndex[2] = INTERNAL_VADPIN_MODE;

            if (!adc_mgr_register_req(&ADC_InitStruct, (adc_callback_function_t)adc_cb,
                                      &test_adc_channel_handler))
            {
                APP_PRINT_ERROR0("test_adc_init: ADC Register Request Fail");
            }
        }
        else
        {
            os_timer_delete(&test_timer);
        }

    }
}
/** @} */ /* End of group ADC_MANAGER_DEMO_Private_Functions */


/*============================================================================*
 *                              Public Functions
 *============================================================================*/
/**
  * @brief  demo code of operation about ADC through adc manger.
            creat a timer task for read adc when timer count to end and regesiter one adc read event.
  * @param   No parameter.
  * @return  void
  */
void adc_manger_demo(void)
{
    os_timer_create(&test_timer, "TEST_TIMER", 2, 100, true, test_timer_handler);
    /* Start Carger Timer */
    if (!os_timer_start(&test_timer))
    {
        APP_PRINT_ERROR0("test_adc_init: Timer Start Fail");
    }

    ADC_InitTypeDef ADC_InitStruct;
    ADC_StructInit(&ADC_InitStruct);
    ADC_InitStruct.adcClock = ADC_CLK_4_88K;
    ADC_InitStruct.bitmap = 0x0007;
    ADC_InitStruct.schIndex[0] = INTERNAL_ICHARGE_MODE;
    ADC_InitStruct.schIndex[1] = INTERNAL_VBAT_MODE;
    ADC_InitStruct.schIndex[2] = INTERNAL_VADPIN_MODE;

    if (!adc_mgr_register_req(&ADC_InitStruct, (adc_callback_function_t)adc_cb,
                              &test_adc_channel_handler))
    {
        APP_PRINT_ERROR0("test_adc_init: ADC Register Request Fail");
    }
}

/** @} */ /* End of group ADC_MANAGER_DEMO */

#endif
