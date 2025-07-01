
/*
 * Copyright (c) 2018, Realsil Semiconductor Corporation. All rights reserved.
 */

#if F_APP_TUYA_SUPPORT

#include <string.h>
#include <stdint.h>
#include <trace.h>
#include <rtl876x.h>
#include "rtk_tuya_ble_ota_flash.h"
#include "dfu_api.h"
#include "fmc_api.h"
//#include "rom_ext.h"
#include "app_ota.h"

#define TUYA_OTA_DEBUG_BYPASS_FLASH       1

uint32_t tuya_ota_get_app_ui_version(void)
{
    T_IMG_HEADER_FORMAT *addr;
    addr = (T_IMG_HEADER_FORMAT *)get_header_addr_by_img_id(IMG_MCUDATA);
    APP_PRINT_INFO1("tuya_ota_get_app_ui_version: 0x%08x", addr->git_ver.version);
    return (0xffff & addr->git_ver.version);
}

bool tuya_ota_check_crc(void *p_header)
{
#if TUYA_OTA_DEBUG_BYPASS_FLASH
    return true;
#else
    bool with_semaphore = true;
    return dfu_check_crc((T_IMG_CTRL_HEADER_FORMAT *)p_header, with_semaphore);
#endif
}

bool tuya_ota_check_image(void *p_header)
{
    bool check_result = tuya_ota_check_crc(p_header);
    APP_PRINT_INFO1("tuya_ota_check_image: check_result %d", check_result);
    return check_result;
}

uint32_t tuya_ota_write_to_flash(uint32_t dfu_base_addr, uint32_t offset, uint32_t length,
                                 void *p_void, bool is_control_header)
{
#if TUYA_OTA_DEBUG_BYPASS_FLASH
    return 0;
#else
    uint8_t ret = 0;
    uint8_t readback_buffer[READBACK_BUFFER_SIZE];
    uint32_t read_back_len;
    uint32_t dest_addr;
    uint8_t *p_src = (uint8_t *)p_void;
    uint32_t remain_size = length;

    uint8_t bp_level;
#if (TUYA_OTA_NEW_IMAGE_HEADER_STRUCTURE_SUPPORT == 1)
    T_FLASH_PARTITION_NAME partition_flash_ota_bank = (app_ota_get_active_bank() == IMAGE_LOCATION_BANK0
                                                       ? PARTITION_FLASH_OTA_BANK_0 : PARTITION_FLASH_OTA_BANK_1);
    fmc_flash_nor_get_bp_lv(flash_partition_addr_get(partition_flash_ota_bank),
                            &bp_level);
    fmc_flash_nor_set_bp_lv(flash_partition_addr_get(partition_flash_ota_bank),
                            0);
#else
    app_flash_get_bp_lv((uint32_t)&bp_level);
    app_flash_set_bp_lv(0);
#endif

    if (is_control_header)
    {
        T_IMG_CTRL_HEADER_FORMAT *p_header = (T_IMG_CTRL_HEADER_FORMAT *)p_void;
        p_header->ctrl_flag.not_ready = 0x1;
    }

    if (((offset + dfu_base_addr) % 0x1000) == 0)
    {
        fmc_flash_nor_erase(dfu_base_addr + offset, FMC_FLASH_NOR_ERASE_SECTOR);
    }
    else
    {
        if (((offset + dfu_base_addr) / 0x1000) != ((offset + length + dfu_base_addr) / 0x1000))
        {
            if ((offset + length + dfu_base_addr) % 0x1000)
            {
                fmc_flash_nor_erase((dfu_base_addr + offset + length) & ~(0x1000 - 1), FMC_FLASH_NOR_ERASE_SECTOR);
            }
        }
    }

    APP_PRINT_INFO3("tuya_ota_write_to_flash: dfu_base_addr 0x%08x, offset %d, length %d, ",
                    dfu_base_addr, offset, length);

    if (!fmc_flash_nor_write(dfu_base_addr + offset, p_void, length))
    {
        ret = 2;
    }
#if (TUYA_OTA_NEW_IMAGE_HEADER_STRUCTURE_SUPPORT == 1)
    SCB_InvalidateDCache_by_Addr((uint32_t *)(dfu_base_addr + offset), length);
    fmc_flash_nor_set_bp_lv(flash_partition_addr_get(partition_flash_ota_bank),
                            bp_level);
#else
    app_flash_set_bp_lv(bp_level);
#endif
    dest_addr = dfu_base_addr + offset;
    while (remain_size)
    {
        read_back_len = (remain_size >= READBACK_BUFFER_SIZE) ? READBACK_BUFFER_SIZE : remain_size;

        if (!fmc_flash_nor_read(dest_addr, readback_buffer, read_back_len))
        {
            ret = 3;
            break;
        }

        if (memcmp(readback_buffer, p_src, read_back_len) != 0)
        {
            ret = 4;
            break;
        }

        dest_addr += read_back_len;
        p_src += read_back_len;
        remain_size -= read_back_len;
    }

    if (ret)
    {
        APP_PRINT_ERROR1("tuya_ota_write_to_flash: ret %d", ret);
    }

    return ret;

#endif
}

