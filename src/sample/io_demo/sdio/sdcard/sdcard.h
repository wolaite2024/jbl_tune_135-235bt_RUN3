/**
*********************************************************************************************************
*               Copyright(c) 2017, Realtek Semiconductor Corporation. All rights reserved.
*********************************************************************************************************
* @file      sdcard.h
* @brief     header file of sdcard driver.
* @details
* @author   elliot chen
* @date     2017-01-13
* @version   v1.0
* *********************************************************************************************************
*/

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __SDCARD_H
#define __SDCARD_H

/* Includes ------------------------------------------------------------------*/
#include "rtl876x_sdio.h"
//#include "rtl_delay.h"

/* Exported types ------------------------------------------------------------*/
typedef enum
{
    SD_OK                               = (0), /* Communication is normal */
    /* Error type in interrupt status register */
    SD_CMD_RSP_TIMEOUT                  = (1), /* Command response timeout */
    SD_CMD_CRC_FAIL                     = (2), /* Command response received (but CRC check failed) */
    SD_CMD_END_BIT_ERR                  = (3), /* The end bit of a command response is 0 */
    SD_CMD_INDEX_ERR                    = (4), /* A command index error occur in the command response */
    SD_DATA_TIMEOUT                     = (5), /* Data time out */
    SD_DATA_CRC_FAIL                    = (6), /* Data bock sent/received (CRC check Failed) */
    SD_DATA_END_BIT_ERR                 = (7),
    SD_CURRENT_LIMIT_ERR                = (8),
    SD_AUTO_CMD12_ERR                   = (9),
    SD_ADMA_ERR                         = (10),
    /* Card status */
    SD_ADDR_OUT_OF_RANGE                = (11), /* Argument of the command is out of the allowed range of this card */
    SD_ADDR_MISALIGNED                  = (12), /* Misaligned address */
    SD_BLOCK_LEN_ERR                    = (13), /* Transferred block length is not allowed for the card or the number of transferred bytes does not match the block length */
    SD_ERASE_SEQ_ERR                    = (14), /* An error in the sequence of erase command occurs.*/
    SD_BAD_ERASE_PARAM                  = (15), /* An Invalid selection for erase groups */
    SD_WRITE_PROT_VIOLATION             = (16), /* Attempt to program a write protect block */
    SD_CARD_IS_LOCKED                   = (17), /* Card is locked by host */
    SD_LOCK_UNLOCK_FAILED               = (18), /* Sequence or password error has been detected in unlock command or if there was an attempt to access a locked card */
    SD_COM_CRC_FAILED                   = (19), /* CRC check of the previous command failed */
    SD_ILLEGAL_CMD                      = (20), /* Command is not legal for the card state */
    SD_CARD_ECC_FAILED                  = (21), /* Card internal ECC was applied but failed to correct the data */
    SD_CC_ERROR                         = (22), /* Internal card controller error */
    SD_GENERAL_UNKNOWN_ERROR            = (23), /* General or Unknown error */
    SD_CID_CSD_OVERWRITE                = (24), /* CID/CSD overwrite error */
    SD_WP_ERASE_SKIP                    = (25), /* only partial address space was erased */
    SD_CARD_ECC_DISABLED                = (26), /* Command has been executed without using internal ECC */
    SD_ERASE_RESET                      = (27), /* Erase sequence was cleared before executing because an out of erase sequence command was received */
    SD_APP_CMD                          = (28), /* The card will expect ACMD */
    SD_AKE_SEQ_ERROR                    = (28), /* Error in sequence of authentication. */
    /* Present status register type */
    SD_SDIO_CMD_INHIBIT                 = (34), /* Can not issue next command */
    SD_CMD_INHIBIT_BEFORE_RESET         = (35),
    SD_SDIO_CMD_INHIBIT_AFTER_RESET     = (36),
    SD_HOST_SW_RESET_ERROR              = (37),
    SD_SDIO_CMD_COMPLETE                = (39),
    SD_HOST_INTER_CLOCK_ERROR           = (40),
    /* command index error in Response2 */
    SD_CMD_ERROR                        = (41),
    /* address error in data tranmssion */
    SD_PROG_TIMEOUT                     = (50), /* Card is in program or receive status */
    SD_INVALID_PARAMETER,
} SD_Status;

