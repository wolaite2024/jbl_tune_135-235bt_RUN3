/**
*********************************************************************************************************
*               Copyright(c) 2017, Realtek Semiconductor Corporation. All rights reserved.
**********************************************************************************************************
* @file       sdcard.c
* @brief     This file provides all the sdcard firmware functions.
* @details
* @author   elliot chen
* @date      2017-01-13
* @version  v1.0
*********************************************************************************************************
*/

/*============================================================================*
 *                              Header Files
 *============================================================================*/
#include "sdcard.h"
#include <string.h>
#include "rtl876x_rcc.h"

/** @defgroup  SDCARD  SDCARD DEMO
    * @brief  SD card implementation demo code
    * @{
    */

/*============================================================================*
 *                              Macros
 *============================================================================*/

/** @defgroup SDCard_Demo_Exported_Macros SDCard Demo Exported Macros
  * @brief
  * @{
  */

/* Private define ------------------------------------------------------------*/
#define SDIO_HOST_INIT_TIMEOUT          ((uint32_t)0xFFFFFF)
#define SDIO_CMDTIMEOUT                 ((uint32_t)0xFFFFFF)
#define SDIO_PROG_TIMEOUT               ((uint32_t)0xFFFFFFFF)

/* CMD8 check pattern and VHS */
#define SD_CHECK_PATTERN                ((uint32_t)0x000001AA)
#define SD_STD_CAPACITY                 ((uint32_t)0x00000000)
#define SD_HIGH_CAPACITY                ((uint32_t)0x40000000)

/* Mask for errors Card Status R1 */
#define SD_OCR_ADDR_OUT_OF_RANGE        ((uint32_t)0x80000000)
#define SD_OCR_ADDR_MISALIGNED          ((uint32_t)0x40000000)
#define SD_OCR_BLOCK_LEN_ERR            ((uint32_t)0x20000000)
#define SD_OCR_ERASE_SEQ_ERR            ((uint32_t)0x10000000)
#define SD_OCR_BAD_ERASE_PARAM          ((uint32_t)0x08000000)
#define SD_OCR_WRITE_PROT_VIOLATION     ((uint32_t)0x04000000)
#define SD_OCR_LOCK_UNLOCK_FAILED       ((uint32_t)0x01000000)
#define SD_OCR_COM_CRC_FAILED           ((uint32_t)0x00800000)
#define SD_OCR_ILLEGAL_CMD              ((uint32_t)0x00400000)
#define SD_OCR_CARD_ECC_FAILED          ((uint32_t)0x00200000)
#define SD_OCR_CC_ERROR                 ((uint32_t)0x00100000)
#define SD_OCR_GENERAL_UNKNOWN_ERROR    ((uint32_t)0x00080000)
#define SD_OCR_CID_CSD_OVERWRIETE       ((uint32_t)0x00010000)
#define SD_OCR_WP_ERASE_SKIP            ((uint32_t)0x00008000)
#define SD_OCR_CARD_ECC_DISABLED        ((uint32_t)0x00004000)
#define SD_OCR_ERASE_RESET              ((uint32_t)0x00002000)
#define SD_OCR_SD_APP_CMD               ((uint32_t)0x00000020)
#define SD_OCR_AKE_SEQ_ERROR            ((uint32_t)0x00000008)
#define SD_OCR_ERRORBITS                ((uint32_t)0xFDFFE008)
#define SD_ALLZERO                      ((uint32_t)0x00000000)

/* Card current status */
#define SD_CARD_IDLE                    ((uint32_t)0x00000000)
#define SD_CARD_READY                   ((uint32_t)0x00000001)
#define SD_CARD_IDENT                   ((uint32_t)0x00000002)
#define SD_CARD_STBY                    ((uint32_t)0x00000003)
#define SD_CARD_TRAN                    ((uint32_t)0x00000004)
#define SD_CARD_DATA                    ((uint32_t)0x00000005)
#define SD_CARD_RECEIVING               ((uint32_t)0x00000006)
#define SD_CARD_PROGRAMMING             ((uint32_t)0x00000007)
#define SD_CARD_DIS                     ((uint32_t)0x00000008)

/** @} */ /* End of group SDCard_Demo_Exported_Macros */

/*============================================================================*
 *                              Variables
 *============================================================================*/
/** @defgroup SDCard_Demo_Exported_Variables SDCard Demo Exported Variables
  * @brief
  * @{
  */

/* Store information of ADMA2 dersciptor table which used for ADMA2 transmission */
SDIO_ADMA2TypeDef   SDIO_ADMA2_DescTab[2];

/* Card Type */
uint8_t CardType =  SDIO_STD_CAPACITY_SD_CARD_V1_1;
/* SetCardType */
#define SetCardType(value)  (CardType = (uint8_t)value)

/* Variables which uesd for sd card information analysis */
uint32_t RCA = 0;
uint32_t CSD_Tab[4] = {0, 0, 0, 0};
uint32_t CID_Tab[4] = {0, 0, 0, 0};

/** @} */ /* End of group SDCard_Demo_Exported_Variables */

/*============================================================================*
 *                              Functions
 *============================================================================*/
/** @defgroup SDCard_Demo_Exported_Functions SDCard Demo Exported Functions
  * @brief
  * @{
  */

/**
  * @brief  Initialize SD host controller.
  * @param  None.
  * @retval  SD_Status: SD status.
  */
SD_Status SD_InitHost(void)
{
    uint32_t time_out = SDIO_HOST_INIT_TIMEOUT;

    /* Software reset host controller */
    SDIO_SoftwareReset();
    while (SDIO_GetSoftwareResetStatus() == SET)
    {
        time_out--;
        if (time_out == 0)
        {
            return SD_HOST_SW_RESET_ERROR;
        }
    }

    /* Enable internal clcok */
    SDIO_InternalClockCmd(ENABLE);
    time_out = SDIO_HOST_INIT_TIMEOUT;
    while (SDIO_GetInternalClockStatus() == RESET)
    {
        time_out--;
        if (time_out == 0)
        {
            return SD_HOST_INTER_CLOCK_ERROR;
        }
    }

    /* Initialize the SDIO host peripheral */
    SDIO_InitTypeDef SDIO_InitStructure;
    SDIO_StructInit(&SDIO_InitStructure);
    SDIO_Init(&SDIO_InitStructure);

    /* Enable SD bus power */
    SDIO_SetBusPower(SDIO_PowerState_ON);

    return SD_OK;
}

/**
  * @brief  Wait CMD line idle and can issue next command.
  * @param  None.
  * @retval SD_Status: SD Card status.
  */
SD_Status SD_WaitCmdLineIdle(void)
{
    uint32_t time_out = 0xffffff;

    /* Check command inhibit(CMD) status */
    while (SDIO_GetFlagStatus(SDIO_FLAG_CMD_INHIBIT) == SET)
    {
        time_out--;
        if (time_out == 0)
        {
            return SD_SDIO_CMD_INHIBIT;
        }
    }

    return SD_OK;
}

/**
  * @brief  Reset command complete when get the end bit of the command response(Except Auto CMD12).
  * @param  None.
  * @retval SD_Status: SD Card status.
  */
SD_Status SD_WaitCmdDatComplete(void)
{
    uint32_t time_out = 0xffffffff;

    /* Check command inhibit(DAT) status */
    while (SDIO_GetFlagStatus(SDIO_FLAG_CMD_DAT_INHIBIT) == SET)
    {
        time_out--;
        if (time_out == 0)
        {
            return SD_SDIO_CMD_COMPLETE;
        }
    }
    return SD_OK;
}

/**
  * @brief  Check command index in response data.
  * @param  None.
  * @retval SD_Status: SD status.
  */
SD_Status SD_CheckCmd(uint32_t cmd)
{
    if ((SDIO_GetResponse(SDIO_RSP2) & 0xFF) != cmd)
    {
        return SD_CMD_ERROR;
    }

    return SD_OK;
}

/**
  * @brief  Reset SD card.
  * @param  None.
  * @retval SD_Status: SD Card status.
  */
SD_Status SD_ResetCard(void)
{
    /* Check command inhibit status */
    if (SD_WaitCmdLineIdle() != SD_OK)
    {
        return SD_CMD_INHIBIT_BEFORE_RESET;
    }

    /* CMD0: GO_IDLE_STATE */
    /* No response required */
    SDIO_CmdInitTypeDef SDIO_CmdInitStructure;
    SDIO_CmdInitStructure.SDIO_Argument     = 0;
    SDIO_CmdInitStructure.SDIO_CmdIndex     = SDIO_GO_IDLE_STATE;
    SDIO_CmdInitStructure.SDIO_CmdType      = NORMAL;
    SDIO_CmdInitStructure.SDIO_DataPresent  = NO_DATA;
    SDIO_CmdInitStructure.SDIO_CmdIdxCheck  = DISABLE;
    SDIO_CmdInitStructure.SDIO_CmdCrcCheck  = DISABLE;
    SDIO_CmdInitStructure.SDIO_ResponseType = SDIO_Response_No;
    SDIO_SendCommand(&SDIO_CmdInitStructure);

    return SD_OK;
}

