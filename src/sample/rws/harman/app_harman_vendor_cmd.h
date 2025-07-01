#ifndef _APP_HARMAN_VENDOR_CMD_H_
#define _APP_HARMAN_VENDOR_CMD_H_

#include <stdint.h>
#include <stdbool.h>
#include "audio_track.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* direct send to connected device */
#define HARMAN_CMD_DIRECT_SEND       0xDD00
/* forward to accessory device after recv cmd, such as dongle or charging case */
#define HARMAN_CMD_FORWARD           0xDD01

#define STATUS_SUCCESS              0x0000
#define GENERAL_ERROR               0xFF

#define FEATURE_VALUE_SIZE_1B       1
#define FEATURE_VALUE_SIZE_2B       2
#define FEATURE_VALUE_SIZE_3B       3
#define FEATURE_VALUE_SIZE_4B       4
#define BD_ADDR_SIZE                6
#define SMART_SWITCH_SIZE           8
#define DEVICE_NAME_SIZE            16
#define EQ_INFO_SIZE                0x13

#define HARMAN_LR_BALANCE_MID_VOL_LEVEL     0x32

typedef enum t_harman_status_error_category
{
    STATUS_ERROR_PROTOCOL              = 0x8100,
    STATUS_ERROR_GENERAL_BUSINESS      = 0x8200,
    STATUS_ERROR_HEADPHONE_BUSINESS    = 0x8300,
    STATUS_ERROR_PARTY_BOX_BUSINESS    = 0x8400,
    STATUS_ERROR_PORTABLE_BUSINESS     = 0x8500,
} T_HARMAN_STATUS_ERROR_CATEGORY;

typedef enum t_harman_protocol_error_code
{
    FIRST_TIMEOUT                      = 0x0001,
    SECOND_TIMEOUT                     = 0x0002,
    PACKET_MISSING                     = 0x0003,
    UNKNOW_IDENTIFIER                  = 0x0004,
    UNSUPPORTED_PROTOCOL_VERSION       = 0x0005,
    UNSUPPORTED_COMMAND_ID             = 0x0006,
    WRONG_PACKET_COUNT                 = 0x0007,
    WRONG_PACKET_INDEX                 = 0x0008,
} T_HARMAN_PROTOCOL_ERROR_CODE;

typedef enum t_harman_general_business_error_code
{
    ALREADY_IN_OTA                      = 0x0001,
    BATTERY_LOW                         = 0x0002,
    BUSY_IN_CALL                        = 0x0003,
    BUSY_IN_AUDIO_STREAMING             = 0x0004,
    BUSY_IN_AI                          = 0x0005,
    SIGNATURE_VERIFICATION_FAILED       = 0x0006,
    WRONG_PASSWORD                      = 0x0007,
    UNSUPPORTED_OTA_PACKAGE_VERSION     = 0x0008,
} T_HARMAN_GENERAL_BUSINESS_ERROR_CODE;

typedef enum t_harman_headphone_business_error_code
{
    TWS_IN_MONO_MODE                     = 0x0001,
    GA_IN_UPGRADING                      = 0x0002,
    AMAZON_IN_UPGRADING                  = 0x0003,
    XIAOWEI_IN_UPGRADING                 = 0x0004,
    UNKNOWN_AI_IN_UPGRADING              = 0x0005,
} T_HARMAN_HEADPHONE_BUSINESS_ERROR_CODE;

