#define _POSIX_C_SOURCE 200809L
#define CL_TARGET_OPENCL_VERSION 120

#include <CL/cl.h>

#include <errno.h>
#include <inttypes.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

typedef enum OutputMode {
    OUTPUT_SCALAR_F32 = 0,
    OUTPUT_VEC4_F32 = 1,
} OutputMode;

typedef struct Options {
    const char *source_path;
    const char *kernel_name;
    const char *platform_substr;
    const char *device_substr;
    const char *build_options;
    size_t elements;
    size_t local_size;
    OutputMode mode;
} Options;

static void usage(const char *argv0) {
    fprintf(stderr,
            "usage: %s --source path.cl --kernel name "
            "[--elements N] [--local-size N] [--mode scalar_f32|vec4_f32] "
            "[--platform-substr name] [--device-substr name] "
            "[--build-options opts]\n",
            argv0);
}

static uint64_t monotonic_ns(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000000000ull + (uint64_t)ts.tv_nsec;
}

static int parse_mode(const char *arg, OutputMode *out) {
    if (strcmp(arg, "scalar_f32") == 0) {
        *out = OUTPUT_SCALAR_F32;
        return 1;
    }
    if (strcmp(arg, "vec4_f32") == 0) {
        *out = OUTPUT_VEC4_F32;
        return 1;
    }
    return 0;
}

static int parse_options(int argc, char **argv, Options *opts) {
    int i;

    memset(opts, 0, sizeof(*opts));
    opts->elements = 4096;
    opts->local_size = 64;
    opts->mode = OUTPUT_SCALAR_F32;
    opts->build_options = "";

    for (i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "--source") == 0 && i + 1 < argc) {
            opts->source_path = argv[++i];
        } else if (strcmp(argv[i], "--kernel") == 0 && i + 1 < argc) {
            opts->kernel_name = argv[++i];
        } else if (strcmp(argv[i], "--elements") == 0 && i + 1 < argc) {
            opts->elements = (size_t)strtoull(argv[++i], NULL, 10);
        } else if (strcmp(argv[i], "--local-size") == 0 && i + 1 < argc) {
            opts->local_size = (size_t)strtoull(argv[++i], NULL, 10);
        } else if (strcmp(argv[i], "--mode") == 0 && i + 1 < argc) {
            if (!parse_mode(argv[++i], &opts->mode)) {
                fprintf(stderr, "unknown mode: %s\n", argv[i]);
                return 0;
            }
        } else if (strcmp(argv[i], "--platform-substr") == 0 && i + 1 < argc) {
            opts->platform_substr = argv[++i];
        } else if (strcmp(argv[i], "--device-substr") == 0 && i + 1 < argc) {
            opts->device_substr = argv[++i];
        } else if (strcmp(argv[i], "--build-options") == 0 && i + 1 < argc) {
            opts->build_options = argv[++i];
        } else {
            usage(argv[0]);
            return 0;
        }
    }

    if (!opts->source_path || !opts->kernel_name || opts->elements == 0 || opts->local_size == 0) {
        usage(argv[0]);
        return 0;
    }
    return 1;
}

static char *read_text(const char *path, size_t *size_out) {
    FILE *fp = fopen(path, "rb");
    long size;
    char *data;

    if (!fp) {
        fprintf(stderr, "failed to open %s: %s\n", path, strerror(errno));
        return NULL;
    }
    if (fseek(fp, 0, SEEK_END) != 0) {
        fclose(fp);
        return NULL;
    }
    size = ftell(fp);
    if (size < 0) {
        fclose(fp);
        return NULL;
    }
    rewind(fp);

    data = (char *)malloc((size_t)size + 1);
    if (!data) {
        fclose(fp);
        return NULL;
    }
    if (fread(data, 1, (size_t)size, fp) != (size_t)size) {
        fprintf(stderr, "failed to read %s\n", path);
        free(data);
        fclose(fp);
        return NULL;
    }
    data[size] = '\0';
    fclose(fp);
    *size_out = (size_t)size;
    return data;
}

static int contains_substr(const char *haystack, const char *needle) {
    if (!needle || !*needle) {
        return 1;
    }
    return strstr(haystack, needle) != NULL;
}

