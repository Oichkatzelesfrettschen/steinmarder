#include "nerf_simd.h"
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#ifdef __AVX2__
#include <immintrin.h>
#endif

typedef struct {
    int iterations;
    const char *mode;
    const char *pattern;
    unsigned int seed;
    int csv;
    const char *hashgrid_path;
    const char *occ_path;
} MLPBenchOptions;

typedef struct {
    int ready;
    float *w0_aligned;
    float *w1_aligned;
    float *wout_t_aligned;
    float *b0_aligned;
    float *b1_aligned;
    float *bout_aligned;
} MLPPrepack;

static void *sm_aligned_calloc(size_t alignment, size_t count, size_t elem_size) {
    void *ptr = NULL;
    size_t size = count * elem_size;
#if defined(_ISOC11_SOURCE)
    ptr = aligned_alloc(alignment, (size + alignment - 1u) / alignment * alignment);
    if (ptr) memset(ptr, 0, (size + alignment - 1u) / alignment * alignment);
#else
    if (posix_memalign(&ptr, alignment, size) != 0) {
        ptr = NULL;
    } else {
        memset(ptr, 0, size);
    }
#endif
    return ptr;
}

static void sm_mlp_prepack_init(MLPPrepack *prepack, const NeRFData *data) {
    uint32_t in_dim = data->config.mlp_in_dim;
    uint32_t hidden_dim = data->config.mlp_hidden_dim;
    uint32_t out_dim = data->config.mlp_out_dim;
    const float *w0 = data->mlp_weights;
    const float *b0 = data->mlp_biases;
    const float *w1 = w0 + (size_t)hidden_dim * in_dim;
    const float *b1 = b0 + hidden_dim;
    const float *w_out = w1 + (size_t)hidden_dim * hidden_dim;
    const float *b_out = b1 + hidden_dim;
    size_t w0_elems = (size_t)in_dim * hidden_dim;
    size_t w1_elems = (size_t)hidden_dim * hidden_dim;
    size_t wout_t_elems = (size_t)out_dim * hidden_dim;

    memset(prepack, 0, sizeof(*prepack));
    prepack->w0_aligned = (float*)sm_aligned_calloc(32u, w0_elems, sizeof(float));
    prepack->w1_aligned = (float*)sm_aligned_calloc(32u, w1_elems, sizeof(float));
    prepack->wout_t_aligned = (float*)sm_aligned_calloc(32u, wout_t_elems, sizeof(float));
    prepack->b0_aligned = (float*)sm_aligned_calloc(32u, hidden_dim, sizeof(float));
    prepack->b1_aligned = (float*)sm_aligned_calloc(32u, hidden_dim, sizeof(float));
    prepack->bout_aligned = (float*)sm_aligned_calloc(32u, out_dim, sizeof(float));
    if (!prepack->w0_aligned || !prepack->w1_aligned || !prepack->wout_t_aligned ||
        !prepack->b0_aligned || !prepack->b1_aligned || !prepack->bout_aligned) {
        free(prepack->w0_aligned);
        free(prepack->w1_aligned);
        free(prepack->wout_t_aligned);
        free(prepack->b0_aligned);
        free(prepack->b1_aligned);
        free(prepack->bout_aligned);
        memset(prepack, 0, sizeof(*prepack));
        return;
    }

    memcpy(prepack->w0_aligned, w0, w0_elems * sizeof(float));
    memcpy(prepack->w1_aligned, w1, w1_elems * sizeof(float));
    memcpy(prepack->b0_aligned, b0, hidden_dim * sizeof(float));
    memcpy(prepack->b1_aligned, b1, hidden_dim * sizeof(float));
    memcpy(prepack->bout_aligned, b_out, out_dim * sizeof(float));
    for (uint32_t h = 0; h < hidden_dim; h++) {
        for (uint32_t o = 0; o < out_dim; o++) {
            prepack->wout_t_aligned[(size_t)o * hidden_dim + h] =
                w_out[(size_t)h * out_dim + o];
        }
    }
    prepack->ready = 1;
}

