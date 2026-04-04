#define _GNU_SOURCE

#include "common.h"

#include <errno.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if defined(__x86_64__) || defined(_M_X64)
#include <cpuid.h>
#include <immintrin.h>
#endif

#ifdef __linux__
#include <sched.h>
#endif

static uint32_t sm_xorshift32(uint32_t *state) {
    uint32_t x = *state;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    *state = x;
    return x;
}

SMZenProbeConfig sm_zen_default_config(void) {
    SMZenProbeConfig cfg;
    cfg.iterations = 1000000u;
    cfg.working_set_bytes = 64u * 1024u * 1024u;
    cfg.cpu = 0;
    cfg.csv = false;
    return cfg;
}

SMZenFeatures sm_zen_detect_features(void) {
    SMZenFeatures features = {0};
#if defined(__x86_64__) || defined(_M_X64)
    uint32_t eax = 0;
    uint32_t ebx = 0;
    uint32_t ecx = 0;
    uint32_t edx = 0;

    __cpuid_count(1, 0, eax, ebx, ecx, edx);
    features.has_fma = (ecx & (1u << 12)) != 0;

    __cpuid_count(7, 0, eax, ebx, ecx, edx);
    features.has_avx2 = (ebx & (1u << 5)) != 0;
    features.has_bmi2 = (ebx & (1u << 8)) != 0;
    features.has_avx512f = (ebx & (1u << 16)) != 0;

    __cpuid_count(0x80000001u, 0, eax, ebx, ecx, edx);
    features.has_3dnow_prefetch = (ecx & (1u << 8)) != 0;
#endif
    return features;
}

static uint64_t sm_parse_u64_arg(const char *arg, const char *name) {
    char *end = NULL;
    unsigned long long value = strtoull(arg, &end, 10);
    if(arg[0] == '\0' || end == arg || *end != '\0') {
        fprintf(stderr, "invalid numeric value for %s: %s\n", name, arg);
        exit(2);
    }
    return (uint64_t)value;
}

void sm_zen_parse_common_args(int argc, char **argv, SMZenProbeConfig *cfg) {
    int i;
    for(i = 1; i < argc; ++i) {
        if(strcmp(argv[i], "--iterations") == 0 && i + 1 < argc) {
            cfg->iterations = sm_parse_u64_arg(argv[++i], "--iterations");
        } else if(strcmp(argv[i], "--size") == 0 && i + 1 < argc) {
            cfg->working_set_bytes = (size_t)sm_parse_u64_arg(argv[++i], "--size");
        } else if(strcmp(argv[i], "--cpu") == 0 && i + 1 < argc) {
            cfg->cpu = (int)sm_parse_u64_arg(argv[++i], "--cpu");
        } else if(strcmp(argv[i], "--csv") == 0) {
            cfg->csv = true;
        } else if(strcmp(argv[i], "--help") == 0) {
            printf("  --iterations N   benchmark loop count\n");
            printf("  --size BYTES     working-set size for memory-oriented probes\n");
            printf("  --cpu ID         pin the process to a CPU core (Linux)\n");
            printf("  --csv            emit one CSV summary line after the prose header\n");
            exit(0);
        } else {
            fprintf(stderr, "unknown argument: %s\n", argv[i]);
            exit(2);
        }
    }
}

bool sm_zen_pin_to_cpu(int cpu) {
#ifdef __linux__
    cpu_set_t set;
    CPU_ZERO(&set);
    CPU_SET(cpu, &set);
    return sched_setaffinity(0, sizeof(set), &set) == 0;
#else
    (void)cpu;
    return false;
#endif
}

void *sm_zen_aligned_alloc64(size_t bytes) {
    void *ptr = NULL;
#if defined(_ISOC11_SOURCE)
    size_t rounded = (bytes + 63u) & ~((size_t)63u);
    ptr = aligned_alloc(64u, rounded);
#else
    if(posix_memalign(&ptr, 64u, bytes) != 0) {
        ptr = NULL;
    }
#endif
    if(!ptr) {
        fprintf(stderr, "allocation failed for %zu bytes\n", bytes);
        exit(1);
    }
    return ptr;
}

void sm_zen_fill_random_u32(uint32_t *dst, size_t count, uint32_t seed) {
    size_t i;
    uint32_t state = seed ? seed : 0x12345678u;
    for(i = 0; i < count; ++i) {
        dst[i] = sm_xorshift32(&state);
    }
}

uint64_t sm_zen_tsc_begin(void) {
#if defined(__x86_64__) || defined(_M_X64)
    _mm_lfence();
    return __rdtsc();
#else
    return 0;
#endif
}

uint64_t sm_zen_tsc_end(void) {
#if defined(__x86_64__) || defined(_M_X64)
    unsigned int aux = 0;
    uint64_t t = __rdtscp(&aux);
    _mm_lfence();
    return t;
#else
    return 0;
#endif
}

double sm_zen_cycles_per_iter(uint64_t cycles, uint64_t iterations) {
    if(iterations == 0u) {
        return 0.0;
    }
    return (double)cycles / (double)iterations;
}

void sm_zen_print_header(const char *probe, const SMZenProbeConfig *cfg, const SMZenFeatures *features) {
    fprintf(stdout, "probe=%s\n", probe);
    fprintf(stdout, "iterations=%" PRIu64 "\n", cfg->iterations);
    fprintf(stdout, "working_set_bytes=%zu\n", cfg->working_set_bytes);
    fprintf(stdout, "cpu=%d\n", cfg->cpu);
    fprintf(stdout, "avx2=%d fma=%d avx512f=%d bmi2=%d 3dnow_prefetch=%d\n",
            features->has_avx2 ? 1 : 0,
            features->has_fma ? 1 : 0,
            features->has_avx512f ? 1 : 0,
            features->has_bmi2 ? 1 : 0,
            features->has_3dnow_prefetch ? 1 : 0);
}
