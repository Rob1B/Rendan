#include "VulkanRenderer.hpp"
#include "Logger.hpp"
#include "VulkanMesh.hpp"
#include "VulkanUtils.hpp"
#include <vector>
#define VMA_IMPLEMENTATION
#include "vk_mem_alloc.h"

VulkanRenderer::VulkanRenderer(GLFWwindow *window) : window(window)
{
    try {
        context = std::make_shared<VulkanContext>(window);
        device = std::make_shared<VulkanDevice>(context);
        vkallocator = createAllocator();
        semaphores = std::make_shared<VulkanSemaphores>(context, device);
        swapchain = std::make_shared<VulkanSwapchain>(window, context, device);
        renderPass = std::make_shared<VulkanRenderPass>(context, device, swapchain);
        framebuffers = std::make_shared<VulkanFramebuffers>(context, device, swapchain, renderPass);
        graphicPipeline = std::make_shared<GraphicPipeline>(context, device, swapchain, renderPass);
        commandPool = std::make_shared<VulkanCommandPool>(context, device);
        commandBuffer = std::make_shared<VulkanCommandBuffers>(context, device, framebuffers, commandPool);
    } catch (VulkanInitialisationException &e) {
        Logger::Error(e.what());
        throw std::runtime_error("Impossible to initialiaze Vulkan");
    }
}

VulkanRenderer::~VulkanRenderer()
{
    delete mesh;
    commandBuffer.reset();
    commandPool.reset();
    graphicPipeline.reset();
    framebuffers.reset();
    renderPass.reset();
    swapchain.reset();
    semaphores.reset();
    vmaDestroyAllocator(vkallocator);
    device.reset();
    context.reset();
}

void VulkanRenderer::present()
{
    uint32_t imageIndex;
    auto commandBuffers = commandBuffer->getCommandBuffers();
    vkAcquireNextImageKHR(device->getDevice(), swapchain->getSwapchain(), UINT64_MAX,
                          semaphores->getAvailableSemaphore(), VK_NULL_HANDLE, &imageIndex);

    VkSemaphore waitSemaphores[] = {semaphores->getAvailableSemaphore()};
    VkSemaphore signalSemaphores[] = {semaphores->getFinishedSemaphore()};
    VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
    VkSubmitInfo submitInfo{
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = waitSemaphores,
        .pWaitDstStageMask = waitStages,
        .commandBufferCount = 1,
        .pCommandBuffers = &commandBuffers[imageIndex],
        .signalSemaphoreCount = 1,
        .pSignalSemaphores = signalSemaphores,
    };

    if (vkQueueSubmit(device->getGraphicQueue(), 1, &submitInfo, VK_NULL_HANDLE) != VK_SUCCESS) {
        throw std::runtime_error("Impossible to submit draw command buffer");
    }

    VkSwapchainKHR swapChains[] = {swapchain->getSwapchain()};
    VkPresentInfoKHR presentInfo{
        .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = signalSemaphores,
        .swapchainCount = 1,
        .pSwapchains = swapChains,
        .pImageIndices = &imageIndex,
        .pResults = nullptr,// Optional
    };
    vkQueuePresentKHR(device->getPresentQueue(), &presentInfo);
    vkQueueWaitIdle(device->getPresentQueue());
}

void VulkanRenderer::render()
{
    // Create the verte
    const std::vector<Vertex> vertices = {
        {{-0.5f, -0.5f, 0.f}, {1.0f, 0.0f, 0.0f}},
        {{0.5f, -0.5f, 0.f}, {0.0f, 1.0f, 0.0f}},
        {{0.5f, 0.5f, 0.f}, {0.0f, 0.0f, 1.0f}},
        {{-0.5f, 0.5f, 0.f}, {1.0f, 1.0f, 1.0f}},

    };

    const std::vector<uint32_t> indices = {0, 1, 2, 2, 3, 0};


    mesh = new Mesh(vkallocator, vertices, indices);
    mesh->load();

    auto commandBuffers = commandBuffer->getCommandBuffers();
    auto _framebuffer = framebuffers->getFramebuffers();
    for (int i = 0; i < _framebuffer.size(); ++i) {
        VulkanCommandBuffers::beginRecording(commandBuffers[i]);
        renderPass->beginRenderPass(commandBuffers[i], _framebuffer[i]);
        vkCmdBindPipeline(commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, graphicPipeline->getPipeline());
        VkDeviceSize offset = 0;
        vkCmdBindVertexBuffers(commandBuffers[i], 0, 1, &mesh->getVertexBuffer(), &offset);
        if (mesh->getIndices().empty()) {
            Logger::Info("Drawing without index");
            vkCmdDraw(commandBuffers[i], mesh->getVertices().size(), 1, 0, 0);
        } else {
            Logger::Info("Drawing with index");
            vkCmdBindIndexBuffer(commandBuffers[i], mesh->getIndexBuffer(), offset, VK_INDEX_TYPE_UINT32);
            vkCmdDrawIndexed(commandBuffers[i], static_cast<uint32_t>(mesh->getIndices().size()), 1, 0, 0, 0);
        }
        renderPass->endRenderPass(commandBuffers[i]);
        VulkanCommandBuffers::endRecording(commandBuffers[i]);
    }
}

void VulkanRenderer::end() { vkDeviceWaitIdle(device->getDevice()); }


VmaAllocator VulkanRenderer::createAllocator()
{
    if (context == nullptr || device == nullptr) {
        throw VulkanInitialisationException("Impossible to create the allocator");
    }
    VmaAllocatorCreateInfo allocatorInfo = {
        .physicalDevice = context->getPhysicalDevice(),
        .device = device->getDevice(),
        .instance = context->getInstance(),
        .vulkanApiVersion = VK_API_VERSION_1_1,
    };

    VmaAllocator vkallocator;
    vmaCreateAllocator(&allocatorInfo, &vkallocator);
    return vkallocator;
}
