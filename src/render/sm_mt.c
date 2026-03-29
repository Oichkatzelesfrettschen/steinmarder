// sm_mt.c -- thread count detection for multi-threaded renderer
#include "sm_mt.h"
#include <stdlib.h>

#if defined(_WIN32)
#include <windows.h>
#elif defined(__linux__) || defined(__APPLE__) || defined(__unix__)
#include <unistd.h>
#endif

int sm_mt_suggest_threads(void) {
    // 1) Env override: SM_THREADS
    const char *env = getenv("SM_THREADS");
    if (env && env[0]) {
        int v = atoi(env);
        if (v > 0) return v;
    }

    // 2) Platform detection
#if defined(_WIN32)
    SYSTEM_INFO sysinfo;
    GetSystemInfo(&sysinfo);
    int n = (int)sysinfo.dwNumberOfProcessors;
    return (n > 0) ? n : 4;
#elif defined(_SC_NPROCESSORS_ONLN)
    long n = sysconf(_SC_NPROCESSORS_ONLN);
    return (n > 0) ? (int)n : 4;
#else
    return 4;
#endif
}
