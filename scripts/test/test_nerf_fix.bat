@echo off
REM Test all three modes again after fix

set SM_NERF_HASHGRID=models/nerf_hashgrid.bin
set SM_NERF_OCC=models/nerf_occ.bin
set SM_W=800
set SM_H=600
set SM_SPP=1
set SM_USE_BVH=1
set SM_NERF_STEPS=64

echo ===================================
echo   NeRF Fix Verification
echo ===================================
echo.
echo Testing mode 19 (buffer validation)...
set SM_RENDER_MODE=19
start "Mode 19" gpu_demo.exe
timeout /t 3 /nobreak >nul

echo.
echo Testing mode 17 (MLP output)...
set SM_RENDER_MODE=17
start "Mode 17" gpu_demo.exe
timeout /t 3 /nobreak >nul

echo.
echo Testing mode 2 (hybrid mesh+NeRF)...
set SM_RENDER_MODE=2
set SM_NERF_BLEND=0.35
start "Mode 2" gpu_demo.exe

echo.
echo All tests launched. Check the windows!
echo.
echo Expected results:
echo   Mode 19: Dark brownish-gray
echo   Mode 17: Trained NeRF colors (should be recognizable object)
echo   Mode 2: Mesh with NeRF overlay
echo.
pause
