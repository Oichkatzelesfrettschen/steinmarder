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

// Measure MTLSharedEvent CPU wakeup latency.
// Encodes an empty command buffer that signals the event, then blocks the CPU thread
// with [event waitUntilSignaledValue:]. The wall-clock delta measures:
//   command-buffer dispatch overhead + GPU scheduling + event signal + CPU wakeup.
// Reports ns statistics across `n_iters` iterations (warm-up: first 5% of iterations
// are discarded from stats).
static void measure_event_latency(id<MTLDevice> device, uint32_t n_iters) {
    id<MTLCommandQueue> queue = [device newCommandQueue];
    id<MTLSharedEvent> event = [device newSharedEvent];

    // Allocate a tiny output buffer (required by some pipeline setups; unused here).
    id<MTLBuffer> dummy = [device newBufferWithLength:4 options:MTLResourceStorageModeShared];

    // Warm-up: 10 iterations before recording stats.
    uint32_t warmup = n_iters / 10 < 10 ? 10 : n_iters / 10;
    uint32_t total = n_iters + warmup;

    double *samples = (double *)malloc(sizeof(double) * n_iters);
    if (samples == NULL) {
        fprintf(stderr, "event_latency: out of memory\n");
        return;
    }

    uint64_t signal_value = 0;
    uint32_t recorded = 0;

    for (uint32_t i = 0; i < total; ++i) {
        signal_value++;

        id<MTLCommandBuffer> cb = [queue commandBuffer];
        // Encode a trivial compute pass — just endEncoding with no work.
        id<MTLBlitCommandEncoder> blit = [cb blitCommandEncoder];
        [blit endEncoding];
        // Signal the shared event at the end of this command buffer.
        [cb encodeSignalEvent:event value:signal_value];

        uint64_t t0 = now_ns();
        [cb commit];
        // Block until the GPU signals the event.
        [event waitUntilSignaledValue:signal_value timeoutMS:2000];
        uint64_t t1 = now_ns();

        if (i >= warmup) {
            samples[recorded++] = (double)(t1 - t0);
        }
    }

    // Sort samples for percentile computation.
    // Simple insertion sort (n_iters is typically small, e.g. 200–1000).
    for (uint32_t i = 1; i < recorded; ++i) {
        double key = samples[i];
        int32_t j = (int32_t)i - 1;
        while (j >= 0 && samples[j] > key) {
            samples[j + 1] = samples[j];
            j--;
        }
        samples[j + 1] = key;
    }

    double sum = 0.0;
    for (uint32_t i = 0; i < recorded; ++i) { sum += samples[i]; }
    double mean_ns = sum / (double)recorded;
    double min_ns  = samples[0];
    double max_ns  = samples[recorded - 1];
    double p50_ns  = samples[(uint32_t)(recorded * 0.50)];
    double p95_ns  = samples[(uint32_t)(recorded * 0.95)];
    double p99_ns  = samples[(uint32_t)(recorded * 0.99 < recorded ? recorded * 0.99 : recorded - 1)];

    fprintf(stdout,
        "event_latency_ns: n=%u warmup=%u\n"
        "  min=%.0f  p50=%.0f  mean=%.0f  p95=%.0f  p99=%.0f  max=%.0f\n",
        recorded, warmup,
        min_ns, p50_ns, mean_ns, p95_ns, p99_ns, max_ns);

    (void)dummy;
    free(samples);
}

