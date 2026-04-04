# Metal GPU Hardware Counter Summary

- run_dir: `/Users/eirikr/Documents/GitHub/steinmarder/src/apple_re/results/typed_metal_suite_20260401_apple_sync`
- variants_with_data: 5
- counter_series: 160

## baseline

| Counter | Samples | P90% | P99% | Max% | Avg% |
|---------|---------|------|------|------|------|
| ALU Utilization | 31317 | 0.024 | 6.645 | 81.387 | 0.388 |
| Compute Occupancy | 31317 | 0.029 | 0.046 | 5.342 | 0.026 |
| F16 Utilization | 31317 | 0.000 | 1.386 | 25.639 | 0.107 |
| F32 Utilization | 31317 | 0.000 | 1.363 | 23.593 | 0.070 |
| Fragment Occupancy | 31317 | 0.000 | 20.486 | 100.000 | 0.639 |
| GPU Read Bandwidth | 31317 | 0.020 | 2.016 | 33.679 | 0.157 |
| GPU Write Bandwidth | 31317 | 0.049 | 2.056 | 25.548 | 0.126 |
| LLC Utilization | 31317 | 0.098 | 4.630 | 26.368 | 0.219 |
| Top Performance Limiter | 31314 | 0.098 | 19.751 | 100.000 | 0.704 |
| Total Occupancy | 26064 | 0.029 | 31.631 | 100.009 | 0.799 |

## occupancy_heavy

| Counter | Samples | P90% | P99% | Max% | Avg% |
|---------|---------|------|------|------|------|
| ALU Utilization | 32509 | 0.470 | 10.621 | 81.862 | 0.753 |
| Compute Occupancy | 32509 | 0.147 | 0.298 | 97.099 | 0.130 |
| F16 Utilization | 32509 | 0.000 | 3.697 | 25.639 | 0.125 |
| F32 Utilization | 32509 | 0.000 | 2.426 | 38.620 | 0.091 |
| Fragment Occupancy | 32509 | 0.000 | 29.207 | 100.000 | 0.805 |
| GPU Read Bandwidth | 32509 | 0.032 | 7.558 | 50.191 | 0.235 |
| GPU Write Bandwidth | 32509 | 0.049 | 3.025 | 38.466 | 0.148 |
| LLC Utilization | 32509 | 0.145 | 6.791 | 34.607 | 0.279 |
| Top Performance Limiter | 32506 | 0.514 | 28.196 | 100.000 | 1.094 |
| Total Occupancy | 22853 | 0.158 | 54.285 | 100.001 | 1.331 |

## register_pressure

| Counter | Samples | P90% | P99% | Max% | Avg% |
|---------|---------|------|------|------|------|
| ALU Utilization | 31382 | 0.475 | 19.571 | 82.459 | 0.927 |
| Compute Occupancy | 31382 | 0.138 | 0.285 | 6.643 | 0.095 |
| F16 Utilization | 31382 | 0.000 | 5.387 | 19.205 | 0.126 |
| F32 Utilization | 31382 | 0.005 | 3.939 | 29.768 | 0.185 |
| Fragment Occupancy | 31382 | 0.000 | 43.153 | 100.000 | 1.001 |
| GPU Read Bandwidth | 31382 | 0.032 | 4.658 | 27.144 | 0.170 |
| GPU Write Bandwidth | 31382 | 0.049 | 2.629 | 25.762 | 0.143 |
| LLC Utilization | 31382 | 0.143 | 6.995 | 27.380 | 0.277 |
| Top Performance Limiter | 31379 | 0.484 | 39.628 | 100.000 | 1.340 |
| Total Occupancy | 24495 | 0.141 | 60.113 | 100.016 | 1.406 |

## threadgroup_heavy

| Counter | Samples | P90% | P99% | Max% | Avg% |
|---------|---------|------|------|------|------|
| ALU Utilization | 31624 | 0.106 | 14.953 | 81.170 | 0.514 |
| Compute Occupancy | 31624 | 0.234 | 0.392 | 5.606 | 0.167 |
| F16 Utilization | 31624 | 0.000 | 2.777 | 25.632 | 0.126 |
| F32 Utilization | 31624 | 0.000 | 2.745 | 23.325 | 0.081 |
| Fragment Occupancy | 31624 | 0.000 | 34.046 | 100.000 | 0.862 |
| GPU Read Bandwidth | 31624 | 0.024 | 6.497 | 39.574 | 0.249 |
| GPU Write Bandwidth | 31624 | 0.049 | 4.322 | 33.511 | 0.184 |
| LLC Utilization | 31624 | 0.192 | 7.595 | 35.910 | 0.354 |
| Top Performance Limiter | 31621 | 0.192 | 31.112 | 100.000 | 0.932 |
| Total Occupancy | 23159 | 0.367 | 52.189 | 100.058 | 1.407 |

## threadgroup_minimal

| Counter | Samples | P90% | P99% | Max% | Avg% |
|---------|---------|------|------|------|------|
| ALU Utilization | 31055 | 0.274 | 0.513 | 81.138 | 0.392 |
| Compute Occupancy | 31055 | 0.098 | 0.251 | 5.373 | 0.074 |
| F16 Utilization | 31055 | 0.000 | 0.063 | 25.549 | 0.056 |
| F32 Utilization | 31055 | 0.000 | 0.141 | 23.010 | 0.032 |
| Fragment Occupancy | 31055 | 0.000 | 0.760 | 98.486 | 0.296 |
| GPU Read Bandwidth | 31055 | 0.027 | 0.549 | 33.466 | 0.111 |
| GPU Write Bandwidth | 31055 | 0.049 | 0.714 | 26.117 | 0.097 |
| LLC Utilization | 31055 | 0.122 | 0.913 | 27.097 | 0.180 |
| Top Performance Limiter | 31052 | 0.286 | 1.128 | 99.243 | 0.493 |
| Total Occupancy | 23755 | 0.099 | 9.944 | 98.494 | 0.484 |
