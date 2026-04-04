#define _POSIX_C_SOURCE 200809L

#include <errno.h>
#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <vulkan/vulkan.h>

typedef enum OutputMode {
    OUTPUT_SCALAR_F32 = 0,
    OUTPUT_VEC4_F32 = 1,
} OutputMode;

typedef struct Options {
    const char *shader_path;
    const char *device_substr;
    uint32_t elements;
    uint32_t local_size;
    OutputMode mode;
} Options;

typedef struct BufferBundle {
    VkBuffer buffer;
    VkDeviceMemory memory;
    void *mapped;
    VkDeviceSize size;
} BufferBundle;

static void usage(const char *argv0) {
    fprintf(stderr,
            "usage: %s --shader path.spv [--elements N] [--local-size N] "
            "[--mode scalar_f32|vec4_f32] [--device-substr name]\n",
            argv0);
}

static uint64_t monotonic_ns(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000000000ull + (uint64_t)ts.tv_nsec;
}

static void *read_binary(const char *path, size_t *size_out) {
    FILE *fp = fopen(path, "rb");
    long size;
    void *data;

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

    data = malloc((size_t)size);
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
    fclose(fp);
    *size_out = (size_t)size;
    return data;
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

    opts->shader_path = NULL;
    opts->device_substr = NULL;
    opts->elements = 4096;
    opts->local_size = 64;
    opts->mode = OUTPUT_SCALAR_F32;

    for (i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "--shader") == 0 && i + 1 < argc) {
            opts->shader_path = argv[++i];
        } else if (strcmp(argv[i], "--elements") == 0 && i + 1 < argc) {
            opts->elements = (uint32_t)strtoul(argv[++i], NULL, 10);
        } else if (strcmp(argv[i], "--local-size") == 0 && i + 1 < argc) {
            opts->local_size = (uint32_t)strtoul(argv[++i], NULL, 10);
        } else if (strcmp(argv[i], "--mode") == 0 && i + 1 < argc) {
            if (!parse_mode(argv[++i], &opts->mode)) {
                fprintf(stderr, "unknown mode: %s\n", argv[i]);
                return 0;
            }
        } else if (strcmp(argv[i], "--device-substr") == 0 && i + 1 < argc) {
            opts->device_substr = argv[++i];
        } else {
            usage(argv[0]);
            return 0;
        }
    }

    if (!opts->shader_path || opts->elements == 0 || opts->local_size == 0) {
        usage(argv[0]);
        return 0;
    }
    return 1;
}

static uint32_t find_memory_type(const VkPhysicalDeviceMemoryProperties *props,
                                 uint32_t bits,
                                 VkMemoryPropertyFlags required) {
    uint32_t i;
    for (i = 0; i < props->memoryTypeCount; ++i) {
        if ((bits & (1u << i)) &&
            (props->memoryTypes[i].propertyFlags & required) == required) {
            return i;
        }
    }
    return UINT32_MAX;
}

static VkPhysicalDevice pick_device(VkInstance instance,
                                    const char *needle,
                                    char *name_out,
                                    size_t name_out_size) {
    VkPhysicalDevice devices[16];
    uint32_t count = 16;
    uint32_t i;
    VkResult result = vkEnumeratePhysicalDevices(instance, &count, devices);
    if (result != VK_SUCCESS || count == 0) {
        fprintf(stderr, "vkEnumeratePhysicalDevices failed: %d\n", result);
        return VK_NULL_HANDLE;
    }
    for (i = 0; i < count; ++i) {
        VkPhysicalDeviceProperties props;
        vkGetPhysicalDeviceProperties(devices[i], &props);
        if (!needle || strstr(props.deviceName, needle) != NULL) {
            snprintf(name_out, name_out_size, "%s", props.deviceName);
            return devices[i];
        }
    }
    {
        VkPhysicalDeviceProperties props;
        vkGetPhysicalDeviceProperties(devices[0], &props);
        snprintf(name_out, name_out_size, "%s", props.deviceName);
    }
    return devices[0];
}

