@echo off
REM Test bilateral denoiser: 4 SPP + denoising should match quality of 32+ SPP without denoising
setlocal enabledelayedexpansion

echo Building with bilateral denoiser...
C:\VulkanSDK\1.4.335.0\Bin\glslangValidator.exe -V shaders/tri.comp -o shaders/tri.comp.spv
REM Note: NOT including denoise.c (we replace it with bilateral_denoise)
gcc -std=c11 -O2 -pthread -o gpu_demo.exe gpu_vulkan_demo.c gpu_bvh_lbv.c bilateral_denoise.c neural_denoise.c -lvulkan-1 -lglfw3 -lws2_32 -luser32 -lm
if errorlevel 1 (
    echo Build failed
    exit /b 1
)

echo.
echo ============================================================
echo Test 1: 4 SPP + Bilateral Denoiser (FAST - best for realtime)
echo ============================================================
set SM_GPU_WINDOW=1
set SM_GPU_W=320
set SM_GPU_H=180
set SM_GPU_SPP=4
set SM_GPU_DUMP_ONESHOT=1
set SM_NEURAL_DENOISE=1
set SM_BILATERAL_SIGMA_S=1.5
set SM_BILATERAL_SIGMA_R=0.1
set SM_BILATERAL_RADIUS=3

echo Running with 4 SPP + bilateral denoiser...
gpu_demo.exe
echo.

echo.
echo ============================================================
echo Test 2: 4 SPP WITHOUT Denoiser (noisy - for comparison)
echo ============================================================
set SM_GPU_SPP=4
set SM_NEURAL_DENOISE=0
set SM_GPU_DUMP_ONESHOT=1

echo Running with 4 SPP (no denoise)...
gpu_demo.exe
echo.

echo.
echo ============================================================
echo Test 3: 32 SPP reference (clean but slower)
echo ============================================================
set SM_GPU_SPP=32
set SM_NEURAL_DENOISE=0
set SM_GPU_DUMP_ONESHOT=1

echo Running with 32 SPP (no denoise, reference)...
gpu_demo.exe
echo.

echo.
echo ============================================================
echo RESULTS:
echo - Test 1: 4 SPP + denoiser = ~8x faster than test 3
echo          Quality should match test 3 closely
echo - Test 2: 4 SPP undenoised = very noisy (reference for comparison)
echo - Test 3: 32 SPP reference = clean output (slow)
echo ============================================================
dir window_dump.ppm

