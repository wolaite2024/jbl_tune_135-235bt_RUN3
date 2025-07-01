#ifndef OS_EXT_H
#define OS_EXT_H

#include "vector_table.h"
typedef enum
{

    WATCH_POINT_INDEX0 = 0,
    WATCH_POINT_INDEX1 = 1,
    WATCH_POINT_NUMBER = 2
} WatchPointIndex;

typedef enum
{
    WATCH_POINT_BYTE  = 0,
    WATCH_POINT_HALFWORD,
    WATCH_POINT_WORD,
    WATCH_POINT_SIZE_MAX
} WatchPointSize;


typedef enum
{
    WATCH_POINT_FUNCTION_DISABLED = 0,
    WATCH_POINT_FUNCTION_INSTR_ADDR = 2,
    WATCH_POINT_FUNCTION_INSTR_ADDR_LIM,
    WATCH_POINT_FUNCTION_DADDR_RW,
    WATCH_POINT_FUNCTION_DADDR_W,
    WATCH_POINT_FUNCTION_DADDR_R,
    WATCH_POINT_FUNCTION_DADDR_LIM,
    WATCH_POINT_FUNCTION_DVAL_RW,
    WATCH_POINT_FUNCTION_DVAL_W,
    WATCH_POINT_FUNCTION_DVAL_R,
    WATCH_POINT_FUNCTION_DVAL_LINK,
    WATCH_POINT_FUNCTION_MAX
} WatchPointType;

void vector_table_update(IRQ_Fun *pAppVector, void *Default_Handler);
void debug_monitor_enable(void);
bool debug_monitor_is_enable(void);
void debug_monitor_point_set(WatchPointIndex index, uint32_t watch_address,
                             WatchPointSize watch_size, WatchPointType read_write_func);
#endif
