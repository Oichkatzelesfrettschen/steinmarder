#include "common.h"

#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char **argv) {
    SMZenProbeConfig cfg = sm_zen_default_config();
    SMZenFeatures features = sm_zen_detect_features();
    uint32_t *pattern;
    size_t count;
    volatile uint64_t sink = 0u;
    uint64_t begin;
    uint64_t end;
    uint64_t i;
    double predictable_cycles;
    double random_cycles;

    sm_zen_parse_common_args(argc, argv, &cfg);
    sm_zen_pin_to_cpu(cfg.cpu);
    sm_zen_print_header("branch", &cfg, &features);

    count = cfg.working_set_bytes / sizeof(uint32_t);
    if(count < 4096u) {
        count = 4096u;
    }
    pattern = (uint32_t *)sm_zen_aligned_alloc64(count * sizeof(uint32_t));
    sm_zen_fill_random_u32(pattern, count, 0x12345678u);

    begin = sm_zen_tsc_begin();
    for(i = 0; i < cfg.iterations; ++i) {
        if((i & 1u) == 0u) {
            sink += 3u;
        } else {
            sink += 1u;
        }
    }
    end = sm_zen_tsc_end();
    predictable_cycles = sm_zen_cycles_per_iter(end - begin, cfg.iterations);

    begin = sm_zen_tsc_begin();
    for(i = 0; i < cfg.iterations; ++i) {
        if((pattern[i % count] & 1u) != 0u) {
            sink += 3u;
        } else {
            sink += 1u;
        }
    }
    end = sm_zen_tsc_end();
    random_cycles = sm_zen_cycles_per_iter(end - begin, cfg.iterations);

    printf("predictable_branch_cycles=%0.4f\n", predictable_cycles);
    printf("unpredictable_branch_cycles=%0.4f\n", random_cycles);
    printf("branch_penalty_delta=%0.4f\n", random_cycles - predictable_cycles);
    printf("sink=%" PRIu64 "\n", sink);
    if(cfg.csv) {
        printf("csv,probe=branch,predictable=%0.6f,unpredictable=%0.6f,delta=%0.6f\n",
               predictable_cycles, random_cycles, random_cycles - predictable_cycles);
    }

    free(pattern);
    return 0;
}
