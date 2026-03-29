#include "nerf_scheduler.h"
#include <stdio.h>
#include <string.h>
#include <math.h>

int nerf_scheduler_init(NerfScheduleQueues* q, uint32_t batch_size){
    if(!q || batch_size == 0) return 0;
    memset(q, 0, sizeof(*q));
    if(!nerf_ray_batch_init(&q->gpu, batch_size)) return 0;
    if(!nerf_ray_batch_init(&q->cpu, batch_size)){
        nerf_ray_batch_free(&q->gpu);
        return 0;
    }
    return 1;
}

void nerf_scheduler_free(NerfScheduleQueues* q){
    if(!q) return;
    nerf_ray_batch_free(&q->gpu);
    nerf_ray_batch_free(&q->cpu);
    memset(q, 0, sizeof(*q));
}

// Copy one ray from src[i] to dst[j].
static void copy_ray(NerfRayBatch* dst, uint32_t j,
                     const NerfRayBatch* src, uint32_t i) {
    dst->pix[j]  = src->pix[i];
    dst->ox[j]   = src->ox[i];
    dst->oy[j]   = src->oy[i];
    dst->oz[j]   = src->oz[i];
    dst->dx[j]   = src->dx[i];
    dst->dy[j]   = src->dy[i];
    dst->dz[j]   = src->dz[i];
    dst->tmin[j] = src->tmin[i];
    dst->tmax[j] = src->tmax[i];
}

void nerf_schedule_split(const NerfScheduleConfig* cfg,
                         const NerfRayBatch* frame_rays,
                         NerfScheduleQueues* out){
    if(!cfg || !frame_rays || !out) return;

    out->gpu.count = 0;
    out->cpu.count = 0;

    // Foveated scheduling: rays within fovea_radius of screen center go to GPU
    // (higher quality, lower latency), periphery goes to CPU.
    //
    // If fovea_radius <= 0 or >= 1, fall back to cpu_share-based proportional split.
    //
    // The pixel ID encodes (y * width + x). We need width to decode, but it is
    // not in the config. Use the max pixel ID as a heuristic for total pixels,
    // then assume square-ish aspect ratio for fovea computation.

    int use_fovea = (cfg->fovea_radius > 0.0f && cfg->fovea_radius < 1.0f);

    // Find max pixel ID to estimate frame dimensions
    uint32_t max_pix = 0;
    if (use_fovea) {
        for (uint32_t i = 0; i < frame_rays->count; i++) {
            if (frame_rays->pix[i] > max_pix) max_pix = frame_rays->pix[i];
        }
    }

    // Estimate width from max pixel (assume ~16:9 aspect)
    uint32_t est_total = max_pix + 1;
    uint32_t est_w = (uint32_t)sqrtf((float)est_total * 16.0f / 9.0f);
    if (est_w < 1) est_w = 1;
    uint32_t est_h = est_total / est_w;
    if (est_h < 1) est_h = 1;

    float center_x = (float)est_w * 0.5f;
    float center_y = (float)est_h * 0.5f;
    float fovea_r_sq = cfg->fovea_radius * cfg->fovea_radius *
                       (center_x * center_x + center_y * center_y);

    // Proportional split threshold (for non-fovea mode)
    float cpu_threshold = cfg->cpu_share;
    if (cpu_threshold < 0.0f) cpu_threshold = 0.0f;
    if (cpu_threshold > 1.0f) cpu_threshold = 1.0f;

    for (uint32_t i = 0; i < frame_rays->count; i++) {
        int to_gpu;

        if (use_fovea) {
            // Fovea: compute distance from screen center
            uint32_t px = frame_rays->pix[i] % est_w;
            uint32_t py = frame_rays->pix[i] / est_w;
            float dx = (float)px - center_x;
            float dy = (float)py - center_y;
            float dist_sq = dx * dx + dy * dy;
            to_gpu = (dist_sq <= fovea_r_sq) ? 1 : 0;
        } else {
            // Proportional: use cpu_share to decide.
            // Deterministic split based on pixel index (no randomness).
            float frac = (float)(i % 256) / 256.0f;
            to_gpu = (frac >= cpu_threshold) ? 1 : 0;
        }

        NerfRayBatch* dst = to_gpu ? &out->gpu : &out->cpu;
        if (dst->count >= dst->capacity) {
            // Queue full -- overflow to the other queue
            dst = to_gpu ? &out->cpu : &out->gpu;
            if (dst->count >= dst->capacity) continue; // Both full, drop ray
        }

        copy_ray(dst, dst->count++, frame_rays, i);
    }
}
