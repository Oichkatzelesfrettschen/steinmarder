# GPU Hardware Counter Deltas

- source: `/Users/eirikr/Documents/GitHub/steinmarder/src/apple_re/results/typed_metal_suite_20260401_apple_sync/typed_format_agx_20260401/gpu_hardware_counters.csv`
- baseline_variant: `float32`
- compared_variants: 11

## bfloat16 vs float32

| Counter | Avg delta | Avg ratio | P99 delta | Peak delta |
|---------|-----------|-----------|-----------|------------|
| ALU Utilization | 0.130 | 1.262 | 4.160 | -0.003 |
| Compute Occupancy | 0.001 | 1.037 | 0.006 | 0.014 |
| F16 Utilization | 0.028 | 1.207 | 2.270 | -0.038 |
| F32 Utilization | 0.021 | 1.226 | 1.407 | -1.155 |
| Fragment Occupancy | 0.220 | 1.260 | 13.255 | 0.000 |
| GPU Read Bandwidth | 0.071 | 1.306 | 5.739 | -0.292 |
| GPU Write Bandwidth | 0.048 | 1.261 | 1.457 | -29.306 |
| LLC Utilization | 0.064 | 1.214 | 3.743 | 4.918 |
| Top Performance Limiter | 0.177 | 1.197 | 8.297 | 0.000 |
| Total Occupancy | 0.286 | 1.289 | 14.186 | -66.184 |
## float16 vs float32

| Counter | Avg delta | Avg ratio | P99 delta | Peak delta |
|---------|-----------|-----------|-----------|------------|
| ALU Utilization | 0.047 | 1.095 | 1.734 | -0.074 |
| Compute Occupancy | 0.000 | 1.000 | 0.135 | 0.888 |
| F16 Utilization | 0.013 | 1.096 | 0.671 | -0.069 |
| F32 Utilization | 0.010 | 1.108 | 0.660 | -1.462 |
| Fragment Occupancy | 0.135 | 1.160 | 7.756 | 0.000 |
| GPU Read Bandwidth | 0.038 | 1.164 | 2.829 | 1.411 |
| GPU Write Bandwidth | 0.020 | 1.109 | 1.045 | -34.000 |
| LLC Utilization | 0.030 | 1.100 | 2.042 | 1.371 |
| Top Performance Limiter | 0.108 | 1.120 | 4.777 | 0.000 |
| Total Occupancy | 0.197 | 1.199 | 9.099 | -66.190 |
## fp8_e4m3 vs float32

| Counter | Avg delta | Avg ratio | P99 delta | Peak delta |
|---------|-----------|-----------|-----------|------------|
| ALU Utilization | 0.406 | 1.819 | 3.907 | 0.029 |
| Compute Occupancy | 0.090 | 4.333 | 0.245 | 54.828 |
| F16 Utilization | 0.026 | 1.193 | 1.715 | 0.026 |
| F32 Utilization | 0.071 | 1.763 | 1.016 | -1.211 |
| Fragment Occupancy | 0.214 | 1.253 | 13.930 | 0.000 |
| GPU Read Bandwidth | 0.072 | 1.310 | 4.771 | 1.114 |
| GPU Write Bandwidth | 0.036 | 1.196 | 1.217 | 0.072 |
| LLC Utilization | 0.081 | 1.271 | 3.847 | 6.944 |
| Top Performance Limiter | 0.439 | 1.489 | 11.236 | 0.000 |
| Total Occupancy | 0.488 | 1.493 | 18.568 | -2.854 |
## fp8_e5m2 vs float32

