/*
 * Copyright (c) 2018, Realsil Semiconductor Corporation. All rights reserved.
 */

#if F_APP_USB_AUDIO_SUPPORT
#include <string.h>
#include "trace.h"
#include "app_usb_audio.h"
#include "app_main.h"
#include "app_gpio.h"
#include "rtl876x_pinmux.h"
#include "sysm.h"
#include "section.h"
#include "app_mmi.h"
#include "audio_track.h"
#include "audio_type.h"
#include "audio.h"
#include "stdlib.h"
#include "app_cfg.h"
#include "app_auto_power_off.h"
#include "app_audio_policy.h"
#include "gap_timer.h"
#include "app_usb_audio_hid.h"
//#include "rom_ext.h"
#if F_APP_TEAMS_FEATURE_SUPPORT
#include "asp_device_link.h"
#endif
#if F_APP_SINGLE_MUTLILINK_SCENERIO_1
#include "app_single_multilink_customer.h"
#include "app_link_util.h"
#endif
#include "app_io_msg.h"

#include "usb_audio.h"
#include "usb_audio_stream.h"

enum {DIR_DS = 1, DIR_US = 2};

#define USB_AUDIO_DS_RB_SIZE    (4*1024)

typedef struct _usb_audio_db
{
    T_USB_AUDIO_STREAM  *stream;
    T_RING_BUFFER       src_buf;
    uint8_t             *pkt_to_send;
    uint16_t            vol_range;
    int16_t             vol_cur;
} T_USB_AUDIO_DB;

T_USB_AUDIO_DB usb_audio_ds_db = {.stream = NULL, .vol_range = 100, .vol_cur = 50};
T_USB_AUDIO_DB usb_audio_us_db;

void usb_init(void);
extern void (*usb_start)(void);
extern void (*usb_stop)(void);

const uint32_t sample_freq[8] = {8000, 16000, 32000, 44100, 48000, 88000, 96000, 192000};
#define USB_AUDIO_DS_MASK   0x01
#define USB_AUDIO_US_MASK   0x02
static uint8_t audio_stream_mask = 0;

static void                        *usb_audio_timer_handle;
static uint8_t                     usb_audio_timer_queue_id;
static bool s_usb_start_timer_flag = true;
static bool s_usb_plug_power_on = false;

static bool g_teams_is_mute = false;
bool g_usb_sys_mic_is_mute = false;

#define UAVOL2DSPVOL(vol, range) (0x10*(vol)/range)

#define     USB_TIMER_USB_STREAM_CHECK     1

typedef enum
{
    MSG_TYPE_PLUG_BC12_TYPE         = 0,
    MSG_TYPE_PLUG                   = 1,
    MSG_TYPE_UNPLUG                 = 2,
    MSG_TYPE_UAC_ACTIVE             = 3,
    MSG_TYPE_UAC_DEACTIVE           = 4,
    MSG_TYPE_UAC_DATA_TRANS_DS      = 5,
    MSG_TYPE_UAC_DATA_TRANS_US      = 6,
    MSG_TYPE_UAC_SET_VOL            = 7,
    MSG_TYPE_UAC_GET_VOL            = 8,
    MSG_TYPE_UAC_SET_MIC_VOL        = 9,
    MSG_TYPE_UAC_SET_MUTE           = 10,

    MSG_TYPE_NONE = 0xff,
} USB_MSG_TYPE_T;

//--------------------------------------------------------------------------//
#if F_APP_SINGLE_MUTLILINK_SCENERIO_1
T_AUDIO_TRACK_STATE app_teams_multilink_get_usb_ds_track_state(void)
{
    T_AUDIO_TRACK_STATE track_state = AUDIO_TRACK_STATE_RELEASED;

    audio_track_state_get(usb_audio_ds_db.stream->track, &track_state);
    return track_state;
}
#endif

static void usb_audio_stream_state_set(T_USB_AUDIO_STREAM *stream, T_AUDIO_STREAM_STATE state)
{
    APP_PRINT_INFO2("usb_audio_stream_state_set:%08x-%08x", state, stream->state);
    stream->state = state;
}

static T_AUDIO_STREAM_STATE usb_audio_stream_state_get(T_USB_AUDIO_STREAM *stream)
{
    APP_PRINT_INFO1("usb_audio_stream_state_get:%08x", stream->state);
    return (stream->state);
}

static void app_usb_audio_vol_set(T_USB_AUDIO_DB *db)
{
    uint16_t vol_cur = db->vol_cur;
    uint16_t vol_range = db->vol_range;
    audio_track_volume_out_set(db->stream->track, UAVOL2DSPVOL(vol_cur, vol_range));
}

