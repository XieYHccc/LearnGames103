#include "Editor/EditorApp.h"

#include <glm/gtx/quaternion.hpp>
#include <imgui.h>
#include <Quark/Core/Window.h>
#include <Quark/Core/FileSystem.h>
#include <Quark/Core/Input.h>
#include <Quark/Scene/Components/TransformCmpt.h>
#include <Quark/Scene/Components/CameraCmpt.h>
#include <Quark/Scene/SceneSerializer.h>
#include <Quark/Asset/ImageLoader.h>
#include <Quark/Asset/AssetManager.h>
#include <Quark/UI/UI.h>

namespace quark {
Application* CreateApplication()
{    
    AppInitSpecs specs;
    specs.uiSpecs.flags = UI_INIT_FLAG_DOCKING | UI_INIT_FLAG_VIEWPORTS;
    specs.title = "Quark Editor";
    specs.width = 1600;
    specs.height = 1000;
    specs.isFullScreen = false;

    return new EditorApp(specs);
}

EditorApp::EditorApp(const AppInitSpecs& specs)
    : Application(specs), m_EditorCamera(60, 1.5, 0.1, 256)
{
    color_format = m_GraphicDevice->GetSwapChainImageFormat();

    // Create Render structures
    SetUpRenderPass();
    CreatePipeline();
    CreateColorDepthAttachments();

    LoadScene();

    // Init UI Panels
    m_HeirarchyPanel.SetScene(m_Scene.get());
    m_InspectorPanel.SetScene(m_Scene.get());

    // SetUp Renderer
    m_SceneRenderer = CreateScope<SceneRenderer>(m_GraphicDevice.get());
    m_SceneRenderer->SetScene(m_Scene.get());
    m_SceneRenderer->SetCubeMap(cubeMap_image);

    // Adjust editor camera's aspect ratio
    float aspect = (float)Window::Instance()->GetWidth() / Window::Instance()->GetHeight();
    m_EditorCamera.aspectRatio = aspect;
}

EditorApp::~EditorApp()
{   
    // Save asset registry
    AssetManager::Get().SaveAssetRegistry();
}

void EditorApp::Update(f32 deltaTime)
{    
    // Update Editor camera's movement
    m_EditorCamera.OnUpdate(deltaTime / 1000);

    // TODO: Update physics

    // Update scene
    m_Scene->OnUpdate();

    // Update UI
    UpdateUI();
}   

void EditorApp::UpdateUI()
{
    UI::Get()->BeginFrame();

    // Update Main menu bar
    UpdateMainMenuUI();

    // Debug Ui
    if (ImGui::Begin("Debug")) 
    {
        ImGui::Text("FPS: %f", m_Status.fps);
        ImGui::Text("Frame Time: %f ms", m_Status.lastFrameDuration);
        ImGui::Text("CmdList Record Time: %f ms", cmdListRecordTime);
    }
    ImGui::End();

    // Update Scene Heirarchy
    m_HeirarchyPanel.Render();
    
    // Update Inspector
    m_InspectorPanel.SetSelectedEntity(m_HeirarchyPanel.GetSelectedEntity());
    m_InspectorPanel.Render();

    // Update Scene view port
    m_SceneViewPort.SetColorAttachment(color_image.get());
    m_SceneViewPort.Render();


    UI::Get()->EndFrame();
}

void EditorApp::NewScene()
{
    m_Scene = CreateScope<Scene>("New Scene");

    m_SceneRenderer->SetScene(m_Scene.get());
    m_HeirarchyPanel.SetScene(m_Scene.get());
    m_InspectorPanel.SetScene(m_Scene.get());
}

void EditorApp::OpenScene()
{
    std::filesystem::path filepath = FileSystem::OpenFileDialog({ { "Quark Scene", "qkscene" } });
    if (!filepath.empty())
    {
        m_Scene = CreateScope<Scene>("");
        SceneSerializer serializer(m_Scene.get());
        serializer.Deserialize(filepath.string());

        m_SceneRenderer->SetScene(m_Scene.get());
        m_HeirarchyPanel.SetScene(m_Scene.get());
        m_InspectorPanel.SetScene(m_Scene.get());

    }

}

void EditorApp::SaveSceneAs()
{
    std::filesystem::path filepath = FileSystem::SaveFileDialog({ { "Quark Scene", "qkscene" } });
    if (!filepath.empty())
    {
        SceneSerializer serializer(m_Scene.get());
        serializer.Serialize(filepath.string());
    }
}

void EditorApp::Render(f32 deltaTime)
{
    // Sync the rendering data with game scene
    CameraUniformBufferBlock cameraData;
    cameraData.proj = m_EditorCamera.GetProjectionMatrix();
    cameraData.proj[1][1] *= -1;
    cameraData.view = m_EditorCamera.GetViewMatrix();
    cameraData.viewproj = cameraData.proj * cameraData.view;

    m_SceneRenderer->UpdateDrawContext(cameraData);

    // Rendering commands
    auto graphic_device = Application::Get().GetGraphicDevice();
    if (graphic_device->BeiginFrame(deltaTime)) {
        auto cmd = graphic_device->BeginCommandList();
        auto* swap_chain_image = graphic_device->GetPresentImage();

        // Geometry pass
        {
            graphic::PipelineImageBarrier image_barrier;
            image_barrier.image = color_image.get();
            image_barrier.srcStageBits = graphic::PIPELINE_STAGE_ALL_GRAPHICS_BIT;
            image_barrier.srcMemoryAccessBits = 0;
            image_barrier.dstStageBits = graphic::PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
            image_barrier.dstMemoryAccessBits = graphic::BARRIER_ACCESS_COLOR_ATTACHMENT_READ_BIT | graphic::BARRIER_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
            image_barrier.layoutBefore = graphic::ImageLayout::UNDEFINED;
            image_barrier.layoutAfter = graphic::ImageLayout::COLOR_ATTACHMENT_OPTIMAL;
            cmd->PipeLineBarriers(nullptr, 0, &image_barrier, 1, nullptr, 0);

            // Viewport and scissor
            graphic::Viewport viewport;
            viewport.x = 0;
            viewport.y = 0;
            viewport.width = (float)color_image->GetDesc().width;
            viewport.height = (float)color_image->GetDesc().height;
            viewport.minDepth = 0;
            viewport.maxDepth = 1;

            graphic::Scissor scissor;
            scissor.extent.width = viewport.width;
            scissor.extent.height = viewport.height;
            scissor.offset.x = 0;
            scissor.offset.y = 0;

            // Begin pass
            forward_pass_info.colorAttachments[0] = color_image.get();
            forward_pass_info.clearColors[0] = {0.5f, 0.5f, 0.5f, 1.f};
            forward_pass_info.depthAttachment = depth_image.get();
            cmd->BeginRenderPass(forward_pass_info);

            // Draw skybox
            cmd->BindPipeLine(*skybox_pipeline);
            cmd->SetViewPort(viewport);
            cmd->SetScissor(scissor);
            m_SceneRenderer->RenderSkybox(cmd);

            // Draw scene
            cmd->BindPipeLine(*graphic_pipeline);
            cmd->SetViewPort(viewport);
            cmd->SetScissor(scissor);
            auto geometry_start = m_Timer.ElapsedMillis();
            m_SceneRenderer->RenderScene(cmd);
            cmdListRecordTime = m_Timer.ElapsedMillis() - geometry_start;

            cmd->EndRenderPass();
        }

        // UI pass
        {
            graphic::PipelineImageBarrier swapchain_image_barrier;
            swapchain_image_barrier.image = swap_chain_image;
            swapchain_image_barrier.srcStageBits = graphic::PIPELINE_STAGE_TOP_OF_PIPE_BIT;
            swapchain_image_barrier.srcMemoryAccessBits = 0;
            swapchain_image_barrier.dstStageBits = graphic::PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
            swapchain_image_barrier.dstMemoryAccessBits = graphic::BARRIER_ACCESS_COLOR_ATTACHMENT_READ_BIT | graphic::BARRIER_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
            swapchain_image_barrier.layoutBefore = graphic::ImageLayout::UNDEFINED;
            swapchain_image_barrier.layoutAfter = graphic::ImageLayout::COLOR_ATTACHMENT_OPTIMAL;
            cmd->PipeLineBarriers(nullptr, 0, &swapchain_image_barrier, 1, nullptr, 0);

            graphic::PipelineImageBarrier color_image_barrier;
            color_image_barrier.image = color_image.get();
            color_image_barrier.srcStageBits = graphic::PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
            color_image_barrier.srcMemoryAccessBits = graphic::BARRIER_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
            color_image_barrier.dstStageBits = graphic::PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
            color_image_barrier.dstMemoryAccessBits = graphic::BARRIER_ACCESS_SHADER_READ_BIT;
            color_image_barrier.layoutBefore = graphic::ImageLayout::COLOR_ATTACHMENT_OPTIMAL;
            color_image_barrier.layoutAfter = graphic::ImageLayout::SHADER_READ_ONLY_OPTIMAL;
            cmd->PipeLineBarriers(nullptr, 0, &color_image_barrier, 1, nullptr, 0);

            ui_pass_info.colorAttachments[0] = swap_chain_image;
            cmd->BeginRenderPass(ui_pass_info);
            UI::Get()->Render(cmd);
            cmd->EndRenderPass();
        }


        // Transit swapchain image to present layout for presenting
        {
            graphic::PipelineImageBarrier present_barrier;
            present_barrier.image = swap_chain_image;
            present_barrier.srcStageBits = graphic::PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
            present_barrier.srcMemoryAccessBits = graphic::BARRIER_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
            present_barrier.dstStageBits = graphic::PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
            present_barrier.dstMemoryAccessBits = 0;
            present_barrier.layoutBefore = graphic::ImageLayout::COLOR_ATTACHMENT_OPTIMAL;
            present_barrier.layoutAfter = graphic::ImageLayout::PRESENT;
            cmd->PipeLineBarriers(nullptr, 0, &present_barrier, 1, nullptr, 0);
        }

        // Submit command list
        graphic_device->SubmitCommandList(cmd);
        graphic_device->EndFrame(deltaTime);
    }
}

void EditorApp::LoadScene()
{   
    // Load cube map
    ImageLoader image_loader(m_GraphicDevice.get());
    cubeMap_image = image_loader.LoadKtx2("Assets/Textures/etc1s_cubemap_learnopengl.ktx2");

    // Load scene
    m_Scene = CreateScope<Scene>("");

}

void EditorApp::UpdateMainMenuUI()
{
    if (ImGui::BeginMainMenuBar())
    {
        if (ImGui::BeginMenu("File"))
        {
            if (ImGui::MenuItem("New Scene", "Ctrl+N"))
                NewScene();

            if (ImGui::MenuItem("Open Scene...", "Ctrl+O"))
                OpenScene();

            if (ImGui::MenuItem("Save Scene As...", "Ctrl+Shift+S"))
                SaveSceneAs();

            //if (ImGui::MenuItem("Exit"))

            ImGui::EndMenu();
        }

        ImGui::EndMainMenuBar();
    }
}

void EditorApp::OnKeyPressed(KeyPressedEvent& e)
{
    if (e.repeatCount > 0)
        return;

    bool control = Input::Get()->IsKeyKeepPressed(Key::LeftControl) || Input::Get()->IsKeyKeepPressed(Key::RightControl);
}

void EditorApp::CreatePipeline()
{
    using namespace quark::graphic;

    // Sky box shaders
    skybox_vert_shader = m_GraphicDevice->CreateShaderFromSpvFile(graphic::ShaderStage::STAGE_VERTEX, "Assets/Shaders/Spirv/skybox.vert.spv");
    skybox_frag_shader = m_GraphicDevice->CreateShaderFromSpvFile(graphic::ShaderStage::STAGE_FRAGEMNT, "Assets/Shaders/Spirv/skybox.frag.spv");

    // Scene shaders
    vert_shader = m_GraphicDevice->CreateShaderFromSpvFile(ShaderStage::STAGE_VERTEX,
        "Assets/Shaders/Spirv/pbr.vert.spv");
    frag_shader = m_GraphicDevice->CreateShaderFromSpvFile(ShaderStage::STAGE_FRAGEMNT,
        "Assets/Shaders/Spirv/pbr.frag.spv");
    
    // Scene pipeline
    GraphicPipeLineDesc pipe_desc;
    pipe_desc.vertShader = vert_shader;
    pipe_desc.fragShader = frag_shader;
    pipe_desc.blendState = PipelineColorBlendState::create_disabled(1);
    pipe_desc.topologyType = TopologyType::TRANGLE_LIST;
    pipe_desc.renderPassInfo = forward_pass_info;
    pipe_desc.depthStencilState.enableDepthTest = true;
    pipe_desc.depthStencilState.enableDepthWrite = true;
    pipe_desc.depthStencilState.depthCompareOp = CompareOperation::LESS_OR_EQUAL;
    pipe_desc.rasterState.cullMode = CullMode::NONE;
    pipe_desc.rasterState.polygonMode = PolygonMode::Fill;
    pipe_desc.rasterState.frontFaceType = FrontFaceType::COUNTER_CLOCKWISE;
    graphic_pipeline = m_GraphicDevice->CreateGraphicPipeLine(pipe_desc);

    // Sky box pipeline
    pipe_desc.vertShader = skybox_vert_shader;
    pipe_desc.fragShader = skybox_frag_shader;
    pipe_desc.depthStencilState.enableDepthTest = false;
    pipe_desc.depthStencilState.enableDepthWrite = false;

    VertexBindInfo vert_bind_info;
    vert_bind_info.binding = 0;
    vert_bind_info.stride = sizeof(Vertex);
    vert_bind_info.inputRate = VertexBindInfo::INPUT_RATE_VERTEX;
    pipe_desc.vertexBindInfos.push_back(vert_bind_info);

    VertexAttribInfo pos_attrib;
    pos_attrib.binding = 0;
    pos_attrib.format = VertexAttribInfo::ATTRIB_FORMAT_VEC3;
    pos_attrib.location = 0;
    pos_attrib.offset = offsetof(Vertex, position);
    pipe_desc.vertexAttribInfos.push_back(pos_attrib);

    skybox_pipeline = m_GraphicDevice->CreateGraphicPipeLine(pipe_desc);
}   

void EditorApp::CreateColorDepthAttachments()
{
        using namespace quark::graphic;
        auto graphic_device = Application::Get().GetGraphicDevice();

        // Create depth image
        ImageDesc image_desc;
        image_desc.type = ImageType::TYPE_2D;
        image_desc.width = uint32_t(Window::Instance()->GetMonitorWidth() * Window::Instance()->GetRatio());
        image_desc.height = uint32_t(Window::Instance()->GetMonitorHeight() * Window::Instance()->GetRatio());
        image_desc.depth = 1;
        image_desc.format = depth_format;
        image_desc.arraySize = 1;
        image_desc.mipLevels = 1;
        image_desc.initialLayout = ImageLayout::DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
        image_desc.usageBits = IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
        depth_image = graphic_device->CreateImage(image_desc);

        // Create color image
        image_desc.format = color_format;
        image_desc.initialLayout = ImageLayout::UNDEFINED;
        image_desc.usageBits = IMAGE_USAGE_COLOR_ATTACHMENT_BIT | graphic::IMAGE_USAGE_SAMPLING_BIT;
        color_image = graphic_device->CreateImage(image_desc);
}


void EditorApp::SetUpRenderPass()
{   
    // First pass : geometry pass
    forward_pass_info = {};
    forward_pass_info.numColorAttachments = 1;
    forward_pass_info.colorAttatchemtsLoadOp[0] = graphic::RenderPassInfo::AttachmentLoadOp::CLEAR;
    forward_pass_info.colorAttatchemtsStoreOp[0] = graphic::RenderPassInfo::AttachmentStoreOp::STORE;
    forward_pass_info.colorAttachmentFormats[0] = color_format;
    forward_pass_info.depthAttachment = depth_image.get();
    forward_pass_info.depthAttachmentLoadOp = graphic::RenderPassInfo::AttachmentLoadOp::CLEAR;
    forward_pass_info.depthAttachmentStoreOp = graphic::RenderPassInfo::AttachmentStoreOp::STORE;
    forward_pass_info.ClearDepthStencil.depth_stencil = {1.f, 0};
    forward_pass_info.depthAttachmentFormat = depth_format;

    // Second pass : UI pass
    ui_pass_info = {};
    ui_pass_info.numColorAttachments = 1;
    ui_pass_info.colorAttatchemtsLoadOp[0] = graphic::RenderPassInfo::AttachmentLoadOp::CLEAR;
    ui_pass_info.colorAttatchemtsStoreOp[0] = graphic::RenderPassInfo::AttachmentStoreOp::STORE;
    ui_pass_info.colorAttachmentFormats[0] = m_GraphicDevice->GetSwapChainImageFormat();

}

}