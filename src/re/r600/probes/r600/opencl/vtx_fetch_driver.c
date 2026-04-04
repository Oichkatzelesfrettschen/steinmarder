/*
 * vtx_fetch_driver.c — Driver for global memory / VTX fetch latency probe
 *
 * Measures pointer-chase latency through global memory on TeraScale-2.
 * Both vertex fetches and compute global reads use the Texture Cache (TC).
 *
 * Build: gcc -O2 -o vtx_fetch_driver vtx_fetch_driver.c -lOpenCL -lm
 * Run:   RUSTICL_ENABLE=r600 ./vtx_fetch_driver vtx_fetch_latency.cl
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <CL/cl.h>

#define CHECK_CL(err, msg) do { \
    if ((err) != CL_SUCCESS) { fprintf(stderr, "CL ERROR %d: %s\n", (err), (msg)); return 1; } \
} while(0)

#define CHAIN_SIZE 1024
#define NUM_HOPS 256
#define NUM_WORK_ITEMS 64

static char *read_file(const char *path, size_t *len) {
    FILE *f = fopen(path, "r");
    if (!f) { perror(path); return NULL; }
    fseek(f, 0, SEEK_END); *len = ftell(f); rewind(f);
    char *buf = malloc(*len + 1);
    fread(buf, 1, *len, f); buf[*len] = 0;
    fclose(f);
    return buf;
}

/* Build a random permutation chain: each element points to another */
static void build_chain(unsigned *chain, unsigned size) {
    /* Simple pseudo-random chain using LCG */
    for (unsigned i = 0; i < size; i++)
        chain[i] = (i * 7 + 13) % size;
}

int main(int argc, char **argv) {
    cl_int err;
    cl_platform_id platform;
    cl_device_id device;
    cl_context ctx;
    cl_command_queue queue;
    cl_program program;

    err = clGetPlatformIDs(1, &platform, NULL); CHECK_CL(err, "platform");
    err = clGetDeviceIDs(platform, CL_DEVICE_TYPE_GPU, 1, &device, NULL); CHECK_CL(err, "device");

    char dev_name[128];
    clGetDeviceInfo(device, CL_DEVICE_NAME, sizeof(dev_name), dev_name, NULL);
    printf("Device: %s\n", dev_name);

    ctx = clCreateContext(NULL, 1, &device, NULL, NULL, &err); CHECK_CL(err, "context");
    queue = clCreateCommandQueue(ctx, device, 0, &err); CHECK_CL(err, "queue");

    const char *src_path = argc > 1 ? argv[1] : "vtx_fetch_latency.cl";
    size_t src_len;
    char *src = read_file(src_path, &src_len);
    if (!src) return 1;

    program = clCreateProgramWithSource(ctx, 1, (const char **)&src, &src_len, &err);
    CHECK_CL(err, "program");
    err = clBuildProgram(program, 1, &device, "-cl-std=CL1.2", NULL, NULL);
    if (err != CL_SUCCESS) {
        char log[4096];
        clGetProgramBuildInfo(program, device, CL_PROGRAM_BUILD_LOG, sizeof(log), log, NULL);
        fprintf(stderr, "Build: %s\n", log);
        return 1;
    }

    /* === Pointer Chase Test === */
    printf("\n=== Pointer Chase: %d hops through %d-element chain ===\n", NUM_HOPS, CHAIN_SIZE);
    {
        cl_kernel kernel = clCreateKernel(program, "pointer_chase", &err);
        CHECK_CL(err, "kernel");

        unsigned chain[CHAIN_SIZE];
        build_chain(chain, CHAIN_SIZE);
        unsigned results[NUM_WORK_ITEMS];
        memset(results, 0, sizeof(results));

        cl_mem buf_chain = clCreateBuffer(ctx, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR,
                                           sizeof(chain), chain, &err);
        cl_mem buf_result = clCreateBuffer(ctx, CL_MEM_WRITE_ONLY,
                                            sizeof(results), NULL, &err);

        unsigned num_hops = NUM_HOPS;
        clSetKernelArg(kernel, 0, sizeof(cl_mem), &buf_chain);
        clSetKernelArg(kernel, 1, sizeof(cl_mem), &buf_result);
        clSetKernelArg(kernel, 2, sizeof(unsigned), &num_hops);

        size_t global_size = NUM_WORK_ITEMS;
        size_t local_size = NUM_WORK_ITEMS;

        /* Warmup */
        clEnqueueNDRangeKernel(queue, kernel, 1, NULL, &global_size, &local_size, 0, NULL, NULL);
        clFinish(queue);

        /* Timed run (wall clock — GPU timer not available via Rusticl) */
        struct timespec t0, t1;
        clock_gettime(CLOCK_MONOTONIC, &t0);

        for (int rep = 0; rep < 100; rep++) {
            clEnqueueNDRangeKernel(queue, kernel, 1, NULL, &global_size, &local_size, 0, NULL, NULL);
        }
        clFinish(queue);

        clock_gettime(CLOCK_MONOTONIC, &t1);
        double elapsed_ms = (t1.tv_sec - t0.tv_sec) * 1000.0 +
                            (t1.tv_nsec - t0.tv_nsec) / 1e6;

        clEnqueueReadBuffer(queue, buf_result, CL_TRUE, 0, sizeof(results), results, 0, NULL, NULL);

        /* At 487 MHz shader clock, 1 cycle = 2.053 ns */
        double total_hops = 100.0 * NUM_HOPS * NUM_WORK_ITEMS;
        double ns_per_hop = (elapsed_ms * 1e6) / total_hops;
        double cycles_per_hop = ns_per_hop / 2.053;

        printf("  Wall time: %.2f ms for 100 dispatches\n", elapsed_ms);
        printf("  Total hops: %.0f\n", total_hops);
        printf("  Per-hop: %.1f ns = %.1f cycles @ 487 MHz\n", ns_per_hop, cycles_per_hop);
        printf("  Result[0]: %u (chain integrity check)\n", results[0]);

        /* Comparison: TEX L1 hit = ~38 cycles, TEX L2 = ~200 cycles */
        if (cycles_per_hop < 60)
            printf("  Classification: TC L1 HIT (< 60 cyc)\n");
        else if (cycles_per_hop < 300)
            printf("  Classification: TC L2 HIT (60-300 cyc)\n");
        else
            printf("  Classification: DRAM (> 300 cyc)\n");

        clReleaseMemObject(buf_chain);
        clReleaseMemObject(buf_result);
        clReleaseKernel(kernel);
    }

    printf("\nDone.\n");
    clReleaseProgram(program);
    clReleaseCommandQueue(queue);
    clReleaseContext(ctx);
    free(src);
    return 0;
}
