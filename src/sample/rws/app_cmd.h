/*
 * Copyright (c) 2018, Realsil Semiconductor Corporation. All rights reserved.
 */

#ifndef _APP_CMD_H_
#define _APP_CMD_H_

#include <stdint.h>
#include <stdbool.h>

#include "app_report.h"
#include "app_relay.h"
#include "app_link_util.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


/** @defgroup APP_CMD App Cmd
  * @brief App Cmd
  * @{
  */

/*============================================================================*
 *                              Macros
 *============================================================================*/
/** @defgroup APP_CMD_Exported_Macros App Cmd Macros
   * @{
   */

#define CMD_SET_VER_MAJOR                   0x01
#define CMD_SET_VER_MINOR                   0x07
#define EQ_SPEC_VER_MAJOR                   0x01
#define EQ_SPEC_VER_MINOR_0                 0x00
#define EQ_SPEC_VER_MINOR_1                 0x01
#define EQ_SPEC_VER_MINOR_2                 0x02
#define CMD_SYNC_BYTE                       0xAA

//align dsp_driver.h/codec_driver.h define
#define APP_MIC_SEL_DMIC_1                  0x00
#define APP_MIC_SEL_DMIC_2                  0x01
#define APP_MIC_SEL_AMIC_1                  0x02
#define APP_MIC_SEL_AMIC_2                  0x03
#define APP_MIC_SEL_AMIC_3                  0x04
#define APP_MIC_SEL_DISABLE                 0x07

#define CHECK_IS_LCH        (app_cfg_const.bud_side == DEVICE_BUD_SIDE_LEFT)
#define CHECK_IS_RCH        (app_cfg_const.bud_side == DEVICE_BUD_SIDE_RIGHT)

#define INVALID_VALUE       0xFF
#define L_CH_PRIMARY        0x01
#define R_CH_PRIMARY        0x02

#define L_CH_BATT_LEVEL     CHECK_IS_LCH ? app_db.local_batt_level : (app_db.remote_session_state == REMOTE_SESSION_STATE_CONNECTED) ? app_db.remote_batt_level : 0
#define R_CH_BATT_LEVEL     CHECK_IS_RCH ? app_db.local_batt_level : (app_db.remote_session_state == REMOTE_SESSION_STATE_CONNECTED) ? app_db.remote_batt_level : 0

//for CMD_AUDIO_DSP_CTRL_SEND to capture dsp data
#define VENDOR_SPP_CAPTURE_DSP_LOG      0x01
#define VENDOR_SPP_CAPTURE_DSP_RWA_DATA 0x02
#define H2D_CMD_DSP_DAC_ADC_DATA_TO_MCU 0x1F
#define H2D_SPPCAPTURE_SET              0x0F01
#define CHANGE_MODE_EXIST               0x00
#define CHANGE_MODE_TO_SCO              0x01
#define DSP_CAPTURE_DATA_START_MASK                 0x01
#define DSP_CAPTURE_DATA_SWAP_TO_MASTER             0x02
#define DSP_CAPTURE_DATA_ENTER_SCO_MODE_MASK        0x04
#define DSP_CAPTURE_DATA_CHANGE_MODE_TO_SCO_MASK    0x08
#define DSP_CAPTURE_RAW_DATA_EXECUTING              0x10
#define DSP_CAPTURE_DATA_LOG_EXECUTING              0x20

/* for 8753BFN */
#if F_APP_DEVICE_CMD_SUPPORT
//for CMD_INQUIRY
#define START_INQUIRY                           0x00
#define STOP_INQUIRY                            0x01
#define NORMAL_INQUIRY                          0x00
#define PERIODIC_INQUIRY                        0x01
#define MAX_INQUIRY_TIME                        0x30

//for CMD_SERVICES_SEARCH
#define START_SERVICES_SEARCH                   0x00
#define STOP_SERVICES_SEARCH                    0x01

//for CMD_PAIR_REPLY and CMD_SSP_CONFIRMATION
#define ACCEPT_PAIRING_REQ                      0x00
#define REJECT_PAIRING_REQ                      0x01

//for CMD_SET_PAIRING_CONTROL
#define ENABLE_AUTO_ACCEPT_ACL_ACF_REQ          0x00
#define ENABLE_AUTO_REJECT_ACL_ACF_REQ          0x01
#define FORWARD_ACL_ACF_REQ_TO_HOST             0x02

//for CMD_GET_REMOTE_DEV_ATTR_INFO
#define GET_AVRCP_ATTR_INFO                     0x00
#define GET_PBAP_ATTR_INFO                      0x01
#endif

#if F_APP_AVRCP_CMD_SUPPORT
#define ALL_ELEMENT_ATTR                        0x00
#define MAX_NUM_OF_ELEMENT_ATTR                 0x07
#endif