static void sm_mlp_prepack_free(MLPPrepack *prepack) {
    free(prepack->w0_aligned);
    free(prepack->w1_aligned);
    free(prepack->wout_t_aligned);
    free(prepack->b0_aligned);
    free(prepack->b1_aligned);
    free(prepack->bout_aligned);
    memset(prepack, 0, sizeof(*prepack));
}

static void sm_mlp_matrix_body_single(
    const float features_in[27],
    const NeRFConfig *config,
    const float *mlp_weights,
    const float *mlp_biases,
    float out_acc[4]
) {
    uint32_t in_dim = config->mlp_in_dim;
    uint32_t hidden_dim = config->mlp_hidden_dim;
    uint32_t out_dim = config->mlp_out_dim;
    const float *w0 = mlp_weights;
    const float *b0 = mlp_biases;
    const float *w1 = w0 + (size_t)hidden_dim * in_dim;
    const float *b1 = b0 + hidden_dim;
    const float *w_out = w1 + (size_t)hidden_dim * hidden_dim;
    const float *b_out = b1 + hidden_dim;
    float hidden[128];
    float hidden2[128];

    if (hidden_dim > 128 || out_dim > 4) {
        memset(out_acc, 0, 4 * sizeof(float));
        return;
    }

#ifdef __AVX2__
    {
        uint32_t h = 0;
        for (; h + 7 < hidden_dim; h += 8) {
            __m256 v = _mm256_loadu_ps(&b0[h]);
            _mm256_storeu_ps(&hidden[h], v);
        }
        for (; h < hidden_dim; ++h) hidden[h] = b0[h];
    }
    for (uint32_t i = 0; i < in_dim; i++) {
        float fi = features_in[i];
        __m256 fvec = _mm256_set1_ps(fi);
        const float *w0_row = &w0[(size_t)i * hidden_dim];
        uint32_t hh = 0;
        for (; hh + 7 < hidden_dim; hh += 8) {
            __m256 wvec = _mm256_loadu_ps(&w0_row[hh]);
            __m256 acc = _mm256_loadu_ps(&hidden[hh]);
            acc = _mm256_add_ps(_mm256_mul_ps(wvec, fvec), acc);
            _mm256_storeu_ps(&hidden[hh], acc);
        }
        for (; hh < hidden_dim; ++hh) hidden[hh] += w0_row[hh] * fi;
    }
    for (uint32_t hh = 0; hh < hidden_dim; ++hh) {
        hidden[hh] = hidden[hh] > 0.0f ? hidden[hh] : 0.0f;
    }

    {
        uint32_t hh = 0;
        for (; hh + 7 < hidden_dim; hh += 8) {
            __m256 v = _mm256_loadu_ps(&b1[hh]);
            _mm256_storeu_ps(&hidden2[hh], v);
        }
        for (; hh < hidden_dim; ++hh) hidden2[hh] = b1[hh];
    }
    for (uint32_t hprev = 0; hprev < hidden_dim; hprev++) {
        float val = hidden[hprev];
        __m256 val_vec = _mm256_set1_ps(val);
        const float *w1_row = &w1[(size_t)hprev * hidden_dim];
        uint32_t h2 = 0;
        for (; h2 + 7 < hidden_dim; h2 += 8) {
            __m256 wvec = _mm256_loadu_ps(&w1_row[h2]);
            __m256 acc = _mm256_loadu_ps(&hidden2[h2]);
            acc = _mm256_add_ps(_mm256_mul_ps(wvec, val_vec), acc);
            _mm256_storeu_ps(&hidden2[h2], acc);
        }
        for (; h2 < hidden_dim; ++h2) hidden2[h2] += w1_row[h2] * val;
    }
    for (uint32_t h3 = 0; h3 < hidden_dim; ++h3) {
        hidden2[h3] = hidden2[h3] > 0.0f ? hidden2[h3] : 0.0f;
    }
#else
    for (uint32_t h = 0; h < hidden_dim; h++) hidden[h] = b0[h];
    for (uint32_t i = 0; i < in_dim; i++) {
        float fi = features_in[i];
        const float *w0_row = &w0[(size_t)i * hidden_dim];
        for (uint32_t h = 0; h < hidden_dim; h++) hidden[h] += w0_row[h] * fi;
    }
    for (uint32_t h = 0; h < hidden_dim; h++) hidden[h] = hidden[h] > 0.0f ? hidden[h] : 0.0f;

    for (uint32_t h = 0; h < hidden_dim; h++) hidden2[h] = b1[h];
    for (uint32_t hprev = 0; hprev < hidden_dim; hprev++) {
        float val = hidden[hprev];
        const float *w1_row = &w1[(size_t)hprev * hidden_dim];
        for (uint32_t h = 0; h < hidden_dim; h++) hidden2[h] += w1_row[h] * val;
    }
    for (uint32_t h = 0; h < hidden_dim; h++) hidden2[h] = hidden2[h] > 0.0f ? hidden2[h] : 0.0f;
#endif

    for (uint32_t o = 0; o < out_dim; o++) out_acc[o] = b_out[o];
    for (uint32_t h = 0; h < hidden_dim; h++) {
        const float *wout_row = &w_out[(size_t)h * out_dim];
        float hv = hidden2[h];
        for (uint32_t o = 0; o < out_dim; o++) out_acc[o] += wout_row[o] * hv;
    }
    for (uint32_t o = out_dim; o < 4; o++) out_acc[o] = 0.0f;
}

