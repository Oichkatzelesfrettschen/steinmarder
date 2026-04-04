# Vulkan Compute Probes

These probes carry the CUDA/SASS dependent-chain and throughput methodology
into the Terakan Vulkan lane.

They are intentionally simple, single-purpose kernels:

- dependent chains for latency and scoreboard behavior
- independent accumulators for throughput and bundle packing
- typed integer sweeps for signed, unsigned, and 24-bit arithmetic
- packed-byte versus regular-lane compare kernels
- pressure and shared-memory variants once the compute lane is stable

The current Vulkan runtime on x130e already enumerates a Terakan device, but
presentation still blocks on DRI3 and `VK_KHR_display`. This directory is for
compute-path probing, so it stays useful even while WSI is incomplete.
