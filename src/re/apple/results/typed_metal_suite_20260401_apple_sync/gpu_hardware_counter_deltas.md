# GPU Hardware Counter Deltas

- source: `/Users/eirikr/Documents/GitHub/steinmarder/src/apple_re/results/typed_metal_suite_20260401_apple_sync/gpu_hardware_counters.csv`
- baseline_variant: `baseline`
- compared_variants: 4

## occupancy_heavy vs baseline

| Counter | Avg delta | Avg ratio | P99 delta | Peak delta |
|---------|-----------|-----------|-----------|------------|
| ALU Utilization | 0.365 | 1.941 | 3.976 | 0.475 |
| Compute Occupancy | 0.104 | 5.000 | 0.252 | 91.757 |
| F16 Utilization | 0.018 | 1.168 | 2.311 | 0.000 |
| F32 Utilization | 0.021 | 1.300 | 1.063 | 15.027 |
| Fragment Occupancy | 0.166 | 1.260 | 8.721 | 0.000 |
| GPU Read Bandwidth | 0.078 | 1.497 | 5.542 | 16.512 |
| GPU Write Bandwidth | 0.022 | 1.175 | 0.969 | 12.918 |
| LLC Utilization | 0.060 | 1.274 | 2.161 | 8.239 |
| Top Performance Limiter | 0.390 | 1.554 | 8.445 | 0.000 |
| Total Occupancy | 0.532 | 1.666 | 22.654 | -0.008 |
## register_pressure vs baseline

| Counter | Avg delta | Avg ratio | P99 delta | Peak delta |
|---------|-----------|-----------|-----------|------------|
| ALU Utilization | 0.539 | 2.389 | 12.926 | 1.072 |
| Compute Occupancy | 0.069 | 3.654 | 0.239 | 1.301 |
| F16 Utilization | 0.019 | 1.178 | 4.001 | -6.434 |
| F32 Utilization | 0.115 | 2.643 | 2.576 | 6.175 |
| Fragment Occupancy | 0.362 | 1.567 | 22.667 | 0.000 |
| GPU Read Bandwidth | 0.013 | 1.083 | 2.642 | -6.535 |
| GPU Write Bandwidth | 0.017 | 1.135 | 0.573 | 0.214 |
| LLC Utilization | 0.058 | 1.265 | 2.365 | 1.012 |
| Top Performance Limiter | 0.636 | 1.903 | 19.877 | 0.000 |
| Total Occupancy | 0.607 | 1.760 | 28.482 | 0.007 |
## threadgroup_heavy vs baseline

| Counter | Avg delta | Avg ratio | P99 delta | Peak delta |
|---------|-----------|-----------|-----------|------------|
| ALU Utilization | 0.126 | 1.325 | 8.308 | -0.217 |
| Compute Occupancy | 0.141 | 6.423 | 0.346 | 0.264 |
| F16 Utilization | 0.019 | 1.178 | 1.391 | -0.007 |
| F32 Utilization | 0.011 | 1.157 | 1.382 | -0.268 |
| Fragment Occupancy | 0.223 | 1.349 | 13.560 | 0.000 |
| GPU Read Bandwidth | 0.092 | 1.586 | 4.481 | 5.895 |
| GPU Write Bandwidth | 0.058 | 1.460 | 2.266 | 7.963 |
| LLC Utilization | 0.135 | 1.616 | 2.965 | 9.542 |
| Top Performance Limiter | 0.228 | 1.324 | 11.361 | 0.000 |
| Total Occupancy | 0.608 | 1.761 | 20.558 | 0.049 |
## threadgroup_minimal vs baseline

| Counter | Avg delta | Avg ratio | P99 delta | Peak delta |
|---------|-----------|-----------|-----------|------------|
| ALU Utilization | 0.004 | 1.010 | -6.132 | -0.249 |
| Compute Occupancy | 0.048 | 2.846 | 0.205 | 0.031 |
| F16 Utilization | -0.051 | 0.523 | -1.323 | -0.090 |
| F32 Utilization | -0.038 | 0.457 | -1.222 | -0.583 |
| Fragment Occupancy | -0.343 | 0.463 | -19.726 | -1.514 |
| GPU Read Bandwidth | -0.046 | 0.707 | -1.467 | -0.213 |
| GPU Write Bandwidth | -0.029 | 0.770 | -1.342 | 0.569 |
| LLC Utilization | -0.039 | 0.822 | -3.717 | 0.729 |
| Top Performance Limiter | -0.211 | 0.700 | -18.623 | -0.757 |
| Total Occupancy | -0.315 | 0.606 | -21.687 | -1.515 |
