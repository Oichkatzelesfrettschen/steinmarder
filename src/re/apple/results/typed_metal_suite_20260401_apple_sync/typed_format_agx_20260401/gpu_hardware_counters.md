# Metal GPU Hardware Counter Summary

- run_dir: `/Users/eirikr/Documents/GitHub/steinmarder/src/apple_re/results/typed_metal_suite_20260401_apple_sync/typed_format_agx_20260401`
- variants_with_data: 12
- counter_series: 384

## bfloat16

| Counter | Samples | P90% | P99% | Max% | Avg% |
|---------|---------|------|------|------|------|
| ALU Utilization | 78845 | 0.061 | 19.389 | 81.696 | 0.626 |
| Compute Occupancy | 78845 | 0.030 | 0.049 | 5.467 | 0.028 |
| F16 Utilization | 78845 | 0.000 | 5.486 | 25.719 | 0.163 |
| F32 Utilization | 78845 | 0.008 | 4.434 | 23.938 | 0.114 |
| Fragment Occupancy | 78845 | 0.000 | 48.630 | 100.000 | 1.065 |
| GPU Read Bandwidth | 78845 | 0.021 | 11.647 | 38.903 | 0.303 |
| GPU Write Bandwidth | 78845 | 0.049 | 5.384 | 37.545 | 0.232 |
| LLC Utilization | 78845 | 0.100 | 11.028 | 38.512 | 0.363 |
| Top Performance Limiter | 78842 | 0.100 | 40.291 | 100.000 | 1.075 |
| Total Occupancy | 67695 | 0.031 | 56.159 | 100.037 | 1.275 |

## float16

| Counter | Samples | P90% | P99% | Max% | Avg% |
|---------|---------|------|------|------|------|
| ALU Utilization | 78644 | 0.026 | 16.963 | 81.625 | 0.543 |
| Compute Occupancy | 78644 | 0.028 | 0.178 | 6.341 | 0.027 |
| F16 Utilization | 78644 | 0.000 | 3.887 | 25.688 | 0.148 |
| F32 Utilization | 78644 | 0.008 | 3.687 | 23.631 | 0.103 |
| Fragment Occupancy | 78644 | 0.000 | 43.131 | 100.000 | 0.980 |
| GPU Read Bandwidth | 78644 | 0.020 | 8.737 | 40.606 | 0.270 |
| GPU Write Bandwidth | 78644 | 0.049 | 4.972 | 32.851 | 0.204 |
| LLC Utilization | 78644 | 0.098 | 9.327 | 34.965 | 0.329 |
| Top Performance Limiter | 78641 | 0.098 | 36.771 | 100.000 | 1.006 |
| Total Occupancy | 66904 | 0.028 | 51.072 | 100.031 | 1.186 |

## float32

| Counter | Samples | P90% | P99% | Max% | Avg% |
|---------|---------|------|------|------|------|
| ALU Utilization | 78751 | 0.026 | 15.229 | 81.699 | 0.496 |
| Compute Occupancy | 78751 | 0.026 | 0.043 | 5.453 | 0.027 |
| F16 Utilization | 78751 | 0.000 | 3.216 | 25.757 | 0.135 |
| F32 Utilization | 78751 | 0.008 | 3.027 | 25.093 | 0.093 |
| Fragment Occupancy | 78751 | 0.000 | 35.375 | 100.000 | 0.845 |
| GPU Read Bandwidth | 78751 | 0.020 | 5.908 | 39.195 | 0.232 |
| GPU Write Bandwidth | 78751 | 0.049 | 3.927 | 66.851 | 0.184 |
| LLC Utilization | 78751 | 0.100 | 7.285 | 33.594 | 0.299 |
| Top Performance Limiter | 78748 | 0.100 | 31.994 | 100.000 | 0.898 |
| Total Occupancy | 69476 | 0.027 | 41.973 | 166.221 | 0.989 |

## fp8_e4m3

| Counter | Samples | P90% | P99% | Max% | Avg% |
|---------|---------|------|------|------|------|
| ALU Utilization | 78857 | 0.429 | 19.136 | 81.728 | 0.902 |
| Compute Occupancy | 78857 | 0.146 | 0.288 | 60.281 | 0.117 |
| F16 Utilization | 78857 | 0.000 | 4.931 | 25.783 | 0.161 |
| F32 Utilization | 78857 | 0.082 | 4.043 | 23.882 | 0.164 |
| Fragment Occupancy | 78857 | 0.000 | 49.305 | 100.000 | 1.059 |
| GPU Read Bandwidth | 78857 | 0.032 | 10.679 | 40.309 | 0.304 |
| GPU Write Bandwidth | 78857 | 0.049 | 5.144 | 66.923 | 0.220 |
| LLC Utilization | 78857 | 0.143 | 11.132 | 40.538 | 0.380 |
| Top Performance Limiter | 78854 | 0.431 | 43.230 | 100.000 | 1.337 |
| Total Occupancy | 62845 | 0.152 | 60.541 | 163.367 | 1.477 |

