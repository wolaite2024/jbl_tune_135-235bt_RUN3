/**
*********************************************************************************************************
*               Copyright(c) 2020, Realtek Semiconductor Corporation. All rights reserved.
**********************************************************************************************************
* @file     app_playback.c
* @brief    This file provides playback application code.
* @details
* @author   lemon hua
* @date     2020-3-20
* @version  v1.0
*********************************************************************************************************
*/
#if F_APP_LOCAL_PLAYBACK_SUPPORT
/* Includes -----------------------------------------------------------------*/
#include "trace.h"
#include "os_mem.h"
#include "os_msg.h"
#include "os_task.h"
#include "audio_track.h"
#include "audio_type.h"
#include "btm.h"
#include "bt_hfp.h"
#include "bt_avrcp.h"
#include "sysm.h"
#include "app_cfg.h"
#include "app_main.h"
#include "app_mmi.h"
#include "app_bt_policy_api.h"
#include "app_bt_policy_int.h"
#include "app_multilink.h"
#include "app_hfp.h"
#include "gap_timer.h"
#include "audio.h"
#include "audio_fs.h"
#include "audio_fs_decode.h"
#include "app_playback.h"
#include "app_dlps.h"
#include "app_sniff_mode.h"

/* Defines ------------------------------------------------------------------*/
#define PLAYBACK_FRAMES_NUM_2           5   //(PLAYBACK_FRAMES_NUM_2 + PLAYBACK_LOW_LEVEL) < LATENCY pkts
#define PLAYBACK_LOW_LEVEL              60  //ms 3pkt
#define PLAYBACK_UPPER_LEVEL            300 //ms 15pkt
#define PLAYBACK_LATENCY                200 //ms 9pkt
#define APP_PLAYBACK_DISABLE_SNIFF_EN    1
/* Globals ------------------------------------------------------------------*/
FRAME_INFO *frame_info = NULL;
FRAME_CONTENT *playback_frame = NULL;
T_PLAYBACK_DATA playback_db;

T_AUDIO_TRACK_HANDLE playback_track_handle;
T_AUDIO_FS_HANDLE playback_audio_fs_handle;

static uint8_t app_sd_pd_timer_queue_id = 0;
static void *power_down_sd_timer_handle = NULL;
static uint8_t app_playback_dlps_timer_queue_id = 0;
static void *playback_dlps_timer_handle = NULL;
#if APP_PLAYBACK_DISABLE_SNIFF_EN
static uint8_t s_playback_peer_addr[6];
#endif
/**
  * @brief Initialize FATFS file system and sd card physical layer.
  * @param   No parameter.
  * @return  void
  */
uint8_t app_playback_parameter_recfg(void)
{
    uint8_t res = PLAYBACK_SUCCESS;
    uint16_t u16_res = 0;

    uint32_t sample_rate = 0;
    uint8_t channel_mode = 0;
    uint32_t bit_rate = 0;

    u16_res = audio_fs_get_frame_para(playback_audio_fs_handle, frame_info);
    if (u16_res == 0)
    {
//        T_AUDIO_STREAM_USAGE usage = AUDIO_STREAM_USAGE_SNOOP;
//        T_AUDIO_STREAM_MODE mode = AUDIO_STREAM_MODE_NORMAL;
        T_AUDIO_FORMAT_INFO format_info;
        uint32_t device = AUDIO_DEVICE_OUT_SPK;

        if ((frame_info->format == RTK) && (frame_info->format_info.rtk.rtkTransFormat == RTK_SBC))
        {
            format_info.type = AUDIO_FORMAT_TYPE_SBC;
            device = AUDIO_DEVICE_OUT_DEFAULT;
            format_info.attr.sbc.sample_rate =
                frame_info->format_info.rtk.rtk_trans_info.sbc.sampling_frequency;
            format_info.attr.sbc.block_length = frame_info->format_info.rtk.rtk_trans_info.sbc.block_length;
            format_info.attr.sbc.subband_num = frame_info->format_info.rtk.rtk_trans_info.sbc.subbands;
            format_info.attr.sbc.allocation_method =
                frame_info->format_info.rtk.rtk_trans_info.sbc.allocation_method;
            format_info.attr.sbc.bitpool = frame_info->format_info.rtk.rtk_trans_info.sbc.bitpool;
            switch (frame_info->format_info.rtk.rtk_trans_info.sbc.channel_mode)
            {
            case 0:
                format_info.attr.sbc.chann_mode = AUDIO_SBC_CHANNEL_MODE_MONO; // Mono
                break;
            case 1:
                format_info.attr.sbc.chann_mode = AUDIO_SBC_CHANNEL_MODE_DUAL; // Mono
                break;
            case 2:
                format_info.attr.sbc.chann_mode = AUDIO_SBC_CHANNEL_MODE_STEREO; // Stereo
                break;
            case 3:
                format_info.attr.sbc.chann_mode = AUDIO_SBC_CHANNEL_MODE_JOINT_STEREO; // Stereo
                break;
            default:
                APP_PRINT_ERROR0("channel not supprot");
                break;
            }
            sample_rate = format_info.attr.sbc.sample_rate;
            APP_PRINT_TRACE6("app_playback_parameter_recfg: RTK_SBC, samplefre:%d, channel_mode:%d,"
                             "block_length:%d, subbands:%d, allocation_method:%d, bitpool:%d",
                             format_info.attr.sbc.sample_rate, format_info.attr.sbc.chann_mode,
                             format_info.attr.sbc.block_length,
                             format_info.attr.sbc.subband_num, format_info.attr.sbc.allocation_method,
                             format_info.attr.sbc.bitpool);
        }
        else if ((frame_info->format == AAC) ||
                 ((frame_info->format == RTK) && (frame_info->format_info.rtk.rtkTransFormat == RTK_LATM)) ||
                 ((frame_info->format == RTK) && (frame_info->format_info.rtk.rtkTransFormat == RTK_ADTS)))
        {
            format_info.type = AUDIO_FORMAT_TYPE_AAC;
            device = AUDIO_DEVICE_OUT_SPK;

            if ((frame_info->format == RTK) && (frame_info->format_info.rtk.rtkTransFormat == RTK_ADTS))
            {
                format_info.attr.aac.transport_format = AUDIO_AAC_TRANSPORT_FORMAT_ADTS;

                sample_rate = frame_info->format_info.rtk.rtk_trans_info.adts.sampling_frequency;
                channel_mode = frame_info->format_info.rtk.rtk_trans_info.adts.channel_mode;
                bit_rate = frame_info->format_info.rtk.rtk_trans_info.adts.bit_rate;
            }
            else if ((frame_info->format == RTK) && (frame_info->format_info.rtk.rtkTransFormat == RTK_LATM))
            {
                format_info.attr.aac.transport_format = AUDIO_AAC_TRANSPORT_FORMAT_LATM;

                sample_rate = frame_info->format_info.rtk.rtk_trans_info.latm.sampling_frequency;
                channel_mode = frame_info->format_info.rtk.rtk_trans_info.latm.channel_mode;
                bit_rate = 0;
            }
            else if (frame_info->format == AAC)
            {
                format_info.attr.aac.transport_format = AUDIO_AAC_TRANSPORT_FORMAT_ADTS;

                sample_rate = frame_info->format_info.aac.sampling_frequency;
                channel_mode = frame_info->format_info.aac.channel_mode;
                bit_rate = 0;
            }

            format_info.attr.aac.sample_rate = sample_rate;
            format_info.attr.aac.bitrate = bit_rate;
            switch (channel_mode)
            {
            case AOT_SPECIFIC_CONFIG:
                format_info.attr.aac.chann_num = 1; // Mono
                break;
            case CHANNEL_1PCS:
            case CHANNEL_2PCS:
            case CHANNEL_3PCS:
            case CHANNEL_4PCS:
            case CHANNEL_5PCS:
            case CHANNEL_6PCS:
            case CHANNEL_7PCS:
                format_info.attr.aac.chann_num = 2; // Stereo
                break;
            default:
                APP_PRINT_ERROR0("channel not supprot");
                break;
            }
            APP_PRINT_TRACE4("app_playback_parameter_recfg: AAC,"
                             " transport_format:%d, samplefrequency:%d, channel_mode:%d, bitrate:%d",
                             format_info.attr.aac.transport_format, format_info.attr.aac.sample_rate,
                             format_info.attr.aac.chann_num,
                             format_info.attr.aac.bitrate);
        }
        else if (frame_info->format == MP3)
        {
            APP_PRINT_ERROR0("MP3 not supprot");
            format_info.type = AUDIO_FORMAT_TYPE_MP3;
            format_info.attr.mp3.sample_rate = frame_info->format_info.mp3.sampling_frequency;
            format_info.attr.mp3.bitrate = frame_info->format_info.mp3.bit_rate;
            switch (frame_info->format_info.mp3.channel_mode)
            {
            case 0:
                format_info.attr.mp3.chann_mode = AUDIO_MP3_CHANNEL_MODE_STEREO;
                break;
            case 1:
                format_info.attr.mp3.chann_mode = AUDIO_MP3_CHANNEL_MODE_JOINT_STEREO;
                break;
            case 2:
                format_info.attr.mp3.chann_mode = AUDIO_MP3_CHANNEL_MODE_DUAL;
                break;
            case 3:
                format_info.attr.mp3.chann_mode = AUDIO_MP3_CHANNEL_MODE_MONO;
                break;

            default:
                break;
            }
            sample_rate = format_info.attr.mp3.sample_rate;
        }
        playback_db.sample_rate = sample_rate;

        playback_track_handle = audio_track_create(AUDIO_STREAM_TYPE_PLAYBACK, //stream_type
                                                   AUDIO_STREAM_MODE_NORMAL, // mode
                                                   AUDIO_STREAM_USAGE_SNOOP, // usage
                                                   format_info, //format_info
                                                   playback_db.volume, //volume
                                                   0,
                                                   device, // device
                                                   NULL,
                                                   NULL);
        app_playback_get_audio_track_state_handle();

        if (playback_track_handle != NULL)
        {
            audio_track_latency_set(playback_track_handle, PLAYBACK_LATENCY, true);
        }
    }
    else
    {
        res = PLAYBACK_READ_ERROR;
    }
    return res;
}

