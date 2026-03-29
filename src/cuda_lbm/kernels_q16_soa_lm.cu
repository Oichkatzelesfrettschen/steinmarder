// Q16.16 Adaptive-Range Fixed-Point LBM kernel (Lloyd-Max principle).
//
// Combines the IADD3 integer pipeline advantage (0.53 cy vs FFMA 4.53 cy)
// with TurboQuant-inspired range adaptation: concentrates all 65536 levels
// in the D3Q19 occupied range [-0.05, 0.45] instead of the full Q16.16
// range [-32768, 32767].
//
// Effective resolution: 65536 levels / 0.50 range = 131072 levels/unit
// vs standard Q16.16: 65536 levels/unit (2x improvement)
// vs FP32: ~16M levels/unit in [0, 0.5] (still coarser, but momentum is exact)
//
// The key insight: integer accumulation with offset+scale preserves the
// IADD3 pipeline benefit. The rho sum is still an integer add (exact).
// Only the final float conversion applies the inverse scale.

#define QLM_RANGE_MIN  (-0.05f)
#define QLM_RANGE_MAX  (0.45f)
#define QLM_RANGE      (QLM_RANGE_MAX - QLM_RANGE_MIN)  // 0.50
#define QLM_SCALE      (65535.0f / QLM_RANGE)           // 131070
#define QLM_INV_SCALE  (QLM_RANGE / 65535.0f)           // 7.629e-6

// Encode: map float to unsigned 16-bit within the adapted range,
// then store as int32 (upper 16 bits unused, giving Q0.16 in adapted range).
// Using unsigned short stored in int avoids sign extension issues.
#define QLM_ENCODE(v)  (__float2int_rn(fminf(fmaxf((v) - QLM_RANGE_MIN, 0.0f) * QLM_SCALE, 65535.0f)))
#define QLM_DECODE(q)  ((float)(q) * QLM_INV_SCALE + QLM_RANGE_MIN)

__constant__ int CX_QLM[19] = {
    0, 1, -1, 0, 0, 0, 0, 1, -1, 1, -1, 1, -1, 1, -1, 0, 0, 0, 0
};
__constant__ int CY_QLM[19] = {
    0, 0, 0, 1, -1, 0, 0, 1, -1, -1, 1, 0, 0, 0, 0, 1, -1, 1, -1
};
__constant__ int CZ_QLM[19] = {
    0, 0, 0, 0, 0, 1, -1, 0, 0, 0, 0, 1, -1, -1, 1, 1, -1, -1, 1
};
__constant__ float W_QLM[19] = {
    1.0f/3.0f,
    1.0f/18.0f, 1.0f/18.0f, 1.0f/18.0f,
    1.0f/18.0f, 1.0f/18.0f, 1.0f/18.0f,
    1.0f/36.0f, 1.0f/36.0f, 1.0f/36.0f,
    1.0f/36.0f, 1.0f/36.0f, 1.0f/36.0f,
    1.0f/36.0f, 1.0f/36.0f, 1.0f/36.0f,
    1.0f/36.0f, 1.0f/36.0f, 1.0f/36.0f
};

__device__ __forceinline__ bool finite_qlm(float x) {
    return (x == x) && (x <= 3.402823466e38f) && (x >= -3.402823466e38f);
}