/**
  * @brief  Checks for error conditions for R1.
  * @param  None.
  * @retval SD_Status: SD Card status.
  */
static SD_Status SD_CheckResp1(uint32_t cmd)
{
    uint32_t timeout = SDIO_CMDTIMEOUT;
    SD_Status sd_status = SD_OK;

    /* Check status */
    while (timeout > 0)
    {
        /* Get command timeout error status which has higher priority than SDIO_INT_CMD_CMPL */
        if (SDIO_GetErrorINTStatus(SDIO_INT_CMD_TIMEOUT_ERR) == SET)
        {
            SDIO_ClearErrorINTPendingBit(SDIO_INT_CMD_TIMEOUT_ERR);
            return SD_CMD_RSP_TIMEOUT;
        }

        /*  Get transfer complete status */
        if (SDIO_GetINTStatus(SDIO_INT_CMD_CMPL) == SET)
        {
            SDIO_ClearINTPendingBit(SDIO_INT_CMD_CMPL);
            break;
        }

        /* Get command CRC error status */
        if (SDIO_GetErrorINTStatus(SDIO_INT_CMD_CRC_ERR) == SET)
        {
            SDIO_ClearErrorINTPendingBit(SDIO_INT_CMD_CRC_ERR);
            return SD_CMD_CRC_FAIL;
        }
        timeout--;
    }

    /* Command response timeout */
    if (timeout == 0)
    {
        return SD_CMD_RSP_TIMEOUT;
    }

    /* Check response received is of desired command */
    if (SD_CheckCmd(cmd) != SD_OK)
    {
        return SD_CMD_ERROR;
    }

    /* Check card status */
    uint32_t response = SDIO_GetResponse(SDIO_RSP0);

    /* Check error or not */
    if ((response & SD_OCR_ERRORBITS) == SD_ALLZERO)
    {
        return sd_status;
    }

    /* Check specified error */
    if (response & SD_OCR_ADDR_OUT_OF_RANGE)
    {
        return SD_ADDR_OUT_OF_RANGE;
    }

    if (response & SD_OCR_ADDR_MISALIGNED)
    {
        return SD_ADDR_MISALIGNED;
    }

    if (response & SD_OCR_BLOCK_LEN_ERR)
    {
        return SD_BLOCK_LEN_ERR;
    }

    if (response & SD_OCR_ERASE_SEQ_ERR)
    {
        return SD_ERASE_SEQ_ERR;
    }

    if (response & SD_OCR_BAD_ERASE_PARAM)
    {
        return SD_BAD_ERASE_PARAM;
    }

    if (response & SD_OCR_WRITE_PROT_VIOLATION)
    {
        return SD_WRITE_PROT_VIOLATION;
    }

    if (response & SD_OCR_LOCK_UNLOCK_FAILED)
    {
        return SD_LOCK_UNLOCK_FAILED;
    }

    if (response & SD_OCR_COM_CRC_FAILED)
    {
        return SD_COM_CRC_FAILED;
    }

    if (response & SD_OCR_ILLEGAL_CMD)
    {
        return SD_ILLEGAL_CMD;
    }

    if (response & SD_OCR_CARD_ECC_FAILED)
    {
        return SD_CARD_ECC_FAILED;
    }

    if (response & SD_OCR_CC_ERROR)
    {
        return SD_CC_ERROR;
    }

    if (response & SD_OCR_GENERAL_UNKNOWN_ERROR)
    {
        return SD_GENERAL_UNKNOWN_ERROR;
    }

    if (response & SD_OCR_CID_CSD_OVERWRIETE)
    {
        return SD_CID_CSD_OVERWRITE;
    }

    if (response & SD_OCR_WP_ERASE_SKIP)
    {
        return SD_WP_ERASE_SKIP;
    }

    if (response & SD_OCR_CARD_ECC_DISABLED)
    {
        return SD_CARD_ECC_DISABLED;
    }

    if (response & SD_OCR_ERASE_RESET)
    {
        return SD_ERASE_RESET;
    }

    if (response & SD_OCR_AKE_SEQ_ERROR)
    {
        return SD_AKE_SEQ_ERROR;
    }
    /*  The card will expect ACMD */
    if (!(response & SD_OCR_SD_APP_CMD))
    {
        return SD_APP_CMD;
    }

    return sd_status;
}

/**
  * @brief  Checks for error conditions for R7.
  * @param  None.
  * @retval SD_Status: SD Card status.
  */
static SD_Status SD_CheckResp7(void)
{
    uint32_t timeout = SDIO_CMDTIMEOUT;

    /* Check status */
    while (timeout > 0)
    {
        /* Get command timeout error status which has higher priority than SDIO_INT_CMD_CMPL */
        if (SDIO_GetErrorINTStatus(SDIO_INT_CMD_TIMEOUT_ERR) == SET)
        {
            SDIO_ClearErrorINTPendingBit(SDIO_INT_CMD_TIMEOUT_ERR);
            /* Card is not V2.0 complient or card does not support the set voltage range */
            return SD_CMD_RSP_TIMEOUT;
        }

        /*  Get transfer complete status */
        if (SDIO_GetINTStatus(SDIO_INT_CMD_CMPL) == SET)
        {
            /* Card is SD V2.0 compliant */
            SDIO_ClearINTPendingBit(SDIO_INT_CMD_CMPL);
            break;
        }

        /* Get command CRC error status */
        if (SDIO_GetErrorINTStatus(SDIO_INT_CMD_CRC_ERR) == SET)
        {
            SDIO_ClearErrorINTPendingBit(SDIO_INT_CMD_CRC_ERR);
            return SD_CMD_CRC_FAIL;
        }
        timeout--;
    }

    /* Command response timeout */
    if (timeout == 0)
    {
        return SD_CMD_RSP_TIMEOUT;
    }

    /* Check response */
    if (SD_CheckCmd(SDIO_SEND_IF_COND) != SD_OK)
    {
        return SD_CMD_ERROR;
    }
    /* Check Pattern error or not */
    if ((SDIO_GetResponse(SDIO_RSP0) & 0xff) != 0xAA)
    {
        return SD_CMD_ERROR;
    }
    /* Check Supply Voltage (VHS) 0x1 */
    if (!(SDIO_GetResponse(SDIO_RSP0) & BIT(8)))
    {
        return SD_CMD_ERROR;
    }

    SetCardType(SDIO_STD_CAPACITY_SD_CARD_V2_0);

    return SD_OK;
}

/**
  * @brief  Check voltage.
  * @param  None.
  * @retval SD_Status: SD Card status.
  */
SD_Status SD_CheckVoltage(void)
{
    SDIO_CmdInitTypeDef SDIO_CmdInitStructure;
    /* SD status */
    SD_Status sd_status = SD_OK;

    /* Check command inhibit status */
    if (SD_WaitCmdLineIdle() != SD_OK)
    {
        return SD_SDIO_CMD_INHIBIT;
    }

    /* CMD8: SDIO_SEND_IF_COND */
    /* Send CMD8 to verify SD card interface operating condition */
    /* Argument: - [31:12]: Reserved (shall be set to '0')
                    - [11:8]: Supply Voltage (VHS) 0x1 (Range: 2.7-3.6 V)
                    - [7:0]: Check Pattern (recommended 0xAA) */
    /* CMD Response: R7 */
    SDIO_CmdInitStructure.SDIO_Argument     = SD_CHECK_PATTERN;
    SDIO_CmdInitStructure.SDIO_CmdIndex     = SDIO_SEND_IF_COND;
    SDIO_CmdInitStructure.SDIO_CmdType      = NORMAL;
    SDIO_CmdInitStructure.SDIO_DataPresent  = NO_DATA;
    SDIO_CmdInitStructure.SDIO_CmdIdxCheck  = ENABLE;
    SDIO_CmdInitStructure.SDIO_CmdCrcCheck  = ENABLE;
    SDIO_CmdInitStructure.SDIO_ResponseType = RSP_LEN_48;
    SDIO_SendCommand(&SDIO_CmdInitStructure);

    /* Check response and status */
    if (SD_WaitCmdLineIdle() != SD_OK)
    {
        return SD_SDIO_CMD_INHIBIT;
    }
    sd_status = SD_CheckResp7();

    return sd_status;
}

/**
  * @brief  Send APP CMD.
  * @param  none.
  * @retval  SD_Status: SD status.
  */
