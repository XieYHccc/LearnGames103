#pragma once
#include "Quark/Graphic/Vulkan/Common_Vulkan.h"
#include "Quark/Graphic/Shader.h"

namespace quark::graphic {
class Shader_Vulkan : public Shader {
public:
    Shader_Vulkan(Device_Vulkan* device, ShaderStage stage, const void* shaderCode, size_t codeSize);
    ~Shader_Vulkan();

    const VkShaderModule GetShaderMoudule() const { return shaderModule_; }
    const VkPipelineShaderStageCreateInfo& GetStageInfo() const { return stageInfo_; }
    const std::vector<VkDescriptorSetLayoutBinding>& GetBindings(u32 set) const { return bindings_[set]; }
    const VkPushConstantRange& GetPushConstant() const { return pushConstant_; }
    
private:
    Device_Vulkan* device_;
    VkShaderModule shaderModule_;
    VkPipelineShaderStageCreateInfo stageInfo_;

    // Layout info
    std::vector<VkDescriptorSetLayoutBinding> bindings_[DESCRIPTOR_SET_MAX_NUM];
    VkPushConstantRange pushConstant_;
};

CONVERT_TO_VULKAN_INTERNAL(Shader)
}