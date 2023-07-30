
#ifndef SCR_HAL_H
#define SCR_HAL_H

#include <stddef.h>
#include <stdint.h>

long esp_random2(long s, long b);
int64_t esp_uptime(void);

#endif