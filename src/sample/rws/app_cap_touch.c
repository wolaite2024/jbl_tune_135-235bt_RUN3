/**
************************************************************************************************************
*            Copyright(c) 2020, Realtek Semiconductor Corporation. All rights reserved.
************************************************************************************************************
* @file      app_cap_touch.c
* @brief     app Cap Touch Demonstration.
* @author
* @date      2020-12-08
* @version   v1.0
*************************************************************************************************************
*/
#if APP_CAP_TOUCH_SUPPORT

#include <stdlib.h>
#include "trace.h"
#include "vector_table.h"
#include "rtl876x_captouch.h"
#include "os_msg.h"
#include "os_sched.h"
#include "os_mem.h"
#include "app_msg.h"
#include "app_main.h"
#include "pmu_api.h"
#include "app_cap_touch.h"
#include "app_key_process.h"
#include "board.h"
#include "app_cfg.h"
#include "app_mmi.h"
#include "gap_timer.h"
#include "app_sensor.h"
#include "app_audio_policy.h"
#include "app_dlps.h"
#include "app_report.h"
#include "app_cmd.h"
#include "app_transfer.h"
#include "app_io_msg.h"

#define APP_CAP_TOUCH_DGB 0
#define APP_CAP_TOUCH_MAX_CHANNEL_NUM   4
#define APP_CAP_TOUCH_MAX_BASELINE 4095
#define APP_CAP_TOUCH_MAX_MBIAS_LEVEL 64
#define APP_CAP_TOUCH_MAX_THRESHOLD 4095

#define APP_CAP_TOUCH_FIRST_REPORT_DELAY_TIME   500
#define APP_CAP_TOUCH_RECORD_DATA_NUM   256
#define APP_CAP_TOUCH_REPORT_INTERVAL   (500)
#define APP_CAP_TOUCH_RECORD_INTERVAL   (fast_mode_scan_interval)

typedef struct t_app_cap_spp_cmd
{
    uint32_t baseline   : 12;
    uint32_t threshold  : 12;
    uint32_t mbias      : 8 ;
} T_APP_CAP_SPP_CFG_SET;

typedef enum
{
    CAP_TOUCH_TIMER_CTC_DLPS,
    CAP_TOUCH_TIMER_CTC_CHANNEL,
    CAP_TOUCH_TIMER_REPORT_ADC_VALUE,
    CAP_TOUCH_TIMER_REPORT_ADC_RECORD,
} T_APP_CAP_TPUCH_TIMER;

typedef enum
{
    CAP_TOUCH_CMD_GET_STATUS,               //0x00
    CAP_TOUCH_CMD_SET_BASELINE,             //0x01
    CAP_TOUCH_CMD_SET_MBIAS,                //0x02
    CAP_TOUCH_CMD_SET_THRESHOLD,            //0x03
    CAP_TOUCH_CMD_SET_TEST_MODE_STATUS,     //0x04
    CAP_TOUCH_CMD_GET_TEST_MODE_STATUS,     //0x05
    CAP_TOUCH_CMD_SET_CURRENT_ADC_REPORT,   //0x06
    CAP_TOUCH_CMD_SET_PERIOD_ADC_REPORT,    //0x07
} T_APP_CAP_TOUCH_SUB_CMD;

typedef enum
{
    CAP_TOUCH_EVENT_REPORT_STATUS,
    CAP_TOUCH_EVENT_REPORT_TEST_MODE_STATUS,
    CAP_TOUCH_EVENT_REPORT_CURRENT_ADC,
    CAP_TOUCH_EVENT_REPORT_PERIOD_ADC,
} T_APP_CAP_TOUCH_SUB_EVENT;

// CTC state machine
typedef enum
{
    SLIDE_STATE_IDLE = 0x00,

    SLIDE_STATE_R_SLIDE_START,          //0x01
    SLIDE_STATE_L_SLIDE_START,          //0x02
    SLIDE_STATE_TOUCH_START,            //0x03

    SLIDE_STATE_R_SLIDE_LEV2_TOUCH,     //0x04
    SLIDE_STATE_R_SLIDE_LEV2_SLIDEL,    //0x05
    SLIDE_STATE_L_SLIDE_LEV2_TOUCH,     //0x06
    SLIDE_STATE_L_SLIDE_LEV2_SLIDER,    //0x07
    SLIDE_STATE_TOUCH_LEV2_TOUCH,       //0x08
    SLIDE_STATE_TOUCH_LEV2_SLIDER,      //0x09
    SLIDE_STATE_TOUCH_LEV2_SLIDEL,      //0x0A

    SLIDE_STATE_R_SLIDE_LEV3_SLIDEL,    //0x0B
    SLIDE_STATE_R_SLIDE_LEV3_TOUCH,     //0x0C
    SLIDE_STATE_L_SLIDE_LEV3_SLIDER,    //0x0D
    SLIDE_STATE_L_SLIDE_LEV3_TOUCH,     //0x0E
    SLIDE_STATE_TOUCH_LEV3_SLIDEL,      //0x0F
    SLIDE_STATE_TOUCH_LEV3_SLIDER,      //0x10
} T_CAP_TOUCH_SLIDE_STATE;

typedef enum
{
    CAP_TOUCH_LD_TYPE_NONE,
    CAP_TOUCH_LD_TYPE_ONE_CH,
    CAP_TOUCH_LD_TYPE_TWO_CH,
} T_CAP_TOUCH_LD_TYPE;

// mapping with config setting
typedef enum
{
    CHANNEL_TYPE_CTC_NONE,
    CHANNEL_TYPE_TOUCH_DET_ISR,
    CHANNEL_TYPE_LEFT_DET_ISR,
    CHANNEL_TYPE_RIGHT_DET_ISR,
    CHANNEL_TYPE_IN_EAR_DET_CH1_ISR,
    CHANNEL_TYPE_IN_EAR_DET_CH2_ISR,

    CHANNEL_TYPE_CTC_TPYE_TOTAL = 0xF,
} T_CAP_TOUCH_CHANNEL_TYPE;

typedef enum
{
    TOUCH_ACTION_CTC_PRESS,
    TOUCH_ACTION_CTC_RELEASE,
    TOUCH_ACTION_CTC_FALSE_TRIGGER,
} T_CAP_TOUCH_ACTION;

typedef enum
{
    CAP_TOUCH_ADC_REPORT_TYPE_OFF,
    CAP_TOUCH_ADC_REPORT_TYPE_REALTIME,
    CAP_TOUCH_ADC_REPORT_TYPE_RECORD,
} T_CAP_TOUCH_ADC_REPORT_TYPE;

static T_CAP_TOUCH_SLIDE_STATE slide_state = SLIDE_STATE_IDLE;
static uint16_t slow_mode_scan_interval = 0x3B;//59
static uint16_t fast_mode_scan_interval = 0x1D;//29
static bool ld_det_ch0 = false;
static bool ld_det_ch1 = false;
static bool ctc_in_ear_from_case = false;
static bool ctc_in_air_from_ear = false;
static bool ctc_ld_cause_action = false;
static bool ctc_test_mode_enable = false;
static bool ctc_ld_set_enable = false;
static uint8_t orignal_dt_resend_num = 3;
static T_CAP_TOUCH_ADC_REPORT_TYPE ctc_adc_report_type = CAP_TOUCH_ADC_REPORT_TYPE_OFF;
static uint8_t *ctc_adc_report_value = NULL;
static uint8_t record_data_num = 0;
static uint8_t  *adc_report_ptr;
static T_APP_CAP_SPP_CFG_SET *cap_touch_param_info;

static void *timer_handle_cap_touch_dlps_check = NULL;
static void *timer_handle_cap_touch_channel_adc = NULL;
static void *timer_handle_cap_touch_report_adc_value = NULL;
static uint8_t cap_touch_timer_queue_id = 0;

struct T_CTC_ISR_CB
{
    void (*func)(uint8_t);
};

