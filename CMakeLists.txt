cmake_minimum_required(VERSION 3.16)
project(steinmarder VERSION 1.0 LANGUAGES C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# ════════════════════════════════════════════════════════════════
# Build type defaults
# ════════════════════════════════════════════════════════════════
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

set(CMAKE_C_FLAGS_RELEASE "-O2 -march=native")
set(CMAKE_C_FLAGS_DEBUG   "-g -O0 -fsanitize=address,undefined")

if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
    add_compile_options(-Wall -Wextra -Wno-unused-parameter -Wno-missing-field-initializers)
endif()

# Link-time optimization for Release builds: enables cross-TU inlining,
# dead code elimination, and interprocedural constant propagation.
# Typical 5-10% speedup across all CPU targets.
include(CheckIPOSupported)
check_ipo_supported(RESULT ipo_supported)
if(ipo_supported AND CMAKE_BUILD_TYPE STREQUAL "Release")
    set(CMAKE_INTERPROCEDURAL_OPTIMIZATION ON)
endif()

# ════════════════════════════════════════════════════════════════
# Output directory — keep build artifacts out of source tree
# ════════════════════════════════════════════════════════════════
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# ════════════════════════════════════════════════════════════════
# Include directories — lets all #include "file.h" resolve
# without changing any existing source code
# ════════════════════════════════════════════════════════════════
set(SM_INCLUDE_DIRS
    ${CMAKE_SOURCE_DIR}/src/core
    ${CMAKE_SOURCE_DIR}/src/render
    ${CMAKE_SOURCE_DIR}/src/denoise
    ${CMAKE_SOURCE_DIR}/src/nerf
    ${CMAKE_SOURCE_DIR}/src/vulkan
    ${CMAKE_SOURCE_DIR}/src/physics
    ${CMAKE_SOURCE_DIR}/src/editor
    ${CMAKE_SOURCE_DIR}/src/upscale
    ${CMAKE_SOURCE_DIR}/src/third_party
)

# ════════════════════════════════════════════════════════════════
# Platform detection
# ════════════════════════════════════════════════════════════════
find_package(Threads REQUIRED)

if(WIN32)
    set(PLATFORM_LIBS ws2_32)
else()
    set(PLATFORM_LIBS m)
endif()

# ════════════════════════════════════════════════════════════════
# Core library (math, primitives, image, materials)
# ════════════════════════════════════════════════════════════════
file(GLOB CORE_SRC src/core/*.c)
# postprocess.c lives in src/render/ but is called by image.c (core), so
# include it in sm_core to resolve the dependency before sm_render is built
list(APPEND CORE_SRC src/render/postprocess.c)
add_library(sm_core STATIC ${CORE_SRC})
target_include_directories(sm_core PUBLIC ${SM_INCLUDE_DIRS})
target_link_libraries(sm_core PUBLIC ${PLATFORM_LIBS})

# ════════════════════════════════════════════════════════════════
# Render library (renderer, BVH, scene, G-buffer)
# ════════════════════════════════════════════════════════════════
set(RENDER_SRC
    src/render/render.c
    src/render/bvh.c
    src/render/sceneloader.c
    src/render/gbuffer.c
    src/render/gbuffer_dump.c
    src/render/sm_mt.c
)
add_library(sm_render STATIC ${RENDER_SRC})
target_include_directories(sm_render PUBLIC ${SM_INCLUDE_DIRS})
target_link_libraries(sm_render PUBLIC sm_core Threads::Threads)

# ════════════════════════════════════════════════════════════════
# Denoise library
# ════════════════════════════════════════════════════════════════
file(GLOB DENOISE_SRC src/denoise/*.c)
add_library(sm_denoise STATIC ${DENOISE_SRC})
target_include_directories(sm_denoise PUBLIC ${SM_INCLUDE_DIRS})
target_link_libraries(sm_denoise PUBLIC sm_core)

# ════════════════════════════════════════════════════════════════
# NeRF library (SIMD inference, scheduling)
# ════════════════════════════════════════════════════════════════
set(NERF_SRC
    src/nerf/nerf_simd.c
    src/nerf/nerf_simd_integration.c
    src/nerf/nerf_batch.c
    src/nerf/nerf_scheduler.c
    src/nerf/depth_hint.c
)
add_library(sm_nerf STATIC ${NERF_SRC})
target_include_directories(sm_nerf PUBLIC ${SM_INCLUDE_DIRS})
target_link_libraries(sm_nerf PUBLIC sm_core)

# AVX2 support (optional, detected at runtime in nerf_simd.c)
include(CheckCCompilerFlag)
check_c_compiler_flag(-mavx2 HAS_AVX2)
if(HAS_AVX2)
    target_compile_options(sm_nerf PRIVATE -mavx2 -mfma)
endif()

# OpenMP (optional, for NeRF batch processing)
find_package(OpenMP QUIET)
if(OpenMP_C_FOUND)
    target_link_libraries(sm_nerf PUBLIC OpenMP::OpenMP_C)
endif()

# ════════════════════════════════════════════════════════════════
# Upscale library
# ════════════════════════════════════════════════════════════════
set(UPSCALE_SRC
    src/upscale/sm_upscale.c
)
add_library(sm_upscale STATIC ${UPSCALE_SRC})
target_include_directories(sm_upscale PUBLIC ${SM_INCLUDE_DIRS})
target_link_libraries(sm_upscale PUBLIC sm_core)

# ════════════════════════════════════════════════════════════════
# Main CPU raytracer executable
# ════════════════════════════════════════════════════════════════
add_executable(steinmarder
    src/render/sm_main.c
    src/editor/sm_360_engine_integration.c
)
target_include_directories(steinmarder PRIVATE ${SM_INCLUDE_DIRS})
target_link_libraries(steinmarder PRIVATE
    sm_render
    sm_denoise
    sm_nerf
    sm_upscale
    sm_core
    Threads::Threads
    ${PLATFORM_LIBS}
)

# ════════════════════════════════════════════════════════════════
# NeRF SIMD standalone test
# ════════════════════════════════════════════════════════════════
add_executable(nerf_simd_test src/nerf/nerf_simd_test.c)
target_include_directories(nerf_simd_test PRIVATE ${SM_INCLUDE_DIRS})
target_link_libraries(nerf_simd_test PRIVATE sm_nerf sm_core ${PLATFORM_LIBS})

# ════════════════════════════════════════════════════════════════
# Vulkan targets (only if Vulkan SDK is found)
# ════════════════════════════════════════════════════════════════
find_package(Vulkan QUIET)
if(Vulkan_FOUND)
    message(STATUS "Vulkan found — building GPU targets")

    # Vulkan GPU library
    set(VULKAN_SRC
        src/vulkan/gpu_vulkan_demo.c
        src/vulkan/gpu_bvh_build.c
        src/vulkan/gpu_bvh_lbv.c
        src/vulkan/gpu_bvh_lbvh_builder.c
        src/vulkan/gpu_obj_loader.c
        src/vulkan/depth_prepass_gpu.c
        src/vulkan/lbvh.c
    )

    add_executable(gpu_demo ${VULKAN_SRC})
    target_include_directories(gpu_demo PRIVATE ${SM_INCLUDE_DIRS})
    target_link_libraries(gpu_demo PRIVATE
        sm_denoise
        sm_nerf
        sm_render
        sm_core
        Vulkan::Vulkan
        Threads::Threads
        ${PLATFORM_LIBS}
    )

    # GLFW for windowed mode (optional)
    find_package(glfw3 QUIET)
    if(glfw3_FOUND)
        target_link_libraries(gpu_demo PRIVATE glfw)
    endif()

    # Physics simulation targets
    set(PHYSICS_COMMON_SRC
        src/physics/quantum_volume.c
        src/physics/nuclear_reaction.c
        src/physics/atomic_fission.c
        src/physics/reactor_thermal.c
    )

    add_executable(quantum_demo
        src/physics/quantum_demo_main.c
        ${PHYSICS_COMMON_SRC}
    )
    target_include_directories(quantum_demo PRIVATE ${SM_INCLUDE_DIRS})
    target_link_libraries(quantum_demo PRIVATE
        sm_core
        Vulkan::Vulkan
        Threads::Threads
        ${PLATFORM_LIBS}
    )
    if(glfw3_FOUND)
        target_link_libraries(quantum_demo PRIVATE glfw)
    endif()

    # Reactor thermal test
    add_executable(test_reactor_thermal src/physics/test_reactor_thermal.c ${PHYSICS_COMMON_SRC})
    target_include_directories(test_reactor_thermal PRIVATE ${SM_INCLUDE_DIRS})
    target_link_libraries(test_reactor_thermal PRIVATE sm_core Vulkan::Vulkan ${PLATFORM_LIBS})

else()
    message(STATUS "Vulkan NOT found — skipping GPU targets")
endif()

# ════════════════════════════════════════════════════════════════
# Editor targets (require raylib)
# ════════════════════════════════════════════════════════════════
find_library(RAYLIB_LIB raylib PATHS ${CMAKE_SOURCE_DIR}/raylib/lib /usr/lib /usr/local/lib)
find_path(RAYLIB_INCLUDE raylib.h PATHS ${CMAKE_SOURCE_DIR}/raylib/include /usr/include /usr/local/include)
if(RAYLIB_LIB AND RAYLIB_INCLUDE)
    message(STATUS "raylib found at ${RAYLIB_LIB} — building editor targets")

    set(EDITOR_LIBS ${RAYLIB_LIB})
    if(WIN32)
        list(APPEND EDITOR_LIBS gdi32 winmm opengl32)
    endif()

    add_executable(sm_mesh_edit src/editor/sm_mesh_edit.c)
    target_include_directories(sm_mesh_edit PRIVATE ${SM_INCLUDE_DIRS} ${RAYLIB_INCLUDE})
    target_link_libraries(sm_mesh_edit PRIVATE sm_core ${EDITOR_LIBS} ${PLATFORM_LIBS})

    # sm_viewport requires GPU integration stubs (Vulkan pipeline not linked to raylib targets)
    add_executable(sm_viewport src/editor/sm_viewport.c)
    target_include_directories(sm_viewport PRIVATE ${SM_INCLUDE_DIRS} ${RAYLIB_INCLUDE})
    target_link_libraries(sm_viewport PRIVATE sm_core sm_render ${EDITOR_LIBS} ${PLATFORM_LIBS})

    add_executable(sm_edit_mode src/editor/sm_edit_mode.c)
    target_include_directories(sm_edit_mode PRIVATE ${SM_INCLUDE_DIRS} ${RAYLIB_INCLUDE})
    target_link_libraries(sm_edit_mode PRIVATE sm_core sm_render ${EDITOR_LIBS} ${PLATFORM_LIBS})
else()
    message(STATUS "raylib NOT found — skipping editor targets")
    message(STATUS "  Install: pacman -S raylib (Arch), apt install libraylib-dev (Debian/Ubuntu),")
    message(STATUS "           dnf install raylib-devel (Fedora), brew install raylib (macOS)")
endif()

# ════════════════════════════════════════════════════════════════
# CUDA LBM targets (only if CUDA Toolkit is found)
# ════════════════════════════════════════════════════════════════
find_package(CUDAToolkit QUIET)
if(CUDAToolkit_FOUND)
    enable_language(CUDA)
    message(STATUS "CUDA found (${CUDAToolkit_VERSION}) — building CUDA LBM targets")
    add_subdirectory(src/cuda_lbm)
else()
    message(STATUS "CUDA NOT found — skipping CUDA LBM targets")
endif()

# ════════════════════════════════════════════════════════════════
# Standalone tools
# ════════════════════════════════════════════════════════════════
add_executable(inspect_ppm src/tools/inspect_ppm.c)
target_include_directories(inspect_ppm PRIVATE ${SM_INCLUDE_DIRS})
target_link_libraries(inspect_ppm PRIVATE ${PLATFORM_LIBS})

add_executable(smb_info src/tools/smb_info.c)
target_include_directories(smb_info PRIVATE ${SM_INCLUDE_DIRS})
target_link_libraries(smb_info PRIVATE ${PLATFORM_LIBS})

add_executable(smb_to_ppm src/tools/smb_to_ppm.c)
target_include_directories(smb_to_ppm PRIVATE ${SM_INCLUDE_DIRS})
target_link_libraries(smb_to_ppm PRIVATE ${PLATFORM_LIBS})

add_executable(test_render_smoke src/tools/test_render_smoke.c)
target_include_directories(test_render_smoke PRIVATE ${SM_INCLUDE_DIRS})
target_link_libraries(test_render_smoke PRIVATE
    sm_render sm_denoise sm_nerf sm_upscale sm_core Threads::Threads ${PLATFORM_LIBS})

# ════════════════════════════════════════════════════════════════
# CTest integration
# ════════════════════════════════════════════════════════════════
enable_testing()

# Reactor thermal unit test (always available)
add_test(NAME test_reactor_thermal COMMAND test_reactor_thermal)

# CUDA LBM tests (only if CUDA available)
if(CUDAToolkit_FOUND)
    add_test(NAME lbm_conservation COMMAND lbm_test)
    add_test(NAME lbm_benchmark_quick COMMAND lbm_bench --kernel "FP32 SoA Fused" --grid 64 --steps 5)
endif()

# CPU render smoke test (dedicated binary, checks for NaN/Inf/zero)
add_test(NAME render_smoke COMMAND test_render_smoke)
set_tests_properties(render_smoke PROPERTIES TIMEOUT 30)

# ════════════════════════════════════════════════════════════════
# Summary
# ════════════════════════════════════════════════════════════════
message(STATUS "")
message(STATUS "=== steinmarder Build Summary ===")
message(STATUS "  Build type:   ${CMAKE_BUILD_TYPE}")
message(STATUS "  C compiler:   ${CMAKE_C_COMPILER_ID} ${CMAKE_C_COMPILER_VERSION}")
message(STATUS "  AVX2:         ${HAS_AVX2}")
message(STATUS "  OpenMP:       ${OpenMP_C_FOUND}")
message(STATUS "  Vulkan:       ${Vulkan_FOUND}")
message(STATUS "  CUDA:         ${CUDAToolkit_FOUND}")
message(STATUS "  raylib:       ${RAYLIB_LIB}")
message(STATUS "================================")
