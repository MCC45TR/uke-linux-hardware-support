// SPDX-License-Identifier: MIT
// Android-side Vulkan enumeration and bounded 4 KiB GPU fill/readback.
#include <vulkan/vulkan.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

struct Resources {
    VkInstance instance = VK_NULL_HANDLE;
    VkDevice device = VK_NULL_HANDLE;
    VkBuffer buffer = VK_NULL_HANDLE;
    VkDeviceMemory memory = VK_NULL_HANDLE;
    VkCommandPool pool = VK_NULL_HANDLE;
    VkFence fence = VK_NULL_HANDLE;
    ~Resources() {
        if (device) {
            if (fence) vkDestroyFence(device, fence, nullptr);
            if (pool) vkDestroyCommandPool(device, pool, nullptr);
            if (buffer) vkDestroyBuffer(device, buffer, nullptr);
            if (memory) vkFreeMemory(device, memory, nullptr);
            vkDestroyDevice(device, nullptr);
        }
        if (instance) vkDestroyInstance(instance, nullptr);
    }
};

static int failed(const char *step, VkResult result) {
    printf("{\"step\":\"%s\",\"vk_result\":%d,\"passed\":false}\n", step, result);
    return 1;
}

int main() {
    Resources r;
    VkApplicationInfo app{};
    app.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    app.pApplicationName = "Uke Waydroid Vulkan check";
    app.apiVersion = VK_API_VERSION_1_1;
    VkInstanceCreateInfo instance_info{};
    instance_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    instance_info.pApplicationInfo = &app;
    VkResult result = vkCreateInstance(&instance_info, nullptr, &r.instance);
    if (result != VK_SUCCESS) return failed("instance", result);
    uint32_t count = 0;
    result = vkEnumeratePhysicalDevices(r.instance, &count, nullptr);
    if (result != VK_SUCCESS || count == 0) return failed("enumeration", result);
    if (count > 16) return failed("device-count-bound", VK_ERROR_UNKNOWN);
    VkPhysicalDevice devices[16]{};
    result = vkEnumeratePhysicalDevices(r.instance, &count, devices);
    if (result != VK_SUCCESS) return failed("devices", result);
    VkPhysicalDevice gpu = VK_NULL_HANDLE;
    VkPhysicalDeviceProperties properties{};
    for (uint32_t i = 0; i < count; ++i) {
        vkGetPhysicalDeviceProperties(devices[i], &properties);
        if (properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU ||
            properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
            gpu = devices[i];
            break;
        }
    }
    if (!gpu) return failed("hardware-gpu", VK_ERROR_INITIALIZATION_FAILED);
    printf("{\"device\":\"");
    for (const unsigned char *p = reinterpret_cast<const unsigned char *>(properties.deviceName); *p; ++p) {
        if (*p == '\"' || *p == '\\') putchar('\\');
        if (*p < 32) printf("\\u%04x", *p); else putchar(*p);
    }
    printf("\",\"api\":\"%u.%u.%u\",\"vendor_id\":%u,\"device_id\":%u,\"software\":false}\n",
           VK_API_VERSION_MAJOR(properties.apiVersion), VK_API_VERSION_MINOR(properties.apiVersion),
           VK_API_VERSION_PATCH(properties.apiVersion), properties.vendorID, properties.deviceID);
    uint32_t queue_count = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(gpu, &queue_count, nullptr);
    if (!queue_count || queue_count > 64) return failed("queue-count", VK_ERROR_UNKNOWN);
    VkQueueFamilyProperties queues[64]{};
    vkGetPhysicalDeviceQueueFamilyProperties(gpu, &queue_count, queues);
    uint32_t family = UINT32_MAX;
    for (uint32_t i = 0; i < queue_count; ++i)
        if (queues[i].queueCount && (queues[i].queueFlags & VK_QUEUE_COMPUTE_BIT)) { family = i; break; }
    if (family == UINT32_MAX) return failed("compute-queue", VK_ERROR_INITIALIZATION_FAILED);
    float priority = 1.0f;
    VkDeviceQueueCreateInfo queue_info{};
    queue_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queue_info.queueFamilyIndex = family;
    queue_info.queueCount = 1;
    queue_info.pQueuePriorities = &priority;
    VkDeviceCreateInfo device_info{};
    device_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    device_info.queueCreateInfoCount = 1;
    device_info.pQueueCreateInfos = &queue_info;
    result = vkCreateDevice(gpu, &device_info, nullptr, &r.device);
    if (result != VK_SUCCESS) return failed("device", result);
    VkQueue queue = VK_NULL_HANDLE;
    vkGetDeviceQueue(r.device, family, 0, &queue);
    constexpr VkDeviceSize bytes = 4096;
    constexpr uint32_t pattern = 0x5a73c491;
    VkBufferCreateInfo buffer_info{};
    buffer_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    buffer_info.size = bytes;
    buffer_info.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT;
    buffer_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    result = vkCreateBuffer(r.device, &buffer_info, nullptr, &r.buffer);
    if (result != VK_SUCCESS) return failed("buffer", result);
    VkMemoryRequirements requirements{};
    vkGetBufferMemoryRequirements(r.device, r.buffer, &requirements);
    if (requirements.size > 64 * 1024 * 1024) return failed("memory-bound", VK_ERROR_UNKNOWN);
    VkPhysicalDeviceMemoryProperties memories{};
    vkGetPhysicalDeviceMemoryProperties(gpu, &memories);
    uint32_t memory_type = UINT32_MAX;
    for (uint32_t i = 0; i < memories.memoryTypeCount; ++i)
        if ((requirements.memoryTypeBits & (1u << i)) &&
            (memories.memoryTypes[i].propertyFlags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT)) {
            memory_type = i;
            if (memories.memoryTypes[i].propertyFlags & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT) break;
        }
    if (memory_type == UINT32_MAX) return failed("host-visible-memory", VK_ERROR_INITIALIZATION_FAILED);
    VkMemoryAllocateInfo allocation{};
    allocation.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocation.allocationSize = requirements.size;
    allocation.memoryTypeIndex = memory_type;
    result = vkAllocateMemory(r.device, &allocation, nullptr, &r.memory);
    if (result != VK_SUCCESS) return failed("allocation", result);
    result = vkBindBufferMemory(r.device, r.buffer, r.memory, 0);
    if (result != VK_SUCCESS) return failed("bind", result);
    VkCommandPoolCreateInfo pool_info{};
    pool_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    pool_info.queueFamilyIndex = family;
    result = vkCreateCommandPool(r.device, &pool_info, nullptr, &r.pool);
    if (result != VK_SUCCESS) return failed("pool", result);
    VkCommandBufferAllocateInfo command_info{};
    command_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    command_info.commandPool = r.pool;
    command_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    command_info.commandBufferCount = 1;
    VkCommandBuffer command = VK_NULL_HANDLE;
    result = vkAllocateCommandBuffers(r.device, &command_info, &command);
    if (result != VK_SUCCESS) return failed("command", result);
    VkCommandBufferBeginInfo begin{};
    begin.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    begin.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    result = vkBeginCommandBuffer(command, &begin);
    if (result != VK_SUCCESS) return failed("begin", result);
    vkCmdFillBuffer(command, r.buffer, 0, bytes, pattern);
    VkMemoryBarrier barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
    barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    barrier.dstAccessMask = VK_ACCESS_HOST_READ_BIT;
    vkCmdPipelineBarrier(command, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_HOST_BIT,
                         0, 1, &barrier, 0, nullptr, 0, nullptr);
    result = vkEndCommandBuffer(command);
    if (result != VK_SUCCESS) return failed("end", result);
    VkFenceCreateInfo fence_info{};
    fence_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    result = vkCreateFence(r.device, &fence_info, nullptr, &r.fence);
    if (result != VK_SUCCESS) return failed("fence", result);
    VkSubmitInfo submit{};
    submit.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit.commandBufferCount = 1;
    submit.pCommandBuffers = &command;
    result = vkQueueSubmit(queue, 1, &submit, r.fence);
    if (result != VK_SUCCESS) return failed("submit", result);
    result = vkWaitForFences(r.device, 1, &r.fence, VK_TRUE, 5000000000ULL);
    // Do not destroy resources while a timed-out queue could still use them.
    if (result != VK_SUCCESS) { failed("wait", result); fflush(stdout); _Exit(1); }
    void *mapped = nullptr;
    result = vkMapMemory(r.device, r.memory, 0, VK_WHOLE_SIZE, 0, &mapped);
    if (result != VK_SUCCESS) return failed("map", result);
    if (!(memories.memoryTypes[memory_type].propertyFlags & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)) {
        VkMappedMemoryRange range{};
        range.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
        range.memory = r.memory;
        range.size = VK_WHOLE_SIZE;
        result = vkInvalidateMappedMemoryRanges(r.device, 1, &range);
        if (result != VK_SUCCESS) { vkUnmapMemory(r.device, r.memory); return failed("invalidate", result); }
    }
    const auto *words = static_cast<const uint32_t *>(mapped);
    uint32_t mismatches = 0;
    for (uint32_t i = 0; i < bytes / sizeof(uint32_t); ++i) if (words[i] != pattern) ++mismatches;
    vkUnmapMemory(r.device, r.memory);
    printf("{\"gpu_fill_bytes\":4096,\"mismatches\":%u,\"passed\":%s}\n",
           mismatches, mismatches ? "false" : "true");
    return mismatches ? 1 : 0;
}
