    #pragma once
#include "Quark/Core/Util/TemporaryHashMap.h"
#include "Quark/Graphic/PipeLine.h"
#include "Quark/Graphic/RenderPassInfo.h"
#include "Quark/Graphic/Vulkan/Common_Vulkan.h"
#include "Quark/Graphic/Vulkan/Shader_Vulkan.h"
#include "Quark/Graphic/Vulkan/DescriptorSetAllocator.h"

namespace quark::graphic {

// Once we have a list of VkDescriptorSetLayouts and push constant layouts, we now have our PipelineLayout.
// This is of course, hashed as well based on the hash of descriptor set layouts and push constant ranges.
struct PipeLineLayout {
    Device_Vulkan* device;
    VkPipelineLayout handle = VK_NULL_HANDLE; 

    DescriptorSetAllocator* setAllocators[DESCRIPTOR_SET_MAX_NUM] = {};
    VkDescriptorUpdateTemplate updateTemplate[DESCRIPTOR_SET_MAX_NUM] = {};
    ShaderResourceLayout combinedLayout = {};

    // shaders(vert shader, frag shader...) => combined resource layout => pipeline layout
    PipeLineLayout(Device_Vulkan* device, const ShaderResourceLayout combinedLayout);
    ~PipeLineLayout();
};

class PipeLine_Vulkan : public PipeLine {
public:
    PipeLine_Vulkan(Device_Vulkan* device, const GraphicPipeLineDesc& desc);
    PipeLine_Vulkan(Device_Vulkan* device, Ref<Shader> computeShader);
    ~PipeLine_Vulkan();

    VkPipeline GetHandle() const { return m_Handle; }
    const PipeLineLayout* GetLayout() const { return m_Layout; }
    const RenderPassInfo2& GetCompatableRenderPassInfo() const { return m_CompatableRenderPassInfo; }
    const VkGraphicsPipelineCreateInfo GetGraphicsPipelineCreateInfo() const { return m_CreateInfo; }

private:
    Device_Vulkan* m_Device;
    VkPipeline m_Handle = VK_NULL_HANDLE;
    PipeLineLayout* m_Layout = nullptr; // no lifetime management here
    RenderPassInfo2 m_CompatableRenderPassInfo = {};
    VkGraphicsPipelineCreateInfo m_CreateInfo = {};
};

CONVERT_TO_VULKAN_INTERNAL_FUNC(PipeLine)
}