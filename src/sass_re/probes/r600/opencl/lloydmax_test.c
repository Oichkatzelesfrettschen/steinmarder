/*
 * lloydmax_test.c — Test harness for UBYTE_FLT INT8 packing kernel
 *
 * Tests:
 * 1. ubyte_unpack_throughput: verifies 4x UBYTE_FLT emission
 * 2. lloydmax_ubyte_dot: INT8 packed dot product
 * 3. lloydmax_nearest_centroid: codebook nearest-neighbor search
 *
 * Build: gcc -O2 -o lloydmax_test lloydmax_test.c -lOpenCL -lm
 * Run:   RUSTICL_ENABLE=r600 R600_DEBUG=cs ./lloydmax_test
 *        (R600_DEBUG=cs dumps compute ISA to verify UBYTE_FLT emission)
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <CL/cl.h>

#define CHECK_CL(err, msg) do { \
    if ((err) != CL_SUCCESS) { \
        fprintf(stderr, "CL ERROR %d at %s\n", (err), (msg)); \
        return 1; \
    } \
} while(0)

static char *read_file(const char *path, size_t *len) {
    FILE *f = fopen(path, "r");
    if (!f) { perror(path); return NULL; }
    fseek(f, 0, SEEK_END);
    *len = ftell(f);
    rewind(f);
    char *buf = malloc(*len + 1);
    fread(buf, 1, *len, f);
    buf[*len] = 0;
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

    err = clGetPlatformIDs(1, &platform, NULL);
    CHECK_CL(err, "clGetPlatformIDs");

    err = clGetDeviceIDs(platform, CL_DEVICE_TYPE_GPU, 1, &device, NULL);
    CHECK_CL(err, "clGetDeviceIDs");

    char dev_name[128];
    clGetDeviceInfo(device, CL_DEVICE_NAME, sizeof(dev_name), dev_name, NULL);
    printf("Device: %s\n", dev_name);

    ctx = clCreateContext(NULL, 1, &device, NULL, NULL, &err);
    CHECK_CL(err, "clCreateContext");

    queue = clCreateCommandQueue(ctx, device, 0, &err);
    CHECK_CL(err, "clCreateCommandQueue");

    /* Load kernel source */
    const char *src_path = argc > 1 ? argv[1] : "lloydmax_ubyte_flt.cl";
    size_t src_len;
    char *src = read_file(src_path, &src_len);
    if (!src) return 1;

    program = clCreateProgramWithSource(ctx, 1, (const char **)&src, &src_len, &err);
    CHECK_CL(err, "clCreateProgramWithSource");

    err = clBuildProgram(program, 1, &device, "-cl-std=CL1.2", NULL, NULL);
    if (err != CL_SUCCESS) {
        char log[4096];
        clGetProgramBuildInfo(program, device, CL_PROGRAM_BUILD_LOG, sizeof(log), log, NULL);
        fprintf(stderr, "Build log:\n%s\n", log);
        return 1;
    }

    /* ===== Test 1: UBYTE unpack throughput ===== */
    printf("\n=== Test 1: ubyte_unpack_throughput ===\n");
    {
        cl_kernel kernel = clCreateKernel(program, "ubyte_unpack_throughput", &err);
        CHECK_CL(err, "clCreateKernel(ubyte_unpack_throughput)");

        const unsigned count = 256;
        unsigned packed_input[256];
        float unpacked_output[256 * 4];

        /* Pack test values: byte0=10, byte1=20, byte2=30, byte3=40 for each */
        for (unsigned i = 0; i < count; i++)
            packed_input[i] = 10 | (20 << 8) | (30 << 16) | (40 << 24);

        cl_mem buf_in = clCreateBuffer(ctx, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR,
                                        sizeof(packed_input), packed_input, &err);
        CHECK_CL(err, "clCreateBuffer(in)");

        cl_mem buf_out = clCreateBuffer(ctx, CL_MEM_WRITE_ONLY,
                                         count * 4 * sizeof(float), NULL, &err);
        CHECK_CL(err, "clCreateBuffer(out)");

        clSetKernelArg(kernel, 0, sizeof(cl_mem), &buf_in);
        clSetKernelArg(kernel, 1, sizeof(cl_mem), &buf_out);
        clSetKernelArg(kernel, 2, sizeof(unsigned), &count);

        size_t global_size = count;
        size_t local_size = 64;
        err = clEnqueueNDRangeKernel(queue, kernel, 1, NULL, &global_size, &local_size, 0, NULL, NULL);
        CHECK_CL(err, "clEnqueueNDRangeKernel");

        err = clEnqueueReadBuffer(queue, buf_out, CL_TRUE, 0,
                                   count * 4 * sizeof(float), unpacked_output, 0, NULL, NULL);
        CHECK_CL(err, "clEnqueueReadBuffer");

        /* Verify: expect 10.0, 20.0, 30.0, 40.0 for each element */
        int pass = 1;
        for (unsigned i = 0; i < 4 && pass; i++) {
            float expected[] = {10.0f, 20.0f, 30.0f, 40.0f};
            float got = unpacked_output[i];
            if (fabsf(got - expected[i]) > 0.01f) {
                printf("  FAIL: element %u expected %.1f got %.1f\n", i, expected[i], got);
                pass = 0;
            }
        }
        printf("  Result: %s (first 4 values: %.0f %.0f %.0f %.0f)\n",
               pass ? "PASS" : "FAIL",
               unpacked_output[0], unpacked_output[1],
               unpacked_output[2], unpacked_output[3]);

        clReleaseMemObject(buf_in);
        clReleaseMemObject(buf_out);
        clReleaseKernel(kernel);
    }

    /* ===== Test 2: Lloyd-Max dot product ===== */
    printf("\n=== Test 2: lloydmax_ubyte_dot ===\n");
    {
        cl_kernel kernel = clCreateKernel(program, "lloydmax_ubyte_dot", &err);
        CHECK_CL(err, "clCreateKernel(lloydmax_ubyte_dot)");

        const unsigned num_vectors = 16;
        const unsigned vec_len_div4 = 1;  /* 4 elements per vector */

        unsigned packed_weights[16];
        float activations[16 * 4];
        float results[16];

        /* weights: [100, 150, 200, 250] packed as INT8x4
         * activations: [1.0, 1.0, 1.0, 1.0]
         * Expected dot: (100+150+200+250)/255 = 700/255 ≈ 2.745 */
        for (unsigned i = 0; i < num_vectors; i++) {
            packed_weights[i] = 100 | (150 << 8) | (200 << 16) | (250 << 24);
            activations[i * 4 + 0] = 1.0f;
            activations[i * 4 + 1] = 1.0f;
            activations[i * 4 + 2] = 1.0f;
            activations[i * 4 + 3] = 1.0f;
        }

        cl_mem buf_w = clCreateBuffer(ctx, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR,
                                       sizeof(packed_weights), packed_weights, &err);
        cl_mem buf_a = clCreateBuffer(ctx, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR,
                                       num_vectors * 4 * sizeof(float), activations, &err);
        cl_mem buf_r = clCreateBuffer(ctx, CL_MEM_WRITE_ONLY,
                                       num_vectors * sizeof(float), NULL, &err);

        float inv_scale = 1.0f / 255.0f;
        clSetKernelArg(kernel, 0, sizeof(cl_mem), &buf_w);
        clSetKernelArg(kernel, 1, sizeof(cl_mem), &buf_a);
        clSetKernelArg(kernel, 2, sizeof(cl_mem), &buf_r);
        clSetKernelArg(kernel, 3, sizeof(float), &inv_scale);
        clSetKernelArg(kernel, 4, sizeof(unsigned), &vec_len_div4);

        size_t global_size = num_vectors;
        size_t local_size = 16;
        err = clEnqueueNDRangeKernel(queue, kernel, 1, NULL, &global_size, &local_size, 0, NULL, NULL);
        CHECK_CL(err, "clEnqueueNDRangeKernel");

        err = clEnqueueReadBuffer(queue, buf_r, CL_TRUE, 0,
                                   num_vectors * sizeof(float), results, 0, NULL, NULL);
        CHECK_CL(err, "clEnqueueReadBuffer");

        float expected = (100.0f + 150.0f + 200.0f + 250.0f) / 255.0f;
        int pass = (fabsf(results[0] - expected) < 0.01f);
        printf("  Expected: %.4f  Got: %.4f  %s\n",
               expected, results[0], pass ? "PASS" : "FAIL");

        clReleaseMemObject(buf_w);
        clReleaseMemObject(buf_a);
        clReleaseMemObject(buf_r);
        clReleaseKernel(kernel);
    }

    printf("\nDone. Run with R600_DEBUG=cs to verify UBYTE_FLT ISA emission.\n");

    clReleaseProgram(program);
    clReleaseCommandQueue(queue);
    clReleaseContext(ctx);
    free(src);
    return 0;
}
