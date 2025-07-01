/**
*********************************************************************************************************
*               Copyright(c) 2020, Realtek Semiconductor Corporation. All rights reserved.
*********************************************************************************************************
* \file     rtl876x_smartcard.h
* \brief    The header file of the peripheral smartcard driver.
* \details  This file provides all ISO7816 firmware functions.
* \author   yuan
* \date     2020-07-13
* \version  v1.0.0
* *********************************************************************************************************
*/


#ifndef _RTL876X_SMARTCARD_H_
#define _RTL876X_SMARTCARD_H_

#ifdef __cplusplus
extern "C" {
#endif

/**
 * \addtogroup  IO          Peripheral Drivers
 * \defgroup    ISO7816          ISO7816
 *
 * \brief       Manage the ISO7816 peripheral functions.
 *
 * \ingroup     IO
 */

/*============================================================================*
 *                         Includes
 *============================================================================*/
#include "rtl876x.h"
#include <stdbool.h>

/*============================================================================*
 *                         Types
 *============================================================================*/

/**
 * \defgroup    ISO7816_Exported_Types Init Params Struct
 * \brief
 * \ingroup     ISO7816
 */

/**
 * \brief       ISO7816 initialize parameters
 *
 * \ingroup     ISO7816_Exported_Types
 */
typedef struct
{
    //baudrate calibration
    uint32_t ISO7816_NACKCmd;            /*!< Specifies the clock rate conversion integer.*/
    uint32_t ISO7816_ProtocolType;       /*!< Specifies the protocol type.*/
    uint32_t ISO7816_ClockStopPolar;     /*!< Specifies clock stop polarity. */
    uint32_t ISO7816_IOMode;             /*!< Specifies IO mode. 0: reception mode. 1: transmission mode. */
    uint32_t ISO7816_CodingConvention;   /*!< Specifies the coding convention. */
    uint16_t ISO7816_IntegerF;           /*!< Specifies the clock rate conversion integer.*/
    uint16_t ISO7816_IntegerD;           /*!< Specifies the baud rate adjustment integer.*/
    uint8_t  ISO7816_ExtraGuardTime;          /*!< Specifies the Extra Guard Time N.
                                         This parameter must range from 0 to 255, N=255: minimum etu (12 in T=0, 11 in T=1). */
    uint16_t ISO7816_WaitingTime;        /*!< Specifies the Waiting Time.
                                         This parameter must range from 0 to 0xFFFE. */
    uint32_t ISO7816_IOStateSample;      /*!< Sample I/O line in middle of start bit to confirm if a falling edge does lead to a character or not.
                                         0: Not to sample. 1: To sample. */
    uint16_t ISO7816_RxAFWaterLevel;     /*!< Used to calculate watermark of RX FIFO almost full signal generation.
                                         RXFIFO ��almost full�� means the number of unread bytes in RXFIFO exceeds 2*SIMRFAFWM. */
    uint8_t  ISO7816_RxRetryCountLevel;     /*!< Specifies the Rx retry count level.
                                         This parameter must range from 0 to 0xF.In T=0 application only. */
    uint8_t  ISO7816_TxRetryCountLevel;     /*!< Specifies the Tx retry count level.
                                         This parameter must range from 0 to 0xF.In T=0 application only. */
} ISO7816_InitTypeDef;

/*============================================================================*
 *                         Registers Definitions
 *============================================================================*/

/* Peripheral: ISO7816 */
/* Description: ISO7816 register defines */

/* Register: CR -------------------------------------------------------*/
/* Description: Control register. Offset: 0x00. Address: 0x400xxxxx. */

/* CR[27]: ISO7816_RX_FIFO_CLEAR. 0x1: Clear. */
#define ISO7816_RX_FIFO_CLEAR_POS                (27)
#define ISO7816_RX_FIFO_CLEAR_MSK                ((uint32_t)0x1 << ISO7816_RX_FIFO_CLEAR_POS)
#define ISO7816_RX_FIFO_CLEAR_CLR                (~ISO7816_RX_FIFO_CLEAR_MSK)
/* CR[26]: ISO7816_TX_FIFO_CLEAR. 0x1: Clear.  */
#define ISO7816_TX_FIFO_CLEAR_POS                (26)
#define ISO7816_TX_FIFO_CLEAR_MSK                ((uint32_t)0x1 << ISO7816_TX_FIFO_CLEAR_POS)
#define ISO7816_TX_FIFO_CLEAR_CLR                (~ISO7816_TX_FIFO_CLEAR_MSK)
/* CR[25]: ISO7816_INIT_GUARD_TIME. */
#define ISO7816_RX_FIFO_OVERWRITE_POS          (25)
#define ISO7816_INIT_GUARD_TIME_MSK         (0xF << ISO7816_INIT_GUARD_TIME_POS)
#define ISO7816_INIT_GUARD_TIME_CLR         (~ISO7816_INIT_GUARD_TIME_MSK)
/* CR[24]: ISO7816_PROTOCOL_TYPE. 0x1: T = 1. 0x0: T = 0,mandatory error signal and character repetition. */
#define ISO7816_PROTOCOL_TYPE_POS                (24)
#define ISO7816_PROTOCOL_TYPE_MSK                (0x1 << ISO7816_PROTOCOL_TYPE_POS)
#define ISO7816_PROTOCOL_TYPE_CLR                (~ISO7816_PROTOCOL_TYPE_MSK)
/* CR[23]: ISO7816_CLOCK_ENABLE. */
#define ISO7816_CLOCK_ENABLE_POS                 (23)
#define ISO7816_CLOCK_ENABLE_MSK                 (0x1 << ISO7816_CLOCK_ENABLE_POS)
#define ISO7816_CLOCK_ENABLE_CLR                 (~ISO7816_CLOCK_ENABLE_MSK)
/* CR[22]: ISO7816_CLOCK_STOP_POLAR. 0x1: high. 0x0: low. */
#define ISO7816_CLOCK_STOP_POLAR_POS             (22)
#define ISO7816_CLOCK_STOP_POLAR_MSK             (0x1 << ISO7816_CLOCK_STOP_POLAR_POS)
#define ISO7816_CLOCK_STOP_POLAR_CLR             (~ISO7816_CLOCK_STOP_POLAR_MSK)
/* CR[21]: ISO7816_VCC_ENABLE. */
#define ISO7816_VCC_ENABLE_POS                   (21)
#define ISO7816_VCC_ENABLE_MSK                   (0x1 << ISO7816_VCC_ENABLE_POS)
#define ISO7816_VCC_ENABLE_CLR                   (~ISO7816_VCC_ENABLE_MSK)
/* CR[20]: ISO7816_RESET_ENABLE. 0x1: bypass. 0x0: not bypass. */
#define ISO7816_RESET_ENABLE_POS                 (20)
#define ISO7816_RESET_ENABLE_MSK                 (0x1 << ISO7816_RESET_ENABLE_POS)
#define ISO7816_RESET_ENABLE_CLR                 (~ISO7816_RESET_ENABLE_MSK)
/* CR[19]: ISO7816_GP_COUNTER_ENABLE. */
#define ISO7816_GP_COUNTER_ENABLE_POS            (19)
#define ISO7816_GP_COUNTER_ENABLE_MSK            (0x1 << ISO7816_GP_COUNTER_ENABLE_POS)
#define ISO7816_GP_COUNTER_ENABLE_CLR            (~ISO7816_GP_COUNTER_ENABLE_MSK)
/* CR[18]: ISO7816_CONTROLLER_ENBALE. */
#define ISO7816_CONTROLLER_ENBALE_POS            (18)
#define ISO7816_CONTROLLER_ENBALE_MSK            (0x1 << ISO7816_CONTROLLER_ENBALE_POS)
#define ISO7816_CONTROLLER_ENBALE_CLR            (~ISO7816_CONTROLLER_ENBALE_MSK)
/* CR[17]: ISO7816_IO_MODE. */
#define ISO7816_IO_MODE_POS                      (17)
#define ISO7816_IO_MODE_MSK                      (0x1 << ISO7816_IO_MODE_POS)
#define ISO7816_IO_MODE_CLR                      (~ISO7816_IO_MODE_MSK)
/* CR[16]: ISO7816_CODING_CONVENTION. 0: direct convention 1: inverse convention. */
#define ISO7816_CODING_CONVENTION_POS            (16)
#define ISO7816_CODING_CONVENTION_MSK            (0x1 << ISO7816_CODING_CONVENTION_POS)
#define ISO7816_CODING_CONVENTION_CLR            (~ISO7816_CODING_CONVENTION_MSK)
/* CR[15:0]: ISO7816_GP_COUNTER. */
#define ISO7816_GP_COUNTER_POS                   (0)
#define ISO7816_GP_COUNTER_MSK                   (0xFFFF << ISO7816_GP_COUNTER_POS)
#define ISO7816_GP_COUNTER_CLR                   (~ISO7816_GP_COUNTER_MSK)

/* Register: GCR -------------------------------------------------------*/
/* Description: Device Global Configuration Register. Offset: 0x04. Address: 0x40024804. */

/* GCR[11:0]: ISO7816_CLOCK_DIV. */
#define ISO7816_CLOCK_DIV_POS                    (0)
#define ISO7816_CLOCK_DIV_MSK                    (0xFFF << ISO7816_CLOCK_DIV_POS)
#define ISO7816_CLOCK_DIV_CLR                    (~ISO7816_CLOCK_DIV_MSK)

/* Register: TCR ---------------------------------------------------------*/
/* Description: Transmitting Configuration Register. Offset: 0x08. Address: 0x40024808. */

/* TCR[7:0]: ISO7816_GUARD_TIME. */
#define ISO7816_GUARD_TIME_POS                   (0)
#define ISO7816_GUARD_TIME_MSK                   (0xFF << ISO7816_GUARD_TIME_POS)
#define ISO7816_GUARD_TIME_CLR                   (~ISO7816_GUARD_TIME_MSK)

/* Register: RCR ---------------------------------------------------------*/
/* Description: Receiving Configuration Register. Offset: 0x0C. Address: 0x4002480C. */

/* RCR[16]: ISO7816_IO_STATE_SAMPLE. 1: sample. 0: no sample. */
#define ISO7816_IO_STATE_SAMPLE_POS              (16)
#define ISO7816_IO_STATE_SAMPLE_MSK              (0x1 << ISO7816_IO_STATE_SAMPLE_POS)
#define ISO7816_IO_STATE_SAMPLE_CLR              (~ISO7816_IO_STATE_SAMPLE_MSK)
/* RCR[15:0]: ISO7816_WAITING_TIME. */
#define ISO7816_WAITING_TIME_POS                 (0)
#define ISO7816_WAITING_TIME_MSK                 (0xFFFF << ISO7816_WAITING_TIME_POS)
#define ISO7816_WAITING_TIME_CLR                 (~ISO7816_WAITING_TIME_MSK)

/* Register: THR ---------------------------------------------------------*/
/* Description: Threshold and Watermark Configuration Register. Offset: 0x10. Address: 0x40024810. */

/* THR[11:8]: ISO7816_AF_WATER_LEVEL. */
#define ISO7816_AF_WATER_LEVEL_POS               (8)
#define ISO7816_AF_WATER_LEVEL_MSK               (0xF << ISO7816_AF_WATER_LEVEL_POS)
#define ISO7816_AF_WATER_LEVEL_CLR               (~ISO7816_AF_WATER_LEVEL_MSK)
/* THR[7:4]: ISO7816_RX_NACK_THD_LEVEL. */
#define ISO7816_RX_NACK_THD_LEVEL_POS            (4)
#define ISO7816_RX_NACK_THD_LEVEL_MSK            (0xF << ISO7816_RX_NACK_THD_LEVEL_POS)
#define ISO7816_RX_NACK_THD_LEVEL_CLR            (~ISO7816_RX_NACK_THD_LEVEL_MSK)
/* THR[3:0]: ISO7816_TX_NACK_THD_LEVEL. */
#define ISO7816_TX_NACK_THD_LEVEL_POS            (0)
#define ISO7816_TX_NACK_THD_LEVEL_MSK            (0xF << ISO7816_TX_NACK_THD_LEVEL_POS)
#define ISO7816_TX_NACK_THD_LEVEL_CLR            (~ISO7816_TX_NACK_THD_LEVEL_MSK)

/* Register: ISR ---------------------------------------------------------*/
/* Description: Interrupt Status Register. Offset: 0x14. Address: 0x40024814. */


/* Register: IER -----------------------------------------------------*/
/* Description: Interrupt Enable Register . Offset: 0x18. Address: 0x40024818. */


/* Register: TSR -----------------------------------------------------*/
/* Description: Transmitter Status Register. Offset: 0x24. Address: 0x40024824. */


/* Register: RSR -----------------------------------------------------*/
/* Description: Receiver Status Register. Offset: 0x28. Address: 0x40024828. */


/* Register: ESR ----------------------------------------------------*/
/* Description: Error Status Register. Offset: 0x2C. Address: 0x4002482C. */


/* Register: CMD -----------------------------------------------------*/
/* Description: ISO7816 Enable Register. Offset: 0x30. Address: 0x40024830. */

/* CMD[0]: ISO7816_ENABLE. */
#define ISO7816_ENABLE_POS                 (0)
#define ISO7816_ENABLE_MSK                 (0x1 << ISO7816_ENABLE_POS)
#define ISO7816_ENABLE_CLR                 (~ISO7816_ENABLE_MSK)

/*============================================================================*
 *                         Constants
 *============================================================================*/

/**
 * \defgroup    ISO7816_Exported_Constants Macro Definitions
 *
 * \ingroup     ISO7816
 */

#define IS_ISO7816_PERIPH(PERIPH) ((PERIPH) == ISO7816)

#define ISO7816_TX_FIFO_SIZE             16
#define ISO7816_RX_FIFO_SIZE             32

/**
 * \defgroup    ISO7816_Clock_Cmd ISO7816 Clock Cmd
 * \{
 * \ingroup     ISO7816_Exported_Constants
 */
#define ISO7816_CLK_CONTACT_Reset                    ISO7816_CLOCK_ENABLE_CLR
#define ISO7816_CLK_CONTACT_Set                      ISO7816_CLOCK_ENABLE_MSK
/** \} */

/**
 * \defgroup    ISO7816_Clock_Stop_Polar ISO7816 Clock Stop Polar
 * \{
 * \ingroup     ISO7816_Exported_Constants
 */
#define ISO7816_CLOCK_STOP_POLAR_LOW                 ISO7816_CLOCK_STOP_POLAR_CLR
#define ISO7816_CLOCK_STOP_POLAR_HIGH                ISO7816_CLOCK_STOP_POLAR_MSK
/** \} */

/**
 * \defgroup    ISO7816_Power_Cmd ISO7816 Power Cmd
 * \{
 * \ingroup     ISO7816_Exported_Constants
 */
#define ISO7816_VCC_CONTACT_Reset                    ISO7816_VCC_ENABLE_CLR
#define ISO7816_VCC_CONTACT_Set                      ISO7816_VCC_ENABLE_MSK
/** \} */

/**
 * \defgroup    ISO7816_Reset_Cmd ISO7816 Reset Cmd
 * \{
 * \ingroup     ISO7816_Exported_Constants
 */
#define ISO7816_RST_CONTACT_Reset                    ISO7816_RESET_ENABLE_CLR
#define ISO7816_RST_CONTACT_Set                      ISO7816_RESET_ENABLE_MSK
/** \} */

/**
 * \defgroup    ISO7816_Generral_Purpose_Counter_Cmd ISO7816 GP Counter Cmd
 * \{
 * \ingroup     ISO7816_Exported_Constants
 */
#define ISO7816_GP_CNT_DISABLE                       ISO7816_GP_COUNTER_ENABLE_CLR
#define ISO7816_GP_CNT_ENABLE                        ISO7816_GP_COUNTER_ENABLE_MSK
/** \} */

/**
 * \defgroup    ISO7816_Controller_Cmd ISO7816 Controller Cmd
 * \{
 * \ingroup     ISO7816_Exported_Constants
 */
#define ISO7816_CR_DISABLE                           ISO7816_CONTROLLER_ENBALE_CLR
#define ISO7816_CR_ENABLE                            ISO7816_CONTROLLER_ENBALE_MSK
/** \} */

/**
 * \defgroup    ISO7816_IO_Mode ISO7816 IO Mode
 * \{
 * \ingroup     ISO7816_Exported_Constants
 */
#define ISO7816_IO_MODE_RX                           ISO7816_IO_MODE_CLR
#define ISO7816_IO_MODE_TX                           ISO7816_IO_MODE_MSK
/** \} */

/**
 * \defgroup    ISO7816_Coding_Convention_Cmd ISO7816 Coding Convention Cmd
 * \{
 * \ingroup     ISO7816_Exported_Constants
 */
#define ISO7816_CODING_CONVENTION_DIRECT             ISO7816_CODING_CONVENTION_CLR
#define ISO7816_CODING_CONVENTION_INVERSE            ISO7816_CODING_CONVENTION_MSK
/** \} */

/**
 * \defgroup    ISO7816_Coding_Convention_Cmd ISO7816 Coding Convention Cmd
 * \{
 * \ingroup     ISO7816_Exported_Constants
 */
#define ISO7816_IO_STATE_SAMPLE_DISABLE              ISO7816_IO_STATE_SAMPLE_CLR
#define ISO7816_IO_STATE_SAMPLE_ENABLE               ISO7816_IO_STATE_SAMPLE_MSK
/** \} */

/**
 * \defgroup    ISO7816_Interrupts_Definition  ISO7816 Interrupts Definition
 * \{
 * \ingroup     ISO7816_Exported_Constants
 */
#define ISO7816_INT_RESET_TIMING_VIOLATION           ((uint16_t)(1 << 0))
#define ISO7816_INT_TX_NAK_THD                       ((uint16_t)(1 << 1))
#define ISO7816_INT_RX_WAIT_TIMEOUT                  ((uint16_t)(1 << 2))
#define ISO7816_INT_GP_COUNTER                       ((uint16_t)(1 << 3))
#define ISO7816_INT_TX_FIFO_EMPTY                    ((uint16_t)(1 << 4))
#define ISO7816_INT_TX_FIFO_NOT_FULL                 ((uint16_t)(1 << 5))
#define ISO7816_INT_TX_FIFO_OVERFLOW                 ((uint16_t)(1 << 6))
#define ISO7816_INT_RX_FIFO_NOT_EMPTY                ((uint16_t)(1 << 7))
#define ISO7816_INT_RX_FIFO_ALMOST_FULL              ((uint16_t)(1 << 8))
#define ISO7816_INT_RX_FIFO_FULL                     ((uint16_t)(1 << 9))
#define ISO7816_INT_RX_FIFO_OVERFLOW                 ((uint16_t)(1 << 10))
#define ISO7816_INT_RX_FIFO_UNDERFLOW                ((uint16_t)(1 << 11))
#define ISO7816_INT_TX_DONE                          ((uint16_t)(1 << 12))
/** \} */

#define IS_ISO7816_INT(INT) (((INT) == ISO7816_INT_RESET_TIMING_VIOLATION) || ((INT) == ISO7816_INT_TX_NAK_THD)\
                             || ((INT) == ISO7816_INT_RX_WAIT_TIMEOUT) || ((INT) == ISO7816_INT_GP_COUNTER)\
                             || ((INT) == ISO7816_INT_TX_FIFO_EMPTY) || ((INT) == ISO7816_INT_TX_FIFO_NOT_FULL)\
                             || ((INT) == ISO7816_INT_TX_FIFO_OVERFLOW) || ((INT) == ISO7816_INT_RX_FIFO_NOT_EMPTY)\
                             || ((INT) == ISO7816_INT_RX_FIFO_ALMOST_FULL) || ((INT) == ISO7816_INT_RX_FIFO_FULL)\
                             || ((INT) == ISO7816_INT_RX_FIFO_OVERFLOW) || ((INT) == ISO7816_INT_RX_FIFO_UNDERFLOW)\
                             || ((INT) == ISO7816_INT_TX_DONE))