SD_Status SD_SendAppCmd(uint32_t RCA)
{
    SDIO_CmdInitTypeDef SDIO_CmdInitStructure;

    /* CMD55, command response: R1 */
    SDIO_CmdInitStructure.SDIO_Argument = RCA;
    SDIO_CmdInitStructure.SDIO_CmdIndex = SDIO_APP_CMD;
    SDIO_CmdInitStructure.SDIO_CmdType  = NORMAL;
    SDIO_CmdInitStructure.SDIO_DataPresent = NO_DATA;
    SDIO_CmdInitStructure.SDIO_CmdIdxCheck = ENABLE;
    SDIO_CmdInitStructure.SDIO_CmdCrcCheck = ENABLE;
    SDIO_CmdInitStructure.SDIO_ResponseType = RSP_LEN_48;
    SDIO_SendCommand(&SDIO_CmdInitStructure);

    if (SD_WaitCmdLineIdle() != SD_OK)
    {
        return SD_SDIO_CMD_INHIBIT;
    }

    return SD_CheckResp1(SDIO_APP_CMD);
}

/**
  * @brief  Get OCR.
  * @param  none.
  * @retval  SD_Status: SD status.
  */
SD_Status SD_GetOCR(void)
{
    SDIO_CmdInitTypeDef SDIO_CmdInitStructure;
    SD_Status sd_status = SD_OK;

    uint32_t retry = (ACMD41_INIT_TIMEOUT / ACMD41_POLL_INTERVAL);
    uint32_t HostOCR = VDD_30_31 | VDD_31_32 | VDD_32_33 | VDD_33_34;
    uint32_t Hcs = SDHC_SUPPORT;

    /* Waiting for CMD line idle */
    if (SD_WaitCmdLineIdle() != SD_OK)
    {
        return SD_SDIO_CMD_INHIBIT;
    }

    /* Send SDIO_APP_CMD and check response */
    sd_status = SD_SendAppCmd(0);
    if (sd_status != SD_OK)
    {
        return sd_status;
    }

    /* Inquiry ACMD41 */
    SDIO_CmdInitStructure.SDIO_Argument     = 0;
    SDIO_CmdInitStructure.SDIO_CmdIndex     = SDIO_CMD_SEND_OP_COND;
    SDIO_CmdInitStructure.SDIO_CmdType      = NORMAL;
    SDIO_CmdInitStructure.SDIO_DataPresent  = NO_DATA;
    SDIO_CmdInitStructure.SDIO_CmdIdxCheck  = DISABLE;
    SDIO_CmdInitStructure.SDIO_CmdCrcCheck  = DISABLE;
    SDIO_CmdInitStructure.SDIO_ResponseType = RSP_LEN_48;
    SDIO_SendCommand(&SDIO_CmdInitStructure);

    /* Waiting for CMD line idle */
    if (SD_WaitCmdLineIdle() != SD_OK)
    {
        return SD_SDIO_CMD_INHIBIT;
    }
    uint32_t OCR = SDIO_GetResponse(SDIO_RSP0) & 0xFFFFFF;
    SDIO_GetResponse(SDIO_RSP2);

    while (--retry)
    {
        /* Send SDIO_APP_CMD and check response */
        sd_status = SD_SendAppCmd(0);
        if (sd_status != SD_OK)
        {
            return sd_status;
        }

        /* Inquiry ACMD41 */
        SDIO_CmdInitStructure.SDIO_Argument     = (Hcs << 30) | HostOCR;
        SDIO_CmdInitStructure.SDIO_CmdIndex     = SDIO_CMD_SEND_OP_COND;
        SDIO_CmdInitStructure.SDIO_CmdType      = NORMAL;
        SDIO_CmdInitStructure.SDIO_DataPresent  = NO_DATA;
        SDIO_CmdInitStructure.SDIO_CmdIdxCheck  = DISABLE;
        SDIO_CmdInitStructure.SDIO_CmdCrcCheck  = DISABLE;
        SDIO_CmdInitStructure.SDIO_ResponseType = RSP_LEN_48;
        SDIO_SendCommand(&SDIO_CmdInitStructure);

        /* Waiting for CMD line idle */
        if (SD_WaitCmdLineIdle() != SD_OK)
        {
            return SD_SDIO_CMD_INHIBIT;
        }
        /* Check SD card status */
        if (SDIO_GetResponse(SDIO_RSP0) & BIT(31))
        {
            break;
        }
        SDIO_GetResponse(SDIO_RSP2);
    }

    return sd_status;
}

/**
  * @brief  Get CID.
  * @param  none.
  * @retval  SD_Status: SD status.
  */
SD_Status SD_GetCID(uint32_t *pBuf)
{
    SDIO_CmdInitTypeDef SDIO_CmdInitStructure;

    /* Check command inhibit status */
    if (SD_WaitCmdLineIdle() != SD_OK)
    {
        return SD_SDIO_CMD_INHIBIT;
    }
    SDIO_CmdInitStructure.SDIO_Argument     = 0;
    SDIO_CmdInitStructure.SDIO_CmdIndex     = SDIO_ALL_SEND_CID;
    SDIO_CmdInitStructure.SDIO_CmdType      = NORMAL;
    SDIO_CmdInitStructure.SDIO_DataPresent  = NO_DATA;
    SDIO_CmdInitStructure.SDIO_CmdIdxCheck  = DISABLE;
    SDIO_CmdInitStructure.SDIO_CmdCrcCheck  = ENABLE;
    SDIO_CmdInitStructure.SDIO_ResponseType = RSP_LEN_136;
    SDIO_SendCommand(&SDIO_CmdInitStructure);

    /* Waiting for CMD line idle */
    if (SD_WaitCmdLineIdle() != SD_OK)
    {
        return SD_SDIO_CMD_INHIBIT;
    }

    /* Store CID */
    *pBuf++ = SDIO_GetResponse(SDIO_RSP0);
    *pBuf++ = SDIO_GetResponse(SDIO_RSP2);
    *pBuf++ = SDIO_GetResponse(SDIO_RSP4);
    *pBuf   = SDIO_GetResponse(SDIO_RSP6);

    return SD_OK;
}

/**
  * @brief  Get RCA.
  * @param  pRCA: address store RCA when function return.
  * @retval  SD_Status: SD status.
  */
SD_Status SD_GetRCA(uint32_t *pRCA)
{
    SDIO_CmdInitTypeDef SDIO_CmdInitStructure;

    /* Check command inhibit status */
    if (SD_WaitCmdLineIdle() != SD_OK)
    {
        return SD_SDIO_CMD_INHIBIT;
    }
    SDIO_CmdInitStructure.SDIO_Argument     = 0;
    SDIO_CmdInitStructure.SDIO_CmdIndex     = SDIO_SET_REL_ADDR;
    SDIO_CmdInitStructure.SDIO_CmdType      = NORMAL;
    SDIO_CmdInitStructure.SDIO_DataPresent  = NO_DATA;
    SDIO_CmdInitStructure.SDIO_CmdIdxCheck  = ENABLE;
    SDIO_CmdInitStructure.SDIO_CmdCrcCheck  = ENABLE;
    SDIO_CmdInitStructure.SDIO_ResponseType = RSP_LEN_48;
    SDIO_SendCommand(&SDIO_CmdInitStructure);

    /* Check command inhibit status */
    if (SD_WaitCmdLineIdle() != SD_OK)
    {
        return SD_SDIO_CMD_INHIBIT_AFTER_RESET;
    }
    if (SD_CheckCmd(SDIO_SET_REL_ADDR) != SD_OK)
    {
        return SD_CMD_ERROR;
    }
    /* Get RCA */
    *pRCA = SDIO_GetResponse(SDIO_RSP0) >> 16;

    return SD_OK;
}

/**
  * @brief  Get CSD.
  * @param  RCA: relative acrd address.
  * @param  pBuf: address store CSD when function return.
  * @retval  SD_Status: SD status.
  */
SD_Status SD_GetCSD(uint32_t RCA, uint32_t *pBuf)
{
    SDIO_CmdInitTypeDef SDIO_CmdInitStructure;

    /* Check command inhibit status */
    if (SD_WaitCmdLineIdle() != SD_OK)
    {
        return SD_SDIO_CMD_INHIBIT;
    }
    SDIO_CmdInitStructure.SDIO_Argument     = RCA << 16;
    SDIO_CmdInitStructure.SDIO_CmdIndex     = SDIO_SEND_CSD;
    SDIO_CmdInitStructure.SDIO_CmdType      = NORMAL;
    SDIO_CmdInitStructure.SDIO_DataPresent  = NO_DATA;
    SDIO_CmdInitStructure.SDIO_CmdIdxCheck  = DISABLE;
    SDIO_CmdInitStructure.SDIO_CmdCrcCheck  = ENABLE;
    SDIO_CmdInitStructure.SDIO_ResponseType = RSP_LEN_136;
    SDIO_SendCommand(&SDIO_CmdInitStructure);
    /* Check command inhibit status */
    if (SD_WaitCmdLineIdle() != SD_OK)
    {
        return SD_SDIO_CMD_INHIBIT;
    }

    /* Get csd */
    *pBuf++ = SDIO_GetResponse(SDIO_RSP0);
    *pBuf++ = SDIO_GetResponse(SDIO_RSP2);
    *pBuf++ = SDIO_GetResponse(SDIO_RSP4);
    *pBuf   = SDIO_GetResponse(SDIO_RSP6);

    return SD_OK;
}

