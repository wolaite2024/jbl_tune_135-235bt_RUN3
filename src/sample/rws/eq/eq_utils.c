/*
 * Copyright (c) 2018, Realsil Semiconductor Corporation. All rights reserved.
 */
#include <stdlib.h>
#include <string.h>
#include "os_mem.h"
#include "eq.h"
#include "btm.h"
#include "trace.h"
#include "app_main.h"
#include "eq_utils.h"
#include "app_cfg.h"
#include "app_dsp_cfg.h"
#include "app_eq.h"
#include "app_harman_vendor_cmd.h"
#include "rtk_tuya_ble_ota.h"
#include "ftl.h"
#include "audio_probe.h"

#define EQ_MSB_IDX      (10)

#define EQ_BIT(_n)     (uint32_t)(1U << (_n))

//#define EQ_MAX_INDEX                9
#define EQ_MIN_INDEX                0

#define EQ_SR_8K                    (0)
#define EQ_SR_16K                   (1)
#define EQ_SR_32K                   (2)
#define EQ_SR_44K                   (3)
#define EQ_SR_48K                   (4)
#define EQ_SR_88K                   (5)
#define EQ_SR_96K                   (6)
#define EQ_SR_192K                  (7)
#define EQ_SR_12K                   (8)
#define EQ_SR_24K                   (9)
#define EQ_SR_11K                   (10)
#define EQ_SR_22K                   (11)
#define EQ_SR_ALL                   (15)

typedef struct t_eq_db
{
    uint8_t normal_eq_map[EQ_MSB_IDX];     /**< index is T_EQ_IDX  spk eq*/
    uint8_t gaming_eq_map[EQ_MSB_IDX];     /**< index is T_EQ_IDX  spk eq*/
    uint8_t anc_eq_map[EQ_MSB_IDX];        /**< index is T_EQ_IDX  spk eq*/
    uint8_t apt_eq_map[EQ_MSB_IDX];        /**< index is T_EQ_IDX  mic eq*/
#if F_APP_LINEIN_SUPPORT
    uint8_t line_in_eq_map[EQ_MSB_IDX];    /**< index is T_EQ_IDX  line in eq*/
#endif
#if F_APP_VOICE_SPK_EQ_SUPPORT
    uint8_t voice_spk_eq_map[EQ_MSB_IDX];  /**< index is T_EQ_IDX  voice spk eq*/
#endif
#if F_APP_VOICE_MIC_EQ_SUPPORT
    uint8_t voice_mic_eq_map[EQ_MSB_IDX];  /**< index is T_EQ_IDX  voice mic eq*/
#endif

    uint8_t normal_eq_num;                  /**< the num of eq group */
    uint8_t gaming_eq_num;                  /**< the num of eq group */
    uint8_t anc_eq_num;                  /**< the num of eq group */
    uint8_t apt_eq_num;                     /**< the num of eq group */
#if F_APP_LINEIN_SUPPORT
    uint8_t line_in_eq_num;               /**< the num of eq group */
#endif
#if F_APP_VOICE_SPK_EQ_SUPPORT
    uint8_t voice_spk_eq_num;
#endif
#if F_APP_VOICE_MIC_EQ_SUPPORT
    uint8_t voice_mic_eq_num;
#endif
} T_EQ_DB;

static T_EQ_DB *eq_db;

