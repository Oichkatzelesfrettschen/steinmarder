// Lloyd-Max optimal codebook generator for D3Q19 LBM distributions.
//
// Runs a short FP32 LBM simulation, collects a histogram of all f_i values,
// then iteratively computes the MSE-optimal 256-level quantization codebook.
//
// The codebook consists of:
//   - centroids[256]: reconstruction values (decode: index -> float)
//   - boundaries[257]: decision boundaries (encode: float -> nearest index)
//
// Usage: call lloydmax_generate() which returns a LloydMaxCodebook struct.
// The codebook can then be used in the INT8 SoA Lloyd-Max kernel variant.

#include "host_wrappers.h"
#include "lbm_metrics.h"
#include "lloydmax_codebook.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <float.h>

// ---------------------------------------------------------------------------
// Histogram collection from FP32 LBM simulation
// ---------------------------------------------------------------------------

#define HIST_BINS 4096
#define HIST_MIN -0.5f
#define HIST_MAX  1.5f

static void collect_f_histogram(
    int nx, int ny, int nz, int steps,
    double* hist, float* out_min, float* out_max
) {
    size_t n_cells = (size_t)nx * ny * nz;

    // Allocate FP32 LBM buffers
    LBMGrid grid = {nx, ny, nz, (int)n_cells};
    void *f_a = NULL, *f_b = NULL;
    float *rho = NULL, *u = NULL, *tau_d = NULL, *force_d = NULL;

    size_t dist_bytes = 19 * n_cells * sizeof(float);
    cudaMalloc(&f_a, dist_bytes);
    cudaMalloc(&f_b, dist_bytes);
    cudaMalloc(&rho, n_cells * sizeof(float));
    cudaMalloc(&u, n_cells * 3 * sizeof(float));
    cudaMalloc(&tau_d, n_cells * sizeof(float));
    cudaMalloc(&force_d, n_cells * 3 * sizeof(float));

    // Initialize tau=0.6, force=0, rho=1, u=0
    {
        float* h_tau = (float*)malloc(n_cells * sizeof(float));
        for (size_t i = 0; i < n_cells; i++) h_tau[i] = 0.6f;
        cudaMemcpy(tau_d, h_tau, n_cells * sizeof(float), cudaMemcpyHostToDevice);
        free(h_tau);
    }
    cudaMemset(force_d, 0, n_cells * 3 * sizeof(float));

    LBMBuffers bufs = {f_a, f_b, NULL, NULL, rho, u, tau_d, NULL, force_d};
    // Initialize with nonzero velocity to spread the f_i distribution.
    // Ma = 0.1 is typical for LBM; this creates a realistic spread around equilibrium.
    launch_lbm_init(LBM_FP32_SOA_FUSED, &grid, &bufs, 1.0f, 0.1f, 0.05f, 0.0f, 0);
    cudaDeviceSynchronize();

    // Add a force perturbation to create additional distribution spread
    {
        float* h_force = (float*)calloc(n_cells * 3, sizeof(float));
        for (size_t i = 0; i < n_cells; i++) {
            // Sinusoidal body force for realistic distribution sampling
            int ix = (int)(i % nx);
            int iy = (int)((i / nx) % ny);
            float phase = 6.2832f * (float)ix / (float)nx;
            h_force[i] = 1e-4f * sinf(phase);                          // fx
            h_force[n_cells + i] = 1e-4f * cosf(phase) * ((float)iy / (float)ny); // fy
        }
        cudaMemcpy(force_d, h_force, n_cells * 3 * sizeof(float), cudaMemcpyHostToDevice);
        free(h_force);
    }

    // Run steps
    for (int s = 0; s < steps; s++) {
        void* in  = (s % 2 == 0) ? f_a : f_b;
        void* out = (s % 2 == 0) ? f_b : f_a;
        launch_lbm_step(LBM_FP32_SOA_FUSED, &grid, &bufs, in, out, 0, 0);
    }
    cudaDeviceSynchronize();

    // Download distributions
    void* last_f = (steps % 2 == 0) ? f_a : f_b;
    float* h_f = (float*)malloc(19 * n_cells * sizeof(float));
    cudaMemcpy(h_f, last_f, 19 * n_cells * sizeof(float), cudaMemcpyDeviceToHost);

    // Build histogram
    for (int i = 0; i < HIST_BINS; i++) hist[i] = 0.0;
    float fmin = FLT_MAX, fmax = -FLT_MAX;
    double bin_width = (HIST_MAX - HIST_MIN) / HIST_BINS;

    size_t total_samples = 19 * n_cells;
    for (size_t i = 0; i < total_samples; i++) {
        float v = h_f[i];
        if (v != v) continue; // skip NaN
        if (v < fmin) fmin = v;
        if (v > fmax) fmax = v;
        int bin = (int)((v - HIST_MIN) / bin_width);
        if (bin < 0) bin = 0;
        if (bin >= HIST_BINS) bin = HIST_BINS - 1;
        hist[bin] += 1.0;
    }

    // Normalize to PDF
    double sum = 0.0;
    for (int i = 0; i < HIST_BINS; i++) sum += hist[i];
    if (sum > 0.0) {
        for (int i = 0; i < HIST_BINS; i++) hist[i] /= (sum * bin_width);
    }

    *out_min = fmin;
    *out_max = fmax;

    free(h_f);
    cudaFree(f_a);
    cudaFree(f_b);
    cudaFree(rho);
    cudaFree(u);
    cudaFree(tau_d);
    cudaFree(force_d);
}

