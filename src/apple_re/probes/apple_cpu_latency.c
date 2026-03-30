#include "apple_re_time.h"

#include <inttypes.h>
#include <math.h>
#include <stdbool.h>
#include <stdatomic.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum {
    APPLE_RE_UNROLL = 32,
    APPLE_RE_TRANSC_UNROLL = 4,
    APPLE_RE_LOAD_STORE_UNROLL = 32,
    APPLE_RE_ATOMIC_UNROLL = 32,
};

typedef uint64_t (*bench_fn_t)(uint64_t iters);

struct probe_def {
    const char *name;
    bench_fn_t fn;
    uint32_t ops_per_iter;
};

static volatile uint64_t g_u64_sink = 0;
static volatile double g_f64_sink = 0.0;
static _Atomic uint64_t g_atomic_counter = 1;

static uint64_t bench_add_dep(uint64_t iters) {
    uint64_t x = 1;
    uint64_t start;
    uint64_t end;

    start = apple_re_now_ns();
    for (uint64_t i = 0; i < iters; ++i) {
#if defined(__aarch64__)
        asm volatile(
            "add %x0, %x0, #1\n\tadd %x0, %x0, #1\n\tadd %x0, %x0, #1\n\tadd %x0, %x0, #1\n\t"
            "add %x0, %x0, #1\n\tadd %x0, %x0, #1\n\tadd %x0, %x0, #1\n\tadd %x0, %x0, #1\n\t"
            "add %x0, %x0, #1\n\tadd %x0, %x0, #1\n\tadd %x0, %x0, #1\n\tadd %x0, %x0, #1\n\t"
            "add %x0, %x0, #1\n\tadd %x0, %x0, #1\n\tadd %x0, %x0, #1\n\tadd %x0, %x0, #1\n\t"
            "add %x0, %x0, #1\n\tadd %x0, %x0, #1\n\tadd %x0, %x0, #1\n\tadd %x0, %x0, #1\n\t"
            "add %x0, %x0, #1\n\tadd %x0, %x0, #1\n\tadd %x0, %x0, #1\n\tadd %x0, %x0, #1\n\t"
            "add %x0, %x0, #1\n\tadd %x0, %x0, #1\n\tadd %x0, %x0, #1\n\tadd %x0, %x0, #1\n\t"
            "add %x0, %x0, #1\n\tadd %x0, %x0, #1\n\tadd %x0, %x0, #1\n\tadd %x0, %x0, #1"
            : "+r"(x));
#else
        for (int lane = 0; lane < APPLE_RE_UNROLL; ++lane) {
            x += 1;
        }
#endif
    }
    end = apple_re_now_ns();
    g_u64_sink = x;
    return end - start;
}

static uint64_t bench_fadd_dep(uint64_t iters) {
    double x = 1.0;
    const double one = 1.0;
    uint64_t start;
    uint64_t end;

    start = apple_re_now_ns();
    for (uint64_t i = 0; i < iters; ++i) {
#if defined(__aarch64__)
        asm volatile(
            "fadd %d0, %d0, %d1\n\tfadd %d0, %d0, %d1\n\tfadd %d0, %d0, %d1\n\tfadd %d0, %d0, %d1\n\t"
            "fadd %d0, %d0, %d1\n\tfadd %d0, %d0, %d1\n\tfadd %d0, %d0, %d1\n\tfadd %d0, %d0, %d1\n\t"
            "fadd %d0, %d0, %d1\n\tfadd %d0, %d0, %d1\n\tfadd %d0, %d0, %d1\n\tfadd %d0, %d0, %d1\n\t"
            "fadd %d0, %d0, %d1\n\tfadd %d0, %d0, %d1\n\tfadd %d0, %d0, %d1\n\tfadd %d0, %d0, %d1\n\t"
            "fadd %d0, %d0, %d1\n\tfadd %d0, %d0, %d1\n\tfadd %d0, %d0, %d1\n\tfadd %d0, %d0, %d1\n\t"
            "fadd %d0, %d0, %d1\n\tfadd %d0, %d0, %d1\n\tfadd %d0, %d0, %d1\n\tfadd %d0, %d0, %d1\n\t"
            "fadd %d0, %d0, %d1\n\tfadd %d0, %d0, %d1\n\tfadd %d0, %d0, %d1\n\tfadd %d0, %d0, %d1\n\t"
            "fadd %d0, %d0, %d1\n\tfadd %d0, %d0, %d1\n\tfadd %d0, %d0, %d1\n\tfadd %d0, %d0, %d1"
            : "+w"(x)
            : "w"(one));
#else
        for (int lane = 0; lane < APPLE_RE_UNROLL; ++lane) {
            x += one;
        }
#endif
    }
    end = apple_re_now_ns();
    g_f64_sink = x;
    return end - start;
}

