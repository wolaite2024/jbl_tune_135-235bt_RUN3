#ifndef __APP_QCY_CONTROL_H__
#define __APP_QCY_CONTROL_H__

#include "trace.h"
#include <string.h>
#include "app_mmi.h"
#include "qcy_ctrl_service.h"
#include "rtl876x.h"

#define ID_RESET_BLE_DEFAULT    0x01
#define ID_CLEAR_PHONE_RECORD   0x02
#define ID_RESET_DEFAULT        0x03
#define ID_MUSIC_PLAY           0x04
#define ID_LED_BLINKING         0x05
#define ID_IN_EAR_DETECTION     0x06
#define ID_CTRL_ANC             0x07
#define ID_CTRL_VOLUME          0x08
#define ID_GAMING_MODE          0x09
#define ID_LISTENING_MODE       0x0C
#define ID_OTA_MODE             0x0E
#define ID_SLEEP_MODE           0x10

#define QCY_APP_EQ_STAGE_NUM    10
#define PI ((double)3.1415926535897932384626433832795)
#define BIT_MARGIN              0
#define BIT_VALID               30
#define EQ_DEFAULT_Q_VALUE      2.0
#define EQ_MAX_INDEX            9
#define SAMPLE_RATE_44K         3
#define SAMPLE_RATE_48K         4

typedef enum
{
    MUSIC_PLAY_PAUSE,
    MUSIC_PLAY,
    MUSIC_PAUSE,
    MUSIC_BWD,
    MUSIC_FWD,
} T_APP_CTRL_MUSIC_PLAY;

typedef enum
{
    LED_START_BLINKING,
    LED_STOP_BLINKING,
} T_APP_CTRL_LED;

typedef enum
{
    IN_EAR_DETECTION_ON = 0x01,
    IN_EAR_DETECTION_OFF,
} T_APP_CTRL_IN_EAR_DETECTION;

typedef enum
{
    GAMING_MODE_ON = 0x01,
    GAMING_MODE_OFF,
} T_APP_CTRL_GAMING_MODE;

typedef enum
{
    LISTENING_MODE_ANC = 0x01,
    LISTENING_MODE_ALL_OFF,
    LISTENING_MODE_APT,
} T_APP_CTRL_LISTENING_MODE;

typedef enum
{
    QCY_KEY_EVENT_SINGLE_CLICK,
    QCY_KEY_EVENT_DOUBLE_CLICK,
    QCY_KEY_EVENT_TRIPLE_CLICK,
    QCY_KEY_EVENT_FOUR_CLICK,
    QCY_KEY_EVENT_LONG_PRESS,
} T_QCY_CTRL_KEY_EVENT;

typedef enum
{
    QCY_KEY_ACTION_NULL = 0x00,
    QCY_KEY_ACTION_PLAY_PAUSE,
    QCY_KEY_ACTION_AV_BWD,
    QCY_KEY_ACTION_AV_FWD,
    QCY_KEY_ACTION_VAD,
    QCY_KEY_ACTION_VOL_UP,
    QCY_KEY_ACTION_VOL_DOWN,
    QCY_KEY_ACTION_GAMING_MODE,
    QCY_KEY_ACTION_APT = 0x0A,
    QCY_KEY_ACTION_LISTENING_MODE_CYCLE = 0x0B,
} T_QCY_CTRL_KEY_ACTION;

typedef enum
{
    LANGUAGE_TONE_ONLY,
    LANGUAGE_MANDARIN,
    LANGUAGE_ENGLISH,
    LANGUAGE_JAPANESE,
    LANGUAGE_KOREAN,
    LANGUAGE_GERMAN,
    LANGUAGE_FRENCH,
    LANGUAGE_ITALIAN,
} T_APP_CTRL_LANGUAGE;

typedef enum
{
    EQ_SETTING_1 = 0x01,
    EQ_SETTING_2,
    EQ_SETTING_3,
    EQ_SETTING_4,
    EQ_SETTING_5,
    EQ_SETTING_6,
    EQ_SELF_DEFINED = 0xFF,
} T_APP_CTRL_EQ;

typedef enum
{
    LEFT_BUD_SETTING,
    RIGHT_BUD_SETTING,
} T_APP_BUD_SIDE;

typedef struct
{
    uint8_t bat_vol : 7;
    uint8_t bat_charging : 1;
} T_APP_BAT_INFO;

typedef struct
{
    T_QCY_CTRL_KEY_EVENT key_event;
    T_QCY_CTRL_KEY_ACTION key_action;
} T_KEY_CFG;

typedef struct
{
    uint8_t feature_status_conn_id;
    uint8_t battery_status_conn_id;
} T_NOTIFY_CONN_ID;

typedef struct
{
    int32_t order;
    double gain;
    double num[5];
    double den[5];
} T_EQ_FILTER_COEF;

typedef struct
{
    uint32_t stage: 4;
    int32_t guadBit: 4;
    int32_t gain: 24;
    int32_t coef[10][5];
} T_EQ_COEF;

