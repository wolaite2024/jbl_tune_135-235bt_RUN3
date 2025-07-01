/**
*********************************************************************************************************
*               Copyright(c) 2015, Realtek Semiconductor Corporation. All rights reserved.
**********************************************************************************************************
* @file     dma_tim_demo.c
* @brief    This file provides demo code of GDMA send data to GPIO.
* @details
* @author   renee
* @date     2017-01-23
* @version  v1.0
*********************************************************************************************************
*/

/*============================================================================*
 *                              Header Files
 *============================================================================*/
#include <string.h>
#include "trace.h"
#include "dma_channel.h"
#include "rtl876x_gdma.h"
#include "rtl876x_gpio.h"
#include "rtl876x_nvic.h"
#include "rtl876x_pinmux.h"
#include "rtl876x_rcc.h"
#include "rtl876x_tim.h"
#include "hw_tim.h"

/** @defgroup  GDMA_TIM_DEMO  GDMA TIM Demo
    * @brief  Gdma send data to gpio with tim toggle implementation demo code
    * @{
    */

/*============================================================================*
 *                              Macros
 *============================================================================*/
/** @defgroup GDMA_TIM_Exported_Macros Gdma transfer to gpio by tim toggle Macros
  * @brief
  * @{
  */

#define PIN_OUT              ADC_2
#define GPIO_PIN_OUT         GPIO_GetPin(PIN_OUT);

#define DMA_TIMER_INTERVAL       (1000)

/** @} */ /* End of group GDMA_TIM_Exported_Macros */

/*============================================================================*
 *                              Variables
 *============================================================================*/
/** @defgroup GDMA_TIM_Exported_Variables Gdma transfer to gpio by tim toggle Variables
  * @brief
  * @{
  */

uint32_t gpio_control_pattern[10] = {0xffffffff, 0x0, 0xffffffff, 0x0, 0xffffffff, 0x0, 0xffffffff, 0x0, 0xffffffff, 0x0}; // dma source memory
uint32_t gpio_control_pattern_1[6][10] = {0xffffffff, 0x0, 0xffffffff, 0x0, 0xffffffff, 0x0, 0xffffffff, 0x0, 0xffffffff, 0x0,
                                          0xffffffff, 0xffffffff, 0x0, 0xffffffff, 0xffffffff, 0x0, 0xffffffff, 0xffffffff, 0x0, 0xffffffff,
                                          0xffffffff, 0x0, 0x0, 0xffffffff, 0x0, 0x0, 0xffffffff, 0x0, 0x0, 0xffffffff,
                                          0xffffffff, 0x0, 0xffffffff, 0x0, 0xffffffff, 0x0, 0xffffffff, 0x0, 0xffffffff, 0x0,
                                          0xffffffff, 0xffffffff, 0x0, 0xffffffff, 0xffffffff, 0x0, 0xffffffff, 0xffffffff, 0x0, 0xffffffff,
                                          0xffffffff, 0x0, 0x0, 0xffffffff, 0x0, 0x0, 0xffffffff, 0x0, 0x0, 0xffffffff
                                         };

GDMA_LLIDef GDMA_LLIStruct[12];
uint8_t tim_dma_ch_num = 0xa5;

#define TIM_DMA_CHANNEL_NUM     tim_dma_ch_num
#define TIM_DMA_CHANNEL         DMA_CH_BASE(tim_dma_ch_num)
#define TIM_DMA_IRQ             DMA_CH_IRQ(tim_dma_ch_num)

T_HW_TIMER_HANDLE demo_dma_timer_handle;

/** @} */ /* End of group GDMA_TIM_Exported_Variables */

/*============================================================================*
 *                              Functions
 *============================================================================*/
/** @defgroup GDMA_TIM_Exported_Functions Gdma transfer to gpio by tim toggle Functions
  * @brief
  * @{
  */
void tim_dma_handler(void);
/**
  * @brief  initialization of pinmux settings and pad settings.
  * @param   No parameter.
  * @return  void
  */
