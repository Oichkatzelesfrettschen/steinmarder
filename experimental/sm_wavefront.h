// sm_wavefront.h
// Experimental: Wavefront path tracing skeleton (CPU)
// Goal: Separate generation/intersection/shading into queues for better batching and cache behavior.
// This is a serious base that you can wire into your existing integrator step by step.

#ifndef SM_WAVEFRONT_H
#define SM_WAVEFRONT_H

#include <stdint.h>
#include "../ray.h"
#include "../vec3.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    Ray ray;
    Vec3 throughput;   // path throughput (RGB)
    uint32_t pixel;    // pixel index (x + y*w)
    uint32_t depth;    // bounce depth
    uint32_t rng;      // per-path RNG state (example: xorshift/pcg seed)
} SM_Path;

typedef struct {
    int hit;
    float t;
    Vec3 p;
    Vec3 n;
    int material_id;
} SM_SurfHit;

typedef struct {
    SM_Path* items;
    uint32_t count;
    uint32_t capacity;
} SM_PathQueue;

void sm_queue_init(SM_PathQueue* q, uint32_t capacity);
void sm_queue_free(SM_PathQueue* q);
void sm_queue_clear(SM_PathQueue* q);
int  sm_queue_push(SM_PathQueue* q, const SM_Path* p);

// Main wavefront pipeline (engine adapter will be needed):
// - generate primary rays into q_active
// - intersect them (BVH) into hits array
// - shade/scatter -> produce q_next
// This file provides the queueing + pipeline skeleton, not your materials/BVH bindings.
typedef struct {
    uint32_t width, height;
    uint32_t spp;
    uint32_t max_depth;
    uint32_t base_seed;
} SM_WavefrontSettings;

typedef struct {
    SM_PathQueue q_active;
    SM_PathQueue q_next;
    SM_SurfHit*  hits;     // size = q_active.capacity
} SM_WavefrontState;

int  sm_wavefront_init(SM_WavefrontState* st, uint32_t max_paths);
void sm_wavefront_free(SM_WavefrontState* st);

// Entry point (adapter expected):
// The adapter should provide two callbacks:
//  (1) intersect_cb: fills hits for each active path
//  (2) shade_cb: consumes hits and produces next paths / accumulates to framebuffer
typedef void (*SM_IntersectCB)(const SM_Path* paths, uint32_t n, SM_SurfHit* out_hits, void* user);
typedef void (*SM_ShadeCB)(const SM_Path* paths, const SM_SurfHit* hits, uint32_t n,
                            SM_PathQueue* q_next, void* user);

void sm_wavefront_render(const SM_WavefrontSettings* s,
                          SM_WavefrontState* st,
                          SM_IntersectCB intersect_cb,
                          SM_ShadeCB shade_cb,
                          void* user);

#ifdef __cplusplus
}
#endif

#endif // SM_WAVEFRONT_H