uint8_t eq_utils_stage_num_get(uint16_t info_len, T_EQ_DATA_DEST eq_data_dest, uint32_t sample_rate)
{
    uint16_t stage_num = 0;
    uint8_t dsp_tool_version = app_dsp_cfg_get_tool_info_version();

    if (dsp_tool_version == DSP_CONFIG_TOOL_DEF_VER_1)
    {
        stage_num = ((info_len - (MCU_TO_SDK_CMD_HDR_SIZE + PUBLIC_VALUE_SIZE + EXT_PUB_VALUE_SIZE +
                                  EQ_DATA_REV + EQ_DATA_TAIL)) / (STAGE_NUM_SIZE + EXT_STAGE_NUM_SIZE_VER_1));
    }
    else if (dsp_tool_version == DSP_CONFIG_TOOL_DEF_VER_2)
    {
#if F_APP_HARMAN_FEATURE_SUPPORT
        if (app_cfg_nv.harman_category_id != 0x00)
        {
            if (eq_data_dest == EQ_DATA_TO_DSP)
            {
                stage_num = app_cfg_nv.harman_customer_stage;
            }
        }
        else
#endif
        {
            if (eq_data_dest == EQ_DATA_TO_DSP)
            {
                if (sample_rate == AUDIO_EQ_SAMPLE_RATE_44_1KHZ)
                {
                    stage_num = ((info_len - (MCU_TO_SDK_CMD_HDR_SIZE + PUBLIC_VALUE_SIZE + EXT_PUB_VALUE_SIZE)) /
                                 (STAGE_NUM_SIZE + EXT_STAGE_NUM_SIZE_VER_2));
                }
                else if (sample_rate == AUDIO_EQ_SAMPLE_RATE_48KHZ)
                {
                    stage_num = ((info_len - (MCU_TO_SDK_CMD_HDR_SIZE + PUBLIC_VALUE_SIZE)) / STAGE_NUM_SIZE);
                }
            }
            else if (eq_data_dest == EQ_DATA_TO_PHONE)
            {
                stage_num = ((info_len - (MCU_TO_SDK_CMD_HDR_SIZE + PUBLIC_VALUE_SIZE + EXT_PUB_VALUE_SIZE)) /
                             (STAGE_NUM_SIZE + EXT_STAGE_NUM_SIZE_VER_2));
            }
        }
    }
    APP_PRINT_TRACE3("eq_utils_stage_num_get: stage_num %d, eq_data_dest %d, info_len %d", stage_num,
                     eq_data_dest, info_len);

    return stage_num;
}

