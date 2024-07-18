#pragma once
#include <Quark/Core/Application.h>
#include <Quark/Scene/Scene.h>
#include <Quark/Renderer/SceneRenderer.h>

class TestBed : public Application  {  
public:
    TestBed(const std::string& title, const std::string& root, int width, int height);
    ~TestBed();

    virtual void Update(f32 deltaTime) override final;
    virtual void Render(f32 deltaTime) override final;

    void CreateDepthImage();
    void CreatePipeline();
    void LoadAsset();
    void SetUpCamera();

    Ref<graphic::Shader> vert_shader;
    Ref<graphic::Shader> frag_shader;
    Ref<graphic::PipeLine> graphic_pipeline;

    Ref<graphic::Image> depth_image;
    graphic::DataFormat depth_format = graphic::DataFormat::D32_SFLOAT;

    Scope<scene::Scene> scene;
    Scope<render::SceneRenderer> scene_renderer;

    // Debug
    float cmdListRecordTime = 0;
};