uint8_t app_playback_mode_init(void)
{
    if (frame_info != NULL)
    {
        os_mem_free(frame_info);
    }
    frame_info = (FRAME_INFO *)malloc(sizeof(FRAME_INFO));

    if (playback_frame != NULL)
    {
        os_mem_free(playback_frame);
    }
    playback_frame = (FRAME_CONTENT *)malloc(sizeof(FRAME_CONTENT));

    app_sd_card_power_down_disable(APP_SD_POWER_DOWN_ENTER_CHECK_PLAYBACK);
    audio_fs_set_playlist(playback_db.playlist_idx);
    app_sd_card_power_down_enable(APP_SD_POWER_DOWN_ENTER_CHECK_PLAYBACK);

    playback_db.sd_play_state = PLAYBACK_STATE_PAUSE;
    playback_db.op_next_action = PLAYBACK_STOPPED_IDLE_ACTION;
#if APP_PLAYBACK_DISABLE_SNIFF_EN
    remote_peer_addr_get(s_playback_peer_addr);
#endif

    return 0;
}

/**
  * @brief playback start function
  * @param   No parameter.
  * @return  void
  */
uint8_t app_playback_start(void)
{
    uint8_t res = PLAYBACK_SUCCESS;

    APP_PRINT_TRACE0("app_playback_start ++");
#if APP_PLAYBACK_DISABLE_SNIFF_EN
    remote_peer_addr_get(s_playback_peer_addr);
    app_sniff_mode_b2s_disable(s_playback_peer_addr, SNIFF_DISABLE_MASK_LP);
#endif
    app_sd_card_dlps_disable(APP_SD_DLPS_ENTER_CHECK_PLAYBACK);
    app_sd_card_power_down_disable(APP_SD_POWER_DOWN_ENTER_CHECK_PLAYBACK);
    app_playback_parameter_recfg();
    audio_track_threshold_set(playback_track_handle, PLAYBACK_UPPER_LEVEL, PLAYBACK_LOW_LEVEL);
    app_playback_volume_set();
    playback_db.sd_play_state = PLAYBACK_STATE_PLAY;
    playback_db.op_next_action = PLAYBACK_STOPPED_IDLE_ACTION;
    playback_db.seq_num = 0;
    playback_db.local_track_state = PLAYBACK_TRACK_STATE_CLEAR;
    playback_db.sec_track_state = PLAYBACK_TRACK_STATE_CLEAR;
    playback_db.buffer_state = PLAYBACK_BUF_NORMAL;
    audio_track_start(playback_track_handle);

    return res;
}

void app_playback_volume_up(void)
{
    uint8_t max_volume;
    uint8_t curr_volume;

    if (playback_track_handle == NULL)
    {
        return;
    }

    audio_track_volume_out_max_get(playback_track_handle, (uint8_t *)&max_volume);
    audio_track_volume_out_get(playback_track_handle, (uint8_t *)&curr_volume);

    APP_PRINT_TRACE2("app_playback_volume_up,volume:%d,max:%d", curr_volume, max_volume);
    if (curr_volume < max_volume)
    {
        curr_volume++;
        audio_track_volume_out_set(playback_track_handle, curr_volume);
        playback_db.volume = curr_volume;
    }
    else
    {
        audio_track_volume_out_set(playback_track_handle, max_volume);
        playback_db.volume = max_volume;
    }
}
void app_playback_volume_down(void)
{
    uint8_t min_volume;
    uint8_t curr_volume;

    if (playback_track_handle == NULL)
    {
        return;
    }

    audio_track_volume_out_min_get(playback_track_handle, (uint8_t *)&min_volume);
    audio_track_volume_out_get(playback_track_handle, (uint8_t *)&curr_volume);

    APP_PRINT_TRACE2("app_playback_volume_down,volume:%d,min:%d", curr_volume, min_volume);
    if (curr_volume > min_volume)
    {
        curr_volume--;
        audio_track_volume_out_set(playback_track_handle, curr_volume);
        playback_db.volume = curr_volume;
    }
    else
    {
        audio_track_volume_out_set(playback_track_handle, min_volume);
        playback_db.volume = min_volume;
    }
}

void app_playback_volume_set(void)
{
    APP_PRINT_TRACE1("app_playback_volume_set vol:%d", playback_db.volume);
    audio_track_volume_out_set(playback_track_handle, playback_db.volume);
}

/**
  * @brief playback stop function
  * @param   No parameter.
  * @return  void
  */
uint8_t app_playback_stop(void)
{
    uint8_t res = PLAYBACK_SUCCESS;

    gap_stop_timer(&playback_db.put_data_timer_handle);
    playback_db.sd_play_state = PLAYBACK_STATE_PAUSE;
    if (playback_track_handle != NULL)
    {
        playback_db.local_track_state = PLAYBACK_TRACK_STATE_CLEAR;
        audio_track_release(playback_track_handle);
//        playback_track_handle = NULL;
    }
    app_sd_card_dlps_enable(APP_SD_DLPS_ENTER_CHECK_PLAYBACK);
    app_sd_card_power_down_enable(APP_SD_POWER_DOWN_ENTER_CHECK_PLAYBACK);
#if APP_PLAYBACK_DISABLE_SNIFF_EN
    app_sniff_mode_b2s_enable(s_playback_peer_addr, SNIFF_DISABLE_MASK_LP);
#endif

    return res;
}
/**
  * @brief .
  * @param   No parameter.
  * @return  void
  */
uint8_t app_playback_mode_deinit(void)
{
    app_playback_stop();
    if (frame_info != NULL)
    {
        os_mem_free(frame_info);
        frame_info = NULL;
    }

    if (playback_frame != NULL)
    {
        os_mem_free(playback_frame);
        playback_frame = NULL;
    }
    app_sd_card_dlps_enable(APP_SD_DLPS_ENTER_CHECK_PLAYBACK);
    app_sd_card_power_down_enable(APP_SD_POWER_DOWN_ENTER_CHECK_PLAYBACK);
#if APP_PLAYBACK_DISABLE_SNIFF_EN
    app_sniff_mode_b2s_enable(s_playback_peer_addr, SNIFF_DISABLE_MASK_LP);
#endif

    APP_PRINT_TRACE0("app_playback_mode_deinit,free success!!!");

    return 0;
}

/**
  * @brief playback set file offset
  * @param   file offset.
  * @return  void
  */
static void app_playback_set_file_offset(uint8_t rd_wr_type, uint32_t offset)
{
    audio_fs_set_file_offset(playback_audio_fs_handle, offset);
    APP_PRINT_TRACE1("app_playback_set_file_offset, offset: 0x%x", offset);
}
/**
  * @brief playback one frame
  * @param   No parameter.
  * @return  void
  */
static uint16_t app_playback_set_playlist(uint8_t play_mode)
{
    uint16_t res = 0;

    switch (play_mode)
    {
    case SINGLE_FILE:
        // fileInfo.g_file_cur not change
        if ((playback_db.playlist_idx < 1) || (playback_db.playlist_idx > PLAYBACK_PLAYLIST_NUM))
        {
            playback_db.playlist_idx = 1;
        }
        break;
    case NEXT_FILE:
        playback_db.playlist_idx++;
        if ((playback_db.playlist_idx < 1) || (playback_db.playlist_idx > PLAYBACK_PLAYLIST_NUM))
        {
            playback_db.playlist_idx = 1;
        }
        break;
    case PREVERSE_FILE:
        if (playback_db.playlist_idx == 1)
        {
            playback_db.playlist_idx = PLAYBACK_PLAYLIST_NUM;
        }
        else if ((playback_db.playlist_idx < 1) || (playback_db.playlist_idx > PLAYBACK_PLAYLIST_NUM))
        {
            playback_db.playlist_idx = 1;
        }
        else
        {
            playback_db.playlist_idx--;
        }
        break;
    case RANDOM_FILE:
//        file_idx = rand() % (g_playlist_file_count - 1);
        break;

    default:
        playback_db.playlist_idx = 1;
        break;
    }

    res = audio_fs_set_playlist(playback_db.playlist_idx);
    APP_PRINT_TRACE3("app_playback_set_playlist,play_mode: %d,file_idx: %d, res: 0x%x",
                     play_mode, playback_db.playlist_idx, res);

    return res;
}
static uint8_t app_playback_update_playlist(uint8_t playlist_mode)
{
    uint8_t res = 0;
    uint8_t playlist_cnt = 0;
    uint8_t playlist_mode_tmp = playlist_mode;
    uint16_t playlist_file_count;

    do
    {
        res = app_playback_set_playlist(playlist_mode_tmp);
        playlist_file_count = audio_fs_get_filecount_from_playlist();
        playlist_cnt++;
        if (((playlist_mode_tmp == SINGLE_FILE) && (res != 0)) || \
            ((playlist_mode_tmp == SINGLE_FILE) && (playlist_file_count == 0)))
        {
            playlist_mode_tmp = NEXT_FILE;
        }

    }
    while (((playlist_cnt < PLAYBACK_PLAYLIST_NUM) && (res != 0)) || \
           ((playlist_cnt < PLAYBACK_PLAYLIST_NUM) && (playlist_file_count == 0)));

    if ((res != 0) || (playlist_cnt > PLAYBACK_PLAYLIST_NUM) || (playlist_file_count == 0))
    {
        res = PLAYBACK_PLAYLIST_ERROR;
    }
    APP_PRINT_TRACE3("app_playback_update_playlist, playlist_cnt:%d, playlist_idx:%d, file_cnt:%d",
                     playlist_cnt, playback_db.playlist_idx, playlist_file_count);
    return res;
}

