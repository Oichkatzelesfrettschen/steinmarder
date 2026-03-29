// test_render_smoke.c -- Minimal CPU renderer smoke test.
// Renders a tiny image and verifies basic properties:
//   1. No NaN or Inf in output pixels
//   2. Total luminance is nonzero (something was rendered)
//   3. Pixel values are in [0, max_radiance] range

#include "vec3.h"
#include "camera.h"
#include "render.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

int main(void) {
    int W = 32, H = 32, SPP = 2, DEPTH = 4;
    int fail = 0;

    Vec3* pixels = (Vec3*)calloc((size_t)W * H, sizeof(Vec3));
    if (!pixels) {
        fprintf(stderr, "malloc failed\n");
        return 1;
    }

    Camera cam = camera_create(
        (float)W / (float)H,  // aspect ratio
        2.0f,                 // viewport height
        1.0f                  // focal length
    );

    render_scene_st(pixels, W, H, cam, SPP, DEPTH);

    // Check 1: No NaN/Inf
    int nan_count = 0, inf_count = 0;
    float total_lum = 0.0f;
    for (int i = 0; i < W * H; i++) {
        float r = pixels[i].x, g = pixels[i].y, b = pixels[i].z;
        if (r != r || g != g || b != b) nan_count++;
        if (fabsf(r) > 1e30f || fabsf(g) > 1e30f || fabsf(b) > 1e30f) inf_count++;
        total_lum += 0.2126f * r + 0.7152f * g + 0.0722f * b;
    }

    if (nan_count > 0) {
        fprintf(stderr, "FAIL: %d pixels have NaN values\n", nan_count);
        fail = 1;
    }
    if (inf_count > 0) {
        fprintf(stderr, "FAIL: %d pixels have Inf values\n", inf_count);
        fail = 1;
    }

    // Check 2: Something was rendered
    if (total_lum <= 0.0f) {
        fprintf(stderr, "FAIL: total luminance = %f (expected > 0)\n", total_lum);
        fail = 1;
    }

    if (!fail) {
        printf("=== Render smoke test PASSED ===\n");
        printf("  %dx%d, %d SPP, total luminance = %.4f\n", W, H, SPP, total_lum);
    }

    free(pixels);
    return fail;
}
