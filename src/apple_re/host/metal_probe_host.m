#import <Foundation/Foundation.h>
#import <Metal/Metal.h>

#include <inttypes.h>
#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

static uint64_t now_ns(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000000000ull + (uint64_t)ts.tv_nsec;
}

static uint64_t checksum_u32(const uint32_t *data, uint32_t count) {
    uint64_t acc = 1469598103934665603ull;
    for (uint32_t i = 0; i < count; ++i) {
        acc ^= (uint64_t)data[i];
        acc *= 1099511628211ull;
    }
    return acc;
}

static void emit_fs_probe_event(const char *path, uint32_t iteration) {
    FILE *fp = fopen(path, "a");
    if (fp != NULL) {
        fprintf(fp, "iter=%u\n", iteration);
        fclose(fp);
    }

    fp = fopen(path, "r");
    if (fp != NULL) {
        char scratch[64];
        (void)fread(scratch, 1, sizeof(scratch), fp);
        fclose(fp);
    }

    if ((iteration & 0x1ffu) == 0u) {
        if (unlink(path) != 0 && errno != ENOENT) {
            /* best effort only */
        }
    }
}

int main(int argc, char **argv) {
    @autoreleasepool {
        const char *metallib_path = NULL;
        const char *kernel_name = "probe_simdgroup_reduce";
        uint32_t width = 1024;
        uint32_t iterations = 100;
        uint32_t fs_probe_every = 0;
        int csv = 0;

        for (int i = 1; i < argc; ++i) {
            if (strcmp(argv[i], "--metallib") == 0 && i + 1 < argc) {
                metallib_path = argv[++i];
            } else if (strcmp(argv[i], "--kernel") == 0 && i + 1 < argc) {
                kernel_name = argv[++i];
            } else if (strcmp(argv[i], "--width") == 0 && i + 1 < argc) {
                width = (uint32_t)strtoul(argv[++i], NULL, 10);
            } else if (strcmp(argv[i], "--iters") == 0 && i + 1 < argc) {
                iterations = (uint32_t)strtoul(argv[++i], NULL, 10);
            } else if (strcmp(argv[i], "--fs-probe-every") == 0 && i + 1 < argc) {
                fs_probe_every = (uint32_t)strtoul(argv[++i], NULL, 10);
            } else if (strcmp(argv[i], "--fs-probe") == 0) {
                fs_probe_every = 128;
            } else if (strcmp(argv[i], "--csv") == 0) {
                csv = 1;
            } else if (strcmp(argv[i], "--help") == 0) {
                fprintf(stdout, "usage: %s [--metallib path] [--kernel name] [--width N] [--iters N] [--fs-probe|--fs-probe-every N] [--csv]\n", argv[0]);
                return 0;
            } else {
                fprintf(stderr, "unknown argument: %s\n", argv[i]);
                return 2;
            }
        }

        if (metallib_path == NULL) {
            fprintf(stderr, "missing --metallib path\n");
            return 2;
        }
        if (width == 0 || iterations == 0) {
            fprintf(stderr, "width and iterations must be > 0\n");
            return 2;
        }

        id<MTLDevice> device = MTLCreateSystemDefaultDevice();
        if (device == nil) {
            fprintf(stderr, "no Metal device available\n");
            return 3;
        }

        NSError *error = nil;
        NSURL *libURL = [NSURL fileURLWithPath:[NSString stringWithUTF8String:metallib_path]];
        id<MTLLibrary> library = [device newLibraryWithURL:libURL error:&error];
        if (library == nil) {
            fprintf(stderr, "failed to load metallib: %s\n",
                    error.localizedDescription.UTF8String ? error.localizedDescription.UTF8String : "unknown");
            return 4;
        }

        id<MTLFunction> function = [library newFunctionWithName:[NSString stringWithUTF8String:kernel_name]];
        if (function == nil) {
            fprintf(stderr, "missing kernel function: %s\n", kernel_name);
            return 5;
        }

        id<MTLComputePipelineState> pipeline = [device newComputePipelineStateWithFunction:function error:&error];
        if (pipeline == nil) {
            fprintf(stderr, "failed to create compute pipeline: %s\n",
                    error.localizedDescription.UTF8String ? error.localizedDescription.UTF8String : "unknown");
            return 6;
        }

        id<MTLCommandQueue> queue = [device newCommandQueue];
        if (queue == nil) {
            fprintf(stderr, "failed to create command queue\n");
            return 7;
        }

        NSUInteger bytes = (NSUInteger)width * sizeof(uint32_t);
        id<MTLBuffer> outBuffer = [device newBufferWithLength:bytes options:MTLResourceStorageModeShared];
        if (outBuffer == nil) {
            fprintf(stderr, "failed to allocate output buffer (%u entries)\n", width);
            return 8;
        }

        char fs_probe_path[256];
        fs_probe_path[0] = '\0';
        if (fs_probe_every > 0) {
            snprintf(fs_probe_path, sizeof(fs_probe_path), "/tmp/steinmarder_fs_probe_%d.log", getpid());
        }

        uint64_t t0 = now_ns();
        for (uint32_t i = 0; i < iterations; ++i) {
            id<MTLCommandBuffer> commandBuffer = [queue commandBuffer];
            if (commandBuffer == nil) {
                fprintf(stderr, "failed to allocate command buffer at iter %u\n", i);
                return 9;
            }

            id<MTLComputeCommandEncoder> encoder = [commandBuffer computeCommandEncoder];
            if (encoder == nil) {
                fprintf(stderr, "failed to allocate command encoder at iter %u\n", i);
                return 10;
            }

            [encoder setComputePipelineState:pipeline];
            [encoder setBuffer:outBuffer offset:0 atIndex:0];

            NSUInteger threadsPerTG = pipeline.maxTotalThreadsPerThreadgroup;
            if (threadsPerTG > 128) {
                threadsPerTG = 128;
            }
            if (threadsPerTG == 0) {
                threadsPerTG = 1;
            }

            MTLSize gridSize = MTLSizeMake((NSUInteger)width, 1, 1);
            MTLSize threadgroupSize = MTLSizeMake(threadsPerTG, 1, 1);
            [encoder dispatchThreads:gridSize threadsPerThreadgroup:threadgroupSize];
            [encoder endEncoding];

            [commandBuffer commit];
            [commandBuffer waitUntilCompleted];

            if (fs_probe_every > 0 && (i % fs_probe_every) == 0) {
                emit_fs_probe_event(fs_probe_path, i);
            }
        }
        uint64_t t1 = now_ns();

        if (fs_probe_every > 0) {
            emit_fs_probe_event(fs_probe_path, iterations);
        }

        const uint32_t *out = (const uint32_t *)outBuffer.contents;
        uint64_t hash = checksum_u32(out, width);
        double elapsed_ns = (double)(t1 - t0);
        double ns_per_iter = elapsed_ns / (double)iterations;
        double ns_per_element = elapsed_ns / ((double)iterations * (double)width);

        if (csv) {
            fprintf(stdout, "iters,width,elapsed_ns,ns_per_iter,ns_per_element,checksum\n");
            fprintf(stdout, "%u,%u,%.0f,%.6f,%.9f,%" PRIu64 "\n",
                    iterations, width, elapsed_ns, ns_per_iter, ns_per_element, hash);
        } else {
            fprintf(stdout,
                    "metal_probe elapsed_ns=%.0f ns_per_iter=%.6f ns_per_element=%.9f checksum=%" PRIu64 "\n",
                    elapsed_ns, ns_per_iter, ns_per_element, hash);
        }

        if (fs_probe_every > 0) {
            (void)unlink(fs_probe_path);
        }
    }
    return 0;
}
