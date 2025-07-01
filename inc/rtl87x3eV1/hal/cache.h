#ifndef __CACHE_H
#define __CACHE_H

#include <stdint.h>

void cache_enable(void);
void cache_disable(void);
void cache_flush_by_addr(uint32_t *addr, uint32_t length);
/**
 * @brief flash cache init.
 *
 * @return
*/
void cache_hit_init(void);

/**
 * @brief get cache hit rate *100.
 *
 * @return cache hit rate *100
*/
uint32_t cache_hit_get(void);
#endif

