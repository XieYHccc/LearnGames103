#include "pch.h"
#define VMA_IMPLEMENTATION
#include "Graphic/Vulkan/Device_Vulkan.h"
#include "Core/Window.h"
#include "Math/Util.h"
#include "Events/EventManager.h"
#include "Util/Hash.h"
#include "Graphic/Vulkan/Shader_Vulkan.h"

namespace graphic {

void Device_Vulkan::CommandQueue::init(Device_Vulkan *device, QueueType type)
{
    this->device = device;
    this->type = type;

    switch (type) {
    case QUEUE_TYPE_GRAPHICS:
        queue = device->context->graphicQueue;
        break;
    case QUEUE_TYPE_ASYNC_COMPUTE:
        queue = device->context->computeQueue;
        break;
    case QUEUE_TYPE_ASYNC_TRANSFER:
        queue = device->context->transferQueue;
        break;
    default:
        CORE_DEBUG_ASSERT(0)
        break;
    }
}

void Device_Vulkan::CommandQueue::submit(VkFence fence)
{
    CORE_DEBUG_ASSERT(!submissions.empty() || fence != VK_NULL_HANDLE)

    std::vector<VkSubmitInfo2> submit_infos(submissions.size());
    for (size_t i = 0; i < submissions.size(); ++i) {
        auto& info = submit_infos[i];
        auto& submission = submissions[i];
        info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2;
        info.commandBufferInfoCount = submission.cmdInfos.size();
        info.pCommandBufferInfos = submission.cmdInfos.data();
        info.signalSemaphoreInfoCount = submission.signalSemaphoreInfos.size();
        info.pSignalSemaphoreInfos = submission.signalSemaphoreInfos.data();
        info.waitSemaphoreInfoCount = submission.waitSemaphoreInfos.size();
        info.pWaitSemaphoreInfos = submission.waitSemaphoreInfos.data();
    }

    device->context->extendFunction.pVkQueueSubmit2KHR(queue, submit_infos.size(), submit_infos.data(), fence);
    
    // Clear submissions
    for(auto& submission : submissions){
        submission.cmdInfos.clear();
        submission.signalSemaphoreInfos.clear();
        submission.waitSemaphoreInfos.clear();
    }

    submissions.clear();
}

void Device_Vulkan::PerFrameData::init(Device_Vulkan *device)
{
    CORE_DEBUG_ASSERT(device->vkDevice != VK_NULL_HANDLE)
    this->device = device;
    this->vmaAllocator = this->device->vmaAllocator;

    for (size_t i = 0; i < QUEUE_TYPE_MAX_ENUM; i++) {
        // Create a fence per queue
        VkFenceCreateInfo fence_create_info = {VK_STRUCTURE_TYPE_FENCE_CREATE_INFO};
        fence_create_info.flags = 0;
        fence_create_info.pNext = nullptr;
        vkCreateFence(device->vkDevice, &fence_create_info, nullptr, &queueFences[i]);
    }
    
    // Create semaphore
    VkSemaphoreCreateInfo semaphore_create_info = {VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO};
    semaphore_create_info.pNext = nullptr;
    vkCreateSemaphore(device->vkDevice, &semaphore_create_info, nullptr, &imageAvailableSemaphore);
    vkCreateSemaphore(device->vkDevice, &semaphore_create_info, nullptr, &imageReleaseSemaphore);
}

void Device_Vulkan::PerFrameData::clear()
{
    VkDevice vk_device = device->vkDevice;
    // Destroy deferred destroyed resources
    for (auto& sampler : garbageSamplers) {
        vkDestroySampler(vk_device, sampler, nullptr);
    }
    for (auto& view : grabageViews) {
        vkDestroyImageView(vk_device, view, nullptr);
    }
    for (auto& buffer : garbageBuffers) {
        vmaDestroyBuffer(vmaAllocator, buffer.first, buffer.second);
    }
    for (auto& image : garbageImages) {
        vmaDestroyImage(vmaAllocator, image.first, image.second);
    }
    for (auto& pipeline : garbagePipelines) {
        vkDestroyPipeline(vk_device, pipeline, nullptr);
    }
    for (auto& shaderModule_ : garbageShaderModules) {
        vkDestroyShaderModule(vk_device, shaderModule_, nullptr);
    }

    garbageSamplers.clear();
    garbageBuffers.clear();
    grabageViews.clear();
    garbageImages.clear();
    garbagePipelines.clear();
    garbageShaderModules.clear();
}

void Device_Vulkan::PerFrameData::reset()
{
    if (!waitedFences.empty()) {
        vkResetFences(device->vkDevice, waitedFences.size(), waitedFences.data());
        waitedFences.clear();
    }

    for (size_t i = 0; i < QUEUE_TYPE_MAX_ENUM; i++){
        cmdListCount[i] = 0;
    }

    imageAvailableSemaphoreConsumed = false;

    // Destroy deferred-destroyed resources
    clear();
}

void Device_Vulkan::PerFrameData::destroy()
{
    for (size_t i = 0; i < QUEUE_TYPE_MAX_ENUM; i++) {
        for (size_t j = 0; j < cmdLists[i].size(); j++) {
            delete cmdLists[i][j];
            cmdLists[i][j] = nullptr;
        }
        cmdLists[i].clear();

        vkDestroyFence(device->vkDevice, queueFences[i], nullptr);
    }

    vkDestroySemaphore(device->vkDevice, imageAvailableSemaphore, nullptr);
    vkDestroySemaphore(device->vkDevice, imageReleaseSemaphore, nullptr);

    clear();
}


void Device_Vulkan::CopyCmdAllocator::init(Device_Vulkan *device)
{
    device_ = device;
}

void Device_Vulkan::CopyCmdAllocator::destroy()
{   
    // Make sure all allocated cmd are in free list
    vkQueueWaitIdle(device_->queues[QUEUE_TYPE_ASYNC_TRANSFER].queue);
    for (auto& x : freeList_)
    {
        vkDestroyCommandPool(device_->vkDevice, x.cmdPool, nullptr);
        vkDestroyFence(device_->vkDevice, x.fence, nullptr);
    }

    freeList_.clear();
}

Device_Vulkan::CopyCmdAllocator::CopyCmd Device_Vulkan::CopyCmdAllocator::allocate(VkDeviceSize required_buffer_size)
{
    CopyCmd newCmd;

    // Try to find a suitable staging buffer in free list
    locker_.lock();
    for (size_t i = 0; i < freeList_.size(); ++i) {
        if (freeList_[i].stageBuffer->GetDesc().size > required_buffer_size) {
            newCmd = std::move(freeList_[i]);
            std::swap(freeList_[i], freeList_.back());
            freeList_.pop_back();
            break;
        }
    }
    locker_.unlock();

    if (!newCmd.isValid()) {    // No suitable staging buffer founded
        // Create command pool
        VkCommandPoolCreateInfo poolInfo = {};
        poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        poolInfo.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
        poolInfo.queueFamilyIndex = device_->context->transferQueueIndex;
        VK_CHECK(vkCreateCommandPool(device_->vkDevice, &poolInfo, nullptr, &newCmd.cmdPool))

        // Allocate command buffer
        VkCommandBufferAllocateInfo commandBufferInfo = {};
        commandBufferInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        commandBufferInfo.commandBufferCount = 1;
        commandBufferInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        commandBufferInfo.commandPool = newCmd.cmdPool;
        VK_CHECK(vkAllocateCommandBuffers(device_->vkDevice, &commandBufferInfo, &newCmd.cmdBuffer))

        // Create fence
        VkFenceCreateInfo fenceInfo = {};
        fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        VK_CHECK(vkCreateFence(device_->vkDevice, &fenceInfo, nullptr, &newCmd.fence))

        // Create staging buffer
        BufferDesc bufferDesc;
        bufferDesc.domain = BufferMemoryDomain::CPU;
        bufferDesc.size = math::GetNextPowerOfTwo(required_buffer_size);
        bufferDesc.size = std::max(bufferDesc.size, uint64_t(65536));
        bufferDesc.usageBits = BUFFER_USAGE_TRANSFER_FROM_BIT;
        newCmd.stageBuffer = device_->CreateBuffer(bufferDesc);
    }

    // Reset fence
    VK_CHECK(vkResetFences(device_->vkDevice, 1, &newCmd.fence))

    // Begin command buffer in valid state:
	VK_CHECK(vkResetCommandPool(device_->vkDevice, newCmd.cmdPool, 0))
    VkCommandBufferBeginInfo beginInfo = {};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    beginInfo.pInheritanceInfo = nullptr;
    VK_CHECK(vkBeginCommandBuffer(newCmd.cmdBuffer, &beginInfo))
    
    return newCmd;
}

void Device_Vulkan::CopyCmdAllocator::submit(CopyCmd cmd)
{
    VK_CHECK(vkEndCommandBuffer(cmd.cmdBuffer))

    VkCommandBufferSubmitInfo cmd_submit_info = {};
    cmd_submit_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO;
    cmd_submit_info.commandBuffer = cmd.cmdBuffer;

    VkSubmitInfo2 submit_info = {};
    submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2;
    submit_info.commandBufferInfoCount = 1;
    submit_info.pCommandBufferInfos = &cmd_submit_info;
    submit_info.waitSemaphoreInfoCount = 0;
    submit_info.pWaitSemaphoreInfos = nullptr;
    submit_info.signalSemaphoreInfoCount = 0;
    submit_info.pSignalSemaphoreInfos = nullptr;
    device_->context->extendFunction.pVkQueueSubmit2KHR(device_->context->transferQueue, 1, &submit_info, cmd.fence);

    // wait the fence and push cmd back to free list
    vkWaitForFences(device_->vkDevice, 1, &cmd.fence, VK_TRUE, 9999999999); //TODO: try to use semaphores and do not block CPU

    std::scoped_lock lock(locker_);
    freeList_.push_back(cmd);
}


void Device_Vulkan::OnWindowResize(const WindowResizeEvent &event)
{
    frameBufferWidth = event.width;
    frameBufferHeight = event.height;
    recreateSwapchain = true;
    CORE_LOGD("Device_Vulkan hook window resize event. Width: {} Height: {}", frameBufferWidth, frameBufferHeight)
}

bool Device_Vulkan::Init()
{
    CORE_LOGI("==========Initializing Vulkan Backend...========")

    // Default values
    recreateSwapchain = false;
    currentFrame = 0;
    frameBufferWidth = Window::Instance()->GetFrambufferWidth();
    frameBufferHeight = Window::Instance()->GetFrambufferHeight();
    context = CreateScope<VulkanContext>(); // TODO: Make configurable
    vkDevice = context->logicalDevice; // Borrow from context
    vmaAllocator = context->vmaAllocator;

    // Store device properties in public interface
    properties.limits.minUniformBufferOffsetAlignment = context->properties2.properties.limits.minUniformBufferOffsetAlignment;

    // Create frame data
    for (size_t i = 0; i < MAX_FRAME_NUM_IN_FLIGHT; i++) {
        frames[i].init(this);
    }

    // Setup command queues
    queues[QUEUE_TYPE_GRAPHICS].init(this, QUEUE_TYPE_GRAPHICS);
    queues[QUEUE_TYPE_ASYNC_COMPUTE].init(this, QUEUE_TYPE_ASYNC_COMPUTE);
    queues[QUEUE_TYPE_ASYNC_TRANSFER].init(this, QUEUE_TYPE_ASYNC_TRANSFER);

    // Create Swapchain
    ResizeSwapchain();

    // Init copy cmds allocator
    copyAllocator.init(this);

    // Register callback functions
    EventManager::Instance().Subscribe<WindowResizeEvent>([this](const WindowResizeEvent& event) { OnWindowResize(event);});
    CORE_LOGI("==========Vulkan Backend Initialized========")
    return true;
}

void Device_Vulkan::ShutDown()
{
    CORE_LOGI("Shutdown vulkan device...")
    vkDeviceWaitIdle(vkDevice);
    
    // Destroy cached pipeline layout
    cached_pipelineLayouts.clear();

    // Destory cached descriptor allocator
    cached_descriptorSetAllocator.clear();

    // Destroy copy allocator
    copyAllocator.destroy();

    // Destroy frames data
    for (size_t i = 0; i < MAX_FRAME_NUM_IN_FLIGHT; i++) {
        frames[i].destroy();
    }

    // Destroy vulkan context
    context.reset();

}

bool Device_Vulkan::BeiginFrame(f32 deltaTime)
{
    // Move to next frame
    currentFrame = (currentFrame + 1) % MAX_FRAME_NUM_IN_FLIGHT;

    // Resize the swapchain if needed. 
    if (recreateSwapchain) {
        ResizeSwapchain();
        recreateSwapchain = false;
    }

    auto& frame = frames[currentFrame];
    
    // Wait for in-flight fences
    if (!frame.waitedFences.empty()) {
        vkWaitForFences(vkDevice, frame.waitedFences.size(), frame.waitedFences.data(), true, 1000000000);
    }
    
    // Reset per frame data
    frame.reset();

    // Put unused (more than 8 frames) descriptor set back to vacant pool
    for (auto& [k, value] : cached_descriptorSetAllocator) {
        value.begin_frame();   
    }

    // Acquire a swapchain image 
    VkResult result = vkAcquireNextImageKHR(
        vkDevice,
        context->swapChain,
        100000000,
        frame.imageAvailableSemaphore,
        nullptr,
        &currentSwapChainImageIdx);

    if (result == VK_ERROR_OUT_OF_DATE_KHR) {
        // Trigger swapchain recreation, then boot out of the render loop.
        ResizeSwapchain();
        return false;
    } else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
        CORE_ASSERT_MSG(0, "Failed to acquire swapchain image!");
        return false;
    }

