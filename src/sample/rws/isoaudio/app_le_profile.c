#include <string.h>
#include "trace.h"
#include "bass_mgr.h"
#if BT_GATT_CLIENT_SUPPORT
#include "bt_gatt_client.h"
#endif
#include "ccp_client.h"
#if GATTC_TBL_STORAGE_SUPPORT
#include "gattc_tbl_storage.h"
#endif
#include "bap.h"
#include "mcs_client.h"
#include "app_ascs.h"
#include "app_csis.h"
#include "app_le_profile.h"
#include "app_main.h"
#include "app_pacs.h"
#include "app_vcs.h"

#if (F_APP_TMAP_CT_SUPPORT || F_APP_TMAP_UMR_SUPPORT || F_APP_TMAP_BMR_SUPPORT)
void app_le_profile_init(void)
{
#if ISOC_AUDIO_SUPPORT
    uint8_t role_mask = 0;
    T_BAP_ROLE_INFO role_info;
#if (F_APP_TMAP_CT_SUPPORT || F_APP_TMAP_UMR_SUPPORT)
    client_init(10);
#endif
    memset(&role_info, 0, sizeof(role_info));
#if (F_APP_TMAP_CT_SUPPORT || F_APP_TMAP_UMR_SUPPORT)
    role_mask |= (BAP_UNICAST_SRV_SRC_ROLE | BAP_UNICAST_SRV_SNK_ROLE);
    role_info.isoc_cig_max_num = APP_ISOC_CIG_MAX_NUM;
    role_info.isoc_cis_max_num = APP_ISOC_CIS_MAX_NUM;
#endif
#if (F_APP_TMAP_BMR_SUPPORT)
    role_mask |= (BAP_BROADCAST_SINK_ROLE | BAP_SCAN_DELEGATOR_ROLE);
    role_info.pa_adv_num = 0;
    role_info.isoc_big_broadcaster_num = 0;
    role_info.isoc_bis_broadcaster_num = 0;
    role_info.pa_sync_num = APP_MAX_SYNC_HANDLE_NUM;
    role_info.isoc_big_receiver_num = APP_SYNC_RECEIVER_MAX_BIG_HANDLE_NUM;
    role_info.isoc_bis_receiver_num = APP_SYNC_RECEIVER_MAX_BIS_NUM;
    role_info.brs_num = APP_SYNC_RECEIVER_MAX_BIG_HANDLE_NUM;
#endif
    role_info.init_gap = true;
    bap_role_init(role_mask, &role_info);
    app_le_audio_init();
#endif
#if GATTC_TBL_STORAGE_SUPPORT
    gattc_tbl_storage_init();
#endif
#if BT_GATT_CLIENT_SUPPORT
    gatt_client_init();
#endif
}

void app_le_audio_role_init(void)
{
#if (F_APP_TMAP_CT_SUPPORT || F_APP_TMAP_UMR_SUPPORT || F_APP_TMAP_BMR_SUPPORT)
    app_pacs_init();
#endif
#if (F_APP_TMAP_CT_SUPPORT || F_APP_TMAP_UMR_SUPPORT)
    app_ascs_init();
#endif
#if F_APP_VCS_SUPPORT
    app_vcs_init();
#endif
#if F_APP_CSIS_SUPPORT
    app_csis_init();
#endif

#if F_APP_MCP_SUPPORT
    mcs_client_init();
#endif
#if F_APP_CCP_SUPPORT
    ccp_client_init();
#endif
}
#endif
