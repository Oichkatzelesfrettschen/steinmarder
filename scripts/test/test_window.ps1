# Quick window test with validation layers
$env:SM_GPU_WINDOW=1
$env:SM_GPU_W=800
$env:SM_GPU_H=600
$env:SM_GPU_FRAMES=1
$env:SM_GPU_SPP=1
$env:SM_GPU_DENOISE_SKIP=4
$env:SM_GPU_WRITE=0
$env:SM_GPU_READBACK=0
$env:VK_LAYER_PATH="$env:VULKAN_SDK\Bin"
$env:VK_INSTANCE_LAYERS="VK_LAYER_KHRONOS_validation"

Write-Host "Running with Vulkan validation layers enabled..." -ForegroundColor Yellow
.\gpu_demo.exe
Write-Host "`nCheck for VK validation errors above" -ForegroundColor Cyan