    return true;
}

bool Device_Vulkan::EndFrame(f32 deltaTime)
{
    auto& frame = GetCurrentFrame();

    // Submit queued command lists with fence which would block the next next frame
    for (size_t i = 0; i < QUEUE_TYPE_MAX_ENUM; ++i) {
        if (frame.cmdListCount[i] > 0) { // This queue is in use in this frame
            queues[i].submit(frame.queueFences[i]);
            frame.waitedFences.push_back(frame.queueFences[i]);
        }
    }

    // Prepare present
    VkSwapchainKHR swapchain = context->swapChain;
	VkPresentInfoKHR presentInfo = {};
	presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	presentInfo.pNext = nullptr;
	presentInfo.pSwapchains = &swapchain;
	presentInfo.swapchainCount = 1;
	presentInfo.pWaitSemaphores = &frame.imageReleaseSemaphore;
	presentInfo.waitSemaphoreCount = 1;
	presentInfo.pImageIndices = &currentSwapChainImageIdx;
	VkResult presentResult = vkQueuePresentKHR(context->graphicQueue, &presentInfo);

    if (presentResult != VK_SUCCESS && presentResult != VK_ERROR_OUT_OF_DATE_KHR
		&& presentResult != VK_SUBOPTIMAL_KHR) {
        CORE_DEBUG_ASSERT(0)
    }
    return true;
}

