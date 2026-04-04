#ifndef APPLE_RE_TIME_H
#define APPLE_RE_TIME_H

#include <stdint.h>

#if defined(__APPLE__)
#include <mach/mach_time.h>

static inline uint64_t apple_re_now_ns(void) {
    static mach_timebase_info_data_t timebase = {0, 0};
    uint64_t ticks;

    if (timebase.denom == 0) {
        (void)mach_timebase_info(&timebase);
    }

    ticks = mach_absolute_time();
    return (ticks * (uint64_t)timebase.numer) / (uint64_t)timebase.denom;
}
#else
#include <time.h>

static inline uint64_t apple_re_now_ns(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ((uint64_t)ts.tv_sec * 1000000000ull) + (uint64_t)ts.tv_nsec;
}
#endif

#endif