static uint8_t app_playback_get_index_file(T_PLAYBACK_FILE play_mode, uint16_t *index)
{
    uint8_t res = PLAYBACK_SUCCESS;
    uint16_t playlist_file_count;

    playlist_file_count = audio_fs_get_filecount_from_playlist();
    switch (play_mode)
    {
    case SINGLE_FILE:
        if (playback_db.file_idx >= playlist_file_count)
        {
            playback_db.file_idx = 0;
        }
        break;
    case NEXT_FILE:
        playback_db.file_idx++;
        if (playback_db.file_idx >= playlist_file_count)
        {
            playback_db.file_idx = 0;
        }
        break;
    case PREVERSE_FILE:
        if (playback_db.file_idx == 0)
        {
            playback_db.file_idx = playlist_file_count - 1;
        }
        else if (playback_db.file_idx >= playlist_file_count)
        {
            playback_db.file_idx = 0;
        }
        else
        {
            playback_db.file_idx--;
        }
        break;
    case RANDOM_FILE:
//        file_idx = rand() % (g_playlist_file_count - 1);
        break;
    case RETURN_IDX:
        *index = playback_db.file_idx;
        return res;

    default:
        playback_db.file_idx = 0;
        break;
    }

    *index = playback_db.file_idx;
    if (playlist_file_count == 0)
    {
        res = PLAYBACK_PLAYLIST_ERROR;
    }
    APP_PRINT_TRACE3("app_playback_open_file, play_mode: %d,file_idx: %d, total file:%d",
                     play_mode, playback_db.file_idx, playlist_file_count);

    return res;
}
static uint8_t app_playback_open_and_get_file_info(uint8_t play_mode, uint8_t *file_name,
                                                   uint16_t *p_name_len,
                                                   uint32_t *p_playback_offset)
{
    uint16_t res = PLAYBACK_SUCCESS;
    uint16_t file_index;
    uint8_t filename[FILE_NAME_LEN]  = {0};
    uint16_t name_len;
    uint8_t play_mode_tmp = play_mode;
    uint16_t open_file_cnt = 0;

    if (playback_audio_fs_handle != NULL)
    {
        if (0 != audio_fs_close(playback_audio_fs_handle))
        {
//            return;
        }
        playback_audio_fs_handle = NULL;
    }

    do
    {
        res = app_playback_get_index_file((T_PLAYBACK_FILE)play_mode_tmp, &file_index);
        if (res != PLAYBACK_SUCCESS)
        {
            res = PLAYBACK_PLAYLIST_ERROR;
            goto L_Return;
        }
        res = audio_fs_get_fileinfo(file_index, filename, &name_len);
        if (res != 0)
        {
            res = PLAYBACK_PLAYLIST_ERROR;
            goto L_Return;
        }
        memcpy(file_name, filename, name_len);
        *p_name_len = name_len;

        playback_audio_fs_handle = audio_fs_open(file_name, *p_name_len, FA_OPEN_EXISTING | FA_READ);
        if (playback_audio_fs_handle == NULL)
        {
            open_file_cnt++;
            if (SINGLE_FILE == play_mode_tmp)
            {
                play_mode_tmp = NEXT_FILE;
            }
            res = PLAYBACK_PLAYLIST_ERROR;
        }
    }
    while ((playback_audio_fs_handle == NULL) &&
           (open_file_cnt < audio_fs_get_filecount_from_playlist()));

    if (playback_audio_fs_handle != NULL)
    {
        *p_playback_offset = audio_fs_get_file_offset(playback_audio_fs_handle);
    }
    else
    {
        *p_playback_offset = 0;
    }
    APP_PRINT_TRACE3("app_playback_get_file_info,playback_offset:0x%x,name_len:%d, open_file_cnt:%d",
                     *p_playback_offset, *p_name_len, open_file_cnt);
    return res;
L_Return:
    APP_PRINT_ERROR1("app_playback_open_and_get_file_info fail, res:0x%x", res);
    return res;
}

void app_playback_put_data(uint8_t pkt_num)
{
    uint16_t res = 0;
    uint8_t frame_cnt = 0;
    uint16_t time_ms = 0;

    APP_PRINT_TRACE1("app_playback_put_data pkt_num: %d", pkt_num);
    while ((frame_cnt < pkt_num) && (res != FS_END_OF_FILE))
    {
        res = audio_fs_get_frame(playback_audio_fs_handle, playback_frame);
        if (res != 0)
        {
            APP_PRINT_ERROR1("app_playback_put_data ERROR,RES:0x%x", res);
            break;
        }

        uint16_t written_len;
        playback_db.seq_num++;
        if (audio_track_write(playback_track_handle,
                              0,//              timestamp,
                              playback_db.seq_num,
                              AUDIO_STREAM_STATUS_CORRECT,
                              playback_frame->frameNum,//            frame_num,
                              playback_frame->content,
                              playback_frame->length,
                              &written_len) == false)
        {
            res = PLAYBACK_WRITE_ERROR;
            break;
        }
        frame_cnt++;
    }
    // This maybe AUDIO_EVENT_TRACK_BUFFER_HIGH event
    time_ms = PLAYBACK_PUT_DATA_TIME_MS;
    if (playback_db.buffer_state == PLAYBACK_BUF_HIGH)
    {
        time_ms = PLAYBACK_PUT_DATA_TIME_MS * 2;
    }
    playback_db.buffer_state = PLAYBACK_BUF_NORMAL;

    if (res == FS_END_OF_FILE)
    {
        gap_stop_timer(&playback_db.put_data_timer_handle);
    }
    else if (playback_db.sd_play_state == PLAYBACK_STATE_PLAY)
    {
        app_playback_put_data_start_timer(time_ms);
    }
}

//==================================couple relay========================================//
static void app_playback_couple_sync_file_info_handle(uint8_t *filename, uint16_t name_len,
                                                      uint32_t playback_offset)
{
    uint16_t len;
    T_PLAYBACK_INFO_TYPE2 couple_sync_info;

    len = sizeof(T_PLAYBACK_INFO_TYPE2) + name_len + 1; // 1:string end 0x00
    uint8_t *couple_sync_buf = (uint8_t *)malloc(len);
    if (couple_sync_buf == NULL)
    {
        return;
    }
    else
    {
        memset(couple_sync_buf, 0, len);
        memset(&couple_sync_info, 0, sizeof(T_PLAYBACK_INFO_TYPE2));
        couple_sync_info.file_crc = 0x1234;

        couple_sync_info.file_idx       = playback_db.file_idx;
        couple_sync_info.file_offset    = playback_offset;
        couple_sync_info.name_len       = name_len;
        couple_sync_info.playlist_idx   = playback_db.playlist_idx;
        couple_sync_info.volume         = playback_db.volume;

        memcpy(couple_sync_buf, (uint8_t *)&couple_sync_info, sizeof(T_PLAYBACK_INFO_TYPE2));
        memcpy(couple_sync_buf + sizeof(T_PLAYBACK_INFO_TYPE2), filename, name_len);
        remote_async_msg_relay(playback_db.relay_handle, PLAYBACK_REMOTE_MSG_CHECK_FILE_SYNC,
                               couple_sync_buf,
                               len, false);

    }
    APP_PRINT_TRACE4("app_playback_couple_sync_file_info_handle,file_idx:%d, 0x%x,0x%x,%b",
                     playback_db.file_idx, playback_offset, len, TRACE_BINARY(len, couple_sync_buf));
    os_mem_free(couple_sync_buf);

}
static uint8_t app_playback_relay_check_sync_handle(uint8_t *buf, uint16_t len)
{
    uint8_t res = PLAYBACK_SUCCESS;
    T_PLAYBACK_INFO_TYPE2 couple_sync_info;
    uint8_t *p_file_name;

    uint16_t loc_file_crc;

    memcpy(&couple_sync_info, buf, sizeof(T_PLAYBACK_INFO_TYPE2));

    p_file_name = buf + sizeof(T_PLAYBACK_INFO_TYPE2);
    APP_PRINT_TRACE3("app_playback_relay_check_sync_handle -idx:%d,file_name_len:%d, p_file_name:%b",
                     couple_sync_info.file_idx, couple_sync_info.name_len,
                     TRACE_BINARY(couple_sync_info.name_len, p_file_name));

    if (playback_audio_fs_handle != NULL)
    {
        if (0 != audio_fs_close(playback_audio_fs_handle))
        {
            res = PLAYBACK_CLOSE_FILE_ERROR;
            goto PB_RETURN_ERR;
        }
        playback_audio_fs_handle = NULL;
    }
    playback_audio_fs_handle = audio_fs_open(p_file_name, couple_sync_info.name_len,
                                             FA_OPEN_EXISTING | FA_READ);
    if (playback_audio_fs_handle == NULL)
    {
        res = PLAYBACK_OPEN_FILE_ERROR;
        goto PB_RETURN_ERR;
    }
    loc_file_crc = 0x1234;
    if (loc_file_crc != couple_sync_info.file_crc)
    {
        res = PLAYBACK_CRC_ERROR;
        goto PB_RETURN_ERR;
    }
    playback_db.file_idx = couple_sync_info.file_idx;
    app_playback_set_file_offset(0, couple_sync_info.file_offset);
    playback_db.volume = couple_sync_info.volume;
    APP_PRINT_TRACE2("app_playback_relay_check_sync_handle, vol:%d, plist %d ",
                     couple_sync_info.volume, couple_sync_info.playlist_idx);

    return res;

PB_RETURN_ERR:
    APP_PRINT_ERROR1("app_playback_relay_check_sync_handle fail,res:0x%x", res);
    return res;
}

// switch next file and sync(fail) switch fail
static uint8_t app_playback_relay_switch_sync_handle(void)
{
    uint8_t res = PLAYBACK_SUCCESS;
    uint32_t playback_offset;
    uint8_t filename[FILE_NAME_LEN]  = {0};
    uint16_t name_len;

    APP_PRINT_TRACE0("app_playback_relay_switch_sync_handle, NEXT_FILE");

    res = app_playback_open_and_get_file_info(NEXT_FILE, filename, &name_len, &playback_offset);
    if (res != PLAYBACK_SUCCESS)
    {
        return res;
    }
    app_playback_couple_sync_file_info_handle(filename, name_len, playback_offset);

    return res;
}
static void app_playback_relay_check_result_handle(uint8_t *buf, uint16_t len)
{
    static uint16_t check_cnt = 0;
    uint8_t action = 0;

    APP_PRINT_TRACE1("app_playback_relay_check_result_handle, result:%d", *((uint8_t *)buf));
    if (*((uint8_t *)buf + 0) == PLAYBACK_SUCCESS)
    {
        check_cnt = 0;
        remote_sync_msg_relay(playback_db.relay_handle, PLAYBACK_REMOTE_MSG_START_SYNC, (uint8_t *)&action,
                              sizeof(action), REMOTE_TIMER_HIGH_PRECISION, 40, false);
    }
    else
    {
        uint16_t res = audio_fs_close(playback_audio_fs_handle);
        playback_audio_fs_handle = NULL;
        check_cnt++;
        if (check_cnt < audio_fs_get_filecount_from_playlist())
        {
            app_playback_relay_switch_sync_handle();
        }
        else
        {
            APP_PRINT_ERROR1("app_playback_relay_check_result_handle,"\
                             " pri and sec don't have same file,check_cnt:%d", check_cnt);
            check_cnt = 0;
        }
    }

}

/**
  * @brief   if enage success, now will stop and after rington, sync couple state and restart
  * @param   state, PLAYBACK_STOP will quit playback mode, PLAYBACK_START will restart playback.
  * @return  void
  */
