/*
 * Copyright (c) 2018, Realsil Semiconductor Corporation. All rights reserved.
 */

#include "trace.h"
#include "os_msg.h"
#include "rtl876x_nvic.h"
#include "rtl876x_rcc.h"
#include "rtl876x_gpio.h"
#include "rtl876x_pinmux.h"
#include "console.h"
#include "app_io_msg.h"
#include "app_gpio.h"
#include "app_key_gpio.h"
#include "app_cfg.h"
#include "app_dlps.h"
#include "app_main.h"
#if F_APP_GPIO_ONOFF_SUPPORT
#include "app_gpio_on_off.h"
#endif
#include "section.h"
#if F_APP_NFC_SUPPORT
#include "app_nfc.h"
#endif
#if (F_APP_SENSOR_SUPPORT == 1)
#include "app_sensor.h"
#endif
#include "vector_table.h"
#if (F_APP_SENSOR_PX318J_SUPPORT == 1)
#include "app_sensor_px318j.h"
#endif
#if (F_APP_SENSOR_IQS773_873_SUPPORT == 1)
#include "app_sensor_iqs773_873.h"
#endif
#if F_APP_LINEIN_SUPPORT
#include "app_line_in.h"
#endif
#if (F_APP_SENSOR_JSA1225_SUPPORT == 1) || (F_APP_SENSOR_JSA1227_SUPPORT == 1)
#include "app_sensor_jsa.h"
#endif

#if (F_APP_SLIDE_SWITCH_SUPPORT == 1)
#include "app_slide_switch.h"
#endif
#if F_APP_EXT_CHARGER_FEATURE_SUPPORT
#include "app_ext_charger.h"
#endif

/***********************************
GPIO must follow BB2
************************************/
static void gpio_init_sub_irq(uint8_t gpio);
void GPIO0_Handler(void);
void GPIO1_Handler(void);



ISR_TEXT_SECTION void gpio_param_config(uint8_t pin_num,
                                        GPIO_InitTypeDef *GPIO_InitStruct)
{
    if (GPIO_GetNum(pin_num) <= GPIO31)
    {
        GPIOx_Init(GPIOA, GPIO_InitStruct);
    }
    else
    {
        GPIOx_Init(GPIOB, GPIO_InitStruct);
    }
}

ISR_TEXT_SECTION void gpio_periphclk_config(uint8_t pin_num, FunctionalState NewState)
{
    if (GPIO_GetNum(pin_num) <= GPIO31)
    {
        RCC_PeriphClockCmd(APBPeriph_GPIOA, APBPeriph_GPIOA_CLOCK, NewState);
    }
    else
    {
        RCC_PeriphClockCmd(APBPeriph_GPIOB, APBPeriph_GPIOB_CLOCK, NewState);
    }
}

ISR_TEXT_SECTION void gpio_int_config(uint8_t pin_num, FunctionalState NewState)
{
    if (GPIO_GetNum(pin_num) <= GPIO31)
    {
        GPIOx_INTConfig(GPIOA, GPIO_GetPin(pin_num), NewState);
    }
    else
    {
        GPIOx_INTConfig(GPIOB, GPIO_GetPin(pin_num), NewState);
    }
}

ISR_TEXT_SECTION void gpio_mask_int_config(uint8_t pin_num, FunctionalState NewState)
{
    if (GPIO_GetNum(pin_num) <= GPIO31)
    {
        GPIOx_MaskINTConfig(GPIOA, GPIO_GetPin(pin_num), NewState);
    }
    else
    {
        GPIOx_MaskINTConfig(GPIOB, GPIO_GetPin(pin_num), NewState);
    }
}

ISR_TEXT_SECTION void gpio_clear_int_pending(uint8_t pin_num)
{
    if (GPIO_GetNum(pin_num) <= GPIO31)
    {
        GPIOx_ClearINTPendingBit(GPIOA, GPIO_GetPin(pin_num));
    }
    else
    {
        GPIOx_ClearINTPendingBit(GPIOB, GPIO_GetPin(pin_num));
    }
}

