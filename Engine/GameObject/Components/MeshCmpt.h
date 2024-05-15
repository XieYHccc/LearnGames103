#pragma once

#include <memory>

#include "GameObject/Components/Component.h"
#include "Renderer/DrawContext.h"
#include "Renderer/Mesh.h"

class MeshCmpt : public Component {
public:
    std::shared_ptr<asset::Mesh> mesh;
    bool isDirty;

public:
    MeshCmpt(GameObject* owner) : 
        mesh(nullptr),
        isDirty(true),
        Component(owner)
    {

    };

    COMPONENT_TYPE("MeshRenderer");
    
    const std::vector<RenderObject>& GetRenderObjects();


private:
    void GenerateRenderObjects();

    std::vector<RenderObject> renderObjects;
};