static uint64_t bench_fmadd_dep(uint64_t iters) {
    double acc = 1.0;
    const double a = 1.00000011920928955078125;
    const double b = 0.999999940395355224609375;
    uint64_t start;
    uint64_t end;

    start = apple_re_now_ns();
    for (uint64_t i = 0; i < iters; ++i) {
#if defined(__aarch64__)
        asm volatile(
            "fmadd %d0, %d0, %d1, %d2\n\tfmadd %d0, %d0, %d1, %d2\n\tfmadd %d0, %d0, %d1, %d2\n\tfmadd %d0, %d0, %d1, %d2\n\t"
            "fmadd %d0, %d0, %d1, %d2\n\tfmadd %d0, %d0, %d1, %d2\n\tfmadd %d0, %d0, %d1, %d2\n\tfmadd %d0, %d0, %d1, %d2\n\t"
            "fmadd %d0, %d0, %d1, %d2\n\tfmadd %d0, %d0, %d1, %d2\n\tfmadd %d0, %d0, %d1, %d2\n\tfmadd %d0, %d0, %d1, %d2\n\t"
            "fmadd %d0, %d0, %d1, %d2\n\tfmadd %d0, %d0, %d1, %d2\n\tfmadd %d0, %d0, %d1, %d2\n\tfmadd %d0, %d0, %d1, %d2\n\t"
            "fmadd %d0, %d0, %d1, %d2\n\tfmadd %d0, %d0, %d1, %d2\n\tfmadd %d0, %d0, %d1, %d2\n\tfmadd %d0, %d0, %d1, %d2\n\t"
            "fmadd %d0, %d0, %d1, %d2\n\tfmadd %d0, %d0, %d1, %d2\n\tfmadd %d0, %d0, %d1, %d2\n\tfmadd %d0, %d0, %d1, %d2\n\t"
            "fmadd %d0, %d0, %d1, %d2\n\tfmadd %d0, %d0, %d1, %d2\n\tfmadd %d0, %d0, %d1, %d2\n\tfmadd %d0, %d0, %d1, %d2\n\t"
            "fmadd %d0, %d0, %d1, %d2\n\tfmadd %d0, %d0, %d1, %d2\n\tfmadd %d0, %d0, %d1, %d2\n\tfmadd %d0, %d0, %d1, %d2"
            : "+w"(acc)
            : "w"(a), "w"(b));
#else
        for (int lane = 0; lane < APPLE_RE_UNROLL; ++lane) {
            acc = (acc * a) + b;
        }
#endif
    }
    end = apple_re_now_ns();
    g_f64_sink = acc;
    return end - start;
}

static uint64_t bench_load_store_chain(uint64_t iters) {
    volatile uint64_t lanes[APPLE_RE_LOAD_STORE_UNROLL];
    uint64_t x = 0x9e3779b97f4a7c15ull;
    uint64_t start;
    uint64_t end;

    for (uint32_t lane = 0; lane < APPLE_RE_LOAD_STORE_UNROLL; ++lane) {
        lanes[lane] = (uint64_t)(lane + 1u);
    }

    start = apple_re_now_ns();
    for (uint64_t i = 0; i < iters; ++i) {
        for (uint32_t lane = 0; lane < APPLE_RE_LOAD_STORE_UNROLL; ++lane) {
            uint64_t loaded = lanes[lane];
            x ^= loaded + (uint64_t)lane;
            x = (x << 1) | (x >> 63);
            lanes[lane] = x + loaded;
        }
    }
    end = apple_re_now_ns();
    g_u64_sink = x ^ lanes[(uint32_t)(x & (APPLE_RE_LOAD_STORE_UNROLL - 1u))];
    return end - start;
}

static uint64_t bench_shuffle_dep(uint64_t iters) {
    uint64_t x = 0x0123456789abcdefull;
    uint64_t start;
    uint64_t end;

    start = apple_re_now_ns();
    for (uint64_t i = 0; i < iters; ++i) {
        for (uint32_t lane = 0; lane < APPLE_RE_UNROLL; ++lane) {
            uint64_t mixed = x ^ ((uint64_t)lane * 0x9e3779b97f4a7c15ull);
            uint32_t shift = (lane & 15u) + 1u;
            mixed = (mixed >> shift) | (mixed << (64u - shift));
            x = __builtin_bswap64(mixed);
        }
    }
    end = apple_re_now_ns();
    g_u64_sink = x;
    return end - start;
}

