// INT8 SoA Lloyd-Max kernel with software-pipelined load/compute overlap.
//
// Root cause: ncu shows 44.1% of warp cycles stalled on L1TEX scoreboard
// (LDG latency = 92 cycles from SASS RE). At 33.3% occupancy, only 57
// cycles of overlap are available from other warps -- 35 cycles short.
//
// Solution: software-pipeline the load and compute phases. Each thread
// processes 2 cells with staggered execution:
//   1. Issue 19 __ldg() for cell A (fire-and-forget)
//   2. Issue 19 __ldg() for cell B
//   3. Consume cell A data (arrived during step 2) -> collision -> write
//   4. Consume cell B data (arrived during step 3) -> collision -> write
//
// The collision phase has ~700 FMA instructions, providing 700 cycles of
// overlap for the 92-cycle LDG latency. This should eliminate the 44.1%
// stall entirely.

#define LM_RANGE_MIN_P  (-0.05f)
#define LM_RANGE_MAX_P  0.45f
#define LM_SCALE_VAL_P  (255.0f / (LM_RANGE_MAX_P - LM_RANGE_MIN_P))
#define LM_INV_SCALE_P  ((LM_RANGE_MAX_P - LM_RANGE_MIN_P) / 255.0f)
#define LM_OFFSET_P     LM_RANGE_MIN_P

__constant__ int CX_LP[19] = {
    0, 1, -1, 0, 0, 0, 0, 1, -1, 1, -1, 1, -1, 1, -1, 0, 0, 0, 0
};
__constant__ int CY_LP[19] = {
    0, 0, 0, 1, -1, 0, 0, 1, -1, -1, 1, 0, 0, 0, 0, 1, -1, 1, -1
};
__constant__ int CZ_LP[19] = {
    0, 0, 0, 0, 0, 1, -1, 0, 0, 0, 0, 1, -1, -1, 1, 1, -1, -1, 1
};
__constant__ float W_LP[19] = {
    1.0f/3.0f,
    1.0f/18.0f, 1.0f/18.0f, 1.0f/18.0f,
    1.0f/18.0f, 1.0f/18.0f, 1.0f/18.0f,
    1.0f/36.0f, 1.0f/36.0f, 1.0f/36.0f,
    1.0f/36.0f, 1.0f/36.0f, 1.0f/36.0f,
    1.0f/36.0f, 1.0f/36.0f, 1.0f/36.0f,
    1.0f/36.0f, 1.0f/36.0f, 1.0f/36.0f
};

__device__ __forceinline__ bool finite_lp(float x) {
    return (x == x) && (x <= 3.402823466e38f) && (x >= -3.402823466e38f);
}

__device__ __forceinline__ unsigned char lm_encode_p(float v) {
    if (!finite_lp(v)) return 0;
    float s = (v - LM_OFFSET_P) * LM_SCALE_VAL_P;
    if (s < 0.0f)   s = 0.0f;
    if (s > 255.0f)  s = 255.0f;
    return (unsigned char)__float2int_rn(s);
}

__device__ __forceinline__ float lm_decode_p(unsigned char idx) {
    return (float)idx * LM_INV_SCALE_P + LM_OFFSET_P;
}

// Prefetched auxiliary data for a cell (tau + force).
// Loaded during the prefetch phase to overlap with prior cell's compute.
typedef struct {
    float tau_val;
    float fx, fy, fz;
} CellAux;

// Prefetch tau and force for a cell (fire-and-forget, data arrives later).
__device__ __forceinline__ void prefetch_aux(
    int idx, int n_cells,
    const float* __restrict__ tau,
    const float* __restrict__ force,
    CellAux* out
) {
    out->tau_val = __ldg(&tau[idx]);
    out->fx      = __ldg(&force[idx]);
    out->fy      = __ldg(&force[n_cells + idx]);
    out->fz      = __ldg(&force[2 * n_cells + idx]);
}

