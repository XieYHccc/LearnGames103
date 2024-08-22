#pragma once
#include "Quark/Core/Base.h"
#include "Quark/Core/Assert.h"

namespace quark::graphic {

// Macro
#define DESCRIPTOR_SET_MAX_NUM 4
#define SET_BINDINGS_MAX_NUM 16
#define PUSH_CONSTANT_DATA_SIZE 128
#define VERTEX_BUFFER_MAX_NUM 8

// Forward declaraton
class Device;
class CommandList;
class Buffer;
class Image;
class Shader;
class PipeLine;
class Sampler;

struct RenderPassInfo;
struct BufferDesc;
struct ImageDesc;
struct ImageInitData;
struct ComputePipeLineDesc;
struct GraphicPipeLineDesc;
struct SamplerDesc;

enum class GpuResourceType {
    BUFFER,
	IMAGE,
	SHADER,
	PIPELINE,
	SAMPLER,
    COMMAND_LIST,
	MAX_ENUM
};

class GpuResource {
public:
    GpuResource() = default;
    GpuResource(const GpuResource&) = delete;
    virtual ~GpuResource() = default;
    GpuResource& operator=(const GpuResource&) = delete;

    virtual GpuResourceType GetGpuResourceType() const = 0;
};

enum QueueType
{
    QUEUE_TYPE_GRAPHICS,
    QUEUE_TYPE_ASYNC_COMPUTE,
    QUEUE_TYPE_ASYNC_TRANSFER,
    QUEUE_TYPE_MAX_ENUM,
};

// TODO: Support various image format
enum class DataFormat {
    UNDEFINED,
    R8G8B8A8_UNORM,
    B8G8R8A8_UNORM,
    R16G16B16A16_SFLOAT,
    R32G32B32_SFLOAT,
    D32_SFLOAT,
    D32_SFLOAT_S8_UINT,
    D24_UNORM_S8_UINT,

    // Compressed image format
    ETC2_R8G8B8A8_UNORM_BLOCK,
    BC7_UNORM_BLOCK,
    BC3_UNORM_BLOCK,
};

enum class LogicOperation {
    CLEAR,
    AND,
    AND_REVERSE,
    COPY,
    AND_INVERTED,
    NO_OP,
    XOR,
    OR,
    NOR,
    EQUIVALENT,
    INVERT,
    OR_REVERSE,
    COPY_INVERTED,
    OR_INVERTED,
    NAND,
    SET,
    MAX_ENUM
};

enum class CompareOperation {
    NEVER,
    LESS,
    EQUAL,
    LESS_OR_EQUAL,
    GREATER,
    NOT_EQUAL,
    GREATER_OR_EQUAL,
    ALWAYS,
    MAX_ENUM
};

enum class ShaderStage{
    STAGE_COMPUTE,
    STAGE_VERTEX,
    STAGE_FRAGEMNT,
    MAX_ENUM
};

enum class SampleCount {
    SAMPLES_1 = 1,
    SAMPLES_2 = 2,
    SAMPLES_4 = 4,
    SAMPLES_8 = 8,
    SAMPLES_16 = 16,
    SAMPLES_32 = 32,
    SAMPLES_64 = 64,
};

struct Viewport
{
    float x, y, width, height, minDepth, maxDepth;
};

struct Offset
{
    int x, y;
};

struct Extent
{
    unsigned int width, height;
};

struct Scissor
{
    Offset offset;
    Extent extent;
};

inline u32 GetFormatStride(DataFormat format)
{
    switch (format) {
    case DataFormat::R8G8B8A8_UNORM:
    case DataFormat::D24_UNORM_S8_UINT:
    case DataFormat::D32_SFLOAT:
        return 4u;
    case DataFormat::R32G32B32_SFLOAT:
        return 12u;
    case DataFormat::R16G16B16A16_SFLOAT:
        return 8u;
    case DataFormat::BC7_UNORM_BLOCK:
        return 16u;
    default:
    {
        CORE_ASSERT_MSG(0, "format not handled yet!")
        return 0u;
    }
    }
}

inline bool IsFormatSupportDepth (DataFormat format)
{
    switch (format) {
    case DataFormat::D24_UNORM_S8_UINT:
    case DataFormat::D32_SFLOAT_S8_UINT:
    case DataFormat::D32_SFLOAT:
        return true;
    default:
        return false;
    }
}

inline void GetFormatBlockDim(DataFormat format, uint32_t& block_dim_x, uint32_t& block_dim_y)
{
#define fmt(x, w, h)     \
    case DataFormat::x: \
        block_dim_x = w; \
        block_dim_y = h; \
        break
    
    switch (format) {
    fmt(ETC2_R8G8B8A8_UNORM_BLOCK, 4, 4);
    fmt(BC7_UNORM_BLOCK, 4, 4);
    fmt(BC3_UNORM_BLOCK, 4, 4);
    
    // non-block
    default:
        block_dim_x = 1;
        block_dim_y = 1;
        break;
    }
}

}