/**
 * \defgroup    ISO7816_Flags_Definition ISO7816 Flags Definition
 * \{
 * \ingroup     ISO7816_Exported_Constants
 */
#define ISO7816_FLAG_TX_FIFO_EMPTY                   ((uint16_t)(1 << 0))
#define ISO7816_FLAG_TX_FIFO_NOT_FULL                ((uint16_t)(1 << 1))
#define ISO7816_FLAG_TX_IDLE                         ((uint16_t)(1 << 2))
#define ISO7816_FLAG_RX_FIFO_NOT_EMPTY               ((uint16_t)(1 << 4))
#define ISO7816_FLAG_RX_FIFO_ALMOST_FULL             ((uint16_t)(1 << 5))
#define ISO7816_FLAG_RX_FIFO_FULL                    ((uint16_t)(1 << 6))
#define ISO7816_FLAG_RX_IO_HIGH_TIMING_VIOLATION     ((uint16_t)(1 << 8))
#define ISO7816_FLAG_RX_ATR_TIMING_VIOLATION         ((uint16_t)(1 << 9))
#define ISO7816_FLAG_TX_NAK_CNT_THD                  ((uint16_t)(1 << 10))
#define ISO7816_FLAG_RX_NAK_CNT_THD                  ((uint16_t)(1 << 11))
#define ISO7816_FLAG_TX_FIFO_OVERFLOW                ((uint16_t)(1 << 12))
#define ISO7816_FLAG_RX_FIFO_OVERFLOW                ((uint16_t)(1 << 13))
#define ISO7816_FLAG_RX_FIFO_UNDERFLOW               ((uint16_t)(1 << 14))
#define ISO7816_FLAG_RX_PARITY_ERR                   ((uint16_t)(1 << 15))
/** \} */