static T_CAP_TOUCH_SLIDE_STATE app_cap_touch_get_slide_state(void);
static T_CAP_TOUCH_ACTION app_cap_touch_get_curr_action(uint8_t event);
static void app_cap_touch_slide_state_enter(T_CAP_TOUCH_CHANNEL_TYPE cur_isr);
static void app_cap_touch_right_trigger(T_CAP_TOUCH_CHANNEL_TYPE isr);
static void app_cap_touch_left_trigger(T_CAP_TOUCH_CHANNEL_TYPE isr);
static void app_cap_touch_slide_state_init(T_CAP_TOUCH_CHANNEL_TYPE isr);
static void app_cap_touch_state_reset(void);
static void app_cap_touch_slide_left_handle(uint8_t event);
static void app_cap_touch_slide_right_handle(uint8_t event);
static void app_cap_touch_handle(uint8_t event);
static void app_cap_touch_in_ear_handle(uint8_t event, T_CAP_TOUCH_CHANNEL_TYPE LD_channel);
static void app_cap_touch_in_ear_detect_ch1(uint8_t event);
static void app_cap_touch_in_ear_detect_ch2(uint8_t event);
static void app_cap_touch_ld_check(void);
static void app_cap_touch_end(void);
static void app_cap_touch_cfg_init(void);
static void app_cap_touch_reg_timer(void);
static void app_cap_touch_parameter_init(void);
static void app_cap_touch_work(bool enable);
void (*cap_touch_channel0)(uint8_t);
void (*cap_touch_channel1)(uint8_t);
void (*cap_touch_channel2)(uint8_t);
void (*cap_touch_channel3)(uint8_t);

const static struct T_CTC_ISR_CB ctc_trigger_event[CHANNEL_TYPE_CTC_TPYE_TOTAL] =
{
    NULL,
    app_cap_touch_handle,
    app_cap_touch_slide_left_handle,
    app_cap_touch_slide_right_handle,
    app_cap_touch_in_ear_detect_ch1,
    app_cap_touch_in_ear_detect_ch2,
};

void app_cap_touch_set_ld_det(bool enable)
{
    ctc_ld_set_enable = enable;
}

bool app_cap_touch_get_ld_det(void)
{
    return ctc_ld_set_enable;
}

static void app_cap_touch_reset_all_setting(void)
{
    app_cap_touch_work(false);
    CapTouch_SysReset();
    app_cap_touch_init(false);
    app_cap_touch_work(true);
}

static void app_cap_touch_reset_test_mode_setting(void)
{
    ctc_test_mode_enable = false;
    ctc_adc_report_type = CAP_TOUCH_ADC_REPORT_TYPE_OFF;
    app_cfg_const.dt_resend_num = orignal_dt_resend_num;
    orignal_dt_resend_num = 0;
    gap_stop_timer(&timer_handle_cap_touch_report_adc_value);
    if (ctc_adc_report_value != NULL)
    {
        free(ctc_adc_report_value);
        ctc_adc_report_value = NULL;
    }
    if (cap_touch_param_info != NULL)
    {
        free(cap_touch_param_info);
        cap_touch_param_info = NULL;
    }
}

static void app_cap_touch_slide_left_handle(uint8_t event)
{
    T_CAP_TOUCH_ACTION action = app_cap_touch_get_curr_action(event);
    CTC_PRINT_TRACE2("app_cap_touch_slide_left_handle act %d do_act %d", event, action);

    switch (action)
    {
    case TOUCH_ACTION_CTC_PRESS:
    case TOUCH_ACTION_CTC_FALSE_TRIGGER:
        break;

    case TOUCH_ACTION_CTC_RELEASE:
        {
            app_cap_touch_slide_state_enter(CHANNEL_TYPE_LEFT_DET_ISR);
        }
        break;
    }
}

static void app_cap_touch_slide_right_handle(uint8_t event)
{
    T_CAP_TOUCH_ACTION action = app_cap_touch_get_curr_action(event);
    CTC_PRINT_TRACE2("app_cap_touch_slide_right_handle  act %d do_act %d", event, action);

    switch (action)
    {
    case TOUCH_ACTION_CTC_PRESS:
    case TOUCH_ACTION_CTC_FALSE_TRIGGER:
        break;

    case TOUCH_ACTION_CTC_RELEASE:
        {
            app_cap_touch_slide_state_enter(CHANNEL_TYPE_RIGHT_DET_ISR);
        }
        break;
    }
}

static void app_cap_touch_handle(uint8_t event)
{
    T_CAP_TOUCH_ACTION action = app_cap_touch_get_curr_action(event);
    CTC_PRINT_TRACE3("app_cap_touch_handle act %d do_act %d, test mode %d", event, action,
                     ctc_test_mode_enable);

    if (action == TOUCH_ACTION_CTC_PRESS)
    {
        app_cap_touch_slide_state_enter(CHANNEL_TYPE_TOUCH_DET_ISR);
    }

    if ((slide_state == SLIDE_STATE_TOUCH_START) || (slide_state == SLIDE_STATE_TOUCH_LEV2_TOUCH) ||
        (slide_state == SLIDE_STATE_IDLE))
    {
        if ((action == TOUCH_ACTION_CTC_PRESS) || (action == TOUCH_ACTION_CTC_RELEASE))
        {
            if (!ctc_test_mode_enable)
            {
                app_key_cap_touch_key_process((action == TOUCH_ACTION_CTC_PRESS) ? KEY_PRESS : KEY_RELEASE);
            }
        }
    }
}

static void app_cap_touch_in_ear_handle(uint8_t event, T_CAP_TOUCH_CHANNEL_TYPE LD_channel)
{
    if (event < IO_MSG_CAP_TOUCH_OVER_N_NOISE)
    {
        T_CAP_TOUCH_ACTION action = app_cap_touch_get_curr_action(event);

        CTC_PRINT_TRACE3("app_cap_touch_in_ear_handle ch %d act %d do_act %d", LD_channel, event, action);
        if (LD_channel == CHANNEL_TYPE_IN_EAR_DET_CH1_ISR)
        {
            ld_det_ch0 = (action == TOUCH_ACTION_CTC_RELEASE) ? false : true;
        }
        else if (LD_channel == CHANNEL_TYPE_IN_EAR_DET_CH2_ISR)
        {
            ld_det_ch1 = (action == TOUCH_ACTION_CTC_RELEASE) ? false : true;
        }
        app_cap_touch_ld_check();
    }
}

static void app_cap_touch_in_ear_detect_ch1(uint8_t event)
{
    app_cap_touch_in_ear_handle(event, CHANNEL_TYPE_IN_EAR_DET_CH1_ISR);
}

static void app_cap_touch_in_ear_detect_ch2(uint8_t event)
{
    app_cap_touch_in_ear_handle(event, CHANNEL_TYPE_IN_EAR_DET_CH2_ISR);
}

static bool app_cap_touch_send_msg(T_IO_MSG_CAP_TOUCH subtype, void *param_buf)
{
    uint8_t  event;
    T_IO_MSG msg;

    event = EVENT_IO_TO_APP;

    msg.type    = IO_MSG_TYPE_CAP_TOUCH;
    msg.subtype = subtype;
    msg.u.buf   = param_buf;
    msg.subtype = subtype;
//   gpio_msg.u.param = (audio_path | bit_res << 8); for feature implement
    if (os_msg_send(audio_evt_queue_handle, &event, 0) == true)
    {
        return os_msg_send(audio_io_queue_handle, &msg, 0);
    }

    return false;
}

/**
  * @brief  CapTouch Interrupt handler.
  * @retval None
  */
