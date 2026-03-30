#include "apple_re_time.h"

#include <inttypes.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum {
    APPLE_RE_UNROLL = 32
};

static volatile uint64_t g_u64_sink = 0;
static volatile double g_f64_sink = 0.0;

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

static void print_result(const char *name, uint64_t elapsed_ns, uint64_t iters, bool csv) {
    const double ops = (double)iters * (double)APPLE_RE_UNROLL;
    const double ns_per_op = (ops > 0.0) ? ((double)elapsed_ns / ops) : 0.0;
    const double ops_per_ns = (elapsed_ns > 0) ? (ops / (double)elapsed_ns) : 0.0;
    const double gops = ops_per_ns;

    if (csv) {
        printf("%s,%" PRIu64 ",%d,%" PRIu64 ",%.6f,%.6f\n",
               name, iters, APPLE_RE_UNROLL, elapsed_ns, ns_per_op, gops);
        return;
    }

    printf("%-16s elapsed_ns=%" PRIu64 " ns/op=%.6f approx_Gops=%.6f\n",
           name, elapsed_ns, ns_per_op, gops);
}

int main(int argc, char **argv) {
    uint64_t iters = 1000000;
    bool csv = false;

    for (int i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "--iters") == 0 && i + 1 < argc) {
            iters = strtoull(argv[++i], NULL, 10);
        } else if (strcmp(argv[i], "--csv") == 0) {
            csv = true;
        } else if (strcmp(argv[i], "--help") == 0) {
            printf("usage: %s [--iters N] [--csv]\n", argv[0]);
            return 0;
        } else {
            fprintf(stderr, "unknown argument: %s\n", argv[i]);
            return 1;
        }
    }

    if (csv) {
        printf("probe,iters,unroll,elapsed_ns,ns_per_op,approx_gops\n");
    }

    print_result("add_dep_u64", bench_add_dep(iters), iters, csv);
    print_result("fadd_dep_f64", bench_fadd_dep(iters), iters, csv);
    print_result("fmadd_dep_f64", bench_fmadd_dep(iters), iters, csv);
    return 0;
}