#define IS_ISO7816_FLAG(FLAG) (((FLAG) == ISO7816_FLAG_TX_FIFO_EMPTY) || ((FLAG) == ISO7816_FLAG_TX_FIFO_NOT_FULL)\
                               || ((FLAG) == ISO7816_FLAG_TX_IDLE) || ((FLAG) == ISO7816_FLAG_RX_FIFO_NOT_EMPTY)\
                               || ((FLAG) == ISO7816_FLAG_RX_FIFO_ALMOST_FULL) || ((FLAG) == ISO7816_FLAG_RX_FIFO_FULL)\
                               || ((FLAG) == ISO7816_FLAG_RX_AD_TIMING_VIOLATION) || ((FLAG) == ISO7816_FLAG_RX_CF_TIMING_VIOLATION)\
                               || ((FLAG) == ISO7816_FLAG_TX_NAK_CNT_THD) || ((FLAG) == ISO7816_FLAG_RX_NAK_CNT_THD)\
                               || ((FLAG) == ISO7816_FLAG_TX_FIFO_OVERFLOW) || ((FLAG) == ISO7816_FLAG_RX_FIFO_OVERFLOW)\
                               || ((FLAG) == ISO7816_FLAG_RX_FIFO_UNDERFLOW) || ((FLAG) == ISO7816_FLAG_RX_PARITY_ERR))

