/**
*********************************************************************************************************
*               Copyright(c) 2015, Realtek Semiconductor Corporation. All rights reserved.
**********************************************************************************************************
* @file     keyscan_demo_dlps.c
* @brief    This file provides demo code of keyscan working in dlps mode.
* @details
* @author   renee
* @date     2017-02-22
* @version  v1.0
*********************************************************************************************************
*/

/*============================================================================*
 *                              Header Files
 *============================================================================*/
#include "rtl876x_nvic.h"
#include "rtl876x_keyscan.h"
#include "rtl876x_pinmux.h"
#include "rtl876x_rcc.h"
#include "trace.h"
#include "rtl876x_io_dlps.h"
#include "dlps_platform.h"
#include "os_timer.h"
#include "os_task.h"
#include "os_msg.h"
#include "string.h"
#include "trace.h"

/** @defgroup  KEYSCAN_DEMO_DLPS  KEYSCAN DEMO DLPS
    * @brief  keyscan work in system dlps mode implementation demo code
    * @{
    */

/** @defgroup Keyscan_Dlps_Exported_Functions Keyscan Dlps Exported Functions
  * @brief
  * @{
  */
/*============================================================================*
 *                         Types
 *============================================================================*/
typedef struct
{
    uint8_t Length;            /**< Keyscan state register */
    uint16_t key[10];
} KeyScanDataStruct, *pKeyScanDataStruct;

/* keyscan message type */
typedef struct
{
    uint16_t msgType;
    union
    {
        uint32_t parm;
        void *pBuf;
    };
} KeyscanMsg;
/** @} */ /* End of group Keyscan_Dlps_Exported_Types */

/*============================================================================*
 *                              Macros
 *============================================================================*/
/** @defgroup Keyscan_Dlps_Exported_Macros Keyscan Dlps Exported Macros
  * @brief
  * @{
  */

/* keypad row and column */
#define KEYPAD_ROW_SIZE       2
#define KEYPAD_COLUMN_SIZE    2
#define COLUMN0               ADC_0
#define COLUMN1               ADC_1
#define ROW0                  ADC_2
#define ROW1                  ADC_3

/* queue size & stak size & task priority */
#define IO_DEMO_EVENT_QUEUE_SIZE        0x10
#define IO_DEMO_MESSAGE_QUEUE_SIZE      0x10
#define IO_DEMO_TASK_STACK_SIZE         1024
#define IO_DEMO_TASK_PRIORITY           (tskIDLE_PRIORITY + 1)

/* event */
#define IO_DEMO_EVENT_KEYSCAN_SCAN_END            0x01
#define IO_DEMO_EVENT_KEYSCAN_ALL_RELEASE         0x02

/** @} */ /* End of group Keyscan_Dlps_Exported_Macros */

/*============================================================================*
 *                              Variables
 *============================================================================*/
/** @defgroup Keyscan_Dlps_Exported_Variables Keyscan Dlps Exported Variables
  * @brief
  * @{
  */

/* task handle & queue handle */
void *iodemo_app_task_handle;
void *io_queue_handle;

/* To record filter data */
uint8_t allreleaseflag = 1;
uint8_t RepeatReport = 0;

KeyScanDataStruct  CurKeyData;
KeyScanDataStruct  PreKeyData;





void *keyscan_timer_handle;

/* To keep System Active */
uint8_t keyscanallowdlps = true;
uint8_t allreleaseFlag = true;

/** @} */ /* End of group Keyscan_Dlps_Exported_Variables */

/*============================================================================*
 *                              Functions
 *============================================================================*/
/** @defgroup Keyscan_Dlps_Exported_Functions Keyscan Dlps Exported Functions
  * @brief
  * @{
  */
void io_demo_task(void *param);

/**
  * @brief  initialization of pinmux settings and pad settings.
  * @param   No parameter.
  * @return  void
  */
void board_keyscan_init(void)
{
    Pad_Config(COLUMN0, PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_NONE, PAD_OUT_ENABLE, PAD_OUT_LOW);
    Pad_Config(COLUMN1, PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_NONE, PAD_OUT_ENABLE, PAD_OUT_LOW);
    Pad_Config(ROW0, PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_UP, PAD_OUT_DISABLE, PAD_OUT_LOW);
    Pad_Config(ROW1, PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_UP, PAD_OUT_DISABLE, PAD_OUT_LOW);

    Pinmux_Config(COLUMN0, KEY_COL_0);
    Pinmux_Config(COLUMN1, KEY_COL_1);
    Pinmux_Config(ROW0, KEY_ROW_0);
    Pinmux_Config(ROW1, KEY_ROW_1);
}

/**
  * @brief  Initialize Keyscan peripheral.
  * @param   No parameter.
  * @return  void
  */