static void *usb_audio_stream_open(T_STREAM_ATTR attr, T_AUDIO_STREAM_TYPE type,
                                   T_USB_AUDIO_ASYNC_IO async_io)
{
    T_AUDIO_STREAM_TYPE  stream_type = AUDIO_STREAM_TYPE_PLAYBACK;
    T_AUDIO_STREAM_USAGE usage = AUDIO_STREAM_USAGE_LOCAL;
    T_AUDIO_STREAM_MODE mode = AUDIO_STREAM_MODE_LOW_LATENCY;
    uint32_t device = AUDIO_DEVICE_OUT_SPK;
    T_AUDIO_FORMAT_INFO format = {.type = AUDIO_FORMAT_TYPE_PCM,};
    T_USB_AUDIO_STREAM *stream = (T_USB_AUDIO_STREAM *)malloc(sizeof(T_USB_AUDIO_STREAM));

    stream->attr = attr;
    memcpy(&(format.attr.pcm), &attr, sizeof(T_STREAM_ATTR));

    if (type == AUDIO_STREAM_TYPE_PLAYBACK)
    {
        device = AUDIO_DEVICE_OUT_SPK;
    }
    else
    {
        device = AUDIO_DEVICE_IN_MIC;
    }

    stream->track =  audio_track_create(type,
                                        mode,
                                        usage,
                                        format,
                                        5,
                                        0,
                                        device,
                                        NULL,
                                        (P_AUDIO_TRACK_ASYNC_IO)async_io);
    if (stream->track)
    {
        audio_track_latency_set(stream->track, 0, true);
        usb_audio_stream_state_set(stream, STREAM_STATE_OPENED);
    }

    return stream;
}

static bool usb_audio_stream_start(T_USB_AUDIO_STREAM *stream)
{
    if (stream)
    {
        T_AUDIO_STREAM_STATE state = usb_audio_stream_state_get(stream);
        if (state == STREAM_STATE_OPENED || state == STREAM_STATE_STOPING || \
            state == STREAM_STATE_STOPED)
        {
            audio_track_start(stream->track);
            usb_audio_stream_state_set(stream, STREAM_STATE_STARTING);
            return true;
        }
    }
    APP_PRINT_ERROR1("usb_audio_stream_start failed,stream:0x%x", stream);
    return false;
}

static bool usb_audio_stream_stop(T_USB_AUDIO_STREAM *stream)
{
    if (stream)
    {
        T_AUDIO_STREAM_STATE state = usb_audio_stream_state_get(stream);
        if (state == STREAM_STATE_STARTING || state == STREAM_STATE_STARTED)
        {
            audio_track_stop(stream->track);
            usb_audio_stream_state_set(stream, STREAM_STATE_STOPED);
            return true;
        }
    }
    APP_PRINT_ERROR1("usb_audio_stream_stop failed,stream:0x%x", stream);
    return false;
}

static bool usb_audio_stream_close(T_USB_AUDIO_STREAM *stream)
{
    if (stream)
    {
        T_AUDIO_STREAM_STATE state = usb_audio_stream_state_get(stream);
        if (state >= STREAM_STATE_OPENING && state <= STREAM_STATE_STOPED)
        {
            audio_track_release(stream->track);
            usb_audio_stream_state_set(stream, STREAM_STATE_IDLE);
            stream->track = NULL;
            free(stream);
            return true;
        }
    }
    APP_PRINT_ERROR1("usb_audio_stream_close failed,stream:0x%x", stream);
    return false;
}

static bool app_usb_audio_ds_start(void)
{
    return usb_audio_stream_start(usb_audio_ds_db.stream);
}

static bool app_usb_audio_ds_stop(void)
{
    return usb_audio_stream_stop(usb_audio_ds_db.stream);
}

