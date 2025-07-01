/**
*****************************************************************************************
*     Copyright(c) 2021, Realtek Semiconductor Corporation. All rights reserved.
*****************************************************************************************
  * @file
  * @brief
  * @details
  * @author
  * @date
  * @version
  ***************************************************************************************
  * @attention
  * <h2><center>&copy; COPYRIGHT 2021 Realtek Semiconductor Corporation</center></h2>
  ***************************************************************************************
  */

/*============================================================================*
 *                      Define to prevent recursive inclusion
 *============================================================================*/

#ifndef _AUDIO_PAD_CFG_H_
#define _AUDIO_PAD_CFG_H_

#ifdef __cplusplus
extern "C" {
#endif

// Speaker types
#define AOUT_TYPE_SINGLE_END            0x00
#define AOUT_TYPE_DIFFERENTIAL          0x01
#define AOUT_TYPE_CAPLESS               0x02
/* Enumeration */
typedef enum t_audio_pad_domain
{
    AUDIO_PAD_DOMAIN_0,
    AUDIO_PAD_DOMAIN_1,
    AUDIO_PAD_DOMAIN_2,
    AUDIO_PAD_DOMAIN_3,
    AUDIO_PAD_DOMAIN_4,
    AUDIO_PAD_DOMAIN_5,
    AUDIO_PAD_DOMAIN_6,
    AUDIO_PAD_DOMAIN_7,
    AUDIO_PAD_DOMAIN_MICBIAS,
    AUDIO_PAD_DOMAIN_MAX,
} T_AUDIO_PAD_DOMAIN;

typedef enum t_audio_pad_group
{
    AUDIO_PAD_GROUP_1,
    AUDIO_PAD_GROUP_2,
    AUDIO_PAD_GROUP_3,
    AUDIO_PAD_GROUP_4,
    AUDIO_PAD_GROUP_ADC,
    AUDIO_PAD_GROUP_HYB,
    AUDIO_PAD_GROUP_MAX,
} T_AUDIO_PAD_GROUP;

typedef union t_audio_pad_cfg_reg
{
    uint16_t d16;
    uint8_t d8[2];
} T_AUDIO_PAD_CFG_REG;

typedef union t_audio_pad_cfg_domain_para
{
    uint16_t d16;
    struct
    {
        uint8_t domain_num;
        uint8_t offset;
    };
} T_AUDIO_PAD_CFG_DOMAIN_PARA;

/* Define */
#define AUDIO_PAD_DRIVING_CTL_SIZE      4       // by bits
#define AUDIO_PAD_DCC_MASK              0xF
#define AUDIO_PAD_DRIVING_CTL_MASK      0xC

void audio_pad_cfg_load(void);
void audio_pad_cfg_bias_enable(bool enable);
void audio_pad_cfg_mic_set(uint32_t mic_sel, uint32_t mic_type);
uint8_t audio_pad_cfg_check_bias_gpio_mode(uint8_t micbias_voltage_sel);

#ifdef __cplusplus
}
#endif

#endif  // _AUDIO_PAD_CFG_H_