ISR_TEXT_SECTION void gpio_intpolarity_config(uint8_t pin_num,
                                              GPIOIT_PolarityType NewState)
{
    if (GPIO_GetNum(pin_num) <= GPIO31)
    {
        if (NewState)
        {
            GPIOA->INTPOLARITY |= GPIO_GetPin(pin_num);
        }
        else
        {
            GPIOA->INTPOLARITY &= ~(GPIO_GetPin(pin_num));
        }
    }
    else
    {
        if (NewState)
        {
            GPIOB->INTPOLARITY |= GPIO_GetPin(pin_num);
        }
        else
        {
            GPIOB->INTPOLARITY &= ~(GPIO_GetPin(pin_num));
        }
    }
}

void gpio_init_irq(uint8_t gpio)
{
    NVIC_InitTypeDef NVIC_InitStruct;

    switch (gpio)
    {
    case GPIO0:
        RamVectorTableUpdate(GPIO_A0_VECTORn, (IRQ_Fun)GPIO0_Handler);
        NVIC_InitStruct.NVIC_IRQChannel = GPIO_A0_IRQn;
        break;
    case GPIO1:
        RamVectorTableUpdate(GPIO_A1_VECTORn, (IRQ_Fun)GPIO1_Handler);
        NVIC_InitStruct.NVIC_IRQChannel = GPIO_A1_IRQn;
        break;
    case GPIO2:
    case GPIO3:
    case GPIO4:
    case GPIO5:
    case GPIO6:
    case GPIO7:
        NVIC_InitStruct.NVIC_IRQChannel = GPIO_A_2_7_IRQn;
        break;
#ifdef TARGET_RTL8773DO
    //https://jira.realtek.com/browse/BBPRO3RD-485
    case GPIO9:
        NVIC_InitStruct.NVIC_IRQChannel = GPIO_A_9_IRQn;
        break;
    case GPIO16:
        NVIC_InitStruct.NVIC_IRQChannel = GPIO_A_16_IRQn;
        break;
#else
    case GPIO9:
        NVIC_InitStruct.NVIC_IRQChannel = GPIO_A_8_15_IRQn;
        break;
    case GPIO16:
        NVIC_InitStruct.NVIC_IRQChannel = GPIO_A_16_23_IRQn;
        break;
#endif
    case GPIO8:
    case GPIO10:
    case GPIO11:
    case GPIO12:
    case GPIO13:
    case GPIO14:
    case GPIO15:
        NVIC_InitStruct.NVIC_IRQChannel = GPIO_A_8_15_IRQn;
        break;
    case GPIO17:
    case GPIO18:
    case GPIO19:
    case GPIO20:
    case GPIO21:
    case GPIO22:
    case GPIO23:
        NVIC_InitStruct.NVIC_IRQChannel = GPIO_A_16_23_IRQn;
        break;
    case GPIO24:
    case GPIO25:
    case GPIO26:
    case GPIO27:
    case GPIO28:
    case GPIO29:
    case GPIO30:
    case GPIO31:
        NVIC_InitStruct.NVIC_IRQChannel = GPIO_A_24_31_IRQn;
        break;
#ifndef TARGET_RTL8753GFE
    case GPIO32:
    case GPIO33:
    case GPIO34:
    case GPIO35:
    case GPIO36:
    case GPIO37:
    case GPIO38:
    case GPIO39:
        NVIC_InitStruct.NVIC_IRQChannel = GPIO_B_0_7_IRQn;
        break;
    case GPIO40:
    case GPIO41:
    case GPIO42:
    case GPIO43:
    case GPIO44:
    case GPIO45:
    case GPIO46:
    case GPIO47:
        NVIC_InitStruct.NVIC_IRQChannel = GPIO_B_8_15_IRQn;
        break;
    case GPIO48:
    case GPIO49:
    case GPIO50:
    case GPIO51:
    case GPIO52:
    case GPIO53:
    case GPIO54:
    case GPIO55:
        NVIC_InitStruct.NVIC_IRQChannel = GPIO_B_16_23_IRQn;
        break;
    case GPIO56:
    case GPIO57:
    case GPIO58:
    case GPIO59:
    case GPIO60:
    case GPIO61:
    case GPIO62:
    case GPIO63:
        NVIC_InitStruct.NVIC_IRQChannel = GPIO_B_24_31_IRQn;
        break;
#endif

    default:
        APP_PRINT_ERROR0("gpio_init_irq: wrong gpio num!!!");
        break;
    }

    NVIC_InitStruct.NVIC_IRQChannelPriority = 3;
    NVIC_InitStruct.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStruct);

    gpio_init_sub_irq(gpio);
}

