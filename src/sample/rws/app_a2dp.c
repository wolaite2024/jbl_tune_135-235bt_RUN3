/*
 * Copyright (c) 2018, Realsil Semiconductor Corporation. All rights reserved.
 */

#include <string.h>
#include "trace.h"
#include "btm.h"
#include "bt_a2dp.h"
#include "app_cfg.h"
#include "app_main.h"
#include "app_multilink.h"
#include "app_roleswap.h"
#include "app_link_util.h"
#include "app_a2dp.h"
#include "app_bt_sniffing.h"
#include "app_sniff_mode.h"
#include "app_hfp.h"
#include "app_audio_policy.h"

static void app_a2dp_bt_cback(T_BT_EVENT event_type, void *event_buf, uint16_t buf_len)
{
    T_BT_EVENT_PARAM *param = event_buf;
    uint8_t active_a2dp_idx = app_get_active_a2dp_idx();
    T_APP_BR_LINK *p_active_a2dp_link = &app_db.br_link[active_a2dp_idx];
    bool handle = true;

    switch (event_type)
    {
    case BT_EVENT_A2DP_CONN_IND:
        {
            T_APP_BR_LINK *p_link = app_find_br_link(param->a2dp_conn_ind.bd_addr);
            if (p_link != NULL)
            {
                bt_a2dp_connect_cfm(p_link->bd_addr, true);
            }
        }
        break;

    case BT_EVENT_A2DP_CONFIG_CMPL:
        {
        }
        break;

    case BT_EVENT_A2DP_SNIFFING_CONN_CMPL:
        {
        }
        break;

    case BT_EVENT_A2DP_STREAM_OPEN:
        {
            // Stream Channel connected
            // For Gaming mode
            if (app_cfg_const.rws_gaming_mode == 1)
            {
                if (app_bt_sniffing_start(param->a2dp_stream_open.bd_addr, BT_SNIFFING_TYPE_A2DP)) {};
            }
        }
        break;

    case BT_EVENT_A2DP_STREAM_START_IND:
        {
            if ((p_active_a2dp_link->streaming_fg == false ||
                 p_active_a2dp_link->avrcp_play_status == BT_AVRCP_PLAY_STATUS_PAUSED) ||
                (memcmp(p_active_a2dp_link->bd_addr,
                        param->a2dp_stream_start_ind.bd_addr, 6) == 0))
            {
                APP_PRINT_INFO3("app_a2dp_bt_cback: BT_EVENT_A2DP_STREAM_START_IND active_a2dp_idx %d, streaming_fg %d, avrcp_play_status %d",
                                active_a2dp_idx, p_active_a2dp_link->streaming_fg, p_active_a2dp_link->avrcp_play_status);
                app_sniff_mode_b2s_disable_all(SNIFF_DISABLE_MASK_A2DP);

                app_audio_set_bud_stream_state(BUD_STREAM_STATE_AUDIO);
            }
        }
        break;

    case BT_EVENT_A2DP_STREAM_START_RSP:
        {
            if (p_active_a2dp_link->streaming_fg == false ||
                (memcmp(p_active_a2dp_link->bd_addr,
                        param->a2dp_stream_start_ind.bd_addr, 6) == 0))
            {
                APP_PRINT_INFO2("app_a2dp_bt_cback: BT_EVENT_A2DP_STREAM_START_RSP active_a2dp_idx %d, streaming_fg %d",
                                active_a2dp_idx, p_active_a2dp_link->streaming_fg);

                app_sniff_mode_b2s_disable_all(SNIFF_DISABLE_MASK_A2DP);

                app_audio_set_bud_stream_state(BUD_STREAM_STATE_AUDIO);
            }
        }
        break;

    case BT_EVENT_A2DP_STREAM_STOP:
        {
            if (memcmp(p_active_a2dp_link->bd_addr,
                       param->a2dp_stream_stop.bd_addr, 6) == 0)
            {
                if (app_find_a2dp_start_num() <= 1)
                {
                    app_sniff_mode_b2s_enable_all(SNIFF_DISABLE_MASK_A2DP);
                }

                if (app_hfp_get_call_status() == BT_HFP_CALL_IDLE)
                {
                    app_audio_set_bud_stream_state(BUD_STREAM_STATE_IDLE);
                }

            }
        }
        break;

    case BT_EVENT_A2DP_STREAM_CLOSE:
        {
            if (memcmp(p_active_a2dp_link->bd_addr,
                       param->a2dp_stream_close.bd_addr, 6) == 0)
            {
                if (app_hfp_get_call_status() == BT_HFP_CALL_IDLE)
                {
                    app_audio_set_bud_stream_state(BUD_STREAM_STATE_IDLE);
                }
            }
        }
        break;

    default:
        {
            handle = false;
        }
        break;
    }

    if (handle == true)
    {
        APP_PRINT_INFO1("app_a2dp_bt_cback: event_type 0x%04x", event_type);
    }
}

