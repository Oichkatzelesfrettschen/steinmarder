# TeraScale External References

This file records external constraints that shape the x130e plan. Prefer
primary sources and official docs.

## Current External Anchors

- Mesa Zink documentation:
  <https://docs.mesa3d.org/drivers/zink.html>
  - Zink’s exposed GL level depends on both Zink implementation and the Vulkan
    driver capability floor.
  - Mesa ships a `zink_check_requirements` path explicitly for requirement
    checking, which reinforces that Zink readiness should be gated rather than
    assumed.
- DXVK driver support documentation:
  <https://github.com/doitsujin/dxvk/wiki/Driver-support>
  - current DXVK expects a Vulkan 1.3-capable driver for supported modern
    releases
  - legacy DXVK versions can run on Vulkan 1.1, but that is a fallback, not a
    strategic goal for this project
- Mesa NIR unit-testing documentation:
  <https://docs.mesa3d.org/nir/unit-testing.html>
  - NIR changes are expected to be testable and Mesa already provides a unit
    test path; custom TeraScale intrinsics and lowering work should use it
- Mesa NIR overview:
  <https://docs.mesa3d.org/nir/index.html>
  - NIR is a shared optimizing compiler stack, so x130e fixes in this layer are
    cross-stack leverage points rather than local hacks

## Local Companion

The narrative conclusions here should always agree with:

- [`TERASCALE_CAPABILITY_LEDGER.md`](TERASCALE_CAPABILITY_LEDGER.md)
- [`tables/table_x130e_capability_matrix.csv`](tables/table_x130e_capability_matrix.csv)
- [`tables/table_x130e_build_blockers.csv`](tables/table_x130e_build_blockers.csv)