#if 0
RAM_TEXT_SECTION
bool usb_audio_us_track_async_read(uint32_t *timestamp,
                                   uint16_t *seq_num,
                                   uint8_t  *frame_num,
                                   void     *buf,
                                   uint16_t  required_len,
                                   uint16_t *actual_len)
{
    if ((usb_audio_db.us_track_state == AUDIO_TRACK_STATE_STARTED) ||
        (usb_audio_db.us_track_state == AUDIO_TRACK_STATE_RESTARTED))
    {
        *actual_len = 0;
        if (buf != NULL)
        {
            *actual_len = ring_buffer_write(&usb_audio_db.us_ring_buf, buf, required_len);
            uint16_t buf_used =  ring_buffer_get_data_count(&usb_audio_db.us_ring_buf);
            USB_PRINT_TRACE3("usb_audio_us_track_async_read *actual_len:%d, required_len %d, buf_used: %d",
                             *actual_len, required_len, buf_used);
            if (*actual_len != required_len)
            {
                ring_buffer_clear(&usb_audio_db.us_ring_buf);
                *actual_len = required_len; // clear this packet in media buffer
            }
        }
        return true;
    }

    return false;
}
uint8_t app_usb_audio_us_start(void)
{
    uint8_t ret = USB_AUDIO_SUCCESS;
    uint32_t param = usb_audio_db.us_param;

    T_AUDIO_STREAM_TYPE    stream_type = AUDIO_STREAM_TYPE_RECORD;
    T_AUDIO_STREAM_USAGE usage = AUDIO_STREAM_USAGE_LOCAL;
    T_AUDIO_STREAM_MODE mode = AUDIO_STREAM_MODE_NORMAL;
    uint32_t device = AUDIO_DEVICE_IN_MIC;

    T_AUDIO_FORMAT_INFO format_info;

    uint8_t bit_res = (uint8_t)(param >> 8);
    uint8_t sf_idx = (uint8_t)(param >> 16);
    uint8_t chann_mode = (uint8_t)(param >> 24);

    format_info.type = AUDIO_FORMAT_TYPE_PCM;
    format_info.attr.pcm.sample_rate = sample_freq[sf_idx];
    format_info.attr.pcm.chann_num = chann_mode;
    if (bit_res == 0)
    {
        format_info.attr.pcm.bit_width = 16;
        format_info.attr.pcm.frame_length = 1024;
    }
    else if (bit_res == 1)
    {
        format_info.attr.pcm.bit_width = 24;
        format_info.attr.pcm.frame_length = 768;
    }
    USB_PRINT_TRACE3("app_usb_audio_us_start: sf: %d, chann_num: %d, bit_width: %d",
                     format_info.attr.pcm.sample_rate,
                     format_info.attr.pcm.chann_num,
                     format_info.attr.pcm.bit_width);

    if (usb_mic_track_handle != NULL)
    {
        USB_PRINT_ERROR1("app_usb_audio_us_start track_handle %p", usb_mic_track_handle);
        audio_track_release(usb_mic_track_handle);
    }
    usb_mic_track_handle = audio_track_create(stream_type,
                                              mode,
                                              usage,
                                              format_info,
                                              0,
                                              10,
                                              device,
                                              NULL,
                                              (P_AUDIO_TRACK_ASYNC_IO)usb_audio_us_track_async_read);
    if (usb_mic_track_handle != NULL)
    {
        audio_track_start(usb_mic_track_handle);
        app_usb_audio_volume_in_set(usb_audio_db.us_vol_param);
        ring_buffer_clear(&usb_audio_db.us_ring_buf);
    }

    return ret;
}

uint8_t app_usb_audio_us_stop(void)
{
    uint8_t ret = USB_AUDIO_SUCCESS;
    USB_PRINT_TRACE0("app_usb_audio_us_stop ++");
    if (usb_mic_track_handle != NULL)
    {
        audio_track_release(usb_mic_track_handle);
        usb_mic_track_handle = NULL;
    }

    return ret;
}
#endif

void app_usb_audio_start(void)
{

    APP_PRINT_TRACE0("app_usb_audio_start ++");
    app_usb_audio_ds_start();
//    usb_audio_stream_start(usb_audio_ds_db.stream);
}
void app_usb_audio_stop(void)
{
    APP_PRINT_TRACE0("app_usb_audio_stop ++");
    app_usb_audio_ds_stop();

}

static void app_usb_audio_policy_cback(T_AUDIO_EVENT event_type, void *event_buf,
                                       uint16_t buf_len)
{
    T_AUDIO_EVENT_PARAM *param = event_buf;
    bool handle = true;

    if ((usb_audio_ds_db.stream == NULL) ||
        ((param->track_state_changed.handle != usb_audio_ds_db.stream->track) &&
         (param->track_state_changed.handle != usb_audio_us_db.stream->track)))
    {
        return;
    }

    APP_PRINT_TRACE3("app_usb_audio_policy_cback: event :0x%x, handle 0x%04x-0x%04x",
                     event_type, param->track_state_changed.handle, usb_audio_ds_db.stream->track);
    switch (event_type)
    {
    case AUDIO_EVENT_TRACK_STATE_CHANGED:
        {
            uint8_t track_state = param->track_state_changed.state;
            if (param->track_state_changed.handle == usb_audio_ds_db.stream->track)
            {
                if (track_state == AUDIO_TRACK_STATE_RELEASED)
                {
//                  usb_audio_stream_state_set(usb_audio_ds_db.stream, STREAM_STATE_IDLE);
//                  usb_audio_ds_db.stream->track = NULL;
//                  free(usb_audio_ds_db.stream);
//                  usb_audio_ds_db.stream = NULL;
                }
                else if (track_state == AUDIO_TRACK_STATE_CREATED)
                {
                    usb_audio_stream_state_set(usb_audio_ds_db.stream, STREAM_STATE_OPENED);
                }
                else if (track_state == AUDIO_TRACK_STATE_STARTED || track_state == AUDIO_TRACK_STATE_RESTARTED)
                {
                    usb_audio_stream_state_set(usb_audio_ds_db.stream, STREAM_STATE_STARTED);
                }
                else if (track_state == AUDIO_TRACK_STATE_STOPPED)
                {
                    usb_audio_stream_state_set(usb_audio_ds_db.stream, STREAM_STATE_STOPED);
                }

            }
            else if (param->track_state_changed.handle == usb_audio_us_db.stream->track)
            {

            }
        }
        break;

    case AUDIO_EVENT_TRACK_BUFFER_LOW:
        {

        }
        break;

    case AUDIO_EVENT_TRACK_BUFFER_HIGH:
        {

        }
        break;

    default:
        handle = false;
        break;
    }

    if (handle == true)
    {
        APP_PRINT_TRACE1("app_usb_audio_policy_cback: event_type 0x%04x", event_type);
    }
}

