#include <string.h>
#include <stdlib.h>
#include "trace.h"
#include "bt_bond.h"
#include "app_main.h"
#include "app_cfg.h"
#include "app_harman_behaviors.h"
#include "app_harman_parser.h"
#include "app_ble_service.h"
#include "app_ble_common_adv.h"
#include "app_harman_ble.h"
#include "app_cmd.h"
#include "app_harman_vendor_cmd.h"
#include "app_harman_report.h"
#include "pm.h"

bool gfps_finder_adv_ei_and_hash = false;
//extern void app_harman_report_le_event(T_APP_LE_LINK *p_link, uint16_t event_id, uint8_t *data,
//                                uint16_t len);

void app_harman_set_cpu_clk(bool evt)
{
    APP_PRINT_TRACE2("app_harman_set_cpu_clk %d, %d", gfps_finder_adv_ei_and_hash, evt);
    gfps_finder_adv_ei_and_hash = evt;
}

void app_harman_cpu_clk_improve(void)
{
    uint32_t default_cpu_freq = pm_cpu_freq_get();
    uint32_t cpu_freq = 0;
    int32_t ret = 0;

    if ((app_db.tone_vp_status.state == APP_TONE_VP_STOP) &&
        (gfps_finder_adv_ei_and_hash == false))
    {
        ret = pm_cpu_freq_set(40, &cpu_freq);
    }
    else
    {
        if (default_cpu_freq != 100)
        {
            ret = pm_cpu_freq_set(100, &cpu_freq);
        }
    }

    APP_PRINT_TRACE5("cpu freq config ret %x default freq %dMHz real freq %dMHz %d,%d", ret,
                     default_cpu_freq, cpu_freq, app_db.tone_vp_status.state, gfps_finder_adv_ei_and_hash);
}

#if 0
void app_harman_hook_register(void)
{
    app_ble_process_hook = app_harman_ble_process;
    app_parser_process_hook = app_harman_parser_process;
    app_handle_cmd_set_hook = app_harman_vendor_cmd_process;
    app_report_le_event_hook = app_harman_report_le_event;
}

void app_harman_init(void)
{
    //DBG_DIRECT("[SD_CHECK] app_harman_init");;

    app_harman_hook_register();
}
#endif