typedef struct
{
    uint32_t freq;
    int8_t gain;
    float q;
} T_EQ_INFO;

typedef struct
{
    uint8_t single_click_action : 4;
    uint8_t double_click_action : 4;
    uint8_t triple_click_action : 4;
    uint8_t four_click_action : 4;
    uint8_t long_press_action : 4;
    uint8_t rsv : 4;
} T_QCY_KEY_SETTING;

typedef struct
{
    T_QCY_KEY_SETTING local_key;
    T_QCY_KEY_SETTING remote_key;
} T_QCY_KEY_INFO;

typedef struct
{
    /* every gain uses 5 bits (separate to 1 bit and 4 bits) to save */
    uint8_t gain_freq_32 : 4;
    uint8_t gain_freq_64 : 4;

    uint8_t gain_freq_125 : 4;
    uint8_t gain_freq_250 : 4;

    uint8_t gain_freq_500 : 4;
    uint8_t gain_freq_1000 : 4;

    uint8_t gain_freq_2000 : 4;
    uint8_t gain_freq_4000 : 4;

    uint8_t gain_freq_8000 : 4;
    uint8_t gain_freq_16000 : 4;

    uint8_t gain_first_bit_freq_32 : 1;
    uint8_t gain_first_bit_freq_64 : 1;
    uint8_t gain_first_bit_freq_125 : 1;
    uint8_t gain_first_bit_freq_250 : 1;
    uint8_t gain_first_bit_freq_500 : 1;
    uint8_t gain_first_bit_freq_1000 : 1;
    uint8_t gain_first_bit_freq_2000 : 1;
    uint8_t gain_first_bit_freq_4000 : 1;

    uint8_t gain_first_bit_freq_8000 : 1;
    uint8_t gain_first_bit_freq_16000 : 1;
    uint8_t rsv : 6;
} T_QCY_EQ_GAIN_DATA;

typedef struct
{
    uint8_t qcy_sleep_mode : 1;
    uint8_t qcy_adv_interval : 1;
    uint8_t qcy_le_connected : 1;
    uint8_t qcy_app_db_rsv : 5;
} T_QCY_APP_DB;

extern T_NOTIFY_CONN_ID app_qcy_conn_id;
extern bool app_ctrl_led_blinking_fg;

void app_qcy_led_blinking(T_APP_CTRL_LED led_ctrl);
bool app_ctrl_reset_ble_default(void);
bool app_ctrl_clear_phone_record(void);
bool app_ctrl_reset_default(void);
bool app_ctrl_music_play(T_APP_CTRL_MUSIC_PLAY music_ctrl);
bool app_ctrl_led_blinking(T_APP_CTRL_LED led_ctrl);
bool app_ctrl_in_ear_detection(T_APP_CTRL_IN_EAR_DETECTION in_ear_ctrl);
bool app_ctrl_anc(uint8_t anc_level);
bool app_ctrl_volume(uint8_t left_ear_volume, uint8_t right_ear_volume);
bool app_ctrl_gaming_mode(T_APP_CTRL_GAMING_MODE gaming_mode_ctrl);
bool app_ctrl_touch_sensor(uint8_t touch_sensor_level);
bool app_ctrl_listening_mode(T_APP_CTRL_LISTENING_MODE listening_mode_ctrl);
bool app_ctrl_key_setting(T_APP_BUD_SIDE side, T_KEY_CFG *key_cfg);
void app_ctrl_get_remote_key_info(T_QCY_KEY_INFO *remote_key_info);
bool app_qcy_set_language(uint8_t language);
bool app_ctrl_language(T_APP_CTRL_LANGUAGE language_ctrl);
bool app_ctrl_set_eq(T_APP_CTRL_EQ eq_ctrl, uint8_t *data);
bool app_ctrl_get_eq(T_APP_CTRL_EQ eq_ctrl, uint8_t *data, uint8_t len);
void get_qcy_user_defined_eq(int8_t *gain);

bool app_read_fw_version(uint8_t *version, uint8_t size);
bool app_read_battery_status(T_APP_BAT_INFO *left, T_APP_BAT_INFO *right, T_APP_BAT_INFO *box);

void app_handle_read_request(uint8_t read_index);
void app_handle_write_request(T_QCY_WRITE_MSG write_msg);
void app_handle_notify_indicate_request(uint8_t conn_id,
                                        T_QCY_NOTIFY_INDICATE_MSG notify_indicate_msg);

void app_qcy_set_notify_value(uint8_t conn_id, T_QCY_PARAM_TYPE param);
void app_qcy_key_handle_single_click(uint8_t key, uint8_t type);
void app_qcy_key_handle_hybrid_click(uint8_t key, uint8_t index, uint8_t hybrid_type);
bool app_qcy_key_hybrid_click_is_set(uint8_t hybrid_type);
void app_qcy_cfg_reset(void);
T_MMI_ACTION qcy_key_to_mmi(T_QCY_CTRL_KEY_ACTION key_ctrl);

#endif
