#pragma once
#include "Core/Base.h"
#include "Graphic/Common.h"

namespace graphic {

enum class BufferType {
    STORAGE_BUFFER,
    VERTEX_BUFFER,
    INDEX_BUFFER,
    UNIFORM_BUFFER
};

enum class IndexBufferFormat{
    UINT16,
    UINT32
};

enum class BufferMemoryDomain {
    GPU,
    CPU,
};

struct BufferDesc {
    u64 size = 0;
    BufferMemoryDomain domain = BufferMemoryDomain::GPU;
    BufferType type = BufferType::STORAGE_BUFFER;
};


class Buffer : public GpuResource{
public:
    ~Buffer() = default;
    
    const BufferDesc& GetDesc() const { return desc_;}
    void* GetMappedDataPtr() {return pMappedData_;}
    u64 GetGpuAddress() { return gpuAddress_;}

protected:
    Buffer(const BufferDesc& desc) : desc_(desc) {}
    BufferDesc desc_;
    void* pMappedData_;
    u64 gpuAddress_ = UINT64_MAX;
};

}