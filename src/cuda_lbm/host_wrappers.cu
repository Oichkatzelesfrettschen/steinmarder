// Host-side dispatch wrappers for all D3Q19 LBM kernel variants.
// Provides unified launch interface regardless of precision tier or layout.

#include "host_wrappers.h"
#include "lbm_metrics.h"
#include <stdio.h>

// ============================================================================
// Kernel forward declarations
// ============================================================================

// Standard ping-pong signature:
//   (const T* f_in, T* f_out, float* rho, float* u, const float* tau,
//    const float* force, int nx, int ny, int nz)
//
// A-A signature:
//   (T* f, float* rho, float* u, const float* tau, const float* force,
//    int nx, int ny, int nz, int parity)
//
// DD signature:
//   (const double* f_hi_in, const double* f_lo_in, double* f_hi_out, double* f_lo_out,
//    float* rho, float* u, const float* force, const float* tau, int nx, int ny, int nz)
//
// Tiled kernels use 3D grid. All others use 1D grid.
// All kernel functions are declared extern "C" in their .cu files.
// Forward declarations must be at file scope for correct C linkage.

extern "C" {

// Step kernels -- standard ping-pong signature
void lbm_step_soa_fused(const float*, float*, float*, float*, const float*, const float*, int, int, int);
void lbm_step_soa_mrt_fused(const float*, float*, float*, float*, const float*, const float*, int, int, int);
void lbm_step_soa_pull(const float*, float*, float*, float*, const float*, const float*, int, int, int);
void lbm_step_soa_mrt_pull(const float*, float*, float*, float*, const float*, const float*, int, int, int);
void lbm_step_soa_tiled(const float*, float*, float*, float*, const float*, const float*, int, int, int);
void lbm_step_soa_mrt_tiled(const float*, float*, float*, float*, const float*, const float*, int, int, int);
void lbm_step_soa_coarsened(const float*, float*, float*, float*, const float*, const float*, int, int, int);
void lbm_step_soa_mrt_coarsened(const float*, float*, float*, float*, const float*, const float*, int, int, int);
void lbm_step_soa_coarsened_float4(const float*, float*, float*, float*, const float*, const float*, int, int, int);
void lbm_step_fp32_soa_cs_kernel(const float*, float*, float*, float*, const float*, const float*, int, int, int);
void lbm_step_fused_fp16_kernel(const void*, void*, float*, float*, const float*, const float*, int, int, int);
void lbm_step_fp16_soa_kernel(const void*, void*, float*, float*, const float*, const float*, int, int, int);
void lbm_step_fp16_soa_half2_kernel(const void*, void*, float*, float*, const float*, const float*, int, int, int);
void lbm_step_fused_bf16_kernel(const void*, void*, float*, float*, const float*, const float*, int, int, int);
void lbm_step_bf16_soa_kernel(const void*, void*, float*, float*, const float*, const float*, int, int, int);
void lbm_step_bf16_soa_bf162_kernel(const void*, void*, float*, float*, const float*, const float*, int, int, int);
void lbm_step_fused_fp8_kernel(const void*, void*, float*, float*, const float*, const float*, int, int, int);
void lbm_step_fp8_soa_kernel(const void*, void*, float*, float*, const float*, const float*, int, int, int);
void lbm_step_fused_fp8e5m2_kernel(const void*, void*, float*, float*, const float*, const float*, int, int, int);
void lbm_step_fp8_e5m2_soa_kernel(const void*, void*, float*, float*, const float*, const float*, int, int, int);
void lbm_step_fused_int8_kernel(const void*, void*, float*, float*, const float*, const float*, int, int, int);
void lbm_step_int8_soa_kernel(const void*, void*, float*, float*, const float*, const float*, int, int, int);
void lbm_step_int8_soa_coarsened_kernel(const signed char*, signed char*, float*, float*, const float*, const float*, int, int, int);
void lbm_step_int8_soa_lloydmax_kernel(const unsigned char*, unsigned char*, float*, float*, const float*, const float*, int, int, int);
void lbm_step_int8_soa_lloydmax_c4_kernel(const unsigned char*, unsigned char*, float*, float*, const float*, const float*, int, int, int);
void lbm_step_int8_soa_lloydmax_pipe_kernel(const unsigned char*, unsigned char*, float*, float*, const float*, const float*, int, int, int);
void lbm_step_int8_soa_lloydmax_hiocc_kernel(const unsigned char*, unsigned char*, float*, float*, const float*, const float*, int, int, int);
void initialize_uniform_int8_soa_lloydmax_kernel(unsigned char*, float*, float*, float*, float, float, float, float, float, int, int, int);
void lbm_step_int16_kernel(const void*, void*, float*, float*, const float*, const float*, int, int, int);
void lbm_step_int16_soa_kernel(const void*, void*, float*, float*, const float*, const float*, int, int, int);
void lbm_step_fused_fp64_kernel(const void*, void*, float*, float*, const float*, const float*, int, int, int);
void lbm_step_fp64_soa_kernel(const void*, void*, float*, float*, const float*, const float*, int, int, int);
void lbm_step_fused_int4_kernel(const void*, void*, float*, float*, const float*, const float*, int, int, int);
void lbm_step_fp4_kernel(const void*, void*, float*, float*, const float*, const float*, int, int, int);

// Step kernels -- A-A signature
void lbm_step_soa_aa(float*, float*, float*, const float*, const float*, int, int, int, int);
void lbm_step_soa_mrt_aa(float*, float*, float*, const float*, const float*, int, int, int, int);

// Step kernels -- Q16.16 fixed-point
void lbm_step_q16_soa_kernel(const int*, int*, float*, float*, const float*, const float*, int, int, int);

// Init kernels -- Q16.16
void initialize_uniform_q16_soa_kernel(int*, float*, float*, float*, float, float, float, float, float, int, int, int);
void lbm_step_q16_soa_lm_kernel(const int*, int*, float*, float*, const float*, const float*, int, int, int);
void initialize_uniform_q16_soa_lm_kernel(int*, float*, float*, float*, float, float, float, float, float, int, int, int);

// Step kernels -- Q32.32 (64-bit fixed-point)
void lbm_step_q32_soa_kernel(const long long*, long long*, float*, float*, const float*, const float*, int, int, int);
void initialize_uniform_q32_soa_kernel(long long*, float*, float*, float*, float, float, float, float, float, int, int, int);

// Step kernels -- DD signature
void lbm_step_fused_dd_kernel(const double*, const double*, double*, double*, float*, float*, const float*, const float*, int, int, int);

// Init kernels -- 10-arg (no tau)
void initialize_uniform_soa_kernel(float*, float*, float*, float, float, float, float, int, int, int);
void initialize_uniform_bf16_kernel(void*, float*, float*, float, float, float, float, int, int, int);
void initialize_uniform_fp64_kernel(void*, float*, float*, float, float, float, float, int, int, int);

// Init kernels -- 12-arg (tau-extended)
void initialize_uniform_fp32_soa_cs_kernel(float*, float*, float*, float*, float, float, float, float, float, int, int, int);
void initialize_uniform_fp16_kernel(void*, float*, float*, float*, float, float, float, float, float, int, int, int);
void initialize_uniform_fp16_soa_kernel(void*, float*, float*, float*, float, float, float, float, float, int, int, int);
void initialize_uniform_fp16_soa_half2_kernel(void*, float*, float*, float*, float, float, float, float, float, int, int, int);
void initialize_uniform_bf16_soa_kernel(void*, float*, float*, float*, float, float, float, float, float, int, int, int);
void initialize_uniform_fp8_kernel(void*, float*, float*, float*, float, float, float, float, float, int, int, int);
void initialize_uniform_fp8_soa_kernel(void*, float*, float*, float*, float, float, float, float, float, int, int, int);
void initialize_uniform_fp8e5m2_kernel(void*, float*, float*, float*, float, float, float, float, float, int, int, int);
void initialize_uniform_fp8_e5m2_soa_kernel(void*, float*, float*, float*, float, float, float, float, float, int, int, int);
void initialize_uniform_int8_kernel(void*, float*, float*, float*, float, float, float, float, float, int, int, int);
void initialize_uniform_int8_soa_kernel(void*, float*, float*, float*, float, float, float, float, float, int, int, int);
void initialize_uniform_int16_kernel(void*, float*, float*, float*, float, float, float, float, float, int, int, int);
void initialize_uniform_int16_soa_kernel(void*, float*, float*, float*, float, float, float, float, float, int, int, int);
void initialize_uniform_fp64_soa_kernel(void*, float*, float*, float*, float, float, float, float, float, int, int, int);
void initialize_uniform_int4_kernel(void*, float*, float*, float*, float, float, float, float, float, int, int, int);
void initialize_uniform_fp4_kernel(void*, float*, float*, float*, float, float, float, float, float, int, int, int);

// Init kernels -- special signatures
void initialize_uniform_dd_kernel(double*, double*, float*, float*, float, float, float, float, int, int, int);
void initialize_uniform_bf16_soa_bf162_kernel(void*, void*, float*, float*, float*, float*, float, float, float, float, int, int, int);

} // extern "C"

