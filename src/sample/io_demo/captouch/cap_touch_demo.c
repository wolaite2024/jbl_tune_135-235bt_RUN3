/**
************************************************************************************************************
*            Copyright(c) 2020, Realtek Semiconductor Corporation. All rights reserved.
************************************************************************************************************
* @file      cap_touch_demo.c
* @brief     Cap Touch Demonstration.
* @author    js.lin
* @date      2020-11-24
* @version   v1.0
*************************************************************************************************************
*/

#include "trace.h"
#include "vector_table.h"
#include "rtl876x_captouch.h"

/**
  * @brief  CapTouch Interrupt handler.
  * @retval None
  */
void CapTouch_Handler(void)
{
    uint32_t int_status = 0;
    int_status = CapTouch_GetINTStatus();
    DBG_DIRECT("[CTC] 0x%08x, ch0: [%4d, %5d, %4d], ch1[%4d, %5d,  %4d], ch2[%4d, %5d, %4d], ch3[%4d, %5d, %4d], fs:%d, touch:%d * interrupt!",
               int_status,
               CapTouch_GetChAveData(CTC_CH0),
               CapTouch_GetChAveData(CTC_CH0) - CapTouch_GetChBaseline(CTC_CH0),
               CapTouch_GetChTouchCnt(CTC_CH0),
               CapTouch_GetChAveData(CTC_CH1),
               CapTouch_GetChAveData(CTC_CH1) - CapTouch_GetChBaseline(CTC_CH1),
               CapTouch_GetChTouchCnt(CTC_CH1),
               CapTouch_GetChAveData(CTC_CH2),
               CapTouch_GetChAveData(CTC_CH2) - CapTouch_GetChBaseline(CTC_CH2),
               CapTouch_GetChTouchCnt(CTC_CH2),
               CapTouch_GetChAveData(CTC_CH3),
               CapTouch_GetChAveData(CTC_CH3) - CapTouch_GetChBaseline(CTC_CH3),
               CapTouch_GetChTouchCnt(CTC_CH3),
               CapTouch_IsFastMode(),
               CapTouch_GetChTouchStatus());
    /* Channel 0 interrupts */
    if (CapTouch_IsChINTTriggered(int_status, CTC_CH0, CTC_TOUCH_PRESS_INT))
    {
        /* do something */
        CapTouch_ChINTClearPendingBit(CTC_CH0, CTC_TOUCH_PRESS_INT);
    }
    if (CapTouch_IsChINTTriggered(int_status, CTC_CH0, CTC_TOUCH_RELEASE_INT))
    {
        /* do something */
        CapTouch_ChINTClearPendingBit(CTC_CH0, CTC_TOUCH_RELEASE_INT);
    }
    if (CapTouch_IsChINTTriggered(int_status, CTC_CH0, CTC_FALSE_TOUCH_INT))
    {
        /* do something */
        CapTouch_ChINTClearPendingBit(CTC_CH0, CTC_FALSE_TOUCH_INT);
    }
    /* Channel 1 interrupts */
    if (CapTouch_IsChINTTriggered(int_status, CTC_CH1, CTC_TOUCH_PRESS_INT))
    {
        /* do something */
        CapTouch_ChINTClearPendingBit(CTC_CH1, CTC_TOUCH_PRESS_INT);
    }
    if (CapTouch_IsChINTTriggered(int_status, CTC_CH1, CTC_TOUCH_RELEASE_INT))
    {
        /* do something */
        CapTouch_ChINTClearPendingBit(CTC_CH1, CTC_TOUCH_RELEASE_INT);
    }
    if (CapTouch_IsChINTTriggered(int_status, CTC_CH1, CTC_FALSE_TOUCH_INT))
    {
        /* do something */
        CapTouch_ChINTClearPendingBit(CTC_CH1, CTC_FALSE_TOUCH_INT);
    }
    /* Channel 2 interrupts */
    if (CapTouch_IsChINTTriggered(int_status, CTC_CH2, CTC_TOUCH_PRESS_INT))
    {
        /* do something */
        CapTouch_ChINTClearPendingBit(CTC_CH2, CTC_TOUCH_PRESS_INT);
    }
    if (CapTouch_IsChINTTriggered(int_status, CTC_CH2, CTC_TOUCH_RELEASE_INT))
    {
        /* do something */
        CapTouch_ChINTClearPendingBit(CTC_CH2, CTC_TOUCH_RELEASE_INT);
    }
    if (CapTouch_IsChINTTriggered(int_status, CTC_CH2, CTC_FALSE_TOUCH_INT))
    {
        /* do something */
        CapTouch_ChINTClearPendingBit(CTC_CH2, CTC_FALSE_TOUCH_INT);
    }
    /* Channel 3 interrupts */
    if (CapTouch_IsChINTTriggered(int_status, CTC_CH3, CTC_TOUCH_PRESS_INT))
    {
        /* do something */
        CapTouch_ChINTClearPendingBit(CTC_CH3, CTC_TOUCH_PRESS_INT);
    }
    if (CapTouch_IsChINTTriggered(int_status, CTC_CH3, CTC_TOUCH_RELEASE_INT))
    {
        /* do something */
        CapTouch_ChINTClearPendingBit(CTC_CH3, CTC_TOUCH_RELEASE_INT);
    }
    if (CapTouch_IsChINTTriggered(int_status, CTC_CH3, CTC_FALSE_TOUCH_INT))
    {
        /* do something */
        CapTouch_ChINTClearPendingBit(CTC_CH3, CTC_FALSE_TOUCH_INT);
    }
    /* Noise Interrupt */
    if (CapTouch_IsNoiseINTTriggered(int_status, CTC_OVER_N_NOISE_INT))
    {
        /* do something */
        CapTouch_NoiseINTClearPendingBit(CTC_OVER_N_NOISE_INT);
    }
    if (CapTouch_IsNoiseINTTriggered(int_status, CTC_OVER_P_NOISE_INT))
    {
        CTC_PRINT_ERROR0("Baseline Error! Need to reset Cap-touch system!");
        CapTouch_NoiseINTClearPendingBit(CTC_OVER_P_NOISE_INT);
    }

}

