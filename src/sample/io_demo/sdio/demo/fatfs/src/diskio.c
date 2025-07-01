/*-----------------------------------------------------------------------*/
/* Low level disk I/O module skeleton for FatFs     (C)ChaN, 2013        */
/*-----------------------------------------------------------------------*/
/* If a working storage control module is available, it should be        */
/* attached to the FatFs via a glue function rather than modifying it.   */
/* This is an example of glue functions to attach various exsisting      */
/* storage control module to the FatFs module with a defined API.        */
/*-----------------------------------------------------------------------*/

#include "diskio.h"     /* FatFs lower layer API */
#include "sdcard.h"

DSTATUS disk_initialize(
    BYTE pdrv               /* Physical drive nmuber (0..) */
)
{
    DSTATUS stat;

    stat = SD_Init();

    return stat;
}

DSTATUS disk_status(
    BYTE pdrv       /* Physical drive nmuber (0..) */
)
{
    return 0;
}

DRESULT disk_read(
    BYTE pdrv,      /* Physical drive nmuber (0..) */
    BYTE *buff,     /* Data buffer to store read data */
    DWORD sector,   /* Sector address (LBA) */
    UINT count      /* Number of sectors to read (1..128) */
)
{
    DRESULT res;

    if (count == 1)
    {
        res = (DRESULT)SD_ReadSingleBlock(sector, (uint32_t)buff, 512);
    }
    else
    {
        res = (DRESULT)SD_ReadMultiBlock(sector, (uint32_t)buff, 512, count);
    }

    return res;
}

#if _USE_WRITE
DRESULT disk_write(
    BYTE pdrv,          /* Physical drive nmuber (0..) */
    const BYTE *buff,   /* Data to be written */
    DWORD sector,       /* Sector address (LBA) */
    UINT count          /* Number of sectors to write (1..128) */
)
{
    DRESULT res;

    if (count == 1)
    {
        res = (DRESULT)SD_WriteSingleBlock(sector, (uint32_t)buff, 512);
    }
    else
    {
        res = (DRESULT)SD_WriteMultiBlock(sector, (uint32_t)buff, 512, count);
    }

    return res;

}
#endif



#if _USE_IOCTL
DRESULT disk_ioctl(
    BYTE pdrv,      /* Physical drive nmuber (0..) */
    BYTE cmd,       /* Control code */
    void *buff      /* Buffer to send/receive control data */
)
{
    DRESULT res = RES_OK;
    return res;
}
#endif

DWORD get_fattime(void)
{
    return 0;
}

