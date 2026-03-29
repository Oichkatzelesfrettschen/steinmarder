// bilateral_denoise.c - Edge-aware bilateral filtering for raytraced images
// Separable bilateral filter: spatial kernel (distance) + range kernel (color similarity)

#include "bilateral_denoise.h"
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <string.h>

// ============================================================================
// Bilateral Filter: Edge-aware denoising via spatial + range kernels
// ============================================================================

// Gaussian spatial kernel: exp(-d^2 / (2 * sigma_s^2))
static inline float gauss_spatial(float dist_sq, float sigma_s_sq) {
    return expf(-dist_sq / (2.0f * sigma_s_sq));
}

// Gaussian range kernel: exp(-diff^2 / (2 * sigma_r^2))
// Penalizes large color differences, preserves edges
static inline float gauss_range(float color_diff_sq, float sigma_r_sq) {
    return expf(-color_diff_sq / (2.0f * sigma_r_sq));
}

// Luminance (for range kernel, more perceptually accurate)
static inline float luminance(const Vec3 c) {
    return 0.2126f * c.x + 0.7152f * c.y + 0.0722f * c.z;
}

// ============================================================================
// 1D Separable Bilateral Pass (horizontal or vertical)
// ============================================================================

typedef struct {
    float sigma_s;      // spatial std dev (pixels)
    float sigma_r;      // range std dev (luminance units)
    int radius;         // filter radius (pixels)
} BilateralParams;

static void bilateral_filter_1d(
    const Vec3 *input,
    Vec3 *output,
    int width, int height,
    int horizontal,          // 1 = horizontal pass, 0 = vertical pass
    const BilateralParams *p)
{
    float sigma_s_sq = p->sigma_s * p->sigma_s;
    float sigma_r_sq = p->sigma_r * p->sigma_r;
    float inv_2sigma_r_sq = 1.0f / (2.0f * sigma_r_sq);

    // Precompute spatial kernel -- depends only on offset, not pixel data.
    // Eliminates one expf() per neighbor per pixel (saves ~50% transcendentals).
    float spatial_lut[41]; // max radius 20 -> 2*20+1 = 41 entries
    int r = p->radius;
    if (r > 20) r = 20;
    for (int d = -r; d <= r; d++) {
        spatial_lut[d + r] = gauss_spatial((float)(d * d), sigma_s_sq);
    }

    if (horizontal) {
        // Horizontal pass: filter rows
        for (int y = 0; y < height; y++) {
            const Vec3 *row_in = input + y * width;
            Vec3 *row_out = output + y * width;

            for (int x = 0; x < width; x++) {
                Vec3 center_col = row_in[x];
                float center_lum = luminance(center_col);

                float sum_x = 0.0f, sum_y = 0.0f, sum_z = 0.0f;
                float weight_sum = 0.0f;

                int dx_min = (x - r >= 0)     ? -r : -x;
                int dx_max = (x + r < width)  ?  r : (width - 1 - x);

                for (int dx = dx_min; dx <= dx_max; dx++) {
                    Vec3 neighbor_col = row_in[x + dx];
                    float neighbor_lum = luminance(neighbor_col);

                    float lum_diff = center_lum - neighbor_lum;
                    float lum_diff_sq = lum_diff * lum_diff;

                    // Range kernel: fast polynomial approx instead of expf
                    float rx = lum_diff_sq * inv_2sigma_r_sq;
                    float w_range;
                    if (rx > 4.0f) {
                        w_range = 0.0f;
                    } else {
                        // exp(-x) ~ max(0, 1 - x + 0.5*x^2) for x in [0,4]
                        w_range = 1.0f - rx + 0.5f * rx * rx;
                    }

                    float weight = spatial_lut[dx + r] * w_range;

                    sum_x += neighbor_col.x * weight;
                    sum_y += neighbor_col.y * weight;
                    sum_z += neighbor_col.z * weight;
                    weight_sum += weight;
                }

                if (weight_sum > 1e-6f) {
                    float inv_w = 1.0f / weight_sum;
                    row_out[x].x = sum_x * inv_w;
                    row_out[x].y = sum_y * inv_w;
                    row_out[x].z = sum_z * inv_w;
                } else {
                    row_out[x] = center_col;
                }
            }
        }
    } else {
        // Vertical pass: filter columns in 64-column strips for L2 cache locality.
        // Strip working set: (2*radius+1) rows * 64 cols * 12 bytes ~ 8.4 KB at radius 5.
        #define VERT_STRIP_W 64
        for (int x_start = 0; x_start < width; x_start += VERT_STRIP_W) {
            int x_end = x_start + VERT_STRIP_W;
            if (x_end > width) x_end = width;

            for (int y = 0; y < height; y++) {
                int dy_min = (y - r >= 0)      ? -r : -y;
                int dy_max = (y + r < height)  ?  r : (height - 1 - y);

                for (int x = x_start; x < x_end; x++) {
                    int center_idx = y * width + x;
                    Vec3 center_col = input[center_idx];
                    float center_lum = luminance(center_col);

                    float sum_x = 0.0f, sum_y = 0.0f, sum_z = 0.0f;
                    float weight_sum = 0.0f;

                    for (int dy = dy_min; dy <= dy_max; dy++) {
                        int neighbor_idx = (y + dy) * width + x;
                        Vec3 neighbor_col = input[neighbor_idx];
                        float neighbor_lum = luminance(neighbor_col);

                        float lum_diff = center_lum - neighbor_lum;
                        float lum_diff_sq = lum_diff * lum_diff;

                        float rx = lum_diff_sq * inv_2sigma_r_sq;
                        float w_range;
                        if (rx > 4.0f) {
                            w_range = 0.0f;
                        } else {
                            w_range = 1.0f - rx + 0.5f * rx * rx;
                        }

                        float weight = spatial_lut[dy + r] * w_range;

                        sum_x += neighbor_col.x * weight;
                        sum_y += neighbor_col.y * weight;
                        sum_z += neighbor_col.z * weight;
                        weight_sum += weight;
                    }

                    if (weight_sum > 1e-6f) {
                        float inv_w = 1.0f / weight_sum;
                        output[center_idx].x = sum_x * inv_w;
                        output[center_idx].y = sum_y * inv_w;
                        output[center_idx].z = sum_z * inv_w;
                    } else {
                        output[center_idx] = center_col;
                    }
                }
            }
        }
        #undef VERT_STRIP_W
    }
}

