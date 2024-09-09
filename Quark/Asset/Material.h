#pragma once
#include <glm/glm.hpp>
#include "Quark/Asset/Asset.h"
#include "Quark/Asset/Texture.h"
#include "Quark/Renderer/ShaderManager.h"

namespace quark {
enum class AlphaMode {
    OPAQUE,
    TRANSPARENT
};

struct Material : public Asset {
    struct UniformBufferBlock {
        glm::vec4 baseColorFactor = glm::vec4(1.0f);
        float metalicFactor = 1.f;
        float roughNessFactor = 1.f;
    } uniformBufferData;

    AlphaMode alphaMode = AlphaMode::OPAQUE;

    Ref<Texture> baseColorTexture;
    Ref<Texture> metallicRoughnessTexture;
    Ref<Texture> normalTexture;
    
    ShaderProgram* shaderProgram = nullptr;

    // (deprecated) we are using push constant to send uniform buffer
    Ref<graphic::Buffer> uniformBuffer;
    size_t uniformBufferOffset = 0;

    
};

}