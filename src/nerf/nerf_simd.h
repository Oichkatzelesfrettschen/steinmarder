#ifndef NERF_SIMD_H
#define NERF_SIMD_H

#include <stdint.h>
#include <stdbool.h>
#include "vec3.h"
#include "camera.h"

/* ===== CPU Feature Detection ===== */
typedef struct {
    bool has_avx2;
    bool has_avx512f;
} CPUFeatures;

typedef enum {
    SM_NERF_MLP_VARIANT_GENERIC = 0,
    SM_NERF_MLP_VARIANT_PREPACKED = 1,
    SM_NERF_MLP_VARIANT_BF16 = 2
} SMNeRFMLPVariant;

/* Get CPU capabilities at runtime */
CPUFeatures sm_detect_cpu_features(void);

/* ===== SIMD Ray Batch Structures ===== */

#define SIMD_BATCH_SIZE 8

typedef struct {
    Vec3 origin[SIMD_BATCH_SIZE];
    Vec3 direction[SIMD_BATCH_SIZE];
    float tmin[SIMD_BATCH_SIZE];
    float tmax[SIMD_BATCH_SIZE];
    uint32_t pixel_id[SIMD_BATCH_SIZE];
    uint8_t active[SIMD_BATCH_SIZE];
    uint32_t count;
} RayBatch;

typedef struct {
    float rgb[SIMD_BATCH_SIZE][3];
    float sigma[SIMD_BATCH_SIZE];
    uint8_t valid[SIMD_BATCH_SIZE];
} NeRFBatchOutput;

typedef struct {
    Vec3 rgb;
    float alpha;
} NeRFPixel;

/* ===== NeRF Configuration (from binary header) ===== */

typedef struct {
    uint32_t magic;
    uint32_t version;
    uint32_t num_levels;
    uint32_t features_per_entry;
    uint32_t hashmap_size;
    uint32_t base_res;
    float per_level_scale;
    uint32_t mlp_in_dim;
    uint32_t mlp_hidden_dim;
    uint32_t mlp_num_layers;
    uint32_t mlp_out_dim;
    float scale;
    Vec3 center;
} NeRFConfig;

typedef struct {
    float *hashgrid_data;        // Hashgrid features as float32 (NULL if compact)
    uint16_t *hashgrid_fp16;     // Hashgrid features as FP16 bits (NULL if float32)
    int hashgrid_compact;        // 1 = FP16 storage (halves memory), 0 = FP32
    float *mlp_weights;          // All MLP weights concatenated
    float *mlp_biases;           // All MLP biases concatenated
    float *mlp_w0_aligned;       // Optional 32-byte-aligned copy of W0
    float *mlp_w1_aligned;       // Optional 32-byte-aligned copy of W1
    float *mlp_wout_t_aligned;   // Optional transposed/aligned output weights [out][hidden]
    float *mlp_b0_aligned;       // Optional aligned copy of first hidden bias
    float *mlp_b1_aligned;       // Optional aligned copy of second hidden bias
    float *mlp_bout_aligned;     // Optional aligned copy of output bias
    int mlp_prepacked_ready;     // 1 when aligned/transposed single-ray data is ready
#ifdef __AVX512BF16__
    uint16_t *mlp_w0_bf16;      // Pair-interleaved BF16, [14 pairs][64 outputs] (28 inputs padded from 27)
    uint16_t *mlp_w1_bf16;      // Pair-interleaved BF16, [32 pairs][64 outputs]
    int mlp_bf16_ready;          // 1 when BF16 pair-interleaved weights are ready
#endif
    uint8_t *occupancy_grid;     // 64^3 occupancy grid
    NeRFConfig config;
} NeRFData;

typedef struct {
    NeRFPixel *pixels;
    uint32_t width;
    uint32_t height;
} NeRFFramebuffer;

/* ===== SIMD Function Declarations ===== */

