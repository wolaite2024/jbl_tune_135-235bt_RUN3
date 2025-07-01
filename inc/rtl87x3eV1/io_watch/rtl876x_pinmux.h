/**
*********************************************************************************************************
*               Copyright(c) 2015, Realtek Semiconductor Corporation. All rights reserved.
*********************************************************************************************************
* @file      rtl876x_pinmux.h
* @brief
* @details
* @author    Chuanguo Xue
* @date      2015-3-27
* @version   v1.0
* *********************************************************************************************************
*/


#ifndef _RTL876X_PINMUX_H_
#define _RTL876X_PINMUX_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "rtl876x.h"


/** @addtogroup PINMUX PINMUX
  * @brief Pinmux driver module
  * @{
  */

/*============================================================================*
 *                         Constants
 *============================================================================*/


/** @defgroup PINMUX_Exported_Constants PINMUX Exported Constants
  * @{
  */



/** @defgroup Pin_Function_Number Pin Function Number
  * @{
  */
#define IDLE_MODE           0
#define DIGI_DEBUG          1

#define I2C0_CLK            5
#define I2C0_DAT            6
#define I2C1_CLK            7
#define I2C1_DAT            8
#define PWM0_P              9
#define PWM0_N              10
#define PWM1_P              11
#define PWM1_N              12
#define TIMER_PWM0          13
#define TIMER_PWM1          14
#define TIMER_PWM2          15
#define TIMER_PWM3          16
#define TIMER_PWM4          17
#define TIMER_PWM5          18
#define TIMER_PWM6          19
#define TIMER_PWM7          20
#define QDEC_PHASE_A_X      21
#define QDEC_PHASE_B_X      22
#define QDEC_PHASE_A_Y      23
#define QDEC_PHASE_B_Y      24
#define QDEC_PHASE_A_Z      25
#define QDEC_PHASE_B_Z      26
#define UART1_TX            27
#define UART1_RX            28
#define UART2_TX            29
#define UART2_RX            30
#define UART2_CTS           31
#define UART2_RTS           32
#define IRDA_TX             33
#define IRDA_RX             34
#define UART0_TX            35
#define UART0_RX            36
#define UART0_CTS           37
#define UART0_RTS           38
#define SPI1_SS_N_0_MASTER  39
#define SPI1_SS_N_1_MASTER  40
#define SPI1_SS_N_2_MASTER  41
#define SPI1_CLK_MASTER     42
#define SPI1_MO_MASTER      43
#define SPI1_MI_MASTER      44
#define SPI0_SS_N_0_SLAVE   45
#define SPI0_CLK_SLAVE      46
#define SPI0_SO_SLAVE       47
#define SPI0_SI_SLAVE       48
#define SPI0_SS_N_0_MASTER  49
#define SPI0_CLK_MASTER     50
#define SPI0_MO_MASTER      51
#define SPI0_MI_MASTER      52

#define SWD_CLK             56
#define SWD_DIO             57

// KEY_COL_0 ~ KEY_COL_19: 58 ~ 77
// KEY_RAW_0 ~ KEY_RAW_11: 78 ~ 89