static uint16_t eq_utils_dsp_param_get(T_EQ_TYPE eq_type, uint8_t index,
                                       void *p_data, uint16_t len, T_EQ_DATA_DEST eq_data_dest, uint32_t sample_rate)
{
    uint32_t eq_offset;
    uint16_t data_len;
    uint8_t  eq_idx;
    uint16_t param_len;
    uint8_t type;
    uint8_t *p_param;
    uint32_t eq_sample_rate = 0;
    uint8_t dsp_cfg_tool_ver = app_dsp_cfg_get_tool_info_version();

    APP_PRINT_TRACE5("eq_utils_dsp_param_get: eq_type %d, dsp cfg index %d, p_data %p, len %u, sample_rate %d",
                     eq_type, index, p_data, len, sample_rate);

    if ((index <= EQ_MAX_INDEX) && (p_data != NULL) && (len >= EQ_COEFF_SIZE))
    {
        for (uint8_t i = 0; i < app_dsp_cfg_data->eq_param.header->eq_num; i++)
        {
            param_len = app_dsp_cfg_data->eq_param.header->sub_header[i].param_length;

            APP_PRINT_TRACE1("[APP_EQ] eq_utils_dsp_param_get param len = %d", param_len);

            if (param_len != 0)
            {
                if (dsp_cfg_tool_ver == DSP_CONFIG_TOOL_DEF_VER_0)
                {
                    param_len = param_len -
                                8; //cmd_length include 8 bytes dsp cfg tool check information at old version
                }

                p_param = calloc(1, param_len);

                if (p_param == NULL)
                {
                    return 0;
                }
                //The 44.1khz or 48khz should be distinguished for each sub_header
                eq_offset = app_dsp_cfg_data->dsp_cfg_header.eq_cmd_block_offset +
                            app_dsp_cfg_data->eq_param.header->sub_header[i].offset;
                eq_sample_rate = app_dsp_cfg_data->eq_param.header->sub_header[i].sample_rate;
                eq_idx = app_dsp_cfg_data->eq_param.header->sub_header[i].eq_idx;
                type = app_dsp_cfg_data->eq_param.header->sub_header[i].eq_type;

                app_dsp_cfg_load_param_r_data(p_param, eq_offset, param_len);

                //H2D_AUDIO_EQ_PARA = 0x100A
                if (eq_data_dest == EQ_DATA_TO_PHONE)
                {
                    if (eq_type == type && eq_idx == index &&
                        eq_sample_rate == AUDIO_EQ_SAMPLE_RATE_44_1KHZ)
                    {
                        param_len -= 4; //need to return total_len - cmd_id_len - len_len
                        data_len = (len < param_len) ? len : param_len;

                        memcpy(p_data, &p_param[4], data_len);

                        free(p_param);

                        return data_len;
                    }
                }
                else if (eq_data_dest == EQ_DATA_TO_DSP)
                {
                    if ((eq_type == type && eq_idx == index) &&
                        (sample_rate == 0 || eq_sample_rate == sample_rate))
                    {
                        param_len -= 4;
                        data_len = (len < param_len) ? len : param_len;

                        APP_PRINT_TRACE3("[SD_CHECK]eq_utils_dsp_param_get harman_category_id %d,idx %d data_len %d",
                                         app_cfg_nv.harman_category_id,
                                         eq_idx,
                                         data_len);
#if F_APP_HARMAN_FEATURE_SUPPORT
                        if (app_cfg_nv.harman_category_id == 0x00)
                        {
                            //default eq
                            memcpy(p_data, &p_param[4], data_len);
                            APP_PRINT_TRACE1("[SD_CHECK]CMD_THIRD_PARTY_EQ eq_off1 p_data = %b", TRACE_BINARY(data_len,
                                             p_data));
                        }
                        else
                        {
                            uint8_t set_harman_eq_data[408] = {0};
                            static const uint8_t set_eq_data_compare[408] = {0};
                            uint8_t load_check = false;

                            if (eq_sample_rate == AUDIO_EQ_SAMPLE_RATE_44_1KHZ)
                            {
                                load_check = ftl_load_from_storage(&set_harman_eq_data[0], APP_HARMAN_EQ_DATA,
                                                                   APP_HARMAN_EQ_DATA_SIZE);
                                //APP_PRINT_TRACE1("[SD_CHECK]CMD_THIRD_PARTY_EQ 44_1KHZ eq = %b",
                                //                 TRACE_BINARY(408, set_harman_eq_data));
                            }
                            else if (eq_sample_rate == AUDIO_EQ_SAMPLE_RATE_48KHZ)
                            {
                                load_check = ftl_load_from_storage(&set_harman_eq_data[0], APP_HARMAN_EQ_DATA,
                                                                   APP_HARMAN_EQ_DATA_SIZE);
                                //APP_PRINT_TRACE1("[SD_CHECK]CMD_THIRD_PARTY_EQ 48KHZ eq = %b",
                                //                 TRACE_BINARY(408, set_harman_eq_data));
                            }
                            APP_PRINT_TRACE2("[SD_CHECK]CMD_THIRD_PARTY_EQ refference sample_rate=%d, load_check =%d",
                                             eq_sample_rate,
                                             load_check);
                            if ((app_cfg_nv.harman_category_id != 0x00) &&
                                (memcmp(&set_harman_eq_data[0], &set_eq_data_compare[0], 408) == 0))
                            {
                                //default eq
                                memcpy(p_data, &p_param[4], data_len);
                                //APP_PRINT_TRACE1("[SD_CHECK]CMD_THIRD_PARTY_EQ eq_off2 p_data = %b", TRACE_BINARY(data_len,
                                //                 p_data));
                            }
                            else
                            {
                                data_len = ((app_cfg_nv.harman_customer_stage * 20) + 8);
                                memcpy(p_data, &set_harman_eq_data[0], data_len);
                                //APP_PRINT_INFO2("[SD_CHECK]CMD_THIRD_PARTY_EQ default_eq_data refference data len %d, %b",
                                //                ((app_cfg_nv.harman_customer_stage * 20) + 8),
                                //                TRACE_BINARY(data_len, p_data));
                            }
                        }
#else
                        memcpy(p_data, &p_param[4], data_len);
#endif

                        free(p_param);

                        return data_len;
                    }
                }
                free(p_param);
            }
            else
            {
                break;
            }
        }
    }
    else
    {
        APP_PRINT_ERROR1("eq_utils_dsp_param_get: error idx %d", index);
    }

    return 0;
}

