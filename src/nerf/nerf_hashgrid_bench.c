#include "nerf_simd.h"
#include <inttypes.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

typedef struct {
    int iterations;
    int logical_rays;
    int levels;
    int features_per_entry;
    size_t cold_cache_bytes;
    unsigned int seed;
    float span;
    int csv;
    int quiet;
    const char *pattern;
    const char *hashgrid_path;
    const char *occ_path;
} HashgridBenchOptions;

static uint64_t sm_now_ns(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC_RAW, &ts);
    return (uint64_t)ts.tv_sec * 1000000000ull + (uint64_t)ts.tv_nsec;
}

static int parse_int_arg(const char *arg, const char *name, int current) {
    char *end = NULL;
    long value = strtol(arg, &end, 10);
    if (!arg[0] || (end && *end != '\0')) {
        fprintf(stderr, "ERROR: Invalid integer for %s: %s\n", name, arg);
        return current;
    }
    return (int)value;
}

static size_t parse_size_arg(const char *arg, const char *name, size_t current) {
    char *end = NULL;
    unsigned long long value = strtoull(arg, &end, 10);
    if (!arg[0] || (end && *end != '\0')) {
        fprintf(stderr, "ERROR: Invalid size for %s: %s\n", name, arg);
        return current;
    }
    return (size_t)value;
}

static float parse_float_arg(const char *arg, const char *name, float current) {
    char *end = NULL;
    float value = strtof(arg, &end);
    if (!arg[0] || (end && *end != '\0')) {
        fprintf(stderr, "ERROR: Invalid float for %s: %s\n", name, arg);
        return current;
    }
    return value;
}

static void usage(const char *argv0) {
    printf("Usage: %s [options]\n", argv0);
    printf("  --iterations N           Loop count (default 1000)\n");
    printf("  --logical-rays N         Unique rays within the 8-lane batch (default 8)\n");
    printf("  --levels N               Override level count, clipped to output capacity\n");
    printf("  --features-per-entry N   Override features per entry, clipped to model limit\n");
    printf("  --pattern coherent|linear|random\n");
    printf("  --span F                 Spatial span around model center (default 1.0)\n");
    printf("  --cold-cache-bytes N     Thrash this many bytes before each lookup (default 0)\n");
    printf("  --seed N                 RNG seed for random mode (default 1337)\n");
    printf("  --csv                    Emit one CSV row instead of human text\n");
    printf("  --quiet                  Suppress setup banner in human mode\n");
    printf("  --hashgrid PATH          Hash-grid binary path\n");
    printf("  --occupancy PATH         Occupancy grid binary path\n");
}

static int parse_args(int argc, char **argv, HashgridBenchOptions *opt) {
    int i;
    for (i = 1; i < argc; i++) {
        const char *arg = argv[i];
        if (strcmp(arg, "--iterations") == 0 && i + 1 < argc) {
            opt->iterations = parse_int_arg(argv[++i], "--iterations", opt->iterations);
        } else if (strcmp(arg, "--logical-rays") == 0 && i + 1 < argc) {
            opt->logical_rays = parse_int_arg(argv[++i], "--logical-rays", opt->logical_rays);
        } else if (strcmp(arg, "--levels") == 0 && i + 1 < argc) {
            opt->levels = parse_int_arg(argv[++i], "--levels", opt->levels);
        } else if (strcmp(arg, "--features-per-entry") == 0 && i + 1 < argc) {
            opt->features_per_entry = parse_int_arg(argv[++i], "--features-per-entry", opt->features_per_entry);
        } else if (strcmp(arg, "--pattern") == 0 && i + 1 < argc) {
            opt->pattern = argv[++i];
        } else if (strcmp(arg, "--span") == 0 && i + 1 < argc) {
            opt->span = parse_float_arg(argv[++i], "--span", opt->span);
        } else if (strcmp(arg, "--cold-cache-bytes") == 0 && i + 1 < argc) {
            opt->cold_cache_bytes = parse_size_arg(argv[++i], "--cold-cache-bytes", opt->cold_cache_bytes);
        } else if (strcmp(arg, "--seed") == 0 && i + 1 < argc) {
            opt->seed = (unsigned int)parse_int_arg(argv[++i], "--seed", (int)opt->seed);
        } else if (strcmp(arg, "--csv") == 0) {
            opt->csv = 1;
        } else if (strcmp(arg, "--quiet") == 0) {
            opt->quiet = 1;
        } else if (strcmp(arg, "--hashgrid") == 0 && i + 1 < argc) {
            opt->hashgrid_path = argv[++i];
        } else if (strcmp(arg, "--occupancy") == 0 && i + 1 < argc) {
            opt->occ_path = argv[++i];
        } else if (strcmp(arg, "--help") == 0 || strcmp(arg, "-h") == 0) {
            usage(argv[0]);
            return 0;
        } else {
            fprintf(stderr, "ERROR: Unknown or incomplete argument: %s\n", arg);
            usage(argv[0]);
            return -1;
        }
    }
    return 1;
}

