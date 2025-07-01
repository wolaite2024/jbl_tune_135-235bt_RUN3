/**
*********************************************************************************************************
*               Copyright(c) 2019, Realtek Semiconductor Corporation. All rights reserved.
**********************************************************************************************************
* @file     rtl876x_uart.c
* @brief    This file provides the UART firmware functions.
* @details
* @author   justin
* @date     2019-07-12
* @version  v0.1
*********************************************************************************************************
*/
#include "stdbool.h"
#include "rtl876x.h"
//#include "rtl876x_bitfields.h"
#include "rtl876x_rcc.h"
#include "rtl876x_uart.h"

#define LCR_DLAB_Set                    ((uint32_t)(1 << 7))
#define LCR_DLAB_Reset                  ((uint32_t)~(1 << 7))
/**
  * @brief Set baud rate of uart.
  * @param  UARTx: selected UART peripheral.
  * @param  baud_rate: baud rate to be set. value reference UartBaudRate_TypeDef.
  * @retval  0 means set success ,1 not support this baud rate.
  */
//bool UART_SetBaudRate(UART_TypeDef *UARTx, UartBaudRate_TypeDef baud_rate)
//{
//    uint16_t div;
//    uint16_t ovsr;
//    uint16_t ovsr_adj;
//    switch (baud_rate)
//    {
//    case BAUD_RATE_1200:
//        div = 2589;
//        ovsr = 7;
//        ovsr_adj = 0x7F7;
//        break;
//    case BAUD_RATE_9600:
//        div = 271;
//        ovsr = 10;
//        ovsr_adj = 0x24A;
//        break;
//    case BAUD_RATE_14400:
//        div = 271;
//        ovsr = 5;
//        ovsr_adj = 0x222;
//        break;
//    case BAUD_RATE_19200:
//        div = 123;
//        ovsr = 11;
//        ovsr_adj = 0x6FF;
//        break;
//    case BAUD_RATE_28800:
//        div = 82;
//        ovsr = 11;
//        ovsr_adj = 0x6FF;
//        break;
//    case BAUD_RATE_38400:
//        div = 85;
//        ovsr = 7;
//        ovsr_adj = 0x222;
//        break;
//    case BAUD_RATE_57600:
//        div = 41;
//        ovsr = 11;
//        ovsr_adj = 0x6FF;
//        break;
//    case BAUD_RATE_76800:
//        div = 35;
//        ovsr = 9;
//        ovsr_adj = 0x7EF;
//        break;
//    case BAUD_RATE_115200:
//        div = 20;
//        ovsr = 12;
//        ovsr_adj = 0x252;
//        break;
//    case BAUD_RATE_128000:
//        div = 25;
//        ovsr = 7;
//        ovsr_adj = 0x555;
//        break;
//    case BAUD_RATE_153600:
//        div = 15;
//        ovsr = 12;
//        ovsr_adj = 0x252;
//        break;
//    case BAUD_RATE_230400:
//        div = 10;
//        ovsr = 12;
//        ovsr_adj = 0x252;
//        break;
//    case BAUD_RATE_460800:
//        div = 5;
//        ovsr = 12;
//        ovsr_adj = 0x252;
//        break;
//    case BAUD_RATE_500000:
//        div = 8;
//        ovsr = 5;
//        ovsr_adj = 0x0;
//        break;

//    case BAUD_RATE_921600:
//        div = 3;
//        ovsr = 9;
//        ovsr_adj = 0x2aa;
//        break;

//    case BAUD_RATE_1000000:
//        div = 4;
//        ovsr = 5;
//        ovsr_adj = 0x0;
//        break;
//    case BAUD_RATE_1382400:
//        div = 2;
//        ovsr = 9;
//        ovsr_adj = 0x2AA;
//        break;
//    case BAUD_RATE_1444400:
//        div = 2;
//        ovsr = 8;
//        ovsr_adj = 0x5F7;
//        break;

//    case BAUD_RATE_1500000:
//        div = 2;
//        ovsr = 8;
//        ovsr_adj = 0x492;
//        break;
//    case BAUD_RATE_1843200:
//        div = 2;
//        ovsr = 5;
//        ovsr_adj = 0x3F7;
//        break;

//    case BAUD_RATE_2000000:
//        div = 2;
//        ovsr = 5;
//        ovsr_adj = 0;
//        break;
//    case BAUD_RATE_3000000:
//        div = 1;
//        ovsr = 8;
//        ovsr_adj = 0x492;
//        break;
//    case BAUD_RATE_4000000:
//        div = 1;
//        ovsr = 5;
//        ovsr_adj = 0;
//        break;
//    default:
//        return 1;
//    }
//    /*set baudrate, firstly set DLAB bit*/
//    UARTx->LCR |= LCR_DLAB_Set;
//    /*set calibration parameters(OVSR)*/
//    UARTx->STSR &= ~0xF0;
//    UARTx->STSR |= (ovsr << 4);
//    /*set calibration parameters(OVSR_adj)*/
//    UARTx->SPR &= (~(0x7ff << 16));
//    UARTx->SPR |= (ovsr_adj << 16);
//    /*set DLL and DLH*/
//    UARTx->DLL = (div & 0x00FF);
//    UARTx->DLH_INTCR = ((div & 0xFF00) >> 8);
//    /*after set baudrate, clear DLAB bit*/
//    UARTx->LCR &= LCR_DLAB_Reset;
//    return 0;
//}

//void UART_IdleIntConfig(UART_TypeDef *UARTx, FunctionalState newState)
//{
//    if (newState == ENABLE)
//    {
//        UARTx->RXIDLE_INTCR |= BIT0;
//        UARTx->RX_IDLE_INTTCR |= BIT31;
//    }
//    else
//    {
//        UARTx->RX_IDLE_INTTCR &= (~BIT31);
//        UARTx->RX_IDLE_SR |= BIT0;
//        UARTx->RXIDLE_INTCR &= (~BIT0);
//    }
//}