//for CMD_GET_LINK_KEY
#define GET_ALL_LINK_KEY                        0x00
#define GET_SPECIAL_ADDR_LINK_KEY               0x01
#define GET_PRORITY_LINK_KEY                    0x02

//for CMD_SET_AND_READ_DLPS
#define SET_DLPS_DISABLE                        0x00
#define SET_DLPS_ENABLE                         0x01
#define GET_DLPS_STATUS                         0x02
// end of for 8753BFN

// for command handler
#define PARAM_START_POINT                       2
#define COMMAND_ID_LENGTH                       2

#define MP_CMD_HCI_OPCODE                       0xFCEB

/** End of APP_CMD_Exported_Macros
    * @}
    */
/*============================================================================*
 *                              Types
 *============================================================================*/
/** @defgroup APP_CMD_Exported_Types App Cmd Types
  * @{
  */
/**  @brief  embedded uart, spp or le vendor command type.
  *    <b> Only <b> valid when BT SOC connects to external MCU via data uart, spp or le.
  *    refer to SDK audio sample code for definition
  */
typedef enum
{
    CMD_ACK                             = 0x0000,
    CMD_BT_READ_PAIRED_RECORD           = 0x0001,
    CMD_BT_CREATE_CONNECTION            = 0x0002,
    CMD_BT_DISCONNECT                   = 0x0003,
    CMD_MMI                             = 0x0004,
    CMD_LEGACY_DATA_TRANSFER            = 0x0005,
    CMD_ASSIGN_BUFFER_SIZE              = 0x0006,
    CMD_BT_READ_LINK_INFO               = 0x0007,
    CMD_TONE_GEN                        = 0x0008,
    CMD_BT_GET_REMOTE_NAME              = 0x0009,
    CMD_BT_IAP_LAUNCH_APP               = 0x000A,
    CMD_TTS                             = 0x000B,
    CMD_INFO_REQ                        = 0x000C,

    CMD_DAC_GAIN_CTRL                   = 0x000F,
    CMD_ADC_GAIN_CTRL                   = 0x0010,
    CMD_BT_SEND_AT_CMD                  = 0x0011,
    CMD_SET_CFG                         = 0x0012,
    CMD_INDICATION                      = 0x0013,
    CMD_LINE_IN_CTRL                    = 0x0014,
    CMD_LANGUAGE_GET                    = 0x0015,
    CMD_LANGUAGE_SET                    = 0x0016,
    CMD_GET_CFG_SETTING                 = 0x0017,
    CMD_GET_STATUS                      = 0x0018,  // used to get part of bud info
    CMD_SUPPORT_MULTILINK               = 0x0019,

    CMD_BT_HFP_DIAL_WITH_NUMBER         = 0x001B,
    CMD_GET_BD_ADDR                     = 0x001C,
    CMD_STRING_MODE                     = 0x001E,
    CMD_SET_VP_VOLUME                   = 0x001F,

    CMD_SET_AND_READ_DLPS               = 0x0020,
    CMD_GET_BUD_INFO                    = 0x0021,  // used to get complete bud info

#if F_APP_DEVICE_CMD_SUPPORT
    CMD_GET_LOCAL_DEV_STATE             = 0x0022,
    CMD_INQUIRY                         = 0x0023,
    CMD_SERVICES_SEARCH                 = 0x0024,
    CMD_SET_PAIRING_CONTROL             = 0x0025,
    CMD_SET_PIN_CODE                    = 0x0026,
    CMD_GET_PIN_CODE                    = 0x0027,
    CMD_PAIR_REPLY                      = 0x0028,
    CMD_SSP_CONFIRMATION                = 0x0029,
    CMD_GET_CONNECTED_DEV_ID            = 0x002A,
    CMD_GET_REMOTE_DEV_ATTR_INFO        = 0x002B,
#endif

    CMD_LE_START_ADVERTISING            = 0x0100,
    CMD_LE_STOP_ADVERTISING             = 0x0101,
    CMD_LE_DATA_TRANSFER                = 0x0102,
    CMD_LE_START_SCAN                   = 0x0103,
    CMD_LE_STOP_SCAN                    = 0x0104,
    CMD_LE_GET_ADDR                     = 0x0105,

    CMD_ANCS_REGISTER                   = 0x0110,
    CMD_ANCS_GET_NOTIFICATION_ATTR      = 0x0111,
    CMD_ANCS_GET_APP_ATTR               = 0x0112,
    CMD_ANCS_PERFORM_NOTIFICATION_ACTION = 0x0113,

    CMD_AUDIO_EQ_QUERY                  = 0x0200,
    CMD_AUDIO_EQ_PARAM_SET              = 0x0203,
    CMD_AUDIO_EQ_PARAM_GET              = 0x0204,
    CMD_AUDIO_EQ_INDEX_SET              = 0x0205,
    CMD_AUDIO_EQ_INDEX_GET              = 0x0206,
    CMD_AUDIO_DSP_CTRL_SEND             = 0x0207,
    CMD_AUDIO_CODEC_CTRL_SEND           = 0x0208,
#if 0
    //supported only in cmd set version v0.0.0.1
    CMD_DSP_GET_AUDIO_EQ_SETTING_IDX    = 0x0207,
    CMD_DSP_SET_AUDIO_EQ_SETTING_IDX    = 0x0208,
    CMD_DSP_SET_APT_GAIN                = 0x0209,
#endif
    CMD_SET_VOLUME                      = 0x020A,
#if F_APP_APT_SUPPORT
    CMD_APT_EQ_INDEX_SET                = 0x020B,
    CMD_APT_EQ_INDEX_GET                = 0x020C,
#endif
    CMD_DSP_DEBUG_SIGNAL_IN             = 0x020D,   // only support BBPro2

#if F_APP_APT_SUPPORT
    CMD_SET_APT_VOLUME_OUT_LEVEL        = 0x020E,
    CMD_GET_APT_VOLUME_OUT_LEVEL        = 0x020F,
#endif

    // for equalizer page
    CMD_AUDIO_EQ_QUERY_PARAM            = 0x0210,

    CMD_SET_TONE_VOLUME_LEVEL           = 0x0211,
    CMD_GET_TONE_VOLUME_LEVEL           = 0x0212,
    CMD_DSP_TOOL_FUNCTION_ADJUSTMENT    = 0x0213,

    CMD_RESET_EQ_DATA                   = 0x0214,
    CMD_SET_SIDETONE                    = 0x0215,

    //for good test
    CMD_LED_TEST                        = 0x0300,
    CMD_CLEAR_MP_DATA                   = 0x0301,
    CMD_BT_GET_LOCAL_ADDR               = 0x0302,
#if F_APP_RSSI_INFO_GET_CMD_SUPPORT
    CMD_GET_LEGACY_RSSI                 = 0x0303,
#endif
    CMD_GET_RF_POWER                    = 0x0304,
    CMD_GET_CRYSTAL_TRIM                = 0x0305,
    CMD_GET_LINK_KEY                    = 0x0306,
    CMD_GET_COUNTRY_CODE                = 0x0307,
    CMD_GET_FW_VERSION                  = 0x0308,
    CMD_BT_BOND_INFO_CLEAR              = 0x0309,
    CMD_GET_ADC_VALUE_1                 = 0x030A,
    CMD_GET_ADC_VALUE_2                 = 0x030B,
    CMD_GET_UNSIZE_RAM                  = 0x030C,
    CMD_GET_FLASH_DATA                  = 0x030D,
    CMD_MIC_SWITCH                      = 0x030E,
    CMD_GET_PACKAGE_ID                  = 0x030F,
    CMD_SWITCH_TO_HCI_DOWNLOAD_MODE     = 0x0310,
    CMD_GET_PAD_VOLTAGE                 = 0x0311,
    CMD_PX318J_CALIBRATION              = 0x0312,
    CMD_READ_CODEC_REG                  = 0x0313,
    CMD_WRITE_CODEC_REG                 = 0x0314,
    CMD_GET_NUM_OF_CONNECTION           = 0x0315,
    CMD_REG_ACCESS                      = 0X0316,
    CMD_DSP_TEST_MODE                   = 0X0317,

    CMD_RF_XTAK_K                       = 0x032A,
    CMD_RF_XTAL_K_GET_RESULT            = 0x032B,

    //0x0400 ~ 0x04FF reserved for profile
#if F_APP_HFP_CMD_SUPPORT
    CMD_SEND_DTMF                       = 0x0400,
    CMD_GET_OPERATOR                    = 0x0401,
    CMD_GET_SUBSCRIBER_NUM              = 0x0402,
#endif

#if F_APP_AVRCP_CMD_SUPPORT
    CMD_AVRCP_LIST_SETTING_ATTR         = 0x0410,
    CMD_AVRCP_LIST_SETTING_VALUE        = 0x0411,
    CMD_AVRCP_GET_CURRENT_VALUE         = 0x0412,
    CMD_AVRCP_SET_VALUE                 = 0x0413,
    CMD_AVRCP_ABORT_DATA_TRANSFER       = 0x0414,
    CMD_AVRCP_SET_ABSOLUTE_VOLUME       = 0x0415,
    CMD_AVRCP_GET_PLAY_STATUS           = 0x0416,
    CMD_AVRCP_GET_ELEMENT_ATTR          = 0x0417,
#endif

#if F_APP_PBAP_CMD_SUPPORT
    CMD_PBAP_DOWNLOAD                   = 0x0420,
    CMD_PBAP_DOWNLOAD_CONTROL           = 0x0421,
    CMD_PBAP_DOWNLOAD_GET_SIZE          = 0x0422,
#endif

    CMD_OTA_DEV_INFO                    = 0x0600,
    CMD_OTA_IMG_VER                     = 0x0601,
    CMD_OTA_START                       = 0x0602,
    CMD_OTA_PACKET                      = 0x0603,
    CMD_OTA_VALID                       = 0x0604,
    CMD_OTA_RESET                       = 0x0605,
    CMD_OTA_ACTIVE_RESET                = 0x0606,
    CMD_OTA_BUFFER_CHECK_ENABLE         = 0x0607,
    CMD_OTA_BUFFER_CHECK                = 0x0608,
    CMD_OTA_IMG_INFO                    = 0x0609,
    CMD_OTA_SECTION_SIZE                = 0x060A,
    CMD_OTA_DEV_EXTRA_INFO              = 0x060B,
    CMD_OTA_PROTOCOL_TYPE               = 0x060C,
    CMD_OTA_GET_RELEASE_VER             = 0x060D,
    CMD_OTA_INACTIVE_BANK_VER           = 0x060E,
    CMD_OTA_COPY_IMG                    = 0x060F,
    CMD_OTA_CHECK_SHA256                = 0x0610,
    CMD_OTA_ROLESWAP                    = 0x0611,
    CMD_OTA_TEST_EN                     = 0x0612,

#if (F_APP_LOCAL_PLAYBACK_SUPPORT == 1)
    /* only support BBPro2 */
    CMD_PLAYBACK_QUERY_INFO                 = 0x0680,
    CMD_PLAYBACK_GET_LIST_DATA              = 0x0681,
    CMD_PLAYBACK_TRANS_START                = 0x0682,
    CMD_PLAYBACK_TRANS_CONTINUE             = 0x0683,
    CMD_PLAYBACK_REPORT_BUFFER_CHECK        = 0x0684,
    CMD_PLAYBACK_VALID_SONG                 = 0x0685,
    CMD_PLAYBACK_TRIGGER_ROLE_SWAP          = 0x0686,
    CMD_PLAYBACK_TRANS_CANCEL               = 0x0687,
    CMD_PLAYBACK_EXIT_TRANS                 = 0x0688,
    CMD_PLAYBACK_PERMANENT_DELETE_SONG      = 0x0689,
    CMD_PLAYBACK_PLAYLIST_ADD_SONG          = 0x068A,
    CMD_PLAYBACK_PLAYLIST_DELETE_SONG       = 0x068B,
    CMD_PLAYBACK_PERMANENT_DELETE_ALL_SONG  = 0x068C,
#endif

    CMD_GET_SUPPORTED_MMI_LIST          = 0x0700,
    CMD_GET_SUPPORTED_CLICK_TYPE        = 0x0701,
    CMD_GET_SUPPORTED_CALL_STATUS       = 0x0702,
    CMD_GET_KEY_MMI_MAP                 = 0x0703,
    CMD_SET_KEY_MMI_MAP                 = 0x0704,

    CMD_RESET_KEY_MMI_MAP               = 0x0707,

    CMD_GET_RWS_KEY_MMI_MAP             = 0x0708,
    CMD_SET_RWS_KEY_MMI_MAP             = 0x0709,
    CMD_RESET_RWS_KEY_MMI_MAP           = 0x070A,

    CMD_VENDOR_SEPC                     = 0x0800, //It has been reserved for vendor customer A, please dont't use this value.
#if HARMAN_VBAT_ONE_ADC_DETECTION
	CMD_USER_SINGLE_NTC 				= 0x0801,
#endif

    CMD_DFU_START                       = 0x0900,

    //for customize
    CMD_RSV1                            = 0x0A00,
    CMD_RSV2                            = 0x0A01,
    CMD_RSV3                            = 0x0A02,
    CMD_SET_MERIDIAN_SOUND_EFFECT_MODE  = 0x0A03,
    CMD_LG_CUSTOMIZED_FEATURE           = 0x0A04,
    CMD_CUSTOMIZED_SITRON_FEATURE       = 0x0A05,
    CMD_JSA_CALIBRATION                 = 0x0A06,
    CMD_MIC_MP_VERIFY_BY_HFP            = 0x0A07,
    CMD_GET_DSP_CONFIG_GAIN             = 0x0A08,
    CMD_CUSTOMIZED_TOZO_FEATURE         = 0x0A09,
    CMD_RSV4                            = 0x0A0A,
    CMD_IO_PIN_PULL_HIGH                = 0x0A0B,
    CMD_ENTER_BAT_OFF_MODE              = 0x0A0C,
    CMD_SASS_FEATURE                    = 0x0A0D,

    //for HCI command
    CMD_HCI                             = 0x0B00,
    CMD_WDG_RESET                       = 0x0B01,
    CMD_DUAL_MIC_MP_VERIFY              = 0x0B02,

    CMD_SOUND_PRESS_CALIBRATION         = 0x0B10,
    CMD_CAP_TOUCH_CTL                   = 0x0B11,

    //for ANC command
    CMD_ANC_TEST_MODE                   = 0x0C00,
    CMD_ANC_WRITE_GAIN                  = 0x0C01,
    CMD_ANC_READ_GAIN                   = 0x0C02,
    CMD_ANC_BURN_GAIN                   = 0x0C03,
    CMD_ANC_COMPARE                     = 0x0C04,
    CMD_ANC_GEN_TONE                    = 0x0C05,
    CMD_ANC_CONFIG_DATA_LOG             = 0x0C06,
    CMD_ANC_READ_DATA_LOG               = 0x0C07,
    CMD_ANC_READ_MIC_CONFIG             = 0x0C08,
    CMD_ANC_READ_SPEAKER_CHANNEL        = 0x0C09,
    CMD_ANC_READ_REGISTER               = 0x0C0A,
    CMD_ANC_WRITE_REGISTER              = 0x0C0B,
    CMD_ANC_LLAPT_WRITE_GAIN            = 0x0C0C,
    CMD_ANC_LLAPT_READ_GAIN             = 0x0C0D,
    CMD_ANC_LLAPT_BURN_GAIN             = 0x0C0E,
    CMD_ANC_LLAPT_FEATURE_CONTROL       = 0x0C0F,

    CMD_ANC_QUERY                       = 0x0C20,
    CMD_ANC_ENABLE_DISABLE              = 0x0C21,
    CMD_LLAPT_QUERY                     = 0x0C22,
    CMD_LLAPT_ENABLE_DISABLE            = 0x0C23,

    CMD_RAMP_GET_INFO                   = 0x0C26,
    CMD_RAMP_BURN_PARA_START            = 0x0C27,
    CMD_RAMP_BURN_PARA_CONTINUE         = 0x0C28,
    CMD_RAMP_BURN_GRP_INFO              = 0x0C29,
    CMD_RAMP_MULTI_DEVICE_APPLY         = 0x0C2A,

    CMD_LISTENING_MODE_CYCLE_SET        = 0x0C2B,
    CMD_LISTENING_MODE_CYCLE_GET        = 0x0C2C,

    CMD_VENDOR_SPP_COMMAND              = 0x0C2D,

    CMD_APT_VOLUME_INFO                 = 0x0C2E,
    CMD_APT_VOLUME_SET                  = 0x0C2F,
    CMD_APT_VOLUME_STATUS               = 0x0C30,
    CMD_LLAPT_BRIGHTNESS_INFO           = 0x0C31,
    CMD_LLAPT_BRIGHTNESS_SET            = 0x0C32,
    CMD_LLAPT_BRIGHTNESS_STATUS         = 0x0C33,
    CMD_LLAPT_SCENARIO_CHOOSE_INFO      = 0x0C36,
    CMD_LLAPT_SCENARIO_CHOOSE_TRY       = 0x0C37,
    CMD_LLAPT_SCENARIO_CHOOSE_RESULT    = 0x0C38,
    CMD_APT_GET_POWER_ON_DELAY_TIME     = 0x0C39,
    CMD_APT_SET_POWER_ON_DELAY_TIME     = 0x0C3A,
    CMD_LISTENING_STATE_SET             = 0x0C3B,
    CMD_LISTENING_STATE_STATUS          = 0x0C3C,

    // ADSP PARA
    CMD_ANC_GET_ADSP_INFO               = 0x0C40,
    CMD_ANC_SEND_ADSP_PARA_START        = 0x0C41,
    CMD_ANC_SEND_ADSP_PARA_CONTINUE     = 0x0C42,
    CMD_ANC_ENABLE_DISABLE_ADAPTIVE_ANC = 0x0C43,

    CMD_ANC_SCENARIO_CHOOSE_INFO        = 0x0C44,
    CMD_ANC_SCENARIO_CHOOSE_TRY         = 0x0C45,
    CMD_ANC_SCENARIO_CHOOSE_RESULT      = 0x0C46,

    // OTA Tooling section
    CMD_OTA_TOOLING_PARKING             = 0x0D00,
    CMD_MEMORY_DUMP                     = 0x0D22,

    CMD_GET_LOW_LATENCY_MODE_STATUS     = 0x0E01,
    CMD_GET_EAR_DETECTION_STATUS        = 0x0E02,
    CMD_GET_GAMING_MODE_EQ_INDEX        = 0x0E03,
    CMD_SET_LOW_LATENCY_LEVEL           = 0x0E04,

    CMD_MP_TEST                         = 0x0F00,
    CMD_FORCE_ENGAGE                    = 0x0F01,
    CMD_AGING_TEST_CONTROL              = 0x0F02,

#if F_APP_AVP_INIT_SUPPORT
    CMD_AVP_LD_EN                       = 0x1100,
    CMD_AVP_ANC_CYCLE_SET               = 0x1101,
    CMD_AVP_CLICK_SET                   = 0x1102,
    CMD_AVP_CONTROL_SET                 = 0x1103,
    CMD_AVP_ANC_SETTINGS                = 0x1104,
#endif

#if (F_APP_HEARABLE_SUPPORT == 1)
    /* only support BBPro2 */
    CMD_HA_SET_PARAM                    = 0x2000,
    CMD_HA_VER_REQ                      = 0x2001,
    CMD_HA_ACCESS_EFFECT_INDEX          = 0x2002,
    CMD_HA_ACCESS_ON_OFF                = 0x2003,
    CMD_HA_GET_TOOL_EXTEND_REQ          = 0x2004,
    CMD_HA_SET_DSP_PARAM                = 0x2005,
    CMD_HA_ACCESS_PROGRAM_ID            = 0x2006,
    CMD_HA_SET_NR_PARAM                 = 0x2007,
    CMD_HA_GET_PROGRAM_NUM              = 0x2008,
    CMD_HA_ACCESS_PROGRAM_NAME          = 0x2009,
    CMD_HA_SET_OVP_PARAM                = 0x200a,
#endif

#if F_APP_SS_SUPPORT
    CMD_SS_REQ                          = 0x2200,
#endif
} T_CMD_ID;

