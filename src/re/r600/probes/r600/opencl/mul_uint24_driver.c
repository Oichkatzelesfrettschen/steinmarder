/*
 * mul_uint24_driver.c — Test driver for MUL_UINT24 emission verification
 *
 * Build: gcc -O2 -o mul_uint24_driver mul_uint24_driver.c -lOpenCL -lm
 * Run:   RUSTICL_ENABLE=r600 R600_DEBUG=cs ./mul_uint24_driver mul_uint24_test.cl
 *        grep 'MUL_UINT24\|MULLO_INT' in stderr to verify emission
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <CL/cl.h>

#define CHECK_CL(err, msg) do { \
    if ((err) != CL_SUCCESS) { fprintf(stderr, "CL ERROR %d: %s\n", (err), (msg)); return 1; } \
} while(0)

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

    const char *src_path = argc > 1 ? argv[1] : "mul_uint24_test.cl";
    size_t src_len;
    char *src = read_file(src_path, &src_len);
    if (!src) return 1;

    program = clCreateProgramWithSource(ctx, 1, (const char **)&src, &src_len, &err);
    CHECK_CL(err, "create_program");
    err = clBuildProgram(program, 1, &device, "-cl-std=CL1.2", NULL, NULL);
    if (err != CL_SUCCESS) {
        char log[4096];
        clGetProgramBuildInfo(program, device, CL_PROGRAM_BUILD_LOG, sizeof(log), log, NULL);
        fprintf(stderr, "Build log:\n%s\n", log);
        return 1;
    }

    /* === Test 1: mul_uint24_basic === */
    printf("\n=== Test 1: mul_uint24_basic (input * 100) ===\n");
    {
        cl_kernel kernel = clCreateKernel(program, "mul_uint24_basic", &err);
        CHECK_CL(err, "kernel(mul_uint24_basic)");

        const unsigned count = 16;
        unsigned input[16], output[16];
        for (unsigned i = 0; i < count; i++) input[i] = i + 1; /* 1..16 */

        cl_mem buf_in = clCreateBuffer(ctx, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR,
                                        sizeof(input), input, &err);
        cl_mem buf_out = clCreateBuffer(ctx, CL_MEM_WRITE_ONLY, sizeof(output), NULL, &err);

        unsigned scale = 100; /* < 2^24 → should trigger MUL_UINT24 */
        clSetKernelArg(kernel, 0, sizeof(cl_mem), &buf_in);
        clSetKernelArg(kernel, 1, sizeof(cl_mem), &buf_out);
        clSetKernelArg(kernel, 2, sizeof(unsigned), &scale);

        size_t global_size = count, local_size = 16;
        err = clEnqueueNDRangeKernel(queue, kernel, 1, NULL, &global_size, &local_size, 0, NULL, NULL);
        CHECK_CL(err, "enqueue");
        err = clEnqueueReadBuffer(queue, buf_out, CL_TRUE, 0, sizeof(output), output, 0, NULL, NULL);
        CHECK_CL(err, "read");

        int pass = 1;
        for (unsigned i = 0; i < 4; i++) {
            unsigned expected = (i + 1) * 100;
            if (output[i] != expected) {
                printf("  FAIL: [%u] expected %u got %u\n", i, expected, output[i]);
                pass = 0;
            }
        }
        printf("  First 4: %u %u %u %u  %s\n",
               output[0], output[1], output[2], output[3], pass ? "PASS" : "FAIL");

        clReleaseMemObject(buf_in);
        clReleaseMemObject(buf_out);
        clReleaseKernel(kernel);
    }

    /* === Test 2: Q4.8 fixed-point multiply === */
    printf("\n=== Test 2: q4_8_fixed_point_mul (1.5 * 2.0 = 3.0) ===\n");
    {
        cl_kernel kernel = clCreateKernel(program, "q4_8_fixed_point_mul", &err);
        CHECK_CL(err, "kernel(q4_8_fixed_point_mul)");

        const unsigned count = 4;
        /* Q4.8 encoding: value * 256 */
        unsigned a_fixed[] = {384, 512, 256, 128}; /* 1.5, 2.0, 1.0, 0.5 */
        unsigned b_fixed[] = {512, 256, 384, 384}; /* 2.0, 1.0, 1.5, 1.5 */
        unsigned result_fixed[4];
        float result_float[4];

        cl_mem buf_a = clCreateBuffer(ctx, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR,
                                       sizeof(a_fixed), a_fixed, &err);
        cl_mem buf_b = clCreateBuffer(ctx, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR,
                                       sizeof(b_fixed), b_fixed, &err);
        cl_mem buf_rf = clCreateBuffer(ctx, CL_MEM_WRITE_ONLY, sizeof(result_fixed), NULL, &err);
        cl_mem buf_ff = clCreateBuffer(ctx, CL_MEM_WRITE_ONLY, sizeof(result_float), NULL, &err);

        clSetKernelArg(kernel, 0, sizeof(cl_mem), &buf_a);
        clSetKernelArg(kernel, 1, sizeof(cl_mem), &buf_b);
        clSetKernelArg(kernel, 2, sizeof(cl_mem), &buf_rf);
        clSetKernelArg(kernel, 3, sizeof(cl_mem), &buf_ff);

        size_t global_size = count, local_size = 4;
        err = clEnqueueNDRangeKernel(queue, kernel, 1, NULL, &global_size, &local_size, 0, NULL, NULL);
        CHECK_CL(err, "enqueue");
        clEnqueueReadBuffer(queue, buf_rf, CL_TRUE, 0, sizeof(result_fixed), result_fixed, 0, NULL, NULL);
        clEnqueueReadBuffer(queue, buf_ff, CL_TRUE, 0, sizeof(result_float), result_float, 0, NULL, NULL);

        /* Expected: 1.5*2.0=3.0, 2.0*1.0=2.0, 1.0*1.5=1.5, 0.5*1.5=0.75 */
        float expected[] = {3.0f, 2.0f, 1.5f, 0.75f};
        int pass = 1;
        for (unsigned i = 0; i < count; i++) {
            if (fabsf(result_float[i] - expected[i]) > 0.01f) {
                printf("  FAIL: [%u] expected %.2f got %.2f (fixed: %u)\n",
                       i, expected[i], result_float[i], result_fixed[i]);
                pass = 0;
            }
        }
        printf("  Results: %.2f %.2f %.2f %.2f  %s\n",
               result_float[0], result_float[1], result_float[2], result_float[3],
               pass ? "PASS" : "FAIL");

        clReleaseMemObject(buf_a);
        clReleaseMemObject(buf_b);
        clReleaseMemObject(buf_rf);
        clReleaseMemObject(buf_ff);
        clReleaseKernel(kernel);
    }

    printf("\nDone. Check R600_DEBUG=cs output for MUL_UINT24 vs MULLO_INT.\n");

    clReleaseProgram(program);
    clReleaseCommandQueue(queue);
    clReleaseContext(ctx);
    free(src);
    return 0;
}