/* SDIO Commands  Index */
#define SDIO_GO_IDLE_STATE                       ((uint32_t)0)
#define SDIO_SEND_OP_COND                        ((uint32_t)1)
#define SDIO_ALL_SEND_CID                        ((uint32_t)2)
#define SDIO_SET_REL_ADDR                        ((uint32_t)3) /* SDIO_SEND_REL_ADDR for SD Card */
#define SDIO_SET_DSR                             ((uint32_t)4)
#define SDIO_SDIO_SEN_OP_COND                    ((uint32_t)5)
#define SDIO_HS_SWITCH                           ((uint32_t)6)
#define SDIO_SEL_DESEL_CARD                      ((uint32_t)7)
#define SDIO_SEND_IF_COND                        ((uint32_t)8)
#define SDIO_SEND_CSD                            ((uint32_t)9)
#define SDIO_SEND_CID                            ((uint32_t)10)
#define SDIO_READ_DAT_UNTIL_STOP                 ((uint32_t)11) /* SD Card doesn't support it */
#define SDIO_STOP_TRANSMISSION                   ((uint32_t)12)
#define SDIO_SEND_STATUS                         ((uint32_t)13)
#define SDIO_HS_BUSTEST_READ                     ((uint32_t)14)
#define SDIO_GO_INACTIVE_STATE                   ((uint32_t)15)
#define SDIO_SET_BLOCKLEN                        ((uint32_t)16)
#define SDIO_READ_SINGLE_BLOCK                   ((uint32_t)17)
#define SDIO_READ_MULT_BLOCK                     ((uint32_t)18)
#define SDIO_HS_BUSTEST_WRITE                    ((uint32_t)19)
#define SDIO_WRITE_DAT_UNTIL_STOP                ((uint32_t)20) /* SD Card doesn't support it */
#define SDIO_SET_BLOCK_COUNT                     ((uint32_t)23) /* SD Card doesn't support it */
#define SDIO_WRITE_SINGLE_BLOCK                  ((uint32_t)24)
#define SDIO_WRITE_MULT_BLOCK                    ((uint32_t)25)
#define SDIO_PROG_CID                            ((uint32_t)26) /* reserved for manufacturers */
#define SDIO_PROG_CSD                            ((uint32_t)27)
#define SDIO_SET_WRITE_PROT                      ((uint32_t)28)
#define SDIO_CLR_WRITE_PROT                      ((uint32_t)29)
#define SDIO_SEND_WRITE_PROT                     ((uint32_t)30)
/* To set the address of the first write block to be erased. (For SD card only) */
#define SDIO_SD_ERASE_GRP_START                  ((uint32_t)32)
/* To set the address of the last write block of the continuous range to be erased. (For SD card only) */
#define SDIO_SD_ERASE_GRP_END                    ((uint32_t)33)
/* To set the address of the first write block to be erased.(For MMC card only spec 3.31) */
#define SDIO_ERASE_GRP_START                     ((uint32_t)35)
/* To set the address of the last write block of the continuous range to be erased. (For MMC card only spec 3.31) */
#define SDIO_ERASE_GRP_END                       ((uint32_t)36)
#define SDIO_ERASE                               ((uint32_t)38)
#define SDIO_FAST_IO                             ((uint32_t)39) /* SD Card doesn't support it */
#define SDIO_GO_IRQ_STATE                        ((uint32_t)40) /* SD Card doesn't support it */
#define SDIO_CMD_SEND_OP_COND                    ((uint32_t)41)
#define SDIO_LOCK_UNLOCK                         ((uint32_t)42)
#define SDIO_APP_CMD                             ((uint32_t)55)
#define SDIO_GEN_CMD                             ((uint32_t)56)
#define SDIO_NO_CMD                              ((uint32_t)64)

/* Following commands are SD Card Specific commands.
   SDIO_APP_CMD should be sent before sending these
   commands. */
#define SDIO_APP_SD_SET_BUSWIDTH                 ((uint32_t)6)  /* For SD Card only */
#define SDIO_SD_APP_STAUS                        ((uint32_t)13) /* For SD Card only */
#define SDIO_SD_APP_SEND_NUM_WRITE_BLOCKS        ((uint32_t)22) /* For SD Card only */
#define SDIO_SD_APP_OP_COND                      ((uint32_t)41) /* For SD Card only */
#define SDIO_SD_APP_SET_CLR_CARD_DETECT          ((uint32_t)42) /* For SD Card only */
#define SDIO_SD_APP_SEND_SCR                     ((uint32_t)51) /* For SD Card only */
#define SDIO_SDIO_RW_DIRECT                      ((uint32_t)52) /* For SD I/O Card only */
#define SDIO_SDIO_RW_EXTENDED                    ((uint32_t)53) /* For SD I/O Card only */

