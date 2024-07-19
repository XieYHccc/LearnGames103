#pragma once
#include <Quark/Core/Application.h>
#include <Quark/Scene/Scene.h>
#include <Quark/Renderer/SceneRenderer.h>

class EditorApp : public Application  {  
public:
    EditorApp(const std::string& title, const std::string& root, int width, int height);
    ~EditorApp();

    virtual void Update(f32 deltaTime) override final;
    virtual void Render(f32 deltaTime) override final;
    virtual void UpdateUI() override final;
    
    void CreateDepthImage();
    void CreatePipeline();
    void SetUpRenderPass();
    void LoadAsset();
    void SetUpCamera();

    Ref<graphic::Shader> vert_shader;
    Ref<graphic::Shader> frag_shader;
    Ref<graphic::PipeLine> graphic_pipeline;

    graphic::RenderPassInfo geometry_pass_info; // First pass
    graphic::RenderPassInfo ui_pass_info;   // Second pass

    Ref<graphic::Image> depth_image;
    graphic::DataFormat depth_format = graphic::DataFormat::D32_SFLOAT;

    Scope<scene::Scene> scene;
    Scope<render::SceneRenderer> scene_renderer;

    // Debug
    float cmdListRecordTime = 0;
};
