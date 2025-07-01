#if (F_APP_HEARABLE_SUPPORT == 1)
#ifndef _APP_HEARABLE_H_
#define _APP_HEARABLE_H_

#include <stdbool.h>
#include "app_listening_mode.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

bool app_ha_get_hearable_on(void);
void app_ha_listening_turn_on_off(uint8_t status);
void app_ha_switch_hearable_prog(void);
void app_ha_cmd_handle(uint8_t *cmd_ptr, uint16_t cmd_len, uint8_t cmd_path, uint8_t app_idx,
                       uint8_t *ack_pkt);
void app_ha_init(void);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _APP_HEARABLE_H_ */
#endif /*F_APP_HEARABLE_SUPPORT */