RAM_TEXT_SECTION
static bool usb_audio_send_msg(uint16_t sub_type, uint32_t param)
{
    T_IO_MSG gpio_msg;

    gpio_msg.type = IO_MSG_TYPE_USB_UAC;
    gpio_msg.subtype = sub_type;
    gpio_msg.u.param = (uint32_t)param;
    if (app_io_send_msg(&gpio_msg) == false)
    {
        APP_PRINT_ERROR1("usb_audio_send_msg(0x%x): msg send fail", sub_type);
        return false;
    }

    return true;
}

static bool usb_audio_plug_bc12(uint32_t param)
{
    return usb_audio_send_msg(MSG_TYPE_PLUG_BC12_TYPE, param);
}

static bool usb_audio_plug(uint32_t param)
{
    return usb_audio_send_msg(MSG_TYPE_PLUG, param);
}

static bool usb_audio_unplug(void)
{
    return usb_audio_send_msg(MSG_TYPE_UNPLUG, 0);
}

static bool usb_audio_active(uint8_t audio_path, uint8_t bit_res, uint8_t sf, uint8_t chann_mode)
{
    return usb_audio_send_msg(MSG_TYPE_UAC_ACTIVE,
                              (audio_path | bit_res << 8 | sf << 16 | chann_mode << 24));
}

static bool usb_audio_deactive(uint8_t audio_path)
{
    return usb_audio_send_msg(MSG_TYPE_UAC_DEACTIVE, audio_path);
}

RAM_TEXT_SECTION
static bool usb_audio_data_trans(void *buf, uint32_t len)
{
    if (usb_audio_stream_state_get(usb_audio_ds_db.stream) == STREAM_STATE_STARTED)
    {
        APP_PRINT_INFO2("usb_audio_data_trans, 0x%x-0x%x", buf, len);

        if (ring_buffer_get_remaining_space(&usb_audio_ds_db.src_buf) >= len)
        {
            ring_buffer_write(&usb_audio_ds_db.src_buf, buf, len);
        }
        else
        {
            APP_PRINT_ERROR0("app_usb_audio_data_trans, ds ring buf full!");
            ring_buffer_clear(&usb_audio_ds_db.src_buf);
            ring_buffer_write(&usb_audio_ds_db.src_buf, buf, len);
        }
    }

    return usb_audio_send_msg(MSG_TYPE_UAC_DATA_TRANS_DS, (uint32_t)buf);
}

static bool usb_audio_set_vol(uint32_t param)
{
    return usb_audio_send_msg(MSG_TYPE_UAC_SET_VOL, (uint32_t)param);
}

static bool usb_audio_get_vol(uint32_t param)
{
    return usb_audio_send_msg(MSG_TYPE_UAC_GET_VOL, (uint32_t)param);
}

static bool usb_audio_set_mute(uint32_t param)
{
    return usb_audio_send_msg(MSG_TYPE_UAC_SET_MUTE, (uint32_t)param);
}

static bool usb_audio_set_mic_vol(uint32_t param)
{
    return usb_audio_send_msg(MSG_TYPE_UAC_SET_MIC_VOL, (uint32_t)param);
}

static void app_usb_handle_msg_plug_bc12(uint32_t param)
{
    uint8_t bc12_type = param;
    APP_PRINT_TRACE1("app_usb_handle_msg_plug_bc12, bc12_type:0x%x", bc12_type);
}

static void app_usb_handle_msg_plug(void *buf)
{

}

static void app_usb_handle_msg_unplug(void *buf)
{
#if F_APP_TEAMS_FEATURE_SUPPORT
    asp_device_free_link(teams_asp_usb_addr);
#endif
}

static bool s_usb_audio_mode_flag = false;
void app_usb_audio_mode_init(void)
{
    USB_PRINT_TRACE1("app_usb_audio_mode_init usb_audio_mode: %d", s_usb_audio_mode_flag);
    if (!s_usb_audio_mode_flag)
    {
        s_usb_audio_mode_flag = true;
//        set_cpu_sleep_en(0);
        usb_start();
        app_auto_power_off_disable(AUTO_POWER_OFF_MASK_USB_AUDIO_MODE);
    }
}

