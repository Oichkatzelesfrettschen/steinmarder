# OpenGL Compute Probes

This lane is deliberately provisional.

Older r600-era stacks may not expose OpenGL compute at all, but keeping the
probe family in-tree prevents us from repeatedly rediscovering that boundary.
If the capability never materializes on this hardware, these kernels still
serve as portable source templates for future Gallium- or Zink-backed testing.
