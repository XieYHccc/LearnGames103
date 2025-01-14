#include "Quark/qkpch.h"
#include "Quark/Scene/Scene.h"
#include "Quark/Scene/Components/CommonCmpts.h"
#include "Quark/Scene/Components/TransformCmpt.h"
#include "Quark/Scene/Components/MeshCmpt.h"
#include "Quark/Scene/Components/MeshRendererCmpt.h"
#include "Quark/Scene/Components/RelationshipCmpt.h"
#include "Quark/Scene/Components/CameraCmpt.h"
#include "Quark/Render/RenderSystem.h"

namespace quark {

Scene::Scene(const std::string& name)
    : m_SceneName(name), m_MainCameraEntity(nullptr)
{
}

Scene::~Scene()
{   

}


void Scene::DeleteEntity(Entity* entity)
{
    // Remove from parent
    auto* relationshipCmpt = entity->GetComponent<RelationshipCmpt>();
    if (relationshipCmpt->GetParentEntity())
    {
        auto* parentRelationshipCmpt = relationshipCmpt->GetParentEntity()->GetComponent<RelationshipCmpt>();
        parentRelationshipCmpt->RemoveChildEntity(entity);
    }

    // Iteratively delete children
    std::vector<Entity*> children = relationshipCmpt->GetChildEntities();
    for (auto* c: children)
        DeleteEntity(c);

    // Delete entity
    m_Registry.DeleteEntity(entity);
}

void Scene::AttachChild(Entity* child, Entity* parent)
{
    auto* childRelationshipCmpt = child->GetComponent<RelationshipCmpt>();
	auto* parentRelationshipCmpt = parent->GetComponent<RelationshipCmpt>();

	if (childRelationshipCmpt->GetParentEntity())
	{
		auto* oldParentRelationshipCmpt = childRelationshipCmpt->GetParentEntity()->GetComponent<RelationshipCmpt>();
		oldParentRelationshipCmpt->RemoveChildEntity(child);
	}

	parentRelationshipCmpt->AddChildEntity(child);
}

void Scene::DetachChild(Entity* child)
{
    auto* relationshipCmpt = child->GetComponent<RelationshipCmpt>();
    if (relationshipCmpt->GetParentEntity())
	{
		auto* parentRelationshipCmpt = relationshipCmpt->GetParentEntity()->GetComponent<RelationshipCmpt>();
		parentRelationshipCmpt->RemoveChildEntity(child);
	}
}

Entity* Scene::CreateEntity(const std::string& name, Entity* parent)
{
    return CreateEntityWithID({}, name, parent);
}

Entity* Scene::CreateEntityWithID(UUID id, const std::string& name, Entity* parent)
{
    // Create entity
    Entity* newEntity = m_Registry.CreateEntity();

    auto* idCmpt = newEntity->AddComponent<IdCmpt>();
    idCmpt->id = id;

    auto* relationshipCmpt = newEntity->AddComponent<RelationshipCmpt>();

    newEntity->AddComponent<TransformCmpt>();
    if (!name.empty()) 
        newEntity->AddComponent<NameCmpt>(name);

    if (parent != nullptr)
    {
        auto* parentRelationshipCmpt = parent->GetComponent<RelationshipCmpt>();
        parentRelationshipCmpt->AddChildEntity(newEntity);
    }

    QK_CORE_ASSERT(m_EntityIdMap.find(id) == m_EntityIdMap.end())
    m_EntityIdMap[id] = newEntity;

    return newEntity;
}

Entity* Scene::GetEntityWithID(UUID id)
{
    auto find = m_EntityIdMap.find(id);
    if (find != m_EntityIdMap.end())
        return find->second;
    else
        return nullptr;
}

Entity* Scene::GetMainCameraEntity()
{
    if (m_MainCameraEntity)
    {
        return m_MainCameraEntity;
    }
    else 
    {
        return nullptr;
    }
}

void Scene::OnUpdate()
{
    RunTransformUpdateSystem();
}

void Scene::RunTransformUpdateSystem()
{
    auto& groupVector = GetComponents<TransformCmpt>();

    for (auto& group : groupVector)
    {
        TransformCmpt* t = GetComponent<TransformCmpt>(group);
        bool dirty = false;
        if (t->IsParentDirty())
        {
            t->UpdateWorldMatrix_Parent();
            t->SetParentDirty(false);
            t->SetDirty(false);
            dirty = true;
        }
        else if (t->IsDirty())
        {
            t->UpdateWorldMatrix();
            t->SetDirty(false);
            dirty = true;
        }
        
        // mark render state dirty
        if (dirty) 
        {
            auto* renderCmpt = t->GetEntity()->GetComponent<MeshRendererCmpt>();
            if (renderCmpt)
                renderCmpt->SetDirty(true);
        }
    }
}

void Scene::FillMeshSwapData()
{
    auto& swapData = RenderSystem::Get().GetSwapContext().GetLogicSwapData();

    const auto& cmpts = GetComponents<IdCmpt, MeshCmpt, MeshRendererCmpt, TransformCmpt>();
    for (const auto [id_cmpt, mesh_cmpt, mesh_renderer_cmpt, transform_cmpt] : cmpts)
    {
        if (!mesh_renderer_cmpt->IsRenderStateDirty())
            continue;
        
        mesh_renderer_cmpt->SetDirty(false);

        auto* mesh = mesh_cmpt->uniqueMesh ? mesh_cmpt->uniqueMesh.get() : mesh_cmpt->sharedMesh.get();
        if (!mesh) 
            continue;
        
        StaticMeshRenderProxy newRenderProxy;
        newRenderProxy.entity_id = id_cmpt->id;
        newRenderProxy.mesh_asset_id = mesh->GetAssetID();
        newRenderProxy.transform = transform_cmpt->GetWorldMatrix();

        for (uint32_t i = 0; i < mesh->subMeshes.size(); ++i) {
            const auto& submesh = mesh->subMeshes[i];
            MeshSectionDesc newSectionDesc;
            
            newSectionDesc.aabb = submesh.aabb;
            newSectionDesc.index_count = submesh.count;
            newSectionDesc.index_offset = submesh.startIndex;
            newSectionDesc.material_asset_id = mesh_renderer_cmpt->GetMaterialID(i);
            newRenderProxy.mesh_sections.push_back(newSectionDesc);
        }

        swapData.dirty_static_mesh_render_proxies.push_back(newRenderProxy);
    }

}

void Scene::FillCameraSwapData()
{
    auto& swapData = RenderSystem::Get().GetSwapContext().GetLogicSwapData();

    auto* mainCameraEntity = GetMainCameraEntity();
    QK_CORE_VERIFY(mainCameraEntity)
    auto* cameraCmpt = mainCameraEntity->GetComponent<CameraCmpt>();

    CameraSwapData cameraSwapData;
    cameraSwapData.view = cameraCmpt->GetViewMatrix();
    cameraSwapData.proj = cameraCmpt->GetProjectionMatrix();
    swapData.camera_swap_data = cameraSwapData;
}
}