/**
 * \defgroup    ISO7816_CMD ISO7816 CMD
 * \{
 * \ingroup     ISO7816_Exported_Constants
 */
#define ISO7816_DISABLE                              ((uint8_t)(0 << 0))
#define ISO7816_ENABLE                               ((uint8_t)(1 << 0))
/** \} */

#define IS_ISO7816_CMD(CMD) (((CMD) == ISO7816_DISABLE) || ((CMD) == ISO7816_ENABLE))

/** \} */ /* End of group ISO7816_Exported_Constants */

/*============================================================================*
 *                         Functions
 *============================================================================*/

/**
 * \defgroup    ISO7816_Exported_Functions Peripheral APIs
 * \{
 * \ingroup     ISO7816
 */

/**
 * \brief   Deinitialize the ISO7816 peripheral registers to their default reset values(turn off ISO7816 clock).
 * \param   None.
 * \return  None.
 *
 * <b>Example usage</b>
 * \code{.c}
 *
 * void driver_smartcard_init(void)
 * {
 *    ISO7816_DeInit();
 * }
 * \endcode
 */
void ISO7816_DeInit(void);

/**
 * \brief       Initialize the ISO7816 peripheral according to the specified parameters in ISO7816_InitStruct.
 * \param[in]   ISO7816_InitStruct: Pointer to a ISO7816_InitTypeDef structure that contains
 *                               the configuration information for the ISO7816 peripheral.
 * \return      None.
 *
 * <b>Example usage</b>
 * \code{.c}
 *
 * void driver_smartcard_init(void)
 * {
 *     ISO7816_DeInit();
 *
 *     RCC_PeriphClockCmd(APBPeriph_ISO7816, APBPeriph_ISO7816_CLOCK, ENABLE);
 *
 *     ISO7816_InitTypeDef ISO7816_InitStruct;
 *     ISO7816_StructInit(&ISO7816_InitStruct);
 *     ISO7816_InitStruct.ISO7816_Div         = 20;
 *     ISO7816_InitStruct.ISO7816_Ovsr        = 12;
 *     ISO7816_InitStruct.ISO7816_OvsrAdj     = 0x252;
 *     ISO7816_InitStruct.ISO7816_RxThdLevel  = 16;
 *     //Add other initialization parameters that need to be configured here.
 *     ISO7816_Init(ISO7816, &ISO7816_InitStruct);
 * }
 * \endcode
 */