/* Load NeRF data from binary file */
NeRFData* sm_nerf_data_load(const char *hashgrid_path, const char *occ_path);
void sm_nerf_data_free(NeRFData *data);

/* Batched hashgrid feature extraction (8 rays in parallel) */
void sm_hashgrid_lookup_batch(
    const Vec3 positions[SIMD_BATCH_SIZE],
    const NeRFConfig *config,
    const float *hashgrid_data,
    float features_out[SIMD_BATCH_SIZE][24]  /* 12 levels * 2 features */
);

/* Batched MLP inference (8 rays through network) */
void sm_mlp_inference_batch(
    const float features_in[SIMD_BATCH_SIZE][27],  /* 24 hashgrid + 3 view direction */
    const NeRFConfig *config,
    const float *mlp_weights,
    const float *mlp_biases,
    float rgb_out[SIMD_BATCH_SIZE][3],
    float sigma_out[SIMD_BATCH_SIZE]
);

/* Single-ray MLP inference (more efficient for single-lane execution) */
void sm_mlp_inference_single(
    const float features_in[27],
    const NeRFConfig *config,
    const float *mlp_weights,
    const float *mlp_biases,
    float rgb_out[3],
    float *sigma_out
);

/* Batched occupancy grid lookup */
void sm_occupancy_lookup_batch(
    const Vec3 positions[SIMD_BATCH_SIZE],
    const NeRFConfig *config,
    const uint8_t *occ_grid,
    uint8_t occupancy_out[SIMD_BATCH_SIZE]
);

/* Volume integration for batch of rays (main rendering kernel) */
void sm_volume_integrate_batch(
    const RayBatch *batch,
    const NeRFConfig *config,
    const NeRFData *nerf_data,
    NeRFFramebuffer *output_fb,
    uint32_t num_steps,
    float density_scale,
    float bounds_max
);

/* Adaptive sampling helpers */
float sm_adaptive_step_size(
    const Vec3 pos,
    const uint8_t *occ_grid,
    const NeRFConfig *config,
    float base_step
);

bool sm_ray_should_terminate(float accumulated_alpha);

/* Profiling utilities */
typedef struct {
    uint64_t sample_count;
    uint64_t total_cycles;
    double total_time_ms;
} PerfCounter;

void sm_perf_start(uint64_t *start_cycle);
void sm_perf_end(uint64_t start_cycle, PerfCounter *counter);
void sm_perf_report(const char *name, const PerfCounter *counter);

typedef struct {
    uint64_t rays_started;
    uint64_t steps_attempted;
    uint64_t steps_completed;
    uint64_t rays_terminated_early;
    uint64_t ns_adaptive;
    uint64_t ns_hashgrid;
    uint64_t ns_mlp;
    uint64_t ns_composite;
    uint64_t ns_writeback;
    uint64_t ns_other;
} NeRFPhaseTiming;

void sm_nerf_phase_timing_reset(void);
NeRFPhaseTiming sm_nerf_phase_timing_snapshot(void);
void sm_nerf_phase_timing_report(const char *label);

/* Accessor to the last-rendered framebuffer (owned by nerf_simd integration)
 * Returns NULL if not initialized. */
NeRFFramebuffer* sm_nerf_get_framebuffer(void);

/* Integration helpers implemented in nerf_simd_integration.c */
void sm_nerf_init(const char *hashgrid_path, const char *occ_path, uint32_t width, uint32_t height);
void sm_nerf_shutdown(void);
void sm_render_nerf_frame(const Camera *camera, uint32_t width, uint32_t height, uint32_t num_steps, float density_scale, float bounds_max);

/* Simple env helpers */
uint32_t sm_nerf_get_steps(void);
float sm_nerf_get_density(void);
float sm_nerf_get_bounds(void);
SMNeRFMLPVariant sm_nerf_mlp_variant(void);
const char *sm_nerf_mlp_variant_name(SMNeRFMLPVariant variant);

#endif  /* NERF_SIMD_H */