Ref<Buffer> Device_Vulkan::CreateBuffer(const BufferDesc &desc, const void* initialData)
{
    return CreateRef<Buffer_Vulkan>(this, desc, initialData);
}

Ref<Image> Device_Vulkan::CreateImage(const ImageDesc &desc, const ImageInitData* init_data)
{
    
    return CreateRef<Image_Vulkan>(this, desc, init_data);
}

Ref<Shader> Device_Vulkan::CreateShaderFromBytes(ShaderStage stage, const void* byteCode, size_t codeSize)
{
    auto new_shader =  CreateRef<Shader_Vulkan>(this, stage, byteCode, codeSize);
    if (new_shader->shaderModule_ == VK_NULL_HANDLE)
        return nullptr;

    return new_shader;
}

Ref<Shader> Device_Vulkan::CreateShaderFromSpvFile(ShaderStage stage, const std::string& file_path)
{
    // open the file. With cursor at the end
    std::ifstream file(file_path, std::ios::ate | std::ios::binary);

    if (!file.is_open()) {
        CORE_LOGE("Failed to open shader file: {}", file_path)
        return nullptr;
    }
    size_t fileSize = (size_t)file.tellg();
    std::vector<uint8_t> buffer(fileSize);

    file.seekg(0);
    file.read((char*)buffer.data(), fileSize);
    file.close();

    auto new_shader = CreateShaderFromBytes(stage, buffer.data(), buffer.size());
    if (new_shader == nullptr) {
        CORE_LOGE("Faile to create shader from file: {}", file_path)
        return nullptr;
    }

    return new_shader;
}

