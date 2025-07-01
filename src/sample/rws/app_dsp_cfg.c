/*
 * Copyright (c) 2018, Realsil Semiconductor Corporation. All rights reserved.
 */
#include <string.h>
#include <stdlib.h>
#include "string.h"
#include "os_mem.h"
#include "trace.h"
#include "eq_utils.h"
#include "patch_header_check.h"
#include "fmc_api.h"
#include "app_dsp_cfg.h"
#include "app_cfg.h"
#include "app_audio_passthrough.h"
#include "bt_types.h"
#include "app_audio_passthrough_brightness.h"

#if F_APP_LINEIN_SUPPORT
#define LINE_IN_EQ_DEFAULT_APPLICATIONS_NUM 3
#endif

typedef enum
{
    AUDIO_SCENARIO   = 0,
    VOICE_SCENARIO   = 1,
    LINEIN_SCENARIO  = 2,
    IDLE_SCENARIO    = 3,
} T_APP_SCENARIO;

T_APP_DSP_CFG_DATA *app_dsp_cfg_data;
#if (F_APP_SIDETONE_SUPPORT == 1)
T_SIDETONE_CFG sidetone_cfg;
#endif

uint8_t app_dsp_cfg_get_tool_info_version(void)
{
    uint8_t ret = 0;

    switch (app_dsp_cfg_data->dsp_cfg_header.reserved2)
    {
    case 0x0000:
    case 0xFFFF:
    case 0x0001:
        ret = DSP_CONFIG_TOOL_DEF_VER_0;
        break;

    case 0x0002:
        ret = DSP_CONFIG_TOOL_DEF_VER_1;
        break;

    default:
        ret = DSP_CONFIG_TOOL_DEF_VER_2;
        break;
    }
    APP_PRINT_TRACE1("app_dsp_cfg_get_tool_info_version: version %d", ret);

    return ret;
}

void app_dsp_cfg_load_param_r_data(void *p_data, uint16_t offset, uint16_t size)
{
    fmc_flash_nor_read((flash_cur_bank_img_payload_addr_get(FLASH_IMG_DSPDATA) +
                        DSP_PARAM_OFFSET + offset),
                       (uint8_t *)p_data, size);
}

static void app_dsp_cfg_load_param_r_header(T_APP_DSP_PARAM_R_ONLY *param)
{
    fmc_flash_nor_read((flash_cur_bank_img_payload_addr_get(FLASH_IMG_DSPDATA) +
                        DSP_PARAM_OFFSET),
                       (uint8_t *)param, sizeof(T_APP_DSP_PARAM_R_ONLY));
}

static void app_dsp_cfg_load_header(void)
{
    app_dsp_cfg_data = calloc(1, sizeof(T_APP_DSP_CFG_DATA));
    if (app_dsp_cfg_data == NULL)
    {
        return;
    }
    app_dsp_cfg_load_param_r_header(&app_dsp_cfg_data->dsp_cfg_header);
}

static void app_dsp_cfg_load_init_data(T_APP_DSP_PARAM_INIT *param_init, uint32_t data_offset)
{
    T_APP_DSP_PARAM_INIT_HEADER *header;
    uint32_t init_setting_size;
    T_APP_DSP_PARAM_INIT_SETTING *setting;

    header = &(param_init->dsp_param_init_header);
    header->sync_word = 0;
    fmc_flash_nor_read((flash_cur_bank_img_payload_addr_get(FLASH_IMG_DSPDATA) +
                        data_offset),
                       (uint8_t *)header, sizeof(T_APP_DSP_PARAM_INIT_HEADER));
    if (header->sync_word != APP_DATA_SYNC_WORD)
    {
        APP_PRINT_ERROR0("app_dsp_cfg_load_init_data:Load DSP init parameter fail");
        return;
    }
    init_setting_size = header->scenario_num * sizeof(T_APP_DSP_PARAM_INIT_SETTING);
    param_init->dsp_param_init_setting = calloc(1, init_setting_size);
    if (param_init->dsp_param_init_setting != NULL)
    {
        setting = param_init->dsp_param_init_setting;
        data_offset += sizeof(T_APP_DSP_PARAM_INIT_HEADER);
        fmc_flash_nor_read((flash_cur_bank_img_payload_addr_get(FLASH_IMG_DSPDATA) +
                            data_offset),
                           (uint8_t *)setting, init_setting_size);
        APP_PRINT_TRACE3("app_dsp_cfg_load_init_data a2dp default level %d, sco default level %d, tone gain level %d",
                         setting[0].dac_l_gain_level_default, setting[1].dac_l_gain_level_default,
                         header->dac_tone_gain_level);
    }
}