| Counter | Avg delta | Avg ratio | P99 delta | Peak delta |
|---------|-----------|-----------|-----------|------------|
| ALU Utilization | 0.354 | 1.714 | 1.818 | 0.099 |
| Compute Occupancy | 0.084 | 4.111 | 0.252 | -0.072 |
| F16 Utilization | 0.013 | 1.096 | 0.359 | 0.004 |
| F32 Utilization | 0.065 | 1.699 | 0.590 | -1.513 |
| Fragment Occupancy | 0.116 | 1.137 | 6.928 | 0.000 |
| GPU Read Bandwidth | 0.038 | 1.164 | 1.534 | 0.644 |
| GPU Write Bandwidth | 0.012 | 1.065 | 0.535 | -0.053 |
| LLC Utilization | 0.051 | 1.171 | 0.969 | 4.193 |
| Top Performance Limiter | 0.352 | 1.392 | 4.406 | 0.000 |
| Total Occupancy | 0.328 | 1.332 | 11.287 | -66.163 |
## int8 vs float32

| Counter | Avg delta | Avg ratio | P99 delta | Peak delta |
|---------|-----------|-----------|-----------|------------|
| ALU Utilization | -0.014 | 0.972 | -1.150 | 0.012 |
| Compute Occupancy | -0.003 | 0.889 | -0.001 | 0.004 |
| F16 Utilization | -0.002 | 0.985 | -0.314 | 0.072 |
| F32 Utilization | -0.008 | 0.914 | -0.365 | -1.298 |
| Fragment Occupancy | -0.033 | 0.961 | -2.039 | 0.000 |
| GPU Read Bandwidth | -0.008 | 0.966 | -0.090 | -3.817 |
| GPU Write Bandwidth | -0.010 | 0.946 | -0.116 | -36.636 |
| LLC Utilization | -0.008 | 0.973 | -0.720 | -0.255 |
| Top Performance Limiter | -0.031 | 0.965 | -1.421 | 0.000 |
| Total Occupancy | -0.048 | 0.951 | -2.246 | -8.609 |
## mxfp4 vs float32

| Counter | Avg delta | Avg ratio | P99 delta | Peak delta |
|---------|-----------|-----------|-----------|------------|
| ALU Utilization | 0.317 | 1.639 | 4.620 | -0.046 |
| Compute Occupancy | 0.129 | 5.778 | 0.338 | 0.160 |
| F16 Utilization | 0.046 | 1.341 | 2.613 | -0.026 |
| F32 Utilization | 0.034 | 1.366 | 1.659 | 0.777 |
| Fragment Occupancy | 0.362 | 1.428 | 19.425 | 0.000 |
| GPU Read Bandwidth | 0.097 | 1.418 | 8.137 | -0.467 |
| GPU Write Bandwidth | 0.055 | 1.299 | 1.767 | -33.586 |
| LLC Utilization | 0.092 | 1.308 | 5.432 | 0.932 |
| Top Performance Limiter | 0.400 | 1.445 | 13.616 | 0.000 |
| Total Occupancy | 0.905 | 1.915 | 27.241 | -66.189 |
## mxfp6 vs float32

| Counter | Avg delta | Avg ratio | P99 delta | Peak delta |
|---------|-----------|-----------|-----------|------------|
| ALU Utilization | 0.720 | 2.452 | 16.046 | -0.471 |
| Compute Occupancy | 0.137 | 6.074 | 0.327 | 93.590 |
| F16 Utilization | 0.110 | 1.815 | 5.493 | -0.091 |
| F32 Utilization | 0.094 | 2.011 | 2.017 | 14.410 |
| Fragment Occupancy | 0.948 | 2.122 | 18.995 | 0.000 |
| GPU Read Bandwidth | 0.179 | 1.772 | 5.604 | 9.453 |
| GPU Write Bandwidth | 0.107 | 1.582 | 1.917 | -4.456 |
| LLC Utilization | 0.194 | 1.649 | 3.364 | 2.934 |
| Top Performance Limiter | 0.977 | 2.088 | 18.530 | 0.000 |
| Total Occupancy | 1.626 | 2.644 | 17.753 | -45.585 |
## mxfp8 vs float32

