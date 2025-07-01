#ifndef _TMAS_MGR_H_
#define _TMAS_MGR_H_

#ifdef  __cplusplus
extern "C" {
#endif      /* __cplusplus */

#include "tmas_def.h"

#if LE_AUDIO_TMAS_SUPPORT
bool tmas_init(uint16_t role);
#endif
#ifdef  __cplusplus
}
#endif      /*  __cplusplus */

#endif
