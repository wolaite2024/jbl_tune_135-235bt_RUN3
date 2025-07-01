#include <stdint.h>
#ifndef IC_TYPE_BB2_CCUT
#include "ftl_ext.h"
#endif
#include "ftl.h"
#include "bt_ext_ftl.h"

uint16_t g_bt_ext_ftl_start_offset = 0;
uint16_t g_bt_ext_ftl_max_offset = 0;

int32_t bt_ext_ftl_partition_init(const T_STORAGE_PARTITION_INFO *info)
{
    return ftl_partition_init(info->address, 4, BT_EXT_FTL_BLOCK_LEN, &g_bt_ext_ftl_start_offset,
                              &g_bt_ext_ftl_max_offset);
}

int32_t bt_ext_ftl_partition_write(const T_STORAGE_PARTITION_INFO *info, uint32_t offset,
                                   uint32_t len,
                                   void *buf)
{
    return ftl_save_to_storage((void *)buf, offset + g_bt_ext_ftl_start_offset, len);
}

int32_t bt_ext_ftl_partition_read(const T_STORAGE_PARTITION_INFO *info, uint32_t offset,
                                  uint32_t len,
                                  void *buf)
{
    return ftl_load_from_storage((void *)buf, offset + g_bt_ext_ftl_start_offset, len);
}