// ============================================================================
// Launch helpers
// ============================================================================

// Standard 1D grid for non-tiled, non-coarsened kernels.
static inline dim3 lbm_grid_1d(int n_cells, int threads_per_block) {
    return dim3((n_cells + threads_per_block - 1) / threads_per_block);
}

// Coarsened grid: ceil(n_cells / cells_per_thread / threads_per_block).
static inline dim3 lbm_grid_coarsened(int n_cells, int cells_per_thread, int threads_per_block) {
    int effective = (n_cells + cells_per_thread - 1) / cells_per_thread;
    return dim3((effective + threads_per_block - 1) / threads_per_block);
}

// Tiled 3D grid for tiled kernels (8x8x4 tiles).
static inline dim3 lbm_grid_tiled(int nx, int ny, int nz) {
    return dim3((nx + 7) / 8, (ny + 7) / 8, (nz + 3) / 4);
}

// ============================================================================
// launch_lbm_step -- dispatch a single LBM step
// ============================================================================

// For ping-pong: f_in = buffers.f_a (or f_b), f_out = buffers.f_b (or f_a).
// For A-A: f = buffers.f_a, parity determines direction swap.
// Caller is responsible for alternating buffers between steps.

typedef void (*lbm_step_fn)(const void*, void*, float*, float*, const float*, const float*, int, int, int);
typedef void (*lbm_aa_fn)(void*, float*, float*, const float*, const float*, int, int, int, int);

