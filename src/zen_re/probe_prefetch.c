#include "common.h"

#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char **argv) {
    SMZenProbeConfig cfg = sm_zen_default_config();
    SMZenFeatures features = sm_zen_detect_features();
    uint32_t *data;
    size_t count;
    volatile uint64_t sink = 0u;
    uint64_t begin;
    uint64_t end;
    double no_prefetch_cycles;
    double prefetch_cycles;
    size_t i;

    sm_zen_parse_common_args(argc, argv, &cfg);
    sm_zen_pin_to_cpu(cfg.cpu);
    sm_zen_print_header("prefetch", &cfg, &features);

    count = cfg.working_set_bytes / sizeof(uint32_t);
    if(count < 16384u) {
        count = 16384u;
    }

    data = (uint32_t *)sm_zen_aligned_alloc64(count * sizeof(uint32_t));
    sm_zen_fill_random_u32(data, count, 0xBADC0DEu);

    begin = sm_zen_tsc_begin();
    for(i = 0; i < count; ++i) {
        sink += data[i];
    }
    end = sm_zen_tsc_end();
    no_prefetch_cycles = (double)(end - begin) / (double)count;

    begin = sm_zen_tsc_begin();
    for(i = 0; i < count; ++i) {
        size_t pf = i + 32u;
        if(pf < count) {
#if defined(__GNUC__) || defined(__clang__)
            __builtin_prefetch(&data[pf], 0, 1);
#endif
        }
        sink += data[i];
    }
    end = sm_zen_tsc_end();
    prefetch_cycles = (double)(end - begin) / (double)count;

    printf("no_prefetch_cycles_per_elem=%0.4f\n", no_prefetch_cycles);
    printf("prefetch_cycles_per_elem=%0.4f\n", prefetch_cycles);
    printf("prefetch_delta=%0.4f\n", prefetch_cycles - no_prefetch_cycles);
    printf("sink=%" PRIu64 "\n", sink);
    if(cfg.csv) {
        printf("csv,probe=prefetch,no_prefetch=%0.6f,prefetch=%0.6f,delta=%0.6f\n",
               no_prefetch_cycles, prefetch_cycles, prefetch_cycles - no_prefetch_cycles);
    }

    free(data);
    return 0;
}