void driver_keyscan_init(void)
{
    /* data struct init */
    memset(&CurKeyData, 0, sizeof(KeyScanDataStruct));
    memset(&PreKeyData, 0, sizeof(KeyScanDataStruct));

    RepeatReport = false;

    RCC_PeriphClockCmd(APBPeriph_KEYSCAN, APBPeriph_KEYSCAN_CLOCK, ENABLE);
    KEYSCAN_InitTypeDef  KeyScan_InitStruct;
    KeyScan_StructInit(KEYSCAN, &KeyScan_InitStruct);
    KeyScan_InitStruct.colSize         = KEYPAD_COLUMN_SIZE;
    KeyScan_InitStruct.rowSize         = KEYPAD_ROW_SIZE;
    KeyScan_InitStruct.scanInterval    = 0xFA;      //50 ms
    KeyScan_InitStruct.debounceEn      = KeyScan_Debounce_Disable;
    KeyScan_InitStruct.clockdiv        = 0x3E8;
    KeyScan_InitStruct.releasecnt      = 0x01;
    KeyScan_Init(KEYSCAN, &KeyScan_InitStruct);
    KeyScan_INTConfig(KEYSCAN, KEYSCAN_INT_SCAN_END | KEYSCAN_INT_ALL_RELEASE, ENABLE);

    NVIC_InitTypeDef nvic_init_struct;
    nvic_init_struct.NVIC_IRQChannel         = KeyScan_IRQn;
    nvic_init_struct.NVIC_IRQChannelCmd      = (FunctionalState)ENABLE;
    nvic_init_struct.NVIC_IRQChannelPriority = 3;
    NVIC_Init(&nvic_init_struct);

    KeyScan_Cmd(KEYSCAN, ENABLE);
}

/**
  * @brief  IO Enter dlps call back function.
  * @param   No parameter.
  * @return  void
  */
void io_dlps_enter(void)
{
    /* switch pad to Software mode */
    Pad_ControlSelectValue(COLUMN0, PAD_SW_MODE);
    Pad_ControlSelectValue(COLUMN1, PAD_SW_MODE);
    Pad_ControlSelectValue(ROW0, PAD_SW_MODE);
    Pad_ControlSelectValue(ROW1, PAD_SW_MODE);
    allreleaseFlag = true;

    /* To Debug */
//    DBG_DIRECT("[DLPS] enter");
}

/**
  * @brief  IO Exit dlps call back function.
  * @param   No parameter.
  * @return  void
  */
void io_dlps_exit(void)
{
    /* switch pad to Software mode */
    Pad_ControlSelectValue(COLUMN0, PAD_PINMUX_MODE);
    Pad_ControlSelectValue(COLUMN1, PAD_PINMUX_MODE);
    Pad_ControlSelectValue(ROW0, PAD_PINMUX_MODE);
    Pad_ControlSelectValue(ROW1, PAD_PINMUX_MODE);

    /* To Debug */
//    DBG_DIRECT("[DLPS] exit");
}

/**
  * @brief  IO Enter dlps check function.
  * @param   No parameter.
  * @return  void
  */
uint8_t io_dlps_check(void)
{
    return keyscanallowdlps;
}

/**
  * @brief  IO Enter dlps check function.
  * @param   No parameter.
  * @return  void
  */
void power_keyscan_init(void)
{
    if (false == dlps_check_cb_reg((DLPSEnterCheckFunc)io_dlps_check))
    {
        APP_PRINT_ERROR0("dlps_check_cb_reg(io_dlps_check) failed!!");
    }
    DLPS_IORegUserDlpsExitCb(io_dlps_exit);
    DLPS_IORegUserDlpsEnterCb(io_dlps_enter);
    DLPS_IORegister();

    lps_mode_set(LPM_DLPS_RET_MODE);

    /* Wake up pin config */
    System_WakeUpPinEnable(ROW0, PAD_WAKEUP_POL_LOW);
    System_WakeUpPinEnable(ROW1, PAD_WAKEUP_POL_LOW);

    System_WakeUpInterruptEnable(ROW0);
    System_WakeUpInterruptEnable(ROW1);
}

void keyscan_release_handle(void)
{
    System_WakeUpPinEnable(ROW0, PAD_WAKEUP_POL_LOW);
    System_WakeUpPinEnable(ROW1, PAD_WAKEUP_POL_LOW);
    APP_PRINT_INFO0("All key release");
    memset(&CurKeyData, 0, sizeof(KeyScanDataStruct));
    memset(&PreKeyData, 0, sizeof(KeyScanDataStruct));
}

void keyscan_timer_handler(void *pxTimer)
{
    if (allreleaseFlag)
    {
        keyscan_release_handle();
    }
    else
    {
        os_timer_restart(&keyscan_timer_handle, 70);
    }

    allreleaseFlag = true;
}

/**
  * @brief  Initialize peripheral Task and IO message queue.
  * @param   No parameter.
  * @return  void
  */
void peripheral_task_init(void)
{
    /* create io test task */
    os_task_create(&iodemo_app_task_handle, "app", io_demo_task, NULL, 384 * 4, 2);

    /* create event queue and message queue */
    os_msg_queue_create(&io_queue_handle, IO_DEMO_EVENT_QUEUE_SIZE, sizeof(KeyScanDataStruct));

    os_timer_create(&keyscan_timer_handle, "keyscan_timer", 2, 70, false, keyscan_timer_handler);
}