ISR_TEXT_SECTION
static void app_cap_touch_Handler(void)
{
    uint32_t int_status = 0;
    int_status = CapTouch_GetINTStatus();
    T_IO_MSG_CAP_TOUCH msg_type = IO_MSG_CAP_TOTAL;

    app_dlps_disable(APP_DLPS_ENTER_CHECK_GPIO);

    if (cap_touch_channel0)
    {
        /* Channel 0 interrupts */
        if (CapTouch_IsChINTTriggered(int_status, CTC_CH0, CTC_TOUCH_PRESS_INT))
        {
            /* do something */
            CapTouch_ChINTClearPendingBit(CTC_CH0, CTC_TOUCH_PRESS_INT);
            msg_type = IO_MSG_CAP_CH0_PRESS;
        }
        if (CapTouch_IsChINTTriggered(int_status, CTC_CH0, CTC_TOUCH_RELEASE_INT))
        {
            /* do something */
            CapTouch_ChINTClearPendingBit(CTC_CH0, CTC_TOUCH_RELEASE_INT);
            msg_type = IO_MSG_CAP_CH0_RELEASE;
        }
        if (CapTouch_IsChINTTriggered(int_status, CTC_CH0, CTC_FALSE_TOUCH_INT))
        {
            /* do something */
            CapTouch_ChINTClearPendingBit(CTC_CH0, CTC_FALSE_TOUCH_INT);
            msg_type = IO_MSG_CAP_CH0_FALSE_TRIGGER;
        }
    }

    if (cap_touch_channel1)
    {
        /* Channel 1 interrupts */
        if (CapTouch_IsChINTTriggered(int_status, CTC_CH1, CTC_TOUCH_PRESS_INT))
        {
            /* do something */
            CapTouch_ChINTClearPendingBit(CTC_CH1, CTC_TOUCH_PRESS_INT);
            msg_type = IO_MSG_CAP_CH1_PRESS;
        }
        if (CapTouch_IsChINTTriggered(int_status, CTC_CH1, CTC_TOUCH_RELEASE_INT))
        {
            /* do something */
            CapTouch_ChINTClearPendingBit(CTC_CH1, CTC_TOUCH_RELEASE_INT);
            msg_type = IO_MSG_CAP_CH1_RELEASE;
        }
        if (CapTouch_IsChINTTriggered(int_status, CTC_CH1, CTC_FALSE_TOUCH_INT))
        {
            /* do something */
            CapTouch_ChINTClearPendingBit(CTC_CH1, CTC_FALSE_TOUCH_INT);
            msg_type = IO_MSG_CAP_CH1_FALSE_TRIGGER;
        }
    }

    if (cap_touch_channel2)
    {
        /* Channel 2 interrupts */
        if (CapTouch_IsChINTTriggered(int_status, CTC_CH2, CTC_TOUCH_PRESS_INT))
        {
            /* do something */
            CapTouch_ChINTClearPendingBit(CTC_CH2, CTC_TOUCH_PRESS_INT);
            msg_type = IO_MSG_CAP_CH2_PRESS;
        }
        if (CapTouch_IsChINTTriggered(int_status, CTC_CH2, CTC_TOUCH_RELEASE_INT))
        {
            /* do something */
            CapTouch_ChINTClearPendingBit(CTC_CH2, CTC_TOUCH_RELEASE_INT);
            msg_type = IO_MSG_CAP_CH2_RELEASE;
        }
        if (CapTouch_IsChINTTriggered(int_status, CTC_CH2, CTC_FALSE_TOUCH_INT))
        {
            /* do something */
            CapTouch_ChINTClearPendingBit(CTC_CH2, CTC_FALSE_TOUCH_INT);
            msg_type = IO_MSG_CAP_CH2_FALSE_TRIGGER;
        }
    }

    if (cap_touch_channel3)
    {
        /* Channel 3 interrupts */
        if (CapTouch_IsChINTTriggered(int_status, CTC_CH3, CTC_TOUCH_PRESS_INT))
        {
            /* do something */
            CapTouch_ChINTClearPendingBit(CTC_CH3, CTC_TOUCH_PRESS_INT);
            msg_type = IO_MSG_CAP_CH3_PRESS;
        }
        if (CapTouch_IsChINTTriggered(int_status, CTC_CH3, CTC_TOUCH_RELEASE_INT))
        {
            /* do something */
            CapTouch_ChINTClearPendingBit(CTC_CH3, CTC_TOUCH_RELEASE_INT);
            msg_type = IO_MSG_CAP_CH3_RELEASE;
        }
        if (CapTouch_IsChINTTriggered(int_status, CTC_CH3, CTC_FALSE_TOUCH_INT))
        {
            /* do something */
            CapTouch_ChINTClearPendingBit(CTC_CH3, CTC_FALSE_TOUCH_INT);
            msg_type = IO_MSG_CAP_CH3_FALSE_TRIGGER;
        }
    }
    /* Noise Interrupt */
    if (CapTouch_IsNoiseINTTriggered(int_status, CTC_OVER_N_NOISE_INT))
    {
        /* do something */
        CapTouch_NoiseINTClearPendingBit(CTC_OVER_N_NOISE_INT);
        msg_type = IO_MSG_CAP_TOUCH_OVER_N_NOISE;
    }
    if (CapTouch_IsNoiseINTTriggered(int_status, CTC_OVER_P_NOISE_INT))
    {
        msg_type = IO_MSG_CAP_TOUCH_OVER_P_NOISE;
        CapTouch_NoiseINTClearPendingBit(CTC_OVER_P_NOISE_INT);
    }
    CTC_PRINT_TRACE1("app_cap_touch_Handler %d", msg_type);
    app_cap_touch_send_msg(msg_type, NULL);
}