void app_usb_audio_mode_deinit(void)
{
    USB_PRINT_TRACE1("app_usb_audio_mode_deinit usb_audio_mode: %d", s_usb_audio_mode_flag);
    if (s_usb_audio_mode_flag)
    {
        usb_stop();
//        set_cpu_sleep_en(1);
        s_usb_audio_mode_flag = false;
        app_auto_power_off_enable(AUTO_POWER_OFF_MASK_USB_AUDIO_MODE, app_cfg_const.timer_auto_power_off);
    }
}

void app_usb_charger_plug_handle_msg(T_IO_MSG *io_driver_msg_recv)
{
    APP_PRINT_INFO1("app_usb_charger_plug_handle_msg, device_state: %d", app_db.device_state);
//    app_usb_audio_mode_init();
    if (app_db.device_state == APP_DEVICE_STATE_OFF)
    {
        if (app_db.bt_is_ready)
        {
            app_mmi_handle_action(MMI_DEV_POWER_ON);
        }
        else
        {
            s_usb_plug_power_on = true;
        }
    }
    else
    {
        app_usb_audio_mode_init();
    }
}

void app_usb_charger_unplug_handle_msg(T_IO_MSG *io_driver_msg_recv)
{
    APP_PRINT_INFO0("app_usb_charger_unplug_handle_msg");
    // usb_stop
    gap_stop_timer(&usb_audio_timer_handle);
    app_usb_audio_mode_deinit();
    s_usb_plug_power_on = false;
}

bool app_usb_audio_plug_in_power_on_check(void)
{
    APP_PRINT_INFO1("app_usb_audio_plug_in_power_on_check,s_usb_plug_power_on:0x%d",
                    s_usb_plug_power_on);
    return s_usb_plug_power_on;
}

void app_usb_audio_plug_in_power_on_clear(void)
{
    s_usb_plug_power_on = false;
}

static void app_usb_uac_active(uint32_t param)
{
    uint8_t audio_path;
    uint8_t bit_res;
    uint8_t chann_mode;
    uint8_t sf_idx;
    T_STREAM_ATTR attr;
    audio_path = (uint8_t)param;
    bit_res = (uint8_t)(param >> 8);
    chann_mode = (uint8_t)(param >> 24);
    sf_idx = (uint8_t)(param >> 16);
    attr.chann_num = chann_mode;
    attr.sample_rate = sample_freq[sf_idx];

    if (bit_res == 0)
    {
        attr.bit_width = 16;
        attr.frame_length = 1024;
    }
    else
    {
        attr.bit_width = 24;
        attr.frame_length = 768;
    }

    if (usb_audio_ds_db.stream == NULL ||
        usb_audio_stream_state_get(usb_audio_ds_db.stream) == STREAM_STATE_IDLE)
    {
        usb_audio_ds_db.stream = usb_audio_stream_open(attr, AUDIO_STREAM_TYPE_PLAYBACK, NULL);
        uint8_t *buf = malloc(USB_AUDIO_DS_RB_SIZE);// TODO
        ring_buffer_init(&usb_audio_ds_db.src_buf, buf, USB_AUDIO_DS_RB_SIZE);
        app_usb_audio_volume_out_set(usb_audio_ds_db.vol_cur);
        usb_audio_ds_db.pkt_to_send = malloc(usb_audio_ds_db.stream->attr.frame_length);
    }

    APP_PRINT_INFO5("app_usb_uac_active_handle:"
                    "param %x-audio_path %x-bit_res %x, chann_mode %d-sf_ds %x- sf_us 0x%x",
                    param, audio_path, bit_res, chann_mode, attr.frame_length);

}

static void app_usb_handle_msg_uac_active(void *buf)
{
    uint32_t param = (uint32_t)buf;

    app_usb_uac_active(param);
}

static uint8_t pkt_num_to_send(T_USB_AUDIO_DB *db, bool *dummy)
{
    uint8_t ret = 0;
    uint16_t track_buf_level_ms = 0;
    uint8_t src_buf_pkt_num = ring_buffer_get_data_count(&db->src_buf) /
                              (db->stream->attr.frame_length);

    if (src_buf_pkt_num > 0)
    {
        if (audio_track_buffer_level_get(db->stream->track, &track_buf_level_ms))
        {
            if (track_buf_level_ms == 0)
            {
                ret = 10; //TODO
                *dummy = true;
            }
            else if (track_buf_level_ms >= 10)
            {
                ret = 0;
            }
            else
            {
                ret = src_buf_pkt_num;
                *dummy = false;
            }
        }
        else
        {
            ret = 10;
            *dummy = true;
        }
    }

    return ret;
}

