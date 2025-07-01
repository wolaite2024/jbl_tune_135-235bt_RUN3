/*
 * Copyright (c) 2018, Realsil Semiconductor Corporation. All rights reserved.
 */
#include <stdlib.h>
#include <string.h>
#include "os_mem.h"
#include "trace.h"
#include "eq.h"
#include "bt_a2dp.h"
#include "eq_utils.h"
#include "app_main.h"
#include "app_link_util.h"
#include "app_cmd.h"
#include "app_report.h"
#include "app_eq.h"
#include "app_cfg.h"
#include "app_audio_policy.h"
#include "app_dsp_cfg.h"
#include "app_multilink.h"
#include "app_bt_policy_api.h"
#include "app_roleswap.h"
#include "app_anc.h"
#include "app_hfp.h"
#include "audio_passthrough.h"

#if F_APP_HARMAN_FEATURE_SUPPORT
#include "app_harman_vendor_cmd.h"
#include "app_harman_eq.h"
#include "ftl.h"
#endif

#if F_APP_LINEIN_SUPPORT
#include "app_line_in.h"
#endif

uint16_t app_eq_dsp_param_get(T_EQ_TYPE eq_type, T_EQ_STREAM_TYPE stream_type,
                              uint8_t eq_mode, uint8_t index, uint8_t *data)
{
    uint16_t eq_len = 0;

    if (eq_len == 0)
    {
        uint8_t *eq_map = eq_utils_map_get(eq_type, eq_mode);
        uint8_t eq_idx_ori = eq_map[index];

        DBG_DIRECT("app_eq_dsp_param_get: idx %d, data %p", eq_idx_ori, data);

        if ((eq_idx_ori <= EQ_MAX_INDEX) && (data != NULL))
        {
            DBG_DIRECT("app_eq_dsp_param_get: eq_num %d", app_dsp_cfg_data->eq_param.header->eq_num);

            for (uint8_t i = 0; i < app_dsp_cfg_data->eq_param.header->eq_num; i++)
            {
                uint8_t type = app_dsp_cfg_data->eq_param.header->sub_header[i].eq_type;
                uint8_t eq_idx = app_dsp_cfg_data->eq_param.header->sub_header[i].eq_idx;
                uint8_t scenario = app_dsp_cfg_data->eq_param.header->sub_header[i].stream_type;

                if (type == eq_type && eq_idx == eq_idx_ori && scenario == stream_type)
                {
                    uint16_t param_len = app_dsp_cfg_data->eq_param.header->sub_header[i].param_length;

                    if (param_len != 0)
                    {
                        uint32_t eq_offset = app_dsp_cfg_data->dsp_cfg_header.eq_cmd_block_offset +
                                             app_dsp_cfg_data->eq_param.header->sub_header[i].offset;

                        if (stream_type == EQ_STREAM_TYPE_AUDIO)
                        {
                            app_dsp_cfg_load_param_r_data(data, eq_offset, param_len);
                            eq_len = param_len;
                        }
#if F_APP_VOICE_SPK_EQ_SUPPORT || F_APP_VOICE_MIC_EQ_SUPPORT
                        else if (stream_type == EQ_STREAM_TYPE_VOICE) // Differentiate between voice spk and voice mic eq
                        {
                            uint8_t voice_eq_header[4] = {0};
                            voice_eq_header[0] = stream_type;
                            voice_eq_header[1] = eq_type;

                            memcpy(data, voice_eq_header, 4);
                            app_dsp_cfg_load_param_r_data(&data[4], eq_offset, param_len);
                            eq_len = param_len + 4;
                        }
#endif
                    }
                    break;
                }

            }
        }
        else
        {
            APP_PRINT_ERROR1("app_eq_dsp_param_get: error idx %d", eq_idx_ori);
        }
    }

    APP_PRINT_TRACE5("app_eq_dsp_param_get: eq_type %d, dsp cfg index %d, stream_type %d, data %p, len %d",
                     eq_type, index, stream_type, data, eq_len);

    return eq_len;
}

/**
 * There are some difference between data transfered to dsp and phone
 *  The function followed is used to process data from dsp bin to dsp eq setting
*/
static void app_eq_dsp_set(T_AUDIO_EFFECT_INSTANCE instance, void *info_buf, uint16_t info_len,
                           T_EQ_TYPE eq_type)
{
    if (instance != NULL)
    {
        if (app_dsp_cfg_get_tool_info_version() != DSP_CONFIG_TOOL_DEF_VER_0)
        {
            uint32_t sample_rate = app_eq_sample_rate_get();

            info_len = eq_utils_stage_num_get(info_len, EQ_DATA_TO_DSP, sample_rate) * STAGE_NUM_SIZE +
                       (MCU_TO_SDK_CMD_HDR_SIZE + PUBLIC_VALUE_SIZE);
        }
#if F_APP_HARMAN_FEATURE_SUPPORT
        APP_PRINT_INFO4("app_eq_dsp_set: category_id: %d, customer_stage: %d, info_len: %d, eq_type: %d",
                        app_cfg_nv.harman_category_id,
                        app_cfg_nv.harman_customer_stage,
                        info_len,
                        eq_type);
        if (eq_type == SPK_SW_EQ)
        {
            if (app_cfg_nv.harman_category_id != 0x00)
            {
                info_len = ((app_cfg_nv.harman_customer_stage * 20) + 8);
            }

            app_harman_set_eq_boost_gain();
            app_harman_send_hearing_protect_status_to_dsp();
        }
#endif
        eq_set(instance, info_buf, info_len);
    }
}

void app_eq_init(void)
{
    uint16_t len = 0;
    uint8_t dsp_tool_version = app_dsp_cfg_get_tool_info_version();

    if (eq_utils_init())
    {
        if (dsp_tool_version == DSP_CONFIG_TOOL_DEF_VER_2)
        {
            len = EQ_GROUP_NUM * (STAGE_NUM_SIZE + EXT_STAGE_NUM_SIZE_VER_2) + MCU_TO_SDK_CMD_HDR_SIZE +
                  PUBLIC_VALUE_SIZE + EXT_PUB_VALUE_SIZE;//262
        }
        else if (dsp_tool_version == DSP_CONFIG_TOOL_DEF_VER_1)
        {
            len = EQ_GROUP_NUM * (STAGE_NUM_SIZE +  EXT_STAGE_NUM_SIZE_VER_1) + MCU_TO_SDK_CMD_HDR_SIZE +
                  PUBLIC_VALUE_SIZE + EQ_DATA_REV + EXT_PUB_VALUE_SIZE + EQ_DATA_TAIL;  //296
        }
        else if (dsp_tool_version == DSP_CONFIG_TOOL_DEF_VER_0)
        {
            len = EQ_GROUP_NUM * STAGE_NUM_SIZE + (MCU_TO_SDK_CMD_HDR_SIZE +
                                                   PUBLIC_VALUE_SIZE);  //208
        }
#if F_APP_HARMAN_FEATURE_SUPPORT
        app_haraman_eq_init();

        len = APP_HARMAN_EQ_DATA_SIZE;
#endif
        app_db.dynamic_eq_len = len;
    }
    else
    {
        app_db.dynamic_eq_len = 0;
        APP_PRINT_TRACE0("app_eq_init: init fail!!!");
    }
}

void app_eq_deinit(void)
{
    if (app_db.dynamic_eq_len != 0)
    {
        app_db.dynamic_eq_len = 0;
    }
    eq_utils_deinit();
}

