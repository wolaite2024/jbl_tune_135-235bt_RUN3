/**
*********************************************************************************************************
*               Copyright(c) 2015, Realtek Semiconductor Corporation. All rights reserved.
**********************************************************************************************************
* @file     dlps_uart_demo.c
* @brief    This file provides demo code of uart work in dlps mode.
* @details
* @author   renee
* @date     2017-02-27
* @version  v1.0
*********************************************************************************************************
*/

/*============================================================================*
 *                              Header Files
 *============================================================================*/
#include "os_timer.h"
#include "os_task.h"
#include "os_msg.h"
#include "rtl876x_pinmux.h"
#include "rtl876x_nvic.h"
#include "rtl876x_ir.h"
#include "rtl876x_uart.h"
#include "rtl876x_rcc.h"
#include "string.h"
#include "trace.h"
#include "rtl876x_tim.h"
#include "pm.h"
#include "io_dlps.h"
#include "os_sched.h"

/** @defgroup  DLPS_UART_DEMO  UART DEMO DLPS
    * @brief  Uart work in system dlps mode implementation demo code
    * @{
    */

/*============================================================================*
 *                              Macros
 *============================================================================*/
/** @defgroup UART_Dlps_Demo_Exported_Macros UART Dlps Demo Exported Macros
  * @brief
  * @{
  */
#define UART_TX_PIN             P3_1
#define UART_RX_PIN             P3_0

/* queue size & stak size & task priority */
#define IO_DEMO_EVENT_QUEUE_SIZE                    0x10
#define IO_DEMO_MESSAGE_QUEUE_SIZE                  0x10
#define IO_DEMO_TASK_STACK_SIZE                     1024
#define IO_DEMO_TASK_PRIORITY                       (tskIDLE_PRIORITY + 1)

#define IR_SEND_TASK_STACK_SIZE                     1024
#define IR_SEND_TASK_PRIORITY                       (tskIDLE_PRIORITY + 1)

#define IO_DEMO_EVENT_UART_RX                       0x01
#define IO_DEMO_EVENT_IR_TX_DONE                    0x02

/** @} */ /* End of group UART_Dlps_Demo_Exported_Macros */

/*============================================================================*
 *                              Variables
 *============================================================================*/
/** @defgroup UART_Dlps_Demo_Exported_Variables UART Dlps Demo Exported Variables
  * @brief
  * @{
  */
/* task handle & queue handle */
void *evt_queue_handle;
void *msg_queue_handle;
void *io_queue_handle;

void *iodemo_app_task_handle;




uint8_t RxBuffer[600];
uint32_t RxCount = 0;
/* global */
uint8_t  gKeepActiveCounter = 0;
bool  allowedSystemEnterDlps = true;

void *uart_dlps_timer_handle = NULL;

/** @} */ /* End of group UART_Dlps_Demo_Exported_Variables */

/*============================================================================*
 *                              Functions
 *============================================================================*/
/** @defgroup UART_Dlps_Demo_Exported_Functions UART Dlps Demo Exported Functions
  * @brief
  * @{
  */

void io_demo_task(void *param);
extern uint32_t vPortGetIPSR(void);
extern void test_gpio_interrupt(void);


/*============================================================================*
 *                         Public   Functions
 *============================================================================*/
/**
  * @brief  Initialize peripheral Task and IO message queue, call this function to demonstrate
  *         uart functions in dlps mode.
  * @param   No parameter.
  * @return  void
  */
void peripheral_task_init(void)
{
    os_task_create(&iodemo_app_task_handle, "app", io_demo_task, NULL, 384 * 4, 2);

    os_msg_queue_create(&io_queue_handle, IO_DEMO_EVENT_QUEUE_SIZE, sizeof(uint8_t));

}

/*============================================================================*
 *                         Private   Functions
 *============================================================================*/

/**
  * @brief  initialization of pinmux settings and pad settings.
  * @param   No parameter.
  * @return  void
  */