extern "C" __launch_bounds__(128, 5) __global__ void lbm_step_q16_soa_lm_kernel(
    const int* __restrict__ f_in,
    int* __restrict__ f_out,
    float* __restrict__ rho_out,
    float* __restrict__ u_out,
    const float* __restrict__ tau,
    const float* __restrict__ force,
    int nx, int ny, int nz
) {
    int idx = blockIdx.x * blockDim.x + threadIdx.x;
    int n_cells = nx * ny * nz;
    if (idx >= n_cells) return;

    int x = idx % nx;
    int y = (idx / nx) % ny;
    int z = idx / (nx * ny);

    // Integer accumulation for density and momentum (IADD3 pipeline).
    // rho_q = sum of all 19 quantized values. Since each is in [0, 65535],
    // rho_q fits in int32 (max: 19 * 65535 = 1,245,165).
    int q_local[19];
    int rho_q = 0;
    int mx_q = 0, my_q = 0, mz_q = 0;

    #pragma unroll
    for (int i = 0; i < 19; i++) {
        int xb = (x - CX_QLM[i] + nx) % nx;
        int yb = (y - CY_QLM[i] + ny) % ny;
        int zb = (z - CZ_QLM[i] + nz) % nz;
        int idx_back = xb + nx * (yb + ny * zb);
        int q = __ldg(&f_in[(long long)i * n_cells + idx_back]);
        q_local[i] = q;
        rho_q += q;
        mx_q += CX_QLM[i] * q;
        my_q += CY_QLM[i] * q;
        mz_q += CZ_QLM[i] * q;
    }

    // Decode: rho = sum(f_i) = sum(q_i * INV_SCALE + RANGE_MIN)
    //       = rho_q * INV_SCALE + 19 * RANGE_MIN
    float rho_local = (float)rho_q * QLM_INV_SCALE + 19.0f * QLM_RANGE_MIN;
    float mx = (float)mx_q * QLM_INV_SCALE;  // momentum: offset cancels (sum of CX * offset = 0)
    float my = (float)my_q * QLM_INV_SCALE;
    float mz = (float)mz_q * QLM_INV_SCALE;

    float ux = 0.0f, uy = 0.0f, uz = 0.0f;
    if (finite_qlm(rho_local) && rho_local > 1.0e-20f) {
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

    // FP32 collision
    float f_local[19];
    #pragma unroll
    for (int i = 0; i < 19; i++) {
        f_local[i] = QLM_DECODE(q_local[i]);
    }

    float tau_local = __ldg(&tau[idx]);
    float inv_tau   = 1.0f / tau_local;
    float u_sq      = ux * ux + uy * uy + uz * uz;
    float base      = fmaf(-1.5f, u_sq, 1.0f);

    #pragma unroll
    for (int i = 0; i < 19; i++) {
        float eu    = fmaf((float)CX_QLM[i], ux, fmaf((float)CY_QLM[i], uy, (float)CZ_QLM[i] * uz));
        float w_rho = W_QLM[i] * rho_local;
        float f_eq  = w_rho * fmaf(fmaf(eu, 4.5f, 3.0f), eu, base);
        f_local[i] -= (f_local[i] - f_eq) * inv_tau;
    }

    float fx = __ldg(&force[idx]);
    float fy = __ldg(&force[n_cells + idx]);
    float fz = __ldg(&force[2 * n_cells + idx]);
    float force_mag_sq = fx * fx + fy * fy + fz * fz;

    if (force_mag_sq >= 1e-40f) {
        float prefactor = 1.0f - 0.5f * inv_tau;
        float u_dot_f = ux * fx + uy * fy + uz * fz;
        #pragma unroll
        for (int i = 0; i < 19; i++) {
            float eix = (float)CX_QLM[i], eiy = (float)CY_QLM[i], eiz = (float)CZ_QLM[i];
            float ei_dot_f   = eix * fx + eiy * fy + eiz * fz;
            float ei_dot_u   = eix * ux + eiy * uy + eiz * uz;
            float em_u_dot_f = ei_dot_f - u_dot_f;
            f_local[i] += prefactor * W_QLM[i] * fmaf(ei_dot_u * ei_dot_f, 9.0f, em_u_dot_f * 3.0f);
        }
    }

    #pragma unroll
    for (int i = 0; i < 19; i++) {
        f_out[(long long)i * n_cells + idx] = QLM_ENCODE(f_local[i]);
    }
}

extern "C" __global__ void initialize_uniform_q16_soa_lm_kernel(
    int* f,
    float* rho_out, float* u_out, float* tau_arr,
    float rho_init, float ux_init, float uy_init, float uz_init,
    float tau_val, int nx, int ny, int nz
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
        float eu   = (float)CX_QLM[i]*ux_init + (float)CY_QLM[i]*uy_init + (float)CZ_QLM[i]*uz_init;
        float f_eq = W_QLM[i] * rho_init * fmaf(fmaf(eu, 4.5f, 3.0f), eu, base);
        f[(long long)i * n_cells + idx] = QLM_ENCODE(f_eq);
    }
}
