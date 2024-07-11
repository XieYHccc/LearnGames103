#pragma once
#include "Common_Vulkan.h"
#include "Graphic/Device.h"
#include "Graphic/Vulkan/Buffer_Vulkan.h"
#include "Graphic/Vulkan/Image_Vulkan.h"
#include "Graphic/Vulkan/Context_Vulkan.h"
#include "Graphic/Vulkan/CommandList_Vulkan.h"
#include "Graphic/Vulkan/PipeLine_Vulkan.h"

namespace graphic {
class Device_Vulkan final: public Device{
    friend class Sampler_Vulkan;
    friend class Image_Vulkan;
    friend class Buffer_Vulkan;
    friend class PipeLine_Vulkan;
    friend class Shader_Vulkan;
    friend struct PipeLineLayout;
private:
    struct CommandQueue {     // Responsible for queuing commad buffers and submit them in batch
        Device_Vulkan* device = nullptr;
        QueueType type = QueueType::QUEUE_TYPE_MAX_ENUM;
        VkQueue queue = VK_NULL_HANDLE;

        // represent a VkSubmitInfo
        struct Submitssion {
            std::vector<VkCommandBufferSubmitInfo> cmdInfos;
            std::vector<VkSemaphoreSubmitInfo> waitSemaphoreInfos;
            std::vector<VkSemaphoreSubmitInfo> signalSemaphoreInfos;
        };
        std::vector<Submitssion> submissions;
        void init(Device_Vulkan* device, QueueType type);
        void submit(VkFence fence = nullptr);
    };

    struct PerFrameData {
        Device_Vulkan* device = nullptr;
        VmaAllocator vmaAllocator = nullptr;
        std::vector<CommandList_Vulkan*> cmdLists[QUEUE_TYPE_MAX_ENUM];
        u32 cmdListCount[QUEUE_TYPE_MAX_ENUM] = {}; //  The count of cmd used in this frame. Cleared when a new frame begin
        VkFence queueFences[QUEUE_TYPE_MAX_ENUM];   // Per queue fence. Signled when all command list submitted from this frame completed.
        std::vector<VkFence> waitedFences;
        
        VkSemaphore imageAvailableSemaphore;
        VkSemaphore imageReleaseSemaphore;
        bool imageAvailableSemaphoreConsumed = false;

        std::vector<std::pair<VkBuffer, VmaAllocation>> garbageBuffers;
        std::vector<std::pair<VkImage, VmaAllocation>> garbageImages;
        std::vector<VkPipeline> garbagePipelines;
        std::vector<VkImageView> grabageViews;
        std::vector<VkShaderModule> garbageShaderModules;
        std::vector<VkSampler> garbageSamplers;

        void init(Device_Vulkan* device);
        void reset();   // Reset this frame
        void destroy();
        void clear();   // Deferred destroy of resources that gpu is already finished with
    };

    struct TransferCmd {    // only used once for static data transfer
        Device_Vulkan* device;
        VkCommandPool cmdPool;
        VkCommandBuffer cmdBuffer;
        // VkFence fence;

        void init(Device_Vulkan* device);
        void destroy();
        VkCommandBuffer begin_immediate_submit();
        void block_submit();
    };

public:
    Device_Vulkan() = default;
    ~Device_Vulkan() = default;
    
    bool Init() override final;
    void ShutDown() override final;
    bool BeiginFrame(f32 deltaTime) override final;
    bool EndFrame(f32 deltaTime) override final;
    void OnWindowResize(const WindowResizeEvent& event) override final;

	/*** RESOURCES ***/ 
    Ref<Buffer> CreateBuffer(const BufferDesc& desc, const void* initialData = nullptr) override final;
    Ref<Image> CreateImage(const ImageDesc& desc, const ImageInitData* init_data = nullptr) override final;
    Ref<Shader> CreateShaderFromBytes(ShaderStage stage, const void* byteCode, size_t codeSize) override final;
    Ref<Shader> CreateShaderFromSpvFile(ShaderStage stage, const std::string& file_path) override final;
    Ref<PipeLine> CreateGraphicPipeLine(const GraphicPipeLineDesc& desc) override final;
    Ref<Sampler> CreateSampler(const SamplerDesc& desc) override final;

	/*** COMMANDS ***/
    CommandList* BeginCommandList(QueueType type = QueueType::QUEUE_TYPE_GRAPHICS) override final;
    void SubmitCommandList(CommandList* cmd, CommandList* waitedCmds = nullptr, uint32_t waitedCmdCounts = 0, bool signal = false) override final;

	/*** SWAPCHAIN ***/
    Ref<Image> GetSwapChainImage() override final { return swapChainImages[currentSwapChainImageIdx]; }
    DataFormat GetSwapChainImageFormat() override final;
private:
    PerFrameData& GetCurrentFrame() { return frames[currentFrame]; }
    DescriptorSetAllocator* Request_DescriptorSetAllocator(const DescriptorSetLayout& layout);
    PipeLineLayout* Request_PipeLineLayout(const std::array<DescriptorSetLayout, DESCRIPTOR_SET_MAX_NUM>& layouts, VkPushConstantRange push_constant, u32 set_mask);
    void ResizeSwapchain();

public:
    static constexpr u8 MAX_FRAME_NUM_IN_FLIGHT = 2;
    VkDevice vkDevice; // Borrowed from context, no lifetime management here
    VmaAllocator vmaAllocator; // Borrowed from context, no lifetime management here
    Scope<VulkanContext> context;
    std::vector<Ref<Image>> swapChainImages; // internal handle owned by swapchain, no lifetime management here
    u32 currentSwapChainImageIdx;
    PerFrameData frames[MAX_FRAME_NUM_IN_FLIGHT];
    CommandQueue queues[QUEUE_TYPE_MAX_ENUM];
    TransferCmd transferCmd;
    bool recreateSwapchain;

    // Cached object
    std::unordered_map<size_t, PipeLineLayout> cached_pipelineLayouts;
    std::unordered_map<size_t, DescriptorSetAllocator> cached_descriptorSetAllocator;
};
}