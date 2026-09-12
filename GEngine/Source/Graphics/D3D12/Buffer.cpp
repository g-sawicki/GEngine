#include "PCH.hpp"

#include "Buffer.hpp"

#include "Core/Utility/Math.hpp"
#include "D3D12Common.hpp"

namespace GEngine {

Buffer::Buffer(Device& device, const BufferDesc& desc) : m_BufferDesc(desc) {
    if (HasFlag(desc.MiscFlags, BufferMiscFlags::ConstantBuffer))
        m_BufferDesc.Size = RoundUp<D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT>(desc.Size);

    const D3D12_RESOURCE_DESC resourceDesc{
        .Dimension = D3D12_RESOURCE_DIMENSION_BUFFER,
        .Alignment = 0,
        .Width = m_BufferDesc.Size,
        .Height = 1,
        .DepthOrArraySize = 1,
        .MipLevels = 1,
        .SampleDesc = {.Count = 1, .Quality = 0},
        .Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR,
        .Flags = desc.Flags,
    };

    D3D12_RESOURCE_STATES initialState{};
    switch (desc.HeapType) {
    case D3D12_HEAP_TYPE_UPLOAD:
        initialState = D3D12_RESOURCE_STATE_GENERIC_READ;
        break;
    case D3D12_HEAP_TYPE_READBACK:
        initialState = D3D12_RESOURCE_STATE_COPY_DEST;
        break;
    default:
        initialState = D3D12_RESOURCE_STATE_COMMON;
        break;
    }

    const D3D12_HEAP_PROPERTIES heapProps{.Type = desc.HeapType};
    ThrowIfFailed(device.Get()->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &resourceDesc, initialState,
                                                        nullptr, IID_PPV_ARGS(&m_Resource)));
}

void* Buffer::Map(UINT subresource, const D3D12_RANGE* readRange) const {
    assert(m_Resource != nullptr);
    void* pData{nullptr};
    ThrowIfFailed(m_Resource->Map(subresource, readRange, &pData));
    return pData;
}

void Buffer::Unmap(UINT subresource, const D3D12_RANGE* writtenRange) const {
    assert(m_Resource != nullptr);
    m_Resource->Unmap(subresource, writtenRange);
}

void Buffer::CreateStructuredBufferSRV(Device& device, const UINT numElements, const UINT strideInBytes) {
    assert(m_SrvIndex == INVALID_BINDLESS_INDEX);

    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{
        .Format = DXGI_FORMAT_UNKNOWN,
        .ViewDimension = D3D12_SRV_DIMENSION_BUFFER,
        .Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING,
        .Buffer =
            {
                .FirstElement = 0,
                .NumElements = numElements,
                .StructureByteStride = strideInBytes,
                .Flags = D3D12_BUFFER_SRV_FLAG_NONE,
            },
    };

    m_SrvIndex = device.GetShaderResourceDescriptorHeap().Allocate().Index;
    const D3D12_CPU_DESCRIPTOR_HANDLE srvHandle = device.GetShaderResourceDescriptorHeap().GetCpuHandle(m_SrvIndex);
    device.Get()->CreateShaderResourceView(m_Resource.Get(), &srvDesc, srvHandle);
}

void Buffer::CreateStructuredBufferUAV(Device& device, const UINT numElements, const UINT strideInBytes) {
    assert(m_UavIndex == INVALID_BINDLESS_INDEX);
    assert((m_BufferDesc.Flags & D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS) != 0);

    D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc{
        .Format = DXGI_FORMAT_UNKNOWN,
        .ViewDimension = D3D12_UAV_DIMENSION_BUFFER,
        .Buffer =
            {
                .FirstElement = 0,
                .NumElements = numElements,
                .StructureByteStride = strideInBytes,
                .CounterOffsetInBytes = 0,
                .Flags = D3D12_BUFFER_UAV_FLAG_NONE,
            },
    };

    m_UavIndex = device.GetShaderResourceDescriptorHeap().Allocate().Index;
    const D3D12_CPU_DESCRIPTOR_HANDLE uavHandle = device.GetShaderResourceDescriptorHeap().GetCpuHandle(m_UavIndex);
    device.Get()->CreateUnorderedAccessView(m_Resource.Get(), nullptr, &uavDesc, uavHandle);
}

[[nodiscard]] D3D12_VERTEX_BUFFER_VIEW Buffer::GetVBV(UINT stride) const noexcept {
    return {.BufferLocation = m_Resource->GetGPUVirtualAddress(),
            .SizeInBytes = static_cast<UINT>(m_BufferDesc.Size),
            .StrideInBytes = stride};
}

[[nodiscard]] D3D12_INDEX_BUFFER_VIEW Buffer::GetIBV(DXGI_FORMAT format) const noexcept {
    return {.BufferLocation = m_Resource->GetGPUVirtualAddress(),
            .SizeInBytes = static_cast<UINT>(m_BufferDesc.Size),
            .Format = format};
}

} // namespace GEngine