ISR_TEXT_SECTION void gpio_write_output_level(uint8_t pin_num, uint8_t level)
{
    if (GPIO_GetNum(pin_num) <= GPIO31)
    {
        GPIOx_WriteBit(GPIOA, GPIO_GetPin(pin_num), (BitAction)level);
    }
    else
    {
        GPIOx_WriteBit(GPIOB, GPIO_GetPin(pin_num), (BitAction)level);
    }
}

ISR_TEXT_SECTION uint8_t gpio_read_input_level(uint8_t pin_num)
{
    uint8_t bit_value;
    if (GPIO_GetNum(pin_num) <= GPIO31)
    {
        bit_value =  GPIOx_ReadInputDataBit(GPIOA, GPIO_GetPin(pin_num));
    }
    else
    {
        bit_value = GPIOx_ReadInputDataBit(GPIOB, GPIO_GetPin(pin_num));
    }
    return bit_value;
}

ISR_TEXT_SECTION void gpio_handler(uint8_t gpio_num)
{
    uint32_t i;

    if (app_cfg_const.key_gpio_support)
    {
        if (GPIO_GetNum(app_cfg_const.key_pinmux[0]) == gpio_num)
        {
            key0_gpio_intr_handler();
            return;
        }

        for (i = 1; i < MAX_KEY_NUM; i++)
        {
            if (app_cfg_const.key_enable_mask & BIT(i))
            {
                if (GPIO_GetNum(app_cfg_const.key_pinmux[i]) == gpio_num)
                {
                    key_gpio_intr_handler(BIT(i), app_cfg_const.key_pinmux[i], i);
                    return;
                }
            }
        }
    }

    if (app_cfg_const.enable_rx_wake_up)
    {
        if (GPIO_GetNum(app_cfg_const.rx_wake_up_pinmux) == gpio_num)
        {
#if F_APP_CONSOLE_SUPPORT
            console_wakeup();
#endif
            return;
        }
    }

#if F_APP_GPIO_ONOFF_SUPPORT
    if (app_cfg_const.box_detect_method == GPIO_DETECT)
    {
        if (GPIO_GetNum(app_cfg_const.gpio_box_detect_pinmux) == gpio_num)
        {
            gpio_detect_intr_handler();
            return;
        }
    }
#endif

#if F_APP_NFC_SUPPORT
    if (app_cfg_const.nfc_support)
    {
        if (GPIO_GetNum(app_cfg_const.nfc_pinmux) == gpio_num)
        {
            nfc_gpio_intr_handler();
            return;
        }
    }
#endif

#if F_APP_LINEIN_SUPPORT
    if (app_cfg_const.line_in_support)
    {
        if (GPIO_GetNum(app_cfg_const.line_in_pinmux) == gpio_num)
        {
            app_line_in_detect_intr_handler();
            return;
        }
    }
#endif

#if (F_APP_SENSOR_SUPPORT == 1)
#if (F_APP_SENSOR_SL7A20_SUPPORT == 1)
    if (app_cfg_const.gsensor_support)
    {
        if (GPIO_GetNum(app_cfg_const.gsensor_int_pinmux) == gpio_num)
        {
            app_sensor_sl_int_gpio_intr_handler();
            return;
        }
    }
#endif

    if (app_cfg_const.psensor_support)
    {
        if (GPIO_GetNum(app_cfg_const.gsensor_int_pinmux) == gpio_num)
        {
            //P sensor is treated as key0
            if (app_cfg_const.psensor_vendor == PSENSOR_VENDOR_IO)
            {
                //P sensor is treated as key0
                key_gpio_intr_handler(BIT0, app_cfg_const.gsensor_int_pinmux, 0);
            }
#if (F_APP_SENSOR_IQS773_873_SUPPORT == 1)
            else if (app_cfg_const.psensor_vendor == PSENSOR_VENDOR_IQS773 ||
                     app_cfg_const.psensor_vendor == PSENSOR_VENDOR_IQS873)
            {
                app_psensor_iqs773_873_intr_handler();
            }
#endif
            return;
        }
    }

    if (app_cfg_const.sensor_support)
    {
#if (F_APP_SENSOR_JSA1225_SUPPORT == 1) || (F_APP_SENSOR_JSA1227_SUPPORT == 1)
        if (app_cfg_const.sensor_vendor == SENSOR_LD_VENDOR_JSA1225 ||
            app_cfg_const.sensor_vendor == SENSOR_LD_VENDOR_JSA1227)
        {
            if (GPIO_GetNum(app_cfg_const.sensor_detect_pinmux) == gpio_num)
            {
                app_sensor_jsa_int_gpio_intr_handler();
                return;
            }
        }
        else if (app_cfg_const.sensor_vendor == SENSOR_LD_VENDOR_IO)
#else
        if (app_cfg_const.sensor_vendor == SENSOR_LD_VENDOR_IO)
#endif
        {
            if (GPIO_GetNum(app_cfg_const.sensor_detect_pinmux) == gpio_num)
            {
                sensor_ld_io_int_gpio_intr_handler();
                return;
            }
        }
#if (F_APP_SENSOR_PX318J_SUPPORT == 1)
        else if (app_cfg_const.sensor_vendor == SENSOR_LD_VENDOR_PX)
        {
            if (GPIO_GetNum(app_cfg_const.sensor_detect_pinmux) == gpio_num)
            {
                app_sensor_px318j_int_gpio_intr_handler();
                return;
            }
        }
#endif
    }
#endif

#if (F_APP_SLIDE_SWITCH_SUPPORT == 1)
    if (app_slide_switch_gpio_handler(gpio_num))
    {
        return;
    }
#endif
}

