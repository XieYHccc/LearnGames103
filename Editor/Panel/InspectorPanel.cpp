#include "Editor/Panel/InspectorPanel.h"

#include <imgui.h>
#include <imgui_internal.h>
#include <Quark/Asset/AssetManager.h>
#include <Quark/Scene/Components/CameraCmpt.h>
#include <Quark/Scene/Components/MeshCmpt.h>
#include <Quark/Scene/Components/CommonCmpts.h>

namespace quark {
static bool DrawVec3Control(const char* label, glm::vec3& vector, float reset = 0.f, float columnWidth = 100.f)
{
    bool changed = false;

    ImGui::PushID(label);

    ImGui::Columns(2);
    ImGui::SetColumnWidth(0, columnWidth);

    ImGui::Text("%s", label);
    ImGui::NextColumn();

    ImGui::PushMultiItemsWidths(3, ImGui::CalcItemWidth());
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2{ 0,0 });

    float lineheight = GImGui->Font->FontSize + GImGui->Style.FramePadding.y * 2.f;
    ImVec2 buttonsize = { lineheight + 3.f,lineheight };

    auto& BoldFont = ImGui::GetIO().Fonts->Fonts[0];

    ImGui::PushStyleColor(ImGuiCol_Button, { 0.8f,0.1f,0.1f,1.f });
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, { 0.7f,0.1f,0.1f,1.f });
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, { 0.6f,0.1f,0.1f,1.f });
    ImGui::PushFont(BoldFont);
    if (ImGui::Button("X", buttonsize))
    {
        vector.x = reset;
        changed = true;
    }
    ImGui::PopStyleColor(3);
    ImGui::PopFont();

    ImGui::SameLine();
    if (ImGui::DragFloat("##X", &vector.x, 0.1f))
        changed = true;
    ImGui::PopItemWidth();
    ImGui::SameLine();

    ImGui::PushStyleColor(ImGuiCol_Button, { 0.1f,0.8f,0.1f,1.f });
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, { 0.1f,0.7f,0.1f,1.f });
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, { 0.1f,0.6f,0.1f,1.f });
    ImGui::PushFont(BoldFont);
    if (ImGui::Button("Y", buttonsize))
    {
        vector.y = reset;
        changed = true;
    }
    ImGui::PopStyleColor(3);
    ImGui::PopFont();

    ImGui::SameLine();
    if (ImGui::DragFloat("##Y", &vector.y, 0.1f))
        changed = true;
    ImGui::PopItemWidth();
    ImGui::SameLine();

    ImGui::PushStyleColor(ImGuiCol_Button, { 0.1f,0.1f,0.8f,1.f });
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, { 0.1f,0.1f,0.7f,1.f });
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, { 0.1f,0.1f,0.6f,1.f });
    ImGui::PushFont(BoldFont);
    if (ImGui::Button("Z", buttonsize))
    {
        vector.z = reset;
        changed = true;
    }
    ImGui::PopStyleColor(3);
    ImGui::PopFont();

    ImGui::SameLine();
    if (ImGui::DragFloat("##Z", &vector.z, 0.1f))
        changed = true;
    ImGui::PopItemWidth();
    ImGui::SameLine();

    ImGui::PopStyleVar();

    ImGui::Columns(1);

    ImGui::PopID();

    return changed;
}

InspectorPanel::InspectorPanel()
{
    m_SelectedEntity = nullptr;
    m_Scene = nullptr;
    m_Rename = false;
    m_Buf[0] = '\0';
}

template<typename T, typename UIFunction>
static void DrawComponent(const std::string& name, Entity* entity, UIFunction uiFunction)
{
    const ImGuiTreeNodeFlags treeNodeFlags = ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_Framed |
        ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_AllowItemOverlap | ImGuiTreeNodeFlags_FramePadding;

    ImGuiIO& io = ImGui::GetIO();
    if (entity->HasComponent<T>())
    {
        auto& component = *entity->GetComponent<T>();
        ImVec2 contentRegionAvailable = ImGui::GetContentRegionAvail();
        float lineHeight = GImGui->Font->FontSize + GImGui->Style.FramePadding.y * 2.0f;

        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2{ 4, 4 });
        ImGui::Separator();
        bool open = ImGui::TreeNodeEx((void*)T::GetStaticComponentType(), treeNodeFlags, "%s", name.c_str());
        ImGui::PopStyleVar();
        ImGui::SameLine(contentRegionAvailable.x - lineHeight * 0.5f);
        if (ImGui::Button("+", ImVec2{ lineHeight, lineHeight }))
        {
            ImGui::OpenPopup("ComponentSettings");
        }

        bool removeComponent = false;
        if (ImGui::BeginPopup("ComponentSettings"))
        {
            if (ImGui::MenuItem("Remove component"))
                removeComponent = true;

            ImGui::EndPopup();
        }

        if (open)
        {
            uiFunction(component);
            ImGui::TreePop();
        }

        if (removeComponent)
            entity->RemoveComponent<T>();
    }
}

