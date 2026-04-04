# OpenCL Probes

These kernels are the OpenCL mirror of the Terakan probe families. They are
useful even if the primary Vulkan path is still being stabilized, because
OpenCL often exposes arithmetic and memory behavior more directly on older
hardware.

The starter set mirrors the current latency and throughput families:

- dependent add and multiply chains
- vectorized MAD throughput
- explicit 24-bit integer chains via `mul24` / `mad24`
- regular `ivec4` / `uvec4` throughput against packed `i8x4` / `u8x4`
- later expansions for local-memory, image, and mixed packing experiments
