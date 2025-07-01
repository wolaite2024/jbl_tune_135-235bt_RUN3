#if (F_APP_DONGLE_FEATURE_SUPPORT == 1)
#include "app_main.h"
#include "app_multilink.h"
#include "app_dongle_record.h"
#include "audio_track.h"
#include "os_mem.h"
#include "trace.h"
#include "app_cfg.h"
#include "app_roleswap_control.h"

/*============================================================================*
  *                                        Variables
  *============================================================================*/
typedef struct
{
    bool is_start;
    uint8_t bd_addr[BD_ADDR_LENGTH];
    T_AUDIO_TRACK_HANDLE handle;
} APP_DONGLE_RECORD;

static APP_DONGLE_RECORD dongle_record = {.is_start = false, .bd_addr = {0}, .handle = NULL};

#define DONGLE_RECORD_DATA_DBG 0

#if DONGLE_RECORD_DATA_DBG
static void dongle_dump_record_data(const char *title, uint8_t *record_data_buf, uint32_t data_len)
{
    const uint32_t bat_num = 8;
    uint32_t times = data_len / bat_num;
    uint32_t residue = data_len % bat_num;
    uint8_t *residue_buf = record_data_buf + times * bat_num;

    APP_PRINT_TRACE3("dongle_dump_record_data0: data_len %d, times %d, residue %d", data_len,
                     times, residue);
    APP_PRINT_TRACE2("dongle_dump_record_data1: record_data_buf is 0x%08x, residue_buf is 0x%08x\r\n",
                     (uint32_t)record_data_buf,
                     (uint32_t)residue_buf);

    for (int32_t i = 0; i < times; i++)
    {
        APP_PRINT_TRACE8("dongle_dump_record_data2: 0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x\r\n",
                         record_data_buf[i * bat_num], record_data_buf[i * bat_num + 1], record_data_buf[i * bat_num + 2],
                         record_data_buf[i * bat_num + 3],
                         record_data_buf[i * bat_num + 4], record_data_buf[i * bat_num + 5],
                         record_data_buf[i * bat_num + 6],
                         record_data_buf[i * bat_num + 7]);
    }

    switch (residue)
    {
    case 1:
        APP_PRINT_TRACE1("dongle_dump_record_data3: 0x%02x\r\n", residue_buf[0]);
        break;
    case 2:
        APP_PRINT_TRACE2("dongle_dump_record_data4: 0x%02x, 0x%02x\r\n", residue_buf[0], residue_buf[1]);
        break;
    case 3:
        APP_PRINT_TRACE3("dongle_dump_record_data5: 0x%02x, 0x%02x, 0x%02x\r\n", residue_buf[0],
                         residue_buf[1],
                         residue_buf[2]);
        break;
    case 4:
        APP_PRINT_TRACE4("dongle_dump_record_data6: 0x%02x, 0x%02x, 0x%02x, 0x%02x\r\n", residue_buf[0],
                         residue_buf[1], residue_buf[2], residue_buf[3]);
        break;
    case 5:
        APP_PRINT_TRACE5("dongle_dump_record_data7: 0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x\r\n",
                         residue_buf[0], residue_buf[1], residue_buf[2], residue_buf[3], residue_buf[4]);
        break;
    case 6:
        APP_PRINT_TRACE6("dongle_dump_record_data8: 0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x\r\n",
                         residue_buf[0], residue_buf[1], residue_buf[2], residue_buf[3], residue_buf[4], residue_buf[5]);
        break;
    case 7:
        APP_PRINT_TRACE7("dongle_dump_record_data9: 0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x\r\n",
                         residue_buf[0], residue_buf[1], residue_buf[2], residue_buf[3], residue_buf[4], residue_buf[5],
                         residue_buf[6]);
        break;

    default:
        break;
    }
}
#endif