static void app_cap_touch_parameter_init(void)
{
    RamVectorTableUpdate(TOUCH_VECTORn, (IRQ_Fun)app_cap_touch_Handler);

    if (cap_touch_channel0)
    {
        APP_PRINT_TRACE0("app_cap_touch_parameter_init cap_touch_channel0");
        CapTouch_ChCmd(CTC_CH0, ENABLE);
        CapTouch_ChINTConfig(CTC_CH0, (CTC_CH_INT_TYPE)(CTC_TOUCH_PRESS_INT | CTC_TOUCH_RELEASE_INT |
                                                        CTC_FALSE_TOUCH_INT), ENABLE);
        CapTouch_SetChBaseline(CTC_CH0, app_cfg_const.ctc_chan_0_default_baseline);
        CapTouch_SetChMbias(CTC_CH0, app_cfg_const.ctc_chan_0_mbias);
        CapTouch_SetChDiffThres(CTC_CH0, app_cfg_const.ctc_chan_0_threshold);
        CapTouch_SetChPNoiseThres(CTC_CH0, app_cfg_const.ctc_chan_0_threshold / 2);
        CapTouch_SetChNNoiseThres(CTC_CH0, app_cfg_const.ctc_chan_0_threshold / 2);
        CapTouch_ChWakeupCmd(CTC_CH0, (FunctionalState)ENABLE);
    }

    if (cap_touch_channel1)
    {
        APP_PRINT_TRACE0("app_cap_touch_parameter_init cap_touch_channel1");
        CapTouch_ChCmd(CTC_CH1, ENABLE);
        CapTouch_ChINTConfig(CTC_CH1, (CTC_CH_INT_TYPE)(CTC_TOUCH_PRESS_INT | CTC_TOUCH_RELEASE_INT |
                                                        CTC_FALSE_TOUCH_INT), ENABLE);
        CapTouch_SetChBaseline(CTC_CH1, app_cfg_const.ctc_chan_1_default_baseline);
        CapTouch_SetChMbias(CTC_CH1, app_cfg_const.ctc_chan_1_mbias);
        CapTouch_SetChDiffThres(CTC_CH1, app_cfg_const.ctc_chan_1_threshold);
        CapTouch_SetChPNoiseThres(CTC_CH1, app_cfg_const.ctc_chan_1_threshold / 2);
        CapTouch_SetChNNoiseThres(CTC_CH1, app_cfg_const.ctc_chan_1_threshold / 2);
        CapTouch_ChWakeupCmd(CTC_CH1, (FunctionalState)ENABLE);
    }
    if (cap_touch_channel2)
    {
        APP_PRINT_TRACE0("app_cap_touch_parameter_init cap_touch_channel2");
        uint16_t ctc_chan_2_threshold = app_cfg_const.ctc_chan_2_threshold_upper << 8 |
                                        app_cfg_const.ctc_chan_2_threshold_lower;
        CapTouch_ChCmd(CTC_CH2, ENABLE);
        CapTouch_ChINTConfig(CTC_CH2, (CTC_CH_INT_TYPE)(CTC_TOUCH_PRESS_INT | CTC_TOUCH_RELEASE_INT |
                                                        CTC_FALSE_TOUCH_INT), ENABLE);
        CapTouch_SetChBaseline(CTC_CH2, app_cfg_const.ctc_chan_2_default_baseline);
        CapTouch_SetChMbias(CTC_CH2, app_cfg_const.ctc_chan_2_mbias);
        CapTouch_SetChDiffThres(CTC_CH2, ctc_chan_2_threshold);
        CapTouch_SetChPNoiseThres(CTC_CH2, ctc_chan_2_threshold / 2);
        CapTouch_SetChNNoiseThres(CTC_CH2, ctc_chan_2_threshold / 2);
        CapTouch_ChWakeupCmd(CTC_CH2, (FunctionalState)ENABLE);
    }
    if (cap_touch_channel3)
    {
        APP_PRINT_TRACE0("app_cap_touch_parameter_init cap_touch_channel3");
        CapTouch_ChCmd(CTC_CH3, ENABLE);

        CapTouch_ChINTConfig(CTC_CH3, (CTC_CH_INT_TYPE)(CTC_TOUCH_PRESS_INT | CTC_TOUCH_RELEASE_INT |
                                                        CTC_FALSE_TOUCH_INT), ENABLE);
        CapTouch_SetChBaseline(CTC_CH3, app_cfg_const.ctc_chan_3_default_baseline);
        CapTouch_SetChMbias(CTC_CH3, app_cfg_const.ctc_chan_3_mbias);
        CapTouch_SetChDiffThres(CTC_CH3, app_cfg_const.ctc_chan_3_threshold);
        CapTouch_SetChPNoiseThres(CTC_CH3, app_cfg_const.ctc_chan_3_threshold / 2);
        CapTouch_SetChNNoiseThres(CTC_CH3, app_cfg_const.ctc_chan_3_threshold / 2);
        CapTouch_ChWakeupCmd(CTC_CH3, (FunctionalState)ENABLE);
    }
    //CapTouch_NoiseINTConfig(CTC_OVER_P_NOISE_INT, ENABLE);

    /* Set scan interval */
    if (!CapTouch_SetScanInterval(slow_mode_scan_interval, CTC_SLOW_MODE))
    {
        CTC_PRINT_WARN0("Slow mode scan interval overange!");
    }
    if (!CapTouch_SetScanInterval(fast_mode_scan_interval, CTC_FAST_MODE))
    {
        CTC_PRINT_WARN0("Fast mode scan interval overange!");
    }

    pmu_set_clk_32k_power_in_powerdown(true); // keep 32k clk when enter power down
}

static void app_cap_touch_work(bool enable)
{
    if (enable && !CapTouch_IsRunning())
    {
        CapTouch_Cmd(ENABLE, DISABLE);
    }
    else if (!enable)
    {
        CapTouch_Cmd(DISABLE, DISABLE);
    }
    CTC_PRINT_INFO2("app_cap_touch_work: %d, %d", enable, CapTouch_IsRunning());
}

void app_cap_touch_msg_handle(T_IO_MSG_CAP_TOUCH subtype)
{
    CTC_PRINT_TRACE1("app_cap_touch_msg_handle 0x%x", subtype);

    if (timer_handle_cap_touch_dlps_check != NULL)
    {
        gap_stop_timer(&timer_handle_cap_touch_dlps_check);
    }

    switch (subtype)
    {
    case IO_MSG_CAP_TOUCH_OVER_P_NOISE: // press ISR
        {
            app_cap_touch_reset_all_setting();
        }
        break;

    case IO_MSG_CAP_CH0_PRESS:
    case IO_MSG_CAP_CH0_RELEASE:
    case IO_MSG_CAP_CH0_FALSE_TRIGGER:
        {
            if (cap_touch_channel0)
            {
                cap_touch_channel0(subtype);
            }
        }
        break;

    case IO_MSG_CAP_CH1_PRESS:
    case IO_MSG_CAP_CH1_RELEASE:
    case IO_MSG_CAP_CH1_FALSE_TRIGGER:
        {
            if (cap_touch_channel1)
            {
                cap_touch_channel1(subtype);
            }
        }
        break;

    case IO_MSG_CAP_CH2_PRESS:
    case IO_MSG_CAP_CH2_RELEASE:
    case IO_MSG_CAP_CH2_FALSE_TRIGGER:
        {
            if (cap_touch_channel2)
            {
                cap_touch_channel2(subtype);
            }
        }
        break;

    case IO_MSG_CAP_CH3_PRESS:
    case IO_MSG_CAP_CH3_RELEASE:
    case IO_MSG_CAP_CH3_FALSE_TRIGGER:
        {
            if (cap_touch_channel3)
            {
                cap_touch_channel3(subtype);
            }
        }
        break;

    default:
        break;
    }

    app_dlps_enable(APP_DLPS_ENTER_CHECK_GPIO);
}

static void app_cap_touch_slide_state_enter(T_CAP_TOUCH_CHANNEL_TYPE cur_isr)
{
    CTC_PRINT_TRACE2("app_cap_touch_slide_state_enter %d isr %d", slide_state, cur_isr);

    switch (slide_state)
    {
    case SLIDE_STATE_IDLE:
        {
            app_key_start_cap_touch_check_timer();
            app_cap_touch_slide_state_init(cur_isr);
        }
        break;

    case SLIDE_STATE_R_SLIDE_START:
    case SLIDE_STATE_R_SLIDE_LEV2_TOUCH:
    case SLIDE_STATE_R_SLIDE_LEV2_SLIDEL:
    case SLIDE_STATE_R_SLIDE_LEV3_SLIDEL:
    case SLIDE_STATE_R_SLIDE_LEV3_TOUCH:
        {
            app_cap_touch_right_trigger(cur_isr);
        }
        break;

    case SLIDE_STATE_L_SLIDE_START:
    case SLIDE_STATE_L_SLIDE_LEV2_TOUCH:
    case SLIDE_STATE_L_SLIDE_LEV2_SLIDER:
    case SLIDE_STATE_L_SLIDE_LEV3_SLIDER:
    case SLIDE_STATE_L_SLIDE_LEV3_TOUCH:
        {
            app_cap_touch_left_trigger(cur_isr);
        }
        break;

        /*case SLIDE_STATE_TOUCH_START:
        case SLIDE_STATE_TOUCH_LEV2_TOUCH:
        case SLIDE_STATE_TOUCH_LEV2_SLIDER:
        case SLIDE_STATE_TOUCH_LEV2_SLIDEL:
        case SLIDE_STATE_TOUCH_LEV3_SLIDEL:
        case SLIDE_STATE_TOUCH_LEV3_SLIDER:
            {
                app_cap_touch_touch_trigger(cur_isr);
            }
            break;*/
    }
}

static void app_cap_touch_slide_state_init(T_CAP_TOUCH_CHANNEL_TYPE isr)
{
    switch (isr)
    {
    case CHANNEL_TYPE_LEFT_DET_ISR:
        {
            slide_state =  SLIDE_STATE_L_SLIDE_START;
        }
        break;
    case CHANNEL_TYPE_RIGHT_DET_ISR:
        {
            slide_state = SLIDE_STATE_R_SLIDE_START;
        }
        break;
    case CHANNEL_TYPE_TOUCH_DET_ISR:
        {
            slide_state = SLIDE_STATE_TOUCH_START;
        }
        break;
    }
}