RAM_TEXT_SECTION
static void usb_audio_data_xmit_ds(T_USB_AUDIO_DB *db)
{
    bool dummy = false;
    uint16_t pkt_size = db->stream->attr.frame_length;
    uint8_t pkt_num = pkt_num_to_send(db, &dummy);
    static uint16_t s_seq_num = 0;

    if (usb_audio_stream_state_get(db->stream) != STREAM_STATE_STARTED)
    {
        return;
    }

    for (uint8_t i = 0; i < pkt_num; i++)
    {
        uint16_t written_len;
        if (!dummy)
        {
            ring_buffer_peek(&(db->src_buf), pkt_size, db->pkt_to_send);
        }
        else
        {
            memset(db->pkt_to_send, 0, pkt_size);
        }
        if (audio_track_write(db->stream->track,
                              0,//              timestamp,
                              s_seq_num,
                              AUDIO_STREAM_STATUS_CORRECT,
                              1,//            frame_num,
                              (uint8_t *)db->pkt_to_send,
                              pkt_size,
                              &written_len) == false)
        {
            USB_PRINT_ERROR0("audio_track_write fail!!!");
        }
        else
        {
            if (!dummy)
            {
                ring_buffer_remove(&(db->src_buf), pkt_size);
            }
        }
        s_seq_num++;
    }

}

RAM_TEXT_SECTION
static void app_usb_handle_msg_data_xmit_ds(void *buf)
{
    usb_audio_data_xmit_ds(&usb_audio_ds_db);
}

void app_usb_audio_volume_up(void)
{
    uint8_t max_volume;
    uint8_t curr_volume;
    T_AUDIO_TRACK_HANDLE usb_track_handle = usb_audio_ds_db.stream->track;

    audio_track_volume_out_max_get(usb_track_handle, (uint8_t *)&max_volume);
    audio_track_volume_out_get(usb_track_handle, (uint8_t *)&curr_volume);

    APP_PRINT_TRACE2("app_usb_audio_volume_up,volume:%d,max:%d", curr_volume, max_volume);
    if (curr_volume < max_volume)
    {
        curr_volume++;
        audio_track_volume_out_set(usb_track_handle, curr_volume);
    }
    else
    {
        audio_track_volume_out_set(usb_track_handle, max_volume);
    }
}
void app_usb_audio_volume_down(void)
{
    uint8_t min_volume;
    uint8_t curr_volume;
    T_AUDIO_TRACK_HANDLE usb_track_handle = usb_audio_ds_db.stream->track;;

    audio_track_volume_out_min_get(usb_track_handle, (uint8_t *)&min_volume);
    audio_track_volume_out_get(usb_track_handle, (uint8_t *)&curr_volume);

    APP_PRINT_TRACE2("app_usb_audio_volume_down,volume:%d,min:%d", curr_volume, min_volume);
    if (curr_volume > min_volume)
    {
        curr_volume--;
        audio_track_volume_out_set(usb_track_handle, curr_volume);
    }
    else
    {
        audio_track_volume_out_set(usb_track_handle, min_volume);
    }
}

void app_usb_audio_volume_out_set(int16_t vol_param)
{
    if (usb_audio_ds_db.stream)
    {
        T_AUDIO_TRACK_HANDLE usb_track_handle = usb_audio_ds_db.stream->track;
        uint8_t volume = UAVOL2DSPVOL(vol_param, usb_audio_ds_db.vol_range);
        if (volume >= 0x0F)
        {
            volume = 0x0F;
        }
        APP_PRINT_TRACE2("app_usb_audio_volume_out_set vol_param:%d, volume: %d", vol_param, volume);
        if (usb_track_handle != NULL)
        {
            audio_track_volume_out_set(usb_track_handle, volume);
        }
    }
}

void app_usb_audio_volume_in_set(int16_t vol_param)
{

}

void app_usb_audio_volume_in_mute(void)
{
    T_AUDIO_TRACK_HANDLE usb_track_handle = NULL;

    APP_PRINT_TRACE0("app_usb_audio_volume_in_mute ++");

    usb_track_handle = usb_audio_us_db.stream->track;
    if (usb_track_handle != NULL)
    {
        audio_track_volume_in_mute(usb_track_handle);
    }
}

void app_usb_audio_volume_in_unmute(void)
{
    T_AUDIO_TRACK_HANDLE usb_track_handle = NULL;

    APP_PRINT_TRACE0("app_usb_audio_volume_in_unmute ++");

    usb_track_handle = usb_audio_us_db.stream->track;
    if (usb_track_handle != NULL)
    {
        audio_track_volume_in_unmute(usb_track_handle);
    }
}

uint8_t app_usb_audio_is_mic_mute(void)
{
    return g_teams_is_mute;
}

