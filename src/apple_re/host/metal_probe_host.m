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

// List all MTLCounterSets available on the device, one per line.
static void list_counter_sets(id<MTLDevice> device) {
    NSArray<id<MTLCounterSet>> *sets = device.counterSets;
    if (sets == nil || sets.count == 0) {
        fprintf(stdout, "counter_sets: none (device does not expose MTLCounterSets)\n");
        return;
    }
    for (id<MTLCounterSet> cs in sets) {
        fprintf(stdout, "counter_set: %s\n", cs.name.UTF8String);
        for (id<MTLCounter> c in cs.counters) {
            fprintf(stdout, "  counter: %s\n", c.name.UTF8String);
        }
    }
}

// Find a named counter set on the device. Returns nil if not found.
static id<MTLCounterSet> find_counter_set(id<MTLDevice> device, const char *name) {
    for (id<MTLCounterSet> cs in device.counterSets) {
        if (strcmp(cs.name.UTF8String, name) == 0) {
            return cs;
        }
    }
    return nil;
}

// Sample a counter buffer at a given index and print results to stdout.
// Returns the number of successfully read counter values.
static int print_counter_sample(id<MTLCounterSampleBuffer> sampleBuf,
                                 NSUInteger sampleIndex,
                                 id<MTLCounterSet> counterSet,
                                 const char *label) {
    NSRange range = NSMakeRange(sampleIndex, 1);
    NSData *data = [sampleBuf resolveCounterRange:range];
    if (data == nil) {
        fprintf(stdout, "counter_sample[%s]: resolve_failed\n", label);
        return 0;
    }

    NSArray<id<MTLCounter>> *counters = counterSet.counters;
    NSUInteger counter_count = counters.count;
    if (data.length < counter_count * sizeof(uint64_t)) {
        fprintf(stdout, "counter_sample[%s]: data_too_short (%lu bytes for %lu counters)\n",
                label, (unsigned long)data.length, (unsigned long)counter_count);
        return 0;
    }

    const uint64_t *values = (const uint64_t *)data.bytes;
    for (NSUInteger i = 0; i < counter_count; ++i) {
        fprintf(stdout, "counter_sample[%s]: %s=%" PRIu64 "\n",
                label, counters[i].name.UTF8String, values[i]);
    }
    return (int)counter_count;
}