static void app_cap_touch_right_trigger(T_CAP_TOUCH_CHANNEL_TYPE isr)
{
    switch (slide_state)
    {
    case SLIDE_STATE_R_SLIDE_START:
        {
            if (isr == CHANNEL_TYPE_LEFT_DET_ISR)
            {
                slide_state = SLIDE_STATE_R_SLIDE_LEV2_SLIDEL;
            }
            else if (isr == CHANNEL_TYPE_TOUCH_DET_ISR)
            {
                slide_state = SLIDE_STATE_R_SLIDE_LEV2_TOUCH;
            }
        }
        break;

    case SLIDE_STATE_R_SLIDE_LEV2_TOUCH:
        {
            slide_state = (isr == CHANNEL_TYPE_LEFT_DET_ISR) ? SLIDE_STATE_R_SLIDE_LEV3_SLIDEL :
                          SLIDE_STATE_R_SLIDE_LEV2_TOUCH;
        }
        break;

    case SLIDE_STATE_R_SLIDE_LEV2_SLIDEL:
        {
            slide_state = (isr == CHANNEL_TYPE_TOUCH_DET_ISR) ? SLIDE_STATE_R_SLIDE_LEV3_TOUCH :
                          SLIDE_STATE_R_SLIDE_LEV2_SLIDEL;
        }
        break;

    case SLIDE_STATE_R_SLIDE_LEV3_SLIDEL:
    case SLIDE_STATE_R_SLIDE_LEV3_TOUCH:
        break;

    default :
        CTC_PRINT_TRACE2("app_cap_touch_right_trigger unexcept state %d %d", slide_state, isr);
        break;
    }
}

static void app_cap_touch_left_trigger(T_CAP_TOUCH_CHANNEL_TYPE isr)
{
    switch (slide_state)
    {
    case SLIDE_STATE_L_SLIDE_START:
        {
            if (isr == CHANNEL_TYPE_RIGHT_DET_ISR)
            {
                slide_state = SLIDE_STATE_L_SLIDE_LEV2_SLIDER;
            }
            else if (isr == CHANNEL_TYPE_TOUCH_DET_ISR)
            {
                slide_state = SLIDE_STATE_L_SLIDE_LEV2_TOUCH;
            }
        }
        break;

    case SLIDE_STATE_L_SLIDE_LEV2_TOUCH:
        {
            slide_state = (isr == CHANNEL_TYPE_RIGHT_DET_ISR) ? SLIDE_STATE_L_SLIDE_LEV3_SLIDER :
                          SLIDE_STATE_L_SLIDE_LEV2_TOUCH;
        }
        break;

    case SLIDE_STATE_L_SLIDE_LEV2_SLIDER:
        {
            slide_state = (isr == CHANNEL_TYPE_TOUCH_DET_ISR) ? SLIDE_STATE_L_SLIDE_LEV3_TOUCH :
                          SLIDE_STATE_L_SLIDE_LEV2_SLIDER;
        }
        break;

    case SLIDE_STATE_L_SLIDE_LEV3_SLIDER:
    case SLIDE_STATE_L_SLIDE_LEV3_TOUCH:
        break;

    default :
        CTC_PRINT_TRACE2("app_cap_touch_left_trigger unexcept state %d %d", slide_state, isr);
        break;
    }
}

/*void app_cap_touch_touch_trigger(T_CAP_TOUCH_CHANNEL_TYPE isr)
{
    switch (slide_state)
    {
    case SLIDE_STATE_TOUCH_START:
    case SLIDE_STATE_TOUCH_LEV2_TOUCH:
    case SLIDE_STATE_TOUCH_LEV2_SLIDER:
    case SLIDE_STATE_TOUCH_LEV2_SLIDEL:
    case SLIDE_STATE_TOUCH_LEV3_SLIDEL:
    case SLIDE_STATE_TOUCH_LEV3_SLIDER:
        {
        }
        break;
    default :
        CTC_PRINT_TRACE2("touch_trigger unexcept state %d %d", slide_state, isr);
        break;
    }

}*/

static T_CAP_TOUCH_SLIDE_STATE app_cap_touch_get_slide_state(void)
{
    CTC_PRINT_TRACE1("app_cap_touch_get_slide_state %x", slide_state);
    return slide_state;
}

static T_CAP_TOUCH_ACTION app_cap_touch_get_curr_action(uint8_t event)
{
    return (T_CAP_TOUCH_ACTION)(event % 3);
}

static void app_cap_touch_state_reset(void)
{
    slide_state = SLIDE_STATE_IDLE;
}

static void app_cap_touch_ld_check(void)
{
    APP_PRINT_TRACE3("app_cap_touch_ld_check ld_det %d %d %d", app_cap_touch_get_ld_det(),
                     ld_det_ch0,
                     ld_det_ch1);
    if ((((app_cfg_const.ctc_light_det_pin_num == CAP_TOUCH_LD_TYPE_TWO_CH) &&
          (ld_det_ch0 == ld_det_ch1)) ||
         (app_cfg_const.ctc_light_det_pin_num == CAP_TOUCH_LD_TYPE_ONE_CH)) &&
        app_cap_touch_get_ld_det())
    {
        T_IO_MSG gpio_msg;   //in-out ear

        gpio_msg.type = IO_MSG_TYPE_GPIO;
        gpio_msg.subtype = IO_MSG_GPIO_SENSOR_LD;
        gpio_msg.u.param = (ld_det_ch0) ? SENSOR_LD_IN_EAR : SENSOR_LD_OUT_EAR;
        if (app_io_send_msg(&gpio_msg) == false)
        {
            APP_PRINT_ERROR0("app_sensor_sent_in_out_ear_event: Send msg error");
        }
    }
}

void app_cap_touch_send_event_check(void)
{
    uint8_t key_action = MMI_NULL;

    switch (slide_state)
    {
    case SLIDE_STATE_R_SLIDE_LEV3_SLIDEL:
    case SLIDE_STATE_L_SLIDE_LEV3_SLIDER:
        {
            app_key_stop_timer();
            app_key_cap_touch_stop_timer();
            key_action = (slide_state == SLIDE_STATE_R_SLIDE_LEV3_SLIDEL) ? app_cfg_const.ctc_slide_key_right :
                         app_cfg_const.ctc_slide_key_left ;
            app_mmi_handle_action(key_action);
        }
        break;

    case SLIDE_STATE_R_SLIDE_LEV2_TOUCH:
    case SLIDE_STATE_L_SLIDE_LEV2_TOUCH:
    case SLIDE_STATE_TOUCH_LEV2_SLIDER:
    case SLIDE_STATE_TOUCH_LEV2_SLIDEL:
    case SLIDE_STATE_R_SLIDE_LEV3_TOUCH:
    case SLIDE_STATE_L_SLIDE_LEV3_TOUCH:
    case SLIDE_STATE_TOUCH_LEV3_SLIDEL:
    case SLIDE_STATE_TOUCH_LEV3_SLIDER:
        {
            //trigger single click
            app_key_single_click(KEY0_MASK);
        }
        break;

    default:
        CTC_PRINT_TRACE1("app_cap_touch_send_event_check exp %d", slide_state);
    }
    app_cap_touch_state_reset();
}

static void app_cap_touch_cfg_init(void)
{
    CTC_PRINT_TRACE4("app_cap_touch_cfg_init CH0 %x CH1 %x CH2 %x CH3 %x",
                     app_cfg_const.ctc_chan_0_type,
                     app_cfg_const.ctc_chan_1_type,
                     app_cfg_const.ctc_chan_2_type,
                     app_cfg_const.ctc_chan_3_type);

    cap_touch_channel0 =
        ctc_trigger_event[app_cfg_const.ctc_chan_0_type].func;
    cap_touch_channel1 =
        ctc_trigger_event[app_cfg_const.ctc_chan_1_type].func;
    cap_touch_channel2 =
        ctc_trigger_event[app_cfg_const.ctc_chan_2_type].func;
    cap_touch_channel3 =
        ctc_trigger_event[app_cfg_const.ctc_chan_3_type].func;
}