void app_dsp_cfg_init_gain_level(void)
{
    T_APP_DSP_PARAM_INIT_SETTING *setting = app_dsp_cfg_data->dsp_param_init.dsp_param_init_setting;

    //playback
    app_cfg_const.playback_volume_max = setting[AUDIO_SCENARIO].dac_l_gain_level_max;
    app_cfg_const.playback_volume_min = 0;
    app_cfg_const.playback_volume_default = setting[AUDIO_SCENARIO].dac_l_gain_level_default;

    //record
    app_cfg_const.record_volume_max = setting[VOICE_SCENARIO].adc1_gain_level_max;
    app_cfg_const.record_volume_min = 0;
    app_cfg_const.record_volume_default = setting[VOICE_SCENARIO].adc1_gain_level_default;

    //voice
    app_cfg_const.voice_out_volume_max = setting[VOICE_SCENARIO].dac_l_gain_level_max;
    app_cfg_const.voice_out_volume_min = 0;
    app_cfg_const.voice_out_volume_default = setting[VOICE_SCENARIO].dac_l_gain_level_default;

    app_cfg_const.voice_volume_in_max = setting[VOICE_SCENARIO].adc1_gain_level_max;
    app_cfg_const.voice_volume_in_min = 0;
    app_cfg_const.voice_volume_in_default = setting[VOICE_SCENARIO].adc1_gain_level_default;

    //apt
    app_cfg_const.apt_volume_out_max = setting[LINEIN_SCENARIO].dac_l_gain_level_max;
    app_cfg_const.apt_volume_out_min = 0;
    app_cfg_const.apt_volume_out_default = setting[LINEIN_SCENARIO].dac_l_gain_level_default;

    if (app_cfg_const.normal_apt_support)
    {
        app_cfg_const.apt_volume_in_max = setting[AUDIO_SCENARIO].adc1_gain_level_max;
        app_cfg_const.apt_volume_in_min = 0;
        app_cfg_const.apt_volume_in_default = setting[AUDIO_SCENARIO].adc1_gain_level_default;
    }
    else if (app_cfg_const.llapt_support)
    {
        app_cfg_const.apt_volume_in_max = setting[VOICE_SCENARIO].adc1_gain_level_max;
        app_cfg_const.apt_volume_in_min = 0;
        app_cfg_const.apt_volume_in_default = setting[VOICE_SCENARIO].adc1_gain_level_default;
    }

    //line in
    app_cfg_const.line_in_volume_out_max = setting[LINEIN_SCENARIO].dac_l_gain_level_max;
    app_cfg_const.line_in_volume_out_min = 0;
    app_cfg_const.line_in_volume_out_default = setting[LINEIN_SCENARIO].dac_l_gain_level_default;

    app_cfg_const.line_in_volume_in_max = setting[LINEIN_SCENARIO].adc1_gain_level_max;
    app_cfg_const.line_in_volume_in_min = 0;
    app_cfg_const.line_in_volume_in_default = setting[LINEIN_SCENARIO].adc1_gain_level_default;

    //ringtone
    app_cfg_const.ringtone_volume_max = setting[AUDIO_SCENARIO].dac_l_gain_level_max;
    app_cfg_const.ringtone_volume_min = 0;
    app_cfg_const.ringtone_volume_default =
        app_dsp_cfg_data->dsp_param_init.dsp_param_init_header.dac_tone_gain_level;

    //voice prompt
    app_cfg_const.voice_prompt_volume_max = setting[AUDIO_SCENARIO].dac_l_gain_level_max;
    app_cfg_const.voice_prompt_volume_min = 0;
    app_cfg_const.voice_prompt_volume_default =
        app_dsp_cfg_data->dsp_param_init.dsp_param_init_header.dac_tone_gain_level;


    memcpy(app_dsp_cfg_data->notification_dac_dig_gain_table, setting[AUDIO_SCENARIO].dac_l_gain_table,
           sizeof(setting[AUDIO_SCENARIO].dac_l_gain_table));
    memcpy(app_dsp_cfg_data->notification_dac_ana_gain_table, setting[AUDIO_SCENARIO].dac_r_gain_table,
           sizeof(setting[AUDIO_SCENARIO].dac_r_gain_table));

    memcpy(app_dsp_cfg_data->audio_dac_dig_gain_table, setting[AUDIO_SCENARIO].dac_l_gain_table,
           sizeof(setting[AUDIO_SCENARIO].dac_l_gain_table));
    memcpy(app_dsp_cfg_data->audio_dac_ana_gain_table, setting[AUDIO_SCENARIO].dac_r_gain_table,
           sizeof(setting[AUDIO_SCENARIO].dac_r_gain_table));
    memcpy(app_dsp_cfg_data->audio_adc_gain_table,
           setting[AUDIO_SCENARIO].adc1_analog_gain_table,
           sizeof(setting[AUDIO_SCENARIO].adc1_analog_gain_table));

    memcpy(app_dsp_cfg_data->voice_dac_dig_gain_table, setting[VOICE_SCENARIO].dac_l_gain_table,
           sizeof(setting[VOICE_SCENARIO].dac_l_gain_table));
    memcpy(app_dsp_cfg_data->voice_dac_ana_gain_table, setting[VOICE_SCENARIO].dac_r_gain_table,
           sizeof(setting[VOICE_SCENARIO].dac_r_gain_table));
    memcpy(app_dsp_cfg_data->voice_adc_gain_table,
           setting[VOICE_SCENARIO].adc1_analog_gain_table,
           sizeof(setting[VOICE_SCENARIO].adc1_analog_gain_table));

    memcpy(app_dsp_cfg_data->apt_dac_dig_gain_table, setting[LINEIN_SCENARIO].dac_l_gain_table,
           sizeof(setting[LINEIN_SCENARIO].dac_l_gain_table));
    memcpy(app_dsp_cfg_data->apt_dac_ana_gain_table, setting[LINEIN_SCENARIO].dac_r_gain_table,
           sizeof(setting[LINEIN_SCENARIO].dac_r_gain_table));
    memcpy(app_dsp_cfg_data->apt_adc_gain_table,
           setting[LINEIN_SCENARIO].adc1_analog_gain_table,
           sizeof(setting[LINEIN_SCENARIO].adc1_analog_gain_table));
#if (F_APP_SIDETONE_SUPPORT == 1)
    //sidetone
    sidetone_cfg.gain = setting[VOICE_SCENARIO].hw_sidetone_digital_gain;
    sidetone_cfg.hw_enable = setting[VOICE_SCENARIO].hw_sidetone_enable;
    sidetone_cfg.hpf_level = setting[VOICE_SCENARIO].hw_sidetone_hpf_level;
#endif
#if (F_APP_APT_SUPPORT == 1)
    memcpy(apt_dac_gain_table, setting[LINEIN_SCENARIO].dac_l_gain_table,
           sizeof(setting[LINEIN_SCENARIO].dac_l_gain_table));
#endif

    //free
    free(app_dsp_cfg_data->dsp_param_init.dsp_param_init_setting);
}