void app_eq_enable_info_get(uint8_t eq_mode, T_EQ_ENABLE_INFO *enable_info)
{
    T_APP_BR_LINK *link = NULL;

    memset(enable_info, 0, sizeof(T_EQ_ENABLE_INFO));

    if (app_db.sw_eq_type == SPK_SW_EQ)
    {
#if F_APP_VOICE_SPK_EQ_SUPPORT
        if (eq_mode == VOICE_SPK_MODE)
        {
            link = &app_db.br_link[app_hfp_get_active_hf_index()];
            enable_info->instance = link->voice_spk_eq_instance;
        }
        else
#endif
        {
            link = &app_db.br_link[app_get_active_a2dp_idx()];
            enable_info->instance = link->eq_instance;
            enable_info->is_enable = link->audio_eq_enabled;
        }

        enable_info->idx = app_cfg_nv.eq_idx;
    }
    else
    {
#if F_APP_VOICE_MIC_EQ_SUPPORT
        if (eq_mode == VOICE_MIC_MODE)
        {
            link = &app_db.br_link[app_hfp_get_active_hf_index()];
            enable_info->instance = link->voice_mic_eq_instance;
            enable_info->idx = app_cfg_nv.eq_idx;
        }
        else
#endif
        {
            enable_info->instance = app_db.apt_eq_instance;
            enable_info->idx = app_cfg_nv.apt_eq_idx;
        }
    }

    APP_PRINT_INFO3("app_eq_enable_info_get: instance %p, is_enable %d, idx %d", enable_info->instance,
                    enable_info->is_enable, enable_info->idx);
}

bool app_eq_index_set(T_EQ_TYPE eq_type, uint8_t mode, uint8_t index)
{
    uint8_t eq_num;
    uint16_t eq_len;
    uint8_t i;
    uint8_t *dynamic_eq_buf = calloc(1, app_db.dynamic_eq_len);
    T_EQ_STREAM_TYPE stream_type = EQ_STREAM_TYPE_AUDIO;
    bool ret = false;

#if F_APP_VOICE_SPK_EQ_SUPPORT
    if (mode == VOICE_SPK_MODE)
    {
        stream_type = EQ_STREAM_TYPE_VOICE;
    }
#endif

    if (dynamic_eq_buf != NULL)
    {
        eq_num = eq_utils_num_get(eq_type, mode);

        APP_PRINT_TRACE4("app_eq_index_set: eq_type %d, mode %d, eq_num %d, index 0x%02x",
                         eq_type, mode, eq_num, index);

        eq_len = app_eq_dsp_param_get(eq_type, stream_type, mode, index, dynamic_eq_buf);

        //when len < 0x10, has eq off effect
        eq_len = (eq_num == 0) ? 0x05 : eq_len;

        if (eq_type == SPK_SW_EQ)
        {
#if F_APP_LINEIN_SUPPORT
            if (app_line_in_plug_state_get())
            {
                eq_set(app_db.line_in_eq_instance, app_db.dynamic_eq_buf, eq_len);
                app_cfg_nv.eq_idx = index;
                ret = true;
            }
            else
#endif
            {
                T_APP_BR_LINK *link;

                for (i = 0; i < MAX_BR_LINK_NUM; i++)
                {
                    link = &app_db.br_link[i];
                    eq_set(link->eq_instance, dynamic_eq_buf, eq_len);
                }
                app_cfg_nv.eq_idx = index;
                ret = true;
            }
        }
        else
        {
            if (index <= eq_num && eq_len != 0)
            {
                eq_set(app_db.apt_eq_instance, dynamic_eq_buf, eq_len);
                app_cfg_nv.apt_eq_idx = index ;
                ret = true;
            }
        }

        free(dynamic_eq_buf);
        dynamic_eq_buf = NULL;
    }
    else
    {
        APP_PRINT_ERROR0("app_eq_index_set: fail!!!");
    }

    return ret;
}

bool app_eq_param_set(uint8_t eq_mode, uint8_t index, void *data, uint16_t len)
{
    uint8_t *buf;
    uint8_t i;
    T_EQ_ENABLE_INFO enable_info;

    app_eq_enable_info_get(eq_mode, &enable_info);

    APP_PRINT_TRACE4("app_eq_param_set: eq_mode %d, index %u, data %p, len 0x%04x", eq_mode, index,
                     data, len);

    buf = data;
    app_db.dynamic_eq_buf = calloc(1, app_db.dynamic_eq_len);

    if (app_db.dynamic_eq_buf != NULL)
    {
        if ((buf != NULL) && (len != 0))
        {
            if (len <= app_db.dynamic_eq_len)
            {
                memcpy(app_db.dynamic_eq_buf, buf, len);

                if (app_db.sw_eq_type == SPK_SW_EQ)
                {
                    T_APP_BR_LINK *link;

                    for (i = 0; i < MAX_BR_LINK_NUM; i++)
                    {
                        link = &app_db.br_link[i];
                        if (link->eq_instance != NULL)
                        {
#if F_APP_HARMAN_FEATURE_SUPPORT
                            app_harman_set_eq_boost_gain();
                            app_harman_send_hearing_protect_status_to_dsp();
#endif
                            eq_set(enable_info.instance, app_db.dynamic_eq_buf, len);
                        }
                    }

                    app_cfg_nv.eq_idx = index;
                }
                else
                {
                    if (app_db.apt_eq_instance != NULL)
                    {
                        eq_set(app_db.apt_eq_instance, app_db.dynamic_eq_buf, len);
                    }

                    app_cfg_nv.apt_eq_idx = index;
                }

                free(app_db.dynamic_eq_buf);
                return true;
            }
        }
        free(app_db.dynamic_eq_buf);
        return false;
    }
    else
    {
        APP_PRINT_ERROR0("app_eq_param_set: fail!!!");
        return false;
    }
}

void app_eq_audio_eq_enable(T_AUDIO_EFFECT_INSTANCE *eq_instance, bool *audio_eq_enabled)
{
    if (eq_instance && audio_eq_enabled)
    {
        if (!(*audio_eq_enabled))
        {
            eq_enable(*eq_instance);
            *audio_eq_enabled = true;
        }
    }
}

// uint32_t app_eq_sample_rate_get(uint8_t *br_addr)
uint32_t app_eq_sample_rate_get()
{
    uint32_t sample_rate = 0;

#if F_APP_LINEIN_SUPPORT
    if (app_line_in_plug_state_get())
    {
        sample_rate = AUDIO_EQ_SAMPLE_RATE_48KHZ;
        APP_PRINT_TRACE0("app_eq_sample_rate_get: line in 48K");

        return sample_rate;
    }
#endif

    if (app_bt_policy_get_b2s_connected_num() == 0 ||
        app_audio_get_bud_stream_state() != BUD_STREAM_STATE_AUDIO)
    {
        sample_rate = AUDIO_EQ_SAMPLE_RATE_44_1KHZ;
        APP_PRINT_ERROR0("app_eq_sample_rate_get: address error, return default sample_rate");

        return sample_rate;
    }

    T_APP_BR_LINK *link = &app_db.br_link[app_get_active_a2dp_idx()];
    sample_rate = link->a2dp_codec_info.sampling_frequency;

    if (link->a2dp_codec_info.sampling_frequency == SAMPLE_RATE_44K)
    {
        sample_rate = AUDIO_EQ_SAMPLE_RATE_44_1KHZ;
    }
    else if (link->a2dp_codec_info.sampling_frequency == SAMPLE_RATE_48K)
    {
        sample_rate = AUDIO_EQ_SAMPLE_RATE_48KHZ;
    }

    APP_PRINT_TRACE2("app_eq_sample_rate_get: codec_type 0x%x, sample_rate %d",
                     link->a2dp_codec_type, link->a2dp_codec_info.sampling_frequency);

    return sample_rate;
}

