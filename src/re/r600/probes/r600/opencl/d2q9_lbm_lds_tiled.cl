/*
 * d2q9_lbm_lds_tiled.cl — D2Q9 Lattice Boltzmann Method with SoA-in-LDS tiling.
 *
 * A real-world compute workload that stresses the full TeraScale-2 pipeline:
 *   - 9 distribution functions per cell (high register pressure, ~36 GPRs)
 *   - Streaming step requires neighbor reads (global memory bandwidth)
 *   - Collision step is pure ALU (MULADD chains, transcendental-free)
 *   - LDS tiling amortizes global memory latency
 *
 * D2Q9 velocities:
 *   e[0] = ( 0, 0)   w[0] = 4/9
 *   e[1] = ( 1, 0)   w[1] = 1/9
 *   e[2] = ( 0, 1)   w[2] = 1/9
 *   e[3] = (-1, 0)   w[3] = 1/9
 *   e[4] = ( 0,-1)   w[4] = 1/9
 *   e[5] = ( 1, 1)   w[5] = 1/36
 *   e[6] = (-1, 1)   w[6] = 1/36
 *   e[7] = (-1,-1)   w[7] = 1/36
 *   e[8] = ( 1,-1)   w[8] = 1/36
 *
 * Memory layout: Structure-of-Arrays (SoA) — f[q][y][x] for each distribution.
 * LDS tile: each work-group loads a (TILE_W+2) x (TILE_H+2) halo region
 * into 9 LDS arrays, computes collision + streaming on the interior, and
 * writes back the interior tile.
 *
 * This kernel tests:
 *   - Global bandwidth (9 SoA arrays streamed with halo)
 *   - LDS capacity (9 * (TILE_W+2) * (TILE_H+2) * 4 bytes)
 *   - ALU throughput (BGK collision operator: 9 MULADD chains)
 *   - Register pressure (~36 GPRs for 9 distributions + macroscopic vars)
 *
 * Build: RUSTICL_ENABLE=r600 R600_DEBUG=cs clang -cl-std=CL1.2
 */

#define TILE_W 14
#define TILE_H 14
/* LDS: 9 * 16 * 16 * 4 = 9216 bytes (well within 32 KB LDS per SIMD) */
#define HALO_W (TILE_W + 2)
#define HALO_H (TILE_H + 2)

/* D2Q9 weights */
#define W0  (4.0f / 9.0f)
#define W1  (1.0f / 9.0f)
#define W5  (1.0f / 36.0f)

/* BGK relaxation parameter (omega = 1/tau) */
#define OMEGA 1.7f

/* SoA index: f[q][y * NX + x] */
inline uint soa_idx(uint q, uint x, uint y, uint nx, uint ny) {
    return q * nx * ny + y * nx + x;
}