#define DWGPIO              90
#define LRC_SPORT1          91
#define BCLK_SPORT1         92
#define ADCDAT_SPORT1       93
#define DACDAT_SPORT1       94
#define SPDIF_TX            95
#define DMIC1_CLK           96
#define DMIC1_DAT           97
#define LRC_I_CODEC_SLAVE   98
#define BCLK_I_CODEC_SLAVE  99
#define SDI_CODEC_SLAVE     100
#define SDO_CODEC_SLAVE     101
#define LRC_I_PCM           102
#define BCLK_I_PCM          103
#define SDI_PCM             104
#define SDO_PCM             105
#define BT_COEX_I_0         106
#define BT_COEX_I_1         107
#define BT_COEX_I_2         108
#define BT_COEX_I_3         109
#define BT_COEX_O_0         110
#define BT_COEX_O_1         111
#define BT_COEX_O_2         112
#define BT_COEX_O_3         113
#define PTA_I2C_CLK_SLAVE   114
#define PTA_I2C_DAT_SLAVE   115
#define PTA_I2C_INT_OUT     116
#define DSP_GPIO_OUT        117
#define DSP_JTCK            118
#define DSP_JTDI            119
#define DSP_JTDO            120
#define DSP_JTMS            121
#define DSP_JTRST           122
#define LRC_SPORT0          123
#define BCLK_SPORT0         124
#define ADCDAT_SPORT0        125
#define DACDAT_SPORT0           126
#define MCLK_M                  127
#define SPI0_SS_N_1_MASTER        128
#define SPI0_SS_N_2_MASTER        129
#define SPI2_SS_N_0_MASTER        130
#define SPI2_CLK_MASTER           131
#define SPI2_MO_MASTER            132
#define SPI2_MI_MASTER            133
#define I2C2_CLK                134
#define I2C2_DAT                135
#define ISO7816_RST             136
#define ISO7816_CLK             137
#define ISO7816_IO              138
#define ISO7816_VCC_EN          139
//#define PWM8                    140
//#define PWM9                    141
//#define PWM10                   142
//#define PWM11                   143
#define DMIC2_CLK               144
#define DMIC2_DAT               145
//#define DMIC3_CLK               146
//#define DMIC3_DAT               147
//#define LRC_SPORT2              148
#define BCLK_SPORT2             149
#define ADCDAT_SPORT2           150
//#define DACDAT_SPORT2           151
//#define LRC_SPORT3              152
//#define BCLK_SPORT3             153
//#define ADCDAT_SPORT3           154
//#define DACDAT_SPORT3           155
//#define MCLK_S                  156
//#define SPIC1_SCK               157
//#define SPIC1_CSN               158
//#define SPIC1_SIO_0             159
//#define SPIC1_SIO_1             160
//#define SPIC1_SIO_2             161
//#define SPIC1_SIO_3             162
//#define EN_EXPA                 163
//#define EN_EXLNA                164
//#define SEL_TPM_SW              165
//#define SEL_TPM_N_SW            166
#define UART1_CTS               167
#define UART1_RTS               168
#define SPIC0_SCK               169
#define SPIC0_CSN               170
#define SPIC0_SIO_0             171
#define SPIC0_SIO_1             172
#define SPIC0_SIO_2             173
#define SPIC0_SIO_3             174
#define LRC_RX_CODEC_SLAVE      175
#define LRC_RX_SPORT0           176
#define LRC_RX_SPORT1           177
#define LRC_RX_SPORT2           178

#define PDM_AMP_DATA            193
#define PDM_AMP_CLK             194
/**
  * @}
  */



/** @defgroup AON_FAST_REG_PAD_TYPE
  * @{
  */
typedef union _AON_FAST_REG_PAD_TYPE
{
    uint16_t d16;
    struct
    {
        uint16_t REG_PAD_DUMMY0: 1;
        uint16_t REG_PAD_DUMMY1: 1;
        uint16_t PAD_WKUP_INT_EN: 1;
        uint16_t PAD_WK_STATUS: 1;
        uint16_t PAD_PINMUX_M_EN: 1;
        uint16_t PAD_SMT: 1;
        uint16_t PAD_E3: 1;
        uint16_t PAD_E2: 1;
        uint16_t PAD_SHDN_PW_ON: 1;
        uint16_t PAD_OUT_EN: 1;
        uint16_t PAD_WKPOL: 1;
        uint16_t PAD_WKEN: 1;
        uint16_t PAD_OUT_VALUE: 1;
        uint16_t PAD_PUPDC_WE_STR: 1;
        uint16_t PAD_PUPD_DIR: 1;
        uint16_t PAD_PU_EN: 1;
    };
} AON_FAST_REG_PAD_TYPE;
/** End of group AON_FAST_REG_PAD_TYPE
  * @}
  */

/** @defgroup AON_FAST_PAD_BIT_POS_TYPE
  * @{
  */
