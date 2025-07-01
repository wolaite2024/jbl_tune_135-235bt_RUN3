#ifndef _APP_WDG_H_
#define _APP_WDG_H_
#include "wdg.h"

#define app_wdg_reset(mode)                                                     \
    {                                                                               \
        DBG_DIRECT("app wdg reset! func: %s, line: %d", __FUNCTION__, __LINE__);    \
        chip_reset(mode);                                                      \
    }
#endif
