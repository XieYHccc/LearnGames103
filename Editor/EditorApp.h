#pragma once
#include <Quark/Core/Application.h>
#include <Quark/Scene/Scene.h>
#include <Quark/Renderer/SceneRenderer.h>
#include <Quark/Events/KeyEvent.h>

#include "Editor/EditorCamera.h"
#include "Editor/UI/SceneHeirarchy.h"
#include "Editor/UI/Inspector.h"
#include "Editor/UI/SceneViewPort.h"

namespace quark {
class EditorApp : public quark::Application  {  
public:
    EditorApp(const quark::AppInitSpecs& specs);
    ~EditorApp();

    void OnUpdate(TimeStep ts) override final;
    void OnRender(TimeStep ts) override final;
    void OnUpdateImGui() override final;
    void UpdateMainMenuUI();

    void OnKeyPressed(KeyPressedEvent& e);

    void NewScene();
    void OpenScene();
    void SaveSceneAs();

    void CreateColorDepthAttachments();
    void CreatePipeline();
    void SetUpRenderPass();
    void LoadScene();

    quark::Ref<quark::graphic::Shader> vert_shader;
    quark::Ref<quark::graphic::Shader> frag_shader;
    quark::Ref<quark::graphic::Shader> skybox_vert_shader;
    quark::Ref<quark::graphic::Shader> skybox_frag_shader;
    
    quark::Ref<quark::graphic::PipeLine> graphic_pipeline;
    quark::Ref<quark::graphic::PipeLine> skybox_pipeline;

    quark::graphic::RenderPassInfo forward_pass_info; // First pass
    quark::graphic::RenderPassInfo ui_pass_info;   // Second pass

    quark::Ref<quark::graphic::Image> cubeMap_image;
    quark::Ref<quark::graphic::Image> depth_image;
    quark::Ref<quark::graphic::Image> color_image;
    quark::graphic::DataFormat depth_format = quark::graphic::DataFormat::D32_SFLOAT;
    quark::graphic::DataFormat color_format; // Same with swapchain format
    
    quark::Scope<quark::Scene> m_Scene;
    quark::Scope<quark::SceneRenderer> m_SceneRenderer;
    Entity* m_EditorCameraEntity = nullptr;
    EditorCamera m_EditorCamera;

    // UI window
    SceneHeirarchy m_HeirarchyPanel;
    Inspector m_InspectorPanel;
    SceneViewPort m_SceneViewPort;
    
    // Debug
    float cmdListRecordTime = 0;
};

}