void ISO7816_Init(ISO7816_InitTypeDef *ISO7816_InitStruct);

/**
 * \brief   Fills each ISO7816_InitStruct member with its default value.
 * \param[in]   ISO7816_InitStruct: pointer to an ISO7816_InitTypeDef structure which will be initialized.
 * \return  None.
 *
 * <b>Example usage</b>
 * \code{.c}
 *
 * void driver_smartcard_init(void)
 * {
 *     ISO7816_DeInit();
 *
 *     RCC_PeriphClockCmd(APBPeriph_ISO7816, APBPeriph_ISO7816_CLOCK, ENABLE);
 *
 *     ISO7816_InitTypeDef ISO7816_InitStruct;
 *     ISO7816_StructInit(&ISO7816_InitStruct);
 *     ISO7816_InitStruct.ISO7816_Div         = 20;
 *     ISO7816_InitStruct.ISO7816_Ovsr        = 12;
 *     ISO7816_InitStruct.ISO7816_OvsrAdj     = 0x252;
 *     ISO7816_InitStruct.ISO7816_RxThdLevel  = 16;
 *     //Add other initialization parameters that need to be configured here.
 *     ISO7816_Init(ISO78160, &ISO7816_InitStruct);
 * }
 * \endcode
 */
void ISO7816_StructInit(ISO7816_InitTypeDef *ISO7816_InitStruct);

/**
 * rtl876x_sc.h
 * \brief   Enables or disables the specified ISO7816 interrupts.
 * \param[in] ISO7816x: ISO7816 peripheral selected, x can be 0 ~ 2.
 * \param[in] ISO7816_IT: Specified the ISO7816 interrupt source that to be enabled or disabled.
 *      This parameter can be any combination of the following values:
 *      \arg ISO7816_INT_RD_AVA: Rx data avaliable interrupt source.
 *      \arg ISO7816_INT_TX_FIFO_EMPTY: Tx FIFO empty interrupt source.
 *      \arg ISO7816_INT_RX_LINE_STS: Rx line status interrupt source.
 *      \arg ISO7816_INT_MODEM_STS: Modem status interrupt source.
 *      \arg ISO7816_INT_TX_DONE: Tx done interrupt source.
 *      \arg ISO7816_INT_TX_THD: Tx threshold(FIFO data length <= thredhold) interrupt source.
 *      \arg ISO7816_INT_RX_BREAK: Rx break signal interrupt source.
 *      \arg ISO7816_INT_RX_IDLE: Rx bus ilde interrupt source.
 * \param[in] newState: New state of the specified ISO7816 interrupt source.
 *      This parameter can be: ENABLE or DISABLE.
 * \return  None.
 *
 * <b>Example usage</b>
 * \code{.c}
 *
 * void driver_smartcard_init(void)
 * {
 *     ISO7816_DeInit(ISO78160);
 *
 *     RCC_PeriphClockCmd(APBPeriph_ISO78160, APBPeriph_ISO78160_CLOCK, ENABLE);
 *
 *     ISO7816_InitTypeDef ISO7816_InitStruct;
 *     ISO7816_StructInit(&ISO7816_InitStruct);
 *     ISO7816_InitStruct.ISO7816_Div         = 20;
 *     ISO7816_InitStruct.ISO7816_Ovsr        = 12;
 *     ISO7816_InitStruct.ISO7816_OvsrAdj     = 0x252;
 *     ISO7816_InitStruct.ISO7816_RxThdLevel  = 16;
 *     //Add other initialization parameters that need to be configured here.
 *     ISO7816_Init(ISO78160, &ISO7816_InitStruct);
 *
 *     ISO7816_INTConfig(ISO78160, ISO7816_INT_RD_AVA, ENABLE);
 * }
 * \endcode
 */
