/* sm_upscale.h — DLSS-class temporal super-resolution for steinmarder
 *
 * Pure Vulkan compute. No proprietary SDK.  Engine-agnostic API.
 *
 * Pipeline:
 *   Pass 1  Temporal reprojection (reproject history, clamp, disocclusion detect)
 *   Pass 2  Neural super-resolution (lightweight CNN, ~11K params, fused compute)
 *   Pass 3  Sharpening + history buffer update
 *
 * Fallback: When neural weights are unavailable, Pass 2 is replaced by
 *           Catmull-Rom upsample + TAA blend (the "temporal-only" path).
 *
 * Usage:
 *   SmUpscaleCtx ctx;
 *   sm_upscale_init(&ctx, device, physDevice, queueFamily, queue, ...);
 *   // each frame:
 *   sm_upscale_execute(&ctx, cmd, &frame_params);
 *   // teardown:
 *   sm_upscale_destroy(&ctx);
 *
 * Build:
 *   Link with Vulkan loader. Shader SPIR-V blobs compiled separately via
 *   glslangValidator / glslc from shaders/upscale_*.comp.
 *
 * Environment overrides (runtime):
 *   SM_UPSCALE_MODE      0=auto, 1=neural, 2=temporal-only
 *   SM_UPSCALE_SHARP     Sharpening strength 0.0–1.0 (default 0.2)
 *   SM_UPSCALE_CLAMP_γ   AABB clamp extent (default 1.0)
 *   SM_UPSCALE_DEBUG      0=off, 1=confidence, 2=rejection, 3=flow, 4=split
 */

#ifndef SM_UPSCALE_H
#define SM_UPSCALE_H

#include <vulkan/vulkan.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ────────────────────────────────────────────────────────────── */
/*  Quality presets (maps to internal render resolution ratio)    */
/* ────────────────────────────────────────────────────────────── */
typedef enum SmUpscaleQuality {
    SM_UPSCALE_QUALITY_ULTRA_PERF  = 0, /* r=3.0 — 720p  → 4K */
    SM_UPSCALE_QUALITY_PERFORMANCE = 1, /* r=2.0 — 1080p → 4K */
    SM_UPSCALE_QUALITY_BALANCED    = 2, /* r=1.7 — 1440p → 4K */
    SM_UPSCALE_QUALITY_QUALITY     = 3, /* r=1.5 — ~1800p → 4K */
    SM_UPSCALE_QUALITY_ULTRA       = 4, /* r=1.3 — ~2160p → 4K (AA-only) */
    SM_UPSCALE_QUALITY_COUNT
} SmUpscaleQuality;

/* Returns the render scale factor for a quality preset (0.33 … 0.77)
 * Defined inline below. */

/* ────────────────────────────────────────────────────────────── */
/*  Jitter generation (Halton 2,3 sequence)                      */
/* ────────────────────────────────────────────────────────────── */
typedef struct SmJitter {
    float x;   /* ∈ [−0.5, +0.5], sub-pixel offset in low-res pixel coords */
    float y;
} SmJitter;

/* Returns jitter for frame_index using Halton(2,3). Sequence length = 16.
 * Defined inline below. */

/* ────────────────────────────────────────────────────────────── */
/*  Per-frame parameters (caller fills before execute)           */
/* ────────────────────────────────────────────────────────────── */
typedef struct SmUpscaleFrameParams {
    /* Input images ──────────────────────────────────────────── */
    VkImageView  color_lo;      /* R16G16B16A16_SFLOAT, W_lo × H_lo */
    VkImageView  depth_lo;      /* R32_SFLOAT,          W_lo × H_lo */
    VkImageView  motion_vec;    /* R16G16_SFLOAT,       W_lo × H_lo */

    /* Output image ─────────────────────────────────────────── */
    VkImageView  output_hi;     /* R16G16B16A16_SFLOAT, W_hi × H_hi, STORAGE */

    /* Dimensions ───────────────────────────────────────────── */
    uint32_t  w_lo, h_lo;       /* internal render resolution */
    uint32_t  w_hi, h_hi;       /* display / output resolution */

    /* Temporal ─────────────────────────────────────────────── */
    SmJitter jitter;            /* current frame jitter (from sm_upscale_jitter) */
    uint32_t  frame_index;       /* monotonic frame counter */
    float     dt;                /* frame delta time in seconds (for motion scaling) */

    /* Camera ───────────────────────────────────────────────── */
    float     near_plane;
    float     far_plane;

    /* Optional overrides (0 = use defaults) ────────────────── */
    float     sharpness;         /* 0.0–1.0, default 0.2 */
    float     clamp_gamma;       /* AABB clamp extent γ, default 1.0 */
    int       debug_mode;        /* 0=off, 1–4 (see header comment) */
} SmUpscaleFrameParams;