/** @brief  packet type for legacy transfer*/
typedef enum t_pkt_type
{
    PKT_TYPE_SINGLE = 0x00,
    PKT_TYPE_START = 0x01,
    PKT_TYPE_CONT = 0x02,
    PKT_TYPE_END = 0x03
} T_PKT_TYPE;

typedef enum
{
    BUD_TYPE_SINGLE      = 0x00,
    BUD_TYPE_RWS         = 0x01,
} T_BUD_TYPE;

typedef enum
{
    APP_REMOTE_MSG_CMD_GET_FW_VERSION           = 0x00,
    APP_REMOTE_MSG_CMD_REPORT_FW_VERSION        = 0x01,
    APP_REMOTE_MSG_CMD_GET_OTA_FW_VERSION       = 0x02,
    APP_REMOTE_MSG_CMD_REPORT_OTA_FW_VERSION    = 0x03,
    APP_REMOTE_MSG_DSP_DEBUG_SIGNAL_IN_SYNC     = 0x04,    // only support BBPro2

    APP_REMOTE_MSG_SYNC_EQ_CTRL_BY_SRC          = 0x05,
    APP_REMOTE_MSG_CMD_TOTAL                    = 0x06,
} T_CMD_REMOTE_MSG;

//for CMD_SET_CFG
typedef enum
{
    CFG_SET_LE_NAME                  = 0x00,
    CFG_SET_LEGACY_NAME              = 0x01,
    CFG_SET_AUDIO_LATENCY            = 0x02,
    CFG_SET_SUPPORT_CODEC            = 0x03,
    CFG_SET_DURIAN_ID                = 0x04,
    CFG_SET_DURIAN_SINGLE_ID         = 0x05,
    CFG_SET_HFP_REPORT_BATT          = 0x06,
} T_CMD_SET_CFG_TYPE;