uint8_t eq_utils_num_get(T_EQ_TYPE eq_type, uint8_t mode)
{
    if (eq_db != NULL)
    {
        uint8_t map[EQ_TYPE_MAX][SPK_EQ_MODE_MAX] =
        {
            {eq_db->normal_eq_num, eq_db->gaming_eq_num, eq_db->anc_eq_num, 0},
            {eq_db->apt_eq_num, 0, 0, 0}
        };

#if F_APP_LINEIN_SUPPORT
        map[SPK_SW_EQ][LINE_IN_MODE] = eq_db->line_in_eq_num;
#endif

#if F_APP_VOICE_SPK_EQ_SUPPORT
        map[SPK_SW_EQ][VOICE_SPK_MODE] = eq_db->voice_spk_eq_num;
#endif

#if F_APP_VOICE_MIC_EQ_SUPPORT
        map[MIC_SW_EQ][VOICE_MIC_MODE] = eq_db->voice_mic_eq_num;
#endif

        APP_PRINT_INFO3("eq_utils_num_get1 %d,%d,%d", eq_type, mode, map[eq_type][mode]);
        return map[eq_type][mode];
    }
    //APP_PRINT_INFO0("eq_utils_num_get2");
    return 0;
}

uint8_t *eq_utils_map_get(T_EQ_TYPE eq_type, uint8_t mode)
{
    if (eq_db != NULL)
    {
        uint8_t *map[EQ_TYPE_MAX][SPK_EQ_MODE_MAX] =
        {
            {eq_db->normal_eq_map, eq_db->gaming_eq_map, eq_db->anc_eq_map, NULL},
            {eq_db->apt_eq_map, NULL, NULL, NULL}
        };

#if F_APP_LINEIN_SUPPORT
        map[SPK_SW_EQ][LINE_IN_MODE] = eq_db->line_in_eq_map;
#endif

#if F_APP_VOICE_SPK_EQ_SUPPORT
        map[SPK_SW_EQ][VOICE_SPK_MODE] = eq_db->voice_spk_eq_map;
#endif

#if F_APP_VOICE_MIC_EQ_SUPPORT
        map[MIC_SW_EQ][VOICE_MIC_MODE] = eq_db->voice_mic_eq_map;
#endif

        return map[eq_type][mode];
    }
    return NULL;
}

uint16_t eq_utils_param_len_get(void)
{
    return EQ_PARAM_SIZE;
}

uint16_t eq_utils_param_get(T_EQ_TYPE eq_type, uint8_t eq_mode, uint8_t index,
                            void *data, uint16_t len, T_EQ_DATA_DEST eq_data_dest, uint32_t sample_rate)
{
    uint8_t *buf;
    uint8_t *eq_map;

    APP_PRINT_TRACE7("eq_utils_param_get: eq_type %d, eq_mode %d, index %u, data %p, len 0x%04x, destin, %d, sample_rate , %d",
                     eq_type, eq_mode, index, data, len, eq_data_dest, sample_rate);

    eq_map = eq_utils_map_get(eq_type, eq_mode);

    buf = data;

    if ((buf != NULL) && (len != 0))
    {
        if (len >= EQ_PARAM_SIZE)
        {
            if (eq_map[index] <= EQ_MAX_INDEX)
            {
                uint16_t data_len = eq_utils_dsp_param_get(eq_type, eq_map[index], buf, len,
                                                           eq_data_dest, sample_rate);

                return data_len;
            }
        }
    }

    return 0;
}

static bool eq_utils_get_spk_capacity(uint8_t mode, uint32_t *bit_map)
{
    if (app_dsp_cfg_data->eq_param.header->sync_word == APP_DATA_SYNC_WORD)
    {
#if F_APP_VOICE_SPK_EQ_SUPPORT
        if (mode == VOICE_SPK_MODE)
        {
            if (app_dsp_cfg_data->eq_param.header->voice_eq_application_num != 0xff)
            {
                *bit_map =
                    app_dsp_cfg_data->eq_param.header->voice_eq_application_header[0].spk_eq_idx_bitmask;
            }
            else
            {
                APP_PRINT_INFO0("eq_utils_get_spk_capacity: not support voice spk eq!");
                return false;
            }
        }
        else
#endif
        {
            *bit_map =
                app_dsp_cfg_data->eq_param.header->eq_spk_application_header[mode].spk_eq_idx_bitmask;
        }

        APP_PRINT_INFO2("eq_utils_get_spk_capacity: mode %d, bit map 0x%x ", mode, *bit_map);

        return true;
    }

    return false;
}

