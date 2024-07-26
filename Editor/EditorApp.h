#pragma once
#include <Quark/Core/Application.h>
#include <Quark/Scene/Scene.h>
#include <Quark/Renderer/SceneRenderer.h>
#include "Editor/UI/SceneHeirarchy.h"
#include "Editor/UI/Inspector.h"
#include "Editor/UI/SceneViewPort.h"

namespace editor {

class EditorApp : public Application  {  
public:
    EditorApp(const AppInitSpecs& specs);
    ~EditorApp();

    void Update(f32 deltaTime) override final;
    void Render(f32 deltaTime) override final;
    void UpdateUI() override final;
    
    void CreateColorDepthAttachments();
    void CreatePipeline();
    void SetUpRenderPass();
    void LoadScene();
    void UpdateMainMenuUI();

    Ref<graphic::Shader> vert_shader;
    Ref<graphic::Shader> frag_shader;
    Ref<graphic::Shader> skybox_vert_shader;
    Ref<graphic::Shader> skybox_frag_shader;
    
    Ref<graphic::PipeLine> graphic_pipeline;
    Ref<graphic::PipeLine> skybox_pipeline;

    graphic::RenderPassInfo forward_pass_info; // First pass
    graphic::RenderPassInfo ui_pass_info;   // Second pass

    Ref<graphic::Image> cubeMap_image;
    Ref<graphic::Image> depth_image;
    Ref<graphic::Image> color_image;
    graphic::DataFormat depth_format = graphic::DataFormat::D32_SFLOAT;
    graphic::DataFormat color_format; // Same with swapchain format
    
    Scope<scene::Scene> scene_;
    Scope<render::SceneRenderer> scene_renderer_;

    // UI window
    ui::SceneHeirarchy heirarchyWindow_;
    ui::Inspector inspector_;
    ui::SceneViewPort sceneViewPort_;
    
    // Debug
    float cmdListRecordTime = 0;
};

}