/**  @brief CMD Set Info Request type. */
typedef enum
{
    CMD_SET_INFO_TYPE_VERSION = 0x00,
    CMD_INFO_GET_CAPABILITY   = 0x01,
} T_CMD_SET_INFO_TYPE;

typedef enum
{
    GET_STATUS_RWS_STATE                  = 0x00,
    GET_STATUS_RWS_CHANNEL                = 0x01,
    GET_STATUS_BATTERY_STATUS             = 0x02,
    GET_STATUS_APT_STATUS                 = 0x03,
    GET_STATUS_APP_STATE                  = 0x04,
    GET_STATUS_BUD_ROLE                   = 0x05,
    GET_STATUS_APT_NR_STATUS              = 0x06,
    GET_STATUS_APT_VOL                    = 0x07,
    GET_STATUS_LOCK_BUTTON                = 0x08,
    GET_STATUS_FIND_ME                    = 0x09,
    GET_STATUS_ANC_STATUS                 = 0x0A,
    GET_STATUS_LLAPT_STATUS               = 0x0B,
    GET_STATUS_RWS_DEFAULT_CHANNEL        = 0x0C,
    GET_STATUS_RWS_BUD_SIDE               = 0x0D,
    GET_STATUS_RWS_SYNC_APT_VOL           = 0x0E,

    /* for LG (BBLite D-cut) */
    GET_STATUS_BUD_ROLE_FOR_LG            = 0xa0,
    GET_STATUS_VOLUME                     = 0xa1,
    GET_STATUS_MERIDIAN_SOUND_EFFECT_MODE = 0xa2,
    GET_STATUS_LIGHT_SENSOR               = 0xa3,

#if F_APP_AVP_INIT_SUPPORT
    GET_STATUS_AVP_RWS_VER                = 0xa4,
    GET_STATUS_AVP_MULTILINK_ON_OFF       = 0xa5,
#endif

    GET_STATUS_FACTORY_RESET_STATUS       = 0xa6,
} T_CMD_GET_STATUS_TYPE;

