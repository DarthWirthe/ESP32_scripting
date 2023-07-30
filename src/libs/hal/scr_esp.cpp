#include "scr_hal.h"
#include <esp_system.h>
#include <esp_timer.h>

static long __random(long limit)
{
    uint32_t x = esp_random();
    uint64_t m = uint64_t(x) * uint64_t(limit);
    uint32_t l = uint32_t(m);
    if (l < limit) {
        uint32_t t = -limit;
        if (t >= limit) {
            t -= limit;
            if (t >= limit)
                t %= limit;
        }
        while (l < t) {
            x = esp_random();
            m = uint64_t(x) * uint64_t(limit);
            l = uint32_t(m);
        }
    }
    return m >> 32;
}

long esp_random2(long s, long b)
{
    if(s >= b) {
        return s;
    }
    long diff = b - s;
    return __random(diff) + s;
}

int64_t esp_uptime() {
    return (int32_t)esp_timer_get_time();
}

