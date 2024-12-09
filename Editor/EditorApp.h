#pragma once
#include <Quark/Core/Application.h>
#include <Quark/Core/FileSystem.h>
#include <Quark/Scene/Scene.h>
#include <Quark/Render/Renderer.h>
#include <Quark/Events/KeyEvent.h>
#include <Quark/Events/MouseEvent.h>

#include "Editor/EditorCamera.h"
#include "Editor/Panel/SceneHeirarchyPanel.h"
#include "Editor/Panel/InspectorPanel.h"
#include "Editor/Panel/ContentBrowserPanel.h"

namespace quark {
class EditorApp : public quark::Application {
public:
    EditorApp(const ApplicationSpecification& specs);
    ~EditorApp();

    void OnUpdate(TimeStep ts) override final;
    void OnRender(TimeStep ts) override final;
    void OnImGuiUpdate() override final;

    void OnKeyPressed(const KeyPressedEvent& e);
    void OnMouseButtonPressed(const MouseButtonPressedEvent& e);

    void NewScene();
    void OpenScene();
    void OpenScene(const std::filesystem::path& path);
    void SaveSceneAs();

    bool OpenProject(const std::filesystem::path& path);
    bool OpenProject();

private:
    void CreateGraphicResources();
    void MainPass(Renderer::DrawContext& context, Renderer::Visibility& vis, rhi::CommandList* cmd);

    Ref<rhi::Image> m_depth_attachment;
    Ref<rhi::Image> m_color_attachment;
    Ref<rhi::Image> m_entityID_color_attachment;
    Ref<rhi::Image> m_entityID_depth_attachment;

    Ref<rhi::Buffer> m_stage_buffer;

    Renderer::DrawContext m_drawContext;
    Renderer::Visibility m_visibility;

    Ref<Texture> m_cubeMapTexture;
    Ref<Scene> m_scene;
    Entity* m_hoverdEntity;
    EditorCamera m_editorCamera;

    glm::vec2 m_viewportSize;
    glm::vec2 m_viewportBounds[2];

    bool m_viewportFocused;
    bool m_viewportHovered;
    int m_gizmoType = -1;

    // UI window
    SceneHeirarchyPanel m_heirarchyPanel;
    InspectorPanel m_inspectorPanel;
    ContentBrowserPanel m_contentBrowserPanel;
    
    // Debug
    double m_cmdListRecordTime = 0;
};

}