__kernel void d2q9_lbm_step(
    __global const float *f_in,   /* 9 * NX * NY input distributions */
    __global float *f_out,         /* 9 * NX * NY output distributions */
    const uint nx,
    const uint ny
)
{
    /* LDS tiles for 9 distributions with halo */
    __local float lds_f[9][HALO_H][HALO_W];

    const uint lx = get_local_id(0);   /* 0..TILE_W-1 */
    const uint ly = get_local_id(1);   /* 0..TILE_H-1 */
    const uint gx = get_group_id(0) * TILE_W + lx;
    const uint gy = get_group_id(1) * TILE_H + ly;

    /* ---- Phase 1: Load halo from global into LDS ---- */
    /* Each work-item loads the interior point + contributes to halo.
     * Interior maps to LDS coords (lx+1, ly+1). */

    /* Clamp global coords for halo reads */
    uint halo_gx = (gx > 0) ? gx - 1 : 0;
    uint halo_gy = (gy > 0) ? gy - 1 : 0;

    /* Load the 2x2 block that this work-item is responsible for */
    /* Each work-item loads its (lx+1, ly+1) interior point */
    for (uint q = 0; q < 9; q++) {
        lds_f[q][ly + 1][lx + 1] = (gx < nx && gy < ny) ?
            f_in[soa_idx(q, gx, gy, nx, ny)] : 0.0f;
    }

    /* Load left halo column (lx == 0 loads the left neighbor) */
    if (lx == 0) {
        for (uint q = 0; q < 9; q++) {
            lds_f[q][ly + 1][0] = (halo_gx < nx && gy < ny) ?
                f_in[soa_idx(q, halo_gx, gy, nx, ny)] : 0.0f;
        }
    }
    /* Load right halo column */
    if (lx == TILE_W - 1) {
        uint rgx = min(gx + 1, nx - 1);
        for (uint q = 0; q < 9; q++) {
            lds_f[q][ly + 1][TILE_W + 1] = (rgx < nx && gy < ny) ?
                f_in[soa_idx(q, rgx, gy, nx, ny)] : 0.0f;
        }
    }
    /* Load top halo row */
    if (ly == 0) {
        for (uint q = 0; q < 9; q++) {
            lds_f[q][0][lx + 1] = (gx < nx && halo_gy < ny) ?
                f_in[soa_idx(q, gx, halo_gy, nx, ny)] : 0.0f;
        }
    }
    /* Load bottom halo row */
    if (ly == TILE_H - 1) {
        uint bgy = min(gy + 1, ny - 1);
        for (uint q = 0; q < 9; q++) {
            lds_f[q][TILE_H + 1][lx + 1] = (gx < nx && bgy < ny) ?
                f_in[soa_idx(q, gx, bgy, nx, ny)] : 0.0f;
        }
    }
    /* Load corner halos */
    if (lx == 0 && ly == 0) {
        for (uint q = 0; q < 9; q++) {
            lds_f[q][0][0] = (halo_gx < nx && halo_gy < ny) ?
                f_in[soa_idx(q, halo_gx, halo_gy, nx, ny)] : 0.0f;
        }
    }
    if (lx == TILE_W - 1 && ly == 0) {
        uint rgx = min(gx + 1, nx - 1);
        for (uint q = 0; q < 9; q++) {
            lds_f[q][0][TILE_W + 1] = (rgx < nx && halo_gy < ny) ?
                f_in[soa_idx(q, rgx, halo_gy, nx, ny)] : 0.0f;
        }
    }
    if (lx == 0 && ly == TILE_H - 1) {
        uint bgy = min(gy + 1, ny - 1);
        for (uint q = 0; q < 9; q++) {
            lds_f[q][TILE_H + 1][0] = (halo_gx < nx && bgy < ny) ?
                f_in[soa_idx(q, halo_gx, bgy, nx, ny)] : 0.0f;
        }
    }
    if (lx == TILE_W - 1 && ly == TILE_H - 1) {
        uint rgx = min(gx + 1, nx - 1);
        uint bgy = min(gy + 1, ny - 1);
        for (uint q = 0; q < 9; q++) {
            lds_f[q][TILE_H + 1][TILE_W + 1] = (rgx < nx && bgy < ny) ?
                f_in[soa_idx(q, rgx, bgy, nx, ny)] : 0.0f;
        }
    }

    barrier(CLK_LOCAL_MEM_FENCE);

    /* ---- Phase 2: Stream + Collide from LDS ---- */
    if (gx >= nx || gy >= ny) return;

    /* Streaming: pull from neighbor cells in LDS (opposite direction) */
    float f0 = lds_f[0][ly + 1][lx + 1];       /* ( 0, 0) */
    float f1 = lds_f[1][ly + 1][lx    ];        /* (-1, 0) → pull from left */
    float f2 = lds_f[2][ly    ][lx + 1];        /* ( 0,-1) → pull from above */
    float f3 = lds_f[3][ly + 1][lx + 2];        /* (+1, 0) → pull from right */
    float f4 = lds_f[4][ly + 2][lx + 1];        /* ( 0,+1) → pull from below */
    float f5 = lds_f[5][ly    ][lx    ];         /* (-1,-1) → pull from top-left */
    float f6 = lds_f[6][ly    ][lx + 2];         /* (+1,-1) → pull from top-right */
    float f7 = lds_f[7][ly + 2][lx + 2];         /* (+1,+1) → pull from bottom-right */
    float f8 = lds_f[8][ly + 2][lx    ];         /* (-1,+1) → pull from bottom-left */

    /* Macroscopic quantities */
    float rho = f0 + f1 + f2 + f3 + f4 + f5 + f6 + f7 + f8;
    float inv_rho = 1.0f / rho;
    float ux = (f1 - f3 + f5 - f6 - f7 + f8) * inv_rho;
    float uy = (f2 - f4 + f5 + f6 - f7 - f8) * inv_rho;

    /* BGK collision: f_out[q] = f[q] - omega * (f[q] - f_eq[q]) */
    float usq = 1.5f * (ux * ux + uy * uy);

    /* Equilibrium distributions and collision */
    float cu, feq;

    /* q=0: e=(0,0) */
    feq = W0 * rho * (1.0f - usq);
    f0 = f0 - OMEGA * (f0 - feq);

    /* q=1: e=(1,0) */
    cu = 3.0f * ux;
    feq = W1 * rho * (1.0f + cu + 0.5f * cu * cu - usq);
    f1 = f1 - OMEGA * (f1 - feq);

    /* q=2: e=(0,1) */
    cu = 3.0f * uy;
    feq = W1 * rho * (1.0f + cu + 0.5f * cu * cu - usq);
    f2 = f2 - OMEGA * (f2 - feq);

    /* q=3: e=(-1,0) */
    cu = -3.0f * ux;
    feq = W1 * rho * (1.0f + cu + 0.5f * cu * cu - usq);
    f3 = f3 - OMEGA * (f3 - feq);

    /* q=4: e=(0,-1) */
    cu = -3.0f * uy;
    feq = W1 * rho * (1.0f + cu + 0.5f * cu * cu - usq);
    f4 = f4 - OMEGA * (f4 - feq);

    /* q=5: e=(1,1) */
    cu = 3.0f * (ux + uy);
    feq = W5 * rho * (1.0f + cu + 0.5f * cu * cu - usq);
    f5 = f5 - OMEGA * (f5 - feq);

    /* q=6: e=(-1,1) */
    cu = 3.0f * (-ux + uy);
    feq = W5 * rho * (1.0f + cu + 0.5f * cu * cu - usq);
    f6 = f6 - OMEGA * (f6 - feq);

    /* q=7: e=(-1,-1) */
    cu = 3.0f * (-ux - uy);
    feq = W5 * rho * (1.0f + cu + 0.5f * cu * cu - usq);
    f7 = f7 - OMEGA * (f7 - feq);

    /* q=8: e=(1,-1) */
    cu = 3.0f * (ux - uy);
    feq = W5 * rho * (1.0f + cu + 0.5f * cu * cu - usq);
    f8 = f8 - OMEGA * (f8 - feq);

    /* ---- Phase 3: Write back to global ---- */
    f_out[soa_idx(0, gx, gy, nx, ny)] = f0;
    f_out[soa_idx(1, gx, gy, nx, ny)] = f1;
    f_out[soa_idx(2, gx, gy, nx, ny)] = f2;
    f_out[soa_idx(3, gx, gy, nx, ny)] = f3;
    f_out[soa_idx(4, gx, gy, nx, ny)] = f4;
    f_out[soa_idx(5, gx, gy, nx, ny)] = f5;
    f_out[soa_idx(6, gx, gy, nx, ny)] = f6;
    f_out[soa_idx(7, gx, gy, nx, ny)] = f7;
    f_out[soa_idx(8, gx, gy, nx, ny)] = f8;
}
