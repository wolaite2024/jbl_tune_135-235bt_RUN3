/*
 * Copyright (c) 2018, Realsil Semiconductor Corporation. All rights reserved.
 */

#ifndef _APP_DSP_CFG_H_
#define _APP_DSP_CFG_H_

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif /*__cplusplus*/

//DSP data
#define DSP_PARAM_OFFSET              (0)//OTA_SIGNATURE_SIZE
#define DSP_PARAM_SIZE                2304
#define DSP_INIT_OFFSET               DSP_PARAM_OFFSET + DSP_PARAM_SIZE
#define DSP_INIT_SIZE                 256

#define DSP_CONFIG_TOOL_DEF_VER_0     0
#define DSP_CONFIG_TOOL_DEF_VER_1     1
#define DSP_CONFIG_TOOL_DEF_VER_2     2

typedef enum
{
    APP_DSP_EXTENSIBLE_PARAM_BRIGHTNESS  = 0x00,
} T_APP_DSP_EXTENSIBLE_PARAM_TYPE;

typedef struct
{
    uint32_t bypass_near_end : 1;
    uint32_t bypass_far_end : 1;
    uint32_t aec : 1;
    uint32_t dmnr : 1;
    uint32_t aes : 1;
    uint32_t nr_near_end : 1;
    uint32_t nr_far_end : 1;
    uint32_t pitch_det_near_end : 1;
    uint32_t pitch_det_far_end : 1;
    uint32_t drc_near_end : 1;
    uint32_t drc_far_end : 1;
    uint32_t rmdc_near_end : 1;
    uint32_t hpf_near_end : 1;
    uint32_t hpf_far_end : 1;
    uint32_t eq_near_end8k : 1;
    uint32_t eq_near_end16k : 1;
    uint32_t eq_far_end8k : 1;
    uint32_t eq_far_end16k : 1;
    uint32_t ignore_call_active_flag : 1;
    uint32_t dmnr_post_process : 1;
    uint32_t dmnr_test_mode : 1;
    uint32_t dmnr_bypass_lorr : 1;//0 for L,1 for R,effect if Dmnr and DmnrTestMode are 1
    uint32_t side_tone : 1;
    uint32_t far_end_loopback : 1;
    uint32_t dmnr_middle : 1; // 0: follow the setting of DmnrByPassLorR, 1: output L/2+R/2
    uint32_t nr_near_end128pts : 1;
    uint32_t bone_fusion : 1;
    uint32_t resv : 5;
} T_VOICE_PROC_ENB;

typedef struct
{
    // audio effect
    uint32_t bypass_audio_effect : 1;
    uint32_t mbagc_audio : 1;
    uint32_t eq_audio : 1;
    uint32_t aw_audio : 1;

    // audio pass through
    uint32_t audio_pass_through : 1;
    uint32_t vad_trigger : 1;
    uint32_t dehowling : 1;
    uint32_t nr_near_end : 1;

    uint32_t drc_near_end : 1;
    uint32_t rmdc_near_end : 1;
    uint32_t hpf_near_end : 1;
    uint32_t eq_near_end : 1;

    uint32_t fw_dsp_mix : 1;
    uint32_t bypass_near_end_effect : 1;
    uint32_t limiter : 1;
    uint32_t apt_mic_select : 1; // 0 for mic0, 1 for mic1

    uint32_t aec_alr : 1;
    uint32_t aes512 : 1;
    uint32_t resv : 14;

} T_AUDIO_PROC_ENB;

typedef struct t_app_dsp_eq_spk_header
{
    uint32_t app_default_spk_eq_idx: 8;
    uint32_t reserved: 24;
    uint32_t spk_eq_idx_bitmask;
} T_APP_DSP_EQ_SPK_HEADER;

typedef struct t_app_dsp_eq_mic_header
{
    uint32_t app_default_mic_eq_idx: 8;
    uint32_t reserved: 24;
    uint32_t mic_eq_idx_bitmask;
} T_APP_DSP_EQ_MIC_HEADER;

typedef struct t_app_dsp_eq_line_in_header
{
    uint32_t app_default_line_in_eq_idx: 8;
    uint32_t reserved: 24;
    uint32_t line_in_eq_idx_bitmask;
} T_APP_DSP_EQ_LINE_IN_HEADER;

typedef struct t_app_dsp_eq_sub_param_header
{
    uint32_t offset;
    uint8_t  sample_rate; // old eq is sample_rate, new eq is reserved1
    uint8_t  stream_type;
    uint16_t param_length;
    uint8_t  eq_type;
    uint8_t  eq_idx;
    uint16_t reserved2;
} T_APP_DSP_EQ_SUB_PARAM_HEADER;

typedef struct t_app_dsp_eq_param_header
{
    uint16_t sync_word;
    uint16_t reserved1;
    uint32_t eq_param_len;
    uint16_t eq_num;
    uint16_t reserved2;
    uint32_t eq_spk_applications_num: 8;
    uint32_t voice_eq_application_num: 8;
    uint32_t reserved3: 16;
    uint32_t eq_mic_applications_num: 8;
    uint32_t reserved4: 24;
    T_APP_DSP_EQ_SPK_HEADER *eq_spk_application_header;
    T_APP_DSP_EQ_MIC_HEADER *eq_mic_application_header;
    T_APP_DSP_EQ_SPK_HEADER *voice_eq_application_header;
    T_APP_DSP_EQ_SUB_PARAM_HEADER *sub_header;
} T_APP_DSP_EQ_PARAM_HEADER;