## fp8_e5m2

| Counter | Samples | P90% | P99% | Max% | Avg% |
|---------|---------|------|------|------|------|
| ALU Utilization | 78760 | 0.429 | 17.047 | 81.798 | 0.850 |
| Compute Occupancy | 78760 | 0.147 | 0.295 | 5.381 | 0.111 |
| F16 Utilization | 78760 | 0.000 | 3.575 | 25.761 | 0.148 |
| F32 Utilization | 78760 | 0.082 | 3.617 | 23.580 | 0.158 |
| Fragment Occupancy | 78760 | 0.000 | 42.303 | 100.000 | 0.961 |
| GPU Read Bandwidth | 78760 | 0.031 | 7.442 | 39.839 | 0.270 |
| GPU Write Bandwidth | 78760 | 0.049 | 4.462 | 66.798 | 0.196 |
| LLC Utilization | 78760 | 0.143 | 8.254 | 37.787 | 0.350 |
| Top Performance Limiter | 78757 | 0.431 | 36.400 | 100.000 | 1.250 |
| Total Occupancy | 64107 | 0.156 | 53.260 | 100.058 | 1.317 |

## int8

| Counter | Samples | P90% | P99% | Max% | Avg% |
|---------|---------|------|------|------|------|
| ALU Utilization | 78770 | 0.025 | 14.079 | 81.711 | 0.482 |
| Compute Occupancy | 78770 | 0.025 | 0.042 | 5.457 | 0.024 |
| F16 Utilization | 78770 | 0.000 | 2.902 | 25.829 | 0.133 |
| F32 Utilization | 78770 | 0.000 | 2.662 | 23.795 | 0.085 |
| Fragment Occupancy | 78770 | 0.000 | 33.336 | 100.000 | 0.812 |
| GPU Read Bandwidth | 78770 | 0.021 | 5.818 | 35.378 | 0.224 |
| GPU Write Bandwidth | 78770 | 0.049 | 3.811 | 30.215 | 0.174 |
| LLC Utilization | 78770 | 0.102 | 6.565 | 33.339 | 0.291 |
| Top Performance Limiter | 78767 | 0.102 | 30.573 | 100.000 | 0.867 |
| Total Occupancy | 70082 | 0.026 | 39.727 | 157.612 | 0.941 |

## mxfp4

| Counter | Samples | P90% | P99% | Max% | Avg% |
|---------|---------|------|------|------|------|
| ALU Utilization | 78361 | 0.264 | 19.849 | 81.653 | 0.813 |
| Compute Occupancy | 78361 | 0.225 | 0.381 | 5.613 | 0.156 |
| F16 Utilization | 78361 | 0.000 | 5.829 | 25.731 | 0.181 |
| F32 Utilization | 78361 | 0.017 | 4.686 | 25.870 | 0.127 |
| Fragment Occupancy | 78361 | 0.000 | 54.800 | 100.000 | 1.207 |
| GPU Read Bandwidth | 78361 | 0.026 | 14.045 | 38.728 | 0.329 |
| GPU Write Bandwidth | 78361 | 0.049 | 5.694 | 33.265 | 0.239 |
| LLC Utilization | 78361 | 0.133 | 12.717 | 34.526 | 0.391 |
| Top Performance Limiter | 78358 | 0.266 | 45.610 | 100.000 | 1.298 |
| Total Occupancy | 56411 | 0.347 | 69.214 | 100.032 | 1.894 |

## mxfp6

| Counter | Samples | P90% | P99% | Max% | Avg% |
|---------|---------|------|------|------|------|
| ALU Utilization | 81173 | 0.325 | 31.275 | 81.228 | 1.216 |
| Compute Occupancy | 81173 | 0.222 | 0.370 | 99.043 | 0.164 |
| F16 Utilization | 81173 | 0.000 | 8.709 | 25.666 | 0.245 |
| F32 Utilization | 81173 | 0.031 | 5.044 | 39.503 | 0.187 |
| Fragment Occupancy | 81173 | 0.000 | 54.370 | 100.000 | 1.793 |
| GPU Read Bandwidth | 81173 | 0.027 | 11.512 | 48.648 | 0.411 |
| GPU Write Bandwidth | 81173 | 0.066 | 5.844 | 62.395 | 0.291 |
| LLC Utilization | 81173 | 0.143 | 10.649 | 36.528 | 0.493 |
| Top Performance Limiter | 81170 | 0.326 | 50.524 | 100.000 | 1.875 |
| Total Occupancy | 60839 | 0.380 | 59.726 | 120.636 | 2.615 |

