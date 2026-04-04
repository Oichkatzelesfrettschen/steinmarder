/*
 * atomic_cas_driver.c — Test driver for LDS/global atomic CAS probes
 *
 * Build: gcc -O2 -o atomic_cas_driver atomic_cas_driver.c -lOpenCL -lm
 * Run:   RUSTICL_ENABLE=r600 ./atomic_cas_driver atomic_cas_throughput.cl
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <CL/cl.h>

#define CHECK_CL(err, msg) do { \
    if ((err) != CL_SUCCESS) { fprintf(stderr, "CL ERROR %d: %s\n", (err), (msg)); return 1; } \
} while(0)

#define NUM_ITERATIONS 1024
#define REPS 50

static char *read_file(const char *path, size_t *len) {
    FILE *f = fopen(path, "r");
    if (!f) { perror(path); return NULL; }
    fseek(f, 0, SEEK_END); *len = ftell(f); rewind(f);
    char *buf = malloc(*len + 1);
    fread(buf, 1, *len, f); buf[*len] = 0;
    fclose(f);
    return buf;
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

    const char *src_path = argc > 1 ? argv[1] : "atomic_cas_throughput.cl";
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

    /* === Test 1: LDS atomic CAS latency (dependent chain) === */
    printf("\n=== LDS atomic CAS latency (dependent chain, %d iterations) ===\n", NUM_ITERATIONS);
    {
        cl_kernel kernel = clCreateKernel(program, "lds_atomic_cas_latency", &err);
        if (err != CL_SUCCESS) {
            printf("  SKIP: kernel not available (err=%d)\n", err);
        } else {
            unsigned result = 0;
            cl_mem buf_result = clCreateBuffer(ctx, CL_MEM_WRITE_ONLY, sizeof(unsigned), NULL, &err);
            unsigned num_iter = NUM_ITERATIONS;

            clSetKernelArg(kernel, 0, sizeof(cl_mem), &buf_result);
            clSetKernelArg(kernel, 1, sizeof(unsigned), NULL);  /* __local */
            clSetKernelArg(kernel, 2, sizeof(unsigned), &num_iter);

            size_t global_size = 64, local_size = 64;

            /* Warmup */
            clEnqueueNDRangeKernel(queue, kernel, 1, NULL, &global_size, &local_size, 0, NULL, NULL);
            clFinish(queue);

            struct timespec t0, t1;
            clock_gettime(CLOCK_MONOTONIC, &t0);
            for (int r = 0; r < REPS; r++)
                clEnqueueNDRangeKernel(queue, kernel, 1, NULL, &global_size, &local_size, 0, NULL, NULL);
            clFinish(queue);
            clock_gettime(CLOCK_MONOTONIC, &t1);

            double elapsed_ms = (t1.tv_sec - t0.tv_sec) * 1000.0 + (t1.tv_nsec - t0.tv_nsec) / 1e6;
            double total_ops = (double)REPS * NUM_ITERATIONS;
            double ns_per_op = (elapsed_ms * 1e6) / total_ops;
            double cyc_per_op = ns_per_op / 2.053;

            clEnqueueReadBuffer(queue, buf_result, CL_TRUE, 0, sizeof(unsigned), &result, 0, NULL, NULL);

            printf("  Wall: %.2f ms (%d reps)\n", elapsed_ms, REPS);
            printf("  Per CAS: %.1f ns = %.1f cycles @ 487 MHz\n", ns_per_op, cyc_per_op);
            printf("  Result: %u (expected: %u) %s\n", result, num_iter,
                   result == num_iter ? "PASS" : "FAIL");

            clReleaseMemObject(buf_result);
            clReleaseKernel(kernel);
        }
    }

    /* === Test 2: Global atomic CAS latency === */
    printf("\n=== Global atomic CAS latency (dependent chain, %d iterations) ===\n", NUM_ITERATIONS);
    {
        cl_kernel kernel = clCreateKernel(program, "global_atomic_cas_latency", &err);
        if (err != CL_SUCCESS) {
            printf("  SKIP: kernel not available (err=%d)\n", err);
        } else {
            unsigned result = 0;
            unsigned atomic_init = 0;
            cl_mem buf_atomic = clCreateBuffer(ctx, CL_MEM_READ_WRITE | CL_MEM_COPY_HOST_PTR,
                                                sizeof(unsigned), &atomic_init, &err);
            cl_mem buf_result = clCreateBuffer(ctx, CL_MEM_WRITE_ONLY, sizeof(unsigned), NULL, &err);
            unsigned num_iter = NUM_ITERATIONS;

            clSetKernelArg(kernel, 0, sizeof(cl_mem), &buf_atomic);
            clSetKernelArg(kernel, 1, sizeof(cl_mem), &buf_result);
            clSetKernelArg(kernel, 2, sizeof(unsigned), &num_iter);

            size_t global_size = 1, local_size = 1;

            /* Warmup */
            clEnqueueNDRangeKernel(queue, kernel, 1, NULL, &global_size, &local_size, 0, NULL, NULL);
            clFinish(queue);

            /* Reset atomic var */
            clEnqueueWriteBuffer(queue, buf_atomic, CL_TRUE, 0, sizeof(unsigned), &atomic_init, 0, NULL, NULL);

            struct timespec t0, t1;
            clock_gettime(CLOCK_MONOTONIC, &t0);
            for (int r = 0; r < REPS; r++) {
                clEnqueueWriteBuffer(queue, buf_atomic, CL_FALSE, 0, sizeof(unsigned), &atomic_init, 0, NULL, NULL);
                clEnqueueNDRangeKernel(queue, kernel, 1, NULL, &global_size, &local_size, 0, NULL, NULL);
            }
            clFinish(queue);
            clock_gettime(CLOCK_MONOTONIC, &t1);

            double elapsed_ms = (t1.tv_sec - t0.tv_sec) * 1000.0 + (t1.tv_nsec - t0.tv_nsec) / 1e6;
            double total_ops = (double)REPS * NUM_ITERATIONS;
            double ns_per_op = (elapsed_ms * 1e6) / total_ops;
            double cyc_per_op = ns_per_op / 2.053;

            clEnqueueReadBuffer(queue, buf_result, CL_TRUE, 0, sizeof(unsigned), &result, 0, NULL, NULL);

            printf("  Wall: %.2f ms (%d reps)\n", elapsed_ms, REPS);
            printf("  Per CAS: %.1f ns = %.1f cycles @ 487 MHz\n", ns_per_op, cyc_per_op);
            printf("  Result: %u (expected: %u) %s\n", result, num_iter,
                   result == num_iter ? "PASS" : "FAIL");

            clReleaseMemObject(buf_atomic);
            clReleaseMemObject(buf_result);
            clReleaseKernel(kernel);
        }
    }

    printf("\nDone.\n");
    clReleaseProgram(program);
    clReleaseCommandQueue(queue);
    clReleaseContext(ctx);
    free(src);
    return 0;
}