static void sm_mlp_inference_single_fixed_27_64_64_4(
    const float features_in[27],
    const float *mlp_weights,
    const float *mlp_biases,
    float rgb_out[3],
    float *sigma_out
) {
    const float *w0 = mlp_weights;
    const float *b0 = mlp_biases;
    const float *w1 = w0 + (size_t)27 * 64;
    const float *b1 = b0 + 64;
    const float *w_out = w1 + (size_t)64 * 64;
    const float *b_out = b1 + 64;
    float hidden[64];
    float hidden2[64];
    float out_acc[4];

#ifdef __AVX2__
    {
        __m256 zero = _mm256_setzero_ps();
        int h = 0;
        for (; h < 64; h += 8) {
            _mm256_storeu_ps(&hidden[h], _mm256_loadu_ps(&b0[h]));
        }
        for (int i = 0; i < 27; i++) {
            __m256 fvec = _mm256_set1_ps(features_in[i]);
            const float *w0_row = &w0[(size_t)i * 64];
            for (h = 0; h < 64; h += 8) {
                __m256 acc = _mm256_loadu_ps(&hidden[h]);
                __m256 wvec = _mm256_loadu_ps(&w0_row[h]);
                acc = _mm256_fmadd_ps(wvec, fvec, acc);
                _mm256_storeu_ps(&hidden[h], acc);
            }
        }
        for (h = 0; h < 64; h += 8) {
            __m256 v = _mm256_loadu_ps(&hidden[h]);
            _mm256_storeu_ps(&hidden[h], _mm256_max_ps(v, zero));
        }

        for (h = 0; h < 64; h += 8) {
            _mm256_storeu_ps(&hidden2[h], _mm256_loadu_ps(&b1[h]));
        }
        for (int hp = 0; hp < 64; hp++) {
            __m256 fvec = _mm256_set1_ps(hidden[hp]);
            const float *w1_row = &w1[(size_t)hp * 64];
            for (h = 0; h < 64; h += 8) {
                __m256 acc = _mm256_loadu_ps(&hidden2[h]);
                __m256 wvec = _mm256_loadu_ps(&w1_row[h]);
                acc = _mm256_fmadd_ps(wvec, fvec, acc);
                _mm256_storeu_ps(&hidden2[h], acc);
            }
        }
        for (h = 0; h < 64; h += 8) {
            __m256 v = _mm256_loadu_ps(&hidden2[h]);
            _mm256_storeu_ps(&hidden2[h], _mm256_max_ps(v, zero));
        }
    }
#else
    for (int h = 0; h < 64; h++) hidden[h] = b0[h];
    for (int i = 0; i < 27; i++) {
        const float fi = features_in[i];
        const float *w0_row = &w0[(size_t)i * 64];
        for (int h = 0; h < 64; h++) hidden[h] += w0_row[h] * fi;
    }
    for (int h = 0; h < 64; h++) hidden[h] = hidden[h] > 0.0f ? hidden[h] : 0.0f;
    for (int h = 0; h < 64; h++) hidden2[h] = b1[h];
    for (int hp = 0; hp < 64; hp++) {
        const float fi = hidden[hp];
        const float *w1_row = &w1[(size_t)hp * 64];
        for (int h = 0; h < 64; h++) hidden2[h] += w1_row[h] * fi;
    }
    for (int h = 0; h < 64; h++) hidden2[h] = hidden2[h] > 0.0f ? hidden2[h] : 0.0f;
#endif

    out_acc[0] = b_out[0];
    out_acc[1] = b_out[1];
    out_acc[2] = b_out[2];
    out_acc[3] = b_out[3];
    for (int h = 0; h < 64; h++) {
        const float hv = hidden2[h];
        const float *wout_row = &w_out[(size_t)h * 4];
        out_acc[0] += wout_row[0] * hv;
        out_acc[1] += wout_row[1] * hv;
        out_acc[2] += wout_row[2] * hv;
        out_acc[3] += wout_row[3] * hv;
    }

    rgb_out[0] = 1.0f / (1.0f + expf(-out_acc[0]));
    rgb_out[1] = 1.0f / (1.0f + expf(-out_acc[1]));
    rgb_out[2] = 1.0f / (1.0f + expf(-out_acc[2]));
    if (out_acc[3] > 20.0f) *sigma_out = out_acc[3];
    else if (out_acc[3] < -20.0f) *sigma_out = 0.0f;
    else *sigma_out = logf(1.0f + expf(out_acc[3]));
}

