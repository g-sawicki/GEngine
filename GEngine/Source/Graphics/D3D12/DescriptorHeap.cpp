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

} // namespace GEngine