static uint32_t find_compute_queue_family(VkPhysicalDevice physical_device) {
    uint32_t count = 0;
    uint32_t i;
    VkQueueFamilyProperties families[32];

    vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &count, NULL);
    if (count > 32) {
        count = 32;
    }
    vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &count, families);
    for (i = 0; i < count; ++i) {
        if ((families[i].queueFlags & VK_QUEUE_COMPUTE_BIT) != 0u) {
            return i;
        }
    }
    return UINT32_MAX;
}

static int create_buffer_bundle(VkPhysicalDevice physical_device,
                                VkDevice device,
                                VkDeviceSize size,
                                BufferBundle *bundle) {
    VkBufferCreateInfo buffer_info = {
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .size = size,
        .usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
    };
    VkMemoryRequirements mem_req;
    VkPhysicalDeviceMemoryProperties mem_props;
    VkMemoryAllocateInfo alloc_info = {
        .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
    };
    uint32_t memory_type;

    memset(bundle, 0, sizeof(*bundle));
    bundle->size = size;

    if (vkCreateBuffer(device, &buffer_info, NULL, &bundle->buffer) != VK_SUCCESS) {
        fprintf(stderr, "vkCreateBuffer failed\n");
        return 0;
    }

    vkGetBufferMemoryRequirements(device, bundle->buffer, &mem_req);
    vkGetPhysicalDeviceMemoryProperties(physical_device, &mem_props);
    memory_type = find_memory_type(&mem_props, mem_req.memoryTypeBits,
                                   VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                                       VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    if (memory_type == UINT32_MAX) {
        fprintf(stderr, "failed to find host-visible memory type\n");
        return 0;
    }

    alloc_info.allocationSize = mem_req.size;
    alloc_info.memoryTypeIndex = memory_type;

    if (vkAllocateMemory(device, &alloc_info, NULL, &bundle->memory) != VK_SUCCESS) {
        fprintf(stderr, "vkAllocateMemory failed\n");
        return 0;
    }
    if (vkBindBufferMemory(device, bundle->buffer, bundle->memory, 0) != VK_SUCCESS) {
        fprintf(stderr, "vkBindBufferMemory failed\n");
        return 0;
    }
    if (vkMapMemory(device, bundle->memory, 0, size, 0, &bundle->mapped) != VK_SUCCESS) {
        fprintf(stderr, "vkMapMemory failed\n");
        return 0;
    }
    memset(bundle->mapped, 0, (size_t)size);
    return 1;
}

static void destroy_buffer_bundle(VkDevice device, BufferBundle *bundle) {
    if (bundle->mapped) {
        vkUnmapMemory(device, bundle->memory);
    }
    if (bundle->memory) {
        vkFreeMemory(device, bundle->memory, NULL);
    }
    if (bundle->buffer) {
        vkDestroyBuffer(device, bundle->buffer, NULL);
    }
}

static void print_result_json(const Options *opts,
                              const char *device_name,
                              uint32_t workgroups_x,
                              VkDeviceSize buffer_bytes,
                              uint64_t dispatch_ns,
                              const float *values,
                              size_t value_count) {
    double checksum = 0.0;
    size_t i;
    const char *mode = opts->mode == OUTPUT_VEC4_F32 ? "vec4_f32" : "scalar_f32";
    for (i = 0; i < value_count; ++i) {
        checksum += (double)values[i];
    }

    printf("{\n");
    printf("  \"shader_path\": \"%s\",\n", opts->shader_path);
    printf("  \"device_name\": \"%s\",\n", device_name);
    printf("  \"mode\": \"%s\",\n", mode);
    printf("  \"elements\": %" PRIu32 ",\n", opts->elements);
    printf("  \"local_size\": %" PRIu32 ",\n", opts->local_size);
    printf("  \"workgroups_x\": %" PRIu32 ",\n", workgroups_x);
    printf("  \"buffer_bytes\": %" PRIu64 ",\n", (uint64_t)buffer_bytes);
    printf("  \"dispatch_ns\": %" PRIu64 ",\n", dispatch_ns);
    printf("  \"checksum\": %.9f,\n", checksum);
    printf("  \"first\": [%.9f, %.9f, %.9f, %.9f],\n",
           value_count > 0 ? values[0] : 0.0f,
           value_count > 1 ? values[1] : 0.0f,
           value_count > 2 ? values[2] : 0.0f,
           value_count > 3 ? values[3] : 0.0f);
    printf("  \"last\": [%.9f, %.9f, %.9f, %.9f]\n",
           value_count > 4 ? values[value_count - 4] : (value_count > 0 ? values[value_count - 1] : 0.0f),
           value_count > 3 ? values[value_count - 3] : 0.0f,
           value_count > 2 ? values[value_count - 2] : 0.0f,
           value_count > 1 ? values[value_count - 1] : 0.0f);
    printf("}\n");
}

int main(int argc, char **argv) {
    Options opts;
    VkInstance instance = VK_NULL_HANDLE;
    VkPhysicalDevice physical_device = VK_NULL_HANDLE;
    VkDevice device = VK_NULL_HANDLE;
    VkQueue queue = VK_NULL_HANDLE;
    VkDescriptorSetLayout desc_layout = VK_NULL_HANDLE;
    VkPipelineLayout pipe_layout = VK_NULL_HANDLE;
    VkPipeline pipeline = VK_NULL_HANDLE;
    VkShaderModule shader_module = VK_NULL_HANDLE;
    VkDescriptorPool desc_pool = VK_NULL_HANDLE;
    VkDescriptorSet desc_set = VK_NULL_HANDLE;
    VkCommandPool cmd_pool = VK_NULL_HANDLE;
    VkCommandBuffer cmd = VK_NULL_HANDLE;
    VkFence fence = VK_NULL_HANDLE;
    BufferBundle output = {0};
    char device_name[VK_MAX_PHYSICAL_DEVICE_NAME_SIZE] = {0};
    float queue_priority = 1.0f;
    uint32_t queue_family;
    uint32_t workgroups_x;
    VkDeviceSize buffer_bytes;
    VkResult result;
    size_t spirv_size = 0;
    void *spirv = NULL;
    uint64_t start_ns;
    uint64_t end_ns;
    const size_t values_per_element = 4;
    size_t float_count;

    if (!parse_options(argc, argv, &opts)) {
        return 2;
    }

    {
        VkApplicationInfo app_info = {
            .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
            .pApplicationName = "terascale_vk_probe_host",
            .applicationVersion = VK_MAKE_VERSION(1, 0, 0),
            .pEngineName = "steinmarder",
            .engineVersion = VK_MAKE_VERSION(1, 0, 0),
            .apiVersion = VK_API_VERSION_1_0,
        };
        VkInstanceCreateInfo instance_info = {
            .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
            .pApplicationInfo = &app_info,
        };
        result = vkCreateInstance(&instance_info, NULL, &instance);
        if (result != VK_SUCCESS) {
            fprintf(stderr, "vkCreateInstance failed: %d\n", result);
            return 1;
        }
    }

    physical_device = pick_device(instance, opts.device_substr, device_name, sizeof(device_name));
    if (!physical_device) {
        vkDestroyInstance(instance, NULL);
        return 1;
    }
    queue_family = find_compute_queue_family(physical_device);
    if (queue_family == UINT32_MAX) {
        fprintf(stderr, "no compute-capable queue family found\n");
        vkDestroyInstance(instance, NULL);
        return 1;
    }

    {
        VkDeviceQueueCreateInfo queue_info = {
            .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
            .queueFamilyIndex = queue_family,
            .queueCount = 1,
            .pQueuePriorities = &queue_priority,
        };
        VkDeviceCreateInfo device_info = {
            .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
            .queueCreateInfoCount = 1,
            .pQueueCreateInfos = &queue_info,
        };
        result = vkCreateDevice(physical_device, &device_info, NULL, &device);
        if (result != VK_SUCCESS) {
            fprintf(stderr, "vkCreateDevice failed: %d\n", result);
            vkDestroyInstance(instance, NULL);
            return 1;
        }
    }

    vkGetDeviceQueue(device, queue_family, 0, &queue);
    spirv = read_binary(opts.shader_path, &spirv_size);
    if (!spirv) {
        vkDestroyDevice(device, NULL);
        vkDestroyInstance(instance, NULL);
        return 1;
    }

    {
        VkShaderModuleCreateInfo shader_info = {
            .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
            .codeSize = spirv_size,
            .pCode = (const uint32_t *)spirv,
        };
        result = vkCreateShaderModule(device, &shader_info, NULL, &shader_module);
        if (result != VK_SUCCESS) {
            fprintf(stderr, "vkCreateShaderModule failed: %d\n", result);
            free(spirv);
            vkDestroyDevice(device, NULL);
            vkDestroyInstance(instance, NULL);
            return 1;
        }
    }
    free(spirv);

    buffer_bytes = (VkDeviceSize)opts.elements *
                   (opts.mode == OUTPUT_VEC4_F32 ? sizeof(float) * 4 : sizeof(float));
    if (!create_buffer_bundle(physical_device, device, buffer_bytes, &output)) {
        vkDestroyShaderModule(device, shader_module, NULL);
        vkDestroyDevice(device, NULL);
        vkDestroyInstance(instance, NULL);
        return 1;
    }

    {
        VkDescriptorSetLayoutBinding binding = {
            .binding = 0,
            .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
            .descriptorCount = 1,
            .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT,
        };
        VkDescriptorSetLayoutCreateInfo layout_info = {
            .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
            .bindingCount = 1,
            .pBindings = &binding,
        };
        result = vkCreateDescriptorSetLayout(device, &layout_info, NULL, &desc_layout);
        if (result != VK_SUCCESS) {
            fprintf(stderr, "vkCreateDescriptorSetLayout failed: %d\n", result);
            destroy_buffer_bundle(device, &output);
            vkDestroyShaderModule(device, shader_module, NULL);
            vkDestroyDevice(device, NULL);
            vkDestroyInstance(instance, NULL);
            return 1;
        }
    }

    {
        VkPipelineLayoutCreateInfo layout_info = {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
            .setLayoutCount = 1,
            .pSetLayouts = &desc_layout,
        };
        result = vkCreatePipelineLayout(device, &layout_info, NULL, &pipe_layout);
        if (result != VK_SUCCESS) {
            fprintf(stderr, "vkCreatePipelineLayout failed: %d\n", result);
            goto cleanup;
        }
    }

    {
        VkPipelineShaderStageCreateInfo stage_info = {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
            .stage = VK_SHADER_STAGE_COMPUTE_BIT,
            .module = shader_module,
            .pName = "main",
        };
        VkComputePipelineCreateInfo pipeline_info = {
            .sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO,
            .stage = stage_info,
            .layout = pipe_layout,
        };
        fprintf(stderr,
                "VK_PROBE stage=create_compute_pipeline shader=%s elements=%" PRIu32
                " local_size=%" PRIu32 " mode=%s\n",
                opts.shader_path,
                opts.elements,
                opts.local_size,
                opts.mode == OUTPUT_VEC4_F32 ? "vec4_f32" : "scalar_f32");
        result = vkCreateComputePipelines(device, VK_NULL_HANDLE, 1, &pipeline_info, NULL, &pipeline);
        if (result != VK_SUCCESS) {
            fprintf(stderr, "vkCreateComputePipelines failed: %d\n", result);
            goto cleanup;
        }
    }

    {
        VkDescriptorPoolSize pool_size = {
            .type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
            .descriptorCount = 1,
        };
        VkDescriptorPoolCreateInfo pool_info = {
            .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
            .maxSets = 1,
            .poolSizeCount = 1,
            .pPoolSizes = &pool_size,
        };
        VkDescriptorSetAllocateInfo alloc_info = {
            .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
            .descriptorPool = desc_pool,
            .descriptorSetCount = 1,
            .pSetLayouts = &desc_layout,
        };
        VkDescriptorBufferInfo buffer_info = {
            .buffer = output.buffer,
            .offset = 0,
            .range = buffer_bytes,
        };
        VkWriteDescriptorSet write = {
            .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            .dstSet = desc_set,
            .dstBinding = 0,
            .descriptorCount = 1,
            .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
            .pBufferInfo = &buffer_info,
        };
        result = vkCreateDescriptorPool(device, &pool_info, NULL, &desc_pool);
        if (result != VK_SUCCESS) {
            fprintf(stderr, "vkCreateDescriptorPool failed: %d\n", result);
            goto cleanup;
        }
        alloc_info.descriptorPool = desc_pool;
        result = vkAllocateDescriptorSets(device, &alloc_info, &desc_set);
        if (result != VK_SUCCESS) {
            fprintf(stderr, "vkAllocateDescriptorSets failed: %d\n", result);
            goto cleanup;
        }
        write.dstSet = desc_set;
        vkUpdateDescriptorSets(device, 1, &write, 0, NULL);
    }

    {
        VkCommandPoolCreateInfo pool_info = {
            .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
            .queueFamilyIndex = queue_family,
        };
        VkCommandBufferAllocateInfo alloc_info = {
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
            .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
            .commandBufferCount = 1,
        };
        result = vkCreateCommandPool(device, &pool_info, NULL, &cmd_pool);
        if (result != VK_SUCCESS) {
            fprintf(stderr, "vkCreateCommandPool failed: %d\n", result);
            goto cleanup;
        }
        alloc_info.commandPool = cmd_pool;
        result = vkAllocateCommandBuffers(device, &alloc_info, &cmd);
        if (result != VK_SUCCESS) {
            fprintf(stderr, "vkAllocateCommandBuffers failed: %d\n", result);
            goto cleanup;
        }
    }

    {
        VkFenceCreateInfo fence_info = {
            .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
        };
        result = vkCreateFence(device, &fence_info, NULL, &fence);
        if (result != VK_SUCCESS) {
            fprintf(stderr, "vkCreateFence failed: %d\n", result);
            goto cleanup;
        }
    }

    {
        VkCommandBufferBeginInfo begin_info = {
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        };
        workgroups_x = (opts.elements + opts.local_size - 1) / opts.local_size;
        result = vkBeginCommandBuffer(cmd, &begin_info);
        if (result != VK_SUCCESS) {
            fprintf(stderr, "vkBeginCommandBuffer failed: %d\n", result);
            goto cleanup;
        }
        vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline);
        vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, pipe_layout,
                                0, 1, &desc_set, 0, NULL);
        vkCmdDispatch(cmd, workgroups_x, 1, 1);
        result = vkEndCommandBuffer(cmd);
        if (result != VK_SUCCESS) {
            fprintf(stderr, "vkEndCommandBuffer failed: %d\n", result);
            goto cleanup;
        }
    }

    {
        VkSubmitInfo submit_info = {
            .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
            .commandBufferCount = 1,
            .pCommandBuffers = &cmd,
        };
        start_ns = monotonic_ns();
        result = vkQueueSubmit(queue, 1, &submit_info, fence);
        if (result != VK_SUCCESS) {
            fprintf(stderr, "vkQueueSubmit failed: %d\n", result);
            goto cleanup;
        }
        result = vkWaitForFences(device, 1, &fence, VK_TRUE, UINT64_MAX);
        end_ns = monotonic_ns();
        if (result != VK_SUCCESS) {
            fprintf(stderr, "vkWaitForFences failed: %d\n", result);
            goto cleanup;
        }
    }

    float_count = opts.elements * (opts.mode == OUTPUT_VEC4_F32 ? values_per_element : 1u);
    print_result_json(&opts, device_name, workgroups_x, buffer_bytes, end_ns - start_ns,
                      (const float *)output.mapped, float_count);

cleanup:
    if (fence) {
        vkDestroyFence(device, fence, NULL);
    }
    if (cmd_pool) {
        vkDestroyCommandPool(device, cmd_pool, NULL);
    }
    if (desc_pool) {
        vkDestroyDescriptorPool(device, desc_pool, NULL);
    }
    if (pipeline) {
        vkDestroyPipeline(device, pipeline, NULL);
    }
    if (pipe_layout) {
        vkDestroyPipelineLayout(device, pipe_layout, NULL);
    }
    if (desc_layout) {
        vkDestroyDescriptorSetLayout(device, desc_layout, NULL);
    }
    destroy_buffer_bundle(device, &output);
    if (shader_module) {
        vkDestroyShaderModule(device, shader_module, NULL);
    }
    if (device) {
        vkDestroyDevice(device, NULL);
    }
    if (instance) {
        vkDestroyInstance(instance, NULL);
    }
    return 0;
}