typedef enum _AON_FAST_PAD_POS_TYPE
{
    REG_PAD_DUMMY0,
    REG_PAD_DUMMY1,
    PAD_WKUP_INT_EN,
    PAD_WK_STATUS,

    PAD_PINMUX_M_EN,
    PAD_SMT,
    PAD_E3,
    PAD_E2,

    PAD_SHDN_PW_ON,
    PAD_OUT_EN,
    PAD_WKPOL,
    PAD_WKEN,

    PAD_OUT_VALUE,
    PAD_PUPDC_WE_STR,
    PAD_PUPD_DIR,
    PAD_PU_EN,
} AON_FAST_PAD_BIT_POS_TYPE;
/** End of group AON_FAST_PAD_BIT_POS_TYPE
  * @}
  */

/** @defgroup PINMUX_Exported_Types PINMUX Exported Types
  * @{
  */

/** @defgroup PAD_Pull_Mode PAD Pull Mode
  * @{
  */

typedef enum _PAD_Pull_Mode
{
    PAD_PULL_DOWN,
    PAD_PULL_UP,
    PAD_PULL_NONE
} PAD_Pull_Mode;

typedef enum _PAD_Pull_Value
{
    PAD_PULL_HIGH,
    PAD_PULL_LOW,

} PAD_Pull_VALUE;

/** @defgroup PAD_Pull_EN PAD Pull enable
  * @{
  */

typedef enum _PAD_Pull_EN
{
    PAD_PULL_DISABLE,
    PAD_PULL_ENABLE
} PAD_Pull_EN;


/** End of group PAD_Pull_Mode
  * @}
  */

/** End of group PAD_Pull_Mode
  * @}
  */

/** @defgroup PAD_Mode PAD Mode
  * @{
  */

typedef enum _PAD_Mode
{
    PAD_SW_MODE,
    PAD_PINMUX_MODE
} PAD_Mode;

/** End of group PAD_Mode
  * @}
  */

/** @defgroup PAD_Power_Mode PAD Power Mode
  * @{
  */

typedef enum _PAD_PWR_Mode
{
    /*PAD_NOT_PWRON,*/
    PAD_SHUTDOWN,
    PAD_IS_PWRON = 1
} PAD_PWR_Mode;

/** End of group PAD_Power_Mode
  * @}
  */

/** @defgroup PAD_Output_Config PAD Output Config
  * @{
  */

typedef enum _PAD_OUTPUT_ENABLE_Mode
{
    PAD_OUT_DISABLE,
    PAD_OUT_ENABLE
} PAD_OUTPUT_ENABLE_Mode;

/** End of group PAD_Output_Config
  * @}
  */

/** @defgroup PAD_Output_Value PAD Output Value
  * @{
  */

typedef enum _PAD_OUTPUT_VAL
{
    PAD_OUT_LOW,
    PAD_OUT_HIGH
} PAD_OUTPUT_VAL;

/** End of group PAD_Output_Value
  * @}
  */
/** @defgroup PAD_WakeUp_EN PAD WakeUp enable
  * @{
  */

typedef enum _PAD_WAKEUP_EN
{
    PAD_WAKEUP_DISABLE,
    PAD_WAKEUP_ENABLE
} PAD_WAKEUP_EN;

/** End of group PAD_WakeUp_Polarity_Value
  * @}
  */

typedef enum _SLEEP_LED_PIN
{
    SLEEP_LED_ADC_0,
    SLEEP_LED_ADC_1,
    SLEEP_LED_ADC_6,
    SLEEP_LED_ADC_7,
    SLEEP_LED_P1_0,
    SLEEP_LED_P1_1,
    SLEEP_LED_P1_4,
    SLEEP_LED_P2_0,
    SLEEP_LED_P2_1,
    SLEEP_LED_P2_2,
} SLEEP_LED_PIN;

/** @defgroup PAD_Function_Config PAD Function Config
  * @{
  */