static uint64_t bench_atomic_add_dep(uint64_t iters) {
    uint64_t acc = 0;
    uint64_t start;
    uint64_t end;

    atomic_store_explicit(&g_atomic_counter, 1, memory_order_relaxed);
    start = apple_re_now_ns();
    for (uint64_t i = 0; i < iters; ++i) {
        for (uint32_t lane = 0; lane < APPLE_RE_ATOMIC_UNROLL; ++lane) {
            acc = atomic_fetch_add_explicit(&g_atomic_counter, 1, memory_order_relaxed);
        }
    }
    end = apple_re_now_ns();
    g_u64_sink = acc;
    return end - start;
}

static uint64_t bench_transcendental_dep(uint64_t iters) {
    double x = 0.123456789;
    uint64_t start;
    uint64_t end;

    start = apple_re_now_ns();
    for (uint64_t i = 0; i < iters; ++i) {
        for (uint32_t lane = 0; lane < APPLE_RE_TRANSC_UNROLL; ++lane) {
            const double t = sin(x + (double)(lane + 1u) * 0.001);
            x = cos(t + x);
        }
    }
    end = apple_re_now_ns();
    g_f64_sink = x;
    return end - start;
}

static void print_result(const char *name, uint64_t elapsed_ns, uint64_t iters, uint32_t ops_per_iter, bool csv) {
    const double ops = (double)iters * (double)ops_per_iter;
    const double ns_per_op = (ops > 0.0) ? ((double)elapsed_ns / ops) : 0.0;
    const double ops_per_ns = (elapsed_ns > 0) ? (ops / (double)elapsed_ns) : 0.0;
    const double gops = ops_per_ns;

    if (csv) {
        printf("%s,%" PRIu64 ",%u,%" PRIu64 ",%.6f,%.6f\n",
               name, iters, ops_per_iter, elapsed_ns, ns_per_op, gops);
        return;
    }

    printf("%-24s elapsed_ns=%" PRIu64 " ns/op=%.6f approx_Gops=%.6f\n",
           name, elapsed_ns, ns_per_op, gops);
}

static const struct probe_def g_probes[] = {
    {"add_dep_u64", bench_add_dep, APPLE_RE_UNROLL},
    {"fadd_dep_f64", bench_fadd_dep, APPLE_RE_UNROLL},
    {"fmadd_dep_f64", bench_fmadd_dep, APPLE_RE_UNROLL},
    {"load_store_chain_u64", bench_load_store_chain, APPLE_RE_LOAD_STORE_UNROLL * 3u},
    {"shuffle_lane_cross_u64", bench_shuffle_dep, APPLE_RE_UNROLL},
    {"atomic_add_relaxed_u64", bench_atomic_add_dep, APPLE_RE_ATOMIC_UNROLL},
    {"transcendental_sin_cos_f64", bench_transcendental_dep, APPLE_RE_TRANSC_UNROLL * 2u},
};

int main(int argc, char **argv) {
    uint64_t iters = 1000000;
    bool csv = false;
    const char *probe_filter = NULL;

    for (int i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "--iters") == 0 && i + 1 < argc) {
            iters = strtoull(argv[++i], NULL, 10);
        } else if (strcmp(argv[i], "--probe") == 0 && i + 1 < argc) {
            probe_filter = argv[++i];
        } else if (strcmp(argv[i], "--csv") == 0) {
            csv = true;
        } else if (strcmp(argv[i], "--list-probes") == 0) {
            for (size_t j = 0; j < (sizeof(g_probes) / sizeof(g_probes[0])); ++j) {
                puts(g_probes[j].name);
            }
            return 0;
        } else if (strcmp(argv[i], "--help") == 0) {
            printf("usage: %s [--iters N] [--probe NAME] [--list-probes] [--csv]\n", argv[0]);
            return 0;
        } else {
            fprintf(stderr, "unknown argument: %s\n", argv[i]);
            return 1;
        }
    }

    if (csv) {
        printf("probe,iters,unroll,elapsed_ns,ns_per_op,approx_gops\n");
    }

    bool ran_any = false;
    for (size_t i = 0; i < (sizeof(g_probes) / sizeof(g_probes[0])); ++i) {
        const struct probe_def *probe = &g_probes[i];
        if (probe_filter != NULL && strcmp(probe_filter, probe->name) != 0) {
            continue;
        }
        ran_any = true;
        print_result(probe->name, probe->fn(iters), iters, probe->ops_per_iter, csv);
    }

    if (!ran_any) {
        fprintf(stderr, "no probe matched --probe %s\n", probe_filter ? probe_filter : "(null)");
        return 1;
    }

    return 0;
}
