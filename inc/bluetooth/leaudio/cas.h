#ifndef _CAS_H_
#define _CAS_H_

#ifdef  __cplusplus
extern "C" {
#endif      /* __cplusplus */

#include "csis_mgr.h"
#include "cas_def.h"

#if LE_AUDIO_CAS_SUPPORT
void cas_init(uint8_t *sirk, T_CSIS_SIRK_TYPE sirk_type, uint8_t size, uint8_t rank,
              uint8_t feature);

#endif

#ifdef  __cplusplus
}
#endif      /*  __cplusplus */

#endif