void tuya_ota_clear_notready_flag(void)
{
#if TUYA_OTA_DEBUG_BYPASS_FLASH
    return;
#else
    uint32_t base_addr;
    uint16_t ctrl_flag;
    T_IMG_HEADER_FORMAT *image_header;
    uint8_t bp_level;
    IMG_ID img_max;

    T_IMG_HEADER_FORMAT *ota_header_addr;
    ota_header_addr = (T_IMG_HEADER_FORMAT *)get_temp_ota_bank_addr_by_img_id(IMG_OTA);

#if (TUYA_OTA_NEW_IMAGE_HEADER_STRUCTURE_SUPPORT == 1)
    T_FLASH_PARTITION_NAME partition_flash_ota_bank = (app_ota_get_active_bank() == IMAGE_LOCATION_BANK0
                                                       ? PARTITION_FLASH_OTA_BANK_0 : PARTITION_FLASH_OTA_BANK_1);
    fmc_flash_nor_get_bp_lv(flash_partition_addr_get(partition_flash_ota_bank),
                            &bp_level);
    fmc_flash_nor_set_bp_lv(flash_partition_addr_get(partition_flash_ota_bank),
                            0);
    img_max = IMAGE_MAX;
#else
    app_flash_get_bp_lv((uint32_t)&bp_level);
    app_flash_set_bp_lv(0);
    img_max = IMG_MAX;
#endif

    for (IMG_ID img_id = IMG_OTA; img_id < img_max; img_id++)
    {
        base_addr = get_temp_ota_bank_addr_by_img_id(img_id);
        image_header = (T_IMG_HEADER_FORMAT *)base_addr;

#if (TUYA_OTA_NEW_IMAGE_HEADER_STRUCTURE_SUPPORT == 1)
        if (img_id != IMG_OTA && !ota_header_addr->image_info[(img_id - IMG_SBL) * 2 + 1])
        {
            continue;
        }
#else
        if (img_id != IMG_OTA && !HAL_READ32((uint32_t)&ota_header_addr->fsbl_size, (img_id - IMG_SBL) * 8))
        {
            continue;
        }
#endif

        if (base_addr == 0 || img_id != image_header->ctrl_header.image_id)
        {
            continue;
        }

        fmc_flash_nor_read((uint32_t)&image_header->ctrl_header.ctrl_flag.value, (uint8_t *)&ctrl_flag,
                           sizeof(ctrl_flag));
        ctrl_flag &= ~0x80;
        fmc_flash_nor_write((uint32_t)&image_header->ctrl_header.ctrl_flag.value,
                            (uint8_t *)&ctrl_flag, sizeof(ctrl_flag));
    }

#if (TUYA_OTA_NEW_IMAGE_HEADER_STRUCTURE_SUPPORT == 1)
    fmc_flash_nor_set_bp_lv(flash_partition_addr_get(partition_flash_ota_bank),
                            bp_level);
#else
    app_flash_set_bp_lv(bp_level);
#endif

#endif
}

#endif