bool app_dongle_record_read_cb(T_AUDIO_TRACK_HANDLE   handle,
                               uint32_t              *timestamp,
                               uint16_t              *seq_num,
                               T_AUDIO_STREAM_STATUS *status,
                               uint8_t               *frame_num,
                               void                  *buf,
                               uint16_t               required_len,
                               uint16_t              *actual_len)
{
    APP_PRINT_TRACE2("app_dongle_record_read_cb: buf 0x%08x, required_len %d", buf, required_len);

    uint8_t app_idx = app_get_active_a2dp_idx();

#if DONGLE_RECORD_DATA_DBG
    dongle_dump_record_data("app_dongle_record_read_cb", buf, required_len);
#endif

    if ((app_db.remote_is_8753bau) && (app_db.dongle_is_enable_mic)
        && (app_roleswap_ctrl_get_status() == APP_ROLESWAP_STATUS_IDLE))
    {
        app_dongle_mic_data_report(buf, required_len);
    }

    *actual_len = required_len;

    return true;
}

static void dongle_voice_gen_format_info(T_AUDIO_FORMAT_INFO *p_format_info)
{
    p_format_info->type = AUDIO_FORMAT_TYPE_SBC;
    p_format_info->attr.sbc.sample_rate = 16000;
    p_format_info->attr.sbc.chann_mode = AUDIO_SBC_CHANNEL_MODE_MONO;
    p_format_info->attr.sbc.block_length = 16;
    p_format_info->attr.sbc.subband_num = 8;
    p_format_info->attr.sbc.allocation_method = 0;
    p_format_info->attr.sbc.bitpool = 22;
}

void app_dongle_force_stop_recording(void)
{
    APP_PRINT_TRACE0("app_dongle_force_stop_recording");
    dongle_record.is_start = false;
    audio_track_stop(dongle_record.handle);
}

/**
    * @brief        This function can stop the record.
    * @return       void
    */
void app_dongle_stop_recording(uint8_t bd_addr[6])
{
    if (memcmp(dongle_record.bd_addr, bd_addr, sizeof(dongle_record.bd_addr)) != 0)
    {
        APP_PRINT_ERROR0("dongle_voice_stop_capture: bd_addr is not matched!");
        return;
    }

    if (dongle_record.is_start != true)
    {
        APP_PRINT_ERROR0("dongle_voice_stop_capture: already stopped!");
        return;
    }

    APP_PRINT_TRACE0("app_dongle_stop_recording");
    dongle_record.is_start = false;
    audio_track_stop(dongle_record.handle);
}

/**
    * @brief        This function can start the record.
    * @return       void
    */
void app_dongle_start_recording(uint8_t bd_addr[6])
{
    if (dongle_record.is_start != false)/*g_voice_data.is_voice_start == false8*/
    {
        APP_PRINT_ERROR0("app_dongle_start_recording: already started");
        return;
    }

    APP_PRINT_INFO0("app_dongle_start_recording");
    dongle_record.is_start = true;
    memcpy(dongle_record.bd_addr, bd_addr, sizeof(dongle_record.bd_addr));
    audio_track_start(dongle_record.handle);
}

void app_dongle_record_init(void)
{
    T_AUDIO_FORMAT_INFO format_info = {};

    dongle_voice_gen_format_info(&format_info);

    /* BAU dongle uses a2dp encode for recording, it is same as normal apt.*/
    dongle_record.handle = audio_track_create(AUDIO_STREAM_TYPE_RECORD,
                                              AUDIO_STREAM_MODE_NORMAL,
                                              AUDIO_STREAM_USAGE_LOCAL,
                                              format_info,
                                              0,
                                              app_cfg_const.apt_volume_in_default,
                                              AUDIO_DEVICE_IN_MIC,
                                              NULL,
                                              app_dongle_record_read_cb);

    if (dongle_record.handle == NULL)
    {
        APP_PRINT_ERROR0("app_dongle_record_init: handle is NULL");
    }
}
#endif
