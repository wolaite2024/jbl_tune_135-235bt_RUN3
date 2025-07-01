/*
 * Copyright (c) 2018, Realsil Semiconductor Corporation. All rights reserved.
 */

#include "trace.h"
#include "app_main.h"
#include "app_cfg.h"
#if BISTO_FEATURE_SUPPORT
#include "app_bisto.h"
#include "app_bisto_ble.h"
#include "app_bisto_bt.h"
#endif

#if GFPS_FEATURE_SUPPORT
#include "app_gfps.h"
#include "app_gfps_rfc.h"
#if GFPS_FINDER_DULT_SUPPORT
#include "app_dult.h"
#endif
#endif

#if XM_XIAOAI_FEATURE_SUPPORT
#include "app_xm_xiaoai.h"
#include "app_xm_xiaoai_rfc.h"
#endif
#if F_APP_XIAOWEI_FEATURE_SUPPORT
#include "xiaowei_protocol.h"
#include "app_xiaowei_rfc.h"
#endif
#if AMA_FEATURE_SUPPORT
#include "app_ama.h"
#endif

#if F_APP_TEAMS_FEATURE_SUPPORT
#include "app_asp_device_rfc.h"
#include "app_hid_rfc.h"
#endif

#if (F_APP_DONGLE_FEATURE_SUPPORT == 1)
#include "app_dongle_spp.h"
#endif

#if DMA_FEATURE_SUPPORT
#include "app_dma.h"
#endif


void app_third_party_srv_init(void)
{
    DBG_DIRECT("app_harman_load_bkp_data3 app_third_party_srv_init");
#if GFPS_FEATURE_SUPPORT
    app_gfps_rfc_init();
    if (extend_app_cfg_const.gfps_support)
    {
        app_gfps_init();
    }
#if GFPS_FINDER_DULT_SUPPORT
    app_dult_svc_init();
#endif
#endif

#if XM_XIAOAI_FEATURE_SUPPORT
    if (extend_app_cfg_const.xiaoai_support)
    {
        app_xm_xiaoai_init();
        app_xiaoai_rfc_init();
    }
#endif

#if F_APP_XIAOWEI_FEATURE_SUPPORT
    if (extend_app_cfg_const.xiaowei_support)
    {
        app_xiaowei_rfc_init();
    }
#endif

#if AMA_FEATURE_SUPPORT
    if (extend_app_cfg_const.ama_support)
    {
        app_ama_init();
    }
#endif

#if BISTO_FEATURE_SUPPORT
    if (extend_app_cfg_const.bisto_support)
    {
        app_bisto_init();
    }
#endif

#if F_APP_TEAMS_FEATURE_SUPPORT
    app_asp_device_rfc_init();
    app_hid_rfc_init();
#endif

#if (F_APP_DONGLE_FEATURE_SUPPORT == 1)
    app_dongle_spp_init();
#endif

#if DMA_FEATURE_SUPPORT
    if (extend_app_cfg_const.dma_support)
    {
        app_dma_init();
    }
#endif
}

