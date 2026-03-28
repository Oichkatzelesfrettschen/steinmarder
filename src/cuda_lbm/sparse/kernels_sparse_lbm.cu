// Sparse A-A D3Q19 LBM kernel for Ada Lovelace (SM 8.9)
//
// Optimized for high-sparsity domains using a Brick Map (Indirect Table).
// Solves 1024^3 in 12GB VRAM by eliminating the ping-pong buffer (A-A pattern)
// and allocating only active 8x8x8 bricks.

static __constant__ int CX[19] = {
    0, 1, -1, 0, 0, 0, 0, 1, -1, 1, -1, 1, -1, 1, -1, 0, 0, 0, 0
};
static __constant__ int CY[19] = {
    0, 0, 0, 1, -1, 0, 0, 1, -1, -1, 1, 0, 0, 0, 0, 1, -1, 1, -1
};
static __constant__ int CZ[19] = {
    0, 0, 0, 0, 0, 1, -1, 0, 0, 0, 0, 1, -1, -1, 1, 1, -1, -1, 1
};
static __constant__ int OPP[19] = {
    0,  2,  1,  4,  3,  6,  5,  8,  7, 10,
    9, 12, 11, 14, 13, 16, 15, 18, 17
};
static __constant__ float W[19] = {
    1.0f/3.0f,
    1.0f/18.0f, 1.0f/18.0f, 1.0f/18.0f,
    1.0f/18.0f, 1.0f/18.0f, 1.0f/18.0f,
    1.0f/36.0f, 1.0f/36.0f, 1.0f/36.0f,
    1.0f/36.0f, 1.0f/36.0f, 1.0f/36.0f,
    1.0f/36.0f, 1.0f/36.0f, 1.0f/36.0f,
    1.0f/36.0f, 1.0f/36.0f, 1.0f/36.0f
};

__device__ __forceinline__ bool finite_check(float x) {
    return (x == x) && (x <= 3.402823466e38f) && (x >= -3.402823466e38f);
}

