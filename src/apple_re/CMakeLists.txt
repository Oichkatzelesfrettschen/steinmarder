cmake_minimum_required(VERSION 3.16)

set(APPLE_RE_INCLUDE_DIRS
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

add_executable(sm_apple_cpu_latency
    probes/apple_cpu_latency.c
)
target_include_directories(sm_apple_cpu_latency PRIVATE ${APPLE_RE_INCLUDE_DIRS})
target_link_libraries(sm_apple_cpu_latency PRIVATE ${PLATFORM_LIBS})

if(CMAKE_C_COMPILER_ID MATCHES "AppleClang|Clang")
    target_compile_options(sm_apple_cpu_latency PRIVATE -O3)
endif()

add_executable(sm_apple_cpu_cache_pressure
    probes/apple_cpu_cache_pressure.c
)
target_include_directories(sm_apple_cpu_cache_pressure PRIVATE ${APPLE_RE_INCLUDE_DIRS})
target_link_libraries(sm_apple_cpu_cache_pressure PRIVATE ${PLATFORM_LIBS})

if(CMAKE_C_COMPILER_ID MATCHES "AppleClang|Clang")
    target_compile_options(sm_apple_cpu_cache_pressure PRIVATE -O3)
endif()

message(STATUS "Apple RE helpers enabled — building sm_apple_cpu_latency and sm_apple_cpu_cache_pressure")