typedef enum _PAD_FUNCTION_CONFIG_VALUE
{
    AON_GPIO,
    LED0,
    LED1,
    LED2,
    CLK_REQ,
} PAD_FUNCTION_CONFIG_VAL;

/** @defgroup PAD_WakeUp_Polarity_Value PAD WakeUp Polarity
  * @{
  */

typedef enum _PAD_WAKEUP_POL_VAL
{
    PAD_WAKEUP_POL_HIGH,
    PAD_WAKEUP_POL_LOW,
    PAD_WAKEUP_NONE
} PAD_WAKEUP_POL_VAL;

/** End of group PAD_WakeUp_Polarity_Value
  * @}
  */
/** @defgroup PAD_WakeUp_Debounce_En PAD WakeUp Debounce enable
  * @{
  */
typedef enum _PAD_WAKEUP_DEBOUNCE_EN
{
    PAD_WK_DEBOUNCE_DISABLE,
    PAD_WK_DEBOUNCE_ENABLE
} PAD_WAKEUP_DEBOUNCE_EN;

/**
  * @}
  */

/** @defgroup PAD_Pull_Value PAD Pull Value
  * @{
  */

typedef enum _PAD_PULL_CONFIG_VAL
{
    PAD_WEAKLY_PULL,
    PAD_STRONG_PULL
} PAD_PULL_VAL;
/**
  * @}
  */


/** @defgroup PAD Driving Current control value
  * @{
  */

/** End of group PAD_Driving_Current_E3_level
  * @}
  */




/** @defgroup PAD_LDO_Type PAD LDO Type
  * @{
  */

typedef enum _PAD_LDO_TYPE
{
    PAD_LDOAUX1,
    PAD_LDOAUX2
} PAD_LDO_TYPE;

/** End of group PAD_LDO_Type
  * @}
  */

typedef enum _PAD_AON_Status
{
    PAD_AON_OUTPUT_LOW,
    PAD_AON_OUTPUT_HIGH,
    PAD_AON_OUTPUT_DISABLE,
    PAD_AON_PINMUX_ON,
    PAD_AON_PIN_ERR
} PAD_AON_Status;


typedef enum _WAKEUP_POL
{
    POL_HIGH,
    POL_LOW,
} WAKEUP_POL;

typedef enum _WAKEUP_EN_MODE
{
    ADP_MODE,
    BAT_MODE,
    MFB_MODE,
    USB_MODE,
} WAKEUP_EN_MODE;

extern uint8_t hci_uart_rx_pin;
extern uint8_t hci_uart_tx_pin;

/** End of group PINMUX_Exported_Constants
  * @}
  */

/*============================================================================*
 *                         Functions
 *============================================================================*/

/**
  * @brief  According to the mode set to the pin , write the regster of AON which the pin coresponding .
  * @param  mode: mean one IO function, please refer to rtl876x_pinmux.h "Pin_Function_Number" part.
  *     @arg SHDN: use software mode.
  *     @arg PAD_OUT_EN: use pinmux mode.
        ......
        reference of bit of AON register mean in pinmux.h
  * @param  Pin_Num: pin number.
  *     This parameter is from ADC_0 to P4_1, please refer to rtl876x.h "Pin_Number" part.
  * @param  value: value of the register bit ,0 or 1.
  * @retval None
  */

void Pad_TableConfig(AON_FAST_PAD_BIT_POS_TYPE pad_bit_set, uint8_t Pin_Num, uint8_t value);

#define Pad_WKTableConfig       Pad_TableConfig

/** @defgroup PINMUX_Exported_Functions PINMUX Exported Functions
  * @{
  */

/**
  * @brief  Reset all pin to default value.
  * @param  None.
  * @note: two SWD pins will also be reset. Please use this function carefully.
  * @retval None
  */
extern void Pinmux_Reset(void);

/**
  * @brief  Deinit the IO function of one pin.
  * @param  Pin_Num: pin number.
  *     This parameter is from ADC_0 to P4_1, please refer to rtl876x.h "Pin_Number" part.
  * @retval None
  */
extern void Pinmux_Deinit(uint8_t Pin_Num);

