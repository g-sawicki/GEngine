#include "PCH.hpp"

#include "DescriptorHeap.hpp"
#include "Device.hpp"

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
    if (m_CurrentIndex >= m_NumDescriptors)
        throw std::out_of_range("DescriptorHeap exhausted: no more descriptors available.");
    return m_CurrentIndex++;
}

DescriptorHandle DescriptorHeap::Allocate() {
    const UINT index = AllocateIndex();
    const CD3DX12_CPU_DESCRIPTOR_HANDLE cpuHandle{m_DescriptorHeap->GetCPUDescriptorHandleForHeapStart(),
                                                  static_cast<INT>(index), m_DescriptorSize};
    return {cpuHandle, index};
}

DescriptorRange DescriptorHeap::AllocateRange(UINT count) {
    if (count == 0)
        throw std::invalid_argument("DescriptorHeap::AllocateRange: count must be non-zero.");
    if (m_CurrentIndex + count > m_NumDescriptors)
        throw std::out_of_range("DescriptorHeap exhausted: no more descriptors available.");

    const UINT index = m_CurrentIndex;
    m_CurrentIndex += count;
    const CD3DX12_CPU_DESCRIPTOR_HANDLE cpuHandle{m_DescriptorHeap->GetCPUDescriptorHandleForHeapStart(),
                                                  static_cast<INT>(index), m_DescriptorSize};
    return {.Base = cpuHandle, .Stride = m_DescriptorSize, .Count = count};
}

} // namespace GEngine