void app_playback_single_to_couple_sync_handle(T_PLAYBACK_PAUSE state)
{
    static uint8_t sd_play_state_old = PLAYBACK_STATE_PAUSE;
    uint8_t buf_len;
    uint8_t *data_buf;

    T_PLAYBACK_INFO sync_info;
    memset(&sync_info, 0x00, sizeof(T_PLAYBACK_INFO));

    APP_PRINT_TRACE3("app_playback_single_to_couple_sync_handle:state:%d-mode:%d, sd_play_state:%d",
                     state, app_db.sd_playback_switch, playback_db.sd_play_state);
    if (state == PLAYBACK_STOP)
    {
        sd_play_state_old = playback_db.sd_play_state;
        if ((app_db.sd_playback_switch == 1) && (playback_db.sd_play_state == PLAYBACK_STATE_PLAY))
        {
            app_playback_stop();
        }
    }
    else if (state == PLAYBACK_START)
    {
        // check state and get some info
        if ((app_db.sd_playback_switch == 1) && (playback_audio_fs_handle != NULL))
        {
            uint8_t *p_file_name = audio_fs_get_filename(playback_audio_fs_handle);
            sync_info.name_len = audio_fs_get_filenameLen(playback_audio_fs_handle);
            sync_info.file_offset = audio_fs_get_file_offset(playback_audio_fs_handle);

            APP_PRINT_TRACE3("app_playback_single_to_couple_sync_handle"\
                             "file_offset:0x%x name_len %d, name %b", sync_info.file_offset,
                             sync_info.name_len, TRACE_BINARY(sync_info.name_len, p_file_name));
            sync_info.switch_mode = app_db.sd_playback_switch;
            sync_info.play_state = sd_play_state_old;
            sync_info.file_crc = 0x1234;
            sync_info.playlist_idx = playback_db.playlist_idx;
            sync_info.volume = playback_db.volume;

            buf_len = sizeof(T_PLAYBACK_INFO) + sync_info.name_len + 1; // 1:string end 0x00
            data_buf = (uint8_t *)malloc(buf_len);
            memset(data_buf, 0, buf_len);

            memcpy(data_buf, (uint8_t *)&sync_info, sizeof(T_PLAYBACK_INFO));
            memcpy((data_buf + sizeof(T_PLAYBACK_INFO)), p_file_name, sync_info.name_len);
        }
        else
        {
            buf_len = sizeof(T_PLAYBACK_INFO);
            data_buf = (uint8_t *)malloc(buf_len);
            memset(data_buf, 0x00, buf_len);
            sync_info.switch_mode = app_db.sd_playback_switch;
            sync_info.playlist_idx = playback_db.playlist_idx;
            sync_info.volume = playback_db.volume;
            memcpy(data_buf, (uint8_t *)&sync_info, sizeof(T_PLAYBACK_INFO));
        }

        APP_PRINT_TRACE2("app_playback_single_to_couple_sync_handle,:%d,%b", buf_len,
                         TRACE_BINARY(buf_len, data_buf));
        if (remote_session_state_get() == REMOTE_SESSION_STATE_CONNECTED) // connected
        {
            if (app_cfg_nv.bud_role == REMOTE_SESSION_ROLE_PRIMARY)
            {
                remote_async_msg_relay(playback_db.relay_handle, PLAYBACK_REMOTE_MSG_SINGLE_COUPLE_PRI_SYNC,
                                       data_buf,
                                       buf_len, false);
            }
            else if (app_cfg_nv.bud_role == REMOTE_SESSION_ROLE_SECONDARY)
            {
                remote_async_msg_relay(playback_db.relay_handle, PLAYBACK_REMOTE_MSG_SINGLE_COUPLE_SEC_SYNC,
                                       data_buf,
                                       buf_len, false);
            }
        }
        os_mem_free(data_buf);
    }

}

static void app_playback_couple_restart_sync(uint8_t *filename, uint16_t name_len,
                                             uint32_t playback_offset)
{
    APP_PRINT_TRACE2("app_playback_couple_restart_sync-playback_offset:%b,0x%x",
                     TRACE_BINARY(name_len, filename), playback_offset);
    if (playback_audio_fs_handle != NULL)
    {
        if (0 != audio_fs_close(playback_audio_fs_handle))
        {
//            APP_PRINT_ERROR1("app_playback_open_and_get_file_info, close FAIL,(%p)",
//                             playback_audio_fs_handle);
        }
        playback_audio_fs_handle = NULL;
    }
    playback_audio_fs_handle = audio_fs_open(filename, name_len, FA_OPEN_EXISTING | FA_READ);
    if (playback_audio_fs_handle == NULL)
    {
        APP_PRINT_ERROR0("app_playback_couple_restart_sync-open fail");
        return;
    }
    app_playback_set_file_offset(0, playback_offset);
    app_playback_couple_sync_file_info_handle(filename, name_len, playback_offset);
}
//playback mode(1byte) | sd_play_state(1byte) | \
//playback_offset(4byte) | file_crc_val(2byte) | file_name_len(2byte) | filename(...)
static void app_playback_relay_single_to_couple_sync_handle(uint16_t event, uint8_t *data_buf,
                                                            uint16_t len)
{
    uint8_t res = PLAYBACK_SUCCESS;
    static bool b_pri_engage = false, b_sec_engage = false;
    static uint8_t *pri_file_name = NULL, *sec_file_name = NULL;
    static T_PLAYBACK_INFO pri_info = {0, 0, PLAYBACK_STATE_PAUSE, 0, 0, 1, 7};
    static T_PLAYBACK_INFO sec_info = {0, 0, PLAYBACK_STATE_PAUSE, 0, 0, 1, 7};

    if (event == PLAYBACK_REMOTE_MSG_SINGLE_COUPLE_PRI_SYNC)
    {
        b_pri_engage = true;
        memcpy((uint8_t *)&pri_info, data_buf, sizeof(T_PLAYBACK_INFO));

        if ((pri_info.name_len != 0) && pri_info.switch_mode)
        {
            if (pri_file_name != NULL)
            {
                os_mem_free(pri_file_name);
                pri_file_name = NULL;
            }
            pri_file_name = (uint8_t *)malloc(pri_info.name_len);
            memset(pri_file_name, 0, pri_info.name_len);
            memcpy(pri_file_name, (data_buf + sizeof(T_PLAYBACK_INFO)), pri_info.name_len);
            APP_PRINT_TRACE2("file_name_len:%d, p_file_name:%b", pri_info.name_len,
                             TRACE_BINARY(pri_info.name_len, pri_file_name));
        }
    }
    else if (event == PLAYBACK_REMOTE_MSG_SINGLE_COUPLE_SEC_SYNC)
    {
        b_sec_engage = true;

        memcpy((uint8_t *)&sec_info, data_buf, sizeof(T_PLAYBACK_INFO));

        if ((sec_info.name_len != 0) && sec_info.switch_mode)
        {
            if (sec_file_name != NULL)
            {
                os_mem_free(sec_file_name);
                sec_file_name = NULL;
            }
            sec_file_name = (uint8_t *)malloc(sec_info.name_len);
            memset(sec_file_name, 0, sec_info.name_len);
            memcpy(sec_file_name, (data_buf + sizeof(T_PLAYBACK_INFO)), sec_info.name_len);
            APP_PRINT_TRACE2("file_name_len:%d, p_file_name:%b", sec_info.name_len,
                             TRACE_BINARY(sec_info.name_len, sec_file_name));
        }
    }
    APP_PRINT_TRACE4("playlist_idx:0x%x,pri volume:0x%x,playlist_idx%x,sec volume%x",
                     pri_info.playlist_idx, pri_info.volume, sec_info.playlist_idx, sec_info.volume);
    APP_PRINT_TRACE4("pri_file_offset:0x%x,pri_file_crc:0x%x,sec_file_offset0x%x,sec_file_crc0x%x",
                     pri_info.file_offset, pri_info.file_crc, sec_info.file_offset, sec_info.file_crc);
    APP_PRINT_TRACE4("pri_switch:%d, pri_play_state:%d, sec_switch:%d, sec_play_state:%d",
                     pri_info.switch_mode, pri_info.play_state, sec_info.switch_mode, sec_info.play_state);

    //check and sync info
    if (b_pri_engage && b_sec_engage) // all ready
    {
        //check playback mode
        if ((pri_info.switch_mode == sec_info.switch_mode) &&
            (pri_info.switch_mode == PLAYBACK_ENTER)) //same
        {
            //need sync volume playlist
            //check old playback state
            if (pri_info.play_state == sec_info.play_state) //same
            {
                if ((pri_info.play_state == PLAYBACK_STATE_PLAY) &&
                    (app_cfg_nv.bud_role == REMOTE_SESSION_ROLE_PRIMARY))
                {
                    app_mmi_handle_action(MMI_SD_PLAYPACK_PAUSE);
                }
            }
            else //diff playing
            {
                if ((pri_info.play_state == PLAYBACK_STATE_PLAY) &&
                    (app_cfg_nv.bud_role == REMOTE_SESSION_ROLE_PRIMARY))
                {
                    app_mmi_handle_action(MMI_SD_PLAYPACK_PAUSE);
                }
                else if ((sec_info.play_state == PLAYBACK_STATE_PLAY) &&
                         (app_cfg_nv.bud_role == REMOTE_SESSION_ROLE_SECONDARY))
                {
                    app_mmi_handle_action(MMI_SD_PLAYPACK_PAUSE);
//                    app_playback_couple_restart_sync(sec_file_name, sec_info.name_len, sec_info.file_offset);
                }
            }
        }
        else if (pri_info.switch_mode != sec_info.switch_mode) //diff
        {
            //playback mode
            if ((pri_info.switch_mode != PLAYBACK_ENTER) && \
                (app_cfg_nv.bud_role == REMOTE_SESSION_ROLE_PRIMARY))
            {
                playback_db.playlist_idx = sec_info.playlist_idx;
                playback_db.volume = sec_info.volume;
                app_mmi_handle_action(MMI_SD_PLAYBACK_SWITCH);
                //sync info app_playback_store_resume_play_state
                //check sec play_state
                if (sec_info.play_state == PLAYBACK_STATE_PLAY)
                {
//                    app_mmi_handle_action(MMI_SD_PLAYPACK_PAUSE);
                    app_playback_couple_restart_sync(sec_file_name, sec_info.name_len, sec_info.file_offset);
                }

            }
            else if ((sec_info.switch_mode != PLAYBACK_ENTER) && \
                     (app_cfg_nv.bud_role == REMOTE_SESSION_ROLE_SECONDARY))
            {
                playback_db.playlist_idx = pri_info.playlist_idx;
                playback_db.volume = pri_info.volume;
                app_mmi_handle_action(MMI_SD_PLAYBACK_SWITCH);
                //sync pri play_state app_playback_store_resume_play_state
                //check and sync pri play_state
                if (pri_info.play_state == PLAYBACK_STATE_PLAY)
                {
//                    app_mmi_handle_action(MMI_SD_PLAYPACK_PAUSE);
                    app_playback_couple_restart_sync(pri_file_name, pri_info.name_len, pri_info.file_offset);
                }
            }
            APP_PRINT_TRACE1("app_playback_relay_single_to_couple_sync_handle,res:0x%x", res);
        }

        b_pri_engage = false;
        b_sec_engage = false;
        os_mem_free(pri_file_name);
        pri_file_name = NULL;
        os_mem_free(sec_file_name);
        sec_file_name = NULL;
    }

}

