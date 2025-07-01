/*
 * Copyright (c) 2018, Realsil Semiconductor Corporation. All rights reserved.
 */

#include "string.h"
#include "rtl876x_nvic.h"
#include "rtl876x_qdec.h"
#include "rtl876x_pinmux.h"
#include "rtl876x_rcc.h"
#include "gap_timer.h"
#include "section.h"
#include "app_dlps.h"
#include "app_msg.h"
#include "app_mmi.h"
#include "app_cfg.h"
#include "app_gpio.h"
#include "app_io_msg.h"
#include "trace.h"

#if (F_APP_QDECODE_SUPPORT == 1)

#define QDEC_Y_PHA_PIN app_cfg_const.qdec_y_pha_pinmux
#define QDEC_Y_PHB_PIN app_cfg_const.qdec_y_phb_pinmux

#define QDEC_WAKEUP_MAGIC_NUM   0xABAB

#define QDEC_BLOCK_DLPS_TIMER_MS    4000

typedef struct
{
    int16_t pre_ct;     //previous counter value
    int16_t cur_ct;     //current counter value
    uint16_t dir;        //1--up; 0-- down
} T_APP_QDEC;

T_APP_QDEC qdec_ctx;

static uint8_t app_qdec_timer_queue_id = 0;
static void *timer_handle_qdec = NULL;

static uint8_t qdecoder_a_status = 0;
static uint8_t qdecoder_b_status = 0;

static uint8_t pre_a_status = 0;
static uint8_t pre_b_status = 0;

static bool wakeup_2phase = false;

#if (F_APP_AVP_INIT_SUPPORT == 1)
bool (*app_qdec_direct_hook)(uint8_t) = NULL;
#endif

static void app_qdec_ctx_clear(void)
{
    memset(&qdec_ctx, 0, sizeof(T_APP_QDEC));
}

static void app_qdec_pinmux_config(void)
{
    Pinmux_Config(QDEC_Y_PHA_PIN, QDEC_PHASE_A_Y);
    Pinmux_Config(QDEC_Y_PHB_PIN, QDEC_PHASE_B_Y);
}

static void app_qdec_timeout_cb(uint8_t timer_id, uint16_t timer_chann)
{
    gap_stop_timer(&timer_handle_qdec);
    app_dlps_enable(APP_DLPS_ENTER_CHECK_QDEC);
    wakeup_2phase = false;
}

void app_qdec_pad_config(void)
{
    Pad_Config(QDEC_Y_PHA_PIN, PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_UP,
               PAD_OUT_DISABLE,
               PAD_OUT_LOW);
    Pad_Config(QDEC_Y_PHB_PIN, PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_UP,
               PAD_OUT_DISABLE,
               PAD_OUT_LOW);
}

void app_qdec_init_status_read(void)
{
    Pinmux_Config(QDEC_Y_PHA_PIN, DWGPIO);
    Pinmux_Config(QDEC_Y_PHB_PIN, DWGPIO);
    Pad_Config(QDEC_Y_PHA_PIN, PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_UP, PAD_OUT_DISABLE,
               PAD_OUT_LOW);
    Pad_Config(QDEC_Y_PHB_PIN, PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_UP, PAD_OUT_DISABLE,
               PAD_OUT_LOW);

    gpio_periphclk_config(QDEC_Y_PHA_PIN, ENABLE);
    gpio_periphclk_config(QDEC_Y_PHB_PIN, ENABLE);

    GPIO_InitTypeDef gpio_param;
    GPIO_StructInit(&gpio_param);
    gpio_param.GPIO_PinBit  = GPIO_GetPin(QDEC_Y_PHA_PIN);
    gpio_param.GPIO_Mode = GPIO_Mode_IN;
    gpio_param.GPIO_ITCmd = DISABLE;
    gpio_param_config(QDEC_Y_PHA_PIN, &gpio_param);

    GPIO_StructInit(&gpio_param);
    gpio_param.GPIO_PinBit  = GPIO_GetPin(QDEC_Y_PHB_PIN);
    gpio_param.GPIO_Mode = GPIO_Mode_IN;
    gpio_param.GPIO_ITCmd = DISABLE;
    gpio_param_config(QDEC_Y_PHB_PIN, &gpio_param);

    qdecoder_a_status = gpio_read_input_level(QDEC_Y_PHA_PIN);
    qdecoder_b_status = gpio_read_input_level(QDEC_Y_PHB_PIN);

    app_qdec_pinmux_config();
}