static float sm_hsum256_ps(__m256 v) {
    float lanes[8];
    _mm256_storeu_ps(lanes, v);
    return lanes[0] + lanes[1] + lanes[2] + lanes[3] +
           lanes[4] + lanes[5] + lanes[6] + lanes[7];
}

static void sm_mlp_inference_single_prepacked(
    const float features_in[27],
    const MLPPrepack *prepack,
    float rgb_out[3],
    float *sigma_out
) {
    float hidden[64];
    float hidden2[64];
    float out_acc[4];
    int h;

    if (!prepack->ready) {
        memset(rgb_out, 0, 3 * sizeof(float));
        *sigma_out = 0.0f;
        return;
    }

#ifdef __AVX2__
    {
        __m256 zero = _mm256_setzero_ps();
        for (h = 0; h < 64; h += 8) {
            _mm256_store_ps(&hidden[h], _mm256_load_ps(&prepack->b0_aligned[h]));
        }
        for (int i = 0; i < 27; i++) {
            __m256 fvec = _mm256_set1_ps(features_in[i]);
            const float *w0_row = &prepack->w0_aligned[(size_t)i * 64];
            for (h = 0; h < 64; h += 8) {
                __m256 acc = _mm256_load_ps(&hidden[h]);
                __m256 wvec = _mm256_load_ps(&w0_row[h]);
                acc = _mm256_fmadd_ps(wvec, fvec, acc);
                _mm256_store_ps(&hidden[h], acc);
            }
        }
        for (h = 0; h < 64; h += 8) {
            __m256 v = _mm256_load_ps(&hidden[h]);
            _mm256_store_ps(&hidden[h], _mm256_max_ps(v, zero));
        }

        for (h = 0; h < 64; h += 8) {
            _mm256_store_ps(&hidden2[h], _mm256_load_ps(&prepack->b1_aligned[h]));
        }
        for (int hp = 0; hp < 64; hp++) {
            __m256 fvec = _mm256_set1_ps(hidden[hp]);
            const float *w1_row = &prepack->w1_aligned[(size_t)hp * 64];
            for (h = 0; h < 64; h += 8) {
                __m256 acc = _mm256_load_ps(&hidden2[h]);
                __m256 wvec = _mm256_load_ps(&w1_row[h]);
                acc = _mm256_fmadd_ps(wvec, fvec, acc);
                _mm256_store_ps(&hidden2[h], acc);
            }
        }
        for (h = 0; h < 64; h += 8) {
            __m256 v = _mm256_load_ps(&hidden2[h]);
            _mm256_store_ps(&hidden2[h], _mm256_max_ps(v, zero));
        }

        for (int o = 0; o < 4; o++) {
            __m256 acc0 = _mm256_setzero_ps();
            const float *wcol = &prepack->wout_t_aligned[(size_t)o * 64];
            for (h = 0; h < 64; h += 8) {
                __m256 hv = _mm256_load_ps(&hidden2[h]);
                __m256 wv = _mm256_load_ps(&wcol[h]);
                acc0 = _mm256_fmadd_ps(hv, wv, acc0);
            }
            out_acc[o] = prepack->bout_aligned[o] + sm_hsum256_ps(acc0);
        }
    }
#else
    sm_mlp_inference_single_fixed_27_64_64_4(features_in, prepack->w0_aligned, prepack->b0_aligned, rgb_out, sigma_out);
    return;
#endif

    rgb_out[0] = 1.0f / (1.0f + expf(-out_acc[0]));
    rgb_out[1] = 1.0f / (1.0f + expf(-out_acc[1]));
    rgb_out[2] = 1.0f / (1.0f + expf(-out_acc[2]));
    if (out_acc[3] > 20.0f) *sigma_out = out_acc[3];
    else if (out_acc[3] < -20.0f) *sigma_out = 0.0f;
    else *sigma_out = logf(1.0f + expf(out_acc[3]));
}