int main(int argc, char **argv) {
    @autoreleasepool {
        const char *metallib_path = NULL;
        // Support multiple --kernel flags; all kernels run back-to-back in the same
        // invocation so the GPU stays warm and clock state is consistent.
        #define MAX_KERNELS 32
        const char *kernel_names[MAX_KERNELS];
        int n_kernels = 0;
        uint32_t width = 1024;
        uint32_t iterations = 100;
        uint32_t fs_probe_every = 0;
        uint32_t event_latency_iters = 0;  // 0 = disabled
        uint32_t l2_chase_entries = 0;     // 0 = disabled; when set, allocates chase buffer as buffer(2)
        uint32_t shader_iters_override = 0; // 0 = use default (100000); set via --shader-iters
        int csv = 0;
        int do_list_counters = 0;
        const char *counter_set_name = NULL;  // NULL = no counter sampling
        int interleave = 0; // if set, alternate dispatches across kernels (round-robin)

        for (int i = 1; i < argc; ++i) {
            if (strcmp(argv[i], "--metallib") == 0 && i + 1 < argc) {
                metallib_path = argv[++i];
            } else if (strcmp(argv[i], "--kernel") == 0 && i + 1 < argc) {
                if (n_kernels < MAX_KERNELS) {
                    kernel_names[n_kernels++] = argv[++i];
                } else {
                    fprintf(stderr, "too many --kernel flags (max %d)\n", MAX_KERNELS);
                    return 2;
                }
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
            } else if (strcmp(argv[i], "--measure-event-latency") == 0 && i + 1 < argc) {
                event_latency_iters = (uint32_t)strtoul(argv[++i], NULL, 10);
            } else if (strcmp(argv[i], "--l2-chase-entries") == 0 && i + 1 < argc) {
                l2_chase_entries = (uint32_t)strtoul(argv[++i], NULL, 10);
            } else if (strcmp(argv[i], "--shader-iters") == 0 && i + 1 < argc) {
                shader_iters_override = (uint32_t)strtoul(argv[++i], NULL, 10);
            } else if (strcmp(argv[i], "--interleave") == 0) {
                interleave = 1;
            } else if (strcmp(argv[i], "--help") == 0) {
                fprintf(stdout,
                    "usage: %s [--metallib path] [--kernel name] [--width N] [--iters N]\n"
                    "          [--fs-probe|--fs-probe-every N] [--csv]\n"
                    "          [--list-counters] [--counters SETNAME]\n"
                    "          [--measure-event-latency N]\n"
                    "\n"
                    "MTLCounterSet options:\n"
                    "  --list-counters        List all available counter sets and their counters, then exit.\n"
                    "  --counters SETNAME     Enable counter sampling for the named counter set.\n"
                    "                         SETNAME examples: 'Timestamp', 'StatisticCounters'\n"
                    "                         Counter values printed to stdout after the run.\n"
                    "\n"
                    "Event latency:\n"
                    "  --measure-event-latency N  Measure MTLSharedEvent CPU wakeup latency over N iters.\n"
                    "                             Encodes empty command buffer + signal; blocks CPU with\n"
                    "                             waitUntilSignaledValue. Prints min/p50/mean/p95/p99/max.\n"
                    "\n"
                    "L2 cache probe (C3):\n"
                    "  --l2-chase-entries N       Allocate a uint32 pointer-chase buffer of N entries\n"
                    "                             (N*4 bytes) as buffer(2). Host initializes chase[i] =\n"
                    "                             (i + 2053) %% N (stride coprime to N, step > L1=8KB).\n"
                    "                             Use with --kernel probe_l2_lat. N=65536 gives 256KB\n"
                    "                             (fits in L2=768KB, guarantees L1 misses at step 8212B).\n",
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

        // Handle --measure-event-latency before requiring --metallib
        if (event_latency_iters > 0) {
            measure_event_latency(device, event_latency_iters);
            return 0;
        }

        if (metallib_path == NULL) {
            fprintf(stderr, "missing --metallib path\n");
            return 2;
        }
        if (n_kernels == 0) {
            fprintf(stderr, "missing --kernel name\n");
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

        // Pre-compile all pipeline states so the first kernel doesn't pay JIT overhead.
        id<MTLComputePipelineState> pipelines[MAX_KERNELS];
        for (int ki = 0; ki < n_kernels; ++ki) {
            id<MTLFunction> function = [library newFunctionWithName:[NSString stringWithUTF8String:kernel_names[ki]]];
            if (function == nil) {
                fprintf(stderr, "missing kernel function: %s\n", kernel_names[ki]);
                return 5;
            }
            pipelines[ki] = [device newComputePipelineStateWithFunction:function error:&error];
            if (pipelines[ki] == nil) {
                fprintf(stderr, "failed to create compute pipeline for %s: %s\n",
                        kernel_names[ki],
                        error.localizedDescription.UTF8String ? error.localizedDescription.UTF8String : "unknown");
                return 6;
            }
        }

        id<MTLCommandQueue> queue = [device newCommandQueue];
        if (queue == nil) {
            fprintf(stderr, "failed to create command queue\n");
            return 7;
        }

        // Create a 1×1 RGBA32F texture for texture-sample dep-chain probes.
        // Value: {0.5, 0.5, 0.5, 1.0} — fixed point of fma(x, 0.4, 0.3).
        // Bound to texture(0) for all kernels; non-texture kernels ignore it.
        MTLTextureDescriptor *texDesc =
            [MTLTextureDescriptor texture2DDescriptorWithPixelFormat:MTLPixelFormatRGBA32Float
                                                               width:1
                                                              height:1
                                                           mipmapped:NO];
        texDesc.usage       = MTLTextureUsageShaderRead;
        texDesc.storageMode = MTLStorageModeShared;
        id<MTLTexture> probeTexture = [device newTextureWithDescriptor:texDesc];
        if (probeTexture == nil) {
            fprintf(stderr, "failed to create probe texture\n");
            return 8;
        }
        float probe_tex_data[4] = {0.5f, 0.5f, 0.5f, 1.0f};
        [probeTexture replaceRegion:MTLRegionMake2D(0, 0, 1, 1)
                       mipmapLevel:0
                         withBytes:probe_tex_data
                       bytesPerRow:sizeof(float) * 4];

        // Sampler: linear filter, clamp_to_edge. Used alongside texture(0).
        // Note: shaders that declare `constexpr sampler` inline do not use this,
        // but binding it to sampler(0) is harmless and future-proofs host/shader pairing.
        MTLSamplerDescriptor *sampDesc = [[MTLSamplerDescriptor alloc] init];
        sampDesc.minFilter    = MTLSamplerMinMagFilterLinear;
        sampDesc.magFilter    = MTLSamplerMinMagFilterLinear;
        sampDesc.sAddressMode = MTLSamplerAddressModeClampToEdge;
        sampDesc.tAddressMode = MTLSamplerAddressModeClampToEdge;
        sampDesc.mipFilter    = MTLSamplerMipFilterNotMipmapped;
        id<MTLSamplerState> probeSampler = [device newSamplerStateWithDescriptor:sampDesc];

        NSUInteger bytes = (NSUInteger)width * sizeof(uint32_t);
        id<MTLBuffer> outBuffer = [device newBufferWithLength:bytes options:MTLResourceStorageModeShared];
        if (outBuffer == nil) {
            fprintf(stderr, "failed to allocate output buffer (%u entries)\n", width);
            return 8;
        }

        // Allocate and initialize L2 pointer-chase buffer if requested (C3 probe).
        // chase[i] = (i + stride) % n_entries  where stride=2053 is coprime to 65536.
        // Step size = 2053 × 4 = 8212 bytes, just above L1 (8192 bytes) → guaranteed L1 miss.
        id<MTLBuffer> chaseBuffer = nil;
        if (l2_chase_entries > 0) {
            NSUInteger chase_bytes = (NSUInteger)l2_chase_entries * sizeof(uint32_t);
            chaseBuffer = [device newBufferWithLength:chase_bytes options:MTLResourceStorageModeShared];
            if (chaseBuffer == nil) {
                fprintf(stderr, "failed to allocate chase buffer (%u entries)\n", l2_chase_entries);
                return 8;
            }
            uint32_t *chase_data = (uint32_t *)chaseBuffer.contents;
            const uint32_t l2_chase_stride = 2053u;
            for (uint32_t chase_idx = 0u; chase_idx < l2_chase_entries; chase_idx++) {
                chase_data[chase_idx] = (chase_idx + l2_chase_stride) % l2_chase_entries;
            }
            fprintf(stdout, "l2_chase: entries=%u stride=%u step_bytes=%u total_bytes=%llu\n",
                    l2_chase_entries, l2_chase_stride,
                    l2_chase_stride * (uint32_t)sizeof(uint32_t),
                    (unsigned long long)chase_bytes);
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

        // Compute shader inner iteration count once; shared across all kernels.
        uint32_t shader_inner_iters = (shader_iters_override > 0) ? shader_iters_override : 100000u;

        // Print CSV header once if needed (multi-kernel csv prints one row per kernel).
        if (csv) {
            fprintf(stdout, "kernel,iters,shader_iters,width,elapsed_ns,ns_per_iter,ns_per_element,checksum\n");
        }

        // --interleave mode: alternate dispatches across all kernels in round-robin so
        // every kernel sees the same average GPU thermal/clock state. Each kernel's
        // elapsed time is tracked separately; the total dispatch count per kernel is
        // `iterations`. Output is the same format as sequential mode.
        if (interleave && n_kernels > 1) {
            uint64_t *k_elapsed = (uint64_t *)calloc((size_t)n_kernels, sizeof(uint64_t));
            if (k_elapsed == NULL) { fprintf(stderr, "out of memory\n"); return 9; }

            MTLSize gridSize = MTLSizeMake((NSUInteger)width, 1, 1);
            for (uint32_t i = 0; i < iterations; ++i) {
                for (int ki = 0; ki < n_kernels; ++ki) {
                    id<MTLComputePipelineState> pipeline = pipelines[ki];
                    NSUInteger threadsPerTG = pipeline.maxTotalThreadsPerThreadgroup;
                    if (threadsPerTG > 128) threadsPerTG = 128;
                    if (threadsPerTG == 0)  threadsPerTG = 1;
                    MTLSize tgSize = MTLSizeMake(threadsPerTG, 1, 1);

                    id<MTLCommandBuffer> cb = [queue commandBuffer];
                    id<MTLComputeCommandEncoder> enc = [cb computeCommandEncoder];
                    [enc setComputePipelineState:pipeline];
                    [enc setBuffer:outBuffer offset:0 atIndex:0];
                    [enc setBytes:&shader_inner_iters length:sizeof(uint32_t) atIndex:1];
                    if (chaseBuffer != nil) [enc setBuffer:chaseBuffer offset:0 atIndex:2];
                    [enc setTexture:probeTexture atIndex:0];
                    [enc setSamplerState:probeSampler atIndex:0];
                    [enc dispatchThreads:gridSize threadsPerThreadgroup:tgSize];
                    [enc endEncoding];

                    uint64_t t0 = now_ns();
                    [cb commit];
                    [cb waitUntilCompleted];
                    k_elapsed[ki] += now_ns() - t0;
                }
            }

            if (csv) {
                fprintf(stdout, "kernel,iters,shader_iters,width,elapsed_ns,ns_per_iter,ns_per_element,checksum\n");
            }
            const uint32_t *out_data = (const uint32_t *)outBuffer.contents;
            for (int ki = 0; ki < n_kernels; ++ki) {
                uint64_t hash = checksum_u32(out_data, width);
                double elapsed_ns   = (double)k_elapsed[ki];
                double ns_per_iter  = elapsed_ns / (double)iterations;
                double ns_per_elem  = elapsed_ns / ((double)iterations * (double)width);
                if (csv) {
                    fprintf(stdout, "%s,%u,%u,%u,%.0f,%.6f,%.9f,%" PRIu64 "\n",
                            kernel_names[ki], iterations, shader_inner_iters, width,
                            elapsed_ns, ns_per_iter, ns_per_elem, hash);
                } else {
                    fprintf(stdout,
                            "metal_probe kernel=%s elapsed_ns=%.0f ns_per_iter=%.6f shader_iters=%u ns_per_element=%.9f checksum=%" PRIu64 "\n",
                            kernel_names[ki], elapsed_ns, ns_per_iter, shader_inner_iters, ns_per_elem, hash);
                }
            }
            free(k_elapsed);
            if (fs_probe_every > 0) { (void)unlink(fs_probe_path); }
            return 0;
        }

        // Run each kernel in sequence within the same process so the GPU stays
        // warm and clock state is consistent across all measurements.
        for (int ki = 0; ki < n_kernels; ++ki) {
            id<MTLComputePipelineState> pipeline = pipelines[ki];
            const char *kernel_name = kernel_names[ki];

            // Accumulate GPU-side time using commandBuffer.GPUStartTime / GPUEndTime.
            double gpu_elapsed_s = 0.0;
            double gpu_first_start_s = -1.0;
            double gpu_last_end_s = 0.0;

            uint64_t t0 = now_ns();
            for (uint32_t i = 0; i < iterations; ++i) {
                id<MTLCommandBuffer> commandBuffer = [queue commandBuffer];
                if (commandBuffer == nil) {
                    fprintf(stderr, "failed to allocate command buffer at iter %u (kernel %s)\n", i, kernel_name);
                    return 9;
                }

                id<MTLComputeCommandEncoder> encoder = [commandBuffer computeCommandEncoder];
                if (encoder == nil) {
                    fprintf(stderr, "failed to allocate command encoder at iter %u (kernel %s)\n", i, kernel_name);
                    return 10;
                }

                // Inline counter sample at start of first iteration of first kernel (only if supported)
                if (counterSampleBuf != nil && supportsDispatchSampling && ki == 0 && i == 0) {
                    [encoder sampleCountersInBuffer:counterSampleBuf
                                      atSampleIndex:0
                                        withBarrier:YES];
                }

                [encoder setComputePipelineState:pipeline];
                [encoder setBuffer:outBuffer offset:0 atIndex:0];
                // Pass iteration count to shaders that declare `constant uint &iters [[buffer(1)]]`.
                [encoder setBytes:&shader_inner_iters length:sizeof(uint32_t) atIndex:1];
                // Bind pointer-chase buffer as buffer(2) when --l2-chase-entries is set.
                if (chaseBuffer != nil) {
                    [encoder setBuffer:chaseBuffer offset:0 atIndex:2];
                }
                // Bind probe texture and sampler for texture dep-chain kernels.
                // Non-texture kernels ignore these bindings.
                [encoder setTexture:probeTexture atIndex:0];
                [encoder setSamplerState:probeSampler atIndex:0];

                NSUInteger threadsPerTG = pipeline.maxTotalThreadsPerThreadgroup;
                if (threadsPerTG > 128) { threadsPerTG = 128; }
                if (threadsPerTG == 0)  { threadsPerTG = 1; }

                MTLSize gridSize = MTLSizeMake((NSUInteger)width, 1, 1);
                MTLSize threadgroupSize = MTLSizeMake(threadsPerTG, 1, 1);
                [encoder dispatchThreads:gridSize threadsPerThreadgroup:threadgroupSize];

                // Inline counter sample at end of last iteration of last kernel (only if supported)
                if (counterSampleBuf != nil && supportsDispatchSampling && ki == n_kernels - 1 && i == iterations - 1) {
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

            const uint32_t *out_data = (const uint32_t *)outBuffer.contents;
            uint64_t hash = checksum_u32(out_data, width);
            double elapsed_ns = (double)(t1 - t0);
            double ns_per_iter = elapsed_ns / (double)iterations;
            double ns_per_element = elapsed_ns / ((double)iterations * (double)width);

            if (csv) {
                fprintf(stdout, "%s,%u,%u,%u,%.0f,%.6f,%.9f,%" PRIu64 "\n",
                        kernel_name, iterations, shader_inner_iters, width,
                        elapsed_ns, ns_per_iter, ns_per_element, hash);
            } else {
                fprintf(stdout,
                        "metal_probe kernel=%s elapsed_ns=%.0f ns_per_iter=%.6f shader_iters=%u ns_per_element=%.9f checksum=%" PRIu64 "\n",
                        kernel_name, elapsed_ns, ns_per_iter, shader_inner_iters, ns_per_element, hash);
            }

            // Print GPU-side timing and counter values (only for single-kernel runs or last kernel)
            if (counter_set_name != NULL && ki == n_kernels - 1) {
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
        } // end per-kernel loop

        if (fs_probe_every > 0) {
            (void)unlink(fs_probe_path);
        }
    }
    return 0;
}