typedef enum t_harman_vendor_cmd_id
{
    CMD_HARMAN_DEVICE_INFO_GET              = 0x0001, // App gets device info from device
    CMD_HARMAN_DEVICE_INFO_SET              = 0x0002, // App sets device info to device
    CMD_HARMAN_DEVICE_INFO_DEVICE_NOTIFY    = 0x0003, // Device notify info to App
    CMD_HARMAN_DEVICE_INFO_APP_NOTIFY       = 0x0004, // App notify info to device

    CMD_HARMAN_OTA_START                    = 0x0101, // App sends the request to start OTA
    CMD_HARMAN_OTA_PACKET                   = 0x0102, // App sends the OTA Data to Device
    CMD_HARMAN_OTA_STOP                     = 0x0103, // App sends the request to stop OTA
    CMD_HARMAN_OTA_NOTIFICATION             = 0x0104, /* Once the Device initiates firmware update mode,
                                                    it will inform the App of any changes in either
                                                    the Status Code or Percentage */
} T_HARMAN_VENDOR_CMD_ID;

typedef enum _t_harman_devinfo_feature_id
{
    FEATURE_UNSUPPORT                           = 0x0000,
    FEATURE_MTU                                 = 0x0001,
    FEATURE_HEARTBEAT                           = 0x0002,
    FEATURE_CONTINUOUS_PACKETS_MIN_TX_INTERVAL  = 0x0003,
    FEATURE_PID                                 = 0x0004,
    FEATURE_COLOR_ID                            = 0x0005,
    FEATURE_DEVICE_NAME                         = 0x0006,
    FEATURE_LEFT_DEVICE_SERIAL_NUM              = 0x0008,
    FEATURE_RIGHT_DEVICE_SERIAL_NUM             = 0x0009,
    FEATURE_MAC_ADDR                            = 0x000A,
    FEATURE_FIEMWARE_VERSION                    = 0x000C,
    FEATURE_LEFT_DEVICE_BATT_STATUS             = 0x000D,
    FEATURE_RIGHT_DEVICE_BATT_STATUS            = 0x000E,
    FEATURE_FACTORY_RESET                       = 0x000F,
    FEATURE_BT_CONN_STATUS                      = 0x0010,
    FEATURE_MANUAL_POWER_OFF                    = 0x0011,
    FEATURE_AUTO_POWER_OFF                      = 0x0012,
    FEATURE_AUTO_STANDBY                        = 0x0013,
    FEATURE_LR_BALANCE                          = 0x0015,
    FEATURE_ANC                                 = 0x0018,
    FEATURE_SIDETONE                            = 0x0023,
    FEATURE_MAX_VOLUME_LIMIT                    = 0x0025,
    FEATURE_AUTO_PLAY_OR_PAUSE_ENABLE           = 0x002A,
    FEATURE_CHARGING_PIN_DETECT                 = 0x002C,
    FEATURE_PAIRED_STATUS                       = 0x002E,
    FEATURE_HOST_TYPE                           = 0x002F,

    FEATURE_WATER_EJECTION                      = 0x0037,
    FEATURE_PERSONIFI                           = 0x003A,

    FEATURE_OTA_DATA_MAX_SIZE_IN_ONE_PACK       = 0x0B00,
    FEATURE_OTA_CONTINUOUS_PACKETS_MAX_COUNT    = 0x0B01,
    FEATURE_OTA_UPGRADE_STATUS                  = 0x0B02,

    FEATURE_MIC_GAIN                            = 0x0C04,
    FEATURE_VOICE_PROMPT_STATUS                 = 0x0C40,
    FEATURE_VOICE_PROMPT_LANGUAGE               = 0x0C41,
    FEATURE_VOICE_PROMPT_FILE_VERSION           = 0x0C42,
    FEATURE_SLEEP_ACTIVE_STATUS                 = 0x0CB0,
    FEATURE_SLEEP_SETTING                       = 0x0CB1,
    FEATURE_WAKE_UP_SETTING                     = 0x0CB2,
    FEATURE_SLEEP_COUNT_DOWN_SETTING            = 0x0CB3,

    FEATURE_LIGHTING_STATUS                     = 0x0D00,
    FEATURE_LIGHTING_INFO_QUERY                 = 0x0D01,
    FEATURE_LIGHTING_INFO                       = 0x0D02,

    FEATURE_EQ_INFO_QUERY                       = 0x0E01,
    FEATURE_EQ_INFO                             = 0x0E02,
    FEATURE_GET_DESIGN_EQ_RAW_DATA              = 0x0E7E,
    FEATURE_EQ_CURVE                            = 0x0E7F,
    FEATURE_DYNAMIC_EQ                          = 0x0E80,
    FEATURE_CUSTOM_EQ_COUNT                     = 0x0E81,

    FEATURE_TWS_INFO                            = 0x1000,
    FEATURE_IN_EAR_STATUS                       = 0x1002,
    FEATURE_SEALING_TEST_STATUS                 = 0x1003,

    FEATURE_TOTAL_PLAYBACK_TIME                 = 0x1008,
    FEATURE_TOTAL_POWER_ON_TIME                 = 0x1009,
    FEATURE_FIND_MY_BUDS_STATUS                 = 0x100A,
    FEATURE_CALL_STATUS                         = 0x100B,
    FEATURE_CALL_CONTROL                        = 0x100e,
    FEATURE_SMART_SWITCH                        = 0x100F,

    FEATURE_PLAYER_OPERATION                    = 0x2D40,
    FEATURE_MUTE_STATUS                         = 0x2D41,
    FEATURE_PLAYER_VOLUME                       = 0x2D42,

    FEATURE_BATTERY_ID                          = 0x2E00,
    FEATURE_BATTERY_REMAIN_PLAYTIME             = 0x2E01,
    FEATURE_BATTERY_TEMP_MAX                    = 0x2E02,
    FEATURE_BATTERY_REMAIN_CAPACITY             = 0x2E03,
    FEATURE_BATTERY_FULL_CHARGE_CAPACITY        = 0x2E04,
    FEATURE_BATTERY_DESIGN_CAPACITY             = 0x2E05,
    FEATURE_BATTERY_CYCLE_COUNT                 = 0x2E06,
    FEATURE_BATTERY_STATE_OF_HEALTH             = 0x2E07,
    FEATURE_BATTERY_CHARGING_STATUS             = 0x2E08,
    FEATURE_BATTERY_TOTAL_POWER_ON_DURATION     = 0x2E0A,
} T_HARMAN_DEVINFO_FEATURE_ID;