static uint64_t sm_now_ns(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC_RAW, &ts);
    return (uint64_t)ts.tv_sec * 1000000000ull + (uint64_t)ts.tv_nsec;
}

static int parse_int_arg(const char *arg, int fallback) {
    char *end = NULL;
    long value = strtol(arg, &end, 10);
    if (!arg[0] || (end && *end != '\0')) {
        return fallback;
    }
    return (int)value;
}

static uint32_t sm_lcg_next(uint32_t *state) {
    *state = (*state * 1664525u) + 1013904223u;
    return *state;
}

static float sm_unit_from_u32(uint32_t bits) {
    return (float)(bits & 0x00ffffffu) / (float)0x01000000u;
}

static void fill_features(
    float features[SIMD_BATCH_SIZE][27],
    const char *pattern,
    unsigned int seed,
    int iter
) {
    uint32_t state = seed + (unsigned int)(iter * 2654435761u);
    int ray;
    int i;
    for (ray = 0; ray < SIMD_BATCH_SIZE; ray++) {
        for (i = 0; i < 27; i++) {
            if (strcmp(pattern, "zeros") == 0) {
                features[ray][i] = 0.0f;
            } else if (strcmp(pattern, "ramp") == 0) {
                features[ray][i] = ((float)i / 27.0f) - 0.5f + (float)ray * 0.03125f;
            } else if (strcmp(pattern, "alt") == 0) {
                features[ray][i] = ((i + ray + iter) & 1) ? 0.75f : -0.75f;
            } else {
                features[ray][i] = sm_unit_from_u32(sm_lcg_next(&state)) * 2.0f - 1.0f;
            }
        }
    }
}

static void usage(const char *argv0) {
    printf("Usage: %s [options]\n", argv0);
    printf("  --iterations N      Loop count (default 10000)\n");
    printf("  --mode single|batch|matrix|activation|fixed|prepacked Benchmark mode (default single)\n");
    printf("  --pattern zeros|ramp|alt|random\n");
    printf("  --seed N            RNG seed (default 1337)\n");
    printf("  --csv               Emit a single CSV-like line\n");
    printf("  --hashgrid PATH     Hash-grid binary path\n");
    printf("  --occupancy PATH    Occupancy grid binary path\n");
}

