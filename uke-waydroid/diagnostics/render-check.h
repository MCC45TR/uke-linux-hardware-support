// SPDX-License-Identifier: MIT
// Bounded offscreen graphics qualification; no display or game conformance claim.
#include "triangle-vertex.h"
#include "triangle-fragment.h"

struct RenderResources {
    VkDevice device;
    VkImage image = VK_NULL_HANDLE;
    VkDeviceMemory memory = VK_NULL_HANDLE;
    VkImageView view = VK_NULL_HANDLE;
    VkRenderPass pass = VK_NULL_HANDLE;
    VkFramebuffer framebuffer = VK_NULL_HANDLE;
    VkShaderModule vertex = VK_NULL_HANDLE, fragment = VK_NULL_HANDLE;
    VkPipelineLayout layout = VK_NULL_HANDLE;
    VkPipeline pipeline = VK_NULL_HANDLE;
    explicit RenderResources(VkDevice value) : device(value) {}
    ~RenderResources() {
        if (pipeline) vkDestroyPipeline(device, pipeline, nullptr);
        if (layout) vkDestroyPipelineLayout(device, layout, nullptr);
        if (vertex) vkDestroyShaderModule(device, vertex, nullptr);
        if (fragment) vkDestroyShaderModule(device, fragment, nullptr);
        if (framebuffer) vkDestroyFramebuffer(device, framebuffer, nullptr);
        if (pass) vkDestroyRenderPass(device, pass, nullptr);
        if (view) vkDestroyImageView(device, view, nullptr);
        if (image) vkDestroyImage(device, image, nullptr);
        if (memory) vkFreeMemory(device, memory, nullptr);
    }
};