bool app_a2dp_is_streaming(void)
{
    T_APP_BR_LINK *p_link = NULL;

    for (uint8_t i = 0; i < MAX_BR_LINK_NUM; i++)
    {
        p_link = &app_db.br_link[i];

        if (p_link->streaming_fg == true)
        {
            return true;
        }
    }

    return false;
}

void app_a2dp_init(void)
{
    if (app_cfg_const.supported_profile_mask & A2DP_PROFILE_MASK)
    {
        APP_PRINT_TRACE2("app_a2dp_init refine 0x%08x,0x%08x,", app_cfg_const.aac_bit_rate,
                         (app_cfg_nv.cmd_aac_bit_rate * 1000));
#if F_APP_HARMAN_FEATURE_SUPPORT
        bt_a2dp_init(app_cfg_const.a2dp_link_number, app_cfg_nv.cmd_latency_in_ms);
        app_cfg_const.aac_bit_rate = (app_cfg_nv.cmd_aac_bit_rate * 1000);
        app_cfg_const.sbc_max_bitpool = app_cfg_nv.cmd_sbc_max_bitpool;
#else
        bt_a2dp_init(app_cfg_const.a2dp_link_number, A2DP_LATENCY_MS);
#endif
        bt_a2dp_role_set(BT_A2DP_ROLE_SNK);
        if (app_cfg_const.a2dp_codec_type_sbc)
        {
            T_BT_A2DP_STREAM_ENDPOINT sep;

            sep.codec_type = BT_A2DP_CODEC_TYPE_SBC;
            sep.u.codec_sbc.sampling_frequency_mask = app_cfg_const.sbc_sampling_frequency;
            sep.u.codec_sbc.channel_mode_mask = app_cfg_const.sbc_channel_mode;
            sep.u.codec_sbc.block_length_mask = app_cfg_const.sbc_block_length;
            sep.u.codec_sbc.subbands_mask = app_cfg_const.sbc_subbands;
            sep.u.codec_sbc.allocation_method_mask = app_cfg_const.sbc_allocation_method;
            sep.u.codec_sbc.min_bitpool = app_cfg_const.sbc_min_bitpool;
#if (F_APP_AVP_INIT_SUPPORT == 1)
            sep.u.codec_sbc.max_bitpool = (app_cfg_const.sbc_max_bitpool >= 48) ? 48 :
                                          app_cfg_const.sbc_max_bitpool;
#else
            sep.u.codec_sbc.max_bitpool = app_cfg_const.sbc_max_bitpool;
#endif
            bt_a2dp_stream_endpoint_set(sep);
        }

        if (app_cfg_const.a2dp_codec_type_aac)
        {
            T_BT_A2DP_STREAM_ENDPOINT sep;

            sep.codec_type = BT_A2DP_CODEC_TYPE_AAC;
            sep.u.codec_aac.object_type_mask = app_cfg_const.aac_object_type;
            sep.u.codec_aac.sampling_frequency_mask = app_cfg_const.aac_sampling_frequency;
            sep.u.codec_aac.channel_number_mask = app_cfg_const.aac_channel_number;
            sep.u.codec_aac.vbr_supported = app_cfg_const.aac_vbr_supported;
            sep.u.codec_aac.bit_rate = app_cfg_const.aac_bit_rate;
            bt_a2dp_stream_endpoint_set(sep);
        }

#if (F_APP_A2DP_CODEC_LDAC_SUPPORT == 1)
        if (app_cfg_const.a2dp_codec_type_ldac)
        {
            T_BT_A2DP_STREAM_ENDPOINT sep;

            sep.codec_type = BT_A2DP_CODEC_TYPE_LDAC;
            sep.u.codec_ldac.sampling_frequency_mask = app_cfg_const.ldac_sampling_frequency;
            sep.u.codec_ldac.channel_mode_mask = app_cfg_const.ldac_channel_mode;
            bt_a2dp_stream_endpoint_set(sep);
        }
#endif

        bt_mgr_cback_register(app_a2dp_bt_cback);
    }
}