ISR_TEXT_SECTION void GPIO0_Handler(void)
{
    gpio_handler(GPIO0);
}
/**
    * @brief  GPIO number 1 interrupt handler.
    * @param  void
    * @return void
    */
ISR_TEXT_SECTION void GPIO1_Handler(void)
{
    gpio_handler(GPIO1);
}

ISR_TEXT_SECTION void GPIO2_Handler(void)
{
    gpio_handler(GPIO2);
}

ISR_TEXT_SECTION void GPIO3_Handler(void)
{
    gpio_handler(GPIO3);
}

ISR_TEXT_SECTION void GPIO4_Handler(void)
{
    gpio_handler(GPIO4);
}

ISR_TEXT_SECTION void GPIO5_Handler(void)
{
    gpio_handler(GPIO5);
}

ISR_TEXT_SECTION void GPIO6_Handler(void)
{
    gpio_handler(GPIO6);
}

ISR_TEXT_SECTION void GPIO7_Handler(void)
{
    gpio_handler(GPIO7);
}

ISR_TEXT_SECTION void GPIO8_Handler(void)
{
    gpio_handler(GPIO8);
}

ISR_TEXT_SECTION void GPIO9_Handler(void)
{
    gpio_handler(GPIO9);
}

ISR_TEXT_SECTION void GPIO10_Handler(void)
{
    gpio_handler(GPIO10);
}

ISR_TEXT_SECTION void GPIO11_Handler(void)
{
    gpio_handler(GPIO11);
}

ISR_TEXT_SECTION void GPIO12_Handler(void)
{
    gpio_handler(GPIO12);
}