int launch_lbm_step(
    LbmKernelVariant variant,
    const LBMGrid* grid,
    LBMBuffers* bufs,
    const void* f_in,
    void* f_out,
    int parity,  // Only used for A-A kernels
    cudaStream_t stream
) {
    const LbmKernelInfo* info = &LBM_KERNEL_INFO[variant];
    int n = grid->n_cells;
    int tpb = info->threads_per_block;
    dim3 block(tpb);
    dim3 grd;

    // Compute grid dimensions
    int is_tiled = (variant == LBM_FP32_SOA_TILED || variant == LBM_FP32_SOA_MRT_TILED);
    if (is_tiled) {
        grd = lbm_grid_tiled(grid->nx, grid->ny, grid->nz);
        // Tiled kernels use 3D thread blocks: TILE_X x TILE_Y x TILE_Z = 8x8x4
        block = dim3(8, 8, 4);
    } else if (info->cells_per_thread > 1) {
        grd = lbm_grid_coarsened(n, info->cells_per_thread, tpb);
    } else {
        grd = lbm_grid_1d(n, tpb);
    }

    // DD has a unique 4-buffer signature
    if (variant == LBM_DD_SOA) {
        // DD: (f_hi_in, f_lo_in, f_hi_out, f_lo_out, rho, u, force, tau, nx, ny, nz)
        void* args[] = {
            (void*)&bufs->f_a, (void*)&bufs->f_b,
            (void*)&bufs->f_c, (void*)&bufs->f_d,
            (void*)&bufs->rho, (void*)&bufs->u,
            (void*)&bufs->force, (void*)&bufs->tau,
            (void*)&grid->nx, (void*)&grid->ny, (void*)&grid->nz
        };
        // Note: cudaLaunchKernel requires the function pointer. Since all
        // kernels are extern "C", we must use the symbol directly. For DD:
        return cudaLaunchKernel((const void*)lbm_step_fused_dd_kernel,
                                grd, block, args, 0, stream);
    }

    // A-A kernels: single-buffer with parity
    if (info->is_aa) {
        void* args[] = {
            (void*)&bufs->f_a,
            (void*)&bufs->rho, (void*)&bufs->u,
            (void*)&bufs->tau, (void*)&bufs->force,
            (void*)&grid->nx, (void*)&grid->ny, (void*)&grid->nz,
            (void*)&parity
        };

        if (variant == LBM_FP32_SOA_AA) {
            return cudaLaunchKernel((const void*)lbm_step_soa_aa, grd, block, args, 0, stream);
        }
        if (variant == LBM_FP32_SOA_MRT_AA) {
            return cudaLaunchKernel((const void*)lbm_step_soa_mrt_aa, grd, block, args, 0, stream);
        }
        return cudaErrorInvalidValue;
    }

    // If inv_tau is precomputed, pass it in the tau slot.
    // Kernels read it directly as inv_tau (no reciprocal needed).
    // If inv_tau is NULL, pass raw tau (kernels compute 1/tau internally).
    const float* tau_or_inv = bufs->inv_tau ? bufs->inv_tau : bufs->tau;

    // AoS kernels use (force, tau); SoA kernels use (tau, force).
    // Build args conditionally based on layout from the info table.
    void* args_soa[] = {
        (void*)&f_in, (void*)&f_out,
        (void*)&bufs->rho, (void*)&bufs->u,
        (void*)&tau_or_inv, (void*)&bufs->force,
        (void*)&grid->nx, (void*)&grid->ny, (void*)&grid->nz
    };
    void* args_aos[] = {
        (void*)&f_in, (void*)&f_out,
        (void*)&bufs->rho, (void*)&bufs->u,
        (void*)&bufs->force, (void*)&tau_or_inv,
        (void*)&grid->nx, (void*)&grid->ny, (void*)&grid->nz
    };
    void** args = info->is_soa ? args_soa : args_aos;

    // Macro to reduce boilerplate for standard-signature kernels.
    // Each kernel is declared extern and launched via cudaLaunchKernel.
    #define DISPATCH_STEP(VARIANT, FUNC_NAME, ELEM_T)                         \
        case VARIANT:                                                         \
            return cudaLaunchKernel((const void*)FUNC_NAME,                   \
                                   grd, block, args, 0, stream);

    switch (variant) {
    // FP32 SoA
    DISPATCH_STEP(LBM_FP32_SOA_FUSED,         lbm_step_soa_fused,              float)
    DISPATCH_STEP(LBM_FP32_SOA_MRT_FUSED,     lbm_step_soa_mrt_fused,          float)
    DISPATCH_STEP(LBM_FP32_SOA_PULL,          lbm_step_soa_pull,               float)
    DISPATCH_STEP(LBM_FP32_SOA_MRT_PULL,      lbm_step_soa_mrt_pull,           float)
    DISPATCH_STEP(LBM_FP32_SOA_TILED,         lbm_step_soa_tiled,              float)
    DISPATCH_STEP(LBM_FP32_SOA_MRT_TILED,     lbm_step_soa_mrt_tiled,          float)
    DISPATCH_STEP(LBM_FP32_SOA_COARSENED,     lbm_step_soa_coarsened,          float)
    DISPATCH_STEP(LBM_FP32_SOA_MRT_COARSENED, lbm_step_soa_mrt_coarsened,      float)
    DISPATCH_STEP(LBM_FP32_SOA_COARSENED_F4,  lbm_step_soa_coarsened_float4,   float)
    DISPATCH_STEP(LBM_FP32_SOA_CS,            lbm_step_fp32_soa_cs_kernel,     float)

    // FP16
    DISPATCH_STEP(LBM_FP16_AOS,              lbm_step_fused_fp16_kernel,       void)
    DISPATCH_STEP(LBM_FP16_SOA,              lbm_step_fp16_soa_kernel,         void)
    DISPATCH_STEP(LBM_FP16_SOA_HALF2,        lbm_step_fp16_soa_half2_kernel,   void)

    // BF16
    DISPATCH_STEP(LBM_BF16_AOS,              lbm_step_fused_bf16_kernel,       void)
    DISPATCH_STEP(LBM_BF16_SOA,              lbm_step_bf16_soa_kernel,         void)
    DISPATCH_STEP(LBM_BF16_SOA_BF162,        lbm_step_bf16_soa_bf162_kernel,   void)

    // FP8
    DISPATCH_STEP(LBM_FP8_E4M3_AOS,          lbm_step_fused_fp8_kernel,        void)
    DISPATCH_STEP(LBM_FP8_E4M3_SOA,          lbm_step_fp8_soa_kernel,          void)
    DISPATCH_STEP(LBM_FP8_E5M2_AOS,          lbm_step_fused_fp8e5m2_kernel,    void)
    DISPATCH_STEP(LBM_FP8_E5M2_SOA,          lbm_step_fp8_e5m2_soa_kernel,     void)

    // INT8
    DISPATCH_STEP(LBM_INT8_AOS,              lbm_step_fused_int8_kernel,       void)
    DISPATCH_STEP(LBM_INT8_SOA,              lbm_step_int8_soa_kernel,         void)
    DISPATCH_STEP(LBM_INT8_SOA_COARSENED,   lbm_step_int8_soa_coarsened_kernel, signed char)
    DISPATCH_STEP(LBM_INT8_SOA_LLOYDMAX,   lbm_step_int8_soa_lloydmax_kernel, unsigned char)
    DISPATCH_STEP(LBM_INT8_SOA_LLOYDMAX_C4, lbm_step_int8_soa_lloydmax_c4_kernel, unsigned char)
    DISPATCH_STEP(LBM_INT8_SOA_LLOYDMAX_PIPE, lbm_step_int8_soa_lloydmax_pipe_kernel, unsigned char)
    DISPATCH_STEP(LBM_INT8_SOA_LLOYDMAX_HIOCC, lbm_step_int8_soa_lloydmax_hiocc_kernel, unsigned char)

    // INT16
    DISPATCH_STEP(LBM_INT16_AOS,             lbm_step_int16_kernel,            void)
    DISPATCH_STEP(LBM_INT16_SOA,             lbm_step_int16_soa_kernel,        void)

    // FP64
    DISPATCH_STEP(LBM_FP64_AOS,              lbm_step_fused_fp64_kernel,       void)
    DISPATCH_STEP(LBM_FP64_SOA,              lbm_step_fp64_soa_kernel,         void)

    // Q16.16 fixed-point
    DISPATCH_STEP(LBM_Q16_SOA,               lbm_step_q16_soa_kernel,          int)
    DISPATCH_STEP(LBM_Q16_SOA_LM,           lbm_step_q16_soa_lm_kernel,       int)

    // Q32.32 uses long long (8 bytes), same buffer layout as FP64
    DISPATCH_STEP(LBM_Q32_SOA,              lbm_step_q32_soa_kernel,          long long)

    // BW ceiling
    DISPATCH_STEP(LBM_INT4_SOA,              lbm_step_fused_int4_kernel,       void)
    DISPATCH_STEP(LBM_FP4_SOA,               lbm_step_fp4_kernel,              void)

    default:
        return cudaErrorInvalidValue;
    }
    #undef DISPATCH_STEP
}

