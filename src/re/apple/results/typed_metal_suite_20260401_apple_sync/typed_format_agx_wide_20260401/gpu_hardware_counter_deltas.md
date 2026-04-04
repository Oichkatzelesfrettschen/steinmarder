# GPU Hardware Counter Deltas

- source: `/Users/eirikr/Documents/GitHub/steinmarder/src/apple_re/results/typed_metal_suite_20260401_apple_sync/typed_format_agx_wide_20260401/gpu_hardware_counters.csv`
- baseline_variant: `float32`
- compared_variants: 9

## fp4 vs float32

| Counter | Avg delta | Avg ratio | P99 delta | Peak delta |
|---------|-----------|-----------|-----------|------------|
| ALU Utilization | -0.183 | 0.747 | -15.366 | 1.499 |
| Compute Occupancy | 0.047 | 2.119 | -0.164 | 2.204 |
| F16 Utilization | -0.101 | 0.466 | -5.699 | 0.168 |
| F32 Utilization | -0.066 | 0.609 | -3.138 | 15.912 |
| Fragment Occupancy | -0.555 | 0.572 | -13.394 | 0.892 |
| GPU Read Bandwidth | -0.133 | 0.655 | -3.719 | -5.248 |
| GPU Write Bandwidth | -0.115 | 0.639 | -3.149 | 23.836 |
| LLC Utilization | -0.157 | 0.663 | -4.736 | -2.107 |
| Top Performance Limiter | -0.393 | 0.693 | -11.373 | 2.253 |
| Total Occupancy | -0.588 | 0.644 | -10.050 | 78.290 |
## int16 vs float32

| Counter | Avg delta | Avg ratio | P99 delta | Peak delta |
|---------|-----------|-----------|-----------|------------|
| ALU Utilization | 4.986 | 7.906 | 51.741 | 13.662 |
| Compute Occupancy | 0.157 | 4.738 | 3.735 | 96.551 |
| F16 Utilization | 1.378 | 8.291 | 16.821 | 26.602 |
| F32 Utilization | 1.358 | 9.036 | 21.481 | 29.601 |
| Fragment Occupancy | 7.031 | 6.421 | 56.754 | 0.892 |
| GPU Read Bandwidth | 1.121 | 3.904 | 16.778 | 10.375 |
| GPU Write Bandwidth | 0.750 | 3.351 | 10.317 | 31.426 |
| LLC Utilization | 0.823 | 2.766 | 4.471 | -3.335 |
| Top Performance Limiter | 7.324 | 6.722 | 63.885 | 2.253 |
| Total Occupancy | 10.765 | 7.516 | 51.644 | 18.075 |
## int32 vs float32

| Counter | Avg delta | Avg ratio | P99 delta | Peak delta |
|---------|-----------|-----------|-----------|------------|
| ALU Utilization | -0.003 | 0.996 | 0.692 | -1.074 |
| Compute Occupancy | -0.010 | 0.762 | -0.029 | -0.188 |
| F16 Utilization | 0.007 | 1.037 | 0.153 | -0.301 |
| F32 Utilization | -0.016 | 0.905 | 0.071 | 0.006 |
| Fragment Occupancy | 0.053 | 1.041 | 2.621 | 0.892 |
| GPU Read Bandwidth | -0.023 | 0.940 | -1.187 | -5.826 |
| GPU Write Bandwidth | -0.028 | 0.912 | -0.833 | 14.306 |
| LLC Utilization | -0.030 | 0.936 | -0.846 | -0.860 |
| Top Performance Limiter | 0.025 | 1.020 | 1.622 | 2.049 |
| Total Occupancy | 0.236 | 1.143 | 13.470 | 0.940 |
## int4 vs float32

| Counter | Avg delta | Avg ratio | P99 delta | Peak delta |
|---------|-----------|-----------|-----------|------------|
| ALU Utilization | -0.365 | 0.494 | -14.358 | 1.310 |
| Compute Occupancy | -0.010 | 0.762 | -0.331 | 2.500 |
| F16 Utilization | -0.096 | 0.492 | -5.877 | 0.302 |
| F32 Utilization | -0.104 | 0.385 | -3.201 | 16.351 |
| Fragment Occupancy | -0.534 | 0.588 | -9.386 | 0.892 |
| GPU Read Bandwidth | -0.160 | 0.585 | -6.438 | -7.325 |
| GPU Write Bandwidth | -0.110 | 0.655 | -2.491 | 30.131 |
| LLC Utilization | -0.175 | 0.624 | -5.868 | -2.950 |
| Top Performance Limiter | -0.512 | 0.600 | -7.946 | 2.253 |
| Total Occupancy | -0.625 | 0.622 | -3.698 | 1.161 |
## int64 vs float32

