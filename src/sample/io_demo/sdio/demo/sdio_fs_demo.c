/**
*********************************************************************************************************
*               Copyright(c) 2017, Realtek Semiconductor Corporation. All rights reserved.
**********************************************************************************************************
* @file     sdio_demo.c
* @brief        This file provides SDIO demo code.
* @details  Please create a folder named BBPro in root directory.
* @author   elliot chen
* @date     2017-1-16
* @version  v1.0
*********************************************************************************************************
*/

/* Includes -----------------------------------------------------------------*/
#include "rtl876x_pinmux.h"
#include "rtl876x_rcc.h"
#include "sdcard.h"
#include "ff.h"

/* Defines ------------------------------------------------------------------*/
#define SDH_CLK             P1_2
#define SDH_CMD             P1_3
#define SDH_D0              P1_4
#define SDH_D1              P1_5
#define SDH_D2              P1_6
#define SDH_D3              P1_7
#define SDH_WT_PROT         P3_0
#define SDH_CD              P3_1

/* Globals ------------------------------------------------------------------*/

/* FatFS */
FATFS fs;
/* file objects */
FIL fsrc, fdst;
/* file copy buffer */
char buffer[1024];
char objectname[50] = "0:/BBPro/";
DIR dir;
FILINFO fileinfo;

/**
  * @brief  initialization of pinmux settings and pad settings.
  * @param   No parameter.
  * @return  void
  */
void Board_SD_Init(void)
{
    Pad_Config(SDH_CLK, PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_UP, PAD_OUT_ENABLE, PAD_OUT_HIGH);
    Pad_Config(SDH_CMD, PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_UP, PAD_OUT_ENABLE, PAD_OUT_HIGH);
    //Pad_Config(SDH_WT_PROT, PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_UP, PAD_OUT_DISABLE, PAD_OUT_HIGH);
    Pad_Config(SDH_CD, PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_UP, PAD_OUT_DISABLE, PAD_OUT_HIGH);
    Pad_Config(SDH_D0, PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_UP, PAD_OUT_ENABLE, PAD_OUT_HIGH);
    Pad_Config(SDH_D1, PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_UP, PAD_OUT_ENABLE, PAD_OUT_HIGH);
    Pad_Config(SDH_D2, PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_UP, PAD_OUT_ENABLE, PAD_OUT_HIGH);
    Pad_Config(SDH_D3, PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_NONE, PAD_OUT_ENABLE, PAD_OUT_HIGH);

    /* Configure SD pin group and enable it */
    SDIO_PinGroupConfig(SDIO_PinGroup_0);
    SDIO_PinOutputCmd(ENABLE);
}

/**
  * @brief  demo code of FATFS file system.
  * @param   No parameter.
  * @return  void
  */
void FatFS_Demo(void)
{
    uint32_t a = 1;
    const char driver_num = 0;

    /* Create workspace */
    uint32_t res = f_mount(&fs, &driver_num, 0);

    /* Open file */
    if (f_open(&fdst, "Test.txt", FA_CREATE_ALWAYS | FA_WRITE) != FR_OK)
    {
        //Open file failure, add error handle code here
        f_close(&fdst);
        return ;
    }

    if (f_open(&fsrc, "Data.txt", FA_OPEN_EXISTING | FA_READ) != FR_OK)
    {
        //Open file failure, add error handle code here
        f_close(&fsrc);
        return ;
    }

    /* File operation */
    for (uint32_t i = 0; i < 1024; i++)
    {
        buffer[i] = i % 0x09 + 0x30;
    }
    buffer[1021] = 69;
    buffer[1022] = 69;
    buffer[1023] = 69;
    //f_read(&fsrc, buffer, 2, &a);
    f_write(&fdst, buffer, 1024, &a);

    /* Close file */
    f_close(&fsrc);
    f_close(&fdst);
}

/**
  * @brief  demo code of FATFS file system.
  * @param   No parameter.
  * @return  void
  */
void FatFS_DirectoyDemo(void)
{
    uint32_t a = 1;
    const char driver_num = 0;

    char *pname = objectname;
    FRESULT res;

    /* Create workspace */
    res = f_mount(&fs, &driver_num, 0);

    /* Open directory */
    if (f_opendir(&dir, "0:/BBPro") != FR_OK)
    {
        //Open directory failure, add error handle code here
    }

    while (1)
    {
        res = f_readdir(&dir, &fileinfo);

        /* Traverse all files */
        if ((res != FR_OK) || (fileinfo.fname[0] == 0))
        {
            break;
        }

        for (uint32_t i = 0; i < 13; i++)
        {
            *(pname + i + 9) = fileinfo.fname[i];
        }

        /* Open file */
        if (f_open(&fdst, pname, FA_CREATE_ALWAYS | FA_WRITE) != FR_OK)
        {
            //Open file failure, add error handle code here
            f_close(&fdst);
            return ;
        }

        /* File operation */
        for (uint32_t i = 0; i < 1024; i++)
        {
            buffer[i] = i % 0x09 + 0x30;
        }
        buffer[1021] = 'e';
        buffer[1022] = 'n';
        buffer[1023] = 'd';
        f_write(&fdst, buffer, 1024, &a);

        /* Close file */
        f_close(&fdst);
    }
}

/**
  * @brief  demo code of SDIO communication.
  * @param   No parameter.
  * @return  void
  */
void SD_DemoCode(void)
{
    /* PAD configure */
    Board_SD_Init();

    /* Demo code */
    FatFS_DirectoyDemo();

    /* Get card infomation */
    SD_CardInfo SDCardInfo;
    SD_GetCardInfo(&SDCardInfo);
}

/******************* (C) COPYRIGHT 2017 Realtek Semiconductor Corporation *****END OF FILE****/