static void app_playback_relay_stop_sync_handle(void)
{
    APP_PRINT_TRACE0("app_playback_relay_stop_sync_handle");
    app_mmi_handle_action(MMI_SD_PLAYBACK_SWITCH);
}
static void app_playback_relay_resum_sync_handle(uint8_t *buf, uint16_t len)
{
    uint8_t old_state_tmp;
    old_state_tmp = *buf;
    APP_PRINT_TRACE1("app_playback_relay_resum_sync_handle, old local_track_state:%d",
                     old_state_tmp);
    app_mmi_handle_action(MMI_SD_PLAYBACK_SWITCH);
    if (old_state_tmp == PLAYBACK_STATE_PLAY)
    {
        app_mmi_handle_action(MMI_SD_PLAYPACK_PAUSE);
    }
}
static void app_playback_entery_hfp_handle(void)
{
    APP_PRINT_TRACE1("app_playback_entery_hfp_handle,role:%d", app_cfg_nv.bud_role);
    if (remote_session_state_get() == REMOTE_SESSION_STATE_CONNECTED)
    {
        if (app_cfg_nv.bud_role == REMOTE_SESSION_ROLE_PRIMARY)
        {
            uint8_t action;
            remote_async_msg_relay(playback_db.relay_handle, PLAYBACK_REMOTE_MSG_STOP_SYNC,
                                   (uint8_t *)&action, sizeof(action), false);
        }
    }
    else
    {
        app_mmi_handle_action(MMI_SD_PLAYBACK_SWITCH);
    }
}
static void app_playback_quit_hfp_handle(uint8_t play_state)
{
    APP_PRINT_TRACE1("app_playback_quit_hfp_handle,play_state:%d", play_state);

    if (remote_session_state_get() == REMOTE_SESSION_STATE_CONNECTED)
    {
        if (app_cfg_nv.bud_role == REMOTE_SESSION_ROLE_PRIMARY)
        {
            remote_sync_msg_relay(playback_db.relay_handle, PLAYBACK_REMOTE_MSG_RESUM_SYNC,
                                  (uint8_t *)&play_state, sizeof(play_state), REMOTE_TIMER_HIGH_PRECISION, 40, false);
        }
    }
    else
    {
        app_mmi_handle_action(MMI_SD_PLAYBACK_SWITCH);
        if (play_state == PLAYBACK_STATE_PLAY)
        {
            app_mmi_handle_action(MMI_SD_PLAYPACK_PAUSE);
        }
    }
}

static void app_playback_relay_playlist_sync_handle(uint8_t *buf, uint16_t len)
{
    uint16_t  res = 0;
    uint8_t playlist_idx;
    playlist_idx = *buf;

    if ((AUDIO_TRACK_STATE_STARTED == playback_db.local_track_state) || \
        (AUDIO_TRACK_STATE_PAUSED  == playback_db.local_track_state))
    {
#if PLAYBACK_PLAYLIST_PAUSE_EN
        app_playback_stop();
        //close file
        if (playback_audio_fs_handle != NULL)
        {
            if (0 != audio_fs_close(playback_audio_fs_handle))
            {
//            return;
            }
            playback_audio_fs_handle = NULL;
        }
#endif
    }

    if ((playlist_idx < 1) || (playlist_idx > PLAYBACK_PLAYLIST_NUM))
    {
        APP_PRINT_ERROR1("app_playback_relay_playlist_sync_handle ERR, playlist_idx:%d", playlist_idx);
        return;
    }
    playback_db.playlist_idx = playlist_idx;
    res = audio_fs_set_playlist(playback_db.playlist_idx);
    uint16_t file_cnt = audio_fs_get_filecount_from_playlist();
    APP_PRINT_TRACE2("app_playback_relay_playlist_sync_handle, playlist_idx:%d, file_cnt:%d",
                     playlist_idx, file_cnt);
    if (res != 0)
    {
        APP_PRINT_ERROR1("app_playback_relay_playlist_sync_handle ERR, res:0x%x", res);
    }
}

static void app_playback_relay_put_data_sync_handle(uint8_t *buf, uint16_t len)
{
    T_PLAYBACK_PUT_DATA_INFO put_data_info;
    uint32_t local_offset = audio_fs_get_file_offset(playback_audio_fs_handle);

    memcpy((uint8_t *)&put_data_info, buf, sizeof(T_PLAYBACK_PUT_DATA_INFO));
    APP_PRINT_TRACE4("app_playback_relay_put_data_sync_handle local seq:%d, offset: 0x%x, rx seq:%d, offset: 0x%x",
                     playback_db.seq_num, local_offset, put_data_info.seq_num, put_data_info.file_offset);
    if ((playback_db.seq_num != put_data_info.seq_num) || (local_offset != put_data_info.file_offset))
    {
        playback_db.seq_num = put_data_info.seq_num;
        audio_fs_set_file_offset(playback_audio_fs_handle, put_data_info.file_offset);
    }
    app_playback_put_data(put_data_info.pkt_num);
}

void app_playback_relay_cback(uint16_t event, T_REMOTE_RELAY_STATUS status, void *buf, uint16_t len)
{
    APP_PRINT_TRACE4("app_playback_relay_cback: event 0x%04x, status %d, len:%d, bud_role:%d",
                     event, status, len, app_cfg_nv.bud_role);
    uint8_t res;

    switch (event)
    {
    case PLAYBACK_REMOTE_MSG_CHECK_FILE_SYNC:
        if (status == REMOTE_RELAY_STATUS_ASYNC_RCVD ||
            status == REMOTE_RELAY_STATUS_ASYNC_SENT_OUT)
        {
            if (app_cfg_nv.bud_role == REMOTE_SESSION_ROLE_SECONDARY)
            {
                res = app_playback_relay_check_sync_handle(buf, len);
                remote_async_msg_relay(playback_db.relay_handle, PLAYBACK_REMOTE_MSG_CHECK_RESULT_SYNC,
                                       (uint8_t *)&res,
                                       sizeof(res), false);
            }
        }
        break;

    case PLAYBACK_REMOTE_MSG_CHECK_RESULT_SYNC:
        if (status == REMOTE_RELAY_STATUS_ASYNC_RCVD ||
            status == REMOTE_RELAY_STATUS_ASYNC_SENT_OUT)
        {
            if (app_cfg_nv.bud_role == REMOTE_SESSION_ROLE_PRIMARY)
            {
                app_playback_relay_check_result_handle(buf, len);
            }
        }
        break;

    case PLAYBACK_REMOTE_MSG_START_SYNC:
        if (status == REMOTE_RELAY_STATUS_SYNC_TOUT || //03
            status == REMOTE_RELAY_STATUS_SYNC_EXPIRED || //04
            status == REMOTE_RELAY_STATUS_SYNC_REF_CHANGED) //08
        {
            app_playback_start();
        }

        break;

    case PLAYBACK_REMOTE_MSG_SWITCH_SYNC:
        if (status == REMOTE_RELAY_STATUS_ASYNC_RCVD ||
            status == REMOTE_RELAY_STATUS_ASYNC_SENT_OUT)
        {
            app_playback_stop();
            if (app_cfg_nv.bud_role == REMOTE_SESSION_ROLE_PRIMARY)
            {
                app_playback_relay_switch_sync_handle();
            }
        }
        break;

    case PLAYBACK_REMOTE_MSG_PUT_DATA_SYNC:
        if (status == REMOTE_RELAY_STATUS_ASYNC_RCVD ||
            status == REMOTE_RELAY_STATUS_ASYNC_SENT_OUT)
        {
            if (AUDIO_TRACK_STATE_STARTED == playback_db.local_track_state)
            {
                app_playback_relay_put_data_sync_handle(buf, len);
            }
        }
        break;

    case PLAYBACK_REMOTE_MSG_SEC_STARTED_SYNC:
        if (status == REMOTE_RELAY_STATUS_ASYNC_RCVD)
        {
            playback_db.sec_track_state = AUDIO_TRACK_STATE_STARTED;
            if (app_cfg_nv.bud_role == REMOTE_SESSION_ROLE_PRIMARY)
            {
                if ((AUDIO_TRACK_STATE_STARTED == playback_db.local_track_state) && \
                    (AUDIO_TRACK_STATE_STARTED == playback_db.sec_track_state))
                {
                    playback_db.sec_track_state = 0;
                    app_playback_put_data(PLAYBACK_COUPLE_PREQ_PKTS(playback_db.sample_rate));
                }
            }
        }
        break;

    case PLAYBACK_REMOTE_MSG_SINGLE_COUPLE_PRI_SYNC:
        if (status == REMOTE_RELAY_STATUS_ASYNC_RCVD ||
            status == REMOTE_RELAY_STATUS_ASYNC_SENT_OUT)
        {
            app_playback_relay_single_to_couple_sync_handle(event, buf, len);
        }
        break;

    case PLAYBACK_REMOTE_MSG_SINGLE_COUPLE_SEC_SYNC:
        if (status == REMOTE_RELAY_STATUS_ASYNC_RCVD ||
            status == REMOTE_RELAY_STATUS_ASYNC_SENT_OUT)
        {
            app_playback_relay_single_to_couple_sync_handle(event, buf, len);
        }
        break;

    case PLAYBACK_REMOTE_MSG_STOP_SYNC:
        if (status == REMOTE_RELAY_STATUS_ASYNC_RCVD ||
            status == REMOTE_RELAY_STATUS_ASYNC_SENT_OUT)
        {
            app_playback_relay_stop_sync_handle();
        }
        break;

    case PLAYBACK_REMOTE_MSG_RESUM_SYNC:
        if (status == REMOTE_RELAY_STATUS_SYNC_TOUT || //03
            status == REMOTE_RELAY_STATUS_SYNC_EXPIRED || //04
            status == REMOTE_RELAY_STATUS_SYNC_REF_CHANGED) //08
        {
            app_playback_relay_resum_sync_handle(buf, len);
        }
        break;

    case PLAYBACK_REMOTE_MSG_PLAYLIST_SYNC:
        if (status == REMOTE_RELAY_STATUS_ASYNC_RCVD)
        {
            if (app_cfg_nv.bud_role == REMOTE_SESSION_ROLE_SECONDARY)
            {
                app_playback_relay_playlist_sync_handle(buf, len);
            }
        }
        break;

    case PLAYBACK_REMOTE_MSG_TRACK_STOP_SYNC:
        if (status == REMOTE_RELAY_STATUS_ASYNC_RCVD ||
            status == REMOTE_RELAY_STATUS_ASYNC_SENT_OUT)
        {
            app_playback_stop();
        }
        break;

    case PLAYBACK_REMOTE_MSG_SEC_TRACK_STOPPED_SYNC:
        if (status == REMOTE_RELAY_STATUS_ASYNC_RCVD ||
            status == REMOTE_RELAY_STATUS_ASYNC_SENT_OUT)
        {
            if (app_cfg_nv.bud_role == REMOTE_SESSION_ROLE_PRIMARY)
            {
                playback_db.sec_track_state = *((uint8_t *)buf + 0);
                APP_PRINT_TRACE2("local_track_state %d sec_track_state %d", playback_db.local_track_state,
                                 playback_db.sec_track_state);
                if ((AUDIO_TRACK_STATE_RELEASED == playback_db.local_track_state) && \
                    (AUDIO_TRACK_STATE_RELEASED == playback_db.sec_track_state))
                {
                    app_playback_stopped_start_next_action(playback_db.op_next_action);
                    playback_db.sec_track_state = 0;
                }
            }
        }
        break;

    default:
        break;
    }
}