// typedef enmu _t_app_harman_gesture_func_type
// {
//     EMPTY                                   = 0x00,
//     VOLUME_UP                               = 0x01,
//     VOLUME_DOWN                             = 0x02,
//     AMBIENT_AWARE                           = 0x03,
//     TALK_THRU                               = 0x04,
//     NEXT_TRACK                              = 0x05,
//     PREVIOUS_TRACK                          = 0x06,
//     ANC                                     = 0x07,
//     PLAY_OR_PAUSE                           = 0x08,
//     ANC_OR_AMBIENT_AWARE_OFF                = 0x09,
//     PLAY_OR_PAUSE_VA                        = 0x0A,
//     ANC_OR_AMBIENT_AWARE                    = 0x0B,
//     ANC_OFF                                 = 0x0C,
//     AMBIENT_AWARE_OFF                       = 0x0D,
//     BT_TONGLE                               = 0x0E,

//     CANCEL_DEFAULT_ASSISTANT                = 0xA0,
//     TALK_TO_DEFAULT_ASSISTANT               = 0xA1,
//     CANCEL_GOOGLE_ASSISTANT                 = 0xA2,
//     GOOGLE_ASSISTANT_NOTIFICATION_CALL_OUT  = 0xA3,
//     TALK_TO_GOOGLE_ASSISTANT                = 0xA4,
//     CANCEL_AMAZON_ALEXA                     = 0xA5,
//     TALK_TO_AMAZON_ALEXA                    = 0xA6,
//     CANCEL_TENCENT_XIAOWEI                  = 0xA7,
//     ALK_TO_TENCENT_XIAOWEI                  = 0xA8,