void CapTouch_Demo(void)
{
    CTC_PRINT_INFO0("Demo Start...");

    /* Register new interrupt handler to vector table */
    RamVectorTableUpdate(TOUCH_VECTORn, (IRQ_Fun)CapTouch_Handler);
    CTC_PRINT_INFO0("Update CTC hanlder done.");

    /* Channel enable */
    CapTouch_ChCmd(CTC_CH0, ENABLE);
    CapTouch_ChCmd(CTC_CH1, ENABLE);
    CapTouch_ChCmd(CTC_CH2, ENABLE);
    CapTouch_ChCmd(CTC_CH3, ENABLE);
    CTC_PRINT_INFO0(" Channel enable done.");

    /* Interrupt enable */
    CapTouch_ChINTConfig(CTC_CH0, (CTC_CH_INT_TYPE)(CTC_TOUCH_PRESS_INT | CTC_TOUCH_RELEASE_INT |
                                                    CTC_FALSE_TOUCH_INT), ENABLE);
    CapTouch_ChINTConfig(CTC_CH1, (CTC_CH_INT_TYPE)(CTC_TOUCH_PRESS_INT | CTC_TOUCH_RELEASE_INT |
                                                    CTC_FALSE_TOUCH_INT), ENABLE);
    CapTouch_ChINTConfig(CTC_CH2, (CTC_CH_INT_TYPE)(CTC_TOUCH_PRESS_INT | CTC_TOUCH_RELEASE_INT |
                                                    CTC_FALSE_TOUCH_INT), ENABLE);
    CapTouch_ChINTConfig(CTC_CH3, (CTC_CH_INT_TYPE)(CTC_TOUCH_PRESS_INT | CTC_TOUCH_RELEASE_INT |
                                                    CTC_FALSE_TOUCH_INT), ENABLE);
    CapTouch_NoiseINTConfig(CTC_OVER_P_NOISE_INT, ENABLE);
    CTC_PRINT_INFO0("Interrupt enable done.");

    /* Set scan interval */
    if (!CapTouch_SetScanInterval(0x3B, CTC_SLOW_MODE))
    {
        CTC_PRINT_WARN0("Slow mode scan interval overange!");
    }
    if (!CapTouch_SetScanInterval(0x1D, CTC_FAST_MODE))
    {
        CTC_PRINT_WARN0("Fast mode scan interval overange!");
    }
    CTC_PRINT_INFO0("Set scan interval done.");

    /* Enable touch wakeup from LPS, DLPS, PowerDown */
    CapTouch_ChWakeupCmd(CTC_CH0, (FunctionalState)ENABLE);
    CapTouch_ChWakeupCmd(CTC_CH1, (FunctionalState)ENABLE);
    CapTouch_ChWakeupCmd(CTC_CH2, (FunctionalState)ENABLE);
    CapTouch_ChWakeupCmd(CTC_CH3, (FunctionalState)ENABLE);

    /* Cap Touch start */
    CapTouch_Cmd(ENABLE, DISABLE);
    CTC_PRINT_INFO0("Cap Touch Enable!");
}