// Process one cell: given pre-loaded raw bytes AND pre-loaded aux data.
__device__ __forceinline__ void process_cell(
    int idx, int n_cells,
    const unsigned char raw[19],
    const CellAux* aux,
    unsigned char* __restrict__ f_out,
    float* __restrict__ rho_out,
    float* __restrict__ u_out
) {
    float f_local[19];
    float rho_local = 0.0f;
    float mx = 0.0f, my = 0.0f, mz = 0.0f;

    #pragma unroll
    for (int i = 0; i < 19; i++) {
        float fv = lm_decode_p(raw[i]);
        f_local[i] = fv;
        rho_local += fv;
        mx += (float)CX_LP[i] * fv;
        my += (float)CY_LP[i] * fv;
        mz += (float)CZ_LP[i] * fv;
    }

    float ux = 0.0f, uy = 0.0f, uz = 0.0f;
    if (finite_lp(rho_local) && rho_local > 1.0e-20f) {
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

    float inv_tau = 1.0f / aux->tau_val;
    float u_sq    = ux * ux + uy * uy + uz * uz;
    float base    = fmaf(-1.5f, u_sq, 1.0f);

    #pragma unroll
    for (int i = 0; i < 19; i++) {
        float eu    = fmaf((float)CX_LP[i], ux, fmaf((float)CY_LP[i], uy, (float)CZ_LP[i] * uz));
        float w_rho = W_LP[i] * rho_local;
        float f_eq  = w_rho * fmaf(fmaf(eu, 4.5f, 3.0f), eu, base);
        f_local[i] -= (f_local[i] - f_eq) * inv_tau;
    }

    float fx = aux->fx, fy = aux->fy, fz = aux->fz;
    float force_mag_sq = fx * fx + fy * fy + fz * fz;

    if (force_mag_sq >= 1e-40f) {
        float prefactor = 1.0f - 0.5f * inv_tau;
        float u_dot_f = ux * fx + uy * fy + uz * fz;
        #pragma unroll
        for (int i = 0; i < 19; i++) {
            float eix = (float)CX_LP[i], eiy = (float)CY_LP[i], eiz = (float)CZ_LP[i];
            float ei_dot_f   = eix * fx + eiy * fy + eiz * fz;
            float ei_dot_u   = eix * ux + eiy * uy + eiz * uz;
            float em_u_dot_f = ei_dot_f - u_dot_f;
            f_local[i] += prefactor * W_LP[i] * fmaf(ei_dot_u * ei_dot_f, 9.0f, em_u_dot_f * 3.0f);
        }
    }

    #pragma unroll
    for (int i = 0; i < 19; i++) {
        f_out[(long long)i * n_cells + idx] = lm_encode_p(f_local[i]);
    }
}

// Issue 19 pull-scheme loads for a cell. Returns raw bytes in out[19].
// The loads are fire-and-forget -- they will be consumed later by process_cell().
__device__ __forceinline__ void prefetch_cell(
    int idx, int n_cells, int nx, int ny, int nz,
    const unsigned char* __restrict__ f_in,
    unsigned char out[19]
) {
    int x = idx % nx;
    int y = (idx / nx) % ny;
    int z = idx / (nx * ny);

    #pragma unroll
    for (int i = 0; i < 19; i++) {
        int xb = (x - CX_LP[i] + nx) % nx;
        int yb = (y - CY_LP[i] + ny) % ny;
        int zb = (z - CZ_LP[i] + nz) % nz;
        int idx_back = xb + nx * (yb + ny * zb);
        out[i] = __ldg(&f_in[(long long)i * n_cells + idx_back]);
    }
}

extern "C" __launch_bounds__(128, 4) __global__ void lbm_step_int8_soa_lloydmax_pipe_kernel(
    const unsigned char* __restrict__ f_in,
    unsigned char* __restrict__ f_out,
    float* __restrict__ rho_out,
    float* __restrict__ u_out,
    const float* __restrict__ tau,
    const float* __restrict__ force,
    int nx, int ny, int nz
) {
    int n_cells = nx * ny * nz;
    // Each thread processes 2 consecutive cells with pipelined load/compute.
    int base_idx = (blockIdx.x * blockDim.x + threadIdx.x) * 2;
    int idx_a = base_idx;
    int idx_b = base_idx + 1;

    if (idx_a >= n_cells) return;

    // Stage 1: Prefetch f_in + tau + force for cell A
    unsigned char raw_a[19];
    CellAux aux_a;
    prefetch_cell(idx_a, n_cells, nx, ny, nz, f_in, raw_a);
    prefetch_aux(idx_a, n_cells, tau, force, &aux_a);

    // Stage 2: Prefetch f_in + tau + force for cell B (cell A loads in flight)
    unsigned char raw_b[19];
    CellAux aux_b;
    int has_b = (idx_b < n_cells);
    if (has_b) {
        prefetch_cell(idx_b, n_cells, nx, ny, nz, f_in, raw_b);
        prefetch_aux(idx_b, n_cells, tau, force, &aux_b);
    }

    // Stage 3: Process cell A (all loads arrived during stage 2)
    process_cell(idx_a, n_cells, raw_a, &aux_a, f_out, rho_out, u_out);

    // Stage 4: Process cell B (all loads arrived during stage 3's 700+ FMA instructions)
    if (has_b) {
        process_cell(idx_b, n_cells, raw_b, &aux_b, f_out, rho_out, u_out);
    }
}