static uint32_t sm_lcg_next(uint32_t *state) {
    *state = (*state * 1664525u) + 1013904223u;
    return *state;
}

static float sm_unit_from_u32(uint32_t bits) {
    return (float)(bits & 0x00ffffffu) / (float)0x01000000u;
}

static void fill_positions(
    Vec3 positions[SIMD_BATCH_SIZE],
    const HashgridBenchOptions *opt,
    const NeRFConfig *config,
    int iter
) {
    int lane;
    uint32_t state = opt->seed + (unsigned int)(iter * 747796405u);
    float half_span = opt->span * 0.5f;
    int unique = opt->logical_rays;
    if (unique < 1) unique = 1;
    if (unique > SIMD_BATCH_SIZE) unique = SIMD_BATCH_SIZE;

    for (lane = 0; lane < unique; lane++) {
        Vec3 p = config->center;
        if (strcmp(opt->pattern, "coherent") == 0) {
            float t = (float)lane / (float)SIMD_BATCH_SIZE;
            p.x += (t - 0.5f) * opt->span * 0.125f;
            p.y += (float)((lane % 3) - 1) * opt->span * 0.03125f;
            p.z += (float)((lane % 2) ? 1 : -1) * opt->span * 0.015625f;
        } else if (strcmp(opt->pattern, "linear") == 0) {
            float t = (float)lane / (float)((unique > 1) ? (unique - 1) : 1);
            p.x += (t - 0.5f) * opt->span;
            p.y += (0.5f - t) * opt->span * 0.5f;
            p.z += sinf((float)(iter + lane) * 0.173f) * opt->span * 0.125f;
        } else {
            p.x += (sm_unit_from_u32(sm_lcg_next(&state)) * opt->span) - half_span;
            p.y += (sm_unit_from_u32(sm_lcg_next(&state)) * opt->span) - half_span;
            p.z += (sm_unit_from_u32(sm_lcg_next(&state)) * opt->span) - half_span;
        }
        positions[lane] = p;
    }

    for (lane = unique; lane < SIMD_BATCH_SIZE; lane++) {
        positions[lane] = positions[lane % unique];
    }
}

static void thrash_cache(uint8_t *buffer, size_t bytes, uint64_t *sink) {
    size_t i;
    uint64_t acc = *sink;
    for (i = 0; i < bytes; i += 64) {
        acc += buffer[i];
        buffer[i] = (uint8_t)(buffer[i] + (uint8_t)(i >> 6));
    }
    *sink = acc;
}