static void app_cap_touch_end(void)
{
    switch (slide_state)
    {
    case SLIDE_STATE_R_SLIDE_LEV3_SLIDEL:
    case SLIDE_STATE_L_SLIDE_LEV3_SLIDER:
    case SLIDE_STATE_R_SLIDE_LEV3_TOUCH:
    case SLIDE_STATE_L_SLIDE_LEV3_TOUCH:
    case SLIDE_STATE_TOUCH_LEV3_SLIDEL:
    case SLIDE_STATE_TOUCH_LEV3_SLIDER:
        {
            CTC_PRINT_TRACE1("app_cap_touch_end %x", slide_state);
            app_cap_touch_state_reset();
        }
        break;
    }
}

static void app_cap_touch_timeout_cb(uint8_t timer_id, uint16_t timer_chann)
{
    CTC_PRINT_TRACE2("app_cap_touch_timeout_cb: timer_id 0x%02x, timer_chann %d", timer_id,
                     timer_chann);

    switch (timer_id)
    {
    case CAP_TOUCH_TIMER_CTC_DLPS:
        {
            gap_stop_timer(&timer_handle_cap_touch_dlps_check);
            app_dlps_enable(APP_DLPS_ENTER_CHECK_GPIO);
        }
        break;
#if APP_CAP_TOUCH_DGB
    case CAP_TOUCH_TIMER_CTC_CHANNEL:
        {
            gap_stop_timer(&timer_handle_cap_touch_channel_adc);
            gap_start_timer(&timer_handle_cap_touch_channel_adc, "CAP_TOUCH_TIMER_CTC_CHANNEL",
                            cap_touch_timer_queue_id, CAP_TOUCH_TIMER_CTC_CHANNEL, 0, 1000);
            //captouch_get_reg(0x108) : cap touch register addres

            if (cap_touch_channel0)
            {
                CTC_PRINT_TRACE4("ch0_baseline 0x%x curr_mbias 0x%x curr_thres 0x%x curr_avg %d",
                                 CapTouch_GetChBaseline(CTC_CH0),
                                 CapTouch_GetChMbias(CTC_CH0) * 25,
                                 CapTouch_GetChDiffThres(CTC_CH0),
                                 CapTouch_GetChAveData(CTC_CH0));
            }
            if (cap_touch_channel1)
            {
                CTC_PRINT_TRACE4("ch1_baseline 0x%x curr_mbias 0x%x curr_thres 0x%x curr_avg %d",
                                 CapTouch_GetChBaseline(CTC_CH1),
                                 CapTouch_GetChMbias(CTC_CH1) * 25,
                                 CapTouch_GetChDiffThres(CTC_CH1),
                                 CapTouch_GetChAveData(CTC_CH1));
            }
            if (cap_touch_channel2)
            {
                CTC_PRINT_TRACE4("ch2_baseline 0x%x curr_mbias 0x%x curr_thres 0x%x curr_avg %d",
                                 CapTouch_GetChBaseline(CTC_CH2),
                                 CapTouch_GetChMbias(CTC_CH2) * 25,
                                 CapTouch_GetChDiffThres(CTC_CH2),
                                 CapTouch_GetChAveData(CTC_CH2));
            }
            if (cap_touch_channel3)
            {
                CTC_PRINT_TRACE4("ch3_baseline 0x%x curr_mbias 0x%x curr_thres 0x%x curr_avg %d",
                                 CapTouch_GetChBaseline(CTC_CH3),
                                 CapTouch_GetChMbias(CTC_CH3) * 25,
                                 CapTouch_GetChDiffThres(CTC_CH3),
                                 CapTouch_GetChAveData(CTC_CH3));
            }
        }
        break;
#endif
    case CAP_TOUCH_TIMER_REPORT_ADC_VALUE:
        {
            uint8_t buf[4] = {0};
            uint16_t average_data;
            uint8_t channel = timer_chann;
            if (ctc_adc_report_type == CAP_TOUCH_ADC_REPORT_TYPE_REALTIME)
            {
                average_data = CapTouch_GetChAveData((CTC_CH_TYPE)channel);

                buf[0] = CAP_TOUCH_EVENT_REPORT_CURRENT_ADC; //status_index
                buf[1] = channel;
                memcpy(&buf[2], &average_data, sizeof(average_data));
                app_report_event_broadcast(EVENT_CAP_TOUCH_CTL, buf, 4);
                app_pop_data_transfer_queue(CMD_PATH_SPP, true);
                gap_start_timer(&timer_handle_cap_touch_report_adc_value, "report_adc_value",
                                cap_touch_timer_queue_id, CAP_TOUCH_TIMER_REPORT_ADC_VALUE, channel, APP_CAP_TOUCH_REPORT_INTERVAL);
            }
        }
        break;
    case CAP_TOUCH_TIMER_REPORT_ADC_RECORD:
        {
            uint8_t channel = timer_chann;
            gap_stop_timer(&timer_handle_cap_touch_report_adc_value);
            if (ctc_adc_report_type == CAP_TOUCH_ADC_REPORT_TYPE_RECORD)
            {
                uint16_t avgDATA = 0;
                if (record_data_num < APP_CAP_TOUCH_RECORD_DATA_NUM - 1)
                {
                    avgDATA = CapTouch_GetChAveData((CTC_CH_TYPE)channel);
                    memcpy(&ctc_adc_report_value[record_data_num * 2 + 2], &avgDATA, 2);
                    record_data_num++;
                    gap_start_timer(&timer_handle_cap_touch_report_adc_value, "report_adc_value",
                                    cap_touch_timer_queue_id, CAP_TOUCH_TIMER_REPORT_ADC_RECORD, channel,
                                    APP_CAP_TOUCH_RECORD_INTERVAL);
                }
                else
                {
                    avgDATA = CapTouch_GetChAveData((CTC_CH_TYPE)channel);
                    memcpy(&ctc_adc_report_value[record_data_num * 2 + 2], &avgDATA, 2);
                    app_report_event_broadcast(EVENT_CAP_TOUCH_CTL, ctc_adc_report_value,
                                               APP_CAP_TOUCH_RECORD_DATA_NUM * 2 + 2);
                    free(ctc_adc_report_value);
                    ctc_adc_report_value = NULL;
                    ctc_adc_report_type = CAP_TOUCH_ADC_REPORT_TYPE_OFF;
                    record_data_num = 0;
                }
            }
            else
            {
                if (ctc_adc_report_value)
                {
                    free(ctc_adc_report_value);
                    ctc_adc_report_value = NULL;
                }
            }
        }
        break;
    default:
        break;
    }
}

/*void app_cap_touch_stop_adc_check_timer(void)
{
    gap_stop_timer(&timer_handle_cap_touch_channel_adc);
}*/

static bool app_cap_touch_enable_adc_report(uint8_t channel, bool enable)
{
    uint8_t error_code = 0;
    if (enable)
    {
        if (ctc_adc_report_type != CAP_TOUCH_ADC_REPORT_TYPE_OFF)
        {
            error_code = 1;
            goto ENABLE_ADC_REPORT_ERROR;
        }
        else if (!CapTouch_GetChStatus((CTC_CH_TYPE)channel))
        {
            error_code = 2;
            goto ENABLE_ADC_REPORT_ERROR;
        }
        else
        {
            ctc_adc_report_type = CAP_TOUCH_ADC_REPORT_TYPE_REALTIME;
            gap_stop_timer(&timer_handle_cap_touch_report_adc_value);
            gap_start_timer(&timer_handle_cap_touch_report_adc_value, "report_adc_value",
                            cap_touch_timer_queue_id, CAP_TOUCH_TIMER_REPORT_ADC_VALUE, channel,
                            APP_CAP_TOUCH_FIRST_REPORT_DELAY_TIME);
            return true;
        }
    }
    else
    {
        if (ctc_adc_report_type != CAP_TOUCH_ADC_REPORT_TYPE_REALTIME)
        {
            error_code = 1;
            goto ENABLE_ADC_REPORT_ERROR;
        }
        else
        {
            ctc_adc_report_type = CAP_TOUCH_ADC_REPORT_TYPE_OFF;
            gap_stop_timer(&timer_handle_cap_touch_report_adc_value);
            app_transfer_queue_reset(CMD_PATH_SPP);
            return true;
        }
    }
ENABLE_ADC_REPORT_ERROR:
    APP_PRINT_ERROR1("app_cap_touch_enable_adc_report error code %d", error_code);
    return false;
}