void board_uart_init(void)
{
    Pad_Config(UART_TX_PIN, PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_NONE, PAD_OUT_ENABLE, PAD_OUT_HIGH);
    Pad_Config(UART_RX_PIN, PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_UP, PAD_OUT_DISABLE, PAD_OUT_LOW);

    Pinmux_Config(UART_TX_PIN, UART2_TX);
    Pinmux_Config(UART_RX_PIN, UART2_RX);
}

/**
  * @brief  Initialize UART peripheral.
  * @param   No parameter.
  * @return  void
  */
void driver_uart_init(void)
{
    RCC_PeriphClockCmd(APBPeriph_UART2, APBPeriph_UART2_CLOCK, ENABLE);
    /* uart init */
    UART_InitTypeDef uartInitStruct;
    UART_StructInit(&uartInitStruct);

    /* default baudrate is 115200 */
    uartInitStruct.dmaEn = UART_DMA_ENABLE;
    UART_Init(UART2, &uartInitStruct);

    //enable rx interrupt and line status interrupt
    UART_INTConfig(UART2, UART_INT_RD_AVA | UART_INT_IDLE, ENABLE);

    /*  Enable UART IRQ  */
    NVIC_InitTypeDef nvic_init_struct;
    nvic_init_struct.NVIC_IRQChannel         = UART2_IRQn;
    nvic_init_struct.NVIC_IRQChannelCmd      = (FunctionalState)ENABLE;
    nvic_init_struct.NVIC_IRQChannelPriority = 3;
    NVIC_Init(&nvic_init_struct);
}

/**
  * @brief  uart send string buffer.
  * @param   No parameter.
  * @return  void
  */
static void uart_send_str(char *str, uint16_t str_len)
{
    uint8_t blk, remain, i;
    blk = str_len / UART_TX_FIFO_SIZE;
    remain = str_len % UART_TX_FIFO_SIZE;

    //send through uart
    for (i = 0; i < blk; i++)
    {
        UART_SendData(UART2, (uint8_t *)&str[16 * i], 16);
        while (UART_GetFlagState(UART2, UART_FLAG_THR_EMPTY) != SET);
    }

    UART_SendData(UART2, (uint8_t *)&str[16 * i], remain);
    while (UART_GetFlagState(UART2, UART_FLAG_THR_EMPTY) != SET);
}

/**
  * @brief  IO Enter dlps call back function.
  * @param   No parameter.
  * @return  void
  */
void uart_dlps_enter(void)
{
    /* switch pad to Software mode */
    Pad_ControlSelectValue(UART_TX_PIN, PAD_SW_MODE);
    Pad_ControlSelectValue(UART_RX_PIN, PAD_SW_MODE);

    System_WakeUpInterruptEnable(UART_RX_PIN);
    System_WakeUpPinEnable(UART_RX_PIN, PAD_WAKEUP_POL_LOW);
    /* To Debug */
    DBG_DIRECT("[DLPS] io_dlps_enter");
}

/**
  * @brief  IO Exit dlps call back function.
  * @param   No parameter.
  * @return  void
  */
void uart_dlps_exit(void)
{
    /* switch pad to Pinmux mode */
    Pad_ControlSelectValue(UART_TX_PIN, PAD_PINMUX_MODE);
    Pad_ControlSelectValue(UART_RX_PIN, PAD_PINMUX_MODE);

    DBG_DIRECT("io_dlps_exit");
}

/**
  * @brief  IO Enter dlps check function.
  * @param   No parameter.
  * @return  void
  */
bool io_dlps_check(void)
{

    return allowedSystemEnterDlps;
}

/**
  * @brief  IO Enter dlps check function.
  * @param   No parameter.
  * @return  void
  */
void power_uart_init(void)
{
    power_check_cb_register(io_dlps_check);
    DLPS_IORegister();

    power_stage_cb_register(uart_dlps_enter, POWER_STAGE_STORE);
    power_stage_cb_register(uart_dlps_exit, POWER_STAGE_RESTORE);

    bt_power_mode_set(BTPOWER_DEEP_SLEEP);
    power_mode_set(POWER_DLPS_MODE);

    /* Config WakeUp pin */
    System_WakeUpPinEnable(UART_RX_PIN, PAD_WAKEUP_POL_LOW);
}

