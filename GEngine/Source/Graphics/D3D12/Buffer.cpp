#include "PCH.hpp"

#include "Buffer.hpp"

#include "Common.hpp"

namespace GEngine {

static void UploadData(ID3D12Resource* resource, const void* data, UINT64 size) {
    if (!data)
        return;
    void* mapped{};
    resource->Map(0, nullptr, &mapped);
    std::memcpy(mapped, data, size);
    resource->Unmap(0, nullptr);
}

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

    UploadData(m_Resource.Get(), initialData, size);
    m_Size = size;
}

Buffer::Buffer(Device& device, const D3D12_HEAP_PROPERTIES& heapProps, const D3D12_RESOURCE_DESC& desc,
               const void* initialData) {
    ThrowIfFailed(device.Get()->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &desc,
                                                        D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,
                                                        IID_PPV_ARGS(&m_Resource)));

    UploadData(m_Resource.Get(), initialData, desc.Width);
    m_Size = desc.Width;
}

} // namespace GEngine