static int pick_platform_device(const Options *opts,
                                cl_platform_id *platform_out,
                                cl_device_id *device_out,
                                char *platform_name,
                                size_t platform_name_size,
                                char *device_name,
                                size_t device_name_size) {
    cl_platform_id platforms[16];
    cl_uint num_platforms = 0;
    cl_int clerr = clGetPlatformIDs(16, platforms, &num_platforms);
    cl_uint i;

    if (clerr != CL_SUCCESS || num_platforms == 0) {
        fprintf(stderr, "clGetPlatformIDs failed: %d\n", clerr);
        return 0;
    }

    for (i = 0; i < num_platforms; ++i) {
        cl_device_id devices[16];
        cl_uint num_devices = 0;
        char current_platform[256] = {0};
        size_t ignored = 0;
        clGetPlatformInfo(platforms[i], CL_PLATFORM_NAME, sizeof(current_platform), current_platform, &ignored);
        if (!contains_substr(current_platform, opts->platform_substr)) {
            continue;
        }

        clerr = clGetDeviceIDs(platforms[i], CL_DEVICE_TYPE_GPU, 16, devices, &num_devices);
        if (clerr != CL_SUCCESS || num_devices == 0) {
            continue;
        }

        for (cl_uint j = 0; j < num_devices; ++j) {
            char current_device[256] = {0};
            clGetDeviceInfo(devices[j], CL_DEVICE_NAME, sizeof(current_device), current_device, &ignored);
            if (!contains_substr(current_device, opts->device_substr)) {
                continue;
            }
            snprintf(platform_name, platform_name_size, "%s", current_platform);
            snprintf(device_name, device_name_size, "%s", current_device);
            *platform_out = platforms[i];
            *device_out = devices[j];
            return 1;
        }
    }

    fprintf(stderr, "failed to find matching GPU device\n");
    return 0;
}

static void print_build_log(cl_program program, cl_device_id device) {
    size_t log_size = 0;
    if (clGetProgramBuildInfo(program, device, CL_PROGRAM_BUILD_LOG, 0, NULL, &log_size) != CL_SUCCESS ||
        log_size == 0) {
        return;
    }
    char *log = (char *)malloc(log_size + 1);
    if (!log) {
        return;
    }
    if (clGetProgramBuildInfo(program, device, CL_PROGRAM_BUILD_LOG, log_size, log, NULL) == CL_SUCCESS) {
        log[log_size] = '\0';
        fprintf(stderr, "%s\n", log);
    }
    free(log);
}

static float safe_sample(const float *values, size_t float_count, size_t index) {
    float value;
    if (index >= float_count) {
        return 0.0f;
    }
    value = values[index];
    if (isnan(value) || isinf(value)) {
        return 0.0f;
    }
    return value;
}

static void print_result_json(const Options *opts,
                              const char *platform_name,
                              const char *device_name,
                              uint64_t build_ns,
                              uint64_t kernel_ns,
                              uint64_t read_ns,
                              size_t workgroups_x,
                              const float *values,
                              size_t float_count) {
    double checksum = 0.0;
    size_t nan_count = 0;
    size_t inf_count = 0;
    size_t i;
    const char *mode = opts->mode == OUTPUT_VEC4_F32 ? "vec4_f32" : "scalar_f32";
    for (i = 0; i < float_count; ++i) {
        if (isnan(values[i])) {
            nan_count += 1;
            continue;
        }
        if (isinf(values[i])) {
            inf_count += 1;
            continue;
        }
        checksum += (double)values[i];
    }

    printf("{\n");
    printf("  \"source_path\": \"%s\",\n", opts->source_path);
    printf("  \"kernel_name\": \"%s\",\n", opts->kernel_name);
    printf("  \"platform_name\": \"%s\",\n", platform_name);
    printf("  \"device_name\": \"%s\",\n", device_name);
    printf("  \"mode\": \"%s\",\n", mode);
    printf("  \"elements\": %" PRIu64 ",\n", (uint64_t)opts->elements);
    printf("  \"local_size\": %" PRIu64 ",\n", (uint64_t)opts->local_size);
    printf("  \"workgroups_x\": %" PRIu64 ",\n", (uint64_t)workgroups_x);
    printf("  \"build_options\": \"%s\",\n", opts->build_options);
    printf("  \"build_ns\": %" PRIu64 ",\n", build_ns);
    printf("  \"kernel_ns\": %" PRIu64 ",\n", kernel_ns);
    printf("  \"read_ns\": %" PRIu64 ",\n", read_ns);
    printf("  \"checksum\": %.9f,\n", checksum);
    printf("  \"nan_count\": %" PRIu64 ",\n", (uint64_t)nan_count);
    printf("  \"inf_count\": %" PRIu64 ",\n", (uint64_t)inf_count);
    printf("  \"first\": [%.9f, %.9f, %.9f, %.9f],\n",
           safe_sample(values, float_count, 0),
           safe_sample(values, float_count, 1),
           safe_sample(values, float_count, 2),
           safe_sample(values, float_count, 3));
    printf("  \"last\": [%.9f, %.9f, %.9f, %.9f]\n",
           float_count > 4 ? safe_sample(values, float_count, float_count - 4) : (float_count > 0 ? safe_sample(values, float_count, float_count - 1) : 0.0f),
           float_count > 3 ? safe_sample(values, float_count, float_count - 3) : 0.0f,
           float_count > 2 ? safe_sample(values, float_count, float_count - 2) : 0.0f,
           float_count > 1 ? safe_sample(values, float_count, float_count - 1) : 0.0f);
    printf("}\n");
}

