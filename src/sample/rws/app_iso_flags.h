
#ifndef _APP_ISO_FLAGS_H_
#define _APP_ISO_FLAGS_H_

#if (ISOC_AUDIO_SUPPORT == 1)

// CT and UMR must support ASCS and PACS service
#define F_APP_TMAP_CT_SUPPORT           1
#define F_APP_TMAP_UMR_SUPPORT          1
// If the Broadcast Sink role is supported, the Scan Delegator role shall be supported
#define F_APP_TMAP_BMR_SUPPORT          1

#if F_APP_TMAP_CT_SUPPORT
#undef F_APP_CCP_SUPPORT
#define F_APP_CCP_SUPPORT               1

#undef F_APP_VCS_SUPPORT
#define F_APP_VCS_SUPPORT               1
#undef F_APP_OPTIONAL_SUPPORT
#define F_APP_OPTIONAL_SUPPORT          1
#endif

#if F_APP_TMAP_UMR_SUPPORT
#undef F_APP_MCP_SUPPORT
#define F_APP_MCP_SUPPORT               1

#undef F_APP_VCS_SUPPORT
#define F_APP_VCS_SUPPORT               1
#undef F_APP_OPTIONAL_SUPPORT
#define F_APP_OPTIONAL_SUPPORT          1
#endif

#if F_APP_TMAP_BMR_SUPPORT
#undef F_APP_MCP_SUPPORT
#define F_APP_MCP_SUPPORT               1

#undef F_APP_VCS_SUPPORT
#define F_APP_VCS_SUPPORT               1
// TODO, remove F_APP_LE_AUDIO_CLI_TEST
#undef F_APP_OPTIONAL_SUPPORT
#define F_APP_OPTIONAL_SUPPORT          1
#endif

#if F_APP_MCP_SUPPORT
#define F_APP_MCP_SUPPORT_WORKAROUND    1
#endif

#if F_APP_CCP_SUPPORT
#define F_APP_CCP_SUPPORT_WORKAROUND    1
#endif

#if F_APP_OPTIONAL_SUPPORT
#define GATTC_TBL_STORAGE_SUPPORT       1
#define BT_GATT_CLIENT_SUPPORT          1
#define F_APP_LE_AUDIO_SM               1
#define F_APP_LE_AUDIO_CLI_TEST         1

#define F_APP_AICS_SUPPORT              0
#define F_APP_AICS_NUM                  1
#define F_APP_MICS_SUPPORT              0
#define F_APP_AICS_MIC_IDX              0
#define F_APP_VOCS_SUPPORT              0
#define F_APP_VOCS_NUM                  1
#define F_APP_VOCS_LEFT                 0
#define F_APP_VOCS_RIGHT                1
#define F_APP_CSIS_SUPPORT              1
#endif

#endif



#endif