/**
  * @brief  Select or deselect the SD card.
  * @param  addr: RCA.
  * @param  NewState: new state of the selection.
  *   This parameter can be: ENABLE or DISABLE.
  *   --ENABLE: select
  *   --DISABLE: deselect
  * @retval  SD_Status: SD Card status.
  */
SD_Status SD_SelectCmd(uint32_t addr, FunctionalState NewState)
{
    SDIO_CmdInitTypeDef SDIO_CmdInitStructure;

    /* Check the parameters */
    assert_param(IS_FUNCTIONAL_STATE(NewState));

    /* Waiting for CMD line idle */
    if (SD_WaitCmdLineIdle() != SD_OK)
    {
        return SD_SDIO_CMD_INHIBIT;
    }

    /* Send CMD7 */
    SDIO_CmdInitStructure.SDIO_Argument     = addr << 16;
    SDIO_CmdInitStructure.SDIO_CmdIndex     = SDIO_SEL_DESEL_CARD;
    SDIO_CmdInitStructure.SDIO_CmdType      = NORMAL;
    SDIO_CmdInitStructure.SDIO_DataPresent  = NO_DATA;
    if (NewState != DISABLE)
    {
        SDIO_CmdInitStructure.SDIO_CmdIdxCheck  = ENABLE;
        SDIO_CmdInitStructure.SDIO_CmdCrcCheck  = ENABLE;
        SDIO_CmdInitStructure.SDIO_ResponseType = RSP_LEN_48_CHK_BUSY;
    }
    else
    {
        SDIO_CmdInitStructure.SDIO_CmdIdxCheck  = DISABLE;
        SDIO_CmdInitStructure.SDIO_CmdCrcCheck  = DISABLE;
        SDIO_CmdInitStructure.SDIO_ResponseType = SDIO_Response_No;
    }
    SDIO_SendCommand(&SDIO_CmdInitStructure);

    /* Waiting for CMD line idle */
    if (SD_WaitCmdLineIdle() != SD_OK)
    {
        return SD_SDIO_CMD_INHIBIT_AFTER_RESET;
    }
    /* Check response */
    if (NewState != DISABLE)
    {
        if (SD_CheckCmd(SDIO_SEL_DESEL_CARD) != SD_OK)
        {
            return SD_CMD_ERROR;
        }
    }

    return SD_OK;
}

/**
  * @brief  Configure bus width.
  * @param  RCA: relative card address.
  * @retval SD_Status: SD status.
  */
SD_Status SD_SetBusWide(uint32_t RCA, uint32_t SDIO_BusWide)
{
    SDIO_CmdInitTypeDef SDIO_CmdInitStructure;
    SD_Status sd_status = SD_OK;

    /* Waiting for CMD line idle */
    if (SD_WaitCmdLineIdle() != SD_OK)
    {
        return SD_SDIO_CMD_INHIBIT;
    }

    /* CMD55 , CMD Response: R1 */
    sd_status = SD_SendAppCmd(RCA << 16);
    if (sd_status != SD_OK)
    {
        return sd_status;
    }

    /* CMD6 */
    SDIO_CmdInitStructure.SDIO_Argument     = SDIO_BusWide;
    SDIO_CmdInitStructure.SDIO_CmdIndex     = SDIO_HS_SWITCH;
    SDIO_CmdInitStructure.SDIO_CmdType      = NORMAL;
    SDIO_CmdInitStructure.SDIO_DataPresent  = NO_DATA;
    SDIO_CmdInitStructure.SDIO_CmdIdxCheck  = ENABLE;
    SDIO_CmdInitStructure.SDIO_CmdCrcCheck  = ENABLE;
    SDIO_CmdInitStructure.SDIO_ResponseType = RSP_LEN_48;
    SDIO_SendCommand(&SDIO_CmdInitStructure);

    /* Waiting for CMD line idle */
    if (SD_WaitCmdLineIdle() != SD_OK)
    {
        return SD_SDIO_CMD_INHIBIT;
    }
    /* Check response */
    if (SD_CheckCmd(SDIO_HS_SWITCH) != SD_OK)
    {
        return SD_CMD_ERROR;
    }
    if (!(SDIO_GetResponse(SDIO_RSP0) & BIT(5)))
    {
        return SD_APP_CMD;
    }

    /* Set host controller bus width */
    SDIO_SetBusWide(SDIO_BusWide_4b);

    return SD_OK;
}

/**
  * @brief  Get Card current status.
  * @param  RCA: relative acrd address.
  * @param  pBuf: address store current status when function return.
  * @retval  SD_Status: SD status.
  */
SD_Status SD_GetCardStatus(uint32_t RCA, uint32_t *pStatus)
{
    SDIO_CmdInitTypeDef SDIO_CmdInitStructure;

    /* Check command inhibit status */
    if (SD_WaitCmdLineIdle() != SD_OK)
    {
        return SD_SDIO_CMD_INHIBIT;
    }
    /* Get SD card current status */
    SDIO_CmdInitStructure.SDIO_Argument     = RCA << 16;
    SDIO_CmdInitStructure.SDIO_CmdIndex     = SDIO_SEND_STATUS;
    SDIO_CmdInitStructure.SDIO_CmdType      = NORMAL;
    SDIO_CmdInitStructure.SDIO_DataPresent  = NO_DATA;
    SDIO_CmdInitStructure.SDIO_CmdIdxCheck  = ENABLE;
    SDIO_CmdInitStructure.SDIO_CmdCrcCheck  = ENABLE;
    SDIO_CmdInitStructure.SDIO_ResponseType = RSP_LEN_48;
    SDIO_SendCommand(&SDIO_CmdInitStructure);

    /* Check command inhibit status */
    if (SD_WaitCmdLineIdle() != SD_OK)
    {
        return SD_SDIO_CMD_INHIBIT;
    }
    if (SD_CheckCmd(SDIO_SEND_STATUS) != SD_OK)
    {
        return SD_CMD_ERROR;
    }
    *pStatus = SDIO_GetResponse(SDIO_RSP0);

    return SD_OK;
}

/**
  * @brief  Checks if the SD card is in programming state.
  * @param  RCA: relative acrd address.
  * @param  pStatus: address store current status of card.
  * @retval  SD_Status: SD status.
  */
SD_Status SD_WaitCardFinishProgram(uint32_t RCA)
{
    SD_Status sd_status = SD_OK;
    uint32_t timeout = SDIO_PROG_TIMEOUT;
    uint32_t card_status = 0;

    /* Get card status */
    sd_status = SD_GetCardStatus(RCA, &card_status);
    /* Find out programming status */
    card_status = (card_status >> 9) & 0x0F;

    /* Wait finish */
    while ((sd_status == SD_OK) && ((sd_status == SD_CARD_PROGRAMMING) ||
                                    (sd_status == SD_CARD_RECEIVING)))
    {
        /* Get card status */
        sd_status = SD_GetCardStatus(RCA, &card_status);
        /* Find out programming status */
        card_status = (card_status >> 9) & 0x0F;

        timeout--;
        if (timeout == 0)
        {
            return SD_PROG_TIMEOUT;
        }
    }

    return sd_status;
}

/**
  * @brief  Returns information about specific card.
  * @param  cardinfo : pointer to a SD_CardInfo structure.
  * @retval  SD_Status: SD status.
  */