typedef struct
{
    // Byte 0
    uint8_t snk_support_get_set_le_name : 1;
    uint8_t snk_support_get_set_br_name : 1;
    uint8_t snk_support_get_set_vp_language : 1;
    uint8_t snk_support_get_battery_info : 1;
    uint8_t snk_support_ota : 1;
    uint8_t snk_support_change_channel : 1;
    uint8_t snk_support_rsv1 : 2;

    // Byte 1
    uint8_t snk_support_tts : 1;
    uint8_t snk_support_get_set_rws_state : 1;
    uint8_t snk_support_get_set_apt_state : 1; //bit 10
    uint8_t snk_support_get_set_eq_state : 1;
    uint8_t snk_support_get_set_vad_state : 1;
    uint8_t snk_support_get_set_anc_state : 1;
    uint8_t snk_support_get_set_llapt_state : 1; //bit 14
    uint8_t snk_support_get_set_listening_mode_cycle : 1;

    // Byte 2
    uint8_t snk_support_llapt_brightness : 1;
    uint8_t snk_support_anc_eq : 1;
    uint8_t snk_support_apt_eq : 1;
    uint8_t snk_support_tone_volume_adjustment : 1;
    uint8_t snk_support_apt_eq_adjust_separate : 1; //bit20
    uint8_t snk_support_multilink_support : 1;
    uint8_t snk_support_avp : 1; //bit22
    uint8_t snk_support_nr_on_off : 1;

    // Byte 3
    uint8_t snk_support_llapt_scenario_choose : 1;
    uint8_t snk_support_ear_detection : 1;
    uint8_t snk_support_power_on_delay_apply_apt_on : 1;
    uint8_t snk_support_phone_set_anc_eq : 1; //bit 27
    uint8_t snk_support_new_report_bud_status_flow : 1;
    uint8_t snk_support_new_report_listening_status : 1;
    uint8_t snk_support_apt_volume_force_adjust_sync : 1; //bit 30
    uint8_t snk_support_reset_key_remap : 1;

    // Byte 4
    uint8_t snk_support_ansc : 1;
    uint8_t snk_support_vibrator : 1;
    uint8_t snk_support_change_mfb_func : 1;
    uint8_t snk_support_gaming_mode : 1;
    uint8_t snk_support_gaming_mode_eq : 1;
    uint8_t snk_support_key_remap : 1;
    uint8_t snk_support_HA: 1;
    uint8_t snk_support_local_playback : 1;  //bit39

    // Byte 5
    uint8_t snk_support_rsv8_1 : 2;
    uint8_t snk_support_anc_scenario_choose : 1;
    uint8_t snk_support_rws_key_remap : 1;
    uint8_t snk_support_rsv8_2 : 1;
    uint8_t snk_support_reset_key_map_by_bud : 1;
    uint8_t snk_support_rsv8_3 : 2;
} T_SNK_CAPABILITY;