void ISO7816_INTConfig(uint32_t ISO7816_INT, FunctionalState NewState);

/**
 * \brief   Send data.
 * \param[in] ISO7816x: ISO7816 peripheral selected, x can be 0 ~ 2.
 * \param[in] inBuf: Buffer of data to be sent.
 * \param[in] count: Length of data to be sent.
 * \return  None.
 *
 * <b>Example usage</b>
 * \code{.c}
 *
 * void smartcard_demo(void)
 * {
 *     uint8_t data[] = "ISO7816 demo";
 *     ISO7816_SendData(ISO78160, data, sizeof(data));
 * }
 * \endcode
 */
void ISO7816_SendData(const uint8_t *inBuf, uint16_t len);

/**
 * \brief   Receive data from RX FIFO.
 * \param[in]  ISO7816x: ISO7816 peripheral selected, x can be 0 ~ 2.
 * \param[out] outBuf: Buffer to store data which read from RX FIFO.
 * \param[in]  count: Length of data to read.
 * \return  None.
 *
 * <b>Example usage</b>
 * \code{.c}
 *
 * void smartcard_demo(void)
 * {
 *     uint8_t data[32] = {10};
 *     ISO7816_ReceiveData(ISO7816, data, 10);
 * }
 * \endcode
 */
void ISO7816_ReceiveData(uint8_t *outBuf, uint16_t len);

/**
 * \brief   Receive data from RX FIFO.
 * \param[in]  ISO7816x: ISO7816 peripheral selected, x can be 0 ~ 2.
 * \param[out] outBuf: Buffer to store data which read from RX FIFO.
 * \param[in]  count: Length of data to read.
 * \return  None.
 *
 * <b>Example usage</b>
 * \code{.c}
 *
 * void smartcard_demo(void)
 * {
 *     uint8_t data[32] = {10};
 *     ISO7816_ReceiveData(ISO7816, data, 10);
 * }
 * \endcode
 */
void ISO7816_CardColdReset(void);

/**
 * \brief   Receive data from RX FIFO.
 * \param[in]  ISO7816x: ISO7816 peripheral selected, x can be 0 ~ 2.
 * \param[out] outBuf: Buffer to store data which read from RX FIFO.
 * \param[in]  count: Length of data to read.
 * \return  None.
 *
 * <b>Example usage</b>
 * \code{.c}
 *
 * void smartcard_demo(void)
 * {
 *     uint8_t data[32] = {10};
 *     ISO7816_ReceiveData(ISO7816, data, 10);
 * }
 * \endcode
 */
void ISO7816_CardWarmReset(void);

/**
 * \brief   Stop the clock output to the pATRBuf.
 * \param   NewState: New state of the physically connected pATRBuf.
 *          This parameter can be: ENABLE or DISABLE.
 * \return  None.
 */
void ISO7816_CardClockStop(void);

/**
 * \brief   Receive data from RX FIFO.
 * \param[in]  ISO7816x: ISO7816 peripheral selected, x can be 0 ~ 2.
 * \param[out] outBuf: Buffer to store data which read from RX FIFO.
 * \param[in]  count: Length of data to read.
 * \return  None.
 *
 * <b>Example usage</b>
 * \code{.c}
 *
 * void smartcard_demo(void)
 * {
 *     uint8_t data[32] = {10};
 *     ISO7816_ReceiveData(ISO7816, data, 10);
 * }
 * \endcode
 */
void ISO7816_CardClockRestart(void);

/**
 * \brief   Deactive a physically connected pATRBuf.
 * \param   NewState: New state of the physically connected pATRBuf.
 *          This parameter can be: ENABLE or DISABLE.
 * \return  None.
 */
void ISO7816_CardDeactive(void);

/**
 * \brief   Receive data from RX FIFO.
 * \param[in]  ISO7816x: ISO7816 peripheral selected, x can be 0 ~ 2.
 * \param[out] outBuf: Buffer to store data which read from RX FIFO.
 * \param[in]  count: Length of data to read.
 * \return  None.
 *
 * <b>Example usage</b>
 * \code{.c}
 *
 * void smartcard_demo(void)
 * {
 *     uint8_t data[32] = {10};
 *     ISO7816_ReceiveData(ISO7816, data, 10);
 * }
 * \endcode
 */
void ISO7816_DelayCycle(uint32_t cycle);