// ---------------------------------------------------------------------------
// Lloyd-Max iterative solver
// ---------------------------------------------------------------------------

static void lloydmax_solve(
    const double* pdf, int n_bins, float bin_min, float bin_max,
    float data_min, float data_max,
    int n_levels, float* centroids, float* boundaries, int max_iter
) {
    double bin_width = (double)(bin_max - bin_min) / n_bins;

    // Initialize centroids within the actual data range with slight padding
    float pad = (data_max - data_min) * 0.05f;
    float init_min = data_min - pad;
    float init_max = data_max + pad;
    float init_range = init_max - init_min;
    for (int i = 0; i < n_levels; i++) {
        centroids[i] = init_min + init_range * ((float)i + 0.5f) / (float)n_levels;
    }

    for (int iter = 0; iter < max_iter; iter++) {
        // Step 1: Update boundaries as midpoints between adjacent centroids
        boundaries[0] = -FLT_MAX;
        for (int i = 1; i < n_levels; i++) {
            boundaries[i] = 0.5f * (centroids[i - 1] + centroids[i]);
        }
        boundaries[n_levels] = FLT_MAX;

        // Step 2: Update centroids as conditional expectations
        double prev_mse = 0.0;
        for (int i = 0; i < n_levels; i++) {
            double weighted_sum = 0.0;
            double weight = 0.0;

            for (int b = 0; b < n_bins; b++) {
                float x = bin_min + ((float)b + 0.5f) * (float)bin_width;
                if (x >= boundaries[i] && x < boundaries[i + 1]) {
                    weighted_sum += (double)x * pdf[b] * bin_width;
                    weight += pdf[b] * bin_width;
                }
            }

            if (weight > 1e-15) {
                centroids[i] = (float)(weighted_sum / weight);
            }
            // else keep previous centroid

            // Accumulate MSE
            for (int b = 0; b < n_bins; b++) {
                float x = bin_min + ((float)b + 0.5f) * (float)bin_width;
                if (x >= boundaries[i] && x < boundaries[i + 1]) {
                    double err = (double)x - (double)centroids[i];
                    prev_mse += err * err * pdf[b] * bin_width;
                }
            }
        }

        // Check convergence (MSE change < 1e-10)
        if (iter > 0 && prev_mse < 1e-10) break;
    }

    // Final boundary update
    boundaries[0] = -FLT_MAX;
    for (int i = 1; i < n_levels; i++) {
        boundaries[i] = 0.5f * (centroids[i - 1] + centroids[i]);
    }
    boundaries[n_levels] = FLT_MAX;
}

// ---------------------------------------------------------------------------
// MSE computation
// ---------------------------------------------------------------------------

static double compute_mse(
    const double* pdf, int n_bins, float bin_min, float bin_max,
    const float* centroids, const float* boundaries, int n_levels
) {
    double bin_width = (double)(bin_max - bin_min) / n_bins;
    double mse = 0.0;

    for (int b = 0; b < n_bins; b++) {
        float x = bin_min + ((float)b + 0.5f) * (float)bin_width;
        // Find nearest centroid via boundaries
        int idx = 0;
        for (int i = 1; i < n_levels; i++) {
            if (x >= boundaries[i]) idx = i;
            else break;
        }
        double err = (double)x - (double)centroids[idx];
        mse += err * err * pdf[b] * bin_width;
    }
    return mse;
}