/**  @brief  cmd set status to phone
  */
typedef enum
{
    CMD_SET_STATUS_COMPLETE = 0x00,
    CMD_SET_STATUS_DISALLOW = 0x01,
    CMD_SET_STATUS_UNKNOW_CMD = 0x02,
    CMD_SET_STATUS_PARAMETER_ERROR = 0x03,
    CMD_SET_STATUS_BUSY = 0x04,
    CMD_SET_STATUS_PROCESS_FAIL = 0x05,
    CMD_SET_STATUS_ONE_WIRE_EXTEND = 0x06,
    CMD_SET_STATUS_VERSION_INCOMPATIBLE = 0x07,
} T_CMD_SET_STATUS;

typedef struct
{
    uint32_t flash_data_start_addr_tmp;
    uint32_t flash_data_start_addr;
    uint32_t flash_data_size;
    uint8_t flash_data_type;
} T_FLASH_DATA;

typedef union
{
    uint8_t d8[16];
    struct
    {
        uint32_t ver_major: 4;      //!< major version
        uint32_t ver_minor: 8;      //!< minor version
        uint32_t ver_revision: 15;  //!< revision version
        uint32_t ver_reserved: 5;   //!< reserved
        uint32_t ver_commitid;      //!< git commit id
        uint8_t customer_name[8];   //!< customer name
    };
} T_PATCH_IMG_VER_FORMAT;