SD_Status SD_GetCardInfo(SD_CardInfo *cardinfo)
{
    SD_Status sd_status = SD_OK;
    uint8_t tmp = 0;

    cardinfo->CardType = (uint8_t)CardType;
    cardinfo->RCA = (uint16_t)RCA;

    /* Byte 0 */
    tmp = (uint8_t)((CSD_Tab[0] & 0xFF000000) >> 24);
    cardinfo->SD_csd.CSDStruct = (tmp & 0xC0) >> 6;
    cardinfo->SD_csd.SysSpecVersion = (tmp & 0x3C) >> 2;
    cardinfo->SD_csd.Reserved1 = tmp & 0x03;

    /* Byte 1 */
    tmp = (uint8_t)((CSD_Tab[0] & 0x00FF0000) >> 16);
    cardinfo->SD_csd.TAAC = tmp;

    /* Byte 2 */
    tmp = (uint8_t)((CSD_Tab[0] & 0x0000FF00) >> 8);
    cardinfo->SD_csd.NSAC = tmp;

    /* Byte 3 */
    tmp = (uint8_t)(CSD_Tab[0] & 0x000000FF);
    cardinfo->SD_csd.MaxBusClkFrec = tmp;

    /* Byte 4 */
    tmp = (uint8_t)((CSD_Tab[1] & 0xFF000000) >> 24);
    cardinfo->SD_csd.CardComdClasses = tmp << 4;

    /* Byte 5 */
    tmp = (uint8_t)((CSD_Tab[1] & 0x00FF0000) >> 16);
    cardinfo->SD_csd.CardComdClasses |= (tmp & 0xF0) >> 4;
    cardinfo->SD_csd.RdBlockLen = tmp & 0x0F;

    /* Byte 6 */
    tmp = (uint8_t)((CSD_Tab[1] & 0x0000FF00) >> 8);
    cardinfo->SD_csd.PartBlockRead = (tmp & 0x80) >> 7;
    cardinfo->SD_csd.WrBlockMisalign = (tmp & 0x40) >> 6;
    cardinfo->SD_csd.RdBlockMisalign = (tmp & 0x20) >> 5;
    cardinfo->SD_csd.DSRImpl = (tmp & 0x10) >> 4;
    cardinfo->SD_csd.Reserved2 = 0; /* Reserved */

    if ((CardType == SDIO_STD_CAPACITY_SD_CARD_V1_1) || (CardType == SDIO_STD_CAPACITY_SD_CARD_V2_0))
    {
        cardinfo->SD_csd.DeviceSize = (tmp & 0x03) << 10;

        /* Byte 7 */
        tmp = (uint8_t)(CSD_Tab[1] & 0x000000FF);
        cardinfo->SD_csd.DeviceSize |= (tmp) << 2;

        /* Byte 8 */
        tmp = (uint8_t)((CSD_Tab[2] & 0xFF000000) >> 24);
        cardinfo->SD_csd.DeviceSize |= (tmp & 0xC0) >> 6;

        cardinfo->SD_csd.MaxRdCurrentVDDMin = (tmp & 0x38) >> 3;
        cardinfo->SD_csd.MaxRdCurrentVDDMax = (tmp & 0x07);

        /* Byte 9 */
        tmp = (uint8_t)((CSD_Tab[2] & 0x00FF0000) >> 16);
        cardinfo->SD_csd.MaxWrCurrentVDDMin = (tmp & 0xE0) >> 5;
        cardinfo->SD_csd.MaxWrCurrentVDDMax = (tmp & 0x1C) >> 2;
        cardinfo->SD_csd.DeviceSizeMul = (tmp & 0x03) << 1;
        /* Byte 10 */
        tmp = (uint8_t)((CSD_Tab[2] & 0x0000FF00) >> 8);
        cardinfo->SD_csd.DeviceSizeMul |= (tmp & 0x80) >> 7;

        cardinfo->CardCapacity = (cardinfo->SD_csd.DeviceSize + 1) ;
        cardinfo->CardCapacity *= (1 << (cardinfo->SD_csd.DeviceSizeMul + 2));
        cardinfo->CardBlockSize = 1 << (cardinfo->SD_csd.RdBlockLen);
        cardinfo->CardCapacity *= cardinfo->CardBlockSize;
    }
    else if (CardType == SDIO_HIGH_CAPACITY_SD_CARD)
    {
        /* Byte 7 */
        tmp = (uint8_t)(CSD_Tab[1] & 0x000000FF);
        cardinfo->SD_csd.DeviceSize = (tmp & 0x3F) << 16;

        /* Byte 8 */
        tmp = (uint8_t)((CSD_Tab[2] & 0xFF000000) >> 24);

        cardinfo->SD_csd.DeviceSize |= (tmp << 8);

        /* Byte 9 */
        tmp = (uint8_t)((CSD_Tab[2] & 0x00FF0000) >> 16);

        cardinfo->SD_csd.DeviceSize |= (tmp);

        /* Byte 10 */
        tmp = (uint8_t)((CSD_Tab[2] & 0x0000FF00) >> 8);

        cardinfo->CardCapacity = (cardinfo->SD_csd.DeviceSize + 1) * 512 * 1024;
        cardinfo->CardBlockSize = 512;
    }


    cardinfo->SD_csd.EraseGrSize = (tmp & 0x40) >> 6;
    cardinfo->SD_csd.EraseGrMul = (tmp & 0x3F) << 1;

    /* Byte 11 */
    tmp = (uint8_t)(CSD_Tab[2] & 0x000000FF);
    cardinfo->SD_csd.EraseGrMul |= (tmp & 0x80) >> 7;
    cardinfo->SD_csd.WrProtectGrSize = (tmp & 0x7F);

    /* Byte 12 */
    tmp = (uint8_t)((CSD_Tab[3] & 0xFF000000) >> 24);
    cardinfo->SD_csd.WrProtectGrEnable = (tmp & 0x80) >> 7;
    cardinfo->SD_csd.ManDeflECC = (tmp & 0x60) >> 5;
    cardinfo->SD_csd.WrSpeedFact = (tmp & 0x1C) >> 2;
    cardinfo->SD_csd.MaxWrBlockLen = (tmp & 0x03) << 2;

    /* Byte 13 */
    tmp = (uint8_t)((CSD_Tab[3] & 0x00FF0000) >> 16);
    cardinfo->SD_csd.MaxWrBlockLen |= (tmp & 0xC0) >> 6;
    cardinfo->SD_csd.WriteBlockPaPartial = (tmp & 0x20) >> 5;
    cardinfo->SD_csd.Reserved3 = 0;
    cardinfo->SD_csd.ContentProtectAppli = (tmp & 0x01);

    /* Byte 14 */
    tmp = (uint8_t)((CSD_Tab[3] & 0x0000FF00) >> 8);
    cardinfo->SD_csd.FileFormatGrouop = (tmp & 0x80) >> 7;
    cardinfo->SD_csd.CopyFlag = (tmp & 0x40) >> 6;
    cardinfo->SD_csd.PermWrProtect = (tmp & 0x20) >> 5;
    cardinfo->SD_csd.TempWrProtect = (tmp & 0x10) >> 4;
    cardinfo->SD_csd.FileFormat = (tmp & 0x0C) >> 2;
    cardinfo->SD_csd.ECC = (tmp & 0x03);

    /* Byte 15 */
    tmp = (uint8_t)(CSD_Tab[3] & 0x000000FF);
    cardinfo->SD_csd.CSD_CRC = (tmp & 0xFE) >> 1;
    cardinfo->SD_csd.Reserved4 = 1;


    /* Byte 0 */
    tmp = (uint8_t)((CID_Tab[0] & 0xFF000000) >> 24);
    cardinfo->SD_cid.ManufacturerID = tmp;

    /* Byte 1 */
    tmp = (uint8_t)((CID_Tab[0] & 0x00FF0000) >> 16);
    cardinfo->SD_cid.OEM_AppliID = tmp << 8;

    /* Byte 2 */
    tmp = (uint8_t)((CID_Tab[0] & 0x000000FF00) >> 8);
    cardinfo->SD_cid.OEM_AppliID |= tmp;

    /* Byte 3 */
    tmp = (uint8_t)(CID_Tab[0] & 0x000000FF);
    cardinfo->SD_cid.ProdName1 = tmp << 24;

    /* Byte 4 */
    tmp = (uint8_t)((CID_Tab[1] & 0xFF000000) >> 24);
    cardinfo->SD_cid.ProdName1 |= tmp << 16;

    /* Byte 5 */
    tmp = (uint8_t)((CID_Tab[1] & 0x00FF0000) >> 16);
    cardinfo->SD_cid.ProdName1 |= tmp << 8;

    /* Byte 6 */
    tmp = (uint8_t)((CID_Tab[1] & 0x0000FF00) >> 8);
    cardinfo->SD_cid.ProdName1 |= tmp;

    /* Byte 7 */
    tmp = (uint8_t)(CID_Tab[1] & 0x000000FF);
    cardinfo->SD_cid.ProdName2 = tmp;

    /* Byte 8 */
    tmp = (uint8_t)((CID_Tab[2] & 0xFF000000) >> 24);
    cardinfo->SD_cid.ProdRev = tmp;

    /* Byte 9 */
    tmp = (uint8_t)((CID_Tab[2] & 0x00FF0000) >> 16);
    cardinfo->SD_cid.ProdSN = tmp << 24;

    /* Byte 10 */
    tmp = (uint8_t)((CID_Tab[2] & 0x0000FF00) >> 8);
    cardinfo->SD_cid.ProdSN |= tmp << 16;

    /* Byte 11 */
    tmp = (uint8_t)(CID_Tab[2] & 0x000000FF);
    cardinfo->SD_cid.ProdSN |= tmp << 8;

    /* Byte 12 */
    tmp = (uint8_t)((CID_Tab[3] & 0xFF000000) >> 24);
    cardinfo->SD_cid.ProdSN |= tmp;

    /* Byte 13 */
    tmp = (uint8_t)((CID_Tab[3] & 0x00FF0000) >> 16);
    cardinfo->SD_cid.Reserved1 |= (tmp & 0xF0) >> 4;
    cardinfo->SD_cid.ManufactDate = (tmp & 0x0F) << 8;

    /* Byte 14 */
    tmp = (uint8_t)((CID_Tab[3] & 0x0000FF00) >> 8);
    cardinfo->SD_cid.ManufactDate |= tmp;

    /* Byte 15 */
    tmp = (uint8_t)(CID_Tab[3] & 0x000000FF);
    cardinfo->SD_cid.CID_CRC = (tmp & 0xFE) >> 1;
    cardinfo->SD_cid.Reserved2 = 1;

    return sd_status;
}

