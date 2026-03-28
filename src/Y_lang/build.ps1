$TOOL_PATH = "C:\SM_Compiler\bin"

Write-Host "--- Cleaning Workspace ---" -ForegroundColor Gray
Remove-Item *.o, sm_kernel.bin -ErrorAction SilentlyContinue

Write-Host "--- Assembling & Compiling ---" -ForegroundColor Cyan
nasm -f elf32 boot.asm -o boot.o
& "$TOOL_PATH\i686-elf-gcc.exe" -c gdt.c -o gdt.o -ffreestanding -fno-stack-protector -m32 -Wno-pointer-to-int-cast
& "$TOOL_PATH\i686-elf-gcc.exe" -c kernel.c -o kernel.o -ffreestanding -fno-stack-protector -m32
& "$TOOL_PATH\i686-elf-gcc.exe" -c sm_shm_portal.c -o sm_shm_portal.o -ffreestanding -fno-stack-protector -m32

Write-Host "--- Linking steinmarder Kernel ---" -ForegroundColor Cyan
& "$TOOL_PATH\i686-elf-ld.exe" -m elf_i386 -T linker.ld -o sm_kernel.bin boot.o kernel.o gdt.o sm_shm_portal.o

if (Test-Path "sm_kernel.bin") {
    Write-Host "`n[SUCCESS] SM_KERNEL.BIN IS READY." -ForegroundColor Green
    Write-Host "--- Launching steinmarder ---" -ForegroundColor Magenta
    qemu-system-i386 -kernel sm_kernel.bin
} else {
    Write-Host "`n[ERROR] Build failed." -ForegroundColor Red
}