// ============================================================================
// Main API: Separable Bilateral Filter
// ============================================================================

void bilateral_denoise(Vec3 *pixels, int width, int height,
                       float sigma_s, float sigma_r, int radius)
{
    if (!pixels || width <= 0 || height <= 0 || radius < 1) return;

    // Allocate temporary buffer for intermediate pass
    Vec3 *temp = (Vec3*)malloc((size_t)width * (size_t)height * sizeof(Vec3));
    if (!temp) {
        fprintf(stderr, "[DENOISE] malloc failed for temp buffer\n");
        return;
    }

    BilateralParams p;
    p.sigma_s = sigma_s;
    p.sigma_r = sigma_r;
    p.radius = radius;

    // Horizontal pass: input -> temp
    bilateral_filter_1d(pixels, temp, width, height, 1, &p);

    // Vertical pass: temp -> output (pixels)
    bilateral_filter_1d(temp, pixels, width, height, 0, &p);

    free(temp);

    fprintf(stderr, "[DENOISE] bilateral complete: sigma_s=%.2f sigma_r=%.4f radius=%d\n",
            sigma_s, sigma_r, radius);
}

// ============================================================================
// Environment-based configuration
// ============================================================================

static int sm_env_int(const char *name, int defv) {
    const char *s = getenv(name);
    if (!s || !s[0]) return defv;
    return atoi(s);
}

static float sm_env_float(const char *name, float defv) {
    const char *s = getenv(name);
    if (!s || !s[0]) return defv;
    char buf[128];
    size_t n = strlen(s);
    if (n >= sizeof(buf)) n = sizeof(buf) - 1;
    memcpy(buf, s, n);
    buf[n] = 0;
    for (size_t i = 0; i < n; i++) if (buf[i] == ',') buf[i] = '.';
    return (float)atof(buf);
}

void bilateral_denoise_maybe(Vec3 *pixels, int width, int height)
{
    if (!pixels || width <= 0 || height <= 0) return;

    int enabled = sm_env_int("SM_BILATERAL_DENOISE", 0) ? 1 : 0;
    if (!enabled) return;

    // Configuration via environment variables
    float sigma_s = sm_env_float("SM_BILATERAL_SIGMA_S", 1.5f);   // spatial (pixels)
    float sigma_r = sm_env_float("SM_BILATERAL_SIGMA_R", 0.1f);   // range (luminance)
    int radius = sm_env_int("SM_BILATERAL_RADIUS", 3);            // filter radius

    if (sigma_s < 0.1f) sigma_s = 0.1f;
    if (sigma_r < 0.01f) sigma_r = 0.01f;
    if (radius < 1) radius = 1;
    if (radius > 20) radius = 20;

    fprintf(stderr, "[DENOISE] bilateral enabled: sigma_s=%.2f sigma_r=%.4f radius=%d\n",
            sigma_s, sigma_r, radius);

    bilateral_denoise(pixels, width, height, sigma_s, sigma_r, radius);
}