ISR_TEXT_SECTION void GPIO13_Handler(void)
{
    gpio_handler(GPIO13);
}

ISR_TEXT_SECTION void GPIO14_Handler(void)
{
    gpio_handler(GPIO14);
}

ISR_TEXT_SECTION void GPIO15_Handler(void)
{
    gpio_handler(GPIO15);
}

ISR_TEXT_SECTION void GPIO16_Handler(void)
{
    gpio_handler(GPIO16);
}

ISR_TEXT_SECTION void GPIO17_Handler(void)
{
    gpio_handler(GPIO17);
}

ISR_TEXT_SECTION void GPIO18_Handler(void)
{
    gpio_handler(GPIO18);
}

ISR_TEXT_SECTION void GPIO19_Handler(void)
{
    gpio_handler(GPIO19);
}

ISR_TEXT_SECTION void GPIO20_Handler(void)
{
    gpio_handler(GPIO20);
}

ISR_TEXT_SECTION void GPIO21_Handler(void)
{
    gpio_handler(GPIO21);
}

ISR_TEXT_SECTION void GPIO22_Handler(void)
{
    gpio_handler(GPIO22);
}

ISR_TEXT_SECTION void GPIO23_Handler(void)
{
    gpio_handler(GPIO23);
}

ISR_TEXT_SECTION void GPIO24_Handler(void)
{
    gpio_handler(GPIO24);
}

ISR_TEXT_SECTION void GPIO25_Handler(void)
{
    gpio_handler(GPIO25);
}

ISR_TEXT_SECTION void GPIO26_Handler(void)
{
    gpio_handler(GPIO26);
}

ISR_TEXT_SECTION void GPIO27_Handler(void)
{
    gpio_handler(GPIO27);
}

ISR_TEXT_SECTION void GPIO28_Handler(void)
{
    gpio_handler(GPIO28);
}

ISR_TEXT_SECTION void GPIO29_Handler(void)
{
    gpio_handler(GPIO29);
}

ISR_TEXT_SECTION void GPIO30_Handler(void)
{
    gpio_handler(GPIO30);
}

ISR_TEXT_SECTION void GPIO31_Handler(void)
{
    gpio_handler(GPIO31);
}
#ifndef TARGET_RTL8753GFE
/*GPIO32~63*/
ISR_TEXT_SECTION void GPIO32_Handler(void)
{
    gpio_handler(GPIO32);
}
/**
    * @brief  GPIOB number 1 interrupt handler.
    * @param  void
    * @return void
    */
ISR_TEXT_SECTION void GPIO33_Handler(void)
{
    gpio_handler(GPIO33);
}

ISR_TEXT_SECTION void GPIO34_Handler(void)
{
    gpio_handler(GPIO34);
}

ISR_TEXT_SECTION void GPIO35_Handler(void)
{
    gpio_handler(GPIO35);
}

ISR_TEXT_SECTION void GPIO36_Handler(void)
{
    gpio_handler(GPIO36);
}

ISR_TEXT_SECTION void GPIO37_Handler(void)
{
    gpio_handler(GPIO37);
}

ISR_TEXT_SECTION void GPIO38_Handler(void)
{
    gpio_handler(GPIO38);
}

ISR_TEXT_SECTION void GPIO39_Handler(void)
{
    gpio_handler(GPIO39);
}

ISR_TEXT_SECTION void GPIO40_Handler(void)
{
    gpio_handler(GPIO40);
}

ISR_TEXT_SECTION void GPIO41_Handler(void)
{
    gpio_handler(GPIO41);
}

ISR_TEXT_SECTION void GPIO42_Handler(void)
{
    gpio_handler(GPIO42);
}

ISR_TEXT_SECTION void GPIO43_Handler(void)
{
    gpio_handler(GPIO43);
}

ISR_TEXT_SECTION void GPIO44_Handler(void)
{
    gpio_handler(GPIO44);
}

