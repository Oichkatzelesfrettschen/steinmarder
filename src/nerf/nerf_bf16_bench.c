/*
 * nerf_bf16_bench.c -- BF16 vs FP32 prepacked MLP inference benchmark
 *
 * Loads a real NeRF model, runs both the prepacked FP32 (AVX2 fmadd) and
 * BF16 (AVX-512 vdpbf16ps) single-ray MLP inference paths in a tight loop,
 * and reports ns/inference plus the speedup ratio.
 *
 * Build:
 *   clang-22 -O2 -march=native -mavx512f -mavx512bf16 -mavx512bw -mavx512vl -mfma \
 *       -I src/core -I src/nerf \
 *       src/nerf/nerf_bf16_bench.c nerf_simd.o vec3.o camera.o ray.o \
 *       -lm -lpthread -o build/nerf_bf16_bench
 *
 * Usage:
 *   ./build/nerf_bf16_bench [--iterations N] [--hashgrid PATH] [--occupancy PATH]
 */
#include "nerf_simd.h"
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

/* ---- timing ---- */
static uint64_t now_ns(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC_RAW, &ts);
    return (uint64_t)ts.tv_sec * 1000000000ULL + (uint64_t)ts.tv_nsec;
}

/* ---- lightweight LCG RNG ---- */
static uint32_t lcg_state = 0xDEADBEEFu;
static uint32_t lcg_next(void) {
    lcg_state = lcg_state * 1664525u + 1013904223u;
    return lcg_state;
}
static float rand_uniform(void) {
    return (float)(lcg_next() & 0x00FFFFFFu) / (float)0x01000000u * 2.0f - 1.0f;
}

/* ---- helpers ---- */
static void fill_random_features(float features_out[27]) {
    for (int feature_idx = 0; feature_idx < 27; feature_idx++) {
        features_out[feature_idx] = rand_uniform();
    }
}

static float max_abs_error(const float *reference_values, const float *test_values, int element_count) {
    float maximum_error = 0.0f;
    for (int element_idx = 0; element_idx < element_count; element_idx++) {
        float absolute_difference = fabsf(reference_values[element_idx] - test_values[element_idx]);
        if (absolute_difference > maximum_error) {
            maximum_error = absolute_difference;
        }
    }
    return maximum_error;
}