Ref<PipeLine> Device_Vulkan::CreateGraphicPipeLine(const GraphicPipeLineDesc &desc, const RenderPassInfo& info)
{
    return CreateRef<PipeLine_Vulkan>(this, desc, info);
}

Ref<Sampler> Device_Vulkan::CreateSampler(const SamplerDesc &desc)
{
    return CreateRef<Sampler_Vulkan>(this, desc);
}

CommandList* Device_Vulkan::BeginCommandList(QueueType type)
{
    auto& frame = GetCurrentFrame();
    auto& cmdLists = frame.cmdLists[type];
    u32 cmd_count = frame.cmdListCount[type]++;
    if (cmd_count >= cmdLists.size()) {
        // Create a new Command list is needed
        cmdLists.emplace_back(new CommandList_Vulkan(this, type));
    }

    CommandList_Vulkan* internal_cmdList = cmdLists[cmd_count];
    internal_cmdList->ResetAndBeginCmdBuffer();
    
    return static_cast<CommandList*>(internal_cmdList);
}

void Device_Vulkan::SubmitCommandList(CommandList* cmd, CommandList* waitedCmds, uint32_t waitedCmdCounts, bool signal)
{
    auto& internal_cmdList = ToInternal(cmd);
    auto& queue = queues[internal_cmdList.type_];

    vkEndCommandBuffer(internal_cmdList.cmdBuffer_);
    internal_cmdList.state_ = CommandListState::READY_FOR_SUBMIT;
    
    if (queue.submissions.empty()) {
        queue.submissions.emplace_back();
    }

    if (!queue.submissions.back().signalSemaphoreInfos.empty() || !queue.submissions.back().waitSemaphoreInfos.empty()) {
        // Need to create a new batch
        queue.submissions.emplace_back();
    }

    auto& submission = queue.submissions.back();
    VkCommandBufferSubmitInfo& cmd_submit_info = submission.cmdInfos.emplace_back();
    cmd_submit_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO;
    cmd_submit_info.commandBuffer = internal_cmdList.cmdBuffer_;
    cmd_submit_info.pNext = nullptr;

    if (waitedCmdCounts > 0) {
        for (size_t i = 0; i < waitedCmdCounts; ++i) {
            auto& internal = ToInternal(&waitedCmds[i]);
            auto& semaphore_info = submission.waitSemaphoreInfos.emplace_back();
            semaphore_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO;
            semaphore_info.semaphore = internal.cmdCompleteSemaphore_;
            semaphore_info.value = 0; // not a timeline semaphore
            semaphore_info.stageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT; // Perfomance trade off here for abstracting away semaphore

        }
    }

    auto& frame = frames[currentFrame];
    if (internal_cmdList.waitForSwapchainImage_ && !frame.imageAvailableSemaphoreConsumed) {
        auto& wait_semaphore_info = submission.waitSemaphoreInfos.emplace_back();
        wait_semaphore_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO;
        wait_semaphore_info.semaphore = frame.imageAvailableSemaphore;
        wait_semaphore_info.value = 0;
        wait_semaphore_info.stageMask = internal_cmdList.swapChainWaitStages_;

        // TODO: This indicates there is only one command buffer in a frame can manipulate swapchain images. Not flexible enough
        auto& submit_semaphore_info = submission.signalSemaphoreInfos.emplace_back();
        submit_semaphore_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO;
        submit_semaphore_info.semaphore = frame.imageReleaseSemaphore;

        frame.imageAvailableSemaphoreConsumed = true;
    }

    // Submit right now to make sure the correct order of submissions
    if (signal) {
        auto& semaphore_info = submission.signalSemaphoreInfos.emplace_back();
        semaphore_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO;
        semaphore_info.semaphore = internal_cmdList.cmdCompleteSemaphore_;
        semaphore_info.value = 0; // not a timeline semaphore
        semaphore_info.stageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;

        queue.submit();
    }
}