ISR_TEXT_SECTION void GPIO45_Handler(void)
{
    gpio_handler(GPIO45);
}

ISR_TEXT_SECTION void GPIO46_Handler(void)
{
    gpio_handler(GPIO46);
}

ISR_TEXT_SECTION void GPIO47_Handler(void)
{
    gpio_handler(GPIO47);
}

ISR_TEXT_SECTION void GPIO48_Handler(void)
{
    gpio_handler(GPIO48);
}

ISR_TEXT_SECTION void GPIO49_Handler(void)
{
    gpio_handler(GPIO49);
}

ISR_TEXT_SECTION void GPIO50_Handler(void)
{
    gpio_handler(GPIO50);
}

ISR_TEXT_SECTION void GPIO51_Handler(void)
{
    gpio_handler(GPIO51);
}

ISR_TEXT_SECTION void GPIO52_Handler(void)
{
    gpio_handler(GPIO52);
}

ISR_TEXT_SECTION void GPIO53_Handler(void)
{
    gpio_handler(GPIO53);
}

ISR_TEXT_SECTION void GPIO54_Handler(void)
{
    gpio_handler(GPIO54);
}

ISR_TEXT_SECTION void GPIO55_Handler(void)
{
    gpio_handler(GPIO55);
}

ISR_TEXT_SECTION void GPIO56_Handler(void)
{
    gpio_handler(GPIO56);
}

ISR_TEXT_SECTION void GPIO57_Handler(void)
{
    gpio_handler(GPIO57);
}

ISR_TEXT_SECTION void GPIO58_Handler(void)
{
    gpio_handler(GPIO58);
}

ISR_TEXT_SECTION void GPIO59_Handler(void)
{
    gpio_handler(GPIO59);
}

ISR_TEXT_SECTION void GPIO60_Handler(void)
{
    gpio_handler(GPIO60);
}

ISR_TEXT_SECTION void GPIO61_Handler(void)
{
    gpio_handler(GPIO61);
}

ISR_TEXT_SECTION void GPIO62_Handler(void)
{
    gpio_handler(GPIO62);
}

ISR_TEXT_SECTION void GPIO63_Handler(void)
{
    gpio_handler(GPIO63);
}

#endif