int main(int argc, char **argv) {
    const char *hashgrid_path = "models/nerf_hashgrid.bin";
    const char *occupancy_path = "models/occupancy_grid.bin";
    int iteration_count = 1000000;

    /* Parse command-line arguments */
    for (int arg_idx = 1; arg_idx < argc; arg_idx++) {
        if (strcmp(argv[arg_idx], "--iterations") == 0 && arg_idx + 1 < argc) {
            iteration_count = atoi(argv[++arg_idx]);
        } else if (strcmp(argv[arg_idx], "--hashgrid") == 0 && arg_idx + 1 < argc) {
            hashgrid_path = argv[++arg_idx];
        } else if (strcmp(argv[arg_idx], "--occupancy") == 0 && arg_idx + 1 < argc) {
            occupancy_path = argv[++arg_idx];
        } else {
            fprintf(stderr, "Usage: %s [--iterations N] [--hashgrid PATH] [--occupancy PATH]\n",
                    argv[0]);
            return 1;
        }
    }

    /* Load model (this calls sm_nerf_prepare_mlp_prepack + sm_nerf_prepare_mlp_bf16) */
    printf("Loading model: %s + %s\n", hashgrid_path, occupancy_path);
    NeRFData *nerf_data = sm_nerf_data_load(hashgrid_path, occupancy_path);
    if (!nerf_data) {
        fprintf(stderr, "ERROR: failed to load NeRF model data\n");
        return 1;
    }

    if (!nerf_data->mlp_prepacked_ready) {
        fprintf(stderr, "ERROR: FP32 prepack not ready (need 27->64->64->4 architecture)\n");
        sm_nerf_data_free(nerf_data);
        return 1;
    }

#ifndef __AVX512BF16__
    fprintf(stderr, "ERROR: compiled without __AVX512BF16__ -- BF16 path unavailable\n");
    sm_nerf_data_free(nerf_data);
    return 1;
#else
    if (!nerf_data->mlp_bf16_ready) {
        fprintf(stderr, "ERROR: BF16 prepack not ready\n");
        sm_nerf_data_free(nerf_data);
        return 1;
    }
#endif

    printf("Model loaded. Architecture: %u -> %u -> %u -> %u (%u hidden layers)\n",
           nerf_data->config.mlp_in_dim,
           nerf_data->config.mlp_hidden_dim,
           nerf_data->config.mlp_hidden_dim,
           nerf_data->config.mlp_out_dim,
           nerf_data->config.mlp_num_layers);
    printf("FP32 prepack: %s | BF16 prepack: %s\n",
           nerf_data->mlp_prepacked_ready ? "ready" : "NOT ready",
#ifdef __AVX512BF16__
           nerf_data->mlp_bf16_ready ? "ready" : "NOT ready"
#else
           "unavailable (no __AVX512BF16__)"
#endif
    );
    printf("Iterations: %d\n\n", iteration_count);

    /* Pre-generate feature vectors so RNG is not in the timed loop */
    /* Use a rotating buffer of 1024 feature sets to avoid L1 cache lock-in */
    const int feature_buffer_size = 1024;
    float (*feature_buffer)[27] = malloc(sizeof(float[27]) * (size_t)feature_buffer_size);
    if (!feature_buffer) {
        fprintf(stderr, "ERROR: failed to allocate feature buffer\n");
        sm_nerf_data_free(nerf_data);
        return 1;
    }
    lcg_state = 0x12345678u;
    for (int buffer_idx = 0; buffer_idx < feature_buffer_size; buffer_idx++) {
        fill_random_features(feature_buffer[buffer_idx]);
    }

    /* === Correctness check: compare first output === */
    {
        float fp32_rgb[3], fp32_sigma;
        float bf16_rgb[3], bf16_sigma;

        sm_mlp_inference_single_prepacked_data(feature_buffer[0], nerf_data,
                                                fp32_rgb, &fp32_sigma);
#ifdef __AVX512BF16__
        sm_mlp_inference_single_bf16(feature_buffer[0], nerf_data,
                                      bf16_rgb, &bf16_sigma);
#endif

        printf("=== Correctness (first inference) ===\n");
        printf("  FP32 prepacked:  rgb=(%.6f, %.6f, %.6f) sigma=%.6f\n",
               fp32_rgb[0], fp32_rgb[1], fp32_rgb[2], fp32_sigma);
#ifdef __AVX512BF16__
        printf("  BF16 vdpbf16ps:  rgb=(%.6f, %.6f, %.6f) sigma=%.6f\n",
               bf16_rgb[0], bf16_rgb[1], bf16_rgb[2], bf16_sigma);

        float fp32_output[4] = { fp32_rgb[0], fp32_rgb[1], fp32_rgb[2], fp32_sigma };
        float bf16_output[4] = { bf16_rgb[0], bf16_rgb[1], bf16_rgb[2], bf16_sigma };
        float maximum_error = max_abs_error(fp32_output, bf16_output, 4);
        printf("  max |FP32 - BF16| = %.6e %s\n\n",
               maximum_error,
               maximum_error < 0.01f ? "(OK -- within BF16 tolerance)" :
               maximum_error < 0.05f ? "(MARGINAL)" : "(HIGH -- check weights)");
#endif
    }

    /* === Benchmark: FP32 prepacked === */
    volatile float checksum_fp32 = 0.0f;
    {
        /* Warmup */
        for (int warmup_idx = 0; warmup_idx < 10000; warmup_idx++) {
            float rgb[3], sigma;
            sm_mlp_inference_single_prepacked_data(
                feature_buffer[warmup_idx & (feature_buffer_size - 1)],
                nerf_data, rgb, &sigma);
            checksum_fp32 += rgb[0];
        }
        checksum_fp32 = 0.0f;

        uint64_t start_time_ns = now_ns();
        for (int iteration_idx = 0; iteration_idx < iteration_count; iteration_idx++) {
            float rgb[3], sigma;
            sm_mlp_inference_single_prepacked_data(
                feature_buffer[iteration_idx & (feature_buffer_size - 1)],
                nerf_data, rgb, &sigma);
            checksum_fp32 += rgb[0];
        }
        uint64_t end_time_ns = now_ns();
        uint64_t elapsed_ns = end_time_ns - start_time_ns;
        double ns_per_inference_fp32 = (double)elapsed_ns / (double)iteration_count;
        printf("=== FP32 prepacked (AVX2 fmadd) ===\n");
        printf("  %d iterations in %.3f ms\n", iteration_count,
               (double)elapsed_ns / 1e6);
        printf("  %.1f ns/inference\n", ns_per_inference_fp32);
        printf("  %.2f M inferences/sec\n", 1e3 / ns_per_inference_fp32);
        printf("  checksum=%.6f\n\n", (double)checksum_fp32);
    }

    /* === Benchmark: BF16 vdpbf16ps === */
#ifdef __AVX512BF16__
    volatile float checksum_bf16 = 0.0f;
    {
        /* Warmup */
        for (int warmup_idx = 0; warmup_idx < 10000; warmup_idx++) {
            float rgb[3], sigma;
            sm_mlp_inference_single_bf16(
                feature_buffer[warmup_idx & (feature_buffer_size - 1)],
                nerf_data, rgb, &sigma);
            checksum_bf16 += rgb[0];
        }
        checksum_bf16 = 0.0f;

        uint64_t start_time_ns = now_ns();
        for (int iteration_idx = 0; iteration_idx < iteration_count; iteration_idx++) {
            float rgb[3], sigma;
            sm_mlp_inference_single_bf16(
                feature_buffer[iteration_idx & (feature_buffer_size - 1)],
                nerf_data, rgb, &sigma);
            checksum_bf16 += rgb[0];
        }
        uint64_t end_time_ns = now_ns();
        uint64_t elapsed_ns = end_time_ns - start_time_ns;
        double ns_per_inference_bf16 = (double)elapsed_ns / (double)iteration_count;
        printf("=== BF16 vdpbf16ps (AVX-512) ===\n");
        printf("  %d iterations in %.3f ms\n", iteration_count,
               (double)elapsed_ns / 1e6);
        printf("  %.1f ns/inference\n", ns_per_inference_bf16);
        printf("  %.2f M inferences/sec\n", 1e3 / ns_per_inference_bf16);
        printf("  checksum=%.6f\n\n", (double)checksum_bf16);

        /* === Summary === */
        /* Recalculate FP32 timing for the ratio */
        checksum_fp32 = 0.0f;
        uint64_t fp32_start = now_ns();
        for (int iteration_idx = 0; iteration_idx < iteration_count; iteration_idx++) {
            float rgb[3], sigma;
            sm_mlp_inference_single_prepacked_data(
                feature_buffer[iteration_idx & (feature_buffer_size - 1)],
                nerf_data, rgb, &sigma);
            checksum_fp32 += rgb[0];
        }
        uint64_t fp32_end = now_ns();
        double ns_fp32_final = (double)(fp32_end - fp32_start) / (double)iteration_count;
        double speedup_ratio = ns_fp32_final / ns_per_inference_bf16;
        printf("=== SUMMARY ===\n");
        printf("  FP32 prepacked: %.1f ns/inference\n", ns_fp32_final);
        printf("  BF16 vdpbf16ps: %.1f ns/inference\n", ns_per_inference_bf16);
        printf("  Speedup: %.2fx\n", speedup_ratio);
    }
#endif

    free(feature_buffer);
    sm_nerf_data_free(nerf_data);
    return 0;
}
