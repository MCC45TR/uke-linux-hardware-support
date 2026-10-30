// SPDX-License-Identifier: MIT
#include <vulkan/vulkan.h>

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>

static void check(VkResult result, const char *operation)
{
    if (result != VK_SUCCESS) {
        std::fprintf(stderr, "%s failed: VkResult=%d\n", operation, result);
        std::exit(1);
    }
}

int main()
{
    // Omit WSI extensions: a display-query failure must not hide compute/transfer.
    VkInstanceCreateInfo instance_info{};
    instance_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    VkInstance instance = VK_NULL_HANDLE;
    check(vkCreateInstance(&instance_info, nullptr, &instance), "vkCreateInstance");
    uint32_t count = 0;
    check(vkEnumeratePhysicalDevices(instance, &count, nullptr), "count devices");
    std::vector<VkPhysicalDevice> devices(count);
    check(vkEnumeratePhysicalDevices(instance, &count, devices.data()), "list devices");
    unsigned tested = 0;
    for (uint32_t index = 0; index < count; ++index) {
        VkPhysicalDeviceProperties properties{};
        vkGetPhysicalDeviceProperties(devices[index], &properties);
        std::printf("device=%s type=%u vendor=0x%x api=%u.%u.%u\n",
                    properties.deviceName, properties.deviceType, properties.vendorID,
                    VK_VERSION_MAJOR(properties.apiVersion),
                    VK_VERSION_MINOR(properties.apiVersion),
                    VK_VERSION_PATCH(properties.apiVersion));
        if (properties.deviceType != VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU &&
            properties.deviceType != VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
            std::puts("skipped: device is not a physical GPU");
            continue;
        }
        uint32_t families = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(devices[index], &families, nullptr);
        std::vector<VkQueueFamilyProperties> queues(families);
        vkGetPhysicalDeviceQueueFamilyProperties(devices[index], &families, queues.data());
        uint32_t family = 0;
        while (family < families && !(queues[family].queueFlags &
               (VK_QUEUE_TRANSFER_BIT | VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT)))
            ++family;
        if (family == families) {
            std::fputs("no transfer-capable queue\n", stderr);
            return 1;
        }
        float priority = 1.0F;
        VkDeviceQueueCreateInfo queue_info{};
        queue_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queue_info.queueFamilyIndex = family;
        queue_info.queueCount = 1;
        queue_info.pQueuePriorities = &priority;
        VkDeviceCreateInfo device_info{};
        device_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        device_info.queueCreateInfoCount = 1;
        device_info.pQueueCreateInfos = &queue_info;
        VkDevice device = VK_NULL_HANDLE;
        check(vkCreateDevice(devices[index], &device_info, nullptr, &device), "vkCreateDevice");

        constexpr VkDeviceSize bytes = 4096;
        constexpr uint32_t pattern = 0x732a5a5a;
        VkBufferCreateInfo buffer_info{};
        buffer_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        buffer_info.size = bytes;
        buffer_info.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT;
        VkBuffer buffer = VK_NULL_HANDLE;
        check(vkCreateBuffer(device, &buffer_info, nullptr, &buffer), "vkCreateBuffer");
        VkMemoryRequirements requirements{};
        vkGetBufferMemoryRequirements(device, buffer, &requirements);
        VkPhysicalDeviceMemoryProperties memory_properties{};
        vkGetPhysicalDeviceMemoryProperties(devices[index], &memory_properties);
        uint32_t memory_type = 0;
        while (memory_type < memory_properties.memoryTypeCount &&
               (!(requirements.memoryTypeBits & (uint32_t{1} << memory_type)) ||
                !(memory_properties.memoryTypes[memory_type].propertyFlags &
                  VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT)))
            ++memory_type;
        if (memory_type == memory_properties.memoryTypeCount) {
            std::fputs("no host-visible transfer memory\n", stderr);
            return 1;
        }
        VkMemoryAllocateInfo allocation{};
        allocation.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocation.allocationSize = requirements.size;
        allocation.memoryTypeIndex = memory_type;
        VkDeviceMemory memory = VK_NULL_HANDLE;
        check(vkAllocateMemory(device, &allocation, nullptr, &memory), "vkAllocateMemory");
        check(vkBindBufferMemory(device, buffer, memory, 0), "vkBindBufferMemory");
        void *mapped = nullptr;
        check(vkMapMemory(device, memory, 0, VK_WHOLE_SIZE, 0, &mapped), "vkMapMemory");
        std::memset(mapped, 0, bytes);
        VkMappedMemoryRange range{};
        range.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
        range.memory = memory;
        range.size = VK_WHOLE_SIZE;
        check(vkFlushMappedMemoryRanges(device, 1, &range), "flush initial zeros");
        VkCommandPoolCreateInfo pool_info{};
        pool_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        pool_info.queueFamilyIndex = family;
        VkCommandPool pool = VK_NULL_HANDLE;
        check(vkCreateCommandPool(device, &pool_info, nullptr, &pool), "vkCreateCommandPool");
        VkCommandBufferAllocateInfo command_info{};
        command_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        command_info.commandPool = pool;
        command_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        command_info.commandBufferCount = 1;
        VkCommandBuffer command = VK_NULL_HANDLE;
        check(vkAllocateCommandBuffers(device, &command_info, &command), "allocate commands");
        VkCommandBufferBeginInfo begin{};
        begin.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        check(vkBeginCommandBuffer(command, &begin), "begin commands");
        vkCmdFillBuffer(command, buffer, 0, bytes, pattern);
        VkMemoryBarrier barrier{};
        barrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_HOST_READ_BIT;
        vkCmdPipelineBarrier(command, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_HOST_BIT,
                             0, 1, &barrier, 0, nullptr, 0, nullptr);
        check(vkEndCommandBuffer(command), "end commands");
        VkQueue queue = VK_NULL_HANDLE;
        vkGetDeviceQueue(device, family, 0, &queue);
        VkFenceCreateInfo fence_info{};
        fence_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        VkFence fence = VK_NULL_HANDLE;
        check(vkCreateFence(device, &fence_info, nullptr, &fence), "vkCreateFence");
        VkSubmitInfo submit{};
        submit.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submit.commandBufferCount = 1;
        submit.pCommandBuffers = &command;
        check(vkQueueSubmit(queue, 1, &submit, fence), "vkQueueSubmit");
        check(vkWaitForFences(device, 1, &fence, VK_TRUE, 5000000000ULL), "wait (5 seconds)");
        check(vkInvalidateMappedMemoryRanges(device, 1, &range), "invalidate mapped memory");
        const auto *words = static_cast<const uint32_t *>(mapped);
        for (unsigned i = 0; i < bytes / sizeof(*words); ++i) {
            if (words[i] != pattern) {
                std::fprintf(stderr, "transfer mismatch at word %u: 0x%x\n", i, words[i]);
                return 1;
            }
        }
        vkUnmapMemory(device, memory);
        vkDestroyFence(device, fence, nullptr);
        vkDestroyCommandPool(device, pool, nullptr);
        vkDestroyBuffer(device, buffer, nullptr);
        vkFreeMemory(device, memory, nullptr);
        vkDestroyDevice(device, nullptr);
        ++tested;
        std::puts("PASS: GPU fill, fence and 1024-word readback; presentation/codecs untested");
    }
    vkDestroyInstance(instance, nullptr);
    if (tested == 0) {
        std::fputs("no physical GPU tested; software enumeration is not a pass\n", stderr);
        return 2;
    }
    return 0;
}