typedef union
{
    uint32_t version;
    struct
    {
        uint32_t ver_major: 8;      //!< major version
        uint32_t ver_minor: 8;      //!< minor version
        uint32_t ver_revision: 8;   //!< revision version
        uint32_t ver_reserved: 8;   //!< reserved
    };
} T_APP_UI_IMG_VER_FORMAT;

typedef union
{
    uint8_t version[4];
    struct
    {
        uint8_t cmd_set_ver_major;
        uint8_t cmd_set_ver_minor;
        uint8_t eq_spec_ver_major;
        uint8_t eq_spec_ver_minor;
    };
} T_SRC_SUPPORT_VER_FORMAT;

typedef enum
{
    LE_RSSI,
    LEGACY_RSSI
} T_RSSI_TYPE;

typedef enum
{
    REG_ACCESS_READ,
    REG_ACCESS_WRITE,
} T_CMD_REG_ACCESS_ACTION;

typedef enum
{
    REG_ACCESS_TYPE_AON,
    REG_ACCESS_TYPE_AON2B,
    REG_ACCESS_TYPE_DIRECT,
} T_CMD_REG_ACCESS_TYPE;

typedef enum
{
    DSP_TOOL_FUNCTION_BRIGHTNESS = 0x0000,
} T_CMD_DSP_TOOL_FUNCTION_TYPE;