// ---------------------------------------------------------------------------
// Sparse A-A Fused Kernel with Shared-Memory Neighbor Cache
// ---------------------------------------------------------------------------
// Each block processes one brick (512 cells = 8^3).
// Threads 0-26 pre-load the 3^3 neighbor stencil of brick pool indices
// into shared memory, eliminating up to 36 global indirect_table reads
// per boundary cell per time step.
//
// Grid: ceil(N_active_cells / 512)
// Block: 512 threads (1 block = 1 brick)
extern "C" __global__ void __launch_bounds__(512)
lbm_step_sparse_aa(
    float* __restrict__ f,                     // [19 * N_active_cells]
    float* __restrict__ rho_out,               // [N_active_cells]
    float* __restrict__ u_out,                 // [3 * N_active_cells]
    const float* __restrict__ tau,             // [N_active_cells]
    const float* __restrict__ force,           // [3 * N_active_cells]
    const int* __restrict__ indirect_table,    // [bx_max * by_max * bz_max]
    const unsigned int* __restrict__ active_brick_ids, // [N_active_bricks]
    int nx, int ny, int nz,
    int bx_max, int by_max, int bz_max,
    int active_cell_start,
    int active_cell_count,
    int n_active_cells_total,
    int parity
) {
    // --- Shared-memory neighbor cache: 27 ints = 108 bytes ---
    // Index encoding: (dx+1)*9 + (dy+1)*3 + (dz+1) for dx,dy,dz in {-1,0,+1}
    __shared__ int neighbor_pool[27];

    int local_tid = blockIdx.x * blockDim.x + threadIdx.x;
    int tid = active_cell_start + local_tid;

    int pool_idx = tid / 512;
    int local_idx = tid % 512;

    int global_brick_idx = (pool_idx < gridDim.x)
        ? active_brick_ids[pool_idx] : 0;
    int bx = global_brick_idx % bx_max;
    int by = (global_brick_idx / bx_max) % by_max;
    int bz = global_brick_idx / (bx_max * by_max);

    // Threads 0-26 load the 3x3x3 neighbor stencil
    if (threadIdx.x < 27) {
        int dz = (int)(threadIdx.x / 9) - 1;
        int dy = (int)((threadIdx.x / 3) % 3) - 1;
        int dx = (int)(threadIdx.x % 3) - 1;
        int nbx = (bx + dx + bx_max) % bx_max;
        int nby = (by + dy + by_max) % by_max;
        int nbz = (bz + dz + bz_max) % bz_max;
        neighbor_pool[threadIdx.x] = indirect_table[nbx + bx_max * (nby + by_max * nbz)];
    }
    __syncthreads();

    if (local_tid >= active_cell_count) return;

    int lx = local_idx % 8;
    int ly = (local_idx / 8) % 8;
    int lz = local_idx / 64;

    // 1. Read distributions (with A-A pull logic and bounce-back)
    float rho_local = 0.0f;
    float mx = 0.0f, my = 0.0f, mz = 0.0f;
    float f_local[19];

    #pragma unroll
    for (int i = 0; i < 19; i++) {
        int read_dir, src_tid;
        if (parity == 0) {
            // Even step: read canonical slot
            read_dir = i;
            src_tid = tid;
        } else {
            // Odd step: pull from neighbor's opposite slot
            int lxn = lx - CX[i];
            int lyn = ly - CY[i];
            int lzn = lz - CZ[i];

            // Determine brick offset: 0 if local coord stays in [0,7], else +/-1
            int dbx = (lxn < 0) ? -1 : (lxn >= 8) ? 1 : 0;
            int dby = (lyn < 0) ? -1 : (lyn >= 8) ? 1 : 0;
            int dbz = (lzn < 0) ? -1 : (lzn >= 8) ? 1 : 0;

            // Wrap local coordinates to [0,7]
            lxn = (lxn + 8) & 7;
            lyn = (lyn + 8) & 7;
            lzn = (lzn + 8) & 7;

            // Lookup neighbor brick from shared memory
            int stencil_idx = (dbz + 1) * 9 + (dby + 1) * 3 + (dbx + 1);
            int n_pool_idx = neighbor_pool[stencil_idx];

            if (n_pool_idx != -1) {
                read_dir = OPP[i];
                src_tid = n_pool_idx * 512 + lxn + 8 * (lyn + 8 * lzn);
            } else {
                // Bounce-back: neighbor is solid.
                read_dir = i;
                src_tid = tid;
            }
        }
        
        float fi = __ldg(&f[read_dir * n_active_cells_total + src_tid]);
        if (!finite_check(fi)) fi = 0.0f;
        f_local[i] = fi;
        rho_local += fi;
        mx += CX[i] * fi;
        my += CY[i] * fi;
        mz += CZ[i] * fi;
    }

    // 2. Macroscopic quantities
    float ux = 0.0f, uy = 0.0f, uz = 0.0f;
    if (finite_check(rho_local) && rho_local > 1.0e-20f) {
        float inv_rho = 1.0f / rho_local;
        ux = mx * inv_rho;
        uy = my * inv_rho;
        uz = mz * inv_rho;
    } else {
        rho_local = 1.0f;
    }

    rho_out[tid] = rho_local;
    u_out[tid]                        = ux;
    u_out[n_active_cells_total + tid]       = uy;
    u_out[2 * n_active_cells_total + tid]   = uz;

    // 3. BGK collision
    float tau_local = __ldg(&tau[tid]);
    float inv_tau = 1.0f / tau_local;
    float u_sq = ux * ux + uy * uy + uz * uz;
    float base = fmaf(-1.5f, u_sq, 1.0f);

    #pragma unroll
    for (int i = 0; i < 19; i++) {
        float eu = fmaf((float)CX[i], ux, fmaf((float)CY[i], uy, (float)CZ[i] * uz));
        float w_rho = W[i] * rho_local;
        float f_eq = w_rho * fmaf(fmaf(eu, 4.5f, 3.0f), eu, base);
        f_local[i] -= (f_local[i] - f_eq) * inv_tau;
    }

    // 4. Guo forcing
    float fx = __ldg(&force[tid]);
    float fy = __ldg(&force[n_active_cells_total + tid]);
    float fz = __ldg(&force[2 * n_active_cells_total + tid]);
    float force_mag_sq = fx * fx + fy * fy + fz * fz;

    if (force_mag_sq >= 1e-40f) {
        float prefactor = 1.0f - 0.5f * inv_tau;
        float u_dot_f = ux * fx + uy * fy + uz * fz;
        #pragma unroll
        for (int i = 0; i < 19; i++) {
            float eix = (float)CX[i], eiy = (float)CY[i], eiz = (float)CZ[i];
            float ei_dot_f   = eix * fx + eiy * fy + eiz * fz;
            float ei_dot_u   = eix * ux + eiy * uy + eiz * uz;
            float em_u_dot_f = ei_dot_f - u_dot_f;
            f_local[i] += prefactor * W[i] * (em_u_dot_f * 3.0f + ei_dot_u * ei_dot_f * 9.0f);
        }
    }

    // 5. Write (with A-A push logic and bounce-back, using SMEM neighbor cache)
    #pragma unroll
    for (int i = 0; i < 19; i++) {
        if (parity == 0) {
            // Even step: push to neighbor's opposite slot
            int lxn_w = lx + CX[i];
            int lyn_w = ly + CY[i];
            int lzn_w = lz + CZ[i];

            int dbx_w = (lxn_w < 0) ? -1 : (lxn_w >= 8) ? 1 : 0;
            int dby_w = (lyn_w < 0) ? -1 : (lyn_w >= 8) ? 1 : 0;
            int dbz_w = (lzn_w < 0) ? -1 : (lzn_w >= 8) ? 1 : 0;

            lxn_w = (lxn_w + 8) & 7;
            lyn_w = (lyn_w + 8) & 7;
            lzn_w = (lzn_w + 8) & 7;

            int stencil_idx_w = (dbz_w + 1) * 9 + (dby_w + 1) * 3 + (dbx_w + 1);
            int n_pool_idx = neighbor_pool[stencil_idx_w];

            if (n_pool_idx != -1) {
                int dst_tid = n_pool_idx * 512 + lxn_w + 8 * (lyn_w + 8 * lzn_w);
                f[OPP[i] * n_active_cells_total + dst_tid] = f_local[i];
            } else {
                // Bounce-back: neighbor is solid. Write to own OPP[i] slot.
                f[OPP[i] * n_active_cells_total + tid] = f_local[i];
            }
        } else {
            // Odd step: write to canonical slot at self
            f[i * n_active_cells_total + tid] = f_local[i];
        }
    }
}