static void app_usb_handle_msg_uac_deactive(void *buf)
{
    uint32_t param = (uint32_t)buf;
    uint8_t audio_path = (uint8_t)param;

    APP_PRINT_INFO1("app_usb_handle_msg_uac_deactive: audio_path:0x%x", audio_path);

    if (audio_path == DIR_DS)
    {
        audio_stream_mask &= ~USB_AUDIO_DS_MASK;
        usb_audio_stream_close(usb_audio_ds_db.stream);
        usb_audio_ds_db.stream = NULL;
        free(usb_audio_ds_db.pkt_to_send);
        usb_audio_ds_db.pkt_to_send = NULL;
        free(usb_audio_ds_db.src_buf.buf);
        ring_buffer_deinit(&usb_audio_ds_db.src_buf);
        gap_stop_timer(&usb_audio_timer_handle);
#if F_APP_SINGLE_MUTLILINK_SCENERIO_1
        app_teams_multilink_update_usb_stream_status(APP_TEAMS_MULTILINK_USB_DOWNSTREAM_STOP);
#endif
    }
    else if (audio_path == DIR_US)
    {
        audio_stream_mask &= ~USB_AUDIO_US_MASK;
//        app_usb_audio_us_stop();
#if F_APP_SINGLE_MUTLILINK_SCENERIO_1
        app_teams_multilink_update_usb_stream_status(APP_TEAMS_MULTILINK_USB_UPSTREAM_STOP);
#endif
    }
}

static void app_usb_handle_msg_set_mic_vol(uint32_t param)
{

}

bool app_usb_audio_is_us_streaming(void)
{
    return (audio_stream_mask & USB_AUDIO_US_MASK);
}

bool app_usb_audio_is_ds_streaming(void)
{
    return (audio_stream_mask & USB_AUDIO_DS_MASK);
}

void app_usb_audio_handle_msg(T_IO_MSG *io_driver_msg_recv)
{
    T_IO_MSG *usb_msg = io_driver_msg_recv;
    bool handle = true;

    uint16_t usb_msg_type = usb_msg->subtype;

    switch (usb_msg_type)
    {
    case MSG_TYPE_PLUG_BC12_TYPE:
        {
            app_usb_handle_msg_plug_bc12(usb_msg->u.param);
        }
        break;
    case MSG_TYPE_PLUG:
        {
            app_usb_handle_msg_plug(usb_msg->u.buf);
#if F_APP_SINGLE_MUTLILINK_SCENERIO_1
            app_teams_multilink_handle_link_connected(multilink_usb_idx);
            if (!app_connected_profile_link_num(HFP_PROFILE_MASK | HSP_PROFILE_MASK))
            {
                app_teams_multilink_handle_first_voice_profile_connect(multilink_usb_idx);
            }
#endif
            //app_mmi_handle_action(0xfc);
        }
        break;
    case MSG_TYPE_UNPLUG:
        {
            app_usb_handle_msg_unplug(usb_msg->u.buf);
#if F_APP_SINGLE_MUTLILINK_SCENERIO_1
            app_teams_multilink_handle_link_disconnected(multilink_usb_idx);
            app_teams_multilink_handle_voice_profile_disconnect(multilink_usb_idx);
#endif
        }
        break;
    case MSG_TYPE_UAC_ACTIVE:
        {
            app_usb_handle_msg_uac_active(usb_msg->u.buf);
        }
        break;
    case MSG_TYPE_UAC_DEACTIVE:
        {
            app_usb_handle_msg_uac_deactive(usb_msg->u.buf);
        }
        break;
    case MSG_TYPE_UAC_DATA_TRANS_DS:
        {
#if F_APP_SINGLE_MUTLILINK_SCENERIO_1
            app_teams_multilink_update_usb_stream_status(APP_TEAMS_MULTILINK_USB_DOWNSTREAM_START);
#endif
            app_usb_audio_ds_check_start_timer(USB_AUDIO_DS_CHECK_INTERVAL);

            APP_PRINT_INFO0("app_usb_handle_msg_data_xmit_ds");
            app_usb_handle_msg_data_xmit_ds(usb_msg->u.buf);
            handle = false;
        }
        break;

    case MSG_TYPE_UAC_SET_VOL:
        {
            usb_audio_ds_db.vol_cur = usb_msg->u.param;
            app_usb_audio_volume_out_set(usb_audio_ds_db.vol_cur);
            APP_PRINT_INFO1("MSG_TYPE_UAC_SET_VOL para: %x", usb_audio_ds_db.vol_cur);

        }
        break;

    case MSG_TYPE_UAC_GET_VOL:
        {
            T_USB_AUDIO_VOL vol = *((T_USB_AUDIO_VOL *) & (usb_msg->u.param));
            if (vol.type == USB_AUDIO_VOL_TYPE_CUR)// mute
            {

            }
            else if (vol.type == USB_AUDIO_VOL_TYPE_RANGE)
            {
                usb_audio_ds_db.vol_range = vol.value;
            }
            APP_PRINT_INFO2("MSG_TYPE_UAC_GET_VOL para: 0x%x-0x%x", usb_msg->u.param,
                            usb_audio_ds_db.vol_range);
        }
        break;

    case MSG_TYPE_UAC_SET_MUTE:
        {
            bool mute  = (bool)usb_msg->u.param;

            if (mute)
            {
                audio_track_volume_out_mute(usb_audio_ds_db.stream->track);
            }
            else
            {
                audio_track_volume_out_unmute(usb_audio_ds_db.stream->track);
            }

            APP_PRINT_INFO1("MSG_TYPE_UAC_GET_VOL para: 0x%x", usb_msg->u.param);
        }
        break;

    case MSG_TYPE_UAC_SET_MIC_VOL:
        {
            app_usb_handle_msg_set_mic_vol(usb_msg->u.param);
        }
        break;
    default:
        handle = false;
        break;
    }
    if (handle == true)
    {
        APP_PRINT_INFO1("app_usb_handle_msg: msg 0x%08x", usb_msg_type);
    }
}