| Counter | Avg delta | Avg ratio | P99 delta | Peak delta |
|---------|-----------|-----------|-----------|------------|
| ALU Utilization | 0.327 | 1.659 | 2.988 | 1.494 |
| Compute Occupancy | 0.143 | 6.296 | 0.334 | 82.176 |
| F16 Utilization | 0.029 | 1.215 | 1.369 | 0.107 |
| F32 Utilization | 0.037 | 1.398 | 0.931 | 0.887 |
| Fragment Occupancy | 0.274 | 1.324 | 16.464 | 0.000 |
| GPU Read Bandwidth | 0.087 | 1.375 | 5.020 | 2.007 |
| GPU Write Bandwidth | 0.028 | 1.152 | 1.032 | -2.173 |
| LLC Utilization | 0.079 | 1.264 | 3.138 | 4.424 |
| Top Performance Limiter | 0.402 | 1.448 | 15.114 | 0.000 |
| Total Occupancy | 0.708 | 1.716 | 37.281 | -66.160 |
## mxint8 vs float32

| Counter | Avg delta | Avg ratio | P99 delta | Peak delta |
|---------|-----------|-----------|-----------|------------|
| ALU Utilization | -0.310 | 0.375 | -14.996 | -66.106 |
| Compute Occupancy | 0.111 | 5.111 | 0.291 | -4.794 |
| F16 Utilization | -0.132 | 0.022 | -3.216 | -22.954 |
| F32 Utilization | -0.073 | 0.215 | -3.009 | -19.395 |
| Fragment Occupancy | -0.776 | 0.082 | -35.375 | -8.417 |
| GPU Read Bandwidth | -0.185 | 0.203 | -5.886 | -12.609 |
| GPU Write Bandwidth | -0.113 | 0.386 | -3.878 | -40.588 |
| LLC Utilization | -0.177 | 0.408 | -7.164 | -6.708 |
| Top Performance Limiter | -0.676 | 0.247 | -31.759 | -29.969 |
| Total Occupancy | -0.716 | 0.276 | -41.627 | -74.638 |
## tf32 vs float32

| Counter | Avg delta | Avg ratio | P99 delta | Peak delta |
|---------|-----------|-----------|-----------|------------|
| ALU Utilization | 0.085 | 1.171 | 2.605 | 0.059 |
| Compute Occupancy | -0.002 | 0.926 | -0.004 | 1.430 |
| F16 Utilization | 0.022 | 1.163 | 1.523 | -0.037 |
| F32 Utilization | 0.018 | 1.194 | 1.032 | -0.919 |
| Fragment Occupancy | 0.164 | 1.194 | 9.990 | 0.000 |
| GPU Read Bandwidth | 0.038 | 1.164 | 2.525 | -1.187 |
| GPU Write Bandwidth | 0.028 | 1.152 | 1.184 | -0.165 |
| LLC Utilization | 0.040 | 1.134 | 2.024 | 0.357 |
| Top Performance Limiter | 0.147 | 1.164 | 4.638 | 0.000 |
| Total Occupancy | 0.199 | 1.201 | 9.561 | -66.159 |
## uint8 vs float32

| Counter | Avg delta | Avg ratio | P99 delta | Peak delta |
|---------|-----------|-----------|-----------|------------|
| ALU Utilization | 0.072 | 1.145 | 2.893 | 0.157 |
| Compute Occupancy | -0.003 | 0.889 | 0.127 | 0.173 |
| F16 Utilization | 0.025 | 1.185 | 1.676 | 0.105 |
| F32 Utilization | 0.009 | 1.097 | 0.882 | -1.364 |
| Fragment Occupancy | 0.197 | 1.233 | 12.749 | 0.000 |
| GPU Read Bandwidth | 0.049 | 1.211 | 2.938 | -0.873 |
| GPU Write Bandwidth | 0.025 | 1.136 | 1.046 | -2.447 |
| LLC Utilization | 0.038 | 1.127 | 2.124 | 2.521 |
| Top Performance Limiter | 0.177 | 1.197 | 7.729 | 0.000 |
| Total Occupancy | 0.268 | 1.271 | 13.040 | 11.198 |