// ============================================================================
// launch_lbm_init -- initialize distributions to equilibrium
// ============================================================================

int launch_lbm_init(
    LbmKernelVariant variant,
    const LBMGrid* grid,
    LBMBuffers* bufs,
    float rho_init,
    float ux_init, float uy_init, float uz_init,
    cudaStream_t stream
) {
    (void)&LBM_KERNEL_INFO[variant];
    int n = grid->n_cells;
    int tpb = 128;
    dim3 block(tpb);
    dim3 grd((n + tpb - 1) / tpb);

    // DD init has unique signature
    if (variant == LBM_DD_SOA) {
        void* args[] = {
            (void*)&bufs->f_a, (void*)&bufs->f_b,
            (void*)&bufs->rho, (void*)&bufs->u,
            &rho_init, &ux_init, &uy_init, &uz_init,
            (void*)&grid->nx, (void*)&grid->ny, (void*)&grid->nz
        };
        return cudaLaunchKernel((const void*)initialize_uniform_dd_kernel,
                                grd, block, args, 0, stream);
    }

    // Init kernels have three signatures:
    //
    // A) FP32 SoA base: (f, rho, u, rho_init, ux, uy, uz, nx, ny, nz) -- 10 args
    //    Used by: initialize_uniform_soa_kernel
    //
    // B) Tau-extended: (f, rho, u, tau, rho_init, ux, uy, uz, tau_val, nx, ny, nz) -- 12 args
    //    Used by: most reduced-precision init kernels (FP16, BF16 SoA, FP8, INT8, INT16, FP64 SoA, INT4, FP4, FP32 CS)
    //
    // C) Special: BF16 BF162 and INT8 MRT AA have unique signatures handled inline.

    float tau_val = 0.6f;  // Default tau for init

    // 10-arg: (f, rho, u, rho_init, ux, uy, uz, nx, ny, nz)
    void* args_10[] = {
        (void*)&bufs->f_a,
        (void*)&bufs->rho, (void*)&bufs->u,
        &rho_init, &ux_init, &uy_init, &uz_init,
        (void*)&grid->nx, (void*)&grid->ny, (void*)&grid->nz
    };

    // 12-arg: (f, rho, u, tau, rho_init, ux, uy, uz, tau_val, nx, ny, nz)
    void* args_12[] = {
        (void*)&bufs->f_a,
        (void*)&bufs->rho, (void*)&bufs->u,
        (void*)&bufs->tau,
        &rho_init, &ux_init, &uy_init, &uz_init,
        &tau_val,
        (void*)&grid->nx, (void*)&grid->ny, (void*)&grid->nz
    };

    #define DISPATCH_INIT_10(VARIANT, FUNC_NAME, ELEM_T)                     \
        case VARIANT:                                                        \
            return cudaLaunchKernel((const void*)FUNC_NAME,                  \
                                   grd, block, args_10, 0, stream);

    #define DISPATCH_INIT_12(VARIANT, FUNC_NAME, ELEM_T)                     \
        case VARIANT:                                                        \
            return cudaLaunchKernel((const void*)FUNC_NAME,                  \
                                   grd, block, args_12, 0, stream);

    switch (variant) {
    // FP32 SoA: 10-arg (no tau in init)
    DISPATCH_INIT_10(LBM_FP32_SOA_FUSED,         initialize_uniform_soa_kernel,         float)
    DISPATCH_INIT_10(LBM_FP32_SOA_MRT_FUSED,     initialize_uniform_soa_kernel,         float)
    DISPATCH_INIT_10(LBM_FP32_SOA_PULL,          initialize_uniform_soa_kernel,         float)
    DISPATCH_INIT_10(LBM_FP32_SOA_MRT_PULL,      initialize_uniform_soa_kernel,         float)
    DISPATCH_INIT_10(LBM_FP32_SOA_TILED,         initialize_uniform_soa_kernel,         float)
    DISPATCH_INIT_10(LBM_FP32_SOA_MRT_TILED,     initialize_uniform_soa_kernel,         float)
    DISPATCH_INIT_10(LBM_FP32_SOA_COARSENED,     initialize_uniform_soa_kernel,         float)
    DISPATCH_INIT_10(LBM_FP32_SOA_MRT_COARSENED, initialize_uniform_soa_kernel,         float)
    DISPATCH_INIT_10(LBM_FP32_SOA_COARSENED_F4,  initialize_uniform_soa_kernel,         float)
    DISPATCH_INIT_10(LBM_FP32_SOA_AA,            initialize_uniform_soa_kernel,         float)
    DISPATCH_INIT_10(LBM_FP32_SOA_MRT_AA,        initialize_uniform_soa_kernel,         float)

    // FP32 CS: 12-arg (tau-extended)
    DISPATCH_INIT_12(LBM_FP32_SOA_CS,            initialize_uniform_fp32_soa_cs_kernel, float)

    // FP16: 12-arg
    DISPATCH_INIT_12(LBM_FP16_AOS,              initialize_uniform_fp16_kernel,         void)
    DISPATCH_INIT_12(LBM_FP16_SOA,              initialize_uniform_fp16_soa_kernel,     void)
    DISPATCH_INIT_12(LBM_FP16_SOA_HALF2,        initialize_uniform_fp16_soa_half2_kernel, void)

    // BF16 AoS: 10-arg (no tau)
    DISPATCH_INIT_10(LBM_BF16_AOS,              initialize_uniform_bf16_kernel,         void)
    // BF16 SoA: 12-arg
    DISPATCH_INIT_12(LBM_BF16_SOA,              initialize_uniform_bf16_soa_kernel,     void)

    // BF16 SoA BF162: special signature (f_a, f_b, rho, u, tau, force, rho_init, ux, uy, uz, nx, ny, nz)
    case LBM_BF16_SOA_BF162: {
        void* args_bf162[] = {
            (void*)&bufs->f_a, (void*)&bufs->f_b,
            (void*)&bufs->rho, (void*)&bufs->u,
            (void*)&bufs->tau, (void*)&bufs->force,
            &rho_init, &ux_init, &uy_init, &uz_init,
            (void*)&grid->nx, (void*)&grid->ny, (void*)&grid->nz
        };
        return cudaLaunchKernel((const void*)initialize_uniform_bf16_soa_bf162_kernel,
                                grd, block, args_bf162, 0, stream);
    }

    // FP8: 12-arg
    DISPATCH_INIT_12(LBM_FP8_E4M3_AOS,          initialize_uniform_fp8_kernel,          void)
    DISPATCH_INIT_12(LBM_FP8_E4M3_SOA,          initialize_uniform_fp8_soa_kernel,      void)
    DISPATCH_INIT_12(LBM_FP8_E5M2_AOS,          initialize_uniform_fp8e5m2_kernel,      void)
    DISPATCH_INIT_12(LBM_FP8_E5M2_SOA,          initialize_uniform_fp8_e5m2_soa_kernel, void)

    // INT8: 12-arg
    DISPATCH_INIT_12(LBM_INT8_AOS,              initialize_uniform_int8_kernel,         void)
    DISPATCH_INIT_12(LBM_INT8_SOA,              initialize_uniform_int8_soa_kernel,     void)
    DISPATCH_INIT_12(LBM_INT8_SOA_COARSENED,   initialize_uniform_int8_soa_kernel,     void)
    DISPATCH_INIT_12(LBM_INT8_SOA_LLOYDMAX,   initialize_uniform_int8_soa_lloydmax_kernel, unsigned char)
    DISPATCH_INIT_12(LBM_INT8_SOA_LLOYDMAX_C4, initialize_uniform_int8_soa_lloydmax_kernel, unsigned char)
    DISPATCH_INIT_12(LBM_INT8_SOA_LLOYDMAX_PIPE, initialize_uniform_int8_soa_lloydmax_kernel, unsigned char)
    DISPATCH_INIT_12(LBM_INT8_SOA_LLOYDMAX_HIOCC, initialize_uniform_int8_soa_lloydmax_kernel, unsigned char)

    // INT16: 12-arg
    DISPATCH_INIT_12(LBM_INT16_AOS,             initialize_uniform_int16_kernel,        void)
    DISPATCH_INIT_12(LBM_INT16_SOA,             initialize_uniform_int16_soa_kernel,    void)

    // FP64 AoS: 10-arg (no tau)
    DISPATCH_INIT_10(LBM_FP64_AOS,              initialize_uniform_fp64_kernel,         void)
    // FP64 SoA: 12-arg
    DISPATCH_INIT_12(LBM_FP64_SOA,              initialize_uniform_fp64_soa_kernel,     void)
    DISPATCH_INIT_12(LBM_Q16_SOA,               initialize_uniform_q16_soa_kernel,      int)
    DISPATCH_INIT_12(LBM_Q16_SOA_LM,           initialize_uniform_q16_soa_lm_kernel,   int)
    DISPATCH_INIT_12(LBM_Q32_SOA,              initialize_uniform_q32_soa_kernel,      long long)

    // BW ceiling: 12-arg
    DISPATCH_INIT_12(LBM_INT4_SOA,              initialize_uniform_int4_kernel,         void)
    DISPATCH_INIT_12(LBM_FP4_SOA,               initialize_uniform_fp4_kernel,          void)

    default:
        return cudaErrorInvalidValue;
    }
    #undef DISPATCH_INIT_10
    #undef DISPATCH_INIT_12
}

