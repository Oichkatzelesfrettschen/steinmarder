// Q16.16 Fixed-Point i-major SoA D3Q19 LBM kernel.
//
// Exploits SASS RE finding: IADD3 latency = 0.53 cycles on SM 8.9,
// vs FFMA = 4.53 cycles (8.5x ratio). By storing distributions as
// Q16.16 fixed-point (32-bit int, 16 fractional bits), the momentum
// accumulation phase uses pure integer arithmetic on the fast IADD3
// pipeline instead of FP32 FMA.
//
// Q16.16 encoding:
//   fixed = (int)(float_val * 65536.0f)
//   float = (float)fixed / 65536.0f
//   Range: [-32768.0, 32767.99998] with 1/65536 = 0.0000153 resolution
//
// For D3Q19 at Ma <= 0.3:
//   f_i in [-0.05, 0.45] -> fixed in [-3277, 29491]
//   Resolution: 0.0000153 (vs INT8's 0.00196 = 128x finer)
//   Same 4 bytes/element as FP32, but momentum accumulation is 8.5x faster.
//
// The collision phase promotes to FP32 for the equilibrium computation
// (which needs fmaf precision), then converts back to Q16.16 for storage.
// This is the storage-compute split pattern: Q16.16 storage, FP32 compute.
//
// Bandwidth model: 46 scalars * 4 bytes = 184 bytes/cell/step (same as FP32).
// VRAM at 128^3: 19 * 2M * 4 * 2 (ping+pong) = ~304 MB (same as FP32).
// Performance advantage comes from the 8.5x faster momentum accumulation,
// not from reduced memory footprint.

#define Q16_FRAC_BITS 16
#define Q16_SCALE     65536.0f
#define Q16_INV_SCALE (1.0f / 65536.0f)

__constant__ int CX_Q16[19] = {
    0, 1, -1, 0, 0, 0, 0, 1, -1, 1, -1, 1, -1, 1, -1, 0, 0, 0, 0
};
__constant__ int CY_Q16[19] = {
    0, 0, 0, 1, -1, 0, 0, 1, -1, -1, 1, 0, 0, 0, 0, 1, -1, 1, -1
};
__constant__ int CZ_Q16[19] = {
    0, 0, 0, 0, 0, 1, -1, 0, 0, 0, 0, 1, -1, -1, 1, 1, -1, -1, 1
};
__constant__ float W_Q16[19] = {
    1.0f/3.0f,
    1.0f/18.0f, 1.0f/18.0f, 1.0f/18.0f,
    1.0f/18.0f, 1.0f/18.0f, 1.0f/18.0f,
    1.0f/36.0f, 1.0f/36.0f, 1.0f/36.0f,
    1.0f/36.0f, 1.0f/36.0f, 1.0f/36.0f,
    1.0f/36.0f, 1.0f/36.0f, 1.0f/36.0f,
    1.0f/36.0f, 1.0f/36.0f, 1.0f/36.0f
};

__device__ __forceinline__ bool finite_q16(float x) {
    return (x == x) && (x <= 3.402823466e38f) && (x >= -3.402823466e38f);
}

__device__ __forceinline__ int float_to_q16(float v) {
    if (!finite_q16(v)) return 0;
    return __float2int_rn(v * Q16_SCALE);
}

__device__ __forceinline__ float q16_to_float(int q) {
    return (float)q * Q16_INV_SCALE;
}

