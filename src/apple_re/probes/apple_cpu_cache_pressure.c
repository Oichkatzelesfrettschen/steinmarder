#include "apple_re_time.h"

#include <inttypes.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum {
    CACHE_LINE_BYTES = 64,
};

typedef uint64_t (*family_fn_t)(uint64_t *buf, size_t elems, size_t stride_elems, uint64_t passes, uint64_t *accesses_out);

static volatile uint64_t g_sink = 0;

static size_t parse_size_kib(uint64_t size_kib) {
    return (size_t)size_kib * 1024u;
}

static size_t stride_to_elems(size_t stride_bytes) {
    size_t elems = stride_bytes / sizeof(uint64_t);
    return elems > 0 ? elems : 1u;
}

static uint64_t run_stream(uint64_t *buf, size_t elems, size_t stride_elems, uint64_t passes, uint64_t *accesses_out) {
    uint64_t acc = 0x9e3779b97f4a7c15ull;
    uint64_t accesses = 0;
    uint64_t start = apple_re_now_ns();

    for (uint64_t pass = 0; pass < passes; ++pass) {
        for (size_t i = 0; i < elems; i += stride_elems) {
            uint64_t v = buf[i];
            acc ^= v + (uint64_t)i + pass;
            buf[i] = acc + v + (uint64_t)i;
            ++accesses;
        }
    }

    uint64_t end = apple_re_now_ns();
    g_sink = acc ^ buf[(size_t)(acc & (uint64_t)(elems - 1u))];
    if (accesses_out) {
        *accesses_out = accesses;
    }
    return end - start;
}

static uint64_t run_pointer_chase(uint64_t *buf, size_t elems, size_t stride_elems, uint64_t passes, uint64_t *accesses_out) {
    uint64_t acc = 0x0123456789abcdefull;
    size_t idx = 0;
    uint64_t accesses = 0;

    for (size_t i = 0; i < elems; ++i) {
        buf[i] = (uint64_t)((i + stride_elems) % elems);
    }

    uint64_t start = apple_re_now_ns();
    for (uint64_t pass = 0; pass < passes; ++pass) {
        for (size_t i = 0; i < elems; ++i) {
            idx = (size_t)buf[idx];
            acc ^= (uint64_t)idx + pass;
            ++accesses;
        }
    }
    uint64_t end = apple_re_now_ns();

    g_sink = acc ^ (uint64_t)idx;
    if (accesses_out) {
        *accesses_out = accesses;
    }
    return end - start;
}

static uint64_t run_reuse(uint64_t *buf, size_t elems, size_t stride_elems, uint64_t passes, uint64_t *accesses_out) {
    uint64_t acc = 0x6a09e667f3bcc909ull;
    uint64_t accesses = 0;
    uint64_t start = apple_re_now_ns();

    for (uint64_t pass = 0; pass < passes; ++pass) {
        for (size_t i = 0; i < elems; i += stride_elems) {
            uint64_t v = buf[i];
            buf[i] = v + acc + pass;
            acc ^= buf[i] + (uint64_t)i;
            ++accesses;
        }
        for (size_t i = elems; i-- > 0;) {
            if ((i % stride_elems) != 0) {
                continue;
            }
            uint64_t v = buf[i];
            buf[i] = v + acc + (uint64_t)i;
            acc += buf[i] ^ (uint64_t)pass;
            ++accesses;
        }
    }

    uint64_t end = apple_re_now_ns();
    g_sink = acc ^ buf[(size_t)(acc & (uint64_t)(elems - 1u))];
    if (accesses_out) {
        *accesses_out = accesses;
    }
    return end - start;
}

