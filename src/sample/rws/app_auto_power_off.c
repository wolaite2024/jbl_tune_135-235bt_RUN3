/*
 * Copyright (c) 2018, Realsil Semiconductor Corporation. All rights reserved.
 */

#include <stdint.h>
#include <stdbool.h>
#include "trace.h"
#include "gap_timer.h"
#include "app_cfg.h"
#include "app_mmi.h"
#include "app_main.h"
#include "app_relay.h"
#include "pm.h"
#include "app_ota.h"

#define TIMER_AUTO_POWER_OFF 0x00

static uint8_t timer_queue_id = 0;
static void *poweroff_timer = NULL;
static uint32_t poweroff_flag = 0;

static void app_auto_power_off_timeout_cback(uint8_t timer_id, uint16_t timer_chann)
{
    APP_PRINT_TRACE2("app_auto_power_off_timeout_cback: timer_id 0x%02x, timer_chann 0x%04x",
                     timer_id, timer_chann);

    switch (timer_id)
    {
    case TIMER_AUTO_POWER_OFF:
        {
            gap_stop_timer(&poweroff_timer);
            if (app_ota_mode_check())
            {
                APP_PRINT_INFO0("app_ota_mode_check is true, stop auto power off");
                break;
            }

            if (app_db.device_state == APP_DEVICE_STATE_ON)
            {
                app_db.power_off_cause = POWER_OFF_CAUSE_AUTO_POWER_OFF;

                if (app_db.remote_session_state == REMOTE_SESSION_STATE_DISCONNECTED)
                {
                    app_mmi_handle_action(MMI_DEV_POWER_OFF);
                }
                else
                {
                    uint8_t action = MMI_DEV_POWER_OFF;

                    if (app_cfg_nv.bud_role == REMOTE_SESSION_ROLE_PRIMARY)
                    {
                        app_relay_sync_single(APP_MODULE_TYPE_MMI, action, REMOTE_TIMER_HIGH_PRECISION,
                                              0, false);
                    }
                    else
                    {
                        app_relay_sync_single(APP_MODULE_TYPE_MMI, action, REMOTE_TIMER_HIGH_PRECISION,
                                              0, true);
                    }
                }
            }
        }
        break;

    default:
        break;
    }
}

void app_auto_power_off_enable(uint32_t flag, uint16_t timeout)
{
    APP_PRINT_TRACE3("app_auto_power_off_enable: curr flag 0x%08x, clear flag 0x%08x, timeout %u",
                     poweroff_flag, flag, timeout);

    poweroff_flag &= ~flag;

    if (poweroff_flag == 0)
    {
        if (timeout == 0)
        {
            gap_stop_timer(&poweroff_timer);
        }
        else
        {
            gap_start_timer(&poweroff_timer, "auto_power_off", timer_queue_id,
                            TIMER_AUTO_POWER_OFF, 0, timeout * 1000);
        }
    }
}

void app_auto_power_off_disable(uint32_t flag)
{
    APP_PRINT_TRACE2("app_auto_power_off_disable: curr flag 0x%08x, set flag 0x%08x",
                     poweroff_flag, flag);

    poweroff_flag |= flag;

    if (poweroff_flag != 0)
    {
        gap_stop_timer(&poweroff_timer);
    }
}

void app_auto_power_off_init(void)
{
    poweroff_timer = NULL;
    poweroff_flag = 0;

    gap_reg_timer_cb(app_auto_power_off_timeout_cback, &timer_queue_id);
    power_register_excluded_handle(&poweroff_timer, PM_EXCLUDED_TIMER);
}
