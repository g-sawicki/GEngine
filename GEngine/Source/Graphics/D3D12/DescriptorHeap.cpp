#include "PCH.hpp"

#include "DescriptorHeap.hpp"

#include "D3D12Common.hpp"
#include "Device.hpp"

namespace GEngine {

DescriptorHeap::DescriptorHeap(Device& device, D3D12_DESCRIPTOR_HEAP_DESC desc)
    : m_Desc(desc), m_Allocator(desc.NumDescriptors) {
    ThrowIfFailed(device.Get()->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&m_DescriptorHeap)));
    m_DescriptorSize = device.Get()->GetDescriptorHandleIncrementSize(desc.Type);
}

D3D12_CPU_DESCRIPTOR_HANDLE DescriptorHeap::GetCpuHandle(UINT index) const noexcept {
    return {m_DescriptorHeap->GetCPUDescriptorHandleForHeapStart().ptr +
            static_cast<SIZE_T>(index) * m_DescriptorSize};
}

D3D12_GPU_DESCRIPTOR_HANDLE DescriptorHeap::GetGpuHandle(UINT index) const noexcept {
    return {m_DescriptorHeap->GetGPUDescriptorHandleForHeapStart().ptr +
            static_cast<UINT64>(index) * m_DescriptorSize};
}

uint32_t DescriptorHeap::GetIndex(D3D12_CPU_DESCRIPTOR_HANDLE handle) const noexcept {
    const SIZE_T heapStart = m_DescriptorHeap->GetCPUDescriptorHandleForHeapStart().ptr;
    assert(handle.ptr >= heapStart);
    const SIZE_T offset = handle.ptr - heapStart;
    assert(m_DescriptorSize != 0 && offset % m_DescriptorSize == 0);
    const auto index = static_cast<uint32_t>(offset / m_DescriptorSize);
    assert(index < m_Desc.NumDescriptors);
    return index;
}

} // namespace GEngine