void app_playback_get_audio_track_state_handle(void)
{
    audio_track_state_get(playback_track_handle, (T_AUDIO_TRACK_STATE *)&playback_db.local_track_state);
    APP_PRINT_TRACE1("app_playback_get_audio_track_state_handle, playback_db.local_track_state:%d",
                     playback_db.local_track_state);
}

void app_playback_track_state_changed_handle(void)
{
    APP_PRINT_TRACE3("app_playback_track_state_changed_handle,local_track_state:%d, bud_role:%d, connect:%d",
                     playback_db.local_track_state, app_cfg_nv.bud_role, remote_session_state_get());
    if (AUDIO_TRACK_STATE_STARTED == playback_db.local_track_state)
    {
        if (remote_session_state_get() == REMOTE_SESSION_STATE_CONNECTED)
        {
            uint8_t action = 0;
            if (app_cfg_nv.bud_role == REMOTE_SESSION_ROLE_PRIMARY)
            {
                // audio_track_start(sync 40ms)  ->pri && sec started -> put data
                if ((AUDIO_TRACK_STATE_STARTED == playback_db.local_track_state) && \
                    (AUDIO_TRACK_STATE_STARTED == playback_db.sec_track_state))
                {
                    playback_db.sec_track_state = 0;
                    app_playback_put_data(PLAYBACK_COUPLE_PREQ_PKTS(playback_db.sample_rate));
                }
            }
            else if (app_cfg_nv.bud_role == REMOTE_SESSION_ROLE_SECONDARY)
            {
                remote_async_msg_relay(playback_db.relay_handle, PLAYBACK_REMOTE_MSG_SEC_STARTED_SYNC,
                                       (uint8_t *)&action, sizeof(action), false);
                app_playback_put_data(PLAYBACK_COUPLE_PREQ_PKTS(playback_db.sample_rate));
            }
        }
        else
        {
            app_playback_put_data(PLAYBACK_SINGLE_PREQ_PKTS(playback_db.sample_rate));
        }
    }
    else if (AUDIO_TRACK_STATE_RELEASED == playback_db.local_track_state)
    {
        playback_track_handle = NULL;
        if (remote_session_state_get() == REMOTE_SESSION_STATE_CONNECTED)
        {
            if (app_cfg_nv.bud_role == REMOTE_SESSION_ROLE_PRIMARY)
            {
                APP_PRINT_TRACE2("local_track_state %d sec_track_state %d", playback_db.local_track_state,
                                 playback_db.sec_track_state);
                if ((AUDIO_TRACK_STATE_RELEASED == playback_db.local_track_state) && \
                    (AUDIO_TRACK_STATE_RELEASED == playback_db.sec_track_state))
                {
                    app_playback_stopped_start_next_action(playback_db.op_next_action);
                    playback_db.sec_track_state = 0;
                }
            }
            else if (app_cfg_nv.bud_role == REMOTE_SESSION_ROLE_SECONDARY)
            {
                remote_async_msg_relay(playback_db.relay_handle, PLAYBACK_REMOTE_MSG_SEC_TRACK_STOPPED_SYNC,
                                       (uint8_t *)&playback_db.local_track_state, sizeof(playback_db.local_track_state), false);
            }
        }
        else
        {
            app_playback_stopped_start_next_action(playback_db.op_next_action);
        }
    }
}

void app_playback_buffer_low_handle(void)
{
    playback_db.buffer_state = PLAYBACK_BUF_LOW;
    if (audio_fs_end_of_file(playback_audio_fs_handle))
    {
        APP_PRINT_WARN0("app_playback_buffer_low_handle,file end, and paly next song!!!");
        if (remote_session_state_get() == REMOTE_SESSION_STATE_CONNECTED)
        {
            if (app_cfg_nv.bud_role == REMOTE_SESSION_ROLE_PRIMARY)
            {
                uint8_t action;
                remote_async_msg_relay(playback_db.relay_handle, PLAYBACK_REMOTE_MSG_TRACK_STOP_SYNC,
                                       (uint8_t *)&action, sizeof(action), false);
            }
        }
        else
        {
            app_playback_stop();
        }

        playback_db.op_next_action = PLAYBACK_STOPPED_FILE_END_TO_NEXT_ACTION;
    }
    else
    {
//        app_playback_put_data();
        app_playback_put_data_stop_timer(PLAYBACK_PUT_DATA_PKT_NUM + 2);
    }

}

void app_playback_buffer_high_handle(void)
{
//    gap_stop_timer(&playback_db.put_data_timer_handle);
    playback_db.buffer_state = PLAYBACK_BUF_HIGH;
}

/**
    * @brief  App need to process sd card playback message that the user trigger play/pause action by the PC.
    * @param  action action ID to be processed
    * @return void
    */
void app_playback_mmi_handle_action(uint8_t action)
{
    uint8_t res = PLAYBACK_SUCCESS;
    uint8_t filename[FILE_NAME_LEN]  = {0};
    uint16_t name_len = 0;
    uint32_t playback_offset = 0;

    APP_PRINT_TRACE2("app_playback_mmi_handle_action, 0x%x, play_state:%d", action,
                     playback_db.sd_play_state);
    switch (action)
    {
    case MMI_SD_PLAYPACK_PAUSE:
        {
            if (playback_db.sd_play_state == PLAYBACK_STATE_PAUSE)
            {
                if ((app_cfg_nv.bud_role == REMOTE_SESSION_ROLE_SECONDARY) && \
                    (remote_session_state_get() == REMOTE_SESSION_STATE_CONNECTED))
                {
                    return;
                }
                if (playback_audio_fs_handle == NULL)
                {
                    res = app_playback_open_and_get_file_info(SINGLE_FILE, filename, &name_len, &playback_offset);
                }
                else
                {
                    uint8_t *p_file_name = audio_fs_get_filename(playback_audio_fs_handle);
                    name_len = audio_fs_get_filenameLen(playback_audio_fs_handle);
                    memcpy(filename, p_file_name, name_len);
                    playback_offset = audio_fs_get_file_offset(playback_audio_fs_handle);
                }

                if (res != PLAYBACK_SUCCESS)
                {
                    return;
                }
                if (remote_session_state_get() == REMOTE_SESSION_STATE_CONNECTED)
                {
                    if (app_cfg_nv.bud_role == REMOTE_SESSION_ROLE_PRIMARY)
                    {
                        app_playback_couple_sync_file_info_handle(filename, name_len, playback_offset);
                    }
                }
                else
                {
                    // single
                    app_playback_start();
                    playback_db.sd_play_state = PLAYBACK_STATE_PLAY;
                }
            }
            else if (playback_db.sd_play_state == PLAYBACK_STATE_PLAY)
            {
                app_playback_stop();
            }
        }
        break;

    case MMI_SD_PLAYBACK_FWD:
    case MMI_SD_PLAYBACK_BWD:
        {
            if ((app_cfg_nv.bud_role == REMOTE_SESSION_ROLE_SECONDARY) && \
                (remote_session_state_get() == REMOTE_SESSION_STATE_CONNECTED))
            {
                return;
            }
            if (action == MMI_SD_PLAYBACK_FWD)
            {
                playback_db.op_next_action = PLAYBACK_STOPPED_SWITCH_FWD_ACTION;
            }
            else if (action == MMI_SD_PLAYBACK_BWD)
            {
                playback_db.op_next_action = PLAYBACK_STOPPED_SWITCH_BWD_ACTION;
            }

            if (remote_session_state_get() == REMOTE_SESSION_STATE_CONNECTED)
            {
                if (app_cfg_nv.bud_role == REMOTE_SESSION_ROLE_PRIMARY)
                {
                    if (playback_db.sd_play_state == PLAYBACK_STATE_PLAY)
                    {
                        uint8_t action;
                        remote_async_msg_relay(playback_db.relay_handle, PLAYBACK_REMOTE_MSG_TRACK_STOP_SYNC,
                                               (uint8_t *)&action, sizeof(action), false);
                    }
                    else
                    {
                        app_playback_stopped_start_next_action(playback_db.op_next_action);
                    }
                }
            }
            else
            {
                if (playback_db.sd_play_state == PLAYBACK_STATE_PLAY)
                {
                    app_playback_stop();
                }
                else
                {
                    app_playback_stopped_start_next_action(playback_db.op_next_action);
                }
            }
        }
        break;

    case MMI_SD_PLAYBACK_FWD_PLAYLIST:
    case MMI_SD_PLAYBACK_BWD_PLAYLIST:
        {
            if ((AUDIO_TRACK_STATE_STARTED == playback_db.local_track_state) || \
                (AUDIO_TRACK_STATE_PAUSED == playback_db.local_track_state))
            {
#if PLAYBACK_PLAYLIST_PAUSE_EN
                app_playback_stop();
                //close file
                if (playback_audio_fs_handle != NULL)
                {
                    if (0 != audio_fs_close(playback_audio_fs_handle))
                    {
                        APP_PRINT_ERROR1("close FAIL,(%p)", playback_audio_fs_handle);
                        //            return;
                    }
                    playback_audio_fs_handle = NULL;
                }
#endif
            }
            if ((app_cfg_nv.bud_role == REMOTE_SESSION_ROLE_SECONDARY) && \
                (remote_session_state_get() == REMOTE_SESSION_STATE_CONNECTED))
            {
                return;
            }

            if (action == MMI_SD_PLAYBACK_FWD_PLAYLIST)
            {
                res = app_playback_update_playlist(PREVERSE_FILE);
            }
            else if (action == MMI_SD_PLAYBACK_BWD_PLAYLIST)
            {
                res = app_playback_update_playlist(NEXT_FILE);
            }

            if ((remote_session_state_get() == REMOTE_SESSION_STATE_CONNECTED) && (res == 0))
            {
                if (app_cfg_nv.bud_role == REMOTE_SESSION_ROLE_PRIMARY)
                {
                    remote_async_msg_relay(playback_db.relay_handle, PLAYBACK_REMOTE_MSG_PLAYLIST_SYNC,
                                           (uint8_t *)&playback_db.playlist_idx, sizeof(playback_db.playlist_idx), false);
                }
            }
            else if (res == 0)
            {
                // single
                APP_PRINT_TRACE0("MMI_SD_PLAYBACK_FWD_PLAYLIST SINGLE");
            }
        }
        break;

    default:
        break;
    }
}

