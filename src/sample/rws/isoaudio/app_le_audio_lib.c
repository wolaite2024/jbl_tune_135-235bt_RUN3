#include "trace.h"
#include "app_ascs.h"
#include "app_ccp.h"
#include "app_csis.h"
#include "app_pacs.h"
#include "app_mcp.h"
#include "app_vcs.h"
#include "app_bass.h"

#if (F_APP_TMAP_CT_SUPPORT || F_APP_TMAP_UMR_SUPPORT || F_APP_TMAP_BMR_SUPPORT)
T_APP_RESULT app_le_audio_msg_cback(T_LE_AUDIO_MSG msg, void *buf)
{
    T_APP_RESULT app_result = APP_RESULT_SUCCESS;
    uint16_t msg_group;

    APP_PRINT_TRACE1("app_le_audio_msg_cback: msg 0x%04x", msg);

    msg_group = msg & 0xff00;
    switch (msg_group)
    {
    case LE_AUDIO_MSG_GROUP_PACS:
        app_result = app_pacs_handle_msg(msg, buf);
        break;
#if F_APP_TMAP_BMR_SUPPORT
    case LE_AUDIO_MSG_GROUP_BASS:
        app_result = app_bass_handle_msg(msg, buf);
        break;
#endif
#if (F_APP_TMAP_CT_SUPPORT || F_APP_TMAP_UMR_SUPPORT)
    case LE_AUDIO_MSG_GROUP_ASCS:
        app_result = app_ascs_handle_msg(msg, buf);
        break;
#endif
#if F_APP_VCS_SUPPORT
    case LE_AUDIO_MSG_GROUP_VCS:
        app_result = app_vcs_handle_msg(msg, buf);
        break;
#endif
#if F_APP_VOCS_SUPPORT
    case LE_AUDIO_MSG_GROUP_VOCS:
        app_result = app_vocs_handle_msg(msg, buf);
        break;
#endif
#if F_APP_AICS_SUPPORT
    case LE_AUDIO_MSG_GROUP_AICS:
        app_result = app_aics_handle_msg(msg, buf);
        break;
#endif
#if F_APP_MICS_SUPPORT
    case LE_AUDIO_MSG_GROUP_MICS:
        app_result = app_mics_handle_msg(msg, buf);
        break;
#endif
#if F_APP_MCP_SUPPORT
    case LE_AUDIO_MSG_GROUP_MCS_CLIENT:
        app_result = app_mcp_handle_msg(msg, buf);
        break;
#endif
#if F_APP_CCP_SUPPORT
    case LE_AUDIO_MSG_GROUP_CCP_CLIENT:
        app_result = app_ccp_handle_msg(msg, buf);
        break;
#endif
#if F_APP_CSIS_SUPPORT
    case LE_AUDIO_MSG_GROUP_CSIS:
        app_result = app_csis_handle_msg(msg, buf);
        break;
#endif
    default:
        break;
    }

    return app_result;
}
#endif