static bool app_dsp_cfg_eq_param_load(void)
{
    uint32_t                   data_offset;
    int32_t                    ret = 0;
    T_APP_DSP_EQ_PARAM_HEADER  *header = NULL;
    T_APP_DSP_EQ_SPK_HEADER    *spk_header = NULL;
    T_APP_DSP_EQ_MIC_HEADER    *mic_header = NULL;
    T_APP_DSP_EQ_SPK_HEADER    *voice_eq_header = NULL;
    uint32_t eq_spk_applications_num = 0;

    app_dsp_cfg_data->eq_param.header = (T_APP_DSP_EQ_PARAM_HEADER *)calloc(1,
                                                                            sizeof(T_APP_DSP_EQ_PARAM_HEADER));
    header = app_dsp_cfg_data->eq_param.header;
    if (header == NULL)
    {
        ret = -1;
        goto fail_alloc_eq_param_head;
    }
    header->sync_word = 0;
    data_offset = DSP_PARAM_OFFSET + app_dsp_cfg_data->dsp_cfg_header.eq_cmd_block_offset;
    fmc_flash_nor_read((flash_cur_bank_img_payload_addr_get(FLASH_IMG_DSPDATA) +
                        data_offset),
                       (uint8_t *)header, sizeof(T_APP_DSP_EQ_PARAM_HEADER) - 16);

    APP_PRINT_TRACE2("app_dsp_cfg_eq_param_load: param len %d, eq num %d", header->eq_param_len,
                     header->eq_num);

    if (header->sync_word != APP_DATA_SYNC_WORD)
    {
        ret = -2;
        goto fail_load_eq_header;
    }

    data_offset += sizeof(T_APP_DSP_EQ_PARAM_HEADER) - 16;

    eq_spk_applications_num = header->eq_spk_applications_num;

#if F_APP_LINEIN_SUPPORT
    if (eq_spk_applications_num == 3)
    {
        /* for compatibility, the value of eq_spk_applications_num should be 4 (normal, gaming, ANC and line-in) */
        APP_PRINT_TRACE0("app_dsp_cfg_eq_param_load: cfg does not support new format for line in EQ");
        eq_spk_applications_num = 4;
    }
#endif

    header->eq_spk_application_header = (T_APP_DSP_EQ_SPK_HEADER *)calloc(1,
                                                                          eq_spk_applications_num * sizeof(T_APP_DSP_EQ_SPK_HEADER));
    spk_header = header->eq_spk_application_header;
    if (spk_header == NULL)
    {
        ret = -3;
        goto fail_alloc_spk_info;
    }

    fmc_flash_nor_read((flash_cur_bank_img_payload_addr_get(FLASH_IMG_DSPDATA) +
                        data_offset),
                       (uint8_t *)spk_header, header->eq_spk_applications_num * sizeof(T_APP_DSP_EQ_SPK_HEADER));

    data_offset += header->eq_spk_applications_num * sizeof(T_APP_DSP_EQ_SPK_HEADER);

    header->eq_mic_application_header = (T_APP_DSP_EQ_MIC_HEADER *)calloc(1,
                                                                          header->eq_mic_applications_num * sizeof(T_APP_DSP_EQ_MIC_HEADER));
    mic_header = header->eq_mic_application_header;
    if (mic_header == NULL)
    {
        ret = -4;
        goto fail_alloc_mic_info;
    }
    fmc_flash_nor_read((flash_cur_bank_img_payload_addr_get(FLASH_IMG_DSPDATA) +
                        data_offset),
                       (uint8_t *)mic_header, header->eq_mic_applications_num * sizeof(T_APP_DSP_EQ_MIC_HEADER));

    data_offset += header->eq_mic_applications_num * sizeof(T_APP_DSP_EQ_MIC_HEADER);

    if (header->voice_eq_application_num != 0xff)
    {
        header->voice_eq_application_header = calloc(1,
                                                     header->voice_eq_application_num * sizeof(T_APP_DSP_EQ_SPK_HEADER));
        voice_eq_header = header->voice_eq_application_header;
        if (voice_eq_header == NULL)
        {
            ret = -6;
            goto fail_alloc_voice_eq_info;
        }
        fmc_flash_nor_read(flash_cur_bank_img_payload_addr_get(FLASH_IMG_DSPDATA) + data_offset,
                           voice_eq_header, header->voice_eq_application_num * sizeof(T_APP_DSP_EQ_SPK_HEADER));

        data_offset += header->voice_eq_application_num * sizeof(T_APP_DSP_EQ_SPK_HEADER);
    }

    header->sub_header = (T_APP_DSP_EQ_SUB_PARAM_HEADER *)calloc(1,
                                                                 header->eq_num * sizeof(T_APP_DSP_EQ_SUB_PARAM_HEADER));

    if (header->sub_header == NULL)
    {
        ret = -5;
        goto fail_alloc_sub_header_info;
    }
    fmc_flash_nor_read((flash_cur_bank_img_payload_addr_get(FLASH_IMG_DSPDATA) +
                        data_offset),
                       (uint8_t *)header->sub_header, header->eq_num * sizeof(T_APP_DSP_EQ_SUB_PARAM_HEADER));

    return true;

fail_alloc_sub_header_info:
    free(voice_eq_header);
fail_alloc_voice_eq_info:
    free(mic_header);
fail_alloc_mic_info:
    free(spk_header);
fail_alloc_spk_info:
fail_load_eq_header:
    free(header);
fail_alloc_eq_param_head:
    APP_PRINT_ERROR1("app_dsp_cfg_eq_param_load: fail %d", ret);
    return false;
}

