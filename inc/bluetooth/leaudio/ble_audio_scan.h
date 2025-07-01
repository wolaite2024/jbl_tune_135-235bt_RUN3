#ifndef _BLE_AUDIO_SCAN_H_
#define _BLE_AUDIO_SCAN_H_

#ifdef  __cplusplus
extern "C" {
#endif      /* __cplusplus */

#include "gap_ext_scan.h"

bool ble_audio_adv_filter_service_data(uint8_t report_data_len, uint8_t *p_report_data,
                                       uint16_t uuid, uint8_t **pp_service_data, uint16_t *p_data_len);

#ifdef  __cplusplus
}
#endif      /*  __cplusplus */

#endif
