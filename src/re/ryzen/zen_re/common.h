#ifndef SM_ZEN_RE_COMMON_H
#define SM_ZEN_RE_COMMON_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef struct {
    bool has_avx2;
    bool has_fma;
    bool has_avx512f;
    bool has_bmi2;
    bool has_3dnow_prefetch;
} SMZenFeatures;

typedef struct {
    uint64_t iterations;
    size_t working_set_bytes;
    int cpu;
    bool csv;
} SMZenProbeConfig;

SMZenFeatures sm_zen_detect_features(void);
SMZenProbeConfig sm_zen_default_config(void);
void sm_zen_parse_common_args(int argc, char **argv, SMZenProbeConfig *cfg);
bool sm_zen_pin_to_cpu(int cpu);
void *sm_zen_aligned_alloc64(size_t bytes);
void sm_zen_fill_random_u32(uint32_t *dst, size_t count, uint32_t seed);
uint64_t sm_zen_tsc_begin(void);
uint64_t sm_zen_tsc_end(void);
double sm_zen_cycles_per_iter(uint64_t cycles, uint64_t iterations);
void sm_zen_print_header(const char *probe, const SMZenProbeConfig *cfg, const SMZenFeatures *features);

#endif