void InspectorPanel::OnImGuiUpdate()
{
    if (ImGui::Begin("InspectorPanel"))
    {
        if (m_SelectedEntity == nullptr) {
            ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "No selected node");
            ImGui::End();
            return;
        }
        
        // Name component
        {   
            auto* nameCmpt = m_SelectedEntity->GetComponent<NameCmpt>();
            char buffer[256];
            strncpy(buffer, nameCmpt->name.c_str(), sizeof(buffer));

            if (ImGui::InputText("##Name", buffer, sizeof(buffer)))
                nameCmpt->name = buffer;
        }

        ImGui::SameLine();
        ImGui::PushItemWidth(-1);

        if (ImGui::Button("Add Component"))
            ImGui::OpenPopup("AddComponent");

        if (ImGui::BeginPopup("AddComponent"))
        {
            if (ImGui::MenuItem("Mesh"))
            {
                if (!m_SelectedEntity->HasComponent<MeshCmpt>())
                    m_SelectedEntity->AddComponent<MeshCmpt>();
                ImGui::CloseCurrentPopup();
            }

            if (ImGui::MenuItem("Camera"))
            {
                if (!m_SelectedEntity->HasComponent<CameraCmpt>())
                    m_SelectedEntity->AddComponent<CameraCmpt>();
                ImGui::CloseCurrentPopup();
            }

            ImGui::EndPopup();
        }
        ImGui::PopItemWidth();
        
        // Transform component
        {
            auto* transformCmpt = m_SelectedEntity->GetComponent<TransformCmpt>();
            if (transformCmpt) {
                glm::vec3 position = transformCmpt->GetPosition();
                glm::vec3 scale = transformCmpt->GetScale();
                glm::vec3 eulerAngles = glm::degrees(glm::eulerAngles(transformCmpt->GetQuat()));

                const ImGuiTreeNodeFlags treeNodeFlags = ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_FramePadding |
                    ImGuiTreeNodeFlags_Framed | ImGuiTreeNodeFlags_AllowItemOverlap | ImGuiTreeNodeFlags_SpanAvailWidth;
                
                if (ImGui::TreeNodeEx((void*)TransformCmpt::GetStaticComponentType(), treeNodeFlags, "Transform"))
                {
                    if (DrawVec3Control("Position", position))
                        transformCmpt->SetPosition(position);

                    if (DrawVec3Control("Rotation", eulerAngles))
                    {
                        if (eulerAngles.x <= 90 && eulerAngles.y <= 90 && eulerAngles.z <= 90
                            && eulerAngles.x >= -90 && eulerAngles.y >= -90 && eulerAngles.z >= -90) {
                            transformCmpt->SetEuler(glm::radians(eulerAngles));
                        }
                        else
                            CORE_LOGW("Rotation values must be less than 90 degrees");
                    }

                    if (DrawVec3Control("Scale", scale))
                        transformCmpt->SetScale(scale);

                    ImGui::TreePop();
                }

            }
        }

        // Mesh component
        DrawComponent<MeshCmpt>("Mesh", m_SelectedEntity, [&](auto& component) {
            bool hasMesh = component.sharedMesh != nullptr;

            Mesh& mesh = *component.sharedMesh;
            std::filesystem::path meshAssetPath = hasMesh? AssetManager::Get().GetAssetMetadata(mesh.GetAssetID()).filePath : 
                std::filesystem::path("None");

            ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x - 125.f);
			ImGui::LabelText("##Name", "%s", meshAssetPath.string().c_str());
			ImGui::PopItemWidth();

			ImGui::SameLine();
			if (ImGui::Button("Set", ImVec2(80.f, 25.f)))
				ImGui::OpenPopup("Set Mesh");

            if (ImGui::BeginPopup("Set Mesh"))
            {
                for (auto id : AssetManager::Get().GetAllAssetsWithType(AssetType::MESH))
				{
					auto metadata = AssetManager::Get().GetAssetMetadata(id);
					if (ImGui::MenuItem(metadata.filePath.string().c_str()))
					{
						component.sharedMesh = AssetManager::Get().GetAsset<Mesh>(id);
						break;
					}
				}
                ImGui::EndPopup();
            }
        });


        // Camera component
        DrawComponent<CameraCmpt>("Camera", m_SelectedEntity, [&](auto& component) {

            if (ImGui::Button("SetMainCamera"))
            {
                if (m_Scene)
					m_Scene->SetMainCameraEntity(m_SelectedEntity);
            }

            float aspect = component.aspect;
            if (ImGui::DragFloat("Aspect", &aspect))
				component.aspect = aspect;

            float perspectiveVerticalFov = component.fov;
            if (ImGui::DragFloat("Vertical FOV", &perspectiveVerticalFov))
                component.fov = perspectiveVerticalFov;

            float perspectiveNear = component.zNear;
            if (ImGui::DragFloat("Near", &perspectiveNear))
                component.zNear = perspectiveNear;

            float perspectiveFar = component.zFar;
            if (ImGui::DragFloat("Far", &perspectiveFar))
                component.zFar = perspectiveFar;
        });
    }
    ImGui::End();
}

}