static int render_check(Resources &r, VkPhysicalDevice gpu, VkQueue queue, VkCommandBuffer command,
                        const VkPhysicalDeviceMemoryProperties &memories, uint32_t readback_type) {
    RenderResources render(r.device);
    constexpr uint32_t side = 32, frames = 16;
    constexpr VkFormat format = VK_FORMAT_R8G8B8A8_UNORM;
    VkFormatProperties formats{};
    vkGetPhysicalDeviceFormatProperties(gpu, format, &formats);
    constexpr VkFormatFeatureFlags needed = VK_FORMAT_FEATURE_COLOR_ATTACHMENT_BIT | VK_FORMAT_FEATURE_TRANSFER_SRC_BIT;
    if ((formats.optimalTilingFeatures & needed) != needed)
        return failed("render-format", VK_ERROR_FORMAT_NOT_SUPPORTED);
    VkImageCreateInfo image{};
    image.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    image.imageType = VK_IMAGE_TYPE_2D;
    image.format = format;
    image.extent = {side, side, 1};
    image.mipLevels = image.arrayLayers = 1;
    image.samples = VK_SAMPLE_COUNT_1_BIT;
    image.tiling = VK_IMAGE_TILING_OPTIMAL;
    image.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
    image.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    VkResult result = vkCreateImage(r.device, &image, nullptr, &render.image);
    if (result != VK_SUCCESS) return failed("render-image", result);
    VkMemoryRequirements requirements{};
    vkGetImageMemoryRequirements(r.device, render.image, &requirements);
    if (requirements.size > 64 * 1024 * 1024) return failed("render-memory-bound", VK_ERROR_UNKNOWN);
    uint32_t memory_type = UINT32_MAX;
    for (uint32_t i = 0; i < memories.memoryTypeCount; ++i) {
        if (!(requirements.memoryTypeBits & (1u << i))) continue;
        memory_type = i;
        if (memories.memoryTypes[i].propertyFlags & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT) break;
    }
    if (memory_type == UINT32_MAX) return failed("render-memory-type", VK_ERROR_INITIALIZATION_FAILED);
    VkMemoryAllocateInfo allocation{};
    allocation.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocation.allocationSize = requirements.size;
    allocation.memoryTypeIndex = memory_type;
    result = vkAllocateMemory(r.device, &allocation, nullptr, &render.memory);
    if (result != VK_SUCCESS) return failed("render-allocation", result);
    result = vkBindImageMemory(r.device, render.image, render.memory, 0);
    if (result != VK_SUCCESS) return failed("render-bind", result);
    VkImageViewCreateInfo view{};
    view.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    view.image = render.image;
    view.viewType = VK_IMAGE_VIEW_TYPE_2D;
    view.format = format;
    view.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};
    result = vkCreateImageView(r.device, &view, nullptr, &render.view);
    if (result != VK_SUCCESS) return failed("render-view", result);
    VkAttachmentDescription attachment{};
    attachment.format = format;
    attachment.samples = VK_SAMPLE_COUNT_1_BIT;
    attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    attachment.finalLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
    VkAttachmentReference reference{0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL};
    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &reference;
    VkSubpassDependency dependency{};
    dependency.srcSubpass = 0;
    dependency.dstSubpass = VK_SUBPASS_EXTERNAL;
    dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.dstStageMask = VK_PIPELINE_STAGE_TRANSFER_BIT;
    dependency.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    dependency.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
    VkRenderPassCreateInfo pass{};
    pass.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    pass.attachmentCount = pass.subpassCount = pass.dependencyCount = 1;
    pass.pAttachments = &attachment;
    pass.pSubpasses = &subpass;
    pass.pDependencies = &dependency;
    result = vkCreateRenderPass(r.device, &pass, nullptr, &render.pass);
    if (result != VK_SUCCESS) return failed("render-pass", result);
    VkFramebufferCreateInfo framebuffer{};
    framebuffer.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    framebuffer.renderPass = render.pass;
    framebuffer.attachmentCount = 1;
    framebuffer.pAttachments = &render.view;
    framebuffer.width = framebuffer.height = side;
    framebuffer.layers = 1;
    result = vkCreateFramebuffer(r.device, &framebuffer, nullptr, &render.framebuffer);
    if (result != VK_SUCCESS) return failed("render-framebuffer", result);
    VkShaderModuleCreateInfo shader{};
    shader.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    shader.codeSize = sizeof(uke_triangle_vertex);
    shader.pCode = uke_triangle_vertex;
    result = vkCreateShaderModule(r.device, &shader, nullptr, &render.vertex);
    if (result != VK_SUCCESS) return failed("vertex-shader", result);
    shader.codeSize = sizeof(uke_triangle_fragment);
    shader.pCode = uke_triangle_fragment;
    result = vkCreateShaderModule(r.device, &shader, nullptr, &render.fragment);
    if (result != VK_SUCCESS) return failed("fragment-shader", result);
    VkPushConstantRange push{VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(uint32_t)};
    VkPipelineLayoutCreateInfo layout{};
    layout.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    layout.pushConstantRangeCount = 1;
    layout.pPushConstantRanges = &push;
    result = vkCreatePipelineLayout(r.device, &layout, nullptr, &render.layout);
    if (result != VK_SUCCESS) return failed("render-layout", result);
    VkPipelineShaderStageCreateInfo stages[2]{};
    stages[0].sType = stages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    stages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
    stages[0].module = render.vertex;
    stages[0].pName = "main";
    stages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    stages[1].module = render.fragment;
    stages[1].pName = "main";
    VkPipelineVertexInputStateCreateInfo vertex_input{};
    vertex_input.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    VkPipelineInputAssemblyStateCreateInfo assembly{};
    assembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    assembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    VkViewport viewport{0.0f, 0.0f, float(side), float(side), 0.0f, 1.0f};
    VkRect2D scissor{{0, 0}, {side, side}};
    VkPipelineViewportStateCreateInfo viewports{};
    viewports.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewports.viewportCount = viewports.scissorCount = 1;
    viewports.pViewports = &viewport;
    viewports.pScissors = &scissor;
    VkPipelineRasterizationStateCreateInfo raster{};
    raster.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    raster.polygonMode = VK_POLYGON_MODE_FILL;
    raster.cullMode = VK_CULL_MODE_NONE;
    raster.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    raster.lineWidth = 1.0f;
    VkPipelineMultisampleStateCreateInfo samples{};
    samples.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    samples.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    VkPipelineColorBlendAttachmentState blend{};
    blend.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    VkPipelineColorBlendStateCreateInfo blends{};
    blends.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    blends.attachmentCount = 1;
    blends.pAttachments = &blend;
    VkGraphicsPipelineCreateInfo pipeline{};
    pipeline.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipeline.stageCount = 2;
    pipeline.pStages = stages;
    pipeline.pVertexInputState = &vertex_input;
    pipeline.pInputAssemblyState = &assembly;
    pipeline.pViewportState = &viewports;
    pipeline.pRasterizationState = &raster;
    pipeline.pMultisampleState = &samples;
    pipeline.pColorBlendState = &blends;
    pipeline.layout = render.layout;
    pipeline.renderPass = render.pass;
    result = vkCreateGraphicsPipelines(r.device, VK_NULL_HANDLE, 1, &pipeline, nullptr, &render.pipeline);
    if (result != VK_SUCCESS) return failed("graphics-pipeline", result);
    uint32_t mismatches = 0;
    for (uint32_t frame = 0; frame < frames; ++frame) {
        result = vkResetCommandPool(r.device, r.pool, 0);
        if (result != VK_SUCCESS) return failed("render-reset-pool", result);
        result = vkResetFences(r.device, 1, &r.fence);
        if (result != VK_SUCCESS) return failed("render-reset-fence", result);
        VkCommandBufferBeginInfo begin{};
        begin.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        begin.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
        result = vkBeginCommandBuffer(command, &begin);
        if (result != VK_SUCCESS) return failed("render-begin", result);
        VkClearValue clear{};
        clear.color.float32[3] = 1.0f;
        VkRenderPassBeginInfo render_begin{};
        render_begin.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        render_begin.renderPass = render.pass;
        render_begin.framebuffer = render.framebuffer;
        render_begin.renderArea = scissor;
        render_begin.clearValueCount = 1;
        render_begin.pClearValues = &clear;
        vkCmdBeginRenderPass(command, &render_begin, VK_SUBPASS_CONTENTS_INLINE);
        vkCmdBindPipeline(command, VK_PIPELINE_BIND_POINT_GRAPHICS, render.pipeline);
        vkCmdPushConstants(command, render.layout, VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(frame), &frame);
        vkCmdDraw(command, 3, 1, 0, 0);
        vkCmdEndRenderPass(command);
        VkBufferImageCopy copy{};
        copy.imageSubresource = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1};
        copy.imageExtent = {side, side, 1};
        vkCmdCopyImageToBuffer(command, render.image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, r.buffer, 1, &copy);
        VkMemoryBarrier barrier{};
        barrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_HOST_READ_BIT;
        vkCmdPipelineBarrier(command, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_HOST_BIT,
                             0, 1, &barrier, 0, nullptr, 0, nullptr);
        result = vkEndCommandBuffer(command);
        if (result != VK_SUCCESS) return failed("render-end", result);
        VkSubmitInfo submit{};
        submit.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submit.commandBufferCount = 1;
        submit.pCommandBuffers = &command;
        result = vkQueueSubmit(queue, 1, &submit, r.fence);
        if (result != VK_SUCCESS) return failed("render-submit", result);
        result = vkWaitForFences(r.device, 1, &r.fence, VK_TRUE, 5000000000ULL);
        if (result != VK_SUCCESS) { failed("render-wait", result); fflush(stdout); _Exit(1); }
        void *mapped = nullptr;
        result = vkMapMemory(r.device, r.memory, 0, VK_WHOLE_SIZE, 0, &mapped);
        if (result != VK_SUCCESS) return failed("render-map", result);
        if (!(memories.memoryTypes[readback_type].propertyFlags & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)) {
            VkMappedMemoryRange range{};
            range.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
            range.memory = r.memory;
            range.size = VK_WHOLE_SIZE;
            result = vkInvalidateMappedMemoryRanges(r.device, 1, &range);
            if (result != VK_SUCCESS) { vkUnmapMemory(r.device, r.memory); return failed("render-invalidate", result); }
        }
        const auto *pixels = static_cast<const uint8_t *>(mapped);
        for (uint32_t y = 0; y < side; ++y) for (uint32_t x = 0; x < side; ++x) {
            // Pixel centers never coincide with the triangle's sloping edges.
            const int dx = int(2 * x + 1) - int(side);
            const bool inside = 2 * abs(dx) < int(2 * side - (2 * y + 1));
            const uint8_t red = inside && !(frame & 1) ? 255 : 0;
            const uint8_t green = inside && (frame & 1) ? 255 : 0;
            const auto *pixel = pixels + 4 * (y * side + x);
            if (pixel[0] != red || pixel[1] != green || pixel[2] != 0 || pixel[3] != 255) ++mismatches;
        }
        vkUnmapMemory(r.device, r.memory);
    }
    printf("{\"graphics_frames\":%u,\"width\":%u,\"height\":%u,\"checked_pixels\":%u,\"pixel_mismatches\":%u,\"passed\":%s}\n",
           frames, side, side, frames * side * side, mismatches, mismatches ? "false" : "true");
    return mismatches ? 1 : 0;
}