void app_qdec_driver_init(void)
{
    RCC_PeriphClockCmd(APBPeriph_QDEC, APBPeriph_QDEC_CLOCK, ENABLE);

    QDEC_InitTypeDef qdecInitStruct;
    QDEC_StructInit(&qdecInitStruct);
    qdecInitStruct.axisConfigY       = ENABLE;
    qdecInitStruct.debounceEnableY   = Debounce_Enable;
    qdecInitStruct.initPhaseY = (qdecoder_a_status << 1) | qdecoder_b_status;
    qdecInitStruct.counterScaleY = CounterScale_1_Phase;
    QDEC_Init(QDEC, &qdecInitStruct);
    QDEC_INTConfig(QDEC, QDEC_Y_INT_NEW_DATA, ENABLE);
    QDEC_INTMask(QDEC, QDEC_Y_ILLEAGE_INT_MASK, ENABLE);
    QDEC_Cmd(QDEC, QDEC_AXIS_Y, ENABLE);

    NVIC_InitTypeDef nvic_init_struct;
    nvic_init_struct.NVIC_IRQChannel         = QDEC_IRQn;
    nvic_init_struct.NVIC_IRQChannelCmd      = (FunctionalState)ENABLE;
    nvic_init_struct.NVIC_IRQChannelPriority = 3;
    NVIC_Init(&nvic_init_struct);

    app_qdec_ctx_clear();
}

void app_qdec_init(void)
{
    gap_reg_timer_cb(app_qdec_timeout_cb, &app_qdec_timer_queue_id);
}

void app_qdec_enter_power_down_cfg(void)
{
    RCC_PeriphClockCmd(APBPeriph_QDEC, APBPeriph_QDEC_CLOCK, DISABLE);
}

void app_qdec_pad_enter_dlps_config(void)
{
    app_qdec_init_status_read();

    pre_a_status = qdecoder_a_status;
    pre_b_status = qdecoder_b_status;

    Pad_Config(QDEC_Y_PHA_PIN, PAD_SW_MODE, PAD_IS_PWRON, PAD_PULL_UP, PAD_OUT_DISABLE, PAD_OUT_LOW);
    Pad_Config(QDEC_Y_PHB_PIN, PAD_SW_MODE, PAD_IS_PWRON, PAD_PULL_UP, PAD_OUT_DISABLE, PAD_OUT_LOW);

    if (qdecoder_a_status)
    {
        app_dlps_set_pad_wake_up(QDEC_Y_PHA_PIN, PAD_WAKEUP_POL_LOW);
    }
    else
    {
        app_dlps_set_pad_wake_up(QDEC_Y_PHA_PIN, PAD_WAKEUP_POL_HIGH);
    }

    if (qdecoder_b_status)
    {
        app_dlps_set_pad_wake_up(QDEC_Y_PHB_PIN, PAD_WAKEUP_POL_LOW);
    }
    else
    {
        app_dlps_set_pad_wake_up(QDEC_Y_PHB_PIN, PAD_WAKEUP_POL_HIGH);
    }

    //DBG_DIRECT("qdecoder_a_status %d  qdecoder_b_status %d  ", qdecoder_a_status,
    //           qdecoder_b_status);
}

void app_qdec_pad_exit_dlps_config(void)
{
    app_qdec_init_status_read();
    app_qdec_driver_init();
}

void app_qdec_wakeup_handle(void)
{
    app_qdec_init_status_read();

    if (pre_a_status != qdecoder_a_status ||
        pre_b_status != qdecoder_b_status)
    {
        if (pre_a_status != qdecoder_a_status &&
            pre_b_status != qdecoder_b_status)
        {
            wakeup_2phase = true;
            T_IO_MSG qdec_msg;
            qdec_msg.type = IO_MSG_TYPE_GPIO;
            qdec_msg.subtype = IO_MSG_GPIO_QDEC;
            qdec_msg.u.param = (qdec_ctx.dir << 16) | 1;
            if (app_io_send_msg(&qdec_msg) == false)
            {
                APP_PRINT_ERROR0("app_qdec_wakeup_handle: Send qdec msg error");
            }
        }
        else
        {
            //to start qdec timer outside
            wakeup_2phase = true;
            T_IO_MSG qdec_msg;
            qdec_msg.type = IO_MSG_TYPE_GPIO;
            qdec_msg.subtype = IO_MSG_GPIO_QDEC;
            qdec_msg.u.param = QDEC_WAKEUP_MAGIC_NUM;
            if (app_io_send_msg(&qdec_msg) == false)
            {
                APP_PRINT_ERROR0("app_qdec_wakeup_handle: Send qdec msg error");
            }
        }
    }
}