typedef struct t_app_dsp_eq_param
{
    T_APP_DSP_EQ_PARAM_HEADER *header;
    uint8_t *param;
} T_APP_DSP_EQ_PARAM;

typedef struct t_app_dsp_param_r_only
{
    uint16_t sync_word;
    uint16_t reserved1;
    uint32_t dsp_tool_version_info;
    uint16_t user_version;
    uint16_t reserved2;
    uint32_t algo_para_block_offset;
    uint32_t eq_cmd_block_offset;
    uint32_t fixed_param_block_offset;
    uint32_t vad_param_block_offset;
    uint32_t eq_extend_info_offset;
    uint32_t hw_eq_offset;
    uint32_t extensible_param_offset;
    uint8_t  reserved[4];
    uint32_t pacakge_features; //need check it in patch
    T_VOICE_PROC_ENB voice_stream_feature_bit;
    T_AUDIO_PROC_ENB audio_stream_feature_bit;
} T_APP_DSP_PARAM_R_ONLY;

typedef struct t_app_dsp_param_init_header
{
    uint16_t sync_word;
    uint8_t scenario_num;
    uint8_t dac_tone_gain_level;
    uint32_t fixed_para_len;
    uint8_t reserved[28];
} T_APP_DSP_PARAM_INIT_HEADER;

typedef struct t_app_dsp_param_init_setting
{
    uint8_t config_mode_num;
    uint8_t dac_l_gain_level_max;
    uint8_t dac_r_gain_level_max;
    uint8_t dac_l_gain_level_default;
    uint8_t dac_r_gain_level_default;
    uint8_t adc1_gain_level_max;
    uint8_t adc2_gain_level_max;
    uint8_t adc3_gain_level_max;
    uint8_t adc4_gain_level_max;
    uint8_t adc1_gain_level_default;
    uint8_t adc2_gain_level_default;
    uint8_t adc3_gain_level_default;
    uint8_t adc4_gain_level_default;
    uint8_t codec_selection;
    uint16_t reserved1;
    uint8_t i2s0_config;
    uint8_t i2s1_config;
    uint8_t i2s2_config;
    uint8_t i2s3_config;
    uint32_t hw_sidetone_digital_gain : 16;
    uint32_t hw_sidetone_mic_source   : 8;
    uint32_t hw_sidetone_enable       : 1;
    uint32_t hw_sidetone_hpf_enable   : 1;
    uint32_t hw_sidetone_hpf_level    : 4;
    uint32_t reserved2                : 2;
    uint8_t reserved3[32];
    int16_t dac_l_gain_table[16];
    int16_t dac_r_gain_table[16];
    int16_t adc1_analog_gain_table[16];
    int16_t adc2_analog_gain_table[16];
    int16_t adc3_analog_gain_table[16];
    int16_t adc4_analog_gain_table[16];
    int16_t adc1_digital_gain_table[16];
    int16_t adc2_digital_gain_table[16];
    int16_t adc3_digital_gain_table[16];
    int16_t adc4_digital_gain_table[16];
} T_APP_DSP_PARAM_INIT_SETTING;

typedef struct t_app_dsp_param_init
{
    T_APP_DSP_PARAM_INIT_HEADER dsp_param_init_header;
    T_APP_DSP_PARAM_INIT_SETTING *dsp_param_init_setting;
} T_APP_DSP_PARAM_INIT;

typedef struct t_app_dsp_extensible_brightness
{
    uint8_t brightness_level_max;
    uint8_t brightness_level_default;
    uint16_t reserve;
    uint16_t brightness_gain_table[16];
} T_APP_DSP_EXTENSIBLE_BRIGHTNESS;

typedef struct t_app_dsp_extensible_param
{
    uint16_t sync_word;
    uint16_t extensible_len;
    uint8_t *raw_data; // [type, length, value] ... [type, length, value]
} T_APP_DSP_EXTENSIBLE_PARAM;

typedef struct t_app_dsp_cfg_data
{
    int16_t notification_dac_dig_gain_table[16];
    int16_t notification_dac_ana_gain_table[16];
    int16_t audio_dac_dig_gain_table[16];
    int16_t audio_dac_ana_gain_table[16];
    int16_t audio_adc_gain_table[16];
    int16_t voice_dac_dig_gain_table[16];
    int16_t voice_dac_ana_gain_table[16];
    int16_t voice_adc_gain_table[16];
    int16_t apt_dac_dig_gain_table[16];
    int16_t apt_dac_ana_gain_table[16];
    int16_t apt_adc_gain_table[16];
    T_APP_DSP_PARAM_R_ONLY dsp_cfg_header;
    T_APP_DSP_EQ_PARAM     eq_param;
    T_APP_DSP_PARAM_INIT   dsp_param_init;
    T_APP_DSP_EXTENSIBLE_PARAM dsp_extensible_param;
} T_APP_DSP_CFG_DATA;


extern T_APP_DSP_CFG_DATA *app_dsp_cfg_data;

void app_dsp_cfg_load_param_r_data(void *p_data, uint16_t offset, uint16_t size);

void app_dsp_cfg_init_gain_level(void);

uint8_t app_dsp_cfg_get_tool_info_version(void);

void app_dsp_cfg_init(void);

#ifdef __cplusplus
}
#endif

#endif
