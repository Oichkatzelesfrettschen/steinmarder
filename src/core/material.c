// material.c
#include <math.h>
#include <stdlib.h>
#include <time.h>

#include "material.h"
#include "vec3.h"
#include "ray.h"

static float rand01(void) {
    return (float)rand() / (float)RAND_MAX;
}

// Direct unit-sphere surface sampling via cylindrical method.
// Uniform on the sphere surface. No rejection loop.
//   z = 2*rand - 1 (uniform in [-1, 1])
//   r = sqrt(1 - z*z)
//   theta = 2*pi*rand
//   return (r*cos(theta), r*sin(theta), z)
static Vec3 random_unit_vector(void) {
    float z = 2.0f * rand01() - 1.0f;
    float r = sqrtf(fmaxf(0.0f, 1.0f - z * z));
    float theta = 6.2831853f * rand01();  // 2*pi
    return vec3(r * cosf(theta), r * sinf(theta), z);
}

// Uniform sampling inside unit sphere: unit vector scaled by cbrt(rand).
// cbrtf gives uniform volume distribution (r^3 is uniform -> r = cbrt(U)).
static Vec3 random_in_unit_sphere(void) {
    Vec3 dir = random_unit_vector();
    float scale = cbrtf(rand01());
    return vec3_scale(dir, scale);
}

static Vec3 reflect(Vec3 v, Vec3 n) {
    return vec3_reflect(v, n);
}

static Vec3 refract(Vec3 uv, Vec3 n, float etai_over_etat) {
    float cos_theta = fminf(vec3_dot(vec3_scale(uv, -1.0f), n), 1.0f);
    Vec3 r_out_perp  = vec3_scale(vec3_add(uv, vec3_scale(n, cos_theta)), etai_over_etat);
    float k = 1.0f - vec3_length_squared(r_out_perp);
    if (k < 0.0f) {
        // total internal reflection
        return reflect(uv, n);
    }
    Vec3 r_out_parallel = vec3_scale(n, -sqrtf(fabsf(k)));
    return vec3_add(r_out_perp, r_out_parallel);
}

static float schlick(float cosine, float ref_idx) {
    float r0 = (1.0f - ref_idx) / (1.0f + ref_idx);
    r0 = r0 * r0;
    float x = 1.0f - cosine;
    float x2 = x * x;
    return r0 + (1.0f - r0) * (x2 * x2 * x);  // x^5 via 3 muls instead of powf
}

bool material_scatter(const Material *mat,
                      Ray in,
                      Vec3 hit_point,
                      Vec3 normal,
                      Ray *scattered,
                      Vec3 *attenuation)
{
    // Emissive: light only, no scatter
    if (mat->type == MAT_EMISSIVE) {
        *attenuation = vec3(0.0f, 0.0f, 0.0f);
        return false;
    }

    Vec3 unit_dir = vec3_unit(in.direction);

    switch (mat->type) {
        case MAT_LAMBERTIAN: {
            Vec3 scatter_dir = vec3_add(normal, random_unit_vector());

            // Degenerate scatter direction: fall back to normal
            if (vec3_length_squared(scatter_dir) < 1e-8f) {
                scatter_dir = normal;
            }

            *scattered = ray(hit_point, scatter_dir);
            *attenuation = mat->albedo;
        } break;

        case MAT_METAL: {
            Vec3 reflected = reflect(unit_dir, normal);
            Vec3 perturbed = vec3_add(reflected,
                                      vec3_scale(random_in_unit_sphere(), mat->fuzz));
            *scattered = ray(hit_point, perturbed);
            *attenuation = mat->albedo;

            // Metal: if reflection goes into surface, absorb
            if (vec3_dot(scattered->direction, normal) <= 0.0f) {
                return false;
            }
        } break;

        case MAT_DIELECTRIC: {
            *attenuation = vec3(1.0f, 1.0f, 1.0f); // Glass: colorless

            float refraction_ratio = vec3_dot(unit_dir, normal) > 0.0f
                                   ? mat->ref_idx
                                   : 1.0f / mat->ref_idx;

            float cos_theta = fminf(vec3_dot(vec3_scale(unit_dir, -1.0f), normal), 1.0f);
            float sin_theta = sqrtf(fmaxf(0.0f, 1.0f - cos_theta * cos_theta));

            bool cannot_refract = refraction_ratio * sin_theta > 1.0f;
            Vec3 direction;

            if (cannot_refract || schlick(cos_theta, refraction_ratio) > rand01()) {
                direction = reflect(unit_dir, normal);
            } else {
                direction = refract(unit_dir, normal, 1.0f / refraction_ratio);
            }

            *scattered = ray(hit_point, direction);
        } break;

        default:
            return false;
    }

    return true;
}