/**
  * @brief  Initial SD card.
  * @param  None.
  * @retval SD_Status: SD Card status.
  */
SD_Status SD_InitCard(void)
{
    SD_Status sd_status = SD_OK;

    /* Enable SDIO interrupt status and error status */
    SDIO_INTStatusConfig(SDIO_INT_CMD_CMPL, ENABLE);
    SDIO_ErrrorINTStatusConfig(SDIO_INT_CMD_TIMEOUT_ERR | SDIO_INT_CMD_CRC_ERR, ENABLE);
    /* Disable SDIO interrupt status and error status */
    //SDIO_INTStatusConfig(SDIO_INT_CMD_CMPL, DISABLE);
    //SDIO_ErrrorINTStatusConfig(SDIO_INT_CMD_TIMEOUT_ERR|SDIO_INT_CMD_CRC_ERR, DISABLE);

    /* CMD0: GO_IDLE_STATE */
    sd_status = SD_ResetCard();
    if (sd_status != SD_OK)
    {
        return sd_status;
    }

    /* CMD8: SEND_IF_COND */
    sd_status = SD_CheckVoltage();
    if (sd_status != SD_OK)
    {
        return sd_status;
    }

    /* CMD41: Get OCR and configure it */
    sd_status = SD_GetOCR();
    if (sd_status != SD_OK)
    {
        return sd_status;
    }

    //delayUS(20);

    /* CMD2: Get CID */
    sd_status = SD_GetCID(&CID_Tab[0]);
    if (sd_status != SD_OK)
    {
        return sd_status;
    }

    /* CMD3: Get RCA */
    sd_status = SD_GetRCA(&RCA);
    if (sd_status != SD_OK)
    {
        return sd_status;
    }

    /* CMD9: Get CSD */
    sd_status = SD_GetCSD(RCA, &CSD_Tab[0]);
    if (sd_status != SD_OK)
    {
        return sd_status;
    }

    /* CMD7 is uesd to put card into the transfer state */
    sd_status = SD_SelectCmd(RCA, ENABLE);
    if (sd_status != SD_OK)
    {
        return sd_status;
    }

    /* CMD6 : Set to 4-bit mode */
    sd_status = SD_SetBusWide(RCA, SDIO_BusWide_4b);
    if (sd_status != SD_OK)
    {
        return sd_status;
    }

#if 0
    /* CMD13 */
    uint32_t CardCurState = 0;
    sd_status = SD_GetCardStatus(RCA, &CardCurState);
    if (sd_status != SD_OK)
    {
        return sd_status;
    }
#endif
    /* Get SCR */
    static uint32_t SCR[8];
    sd_status = SD_GetSCR(RCA, SCR);

    if (sd_status != SD_OK)
    {
        return sd_status;
    }

    return sd_status;
}

/**
  * @brief  Initializes the SD Card and Set it into StandBy State.
  * @param  None.
  * @retval SD_Error: SD Card Error code.
  */
SD_Status SD_Init(void)
{
    /* SD status */
    SD_Status sd_status = SD_OK;

    /* Enable SDIO clock */
    RCC_PeriphClockCmd(APBPeriph_SD_HOST, APBPeriph_SD_HOST_CLOCK, ENABLE);

    /* Power on SD host */
    SD_InitHost();

    /* Initialize SD card */
    sd_status = SD_InitCard();

    /* Initialize the SDIO host peripheral */
    SDIO_InitTypeDef SDIO_InitStructure;
    SDIO_InitStructure.SDIO_TimeOut = 0x0E; /* !< TMCLK * 2^SDIO_TimeOut */
    SDIO_InitStructure.SDIO_BusWide = SDIO_BusWide_4b;
    SDIO_InitStructure.SDIO_ClockDiv = SDIO_CLOCK_DIV_8;
    SDIO_Init(&SDIO_InitStructure);

    return sd_status;
}

/**
  * @brief  Read one block from a specified address in a card.
  * @param  addr: Address from where data are to be read
  * @param  pDescriptorTab: pointer to the buffer that will contain the received data
  * @param  blockSize: the SD card Data block size
  * @retval  SD_Error: SD Card Error code.
  */
SD_Status SD_ReadSingleBlock(uint32_t addr, uint32_t buf, uint16_t blockSize)
{
    SDIO_CmdInitTypeDef SDIO_CmdInitStructure;
    SDIO_DataInitTypeDef SDIO_DataInitStruct;
    SD_Status sd_status = SD_OK;

    if (NULL == buf)
    {
        sd_status = SD_INVALID_PARAMETER;
        return sd_status;
    }

    /* Data Configure */
    SDIO_ADMA2_DescTab[0].SDIO_Address              = (uint32_t)buf;
    SDIO_ADMA2_DescTab[0].SDIO_Length               =   blockSize;
    SDIO_ADMA2_DescTab[0].SDIO_Attribute.SDIO_Valid =   1;
    SDIO_ADMA2_DescTab[0].SDIO_Attribute.SDIO_End   =   1;
    SDIO_ADMA2_DescTab[0].SDIO_Attribute.SDIO_Int   =   0;
    SDIO_ADMA2_DescTab[0].SDIO_Attribute.SDIO_Act1  =   0;
    SDIO_ADMA2_DescTab[0].SDIO_Attribute.SDIO_Act2  =   1;

    SDIO_DataStructInit(&SDIO_DataInitStruct);
    SDIO_DataInitStruct.SDIO_Address        = (uint32_t)SDIO_ADMA2_DescTab;
    SDIO_DataInitStruct.SDIO_BlockCount     = 1;
    SDIO_DataInitStruct.SDIO_BlockSize      = blockSize;
    SDIO_DataInitStruct.SDIO_TransferDir    = SDIO_TransferDir_READ;
    SDIO_DataInitStruct.SDIO_DMAEn          = ENABLE;
    SDIO_DataConfig(&SDIO_DataInitStruct);

    /* CMD17 */
    SDIO_CmdInitStructure.SDIO_Argument     = addr;
    SDIO_CmdInitStructure.SDIO_CmdIndex     = SDIO_READ_SINGLE_BLOCK;
    SDIO_CmdInitStructure.SDIO_CmdType      = NORMAL;
    SDIO_CmdInitStructure.SDIO_DataPresent  = WITH_DATA;
    SDIO_CmdInitStructure.SDIO_CmdIdxCheck  = ENABLE;
    SDIO_CmdInitStructure.SDIO_CmdCrcCheck  = ENABLE;
    SDIO_CmdInitStructure.SDIO_ResponseType = RSP_LEN_48;
    SDIO_SendCommand(&SDIO_CmdInitStructure);

    sd_status = SD_WaitCmdDatComplete();

    return sd_status;
}

/**
  * @brief  Read multiple block from a specified address in a card.
  * @param  addr: Address from where data are to be read
  * @param  pDescriptorTab: pointer to the buffer that will contain the received data
  * @param  blockSize: the SD card Data block size
  * @retval  SD_Error: SD Card Error code.
  */
