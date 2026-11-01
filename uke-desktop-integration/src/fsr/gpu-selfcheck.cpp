// SPDX-License-Identifier: MIT
#include <vulkan/vulkan.h>
#include <FidelityFX/host/backends/vk/ffx_vk.h>
#include <FidelityFX/host/ffx_opticalflow.h>
#include <FidelityFX/host/ffx_frameinterpolation.h>
#include <cstddef>
#include <cstdio>
#include <cstring>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

static void vkcheck(VkResult result, const char* operation)
{
    if (result != VK_SUCCESS)
        throw std::runtime_error(std::string(operation) + ": VkResult=" + std::to_string(result));
}

static void ffxcheck(FfxErrorCode result, const char* operation)
{
    if (result != FFX_OK)
        throw std::runtime_error(std::string(operation) + ": FfxErrorCode=" + std::to_string(result));
}

int main()
try {
    std::setvbuf(stdout, nullptr, _IONBF, 0);
    VkApplicationInfo application{};
    application.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    application.apiVersion = VK_API_VERSION_1_3;
    VkInstanceCreateInfo instanceInfo{};
    instanceInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    instanceInfo.pApplicationInfo = &application;
    VkInstance instance = VK_NULL_HANDLE;
    vkcheck(vkCreateInstance(&instanceInfo, nullptr, &instance), "create instance");
    uint32_t count = 0;
    vkcheck(vkEnumeratePhysicalDevices(instance, &count, nullptr), "count devices");
    std::vector<VkPhysicalDevice> devices(count);
    vkcheck(vkEnumeratePhysicalDevices(instance, &count, devices.data()), "list devices");
    VkPhysicalDevice physical = VK_NULL_HANDLE;
    for (auto candidate : devices) {
        VkPhysicalDeviceProperties properties{};
        vkGetPhysicalDeviceProperties(candidate, &properties);
        if (properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU ||
            properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
            physical = candidate;
            std::printf("physical_gpu=%s api=%u\n", properties.deviceName, properties.apiVersion);
            break;
        }
    }
    if (!physical) throw std::runtime_error("no physical GPU; refusing software fallback");
    vkGetPhysicalDeviceQueueFamilyProperties(physical, &count, nullptr);
    std::vector<VkQueueFamilyProperties> queues(count);
    vkGetPhysicalDeviceQueueFamilyProperties(physical, &count, queues.data());
    uint32_t family = 0;
    constexpr auto required = VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT;
    while (family < count && (queues[family].queueFlags & required) != required) ++family;
    if (family == count) throw std::runtime_error("no graphics/compute queue");

    VkPhysicalDeviceVulkan13Features available13{};
    available13.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES;
    VkPhysicalDeviceVulkan12Features available12{};
    available12.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES;
    available12.pNext = &available13;
    VkPhysicalDeviceVulkan11Features available11{};
    available11.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES;
    available11.pNext = &available12;
    VkPhysicalDeviceFeatures2 available{};
    available.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
    available.pNext = &available11;
    vkGetPhysicalDeviceFeatures2(physical, &available);
    VkPhysicalDeviceVulkan13Features features13{};
    features13.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES;
    features13.subgroupSizeControl = available13.subgroupSizeControl;
    features13.computeFullSubgroups = available13.computeFullSubgroups;
    VkPhysicalDeviceVulkan12Features features12{};
    features12.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES;
    features12.pNext = &features13;
    features12.shaderFloat16 = available12.shaderFloat16;
    features12.shaderSubgroupExtendedTypes = available12.shaderSubgroupExtendedTypes;
    features12.descriptorBindingPartiallyBound = available12.descriptorBindingPartiallyBound;
    features12.runtimeDescriptorArray = available12.runtimeDescriptorArray;
    features12.shaderSampledImageArrayNonUniformIndexing = available12.shaderSampledImageArrayNonUniformIndexing;
    features12.shaderStorageImageArrayNonUniformIndexing = available12.shaderStorageImageArrayNonUniformIndexing;
    VkPhysicalDeviceVulkan11Features features11{};
    features11.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES;
    features11.pNext = &features12;
    features11.storageBuffer16BitAccess = available11.storageBuffer16BitAccess;
    features11.uniformAndStorageBuffer16BitAccess = available11.uniformAndStorageBuffer16BitAccess;
    features11.storagePushConstant16 = available11.storagePushConstant16;
    features11.storageInputOutput16 = available11.storageInputOutput16;
    VkPhysicalDeviceFeatures features{};
    features.shaderInt16 = available.features.shaderInt16;
    features.shaderInt64 = available.features.shaderInt64;
    features.shaderStorageImageExtendedFormats = available.features.shaderStorageImageExtendedFormats;
    features.shaderStorageImageReadWithoutFormat = available.features.shaderStorageImageReadWithoutFormat;
    features.shaderStorageImageWriteWithoutFormat = available.features.shaderStorageImageWriteWithoutFormat;
    float priority = 1.0F;
    VkDeviceQueueCreateInfo queueInfo{};
    queueInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queueInfo.queueFamilyIndex = family;
    queueInfo.queueCount = 1;
    queueInfo.pQueuePriorities = &priority;
    VkDeviceCreateInfo deviceInfo{};
    deviceInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    deviceInfo.pNext = &features11;
    deviceInfo.pEnabledFeatures = &features;
    deviceInfo.queueCreateInfoCount = 1;
    deviceInfo.pQueueCreateInfos = &queueInfo;
    const char* extensions[] = {VK_KHR_GET_MEMORY_REQUIREMENTS_2_EXTENSION_NAME,
                                VK_KHR_DEDICATED_ALLOCATION_EXTENSION_NAME};
    vkcheck(vkEnumerateDeviceExtensionProperties(physical, nullptr, &count, nullptr), "count extensions");
    std::vector<VkExtensionProperties> supportedExtensions(count);
    vkcheck(vkEnumerateDeviceExtensionProperties(physical, nullptr, &count, supportedExtensions.data()), "list extensions");
    for (const auto* extension : extensions) {
        bool supported = false;
        for (const auto& candidate : supportedExtensions)
            supported |= std::strcmp(candidate.extensionName, extension) == 0;
        if (!supported) throw std::runtime_error(std::string("missing SDK extension: ") + extension);
    }
    deviceInfo.enabledExtensionCount = 2;
    deviceInfo.ppEnabledExtensionNames = extensions;
    VkDevice device = VK_NULL_HANDLE;
    vkcheck(vkCreateDevice(physical, &deviceInfo, nullptr, &device), "create device");
    if (!vkGetDeviceProcAddr(device, "vkGetBufferMemoryRequirements2KHR"))
        throw std::runtime_error("SDK requires the KHR memory-requirements entry point");
    VkDeviceContext deviceContext{device, physical, vkGetDeviceProcAddr};
    constexpr size_t contexts = FFX_OPTICALFLOW_CONTEXT_COUNT + FFX_FRAMEINTERPOLATION_CONTEXT_COUNT;
    const auto scratchBytes = ffxGetScratchMemorySizeVK(physical, contexts);
    std::printf("backend_scratch_bytes=%zu\n", scratchBytes);
    if (!scratchBytes || scratchBytes > 256U * 1024U * 1024U)
        throw std::runtime_error("scratch allocation outside bounded test budget");
    std::vector<std::max_align_t> scratch((scratchBytes + sizeof(std::max_align_t) - 1) / sizeof(std::max_align_t));
    FfxInterface backend{};
    ffxcheck(ffxGetInterfaceVK(&backend, ffxGetDeviceVK(&deviceContext), scratch.data(), scratchBytes, contexts), "backend interface");
    auto flow = std::make_unique<FfxOpticalflowContext>();
    FfxOpticalflowContextDescription flowInfo{};
    flowInfo.backendInterface = backend;
    flowInfo.resolution = {128, 128};
    std::puts("creating_optical_flow_pipelines");
    ffxcheck(ffxOpticalflowContextCreate(flow.get(), &flowInfo), "optical flow context");
    auto interpolation = std::make_unique<FfxFrameInterpolationContext>();
    FfxFrameInterpolationContextDescription interpolationInfo{};
    interpolationInfo.backendInterface = backend;
    interpolationInfo.maxRenderSize = {128, 128};
    interpolationInfo.displaySize = {128, 128};
    interpolationInfo.backBufferFormat = FFX_SURFACE_FORMAT_R8G8B8A8_UNORM;
    interpolationInfo.previousInterpolationSourceFormat = FFX_SURFACE_FORMAT_R8G8B8A8_UNORM;
    std::puts("creating_interpolation_pipelines");
    ffxcheck(ffxFrameInterpolationContextCreate(interpolation.get(), &interpolationInfo), "interpolation context");
    vkcheck(vkDeviceWaitIdle(device), "device idle");
    ffxcheck(ffxFrameInterpolationContextDestroy(interpolation.get()), "destroy interpolation");
    ffxcheck(ffxOpticalflowContextDestroy(flow.get()), "destroy optical flow");
    vkDestroyDevice(device, nullptr);
    vkDestroyInstance(instance, nullptr);
    std::puts("PASS: physical GPU SDK pipeline initialization; no effect dispatch or presentation");
    return 0;
} catch (const std::exception& error) {
    std::fprintf(stderr, "FAIL: %s\n", error.what());
    return 1;
}
