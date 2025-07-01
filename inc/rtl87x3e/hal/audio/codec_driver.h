/**
*****************************************************************************************
*     Copyright(c) 2017, Realtek Semiconductor Corporation. All rights reserved.
*****************************************************************************************
  * @file
  * @brief
  * @details
  * @author
  * @date
  * @version
  ***************************************************************************************
  * @attention
  * <h2><center>&copy; COPYRIGHT 2017 Realtek Semiconductor Corporation</center></h2>
  ***************************************************************************************
  */

/*============================================================================*
 *                      Define to prevent recursive inclusion
 *============================================================================*/

#ifndef _CODEC_DRIVER_H_
#define _CODEC_DRIVER_H_

#ifdef __cplusplus
extern "C" {
#endif

/* Version Description
 *
 * AVCC control flow - [BB2 AVCC control flow_v3.docx](20210401)
 * Codec register command - [BB2_Codec_Register_Commanding_Procedure_B-cut_v2.xlsx](20210727)
 * sidetone control - [BB2 HW Sidetone@5M_v1.docx], [BB2 HW Sidetone@ds_v1.docx](20210722)
 *
 */

#define CODEC_BYPASS_APAD_MUTE      0   // bypass APAD output if FT DC K not ready

/* Enumeration */
typedef enum t_codec_err
{
    CODEC_ERR_NONE,
    CODEC_ERR_PARAM,
    CODEC_ERR_MEM,
    CODEC_ERR_STATE,
    CODEC_ERR_EMPTY,

    CODEC_ERR_MAX,
} T_CODEC_ERR;

typedef enum t_codec_event
{
    CODEC_EVENT_NONE,
    CODEC_EVENT_SET_CONFIG,
    CODEC_EVENT_HALT_TIMEOUT,

    CODEC_EVENT_MAX,
} T_CODEC_EVENT;

typedef enum t_codec_cb_event
{
    CODEC_CB_EVENT_NONE,
    CODEC_CB_STATE_MSG,
    CODEC_CB_GET_ANC_STATUS,
    CODEC_CB_SET_MICBIAS_PAD,
    CODEC_CB_CLEAR_SIDETONE,
    CODEC_CB_EVENT_MAX,
} T_CODEC_CB_EVENT;

typedef enum t_codec_parse_type
{
    CODEC_REQ_TYPE_DAC_ANA_PARSER,
    CODEC_REQ_TYPE_DAC_DIG_PARSER,
    CODEC_REQ_TYPE_DAC_BIAS_PARSER,
    CODEC_REQ_TYPE_ADC_ANA_PARSER,
    CODEC_REQ_TYPE_ADC_DIG_PARSER,
    CODEC_REQ_TYPE_I2S_PARSER,
    CODEC_REQ_TYPE_EQ_DAC_PARSER,
    CODEC_REQ_TYPE_EQ_ADC_PARSER,
    CODEC_REQ_TYPE_MICBIAS_PARSER,
    CODEC_REQ_TYPE_HALT_CFG,
    CODEC_REQ_TYPE_SIDETONE_PARSER,

    CODEC_REQ_TYPE_MAX,
} T_CODEC_REQ_TYPE;

typedef enum t_codec_state
{
    CODEC_STATE_OFF,
    CODEC_STATE_MUTE,
    CODEC_STATE_ON,

    CODEC_STATE_MAX,
} T_CODEC_STATE;

typedef enum t_dc_offset_k_phase
{
    DC_OFFSET_K_PHASE_INIT,
    DC_OFFSET_K_PHASE_0,
    DC_OFFSET_K_PHASE_1,
    DC_OFFSET_K_PHASE_2,
    DC_OFFSET_K_PHASE_3,
    DC_OFFSET_K_PHASE_4,
    DC_OFFSET_K_PHASE_5,
    DC_OFFSET_K_PHASE_6,
    DC_OFFSET_K_PHASE_7,
    DC_OFFSET_K_PHASE_8,
    DC_OFFSET_K_PHASE_9,
    DC_OFFSET_K_PHASE_10,
    DC_OFFSET_K_PHASE_11,
    DC_OFFSET_K_PHASE_12,
    DC_OFFSET_K_PHASE_13,
    DC_OFFSET_K_PHASE_DONE,
} T_DC_OFFSET_K_PHASE;

typedef enum t_codec_info
{
    CODEC_INFO_CUR_STATE,
    CODEC_INFO_CFG_STATE,
    CODEC_INFO_CONVERT_SAMPLE_RATE,
    CODEC_INFO_CONVERT_MIC_VOLTAGE,
} T_CODEC_INFO;

typedef enum t_codec_dac_ana_gain
{
    CODEC_DAC_ANA_GAIN_0,       // 0,      0dB
    CODEC_DAC_ANA_GAIN_0P5,     // 1,   -0.5dB
    CODEC_DAC_ANA_GAIN_1,       // 2,     -1dB
    CODEC_DAC_ANA_GAIN_1P5,     // 3,   -1.5dB
    CODEC_DAC_ANA_GAIN_2,       // 4,     -2dB
    CODEC_DAC_ANA_GAIN_2P5,     // 5,   -2.5dB
    CODEC_DAC_ANA_GAIN_3,       // 6,     -3dB
    CODEC_DAC_ANA_GAIN_3P5,     // 7,   -3.5dB
    CODEC_DAC_ANA_GAIN_4,       // 8,     -4dB
    CODEC_DAC_ANA_GAIN_4P5,     // 9,   -4.5dB
    CODEC_DAC_ANA_GAIN_5,       // 10,    -5dB
    CODEC_DAC_ANA_GAIN_5P5,     // 11,  -5.5dB
    CODEC_DAC_ANA_GAIN_6,       // 12,    -6dB
    CODEC_DAC_ANA_GAIN_6P5,     // 13,  -6.5dB
    CODEC_DAC_ANA_GAIN_7,       // 14,    -7dB
    CODEC_DAC_ANA_GAIN_7P5,     // 15,  -7.5dB
    CODEC_DAC_ANA_GAIN_8,       // 16,    -8dB
    CODEC_DAC_ANA_GAIN_8P5,     // 17,  -8.5dB
    CODEC_DAC_ANA_GAIN_9,       // 18,    -9dB
    CODEC_DAC_ANA_GAIN_9P5,     // 19,  -9.5dB
    CODEC_DAC_ANA_GAIN_10,      // 20,   -10dB
    CODEC_DAC_ANA_GAIN_10P5,    // 21, -10.5dB
    CODEC_DAC_ANA_GAIN_11,      // 22,   -11dB
    CODEC_DAC_ANA_GAIN_11P5,    // 23, -11.5dB
    CODEC_DAC_ANA_GAIN_12,      // 24,   -12dB
    CODEC_DAC_ANA_GAIN_12P5,    // 25, -12.5dB
    CODEC_DAC_ANA_GAIN_13,      // 26,   -13dB
    CODEC_DAC_ANA_GAIN_13P5,    // 27, -13.5dB
    CODEC_DAC_ANA_GAIN_14,      // 28,   -14dB
    CODEC_DAC_ANA_GAIN_14P5,    // 29, -14.5dB
    CODEC_DAC_ANA_GAIN_15,      // 30,   -15dB
    CODEC_DAC_ANA_GAIN_15P5,    // 31, -15.5dB
    CODEC_DAC_ANA_GAIN_MAX,
} T_CODEC_DAC_ANA_GAIN;

typedef enum t_codec_adc_ana_gain
{
    CODEC_ADC_ANA_GAIN_0,       // 0dB
    CODEC_ADC_ANA_GAIN_3,       // 3dB
    CODEC_ADC_ANA_GAIN_6,       // 6dB
    CODEC_ADC_ANA_GAIN_9,       // 9dB
    CODEC_ADC_ANA_GAIN_12,      // 12dB
    CODEC_ADC_ANA_GAIN_18,      // 18dB
    CODEC_ADC_ANA_GAIN_24,      // 24dB
    CODEC_ADC_ANA_GAIN_30,      // 30dB
    CODEC_ADC_ANA_GAIN_MAX,
} T_CODEC_ADC_ANA_GAIN;

typedef enum t_codec_adc_boost_gain
{
    CODEC_ADC_BOOST_GAIN_0,     // 0dB
    CODEC_ADC_BOOST_GAIN_12,    // 12dB
    CODEC_ADC_BOOST_GAIN_24,    // 24dB
    CODEC_ADC_BOOST_GAIN_36,    // 36dB
    CODEC_ADC_BOOST_GAIN_MAX,
} T_CODEC_ADC_BOOST_GAIN;

typedef enum t_codec_sample_rate
{
    CODEC_SAMPLE_RATE_48K,      // 0: 48KHz
    CODEC_SAMPLE_RATE_96K,      // 1: 96KHz
    CODEC_SAMPLE_RATE_192K,     // 2: 192KHz
    CODEC_SAMPLE_RATE_32K,      // 3: 32KHz
    CODEC_SAMPLE_RATE_176K,     // 4: 176.4KHz
    CODEC_SAMPLE_RATE_16K,      // 5: 16KHz
    CODEC_SAMPLE_RATE_RSV_1,
    CODEC_SAMPLE_RATE_8K,       // 7: 8KHz
    CODEC_SAMPLE_RATE_44_1K,    // 8: 44.1KHz
    CODEC_SAMPLE_RATE_88_2K,    // 9: 88.2KHz
    CODEC_SAMPLE_RATE_24K,      // 10: 24KHz
    CODEC_SAMPLE_RATE_12K,      // 11: 12KHz
    CODEC_SAMPLE_RATE_22_05K,   // 12: 22.05KHz
    CODEC_SAMPLE_RATE_11_025K,  // 13: 11.025KHz
    CODEC_SAMPLE_RATE_NUM,
} T_CODEC_SAMPLE_RATE;

typedef enum t_codec_dmic_clk_sel
{
    DMIC_CLK_5MHZ,
    DMIC_CLK_2P5MHZ,
    DMIC_CLK_1P25MHZ,
    DMIC_CLK_625kHZ,
    DMIC_CLK_312P5kHZ,
    DMIC_CLK_RSV_1,
    DMIC_CLK_RSV_2,
    DMIC_CLK_769P2kHZ,
    DMIC_CLK_SEL_MAX,
} T_CODEC_DMIC_CLK_SEL;

typedef enum t_codec_ad_src_sel
{
    AD_SRC_DMIC,
    AD_SRC_AMIC,
    AD_SRC_MAX,
} T_CODEC_AD_SRC_SEL;

typedef enum t_codec_ad_dchpf_sel
{
    ADC_DCHPF_1EN2,
    ADC_DCHPF_5EN3,
    ADC_DCHPF_2P5EN3,
    ADC_DCHPF_1P25EN3,
    ADC_DCHPF_6P25EN4,
    ADC_DCHPF_3P125EN4,
    ADC_DCHPF_1P5625EN4,
    ADC_DCHPF_7P8125EN5
} T_CODEC_AD_DCHPF_SEL;

typedef enum t_codec_i2s_channel_sel
{
    I2S_CHANNEL_0,
    I2S_CHANNEL_1,
    I2S_CHANNEL_2,      // Rx only
    I2S_CHANNEL_MAX,
} T_CODEC_I2S_CHANNEL_SEL;

typedef enum t_codec_adc_channel_sel
{
    ADC_CHANNEL0,
    ADC_CHANNEL1,
    ADC_CHANNEL2,
    ADC_CHANNEL3,
    ADC_CHANNEL4,
    ADC_CHANNEL5,
    ADC_CHANNEL_MAX,
} T_CODEC_ADC_CHANNEL_SEL;

typedef enum t_codec_dac_channel_sel
{
    DAC_CHANNEL_L,
    DAC_CHANNEL_R,
    DAC2_CHANNEL,
    DAC_CHANNEL_MAX,
} T_CODEC_DAC_CHANNEL_SEL;

typedef enum t_codec_dmic_channel_sel
{
    DMIC_CHANNEL1,
    DMIC_CHANNEL2,
    DMIC_CHANNEL3,
    DMIC_CHANNEL_MAX,
} T_CODEC_DMIC_CHANNEL_SEL;

typedef enum t_codec_amic_channel_sel
{
    AMIC_CHANNEL1,
    AMIC_CHANNEL2,
    AMIC_CHANNEL3,
    AMIC_CHANNEL4,
    AMIC_CHANNEL_MAX,
} T_CODEC_AMIC_CHANNEL_SEL;

typedef enum t_codec_sidetone_channel_sel
{
    SIDETONE_CHANNEL_L,
    SIDETONE_CHANNEL_R,
    SIDETONE_CHANNEL_MAX,
} T_CODEC_SIDETONE_CHANNEL_SEL;

typedef enum t_codec_dmic_type_sel
{
    DMIC_TYPE_FALLING,
    DMIC_TYPE_RAISING,
    DMIC_TYPE_MAX,
} T_CODEC_DMIC_TYPE_SEL;

typedef enum t_codec_amic_type_sel
{
    AMIC_TYPE_SINGLE,
    AMIC_TYPE_DIFFERENTIAL,
    AMIC_TYPE_MAX,
} T_CODEC_AMIC_TYPE_SEL;

typedef enum t_codec_spk_type_sel
{
    SPK_TYPE_SINGLE_END,
    SPK_TYPE_DIFFERENTIAL,
    SPK_TYPE_CAPLESS,
    SPK_TYPE_MAX,
} T_CODEC_SPK_TYPE_SEL;

typedef enum t_codec_anc_channel_sel
{
    ANC_CHANNEL_EXT_L,
    ANC_CHANNEL_EXT_R,
    ANC_CHANNEL_INT_L,
    ANC_CHANNEL_INT_R,
    ANC_CHANNEL_MAX,
} T_CODEC_ANC_CHANNEL_SEL;

typedef enum t_codec_i2s_datalen_sel
{
    I2S_DATALEN_16,
    I2S_DATALEN_32,
    I2S_DATALEN_24,
    I2S_DATALEN_8,
    I2S_DATALEN_MAX,
} T_CODEC_I2S_DATALEN_SEL;

typedef enum t_codec_i2s_ch_len_sel
{
    CODEC_I2S_CH_LEN_16,
    CODEC_I2S_CH_LEN_32,
    CODEC_I2S_CH_LEN_24,
    CODEC_I2S_CH_LEN_8,
    CODEC_I2S_CH_LEN_MAX,
} T_CODEC_I2S_CH_LEN_SEL;

typedef enum t_codec_i2s_tx_ch_sel
{
    I2S_TX_CH_SEL_L_R,
    I2S_TX_CH_SEL_R_L,
    I2S_TX_CH_SEL_L_L,
    I2S_TX_CH_SEL_R_R,
    I2S_TX_CH_SEL_MAX,
} T_CODEC_I2S_TX_CH_SEL;

typedef enum t_codec_i2s_rx_ch_sel
{
    I2S_RX_CH_SEL_ADC0,
    I2S_RX_CH_SEL_ADC1,
    I2S_RX_CH_SEL_ADC2,
    I2S_RX_CH_SEL_ADC3,
    I2S_RX_CH_SEL_ADC4,
    I2S_RX_CH_SEL_ADC5,
    I2S_RX_CH_SEL_ADC6,
    I2S_RX_CH_SEL_ADC7,
    I2S_RX_CH_SEL_MAX,
} T_CODEC_I2S_RX_CH_SEL;

typedef enum t_codec_i2s_rx_ch
{
    I2S_RX_CH_0,
    I2S_RX_CH_1,
    I2S_RX_CH_2,
    I2S_RX_CH_3,
    I2S_RX_CH_4,
    I2S_RX_CH_5,
    I2S_RX_CH_6,
    I2S_RX_CH_7,
    I2S_RX_CH_MAX,
} T_CODEC_I2S_RX_CH;

typedef enum t_codec_input_dev
{
    INPUT_DEV_NONE,
    INPUT_DEV_MIC,
    INPUT_DEV_AUX,
    INPUT_DEV_MAX,
} T_CODEC_INPUT_DEV;

typedef enum t_codec_micbias_level
{
    MICBIAS_LEVEL_1P05V,
    MICBIAS_LEVEL_1P00V,
    MICBIAS_LEVEL_0P90V,
    MICBIAS_LEVEL_VCM,
    MICBIAS_LEVEL_NUM,
} T_CODEC_MICBIAS_LEVEL;

typedef enum t_codec_micbias_vol
{
    MICBIAS_VOLTAGE_1P3V,
    MICBIAS_VOLTAGE_1P4V,
    MICBIAS_VOLTAGE_1P5V,
    MICBIAS_VOLTAGE_1P6V,
    MICBIAS_VOLTAGE_1P7V,
    MICBIAS_VOLTAGE_1P8V,
    MICBIAS_VOLTAGE_1P9V,
    MICBIAS_VOLTAGE_2P0V,
    MICBIAS_VOLTAGE_NUM,
} T_CODEC_MICBIAS_VOL;

typedef enum t_codec_micbias_vol_option
{
    MICBIAS_VOL_OPTION_1P98V,
    MICBIAS_VOL_OPTION_1P8V,
    MICBIAS_VOL_OPTION_1P62V,
    MICBIAS_VOL_OPTION_1P44V,
    MICBIAS_VOL_OPTION_NUM,
} T_CODEC_MICBIAS_VOL_OPTION;

typedef enum t_codec_sidetone_fading_status
{
    CODEC_SIDETONE_FADING_STATUS_INIT,
    CODEC_SIDETONE_FADING_STATUS_FADE_IN,
    CODEC_SIDETONE_FADING_STATUS_FADE_OUT,
    CODEC_SIDETONE_FADING_STATUS_NONE,
} T_CODEC_SIDETONE_FADING_STATUS;

typedef enum t_codec_sidetone_type
{
    CODEC_SIDETONE_TYPE_5M,
    CODEC_SIDETONE_TYPE_DOWNSAMPLE,
    CODEC_SIDETONE_TYPE_MAX,
} T_CODEC_SIDETONE_TYPE;

typedef enum t_codec_sidetone_boost_gain
{
    CODEC_SIDETONE_BOOST_GAIN_0,    // 0dB
    CODEC_SIDETONE_BOOST_GAIN_12,   // 12dB
    CODEC_SIDETONE_BOOST_GAIN_MAX,
} T_CODEC_SIDETONE_BOOST_GAIN;

typedef enum t_codec_sidetone_mic_src
{
    CODEC_SIDETONE_MIC_SRC_PRIMARY,
    CODEC_SIDETONE_MIC_SRC_AUXILIARY,
    CODEC_SIDETONE_MIC_SRC_MAX,
} T_CODEC_SIDETONE_MIC_SRC;

typedef enum t_codec_sidetone_input
{
    CODEC_SIDETONE_INPUT_L,
    CODEC_SIDETONE_INPUT_R,
    CODEC_SIDETONE_INPUT_L_R_AVG,
    CODEC_SIDETONE_INPUT_MAX,
} T_CODEC_SIDETONE_INPUT;

typedef enum t_codec_config_sel
{
    CODEC_CONFIG_SEL_BIAS,
    CODEC_CONFIG_SEL_SIDETONE,
    CODEC_CONFIG_SEL_SPK,
    CODEC_CONFIG_SEL_DAC,
    CODEC_CONFIG_SEL_DAC_BIAS,
    CODEC_CONFIG_SEL_ADC,
    CODEC_CONFIG_SEL_I2S,
    CODEC_CONFIG_SEL_ANC,
    CODEC_CONFIG_SEL_MAX,
} T_CODEC_CONFIG_SEL;

typedef enum t_codec_offset_direction
{
    CODEC_DAC_DC_DIR_RSVD,
    CODEC_DAC_DC_DIR_NEG,
    CODEC_DAC_DC_DIR_POS,
    CODEC_DAC_DC_DIR_RSVD2,
} T_CODEC_DAC_DC_DIRECTION;

typedef enum t_codec_dc_k_mode
{
    CODEC_DC_K_MODE_DISABLE,
    CODEC_DC_K_MODE_DMA,            // BB2 invalid
    CODEC_DC_K_MODE_JUST_ONCE,      // BB2 invalid
    CODEC_DC_K_MODE_EFUSE,
    CODEC_DC_K_MODE_MAX,
} T_CODEC_DC_K_MODE;

typedef enum t_sport_tdm_mode_sel
{
    TDM_2_CHANNEL,
    TDM_4_CHANNEL,
    TDM_6_CHANNEL,
    TDM_8_CHANNEL,
    TDM_CHANNEL_MAX,
} T_SPORT_TDM_MODE_SEL;

typedef enum t_sport_data_format_sel
{
    FORMAT_I2S,
    FORMAT_LEFT_JUSTIFY,
    FORMAT_PCM_MODE_A,
    FORMAT_PCM_MODE_B,
    FORMAT_MAX,
} T_SPORT_DATA_FORMAT_SEL;

typedef enum t_codec_eq_group_sel
{
    CODEC_EQ_GROUP_DAC_L,
    CODEC_EQ_GROUP_DAC_R,
    CODEC_EQ_GROUP_DAC2,
    CODEC_EQ_GROUP_ADC_0,
    CODEC_EQ_GROUP_ADC_1,
    CODEC_EQ_GROUP_ADC_2,
    CODEC_EQ_GROUP_ADC_3,
    CODEC_EQ_GROUP_ADC_4,
    CODEC_EQ_GROUP_ADC_5,
    CODEC_EQ_GROUP_ADC_6,
    CODEC_EQ_GROUP_ADC_7,
    CODEC_EQ_GROUP_SIDETONE,
    CODEC_EQ_GROUP_SEL_MAX,
} T_CODEC_EQ_GROUP_SEL;

typedef enum t_codec_eq_config_path
{
    CODEC_EQ_CONFIG_PATH_ADC = 1,
    CODEC_EQ_CONFIG_PATH_DAC,
    CODEC_EQ_CONFIG_PATH_SIDETONE,
    CODEC_EQ_CONFIG_PATH_MAX,
} T_CODEC_EQ_CONFIG_PATH;

typedef enum t_codec_io_path
{
    CODEC_IO_PATH_ADC0,
    CODEC_IO_PATH_ADC1,
    CODEC_IO_PATH_ADC2,
    CODEC_IO_PATH_ADC3,
    CODEC_IO_PATH_ADC4,
    CODEC_IO_PATH_ADC5,
    CODEC_IO_PATH_DAC,
    CODEC_IO_PATH_DAC_L = CODEC_IO_PATH_DAC,
    CODEC_IO_PATH_DAC_R,
} T_CODEC_IO_PATH;

typedef enum t_codec_hw_eq_type
{
    CODEC_SPK_HW_EQ = 0,
    CODEC_MIC_HW_EQ = 1,
} T_CODEC_HW_EQ_TYPE;

typedef enum t_codec_gain_config_path
{
    CODEC_GAIN_CONFIG_PATH_DAC = 0,
    CODEC_GAIN_CONFIG_PATH_ADC = 1,
} T_CODEC_GAIN_CONFIG_PATH;

typedef enum t_codec_tri_csel_cp
{
    CODEC_TRI_CSEL_CP_1P50,
    CODEC_TRI_CSEL_CP_1P75,
    CODEC_TRI_CSEL_CP_2P00,
    CODEC_TRI_CSEL_CP_2P25,
    CODEC_TRI_CSEL_CP_MAX,
} T_CODEC_TRI_CSEL_CP;

typedef enum t_codec_downlink_mix
{
    CODEC_DOWNLINK_MIX_NONE = 0x0,
    CODEC_DOWNLINK_MIX_L_R = 0x1,       // mix Rch to Lch
    //CODEC_DOWNLINK_MIX_L_2 = 0x2,     // mix 2ch to Lch
    //CODEC_DOWNLINK_MIX_L_R_2 = 0x3,   // mix Rch, 2ch to Lch
    //CODEC_DOWNLINK_MIX_R_2 = 0x4,     // mix 2ch to Lch
    CODEC_DOWNLINK_MIX_R_L = 0x8,       // mix Lch to Rch
    //CODEC_DOWNLINK_MIX_R_L_2 = 0xC,   // mix Lch,2ch to Rch
} T_CODEC_DOWNLINK_MIX;

typedef enum t_codec_amp_r2i_sel
{
    CODEC_AMP_R2I_SEL_26K,
    CODEC_AMP_R2I_SEL_42K,
    CODEC_AMP_R2I_SEL_60K,
    CODEC_AMP_R2I_SEL_78K,
    CODEC_AMP_R2I_SEL_MAX,
} T_CODEC_AMP_R2I_SEL;

typedef enum t_codec_bias_current_sel
{
    CODEC_BIAS_CURRENT_SEL_0P5,     // uA
    CODEC_BIAS_CURRENT_SEL_0P75,
    CODEC_BIAS_CURRENT_SEL_1,
    CODEC_BIAS_CURRENT_SEL_1P25,
    CODEC_BIAS_CURRENT_SEL_1P5,
    CODEC_BIAS_CURRENT_SEL_1P75,
    CODEC_BIAS_CURRENT_SEL_2,
    CODEC_BIAS_CURRENT_SEL_2P25,
    CODEC_BIAS_CURRENT_SEL_MAX,
} T_CODEC_BIAS_CURRENT_SEL;

// for c-cut ECO
typedef enum t_codec_int_bias_current_sel
{
    CODEC_INT_BIAS_CURRENT_SEL_1,   // uA
    CODEC_INT_BIAS_CURRENT_SEL_1P5,
    CODEC_INT_BIAS_CURRENT_SEL_2,
    CODEC_INT_BIAS_CURRENT_SEL_2P5,
    CODEC_INT_BIAS_CURRENT_SEL_3,
    CODEC_INT_BIAS_CURRENT_SEL_3P5,
    CODEC_INT_BIAS_CURRENT_SEL_4,
    CODEC_INT_BIAS_CURRENT_SEL_4P5,
    CODEC_INT_BIAS_CURRENT_SEL_MAX,
} T_CODEC_INT_BIAS_CURRENT_SEL;

// for c-cut ECO
typedef enum t_codec_sdm_bias_current_sel
{
    CODEC_SDM_BIAS_CURRENT_SEL_2,   // uA
    CODEC_SDM_BIAS_CURRENT_SEL_3,
    CODEC_SDM_BIAS_CURRENT_SEL_4,
    CODEC_SDM_BIAS_CURRENT_SEL_5,
    CODEC_SDM_BIAS_CURRENT_SEL_6,
    CODEC_SDM_BIAS_CURRENT_SEL_7,
    CODEC_SDM_BIAS_CURRENT_SEL_8,
    CODEC_SDM_BIAS_CURRENT_SEL_9,
    CODEC_SDM_BIAS_CURRENT_SEL_MAX,
} T_CODEC_SDM_BIAS_CURRENT_SEL;

// for c-cut ECO
typedef enum t_codec_micbst_bias_current_sel
{
    CODEC_MICBST_BIAS_CURRENT_SEL_1,    // uA
    CODEC_MICBST_BIAS_CURRENT_SEL_1P5,
    CODEC_MICBST_BIAS_CURRENT_SEL_2,
    CODEC_MICBST_BIAS_CURRENT_SEL_2P5,
    CODEC_MICBST_BIAS_CURRENT_SEL_3,
    CODEC_MICBST_BIAS_CURRENT_SEL_3P5,
    CODEC_MICBST_BIAS_CURRENT_SEL_4,
    CODEC_MICBST_BIAS_CURRENT_SEL_4P5,
    CODEC_MICBST_BIAS_CURRENT_SEL_MAX,
} T_CODEC_MICBST_BIAS_CURRENT_SEL;

typedef struct
{
    uint8_t     msg_type;
    uint16_t    data_len;
    void        *p_data;
} T_CODEC_TO_APP_MSG;

typedef union t_codec_feature_map
{
    uint32_t d32;
    struct
    {
        uint32_t depop_en: 1;               // [0]
        uint32_t dc_k_mode: 2;            // [2:1] T_CODEC_DC_K_MODE
        uint32_t pwr_saving_ctl: 1;       // [4]
        uint32_t rsvd: 27;
    };
} T_CODEC_FEATURE_MAP;

typedef union t_codec_sidetone_ctrl
{
    uint8_t d8[9];
    struct
    {
        uint8_t fading_status;
        uint16_t sidetone_offset[SIDETONE_CHANNEL_MAX];
        void *timer_handle;
    };
} T_CODEC_SIDETONE_CTRL;

typedef union t_codec_amic_config
{
    uint8_t d8[5];
    struct __attribute__((packed))
    {
        uint8_t enable;
        T_CODEC_AMIC_CHANNEL_SEL ch_sel;
        uint8_t mic_type;                   // 0: single-end, 1: differential
        T_CODEC_ADC_ANA_GAIN ana_gain;
        T_CODEC_INPUT_DEV input_dev;
    };
} T_CODEC_AMIC_CONFIG;

typedef union t_codec_dmic_config
{
    uint8_t d8[5];
    struct __attribute__((packed))
    {
        uint8_t enable;
        T_CODEC_DMIC_CHANNEL_SEL ch_sel;
        uint8_t mic_type;                   // 0: raising, 1: falling
        T_CODEC_DMIC_CLK_SEL dmic_clk_sel;
        uint8_t rsvd;                       // align with T_CODEC_AMIC_CONFIG
    };
} T_CODEC_DMIC_CONFIG;

typedef union t_codec_sidetone_dig_gain
{
    uint8_t d8[2];
    uint16_t val;
} T_CODEC_SIDETONE_DIG_GAIN;

typedef union t_codec_spk_config
{
    uint8_t d8[2];
    struct __attribute__((packed))
    {
        uint8_t spk_type;                   // 0: Single end, 1: Differential, 2: Capless, deprecated
        uint8_t spk_asrc_en;
    };
} T_CODEC_SPK_CONFIG;

typedef union t_codec_dac_config
{
    uint8_t d8[9];
    struct __attribute__((packed))
    {
        uint8_t power_en;
        T_CODEC_DAC_ANA_GAIN ana_gain;      // deprecated, control by HW DRE
        uint8_t dig_gain;
        uint8_t music_mute_en;
        uint8_t anc_mute_en;
        uint8_t equalizer_en;
        T_CODEC_I2S_CHANNEL_SEL i2s_sel;
        T_CODEC_SAMPLE_RATE sample_rate;
        T_CODEC_DOWNLINK_MIX downlink_mix;
    };
} T_CODEC_DAC_CONFIG;

typedef union t_codec_dac_bias_config
{
    uint8_t d8[2];
    struct __attribute__((packed))
    {
        uint8_t classd_bias;
        uint8_t dac_bias;
    };
} T_CODEC_DAC_BIAS_CONFIG;

typedef union t_codec_adc_config
{
    uint8_t d8[15];
    struct __attribute__((packed))
    {
        uint8_t asrc_en;
        uint8_t dig_gain;
        T_CODEC_ADC_BOOST_GAIN boost_gain;
        T_CODEC_SAMPLE_RATE sample_rate;
        uint8_t loopback;                   // ANC decimation path
        uint8_t equalizer_en;
        uint8_t ad_dchpf_sel;
        //-- following config also used on ANC config
        T_CODEC_I2S_CHANNEL_SEL i2s_sel;
        T_CODEC_AD_SRC_SEL ad_src_sel;

        union
        {
            T_CODEC_AMIC_CONFIG amic;
            T_CODEC_DMIC_CONFIG dmic;
        };
    };
} T_CODEC_ADC_CONFIG;

typedef union t_codec_i2s_config
{
    uint8_t d8[24];
    struct __attribute__((packed))
    {
        T_SPORT_TDM_MODE_SEL rx_tdm_mode;
        T_SPORT_DATA_FORMAT_SEL tx_data_format;
        T_SPORT_DATA_FORMAT_SEL rx_data_format;
        T_CODEC_I2S_DATALEN_SEL tx_data_len;
        T_CODEC_I2S_DATALEN_SEL rx_data_len;
        T_CODEC_I2S_CH_LEN_SEL tx_channel_len;
        T_CODEC_I2S_CH_LEN_SEL rx_channel_len;
        T_CODEC_I2S_TX_CH_SEL tx_channel_sel;
        uint8_t rx_data_ch_en[I2S_RX_CH_MAX];
        T_CODEC_I2S_RX_CH_SEL rx_data_ch_sel[I2S_RX_CH_MAX];

        // open other setting if needed
    };
} T_CODEC_I2S_CONFIG;

typedef union t_codec_bias_config
{
    uint8_t d8[4];
    struct __attribute__((packed))
    {
        uint8_t gpio_mode;
        T_CODEC_MICBIAS_LEVEL micbias_level;
        T_CODEC_MICBIAS_VOL micbias_voltage;
        uint8_t micbias_state;
    };
} T_CODEC_BIAS_CONFIG;

typedef union t_codec_sidetone_config
{
    uint8_t d8[10];
    struct __attribute__((packed))
    {
        T_CODEC_SIDETONE_DIG_GAIN dig_gain;
        //-- special case for above config, since 0xFF is valid
        uint8_t enable;
        T_CODEC_SIDETONE_BOOST_GAIN boost_gain;
        T_CODEC_ADC_CHANNEL_SEL src;
        T_CODEC_SIDETONE_INPUT input;
        T_CODEC_SIDETONE_TYPE type;
        uint8_t eq_en;
        uint8_t hpf_en;
        uint8_t hpf_fc_sel;
    };
} T_CODEC_SIDETONE_CONFIG;

typedef union t_codec_eq_band_param
{
    uint32_t d32[5];
    struct
    {
        uint32_t h0: 29;
        uint32_t rsvd0: 3;
        uint32_t b1: 29;
        uint32_t rsvd1: 3;
        uint32_t b2: 29;
        uint32_t rsvd2: 3;
        uint32_t a1: 29;
        uint32_t rsvd3: 3;
        uint32_t a2: 29;
        uint32_t stage_en: 1;
        uint32_t rsvd4: 2;
    };
} T_CODEC_EQ_BAND_PARAM;

typedef union t_codec_gain_data
{
    uint16_t d16;
    struct
    {
        uint16_t dig_gain: 8;
        uint16_t dig_boost_gain: 4;
        uint16_t analog_gain: 4;
    };
} T_CODEC_GAIN_DATA;

typedef struct t_codec_eq_cmd_data
{
    uint16_t opcode;
    uint16_t rsvd0;
    uint16_t param_len; // codec biquad param len
    uint16_t rsvd1; // dsp biquad param len
    uint32_t *biquad_param;
} T_CODEC_EQ_CMD_DATA;

typedef struct t_codec_gain_cmd_data
{
    uint16_t opcode;
    uint16_t param_len;
    uint16_t gain_param;
} T_CODEC_GAIN_CMD_DATA;

/**
 * codec_config.h
 *
 * \brief   Define codec ADC decimation selection.
 *
 * \ingroup CODEC_CONFIG
 */
typedef enum t_codec_adc_deci_sel
{
    CODEC_ADC_DECI_SEL_NONE,                // compatible with BBpro2/BBLite
    CODEC_ADC_DECI_SEL_AMIC,
    CODEC_ADC_DECI_SEL_ANC,
    CODEC_ADC_DECI_SEL_MUSIC,
    CODEC_ADC_DECI_SEL_ANC_MUSIC,
    CODEC_ADC_DECI_SEL_MAX,
} T_CODEC_ADC_DECI_SEL;


/* Define */
typedef bool (*P_CODEC_CBACK)(T_CODEC_CB_EVENT event, uint32_t param);
typedef void (*P_CODEC_REQ_CBACK)(uint32_t param);

#define CODEC_CFG_IGNORE                0xFF
#define CODEC_CFG_IGNORE_U16            0xFFFF
#define CODEC_CFG_VALID(config)         (config != CODEC_CFG_IGNORE)

#define CODEC_MICBIAS_GPIO_MODE(micbias_pinmux)  \
    ((micbias_pinmux != MICBIAS) && (micbias_pinmux != CODEC_CFG_IGNORE))
// MICBIAS level force to set 0.9V, 2.0V * 0.9V = 1.8V
#define CODEC_MICBIAS_GPIO_VOL(micbias_vol)  \
    (micbias_vol == MICBIAS_VOL_OPTION_1P8V)


#define CODEC_DRV_GAIN_STEP             0.375
#define CODEC_DRV_DAC_GAIN_DEFAULT      0xAF
#define CODEC_DRV_ADC_GAIN_DEFAULT      0x2F
#define CODEC_DRV_DAC_DIG_GAIN_MAX_HEX  0xAF        // 0 dB
#define CODEC_DRV_DAC_DIG_GAIN_MIN_HEX  0x00        // -65.625 dB
#define CODEC_DRV_ADC_DIG_GAIN_MAX_HEX  0x7F        // 30 dB
#define CODEC_DRV_ADC_DIG_GAIN_MIN_HEX  0x00        // -17.625 dB

#define CODEC_SIDETONE_BOOST_GAIN_DB            12.0f
#define CODEC_SIDETONE_5M_DIG_GAIN_MAX_DB       30.0f
#define CODEC_SIDETONE_DS_DIG_GAIN_MAX_DB       6.0f
#define CODEC_SIDETONE_5M_DIG_GAIN_MAX_HEX      0xFF        // 30 dB
#define CODEC_SIDETONE_5M_DIG_GAIN_MIN_HEX      0x00        // -65.625 dB
#define CODEC_SIDETONE_DS_DIG_GAIN_MAX_HEX      0xBF        // 6 dB
#define CODEC_SIDETONE_DIG_GAIN_MIN_HEX         0x00        // -65.625 dB
#define CODEC_SIDETONE_GAIN_STEP_DB             0.375f      // 0.375 dB/step
#define CODEC_SIDETONE_GAIN_TABLE_CONSTANT      128
#define CODEC_SIDETONE_DVOL_OUT_SRC             0x08
#define CODEC_SIDETONE_HFP_FC_MAX               7
#define CODEC_SIDETONE_SRC_SEL_DMIC_OFFSET      8
#define CODEC_SIDETONE_FADING_TIMEOUT           100         // 100 ms
#define CODEC_SIDETONE_CFG_COPY                 2

// MIC select
#define MIC_SEL_DMIC_1                  0x00
#define MIC_SEL_DMIC_2                  0x01
#define MIC_SEL_AMIC_1                  0x02
#define MIC_SEL_AMIC_2                  0x03
#define MIC_SEL_AMIC_3                  0x04
#define MIC_SEL_AMIC_4                  0x05
#define MIC_SEL_DMIC_3                  0x06
#define MIC_SEL_DISABLE                 0x07

// AMIC types
#define MIC_TYPE_SINGLE_END_AMIC        0x00
#define MIC_TYPE_DIFFERENTIAL_AMIC      0x01

// DMIC types
#define MIC_TYPE_FALLING_DMIC           0x00
#define MIC_TYPE_RAISING_DMIC           0x01

// Basic function
bool codec_drv_init(T_CODEC_FEATURE_MAP feature_map, P_CODEC_CBACK cback);
bool codec_drv_deinit(void);
uint8_t codec_drv_power_on(void);
uint8_t codec_drv_power_off(void);
uint8_t codec_drv_enable(void);
uint8_t codec_drv_disable(void);
uint8_t codec_drv_set_mute(void);
void codec_drv_get_info(void *buf, T_CODEC_INFO info_id);
uint32_t codec_msg_handler(void);

// Config API
void codec_drv_config_default(T_CODEC_CONFIG_SEL config_sel, void *config_ptr);
void codec_drv_config_init(T_CODEC_CONFIG_SEL config_sel, void *config_ptr);
uint8_t codec_drv_bias_config_set(T_CODEC_BIAS_CONFIG *bias_config, bool set_immediately);
uint8_t codec_drv_sidetone_config_set(T_CODEC_SIDETONE_CHANNEL_SEL ch_sel,
                                      T_CODEC_SIDETONE_CONFIG *sidetone_config,
                                      bool set_immediately);
uint8_t codec_drv_spk_config_set(T_CODEC_SPK_CONFIG *spk_config, bool set_immediately);
uint8_t codec_drv_dac_config_set(T_CODEC_DAC_CHANNEL_SEL ch_sel,
                                 T_CODEC_DAC_CONFIG *dac_config,
                                 bool set_immediately);
uint8_t codec_drv_dac_bias_config_set(T_CODEC_DAC_BIAS_CONFIG *spk_config, bool set_immediately);
uint8_t codec_drv_adc_config_set(T_CODEC_ADC_CHANNEL_SEL ch_sel,
                                 T_CODEC_ADC_CONFIG *adc_config,
                                 bool set_immediately);
uint8_t codec_drv_i2s_config_set(T_CODEC_I2S_CHANNEL_SEL ch_sel,
                                 T_CODEC_I2S_CONFIG *i2s_config,
                                 bool set_immediately);
uint8_t codec_drv_dmic_config_set(T_CODEC_ADC_CHANNEL_SEL ch_sel,
                                  uint8_t mic_en,
                                  uint32_t mcu_config_mic_sel,
                                  T_CODEC_DMIC_TYPE_SEL dmic_type,
                                  T_CODEC_DMIC_CLK_SEL dmic_clock,
                                  bool set_immediately);
uint8_t codec_drv_amic_config_set(T_CODEC_ADC_CHANNEL_SEL ch_sel,
                                  uint8_t mic_en,
                                  uint32_t mcu_config_mic_sel,
                                  T_CODEC_AMIC_TYPE_SEL amic_type,
                                  T_CODEC_ADC_ANA_GAIN ana_gain,
                                  bool set_immediately);
uint8_t codec_drv_anc_mic_config_set(T_CODEC_ANC_CHANNEL_SEL ch_sel,
                                     uint8_t mic_en,
                                     uint32_t mcu_config_mic_sel,
                                     uint8_t mic_type,
                                     T_CODEC_DMIC_CLK_SEL clk_sel,
                                     T_CODEC_ADC_ANA_GAIN ana_gain,
                                     bool set_immediately);
uint8_t codec_drv_aux_config_set(T_CODEC_ADC_CHANNEL_SEL ch_sel,
                                 uint8_t mic_en,
                                 uint32_t mcu_config_mic_sel,
                                 T_CODEC_ADC_ANA_GAIN ana_gain,
                                 T_CODEC_SAMPLE_RATE sample_rate,
                                 bool set_immediately);
uint8_t codec_drv_bias_config_clear(bool set_immediately);
uint8_t codec_drv_sidetone_config_clear(T_CODEC_SIDETONE_CHANNEL_SEL ch_sel, bool set_immediately);
uint8_t codec_drv_spk_config_clear(bool is_set_immediately);
uint8_t codec_drv_dac_config_clear(T_CODEC_DAC_CHANNEL_SEL ch_sel, bool set_immediately);
uint8_t codec_drv_adc_config_clear(T_CODEC_ADC_CHANNEL_SEL ch_sel, bool set_immediately);
uint8_t codec_drv_i2s_config_clear(T_CODEC_I2S_CHANNEL_SEL i2s_sel, bool set_immediately);
uint8_t codec_drv_dmic_config_clear(T_CODEC_ADC_CHANNEL_SEL ch_sel, bool set_immediately);
uint8_t codec_drv_amic_config_clear(T_CODEC_ADC_CHANNEL_SEL ch_sel, bool set_immediately);
uint8_t codec_drv_anc_mic_config_clear(T_CODEC_ANC_CHANNEL_SEL ch_sel, bool set_immediately);
uint8_t codec_drv_get_mic_sel(uint8_t mcu_config_mic_sel, uint8_t *mic_src);


// Run time control
void codec_drv_mbias_pow_enable(uint8_t enable);
void codec_drv_micbias_pow_enable(uint8_t enable);
void codec_drv_adda_loopback_enable(uint8_t enable);

// codec probe function
void codec_drv_probe_dump_codec_regs(void);
void codec_drv_probe_write_codec_regs(uint16_t offset, uint32_t value);

// codec eq biquad function
void codec_drv_eq_group_clear(T_CODEC_EQ_CONFIG_PATH path);
bool codec_drv_eq_data_handle(uint8_t *buf, uint8_t config_path);

// codec sidetone function
void codec_drv_sidetone_enable(void);
void codec_drv_sidetone_disable(void);
void codec_drv_sidetone_gain_convert(T_CODEC_SIDETONE_TYPE type, int16_t gain,
                                     uint8_t *boost_gain, uint8_t *dig_gain);

// codec gain set function
void codec_drv_adc_gain_data_handle(T_CODEC_GAIN_DATA gain_data);

#ifdef __cplusplus
}
#endif

#endif  // _CODEC_DRIVER_H_

