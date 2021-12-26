#pragma once

#include "VulkanPipeline.hpp"
#include "VulkanPlatform.hpp"
#include "VulkanSwapChain.hpp"
#define GLFW_INCLUDE_VULKAN
#include "VulkanContext.hpp"
#include <GLFW/glfw3.h>
#include <memory>
#include "VulkanDevice.hpp"

class Engine
{
private:
    int width, height;

    GLFWwindow *window;
    std::shared_ptr<VulkanContext> context;
    std::shared_ptr<VulkanDevice> device;
    std::shared_ptr<VulkanSwapchain> swapchain;
    std::shared_ptr<GraphicPipeline> graphicPipeline;
    std::shared_ptr<VulkanRenderPass> renderPass;


public:
    Engine(int width, int height);

    void init();
    void run();
    void cleanup();
};