/** End of APP_CMD_Exported_Types
    * @}
    */
/*============================================================================*
 *                              Functions
 *============================================================================*/
/** @defgroup APP_CMD_Exported_Functions App Cmd Functions
    * @{
    */
/**
    * @brief  App process uart or embedded spp vendor command.
    * @param  cmd_ptr command type
    * @param  cmd_len command length
    * @param  cmd_path command path use for distinguish uart,or le,or spp channel.
    * @param  rx_seqn recieved command sequence
    * @param  app_idx received rx command device index
    * @return void
    */
void app_handle_cmd_set(uint8_t *cmd_ptr, uint16_t cmd_len, uint8_t cmd_path, uint8_t rx_seqn,
                        uint8_t app_idx);

#if (F_APP_OTA_TOOLING_SUPPORT == 1)
void app_cmd_ota_tooling_parking(void);
void app_cmd_stop_ota_parking_power_off(void);
#endif

#if F_APP_DEVICE_CMD_SUPPORT
bool app_cmd_get_auto_reject_conn_req_flag(void);
bool app_cmd_get_auto_accept_conn_req_flag(void);
bool app_cmd_get_report_attr_info_flag(void);
void app_cmd_set_report_attr_info_flag(bool flag);
#endif

#if (F_APP_SPP_CAPTURE_DSP_DATA == 1)
uint8_t app_cmd_dsp_capture_data_state(void);
bool app_cmd_spp_capture_executing_check(void);
bool app_cmd_spp_capture_audio_dsp_ctrl_send_handler(uint8_t *cmd_ptr, uint16_t cmd_len,
                                                     uint8_t cmd_path, uint8_t app_idx, uint8_t *ack_pkt, bool send_flag);
extern void spp_capture_ble_stop(void);
#endif
/* @brief  app cmd init
*
* @param  void
* @return none
*/
void app_cmd_init(void);
bool app_cmd_relay_command_set(uint16_t cmd_id, uint8_t *cmd_ptr, uint16_t cmd_len,
                               T_APP_MODULE_TYPE module_type, uint8_t relay_cmd_id, bool sync);
bool app_cmd_relay_event(uint16_t event_id, uint8_t *event_ptr, uint16_t event_len,
                         T_APP_MODULE_TYPE module_type, uint8_t relay_event_id);

bool app_cmd_get_tool_connect_status(void);
void app_cmd_update_eq_ctrl(uint8_t value, bool is_need_relay);
T_SRC_SUPPORT_VER_FORMAT *app_cmd_get_src_version(uint8_t cmd_path, uint8_t app_idx);
bool app_cmd_check_specific_version(T_SRC_SUPPORT_VER_FORMAT *version, uint8_t ver_major,
                                    uint8_t ver_minor);
bool app_cmd_check_src_cmd_version(uint8_t cmd_path, uint8_t app_idx);
bool app_cmd_check_src_eq_spec_version(uint8_t cmd_path, uint8_t app_idx);
void app_cmd_handle_mp_cmd_hci_evt(void *p_gap_vnd_cmd_cb_data);
/** @} */ /* End of group APP_CMD_Exported_Functions */
/** End of APP_CMD
* @}
*/

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _APP_CMD_H_ */