/**
  * @brief  Config pin to its corresponding IO function.
  * @param  Pin_Num: pin number.
  *     This parameter is from ADC_0 to P4_1, please refer to rtl876x.h "Pin_Number" part.
  * @param  Pin_Func: mean one IO function, can be a value of @ref Pin_Function_Number.
  * @retval None
  */
extern void (*Pinmux_Config)(uint8_t Pin_Num, uint8_t Pin_Func);

/**
  * @brief  config the corresponding pad.
  * @param  Pin_Num: pin number.
  *     This parameter is from ADC_0 to P4_1, please refer to rtl876x.h "Pin_Number" part.
  * @param  AON_PAD_Mode: use software mode or pinmux mode.
  *     This parameter can be one of the following values:
  *     @arg PAD_SW_MODE: use software mode.
  *     @arg PAD_PINMUX_MODE: use pinmux mode.
  * @param  AON_PAD_PwrOn: config power of pad.
  *     This parameter can be one of the following values:
  *     @arg PAD_NOT_PWRON: shutdown power of pad.
  *     @arg PAD_IS_PWRON: enable power of pad.
  * @param  AON_PAD_Pull: config pad pull mode.
  *     This parameter can be one of the following values:
  *     @arg PAD_PULL_NONE: no pull.
  *     @arg PAD_PULL_UP: pull this pin up.
  *     @arg PAD_PULL_DOWN: pull thi pin down.
  * @param  AON_PAD_E: config pad out put function.
  *     This parameter can be one of the following values:
  *     @arg PAD_OUT_DISABLE: disable pin output.
  *     @arg PAD_OUT_ENABLE: enable pad output.
  * @param  AON_PAD_O: config pin output level.
  *     This parameter can be one of the following values:
  *     @arg PAD_OUT_LOW: pad output low.
  *     @arg PAD_OUT_HIGH: pad output high.
  * @retval None
  */
extern void (*Pad_Config)(uint8_t Pin_Num,
                          PAD_Mode AON_PAD_Mode,
                          PAD_PWR_Mode AON_PAD_PwrOn,
                          PAD_Pull_Mode AON_PAD_Pull,
                          PAD_OUTPUT_ENABLE_Mode AON_PAD_E,
                          PAD_OUTPUT_VAL AON_PAD_O);

/**
  * @brief  config the corresponding pad.
  * @param  Pin_Num: pin number.
  *     This parameter is from ADC_0 to P4_1, please refer to rtl876x.h "Pin_Number" part.
  * @param  AON_PAD_Mode: use software mode or pinmux mode.
  *     This parameter can be one of the following values:
  *     @arg PAD_SW_MODE: use software mode.
  *     @arg PAD_PINMUX_MODE: use pinmux mode.
  * @param  AON_PAD_PwrOn: config power of pad.
  *     This parameter can be one of the following values:
  *     @arg PAD_NOT_PWRON: shutdown power of pad.
  *     @arg PAD_IS_PWRON: enable power of pad.
  * @param  AON_PAD_Pull: config pad pull mode.
  *     This parameter can be one of the following values:
  *     @arg PAD_PULL_NONE: no pull.
  *     @arg PAD_PULL_UP: pull this pin up.
  *     @arg PAD_PULL_DOWN: pull thi pin down.
  * @param  AON_PAD_E: config pad out put function.
  *     This parameter can be one of the following values:
  *     @arg PAD_OUT_DISABLE: disable pin output.
  *     @arg PAD_OUT_ENABLE: enable pad output.
  * @param  AON_PAD_O: config pin output level.
  *     This parameter can be one of the following values:
  *     @arg PAD_OUT_LOW: pad output low.
  *     @arg PAD_OUT_HIGH: pad output high.
  * @param  AON_PAD_P: config pin output level.
  *     This parameter can be one of the following values:
  *     @arg PAD_150K_PULL: pad pull 150k resistance.
  *     @arg PAD_15K_PULL: pad pull 15k resistance.
  * @retval None
  */