/* ────────────────────────────────────────────────────────────── */
/*  Initialization parameters                                    */
/* ────────────────────────────────────────────────────────────── */
typedef struct SmUpscaleInitInfo {
    VkDevice                device;
    VkPhysicalDevice        phys_device;
    uint32_t                queue_family_index;     /* compute-capable family */
    VkQueue                 compute_queue;

    /* Maximum display resolution (allocates history/intermediate at this size) */
    uint32_t                max_display_w;
    uint32_t                max_display_h;

    /* Path to neural network weight blob (NULL = temporal-only fallback) */
    const char*             weight_file_path;

    /* SPIR-V shader modules (caller compiles .comp → .spv externally) */
    /* Pass NULL to use embedded SPIR-V if available */
    const uint32_t*         spv_reproj;
    size_t                  spv_reproj_size;
    const uint32_t*         spv_neural;
    size_t                  spv_neural_size;
    const uint32_t*         spv_sharpen;
    size_t                  spv_sharpen_size;

    /* Vulkan allocator (NULL for default) */
    const VkAllocationCallbacks* allocator;
} SmUpscaleInitInfo;

/* ────────────────────────────────────────────────────────────── */
/*  Internal state (opaque to caller)                            */
/* ────────────────────────────────────────────────────────────── */

#define SM_UPSCALE_WEIGHT_MAGIC  0x5955574Eu  /* 'YUWN' */
#define SM_UPSCALE_WEIGHT_VER    1
#define SM_UPSCALE_MAX_PARAMS    16384         /* max FP16 params */

typedef struct SmUpscaleWeightHeader {
    uint32_t magic;
    uint32_t version;
    uint32_t param_count;  /* number of FP16 values */
    uint32_t flags;        /* bit 0: has cooperative matrix layout */
} SmUpscaleWeightHeader;

typedef struct SmUpscaleCtx {
    VkDevice         device;
    VkQueue          compute_queue;
    uint32_t         queue_family;
    const VkAllocationCallbacks* alloc;

    /* Display resolution caps */
    uint32_t         max_w, max_h;

    /* History ping-pong (double-buffered) */
    VkImage          history_img[2];
    VkDeviceMemory   history_mem[2];
    VkImageView      history_view[2];
    uint32_t         history_idx;          /* current write target (0 or 1) */

    /* Intermediate buffers */
    VkImage          reproj_img;
    VkDeviceMemory   reproj_mem;
    VkImageView      reproj_view;

    VkImage          confidence_img;
    VkDeviceMemory   confidence_mem;
    VkImageView      confidence_view;

    /* Neural network weights */
    VkBuffer         weight_buf;
    VkDeviceMemory   weight_mem;
    int              has_neural_weights;    /* 0 = temporal-only fallback */

    /* Samplers */
    VkSampler        sampler_linear;       /* bilinear, clamp-to-edge */
    VkSampler        sampler_nearest;      /* point, clamp-to-edge */

    /* Descriptor pool & sets */
    VkDescriptorPool       desc_pool;
    VkDescriptorSetLayout  dsl_reproj;
    VkDescriptorSetLayout  dsl_neural;
    VkDescriptorSetLayout  dsl_sharpen;
    VkDescriptorSet        ds_reproj;
    VkDescriptorSet        ds_neural;
    VkDescriptorSet        ds_sharpen;

    /* Pipeline layouts & pipelines */
    VkPipelineLayout  pl_layout_reproj;
    VkPipelineLayout  pl_layout_neural;
    VkPipelineLayout  pl_layout_sharpen;
    VkPipeline        pipeline_reproj;
    VkPipeline        pipeline_neural;
    VkPipeline        pipeline_neural_fallback;   /* temporal-only path */
    VkPipeline        pipeline_sharpen;

    /* Shader modules (owned, destroyed on teardown) */
    VkShaderModule    sm_reproj;
    VkShaderModule    sm_neural;
    VkShaderModule    sm_neural_fallback;
    VkShaderModule    sm_sharpen;

    /* Feature flags */
    int               supports_fp16;
    int               supports_coop_matrix;

    /* Frame counter (for Halton jitter phase tracking) */
    uint32_t          frame_count;
} SmUpscaleCtx;