// ============================================================================
// Occupancy queries
// ============================================================================

// Query occupancy for a single variant on the current device.
// Map variant -> kernel function pointer for occupancy queries.
// Returns NULL for variants without a step kernel (should not happen).
static const void* get_step_kernel_ptr(LbmKernelVariant variant) {
    switch (variant) {
    case LBM_FP32_SOA_FUSED:         return (const void*)lbm_step_soa_fused;
    case LBM_FP32_SOA_MRT_FUSED:     return (const void*)lbm_step_soa_mrt_fused;
    case LBM_FP32_SOA_PULL:          return (const void*)lbm_step_soa_pull;
    case LBM_FP32_SOA_MRT_PULL:      return (const void*)lbm_step_soa_mrt_pull;
    case LBM_FP32_SOA_TILED:         return (const void*)lbm_step_soa_tiled;
    case LBM_FP32_SOA_MRT_TILED:     return (const void*)lbm_step_soa_mrt_tiled;
    case LBM_FP32_SOA_COARSENED:     return (const void*)lbm_step_soa_coarsened;
    case LBM_FP32_SOA_MRT_COARSENED: return (const void*)lbm_step_soa_mrt_coarsened;
    case LBM_FP32_SOA_COARSENED_F4:  return (const void*)lbm_step_soa_coarsened_float4;
    case LBM_FP32_SOA_CS:            return (const void*)lbm_step_fp32_soa_cs_kernel;
    case LBM_FP32_SOA_AA:            return (const void*)lbm_step_soa_aa;
    case LBM_FP32_SOA_MRT_AA:        return (const void*)lbm_step_soa_mrt_aa;
    case LBM_FP16_AOS:               return (const void*)lbm_step_fused_fp16_kernel;
    case LBM_FP16_SOA:               return (const void*)lbm_step_fp16_soa_kernel;
    case LBM_FP16_SOA_HALF2:         return (const void*)lbm_step_fp16_soa_half2_kernel;
    case LBM_BF16_AOS:               return (const void*)lbm_step_fused_bf16_kernel;
    case LBM_BF16_SOA:               return (const void*)lbm_step_bf16_soa_kernel;
    case LBM_BF16_SOA_BF162:         return (const void*)lbm_step_bf16_soa_bf162_kernel;
    case LBM_FP8_E4M3_AOS:           return (const void*)lbm_step_fused_fp8_kernel;
    case LBM_FP8_E4M3_SOA:           return (const void*)lbm_step_fp8_soa_kernel;
    case LBM_FP8_E5M2_AOS:           return (const void*)lbm_step_fused_fp8e5m2_kernel;
    case LBM_FP8_E5M2_SOA:           return (const void*)lbm_step_fp8_e5m2_soa_kernel;
    case LBM_INT8_AOS:               return (const void*)lbm_step_fused_int8_kernel;
    case LBM_INT8_SOA:               return (const void*)lbm_step_int8_soa_kernel;
    case LBM_INT8_SOA_COARSENED:     return (const void*)lbm_step_int8_soa_coarsened_kernel;
    case LBM_INT8_SOA_LLOYDMAX:      return (const void*)lbm_step_int8_soa_lloydmax_kernel;
    case LBM_INT8_SOA_LLOYDMAX_C4:   return (const void*)lbm_step_int8_soa_lloydmax_c4_kernel;
    case LBM_INT8_SOA_LLOYDMAX_PIPE: return (const void*)lbm_step_int8_soa_lloydmax_pipe_kernel;
    case LBM_INT8_SOA_LLOYDMAX_HIOCC: return (const void*)lbm_step_int8_soa_lloydmax_hiocc_kernel;
    case LBM_INT16_AOS:              return (const void*)lbm_step_int16_kernel;
    case LBM_INT16_SOA:              return (const void*)lbm_step_int16_soa_kernel;
    case LBM_FP64_AOS:               return (const void*)lbm_step_fused_fp64_kernel;
    case LBM_FP64_SOA:               return (const void*)lbm_step_fp64_soa_kernel;
    case LBM_DD_SOA:                 return (const void*)lbm_step_fused_dd_kernel;
    case LBM_Q16_SOA:                return (const void*)lbm_step_q16_soa_kernel;
    case LBM_Q16_SOA_LM:            return (const void*)lbm_step_q16_soa_lm_kernel;
    case LBM_Q32_SOA:               return (const void*)lbm_step_q32_soa_kernel;
    case LBM_INT4_SOA:               return (const void*)lbm_step_fused_int4_kernel;
    case LBM_FP4_SOA:                return (const void*)lbm_step_fp4_kernel;
    default:                         return NULL;
    }
}

