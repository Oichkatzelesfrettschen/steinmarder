@echo off
REM Quick-start script for GPU NeRF demo
REM Sets up environment variables and runs the GPU demo

REM === NeRF Model Files ===
set SM_NERF_HASHGRID=models/nerf_hashgrid.bin
set SM_NERF_OCC=models/nerf_occ.bin

REM === Render Mode ===
REM 0 = mesh only
REM 1 = probe placeholder
REM 2 = hybrid (mesh + NeRF blend)
REM 3 = NeRF only (full volumetric)
REM 7-20 = debug modes (see shader tri.comp for details)
set SM_RENDER_MODE=2

REM === NeRF Parameters ===
set SM_NERF_BLEND=0.35
set SM_NERF_STRENGTH=1.0
set SM_NERF_DENSITY=1.0
set SM_NERF_BOUNDS=8.0
set SM_NERF_STEPS=64
set SM_NERF_SKIP_OCC=0

REM === Optional: Override center/scale (usually auto-detected from .bin file) ===
REM set SM_NERF_CENTER_X=0.0
REM set SM_NERF_CENTER_Y=0.0
REM set SM_NERF_CENTER_Z=0.0
REM set SM_NERF_SCALE=1.0

REM === Window/Rendering Settings ===
set SM_W=1280
set SM_H=720
set SM_SPP=4
set SM_DEPTH=4

REM === BVH Settings ===
set SM_USE_BVH=1
set SM_GPU_BVH_CACHE=cache/bunny.bvhbin

REM === Optional: Enable GPU denoiser ===
REM set SM_GPU_DENOISE=1
REM set SM_GPU_DENOISE_RADIUS=3

echo ===================================
echo   steinmarder NeRF GPU Demo
echo ===================================
echo.
echo Render Mode: %SM_RENDER_MODE%
echo NeRF HashGrid: %SM_NERF_HASHGRID%
echo NeRF Occupancy: %SM_NERF_OCC%
echo Resolution: %SM_W%x%SM_H%
echo NeRF Steps: %SM_NERF_STEPS%
echo NeRF Blend: %SM_NERF_BLEND%
echo.
echo Starting demo...
echo.

gpu_demo.exe
