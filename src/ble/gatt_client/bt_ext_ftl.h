#ifndef _BT_EXT_FTL_H_
#define _BT_EXT_FTL_H_

#ifdef  __cplusplus
extern "C" {
#endif      /* __cplusplus */

#include "storage.h"

#define BT_EXT_FTL_BLOCK_LEN     128

#define BT_EXT_FTL_PARTITION_NAME "BT_FTL"

extern uint16_t g_bt_ext_ftl_start_offset;
extern uint16_t g_bt_ext_ftl_max_offset;

int32_t bt_ext_ftl_partition_init(const T_STORAGE_PARTITION_INFO *info);
int32_t bt_ext_ftl_partition_write(const T_STORAGE_PARTITION_INFO *info, uint32_t offset,
                                   uint32_t len,
                                   void *buf);
int32_t bt_ext_ftl_partition_read(const T_STORAGE_PARTITION_INFO *info, uint32_t offset,
                                  uint32_t len,
                                  void *buf);

#ifdef  __cplusplus
}
#endif      /*  __cplusplus */

#endif