/**
* @brief  System interrupt handler function, For Wakeup pin.
* @param   No parameter.
* @return  void
*/
void System_Handler(void)
{
    if (System_WakeUpInterruptValue(ROW0) == SET)
    {
        Pad_ClearWakeupINTPendingBit(ROW0);
        keyscanallowdlps = false;
        /* Wake up pin config */
        System_WakeUpPinDisable(ROW0);
    }
    if (System_WakeUpInterruptValue(ROW1) == SET)
    {
        Pad_ClearWakeupINTPendingBit(ROW1);
        keyscanallowdlps = false;
        System_WakeUpPinDisable(ROW1);
    }
    NVIC_DisableIRQ(System_IRQn);
    NVIC_ClearPendingIRQ(System_IRQn);
}



/**
  * @brief  IO_Demo Task Handle.
  * @param   No parameter.
  * @return  void
  */
void io_demo_task(void *param)
{
    KeyscanMsg msg;

    /* Initialize pinmux & pad */
    board_keyscan_init();

    /* KeyScan Peripheral configuration */
    driver_keyscan_init();

    /* power setting */
    power_keyscan_init();

    while (1)
    {
        if (os_msg_recv(io_queue_handle, &msg, 0xFFFFFFFF) == true)
        {
            if (msg.msgType == IO_DEMO_EVENT_KEYSCAN_SCAN_END)
            {
                pKeyScanDataStruct pKeyData = (pKeyScanDataStruct)(msg.pBuf);
                for (uint8_t i = 0; i < pKeyData->Length; i++)
                {
                    APP_PRINT_INFO2("pKeyData->key[%d] = %x", i, pKeyData->key[i]);
                }


            }
            else if (msg.msgType == IO_DEMO_EVENT_KEYSCAN_ALL_RELEASE)
            {
                keyscan_release_handle();
            }
        }
    }
}

/**
* @brief  Keyscan interrupt handler function.
* @param   No parameter.
* @return  void
*/
void KeyScan_Handler(void)
{
    keyscanallowdlps = true;
    pKeyScanDataStruct pKeyData = &CurKeyData;
    KeyscanMsg msg;

    if (KeyScan_GetFlagState(KEYSCAN, KEYSCAN_INT_FLAG_ALL_RELEASE) == SET)   //all release
    {
        /* Mask keyscan interrupt */
        KeyScan_INTMask(KEYSCAN, KEYSCAN_INT_ALL_RELEASE, ENABLE);

        msg.msgType = IO_DEMO_EVENT_KEYSCAN_ALL_RELEASE;
        if (os_msg_send(io_queue_handle, &msg, 0) == false)
        {
            APP_PRINT_ERROR0("Send queue DemoIOEventQueue fail");
        }
        /* clear & Unmask keyscan interrupt */
        KeyScan_ClearINTPendingBit(KEYSCAN, KEYSCAN_INT_ALL_RELEASE);
        KeyScan_INTMask(KEYSCAN, KEYSCAN_INT_ALL_RELEASE, DISABLE);
    }
    if (KeyScan_GetFlagState(KEYSCAN, KEYSCAN_INT_FLAG_SCAN_END) == SET)   // scan finish
    {
        /* Mask keyscan interrupt */
        KeyScan_INTMask(KEYSCAN, KEYSCAN_INT_SCAN_END, ENABLE);

        /* KeyScan fifo not empty */
        if (KeyScan_GetFlagState(KEYSCAN, KEYSCAN_FLAG_EMPTY) != SET)
        {
            allreleaseFlag = false;

            os_timer_restart(&keyscan_timer_handle, 70);

            /* Read fifo data */
            uint8_t len = KeyScan_GetFifoDataNum(KEYSCAN);
            KeyScan_Read(KEYSCAN, (uint16_t *)(&pKeyData->key[0]), len);
            pKeyData->Length = len;

            if (!RepeatReport)
            {
                if (!memcmp(pKeyData, &PreKeyData, sizeof(KeyScanDataStruct)))
                {
                    goto UNMASK_INT;
                }
                else
                {
                    memcpy(&PreKeyData, pKeyData, sizeof(KeyScanDataStruct));
                }
            }

            msg.msgType = IO_DEMO_EVENT_KEYSCAN_SCAN_END;
            msg.pBuf = (void *)pKeyData;
            if (os_msg_send(io_queue_handle, &msg, 0) == false)
            {
                APP_PRINT_ERROR0("Send queue DemoIOEventQueue fail");
            }
        }
        else
        {
            allreleaseFlag = true;
            os_timer_stop(&keyscan_timer_handle);
        }

UNMASK_INT:
        /* clear & Unmask keyscan interrupt */
        KeyScan_ClearINTPendingBit(KEYSCAN, KEYSCAN_INT_SCAN_END);
        KeyScan_INTMask(KEYSCAN, KEYSCAN_INT_SCAN_END, DISABLE);
    }
}

/** @} */ /* End of group Keyscan_Dlps_Exported_Functions */
/** @} */ /* End of group KEYSCAN_DEMO_DLPS */