void board_dma_tim_init(void)
{
    Pad_Config(PIN_OUT, PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_NONE, PAD_OUT_ENABLE, PAD_OUT_HIGH);

    Pinmux_Config(PIN_OUT, DWGPIO);
}

bool driver_dma_tim_demo_create_hw_timer(void)
{
    demo_dma_timer_handle = hw_timer_create_dma_mode("demo_dma_timer", DMA_TIMER_INTERVAL, true, NULL);
    if (!demo_dma_timer_handle)
    {
        IO_PRINT_ERROR0("fail to create demo dma timer handle, check hw timer usage!");
        return false;
    }
    return true;
}

/**
  * @brief  Initialize GDMA peripheral.
  * @param   No parameter.
  * @return  void
  */
void driver_dma_multiblock_init(void)
{
    /* Initialize GPIO peripheral */
    RCC_PeriphClockCmd(APBPeriph_GPIO, APBPeriph_GPIO_CLOCK, ENABLE);
    GPIO_InitTypeDef GPIO_InitStruct;

    GPIO_StructInit(&GPIO_InitStruct);
    GPIO_InitStruct.GPIO_PinBit  = GPIO_PIN_OUT;
    GPIO_InitStruct.GPIO_Mode = GPIO_Mode_OUT;
    GPIO_InitStruct.GPIO_ITCmd = DISABLE;
    GPIO_InitStruct.GPIO_ITTrigger = GPIO_INT_Trigger_EDGE;
    GPIO_InitStruct.GPIO_ITPolarity = GPIO_INT_POLARITY_ACTIVE_HIGH;
    GPIO_InitStruct.GPIO_ITDebounce = GPIO_INT_DEBOUNCE_DISABLE;
    GPIO_InitStruct.GPIO_ControlMode = GPIO_HARDWARE_MODE;
    GPIOx_Init(GPIOA, &GPIO_InitStruct);

    if (!GDMA_channel_request(&tim_dma_ch_num, tim_dma_handler, true))
    {
        return;
    }

    // set DMA with GDMA_Handshake_TIM4; msize=1, transfer width = 32
    RCC_PeriphClockCmd(APBPeriph_GDMA, APBPeriph_GDMA_CLOCK, ENABLE);
    GDMA_InitTypeDef GDMA_InitStruct;

    GDMA_StructInit(&GDMA_InitStruct);
    GDMA_InitStruct.GDMA_ChannelNum          = TIM_DMA_CHANNEL_NUM;
    GDMA_InitStruct.GDMA_DIR                 = GDMA_DIR_MemoryToPeripheral;
    GDMA_InitStruct.GDMA_BufferSize          = 10;//determine total transfer size
    GDMA_InitStruct.GDMA_SourceInc           = DMA_SourceInc_Inc;
    GDMA_InitStruct.GDMA_DestinationInc      = DMA_DestinationInc_Fix;
    GDMA_InitStruct.GDMA_SourceDataSize      = GDMA_DataSize_Word;
    GDMA_InitStruct.GDMA_DestinationDataSize = GDMA_DataSize_Word;
    GDMA_InitStruct.GDMA_SourceMsize         = GDMA_Msize_1;
    GDMA_InitStruct.GDMA_DestinationMsize    = GDMA_Msize_1;
    GDMA_InitStruct.GDMA_SourceAddr          = (uint32_t)gpio_control_pattern;
    GDMA_InitStruct.GDMA_DestinationAddr     = (uint32_t)(
                                                   GPIOA_DMA_PORT_ADDR);  // vendor gpio reg address
    GDMA_InitStruct.GDMA_DestHandshake       = hw_timer_get_dma_handshake(demo_dma_timer_handle);
    GDMA_InitStruct.GDMA_Multi_Block_En = 0;
    GDMA_InitStruct.GDMA_Multi_Block_Struct = (uint32_t)GDMA_LLIStruct;
    GDMA_InitStruct.GDMA_Multi_Block_Mode = LLI_TRANSFER;
    for (int i = 0; i < 6; i++)
    {
        if (i == 5)
        {
            //GDMA_LLIStruct[i].LLP=0;
            GDMA_LLIStruct[i].SAR = (uint32_t)(&(gpio_control_pattern_1[0]));
            GDMA_LLIStruct[i].DAR = (uint32_t)(GPIOA_DMA_PORT_ADDR);
            GDMA_LLIStruct[i].LLP = 0;  // link back to beginning
            /* configure low 32 bit of CTL register */
            GDMA_LLIStruct[i].CTL_LOW = BIT(0)
                                        | (GDMA_InitStruct.GDMA_DestinationDataSize << 1)
                                        | (GDMA_InitStruct.GDMA_SourceDataSize << 4)
                                        | (GDMA_InitStruct.GDMA_DestinationInc << 7)
                                        | (GDMA_InitStruct.GDMA_SourceInc << 9)
                                        | (GDMA_InitStruct.GDMA_DestinationMsize << 11)
                                        | (GDMA_InitStruct.GDMA_SourceMsize << 14)
                                        | (GDMA_InitStruct.GDMA_DIR << 20);
            /* configure high 32 bit of CTL register */
            GDMA_LLIStruct[i].CTL_HIGH = GDMA_InitStruct.GDMA_BufferSize;
        }
        else
        {
            GDMA_LLIStruct[i].SAR = (uint32_t)(&(gpio_control_pattern_1[i]));
            GDMA_LLIStruct[i].DAR = (uint32_t)(GPIOA_DMA_PORT_ADDR);
            GDMA_LLIStruct[i].LLP = (uint32_t)&GDMA_LLIStruct[i + 1];
            /* configure low 32 bit of CTL register */
            GDMA_LLIStruct[i].CTL_LOW = BIT(0)
                                        | (GDMA_InitStruct.GDMA_DestinationDataSize << 1)
                                        | (GDMA_InitStruct.GDMA_SourceDataSize << 4)
                                        | (GDMA_InitStruct.GDMA_DestinationInc << 7)
                                        | (GDMA_InitStruct.GDMA_SourceInc << 9)
                                        | (GDMA_InitStruct.GDMA_DestinationMsize << 11)
                                        | (GDMA_InitStruct.GDMA_SourceMsize << 14)
                                        | (GDMA_InitStruct.GDMA_DIR << 20)
                                        | BIT(28) | BIT(27);
            /* configure high 32 bit of CTL register */
            GDMA_LLIStruct[i].CTL_HIGH = GDMA_InitStruct.GDMA_BufferSize;
        }
    }

    GDMA_Init(TIM_DMA_CHANNEL, &GDMA_InitStruct);
    GDMA_INTConfig(TIM_DMA_CHANNEL_NUM, GDMA_INT_Transfer, ENABLE);
    /*  Enable GDMA IRQ  */
    NVIC_InitTypeDef nvic_init_struct;
    nvic_init_struct.NVIC_IRQChannel         = TIM_DMA_IRQ;
    nvic_init_struct.NVIC_IRQChannelCmd      = (FunctionalState)ENABLE;
    nvic_init_struct.NVIC_IRQChannelPriority = 3;
    NVIC_Init(&nvic_init_struct);

    GDMA_Cmd(TIM_DMA_CHANNEL_NUM, ENABLE);

    // set timer toggle
    hw_timer_start(demo_dma_timer_handle);
}

/**
  * @brief  GDMA to GPIO Demo Function.
  * @param   No parameter.
  * @return  void
  */
void gpio_gdma_demo(void)
{
    board_dma_tim_init();

    if (driver_dma_tim_demo_create_hw_timer() == false)
    {
        return;
    }

    driver_dma_multiblock_init();
}

/**
* @brief  GDMA interrupt handler function.
* @param   No parameter.
* @return  void
*/
void tim_dma_handler(void)
{
    GDMA_ClearINTPendingBit(TIM_DMA_CHANNEL_NUM, GDMA_INT_Transfer);
    GDMA_channel_release(TIM_DMA_CHANNEL_NUM);
}

/** @} */ /* End of group GDMA_TIM_Exported_Functions */
/** @} */ /* End of group GDMA_TIM_DEMO */