void app_qdec_msg_handler(T_IO_MSG *io_driver_msg_recv)
{
    uint16_t direction = io_driver_msg_recv->u.param >> 16;
    uint16_t delta = (uint16_t)(io_driver_msg_recv->u.param & 0xFFFF);
    bool vol_is_up = false;

    APP_PRINT_INFO2("app_qdec_msg_handler: delta 0x%x , direction %d", delta, direction);

#if (F_APP_AVP_INIT_SUPPORT == 1)
    if (app_qdec_direct_hook)
    {
        vol_is_up = app_qdec_direct_hook(direction);
    }
#else
    vol_is_up = (direction != 0) ? true : false;
#endif


    if (delta == QDEC_WAKEUP_MAGIC_NUM)
    {
        //only need start timer
    }
    else
    {
        if (vol_is_up)
        {
            //TODO: may need to add a counter threshold
            app_mmi_handle_action(MMI_DEV_SPK_VOL_UP);
        }
        else
        {
            app_mmi_handle_action(MMI_DEV_SPK_VOL_DOWN);
        }
    }
    app_dlps_disable(APP_DLPS_ENTER_CHECK_QDEC);
    gap_start_timer(&timer_handle_qdec, "qdec_dlps_timer", app_qdec_timer_queue_id, 0, 0,
                    QDEC_BLOCK_DLPS_TIMER_MS);
}

RAM_TEXT_SECTION void QDEC_Handler(void)
{
    if (QDEC_GetFlagState(QDEC, QDEC_FLAG_NEW_CT_STATUS_Y) == SET)
    {
        /* Mask qdec interrupt */
        QDEC_INTMask(QDEC, QDEC_Y_CT_INT_MASK, ENABLE);

        /* Read direction & count */
        qdec_ctx.dir = QDEC_GetAxisDirection(QDEC, QDEC_AXIS_Y);
        qdec_ctx.cur_ct = QDEC_GetAxisCount(QDEC, QDEC_AXIS_Y);

        T_IO_MSG qdec_msg;

        qdec_msg.type = IO_MSG_TYPE_GPIO;
        qdec_msg.subtype = IO_MSG_GPIO_QDEC;
        qdec_msg.u.param = (qdec_ctx.dir << 16) | 1;

        APP_PRINT_INFO3("QDEC_Handler: pre_ct %d , cur_ct %d  wakeup_2phase %d", qdec_ctx.pre_ct,
                        qdec_ctx.cur_ct, wakeup_2phase);

        if (qdec_ctx.pre_ct == 0)
        {
            if (wakeup_2phase == false)
            {
                if (app_io_send_msg(&qdec_msg) == false)
                {
                    APP_PRINT_ERROR0("QDEC_Handler: Send qdec msg error");
                }
                qdec_ctx.pre_ct = qdec_ctx.cur_ct;
            }
        }
        else if ((qdec_ctx.cur_ct - qdec_ctx.pre_ct >= 2) || (qdec_ctx.cur_ct - qdec_ctx.pre_ct <= -2))
        {
            if (app_io_send_msg(&qdec_msg) == false)
            {
                APP_PRINT_ERROR0("QDEC_Handler: Send qdec msg error");
            }
            qdec_ctx.pre_ct = qdec_ctx.cur_ct;
        }

        /* clear qdec interrupt flags */
        QDEC_ClearINTPendingBit(QDEC, QDEC_CLR_NEW_CT_Y);
        /* Unmask qdec interrupt */
        QDEC_INTMask(QDEC, QDEC_Y_CT_INT_MASK, DISABLE);

    }
    wakeup_2phase = false;
}
#endif