/**
 * \brief       Check whether the specified SmartCard flag is set.
 * \param[in]   ISO7816_FLAG: Specified ISO7816 flag to check.
 *              This parameter can be one of the following values:
 *              \arg ISO7816_FLAG_RX_DATA_AVA: Rx data is avaliable.
 *              \arg ISO7816_FLAG_RX_OVERRUN: Rx FIFO overrun.
 *              \arg ISO7816_FLAG_RX_PARITY_ERR: Rx parity error.
 *              \arg ISO7816_FLAG_RX_FRAME_ERR: Rx frame error.
 *              \arg ISO7816_FLAG_RX_BREAK_ERR: Rx break error.
 *              \arg ISO7816_FLAG_TX_FIFO_EMPTY: Tx Holding Register or Tx FIFO empty
 *              \arg ISO7816_FLAG_TX_EMPTY: Tx FIFO and Tx shift register are both empty.
 *              \arg ISO7816_FLAG_RX_FIFO_ERR: Rx FIFO error.
 *              \arg ISO7816_FLAG_RX_BREAK: Rx break.
 *              \arg ISO7816_FLAG_RX_IDLE: Rx idle.
 *              \arg ISO7816_FLAG_TX_DONE: Tx waveform done & TX_FIFO_EMPTY = 1.
 *              \arg ISO7816_FLAG_TX_THD: TX_FIFO_LEVEL<=txfifo_trigger_level.
 * \return      New status of ISO7816 flag.
 * \retval      SET: The specified ISO7816 flag bit is set.
 * \retval      RESET: The specified flag is not set.
 *
 * <b>Example usage</b>
 * \code{.c}
 *
 * void ISO7816_senddata_continuous(ISO7816_TypeDef *ISO7816x, const uint8_t *pSend_Buf, uint16_t vCount)
 * {
 *     uint8_t count;
 *
 *     while (vCount / ISO7816_TX_FIFO_SIZE > 0)
 *     {
 *         while (ISO7816_GetFlagStatus(ISO7816x, ISO7816_FLAG_TX_FIFO_EMPTY) == 0);
 *         for (count = ISO7816_TX_FIFO_SIZE; count > 0; count--)
 *         {
 *             ISO7816x->RB_THR = *pSend_Buf++;
 *         }
 *         vCount -= ISO7816_TX_FIFO_SIZE;
 *     }
 *
 *     while (ISO7816_GetFlagStatus(ISO7816x, ISO7816_FLAG_TX_FIFO_EMPTY) == 0);
 *     while (vCount--)
 *     {
 *         ISO7816x->RB_THR = *pSend_Buf++;
 *     }
 * }
 * \endcode
 */
FlagStatus ISO7816_GetFlagStatus(uint32_t ISO7816_FLAG);

/**
 * @brief       Check whether the specified ISO7816 interrupt source is set or not.
                Software reads to clear all interrupts.
 * @param[in]   ISO7816_INT: specifies the interrupt source to check.
 *              This parameter can be one of the following values:
 *              @arg ISO7816_INT_RESET_TIMING_VIOLATION: Reset timing violation.
 *              @arg ISO7816_INT_TX_NAK_THD:             TNAKTH exceeded.
 *              @arg ISO7816_INT_RX_WAIT_TIMEOUT:        Character wait timeout.
 *              @arg ISO7816_INT_GP_COUNTER:             General purpose counter hit.
 *              @arg ISO7816_INT_TX_FIFO_EMPTY:          TX FIFO empty.
 *              @arg ISO7816_INT_TX_FIFO_NOT_FULL:       TX FIFO not full, triggered when TX FIFO changes from full to non-full.
 *              @arg ISO7816_INT_TX_FIFO_OVERFLOW:       TX FIFO overflow.
 *              @arg ISO7816_INT_RX_FIFO_NOT_EMPTY:      RX FIFO not empty, triggered when RX FIFO changes from empty to non-empty.
 *              @arg ISO7816_INT_RX_FIFO_ALMOST_FULL:    RX FIFO almost full
 *              @arg ISO7816_INT_RX_FIFO_FULL:           RX FIFO full.
 *              @arg ISO7816_INT_RX_FIFO_OVERFLOW:       RX FIFO overflow.
 *              @arg ISO7816_INT_RX_FIFO_UNDERFLOW:      RX FIFO underflow.
 *              @arg ISO7816_INT_TX_DONE:                Transmission completed, triggered when TX FIFO is empty and the last bit has been sent out.
 * @return      The new state of ISO7816_INT (SET or RESET).
 */
//ITStatus ISO7816_GetINTStatus(uint32_t ISO7816_INT);
__STATIC_INLINE uint16_t ISO7816_GetINTStatus(void)
{
    return (uint16_t)(ISO7816->ISR & 0x1FFF);
}

/**
 * \brief       Restart the general purpose counter.
 * \param[in]   value: 0~0xFFFF.
 * \return      None.
 *
 * <b>Example usage</b>
 * \code{.c}
 *
 * void smartcard_demo(void)
 * {
 *     ISO7816_GPCounterRestart(400);
 * }
 * \endcode
  */
__STATIC_INLINE uint8_t ISO7816_GetErrStatus(void)
{
    return (uint8_t)(ISO7816->ESR & 0xFF);
}

/**
 * \brief       Restart the general purpose counter.
 * \param[in]   value: 0~0xFFFF.
 * \return      None.
 *
 * <b>Example usage</b>
 * \code{.c}
 *
 * void smartcard_demo(void)
 * {
 *     ISO7816_GPCounterRestart(400);
 * }
 * \endcode
  */
__STATIC_INLINE void ISO7816_ClearErrStatus(uint8_t status)
{
    ISO7816->ESR |= status & 0xFF;
}

/**
 * \brief       Send one byte of data.
 * \param[in]   data: Byte data to send.
 * \return      None.
 *
 * <b>Example usage</b>
 * \code{.c}
 *
 * void smartcard_demo(void)
 * {
 *     uint8_t data = 0x55;
 *     ISO7816_SendByte(data);
 * }
 * \endcode
 */
__STATIC_INLINE void ISO7816_SendByte(uint8_t data)
{
    ISO7816->TX_FIFO = data;
}

/**
 * \brief   Read a byte of data from SmartCard RX FIFO.
 * \param   None.
 * \return  Which byte data has been read.
 *
 * <b>Example usage</b>
 * \code{.c}
 *
 * void smartcard_demo(void)
 * {
 *     uint8_t data = ISO7816_ReceiveByte();
 *
 * }
 * \endcode
 */
__STATIC_INLINE uint8_t ISO7816_ReceiveByte(void)
{
    return (uint8_t)(ISO7816->RX_FIFO);
}

/**
 * \brief   Clear Tx FIFO of SmartCard peripheral.
 * \param   None.
 * \return  None.
 *
 * <b>Example usage</b>
 * \code{.c}
 *
 * void smartcard_demo(void)
 * {
 *     ISO7816_ClearTxFIFO();
 * }
 * \endcode
 */