static void gpio_init_sub_irq(uint8_t gpio)
{
    switch (gpio)
    {
    case GPIO0:
    case GPIO1:
        // GPIOA0, GPIOA1 not sub irq
        break;
    case GPIO2:
        RamVectorTableUpdate(GPIOA2_VECTORn, (IRQ_Fun)GPIO2_Handler);
        break;
    case GPIO3:
        RamVectorTableUpdate(GPIOA3_VECTORn, (IRQ_Fun)GPIO3_Handler);
        break;
    case GPIO4:
        RamVectorTableUpdate(GPIOA4_VECTORn, (IRQ_Fun)GPIO4_Handler);
        break;
    case GPIO5:
        RamVectorTableUpdate(GPIOA5_VECTORn, (IRQ_Fun)GPIO5_Handler);
        break;
    case GPIO6:
        RamVectorTableUpdate(GPIOA6_VECTORn, (IRQ_Fun)GPIO6_Handler);
        break;
    case GPIO7:
        RamVectorTableUpdate(GPIOA7_VECTORn, (IRQ_Fun)GPIO7_Handler);
        break;
    case GPIO8:
        RamVectorTableUpdate(GPIOA8_VECTORn, (IRQ_Fun)GPIO8_Handler);
        break;
    case GPIO9:
        RamVectorTableUpdate(GPIOA9_VECTORn, (IRQ_Fun)GPIO9_Handler);
        break;
    case GPIO10:
        RamVectorTableUpdate(GPIOA10_VECTORn, (IRQ_Fun)GPIO10_Handler);
        break;
    case GPIO11:
        RamVectorTableUpdate(GPIOA11_VECTORn, (IRQ_Fun)GPIO11_Handler);
        break;
    case GPIO12:
        RamVectorTableUpdate(GPIOA12_VECTORn, (IRQ_Fun)GPIO12_Handler);
        break;
    case GPIO13:
        RamVectorTableUpdate(GPIOA13_VECTORn, (IRQ_Fun)GPIO13_Handler);
        break;
    case GPIO14:
        RamVectorTableUpdate(GPIOA14_VECTORn, (IRQ_Fun)GPIO14_Handler);
        break;
    case GPIO15:
        RamVectorTableUpdate(GPIOA15_VECTORn, (IRQ_Fun)GPIO15_Handler);
        break;
    case GPIO16:
        RamVectorTableUpdate(GPIOA16_VECTORn, (IRQ_Fun)GPIO16_Handler);
        break;
    case GPIO17:
        RamVectorTableUpdate(GPIOA17_VECTORn, (IRQ_Fun)GPIO17_Handler);
        break;
    case GPIO18:
        RamVectorTableUpdate(GPIOA18_VECTORn, (IRQ_Fun)GPIO18_Handler);
        break;
    case GPIO19:
        RamVectorTableUpdate(GPIOA19_VECTORn, (IRQ_Fun)GPIO19_Handler);
        break;
    case GPIO20:
        RamVectorTableUpdate(GPIOA20_VECTORn, (IRQ_Fun)GPIO20_Handler);
        break;
    case GPIO21:
        RamVectorTableUpdate(GPIOA21_VECTORn, (IRQ_Fun)GPIO21_Handler);
        break;
    case GPIO22:
        RamVectorTableUpdate(GPIOA22_VECTORn, (IRQ_Fun)GPIO22_Handler);
        break;
    case GPIO23:
        RamVectorTableUpdate(GPIOA23_VECTORn, (IRQ_Fun)GPIO23_Handler);
        break;
    case GPIO24:
        RamVectorTableUpdate(GPIOA24_VECTORn, (IRQ_Fun)GPIO24_Handler);
        break;
    case GPIO25:
        RamVectorTableUpdate(GPIOA25_VECTORn, (IRQ_Fun)GPIO25_Handler);
        break;
    case GPIO26:
        RamVectorTableUpdate(GPIOA26_VECTORn, (IRQ_Fun)GPIO26_Handler);
        break;
    case GPIO27:
        RamVectorTableUpdate(GPIOA27_VECTORn, (IRQ_Fun)GPIO27_Handler);
        break;
    case GPIO28:
        RamVectorTableUpdate(GPIOA28_VECTORn, (IRQ_Fun)GPIO28_Handler);
        break;
    case GPIO29:
        RamVectorTableUpdate(GPIOA29_VECTORn, (IRQ_Fun)GPIO29_Handler);
        break;
    case GPIO30:
        RamVectorTableUpdate(GPIOA30_VECTORn, (IRQ_Fun)GPIO30_Handler);
        break;
    case GPIO31:
        RamVectorTableUpdate(GPIOA31_VECTORn, (IRQ_Fun)GPIO31_Handler);
        break;
#ifndef TARGET_RTL8753GFE
    case GPIO32:
        RamVectorTableUpdate(GPIOB0_VECTORn, (IRQ_Fun)GPIO32_Handler);
        break;
    case GPIO33:
        RamVectorTableUpdate(GPIOB1_VECTORn, (IRQ_Fun)GPIO33_Handler);
        break;
    case GPIO34:
        RamVectorTableUpdate(GPIOB2_VECTORn, (IRQ_Fun)GPIO34_Handler);
        break;
    case GPIO35:
        RamVectorTableUpdate(GPIOB3_VECTORn, (IRQ_Fun)GPIO35_Handler);
        break;
    case GPIO36:
        RamVectorTableUpdate(GPIOB4_VECTORn, (IRQ_Fun)GPIO36_Handler);
        break;
    case GPIO37:
        RamVectorTableUpdate(GPIOB5_VECTORn, (IRQ_Fun)GPIO37_Handler);
        break;
    case GPIO38:
        RamVectorTableUpdate(GPIOB6_VECTORn, (IRQ_Fun)GPIO38_Handler);
        break;
    case GPIO39:
        RamVectorTableUpdate(GPIOB7_VECTORn, (IRQ_Fun)GPIO39_Handler);
        break;
    case GPIO40:
        RamVectorTableUpdate(GPIOB8_VECTORn, (IRQ_Fun)GPIO40_Handler);
        break;
    case GPIO41:
        RamVectorTableUpdate(GPIOB9_VECTORn, (IRQ_Fun)GPIO41_Handler);
        break;
    case GPIO42:
        RamVectorTableUpdate(GPIOB10_VECTORn, (IRQ_Fun)GPIO42_Handler);
        break;
    case GPIO43:
        RamVectorTableUpdate(GPIOB11_VECTORn, (IRQ_Fun)GPIO43_Handler);
        break;
    case GPIO44:
        RamVectorTableUpdate(GPIOB12_VECTORn, (IRQ_Fun)GPIO44_Handler);
        break;
    case GPIO45:
        RamVectorTableUpdate(GPIOB13_VECTORn, (IRQ_Fun)GPIO45_Handler);
        break;
    case GPIO46:
        RamVectorTableUpdate(GPIOB14_VECTORn, (IRQ_Fun)GPIO46_Handler);
        break;
    case GPIO47:
        RamVectorTableUpdate(GPIOB15_VECTORn, (IRQ_Fun)GPIO47_Handler);
        break;
    case GPIO48:
        RamVectorTableUpdate(GPIOB16_VECTORn, (IRQ_Fun)GPIO48_Handler);
        break;
    case GPIO49:
        RamVectorTableUpdate(GPIOB17_VECTORn, (IRQ_Fun)GPIO49_Handler);
        break;
    case GPIO50:
        RamVectorTableUpdate(GPIOB18_VECTORn, (IRQ_Fun)GPIO50_Handler);
        break;
    case GPIO51:
        RamVectorTableUpdate(GPIOB19_VECTORn, (IRQ_Fun)GPIO51_Handler);
        break;
    case GPIO52:
        RamVectorTableUpdate(GPIOB20_VECTORn, (IRQ_Fun)GPIO52_Handler);
        break;
    case GPIO53:
        RamVectorTableUpdate(GPIOB21_VECTORn, (IRQ_Fun)GPIO53_Handler);
        break;
    case GPIO54:
        RamVectorTableUpdate(GPIOB22_VECTORn, (IRQ_Fun)GPIO54_Handler);
        break;
    case GPIO55:
        RamVectorTableUpdate(GPIOB23_VECTORn, (IRQ_Fun)GPIO55_Handler);
        break;
    case GPIO56:
        RamVectorTableUpdate(GPIOB24_VECTORn, (IRQ_Fun)GPIO56_Handler);
        break;
    case GPIO57:
        RamVectorTableUpdate(GPIOB25_VECTORn, (IRQ_Fun)GPIO57_Handler);
        break;
    case GPIO58:
        RamVectorTableUpdate(GPIOB26_VECTORn, (IRQ_Fun)GPIO58_Handler);
        break;
    case GPIO59:
        RamVectorTableUpdate(GPIOB27_VECTORn, (IRQ_Fun)GPIO59_Handler);
        break;
    case GPIO60:
        RamVectorTableUpdate(GPIOB28_VECTORn, (IRQ_Fun)GPIO60_Handler);
        break;
    case GPIO61:
        RamVectorTableUpdate(GPIOB29_VECTORn, (IRQ_Fun)GPIO61_Handler);
        break;
    case GPIO62:
        RamVectorTableUpdate(GPIOB30_VECTORn, (IRQ_Fun)GPIO62_Handler);
        break;
    case GPIO63:
        RamVectorTableUpdate(GPIOB31_VECTORn, (IRQ_Fun)GPIO63_Handler);
        break;
#endif
    default:
        APP_PRINT_ERROR1("gpio_init_sub_irq: wrong gpio num %d", gpio);
        break;
    }
}

