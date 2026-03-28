#pragma once
#include "vec3.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    Vec3 *normal;   // RGB float32 (xyz)
    Vec3 *albedo;   // RGB float32
    float *depth;   // float32 (t or distance)
    int width, height;
} SM_GBuffer;

// global target accessed from render.c
void sm_gbuffer_set_targets(SM_GBuffer gb);

#ifdef __cplusplus
}
#endif