extern void (*Pad_ConfigExt)(uint8_t Pin_Num,
                             PAD_Mode AON_PAD_Mode,
                             PAD_PWR_Mode AON_PAD_PwrOn,
                             PAD_Pull_Mode AON_PAD_Pull,
                             PAD_OUTPUT_ENABLE_Mode AON_PAD_E,
                             PAD_OUTPUT_VAL AON_PAD_O,
                             PAD_PULL_VAL AON_PAD_P);

/**
  * @brief  Set all pins to the default state.
  * @param  None.
  * @retval None.
  */
extern void (*Pad_AllConfigDefault)(void);

/**
  * @brief  Enable pin wakeup function.
  * @param  Pin_Num: pin number.
  *     This parameter is from ADC_0 to P4_1, please refer to rtl876x.h "Pin_Number" part.
  * @param  Polarity: PAD_WAKEUP_POL_HIGH--use high level wakeup, PAD_WAKEUP_POL_LOW-- use low level wakeup.
  * @retval None
  */
extern void (*System_WakeUpPinEnable)(uint8_t Pin_Num, uint8_t Polarity);

/**
  * @brief  Disable pin wakeup function.
  * @param  Pin_Num: pin number.
  *     This parameter is from ADC_0 to P4_1, please refer to rtl876x.h "Pin_Number" part.
  * @retval None
  */
extern void (*System_WakeUpPinDisable)(uint8_t Pin_Num);

/**
  * @brief  Disable wake up pin interrupt function.
  * @param  Pin_Num: pin number.
  *     This parameter is from ADC_0 to P4_1, please refer to rtl876x.h "Pin_Number" part.
  * @retval None
  */
extern void (*System_WakeUpInterruptDisable)(uint8_t Pin_Num);

/**
  * @brief  Disable wake up pin interrupt function.
  * @param  Pin_Num: pin number.
  *     This parameter is from ADC_0 to P4_1, please refer to rtl876x.h "Pin_Number" part.
  * @retval None
  */
extern void (*System_WakeUpInterruptEnable)(uint8_t Pin_Num);
/**
  * @brief  Check wake up pin interrupt status.
  * @param  Pin_Num: pin number.
  *     This parameter is from ADC_0 to P4_1, please refer to rtl876x.h "Pin_Number" part.
  * @retval Pin interrupt status
  */
extern uint8_t (*System_WakeUpInterruptValue)(uint8_t Pin_Num);

/**
  * @brief  config the pad output function.
  * @param  Pin_Num: pin number.
  *     This parameter is from ADC_0 to P4_1, please refer to rtl876x.h "Pin_Number" part.
  * @param  value: config pad out put function.
  *     This parameter can be one of the following values:
  *     @arg PAD_OUT_DISABLE: disable pin output.
  *     @arg PAD_OUT_ENABLE: enable pad output.
  * @retval None
  */
#define Pad_OutputEnableValue(Pin_Num, value) Pad_TableConfig(PAD_OUT_EN, Pin_Num, value)

/**
  * @brief  config pad pull enable or not.
  * @param  Pin_Num: pin number.
  *     This parameter is from ADC_0 to P4_1, please refer to rtl876x.h "Pin_Number" part.
  * @param  value: config pad out put function.
  *     This parameter can be one of the following values:
  *     @arg DISABLE: disable pin pull.
  *     @arg ENABLE: enable pin pull.
  * @retval None
  */
#define Pad_PullEnableValue(Pin_Num, value) Pad_TableConfig(PAD_PU_EN, Pin_Num, value)

extern  void Pad_PullEnableValue_Dir(uint8_t Pin_Num, uint8_t value,
                                     PAD_Pull_Mode Pull_Direction_value);

/**
  * @brief  config pad pull up or down or none.
  * @param  Pin_Num: pin number.
  *     This parameter is from ADC_0 to P4_1, please refer to rtl876x.h "Pin_Number" part.
  * @param  value : config pad PAD_Pull_Mode.
  *     This parameter can be one of the PAD_Pull_Mode type values:
  * @retval None
  */