| Counter | Avg delta | Avg ratio | P99 delta | Peak delta |
|---------|-----------|-----------|-----------|------------|
| ALU Utilization | -0.027 | 0.963 | 0.597 | 0.314 |
| Compute Occupancy | -0.003 | 0.929 | -0.020 | -0.035 |
| F16 Utilization | -0.001 | 0.995 | 0.156 | 0.085 |
| F32 Utilization | -0.026 | 0.846 | -0.103 | 0.006 |
| Fragment Occupancy | -0.283 | 0.782 | -6.445 | 0.892 |
| GPU Read Bandwidth | -0.115 | 0.702 | -5.482 | -5.982 |
| GPU Write Bandwidth | -0.072 | 0.774 | -2.356 | 16.945 |
| LLC Utilization | -0.088 | 0.811 | -5.197 | -3.841 |
| Top Performance Limiter | -0.237 | 0.815 | -3.549 | 2.210 |
| Total Occupancy | -0.404 | 0.755 | -11.895 | 0.935 |
## uint16 vs float32

| Counter | Avg delta | Avg ratio | P99 delta | Peak delta |
|---------|-----------|-----------|-----------|------------|
| ALU Utilization | 0.044 | 1.061 | 1.135 | -3.266 |
| Compute Occupancy | -0.001 | 0.976 | 0.028 | 0.267 |
| F16 Utilization | 0.032 | 1.169 | 0.141 | 0.594 |
| F32 Utilization | 0.012 | 1.071 | 0.171 | 0.006 |
| Fragment Occupancy | -0.017 | 0.987 | -4.049 | 0.892 |
| GPU Read Bandwidth | -0.089 | 0.769 | -4.837 | -11.297 |
| GPU Write Bandwidth | -0.051 | 0.840 | -1.778 | 18.300 |
| LLC Utilization | -0.055 | 0.882 | -3.557 | -5.954 |
| Top Performance Limiter | -0.062 | 0.952 | -3.086 | 2.253 |
| Total Occupancy | 0.161 | 1.097 | -2.620 | 0.958 |
## uint32 vs float32

| Counter | Avg delta | Avg ratio | P99 delta | Peak delta |
|---------|-----------|-----------|-----------|------------|
| ALU Utilization | -0.355 | 0.508 | -14.201 | -1.997 |
| Compute Occupancy | 0.005 | 1.119 | -0.304 | 56.870 |
| F16 Utilization | -0.096 | 0.492 | -5.701 | 0.851 |
| F32 Utilization | -0.102 | 0.396 | -3.449 | 15.931 |
| Fragment Occupancy | -0.502 | 0.613 | -5.151 | 0.892 |
| GPU Read Bandwidth | -0.145 | 0.624 | -4.836 | -7.255 |
| GPU Write Bandwidth | -0.090 | 0.718 | -1.919 | 0.592 |
| LLC Utilization | -0.152 | 0.674 | -4.626 | -0.589 |
| Top Performance Limiter | -0.509 | 0.602 | -6.743 | 2.253 |
| Total Occupancy | -0.643 | 0.611 | -2.641 | 0.892 |
## uint4 vs float32

| Counter | Avg delta | Avg ratio | P99 delta | Peak delta |
|---------|-----------|-----------|-----------|------------|
| ALU Utilization | -0.144 | 0.801 | -6.038 | 1.424 |
| Compute Occupancy | 0.001 | 1.024 | -0.296 | 2.014 |
| F16 Utilization | -0.032 | 0.831 | -3.631 | 0.289 |
| F32 Utilization | -0.065 | 0.615 | -1.808 | 16.305 |
| Fragment Occupancy | -0.116 | 0.911 | 15.343 | 0.892 |
| GPU Read Bandwidth | -0.035 | 0.909 | 3.342 | -5.169 |
| GPU Write Bandwidth | -0.036 | 0.887 | -0.181 | -2.159 |
| LLC Utilization | -0.055 | 0.882 | 1.657 | -2.241 |
| Top Performance Limiter | -0.094 | 0.927 | 8.018 | 2.253 |
| Total Occupancy | -0.242 | 0.854 | 14.101 | 0.946 |
## uint64 vs float32

| Counter | Avg delta | Avg ratio | P99 delta | Peak delta |
|---------|-----------|-----------|-----------|------------|
| ALU Utilization | -0.213 | 0.705 | -10.066 | 1.389 |
| Compute Occupancy | 0.001 | 1.024 | -0.287 | 2.016 |
| F16 Utilization | -0.049 | 0.741 | -4.130 | 0.155 |
| F32 Utilization | -0.094 | 0.444 | -2.906 | 16.591 |
| Fragment Occupancy | -0.365 | 0.719 | 0.222 | 0.892 |
| GPU Read Bandwidth | -0.086 | 0.777 | -0.040 | -6.264 |
| GPU Write Bandwidth | -0.071 | 0.777 | -0.878 | 20.928 |
| LLC Utilization | -0.109 | 0.766 | -1.979 | -5.107 |
| Top Performance Limiter | -0.408 | 0.681 | -2.375 | 2.253 |
| Total Occupancy | -0.473 | 0.714 | 4.999 | 0.936 |