static void print_result(
    const char *family,
    uint64_t size_kib,
    uint64_t size_bytes,
    uint64_t stride_bytes,
    uint64_t passes,
    uint64_t accesses,
    uint64_t elapsed_ns,
    bool csv
) {
    const double access_rate = (elapsed_ns > 0) ? ((double)accesses / ((double)elapsed_ns / 1e9)) : 0.0;
    const double ns_per_access = (accesses > 0) ? ((double)elapsed_ns / (double)accesses) : 0.0;
    const double gaccesses_per_sec = access_rate / 1e9;

    if (csv) {
        printf(
            "%s,%" PRIu64 ",%" PRIu64 ",%" PRIu64 ",%" PRIu64 ",%" PRIu64 ",%" PRIu64 ",%.6f,%.6f,%.6f\n",
            family,
            size_kib,
            size_bytes,
            stride_bytes,
            passes,
            accesses,
            elapsed_ns,
            ns_per_access,
            access_rate,
            gaccesses_per_sec
        );
        return;
    }

    printf(
        "%-16s size_kib=%" PRIu64 " stride_bytes=%" PRIu64 " passes=%" PRIu64 " accesses=%" PRIu64
        " elapsed_ns=%" PRIu64 " ns/access=%.6f gaccesses/s=%.6f\n",
        family,
        size_kib,
        stride_bytes,
        passes,
        accesses,
        elapsed_ns,
        ns_per_access,
        gaccesses_per_sec
    );
}

static void list_families(void) {
    puts("stream");
    puts("pointer");
    puts("reuse");
}

static void usage(const char *argv0) {
    printf(
        "usage: %s [--family NAME] [--size-kib N] [--stride-bytes N] [--passes N] [--csv] [--list-families]\n",
        argv0
    );
}

int main(int argc, char **argv) {
    const char *family = "stream";
    uint64_t size_kib = 64;
    uint64_t stride_bytes = CACHE_LINE_BYTES;
    uint64_t passes = 64;
    bool csv = false;

    for (int i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "--family") == 0 && i + 1 < argc) {
            family = argv[++i];
        } else if (strcmp(argv[i], "--size-kib") == 0 && i + 1 < argc) {
            size_kib = strtoull(argv[++i], NULL, 10);
        } else if (strcmp(argv[i], "--stride-bytes") == 0 && i + 1 < argc) {
            stride_bytes = strtoull(argv[++i], NULL, 10);
        } else if (strcmp(argv[i], "--passes") == 0 && i + 1 < argc) {
            passes = strtoull(argv[++i], NULL, 10);
        } else if (strcmp(argv[i], "--csv") == 0) {
            csv = true;
        } else if (strcmp(argv[i], "--list-families") == 0) {
            list_families();
            return 0;
        } else if (strcmp(argv[i], "--help") == 0) {
            usage(argv[0]);
            return 0;
        } else {
            fprintf(stderr, "unknown argument: %s\n", argv[i]);
            return 1;
        }
    }

    const size_t size_bytes = parse_size_kib(size_kib);
    const size_t elems = size_bytes / sizeof(uint64_t);
    const size_t stride_elems = stride_to_elems((size_t)stride_bytes);

    if (elems == 0) {
        fprintf(stderr, "size too small: %" PRIu64 " KiB\n", size_kib);
        return 1;
    }

    uint64_t *buf = NULL;
    if (posix_memalign((void **)&buf, CACHE_LINE_BYTES, elems * sizeof(uint64_t)) != 0 || buf == NULL) {
        fprintf(stderr, "posix_memalign failed for size=%zu bytes\n", size_bytes);
        return 1;
    }

    for (size_t i = 0; i < elems; ++i) {
        buf[i] = (uint64_t)(i + 1u);
    }

    uint64_t accesses = 0;
    uint64_t elapsed_ns = 0;
    if (strcmp(family, "stream") == 0) {
        elapsed_ns = run_stream(buf, elems, stride_elems, passes, &accesses);
    } else if (strcmp(family, "pointer") == 0) {
        elapsed_ns = run_pointer_chase(buf, elems, stride_elems, passes, &accesses);
    } else if (strcmp(family, "reuse") == 0) {
        elapsed_ns = run_reuse(buf, elems, stride_elems, passes, &accesses);
    } else {
        fprintf(stderr, "unknown family: %s\n", family);
        free(buf);
        return 1;
    }

    if (csv) {
        printf("family,size_kib,size_bytes,stride_bytes,passes,accesses,elapsed_ns,ns_per_access,accesses_per_sec,approx_gaccesses_per_sec\n");
    }
    print_result(family, size_kib, (uint64_t)size_bytes, stride_bytes, passes, accesses, elapsed_ns, csv);

    free(buf);
    return 0;
}