void Device_Vulkan::ResizeSwapchain()
{        
    CORE_LOGI("Resizing swapchain...")
    
    context->DestroySwapChain();
    context->CreateSwapChain();

    swapChainImages.clear();
    for (size_t i = 0; i < context->swapChianImages.size(); i++) {
        ImageDesc desc;
        desc.type = ImageType::TYPE_2D;
        desc.height = context->swapChainExtent.height;
        desc.width = context->swapChainExtent.width;
        desc.depth = 1;
        desc.format = GetSwapChainImageFormat();

        Ref<Image> newImage = CreateRef<Image_Vulkan>(desc);
        auto& internal_image = ToInternal(newImage.get());
        internal_image.device_ = this;
        internal_image.handle_ = context->swapChianImages[i];
        internal_image.view_ = context->swapChainImageViews[i];
        internal_image.isSwapChainImage_ = true;
        swapChainImages.push_back(newImage);
        
    }

}

DataFormat Device_Vulkan::GetSwapChainImageFormat()
{
    VkFormat format = context->surfaceFormat.format;

    switch (format) {
    case VK_FORMAT_R8G8B8A8_UNORM:
        return DataFormat::R8G8B8A8_UNORM;
    case VK_FORMAT_R16G16B16A16_SFLOAT:
        return DataFormat::R16G16B16A16_SFLOAT;
    case VK_FORMAT_B8G8R8A8_UNORM:
        return DataFormat::B8G8R8A8_UNORM;
    default:
        CORE_ASSERT_MSG(0, "format not handled yet")
    }
}