SD_Status SD_ReadMultiBlock(uint32_t addr, uint32_t buf, uint16_t blockSize, uint16_t blockCount)
{
    SDIO_CmdInitTypeDef SDIO_CmdInitStructure;
    SDIO_DataInitTypeDef SDIO_DataInitStruct;
    SD_Status sd_status = SD_OK;

    if (NULL == buf)
    {
        sd_status = SD_INVALID_PARAMETER;
        return sd_status;
    }

    /* Data Configure */
    SDIO_ADMA2_DescTab[0].SDIO_Address              = (uint32_t)buf;
    SDIO_ADMA2_DescTab[0].SDIO_Length               =   blockSize * blockCount;
    SDIO_ADMA2_DescTab[0].SDIO_Attribute.SDIO_Valid =   1;
    SDIO_ADMA2_DescTab[0].SDIO_Attribute.SDIO_End   =   1;
    SDIO_ADMA2_DescTab[0].SDIO_Attribute.SDIO_Int   =   0;
    SDIO_ADMA2_DescTab[0].SDIO_Attribute.SDIO_Act1  =   0;
    SDIO_ADMA2_DescTab[0].SDIO_Attribute.SDIO_Act2  =   1;

    SDIO_DataStructInit(&SDIO_DataInitStruct);
    SDIO_DataInitStruct.SDIO_Address        = (uint32_t)SDIO_ADMA2_DescTab;
    SDIO_DataInitStruct.SDIO_BlockCount     = blockCount;
    SDIO_DataInitStruct.SDIO_BlockSize      = blockSize;
    SDIO_DataInitStruct.SDIO_TransferDir    = SDIO_TransferDir_READ;
    SDIO_DataInitStruct.SDIO_DMAEn          = ENABLE;
    SDIO_DataConfig(&SDIO_DataInitStruct);

    /* CMD18 */
    SDIO_CmdInitStructure.SDIO_Argument     = addr;
    SDIO_CmdInitStructure.SDIO_CmdIndex     = SDIO_READ_MULT_BLOCK;
    SDIO_CmdInitStructure.SDIO_CmdType      = NORMAL;
    SDIO_CmdInitStructure.SDIO_DataPresent  = WITH_DATA;
    SDIO_CmdInitStructure.SDIO_CmdIdxCheck  = ENABLE;
    SDIO_CmdInitStructure.SDIO_CmdCrcCheck  = ENABLE;
    SDIO_CmdInitStructure.SDIO_ResponseType = RSP_LEN_48;
    SDIO_SendCommand(&SDIO_CmdInitStructure);

    sd_status = SD_WaitCmdDatComplete();

    return sd_status;
}

/**
  * @brief  Read one block from a specified address in a card.
  * @param  addr: Address from where data are to be read
  * @param  pDescriptorTab: pointer to the buffer that will contain data to be written.
  * @param  blockSize: the SD card Data block size
  * @retval  SD_Error: SD Card Error code.
  */
SD_Status SD_WriteSingleBlock(uint32_t addr, uint32_t buf, uint16_t blockSize)
{
    SDIO_CmdInitTypeDef SDIO_CmdInitStructure;
    SDIO_DataInitTypeDef SDIO_DataInitStruct;
    SD_Status sd_status = SD_OK;

    if (NULL == buf)
    {
        sd_status = SD_INVALID_PARAMETER;
        return sd_status;
    }

    /* Check SD card status */
    SD_WaitCardFinishProgram(RCA);

    /* Data Configure */
    SDIO_ADMA2_DescTab[0].SDIO_Address              =   buf;
    SDIO_ADMA2_DescTab[0].SDIO_Length               =   blockSize;
    SDIO_ADMA2_DescTab[0].SDIO_Attribute.SDIO_Valid     =   1;
    SDIO_ADMA2_DescTab[0].SDIO_Attribute.SDIO_End   =   1;
    SDIO_ADMA2_DescTab[0].SDIO_Attribute.SDIO_Int   =   0;
    SDIO_ADMA2_DescTab[0].SDIO_Attribute.SDIO_Act1  =   0;
    SDIO_ADMA2_DescTab[0].SDIO_Attribute.SDIO_Act2  =   1;
    SDIO_DataStructInit(&SDIO_DataInitStruct);
    SDIO_DataInitStruct.SDIO_Address        = (uint32_t)SDIO_ADMA2_DescTab;
    SDIO_DataInitStruct.SDIO_BlockCount     = 1;
    SDIO_DataInitStruct.SDIO_BlockSize      = blockSize;
    SDIO_DataInitStruct.SDIO_TransferDir    = SDIO_TransferDir_WRITE;
    SDIO_DataInitStruct.SDIO_DMAEn          = ENABLE;
    SDIO_DataConfig(&SDIO_DataInitStruct);

    /* CMD24 */
    SDIO_CmdInitStructure.SDIO_Argument     = addr;
    SDIO_CmdInitStructure.SDIO_CmdIndex     = SDIO_WRITE_SINGLE_BLOCK;
    SDIO_CmdInitStructure.SDIO_CmdType      = NORMAL;
    SDIO_CmdInitStructure.SDIO_DataPresent  = WITH_DATA;
    SDIO_CmdInitStructure.SDIO_CmdIdxCheck  = ENABLE;
    SDIO_CmdInitStructure.SDIO_CmdCrcCheck  = ENABLE;
    SDIO_CmdInitStructure.SDIO_ResponseType = RSP_LEN_48;
    SDIO_SendCommand(&SDIO_CmdInitStructure);

    sd_status = SD_WaitCmdDatComplete();
    if (sd_status != SD_OK)
    {
        return sd_status;
    }

    /* Check block whether write protection */
    if (SDIO_GetResponse(SDIO_RSP0) & SD_OCR_WRITE_PROT_VIOLATION)
    {
        return SD_WRITE_PROT_VIOLATION;
    }

    return sd_status;
}

/**
  * @brief  Read multiple block from a specified address in a card.
  * @param  addr: Address from where data are to be read
  * @param  pDescriptorTab: pointer to the buffer that will contain data to be written.
  * @param  blockSize: the SD card Data block size
  * @retval  SD_Error: SD Card Error code.
  */
SD_Status SD_WriteMultiBlock(uint32_t addr, uint32_t buf, uint16_t blockSize, uint16_t blockCount)
{
    SDIO_CmdInitTypeDef SDIO_CmdInitStructure;
    SDIO_DataInitTypeDef SDIO_DataInitStruct;
    SD_Status sd_status = SD_OK;

    if (NULL == buf)
    {
        sd_status = SD_INVALID_PARAMETER;
        return sd_status;
    }

    /* Check SD card status */
    SD_WaitCardFinishProgram(RCA);

    /* Data Configure */
    if (blockCount <= 127)
    {
        SDIO_ADMA2_DescTab[0].SDIO_Address              = (uint32_t)buf;
        SDIO_ADMA2_DescTab[0].SDIO_Length               =   blockSize * blockCount;
        SDIO_ADMA2_DescTab[0].SDIO_Attribute.SDIO_Valid     =   1;
        SDIO_ADMA2_DescTab[0].SDIO_Attribute.SDIO_End   =   1;
        SDIO_ADMA2_DescTab[0].SDIO_Attribute.SDIO_Int   =   0;
        SDIO_ADMA2_DescTab[0].SDIO_Attribute.SDIO_Act1  =   0;
        SDIO_ADMA2_DescTab[0].SDIO_Attribute.SDIO_Act2  =   1;
    }
    else if (blockCount <= 254)
    {
        SDIO_ADMA2_DescTab[0].SDIO_Address              = (uint32_t)buf;
        SDIO_ADMA2_DescTab[0].SDIO_Length               =   blockSize * 127;
        SDIO_ADMA2_DescTab[0].SDIO_Attribute.SDIO_Valid     =   1;
        SDIO_ADMA2_DescTab[0].SDIO_Attribute.SDIO_End   =   0;
        SDIO_ADMA2_DescTab[0].SDIO_Attribute.SDIO_Int   =   0;
        SDIO_ADMA2_DescTab[0].SDIO_Attribute.SDIO_Act1  =   0;
        SDIO_ADMA2_DescTab[0].SDIO_Attribute.SDIO_Act2  =   1;

        SDIO_ADMA2_DescTab[1].SDIO_Address              = (uint32_t)buf + blockSize * 127 + 1;
        SDIO_ADMA2_DescTab[1].SDIO_Length               =   blockSize * (blockCount - 127);
        SDIO_ADMA2_DescTab[1].SDIO_Attribute.SDIO_Valid     =   1;
        SDIO_ADMA2_DescTab[1].SDIO_Attribute.SDIO_End   =   1;
        SDIO_ADMA2_DescTab[1].SDIO_Attribute.SDIO_Int   =   0;
        SDIO_ADMA2_DescTab[1].SDIO_Attribute.SDIO_Act1  =   0;
        SDIO_ADMA2_DescTab[1].SDIO_Attribute.SDIO_Act2  =   1;
    }
    else
    {
        //Add by malloc
    }
    SDIO_DataStructInit(&SDIO_DataInitStruct);
    SDIO_DataInitStruct.SDIO_Address        = (uint32_t)SDIO_ADMA2_DescTab;
    SDIO_DataInitStruct.SDIO_BlockCount     = blockCount;
    SDIO_DataInitStruct.SDIO_BlockSize      = blockSize;
    SDIO_DataInitStruct.SDIO_TransferDir    = SDIO_TransferDir_WRITE;
    SDIO_DataInitStruct.SDIO_DMAEn          = ENABLE;
    SDIO_DataConfig(&SDIO_DataInitStruct);

    /* CMD25 */
    SDIO_CmdInitStructure.SDIO_Argument     = addr;
    SDIO_CmdInitStructure.SDIO_CmdIndex     = SDIO_WRITE_MULT_BLOCK;
    SDIO_CmdInitStructure.SDIO_CmdType      = NORMAL;
    SDIO_CmdInitStructure.SDIO_DataPresent  = WITH_DATA;
    SDIO_CmdInitStructure.SDIO_CmdIdxCheck  = ENABLE;
    SDIO_CmdInitStructure.SDIO_CmdCrcCheck  = ENABLE;
    SDIO_CmdInitStructure.SDIO_ResponseType = RSP_LEN_48;
    SDIO_SendCommand(&SDIO_CmdInitStructure);

    sd_status = SD_WaitCmdDatComplete();
    if (sd_status != SD_OK)
    {
        return sd_status;
    }

    /* Check block whether write protection */
    if (SDIO_GetResponse(SDIO_RSP0) & SD_OCR_WRITE_PROT_VIOLATION)
    {
        return SD_WRITE_PROT_VIOLATION;
    }

    return sd_status;
}