static bool app_eq_report_get_eq_param(T_AUDIO_EQ_REPORT_DATA *eq_report_data)
{
    bool ret = true;
    int err = 0;

    uint32_t sample_rate = app_eq_sample_rate_get();
    uint16_t eq_len = 0;
    uint16_t eq_len_to_dsp = 0;
    uint8_t dsp_tool_info_version = app_dsp_cfg_get_tool_info_version();
    T_AUDIO_EQ_INFO *p_eq_info = eq_report_data->eq_info;
    uint8_t  *eq_data_temp = calloc(app_db.dynamic_eq_len, sizeof(uint8_t));

    if (!eq_data_temp)
    {
        err = -1;
        ret = false;
        goto exit;
    }

    eq_len = eq_utils_param_get((T_EQ_TYPE)p_eq_info->sw_eq_type, p_eq_info->eq_mode, p_eq_info->eq_idx,
                                eq_data_temp,  app_db.dynamic_eq_len, EQ_DATA_TO_PHONE, sample_rate);

    if (dsp_tool_info_version != DSP_CONFIG_TOOL_DEF_VER_0)
    {
        // data sent to dsp don't include extend info
        eq_len_to_dsp = eq_utils_stage_num_get(eq_len, EQ_DATA_TO_PHONE, sample_rate) * STAGE_NUM_SIZE +
                        MCU_TO_SDK_CMD_HDR_SIZE + PUBLIC_VALUE_SIZE;

        if (dsp_tool_info_version == DSP_CONFIG_TOOL_DEF_VER_2)
        {
            p_eq_info->eq_data_len = eq_len - eq_len_to_dsp + EQ_SAMPLE_RATE_SIZE;
        }
        else if (dsp_tool_info_version == DSP_CONFIG_TOOL_DEF_VER_1)
        {
            p_eq_info->eq_data_len = eq_len - eq_len_to_dsp  - (EQ_DATA_REV + EQ_DATA_TAIL) +
                                     EQ_SAMPLE_RATE_SIZE;
        }
    }
    else
    {
        eq_len_to_dsp = EQ_SDK_HEADER_SIZE;

        //except 4byte sdk header,add 1byte sample rate
        p_eq_info->eq_data_len = eq_len - EQ_SDK_HEADER_SIZE + EQ_SAMPLE_RATE_SIZE;
    }

    if (p_eq_info->eq_data_buf != NULL)
    {
        free(p_eq_info->eq_data_buf);
    }

    p_eq_info->eq_data_buf = malloc(p_eq_info->eq_data_len);
    if (p_eq_info->eq_data_buf == NULL)
    {
        err = -2;
        ret = false;
        goto exit;
    }

    p_eq_info->eq_data_buf[0] = sample_rate; //3 - > 44.1K   4 -> 48k
    memcpy(&(p_eq_info->eq_data_buf)[1], &eq_data_temp[eq_len_to_dsp],
           p_eq_info->eq_data_len - EQ_SAMPLE_RATE_SIZE);

exit:
    if (eq_data_temp)
    {
        free(eq_data_temp);
    }

    APP_PRINT_TRACE2("app_eq_generate_eq_param_for_report: ret %d, err %d", ret, err);

    return ret;
}

void app_eq_report_abort_frame(T_AUDIO_EQ_REPORT_DATA *eq_report_data)
{
    uint8_t abort_frame[9];
    uint16_t frame_len = 0x02;

    abort_frame[0] = eq_report_data->eq_info->eq_idx;
    abort_frame[1] = eq_report_data->eq_info->sw_eq_type;
    abort_frame[2] = eq_report_data->eq_info->eq_mode;
    abort_frame[3] = AUDIO_EQ_FRAME_ABORT;
    abort_frame[4] = eq_report_data->eq_info->eq_seq;
    abort_frame[5] = (uint8_t)(frame_len & 0xFF);
    abort_frame[6] = (uint8_t)((frame_len >> 8) & 0xFF);
    abort_frame[7] = (uint8_t)(CMD_AUDIO_EQ_PARAM_GET & 0xFF);
    abort_frame[8] = (uint8_t)((CMD_AUDIO_EQ_PARAM_GET >> 8) & 0xFF);

    app_report_event(eq_report_data->cmd_path, EVENT_AUDIO_EQ_PARAM_REPORT, eq_report_data->id,
                     abort_frame, frame_len + EQ_ABORT_FRAME_HEADER_LEN);

    if (eq_report_data->eq_info->eq_data_buf != NULL)
    {
        free(eq_report_data->eq_info->eq_data_buf);
        eq_report_data->eq_info->eq_data_buf = NULL;
    }
}

bool app_eq_get_link_info(void *p_link, uint8_t cmd_path, T_AUDIO_EQ_REPORT_DATA *eq_report_data)
{
    bool ret = true;

    eq_report_data->cmd_path = cmd_path;

    if (cmd_path == CMD_PATH_SPP || cmd_path == CMD_PATH_IAP)
    {
        T_APP_BR_LINK *p_br_link = (T_APP_BR_LINK *)p_link;

        eq_report_data->eq_info = &p_br_link->audio_eq_info;
        eq_report_data->id = p_br_link->id;
    }
    else if (cmd_path == CMD_PATH_LE)
    {
        T_APP_LE_LINK *p_le_link = (T_APP_LE_LINK *)p_link;
        eq_report_data->eq_info = &p_le_link->audio_eq_info;

        /* EQ: 3 is att header,  6 is trasmint service header */
        eq_report_data->max_frame_len = p_le_link->mtu_size - 3 - EQ_START_FRAME_HEADER_LEN - 6;
        eq_report_data->id = p_le_link->id;
    }
    else
    {
        ret = false;
    }

    return ret;
}

bool app_eq_report_eq_frame(T_AUDIO_EQ_REPORT_DATA *eq_report_data)
{
    T_AUDIO_EQ_INFO *p_eq_info = eq_report_data->eq_info;
    uint16_t frame_len = p_eq_info->eq_data_len;
    T_AUDIO_EQ_FRAME_TYPE eq_frame_type;
    uint8_t *frame;
    uint8_t frame_idx = 0;
    uint8_t total_frame_len = 0;

    if (eq_report_data->cmd_path == CMD_PATH_LE)
    {
        if (p_eq_info->eq_data_offset == 0)
        {
            if (p_eq_info->eq_data_len > eq_report_data->max_frame_len)
            {
                eq_frame_type = AUDIO_EQ_FRAME_START;
                frame_len = eq_report_data->max_frame_len;
            }
            else
            {
                eq_frame_type = AUDIO_EQ_FRAME_SINGLE;
            }
        }
        else
        {
            if (p_eq_info->eq_data_len > eq_report_data->max_frame_len)
            {
                eq_frame_type = AUDIO_EQ_FRAME_CONTINUE;
                frame_len = eq_report_data->max_frame_len;
            }
            else
            {
                eq_frame_type = AUDIO_EQ_FRAME_END;
            }
        }
    }
    else
    {
        eq_frame_type = AUDIO_EQ_FRAME_SINGLE;
    }

    if (eq_frame_type == AUDIO_EQ_FRAME_START)
    {
        total_frame_len = frame_len + EQ_START_FRAME_HEADER_LEN;
    }
    else
    {
        total_frame_len = frame_len + EQ_SINGLE_FRAME_HEADER_LEN;
    }

    frame = malloc(total_frame_len);

    if (frame == NULL)
    {
        app_eq_report_abort_frame(eq_report_data);
        return false;
    }

    frame[frame_idx++] = p_eq_info->eq_idx;
    frame[frame_idx++] = p_eq_info->sw_eq_type;
    frame[frame_idx++] = p_eq_info->eq_mode;
    frame[frame_idx++] = eq_frame_type;
    frame[frame_idx++] = p_eq_info->eq_seq;

    if (eq_frame_type == AUDIO_EQ_FRAME_START)
    {
        frame[frame_idx++] = (uint8_t)(p_eq_info->eq_data_len & 0xFF);/*the low byte of frame_len*/
        frame[frame_idx++] = (uint8_t)((p_eq_info->eq_data_len >> 8) & 0xFF);/*the high byte of frame_len*/
    }

    frame[frame_idx++] = (uint8_t)(frame_len & 0xFF);/*the low byte of frame_len*/
    frame[frame_idx++] = (uint8_t)((frame_len >> 8) & 0xFF);/*the high byte of frame_len*/

    memcpy(&frame[frame_idx], p_eq_info->eq_data_buf, frame_len);

    app_report_event(eq_report_data->cmd_path, EVENT_AUDIO_EQ_PARAM_REPORT, eq_report_data->id,
                     frame, total_frame_len);

    p_eq_info->eq_data_len -= frame_len;

    if (frame[3] == AUDIO_EQ_FRAME_START || frame[3] == AUDIO_EQ_FRAME_CONTINUE)
    {
        p_eq_info->eq_data_offset += frame_len;
        (p_eq_info->eq_seq)++;
    }
    else
    {
        if (p_eq_info->eq_data_buf != NULL)
        {
            free(p_eq_info->eq_data_buf);
            p_eq_info->eq_data_buf = NULL;
        }
    }

    free(frame);

    return true;
}