int main(int argc, char **argv) {
    @autoreleasepool {
        const char *metallib_path = NULL;
        const char *kernel_name = "probe_simdgroup_reduce";
        uint32_t width = 1024;
        uint32_t iterations = 100;
        uint32_t fs_probe_every = 0;
        int csv = 0;
        int do_list_counters = 0;
        const char *counter_set_name = NULL;  // NULL = no counter sampling

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
            } else if (strcmp(argv[i], "--list-counters") == 0) {
                do_list_counters = 1;
            } else if (strcmp(argv[i], "--counters") == 0 && i + 1 < argc) {
                counter_set_name = argv[++i];
            } else if (strcmp(argv[i], "--help") == 0) {
                fprintf(stdout,
                    "usage: %s [--metallib path] [--kernel name] [--width N] [--iters N]\n"
                    "          [--fs-probe|--fs-probe-every N] [--csv]\n"
                    "          [--list-counters] [--counters SETNAME]\n"
                    "\n"
                    "MTLCounterSet options:\n"
                    "  --list-counters        List all available counter sets and their counters, then exit.\n"
                    "  --counters SETNAME     Enable counter sampling for the named counter set.\n"
                    "                         SETNAME examples: 'Timestamp', 'StatisticCounters'\n"
                    "                         Counter values printed to stdout after the run.\n",
                    argv[0]);
                return 0;
            } else {
                fprintf(stderr, "unknown argument: %s\n", argv[i]);
                return 2;
            }
        }

        id<MTLDevice> device = MTLCreateSystemDefaultDevice();
        if (device == nil) {
            fprintf(stderr, "no Metal device available\n");
            return 3;
        }

        // Handle --list-counters before requiring --metallib
        if (do_list_counters) {
            list_counter_sets(device);
            return 0;
        }

        if (metallib_path == NULL) {
            fprintf(stderr, "missing --metallib path\n");
            return 2;
        }
        if (width == 0 || iterations == 0) {
            fprintf(stderr, "width and iterations must be > 0\n");
            return 2;
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

        // Set up MTLCounterSet sampling if requested.
        // We take one sample at dispatch start and one at dispatch end across
        // all iterations (not per-iteration, to minimize overhead).
        //
        // Important: inline counter sampling inside a compute encoder requires
        // MTLCounterSamplingPointAtDispatchBoundary support. Many Apple GPU
        // families (e.g. AGXG13G) do NOT support this even if they expose the
        // 'timestamp' counter set. We check support before attempting.
        //
        // If dispatch-boundary sampling is not supported, the run still completes;
        // per-command-buffer GPU timestamps (commandBuffer.GPUStartTime /
        // commandBuffer.GPUEndTime) are always collected and printed with --counters.
        id<MTLCounterSampleBuffer> counterSampleBuf = nil;
        id<MTLCounterSet> counterSet = nil;
        int supportsDispatchSampling = 0;
        // Sample indices: 0 = before run, 1 = after run
        const NSUInteger kSampleCount = 2;

        if (counter_set_name != NULL) {
            counterSet = find_counter_set(device, counter_set_name);
            if (counterSet == nil) {
                fprintf(stderr, "counter set '%s' not found on this device\n", counter_set_name);
                fprintf(stderr, "run with --list-counters to see available sets\n");
                return 11;
            }

            // Check if this device supports inline sampling at dispatch boundaries
            supportsDispatchSampling = [device supportsCounterSampling:MTLCounterSamplingPointAtDispatchBoundary];

            if (supportsDispatchSampling) {
                MTLCounterSampleBufferDescriptor *desc = [[MTLCounterSampleBufferDescriptor alloc] init];
                desc.counterSet = counterSet;
                desc.sampleCount = kSampleCount;
                desc.storageMode = MTLStorageModeShared;
                counterSampleBuf = [device newCounterSampleBufferWithDescriptor:desc error:&error];
                if (counterSampleBuf == nil) {
                    fprintf(stderr, "failed to create counter sample buffer: %s\n",
                            error.localizedDescription.UTF8String ? error.localizedDescription.UTF8String : "unknown");
                    // Non-fatal: continue without inline counter sampling
                    counterSet = nil;
                }
            } else {
                fprintf(stdout, "counter_info: dispatch_boundary_sampling_not_supported_on_this_device\n");
                fprintf(stdout, "counter_info: using_commandbuffer_gputime_instead\n");
            }
        }

        char fs_probe_path[256];
        fs_probe_path[0] = '\0';
        if (fs_probe_every > 0) {
            snprintf(fs_probe_path, sizeof(fs_probe_path), "/tmp/steinmarder_fs_probe_%d.log", getpid());
        }

        // Accumulate GPU-side time using commandBuffer.GPUStartTime / GPUEndTime.
        // These are always available (no special counter set required).
        // Units are seconds in the GPU time domain (not CPU wall time).
        double gpu_elapsed_s = 0.0;
        double gpu_first_start_s = -1.0;
        double gpu_last_end_s = 0.0;

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

            // Inline counter sample at start of first iteration (only if supported)
            if (counterSampleBuf != nil && supportsDispatchSampling && i == 0) {
                [encoder sampleCountersInBuffer:counterSampleBuf
                                  atSampleIndex:0
                                    withBarrier:YES];
            }

            [encoder setComputePipelineState:pipeline];
            [encoder setBuffer:outBuffer offset:0 atIndex:0];
            // Pass iteration count to shaders that declare `constant uint &iters [[buffer(1)]]`.
            // Without this, the constant reads as 0 and the inner loop never executes.
            // Use a fixed high value (100000) so GPU compute dominates command-buffer overhead.
            uint32_t shader_inner_iters = 100000u;
            [encoder setBytes:&shader_inner_iters length:sizeof(uint32_t) atIndex:1];

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

            // Inline counter sample at end of last iteration (only if supported)
            if (counterSampleBuf != nil && supportsDispatchSampling && i == iterations - 1) {
                [encoder sampleCountersInBuffer:counterSampleBuf
                                  atSampleIndex:1
                                    withBarrier:YES];
            }
            [encoder endEncoding];

            [commandBuffer commit];
            [commandBuffer waitUntilCompleted];

            // Accumulate GPU-side timing
            if (counter_set_name != NULL) {
                double start_s = commandBuffer.GPUStartTime;
                double end_s   = commandBuffer.GPUEndTime;
                if (start_s > 0.0) {
                    if (gpu_first_start_s < 0.0) { gpu_first_start_s = start_s; }
                    gpu_last_end_s = end_s;
                    gpu_elapsed_s += (end_s - start_s);
                }
            }

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

        // Print GPU-side timing and counter values
        if (counter_set_name != NULL) {
            double gpu_ns_per_iter = (gpu_elapsed_s * 1e9) / (double)iterations;
            double gpu_ns_per_element = gpu_ns_per_iter / (double)width;
            fprintf(stdout, "gpu_time_ns_total=%.0f gpu_ns_per_iter=%.6f gpu_ns_per_element=%.9f\n",
                    gpu_elapsed_s * 1e9, gpu_ns_per_iter, gpu_ns_per_element);

            if (counterSampleBuf != nil && counterSet != nil && supportsDispatchSampling) {
                fprintf(stdout, "counter_set: %s\n", counter_set_name);
                print_counter_sample(counterSampleBuf, 0, counterSet, "start");
                print_counter_sample(counterSampleBuf, 1, counterSet, "end");
            }
        }

        if (fs_probe_every > 0) {
            (void)unlink(fs_probe_path);
        }
    }
    return 0;
}