__STATIC_INLINE void ISO7816_ClearTxFIFO(void)
{
    ISO7816->CR |= ISO7816_TX_FIFO_CLEAR_MSK;
}

/**
 * \brief   Clear Rx FIFO of SmartCard peripheral.
 * \param   None.
 * \return  None.
 *
 * <b>Example usage</b>
 * \code{.c}
 *
 * void smartcard_demo(void)
 * {
 *     ISO7816_ClearRxFIFO();
 * }
 * \endcode
 */
__STATIC_INLINE void ISO7816_ClearRxFIFO(void)
{
    ISO7816->CR |= ISO7816_RX_FIFO_CLEAR_MSK;
}

/**
 * \brief       Restart the general purpose counter.
 * \param[in]   value: 0~0xFFFF.
 * \return      None.
 *
 * <b>Example usage</b>
 * \code{.c}
 *
 * void smartcard_demo(void)
 * {
 *     ISO7816_GPCounterRestart(400);
 * }
 * \endcode
  */
__STATIC_INLINE void ISO7816_GPCounterRestart(uint16_t value)
{
    ISO7816->CR &= ISO7816_GP_COUNTER_ENABLE_CLR;
    ISO7816->CR |= value;
    ISO7816->CR |= ISO7816_GP_COUNTER_ENABLE_MSK;
}

/**
 * \brief       Restart the general purpose counter.
 * \param[in]   value: 0~0xFFFF.
 * \return      None.
 *
 * <b>Example usage</b>
 * \code{.c}
 *
 * void smartcard_demo(void)
 * {
 *     ISO7816_GPCounterRestart(400);
 * }
 * \endcode
  */
__STATIC_INLINE void ISO7816_SetRST(void)
{
    ISO7816->CR |= ISO7816_RESET_ENABLE_MSK;//RST = 1.
}

//__STATIC_INLINE void ISO7816_ResetRST(void)
//{
//    ISO7816->CR &= ISO7816_RESET_ENABLE_CLR;
//}
/**
 * \brief       Restart the general purpose counter.
 * \param[in]   value: 0~0xFFFF.
 * \return      None.
 *
 * <b>Example usage</b>
 * \code{.c}
 *
 * void smartcard_demo(void)
 * {
 *     ISO7816_GPCounterRestart(400);
 * }
 * \endcode
  */
__STATIC_INLINE void ISO7816_ProtocolTypeConfig(uint8_t protocol)
{
    if (protocol)
    {
        ISO7816->CR |= ISO7816_PROTOCOL_TYPE_MSK;//RST = 1.
    }
    else
    {
        ISO7816->CR &= ISO7816_PROTOCOL_TYPE_CLR;//RST = 1.
    }
}

/**
 * \brief       Restart the general purpose counter.
 * \param[in]   value: 0~0xFFFF.
 * \return      None.
 *
 * <b>Example usage</b>
 * \code{.c}
 *
 * void smartcard_demo(void)
 * {
 *     ISO7816_GPCounterRestart(400);
 * }
 * \endcode
  */
__STATIC_INLINE void ISO7816_ClockPolarConfig(uint8_t polar)
{
    if (polar)
    {
        ISO7816->CR |= ISO7816_CLOCK_STOP_POLAR_MSK;//RST = 1.
    }
    else
    {
        ISO7816->CR &= ISO7816_CLOCK_STOP_POLAR_CLR;//RST = 1.
    }
}

/**
 * \brief       Restart the general purpose counter.
 * \param[in]   value: 0~0xFFFF.
 * \return      None.
 *
 * <b>Example usage</b>
 * \code{.c}
 *
 * void smartcard_demo(void)
 * {
 *     ISO7816_GPCounterRestart(400);
 * }
 * \endcode
  */
__STATIC_INLINE void ISO7816_ConventionConfig(uint8_t convention)
{
    if (convention)
    {
        ISO7816->CR |= ISO7816_CODING_CONVENTION_MSK;//RST = 1.
    }
    else
    {
        ISO7816->CR &= ISO7816_CODING_CONVENTION_CLR;//RST = 1.
    }
}


/**
 * \brief       Restart the general purpose counter.
 * \param[in]   value: 0~0xFFFF.
 * \return      None.
 *
 * <b>Example usage</b>
 * \code{.c}
 *
 * void smartcard_demo(void)
 * {
 *     ISO7816_GPCounterRestart(400);
 * }
 * \endcode
  */
__STATIC_INLINE void ISO7816_SetSpeed(uint16_t speed)
{
    ISO7816->GCR = speed & 0xFFF;
}

/**
 * \brief       Restart the general purpose counter.
 * \param[in]   value: 0~0xFFFF.
 * \return      None.
 *
 * <b>Example usage</b>
 * \code{.c}
 *
 * void smartcard_demo(void)
 * {
 *     ISO7816_GPCounterRestart(400);
 * }
 * \endcode
  */
__STATIC_INLINE void ISO7816_SetGT(uint8_t time)
{
    ISO7816->TCR = time;
}

/**
 * \brief       Restart the general purpose counter.
 * \param[in]   value: 0~0xFFFF.
 * \return      None.
 *
 * <b>Example usage</b>
 * \code{.c}
 *
 * void smartcard_demo(void)
 * {
 *     ISO7816_GPCounterRestart(400);
 * }
 * \endcode
  */
__STATIC_INLINE void ISO7816_SetWT(uint16_t time)
{
    ISO7816->RCR |= time & 0xFFFE;
}


/** \} */ /* End of group ISO7816_Exported_Functions */

#ifdef __cplusplus
}
#endif

#endif /* _RTL876X_ISO7816_H_ */


/******************* (C) COPYRIGHT 2020 Realtek Semiconductor *****END OF FILE****/