## mxfp8

| Counter | Samples | P90% | P99% | Max% | Avg% |
|---------|---------|------|------|------|------|
| ALU Utilization | 78812 | 0.349 | 18.217 | 83.193 | 0.823 |
| Compute Occupancy | 78812 | 0.231 | 0.377 | 87.629 | 0.170 |
| F16 Utilization | 78812 | 0.000 | 4.585 | 25.864 | 0.164 |
| F32 Utilization | 78812 | 0.037 | 3.958 | 25.980 | 0.130 |
| Fragment Occupancy | 78812 | 0.000 | 51.839 | 100.000 | 1.119 |
| GPU Read Bandwidth | 78812 | 0.025 | 10.928 | 41.202 | 0.319 |
| GPU Write Bandwidth | 78812 | 0.049 | 4.959 | 64.678 | 0.212 |
| LLC Utilization | 78812 | 0.143 | 10.423 | 38.018 | 0.378 |
| Top Performance Limiter | 78809 | 0.351 | 47.108 | 100.000 | 1.300 |
| Total Occupancy | 59901 | 0.290 | 79.254 | 100.061 | 1.697 |

## mxint8

| Counter | Samples | P90% | P99% | Max% | Avg% |
|---------|---------|------|------|------|------|
| ALU Utilization | 78843 | 0.232 | 0.233 | 15.593 | 0.186 |
| Compute Occupancy | 78843 | 0.195 | 0.334 | 0.659 | 0.138 |
| F16 Utilization | 78843 | 0.000 | 0.000 | 2.803 | 0.003 |
| F32 Utilization | 78843 | 0.018 | 0.018 | 5.698 | 0.020 |
| Fragment Occupancy | 78843 | 0.000 | 0.000 | 91.583 | 0.069 |
| GPU Read Bandwidth | 78843 | 0.022 | 0.022 | 26.586 | 0.047 |
| GPU Write Bandwidth | 78843 | 0.049 | 0.049 | 26.263 | 0.071 |
| LLC Utilization | 78843 | 0.120 | 0.121 | 26.886 | 0.122 |
| Top Performance Limiter | 78840 | 0.234 | 0.235 | 70.031 | 0.222 |
| Total Occupancy | 59804 | 0.195 | 0.346 | 91.583 | 0.273 |

## tf32

| Counter | Samples | P90% | P99% | Max% | Avg% |
|---------|---------|------|------|------|------|
| ALU Utilization | 77847 | 0.035 | 17.834 | 81.758 | 0.581 |
| Compute Occupancy | 77847 | 0.020 | 0.039 | 6.883 | 0.025 |
| F16 Utilization | 77847 | 0.000 | 4.739 | 25.720 | 0.157 |
| F32 Utilization | 77847 | 0.009 | 4.059 | 24.174 | 0.111 |
| Fragment Occupancy | 77847 | 0.000 | 45.365 | 100.000 | 1.009 |
| GPU Read Bandwidth | 77847 | 0.021 | 8.433 | 38.008 | 0.270 |
| GPU Write Bandwidth | 77847 | 0.049 | 5.111 | 66.686 | 0.212 |
| LLC Utilization | 77847 | 0.101 | 9.309 | 33.951 | 0.339 |
| Top Performance Limiter | 77844 | 0.101 | 36.632 | 100.000 | 1.045 |
| Total Occupancy | 67801 | 0.021 | 51.534 | 100.062 | 1.188 |

## uint8

| Counter | Samples | P90% | P99% | Max% | Avg% |
|---------|---------|------|------|------|------|
| ALU Utilization | 78820 | 0.008 | 18.122 | 81.856 | 0.568 |
| Compute Occupancy | 78820 | 0.024 | 0.170 | 5.626 | 0.024 |
| F16 Utilization | 78820 | 0.000 | 4.892 | 25.862 | 0.160 |
| F32 Utilization | 78820 | 0.000 | 3.909 | 23.729 | 0.102 |
| Fragment Occupancy | 78820 | 0.000 | 48.124 | 100.000 | 1.042 |
| GPU Read Bandwidth | 78820 | 0.020 | 8.846 | 38.322 | 0.281 |
| GPU Write Bandwidth | 78820 | 0.049 | 4.973 | 64.404 | 0.209 |
| LLC Utilization | 78820 | 0.098 | 9.409 | 36.115 | 0.337 |
| Top Performance Limiter | 78817 | 0.098 | 39.723 | 100.000 | 1.075 |
| Total Occupancy | 66857 | 0.025 | 55.013 | 177.419 | 1.257 |