#define Pad_PullUpOrDownValue(Pin_Num, value) Pad_TableConfig(PAD_PUPD_DIR, Pin_Num, value)

/**
  * @brief  config the pad control selected value.
  * @param  Pin_Num: pin number.
  *     This parameter is from ADC_0 to P4_1, please refer to rtl876x.h "Pin_Number" part.
  * @param  value: use software mode or pinmux mode.
  *     This parameter can be one of the following values:
  *     @arg PAD_SW_MODE: use software mode, aon control.
  *     @arg PAD_PINMUX_MODE: use pinmux mode, core control.
  * @retval None
  */
#define Pad_ControlSelectValue(Pin_Num, value) Pad_TableConfig(PAD_PINMUX_M_EN, Pin_Num, value)

/**
  * @brief  config the pad output value.
  * @param  Pin_Num: pin number.
  *     This parameter is from ADC_0 to P4_1, please refer to rtl876x.h "Pin_Number" part.
  * @param  value: config pin output level.
  *     This parameter can be one of the following values:
  *     @arg PAD_OUT_LOW: pad output low.
  *     @arg PAD_OUT_HIGH: pad output high.
  * @retval None
  */
#define Pad_OutputControlValue(Pin_Num, value) Pad_TableConfig(PAD_OUT_VALUE, Pin_Num, value)


/**
  * @brief  config pad wake up function.
  * @param  Pin_Num: pin number.
  *     This parameter is from ADC_0 to P4_1, please refer to rtl876x.h "Pin_Number" part.
  * @param  value: config pin output level.
  *     This parameter can be one of the following values:
  *     @arg ENABLE: Enable pad to wake up system from DLPS.
  *     @arg DISABLE: Disable pad to wake up from DLPS.
  * @retval None
  */
#define Pad_WakeupEnableValue(Pin_Num, value) Pad_WKTableConfig(PAD_WKEN, Pin_Num, value)


/**
  * @brief  config the pad wake up polarity.
  * @param  Pin_Num: pin number.
  *     This parameter is from ADC_0 to P4_1, please refer to rtl876x.h "Pin_Number" part.
  * @param  value: config pin output level.
  *     This parameter can be one of the following values:
  *     @arg PAD_WAKEUP_POL_HIGH: Positive level to wake up.
  *     @arg PAD_WAKEUP_POL_LOW: Negative level to wake up.
  * @retval None
  */
#define Pad_WakeupPolarityValue(Pin_Num, value) Pad_WKTableConfig(PAD_WKPOL, Pin_Num, value)

/**
  * @brief  config pad wake up interrupt.
  * @param  Pin_Num: pin number.
  *     This parameter is from ADC_0 to P4_1, please refer to rtl876x.h "Pin_Number" part.
  * @param  value: config pin output level.
  *     This parameter can be one of the following values:
  *     @arg ENABLE: Enable pad wake up to trigger System interrupt .
  *     @arg DISABLE: Disable pad wake up to trigger System interrupt.
  * @retval None
  */
extern void Pad_WakeupInterruptEnable(uint8_t Pin_Num, uint8_t value);

/**
  * @brief  Check wake up pin interrupt status.
  * @param  Pin_Num: pin number.
  *     This parameter is from ADC_0 to P4_1, please refer to rtl876x.h "Pin_Number" part.
  * @retval Pin interrupt status
  */
extern uint8_t Pad_WakeupInterruptValue(uint8_t Pin_Num);

/**
  * @brief  Clear pad wake up pin interrupt pending bit.
  * @param  Pin_Num: pin number.
  *     This parameter is from ADC_0 to P4_1, please refer to rtl876x.h "Pin_Number" part.
  * @retval None
  */
extern void Pad_ClearWakeupINTPendingBit(uint8_t Pin_Num);

/**
  * @brief  Clear all wake up pin interrupt pending bit.
  * @param  None
  * @retval None
  */
extern void Pad_ClearAllWakeupINT(void);

