#include "PCH.hpp"

#include "Buffer.hpp"

#include "D3D12Common.hpp"

namespace GEngine {

Buffer::Buffer(Device& device, UINT64 size, const void* initialData) {
    const D3D12_HEAP_PROPERTIES heapProps{.Type = D3D12_HEAP_TYPE_UPLOAD};
    const D3D12_RESOURCE_DESC desc{
        .Dimension = D3D12_RESOURCE_DIMENSION_BUFFER,
        .Width = size,
        .Height = 1,
        .DepthOrArraySize = 1,
        .MipLevels = 1,
        .SampleDesc = {.Count = 1},
        .Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR,
    };

    ThrowIfFailed(device.Get()->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &desc,
                                                        D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,
                                                        IID_PPV_ARGS(&m_Resource)));

    m_Size = size;
    if (initialData)
        Write(initialData, size);
}

Buffer::Buffer(Device& device, const D3D12_HEAP_PROPERTIES& heapProps, const D3D12_RESOURCE_DESC& desc) {
    ThrowIfFailed(device.Get()->CreateCommittedResource(
        &heapProps, D3D12_HEAP_FLAG_NONE, &desc, D3D12_RESOURCE_STATE_COMMON, nullptr, IID_PPV_ARGS(&m_Resource)));

    m_Size = desc.Width;
}

void Buffer::Write(const void* data, const UINT64 size) {
    assert(size <= m_Size);
    void* mapped{};
    m_Resource->Map(0, nullptr, &mapped);
    std::memcpy(mapped, data, size);
    m_Resource->Unmap(0, nullptr);
}

} // namespace GEngine