//     VOLUME                                  = 0xB0,
//     GAME_OR_CHAT_BALANCE                    = 0xB1,
//     EQ_PRESENT_SELECTION                    = 0xB2,
//     MIC_VOLUME                              = 0xB3,
//     CHANGE_VOICE_EFFECT                     = 0xB4,
//     CHANGE_SIDETONE                         = 0xB5,

//     MIC_ON_OR_OFF                           = 0xC0,
//     LIGHTING_ON_OR_OFF                      = 0xC1,
//     SPATIAL_SOUND_OR_HEAD_TRACING_OFF       = 0xC2,
//     ANC_OR_TT_OFF                           = 0xC3,
//     SEQUENTIAL_FEATURES                     = 0xC4,
//     MACRO                                   = 0xC5,
// } T_APP_HARMAN_GESTURE_FUNC_TYPE;

typedef struct t_harman_cmd_header
{
    uint16_t sync_word;                 // 2bytes, 0xDD00 or 0xDD01
    T_HARMAN_VENDOR_CMD_ID cmd_id;      // 2bytes
    uint8_t packet_count;
    uint8_t packet_index;
    uint16_t payload_len;
} T_HARMAN_CMD_HEADER;

typedef struct t_harman_feature_id_header
{
    uint16_t feature_id;
    uint16_t value_size;
} T_HARMAN_FEATURE_ID_HEADER;

#define  HARMAN_CMD_HEADER_SIZE             sizeof(T_HARMAN_CMD_HEADER)
#define  HARMAN_FEATURE_ID_HEADER_SIZE      sizeof(T_HARMAN_FEATURE_ID_HEADER)
#define  HARMAN_ABSOLUTE_OFFSET_SIZE        sizeof(uint32_t)

typedef enum
{
    SIDETOME_LEVEL_L              = 0x01,
    SIDETOME_LEVEL_M              = 0x02,
    SIDETOME_LEVEL_H              = 0x03,
    SIDETOME_LEVEL_MAX            = 0x04,
} T_SIDETOME_LEVEL;

typedef enum
{
    SIDETONE_SWTCH_DISABLE        = 0x00,
    SIDETONE_SWTCH_ENABLE         = 0x01,
} T_SIDETONE_SWTCH;

void app_harman_lr_balance_set(T_AUDIO_STREAM_TYPE volume_type, uint8_t vol,
                               const char *func_name, const uint32_t line_no);
void harman_set_vp_ringtone_balance(const char *func_name, const uint32_t line_no);
void app_harman_ble_paired_status_notify(uint8_t ble_paired_status, uint8_t app_idx);
void app_harman_battery_status_notify(void);
void app_harman_sco_status_notify(void);
void harman_sidetone_set_dsp(void);
void app_harman_devinfo_notify(uint8_t le_idx);

uint8_t app_harman_cmd_support_check(uint16_t cmd_id, uint16_t data_len, uint8_t id);
void app_harman_vendor_cmd_process(uint8_t *cmd_ptr, uint16_t cmd_len, uint8_t cmd_path,
                                   uint8_t rx_seqn, uint8_t app_idx);
void app_harman_vendor_cmd_init(void);
void app_harman_recon_a2dp(uint8_t idx);

uint8_t app_harman_cmd_support_check(uint16_t cmd_id, uint16_t data_len, uint8_t id);
uint16_t app_harman_devinfo_feature_pack(uint8_t *p_buf, uint16_t id, uint16_t size,
                                         uint8_t *value);
void app_harman_devinfo_notification_report(uint8_t *p_buf, uint16_t buf_len, uint8_t app_idx);
uint8_t harman_get_active_mobile_cmd_link(uint8_t *idx);
void app_harman_find_my_buds_ring_stop(void);
void app_harman_total_playback_time_update(void);
void app_harman_total_power_on_time_update(void);
void app_harman_vp_data_header_get(void);
void app_harman_cfg_reset(void);
void app_harman_water_ejection_stop(void);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
