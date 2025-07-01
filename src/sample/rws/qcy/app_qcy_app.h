#ifndef __APP_QCY_APP_H__
#define __APP_QCY_APP_H__

#include <stdbool.h>
#include "app_qcy_control.h"
#include "remote.h"
#include "btm.h"

typedef enum
{
    QCY_APP_ADV_TIMEOUT,
    QCY_APP_TIMER_LED_BLINKING,
    QCY_APP_TIMER_REBOOT,

    QCY_APP_TIMER_TOTAL
} T_QCY_APP_TIMER;

typedef enum
{
    APP_QCY_ADV_LONG_TIMEOUT,
    APP_QCY_ADV_SHORT_TIMEOUT,
    /* used to adjust adv interval; if current not play adv; no need to play adv */
    APP_QCY_ADV_NO_RESTART_TIMER,
} T_QCY_APP_ADV_TIMEOUT;

extern uint8_t app_qcy_timer_queue_id;
extern void *timer_handle_app_ctrl_led_blinking;
extern void *timer_handle_app_ctrl_reboot;

typedef struct
{
    /* set true if wants remote to play adv after info sync */
    uint8_t play_adv_after_get_info : 1;
    uint8_t qcy_sleep_mode : 1;
    uint8_t rsv : 6;

    uint8_t adv_cnt;
    uint8_t batt_level;
    uint8_t charger_state;
    uint8_t bud_location;
    T_QCY_KEY_INFO key_info;
} T_QCY_APP_RELAY_MSG;

void app_qcy_info_sync(bool play_adv_after_get_info);
void app_qcy_adv_init(void);
void app_qcy_init(void);

void app_qcy_get_bat_info(bool *left_charging, uint8_t *left_bat,
                          bool *right_charging, uint8_t *right_bat,
                          bool *case_charging, uint8_t *case_bat);
void app_qcy_adv_start(bool set_out_box_flag, T_QCY_APP_ADV_TIMEOUT timeout);
void app_qcy_adv_stop(bool stop_timer);
void app_qcy_handle_in_out_box_adv(void);
bool app_qcy_adv_is_advertising(void);
void app_qcy_handle_role_swap(T_REMOTE_SESSION_ROLE role, T_BTM_ROLESWAP_STATUS status);


#endif