void app_eq_report_eq_param(void *p_link, uint8_t cmd_path)
{
    int8_t err = 0;
    T_AUDIO_EQ_REPORT_DATA eq_report_data;

    if (!app_eq_get_link_info(p_link, cmd_path, &eq_report_data))
    {
        err = -1;
        goto exit;
    }

    if (eq_report_data.eq_info->eq_data_len > 0)
    {
        if (!app_eq_report_eq_frame(&eq_report_data))
        {
            err = -2;
            goto exit;
        }
    }

exit:

    APP_PRINT_TRACE3("app_eq_report_eq_param: cmd_path %d, len %d, ret %d", cmd_path,
                     eq_report_data.eq_info->eq_data_len, err);
}

void app_eq_report_terminate_param_report(void *p_link, uint8_t cmd_path)
{
    T_AUDIO_EQ_REPORT_DATA eq_report_data;

    int8_t err = 0;

    if (!app_eq_get_link_info(p_link, cmd_path, &eq_report_data))
    {
        err = -1;
        goto exit;
    }

    app_eq_report_abort_frame(&eq_report_data);

exit:
    APP_PRINT_TRACE1("app_eq_report_abort: err %d", err);
}

void app_eq_idx_check_accord_mode(void)
{
#if F_APP_LINEIN_SUPPORT
    if ((app_db.spk_eq_mode == LINE_IN_MODE) &&
        (eq_utils_num_get(SPK_SW_EQ, app_db.spk_eq_mode) != 0))
    {
        app_cfg_nv.eq_idx = app_cfg_nv.eq_idx_line_in_mode_record;
    }
    else
#endif
    {
        APP_PRINT_INFO3("app_eq_idx_check_accord_mode %d,%d,%d", app_db.spk_eq_mode,
                        app_cfg_nv.eq_idx,
                        app_cfg_nv.eq_idx_normal_mode_record);
        if ((app_db.spk_eq_mode == GAMING_MODE) &&
            (eq_utils_num_get(SPK_SW_EQ, app_db.spk_eq_mode) != 0))
        {
            app_cfg_nv.eq_idx = app_cfg_nv.eq_idx_gaming_mode_record;
        }
        else if ((app_db.spk_eq_mode == ANC_MODE) &&
                 (eq_utils_num_get(SPK_SW_EQ, app_db.spk_eq_mode) != 0))
        {
            app_cfg_nv.eq_idx = app_cfg_nv.eq_idx_anc_mode_record;
        }
#if F_APP_VOICE_SPK_EQ_SUPPORT
        else if ((app_db.spk_eq_mode == VOICE_SPK_MODE) &&
                 (eq_utils_num_get(SPK_SW_EQ, app_db.spk_eq_mode) != 0))
        {
            app_cfg_nv.eq_idx = 0;
        }
#endif
        else if ((eq_utils_num_get(SPK_SW_EQ, NORMAL_MODE) != 0))
        {
            app_cfg_nv.eq_idx = app_cfg_nv.eq_idx_normal_mode_record;
        }
    }
}

void app_eq_sync_idx_accord_eq_mode(uint8_t eq_mode, uint8_t index)
{
    if (eq_mode == GAMING_MODE)
    {
        app_cfg_nv.eq_idx_gaming_mode_record = index;
        app_relay_async_single(APP_MODULE_TYPE_AUDIO_POLICY, APP_REMOTE_MSG_SYNC_GAMING_RECORD_EQ_INDEX);
    }
    else if (eq_mode == NORMAL_MODE)
    {
        app_cfg_nv.eq_idx_normal_mode_record = index;
        app_relay_async_single(APP_MODULE_TYPE_AUDIO_POLICY, APP_REMOTE_MSG_SYNC_NORMAL_RECORD_EQ_INDEX);
    }
    else if (eq_mode == ANC_MODE)
    {
        app_cfg_nv.eq_idx_anc_mode_record = index;
        app_relay_async_single(APP_MODULE_TYPE_AUDIO_POLICY, APP_REMOTE_MSG_SYNC_ANC_RECORD_EQ_INDEX);
    }
#if F_APP_LINEIN_SUPPORT
    else if (eq_mode == LINE_IN_MODE)
    {
        /* do not need to relay due to only stereo support line-in */
        app_cfg_nv.eq_idx_line_in_mode_record = index;
    }
#endif
}

void app_eq_play_audio_eq_tone(void)
{
#if F_APP_TEAMS_FEATURE_SUPPORT
    return;
#else

    uint8_t *eq_map = NULL;

    eq_map = eq_utils_map_get(SPK_SW_EQ, app_db.spk_eq_mode);

    APP_PRINT_INFO1("app_eq_play_audio_eq_tone: language_status %x", app_cfg_nv.language_status);

#if F_APP_HARMAN_FEATURE_SUPPORT
    if (app_cfg_nv.language_status == 0) // play EQ tone
    {
        app_audio_tone_type_play((T_APP_AUDIO_TONE_TYPE)(TONE_AUDIO_EQ_0 + eq_map[app_cfg_nv.eq_idx]),
                                 false, true);
    }
    else // play EQ VP
    {
        app_audio_tone_type_play((T_APP_AUDIO_TONE_TYPE)(TONE_APT_EQ_0 + eq_map[app_cfg_nv.eq_idx]),
                                 false, true);
    }
#else
    app_audio_tone_type_play((T_APP_AUDIO_TONE_TYPE)(TONE_AUDIO_EQ_0 + eq_map[app_cfg_nv.eq_idx]),
                             false, true);
#endif
#endif
}


void app_eq_play_apt_eq_tone(void)
{
    uint8_t *apt_eq_map = eq_utils_map_get(MIC_SW_EQ, APT_MODE);

#if F_APP_TEAMS_FEATURE_SUPPORT
#else
    app_audio_tone_type_play((T_APP_AUDIO_TONE_TYPE)(TONE_APT_EQ_0 + apt_eq_map[app_cfg_nv.apt_eq_idx]),
                             false, true);
#endif
}

