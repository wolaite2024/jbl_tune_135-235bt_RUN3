#ifndef _APP_LE_AUDIO_SCAN_H_
#define _APP_LE_AUDIO_SCAN_H_

#ifdef  __cplusplus
extern "C" {
#endif      /* __cplusplus */

#include "gap_callback_le.h"

#define LE_AUDIO_SCAN_TIME 120

typedef enum
{
    LE_AUDIO_SCAN_TO,

    LE_AUDIO_SCAN_TOTAL,
} T_LE_AUDIO_SCAN_TIMER;

void app_le_audio_scan_start(uint16_t timeout);
void app_le_audio_scan_stop(void);
bool app_le_audio_scan_handle_report(T_LE_EXT_ADV_REPORT_INFO *p_report);

#ifdef  __cplusplus
}
#endif      /*  __cplusplus */

#endif
