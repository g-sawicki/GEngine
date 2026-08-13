#include "PCH.hpp"

#include "DescriptorHeap.hpp"

#include "D3D12Common.hpp"

namespace GEngine {

DescriptorHeap::DescriptorHeap(Device& device, D3D12_DESCRIPTOR_HEAP_TYPE type, UINT numDescriptors,
                               D3D12_DESCRIPTOR_HEAP_FLAGS flags)
    : m_Type(type), m_Flags(flags), m_NumDescriptors(numDescriptors) {
    D3D12_DESCRIPTOR_HEAP_DESC desc{
        .Type = type,
        .NumDescriptors = numDescriptors,
        .Flags = flags,
    };

    ThrowIfFailed(device.Get()->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&m_DescriptorHeap)));
    m_DescriptorSize = device.Get()->GetDescriptorHandleIncrementSize(type);
}

UINT DescriptorHeap::AllocateIndex() {
    if (m_CurrentIndex >= m_NumDescriptors) {
        assert(!"DescriptorHeap exhausted: no more descriptors available.");
        return UINT32_MAX;
    }
    return m_CurrentIndex++;
}

DescriptorHandle DescriptorHeap::Allocate() {
    const UINT index = AllocateIndex();
    if (index == UINT32_MAX)
        return {};

    const CD3DX12_CPU_DESCRIPTOR_HANDLE cpuHandle{m_DescriptorHeap->GetCPUDescriptorHandleForHeapStart(),
                                                  static_cast<INT>(index), m_DescriptorSize};

    // GPU handles only exist for shader-visible heaps.
    CD3DX12_GPU_DESCRIPTOR_HANDLE gpuHandle{};
    if (IsShaderVisible()) {
        gpuHandle = CD3DX12_GPU_DESCRIPTOR_HANDLE{m_DescriptorHeap->GetGPUDescriptorHandleForHeapStart(),
                                                  static_cast<INT>(index), m_DescriptorSize};
    }
    return {cpuHandle, gpuHandle};
}

} // namespace GEngine