static int parse_args(int argc, char **argv, MLPBenchOptions *opt) {
    int i;
    for (i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--iterations") == 0 && i + 1 < argc) {
            opt->iterations = parse_int_arg(argv[++i], opt->iterations);
        } else if (strcmp(argv[i], "--mode") == 0 && i + 1 < argc) {
            opt->mode = argv[++i];
        } else if (strcmp(argv[i], "--pattern") == 0 && i + 1 < argc) {
            opt->pattern = argv[++i];
        } else if (strcmp(argv[i], "--seed") == 0 && i + 1 < argc) {
            opt->seed = (unsigned int)parse_int_arg(argv[++i], (int)opt->seed);
        } else if (strcmp(argv[i], "--csv") == 0) {
            opt->csv = 1;
        } else if (strcmp(argv[i], "--hashgrid") == 0 && i + 1 < argc) {
            opt->hashgrid_path = argv[++i];
        } else if (strcmp(argv[i], "--occupancy") == 0 && i + 1 < argc) {
            opt->occ_path = argv[++i];
        } else if (strcmp(argv[i], "--help") == 0 || strcmp(argv[i], "-h") == 0) {
            usage(argv[0]);
            return 0;
        } else {
            fprintf(stderr, "ERROR: unknown arg %s\n", argv[i]);
            usage(argv[0]);
            return -1;
        }
    }
    return 1;
}