/**
  * @brief  IO_Demo Task Handle.
  * @param   No parameter.
  * @return  void
  */
void io_demo_task(void *param)
{
    uint8_t event = 0;
    uint8_t strLen = 0;
    uint16_t index = 0;

    /* Pinmux & Pad Config */
    board_uart_init();

    /* Initialize UART peripheral */
    driver_uart_init();

    /* Power Setting */
    power_uart_init();


    /* Send demo buffer */
    char *demoStr = "### Welcome to use RealTek Bumblebee ###\r\n";
    strLen = strlen(demoStr);
    uart_send_str(demoStr, strLen);

    while (1)
    {
        if (os_msg_recv(io_queue_handle, &event, 0xFFFFFFFF) == true)
        {
            if (event == IO_DEMO_EVENT_UART_RX)
            {
                uart_send_str((char *)RxBuffer, RxCount);

                for (index = 0; index < 500; index++)
                {
                    RxBuffer[index] = 0;
                }

                RxCount = 0;
            }

        }
    }
}

/**
* @brief  Data Uart interrupt handler function.
* @param   No parameter.
* @return  void
*/
void UART2_Handler(void)
{
    uint8_t event = IO_DEMO_EVENT_UART_RX;
    uint32_t int_status = 0;
    uint8_t rxfifocnt = 0;
    int_status = UART_GetIID(UART2);

    if (UART_GetFlagState(UART2, UART_FLAG_RX_IDLE) == SET)
    {
        //clear Flag
        UART_INTConfig(UART2, UART_INT_IDLE, DISABLE);
        if (os_msg_send(io_queue_handle, &event, 0) == false)
        {
            APP_PRINT_ERROR0("Send queue error!!!!");
        }
        allowedSystemEnterDlps = true;
        UART_INTConfig(UART2, UART_INT_IDLE, ENABLE);
    }

    switch (int_status)
    {
    /* tx fifo empty, not enable */
    case UART_INT_ID_TX_EMPTY:
        break;

    /* rx data valiable */
    case UART_INT_ID_RX_LEVEL_REACH:
        rxfifocnt = UART_GetRxFIFOLen(UART2);
        UART_ReceiveData(UART2, &RxBuffer[RxCount], rxfifocnt);
        RxCount += rxfifocnt;
        break;

    case UART_INT_ID_RX_TMEOUT:
        rxfifocnt = UART_GetRxFIFOLen(UART2);
        UART_ReceiveData(UART2, &RxBuffer[RxCount], rxfifocnt);
        RxCount += rxfifocnt;
        break;

    /* receive line status interrupt */
    case UART_INT_ID_LINE_STATUS:
        {
            APP_PRINT_ERROR1("Line status error!!!! Status = %x", 1, UART->LSR);
        }
        break;

    default:
        break;
    }

    return;
}

/**
* @brief  System interrupt handler function, For Wakeup pin.
* @param   No parameter.
* @return  void
*/
void System_Handler(void)
{
    if (System_WakeUpInterruptValue(UART_RX_PIN) == SET)
    {
        Pad_ClearWakeupINTPendingBit(UART_RX_PIN);
        allowedSystemEnterDlps = false;
        System_WakeUpInterruptDisable(UART_RX_PIN);
        System_WakeUpPinDisable(UART_RX_PIN);
    }
}

/**
* @brief  IO Parameter check fail.
* @param   No parameter.
* @return  void
*/
void io_assert_failed(uint8_t *file, uint32_t line)
{
    DBG_DIRECT("io driver parameters error! file_name: %s, line: %d", file, line);

    for (;;);
}
/** @} */ /* End of group UART_Dlps_Demo_Exported_Functions */
/** @} */ /* End of group DLPS_UART_DEMO */
