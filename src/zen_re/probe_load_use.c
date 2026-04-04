#include "common.h"

#include <stdio.h>
#include <stdlib.h>

int main(int argc, char **argv) {
    SMZenProbeConfig cfg = sm_zen_default_config();
    SMZenFeatures features = sm_zen_detect_features();
    size_t count;
    uint32_t *next;
    uint32_t index = 0u;
    uint64_t begin;
    uint64_t end;
    uint64_t i;
    volatile uint32_t sink;
    double cycles_per_load;

    sm_zen_parse_common_args(argc, argv, &cfg);
    sm_zen_pin_to_cpu(cfg.cpu);
    sm_zen_print_header("load_use", &cfg, &features);

    count = cfg.working_set_bytes / sizeof(uint32_t);
    if(count < 4096u) {
        count = 4096u;
    }

    next = (uint32_t *)sm_zen_aligned_alloc64(count * sizeof(uint32_t));
    for(i = 0; i < count; ++i) {
        next[i] = (uint32_t)((i * 167u + 13u) % count);
    }

    begin = sm_zen_tsc_begin();
    for(i = 0; i < cfg.iterations; ++i) {
        index = next[index];
    }
    end = sm_zen_tsc_end();
    sink = index;
    cycles_per_load = sm_zen_cycles_per_iter(end - begin, cfg.iterations);

    printf("pointer_chase_cycles_per_load=%0.4f\n", cycles_per_load);
    printf("sink=%u\n", sink);
    if(cfg.csv) {
        printf("csv,probe=load_use,pointer_chase_cycles_per_load=%0.6f\n", cycles_per_load);
    }

    free(next);
    return 0;
}