T_AUDIO_EFFECT_INSTANCE app_eq_create(T_EQ_CONTENT_TYPE eq_content_type,
                                      T_EQ_STREAM_TYPE stream_type,
                                      T_EQ_TYPE eq_type, uint8_t eq_mode, uint8_t eq_index)
{
    T_AUDIO_EFFECT_INSTANCE eq_instance = NULL;

    uint8_t *eq_buf = calloc(1, app_db.dynamic_eq_len);

    if (eq_buf != NULL)
    {
        uint16_t eq_len = 0;

#if F_APP_HARMAN_FEATURE_SUPPORT
        if (eq_type == SPK_SW_EQ && stream_type == EQ_STREAM_TYPE_AUDIO)
        {
            uint8_t category_id = app_cfg_nv.harman_category_id;

            if (category_id != CATEGORY_ID_OFF)
            {
                uint8_t harman_eq_info[APP_HARMAN_CUSTOMER_EQ_INFO_SIZE] = {0};

                app_harman_eq_info_load_from_ftl(category_id, harman_eq_info,
                                                 APP_HARMAN_CUSTOMER_EQ_INFO_SIZE);

                eq_len = app_harman_get_eq_data((T_APP_HARMAN_EQ_INFO *)harman_eq_info,
                                                (T_APP_HARMAN_EQ_DATA *)eq_buf);
            }
            else
            {
                eq_len = app_harman_eq_dsp_param_get(eq_type, stream_type, eq_mode, eq_index, eq_buf);
            }

            APP_PRINT_INFO3("app_eq_create category_id=%d, eq_len=%d, eq_type=%d",
                            app_cfg_nv.harman_category_id,
                            eq_len,
                            eq_type);

            app_harman_set_eq_boost_gain();
            app_harman_send_hearing_protect_status_to_dsp();
        }
        else
#endif
        {
            eq_len = app_eq_dsp_param_get(eq_type, stream_type, eq_mode, eq_index, eq_buf);
        }

        eq_instance = eq_create(eq_content_type, eq_buf, eq_len);
        free(eq_buf);
        eq_buf = NULL;
    }
    else
    {
        APP_PRINT_ERROR0("app_eq_create: fail!!!");
    }

    return eq_instance;
}

static uint8_t app_eq_get_stage_num_accord_eq_len(uint16_t info_len)
{
    uint16_t stage_num_from_cmd = 0;
    uint8_t dsp_tool_version = app_dsp_cfg_get_tool_info_version();

    if (dsp_tool_version == DSP_CONFIG_TOOL_DEF_VER_1)
    {
        stage_num_from_cmd = (info_len - EQ_SAMPLE_RATE_SIZE - EQ_SDK_HEADER_SIZE - EQ_HEADER_SIZE -
                              EQ_INFO_HEADER_SIZE) /
                             (STAGE_NUM_SIZE +
                              EXT_STAGE_NUM_SIZE_VER_1);
    }
    else if (dsp_tool_version == DSP_CONFIG_TOOL_DEF_VER_2)
    {
        stage_num_from_cmd = (info_len - EQ_SAMPLE_RATE_SIZE - EQ_SDK_HEADER_SIZE - EQ_HEADER_SIZE -
                              EQ_INFO_HEADER_SIZE) /
                             (STAGE_NUM_SIZE +
                              EXT_STAGE_NUM_SIZE_VER_2);
    }
    APP_PRINT_TRACE1("app_eq_get_stage_num_accord_eq_len %d", stage_num_from_cmd);

    return stage_num_from_cmd;
}

bool app_eq_cmd_operate(uint8_t eq_mode, uint8_t eq_adjust_side, uint8_t is_play_eq_tone,
                        uint8_t eq_idx, uint8_t eq_len_to_dsp, uint8_t *cmd_ptr)
{
    bool ret = true;
    T_EQ_ENABLE_INFO enable_info;

    app_eq_enable_info_get(eq_mode, &enable_info);

    APP_PRINT_TRACE7("app_eq_cmd_operate: type:%d, mode:%d, side:%d, tone:%d, idx:%d, len:%d, ignore:%d",
                     app_db.sw_eq_type, eq_mode, eq_adjust_side, is_play_eq_tone, eq_idx, eq_len_to_dsp,
                     app_db.ignore_set_pri_audio_eq);

#if F_APP_ERWS_SUPPORT
    app_relay_async_single(APP_MODULE_TYPE_AUDIO_POLICY, APP_REMOTE_MSG_SYNC_SW_EQ_TYPE);
#endif

    if (app_db.sw_eq_type == SPK_SW_EQ)
    {
        if (eq_adjust_side != BOTH_SIDE_ADJUST)
        {
            ret = false;
            goto exit;
        }

        if (app_cfg_nv.eq_idx != eq_idx)
        {
            app_cfg_nv.eq_idx = eq_idx;
            app_relay_async_single(APP_MODULE_TYPE_AUDIO_POLICY, APP_REMOTE_MSG_SYNC_EQ_INDEX);
        }

        if (app_db.ignore_set_pri_audio_eq)
        {
            app_db.ignore_set_pri_audio_eq = false;
            app_relay_async_single_with_raw_msg(APP_MODULE_TYPE_AUDIO_POLICY, APP_REMOTE_MSG_EQ_DATA,
                                                &cmd_ptr[0], eq_len_to_dsp);
        }
        else
        {
            if (is_play_eq_tone)
            {
                app_eq_play_audio_eq_tone();
            }

            if (app_db.remote_session_state == REMOTE_SESSION_STATE_CONNECTED)
            {
                uint16_t relay_data_len = eq_len_to_dsp + 1;
                uint8_t *relay_data = (uint8_t *)malloc(relay_data_len);

                relay_data[0] = eq_mode;
                memcpy(&relay_data[1], &cmd_ptr[0], eq_len_to_dsp);
                app_relay_sync_single_with_raw_msg(APP_MODULE_TYPE_AUDIO_POLICY, APP_REMOTE_MSG_EQ_DATA,
                                                   &cmd_ptr[0], eq_len_to_dsp, REMOTE_TIMER_HIGH_PRECISION, 0, false);
                free(relay_data);
            }
            else
            {
                app_eq_param_set(eq_mode, eq_idx, &cmd_ptr[0], eq_len_to_dsp);

                app_eq_audio_eq_enable(&enable_info.instance, &enable_info.is_enable);
            }
        }
    }
    else if (app_db.sw_eq_type == MIC_SW_EQ)
    {
#if F_APP_APT_SUPPORT
        if (app_cfg_nv.apt_eq_idx != eq_idx)
        {
            app_cfg_nv.apt_eq_idx = eq_idx;
            app_relay_async_single(APP_MODULE_TYPE_APT, APP_REMOTE_MSG_APT_EQ_INDEX_SYNC);
        }

        if (is_play_eq_tone)
        {
            app_eq_play_apt_eq_tone();
        }
#endif

        if (eq_adjust_side == BOTH_SIDE_ADJUST)
        {
            if (app_db.remote_session_state == REMOTE_SESSION_STATE_CONNECTED)
            {
                uint16_t relay_data_len = eq_len_to_dsp + 1;
                uint8_t *relay_data = (uint8_t *)malloc(relay_data_len);

                relay_data[0] = eq_mode;
                memcpy(&relay_data[1], &cmd_ptr[0], eq_len_to_dsp);

                app_relay_sync_single_with_raw_msg(APP_MODULE_TYPE_AUDIO_POLICY, APP_REMOTE_MSG_EQ_DATA,
                                                   &cmd_ptr[0], eq_len_to_dsp, REMOTE_TIMER_HIGH_PRECISION, 0, false);
                free(relay_data);
            }
            else
            {
                app_eq_param_set(eq_mode, eq_idx, &cmd_ptr[0], eq_len_to_dsp);
                eq_enable(enable_info.instance);
            }
        }
        else if (eq_adjust_side == app_cfg_const.bud_side)
        {
            app_eq_param_set(eq_mode, eq_idx, &cmd_ptr[0], eq_len_to_dsp);
            eq_enable(enable_info.instance);
        }
        else if (eq_adjust_side ^ app_cfg_const.bud_side)
        {
            uint16_t relay_data_len = eq_len_to_dsp + 1;
            uint8_t *relay_data = (uint8_t *)malloc(relay_data_len);

            relay_data[0] = eq_mode;
            memcpy(&relay_data[1], &cmd_ptr[0], eq_len_to_dsp);

            app_relay_async_single_with_raw_msg(APP_MODULE_TYPE_AUDIO_POLICY, APP_REMOTE_MSG_EQ_DATA,
                                                &cmd_ptr[0], eq_len_to_dsp);
            free(relay_data);
        }
    }
    else
    {
        ret = false;
    }

exit:
    return ret;
}

