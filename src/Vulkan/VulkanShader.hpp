#pragma once

#include "UniformBuffer.hpp"
#include "VulkanContext.hpp"
#include "VulkanDevice.hpp"
#include <memory>
#include <optional>
#include <stdexcept>
#include <unordered_map>
#include <vector>
#include <vulkan/vulkan_core.h>

enum ShaderStage
{
    VertexStage,
    FragmentStage,
};

VkShaderStageFlagBits StageToVulkanStage(ShaderStage stage);


class VulkanShaderException : std::runtime_error
{
public:
    VulkanShaderException(const char *msg) : runtime_error(msg) {}
};


std::vector<char> readFile(const std::string &filename);


VkShaderModule createShaderModule(const std::vector<char> &code, VkDevice device, VkAllocationCallbacks *alloc);

class VulkanShader
{
private:
    class Uniform
    {
        friend class VulkanShader;

    private:
        uint32_t size;

    public:
        Uniform(uint32_t size = 0) : size(size) {}
    };


    std::unordered_map<ShaderStage, std::string> shaderFiles;
    std::unordered_map<ShaderStage, VkShaderModule> shaderModules;
    std::shared_ptr<VulkanDevice> device;

    std::unordered_map<ShaderStage, std::vector<Uniform>> uniforms;

public:
    VulkanShader(const std::unordered_map<ShaderStage, std::string> shaderFiles, std::shared_ptr<VulkanDevice> device);


    VkShaderModule getShaderModule(ShaderStage stage);
    std::unordered_map<ShaderStage, VkShaderModule> &getShaders();

    std::vector<VkDescriptorSetLayoutBinding> getDescriptorBindings();

    void addUniform(ShaderStage stage, uint32_t size);


    virtual ~VulkanShader();
};