static bool app_dsp_cfg_extensible_param_load(void)
{
    uint32_t data_offset;
    uint16_t index = 0;
    int32_t  ret = 0;
    T_APP_DSP_EXTENSIBLE_PARAM *header;

    header = (T_APP_DSP_EXTENSIBLE_PARAM *)(&app_dsp_cfg_data->dsp_extensible_param);
    header->sync_word = 0;

    if (app_dsp_cfg_data->dsp_cfg_header.extensible_param_offset == 0)
    {
        //for DSP configuration bin compatibility
        ret = -1;
        goto extensible_invalid_address;
    }

    data_offset = DSP_PARAM_OFFSET + app_dsp_cfg_data->dsp_cfg_header.extensible_param_offset;
    fmc_flash_nor_read((flash_cur_bank_img_payload_addr_get(FLASH_IMG_DSPDATA) +
                        data_offset),
                       (uint8_t *)header,
                       sizeof(header->sync_word) + sizeof(header->extensible_len));

    if (header->sync_word != APP_DATA_SYNC_WORD || header->extensible_len == 0)
    {
        ret = -2;
        goto extensible_param_header_fail;
    }

    data_offset += sizeof(header->sync_word);
    data_offset += sizeof(header->extensible_len);

    header->raw_data = (uint8_t *)calloc(1, header->extensible_len);

    if (header->raw_data == NULL)
    {
        ret = -3;
        goto extensible_raw_data_alloc_fail;
    }

    fmc_flash_nor_read((flash_cur_bank_img_payload_addr_get(FLASH_IMG_DSPDATA) +
                        data_offset),
                       header->raw_data, header->extensible_len);

    while (index < header->extensible_len)
    {
        uint16_t type;
        uint16_t length;

        LE_ARRAY_TO_UINT16(type, &header->raw_data[index]);
        index += 2;

        LE_ARRAY_TO_UINT16(length, &header->raw_data[index]);
        index += 2;

        switch (type)
        {
#if F_APP_BRIGHTNESS_SUPPORT
        case APP_DSP_EXTENSIBLE_PARAM_BRIGHTNESS:
            {
                T_APP_DSP_EXTENSIBLE_BRIGHTNESS *ext_data = (T_APP_DSP_EXTENSIBLE_BRIGHTNESS *)
                                                            &header->raw_data[index];

                dsp_config_support_brightness = true;
                brightness_level_max = ext_data->brightness_level_max;
                brightness_level_min = 0;
                brightness_level_default = ext_data->brightness_level_default;
                memcpy(brightness_gain_table, &ext_data->brightness_gain_table[0],
                       sizeof(ext_data->brightness_gain_table));
            }
            break;
#endif

        default:
            break;
        }

        index += length;
    }

    free(header->raw_data);

    return true;

extensible_invalid_address:
extensible_raw_data_alloc_fail:
extensible_param_header_fail:

    APP_PRINT_ERROR1("app_dsp_cfg_extensible_param_load: fail %d", ret);
    return false;
}

void app_dsp_cfg_init(void)
{
    app_dsp_cfg_load_header();
    app_dsp_cfg_eq_param_load();
    app_dsp_cfg_extensible_param_load();
    app_dsp_cfg_load_init_data(&app_dsp_cfg_data->dsp_param_init,
                               app_dsp_cfg_data->dsp_cfg_header.fixed_param_block_offset);
}