static bool eq_utils_get_mic_capacity(uint8_t mode, uint32_t *bit_map)
{
    if (app_dsp_cfg_data->eq_param.header->sync_word == APP_DATA_SYNC_WORD)
    {
#if F_APP_VOICE_MIC_EQ_SUPPORT
        if (mode == VOICE_MIC_MODE)
        {
            if (app_dsp_cfg_data->eq_param.header->voice_eq_application_num != 0xff)
            {
                *bit_map =
                    app_dsp_cfg_data->eq_param.header->voice_eq_application_header[0].spk_eq_idx_bitmask;
            }
            else
            {
                APP_PRINT_INFO0("eq_utils_get_spk_capacity: not support voice mic eq!");
                return false;
            }
        }
        else
#endif
        {
            *bit_map =
                app_dsp_cfg_data->eq_param.header->eq_mic_application_header[mode].mic_eq_idx_bitmask;
        }

        APP_PRINT_INFO2("eq_utils_get_mic_capacity: mode %d, bit map 0x%x", mode, *bit_map);

        return true;
    }

    return false;
}

static bool eq_utils_get_capacity(T_EQ_TYPE type, uint8_t mode,
                                  uint32_t *bit_map)
{
    if (type == SPK_SW_EQ)
    {
        if (eq_utils_get_spk_capacity(mode, bit_map) == false)
        {
            return false;
        }
    }
    else if (type == MIC_SW_EQ)
    {
        if (eq_utils_get_mic_capacity(mode, bit_map) == false)
        {
            return false;
        }
    }

    return true;
}

static uint8_t eq_utils_map_init(T_EQ_TYPE eq_type, uint8_t mode)
{
    uint32_t eq_capacity = 0;
    uint8_t i ;
    uint8_t eq_num = 0;
    uint8_t *map = eq_utils_map_get(eq_type, mode);

    if (eq_utils_get_capacity(eq_type, mode, &eq_capacity) == false || (map == NULL))
    {
        APP_PRINT_ERROR4("eq_utils_map_init: init fail, type %d, mode %d, capacity %d, map %p",
                         eq_type, mode, eq_capacity, map);
        return 0;
    }

    for (i = 0; i < EQ_GROUP_NUM; ++i)
    {
        if (eq_capacity & EQ_BIT(i))
        {
            map[eq_num] = i;
            eq_num++;
        }
    }

    return eq_num;
}

uint8_t eq_utils_get_default_idx(T_EQ_TYPE eq_type, uint8_t mode)
{
    if (eq_type == SPK_SW_EQ)
    {
        return app_dsp_cfg_data->eq_param.header->eq_spk_application_header[mode].app_default_spk_eq_idx;
    }
    else
    {
        return app_dsp_cfg_data->eq_param.header->eq_mic_application_header[mode].app_default_mic_eq_idx;
    }
}

bool eq_utils_init(void)
{
    eq_db = calloc(1, sizeof(T_EQ_DB));

    if (eq_db == NULL)
    {
        APP_PRINT_ERROR0("eq_utils_init: eq init fail, calloc fail");
        return false;
    }

    eq_db->normal_eq_num = eq_utils_map_init(SPK_SW_EQ, NORMAL_MODE);
    eq_db->gaming_eq_num = eq_utils_map_init(SPK_SW_EQ, GAMING_MODE);
    eq_db->anc_eq_num = eq_utils_map_init(SPK_SW_EQ, ANC_MODE);

    eq_db->apt_eq_num = eq_utils_map_init(MIC_SW_EQ, APT_MODE);

#if F_APP_LINEIN_SUPPORT
    eq_db->line_in_eq_num = eq_utils_map_init(SPK_SW_EQ, LINE_IN_MODE);
#endif

    return true;
}

bool eq_utils_deinit(void)
{
    if (eq_db != NULL)
    {
        free(eq_db);
    }

    return true;
}