static uint8_t app_eq_judge_mic_eq_mode(uint8_t stream_type)
{
    uint8_t new_eq_mode = MIC_EQ_MODE_MAX;

    if (stream_type == EQ_STREAM_TYPE_AUDIO)
    {
#if (F_APP_ANC_SUPPORT == 1)
        if (app_anc_is_anc_on_state((T_ANC_APT_STATE)app_db.current_listening_state)
            && (eq_utils_num_get(MIC_SW_EQ, APT_MODE) != 0))
        {
            new_eq_mode = APT_MODE;
        }
#endif
    }
    else if (stream_type == EQ_STREAM_TYPE_VOICE)
    {
#if F_APP_VOICE_MIC_EQ_SUPPORT
        if (app_hfp_sco_is_connected() && (eq_utils_num_get(MIC_SW_EQ, VOICE_MIC_MODE) != 0))
        {
            new_eq_mode = VOICE_MIC_MODE;
        }
#endif
    }

    return new_eq_mode;
}

uint8_t app_eq_judge_audio_eq_mode(void)
{
    uint8_t new_eq_mode = SPK_EQ_MODE_MAX;

    if (0)
    {
    }
#if (F_APP_LINEIN_SUPPORT == 1)
    else if (app_line_in_plug_state_get() && (eq_utils_num_get(SPK_SW_EQ, LINE_IN_MODE) != 0))
    {
        new_eq_mode = LINE_IN_MODE;
    }
#endif
#if F_APP_VOICE_SPK_EQ_SUPPORT
    else if (app_hfp_sco_is_connected() && (eq_utils_num_get(SPK_SW_EQ, VOICE_SPK_MODE) != 0))
    {
        new_eq_mode = VOICE_SPK_MODE;
    }
#endif
    else if (app_db.gaming_mode && (eq_utils_num_get(SPK_SW_EQ, GAMING_MODE) != 0))
    {
        new_eq_mode = GAMING_MODE;
    }
#if (F_APP_ANC_SUPPORT == 1)
    else if (app_anc_is_anc_on_state((T_ANC_APT_STATE)app_db.current_listening_state)
             && (eq_utils_num_get(SPK_SW_EQ, ANC_MODE) != 0))
    {
        new_eq_mode = ANC_MODE;
    }
#endif
    else
    {
        new_eq_mode = NORMAL_MODE;
    }

    return new_eq_mode;
}

uint8_t app_eq_mode_get(uint8_t eq_type, uint8_t stream_type)
{
    uint8_t eq_mode = MIC_EQ_MODE_MAX;

    if (eq_type == MIC_SW_EQ)
    {
        eq_mode = app_eq_judge_mic_eq_mode(stream_type);
    }
    else
    {
        eq_mode = app_eq_judge_audio_eq_mode();
    }

    APP_PRINT_TRACE2("app_eq_mode_get: eq_type %d, eq_mode %d", eq_type, eq_mode);
    return eq_mode;
}

void app_eq_enable_effect(uint8_t eq_mode, uint16_t eq_len)
{
    T_APP_BR_LINK *link = NULL;
    uint8_t *buf = calloc(1, APP_HARMAN_EQ_DATA_SIZE);

    if (buf != NULL)
    {
        if (app_db.sw_eq_type == SPK_SW_EQ)
        {
#if F_APP_VOICE_SPK_EQ_SUPPORT
            if (eq_mode == VOICE_SPK_MODE)
            {
                link = &app_db.br_link[app_hfp_get_active_hf_index()];
                link->voice_spk_eq_instance = eq_create(EQ_CONTENT_TYPE_VOICE, buf, eq_len);

                if (link->voice_spk_eq_instance != NULL)
                {
                    eq_enable(link->voice_spk_eq_instance);
                    audio_track_effect_attach(link->sco_track_handle, link->voice_spk_eq_instance);
                }
            }
            else
#endif
            {
                link = &app_db.br_link[app_get_active_a2dp_idx()];
                link->eq_instance = eq_create(EQ_CONTENT_TYPE_AUDIO, buf, eq_len);

                if (link->eq_instance != NULL)
                {
                    app_eq_audio_eq_enable(&link->eq_instance, &link->audio_eq_enabled);
                    audio_track_effect_attach(link->a2dp_track_handle, link->eq_instance);
                }
            }
        }
        else
        {
#if F_APP_VOICE_MIC_EQ_SUPPORT
            if (eq_mode == VOICE_MIC_MODE)
            {
                link = &app_db.br_link[app_hfp_get_active_hf_index()];
                link->voice_mic_eq_instance = eq_create(EQ_CONTENT_TYPE_VOICE, buf, eq_len);

                if (link->voice_mic_eq_instance != NULL)
                {
                    eq_enable(link->voice_mic_eq_instance);
                    audio_track_effect_attach(link->sco_track_handle, link->voice_mic_eq_instance);
                }
            }
            else
#endif
            {
#if F_APP_APT_SUPPORT
                app_db.apt_eq_instance = eq_create(EQ_CONTENT_TYPE_PASSTHROUGH, buf, eq_len);

                if (app_db.apt_eq_instance != NULL)
                {
                    eq_enable(app_db.apt_eq_instance);
                    audio_passthrough_effect_attach(app_db.apt_eq_instance);
                }
#endif
            }
        }
        free(buf);
        buf = NULL;
    }
    else
    {
        APP_PRINT_ERROR0("app_eq_enable_effect: fail");
    }
}

void app_eq_change_audio_eq_mode(bool report_when_eq_mode_change)
{
    uint8_t new_eq_mode;

    new_eq_mode = app_eq_judge_audio_eq_mode();

    if (app_db.spk_eq_mode != new_eq_mode)
    {
        APP_PRINT_TRACE2("app_eq_change_audio_eq_mode: eq_mode %d -> %d", app_db.spk_eq_mode,
                         new_eq_mode);
        app_db.spk_eq_mode = new_eq_mode;
        app_eq_idx_check_accord_mode();

        if (!app_db.eq_ctrl_by_src)
        {
            app_eq_index_set(SPK_SW_EQ, app_db.spk_eq_mode, app_cfg_nv.eq_idx);
        }
        else
        {
#if F_APP_ERWS_SUPPORT
            if ((app_cfg_nv.bud_role == REMOTE_SESSION_ROLE_PRIMARY) ||
                (app_db.remote_session_state == REMOTE_SESSION_STATE_DISCONNECTED))
#endif
            {
                if (report_when_eq_mode_change)
                {
                    app_report_eq_index(EQ_INDEX_REPORT_BY_CHANGE_MODE);
                }
            }
        }
    }
}