int main(int argc, char **argv) {
    Options opts;
    cl_platform_id platform = NULL;
    cl_device_id device = NULL;
    cl_context context = NULL;
    cl_command_queue queue = NULL;
    cl_program program = NULL;
    cl_kernel kernel = NULL;
    cl_mem output = NULL;
    char platform_name[256] = {0};
    char device_name[256] = {0};
    size_t source_size = 0;
    char *source = NULL;
    cl_int clerr;
    uint64_t build_start_ns;
    uint64_t build_end_ns;
    uint64_t kernel_start_ns;
    uint64_t kernel_end_ns;
    uint64_t read_start_ns;
    uint64_t read_end_ns;
    size_t bytes_per_element = 0;
    size_t buffer_bytes;
    size_t global = 0;
    size_t local = 0;
    size_t workgroups_x = 0;
    size_t float_count;
    float *readback = NULL;

    if (!parse_options(argc, argv, &opts)) {
        return 2;
    }

    source = read_text(opts.source_path, &source_size);
    if (!source) {
        return 1;
    }

    if (!pick_platform_device(&opts, &platform, &device, platform_name, sizeof(platform_name),
                              device_name, sizeof(device_name))) {
        free(source);
        return 1;
    }

    context = clCreateContext(NULL, 1, &device, NULL, NULL, &clerr);
    if (clerr != CL_SUCCESS) {
        fprintf(stderr, "clCreateContext failed: %d\n", clerr);
        free(source);
        return 1;
    }

    queue = clCreateCommandQueue(context, device, 0, &clerr);
    if (clerr != CL_SUCCESS) {
        fprintf(stderr, "clCreateCommandQueue failed: %d\n", clerr);
        goto cleanup;
    }

    build_start_ns = monotonic_ns();
    program = clCreateProgramWithSource(context, 1, (const char **)&source, &source_size, &clerr);
    if (clerr != CL_SUCCESS) {
        fprintf(stderr, "clCreateProgramWithSource failed: %d\n", clerr);
        goto cleanup;
    }
    clerr = clBuildProgram(program, 1, &device, opts.build_options, NULL, NULL);
    build_end_ns = monotonic_ns();
    if (clerr != CL_SUCCESS) {
        fprintf(stderr, "clBuildProgram failed: %d\n", clerr);
        print_build_log(program, device);
        goto cleanup;
    }

    kernel = clCreateKernel(program, opts.kernel_name, &clerr);
    if (clerr != CL_SUCCESS) {
        fprintf(stderr, "clCreateKernel(%s) failed: %d\n", opts.kernel_name, clerr);
        goto cleanup;
    }

    bytes_per_element = opts.mode == OUTPUT_VEC4_F32 ? sizeof(float) * 4 : sizeof(float);
    buffer_bytes = opts.elements * bytes_per_element;
    output = clCreateBuffer(context, CL_MEM_READ_WRITE, buffer_bytes, NULL, &clerr);
    if (clerr != CL_SUCCESS) {
        fprintf(stderr, "clCreateBuffer failed: %d\n", clerr);
        goto cleanup;
    }

    clerr = clSetKernelArg(kernel, 0, sizeof(output), &output);
    if (clerr != CL_SUCCESS) {
        fprintf(stderr, "clSetKernelArg failed: %d\n", clerr);
        goto cleanup;
    }

    global = opts.elements;
    local = opts.local_size;
    workgroups_x = (global + local - 1) / local;
    kernel_start_ns = monotonic_ns();
    clerr = clEnqueueNDRangeKernel(queue, kernel, 1, NULL, &global, &local, 0, NULL, NULL);
    if (clerr != CL_SUCCESS) {
        fprintf(stderr, "clEnqueueNDRangeKernel failed: %d\n", clerr);
        goto cleanup;
    }
    clerr = clFinish(queue);
    kernel_end_ns = monotonic_ns();
    if (clerr != CL_SUCCESS) {
        fprintf(stderr, "clFinish failed: %d\n", clerr);
        goto cleanup;
    }

    readback = (float *)malloc(buffer_bytes);
    if (!readback) {
        fprintf(stderr, "failed to allocate readback buffer\n");
        goto cleanup;
    }

    read_start_ns = monotonic_ns();
    clerr = clEnqueueReadBuffer(queue, output, CL_TRUE, 0, buffer_bytes, readback, 0, NULL, NULL);
    read_end_ns = monotonic_ns();
    if (clerr != CL_SUCCESS) {
        fprintf(stderr, "clEnqueueReadBuffer failed: %d\n", clerr);
        goto cleanup;
    }

    float_count = buffer_bytes / sizeof(float);
    print_result_json(&opts, platform_name, device_name,
                      build_end_ns - build_start_ns,
                      kernel_end_ns - kernel_start_ns,
                      read_end_ns - read_start_ns,
                      workgroups_x,
                      readback, float_count);

cleanup:
    free(readback);
    if (output) {
        clReleaseMemObject(output);
    }
    if (kernel) {
        clReleaseKernel(kernel);
    }
    if (program) {
        clReleaseProgram(program);
    }
    if (queue) {
        clReleaseCommandQueue(queue);
    }
    if (context) {
        clReleaseContext(context);
    }
    free(source);
    return clerr == CL_SUCCESS ? 0 : 1;
}