static double compute_uniform_mse(
    const double* pdf, int n_bins, float bin_min, float bin_max,
    float scale, int n_levels
) {
    double bin_width = (double)(bin_max - bin_min) / n_bins;
    double mse = 0.0;
    float inv_scale = 1.0f / scale;

    for (int b = 0; b < n_bins; b++) {
        float x = bin_min + ((float)b + 0.5f) * (float)bin_width;
        // Uniform quantization: round(x * scale) / scale
        float q = (float)(int)(x * scale + (x >= 0.0f ? 0.5f : -0.5f));
        if (q > 127.0f) q = 127.0f;
        if (q < -128.0f) q = -128.0f;
        float recon = q * inv_scale;
        double err = (double)x - (double)recon;
        mse += err * err * pdf[b] * bin_width;
    }
    return mse;
}

// ---------------------------------------------------------------------------
// Public API
// ---------------------------------------------------------------------------

LloydMaxCodebook lloydmax_generate(int grid_size, int sim_steps) {
    LloydMaxCodebook cb;
    memset(&cb, 0, sizeof(cb));
    cb.n_levels = 256;

    printf("Lloyd-Max codebook generation:\n");
    printf("  Grid: %d^3, simulation: %d steps\n", grid_size, sim_steps);

    // Collect histogram
    double* hist = (double*)calloc(HIST_BINS, sizeof(double));
    collect_f_histogram(grid_size, grid_size, grid_size, sim_steps,
                        hist, &cb.f_min, &cb.f_max);

    printf("  f_i range: [%.6f, %.6f]\n", cb.f_min, cb.f_max);

    // Run Lloyd-Max solver
    lloydmax_solve(hist, HIST_BINS, HIST_MIN, HIST_MAX,
                   cb.f_min, cb.f_max,
                   256, cb.centroids, cb.boundaries, 100);

    // Compute MSE comparison
    cb.mse_lloydmax = (float)compute_mse(hist, HIST_BINS, HIST_MIN, HIST_MAX,
                                          cb.centroids, cb.boundaries, 256);
    cb.mse_uniform = (float)compute_uniform_mse(hist, HIST_BINS, HIST_MIN, HIST_MAX,
                                                 64.0f, 256);

    printf("  MSE uniform (DIST_SCALE=64): %.8f\n", cb.mse_uniform);
    printf("  MSE Lloyd-Max (256 levels):  %.8f\n", cb.mse_lloydmax);
    if (cb.mse_uniform > 1e-12f) {
        printf("  Improvement: %.1fx reduction\n",
               (double)cb.mse_uniform / (double)cb.mse_lloydmax);
    }

    // Print centroid distribution summary
    printf("  Centroid range: [%.6f, %.6f]\n", cb.centroids[0], cb.centroids[255]);
    printf("  Centroid density near f_0=0.333: ");
    int count_near_f0 = 0;
    for (int i = 0; i < 256; i++) {
        if (cb.centroids[i] >= 0.30f && cb.centroids[i] <= 0.37f) count_near_f0++;
    }
    printf("%d levels in [0.30, 0.37]\n", count_near_f0);

    free(hist);
    return cb;
}

// Print codebook as C constant array (for embedding in kernel source)
void lloydmax_print_codebook_c(const LloydMaxCodebook* cb) {
    printf("\n// Auto-generated Lloyd-Max codebook for D3Q19 LBM\n");
    printf("// MSE improvement: %.1fx vs uniform DIST_SCALE=64\n",
           cb->mse_uniform > 1e-12f ? (double)cb->mse_uniform / (double)cb->mse_lloydmax : 0.0);
    printf("__constant__ float LM_CENTROIDS[256] = {\n");
    for (int i = 0; i < 256; i++) {
        printf("    %.8ff%s", cb->centroids[i], (i < 255) ? ",\n" : "\n");
    }
    printf("};\n\n");

    printf("__constant__ float LM_BOUNDARIES[257] = {\n");
    for (int i = 0; i <= 256; i++) {
        if (cb->boundaries[i] == -FLT_MAX) printf("    -FLT_MAX%s\n", (i < 256) ? "," : "");
        else if (cb->boundaries[i] == FLT_MAX) printf("    FLT_MAX\n");
        else printf("    %.8ff%s\n", cb->boundaries[i], (i < 256) ? "," : "");
    }
    printf("};\n");
}
