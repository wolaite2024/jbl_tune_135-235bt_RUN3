/**
*********************************************************************************************************
*               Copyright(c) 2015, Realtek Semiconductor Corporation. All rights reserved.
**********************************************************************************************************
* @file     keyscan_demo.c
* @brief    This file provides demo code of keyscan working in active mode.
* @details
* @author   renee
* @date     2017-01-23
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
#include "os_timer.h"
#include "os_task.h"
#include "os_msg.h"
#include "string.h"
#include "trace.h"



/** @defgroup  KEYSCAN_DEMO  KEYSCAN DEMO
    * @brief  keyscan work in system active mode implementation demo code
    * @{
    */


/*============================================================================*
 *                         Types
 *============================================================================*/
/** @defgroup Keyscan_Demo_Exported_Types Keyscan Demo Exported Types
  * @brief
  * @{
  */

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
/** @} */ /* End of group Keyscan_Demo_Exported_Types */


/*============================================================================*
 *                              Macros
 *============================================================================*/
/** @defgroup Keyscan_Demo_Exported_Macros Keyscan Demo Exported Macros
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

/* task handle & queue handle */
void *iodemo_app_task_handle;
void *io_queue_handle;

/* To record filter data */
uint8_t RepeatReport = 0;

KeyScanDataStruct  CurKeyData;
KeyScanDataStruct  PreKeyData;

/* event */
#define IO_DEMO_EVENT_KEYSCAN_SCAN_END            0x01
#define IO_DEMO_EVENT_KEYSCAN_ALL_RELEASE         0x02


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
    KeyScan_InitStruct.scanInterval    = 0x80;
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
}

void keyscan_demo_code(void)
{
    /* Initialize pinmux & pad */
    board_keyscan_init();

    /* KeyScan Peripheral configuration */
    driver_keyscan_init();
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
                APP_PRINT_INFO0("All key release");
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
            /* Read fifo data */
            uint8_t len = KeyScan_GetFifoDataNum(KEYSCAN);
            KeyScan_Read(KEYSCAN, &pKeyData->key[0], len);
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

UNMASK_INT:
        /* clear & Unmask keyscan interrupt */
        KeyScan_ClearINTPendingBit(KEYSCAN, KEYSCAN_INT_SCAN_END);
        KeyScan_INTMask(KEYSCAN, KEYSCAN_INT_SCAN_END, DISABLE);
    }
}

/** @} */ /* End of group Keyscan_Demo_Exported_Functions */
/** @} */ /* End of group KEYSCAN_DEMO */

