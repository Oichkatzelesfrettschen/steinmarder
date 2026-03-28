@echo off
setlocal enabledelayedexpansion
set SM_GPU_WINDOW=1
set SM_GPU_W=320
set SM_GPU_H=180
set SM_GPU_SPP=4
set SM_GPU_DUMP_ONESHOT=1
set SM_GPU_TONEMAP=0
echo Running window mode test with tonemap disabled...
gpu_demo.exe
echo Test completed - check window_dump.ppm
