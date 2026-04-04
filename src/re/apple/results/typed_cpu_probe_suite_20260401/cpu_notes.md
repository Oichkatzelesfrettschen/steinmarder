# Next42 CPU Probe Draft

- families: integer add/sub, floating add/mul/FMA, load/store, shuffle, atomics, transcendentals
- widened types: int8, int16, int64, uint8, uint16, uint32, float32, bf16 proxy
- binary: sm_apple_cpu_latency
- expected artifacts: cpu_probe_families.txt, cpu_probe_inventory.txt, compile_matrix.txt, cpu_bins/*, cpu_runs/*.csv, disassembly/*.txt, llvm_mca/*.txt