int query_occupancy(LbmKernelVariant variant, int device_id, OccupancyInfo* out) {
    const LbmKernelInfo* info = &LBM_KERNEL_INFO[variant];
    out->variant = variant;
    out->max_active_blocks = 0;
    out->max_warps = 0;
    out->occupancy_pct = 0.0f;

    const void* kernel_ptr = get_step_kernel_ptr(variant);
    if (!kernel_ptr) return -1;

    int block_size = info->threads_per_block;
    int num_blocks = 0;

    cudaError_t err = cudaOccupancyMaxActiveBlocksPerMultiprocessor(
        &num_blocks, kernel_ptr, block_size, 0);
    if (err != cudaSuccess) {
        // Fallback: estimate from register heuristic
        int warps_per_block = (block_size + 31) / 32;
        num_blocks = 48 / warps_per_block;  // max 48 warps/SM on Ada
        if (num_blocks < 1) num_blocks = 1;
    }

    // Query max warps per SM from device properties
    cudaDeviceProp prop;
    cudaGetDeviceProperties(&prop, device_id);
    int max_warps_per_sm = prop.maxThreadsPerMultiProcessor / 32;

    int warps_per_block = (block_size + 31) / 32;
    out->max_active_blocks = num_blocks;
    out->max_warps = num_blocks * warps_per_block;
    if (out->max_warps > max_warps_per_sm) out->max_warps = max_warps_per_sm;
    out->occupancy_pct = (float)out->max_warps / (float)max_warps_per_sm * 100.0f;

    return 0;
}

// Print occupancy table for all kernel variants.
void report_all_occupancy(int device_id) {
    printf("%-30s  %6s  %6s  %8s\n", "Kernel", "Blocks", "Warps", "Occ%%");
    printf("%-30s  %6s  %6s  %8s\n", "------", "------", "-----", "----");

    for (int v = 0; v < LBM_VARIANT_COUNT; v++) {
        OccupancyInfo occ;
        query_occupancy((LbmKernelVariant)v, device_id, &occ);
        printf("%-30s  %6d  %6d  %7.1f%%\n",
               LBM_KERNEL_INFO[v].name,
               occ.max_active_blocks,
               occ.max_warps,
               occ.occupancy_pct);
    }
}
