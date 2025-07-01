#ifndef HAL_DEBUG_H
#define HAL_DEBUG_H

#define F_HAL_DEBUG_TASK_SCHEDULING 1
#define F_HAL_DEBUG_HIT_RATE_PRINT  1
#define F_HAL_DEBUG_HEAP_USAGE_INFO 1
#define F_HAL_DEBUG_PC_SAMPLING     1
#define F_HAL_DEBUG_QUEUE_USAGE     1
#define F_HAL_DEBUG_TASK_TIME_PROPORTION    1
#define F_HAL_DEBUG_HW_TIMER_IRQ    1

#define HAL_DEBUG_CACHE_HIT_TIMER   0
#define HAL_DEBUG_QUEUE_USAGE_TIMER 1
#define HAL_DEBUG_TASK_TIME_TIMER   2

void hal_debug_init(void);
void hal_debug_task_schedule_init(uint32_t task_num);
void hal_debug_print_task_info(void);
void hal_debug_cache_hit_count_init(uint32_t period_ms);
void hal_debug_memory_dump(void);
void hal_debug_msg_queue_usage_monitor(uint32_t period_ms, void *queue_handle1, void *queue_handle2,
                                       void *queue_handle3);
void hal_debug_task_time_proportion_init(uint32_t period_ms);
void hal_debug_print_pc_sampling(void);
void hal_debug_pc_sampling_init(uint8_t num, uint32_t period_ms);
void hal_debug_hw_timer_irq_init(void);
#endif