int main(int argc, char **argv) {
    MLPBenchOptions opt;
    NeRFData *data;
    MLPPrepack prepack;
    float features[SIMD_BATCH_SIZE][27];
    float rgb_batch[SIMD_BATCH_SIZE][3];
    float sigma_batch[SIMD_BATCH_SIZE];
    uint64_t t0;
    uint64_t t1;
    double checksum = 0.0;
    int iter;
    int parse_status;

    memset(&opt, 0, sizeof(opt));
    opt.iterations = 10000;
    opt.mode = "single";
    opt.pattern = "ramp";
    opt.seed = 1337u;
    opt.hashgrid_path = "models/nerf_hashgrid.bin";
    opt.occ_path = "models/occupancy_grid.bin";

    parse_status = parse_args(argc, argv, &opt);
    if (parse_status <= 0) {
        return parse_status < 0 ? 1 : 0;
    }
    if (strcmp(opt.mode, "single") != 0 &&
        strcmp(opt.mode, "batch") != 0 &&
        strcmp(opt.mode, "matrix") != 0 &&
        strcmp(opt.mode, "activation") != 0 &&
        strcmp(opt.mode, "fixed") != 0 &&
        strcmp(opt.mode, "prepacked") != 0) {
        fprintf(stderr, "ERROR: mode must be single, batch, matrix, activation, fixed, or prepacked\n");
        return 1;
    }

    data = sm_nerf_data_load(opt.hashgrid_path, opt.occ_path);
    if (!data) {
        fprintf(stderr, "ERROR: failed to load NeRF data\n");
        return 1;
    }
    memset(&prepack, 0, sizeof(prepack));
    sm_mlp_prepack_init(&prepack, data);

    t0 = sm_now_ns();
    for (iter = 0; iter < opt.iterations; iter++) {
        fill_features(features, opt.pattern, opt.seed, iter);
        if (strcmp(opt.mode, "batch") == 0) {
            sm_mlp_inference_batch((const float(*)[27])features, &data->config,
                                   data->mlp_weights, data->mlp_biases,
                                   rgb_batch, sigma_batch);
            checksum += rgb_batch[0][0] + rgb_batch[SIMD_BATCH_SIZE - 1][2] + sigma_batch[0];
        } else if (strcmp(opt.mode, "matrix") == 0) {
            int ray;
            for (ray = 0; ray < SIMD_BATCH_SIZE; ray++) {
                float out_acc[4];
                sm_mlp_matrix_body_single(features[ray], &data->config,
                                          data->mlp_weights, data->mlp_biases,
                                          out_acc);
                checksum += out_acc[0] + out_acc[2] + out_acc[3];
            }
        } else if (strcmp(opt.mode, "activation") == 0) {
            int ray;
            for (ray = 0; ray < SIMD_BATCH_SIZE; ray++) {
                float out_acc[4];
                int o;
                for (o = 0; o < 4; o++) {
                    out_acc[o] = features[ray][o] * 6.0f + features[ray][o + 4] * 3.0f;
                }
                for (o = 0; o < 4; o++) {
                    float val = out_acc[o];
                    if (o < 3) {
                        checksum += 1.0f / (1.0f + expf(-val));
                    } else {
                        float sigma;
                        if (val > 20.0f) sigma = val;
                        else if (val < -20.0f) sigma = 0.0f;
                        else sigma = logf(1.0f + expf(val));
                        checksum += sigma;
                    }
                }
            }
        } else if (strcmp(opt.mode, "fixed") == 0) {
            int ray;
            if (data->config.mlp_in_dim != 27 || data->config.mlp_hidden_dim != 64 ||
                data->config.mlp_out_dim != 4 || data->config.mlp_num_layers != 2) {
                fprintf(stderr, "ERROR: fixed mode requires 27->64->64->4 with 2 hidden layers\n");
                sm_nerf_data_free(data);
                sm_mlp_prepack_free(&prepack);
                return 1;
            }
            for (ray = 0; ray < SIMD_BATCH_SIZE; ray++) {
                float rgb[3];
                float sigma = 0.0f;
                sm_mlp_inference_single_fixed_27_64_64_4(features[ray],
                                                         data->mlp_weights,
                                                         data->mlp_biases,
                                                         rgb, &sigma);
                checksum += rgb[0] + rgb[2] + sigma;
            }
        } else if (strcmp(opt.mode, "prepacked") == 0) {
            int ray;
            if (data->config.mlp_in_dim != 27 || data->config.mlp_hidden_dim != 64 ||
                data->config.mlp_out_dim != 4 || data->config.mlp_num_layers != 2 ||
                !prepack.ready) {
                fprintf(stderr, "ERROR: prepacked mode requires ready 27->64->64->4 prepack data\n");
                sm_nerf_data_free(data);
                sm_mlp_prepack_free(&prepack);
                return 1;
            }
            for (ray = 0; ray < SIMD_BATCH_SIZE; ray++) {
                float rgb[3];
                float sigma = 0.0f;
                sm_mlp_inference_single_prepacked(features[ray], &prepack, rgb, &sigma);
                checksum += rgb[0] + rgb[2] + sigma;
            }
        } else {
            int ray;
            for (ray = 0; ray < SIMD_BATCH_SIZE; ray++) {
                float rgb[3];
                float sigma = 0.0f;
                sm_mlp_inference_single(features[ray], &data->config,
                                        data->mlp_weights, data->mlp_biases,
                                        rgb, &sigma);
                checksum += rgb[0] + rgb[2] + sigma;
            }
        }
    }
    t1 = sm_now_ns();

    if (opt.csv) {
        printf("mode=%s,pattern=%s,iterations=%d,total_us=%.2f,us_per_iter=%.4f,checksum=%.6f\n",
               opt.mode,
               opt.pattern,
               opt.iterations,
               (double)(t1 - t0) / 1000.0,
               (double)(t1 - t0) / 1000.0 / (double)opt.iterations,
               checksum);
    } else {
        printf("NeRF MLP benchmark\n");
        printf("  mode=%s pattern=%s iterations=%d\n", opt.mode, opt.pattern, opt.iterations);
        printf("  total_us=%.2f us_per_iter=%.4f checksum=%.6f\n",
               (double)(t1 - t0) / 1000.0,
               (double)(t1 - t0) / 1000.0 / (double)opt.iterations,
               checksum);
    }

    sm_mlp_prepack_free(&prepack);
    sm_nerf_data_free(data);
    return 0;
}