/* ────────────────────────────────────────────────────────────── */
/*  Push constants (mirrored in GLSL shaders)                    */
/* ────────────────────────────────────────────────────────────── */

/* Pass 1: Reprojection */
typedef struct SmReprojPC {
    uint32_t  w_lo, h_lo;
    uint32_t  w_hi, h_hi;
    float     jitter_x, jitter_y;
    float     clamp_gamma;       /* AABB extent */
    float     near_z, far_z;
    uint32_t  frame_index;
    int32_t   debug_mode;
    float     _pad;
} SmReprojPC;

/* Pass 2: Neural upscale */
typedef struct SmNeuralPC {
    uint32_t  w_lo, h_lo;
    uint32_t  w_hi, h_hi;
    float     jitter_x, jitter_y;
    float     upscale_ratio;     /* w_hi / w_lo */
    uint32_t  param_count;       /* number of FP16 weight values */
    uint32_t  use_coop_matrix;   /* 1 if cooperative matrix path */
    float     _pad[3];
} SmNeuralPC;

/* Pass 3: Sharpen + history write */
typedef struct SmSharpenPC {
    uint32_t  w_hi, h_hi;
    float     sharpness;         /* 0.0–1.0 */
    int32_t   debug_mode;
} SmSharpenPC;

/* ────────────────────────────────────────────────────────────── */
/*  Public API                                                   */
/* ────────────────────────────────────────────────────────────── */

/* Initialize upscale context. Returns VK_SUCCESS or error code.
 * Allocates GPU resources for max display resolution.              */
VkResult sm_upscale_init(SmUpscaleCtx* ctx, const SmUpscaleInitInfo* info);

/* Record upscale passes into an existing command buffer.
 * The command buffer must be in recording state.
 * Inserts appropriate pipeline barriers between passes.            */
VkResult sm_upscale_execute(SmUpscaleCtx* ctx,
                             VkCommandBuffer cmd,
                             const SmUpscaleFrameParams* params);

/* Update descriptor sets when input image views change.
 * Must be called at least once before first execute, and again
 * whenever color_lo/depth_lo/motion_vec/output_hi views change.    */
void sm_upscale_update_descriptors(SmUpscaleCtx* ctx,
                                    const SmUpscaleFrameParams* params);

/* Reload neural network weights from file. Can be called at runtime
 * to hot-swap trained models.                                       */
VkResult sm_upscale_load_weights(SmUpscaleCtx* ctx, const char* path);

/* Destroy all GPU resources. Safe to call with partially-initialized ctx. */
void sm_upscale_destroy(SmUpscaleCtx* ctx);

/* ────────────────────────────────────────────────────────────── */
/*  Inline utility implementations                              */
/* ────────────────────────────────────────────────────────────── */

static inline float sm_halton(uint32_t index, uint32_t base) {
    float f = 1.0f, result = 0.0f;
    uint32_t i = index;
    while (i > 0) {
        f /= (float)base;
        result += f * (float)(i % base);
        i /= base;
    }
    return result;
}

static inline SmJitter sm_upscale_jitter(uint32_t frame_index) {
    /* 16-phase Halton sequence; wrap to avoid degenerate clustering */
    uint32_t idx = (frame_index % 16) + 1; /* 1-based to skip origin */
    SmJitter j;
    j.x = sm_halton(idx, 2) - 0.5f;
    j.y = sm_halton(idx, 3) - 0.5f;
    return j;
}

static inline float sm_upscale_quality_factor(SmUpscaleQuality q) {
    static const float factors[SM_UPSCALE_QUALITY_COUNT] = {
        1.0f / 3.0f,   /* ULTRA_PERF  */
        1.0f / 2.0f,   /* PERFORMANCE */
        1.0f / 1.7f,   /* BALANCED    */
        1.0f / 1.5f,   /* QUALITY     */
        1.0f / 1.3f,   /* ULTRA       */
    };
    if (q < 0 || q >= SM_UPSCALE_QUALITY_COUNT) return 0.5f;
    return factors[q];
}

#ifdef __cplusplus
}
#endif

#endif /* SM_UPSCALE_H */