/* Following commands are SD Card Specific security commands.
   SDIO_APP_CMD should be sent before sending these commands. */
#define SDIO_SD_APP_GET_MKB                      ((uint32_t)43) /* For SD Card only */
#define SDIO_SD_APP_GET_MID                      ((uint32_t)44) /* For SD Card only */
#define SDIO_SD_APP_SET_CER_RN1                  ((uint32_t)45) /* For SD Card only */
#define SDIO_SD_APP_GET_CER_RN2                  ((uint32_t)46) /* For SD Card only */
#define SDIO_SD_APP_SET_CER_RES2                 ((uint32_t)47) /* For SD Card only */
#define SDIO_SD_APP_GET_CER_RES1                 ((uint32_t)48) /* For SD Card only */
#define SDIO_SD_APP_SECURE_READ_MULTIPLE_BLOCK   ((uint32_t)18) /* For SD Card only */
#define SDIO_SD_APP_SECURE_WRITE_MULTIPLE_BLOCK  ((uint32_t)25) /* For SD Card only */
#define SDIO_SD_APP_SECURE_ERASE                 ((uint32_t)38) /* For SD Card only */
#define SDIO_SD_APP_CHANGE_SECURE_AREA           ((uint32_t)49) /* For SD Card only */
#define SDIO_SD_APP_SECURE_WRITE_MKB             ((uint32_t)48) /* For SD Card only */

/* Exported constants --------------------------------------------------------*/

/* Supported Memory Cards */
#define SDIO_STD_CAPACITY_SD_CARD_V1_1          ((uint32_t)0x0)
#define SDIO_STD_CAPACITY_SD_CARD_V2_0          ((uint32_t)0x1)
#define SDIO_HIGH_CAPACITY_SD_CARD              ((uint32_t)0x2)
#define SDIO_MULTIMEDIA_CARD                    ((uint32_t)0x3)
#define SDIO_SECURE_DIGITAL_IO_CARD             ((uint32_t)0x4)
#define SDIO_HIGH_SPEED_MULTIMEDIA_CARD         ((uint32_t)0x5)
#define SDIO_SECURE_DIGITAL_IO_COMBO_CARD       ((uint32_t)0x6)
#define SDIO_HIGH_CAPACITY_MMC_CARD             ((uint32_t)0x7)

/* Operating Condition (ACMD41) */
#define ACMD41_POLL_INTERVAL            10000UL
#define ACMD41_INIT_TIMEOUT             1000000UL

/* OCR Register */
typedef enum
{
    VDD_27_28           = BIT15,
    VDD_28_29           = BIT16,
    VDD_29_30           = BIT17,
    VDD_30_31           = BIT18,
    VDD_31_32           = BIT19,
    VDD_32_33           = BIT20,
    VDD_33_34           = BIT21,
    VDD_34_35           = BIT22,
    VDD_35_36           = BIT23,
    CARD_CAPA_STATUS    = BIT30,
    //CARD_PWR_UP_STATUS    = BIT31
} OCR_VOLTAGE_PROFILE;

typedef enum
{
    SDSC_ONLY           = 0,
    SDHC_SUPPORT        = 1
} HOST_CAPACITY_SUPPORT;

#if 1

#if 0
typedef enum
{
    SD_NO_TRANSFER  = 0,
    SD_TRANSFER_IN_PROGRESS
} SDTransferState;

