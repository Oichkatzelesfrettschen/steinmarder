@echo off
REM Quick test of mode 17 with diagnostics

set SM_NERF_HASHGRID=models/nerf_hashgrid.bin
set SM_NERF_OCC=models/nerf_occ.bin
set SM_RENDER_MODE=17
set SM_W=800
set SM_H=600
set SM_SPP=1
set SM_DEPTH=2
set SM_USE_BVH=1
set SM_NERF_STEPS=32
set SM_NERF_BOUNDS=8.0

echo.
echo Mode 17 Diagnostic Test
echo =======================
echo.
echo Expected colors:
echo   - Normal: Trained NeRF colors (e.g. fox orange/white/black)
echo   - Magenta: Sampling position out of bounds
echo   - Yellow: NeRF buffer not valid
echo   - Red: NaN/Inf detected in MLP output
echo   - Blue streaks: Corrupted data (the bug we're fixing)
echo.
echo Press Ctrl+C to exit after viewing
echo.

gpu_demo.exe