static bool app_cap_touch_enable_adc_record(uint8_t channel, bool enable)
{
    uint8_t error_code = 0;
    if (enable)
    {
        if (ctc_adc_report_type != CAP_TOUCH_ADC_REPORT_TYPE_OFF)
        {
            error_code = 1;
            goto ENABLE_ADC_RECORD_ERROR;
        }
        else if (!CapTouch_GetChStatus((CTC_CH_TYPE)channel))
        {
            error_code = 2;
            goto ENABLE_ADC_RECORD_ERROR;
        }
        else
        {
#if APP_CAP_TOUCH_DGB
            APP_PRINT_WARN1("unused memory: %d", mem_peek());
#endif
            ctc_adc_report_value = malloc(APP_CAP_TOUCH_RECORD_DATA_NUM * 2 + 2);
            if (ctc_adc_report_value != NULL)
            {
                ctc_adc_report_type = CAP_TOUCH_ADC_REPORT_TYPE_RECORD;
                record_data_num = 0;
                memset(ctc_adc_report_value, 0xff, APP_CAP_TOUCH_RECORD_DATA_NUM * 2 + 2);
                ctc_adc_report_value[0] = CAP_TOUCH_EVENT_REPORT_PERIOD_ADC;
                ctc_adc_report_value[1] = channel;
                gap_stop_timer(&timer_handle_cap_touch_report_adc_value);
                gap_start_timer(&timer_handle_cap_touch_report_adc_value, "report_adc_value",
                                cap_touch_timer_queue_id, CAP_TOUCH_TIMER_REPORT_ADC_RECORD, channel,
                                APP_CAP_TOUCH_FIRST_REPORT_DELAY_TIME);
                return true;
            }
            else
            {
                error_code = 3;
                goto ENABLE_ADC_RECORD_ERROR;
            }
        }
    }
    else
    {
        if (ctc_adc_report_type != CAP_TOUCH_ADC_REPORT_TYPE_RECORD)
        {
            error_code = 1;
            goto ENABLE_ADC_RECORD_ERROR;
        }
        else
        {
            ctc_adc_report_type = CAP_TOUCH_ADC_REPORT_TYPE_OFF;
            gap_stop_timer(&timer_handle_cap_touch_report_adc_value);
            if (ctc_adc_report_value != NULL)
            {
                app_report_event_broadcast(EVENT_CAP_TOUCH_CTL, ctc_adc_report_value,
                                           APP_CAP_TOUCH_RECORD_DATA_NUM * 2 + 2);
                free(ctc_adc_report_value);
                ctc_adc_report_value = NULL;
                record_data_num = 0;
            }
            return true;
        }
    }
ENABLE_ADC_RECORD_ERROR:
    APP_PRINT_ERROR1("app_cap_touch_enable_adc_record error code %d", error_code);
    return false;
}

static uint8_t app_cap_touch_set_cfg(uint8_t cmd_id, uint8_t *cmd_ptr)
{
    uint8_t ret = CMD_SET_STATUS_PROCESS_FAIL;
    uint16_t baseline;
    uint16_t mbias;
    uint16_t threshold;
    uint16_t average_data;
    uint8_t channel_num = cmd_ptr[0];
    if (!ctc_test_mode_enable)
    {
        ret = CMD_SET_STATUS_DISALLOW;
        return ret;
    }
    app_cap_touch_work(false);
    CapTouch_SysReset();
    app_cap_touch_parameter_init();
    switch (cmd_id)
    {
    case CAP_TOUCH_CMD_SET_BASELINE:
        {
            baseline = (uint16_t)(cmd_ptr[1] | (cmd_ptr[2] << 8));
            if (baseline <= APP_CAP_TOUCH_MAX_BASELINE)
            {
                cap_touch_param_info[channel_num].baseline = baseline;
                ret = CMD_SET_STATUS_COMPLETE;
            }
        }
        break;
    case CAP_TOUCH_CMD_SET_MBIAS:
        {
            mbias = cmd_ptr[1];

            if (mbias < APP_CAP_TOUCH_MAX_MBIAS_LEVEL)
            {
                cap_touch_param_info[channel_num].mbias = mbias;
                ret = CMD_SET_STATUS_COMPLETE;
            }
        }
        break;
    case CAP_TOUCH_CMD_SET_THRESHOLD:
        {
            threshold = (uint16_t)(cmd_ptr[1] | (cmd_ptr[2] << 8));
            if (threshold <= APP_CAP_TOUCH_MAX_THRESHOLD)
            {
                cap_touch_param_info[channel_num].threshold = threshold;
                ret = CMD_SET_STATUS_COMPLETE;
            }
        }
        break;
    }

    for (uint8_t i = 0; i < APP_CAP_TOUCH_MAX_CHANNEL_NUM ; i++)
    {
        CapTouch_SetChBaseline((CTC_CH_TYPE)i, cap_touch_param_info[i].baseline);
        CapTouch_SetChMbias((CTC_CH_TYPE)i, cap_touch_param_info[i].mbias);
        CapTouch_SetChDiffThres((CTC_CH_TYPE)i, cap_touch_param_info[i].threshold);
        CapTouch_SetChPNoiseThres((CTC_CH_TYPE)i, cap_touch_param_info[i].threshold / 2);
        CapTouch_SetChNNoiseThres((CTC_CH_TYPE)i, cap_touch_param_info[i].threshold / 2);
    }
    app_cap_touch_work(true);
    return ret;
}

static bool app_cap_touch_check_channel_type_enable(CTC_CH_TYPE channel_num)
{
    if (((app_cfg_const.ctc_chan_0_type) && (channel_num == 0)) ||
        ((app_cfg_const.ctc_chan_1_type) && (channel_num == 1))
        ||
        ((app_cfg_const.ctc_chan_2_type) && (channel_num == 2)) ||
        ((app_cfg_const.ctc_chan_3_type) && (channel_num == 3)))
    {
        return true;
    }
    else
    {
        return false;
    }
}

