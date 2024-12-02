#pragma once
#include "Quark/Core/Math/Aabb.h"
#include "Quark/Core/Util/EnumCast.h"
#include "Quark/Asset/Material.h"

#include <glm/glm.hpp>

namespace quark {

enum class MeshAttribute : unsigned
{
    POSITION = 0,
    UV = 1,
    NORMAL = 2,
    TANGENT = 3,
    BONE_INDEX = 4,
    BONE_WIGHTS = 5,
    VERTEX_COLOR = 6,
    MAX_ENUM,
    NONE
};

enum MeshAttributeFlagBits
{
    MESH_ATTRIBUTE_POSITION_BIT = 1u << util::ecast(MeshAttribute::POSITION),
    MESH_ATTRIBUTE_UV_BIT = 1u << util::ecast(MeshAttribute::UV),
    MESH_ATTRIBUTE_NORMAL_BIT = 1u << util::ecast(MeshAttribute::NORMAL),
    MESH_ATTRIBUTE_TANGENT_BIT = 1u << util::ecast(MeshAttribute::TANGENT),
    MESH_ATTRIBUTE_BONE_INDEX_BIT = 1u << util::ecast(MeshAttribute::BONE_INDEX),
    MESH_ATTRIBUTE_BONE_WEIGHTS_BIT = 1u << util::ecast(MeshAttribute::BONE_WIGHTS),
    MESH_ATTRIBUTE_VERTEX_COLOR_BIT = 1u << util::ecast(MeshAttribute::VERTEX_COLOR)
};

class Mesh : public Asset {
public:
    struct SubMeshDescriptor 
    {
        uint32_t startVertex = 0;
        uint32_t startIndex = 0; // This is not relative to the startVertex
        uint32_t count = 0;
        math::Aabb aabb = {};
    };
    std::vector<SubMeshDescriptor> subMeshes;

    std::vector<uint32_t> indices;
    std::vector<glm::vec3> vertex_positions;
    std::vector<glm::vec2> vertex_uvs;
    std::vector<glm::vec3> vertex_normals;
    std::vector<glm::vec4> vertex_colors;

    math::Aabb aabb = {};

public:
    Mesh() = default;

    uint32_t GetMeshAttributeMask() const;
    size_t GetVertexCount() const { return vertex_positions.size(); }
    size_t GetPositionBufferStride() const;
    size_t GetAttributeBufferStride() const;

    void SetDynamic(bool isDynamic) { this->m_isDynamic = isDynamic; }
    bool IsDynamic() const { return m_isDynamic; }

    void CalculateAabbs();
    void CalculateNormals();

private:
    // Called from renderer
    Ref<rhi::Buffer> GetAttributeBuffer();
    Ref<rhi::Buffer> GetPositionBuffer();
    Ref<rhi::Buffer> GetIndexBuffer();

    void UpdateGpuBuffers();
    bool IsVertexDataArraysValid() const;

private:
    // Gpu resources
    Ref<rhi::Buffer> m_positionBuffer;
    Ref<rhi::Buffer> m_attributeBuffer;
    Ref<rhi::Buffer> m_indexBuffer;

    // Overlapped vertex data
    std::vector<uint8_t> m_cachedAttributeData;

    bool m_isDynamic = false;

    friend class Renderer;

};
}