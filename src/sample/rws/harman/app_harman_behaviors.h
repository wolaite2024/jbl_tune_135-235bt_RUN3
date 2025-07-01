#ifndef _APP_HARMAN_BEHAVIORS_H_
#define _APP_HARMAN_BEHAVIORS_H_

#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

void app_harman_set_cpu_clk(bool evt);
void app_harman_cpu_clk_improve(void);
void app_harman_init(void);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