/**
  * @brief  Get SCR which include infomation about
  * SCR_STRUCTURE, SD_SPEC, DATA_STAT_AFTER_EARSE, SD_SECURITY and SD_BUS_WIDTHS.
  * @param  None.
  * @retval  SD_Status: SD status.
  */
SD_Status SD_GetSCR(uint32_t RCA, uint32_t *pBuf)
{
    SDIO_CmdInitTypeDef SDIO_CmdInitStructure;
    SD_Status sd_status = SD_OK;

    /* Waiting for CMD line idle */
    if (SD_WaitCmdLineIdle() != SD_OK)
    {
        return SD_SDIO_CMD_INHIBIT;
    }

    /* CMD55 */
    sd_status = SD_SendAppCmd(RCA << 16);
    if (sd_status != SD_OK)
    {
        return sd_status;
    }

    /* ACMD51 */
    SDIO_ADMA2_DescTab[0].SDIO_Address              = (uint32_t)pBuf;
    SDIO_ADMA2_DescTab[0].SDIO_Length               =   8;
    SDIO_ADMA2_DescTab[0].SDIO_Attribute.SDIO_Valid     =   1;
    SDIO_ADMA2_DescTab[0].SDIO_Attribute.SDIO_End   =   1;
    SDIO_ADMA2_DescTab[0].SDIO_Attribute.SDIO_Int   =   0;
    SDIO_ADMA2_DescTab[0].SDIO_Attribute.SDIO_Act1  =   0;
    SDIO_ADMA2_DescTab[0].SDIO_Attribute.SDIO_Act2  =   1;
    /* Data Configure */
    SDIO_DataInitTypeDef SDIO_DataInitStruct;
    SDIO_DataStructInit(&SDIO_DataInitStruct);
    SDIO_DataInitStruct.SDIO_Address        = (uint32_t)SDIO_ADMA2_DescTab;
    SDIO_DataInitStruct.SDIO_BlockCount     = 1;
    SDIO_DataInitStruct.SDIO_BlockSize      = 8;
    SDIO_DataInitStruct.SDIO_TransferDir    = SDIO_TransferDir_READ;
    SDIO_DataInitStruct.SDIO_DMAEn          = ENABLE;
    SDIO_DataConfig(&SDIO_DataInitStruct);

    SDIO_CmdInitStructure.SDIO_Argument     = 0;
    SDIO_CmdInitStructure.SDIO_CmdIndex     = SDIO_SD_APP_SEND_SCR;
    SDIO_CmdInitStructure.SDIO_CmdType      = NORMAL;
    SDIO_CmdInitStructure.SDIO_DataPresent  = WITH_DATA;
    SDIO_CmdInitStructure.SDIO_CmdIdxCheck  = ENABLE;
    SDIO_CmdInitStructure.SDIO_CmdCrcCheck  = ENABLE;
    SDIO_CmdInitStructure.SDIO_ResponseType = RSP_LEN_48;
    SDIO_SendCommand(&SDIO_CmdInitStructure);

    sd_status = SD_WaitCmdDatComplete();

    //ACMD51 response type is R1
    //Add SCR information check code if use it

    return sd_status;
}


/**
  * @brief  Earse SD card.
  * @param  startAddr:  Start address which need to erase.
  * @param  endAddr: End address which need to erase.
  * @retval  SD_Status: SD status.
  */
SD_Status SD_Earse(uint32_t startAddr, uint32_t endAddr)
{
    SDIO_CmdInitTypeDef SDIO_CmdInitStructure;
    SD_Status sd_status = SD_OK;

    /* Waiting for CMD line idle */
    if (SD_WaitCmdLineIdle() != SD_OK)
    {
        return SD_SDIO_CMD_INHIBIT;
    }

    /* CMD32 */
    SDIO_CmdInitStructure.SDIO_Argument     = startAddr;
    SDIO_CmdInitStructure.SDIO_CmdIndex     = SDIO_SD_ERASE_GRP_START;
    SDIO_CmdInitStructure.SDIO_CmdType      = NORMAL;
    SDIO_CmdInitStructure.SDIO_DataPresent  = NO_DATA;
    SDIO_CmdInitStructure.SDIO_CmdIdxCheck  = ENABLE;
    SDIO_CmdInitStructure.SDIO_CmdCrcCheck  = ENABLE;
    SDIO_CmdInitStructure.SDIO_ResponseType = RSP_LEN_48;
    SDIO_SendCommand(&SDIO_CmdInitStructure);

    /* Waiting for CMD line idle */
    if (SD_WaitCmdLineIdle() != SD_OK)
    {
        return SD_SDIO_CMD_INHIBIT;
    }
    /* Check response */
    if (SD_CheckCmd(SDIO_SD_ERASE_GRP_START) != SD_OK)
    {
        return SD_CMD_ERROR;
    }

    /* --------------  CMD33 --------------------*/
    SDIO_CmdInitStructure.SDIO_Argument     = endAddr;
    SDIO_CmdInitStructure.SDIO_CmdIndex     = SDIO_SD_ERASE_GRP_END;
    SDIO_CmdInitStructure.SDIO_CmdType      = NORMAL;
    SDIO_CmdInitStructure.SDIO_DataPresent  = NO_DATA;
    SDIO_CmdInitStructure.SDIO_CmdIdxCheck  = ENABLE;
    SDIO_CmdInitStructure.SDIO_CmdCrcCheck  = ENABLE;
    SDIO_CmdInitStructure.SDIO_ResponseType = RSP_LEN_48;
    SDIO_SendCommand(&SDIO_CmdInitStructure);

    /* Waiting for CMD line idle */
    if (SD_WaitCmdLineIdle() != SD_OK)
    {
        return SD_SDIO_CMD_INHIBIT;
    }

    /* Check response */
    if (SD_CheckCmd(SDIO_SD_ERASE_GRP_END) != SD_OK)
    {
        return SD_CMD_ERROR;
    }

    /* CMD38 */
    SDIO_CmdInitStructure.SDIO_Argument     = 0;
    SDIO_CmdInitStructure.SDIO_CmdIndex     = SDIO_ERASE;
    SDIO_CmdInitStructure.SDIO_CmdType      = NORMAL;
    SDIO_CmdInitStructure.SDIO_DataPresent  = NO_DATA;
    SDIO_CmdInitStructure.SDIO_CmdIdxCheck  = ENABLE;
    SDIO_CmdInitStructure.SDIO_CmdCrcCheck  = ENABLE;
    SDIO_CmdInitStructure.SDIO_ResponseType = RSP_LEN_48_CHK_BUSY;
    SDIO_SendCommand(&SDIO_CmdInitStructure);

    /* Waiting for CMD line idle */
    if (SD_WaitCmdLineIdle() != SD_OK)
    {
        return SD_SDIO_CMD_INHIBIT;
    }

    return sd_status;
}

/**
  * @brief  Stop transfer.
  * @param  none.
  * @retval  SD_Status: SD status.
  */
SD_Status SD_StopTransfer(void)
{
    SDIO_CmdInitTypeDef SDIO_CmdInitStructure;
    SD_Status sd_status = SD_OK;

    /* CMD12, command response: R1b */
    SDIO_CmdInitStructure.SDIO_Argument     = 0;
    SDIO_CmdInitStructure.SDIO_CmdIndex     = SDIO_STOP_TRANSMISSION;
    SDIO_CmdInitStructure.SDIO_CmdType      = NORMAL;
    SDIO_CmdInitStructure.SDIO_DataPresent  = NO_DATA;
    SDIO_CmdInitStructure.SDIO_CmdIdxCheck  = ENABLE;
    SDIO_CmdInitStructure.SDIO_CmdCrcCheck  = ENABLE;
    SDIO_CmdInitStructure.SDIO_ResponseType = RSP_LEN_48;
    SDIO_SendCommand(&SDIO_CmdInitStructure);

    /* Check response */
    sd_status = SD_CheckResp1(SDIO_STOP_TRANSMISSION);
    return sd_status;
}

/** @} */ /* End of group SDCard_Demo_Exported_Functions */
/** @} */ /* End of group SDCARD */


/******************* (C) COPYRIGHT 2017 Realtek Semiconductor Corporation *****END OF FILE****/

