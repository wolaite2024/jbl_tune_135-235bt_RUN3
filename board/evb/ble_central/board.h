/**
*********************************************************************************************************
*               Copyright(c) 2015, Realtek Semiconductor Corporation. All rights reserved.
*********************************************************************************************************
* @file     board.h
* @brief    Peripheral definitions for the project
* @details
* @author   Chuanguo Xue
* @date     2015-4-7
* @version  v1.0
* *********************************************************************************************************
*/

#ifndef _BOARD_H_
#define _BOARD_H_

#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

#define DATA_UART_TX_PIN    P3_1
#define DATA_UART_RX_PIN    P3_0

#define KEY_NULL                0
#define KEY0_MASK               0x01
#define KEY1_MASK               0x02
#define KEY2_MASK               0x04
#define KEY3_MASK               0x08
#define KEY4_MASK               0x10
#define KEY5_MASK               0x20
#define KEY6_MASK               0x40
#define KEY7_MASK               0x80

#define KEY_RELEASE             0
#define KEY_PRESS               1

#define MAX_KEY_NUM             8
#define HYBRID_KEY_NUM          8

#define LED_NUM                 3 //MAX: 3 LED

#define PWM_TIMER_PINMUX        timer_pwm5

#define HW_TIM_ADP_DET          TIM2
#define SENSOR_LD_TIMER         TIM3
#define LINE_IN_TIMER           TIM3
#define PWM_TIMER               TIM5
#define MULTI_SPK_TIMER         TIM6
#define DSP_TIMER               TIM7 //DSP HW fixed at timer7

#define HW_TIM_IRQn_ADP_DET     TIM2_IRQn
#define SENSOR_LD_TIMER_IRQ     TIM3_IRQn
#define LINE_IN_TIMER_IRQ       TIM3_IRQn
#define PWM_TIMER_IRQ           TIM5_IRQn
#define MULTI_SPK_TIMER_IRQ     TIM6_IRQn

#define APP_ADP_DET_TIMER_HANDLER       TIM2_Handler
#define LINE_IN_TIMER_INTR_HANDLER      TIM3_Handler
#define PWM_TIMER_INTR_HANDLER          TIM5_Handler
#define MULTI_SPK_TIMER_INTR_HANDLER    TIM6_Handler

#define UART_RX_BUFFER_SIZE         1024
#define RX_GDMA_START_ADDR          UART_RX_BUFFER_SIZE
#define RX_GDMA_BUFFER_SIZE         300

#define UART_TX_DMA_CHANNEL_NUM     8
#define UART_TX_DMA_CHANNEL         GDMA_Channel8
#define UART_TX_DMA_IRQ             GDMA0_Channel8_IRQn

#if SUPPORT_GDMA_CHANNAL9
#define UART_RX_DMA_CHANNEL_NUM     9
#define UART_RX_DMA_CHANNEL         GDMA_Channel9
#define UART_RX_DMA_IRQ             GDMA0_Channel9_IRQn
#endif
/** @defgroup IO Driver Config
  * @note user must config it firstly!! Do not change macro names!!
  * @{
  */

/* if use user define dlps enter/dlps exit callback function */
#define USE_USER_DEFINE_DLPS_EXIT_CB      1
#define USE_USER_DEFINE_DLPS_ENTER_CB     1

/* if use any peripherals below, #define it 1 */
#define USE_I2C0_DLPS               0
#define USE_I2C1_DLPS               0
#define USE_I2C2_DLPS               0
#define USE_TIM_DLPS                1
#define USE_QDECODER_DLPS           0
#define USE_IR_DLPS                 0
#define USE_RTC_DLPS                0
#define USE_UART0_DLPS              0
#define USE_UART1_DLPS              1
#define USE_UART2_DLPS              0
#define USE_UART3_DLPS              0
#define USE_ADC_DLPS                0
#define USE_SPI0_DLPS               0
#define USE_SPI1_DLPS               0
#define USE_SPI2W_DLPS              0
#define USE_KEYSCAN_DLPS            0
#define USE_DMIC_DLPS               0
#define USE_GPIOA_DLPS              0
#define USE_GPIOB_DLPS              0
#define USE_PWM0_DLPS               0
#define USE_PWM1_DLPS               0
#define USE_PWM2_DLPS               0
#define USE_PWM3_DLPS               0

#define USE_GDMACHANNEL0_DLPS       0
#define USE_GDMACHANNEL1_DLPS       1
#define USE_GDMACHANNEL2_DLPS       1
#define USE_GDMACHANNEL3_DLPS       0
#define USE_GDMACHANNEL4_DLPS       0
#define USE_GDMACHANNEL5_DLPS       0
#define USE_GDMACHANNEL6_DLPS       0
#define USE_GDMACHANNEL7_DLPS       0

#define USE_GDMA_DLPS               (USE_GDMACHANNEL0_DLPS | USE_GDMACHANNEL1_DLPS | USE_GDMACHANNEL2_DLPS\
                                     | USE_GDMACHANNEL3_DLPS | USE_GDMACHANNEL4_DLPS | USE_GDMACHANNEL5_DLPS\
                                     | USE_GDMACHANNEL6_DLPS | USE_GDMACHANNEL7_DLPS)

/* do not modify USE_IO_DRIVER_DLPS macro */
#define USE_IO_DRIVER_DLPS   (USE_I2C0_DLPS | USE_I2C1_DLPS | USE_I2C2_DLPS | USE_TIM_DLPS | USE_QDECODER_DLPS\
                              | USE_IR_DLPS | USE_RTC_DLPS | USE_UART_DLPS | USE_UART0_DLPS |USE_UART1_DLPS |USE_UART2_DLPS |USE_UART3_DLPS |USE_SPI0_DLPS\
                              | USE_SPI1_DLPS | USE_SPI2W_DLPS | USE_KEYSCAN_DLPS | USE_DMIC_DLPS\
                              | USE_GPIOB_DLPS | USE_GPIOA_DLPS | USE_USER_DEFINE_DLPS_EXIT_CB | USE_GDMA_DLPS\
                              | USE_RTC_DLPS | USE_PWM0_DLPS | USE_PWM1_DLPS | USE_PWM2_DLPS\
                              | USE_PWM3_DLPS | USE_USER_DEFINE_DLPS_ENTER_CB)

/**
  * @}
  */
#ifdef __cplusplus
}
#endif

#endif /* _BOARD_H_ */