/**
  * @brief  config the pad output value.
  * @param  Pin_Num: pin number.
  *     This parameter is from ADC_0 to P4_1, please refer to rtl876x.h "Pin_Number" part.
  * @param  value: config pin output level.
  *     This parameter can be one of the following values:
  *     @arg PAD_OUT_LOW: pad output low.
  *     @arg PAD_OUT_HIGH: pad output high.
  * @retval None
  */
#define Pad_PowerOrShutDownValue(Pin_Num, value) Pad_TableConfig(PAD_SHDN_PW_ON, Pin_Num, value)

/**
  * @brief  config the pad pull value.
  * @param  Pin_Num: pin number.
  *     This parameter is from ADC_0 to P4_1, please refer to rtl876x.h "Pin_Number" part.
  * @param  value: config pin output level.
  *     This parameter can be one of the following values:
  *     @arg PAD_150K_PULL: pad pull 150k resistance.
  *     @arg PAD_15K_PULL: pad pull 15k resistance.
  * @retval None
  */
#define Pad_PullConfigValue(Pin_Num, value) Pad_TableConfig(PAD_PUPDC_WE_STR, Pin_Num, value)

/**
  * @brief  Config driving current value.
  * @param  Pin_Num: pin number.
  *     This parameter is from ADC_0 to P4_1, please refer to rtl876x.h "Pin_Number" part.
  * @param  e2_value: this parameter can be LEVEL_E2_0 and LEVEL_E2_1.
  * @param  e3_value: this parameter can be LEVEL_E3_0 and LEVEL_E3_1.
  * @retval none
  */
extern void Pad_DrivingCurrentControl(uint8_t Pin_Num, uint8_t e2_value, uint8_t e3_value);

/**
  * @brief  Config Pad Function.
  * @param  Pin_Num: pin number.
  *     This parameter is from ADC_0 to P7_1, please refer to rtl876x.h "Pin_Number" part.
  * @param  PAD_FUNCTION_CONFIG_VAL: value.
  *   This parameter can be: AON_GPIO, LED0, LED1, LED2, CLK_REQ.
  * @retval None
  */

extern void Pad_FunctionConfig(uint8_t Pin_Num, PAD_FUNCTION_CONFIG_VAL value);

/**
  * @brief  Config Pad Function.
  * @param  Pin_Num: pin number.
  *     This parameter is from ADC_0 to P4_1, please refer to rtl876x.h "Pin_Number" part.
  * @param  value: config value @ref PAD_FUNCTION_CONFIG_VAL
  *   This parameter can be: AON_GPIO, LED0, LED1, LED2, CLK_REQ.
  * @retval None
  */
extern void Pad_FunctionConfig(uint8_t Pin_Num, PAD_FUNCTION_CONFIG_VAL value);


/**
  * @brief  Get the Pad AON output value .
  * @param  Pin_Num:  pin number
  *            This parameter is from ADC_0 to P7_1, please refer to rtl876x.h "Pin_Number" part.
  * @retval reference PAD_AON_Status.
  */
uint8_t Pad_GetOutputCtrl(uint8_t Pin_Num);

/**
  * @brief  set the system wakeup mode  .
  * @param  mode: mode of set .
  *            This parameter reference WAKEUP_EN_MODE .
  *        pol: polarity to wake up
  *            This parameter WAKEUP_POL POL_HIGH means high level POL_LOW means low level to wakeup.
  *        NewState: Enable or disable to wake up
  *            This parameter value is ENABLE or DISABLE.
  * @retval  1 means wrong mode.
  */
extern uint8_t (*Pad_WakeUpCmd)(WAKEUP_EN_MODE mode, WAKEUP_POL pol, FunctionalState NewState);
extern const uint8_t digi_debug_pin[32];
#ifdef __cplusplus
}
#endif

#endif /* _RTL876X_PINMUX_H_ */

/** @} */ /* End of group PINMUX_Exported_Functions */
/** @} */ /* End of group PINMUX */


/******************* (C) COPYRIGHT 2015 Realtek Semiconductor *****END OF FILE****/

