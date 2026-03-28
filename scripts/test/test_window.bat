@echo off
setlocal enabledelayedexpansion
set SM_GPU_WINDOW=1
set SM_GPU_W=320
set SM_GPU_H=180
set SM_GPU_SPP=128
set SM_GPU_DUMP_ONESHOT=1
echo Running window mode test...
gpu_demo.exe
echo Window mode test completed
dir window_dump.ppm