void app_playback_stopped_start_next_action(uint8_t next_action)
{
    uint8_t filename[FILE_NAME_LEN]  = {0};
    uint16_t name_len = 0;
    uint32_t playback_offset;
    uint8_t result = PLAYBACK_SUCCESS;

    APP_PRINT_TRACE2("app_playback_stopped_start_next_action, op_next_action 0x%x, play_state:%d",
                     next_action, playback_db.sd_play_state);
    switch (next_action)
    {
    case PLAYBACK_STOPPED_SWITCH_BWD_ACTION:
    case PLAYBACK_STOPPED_SWITCH_FWD_ACTION:
        {
            if (playback_audio_fs_handle != NULL)
            {
                uint16_t res = audio_fs_close(playback_audio_fs_handle);
                if (res != 0)
                {
                    APP_PRINT_ERROR1("playback_audio_fs_handle close fail, res :0x%x", res);
                    return;
                }
                playback_audio_fs_handle = NULL;
            }
            if (next_action == PLAYBACK_STOPPED_SWITCH_FWD_ACTION)
            {
                result = app_playback_open_and_get_file_info(PREVERSE_FILE, filename, &name_len, &playback_offset);
            }
            else if (next_action == PLAYBACK_STOPPED_SWITCH_BWD_ACTION)
            {
                result = app_playback_open_and_get_file_info(NEXT_FILE, filename, &name_len, &playback_offset);
            }
            if (result != PLAYBACK_SUCCESS)
            {
                return;
            }
//                app_playback_start(); // ->audio track ->return audio track state change ->send msg
            if (remote_session_state_get() == REMOTE_SESSION_STATE_CONNECTED)
            {
                if (app_cfg_nv.bud_role == REMOTE_SESSION_ROLE_PRIMARY)
                {
                    app_playback_couple_sync_file_info_handle(filename, name_len, playback_offset);
                }
            }
            else
            {
                // single
                app_playback_start();
                playback_db.sd_play_state = PLAYBACK_STATE_PLAY;
            }
        }
        break;

    case PLAYBACK_STOPPED_FILE_END_TO_NEXT_ACTION:
        {
            if (remote_session_state_get() == REMOTE_SESSION_STATE_CONNECTED)
            {
                if (app_cfg_nv.bud_role == REMOTE_SESSION_ROLE_PRIMARY)
                {
                    uint8_t action = 0;
                    remote_async_msg_relay(playback_db.relay_handle, PLAYBACK_REMOTE_MSG_SWITCH_SYNC,
                                           (uint8_t *)&action, 1, false);
                }
            }
            else
            {
                if (playback_audio_fs_handle != NULL)
                {
                    uint16_t res = audio_fs_close(playback_audio_fs_handle);
                    if (res != 0)
                    {
                        return;
                    }
                    playback_audio_fs_handle = NULL;
                }
                result = app_playback_open_and_get_file_info(NEXT_FILE, filename, &name_len, &playback_offset);
                if (result != PLAYBACK_SUCCESS)
                {
                    return;
                }

                app_playback_start();
            }
        }
        break;

    default:
        break;
    }
    playback_db.op_next_action = PLAYBACK_STOPPED_IDLE_ACTION;
}

//////////////////////////////////////////REG CB////////////////////////////////////////////
static uint8_t sd_playback_mode_old = 0;
static uint8_t playback_play_state_old = 0;
static void app_playback_bt_policy_cback(T_BT_EVENT event_type, void *event_buf, uint16_t buf_len)
{
    switch (event_type)
    {
    case BT_EVENT_REMOTE_CONN_CMPL:
        {
            APP_PRINT_TRACE3("BT_EVENT_REMOTE_CONN_CMPL bud_role:%d -sd_playback_switch:%d,remote state%d",
                             app_cfg_nv.bud_role, app_db.sd_playback_switch, remote_session_state_get());
            app_playback_single_to_couple_sync_handle(PLAYBACK_STOP);
//            app_playback_single_to_couple_sync_handle(PLAYBACK_START);
            app_playback_single_to_couple_sync_timer_start();
        }
        break;

    default:
        break;
    }
}

static void app_playback_hfp_bt_cback(T_BT_EVENT event_type, void *event_buf, uint16_t buf_len)
{
    T_BT_EVENT_PARAM *param = event_buf;
    bool handle = true;

    switch (event_type)
    {
    case BT_EVENT_HFP_CALL_STATUS:
        {
            T_APP_BR_LINK *p_link;

            p_link = app_find_br_link(param->hfp_call_status.bd_addr);
            APP_PRINT_TRACE3("app_hfp_get_call_status:0x%x,sd_playback_switch:%d, old play state:%d",
                             app_hfp_get_call_status(), app_db.sd_playback_switch, playback_play_state_old);
            if (p_link != NULL)
            {
                if ((app_hfp_get_call_status() != BT_HFP_CALL_IDLE) && app_db.sd_playback_switch)
                {
                    //relay playback stop
                    sd_playback_mode_old = 1;
                    playback_play_state_old = playback_db.sd_play_state;
                    app_playback_entery_hfp_handle();
                }
            }

            if ((app_hfp_get_call_status() == BT_HFP_CALL_IDLE) && sd_playback_mode_old)
            {
                //relay playback restart
//                app_playback_quit_hfp_handle(playback_play_state_old);
                app_playback_resume_timer_start(playback_play_state_old);
                sd_playback_mode_old = 0;
                playback_play_state_old = 0;
            }
        }
        break;

    case BT_EVENT_AVRCP_PLAY_STATUS_CHANGED:
        {
            uint8_t app_idx = app_get_active_a2dp_idx();
            if ((app_cfg_nv.bud_role == REMOTE_SESSION_ROLE_SECONDARY) && \
                (remote_session_state_get() == REMOTE_SESSION_STATE_CONNECTED))
            {
                return;
            }

            if ((app_db.br_link[app_idx].connected_profile & A2DP_PROFILE_MASK) &&
                (app_db.br_link[app_idx].connected_profile & AVRCP_PROFILE_MASK))
            {
                if ((app_db.br_link[app_idx].avrcp_play_status == BT_AVRCP_PLAY_STATUS_PLAYING) &&
                    app_db.sd_playback_switch)
                {
                    bt_avrcp_pause(app_db.br_link[app_idx].bd_addr);
                }
            }
        }
        break;

    default:
        handle = false;
        break;
    }

    if (handle == true)
    {
        APP_PRINT_TRACE1("app_playback_hfp_bt_cback: event_type 0x%04x", event_type);
    }
}


static void app_playback_audio_policy_cback(T_AUDIO_EVENT event_type, void *event_buf,
                                            uint16_t buf_len)
{
    T_AUDIO_EVENT_PARAM *param = event_buf;
    bool handle = true;

    if (param->track_state_changed.handle != playback_track_handle)
    {
        return;
    }

    switch (event_type)
    {
    case AUDIO_EVENT_TRACK_STATE_CHANGED:
        {
            playback_db.local_track_state = param->track_state_changed.state;
            app_playback_get_audio_track_state_handle();
            app_playback_track_state_changed_handle();
        }
        break;

    case AUDIO_EVENT_TRACK_BUFFER_LOW:
        {
            if (playback_db.sd_play_state == PLAYBACK_STATE_PLAY)
            {
                app_playback_buffer_low_handle();
            }
        }
        break;

    case AUDIO_EVENT_TRACK_BUFFER_HIGH:
        {
            if (playback_db.sd_play_state == PLAYBACK_STATE_PLAY)
            {
                app_playback_buffer_high_handle();
            }
        }
        break;

    default:
        handle = false;
        break;
    }

    if (handle == true)
    {
        APP_PRINT_TRACE1("app_playback_audio_policy_cback: event_type 0x%04x", event_type);
    }
}

