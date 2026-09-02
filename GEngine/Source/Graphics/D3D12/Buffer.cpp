#include "PCH.hpp"

#include "Buffer.hpp"

#include "CommandList.hpp"
#include "CommandQueue.hpp"
#include "Core/Utility/Math.hpp"
#include "D3D12Common.hpp"
#include "Fence.hpp"

namespace GEngine {

Buffer::Buffer(Device& device, CommandQueue& commandQueue, const BufferDesc& desc, const void* initialData)
    : m_Size(desc.Size), m_HeapType(desc.HeapType) {
    if (HasFlag(desc.MiscFlags, BufferMiscFlags::ConstantBuffer)) {
        m_Size = RoundUp<D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT>(m_Size);
    }
    const D3D12_RESOURCE_DESC resourceDesc{
        .Dimension = D3D12_RESOURCE_DIMENSION_BUFFER,
        .Width = m_Size,
        .Height = 1,
        .DepthOrArraySize = 1,
        .MipLevels = 1,
        .SampleDesc = {.Count = 1},
        .Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR,
        .Flags = desc.Flags,
    };
    const D3D12_HEAP_PROPERTIES heapProps{.Type = desc.HeapType};

    const bool needsUpload = initialData && desc.HeapType == D3D12_HEAP_TYPE_DEFAULT;
    const D3D12_RESOURCE_STATES initialState =
        needsUpload ? D3D12_RESOURCE_STATE_COMMON : D3D12_RESOURCE_STATE_GENERIC_READ;

    ThrowIfFailed(device.Get()->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &resourceDesc, initialState,
                                                        nullptr, IID_PPV_ARGS(&m_Resource)));

    if (!initialData)
        return;

    if (desc.HeapType == D3D12_HEAP_TYPE_UPLOAD) {
        Write(initialData, desc.Size);
        return;
    }

    if (desc.HeapType != D3D12_HEAP_TYPE_DEFAULT)
        return;

    const BufferDesc uploadDesc{.Size = m_Size, .HeapType = D3D12_HEAP_TYPE_UPLOAD};
    Buffer staging{device, commandQueue, uploadDesc, initialData};

    Microsoft::WRL::ComPtr<ID3D12CommandAllocator> uploadAllocator;
    ThrowIfFailed(device.Get()->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&uploadAllocator)));

    CommandList uploadCmdList{device, uploadAllocator.Get(), D3D12_COMMAND_LIST_TYPE_DIRECT};
    uploadCmdList.Reset(uploadAllocator.Get());

    auto* cmdList = uploadCmdList.GetHandle();
    cmdList->CopyBufferRegion(m_Resource.Get(), 0, staging.Get(), 0, m_Size);

    const CD3DX12_RESOURCE_BARRIER barrier{CD3DX12_RESOURCE_BARRIER::Transition(
        m_Resource.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_GENERIC_READ)};
    cmdList->ResourceBarrier(1, &barrier);

    uploadCmdList.Close();

    ID3D12CommandList* lists[]{uploadCmdList.GetHandle()};
    commandQueue.GetHandle()->ExecuteCommandLists(1, lists);

    Fence uploadFence{device};
    uploadFence.Flush(commandQueue.GetHandle());
}

void Buffer::Write(const void* data, const UINT64 size) {
    assert(m_HeapType == D3D12_HEAP_TYPE_UPLOAD);
    assert(size <= m_Size);
    void* mapped{};
    m_Resource->Map(0, nullptr, &mapped);
    std::memcpy(mapped, data, size);
    m_Resource->Unmap(0, nullptr);
}

void Buffer::CreateStructuredBufferSRV(Device& device, const UINT numElements, const UINT strideInBytes) {
    assert(m_SrvIndex == INVALID_BINDLESS_INDEX);
    m_SrvIndex = device.GetShaderResourceDescriptorHeap().Allocate().Index;

    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srvDesc.Buffer = {
        .FirstElement = 0,
        .NumElements = numElements,
        .StructureByteStride = strideInBytes,
        .Flags = D3D12_BUFFER_SRV_FLAG_NONE,
    };
    const D3D12_CPU_DESCRIPTOR_HANDLE srvHandle = device.GetShaderResourceDescriptorHeap().GetCpuHandle(m_SrvIndex);
    device.Get()->CreateShaderResourceView(m_Resource.Get(), &srvDesc, srvHandle);
}

} // namespace GEngine
