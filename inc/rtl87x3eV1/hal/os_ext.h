#ifndef OS_EXT_H
#define OS_EXT_H

void monitor_memory_and_timer(uint16_t heap_and_timer_monitor_timer_timeout_s);
bool os_timer_state_get(void **pp_handle, uint32_t *p_timer_state);
#endif
