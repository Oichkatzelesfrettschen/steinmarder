#include <metal_stdlib>
using namespace metal;

// C3: AGX L2 cache dep-chain latency via pointer chasing.
//
// Method: buffer(2) is a uint32 array initialized by the host as a
// Hamiltonian pointer-chase with stride 2053:
//   chase[i] = (i + 2053) % n_entries    where n_entries = 65536 (256KB)
//
// Properties:
//   - stride 2053 is coprime to 65536, so the chain visits all 65536 entries
//     before repeating — a full Hamiltonian cycle over the 256KB buffer
//   - step size = 2053 × 4 bytes = 8212 bytes > L1 size (8192 bytes = 8KB)
//   - working set = 65536 × 4 = 262144 bytes = 256KB < L2 (768KB/core)
//   - each sequential access is GUARANTEED to miss L1 (step > L1 size)
//   - guaranteed to HIT L2 (working set < L2)
//
// Dep chain: next_addr = chase[addr] & (N-1)
// Each load depends on the PREVIOUS load's result — pure serialized latency.
//
// Address masking: since N = 65536 = 2^16, mask = 0xFFFF.
//
// ops_per_iter = 8 (8 serial loads per outer iteration)
//
// Expected result: if L2 latency = 24–50 cycles (~18–38 ns), the measurement
// will reveal the first cache level ABOVE L1 (10 ns, ~13 cyc L1-hit latency).

kernel void probe_l2_lat(
    device float* out [[buffer(0)]],
    constant uint& ignored_iters [[buffer(1)]],
    device uint* chase [[buffer(2)]],
    uint gid [[thread_position_in_grid]])
{
    // Start each thread at a different entry to avoid all threads hitting the
    // same cache line. Offset by 1024 entries (4KB) per thread, well-distributed.
    uint32_t addr = (gid * 1024u) & 0xFFFFu;

    for (uint outer_iter = 0u; outer_iter < 100000u; outer_iter++) {
        #pragma clang fp reassociate(off)
        {
            // 8 serial pointer-chase loads per iteration.
            // Each load's address comes from the previous load's value.
            addr = chase[addr] & 0xFFFFu;
            addr = chase[addr] & 0xFFFFu;
            addr = chase[addr] & 0xFFFFu;
            addr = chase[addr] & 0xFFFFu;
            addr = chase[addr] & 0xFFFFu;
            addr = chase[addr] & 0xFFFFu;
            addr = chase[addr] & 0xFFFFu;
            addr = chase[addr] & 0xFFFFu;
        }
    }

    // Write addr to output to prevent the loop from being optimized away.
    out[gid] = float(addr);
}