int main(int argc, char **argv) {
    HashgridBenchOptions opt;
    NeRFData *data;
    NeRFConfig config;
    Vec3 positions[SIMD_BATCH_SIZE];
    float features[SIMD_BATCH_SIZE][24];
    uint8_t *cold_cache = NULL;
    uint64_t cold_sink = 0;
    uint64_t t0;
    uint64_t t1;
    double total_us;
    double checksum = 0.0;
    int iter;
    int parse_status;

    memset(&opt, 0, sizeof(opt));
    opt.iterations = 1000;
    opt.logical_rays = SIMD_BATCH_SIZE;
    opt.levels = -1;
    opt.features_per_entry = -1;
    opt.cold_cache_bytes = 0;
    opt.seed = 1337u;
    opt.span = 1.0f;
    opt.pattern = "coherent";
    opt.hashgrid_path = "models/nerf_hashgrid.bin";
    opt.occ_path = "models/occupancy_grid.bin";

    parse_status = parse_args(argc, argv, &opt);
    if (parse_status <= 0) {
        return parse_status < 0 ? 1 : 0;
    }

    if (opt.iterations <= 0) {
        fprintf(stderr, "ERROR: iterations must be positive\n");
        return 1;
    }
    if (strcmp(opt.pattern, "coherent") != 0 &&
        strcmp(opt.pattern, "linear") != 0 &&
        strcmp(opt.pattern, "random") != 0) {
        fprintf(stderr, "ERROR: pattern must be coherent, linear, or random\n");
        return 1;
    }

    data = sm_nerf_data_load(opt.hashgrid_path, opt.occ_path);
    if (!data) {
        fprintf(stderr, "ERROR: failed to load NeRF data\n");
        return 1;
    }
    if (!data->hashgrid_data) {
        fprintf(stderr, "ERROR: nerf_hashgrid_bench currently requires float32 hash-grid storage\n");
        sm_nerf_data_free(data);
        return 1;
    }

    config = data->config;
    if (opt.features_per_entry > 0 && (uint32_t)opt.features_per_entry <= data->config.features_per_entry) {
        config.features_per_entry = (uint32_t)opt.features_per_entry;
    }
    if (opt.levels > 0 && (uint32_t)opt.levels <= data->config.num_levels) {
        config.num_levels = (uint32_t)opt.levels;
    }
    {
        uint32_t max_levels = 24u / config.features_per_entry;
        if (config.num_levels > max_levels) {
            config.num_levels = max_levels;
        }
    }

    if (opt.cold_cache_bytes > 0) {
        cold_cache = (uint8_t*)malloc(opt.cold_cache_bytes);
        if (!cold_cache) {
            fprintf(stderr, "ERROR: failed to allocate cold-cache buffer\n");
            sm_nerf_data_free(data);
            return 1;
        }
        memset(cold_cache, 1, opt.cold_cache_bytes);
    }

    if (!opt.csv && !opt.quiet) {
        printf("NeRF hash-grid focused benchmark\n");
        printf("  pattern=%s iterations=%d logical_rays=%d\n", opt.pattern, opt.iterations, opt.logical_rays);
        printf("  levels=%u features_per_entry=%u cold_cache_bytes=%zu span=%.3f\n",
               config.num_levels, config.features_per_entry, opt.cold_cache_bytes, opt.span);
        printf("  prefetch_env=%s compact_fp16=%d\n",
               getenv("SM_NERF_HASHGRID_PREFETCH") ? getenv("SM_NERF_HASHGRID_PREFETCH") : "<default>",
               data->hashgrid_compact);
    }

    t0 = sm_now_ns();
    for (iter = 0; iter < opt.iterations; iter++) {
        if (cold_cache) {
            thrash_cache(cold_cache, opt.cold_cache_bytes, &cold_sink);
        }
        fill_positions(positions, &opt, &config, iter);
        sm_hashgrid_lookup_batch(positions, &config, data->hashgrid_data, features);
        checksum += (double)features[0][0] + (double)features[0][1] + (double)features[SIMD_BATCH_SIZE - 1][0];
    }
    t1 = sm_now_ns();

    total_us = (double)(t1 - t0) / 1000.0;

    if (opt.csv) {
        printf("pattern=%s,iterations=%d,logical_rays=%d,levels=%u,features_per_entry=%u,"
               "cold_cache_bytes=%zu,compact_fp16=%d,prefetch_env=%s,total_us=%.2f,us_per_iter=%.4f,"
               "checksum=%.6f,cold_sink=%" PRIu64 "\n",
               opt.pattern,
               opt.iterations,
               opt.logical_rays,
               config.num_levels,
               config.features_per_entry,
               opt.cold_cache_bytes,
               data->hashgrid_compact,
               getenv("SM_NERF_HASHGRID_PREFETCH") ? getenv("SM_NERF_HASHGRID_PREFETCH") : "default",
               total_us,
               total_us / (double)opt.iterations,
               checksum,
               cold_sink);
    } else {
        printf("  total_us=%.2f\n", total_us);
        printf("  us_per_iter=%.4f\n", total_us / (double)opt.iterations);
        printf("  checksum=%.6f\n", checksum);
        printf("  cold_sink=%" PRIu64 "\n", cold_sink);
    }

    free(cold_cache);
    sm_nerf_data_free(data);
    return 0;
}