// Q16.16 SoA fused collision + pull-streaming.
// Momentum uses integer accumulation (IADD3 pipeline, 0.53 cy/op).
// Collision uses FP32 promotion (FFMA pipeline, 4.53 cy/op).
extern "C" __launch_bounds__(128, 4) __global__ void lbm_step_q16_soa_kernel(
    const int* __restrict__ f_in,    // [19 * n_cells] Q16.16, i-major SoA
    int* __restrict__ f_out,         // [19 * n_cells] Q16.16, i-major SoA
    float* __restrict__ rho_out,
    float* __restrict__ u_out,       // [3 * n_cells] SoA
    const float* __restrict__ tau,
    const float* __restrict__ force,  // [3 * n_cells] SoA
    int nx, int ny, int nz
) {
    int idx = blockIdx.x * blockDim.x + threadIdx.x;
    int n_cells = nx * ny * nz;
    if (idx >= n_cells) return;

    int x = idx % nx;
    int y = (idx / nx) % ny;
    int z = idx / (nx * ny);

    // Phase 1: Pull-scheme load + integer momentum accumulation.
    // All 19 loads + 57 integer adds (3 per direction) use IADD3 at 0.53 cy each.
    int q_local[19];
    int rho_q = 0;          // Q16.16 density sum
    int mx_q = 0, my_q = 0, mz_q = 0;  // Q16.16 momentum

    #pragma unroll
    for (int i = 0; i < 19; i++) {
        int xb = (x - CX_Q16[i] + nx) % nx;
        int yb = (y - CY_Q16[i] + ny) % ny;
        int zb = (z - CZ_Q16[i] + nz) % nz;
        int idx_back = xb + nx * (yb + ny * zb);
        int q = __ldg(&f_in[(long long)i * n_cells + idx_back]);
        q_local[i] = q;
        rho_q += q;                       // IADD3: 0.53 cy
        mx_q += CX_Q16[i] * q;           // IADD3 (or IMAD): still integer
        my_q += CY_Q16[i] * q;
        mz_q += CZ_Q16[i] * q;
    }

    // Phase 2: Promote to FP32 for macroscopic quantities and collision.
    float rho_local = q16_to_float(rho_q);
    float mx = q16_to_float(mx_q);
    float my = q16_to_float(my_q);
    float mz = q16_to_float(mz_q);

    float ux = 0.0f, uy = 0.0f, uz = 0.0f;
    if (finite_q16(rho_local) && rho_local > 1.0e-20f) {
        float inv_rho = 1.0f / rho_local;
        ux = mx * inv_rho;
        uy = my * inv_rho;
        uz = mz * inv_rho;
    } else {
        rho_local = 1.0f;
    }

    rho_out[idx] = rho_local;
    u_out[idx]               = ux;
    u_out[n_cells + idx]     = uy;
    u_out[2 * n_cells + idx] = uz;

    // Phase 3: FP32 collision (same as all other kernels).
    float f_local[19];
    #pragma unroll
    for (int i = 0; i < 19; i++) {
        f_local[i] = q16_to_float(q_local[i]);
    }

    float tau_local = __ldg(&tau[idx]);
    float inv_tau   = 1.0f / tau_local;
    float u_sq      = ux * ux + uy * uy + uz * uz;
    float base      = fmaf(-1.5f, u_sq, 1.0f);

    #pragma unroll
    for (int i = 0; i < 19; i++) {
        float eu    = fmaf((float)CX_Q16[i], ux, fmaf((float)CY_Q16[i], uy, (float)CZ_Q16[i] * uz));
        float w_rho = W_Q16[i] * rho_local;
        float f_eq  = w_rho * fmaf(fmaf(eu, 4.5f, 3.0f), eu, base);
        f_local[i] -= (f_local[i] - f_eq) * inv_tau;
    }

    // Phase 4: Guo forcing (FP32, same pattern as other kernels).
    float fx = __ldg(&force[idx]);
    float fy = __ldg(&force[n_cells + idx]);
    float fz = __ldg(&force[2 * n_cells + idx]);
    float force_mag_sq = fx * fx + fy * fy + fz * fz;

    if (force_mag_sq >= 1e-40f) {
        float prefactor = 1.0f - 0.5f * inv_tau;
        float u_dot_f = ux * fx + uy * fy + uz * fz;
        #pragma unroll
        for (int i = 0; i < 19; i++) {
            float eix = (float)CX_Q16[i], eiy = (float)CY_Q16[i], eiz = (float)CZ_Q16[i];
            float ei_dot_f   = eix * fx + eiy * fy + eiz * fz;
            float ei_dot_u   = eix * ux + eiy * uy + eiz * uz;
            float em_u_dot_f = ei_dot_f - u_dot_f;
            f_local[i] += prefactor * W_Q16[i] * fmaf(ei_dot_u * ei_dot_f, 9.0f, em_u_dot_f * 3.0f);
        }
    }

    // Phase 5: Convert back to Q16.16 and write (coalesced SoA).
    #pragma unroll
    for (int i = 0; i < 19; i++) {
        f_out[(long long)i * n_cells + idx] = float_to_q16(f_local[i]);
    }
}

// Q16.16 initialization kernel.
extern "C" __global__ void initialize_uniform_q16_soa_kernel(
    int* f,
    float* rho_out,
    float* u_out,
    float* tau_arr,
    float rho_init,
    float ux_init,
    float uy_init,
    float uz_init,
    float tau_val,
    int nx, int ny, int nz
) {
    int idx = blockIdx.x * blockDim.x + threadIdx.x;
    int n_cells = nx * ny * nz;
    if (idx >= n_cells) return;

    rho_out[idx] = rho_init;
    u_out[idx]               = ux_init;
    u_out[n_cells + idx]     = uy_init;
    u_out[2 * n_cells + idx] = uz_init;
    tau_arr[idx] = tau_val;

    float u_sq = ux_init*ux_init + uy_init*uy_init + uz_init*uz_init;
    float base = fmaf(-1.5f, u_sq, 1.0f);

    #pragma unroll
    for (int i = 0; i < 19; i++) {
        float eu   = (float)CX_Q16[i]*ux_init + (float)CY_Q16[i]*uy_init + (float)CZ_Q16[i]*uz_init;
        float f_eq = W_Q16[i] * rho_init * fmaf(fmaf(eu, 4.5f, 3.0f), eu, base);
        f[(long long)i * n_cells + idx] = float_to_q16(f_eq);
    }
}