typedef struct
{
    uint16_t TransferredBytes;
    SD_Status TransferError;
    uint8_t  padding;
} SDLastTransferInfo;
#endif
typedef struct       /* Card Specific Data */
{
    uint8_t  CSDStruct;            /* CSD structure */
    uint8_t  SysSpecVersion;       /* System specification version */
    uint8_t  Reserved1;            /* Reserved */
    uint8_t  TAAC;                 /* Data read access-time 1 */
    uint8_t  NSAC;                 /* Data read access-time 2 in CLK cycles */
    uint8_t  MaxBusClkFrec;        /* Max. bus clock frequency */
    uint16_t CardComdClasses;      /* Card command classes */
    uint8_t  RdBlockLen;           /* Max. read data block length */
    uint8_t  PartBlockRead;        /* Partial blocks for read allowed */
    uint8_t  WrBlockMisalign;      /* Write block misalignment */
    uint8_t  RdBlockMisalign;      /* Read block misalignment */
    uint8_t  DSRImpl;              /* DSR implemented */
    uint8_t  Reserved2;            /* Reserved */
    uint32_t DeviceSize;           /* Device Size */
    uint8_t  MaxRdCurrentVDDMin;   /* Max. read current @ VDD min */
    uint8_t  MaxRdCurrentVDDMax;   /* Max. read current @ VDD max */
    uint8_t  MaxWrCurrentVDDMin;   /* Max. write current @ VDD min */
    uint8_t  MaxWrCurrentVDDMax;   /* Max. write current @ VDD max */
    uint8_t  DeviceSizeMul;        /* Device size multiplier */
    uint8_t  EraseGrSize;          /* Erase group size */
    uint8_t  EraseGrMul;           /* Erase group size multiplier */
    uint8_t  WrProtectGrSize;      /* Write protect group size */
    uint8_t  WrProtectGrEnable;    /* Write protect group enable */
    uint8_t  ManDeflECC;           /* Manufacturer default ECC */
    uint8_t  WrSpeedFact;          /* Write speed factor */
    uint8_t  MaxWrBlockLen;        /* Max. write data block length */
    uint8_t  WriteBlockPaPartial;  /* Partial blocks for write allowed */
    uint8_t  Reserved3;            /* Reserded */
    uint8_t  ContentProtectAppli;  /* Content protection application */
    uint8_t  FileFormatGrouop;     /* File format group */
    uint8_t  CopyFlag;             /* Copy flag (OTP) */
    uint8_t  PermWrProtect;        /* Permanent write protection */
    uint8_t  TempWrProtect;        /* Temporary write protection */
    uint8_t  FileFormat;           /* File Format */
    uint8_t  ECC;                  /* ECC code */
    uint8_t  CSD_CRC;              /* CSD CRC */
    uint8_t  Reserved4;            /* always 1*/
} SD_CSD;

typedef struct      /*Card Identification Data*/
{
    uint8_t  ManufacturerID;       /* ManufacturerID */
    uint16_t OEM_AppliID;          /* OEM/Application ID */
    uint32_t ProdName1;            /* Product Name part1 */
    uint8_t  ProdName2;            /* Product Name part2 */
    //uint8_t  ProdName[5];          /* Product Name */
    uint8_t  ProdRev;              /* Product Revision */
    uint32_t ProdSN;               /* Product Serial Number */
    uint8_t  Reserved1;            /* Reserved1 */
    uint16_t ManufactDate;         /* Manufacturing Date */
    uint8_t  CID_CRC;              /* CID CRC */
    uint8_t  Reserved2;            /* always 1 */
} SD_CID;

typedef struct
{
    SD_CSD SD_csd;
    SD_CID SD_cid;
    uint32_t CardCapacity;        /* Card Capacity */
    uint32_t CardBlockSize;       /* Card Block Size */
    uint16_t RCA;
    uint8_t CardType;
} SD_CardInfo, *pSD_CardInfo;
#endif

/* Exported functions ------------------------------------------------------- */
SD_Status SD_InitHost(void);
SD_Status SD_InitCard(void);
SD_Status SD_SetBusWide(uint32_t RCA, uint32_t SDIO_BusWide);
SD_Status SD_Init(void);
SD_Status SD_GetSCR(uint32_t RCA, uint32_t *pBuf);
SD_Status SD_GetCardInfo(SD_CardInfo *cardinfo);
SD_Status SD_ReadSingleBlock(uint32_t addr, uint32_t buf, uint16_t blockSize);
SD_Status SD_ReadMultiBlock(uint32_t addr, uint32_t buf, uint16_t blockSize, uint16_t blockCount);
SD_Status SD_WriteSingleBlock(uint32_t addr, uint32_t buf, uint16_t blockSize);
SD_Status SD_WriteMultiBlock(uint32_t addr, uint32_t buf, uint16_t blockSize, uint16_t blockCount);
SD_Status SD_Earse(uint32_t startAddr, uint32_t endAddr);
SD_Status SD_GetCardStatus(uint32_t RCA, uint32_t *pStatus);
SD_Status SD_WaitCardFinishProgram(uint32_t RCA);
SD_Status SD_StopTransfer(void);

#endif /* __SDCARD_H */

/******************* (C) COPYRIGHT 2017 Realtek Semiconductor Corporation *****END OF FILE****/