static void app_playback_dm_cback(T_SYS_EVENT event_type, void *event_buf, uint16_t buf_len)
{
    bool handle = true;

    switch (event_type)
    {
    case SYS_EVENT_POWER_OFF:
        {
            app_playback_power_off_handle();
        }
        break;

    case SYS_EVENT_POWER_ON:
        {
            app_sd_card_dlps_clear();
            app_sd_card_power_down_clear();
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
        APP_PRINT_INFO1("app_playback_dm_cback: event_type 0x%04x", event_type);
    }
}

void app_playback_init(void)
{
    memset(&playback_db, 0, sizeof(T_PLAYBACK_DATA));
    playback_db.file_idx = 0;
    playback_db.playlist_idx = 1;
    playback_db.volume = 7;
    sys_mgr_cback_register(app_playback_dm_cback);
    audio_mgr_cback_register(app_playback_audio_policy_cback);
    bt_mgr_cback_register(app_playback_bt_policy_cback); //single -> couple

    if (app_cfg_const.supported_profile_mask & (HFP_PROFILE_MASK | HSP_PROFILE_MASK))
    {
        bt_mgr_cback_register(app_playback_hfp_bt_cback);
    }

    playback_db.relay_handle = remote_relay_register(app_playback_relay_cback);
    app_playback_timer_init();
}
//////////////////////////////////////////////////////////////////////////////////////

static void app_playback_timer_callback(uint8_t timer_id, uint16_t timer_chann)
{
    APP_PRINT_TRACE2("app_playback_timer_callback: timer_id 0x%02x, timer_chann 0x%x",
                     timer_id, timer_chann);

    switch (timer_id)
    {
    case PLAYBACK_TIMER_SD_INIT:
        {
            gap_stop_timer(&playback_db.timer_handle);
        }
        break;

    case PLAYBACK_TIMER_SIGNLE_TO_COUPLE_SYNC:
        {
            gap_stop_timer(&playback_db.timer_handle);
            app_playback_single_to_couple_sync_handle(PLAYBACK_START);
        }
        break;

    case PLAYBACK_TIMER_QUIT_HFP:
        {
            gap_stop_timer(&playback_db.timer_handle);
            app_playback_quit_hfp_handle((uint8_t)timer_chann);
        }
        break;

    default:
        break;
    }
}
void app_playback_single_to_couple_sync_timer_start(void)
{
    gap_start_timer(&playback_db.timer_handle, "sd_playback_engage_sync", playback_db.timer_queue_id,
                    PLAYBACK_TIMER_SIGNLE_TO_COUPLE_SYNC, 0, 200);
}
void app_playback_resume_timer_start(uint8_t play_state_old)
{
    gap_start_timer(&playback_db.timer_handle, "app_playback_resume_timer_start0",
                    playback_db.timer_queue_id,
                    PLAYBACK_TIMER_QUIT_HFP, play_state_old, 200);
}

void app_playback_put_data_stop_timer(uint8_t pkt_num)
{
    APP_PRINT_TRACE1("app_playback_put_data_stop_timer connect:%d", remote_session_state_get());
    if (playback_db.put_data_timer_handle != NULL)
    {
        gap_stop_timer(&playback_db.put_data_timer_handle);

        if (playback_db.sd_play_state != PLAYBACK_STATE_PLAY)
        {
            return;
        }
        if (remote_session_state_get() == REMOTE_SESSION_STATE_CONNECTED)
        {
            if (app_cfg_nv.bud_role == REMOTE_SESSION_ROLE_PRIMARY)
            {
                T_PLAYBACK_PUT_DATA_INFO put_data_info;
                put_data_info.pkt_num = pkt_num;
                put_data_info.seq_num = playback_db.seq_num;
                put_data_info.file_offset = audio_fs_get_file_offset(playback_audio_fs_handle);
                remote_async_msg_relay(playback_db.relay_handle, PLAYBACK_REMOTE_MSG_PUT_DATA_SYNC,
                                       (uint8_t *)&put_data_info, sizeof(T_PLAYBACK_PUT_DATA_INFO), false);
            }
        }
        else // single and solo
        {
            app_playback_put_data(pkt_num);
        }
    }
}

void app_playback_put_data_start_timer(uint8_t time_ms)
{
    gap_start_timer(&playback_db.put_data_timer_handle, "app_playback_put_data_timer_start",
                    playback_db.put_data_timer_queue_id, PLAYBACK_TIMER_PUT_DATA, 0, time_ms);
}

static void app_playback_put_data_timer_callback(uint8_t timer_id, uint16_t timer_chann)
{
    APP_PRINT_TRACE2("app_playback_put_data_timer_callback: timer_id 0x%02x, timer_chann 0x%x",
                     timer_id, timer_chann);

    switch (timer_id)
    {
    case PLAYBACK_TIMER_PUT_DATA:
        {
            app_playback_put_data_stop_timer(PLAYBACK_PUT_DATA_PKT_NUM);
        }
        break;

    default:
        break;
    }
}

static void app_sd_power_down_timer_callback(uint8_t timer_id, uint16_t timer_chann)
{
    APP_PRINT_TRACE2("app_sd_power_down_timer_callback: timer_id 0x%02x, timer_chann 0x%x",
                     timer_id, timer_chann);

    switch (timer_id)
    {
    case PLAYBACK_TIMER_SD_POWER_DOWN:
        {
            gap_stop_timer(&power_down_sd_timer_handle);
            if (app_sd_card_power_down_check_idle())
            {
                app_sd_card_power_down();
            }
        }
        break;

    default:
        break;
    }
}

static void app_playback_dlps_timer_callback(uint8_t timer_id, uint16_t timer_chann)
{
    APP_PRINT_TRACE2("app_playback_dlps_timer_callback: timer_id 0x%02x, timer_chann 0x%x",
                     timer_id, timer_chann);

    switch (timer_id)
    {
    case PLAYBACK_TIMER_DLPS_DISABLE:
        {
            gap_stop_timer(&playback_dlps_timer_handle);
            if (app_sd_card_dlps_check_idle())
            {
                app_dlps_enable(APP_DLPS_ENTER_CHECK_PLAYBACK);
            }
        }
        break;

    default:
        break;
    }
}

void app_playback_timer_init(void)
{
    gap_reg_timer_cb(app_playback_timer_callback, &playback_db.timer_queue_id);
    gap_reg_timer_cb(app_playback_put_data_timer_callback, &playback_db.put_data_timer_queue_id);
    gap_reg_timer_cb(app_sd_power_down_timer_callback, &app_sd_pd_timer_queue_id);
    gap_reg_timer_cb(app_playback_dlps_timer_callback, &app_playback_dlps_timer_queue_id);
}

static uint16_t s_sd_pd_bitmap = 0;                /**< power down locking bitmap */
void app_sd_card_power_down_clear(void)
{
    s_sd_pd_bitmap = 0;
}
void app_sd_card_power_down_enable(uint16_t bit)
{
    if (s_sd_pd_bitmap & bit)
    {
        APP_PRINT_TRACE3("app_sd_card_power_down_enable: %08x %08x -> %08x", bit, s_sd_pd_bitmap,
                         (s_sd_pd_bitmap & ~bit));
    }
    s_sd_pd_bitmap &= ~bit;
    gap_start_timer(&power_down_sd_timer_handle, "sd_card_power_down_enable check",
                    app_sd_pd_timer_queue_id, PLAYBACK_TIMER_SD_POWER_DOWN, 0, POWER_DOWN_SD_TIME);
//    if (app_sd_card_power_down_check_idle())
//    {
//        app_sd_card_power_down();
//    }
}

void app_sd_card_power_down_disable(uint16_t bit)
{
    if ((s_sd_pd_bitmap & bit) == 0)
    {
        APP_PRINT_TRACE3("app_sd_card_power_down_disable: %08x %08x -> %08x", bit, s_sd_pd_bitmap,
                         (s_sd_pd_bitmap | bit));
    }
    if (app_sd_card_power_down_check_idle() == true)
    {
        app_sd_card_power_on();
    }

    s_sd_pd_bitmap |= bit;
}

bool app_sd_card_power_down_check_idle(void)
{
    if (s_sd_pd_bitmap == APP_SD_POWER_DOWN_ENTER_CHECK_IDLE)
    {
        return true;
    }
    return false;
}

void app_sd_card_power_down(void)
{
    APP_PRINT_TRACE0("app_sd_card_power_down, res: 0x%x");
    audio_fs_sd_power_off();
}

void app_sd_card_power_on(void)
{
    uint16_t res;
    res = audio_fs_sd_status_check_and_init();
    APP_PRINT_TRACE1("app_sd_card_power_on, res: 0x%x", res);
}

static uint16_t s_sd_dlps_bitmap = 0;                /**< sd dlps locking bitmap */
void app_sd_card_dlps_clear(void)
{
    s_sd_dlps_bitmap = APP_SD_DLPS_ENTER_CHECK_IDLE;
    app_dlps_enable(APP_DLPS_ENTER_CHECK_PLAYBACK);
}
void app_sd_card_dlps_enable(uint16_t bit)
{
    if (s_sd_dlps_bitmap & bit)
    {
        APP_PRINT_TRACE3("app_sd_card_dlps_enable: %08x %08x -> %08x", bit, s_sd_dlps_bitmap,
                         (s_sd_dlps_bitmap & ~bit));
    }
    s_sd_dlps_bitmap &= ~bit;

    gap_start_timer(&playback_dlps_timer_handle, "app_playback_dlps_enable_start_timer",
                    app_playback_dlps_timer_queue_id, PLAYBACK_TIMER_DLPS_DISABLE, 0, DLPS_DISABLE_TIME);
}

void app_sd_card_dlps_disable(uint16_t bit)
{
    if ((s_sd_dlps_bitmap & bit) == 0)
    {
        APP_PRINT_TRACE3("app_sd_card_dlps_disable: %08x %08x -> %08x", bit, s_sd_dlps_bitmap,
                         (s_sd_dlps_bitmap | bit));
    }
    app_dlps_disable(APP_DLPS_ENTER_CHECK_PLAYBACK);

    s_sd_dlps_bitmap |= bit;
}

bool app_sd_card_dlps_check_idle(void)
{
    if (s_sd_dlps_bitmap == APP_SD_DLPS_ENTER_CHECK_IDLE)
    {
        return true;
    }
    return false;
}

void app_playback_power_off_handle(void)
{
    APP_PRINT_TRACE0("app_playback_power_off_handle ++");
    if (app_db.sd_playback_switch == 1)
    {
        app_mmi_handle_action(MMI_SD_PLAYBACK_SWITCH);
    }
    app_sd_card_power_down_disable(APP_SD_POWER_DOWN_ENTER_CHECK_PLAYBACK);
    if (playback_audio_fs_handle != NULL)
    {
        if (0 != audio_fs_close(playback_audio_fs_handle))
        {
            APP_PRINT_ERROR0("audio_fs_close fail!!!");
        }
        playback_audio_fs_handle = NULL;
    }
    audio_fs_unlink_file();
    app_sd_card_power_down_enable(APP_SD_POWER_DOWN_ENTER_CHECK_PLAYBACK);
    app_sd_card_dlps_clear();
    app_sd_card_power_down_clear();
    app_sd_card_power_down();
}

#endif //F_APP_LOCAL_PLAYBACK_SUPPORT
/******************* (C) COPYRIGHT 2019 Realtek Semiconductor Corporation *****END OF FILE****/