void app_eq_cmd_handle(uint8_t *cmd_ptr, uint16_t cmd_len, uint8_t cmd_path, uint8_t app_idx,
                       uint8_t *ack_pkt)
{
    uint16_t cmd_id = (uint16_t)(cmd_ptr[0] | (cmd_ptr[1] << 8));
    ack_pkt[2] = CMD_SET_STATUS_COMPLETE;

    switch (cmd_id)
    {
    case CMD_AUDIO_EQ_QUERY:
        {
            uint8_t query_type;
            uint8_t buf[5];

            query_type = cmd_ptr[2];

            if (query_type == AUDIO_EQ_QUERY_STATE)
            {
                app_report_event(cmd_path, EVENT_ACK, app_idx, ack_pkt, 3);

                buf[0] = query_type;
                buf[1] = 1;
            }
            else if (query_type == AUDIO_EQ_QUERY_NUM)
            {
                app_report_event(cmd_path, EVENT_ACK, app_idx, ack_pkt, 3);

                buf[0] = query_type;
                buf[1] = eq_utils_num_get(SPK_SW_EQ, NORMAL_MODE);
                buf[2] = eq_utils_num_get(SPK_SW_EQ, GAMING_MODE);
                buf[3] = eq_utils_num_get(MIC_SW_EQ, APT_MODE);
                buf[4] = eq_utils_num_get(SPK_SW_EQ, ANC_MODE);
            }
            else
            {
                ack_pkt[2] = CMD_SET_STATUS_PARAMETER_ERROR;
                app_report_event(cmd_path, EVENT_ACK, app_idx, ack_pkt, 3);
                break;
            }
            app_report_event(cmd_path, EVENT_AUDIO_EQ_REPLY, app_idx, buf, 5);
        }
        break;

    case CMD_AUDIO_EQ_QUERY_PARAM:
        {
            /*
                combine 3 commands:
                1. CMD_AUDIO_EQ_QUERY: AUDIO_EQ_QUERY_STATE
                2. CMD_AUDIO_EQ_QUERY: AUDIO_EQ_QUERY_NUM
                3. CMD_AUDIO_EQ_INDEX_GET
            */
            uint8_t eq_mode = cmd_ptr[2];
            uint8_t buf[4];

            buf[0] = eq_mode;
            buf[1] = 1;
            if (eq_mode == GAMING_MODE)
            {
                buf[2] = eq_utils_num_get(SPK_SW_EQ, GAMING_MODE);
                buf[3] = app_cfg_nv.eq_idx_gaming_mode_record;
            }
            else if (eq_mode == NORMAL_MODE)
            {
                buf[2] = eq_utils_num_get(SPK_SW_EQ, NORMAL_MODE);
                buf[3] = app_cfg_nv.eq_idx_normal_mode_record;
            }
            else if (eq_mode == ANC_MODE)
            {
                buf[2] = eq_utils_num_get(SPK_SW_EQ, ANC_MODE);
                buf[3] = app_cfg_nv.eq_idx_anc_mode_record;
            }

            app_report_event(cmd_path, EVENT_AUDIO_EQ_REPLY_PARAM, app_idx, buf, 4);
        }
        break;

    case CMD_AUDIO_EQ_PARAM_SET:
        {
            uint8_t eq_idx = cmd_ptr[2];
            uint8_t eq_adjust_side = cmd_ptr[3];
            uint8_t eq_mode = cmd_ptr[5];
            uint8_t type = cmd_ptr[6];
            uint8_t seq = cmd_ptr[7];

            bool is_play_eq_tone = false;
            uint16_t eq_len_to_dsp = 0;
            uint32_t sample_rate = app_eq_sample_rate_get();
            T_AUDIO_EQ_INFO *p_audio_eq_info = NULL;
            uint8_t *eq_data = NULL;

            app_db.sw_eq_type = cmd_ptr[4];

            if ((app_db.sw_eq_type == SPK_SW_EQ) && (app_cfg_nv.eq_idx != eq_idx) ||
                (app_db.sw_eq_type == MIC_SW_EQ) && (app_cfg_nv.apt_eq_idx != eq_idx))
            {
                /* change eq, it needs to play eq tone */
                is_play_eq_tone = true;
            }

            if ((app_db.sw_eq_type == SPK_SW_EQ) && (app_db.spk_eq_mode != eq_mode))
            {
                ack_pkt[2] = CMD_SET_STATUS_PROCESS_FAIL;
                app_report_event(cmd_path, EVENT_ACK, app_idx, ack_pkt, 3);
                break;
            }

            if (cmd_path == CMD_PATH_SPP || cmd_path == CMD_PATH_IAP)
            {
                p_audio_eq_info = &app_db.br_link[app_idx].audio_eq_info;
            }
            else if (cmd_path == CMD_PATH_LE)
            {
                p_audio_eq_info = &app_db.le_link[app_idx].audio_eq_info;
            }
            else
            {
                ack_pkt[2] = CMD_SET_STATUS_PROCESS_FAIL;
                app_report_event(cmd_path, EVENT_ACK, app_idx, ack_pkt, 3);
                break;
            }

            if (type == AUDIO_EQ_FRAME_SINGLE || type == AUDIO_EQ_FRAME_START)
            {
                p_audio_eq_info->eq_seq = EQ_INIT_SEQ;
                p_audio_eq_info->eq_mode = cmd_ptr[5];
                p_audio_eq_info->eq_data_len = (uint16_t)(cmd_ptr[8] | cmd_ptr[9] << 8);
                p_audio_eq_info->eq_data_offset = 0;

                if (type == AUDIO_EQ_FRAME_START)
                {
                    if (p_audio_eq_info->eq_data_buf != NULL)
                    {
                        free(p_audio_eq_info->eq_data_buf);
                    }

                    p_audio_eq_info->eq_data_buf = malloc(p_audio_eq_info->eq_data_len);

                    if (p_audio_eq_info->eq_data_buf == NULL)
                    {
                        ack_pkt[2] = CMD_SET_STATUS_PROCESS_FAIL;
                        app_report_event(cmd_path, EVENT_ACK, app_idx, ack_pkt, 3);
                        break;
                    }
                }
            }

            if (seq != p_audio_eq_info->eq_seq)
            {
                ack_pkt[2] = CMD_SET_STATUS_PARAMETER_ERROR;
                app_report_event(cmd_path, EVENT_ACK, app_idx, ack_pkt, 3);
                break;
            }

            if (type == AUDIO_EQ_FRAME_SINGLE)
            {
                eq_data = &cmd_ptr[10];
            }
            else
            {
                uint8_t frame_len = 0;
                uint8_t cmd_idx;

                if (type == AUDIO_EQ_FRAME_START)
                {
                    frame_len = (uint16_t)(cmd_ptr[10] | cmd_ptr[11] << 8);
                    cmd_idx = 12;
                }
                else
                {
                    frame_len = (uint16_t)(cmd_ptr[8] | cmd_ptr[9] << 8);
                    cmd_idx = 10;
                }

                p_audio_eq_info->eq_seq = p_audio_eq_info->eq_seq + 1;
                memcpy(p_audio_eq_info->eq_data_buf + p_audio_eq_info->eq_data_offset, &cmd_ptr[cmd_idx],
                       frame_len);
                p_audio_eq_info->eq_data_offset += frame_len;
                eq_data = p_audio_eq_info->eq_data_buf;
            }

            if (type == AUDIO_EQ_FRAME_SINGLE || type == AUDIO_EQ_FRAME_END)
            {
                uint8_t dsp_tool_version = app_dsp_cfg_get_tool_info_version();
                uint8_t stage_num = app_eq_get_stage_num_accord_eq_len(p_audio_eq_info->eq_data_len);

                if (dsp_tool_version == DSP_CONFIG_TOOL_DEF_VER_2)
                {
                    eq_len_to_dsp = p_audio_eq_info->eq_data_len - stage_num * EXT_STAGE_NUM_SIZE_VER_2
                                    - EQ_INFO_HEADER_SIZE - EQ_SAMPLE_RATE_SIZE;
                }
                else if (dsp_tool_version == DSP_CONFIG_TOOL_DEF_VER_1)
                {
                    eq_len_to_dsp = p_audio_eq_info->eq_data_len - stage_num * EXT_STAGE_NUM_SIZE_VER_1
                                    - EQ_INFO_HEADER_SIZE - EQ_SAMPLE_RATE_SIZE;
                }
                else if (dsp_tool_version == DSP_CONFIG_TOOL_DEF_VER_0)
                {
                    eq_len_to_dsp = p_audio_eq_info->eq_data_len - EQ_SAMPLE_RATE_SIZE;
                }

                if (app_db.sw_eq_type == SPK_SW_EQ)
                {
                    app_eq_sync_idx_accord_eq_mode(eq_mode, eq_idx);
                }

                if (eq_data[0] == sample_rate)
                {
                    if (!app_eq_cmd_operate(eq_mode, eq_adjust_side, is_play_eq_tone, eq_idx, eq_len_to_dsp,
                                            &eq_data[1]))
                    {
                        ack_pkt[2] = CMD_SET_STATUS_PROCESS_FAIL;
                    }
                }
                else
                {
                    ack_pkt[2] = CMD_SET_STATUS_PARAMETER_ERROR;
                }
            }

            app_report_event(cmd_path, EVENT_ACK, app_idx, ack_pkt, 3);
        }
        break;

    case CMD_AUDIO_EQ_PARAM_GET:
        {
            if (cmd_len < 5)
            {
                /* if the length of cmd less than 5, the EQ version is 1.1 */
                T_SRC_SUPPORT_VER_FORMAT *version = app_cmd_get_src_version(cmd_path, app_idx);

                version->eq_spec_ver_major = 1;
                version->eq_spec_ver_minor = 1;
                ack_pkt[2] = CMD_SET_STATUS_DISALLOW;
                app_cmd_update_eq_ctrl(false, true);
                app_report_event(cmd_path, EVENT_ACK, app_idx, ack_pkt, 3);
                break;
            }

            T_AUDIO_EQ_REPORT_DATA eq_report_data;
            void *p_link = NULL;

            if (cmd_path == CMD_PATH_SPP || cmd_path == CMD_PATH_IAP)
            {
                p_link = (void *) &app_db.br_link[app_idx];
            }
            else if (cmd_path == CMD_PATH_LE)
            {
                p_link = (void *)&app_db.le_link[app_idx];
            }

            if (p_link && app_eq_get_link_info(p_link, cmd_path, &eq_report_data))
            {
                T_AUDIO_EQ_INFO *p_eq_info = eq_report_data.eq_info;

                p_eq_info->eq_idx = cmd_ptr[2];
                p_eq_info->sw_eq_type = (T_EQ_TYPE)cmd_ptr[3];
                p_eq_info->eq_mode = cmd_ptr[4];
                p_eq_info->eq_seq = EQ_INIT_SEQ;
                p_eq_info->eq_data_offset = 0;

                if (app_eq_report_get_eq_param(&eq_report_data))
                {
                    app_report_event(cmd_path, EVENT_ACK, app_idx, ack_pkt, 3);
                    app_eq_report_eq_frame(&eq_report_data);
                }
                else
                {
                    ack_pkt[2] = CMD_SET_STATUS_PROCESS_FAIL;
                    app_report_event(cmd_path, EVENT_ACK, app_idx, ack_pkt, 3);
                }
            }
            else
            {
                ack_pkt[2] = CMD_SET_STATUS_PROCESS_FAIL;
                app_report_event(cmd_path, EVENT_ACK, app_idx, ack_pkt, 3);
            }
        }
        break;

    case CMD_AUDIO_EQ_INDEX_SET:
        {
            uint8_t eq_idx;
            uint8_t eq_mode;
            bool is_play_eq_tone = false;

            eq_idx = cmd_ptr[2];
            eq_mode = cmd_ptr[3];

            if (app_cfg_nv.eq_idx != eq_idx)
            {
                is_play_eq_tone = true;
            }

            if (app_db.spk_eq_mode != eq_mode)
            {
                ack_pkt[2] = CMD_SET_STATUS_PROCESS_FAIL;
                app_report_event(cmd_path, EVENT_ACK, app_idx, ack_pkt, 3);
            }
            else
            {
                app_eq_sync_idx_accord_eq_mode(eq_mode, eq_idx);
                app_cfg_nv.eq_idx = eq_idx;

                if (!app_db.ignore_set_pri_audio_eq)
                {
                    if (app_db.remote_session_state == REMOTE_SESSION_STATE_CONNECTED)
                    {
                        app_relay_sync_single_with_raw_msg(APP_MODULE_TYPE_AUDIO_POLICY,
                                                           APP_REMOTE_MSG_SYNC_DEFAULT_EQ_INDEX,
                                                           &eq_idx, sizeof(uint8_t), REMOTE_TIMER_HIGH_PRECISION,
                                                           0, false);
                    }
                    else
                    {
                        app_eq_index_set(SPK_SW_EQ, eq_mode, eq_idx);
                        T_APP_BR_LINK *p_link = &app_db.br_link[app_get_active_a2dp_idx()];
                        if (p_link)
                        {
                            app_eq_audio_eq_enable(&p_link->eq_instance, &p_link->audio_eq_enabled);
                        }
                    }
                }
                else
                {
                    app_db.ignore_set_pri_audio_eq = false;
                    app_relay_async_single(APP_MODULE_TYPE_AUDIO_POLICY, APP_REMOTE_MSG_SYNC_DEFAULT_EQ_INDEX);
                }

                if (is_play_eq_tone)
                {
                    app_eq_play_audio_eq_tone();
                }

                app_report_event(cmd_path, EVENT_ACK, app_idx, ack_pkt, 3);
            }
        }
        break;

    case CMD_AUDIO_EQ_INDEX_GET:
        {
            uint8_t eq_mode = cmd_ptr[2];

            if (app_db.spk_eq_mode != eq_mode)
            {
                ack_pkt[2] = CMD_SET_STATUS_PROCESS_FAIL;
                app_report_event(cmd_path, EVENT_ACK, app_idx, ack_pkt, 3);
                break;
            }

            app_report_event(cmd_path, EVENT_ACK, app_idx, ack_pkt, 3);
            app_report_eq_index(EQ_INDEX_REPORT_BY_GET_EQ_INDEX);
        }
        break;

#if F_APP_APT_SUPPORT
    case CMD_APT_EQ_INDEX_SET:
        {
            uint8_t event_data = cmd_ptr[2];
            uint8_t eq_num;
            bool is_play_eq_tone = false;
            eq_num = eq_utils_num_get(MIC_SW_EQ, APT_MODE);

            if (app_cfg_nv.apt_eq_idx != event_data)
            {
                is_play_eq_tone = true;
            }

            if (eq_num != 0)
            {
                app_cfg_nv.apt_eq_idx = event_data;

                if (app_cfg_nv.bud_role == REMOTE_SESSION_ROLE_PRIMARY ||
                    app_db.remote_session_state == REMOTE_SESSION_STATE_DISCONNECTED)
                {
                    if (is_play_eq_tone)
                    {
                        app_eq_play_apt_eq_tone();
                    }

                    if (app_db.remote_session_state == REMOTE_SESSION_STATE_CONNECTED)
                    {
                        app_relay_sync_single_with_raw_msg(APP_MODULE_TYPE_APT, APP_REMOTE_MSG_APT_EQ_DEFAULT_INDEX_SYNC,
                                                           &event_data, sizeof(uint8_t), REMOTE_TIMER_HIGH_PRECISION,
                                                           0, false);
                    }
                    else
                    {
                        app_eq_index_set(MIC_SW_EQ, APT_MODE, app_cfg_nv.apt_eq_idx);
                        eq_enable(app_db.apt_eq_instance);
                    }
                }
            }
            else
            {
                ack_pkt[2] = CMD_SET_STATUS_PROCESS_FAIL;
            }

            app_report_event(cmd_path, EVENT_ACK, app_idx, ack_pkt, 3);
        }
        break;

    case CMD_APT_EQ_INDEX_GET:
        {
            app_report_event(cmd_path, EVENT_ACK, app_idx, ack_pkt, 3);
            app_report_apt_index_info(EQ_INDEX_REPORT_BY_GET_APT_EQ_INDEX);
        }
        break;
#endif

    default:
        break;
    }
}