void app_cap_touch_cmd_param_handle(uint8_t cmd_path, uint8_t *cmd_ptr, uint8_t app_idx)
{
    uint8_t sub_id = cmd_ptr[2];
    uint8_t i;
    uint8_t temp = 0;
    uint8_t  ack_pkt[3];
    uint16_t baseline;
    uint16_t mbias;
    uint16_t threshold;
    uint16_t average_data;
    ack_pkt[0] = cmd_ptr[0];
    ack_pkt[1] = cmd_ptr[1];
    ack_pkt[2] = CMD_SET_STATUS_PROCESS_FAIL;

    switch (sub_id)
    {
    case CAP_TOUCH_CMD_GET_STATUS:
        {
            uint8_t *buf = NULL;//sub event_id 1 + test_mode status 1 + channel data 36
            buf = malloc(APP_CAP_TOUCH_MAX_CHANNEL_NUM * 9 + 2);
            memset(buf, 0x00, APP_CAP_TOUCH_MAX_CHANNEL_NUM * 9 + 2);
            buf[0] = CAP_TOUCH_EVENT_REPORT_STATUS;
            buf[1] = ctc_test_mode_enable;
            for (i = 0; i < APP_CAP_TOUCH_MAX_CHANNEL_NUM; i++)
            {
                temp = i * 9 + 2;
                buf[temp++] = i;
                if (app_cap_touch_check_channel_type_enable((CTC_CH_TYPE)i))
                {
                    baseline = CapTouch_GetChBaseline((CTC_CH_TYPE)i);
                    memcpy(&buf[temp], &baseline, sizeof(baseline));
                    temp += 2;
                    mbias = CapTouch_GetChMbias((CTC_CH_TYPE)i);
                    memcpy(&buf[temp], &mbias, sizeof(mbias));
                    temp += 2;
                    threshold = CapTouch_GetChDiffThres((CTC_CH_TYPE)i);
                    memcpy(&buf[temp], &threshold, sizeof(threshold));
                    temp += 2;
                    average_data = CapTouch_GetChAveData((CTC_CH_TYPE)i);
                    memcpy(&buf[temp], &average_data, sizeof(average_data));
                }
            }
            ack_pkt[2] = CMD_SET_STATUS_COMPLETE;
            app_report_event(cmd_path, EVENT_ACK, app_idx, ack_pkt, 3);
            app_report_event(cmd_path, EVENT_CAP_TOUCH_CTL, app_idx, buf,
                             APP_CAP_TOUCH_MAX_CHANNEL_NUM * 9 + 2);
            free(buf);
        }
        break;
    case CAP_TOUCH_CMD_SET_BASELINE:
    case CAP_TOUCH_CMD_SET_MBIAS:
    case CAP_TOUCH_CMD_SET_THRESHOLD:
        {
            ack_pkt[2] = app_cap_touch_set_cfg(sub_id, &cmd_ptr[3]);
            app_report_event(cmd_path, EVENT_ACK, app_idx, ack_pkt, 3);
        }
        break;
    case CAP_TOUCH_CMD_SET_TEST_MODE_STATUS:
        {
            ack_pkt[2] = CMD_SET_STATUS_COMPLETE;
            bool enable = cmd_ptr[3];
            app_cfg_const.dt_resend_num = 0;

            if (enable)
            {
                if (cap_touch_param_info == NULL)
                {
                    cap_touch_param_info = malloc(sizeof(T_APP_CAP_SPP_CFG_SET) * APP_CAP_TOUCH_MAX_CHANNEL_NUM);
                    memset(cap_touch_param_info, 0x00, sizeof(T_APP_CAP_SPP_CFG_SET) * APP_CAP_TOUCH_MAX_CHANNEL_NUM);
                    for (uint8_t i = 0; i < APP_CAP_TOUCH_MAX_CHANNEL_NUM; i++)
                    {
                        if (app_cap_touch_check_channel_type_enable((CTC_CH_TYPE)i))
                        {
                            cap_touch_param_info[i].baseline = CapTouch_GetChBaseline((CTC_CH_TYPE) i);
                            cap_touch_param_info[i].mbias = CapTouch_GetChMbias((CTC_CH_TYPE) i);
                            cap_touch_param_info[i].threshold = CapTouch_GetChDiffThres((CTC_CH_TYPE) i);
                        }
                    }
                }
            }
            else
            {
                app_cap_touch_reset_all_setting();
                app_cap_touch_reset_test_mode_setting();
            }
            ctc_test_mode_enable = enable;
            app_report_event(cmd_path, EVENT_ACK, app_idx, ack_pkt, 3);
        }
        break;
    case CAP_TOUCH_CMD_GET_TEST_MODE_STATUS:
        {
            uint8_t buf[2] = {0};
            ack_pkt[2] = CMD_SET_STATUS_COMPLETE;
            buf[0] = CAP_TOUCH_EVENT_REPORT_TEST_MODE_STATUS;
            buf[1] = ctc_test_mode_enable;
            app_report_event(cmd_path, EVENT_ACK, app_idx, ack_pkt, 3);
            app_report_event(cmd_path, EVENT_CAP_TOUCH_CTL, app_idx, buf,
                             sizeof(buf));
        }
        break;
    case CAP_TOUCH_CMD_SET_CURRENT_ADC_REPORT:
    case CAP_TOUCH_CMD_SET_PERIOD_ADC_REPORT:
        {
            uint8_t channel = cmd_ptr[3];
            bool enable = (bool)cmd_ptr[4];

            if (ctc_test_mode_enable)
            {
                if (!CapTouch_IsChAllowed((CTC_CH_TYPE)channel))
                {
                    ack_pkt[2] = CMD_SET_STATUS_PARAMETER_ERROR;
                }
                else
                {
                    if (sub_id == CAP_TOUCH_CMD_SET_CURRENT_ADC_REPORT)
                    {
                        if (!app_cap_touch_enable_adc_report(channel, enable))
                        {
                            ack_pkt[2] = CMD_SET_STATUS_PROCESS_FAIL;
                        }
                        else
                        {
                            ack_pkt[2] = CMD_SET_STATUS_COMPLETE;
                        }
                    }
                    else
                    {
                        if (!app_cap_touch_enable_adc_record(channel, enable))
                        {
                            ack_pkt[2] = CMD_SET_STATUS_PROCESS_FAIL;
                        }
                        else
                        {
                            ack_pkt[2] = CMD_SET_STATUS_COMPLETE;
                        }
                    }
                }
            }
            else
            {
                ack_pkt[2] = CMD_SET_STATUS_DISALLOW;
            }
            app_report_event(cmd_path, EVENT_ACK, app_idx, ack_pkt, 3);
        }
        break;
    }
}

static void app_cap_touch_bt_cback(T_BT_EVENT event_type, void *event_buf, uint16_t buf_len)
{
    T_BT_EVENT_PARAM *param = event_buf;

    bool handle = true;

    switch (event_type)
    {
    case BT_EVENT_SPP_DISCONN_CMPL:
        {
            app_cap_touch_reset_all_setting();
            app_cap_touch_reset_test_mode_setting();
        }
        break;
    default:
        handle = false;
        break;
    }
    if (handle)
    {
        APP_PRINT_TRACE1("app_cap_touch_bt_cback type 0x%x", event_type);
    }
}

static void app_cap_touch_reg_timer(void)
{
    gap_reg_timer_cb(app_cap_touch_timeout_cb, &cap_touch_timer_queue_id);
}

void app_cap_touch_init(bool first_power_on)
{
    /*uint32_t reg_data = 0;
    CTC_PRINT_TRACE1("app_cap_touch_init start reg_data %d", reg_data);
    reg_data = HAL_READ32(0x40007000UL, (0x0));
    CTC_PRINT_TRACE1("app_cap_touch_init get reg_data %d", reg_data);*/

    /*need cap touch mcu configure setting
    1.using channel
    2.isr
    3.scan interval:CTC_SLOW_MODE & CTC_FAST_MODE*/
    if (first_power_on)
    {
        bt_mgr_cback_register(app_cap_touch_bt_cback);
        app_cap_touch_cfg_init();
        app_cap_touch_reg_timer();
    }
    app_dlps_disable(APP_DLPS_ENTER_CHECK_GPIO);

    app_cap_touch_parameter_init();

    /* Cap Touch start */
    CapTouch_Cmd(ENABLE, DISABLE);
#if APP_CAP_TOUCH_DGB
    APP_PRINT_TRACE4("MBIAS %d,%d,%d,%d", app_cfg_const.ctc_chan_0_mbias,
                     app_cfg_const.ctc_chan_1_mbias, app_cfg_const.ctc_chan_2_mbias, app_cfg_const.ctc_chan_3_mbias);
    APP_PRINT_TRACE4("DiffThreshold %d,%d,%d,%d", app_cfg_const.ctc_chan_0_threshold,
                     app_cfg_const.ctc_chan_1_threshold,
                     app_cfg_const.ctc_chan_2_threshold_upper << 8 | app_cfg_const.ctc_chan_2_threshold_lower,
                     app_cfg_const.ctc_chan_3_threshold);
#endif
    CTC_PRINT_TRACE0("Cap Touch Enable!");

    gap_start_timer(&timer_handle_cap_touch_dlps_check, "cap_touch_dlps_check",
                    cap_touch_timer_queue_id, CAP_TOUCH_TIMER_CTC_DLPS, 0, 1000);
#if APP_CAP_TOUCH_DGB
    gap_start_timer(&timer_handle_cap_touch_channel_adc, "cap_touch_channel_adc",
                    cap_touch_timer_queue_id, CAP_TOUCH_TIMER_CTC_CHANNEL, 0, 100);
#endif
}

#endif