PipeLineLayout* Device_Vulkan::Request_PipeLineLayout(const std::array<DescriptorSetLayout, DESCRIPTOR_SET_MAX_NUM>& layouts, VkPushConstantRange push_constant, u32 layout_mask)
{
    // Compute pipeline layout hash
    size_t hash = 0;
    for (size_t i = 0; i < DESCRIPTOR_SET_MAX_NUM; ++i) {
        for (size_t j = 0; j < SET_BINDINGS_MAX_NUM; ++j) {
            auto& binding = layouts[i].vk_bindings[j];
            util::hash_combine(hash, binding.binding);
            util::hash_combine(hash, binding.descriptorCount);
            util::hash_combine(hash, binding.descriptorType);
            util::hash_combine(hash, binding.stageFlags);
            // util::hash_combine(hash, pipeline_layout.imageViewTypes[i][j]);
        }
    }
    util::hash_combine(hash, push_constant.offset);
    util::hash_combine(hash, push_constant.size);
    util::hash_combine(hash, push_constant.stageFlags);
    util::hash_combine(hash, layout_mask);

    auto find = cached_pipelineLayouts.find(hash);
    if (find == cached_pipelineLayouts.end()) { 
        // need to create a new pipeline layout
        auto result = cached_pipelineLayouts.try_emplace(hash, this, layouts, push_constant, layout_mask);
        return &(result.first->second);
    }
    else { 
        return &find->second;
    }
}

DescriptorSetAllocator* Device_Vulkan::Request_DescriptorSetAllocator(const DescriptorSetLayout& layout)
{
    size_t hash = 0;
    util::hash_combine(hash, layout.uniform_buffer_mask);
    util::hash_combine(hash, layout.storage_buffer_mask);
    util::hash_combine(hash, layout.sampled_image_mask);
    util::hash_combine(hash, layout.storage_image_mask);
    util::hash_combine(hash, layout.separate_image_mask);
    util::hash_combine(hash, layout.sampler_mask);
    util::hash_combine(hash, layout.input_attachment_mask);

    auto find = cached_descriptorSetAllocator.find(hash);
    if (find == cached_descriptorSetAllocator.end()) {
        // need to create a new descriptor set allocator
        auto ret = cached_descriptorSetAllocator.try_emplace(hash, this, layout);
        return &(ret.first->second);
    }
    else {
        return &find->second;
    }

}
}
