#ifndef _APP_SLIDE_SWITCH_H_
#define _APP_SLIDE_SWITCH_H_

#include <stdint.h>
#include "app_io_msg.h"

typedef enum
{
    APP_SLIDE_SWITCH_ALL_OFF,
    APP_SLIDE_SWITCH_ANC_ON,
    APP_SLIDE_SWITCH_APT_ON,
} T_APP_SLIDE_SWITCH_ANC_APT_STATE;

void app_slide_switch_cfg_init(void);
void app_slide_switch_power_on_off_gpio_status_reset(void);
void app_slide_switch_board_init(void);
void app_slide_switch_driver_init(void);
bool app_slide_switch_gpio_handler(uint8_t gpio_num);
void app_slide_switch_handle_msg(T_IO_MSG *msg);
bool app_slide_switch_power_on_valid_check(void);
void app_slide_switch_resume_apt(void);
void app_slide_switch_init(void);
T_APP_SLIDE_SWITCH_ANC_APT_STATE app_slide_switch_anc_apt_state_get(void);
bool app_slide_switch_between_anc_apt_support(void);
bool app_slide_switch_anc_on_off_support(void);
bool app_slide_switch_apt_on_off_support(void);
void  app_slide_switch_enter_dlps_pad_set(void);
void  app_slide_switch_exit_dlps_pad_set(void);
void  app_slide_switch_enter_power_down_pad_set(void);
#endif