bool app_usb_connected(void)
{
    APP_PRINT_INFO1("app_usb_connected: %d", app_cfg_nv.adaptor_is_plugged);

    return (app_cfg_nv.adaptor_is_plugged == 1);
}

bool app_usb_audio_is_active(void)
{
//    APP_PRINT_INFO1("app_usb_audio_is_active: %d", g_usb_audio_connected);
//    return (g_usb_audio_connected == 1);
    return true;
}

void app_usb_audio_ds_check_start_timer(uint16_t time_ms)
{
    static uint16_t cnt = 0;
    if (cnt < time_ms / 5)
    {
        cnt++;
    }
    else
    {
        gap_start_timer(&usb_audio_timer_handle, "usb_audio_ds_check",
                        usb_audio_timer_queue_id, USB_TIMER_USB_STREAM_CHECK, 0, time_ms);
        cnt = 0;
    }
}

static void app_usb_audio_timer_callback(uint8_t timer_id, uint16_t timer_chann)
{
    APP_PRINT_TRACE2("app_usb_audio_timer_callback: timer_id 0x%02x, timer_chann 0x%x",
                     timer_id, timer_chann);

    switch (timer_id)
    {

    case USB_TIMER_USB_STREAM_CHECK:
        {
            gap_stop_timer(&usb_audio_timer_handle);

            app_usb_audio_stop();
#if F_APP_SINGLE_MUTLILINK_SCENERIO_1
            app_teams_multilink_update_usb_stream_status(APP_TEAMS_MULTILINK_USB_DOWNSTREAM_STOP);
#endif
        }
        break;

    default:
        break;
    }
}

static void app_usb_audio_dm_cback(T_SYS_EVENT event_type, void *event_buf, uint16_t buf_len)
{
    bool handle = true;

    switch (event_type)
    {
    case SYS_EVENT_POWER_OFF:
        {
            app_usb_audio_mode_deinit();
        }
        break;

    case SYS_EVENT_POWER_ON:
        {
            if (app_cfg_nv.adaptor_is_plugged)
            {
                app_usb_audio_mode_init();
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
        APP_PRINT_INFO1("app_usb_audio_dm_cback: event_type 0x%04x", event_type);
    }
}

static const USB_AUDIO_STREAM_CB app_usb_driver_cbs[DRIVER_CB_MAX] =
{
    [DRIVER_CB_ACTIVE] = (USB_AUDIO_STREAM_CB)usb_audio_active,
    [DRIVER_CB_DEACTIVE] = (USB_AUDIO_STREAM_CB)usb_audio_deactive,
    [DRIVER_CB_DATA_TRANS] = (USB_AUDIO_STREAM_CB)usb_audio_data_trans,
};

static const USB_AUDIO_CTRL_CB app_usb_ctrl_cbs[CTRL_CB_MAX] =
{
    [CTRL_CB_SPK_VOL_SET] = (USB_AUDIO_CTRL_CB)usb_audio_set_vol,
    [CTRL_CB_SPK_VOL_GET] = (USB_AUDIO_CTRL_CB)usb_audio_get_vol,
    [CTRL_CB_SPK_MUTE_SET] = (USB_AUDIO_CTRL_CB)usb_audio_set_mute,
};

void app_usb_audio_init(void)
{
    usb_audio_init();
    usb_audio_streaming_cb_register((USB_AUDIO_STREAM_CB *)app_usb_driver_cbs);
    usb_audio_ctrl_cb_register((USB_AUDIO_CTRL_CB *)app_usb_ctrl_cbs);
    usb_init();//TODO
    audio_mgr_cback_register(app_usb_audio_policy_cback);

    sys_mgr_cback_register(app_usb_audio_dm_cback);

    gap_reg_timer_cb(app_usb_audio_timer_callback, &usb_audio_timer_queue_id);
}
#endif
